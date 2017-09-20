/*******************************************************************
**                                                                **
** Device Hardware Implementation File                            **
** This file must be included once in the project main file  	  **
**                                                                **
*******************************************************************/
/** \addtogroup HAL Hardware Abstraction Layer
 * @brief set of functions that abstract the hardware used for code portability
 *  @{
 */
#ifndef __DEVICE_IMPL_H__
#define __DEVICE_IMPL_H__
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include "device_def.h"
/* Hardware Init */

#include <mmi_adc.h>
#include <flash.h>
#include <crc16.h>
#include <string.h>

#include "commissioning.h"

#ifndef APP_ID
#error "HAL definition file not included"
#endif

/**
 * @brief Hardware Abstraction Layer Ports Definition
 * This array of SystemPort types describes all physical connections for all software functions
 * as an abstracted name used throughout the whole application.
 */
extern const SystemPort HAL[];
/** @cond */
#ifndef HAL_USE_HFXO
#define HAL_USE_HFXO	0
#endif
#ifndef HAL_USE_LFXO
#define HAL_USE_LFXO	0
#endif
#ifndef HAL_CPU_SPEED
#define HAL_CPU_SPEED	DEFAULTSPEED
#endif

#ifndef EVENT_QUEUE_SIZE
#define EVENT_QUEUE_SIZE	16
#endif
static xQueueHandle xEventQueue = 0;
#ifndef HAL_NB_BUTTON
#define HAL_NB_BUTTON 0
#endif
#if HAL_NB_BUTTON > 0
static volatile LIST_INDEX ButtonNumber = 0;
#endif
#ifndef HAL_NB_LED
#define HAL_NB_LED 0
#endif

#ifndef HAL_NB_PULSE_IN
#define HAL_NB_PULSE_IN 0
#endif
#if HAL_NB_PULSE_IN > 0
static volatile unsigned long PulseInValue[HAL_NB_PULSE_IN];
#endif
/** @endcond */

/* Replace Timer based 1ms wait functions with FreeRTOS based ones
 * to allow low power fall down during waiting times
 */
void SysTimerWait1ms(const unsigned long n) {
	if (xTaskGetSchedulerState()==taskSCHEDULER_RUNNING) {
		portTickType timerFutureValue = xTaskGetTickCount();
		vTaskDelayUntil(&timerFutureValue,(((n*1000)+500) / configTICK_RATE_HZ));
	}
	else {
		TIMER_TYPE timerFutureValue;
		SysTimerStart1ms(&timerFutureValue,n);
		while(!SysTimerIsStopped(&timerFutureValue)) __nop();
		SysTimerStop(&timerFutureValue);
	}
}
//Device Status flags container as defined in device.h
volatile unsigned long DeviceStatus;
/** @cond */
/* Reserve a whole flash page inside the main Flash Memory */
const FLASH_PAGE 	__attribute__((aligned(2048)))
					__attribute__ ((__used__))
					__attribute__((section(".userdata")))
_USERPAGE_ = {
	.UserData = {
		.DataCRC = 0xFFFF,
		.DeviceType = DEVICETYPE_DEFAULT,
		.DeviceSerialNumber = LORAWAN_DEVICE_ADDRESS,
#if (OVER_THE_AIR_ACTIVATION > 0)
#if (USE_SKT_FORMAT > 0)
		.DeviceFlags = FLAG_USE_OTAA | FLAG_USE_SKT_APP,
#else
		.DeviceFlags = FLAG_USE_OTAA,
#endif
#else
#if (USE_SKT_FORMAT > 0)
		.DeviceFlags = FLAG_USE_SKT_APP,
#else
		.DeviceFlags = 0,
#endif
#endif
		.DefaultRFPeriod = LORAMAC_DEFAULT_RF_PERIOD,
		.LoRaWAN = {
			.Region = LORAMAC_REGION_DEFAULT,
			.NetID = LORAWAN_NETWORK_ID,
			.DevEui = LORAWAN_DEVICE_EUI,
			.AppEui = LORAWAN_APPLICATION_EUI,
			.AppKey = LORAWAN_APPLICATION_KEY,
			.NwkSKey = LORAWAN_NWKSKEY,
			.AppSKey = LORAWAN_APPSKEY
		}
	}
};
/** @endcond */

unsigned char* DeviceApplicationID(void) {
	return (unsigned char*)APP_ID;
}
unsigned short DeviceVersion(void) {
	return _NUM_VERSION;
}
unsigned short DeviceHWVersion(void) {
	return _HW_VERSION;
}
unsigned char* DeviceApplicationBuild(void) {
	return (unsigned char*)__DATE__" "__TIME__;
}

/*
 * Device Event Handling
 */
static StaticQueue_t xStaticEventQueue;
static uint8_t ucEventQueueStorageArea[ EVENT_QUEUE_SIZE * sizeof(EVENT_TYPE) ];

static inline void DeviceEventInit(void) {
	xEventQueue = xQueueCreateStatic(EVENT_QUEUE_SIZE,sizeof(EVENT_TYPE),ucEventQueueStorageArea,&xStaticEventQueue);
}

