/*
 * Hardware Abstraction Layer Definition
 *
 * HAL_def.h
 */
/** \addtogroup S40 S40 Main Application
 *  @{
 */

//! Application Version as string value.
#define _APP_VERSION	"-V 1.00"
//! Application Version as numerical value
#define _NUM_VERSION	0x0100
//! Application name ID
#define APP_ID			"SKT"
//! Hardware version as a numerical value
#define _HW_VERSION		0x0100

//! No main clock external crystal usage
#define HAL_USE_HFXO	0
//! RTC uses external 32.768 kHz crystal
#define HAL_USE_LFXO 	0
//! Define processor speed to use
#define HAL_CPU_SPEED	HIGHSPEED
//! Low battery detection level in mV
#define HAL_VCMP_LEVEL	3000
//! Macro that defines an abstracted port
#define MAKEPORT(n)			HAL[(n)]
//! Macro that defines an array list of abstracted ports
#define MAKEPORTLIST(n)		((SystemPort*)&HAL[(n)])

/**
 * @brief Hardware Abstraction Layer Ports Definition
 * This array of SystemPort types describes all physical connections for all software functions
 * as an abstracted name used throughout the whole application.
 */
extern const SystemPort HAL[];
/** @cond */
#ifdef DEFINE_HAL
const SystemPort HAL[] = {
// For debugging purpose, use of standard MMI RF board
#ifdef USE_MMIPCB
/* 00 */	{GPIOPortC,  7, PortOut0}, 		/* LED 0 */
/* 01 */	{GPIOPortF,  4, PortIn}, 		/* BTN 0 */
/* 02 */	{GPIOPortD, 11, PortDisabled},	/* LOOP0 */
/* 03 */	{GPIOPortD, 10, PortDisabled},	/* LOOP1 */
/* 04 */	{GPIOPortD, 15, PortDisabled}, 	/* IN_0 */
/* 05 */	{GPIOPortD, 14, PortDisabled},	/* IN_1 */
/* 06 */	{GPIOPortA,  0, PortDisabled}, 	/* LEUART 0 Rx  Pulse0 input */
/* 07 */	{GPIOPortA,  1, PortDisabled}, 	/* LEUART 0 Tx  Pulse1 input */
			/* RF CONFIG */
/* 08 */	{GPIOPortB, 13, PortOut0},  	/* SCLK */
/* 09 */	{GPIOPortB, 12, PortInDown},	/* MISO */
/* 10 */	{GPIOPortB, 11, PortOut0},  	/* MOSI */
/* 11 */	{GPIOPortC, 11, PortOut1},  	/* NSS */
/* 12 */	{GPIOPortF,  2, PortIn},		/* DIO0 */
/* 13 */	{GPIOPortF,  3, PortIn}, 		/* DIO1 */
/* 14 */	{GPIOPortC, 10, PortIn}, 		/* DIO2 */
/* 15 */	{GPIOPortD, 13, PortIn}, 		/* DIO3 */
/* 16 */	{GPIOPortC,  9, PortIn}, 		/* DIO4 */
/* 17 */	{GPIOPortD, 12, PortDisabled},	/* DIO5 */
/* 18 */	{GPIOPortC,  8, PortOut1}, 		/* RF_Reset */

			SYSTEMPORT_INVALID,				/* End of List */
#else
// This is a regular S40 PCB A
/* 00 */	{gpioPortC, 8, PortOut0}, 	  	/* LED 0 */
/* 01 */	{gpioPortF, 4, PortIn}, 	  	/* BTN 0 */
/* 02 */	{gpioPortD, 11, PortDisabled}, 	/* LOOP0 */
/* 03 */	{gpioPortD, 10, PortDisabled},	/* LOOP1 */
/* 04 */	{gpioPortD, 15, PortDisabled},	/* IN_0 */
/* 05 */	{gpioPortD, 14, PortDisabled},	/* IN_1 */
/* 06 */	{gpioPortA, 0, PortDisabled}, 	/* LEUART 0 Rx */
/* 07 */	{gpioPortA, 1, PortDisabled}, 	/* LEUART 0 Tx */
	/* RF CONFIG */
/* 08 */	{gpioPortB, 13, PortOut0},  	/* SCLK */
/* 09 */	{gpioPortB, 12, PortInDown},	/* MISO */
/* 10 */	{gpioPortB, 11, PortOut0},  	/* MOSI */
/* 11 */	{gpioPortC, 11, PortOut1},  	/* NSS */
/* 12 */	{gpioPortF,  2, PortIn},  		/* DIO0 */
/* 13 */	{gpioPortF,  3, PortIn},  		/* DIO1 */
/* 14 */	{gpioPortC, 10, PortIn},  		/* DIO2 */
/* 15 */	{gpioPortC,  7, PortIn},  		/* DIO3 */
/* 16 */	{gpioPortC,  9, PortIn},  		/* DIO4 */
/* 17 */	{gpioPortD, 13, PortDisabled},  /* DIO5 */
/* 18 */	{gpioPortD, 12, PortOut1},  	/* RF_Reset */
			SYSTEMPORT_INVALID,				/* End of List */
#endif // USE_MMIPCB
};
#endif // DEFINE_HAL

