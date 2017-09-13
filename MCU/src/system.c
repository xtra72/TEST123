/*******************************************************************
**                                                                **
** MCU dependent system functions                                 **
**                                                                **
*******************************************************************/

#include "system.h"
#include "mmi_adc.h"
#include <stdio.h>
#include <em_device.h>
#include <em_rtc.h>
#include <em_chip.h>
#include <em_cmu.h>
#include <em_emu.h>
#include <em_gpio.h>
#include <em_rmu.h>
#include <em_rtcc.h>
#include <em_cryotimer.h>
#ifdef WDOG_PRESENT
#include <em_wdog.h>
#endif

/** \addtogroup MMI MyMeterInfo add-on functions
 *  @{
 */

static SYSTEMPORT_IRQHANDLER SystemIrqs[16] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
												NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

//! @brief Define EXCLUDE_DEFAULT_GPIO_IRQ_HANDLER to use user defined GPIO IRQ handler instead
#ifndef EXCLUDE_DEFAULT_GPIO_IRQ_HANDLER
static void _doIrq(void) {
	uint32_t Irq = GPIO_IntGet() & 0x0FFFF;
	for (int i= __builtin_ffs(Irq); i; i= __builtin_ffs(Irq)) {
		GPIO_IntClear(0x01 << (i-1));
		Irq ^= (0x01 << (i-1));		// Mask found set bit
		if (SystemIrqs[i-1]) (*SystemIrqs[i-1])(i-1);	// Execute IRQ if exists
	}
}
__interrupt_handler __attribute__((used)) void GPIO_ODD_IRQHandler(void) { _doIrq(); }
__interrupt_handler __attribute__((used)) void GPIO_EVEN_IRQHandler(void){ _doIrq(); }

#endif
/******************************************************************************************
** Main function helpers
*******************************************************************************************/

/*******************************************************************
**                System Public functions                         **
**                not related to hardware                         **
*******************************************************************/

/*******************************************************************
**                System Public functions                         **
**                hardware oriented                               **
*******************************************************************/

void SystemIRQEnable(int IRQn) {
	NVIC_ClearPendingIRQ(IRQn);
	NVIC_SetPriority(IRQn,7);	// Set Priority for FreeRTOS
	NVIC_EnableIRQ(IRQn);
}

static volatile int _bEnable = 0;
__weak void SystemIrqDisable(void) {
	__disable_irq();
	_bEnable++;
}
__weak void SystemIrqEnable(void) {
	if (_bEnable > 0) _bEnable--;
	if (!_bEnable) __enable_irq();
}

/*******************************************************************
**                        RTC functions                           **
*******************************************************************/
/** @cond */
#define RTC_POWEROF2 (15 - ((RTCC->CTRL & _RTCC_CTRL_CNTPRESC_MASK) >> _RTCC_CTRL_CNTPRESC_SHIFT))
#define RTC_INCREMENT (0x0001UL << RTC_POWEROF2)
static volatile unsigned long system_seconds = 0UL;
static volatile unsigned long _micro_seconds = 0UL;
/** @endcond */

void INTRTC_IRQHandler(unsigned long n)
{
	if (n) {
		int PowerOf2 = RTC_POWEROF2;
		unsigned long _RTC_increment = (0x0001UL << PowerOf2)-1;
		// Each n is  (1/(_RTC_increment+1))th of second
		system_seconds += n >> PowerOf2;						// Division by _RTC_Increment+1
		n &= _RTC_increment;									// Reminder = milliseconds
		if (n) {
			n *= 1000;											// plain milliseconds
			if (n > _RTC_increment) {
				_micro_seconds += (n >> PowerOf2) * 1000;		// milliseconds in microseconds
				n &= _RTC_increment;							// reminder = microseconds
			}
			if (n) _micro_seconds += (n * 1000) >> PowerOf2;	// plain microseconds
			if (_micro_seconds > 1000000) {						// Accumulated seconds in microseconds ?
				// Now update seconds out of accumulated microseconds
				system_seconds += (_micro_seconds / 1000000);
				// Now keep only milli & microseconds
				_micro_seconds %= 1000000;
			}
		}
	}
	// From here elapsed time is system_seconds._micro_seconds
}
//! @brief Define this macro to use user defined RTC IRQ handler
#ifndef EXCLUDE_DEFAULT_RTC_IRQ_HANDLER
__interrupt_handler void RTCC_IRQHandler() { INTRTC_IRQHandler(RTCC->CC[0].CCV +1); RTCC->IFC = RTCC_IFC_CNTTICK; }
#endif