EVENT_TYPE DeviceWaitForEvent(portTickType duration) {
EVENT_TYPE evt;
	if (xEventQueue) {
		if (xQueueReceive(xEventQueue,&evt,duration) == pdTRUE) return evt;
	}
	return IDLE_EVENT;
}
void DeviceSendEvent(EVENT_TYPE event) {
	if (xEventQueue) {
		xQueueSendToBack(xEventQueue,&event,0);
	}
}
void DevicePostEvent(EVENT_TYPE event) {
	if (xEventQueue) {
		EVENT_TYPE e;
		// Ignore already waiting event (avoid redondency)
		if ((xQueuePeek(xEventQueue,&e,0) == pdTRUE) && (e == event)) return;
		xQueueSendToFront(xEventQueue,&event,0);
	}
}
void DevicePostEventFromISR(EVENT_TYPE event) {
	if (xEventQueue) {
		EVENT_TYPE e;
		// Ignore already waiting event (avoid redondency)
		if ((xQueuePeekFromISR(xEventQueue,&e) == pdTRUE) && (e == event)) return;
		xQueueSendToFrontFromISR(xEventQueue,&event,NULL);
	}
}
static signed portBASE_TYPE DeviceSendEventFromISR(EVENT_TYPE event) {
	signed portBASE_TYPE sHigherPriorityTaskWoken = pdFALSE;
	if (xEventQueue) {
		xQueueSendToBackFromISR(xEventQueue,&event,&sHigherPriorityTaskWoken);
	}
	return sHigherPriorityTaskWoken;
}

/*
 * Device Power Management handling
 */

static inline void DevicePowerMngInit(void) {
#ifdef HAL_VCMP_LEVEL
	SystemBatterySetDetectionLevel(HAL_VCMP_LEVEL);
#endif // HAL_VCMP_LEVEL
}
#ifdef HAL_VCMP_LEVEL
void SystemBatteryLowIrq(void) {
	SET_FLAG(DEVICE_LOW_BATTERY);
	portEND_SWITCHING_ISR( DeviceSendEventFromISR( BATTERYLOW_EVENT ));
}
#endif //HAL_VCMP_LEVEL

void DeviceLowBatteryEnable(BOOL enable) {
#ifdef HAL_VCMP_LEVEL
	SystemBatteryEnableDetection(enable);
#endif // HAL_VCMP_LEVEL
}
BOOL DeviceLowBatteryState(void) {
#ifdef HAL_VCMP_LEVEL
	return SystemBatteryIsLow();
#else
	return 0;
#endif // HAL_VCMP_LEVEL
}
unsigned long DeviceGetBatteryVoltage(void) {
	return SystemBatteryGetVoltage();
}

void DeviceRTCSetSeconds(unsigned long time) {
	RTCSetSeconds(time);
}

static inline void DeviceRFInit(void)
{
	SX1276IoInit( );
	uint8_t version = SX1276Read(0x42);
	if ((version == 0) || (version == 0xFF))
		DeviceShowErrorCode(DEVICE_RF_ERROR);
}

/*
 * Device button handling
 */

void BUTTON_IRQHandler(int pin) {
	for (int i=0; i< HAL_NB_BUTTON;i++) {
		if ((BTN[i].pin == pin) && (!(ButtonNumber & (0x01 << BTN[i].pin)))) {
			ButtonNumber |= (0x01 << BTN[i].pin);
			portEND_SWITCHING_ISR(  DeviceSendEventFromISR( BUTTON_EVENT ) );
		}
	}
}

static inline void DeviceButtonsInit(void) {
#if HAL_NB_BUTTON > 0
	for (int i=0; i< HAL_NB_BUTTON;i++) {
		/* Set falling edge interrupt for button port */
    	SystemDefinePortIrqHandler(BTN[i],BUTTON_IRQHandler,GPIOIRQFalling);
	}
#endif
}

BOOL DeviceIsButtonPressed(LIST_INDEX button) {
#if HAL_NB_BUTTON > 0
	if (button < HAL_NB_BUTTON)
		return (ButtonNumber & SYSTEMBITMASK(button)) ? 1 : 0;
#endif
	return 0;
}
void DeviceResetButton(LIST_INDEX button) {
#if HAL_NB_BUTTON > 0
	if (button < HAL_NB_BUTTON)
		ButtonNumber &= ~SYSTEMBITMASK(button);
#endif
}

