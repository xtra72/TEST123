/*******************************************************************
**                                                                **
** Device Hardware Definitions                                    **
**                                                                **
*******************************************************************/
#ifndef __DEVICE_DEF_H__
#define __DEVICE_DEF_H__

#include <system.h>
#include <FreeRTOS.h>
#include <mcu_rtc.h>

/** \addtogroup HAL Hardware Abstraction Layer
 *  @{
 */

extern volatile unsigned long DeviceStatus;
/*!
 * @brief Generic LIST_INDEX typedef used by device peripherals
 */
#define LIST_INDEX unsigned char
#define DEVICE_PULSE_OFF		0x0100	//!< When this flag is on, no PULSE_EVENT is sent to the queue

/** List of device status word flags */
#define DEVICE_UNINSTALLED		0x0080	//!< Device not installed status flag
#define DEVICE_COMM_ERROR		0x0040	//!< Device no communication error status flag
#define DEVICE_ERROR			0x0020	//!< Generic error status flag
#define DEVICE_TEMPORARY_ERROR	0x0010	//!< Temporary error status flag
#define DEVICE_PERMANENT_ERROR	0x0008	//!< Permanent error status flag
#define DEVICE_LOW_BATTERY		0x0004	//!< Device Low Battery error status flag

/** Flash LED error codes */
#define DEVICE_MEMORY_ERROR			1	//!< Device Memory Error flash code @hideinitializer
#define DEVICE_STACK_ERROR			2	//!< Stack Overflow Error flash code @hideinitializer
#define DEVICE_LFXO_ERROR			3	//!< Low Frequency Crystal start error flash code @hideinitializer
#define DEVICE_RF_ERROR				4	//!< Communication error with radio transceiver @hideinitializer

//! @brief Macro to set a status flag in the DeviceStatus variable
#define SET_FLAG(f)		DeviceStatus |= (f)
//! @brief Macro to clear a status flag in the DeviceStatus variable
#define CLEAR_FLAG(f)	DeviceStatus &= ~(f)
//! @brief Macro to get a status flag value from the DeviceStatus variable
#define GET_FLAG(f)		(DeviceStatus & (f))
//! @brief Macro to set/clear a status flag in the DeviceStatus variable depending of a boolean parameter
#define UPDATE_FLAG(f,b)	DeviceStatus = (b) ? (DeviceStatus & ~(f)) : (DeviceStatus | (f))

/**
 * LED management values
 */
#define LED_FLASH_ON			-1		//!< Set LED steady ON @hideinitializer
#define LED_FLASH_TOGGLE		-2		//!< Toggle LED	@hideinitializer
#define LED_FLASH_OFF			0		//!< Set LED OFF @hideinitializer
#define LED_FLASH_DEFAULT		0		//!< Default LED state @hideinitializer
#define LED_FLASH_SHORTER		100		//!< Very Short LED flash duration @hideinitializer
#define LED_FLASH_SHORT			50		//!< Short LED flash duration @hideinitializer
#define LED_FLASH_NORMAL		10		//!< Medium LED flash duration @hideinitializer
#define LED_FLASH_LONG			5		//!< Long LED flash duration @hideinitializer
#define LED_FLASH_LONGER		1		//!< Very Long flash duration @hideinitializer

#define LED0					0		//!< Alias to main LED @hideinitializer

/*!
 * @brief System Event typedef
 */