void SystemSetDefaultRTC(BOOL enable_irq) {
	if ((RTCC->CTRL & RTCC_CTRL_ENABLE) == 0) {
		// If RTC is not setup yet, just do it.
		CMU_ClockSelectSet(cmuClock_HFLE,(CMU->STATUS & CMU_STATUS_LFXORDY) ? cmuSelect_LFXO : cmuSelect_LFRCO);
		CMU_ClockEnable(cmuClock_HFLE,true);
		CMU_ClockEnable(cmuClock_RTCC, true);	// Start RTCC
		RTCC_CCChConf_TypeDef RtccChannelInit = RTCC_CH_INIT_COMPARE_DEFAULT;
		RTCC_ChannelInit(0,&RtccChannelInit);
	    // About 1 ms per increment in cnt
		RTCC_ChannelCCVSet(0,32);
		if (enable_irq)
			RTCC_IntEnable(RTCC_IEN_CNTTICK);
		RTCC_Init_TypeDef RtccInit = RTCC_INIT_DEFAULT;
	//	RtccInit.presc = rtccCntPresc_8;	// Any prescaler would fail => processor bug
		RtccInit.presc = rtccCntPresc_1;
		RtccInit.precntWrapOnCCV0 = true;
		/* Start Counter */
		RTCC_Init(&RtccInit);
	}
  if (enable_irq)
	  SystemIRQEnable(RTCC_IRQn);
}

__inline__ unsigned long SystemGetMicroSeconds(void) {
	return (_micro_seconds % 1000);
}

unsigned long SystemGetSystemTicks()
{
#if CRYOTIMER_COUNT > 0
	// Use free run CRYOTIMER if exists
	if (CRYOTIMER->CTRL & CRYOTIMER_CTRL_EN) {
		// Get instantaneous CRYOTIMER value
		register uint32_t timer = CRYOTIMER_CounterGet();
		// Compute the number of full seconds in milliseconds
		register uint32_t value = (timer / 1024) * 1000;
		// Add remaining milliseconds
		return value + (((timer % 1024) * 1000) / 1024);
	}
	else
#endif
	// Use RTCC instead
	if (RTCC->IEN) {
		// This is linked to RTCC IRQ usage, so might be out of sync
		// (FreeRTOS Low Power mode for instance).
		SystemIrqDisable();
		unsigned long t = (system_seconds * 1000) + (_micro_seconds / 1000);
		SystemIrqEnable();
		return t;
	}
	else {
		// RTCC is used as free run 32.768kHz / 1024
		// Get instantaneous RTCC value
		register uint32_t timer = RTCC_CounterGet();
		// Compute the number of full seconds in milliseconds
		register uint32_t value = (timer / 1024) * 1000;
		// Add remaining milliseconds
		return value + (((timer % 1024) * 1000) / 1024);
	}
}

__inline__ unsigned long SystemGetSystemSeconds()
{
  return system_seconds;
}

/*******************************************************************
**                        Timer functions                         **
**          weak definitions to allow user defined replacement    **
*******************************************************************/
__weak void SysTimerWait1us(unsigned long n) __attribute__((optimize("-O0")));
/*!
 * @brief This function will wait for about (n) microseconds.
 * @remark The function uses a calculated loop and is not precise
 * @param[in] n number of microseconds to wait
 */