void DeviceResetAllButtons(void) {
#if HAL_NB_BUTTON > 0
	ButtonNumber = 0;
#endif
}
#if HAL_NB_BUTTON > 0
static LIST_INDEX _btn;
#endif
static inline BOOL _getBtnDown(void) {
#if HAL_NB_BUTTON > 0
	return (!SystemGetPortState(BTN[_btn]));
#else
	return 0;
#endif
}
static inline BOOL _getBtnUp(void) {
#if HAL_NB_BUTTON > 0
	return SystemGetPortState(BTN[_btn]);
#else
	return 0;
#endif
}
static BOOL _checkButton(LIST_INDEX button, unsigned short duration_ms, int level) {
#if HAL_NB_BUTTON > 0
	/* Check High level on BTN0 to end showing key pressed */
	/* If no keypress, SystemWaitForEvent will return 0 */
	if (button < HAL_NB_BUTTON) {
		_btn = button;
		return (SystemWaitForFunction(level ? _getBtnUp : _getBtnDown,(unsigned long)duration_ms) == 0);
	}
	else
#endif
		return 0;
}
BOOL DeviceCheckAlternateButtonIsStillUp(LIST_INDEX button, unsigned short duration_ms) {
	return _checkButton(button,duration_ms, 0);
}
BOOL DeviceCheckAlternateButtonIsStillDown(LIST_INDEX button, unsigned short duration_ms) {
	return _checkButton(button,duration_ms, 1);
}
void DevicePressAlternateButton(LIST_INDEX button) {
#if HAL_NB_BUTTON > 0
	if (button < HAL_NB_BUTTON) {
		ButtonNumber |= SYSTEMBITMASK(button);
		DeviceSendEvent(BUTTON_EVENT);
	}
#endif
}
#if HAL_NB_BUTTON > 0
static inline BOOL DeviceWaitForAlternateButton(LIST_INDEX button, unsigned short time, unsigned short period) {
	if (!period) period = time;
	for (unsigned short i=(time/period);i; i--) {
		if (!DeviceCheckAlternateButtonIsStillDown(button,period)) { // Button released
			return 0;
		}
		DeviceFlashLed((i%2) ? LED_FLASH_OFF : LED_FLASH_ON);
	}
    DeviceFlashLed(LED_FLASH_OFF); SysTimerWait1ms(50);
    DeviceFlashLed(LED_FLASH_ON);
	return 1;
}
#endif
BOOL DevicePerformKeypressTasks(LIST_INDEX button,const BUTTON_TASKLIST* tasks) {
#if HAL_NB_BUTTON > 0
	int tasknb = 0;
	TRACE("Button pressed.\n");
	while (tasks[tasknb].KeypressDuration) {
		if (!DeviceWaitForAlternateButton(button,tasks[tasknb].KeypressDuration,tasks[tasknb].LedFlashCycle)){
			TRACE("Button released.\n");
			break; //Button released
		}
		tasknb++;
	}
	if (tasknb-- && (tasks[tasknb].KeypressTask)) {
	    DeviceFlashLed(LED_FLASH_ON);
		(*tasks[tasknb].KeypressTask)();
		return 1;
	}
#endif

	TRACE("Button canceled.\n");
    DeviceFlashLed(LED_FLASH_OFF);
	return 0;
}

/*
* Device pulse handling
 */
#if HAL_NB_PULSE_IN > 0
#if (NODE_TEMP > 0)
// Nothing to do for temperature sensor
#elif (NODE_ANALOG > 0)
// Nothing to do for analog sensor
#else
void PULSE_IN_IRQHandler(int pin) {
	for(int i=0; i < HAL_NB_PULSE_IN ;i++) {
		if(PULSE_IN[i].pin == pin) {
			PulseInValue[i]++;
#ifndef PULSE_NO_EVENT
			portEND_SWITCHING_ISR( DeviceSendEventFromISR( PULSE_EVENT ) );
#endif
			break;
		}
	}
}
#endif
#endif

static inline void DevicePulsesInit(void) {
#if HAL_NB_PULSE_IN > 0
	for (int i=0; i < HAL_NB_PULSE_IN; i++) {
		PulseInValue[i] = 0;
	}
	DevicePulseInEnable(true);
#endif
}
void DevicePulseInEnable(BOOL bEnable) {
#if HAL_NB_PULSE_IN > 0
#if (NODE_TEMP > 0)
// Nothing to do for temperature sensor
#elif (NODE_PULSE > 0)
	for (int i=0; i < HAL_NB_PULSE_IN; i++) {
		SystemSetPortMode(PULSE_IN[i],(bEnable) ? PortIn : PortDisabled);
#ifdef PULSE_USE_RISING_EDGE
    	SystemDefinePortIrqHandler(PULSE_IN[i], PULSE_IN_IRQHandler, GPIOIRQRising);
#else
    	SystemDefinePortIrqHandler(PULSE_IN[i], PULSE_IN_IRQHandler, GPIOIRQFalling);
#endif
	}
#endif
#else
	(void)bEnable;	// Remove warning
#endif // HAL_NB_PULSE_IN > 0
}
unsigned long DeviceGetPulseInNumber(void) {
	return HAL_NB_PULSE_IN;
}
unsigned long *DeviceGetPulseInValuePtr(LIST_INDEX pulse) {
#if HAL_NB_PULSE_IN > 0
	if (pulse < HAL_NB_PULSE_IN)
		return (unsigned long *)&PulseInValue[pulse];
#else
	(void) pulse;
#endif
	return NULL;
}
unsigned long DeviceGetPulseInValue(LIST_INDEX pulse) {
#if HAL_NB_PULSE_IN > 0
	if (pulse < HAL_NB_PULSE_IN)
		return PulseInValue[pulse];
#else
	(void) pulse;
#endif
	return 0;
}
void DeviceSetPulseInValue(LIST_INDEX pulse, unsigned long value) {
#if HAL_NB_PULSE_IN > 0
	if (pulse < HAL_NB_PULSE_IN)
		PulseInValue[pulse] = value;
#else
	(void) pulse;
	(void) value;
#endif
}
BOOL DeviceGetPulseInState(LIST_INDEX pulse) {
#if HAL_NB_PULSE_IN > 0
	if (pulse < HAL_NB_PULSE_IN)
		return (BOOL)SystemGetPortState(PULSE_IN[pulse]);
#else
	(void) pulse;
#endif
	return 1;
}
void DevicePulseInCheck(void) {
#if HAL_NB_LOOP > 0
	for (int i=0; (i < HAL_NB_LOOP) && !GET_FLAG(DEVICE_COMM_ERROR); i++) {
		SystemSetPortMode(LOOP_IN[i],PortInUp);
		if (SystemGetPortState(LOOP_IN[i])) SET_FLAG(DEVICE_COMM_ERROR);
		SystemSetPortMode(LOOP_IN[i],PortDisabled);
	}
#endif
}
BOOL DevicePulseOutState(void) {
#ifdef PULSE_OUT
	return SystemGetPortState(PULSE_OUT);
//	return GPIO_PinInGet(PULSE_OUT->port,PULSE_OUT->pin);
#else
	return 0;
#endif
}

