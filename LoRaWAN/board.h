/**
 * @file board.h
 * @remark This file is derived from the original LoRaMote board.h file from Semtech
 */
/** \addtogroup LW LoRaWAN Implementation
 * @brief LoRaWAN protocol implementation using hardware abstracted layer
 *  @{
 */

#ifndef INC_BOARD_H_
#define INC_BOARD_H_

#include <mmi_spi.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*!
 * This section makes the translation between the original LoRaWAN source code
 * and the abstraction layer to allow any micro-controller usage.
 */
#include "timer.h"

#include <system.h>
#undef DEFINE_HAL
#include "HAL_def.h"
/*!
 * Operation Mode for the GPIO
 */
typedef enum
{
    PIN_INPUT = 0,
    PIN_OUTPUT,
    PIN_ALTERNATE_FCT,
    PIN_ANALOGIC
}PinModes;

/*!
 * Add a pull-up, a pull-down or nothing on the GPIO line
 */
typedef enum
{
    PIN_NO_PULL = 0,
    PIN_PULL_UP,
    PIN_PULL_DOWN
}PinTypes;

/*!
 * Define the GPIO as Push-pull type or Open Drain
 */
typedef enum
{
    PIN_PUSH_PULL = 0,
    PIN_OPEN_DRAIN
}PinConfigs;

/*!
 * @brief Fake definition needed by LoRaWAN implementation
 */
typedef int PinNames;

#define Gpio_t	SystemPort			//!< LoRaWAN definition mapped to Hardware Abstraction Layer

/*!
 * @brief Fake definition needed by LoRaWAN implementation
 */
typedef struct {
	/*!
	 * @brief Useless member needed by LoRaWAN implementation
	 */
	void* Instance;
} SPI_HandleTypeDef;

/*!
 * @brief LoRaWAN SPI port definition mapped to Hardware Abstraction Layer
 */
typedef struct {
	SystemPort Sclk; 			//!< SPI Clock GPIO pin
	SystemPort Miso;			//!< SPI Master In Slave Out (RX) GPIO pin
	SystemPort Mosi;			//!< SPI Master Out Slave In (TX) GPIO pin
	SystemPort Nss;				//!< SPI chip select GPIO pin
	SPI_HandleTypeDef Spi;		//!< Useless member needed by LoRaWAN implementation
} Spi_t;

#define RADIO_RESET		0					//!< Needed by LoRaWAN SX1276 Driver

#define assert_param(expr) ((void)0U)		//!< Fake assert_param

/*!
 * @brief Replacement of LoRaWAN implementation of GPIO initialization function
 * @param[in] port		Port to initialize
 * @param[in] name		Pin name in LoRaWAN implementation (unused)
 * @param[in] mode		Pin mode
 * @param[in] type		Pin type in LoRaWAN implementation (unsused)
 * @param[in] pullup	true if pull up resistor shall be set on port pin
 * @param[in] value		true to put port pin to high level, false for low level
 */
static inline void GpioInit(void* port, PinNames name, PinModes mode, PinTypes type, PinConfigs pullup, int value) {
	SystemSetPortMode(*((SystemPort*)port), (mode == PIN_OUTPUT) ? ((value) ? PortOut1 : PortOut0) : ((value) ? PortInUp : PortIn));
}

static inline void GpioWrite(void* port, int value) {
	SystemSetPortState(*((SystemPort*)port), value);
}

static inline uint8_t SpiInOut(Spi_t* spi, int address) {
	return SPITransferChar((SPIPORT*)spi->Spi.Instance,address);
}

static inline void DelayMs(int delay) { SysTimerWait1ms(delay); }

/*!
 * Possible power sources
 */
enum BoardPowerSources
{
    USB_POWER = 0,
    BATTERY_POWER,
};
/*!
 * \brief Get the board power source
 *
 * \retval value  power source [0: USB_POWER, 1: BATTERY_POWER]
 */
static inline uint8_t GetBoardPowerSource( void ) { return BATTERY_POWER; }

/*!
 * \brief Disable interrupts
 *
 * \remark IRQ nesting is managed
 */
static inline void BoardDisableIrq( void ) { SystemIrqDisable(); }

/*!
 * \brief Enable interrupts
 *
 * \remark IRQ nesting is managed
 */
static inline void BoardEnableIrq( void ) { SystemIrqEnable(); }

/*!
 * \brief Measure the Battery voltage
 *
 * \retval value  battery voltage in volts
 */
static inline uint32_t BoardGetBatteryVoltage( void ) { return SystemBatteryGetVoltage(); }

/*!
 * Battery thresholds
 */
#define BATTERY_MAX_LEVEL                           3700 //!< Maximum battery level in mV
#define BATTERY_MIN_LEVEL                           3000 //!< Minimum battery level in mV
#define BATTERY_SHUTDOWN_LEVEL                      2500 //!< lowest Battery level that causes a system shutdown in mV
/*!
 * \brief Get the current battery level
 *
 * \retval value  battery level [  0: USB,
 *                                 1: Min level,
 *                                 x: level
 *                               254: fully charged,
 *                               255: Error]
 */
uint8_t BoardGetBatteryLevel( void );

/** @cond */
/*!
 * Unique Devices IDs register set ( EFM32 )
 */
#define         ID1                                 ( 0x0FE081B0 + 0x040)
#define         ID2                                 ( 0x0FE081B0 + 0x044)
#define         ID3                                 ( 0x0FE081B0 + 0X028)
/** @endcond */

static inline uint32_t BoardGetRandomSeed( void )
{
    return ( ( *( uint32_t* )ID1 ) ^ ( *( uint32_t* )ID2 ) ^ ( *( uint32_t* )ID3 ) );
}
/*!
 * \brief Gets the board 64 bits unique ID
 *
 * \param [IN] id Pointer to an array that will contain the Unique ID
 */
static inline void BoardGetUniqueId( uint8_t *id ) {
	memcpy(id,( uint32_t* )ID1,8);
}

/** End of translation section **/

#include "utilities.h"
#include "radio.h"
#include "sx1276/sx1276.h"
#include "rtc-board.h"
#include "sx1276-board.h"

/**  }@
 */

#endif /* INC_BOARD_H_ */