__weak void SysTimerWait1us(unsigned long n) {
	n &= 0xFFF;	// limit max 8192 us
	n *= (SystemHFClockGet() >> 20) + 1 ;
//	n /= 3;	// Loop takes at least 3 cycles
	n >>=4;
	while (n--) __nop();
}
/*!
 * @brief Waits for (n) milliseconds using system tick/rtc
 * @param[in] n  number of milliseconds to wait
 */
__weak void SysTimerWait1ms(const unsigned long n)  // Waits for n ms
{
  TIMER_TYPE t;
  SysTimerStart1ms(&t,n);
  while (!SysTimerIsStopped(&t)) __nop();
}

__weak void SysTimerStart1ms(TIMER_TYPE *timer, const unsigned long n)
{
	if (!timer) return;
	if ((RTCC->CTRL & RTCC_CTRL_ENABLE) == 0)
		SystemSetDefaultRTC(false);
	// Make sure system ticks is started at a minimum
	timer->start = SystemGetSystemTicks();
	timer->delay = n;
}

__weak unsigned long SysTimerStop(TIMER_TYPE *timer)
{
	if (SysTimerIsStopped(timer)) return 0;
	unsigned long t = SystemGetSystemTicks() - timer->start;
	timer->delay = 0;
	return t;
}

__weak int SysTimerIsStopped(TIMER_TYPE *timer)
{
	if (!timer) return 1;
	if (timer->delay) {
		register unsigned long elapsedTime = SystemGetSystemTicks();
	    if( elapsedTime < timer->start )
	    { // roll over of the counter
	        if (( elapsedTime + ( 0xFFFFFFFF - timer->start )) >= timer->delay) timer->delay = 0;
	    }
	    else
	    {
	        if (( elapsedTime - timer->start) >= timer->delay) timer->delay = 0;
	    }
	}
	return (timer->delay == 0);
}

/*******************************************************************
**                 Reset/Reboot functions                         **
*******************************************************************/
__attribute__((noreturn)) void SystemReboot(void)
{
	__disable_irq();
	__set_BASEPRI(255);
	NVIC_SystemReset();
	__builtin_unreachable();
}

RESETCAUSE SystemRebootCause(void) {
static RESETCAUSE rc = UNKNOWN;
  if (rc == UNKNOWN) {
	  int ResetCause = RMU_ResetCauseGet();
	  RMU_ResetCauseClear();
	  if (ResetCause & RMU_RSTCAUSE_PORST) rc |= POWER_ON;
	  if (ResetCause & (RMU_RSTCAUSE_DVDDBOD | RMU_RSTCAUSE_DECBOD | RMU_RSTCAUSE_AVDDBOD
			  )) rc |= BROWN_OUT;
	  if (ResetCause & (RMU_RSTCAUSE_EXTRST
#ifdef RMU_RSTCAUSE_EM4RST
			  | RMU_RSTCAUSE_EM4RST
#endif
			  )) rc = EXTERNAL;			//< External Pin Reset
	  if (ResetCause & RMU_RSTCAUSE_WDOGRST) rc = WATCHDOG;
	  if (ResetCause & RMU_RSTCAUSE_LOCKUPRST) rc = SYSTEM_LOCKUP;
	  if (ResetCause & RMU_RSTCAUSE_SYSREQRST) rc = SYSTEM_REQUEST;
  }
  return rc;
}

#ifdef WDOG_PRESENT
static const WDOG_Init_TypeDef WDInit = {
			false,              /* Do not start watchdog when init done */                                      \
		    false,              /* WDOG not counting during debug halt */                                \
		    true,               /* WDOG counting when in EM2 */                                      \
		    false,              /* WDOG not counting when in EM3 */                                      \
		    false,              /* EM4 can be entered */                                                 \
		    false,              /* Do not block disabling LFRCO/LFXO in CMU */                           \
		    false,              /* Do not lock WDOG configuration (if locked, reset needed to unlock) */ \
		    wdogClkSelULFRCO,   /* Select 1kHZ WDOG oscillator */                                        \
		    wdogPeriod_256k     /* Set longest possible timeout period = about 4 min.*/                                \
		  };