#if (NODE_TEMP > 0)	// TSIC 306 type of sensor
static inline BOOL EvenParity( unsigned char b )
{
    b ^= b >> 4;
    b ^= b >> 2;
    b ^= b >> 1;
    return (BOOL)(b & 1);
}
/** @cond */
#define TEMPLOWREF		(-50)
#define TEMPHIREF		(150)
#define TEMP_TIMEOUT	0x0000FFFFUL
/** @endcond */
/*
 * @brief Reads a bit from the data wire of the TSIC sensor.
 * @param[in] maxTimeout  Maximum amount of loops to wait for a falling edge
 * @return the number of iteration between the rising edge and falling edge detection
 */
static int DeviceGetZACWireBit(int maxTimeout) {
	/* Mesure bit low level length */
	/* Wait for falling edge of start of next bit.
	 *  We are at 8k bps, so the timeout is of some 125 µsec */
	/* This loop lasts for about 5 sec to timeout in case of missing sensor */
	int timeout = maxTimeout;
	/* Wait for falling edge */
	while (--timeout && SystemGetPortState(TEMPSENSOR_VALUE)) __nop();
	if (timeout) {
		/* Wait for rising edge */
		timeout = TEMP_TIMEOUT;
		/* Block FreeRTOS Kernel during acquisition */
		taskENTER_CRITICAL();
		while(--timeout && !SystemGetPortState(TEMPSENSOR_VALUE)) __nop();
		/* Resume FreeRTOS Kernel */
		taskEXIT_CRITICAL();
	}
	/* Compute bit duration */
	return (timeout) ? (TEMP_TIMEOUT - timeout) : 0;
}
/*
 * @brief Read a whole byte from the data wire of the TSIC sensor.
 * A byte consists in 8 bits + 1 even parity bit.
 * @return the read byte or 0 if error
 * @remark This function would raise the DEVICE_COMM_ERROR status flag upon communication error,
 * and add the DEVICE_PERMANENT_ERROR status flag if no falling edge has been detected which could
 * mean that the sensor is missing.
 */
static unsigned long DeviceGetZACWireByte() {
unsigned long value = 0;
	if (GET_FLAG(DEVICE_COMM_ERROR) == 0) {
		/* Get Strobe timer = 50% duty cycle */
		int bitTime = DeviceGetZACWireBit(TEMP_TIMEOUT << 5) + 1;
		if (bitTime > 1) {
			/* Read 9 bits = (8 bits + parity) */
			for (int b = 9; b ; b--) {
				int timeout = DeviceGetZACWireBit(TEMP_TIMEOUT);
				if (timeout) {
					/* If rising edge occured before 50% duty cycle, it's a logic 1 */
					value <<= 1;
					value |= (timeout < bitTime) ? 1 : 0;
				}
				else {
					SET_FLAG(DEVICE_COMM_ERROR);	/* Set Communication Status */
					break;
				}
			}
			/* Check Parity */
			if ((value & 0x01) != EvenParity((unsigned char)(value >> 1))) {
				SET_FLAG(DEVICE_COMM_ERROR);	/* Set Communication Status */
			}
			else
				value >>=1;	/* Remove parity bit */
		}
		else {
			/* Strobe capture error - Sensor is absent or failing. Set Permanent error flag*/
			SET_FLAG(DEVICE_COMM_ERROR | DEVICE_PERMANENT_ERROR);	/* Set Communication Status */
		}
	}
	return (GET_FLAG(DEVICE_COMM_ERROR) ? 0: (value & 0xFF));
}
/*
 * @brief Read two consecutive bytes from the data wire of the TSIC sensor.
 * The read value, if no error, will be translated in a temperature value in °C
 * @return the read temperature in °C or 0 if error
 */
static unsigned long DeviceGetZACWireWord() {
	SystemSetPortMode(TEMPSENSOR_VALUE,PortInUp);
	/* Detect sensor presence. It shall start after 60ms to 85ms
	 * and then issue measure information (about 2.5 ms transmission time) 10 times per seconds
	 */
	SysTimerWait1ms(10);	/* Wait 10 ms for ZAC Wire input signal to get high */
	int value = (DeviceGetZACWireByte() << 8) | DeviceGetZACWireByte();
	if (value > 0x7FF) { value = 0; SET_FLAG(DEVICE_COMM_ERROR); }	/* Set Communication Status */
	/* Compute °C from acquired value using manufacturer formula:
	 *
	 * T°C = ((Value / 2047) * (HIGHREF - LOWREF)) + LOWREF
	 *
	 * Where LOWREF = -50°C and HIGHREF = 150°C for TSic 306
	 *
	 */
	if (!GET_FLAG(DEVICE_COMM_ERROR))
		value = ((value * (long)((TEMPHIREF - TEMPLOWREF) * 10)) / 2047L) + (long)(TEMPLOWREF * 10);
	SystemDefinePort(TEMPSENSOR_VALUE);
	return value;
}
#endif