typedef enum {
	IDLE_EVENT 		= 0,	//!< No pending event @hideinitializer
	BUTTON_EVENT,			//!< Button was pressed - From IRQ @hideinitializer
	PERIODIC_EVENT,			//!< Periodic timer fired - Manually set @hideinitializer
	PERIODIC_RESEND,		//!< Resend last recorded values - Manually set @hideinitializer
	RF_INDICATION,			//!< A LoRaWAN INDICATION event occurred - Manually set @hideinitializer
	RF_MLME,				//!< A LoRaWAN MLME event occurred - Manually set @hideinitializer
	RF_ERROR_EVENT,			//!< An RF error situation occurred - Manually set @hideinitializer
	PULSE_EVENT,			//!< A pulse input was detected - From IRQ @hideinitializer
	BATTERYLOW_EVENT,		//!< A Battery Low condition has been detected - from IRQ @hideinitializer
	POWER_FAIL_EVENT,		//!< A Power Fail condition has been detected - from IRQ @hideinitializer
	ERROR_EVENT,			//!< An error situation occurred - Manually set @hideinitializer
	RESET_EVENT,			//!< Reset Unit requested - Manually set @hideinitializer
	RELOAD_EVENT,			//!< Unit reload parameters - Manually set @hideinitializer
	USER_EVENT,				//!< User defined event @hideinitializer
	RUN_ATTACH,
	RUN_ATTACH_USE_OTTA,
	RUN_ATTACH_USE_ABP,
	PSEUDO_JOIN_NETWORK,
	PSEUDO_JOIN_NETWORK_COMPLETED,
	REAL_JOIN_NETWORK,
	REAL_JOIN_NETWORK_COMPLETED,
	REQ_REAL_APP_KEY_ALLOC,
	REAL_APP_KEY_ALLOC_COMPLETED,
	REQ_REAL_APP_KEY_RX_REPORT,
	REAL_APP_KEY_RX_REPORT_COMPLETED,
	JOIN_COMPLETED,
	SYSTEM_RESET,
	RUN_TEST_EVENT,
	UNKNOWN_EVENT	= 255	//!< Undefined event @hideinitializer
} EVENT_TYPE;

/* User data management */
#define USERPAGE    0x0FE00000 	//!< Address of the user page in FLASH memory @hideinitializer
/** @cond */
#define USERPAGEPTR ((const USERDATA*)0x0FE00000)
/** @endcond */
typedef struct __packed__ {
unsigned long Region;				//!< LoRaWAN Region ID
unsigned long NetID;				//!< LoRaWAN Network ID
unsigned char DevEui[8];			//!< LoRaWAN Device Unique ID
unsigned char AppEui[8];			//!< LoRaWAN Application Unique ID
unsigned char AppKey[16];			//!< LoRaWAN Encryption Key
unsigned char NwkSKey[16];			//!< LoRaWAN Network Shared Encryption Key
unsigned char AppSKey[16];			//!< LoRaWAN Application Shared Encryption Key
}LORAWAN_INFO;

typedef	struct __packed__ {
unsigned char RealAppKey[16];			//!< LoRaWAN SKT Real Encryption Key
}SKT_INFO;

/*!
 * @brief User data information block format stored in user page FLASH memory
 * @remark This information will be moved from the USER Page Flash region to the internal Flash for security
 */
typedef struct __packed__ {
unsigned short DataCRC;				//!< Data CRC16 checksum
/*!
 * @brief The device type value identifies the kind of sensor attached to the device.
 * (1 or 2 pulse input(s), etc.)
 * @remark Using alternate messages, if the device type is > 16, the values transmitted are contained
 * in unsigned short values instead of unsigned long to save transmission time.
 */
unsigned char 	DeviceType;
unsigned char 	RunCyclicTask;
unsigned long 	DefaultRFPeriod;		//!< Default RF periodic transmission (in sec.)
unsigned long 	DeviceSerialNumber;	//!< Device Serial Number
unsigned short 	DeviceFlags;			//!< Device permanent stored status flag
unsigned short 	TraceFlags;
LORAWAN_INFO LoRaWAN;				//!< LoRaWAN information
SKT_INFO	SKT;
} USERDATA;

/** @cond */
/*
 * @brief This union defines the hidden reserved Flash page that will contain device information
 */
typedef union {
	unsigned char Flash[2048];
	USERDATA UserData;
}FLASH_PAGE;
extern const FLASH_PAGE _USERPAGE_;
/*
 * Make const USERDATA* volatile to force compiler to reload real data each time and not
 * optimize code assuming the value from the empty initialization
 */
#define USERDATAPTR ((volatile const USERDATA*)&_USERPAGE_.UserData)
/** @endcond */

