/*******************************************************************
**                                                                **
** MCU dependent system functions                                 **
**                                                                **
*******************************************************************/

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <stdbool.h>
/** \addtogroup MMI MyMeterInfo add-on functions
 * @brief Collection of add-on functions that will fit with any used hardware
 *  @{
 */

//! @def Macro to declare an IRQ handler function
#ifndef __interrupt_handler
#define __interrupt_handler	__attribute__((interrupt))
#endif

//! @def Macro to declare a weak function definition
#ifndef __weak
#define __weak 	__attribute__((weak))
#endif

//! @def Macro to define a variable alignment
#ifndef __align
#define __align(c) 	__attribute__((align(c)))
#endif

//! @def Macro to declare a packed structure or union
#ifndef __packed__
#define __packed__ 	__attribute__((packed))
#endif

//! @def Macro to define system wide BOOL equivalent
#define BOOL int

#ifndef FALSE
#define FALSE 0				//!< FALSE value definition
#endif

#ifndef TRUE
#define TRUE 1				//!< TRUE value definition
#endif

#ifndef false
#define false 0				//!< false value definition
#endif

#ifndef true
#define true 1				//!< true value definition
#endif

#ifndef NULL
#define NULL  ((void*)0)	//!< NULL pointer definition
#endif

//! @def Macro to determine the minimum of two values
#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
//! @def Macro to determine the maximum of two values
#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef __nop
/*!
 * @brief system wide nop (No OPeration) defintion
 */
static inline void __nop(void) {
	__asm__ volatile ("nop");
}
#endif

//! @def Macro to set a bit at a specific rank
#define SYSTEMBITMASK(b)	(0x01 << (b))

/*!
 * @brief Abstraction of GPIO port functionality
 */
typedef enum
{
  PortOut0 = 0,         //!< Output mode, set to low level
  PortOut1 = 1,         //!< Output mode, set to high level
  PortIn = 2,           //!< Input mode without pull up resistor
  PortInUp = 3,         //!< Input mode with pull up resistor
  PortDrainOpen = 4,    //!< Open Collector mode, (Low level) = blocked
  PortDrainClose = 5,   //!< Open Collector mode, (High Level) = conduct
  PortSourceOpen = 6,	//!< Open Source mode
  PortInDown = 7,  		//!< Input mode with pull down resistor
  PortDisabled = 254,	//!< Port in Tri-State mode
  PortInvalid = 255		//!< Invalid port mode
} PORTMODE;

/*!
 * @brief Abstraction of GPIO IRQ level detection
 */
typedef enum {
	GPIOIRQNone = 0,   //!< No IRQ detection
	GPIOIRQFalling = 1,//!< IRQ Falling edge detection
	GPIOIRQRising = 2, //!< IRQ Rising edge detection
	GPIOIRQBoth = 3    //!< IRQ Both rising and falling edge detection
}GPIOIRQ;

/*!
 * @brief Abstraction of GPIO Port names
 * @remark Depending of microcontroller, some of these ports can be missing
 */
typedef enum
{
  GPIOPortA = 0,	//!< GPIOPortA
  GPIOPortB = 1,	//!< GPIOPortB
  GPIOPortC = 2,	//!< GPIOPortC
  GPIOPortD = 3,	//!< GPIOPortD
  GPIOPortE = 4,	//!< GPIOPortE
  GPIOPortF = 5,	//!< GPIOPortF
  GPIOPortG = 6,	//!< GPIOPortG
  GPIOPortH = 7,	//!< GPIOPortH
  GPIOPortI = 8,	//!< GPIOPortI
  GPIOPortJ = 9 	//!< GPIOPortJ
} GPIOPORT;

/*!
 * @brief Abstraction of GPIO port descriptor
 */
typedef struct  {
GPIOPORT port;			//!< GPIO port name
unsigned long pin;		//!< GPIO port pin definition
PORTMODE mode;			//!< GPIO port pin mode
} SystemPort;

//! @brief Macro defining an invalid port descriptor
//! @remark This is used in SystemPort lists as a terminator
#define SYSTEMPORT_INVALID      	{(GPIOPORT)0, 255, PortInvalid }

//! @def Macro to check whether a SystemPort descriptor represents a valid port
#define IS_SYSTEMPORT_VALID(p)  	(((p).pin < 16) && ((p).mode != PortInvalid))
//! @def Macro to check whether a SystemPort descriptor represents an invalid port
#define IS_SYSTEMPORT_INVALID(p)  	(((p).pin > 15) || ((p).mode == PortInvalid))

/*!
 * @brief Abstraction of system main clock speed
 */