#endif
void SystemWatchDogStart(BOOL enable) {
#ifdef WDOG_PRESENT
	WDOG_Feed();
	WDOG_Init(&WDInit);
	if (enable) SystemWatchDogEnable(enable);
#else
	(void)enable;
#endif
}
void SystemWatchDogEnable(BOOL enable) {
#ifdef WDOG_PRESENT
	WDOG_Enable(enable);
#else
	(void)enable;
#endif
}
void SystemWatchDogFeed(void) {
#ifdef WDOG_PRESENT
	WDOG_Feed();
#endif
}

BOOL SystemInitHardware(CLOCK_SPEED speed, BOOL useLFXO) {
	/* Chip errata */
	CHIP_Init();
	SystemInitGPIO();
	EMU_DCDCPowerOff();	// DCDC is not handled on this Jade
/*
 * Start LF & HF oscillators to allow warmup
 */
	CMU->OSCENCMD = CMU_OSCENCMD_LFRCOEN
	// Start LF Crystal if any
	| ((useLFXO) ? CMU_OSCENCMD_LFXOEN : 0)
	// Start HF Crystal, if any
	| ((speed == EXTSPEED) ? CMU_OSCENCMD_HFXOEN : 0)
	;
	CMU_ClockEnable(cmuClock_HFLE, true);
	SystemSetClockSpeed(speed);
	/* If we use external 32.768 kHz crystal
	 * wait for it to start, and select it.
	 */
	if (useLFXO) {
		for (int i = 0xFFFFFF; i && ((CMU->STATUS & CMU_STATUS_LFXORDY)==0); i--) ;
		if (CMU->STATUS & CMU_STATUS_LFXORDY)
			CMU->OSCENCMD = CMU_OSCENCMD_LFRCODIS;
		else {
			/* Stop Crystal Oscillator */
			CMU->OSCENCMD = CMU_OSCENCMD_LFXODIS;
			return false;
		}
	}
/*
 * Enable LFA & LFB clocks
 */
	CMU->LFACLKSEL &= ~(_CMU_LFACLKSEL_LFA_MASK);
	CMU->LFACLKSEL |= (CMU->STATUS & CMU_STATUS_LFXORDY) ? CMU_LFACLKSEL_LFA_LFXO : CMU_LFACLKSEL_LFA_LFRCO;
	CMU->LFBCLKSEL &= ~(_CMU_LFBCLKSEL_LFB_MASK);
	CMU->LFBCLKSEL |= (CMU->STATUS & CMU_STATUS_LFXORDY) ? CMU_LFBCLKSEL_LFB_LFXO : CMU_LFBCLKSEL_LFB_LFRCO;
	CMU->LFECLKSEL &= ~(_CMU_LFECLKSEL_LFE_MASK);
	CMU->LFECLKSEL |= (CMU->STATUS & CMU_STATUS_LFXORDY) ? CMU_LFECLKSEL_LFE_LFXO : CMU_LFECLKSEL_LFE_LFRCO;
	// Enable clock for CRYOTIMER
	CMU_ClockEnable(cmuClock_CRYOTIMER, false);	// Reset CRYOTIMER
	CMU_ClockEnable(cmuClock_CRYOTIMER, true);
	CRYOTIMER_Init_TypeDef CRYOTIMER_INIT =
	{
	  true,                 /* Start counting when init done.                    */
	  false,                /* Disable CRYOTIMER during debug halt.              */
	  false,                /* Disable EM4 wakeup.                               */
	  cryotimerOscLFXO,     /* Select Low Frequency RC Oscillator.               */
	  cryotimerPresc_32,    /* LF Oscillator frequency divided by 32.            */
	  cryotimerPeriod_4096m, /* Wakeup event after 4096M pre-scaled clock cycles. */
	};
	CRYOTIMER_INIT.osc = (CMU->STATUS & CMU_STATUS_LFXORDY) ?  cryotimerOscLFXO : cryotimerOscLFRCO;
	CRYOTIMER_Init(&CRYOTIMER_INIT);	// Perpetual free run timer
	return true;
}