//! @brief Macro replacement to access permanent data checksum
#define UNIT_CHECKSUM		((const unsigned short)(USERDATAPTR)->DataCRC) 			//! @hideinitializer
//! @brief Macro replacement to access permanent data Device Type
#define UNIT_DEVICETYPE		((const unsigned short)(USERDATAPTR)->DeviceType) 		//! @hideinitializer
//! @brief Macro replacement to access permanent data Serial Number
#define UNIT_SERIALNUMBER	((const unsigned long)(USERDATAPTR)->DeviceSerialNumber) //! @hideinitializer
//! @brief Macro replacement to access permanent data Run Cyclic Task
#define UNIT_RUN			((const unsigned char)(USERDATAPTR)->RunCyclicTask) 	//! @hideinitializer
//! @brief Macro replacement to access permanent data RF Period
#define UNIT_RFPERIOD		((const unsigned long)(USERDATAPTR)->DefaultRFPeriod) 	//! @hideinitializer
//! @brief Macro replacement to access permanent data LoRaWAN Region ID
#define UNIT_REGION			((const unsigned long)(USERDATAPTR)->LoRaWAN.Region)	//! @hideinitializer
//! @brief Macro replacement to access permanent data LoRaWAN Network ID
#define UNIT_NETWORKID		((const unsigned long)(USERDATAPTR)->LoRaWAN.NetID)		//! @hideinitializer
//! @brief Macro replacement to access permanent data Device Unit ID
#define UNIT_DEVEUID		((const unsigned char*)(USERDATAPTR)->LoRaWAN.DevEui) 	//! @hideinitializer
//! @brief Macro replacement to access permanent data Application Unit ID
#define UNIT_APPEUID		((const unsigned char*)(USERDATAPTR)->LoRaWAN.AppEui) 	//! @hideinitializer
//! @brief Macro replacement to access permanent data Application encryption key
#define UNIT_APPKEY			((const unsigned char*)(USERDATAPTR)->LoRaWAN.AppKey) 	//! @hideinitializer
//! @brief Macro replacement to access permanent data Network session encryption key
#define UNIT_NETSKEY		((const unsigned char*)(USERDATAPTR)->LoRaWAN.NwkSKey) 	//! @hideinitializer
//! @brief Macro replacement to access permanent data Application session encryption key
#define UNIT_APPSKEY		((const unsigned char*)(USERDATAPTR)->LoRaWAN.AppSKey) 	//! @hideinitializer
//! @brief Macro replacement to access permanent data Application real encryption key
#define UNIT_REALAPPKEY		((const unsigned char*)(USERDATAPTR)->SKT.RealAppKey) 	//! @hideinitializer

/**
 * Permanently stored status flags
 */
#define FLAG_INSTALLED		0x0001	//!< Device is installed @hideinitializer
#define FLAG_USE_OTAA		0x0002	//!< Device uses OTAA to join LoRaWAN network @hideinitializer
#define FLAG_DISALLOW_RESET	0x0004	//!< Device cannot be reset by user using magnet when set @hideinitializer
#define FLAG_USE_CTM		0x0008	//!< Device is use to cyclic transmission mode.
#define	FLAG_USE_RAK		0x0010	//!< Device is use to real app key.

#define	FLAG_
/** @cond */
#define FLAG_3				0x0020	// to be defined
#define FLAG_4				0x0040	// to be defined
/** @endcond */
/*!
 * @brief Set this flag to use SKT/Daliworks LoRaWAN types of messages. If this flag is set
 * to zero, a default set of messages/commands will be used. this value is copied from the
 * User Data Flash Region upon device reset
 * @remark This will probably be the version used in the rest of the world
 */
#define FLAG_USE_SKT_APP	0x0040
#define FLAG_FACTORY_TEST	0x0080	//!< Device runs factory test

//! @brief Macro to permanently set a flag value in the User Information block
#define SET_USERFLAG(f)		DeviceUserDataSetFlag((f),(f))
//! @brief Macro to permanently clear a flag value in the User Information block
#define CLEAR_USERFLAG(f)	DeviceUserDataSetFlag((f),0)
//! @brief Macro to permanently set a flag value in the User Information block depending on the boolean parameter
#define UPDATE_USERFLAG(f,b)	DeviceUserDataSetFlag((f),(b) ? (f) : 0)