#if (NODE_HYGRO > 0)
#define I2CDelay() __nop()

/*
 * @brief Initialize SPI bus to work with SHT75 sensor
 * @remark SDA is put in Open Drain mode due to SHT75 special I2C protocol
 */
static inline void I2CInit(void) {
	SystemSetPortMode(I2C_SCL,PortOut1);
	SystemSetPortState1(I2C_SCL);
	SystemSetPortMode(I2C_SDA,PortDrainOpen);
}
// @brief SHT75 Pseudo I2C start sequence
static BOOL sendI2CStart(void) {
	SystemSetPortState1(I2C_SDA);
	// Is SDA line free or busy
	if (SystemGetPortState(I2C_SDA)) {
		SystemSetPortState1(I2C_SCL);
		for(int i=10000;i;i--) __nop();
		SystemSetPortState0(I2C_SDA);
		for(int i=10000;i;i--) __nop();
		SystemSetPortState0(I2C_SCL);
		for(int i=10000;i;i--) __nop();
		SystemSetPortState1(I2C_SCL);
		SystemSetPortState1(I2C_SDA);
		for(int i=10000;i;i--) __nop();
		SystemSetPortState0(I2C_SCL);
		for(int i=10000;i;i--) __nop();
	}
	else return 0;	// Line Busy
	return 1;		// OK
}
/*
 * @brief Send 1 bit on the bus using I2C protocol
 */
static inline void I2CSend1Bit(int bit) {
	SystemSetPortState0(I2C_SCL);
	SystemSetPortState(I2C_SDA,bit);
	I2CDelay();
	SystemSetPortState(I2C_SDA,bit);
	SystemSetPortState1(I2C_SCL);
	I2CDelay();
	SystemSetPortState1(I2C_SCL);
	I2CDelay();
	SystemSetPortState0(I2C_SCL);
}
/*
 * @brief Receive 1 bit from the bus using I2C protocol
 */
static inline BOOL I2CReceive1Bit(void) {
	SystemSetPortState0(I2C_SCL);
	SystemSetPortState(I2C_SDA,1);
	SystemSetPortState1(I2C_SCL);
	I2CDelay();
	BOOL value = (SystemGetPortState(I2C_SDA) != 0);
	SystemSetPortState0(I2C_SCL);
	return value;
}
/*
 * @brief Send 1 byte on the bus using I2C protocol
 */
static BOOL I2CRawSend(unsigned char value) {
	for (unsigned char i=0x80; i != 0; i >>= 1) {
		I2CSend1Bit(value & i);
	}
	return I2CReceive1Bit();
}
/*
 * @brief Receive 1 byte from the bus using I2C protocol
 */
static unsigned char I2CRawReceive(BOOL ack) {
unsigned char value = 0;
	SystemSetPortState1(I2C_SDA);
	I2CDelay();
	for (unsigned char i=0x80; i != 0; i >>= 1) {
		SystemSetPortState0(I2C_SCL);
		I2CDelay();
		SystemSetPortState(I2C_SCL,0);
		I2CDelay();
		SystemSetPortState1(I2C_SCL);
		I2CDelay();
		SystemSetPortState(I2C_SCL,1);
		I2CDelay();
		value |= (SystemGetPortState(I2C_SDA)) ? i : 0;
	}
	I2CSend1Bit(ack);
	return value;
}
/*
 * @ brief SHT75 special read value process
 * @remark The SHT75 sensor can insert a wait state by holding down the SDA line when it is busy
 */
static inline BOOL _getSDA(void) { return (SystemGetPortState(I2C_SDA)==0); }
static unsigned long ReadSensorData(int reg) {
	unsigned long result = 0;
	if (sendI2CStart()) {
		if (I2CRawSend(reg) == 0) {							// Ask for data
			SysTimerWait1ms(30);							// Wait for sensor to proceed
			if (SystemWaitForFunction(_getSDA,1000)) {		// Wait 1000 ms for sensor to release SDA line
				result = (unsigned long)I2CRawReceive(0) << 8;
				result |= (unsigned long)I2CRawReceive(1);
			}
			else
				SET_FLAG(DEVICE_COMM_ERROR);
		}
		else
			SET_FLAG(DEVICE_COMM_ERROR);
	}
	else
		SET_FLAG(DEVICE_COMM_ERROR);
	return result;
}
#endif

#if (NODE_TEMP > 0) || (NODE_HYGRO > 0)
/*
 * @brief read the temperature from the given sensor
 * @param[in] sensor index
 */