//! @brief Number of buttons on the board
#define HAL_NB_BUTTON		1
//! @brief Number of LED on the board
#define HAL_NB_LED			1

#define LED   				MAKEPORTLIST(0)
#define BTN   				MAKEPORTLIST(1)
#define LOOP_IN				MAKEPORTLIST(2)
#define PULSE_IN			MAKEPORTLIST(4)

#if (NODE_TEMP > 0)
#define DELAY_READ_DURATION 5	//!< Assume we need less than 5 sec to read sensor value
#define HAL_NB_PULSE_IN		NODE_TEMP
#define HAL_NB_LOOP			0
#define TEMPSENSOR_ENABLE 	MAKEPORTLIST(2)
#define TEMPSENSOR_VALUE 	MAKEPORT(2+NODE_TEMP)
#elif (NODE_HYGRO > 0)
#define DELAY_READ_DURATION 5	//!< Assume we need less than 5 sec to read sensor value
#define HAL_NB_PULSE_IN		2
#define HAL_NB_LOOP			0
#define TEMPSENSOR_ENABLE 	MAKEPORTLIST(2)
#define I2C_SDA				MAKEPORT(5)
#define I2C_SCL				MAKEPORT(3)
#elif (NODE_ANALOG > 0)
#define DELAY_READ_DURATION 2	//!< Assume we need less than 2 sec to read sensor value
#define CONVERT_4_20MA
//#define CONVERT_0_10V
//#define CONVERT_0_5V
#define HAL_NB_PULSE_IN		1
#define HAL_NB_LOOP			0
#define ADCNUM				0
#define ADCLOCATION			3					// Location 3 = Analog Port 3
#define ADCPCHANNEL			6					// Channel 6 = GPIOPortD, 14
#define ADCNCHANNEL			7					// Channel 7 = GPIOPortD, 15
#ifdef CONVERT_0_5V
#define ADCREF				ADCMidReference		// 2.5 V internal reference
#else
#define ADCREF				ADCHighReference	// 5 V internal reference
#endif
#define ADCSAMPLEDELAY		ADCSamplingTime64
#define ADCTIME				500
#else
//! @brief Define the number of pulse inputs
#define DELAY_READ_DURATION 0
#define HAL_NB_PULSE_IN		NODE_PULSE
#define HAL_NB_LOOP			NODE_PULSE
#endif

#define SPIBus 				MAKEPORTLIST(8)
#define SPI_SCLK			MAKEPORT(8)
#define SPI_MISO			MAKEPORT(9)
#define SPI_MOSI			MAKEPORT(10)
#define SPI_NAME			DEVICE_SPI1
#define SPI_LOCATION		6

#define RF_NSS				MAKEPORT(11)
#define RF_DIO0				MAKEPORT(12)
#define RF_DIO1				MAKEPORT(13)
#define RF_DIO2				MAKEPORT(14)
#define RF_DIO3				MAKEPORT(15)
#define RF_DIO4				MAKEPORT(16)
#define RF_DIO5				MAKEPORT(17)
#define RF_RESET			MAKEPORT(18)
/** @endcond */

/** @ */