void SystemInitGPIO(void) {
	// Check if already called
	if (CMU->HFBUSCLKEN0 & CMU_HFBUSCLKEN0_GPIO) return;
	/* Chip errata */
	CHIP_Init();
//	EMU_SetBiasMode(emuBiasMode_1KHz);
	CMU_ClockEnable(cmuClock_HFPER, true);
	CMU_ClockEnable(cmuClock_GPIO, true);
	/*
	 * Enable interrupt in core for even and odd gpio interrupts
	 */
	SystemIRQEnable(GPIO_ODD_IRQn);
	SystemIRQEnable(GPIO_EVEN_IRQn);
}

/*******************************************************************
**                      Battery functions                         **
*******************************************************************/
#if defined( EMU_STATUS_VMONRDY )

__interrupt_handler __attribute__((used)) void EMU_IRQHandler(void) {
	if(EMU_IntGet() & EMU_IF_VMONDVDDFALL) {
		EMU_IntClear(EMU_IFC_VMONDVDDFALL);
		SystemBatteryLowIrq();
	}
}
#endif
void SystemBatterySetDetectionLevel(int voltage_mV) {
#if defined(EMU_STATUS_VMONRDY)
	EMU_VmonInit_TypeDef VMonInit = {                                                                                  \
	  emuVmonChannel_DVDD,                  /* DVDD VMON channel */
	  3200,                                 /* 3.2 V threshold */
	  false,                                /* Don't wake from EM4H on rising edge */
	  false,                                /* Don't wake from EM4H on falling edge */
	  true,                                 /* Enable VMON channel */
	  false                                 /* Don't disable IO0 retention */
	};
	VMonInit.threshold = voltage_mV;
	EMU_VmonInit((EMU_VmonInit_TypeDef*)&VMonInit);
	EMU_IntEnable(EMU_IEN_VMONDVDDFALL);
	SystemIRQEnable(EMU_IRQn);
#endif
}

__weak __interrupt_handler void SystemBatteryLowIrq(void) { }

void SystemBatteryEnableDetection(BOOL enable) {
#if defined(EMU_STATUS_VMONRDY)
	if (enable)
		EMU_IntEnable(EMU_IEN_VMONDVDDFALL);
	else
		EMU_IntDisable(EMU_IEN_VMONDVDDFALL);
	EMU_VmonEnable(emuVmonChannel_DVDD,enable);
#endif
}

BOOL SystemBatteryIsLow(void) {
#if defined(EMU_STATUS_VMONRDY)
	return !(EMU_VmonChannelStatusGet(emuVmonChannel_DVDD));
#else
	return 0;
#endif
}

unsigned long SystemBatteryGetVoltage(void) {
	ADCOpen(0,0,ADCChannelVDD,ADCHighReference,ADCSamplingTime8,15);
	unsigned long value = (ADCConvertOnce(0) * 5000L) / 4096L;
	ADCClose(0,ADCChannelVDD);
	return value;
}