static void DeviceGetTemperature(LIST_INDEX sensor) {
	/* Switch on temperature sensor */
	SystemSetPortMode(TEMPSENSOR_ENABLE[sensor],PortOut1);
	/* Acquire Sensor information */
#if (NODE_HYGRO > 0)
	I2CInit();
	PulseInValue[0] = PulseInValue[1] = 0;
	SysTimerWait1ms(5);		// Wait for sensor startup
	unsigned long temp = ReadSensorData(0x03);
	if (!GET_FLAG(DEVICE_COMM_ERROR)) {
		PulseInValue[0] = (temp / 10) - 397;
		double hum = (double)ReadSensorData(0x05);
		if ((DeviceStatus & DEVICE_COMM_ERROR) == 0) {
			hum += (double)(unsigned long)I2CRawReceive(1);
			hum = (-20.468f + (0.367f * hum) + (-1.5955E-5f * (hum * hum)))
				+ ((((double)((short)temp)/10) - 25.0f) * ((0.01f + 0.00008f * hum)));
			PulseInValue[1] = ((unsigned long)((int)(hum)));
		}
	}
	/* Switch off temperature sensor */
	SystemDefinePort(I2C_SDA);
	SystemDefinePort(I2C_SCL);
#elif (NODE_TEMP > 0)
	PulseInValue[sensor] = DeviceGetZACWireWord();
#endif
#if (NODE_HYGRO == 0)
	/* Switch off temperature sensor power supply to reduce consumption */
	SystemSetPortMode(TEMPSENSOR_ENABLE[sensor],PortOut0);
#else
	/* Let SHT power supply floating to reduce power consumption */
	SystemSetPortMode(TEMPSENSOR_ENABLE[sensor],PortDisabled);
#endif
}
#endif

#define OVERSAMPLE 50
void DeviceCaptureAnalogValue(LIST_INDEX pulse) {
#if (HAL_NB_PULSE_IN > 0)
	if (pulse >= HAL_NB_PULSE_IN) return;
#if ((NODE_TEMP > 0) || (NODE_HYGRO > 0))
	DeviceGetTemperature(pulse);
#elif (NODE_ANALOG > 0) // Read Analog Value is enabled
	// Open ADC in differential mode -> ADC full scale = Full Scale / 2
	ADCOpenDifferential(ADCNUM,ADCLOCATION,ADCPCHANNEL,ADCNCHANNEL,ADCREF,ADCSAMPLEDELAY,ADCTIME);
	PulseInValue[pulse] = 0;						// Reset result
	for (int i = OVERSAMPLE; i; i--) {
		int value  = (int)ADCConvertOnce(ADCNUM);	// Value can be negative
		if (value < 0 ) value = -value;				// Make sure value is positive
		PulseInValue[pulse] += value;				// Accumulate results
	}
	PulseInValue[pulse] /= (OVERSAMPLE);			// Compute mean value
#if defined(CONVERT_4_20MA)
	// Convert to 400-20000 µA -> each ADC unit = (5000000 µV /2) / 120 Ohms / (FullScale/2)
	// With minimum value = 0, and maximum value = 2047 => 0 µA to 20833 µA
	// After division simplification by 40 to keep intermediate result in unsigned long format:
	PulseInValue[pulse] = ( (ADCGetReference(ADCNUM) / 2) * PulseInValue[pulse] * 25) / (3 * (ADCGetFullScale(ADCNUM)/2));
	if (PulseInValue[pulse] < 3000)	SET_FLAG(DEVICE_COMM_ERROR); // Error if < 3 mA
#elif defined(CONVERT_0_10V) || defined(CONVERT_0_5V)
	// Convert to 0 - 10000mV -> each ADC unit = 5000 / 2 * 4 / ((FullScale/2) - 1)
	// Convert to 0 - 5000mV  -> each ADC unit = 2500 / 2 * 4 / ((FullScale/2) - 1)
	PulseInValue[pulse] = ( ADCGetReference(ADCNUM) * PulseInValue[pulse] * 2) / (ADCGetFullScale(ADCNUM) / 2);
#endif
	ADCClose(ADCNUM,ADCPCHANNEL);
#endif //  TEMP / ANALOG
#else
	(void) pulse;
#endif // NB_PULSE > 0
}
/*
 * Device LED handling
 */
/** @cond */
#ifndef HAL_NB_LED
#define HAL_NB_LED	0
#pragma message "Warning: Added HAL_NB_LED definition"
#endif
static inline void DeviceInitLeds(void) {
#if (HAL_NB_LED > 0)
	DeviceFlashLed(LED_FLASH_ON);
#endif // HAL_NB_LED > 0
}

#define DELAY50MS	10000UL
/** @endcond */
__attribute__((noreturn)) void DeviceShowErrorCode(int code) {
	SystemSetClockSpeed(VERYLOWSPEED);
	for (int j=40; j; j--) {			/* Run this loop for at least 1 minute */
		for (int i=10;i ; i--) {
			SystemSetPortState1(LED[0]);
			for(int k=DELAY50MS;k; k--) __nop();
			SystemSetPortState0(LED[0]);
			for(int k=DELAY50MS;k; k--) __nop();
		}
		for(int k=DELAY50MS*5;k; k--) __nop();
		for (int i=code;i; i--) {
			SystemSetPortState1(LED[0]);
			for(int k=DELAY50MS*5;k; k--) __nop();
			SystemSetPortState0(LED[0]);
			for(int k=DELAY50MS*5;k; k--) __nop();
		}
		for(int k=DELAY50MS*5;k; k--) __nop();
	}
	// Then reboot to see if it goes better
	SystemReboot();
	for(;;);
	__builtin_unreachable();
}