#define UNIT_INSTALLED		((USERDATAPTR)->DeviceFlags & FLAG_INSTALLED)	//!< Device is installed @hideinitializer
#define UNIT_USE_OTAA		((USERDATAPTR)->DeviceFlags & FLAG_USE_OTAA) 	//!< Device uses OTAA to join LoRaWAN network @hideinitializer
#define UNIT_DISALLOW_RESET ((USERDATAPTR)->DeviceFlags & FLAG_DISALLOW_RESET) //!< Device cannot be reset by user @hideinitializer
#define UNIT_USE_SKT_APP	((USERDATAPTR)->DeviceFlags & FLAG_USE_SKT_APP)	//!< Device is using SKT/Daliworks LoRaWAN Application @hideinitializer
#define UNIT_FACTORY_TEST	((USERDATAPTR)->DeviceFlags & FLAG_FACTORY_TEST)//!< Device is in factory test mode @hideinitializer
#define UNIT_CTM_ON			((USERDATAPTR)->DeviceFlags & FLAG_USE_CTM)//!< Device is use to cyclic transmission mode. @hideinitializer
#define UNIT_USE_RAK		((USERDATAPTR)->DeviceFlags & FLAG_USE_RAK)//!< Device is use to real app key @hideinitializer

#define	FLAG_TRACE_ENABLE		0x0001
#define	FLAG_TRACE_DUMP			0x0002
#define	FLAG_TRACE_LORAMAC		0x0010
#define	FLAG_TRACE_LORAWAN		0x0020
#define	FLAG_TRACE_DALIWORKS	0x0040
#define	FLAG_TRACE_SKT			0x0080
#define	FLAG_TRACE_SUPERVISOR	0x0100

//! @brief Macro to permanently set a flag value in the User Information block
#define SET_TRACE_FLAG(f)		DeviceUserDataSetTraceFlag((f),(f))
//! @brief Macro to permanently clear a flag value in the User Information block
#define CLR_RACE_FLAG(f)		DeviceUserDataSetTraceFlag((f),0)
//! @brief Macro to permanently set a flag value in the User Information block depending on the boolean parameter
#define UPDATE_TRACE_FLAG(f,b)	DeviceUserDataSetTraceFlag((f),(b) ? (f) : 0)

#define UNIT_TRACE_ENABLE		((USERDATAPTR)->TraceFlags & FLAG_TRACE_ENABLE)	//!< Device is installed @hideinitializer
#define	UNIT_TRACE_DUMP			((USERDATAPTR)->TraceFlags & FLAG_TRACE_DUMP)
#define	UNIT_TRACE_LORAMAC		((USERDATAPTR)->TraceFlags & FLAG_TRACE_LORAMAC)
#define	UNIT_TRACE_LORAWAN		((USERDATAPTR)->TraceFlags & FLAG_TRACE_LORAWAN)
#define	UNIT_TRACE_DALIWORKS	((USERDATAPTR)->TraceFlags & FLAG_TRACE_DALIWORKS)
#define	UNIT_TRACE_SKT			((USERDATAPTR)->TraceFlags & FLAG_TRACE_SKT)
#define	UNIT_TRACE(f)			((f) && (((USERDATAPTR)->TraceFlags & (f)) == (f)))

/*!
 * @brief Initializes Device Hardware Abstraction Layer
 */
void DeviceInitHardware(void);
/*!
 * @brief Enables/Disables Battery Low detection
 * @param[in] enable	true to enable, false to disable Low Power detection
 */
void DeviceLowBatteyrEnable(BOOL enable);
/*!
 * @brief Checks whether the device is currently under Battery Low condition
 */
BOOL DeviceLowBatteryState(void);
/*!
 * @brief Return current battery voltage in mV
 */
unsigned long DeviceGetBatteryVoltage(void);

/*!
 * @brief Get device current real time counter
 * @return number of elapsed seconds since 01/01/2000
 */
static inline unsigned long DeviceRTCGetSeconds() { return RTCGetSeconds(); }
/*!
 * @brief Set device current real time counter
 * @param[in] time	number of elapsed seconds since 01/01/2000
 */
void DeviceRTCSetSeconds(unsigned long time);
/*!
 * @brief Returns Device Application Name
 * @return a pointer to the application name string
 */
unsigned char* DeviceApplicationID(void);
/*!
 * @brief Get device application version in hexadecimal numeric format
 * @remark The version number is in the form MMmm (MM major version, mm minor version)
 * @remark ex.: 0100 for version 1.00
 * @return version number
 */