/*******************************************************************
**                  CPU Clock functions                           **
*******************************************************************/
CLOCK_SPEED SystemGetClockSpeed(void)
{
  /* If we run on LF (32.768 kHz) clock, return SUBSPEED */
  if (CMU->STATUS & (CMU_STATUS_LFXOENS | CMU_STATUS_LFRCOENS)) return SUBSPEED;
  /* If we run on external HF crystal, consider it's HISPEED */
  if (CMU->STATUS & CMU_STATUS_HFXOENS) return EXTSPEED;
  /* If we run on HF RC, extract 3 values to consider speed */
  switch(CMU_HFRCOBandGet()) {
  /** 1MHz RC band. */
  case cmuHFRCOFreq_2M0Hz:
  case CMU_HFRCO_MIN: return VERYLOWSPEED;
  /** 7MHz RC band. */
  case cmuHFRCOFreq_4M0Hz:
  case cmuHFRCOFreq_7M0Hz: return LOWSPEED;
  /** 13MHz RC band. */
  case cmuHFRCOFreq_13M0Hz: return MIDSPEED;
  /** 16MHz RC band. */
  case cmuHFRCOFreq_19M0Hz:
  case cmuHFRCOFreq_16M0Hz: return DEFAULTSPEED;
  /** 26MHz RC band. */
  case cmuHFRCOFreq_26M0Hz: return HIGHSPEED;
  /** 32MHz RC band. */
  case CMU_HFRCO_MAX:
  case cmuHFRCOFreq_32M0Hz: return MAXSPEED;
  case cmuHFRCOFreq_UserDefined:  return EXTSPEED;
  }
  return MAXSPEED;
}
unsigned long SystemGetClockFrequency(void) {
	  return SystemHFClockGet();
}

CLOCK_SPEED SystemSetClockSpeed(const CLOCK_SPEED cs)
{
  CLOCK_SPEED oldcs = SystemGetClockSpeed();
  if (cs == oldcs) return cs;
  if (cs != EXTSPEED)
	  CMU_OscillatorEnable(cmuOsc_HFXO, false, false);
  else
	  CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
  switch(cs) {
  case SUBSPEED:
	  /* Check if LF oscillator exists */
	  if (CMU->STATUS & CMU_STATUS_LFXORDY)
		  CMU_ClockSelectSet(cmuClock_HF,cmuSelect_LFXO);
	  else {
		  CMU->OSCENCMD = CMU_OSCENCMD_LFRCOEN;
		  CMU_ClockSelectSet(cmuClock_HF,cmuSelect_LFRCO);
	  }
	  break;
  case VERYLOWSPEED:
	  CMU_HFRCOBandSet(CMU_HFRCO_MIN);
	  if (!(CMU->STATUS & CMU_STATUS_HFRCOENS)) CMU_ClockSelectSet(cmuClock_HF,cmuSelect_HFRCO);
	  break;
  case LOWSPEED:
	  CMU_HFRCOBandSet(cmuHFRCOFreq_7M0Hz);
	  if (!(CMU->STATUS & CMU_STATUS_HFRCOENS)) CMU_ClockSelectSet(cmuClock_HF,cmuSelect_HFRCO);
	  break;
  case MIDSPEED:
	  CMU_HFRCOBandSet(cmuHFRCOFreq_13M0Hz);
	  if (!(CMU->STATUS & CMU_STATUS_HFRCOENS)) CMU_ClockSelectSet(cmuClock_HF,cmuSelect_HFRCO);
	  break;
  default:
	  CMU_HFRCOBandSet(cmuHFRCOFreq_16M0Hz);
	  if (!(CMU->STATUS & CMU_STATUS_HFRCOENS)) CMU_ClockSelectSet(cmuClock_HF,cmuSelect_HFRCO);
	  return DEFAULTSPEED;
	  /* no break */
  case DEFAULTSPEED:
	  CMU_HFRCOBandSet(cmuHFRCOFreq_16M0Hz);
	  if (!(CMU->STATUS & CMU_STATUS_HFRCOENS)) CMU_ClockSelectSet(cmuClock_HF,cmuSelect_HFRCO);
	  break;
  case HIGHSPEED:
	  CMU_HFRCOBandSet(cmuHFRCOFreq_26M0Hz);
	  if (!(CMU->STATUS & CMU_STATUS_HFRCOENS)) CMU_ClockSelectSet(cmuClock_HF,cmuSelect_HFRCO);
	  break;
  case EXTSPEED:
	  if (CMU->STATUS & CMU_STATUS_HFXORDY) {
		  CMU_ClockSelectSet(cmuClock_HF,cmuSelect_HFXO);
		  break;
	  }
	  /* no break */
  case MAXSPEED:
	  CMU_HFRCOBandSet(CMU_HFRCO_MAX);
	  if (!(CMU->STATUS & CMU_STATUS_HFRCOENS)) CMU_ClockSelectSet(cmuClock_HF,cmuSelect_HFRCO);
	  break;
  }
  return oldcs;
}
int SystemGetFlashSize(void) {
	return (int)SYSTEM_GetFlashSize();
}
int SystemGetRAMSize(void) {
	return(int)SYSTEM_GetSRAMSize();
}
int SystemGetNVRAMSize(void) {
#if defined(RTCC_PRESENT)
	return 32;
#endif
	return 0;
}
void SystemSetNVRAMValue(int index, int value) {
#if defined(RTCC_PRESENT)
	RTCC->RET[index & 0x1F].REG = value;
#endif
}
int SystemGetNVRAMValue(int index) {
#if defined(RTCC_PRESENT)
	return RTCC->RET[index & 0x1F].REG;
#else
	return 0;
#endif
}