#if  HAL_NB_LED > 0
static int bLED = 0;
#endif
void DeviceFlashOneLedExt(LIST_INDEX led, short number, short duration) {
#if  HAL_NB_LED > 0
	if (led < HAL_NB_LED) {
		if (number == LED_FLASH_ON)
			bLED |= SYSTEMBITMASK(led);
		else if (number == LED_FLASH_TOGGLE)
			bLED ^= SYSTEMBITMASK(led);
		else if (number == LED_FLASH_OFF)
			bLED &= ~SYSTEMBITMASK(led);
		else
			for(int i=(number*2);i; i--) {
				SystemTogglePort(LED[led]);
				vTaskDelay(configTICK_RATE_HZ / ((duration) ? duration : LED_FLASH_NORMAL));
			}
		SystemSetPortState(LED[led],bLED & SYSTEMBITMASK(led));
	}
#else
	(void) led;
	(void) number;
	(void) duration;
#endif // HAL_NB_LED > 0
}
BOOL DeviceGetLedStateExt(LIST_INDEX led) {
#if HAL_NB_LED > 0
	if (led < HAL_NB_LED) {
		return (BOOL)SystemGetPortState(LED[led]);
	}
#else
	(void) led;
#endif // HAL_NB_LED > 0
	return 0;
}
LIST_INDEX DeviceGetLedNumber(void) {
	return HAL_NB_LED;
}

/*
 * Device User Data Management
 */
static bool checkBlank(unsigned char* p, int len) {
bool rc = true;
	while (rc && len--) {
		if (*p && (*p != 0xFF)) rc = false;
		p++;
	}
	return rc;
}
/*!
 * @brief Checks User Data integrity in the Flash memory region
 * @remark Will get default commissioning data from the User Data Flash memory region.
 */
static inline void DeviceUserDataCheck(void) {
USERDATA UData;
	if (CRC16_CalculateRange(((unsigned char*)USERPAGE)+sizeof(short),sizeof(USERDATA)-sizeof(short),0xFFFF) == ((USERDATA*)USERPAGE)->DataCRC) {
		memcpy((unsigned char*)&UData,(unsigned char*)USERPAGE,sizeof(USERDATA));
		UData.DeviceFlags &= ~FLAG_INSTALLED;		// Make sure device is not installed by default
		DeviceUserDataSave(&UData);
		// Reset sensitive data
		memset(&UData.LoRaWAN, 0xFF, sizeof(LORAWAN_INFO));
		FLASHOpen();
		FLASHEraseUserData();
//		FLASHWriteUserData(0,(unsigned char*)&UData,sizeof(USERDATA));
		FLASHClose();
	}
	if (CRC16_CalculateRange(((unsigned char*)USERDATAPTR)+sizeof(short),sizeof(USERDATA)-sizeof(short),0xFFFF) != UNIT_CHECKSUM)
	{
		memcpy((unsigned char*)&UData,(unsigned char*)USERDATAPTR,sizeof(USERDATA));
		DeviceUserDataSave(&UData);
	}
}

void DeviceUserDataSave(USERDATA* DataPtr) {
	if (memcmp((void*)USERDATAPTR,DataPtr,sizeof(USERDATA))) {
		/* Make sure some correct LoRaWAN info is stored in Flash Memory */
		if (checkBlank(DataPtr->LoRaWAN.DevEui,sizeof(DataPtr->LoRaWAN.DevEui))) {
			memcpy(DataPtr->LoRaWAN.DevEui,(const unsigned char[])LORAWAN_DEVICE_EUI,sizeof(DataPtr->LoRaWAN.DevEui));
		}
		if (checkBlank(DataPtr->LoRaWAN.AppEui,sizeof(DataPtr->LoRaWAN.AppEui))) {
			memcpy(DataPtr->LoRaWAN.AppEui,(const unsigned char[])LORAWAN_APPLICATION_EUI,sizeof(DataPtr->LoRaWAN.AppEui));
		}
		if (checkBlank(DataPtr->LoRaWAN.AppKey,sizeof(DataPtr->LoRaWAN.AppKey))) {
			memcpy(DataPtr->LoRaWAN.AppKey,(const unsigned char[])LORAWAN_APPLICATION_KEY,sizeof(DataPtr->LoRaWAN.AppKey));
		}
		if (checkBlank(DataPtr->LoRaWAN.AppSKey,sizeof(DataPtr->LoRaWAN.AppSKey))) {
			memcpy(DataPtr->LoRaWAN.AppSKey,(const unsigned char[])LORAWAN_APPSKEY,sizeof(DataPtr->LoRaWAN.AppSKey));
		}
		if (checkBlank(DataPtr->LoRaWAN.NwkSKey,sizeof(DataPtr->LoRaWAN.NwkSKey))) {
			memcpy(DataPtr->LoRaWAN.NwkSKey,(const unsigned char[])LORAWAN_NWKSKEY,sizeof(DataPtr->LoRaWAN.NwkSKey));
		}
		DataPtr->DataCRC = CRC16_CalculateRange(((unsigned char*)DataPtr)+sizeof(short),sizeof(USERDATA)-sizeof(short),0xFFFF);
		vTaskSuspendAll();
		FLASHOpen();
		if (FLASHEraseBlock((void*)USERDATAPTR) == FLASH_NO_ERROR) {
			FLASHWrite((void*)USERDATAPTR,(unsigned char*)DataPtr,sizeof(USERDATA));
		}
		FLASHClose();
		xTaskResumeAll();
	}
}
void DeviceUserDateSetSerialNumber(unsigned long serial) {
	if (USERDATAPTR->DeviceSerialNumber != serial) {
		USERDATA UData;
		memcpy((unsigned char*)&UData,(unsigned char*)USERDATAPTR,sizeof(USERDATA));
		UData.DeviceSerialNumber = serial;
		DeviceUserDataSave(&UData);
		// Reset sensitive data
		memset(&UData.LoRaWAN, 0xFF, sizeof(LORAWAN_INFO));
		UData.DeviceFlags &= ~FLAG_INSTALLED;
		FLASHOpen();
		FLASHWriteUserData(0,(unsigned char*)&UData,sizeof(USERDATA));
		FLASHClose();
	}
}
void DeviceUserDataSetFlag(unsigned char Mask, unsigned char Value) {
	if ((USERDATAPTR->DeviceFlags & Mask) != Value) {
		USERDATA UData;
		memcpy((unsigned char*)&UData,(unsigned char*)USERDATAPTR,sizeof(USERDATA));
		UData.DeviceFlags &= ~Mask;
		UData.DeviceFlags |= Value;
		DeviceUserDataSave(&UData);
	}
}
void DeviceUserDataSetRFPeriod(unsigned long Period) {
	if ((USERDATAPTR->DefaultRFPeriod) != Period) {
		USERDATA UData;
		memcpy((unsigned char*)&UData,(unsigned char*)USERDATAPTR,sizeof(USERDATA));
		UData.DefaultRFPeriod = Period;
		DeviceUserDataSave(&UData);
	}
}