unsigned short DeviceVersion(void);
/*!
 * @brief Get device hardware version in hexadecimal numeric format
 * @remark The version number is in the form MMmm (MM major version, mm minor version)
 * @remark ex.: 0100 for version 1.00
 * @return version number
 */
unsigned short DeviceHWVersion(void);
/*!
 * @brief Get device application build number
 * @remark The build is the concatenation of date and time of compilation
 * @return a pointer to the application build string
 */
unsigned char* DeviceApplicationBuild(void);

/* Pulse input handling */
/*!
 * @brief Read an analog value and store it in the pulse value accumulator
 * @param[in] pulse	pulse index to populate
 */
void DeviceCaptureAnalogValue(LIST_INDEX pulse);
/*!
 * @brief enable/disable pulse detection (only for pulse input devices)
 * @param[in] bEnable	true to enable pulse detection, false to disable pulse detection
 */
void DevicePulseInEnable(BOOL bEnable);
/*!
 * @brief get the number of pulse inputs (a.k.a returned values) in the device
 * @return the number of values
 */
unsigned long DeviceGetPulseInNumber(void);
/*!
 * @brief return a pointer to a pulse value accumulator
 * @param[in] pulse pulse index pointer to return
 * @return a pointer to the requested value
 */
unsigned long *DeviceGetPulseInValuePtr(LIST_INDEX pulse);
/*!
 * @brief Return the accumulated (or immediate) value of a pulse input
 * @param[in] pulse pulse index value to return
 * @return The value of the requested input number
 */
unsigned long DeviceGetPulseInValue(LIST_INDEX pulse);
/*!
 * @brief	Set a pulse accumulator value
 * @param[in] pulse pulse index pointer to return
 * @param[in] value value to store
 */
void DeviceSetPulseInValue(LIST_INDEX pulse, unsigned long value);
/*!
 * @brief Get actual pulse input level
 * @param[in] pulse pulse index pointer to return
 * @return true if pulse input is high, false otherwise
 */
BOOL DeviceGetPulseInState(LIST_INDEX pulse);
/*!
 * @brief Check pulse input integrity
 * @note This for instance will check ground loop integrity for pulse input devices
 */
void DevicePulseInCheck(void);

/* Led Handling */
/*!
 * @brief Flash LED0 code times slowly followed by 10 quick flashes to signal a system error
 * @param[in] code	code to display
 */
void DeviceShowErrorCode(int code);
/* Multiple led platform */
/*!
 * @brief Flash a system LED
 * @param[in] led		system LED to flash
 * @param[in] number	number of flashes to execute
 * @param[in] duration	duration of each flash
 */
void DeviceFlashOneLedExt(LIST_INDEX led, short number, short duration);
//! @brief Macro to issue a normal flash to a LED
#define DeviceFlashOneLed(l,n) DeviceFlashOneLedExt((l),(n),LED_FLASH_NORMAL)
/* Single led platform */
//! @brief Macro to flash LED0
#define DeviceFlashLed(n) DeviceFlashOneLedExt(0,(n),LED_FLASH_NORMAL)
//! @brief Macro to get LED0 current state
#define DeviceGetLedState() DeviceGetLedStateExt(0)
/*!
 * @brief Get a system LED current state
 * @param[in] led	led to test
 * @return	true if LED is on, false otherwise
 */
BOOL DeviceGetLedStateExt(LIST_INDEX led);
/*!
 * @brief Return the total number of system LEDs
 * @return number of system LEDs
 */
LIST_INDEX DeviceGetLedNumber(void);

/* UserData Flash Memory handling */
/*!
 * @brief Permanently save User Data to the flash memory
 * @param[in] DataPtr	pointer to the User Data to save
 */
void DeviceUserDataSave(USERDATA* DataPtr);
/**
 * @brief Permanently save unit serial number in flash memory
 * @param[in] serial Serial number to store in flash memory
 */
void DeviceUserDateSetSerialNumber(unsigned long serial);
/*!
 * @brief Permanently save system flag value to the flash memory
 * @param[in] Mask	Mask to apply to the flag value
 * @param[in] Value	Value to set in the flag value
 */