typedef enum {
  SUBSPEED,    		//!< SUBSPEED - 32.768 kHz used as main clock
  VERYLOWSPEED,		//!< VERYLOWSPEED
  LOWSPEED,    		//!< LOWSPEED
  MIDSPEED,    		//!< MIDSPEED
  DEFAULTSPEED,		//!< DEFAULTSPEED
  HIGHSPEED,   		//!< HIGHSPEED
  MAXSPEED,    		//!< MAXSPEED
  EXTSPEED     		//!< EXTSPEED - Main clock is driven by external crystal
} CLOCK_SPEED;

/*!
 * @brief Enables an interrupt number in the ARM interrupt controller (a.k.a. NVIC)
 * @param[in] IRQn IRQ number to enable
 * @remark This function will also set IRQ priority to a lower level to cope with FreeRTOS IRQ management
 */
void SystemIRQEnable(int IRQn);
/*!
 * @brief Disable system wide IRQ
 * @remark This function has a weak definition to be replaced by user if need be.
 * @remark The function is reentrant and keeps track of number of calls
 */
__weak void SystemIrqDisable(void);
/*!
 * @brief Enable system wide IRQ
 * @remark This function has a weak definition to be replaced by user if need be.
 * @remark The function is reentrant and keeps track of number of calls
 */
__weak void SystemIrqEnable(void);

// RTC IRQ Handling functions
// Internal RTC_IRQ taking RTC register ticks as a parameter to convert
// them into nanoseconds/milliseconds/seconds internal for all timer
// related functions
/*!
 * @brief Internal RTC_IRQ taking RTC register ticks as a parameter to convert
 * them into nanoseconds/milliseconds/seconds internal for all timer
 * related functions
 * @param[in] RTCTicks Number of ticks elapsed since last call
 */
void INTRTC_IRQHandler(unsigned long RTCTicks);

//! @def Number of system ticks per second
#define SYSTEM_TICKS_PER_SECOND 1000
/*!
 * @brief Get the number of elapsed system ticks since system start up
 * @return number of elapsed system ticks
 */
extern unsigned long SystemGetSystemTicks(void);
/*!
 * @brief Get the number of elapsed seconds since system start up
 * @return number of elapsed seconds
 */
extern unsigned long SystemGetSystemSeconds(void);
/*!
 * @brief Get the number of microseconds elapsed between two system ticks
 * @return number of pending microseconds
 */
extern unsigned long SystemGetMicroSeconds(void);

/*******************************************************************
**                 Reset/Reboot functions                         **
*******************************************************************/
/*!
 * @brief List of possible system reset causes
 */
typedef enum {
	UNKNOWN 		= 0x00,     //!< UNKNOWN - Unknown or undefined cause
	POWER_ON 		= 0x01,    	//!< POWER_ON - System POR (Power On Reset)
	BROWN_OUT 		= 0x02,   	//!< BROWN_OUT - Power supply default detected
	EXTERNAL 		= 0x04,    	//!< EXTERNAL - Reset external pin request
	WATCHDOG 		= 0x08,    	//!< WATCHDOG - Reset triggered by Watchdog timer
	SYSTEM_LOCKUP 	= 0x10,		//!< SYSTEM_LOCKUP - ARM Core lockup detected
	SYSTEM_REQUEST	= 0x20,		//!< SYSTEM_REQUEST - ARM NVIC software request
	SYSTEM_BACKUP	= 0x40, 	//!< SYSTEM_BACKUP - System Backup Power request
}RESETCAUSE;

/*!
 * @brief Request a system reset using the ARM NVIC registers
 */
__attribute__((noreturn)) void SystemReboot(void);
/*!
 * @brief Get the last system reset cause
 * @return a reset cause enumeration value
 */
RESETCAUSE SystemRebootCause(void);

/*!
 * @brief Power up and initialize GPIO subsystem
 */
void SystemInitGPIO(void);
/*!
 * @brief Power up system clocks subsystems and initialize hardware dependent registers
 * @param[in] speed		Core Clock speed to use
 * @param[in] useLFXO	Flag to indicate to use external 32.786 kHz crystal (if true)
 * @return true if hardware initialization was successful
 */
BOOL SystemInitHardware(CLOCK_SPEED speed,BOOL useLFXO);
/*!
 * @brief Start and initialize Watchdog function
 * @param[in] enable true to enable the Watchdog timer.
 */
void SystemWatchDogStart(BOOL enable);
/*!
 * @brief Enable/Disable Watchdog function
 * @param[in] enable true to enable the Watchdog timer.
 */
void SystemWatchDogEnable(BOOL enable);
/*!
 * @brief Reset Watchdog timer
 * @remark This function shall be call regularly to prevent a system reset
 */
void SystemWatchDogFeed(void);


/*******************************************************************
**                    Battery functions                           **
*******************************************************************/
/*!
 * @brief Set battery low detection level in mV
 * @param[in] voltage_mV
 * @remark When this level is detected, the SystemBatteryLowIrq is triggered
 */