unsigned char* SystemGetModel(unsigned char* buffer) {
static const char EFMModel[] = {' ','G','T','L','W','Z','H','J','P','W','L','H'};
	if (buffer) {
	unsigned short model = ((((DEVINFO->PART & _DEVINFO_PART_DEVICE_FAMILY_MASK) >> _DEVINFO_PART_DEVICE_FAMILY_SHIFT) - _DEVINFO_PART_DEVICE_FAMILY_G));
	sprintf((char*)buffer,"%s32%cG%dF%d",(model > 8) ? "EZR" : "EFM",
		EFMModel[(model > 8) ? (model - 40) : model],
		(int)(DEVINFO->PART & _DEVINFO_PART_DEVICE_NUMBER_MASK),
		SystemGetFlashSize()
		);
	}
	return buffer;	// return buffer address for convenience
}

unsigned long long SystemGetSerialNumber(void) {
	return SYSTEM_GetUnique();
}

static int _randomValue = 1;
void SystemRandSeed(int seed) {
	_randomValue = seed;
}
int SystemRandom(void) {
	/* RAND_MAX assumed to be 32767 */
	_randomValue = _randomValue * 1103515245 + 12345;
	return((unsigned)(_randomValue/65536) % 32768);
}
unsigned char SystemRand(const unsigned char MaxValue)
{
	unsigned char result;
	_randomValue = (int)(_micro_seconds) +1;
	do {
		_randomValue = _randomValue * 1103515245 + 12345;
		result = (unsigned char)((unsigned)(_randomValue/65536) % 32768);
	} while (result > MaxValue);
	return result;
}

/*******************************************************************
**                      Event functions                           **
*******************************************************************/

unsigned long SystemWaitForFunction(SYSWAITFUNCTION_DELEGATE func, unsigned long n)
{
  if (func == (SYSWAITFUNCTION_DELEGATE)0) return 0;
  if ((*func)()) return n;
  TIMER_TYPE t;
  SysTimerStart1ms(&t,n);
  do
  {
	if ((*func)()) {
		n = SysTimerStop(&t);
		return (n) ? n : 1;
	}
  } while (!SysTimerIsStopped(&t));
  return SysTimerStop(&t);
}

/*******************************************************************
**                       GPIO functions                           **
*******************************************************************/
void SystemDefinePort(const SystemPort port)
{
  SystemSetPortMode(port, port.mode);
}

void SystemDefinePortRange(const SystemPort *portArray, short len)
{
  SystemPort* port = (SystemPort*)portArray;
  while (IS_SYSTEMPORT_VALID(*port) && (len != 0))
  {
    SystemSetPortMode(*port,port->mode);
    port++;
    if (len > 0) len --;
  }
}

void SystemDefinePorts(const SystemPort *portArray) {
	SystemDefinePortRange(portArray,-1);
}