void DeviceUserDataSetDevEUI(uint8_t *DevEUI) {
	if (memcmp(DevEUI, (void*)(USERDATAPTR->LoRaWAN.DevEui), 8) != 0) {
		USERDATA UData;
		memcpy((unsigned char*)&UData,(unsigned char*)USERDATAPTR,sizeof(USERDATA));
		memcpy(UData.LoRaWAN.DevEui, DevEUI, 8);
		DeviceUserDataSave(&UData);
		// Reset sensitive data
		UData.DeviceFlags &= ~FLAG_INSTALLED;
		FLASHOpen();
		FLASHWriteUserData(0,(unsigned char*)&UData,sizeof(USERDATA));
		FLASHClose();
	}
}

void DeviceUserDataSetAppEUI(uint8_t *AppEUI) {
	if (memcmp(AppEUI, (void*)(USERDATAPTR->LoRaWAN.AppEui), 8) != 0) {
		USERDATA UData;
		memcpy((unsigned char*)&UData,(unsigned char*)USERDATAPTR,sizeof(USERDATA));
		memcpy(UData.LoRaWAN.AppEui, AppEUI, 8);
		DeviceUserDataSave(&UData);
		// Reset sensitive data
		UData.DeviceFlags &= ~FLAG_INSTALLED;
		FLASHOpen();
		FLASHWriteUserData(0,(unsigned char*)&UData,sizeof(USERDATA));
		FLASHClose();
	}
}

void DeviceUserDataSetAppKey(uint8_t *AppKey) {
	if (memcmp(AppKey, (void*)(USERDATAPTR->LoRaWAN.AppKey), 16) != 0) {
		USERDATA UData;
		memcpy((unsigned char*)&UData,(unsigned char*)USERDATAPTR,sizeof(USERDATA));
		memcpy(UData.LoRaWAN.AppKey, AppKey, 16);
		DeviceUserDataSave(&UData);
		// Reset sensitive data
		UData.DeviceFlags &= ~FLAG_INSTALLED;
		FLASHOpen();
		FLASHWriteUserData(0,(unsigned char*)&UData,sizeof(USERDATA));
		FLASHClose();
	}
}

void DeviceUserDataSetSKTRealAppKey(uint8_t *RealAppKey) {
	if (memcmp(RealAppKey, (void*)(USERDATAPTR->SKT.RealAppKey), 16) != 0) {
		USERDATA UData;
		memcpy((unsigned char*)&UData,(unsigned char*)USERDATAPTR,sizeof(USERDATA));
		memcpy(UData.SKT.RealAppKey, RealAppKey, 16);
		DeviceUserDataSave(&UData);
		// Reset sensitive data
		UData.DeviceFlags &= ~FLAG_INSTALLED;
		FLASHOpen();
		FLASHWriteUserData(0,(unsigned char*)&UData,sizeof(USERDATA));
		FLASHClose();
	}
}

/*
 *
 * Module Hardware Initialisation
 *
 * Moved down to allow usage of static inline functions
 * in order to split function blocs for readability
 *
 */
#include <em_msc.h>

void DeviceInitHardware() {
	SystemInitGPIO();
	SystemDefinePorts(HAL);
	DeviceUserDataCheck();
	DeviceInitLeds();
	if (!SystemInitHardware(HAL_CPU_SPEED,HAL_USE_LFXO))
    	DeviceShowErrorCode(DEVICE_LFXO_ERROR);
	DevicePulsesInit();
	DeviceEventInit();
	DeviceButtonsInit();
	DeviceRFInit();
	DevicePowerMngInit();
	RtcInit();
}
/** @}*/
#endif