void SystemBatterySetDetectionLevel(int voltage_mV);
/*!
 * @brief Enable/Disable battery low detection
 * @param enable true to enable detection
 */
void SystemBatteryEnableDetection(BOOL enable);
/*!
 * @brief Check whether battery voltage is lower than detection level
 * @return true if battery voltage is lower than detection level
 */
BOOL SystemBatteryIsLow(void);
/*!
 * @brief Get battery voltage in mV
 * @return Battery voltage in mV
 */
unsigned long SystemBatteryGetVoltage(void);
/*!
 * @brief External user defined Battery Low exception IRQ handler
 * @remark A default handler is defined as a weak defined function
 */
extern void SystemBatteryLowIrq(void);

/*******************************************************************
**                      Event functions                           **
*******************************************************************/
// Wait n ms for an event.
// Returns 0 if timeout, or the remaining time.
//unsigned long SystemWaitForEvent(const SystemPort port, const unsigned long n, const BOOL eventLevel);  // Waits for n ms or bit set/reset
// Waits for a function to return true

/*!
 * @brief Prototype of function wait delegate
 * @return true if condition is met and wait loop shall be left
 */
typedef BOOL (*SYSWAITFUNCTION_DELEGATE)(void);

/*!
 * @brief Wait for n milliseconds for delegate function fn to return true
 * @param[in] fn 	Delegate function that returns true when waiting loop shall be left
 * @param[in] n		Number of milliseconds to wait for delegate function to return true
 * @return the number of remaining milliseconds when delegate function returned true, 0 if timeout
 */
unsigned long SystemWaitForFunction(SYSWAITFUNCTION_DELEGATE fn, unsigned long n);

/*******************************************************************
**                       GPIO functions                           **
*******************************************************************/
/*!
 * @brief User defined GPIO IRQ exception delegate
 * @param[in] pin Pin that generated the exception
 */
typedef void (*SYSTEMPORT_IRQHANDLER)(int pin);
/*!
 * @brief Set a GPIO port pin to low level
 * @param[in] p SystemPort GPIO descriptor
 */
extern void SystemSetPortState0(SystemPort p);
/*!
 * @brief Set a GPIO port pin to high level
 * @param[in] p SystemPort GPIO descriptor
 */
extern void SystemSetPortState1(SystemPort p);
/*!
 * @brief Toggle a GPIO port pin level
 * @param[in] p SystemPort GPIO descriptor
 */
extern void SystemTogglePort(SystemPort p);
/*!
 * @brief Set a GPIO port pin to logical level c
 * @param[in] p SystemPort GPIO descriptor
 * @param[in] c level to set
 */
extern void SystemSetPortState(SystemPort p, BOOL c);
/*!
 * @brief Return a GPIO port pin level
 * @param[in] p SystemPort GPIO descriptor
 * @return true if pin level is high
 */
extern BOOL SystemGetPortState(SystemPort p);
/*!
 * @brief Define a GPIO port pin according to descriptor contents
 * @param[in] p SystemPort GPIO descriptor
 */
extern void SystemDefinePort(const SystemPort p);
/*!
 * @brief Set a GPIO port pin IRQ detection
 * @param[in] p SystemPort GPIO descriptor
 * @param IRQMode pin level IRQ detection
 */
void SystemDefinePortIrq(const SystemPort p, GPIOIRQ IRQMode);
/*!
 * @brief Set a GPIO port pin IRQ detection and IRQ handler delegate
 * @param[in] p SystemPort GPIO descriptor
 * @param[in] handler User defined IRQ handler delegate
 * @param[in] IRQMode pin level IRQ detection
 * @return Previous IRQ handler delegate
 */
SYSTEMPORT_IRQHANDLER SystemDefinePortIrqHandler(const SystemPort p, SYSTEMPORT_IRQHANDLER handler, GPIOIRQ IRQMode);
/*!
 * @brief define a GPIO port pin overriding descriptor contents
 * @param[in] p SystemPort GPIO descriptor
 * @param[in] mode GPIO pin mode to be set
 */
void SystemSetPortMode(SystemPort p,const PORTMODE mode);
/*!
 * @brief Get current GPIO port pin mode
 * @param[in] p SystemPort GPIO descriptor
 * @return current GPIO pin mode
 * @remark This function reads real current GPIO pin mode, it does not use descriptor value
 */
PORTMODE SystemGetPortMode(const SystemPort p);
/*!
 * @brief Set a list of GPIO pin mode according to their descriptor contents
 * @param[in] portArray  Array of SystemPort GPIO descriptors
 * @param[in] len	Number of ports to define. Use -1 to perform the complete list (terminated with SYSTEMPORT_INVALID constant)
 */