void SystemSetPortMode(SystemPort port,const PORTMODE mode)
{
  if (IS_SYSTEMPORT_VALID(port) && GPIO_PORT_PIN_VALID(port.port,port.pin))
  {
	if (mode == PortDisabled) {
	      GPIO_PinModeSet(port.port,port.pin,gpioModeDisabled,0);
	}
	else if ((mode == PortIn) || (mode == PortInUp) || (mode == PortInDown))
    {
      GPIO_PinModeSet(port.port,port.pin,(mode == PortIn) ? gpioModeInput : gpioModeInputPull,(mode == PortInUp));
    }
    else if ((mode == PortOut0) || (mode == PortOut1))
    {
      GPIO_PinModeSet(port.port,port.pin,gpioModePushPull,(mode == PortOut1));
    }
    else if ((mode == PortDrainOpen) || (mode == PortDrainClose))
    {
      GPIO_PinModeSet(port.port,port.pin,gpioModeWiredAnd,(mode == PortDrainOpen));
    }
    else if (mode == PortSourceOpen) {
      GPIO_PinModeSet(port.port,port.pin,gpioModeWiredOr,0);
    }
  }
}

void SystemDefinePortIrq(const SystemPort port, GPIOIRQ IRQMode) {
  if (IS_SYSTEMPORT_VALID(port) && GPIO_PORT_PIN_VALID(port.port,port.pin)) {
	if (IRQMode != GPIOIRQNone) {
		GPIO_IntClear(1 << (port.pin));
	}
	GPIO_ExtIntConfig(port.port, port.pin, port.pin, ((IRQMode & GPIOIRQRising) != 0), ((IRQMode & GPIOIRQFalling) != 0), (IRQMode != GPIOIRQNone));
  }
}

SYSTEMPORT_IRQHANDLER SystemDefinePortIrqHandler(const SystemPort port, SYSTEMPORT_IRQHANDLER handler, GPIOIRQ IRQMode) {
	if (IS_SYSTEMPORT_VALID(port) && GPIO_PORT_PIN_VALID(port.port,port.pin)) {
		SYSTEMPORT_IRQHANDLER old = SystemIrqs[port.pin];
		SystemIrqs[port.pin] = handler;
		SystemDefinePortIrq(port,IRQMode);
		return old;
	}
	return NULL;
}

PORTMODE SystemGetPortMode(const SystemPort port)
{
  if (IS_SYSTEMPORT_VALID(port) && GPIO_PORT_PIN_VALID(port.port,port.pin)) {
    /* There are two registers controlling the pins for each port. The MODEL
     * register controls pins 0-7 and MODEH controls pins 8-15. */
    unsigned short mode = (port.pin < 8) ? ((GPIO->P[port.port].MODEL & (0xF << (port.pin * 4))) >> (port.pin *4)) 
      : ((GPIO->P[port.port].MODEH & (0xF << ((port.pin-8) * 4))) >> ((port.pin-8) * 4));
      switch(mode) {
      case gpioModeDisabled: 		return PortDisabled;
      case gpioModeInputPullFilter:
      case gpioModeInputPull: 		return (SystemGetPortState(port)) ? PortInUp : PortInDown;
      case gpioModeInput: 			return PortIn;
      case gpioModeWiredAnd: 		return (SystemGetPortState(port)) ? PortDrainOpen : PortDrainClose;
      case gpioModeWiredOr: 		return PortSourceOpen;
      case gpioModePushPull:		return (SystemGetPortState(port)) ? PortOut1 : PortOut0;
      }
  }
  return PortDisabled;
}

void SystemSetPortState0(SystemPort p) 	{ GPIO_PinOutClear(p.port,p.pin); }
void SystemSetPortState1(SystemPort p) 	{ GPIO_PinOutSet(p.port,p.pin); }
void SystemTogglePort(SystemPort p)  	{ GPIO_PinOutToggle(p.port,p.pin); }
void SystemSetPortState(SystemPort p, BOOL c) { if ((c)) SystemSetPortState1(p); else SystemSetPortState0(p); }
BOOL SystemGetPortState(SystemPort p)    { return (BOOL)/*GPIO_PinInGet(p.port,p.pin);*/ ((GPIO->P[p.port].DIN >> p.pin) & 1); }

/** }@ */