void DeviceUserDataSetFlag(unsigned char Mask, unsigned char Value);
/*!
 * @brief Permanently save default RF period to the flash memory
 * @param Period New default period in minutes
 */
void DeviceUserDataSetRFPeriod(unsigned long Period);

void DeviceUserDataSetAppKey(uint8_t *AppKey) ;
void DeviceUserDataSetAppEUI(uint8_t *AppEUI) ;
void DeviceUserDataSetDevEUI(uint8_t *DevEUI) ;

void DeviceUserDataSetTraceFlag(unsigned char Mask, unsigned char Value);

void DeviceUserDataSetSKTRealAppKey(uint8_t *RealAppKey) ;

/* Buttons handling */

/*!
 * @brief Keypress handling
 */
typedef struct __packed__ {
unsigned short KeypressDuration;	//!< Time to wait for keypress to occur to perform the task
unsigned short LedFlashCycle;		//!< LED flash rate. Use 0 for Fixed
void (*KeypressTask)(void);			//!< Task to perform when keypress is validated
}BUTTON_TASKLIST;
/*!
 * @brief Generic keypress handling
 * @remark Call this function to perform a different task depending on the duration of keypress.
 * @remark The provided Tasklist parameter terminates with a KeypressDuration of 0.
 * @param[in] button 	Button number
 * @param[in] tasks		List of tasks to execute depending on keypress duration
 * @return true if a task has been performed. false if the 1st duration is not met.
 */
BOOL DevicePerformKeypressTasks(LIST_INDEX button,const BUTTON_TASKLIST* tasks);
/*!
 * @brief Checks whether a button is currently pressed
 * @param[in] button	button to check
 * @return	true if button is pressed
 */
BOOL DeviceIsButtonPressed(LIST_INDEX button);
/*!
 * @brief Reset button status
 * @param[in] button	button to reset
 */
void DeviceResetButton(LIST_INDEX button);
/*!
 * @brief Reset all buttons status
 */
void DeviceResetAllButtons(void);
/*!
 * @brief Checks if button is released for at least duration
 * @param[in] button		button to be checked
 * @param[in] duration_ms	duration in ms to wait
 * @return	true if button is still up after duration
 */
BOOL DeviceCheckAlternateButtonIsStillUp(LIST_INDEX button, unsigned short duration_ms);
/*!
 * @brief Checks if button is pressed for at least duration
 * @param[in] button		button to be checked
 * @param[in] duration_ms	duration in ms to wait
 * @return	true if button is still down after duration
 */
BOOL DeviceCheckAlternateButtonIsStillDown(LIST_INDEX button, unsigned short duration_ms);
/*!
 * @brief Simulates a button press by software
 * @param[in] button	button to press
 */
void DevicePressAlternateButton(LIST_INDEX button);
/* Single button platform aliases */
//! @brief Single button alternate function call
#define DeviceCheckButtonIsStillDown(m) DeviceCheckAlternateButtonIsStillDown(0,(m))
//! @brief Single button alternate function call
#define DeviceCheckButtonIsStillUp(m) DeviceCheckAlternateButtonIsStillUp(0,(m))
//! @brief Single button alternate function call
#define DevicePressButton() DevicePressAlternateButton(0)

/*!
 * @brief returns the device battery voltage in mV
 * @return voltage (in mV)
 */
unsigned long DeviceGetBatteryVoltage(void);

/* Event handling */

/*!
 * @brief Waits for a device event
 * @param[in] duration	Duration of device event wait (in system ticks)
 * @return an event value or 0 if no event occurred after duration
 */
EVENT_TYPE DeviceWaitForEvent(portTickType duration);
/*!
 * @brief Sends a event to the event queue back (to be execute after all pending events)
 * @param[in] event type
 */
void DeviceSendEvent(EVENT_TYPE event);
/*!
 * @brief Posts an event to the event queue front (to be executed before any pending events)
 * @param[in] event type
 */
void DevicePostEvent(EVENT_TYPE event);
/*!
 * @brief Posts an event to the event queue front (to be executed before any pending events)
 * @remark Must be used when called from an ISR.
 * @param[in] event type
 */
void DevicePostEventFromISR(EVENT_TYPE event);

/** }@ */

#endif