void SystemDefinePortRange(const SystemPort *portArray, short len);
/*!
 * @brief Set a list of GPIO pin mode according to their descriptor contents
 * @param[in] portArray  Array of SystemPort GPIO descriptors (terminated with SYSTEMPORT_INVALID constant)
 */
void SystemDefinePorts(const SystemPort *portArray);

// System Timer functions
/*!
 * @brief System Timer variable type
 */
typedef struct {
	unsigned long start;	//!< Timer starting point
	unsigned long delay;	//!< Timer expected duration
}TIMER_TYPE;
/*!
 * @brief Wait for n microseconds
 * @param[in] n Number of microseconds to wait for
 * @remark This function does an approximation loop that is limited to 8192 µs
 * @remark This function has a weak definition to allow user defined replacement
 */
__weak void SysTimerWait1us(unsigned long n);
/*!
 * @brief Wait for n milliseconds
 * @param[in] n Number of milliseconds to wait for
 * @remark This function uses System Tick values based upon RTC (Real Time Clock)
 * @remark This function has a weak definition to allow user defined replacement
 */
__weak extern void SysTimerWait1ms(const unsigned long n);
/*!
 * @brief Starts a system timer for n milliseconds
 * @param[in] timer Reference to a timer variable
 * @param[in] n Number of milliseconds to wait for
 * @remark This function uses System Tick values based upon RTC (Real Time Clock)
 * @remark This function has a weak definition to allow user defined replacement
 */
__weak extern void SysTimerStart1ms(TIMER_TYPE*timer, const unsigned long n);
/*!
 * @brief Stop a system timer
 * @param[in] timer Reference to a timer variable
 * @return The remaining milliseconds before timer expiration
 * @remark This function uses System Tick values based upon RTC (Real Time Clock)
 * @remark This function has a weak definition to allow user defined replacement
 */
__weak extern unsigned long SysTimerStop(TIMER_TYPE* timer);
/*!
 * @brief Check if a system timer has expired
 * @param[in] timer Reference to a timer variable
 * @return true if timer expired
 * @remark This function uses System Tick values based upon RTC (Real Time Clock)
 * @remark This function has a weak definition to allow user defined replacement
 */
__weak extern int SysTimerIsStopped(TIMER_TYPE* timer);

// System processor speed handling

/*!
 * @brief Get abstracted core clock speed
 * @return An abstracted core clock speed enum constant
 */
CLOCK_SPEED SystemGetClockSpeed(void);  // Get current uC clock speed
/*!
 * @brief Set system core clock speed
 * @param[in] s  Abstracted core clock speed value
 * @return previous abstracted core clock speed enum constant
 */
CLOCK_SPEED SystemSetClockSpeed(const CLOCK_SPEED s);
/*!
 * @brief Get actual System core clock speed in Hz
 * @return Core clock speed in Hz
 */
unsigned long SystemGetClockFrequency(void);
/*!
 * @brief Get current System Flash Memory size
 * @return Flash Memory size in bytes
 */
int SystemGetFlashSize(void);
/*!
 * @brief Get current System RAM memory size
 * @return RAM memory size in bytes
 */
int SystemGetRAMSize(void);
/*!
 * @brief Get the non-volatile RAM size (a.k.a. not zeroed by a reset, but zeroed by battery failure)
 * @return non-volatile RAM size in bytes
 */
int SystemGetNVRAMSize(void);
/*!
 * @brief Store a value in non-volatile RAM
 * @param[in] index Index of non-volatile RAM cell to fill
 * @param[in] value Value to store in non-volatile RAM
 */
void SystemSetNVRAMValue(int index, int value);
/*!
 * @brief Read a value from non-volatile RAM
 * @param[in] index Index of non-volatile RAM cell to read
 * @return Value retrieved in non-volatile RAM
 */
int SystemGetNVRAMValue(int index);
/*!
 * @brief Get microcontroller model in use
 * @param[in] buffer String buffer to fill with model name
 * @return pointer to the provided string buffer (for convenience)
 * @remark the returned value will be of the form "EFM32JG100F128" for instance
 */
unsigned char* SystemGetModel(unsigned char* buffer);
/*!
 * @brief Return microcontroller factory unique serial number
 * @return Microcontroller factory unique serial number as a 64 bits integer
 */
unsigned long long SystemGetSerialNumber(void);

/*!
 * @brief Give a starting point for the pseudo random generator
 * @param[in] seed Integer that will initiate the pseudo random generator
 */
void SystemRandSeed(int seed);
/*!
 * @brief Get a random number from the pseudo random generator
 * @return A number from 0 to 32767
 */
int SystemRandom(void);	// A number from 0 to 32767
/*!
 * @brief Get a random number from the pseudo random generator, limited to MaxValue
 * @param[in] MaxValue Maximum value for the result number
 * @return A number from 0 to MaxValue
 */
unsigned char SystemRand(const unsigned char MaxValue);

/** }@ */

#endif
