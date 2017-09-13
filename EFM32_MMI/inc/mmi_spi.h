/**
 * @file mmi_spi.h
 *
 */

#ifndef EFM32_MMI_INC_MMI_SPI_H_
#define EFM32_MMI_INC_MMI_SPI_H_

#include "system.h"
/** \addtogroup MMI MyMeterInfo add-on functions
 *  @{
 */

/*!
 * Abstracted device SPI bus port
 */
typedef enum {
	DEVICE_SPI0 = 0,    //!< DEVICE_SPI0
	DEVICE_SPI1 = 1,    //!< DEVICE_SPI1
	DEVICE_SPI2 = 2,    //!< DEVICE_SPI2
	DEVICE_SPI3 = 3,    //!< DEVICE_SPI3
	DEVICE_SPI4 = 4,    //!< DEVICE_SPI4
	DEVICE_SOFT_SPI = -1//!< DEVICE_SOFT_SPI - Emulated SPI bus
} SPI_NAME;

/*!
 * @brief SPI port descriptor
 */
typedef struct {
SystemPort Sclk;				//!< SPI Clock GPIO pin
SystemPort Miso;				//!< SPI Master In Slave Out (RX) GPIO pin
SystemPort Mosi;				//!< SPI Master Out Slave In (TX) GPIO pin
SPI_NAME spi_name;				//!< SPI bus port name
unsigned int routeLocation;		//!< SPI bus port pinout location (depending on microcontroller used)
} SPIPORT;

/*!
 * @brief Open and initialize a SPI port bus
 * @param[in] SpiPort	SPI port descriptor
 * @param[in] speed		SPI bus speed in Hz
 * @return true if function is successful
 */
BOOL SPIOpen(SPIPORT* SpiPort, const unsigned long speed);
/*!
 * @brief Check whether a SPI port is opened
 * @param[in] SpiPort	SPI port descriptor
 * @return true if SPI port is opened
 */
BOOL SPIIsOpen(const SPIPORT *SpiPort);
/*!
 * @brief Close a SPI port bus
 * @param[in] SpiPort	SPI port descriptor
 * @param[in] PowerOffUSART true to put the port in low power mode
 */
void SPIClose(const SPIPORT *SpiPort, BOOL PowerOffUSART);
/*!
 * @brief Indicates whether this port can run in low power mode
 * @param[in] SpiPort	SPI port descriptor
 * @return true if port can run in low power mode
 */
BOOL SPIIsLowPowerMode(const SPIPORT *SpiPort);
/*!
 * @brief Get SPI port bus speed
 * @param[in] SpiPort	SPI port descriptor
 */
unsigned long SPIGetSpeed(const SPIPORT *SpiPort);
/*!
 * @brief Get SPI port MISO signal level
 * @param[in] SpiPort	SPI port descriptor
 * @return true if MISO signal is high
 */
BOOL SPIGetRXLevel(const SPIPORT *SpiPort);
/*!
 * @brief Set SPI port MOSI signal level
 * @param[in] SpiPort	SPI port descriptor
 * @param[in] Level true to put signal to high level, false for low level
 */
void SPISetTXLevel(const SPIPORT *SpiPort, BOOL Level);
/*!
 * @brief Set SPI port CLOCK signal level
 * @param[in] SpiPort	SPI port descriptor
 * @param[in] Level true to put signal to high level, false for low level
 */
void SPISetClockLevel(const SPIPORT *SpiPort, BOOL Level);
/*!
 * @brief Read last received character on the SPI bus
 * @param[in] SpiPort	SPI port descriptor
 * @return the waiting character or -1 if no character present
 */
short SPIGetChar(const SPIPORT *SpiPort);
/*!
 * @brief Send a character in the SPI bus
 * @param[in] SpiPort	SPI port descriptor
 * @param[in] c character to send
 */
void SPIPutChar(const SPIPORT *SpiPort, unsigned char c);
/*!
 * @brief Send a character on the SPI bus and return read character
 * @param[in] SpiPort	SPI port descriptor
 * @param[in] c	character to send
 * @return character read
 */
unsigned char SPITransferChar(const SPIPORT *SpiPort, unsigned char c);
/*!
 * @brief Send a NULL terminated string on the SPI bus
 * @param[in] SpiPort	SPI port descriptor
 * @param[in] buffer
 */
void SPIPutString(const SPIPORT *SpiPort, unsigned char const *buffer);
/*!
 * @brief Send a memory range on the SPI bus
 * @param[in] SpiPort	SPI port descriptor
 * @param[in] buffer	Buffer start address
 * @param[in] len		Number of bytes to transfer
 */
void SPIPutBuffer(const SPIPORT *SpiPort, unsigned char const *buffer, unsigned short len);
/*!
 * @brief Check is a character was received on the SPI bus
 * @param[in] SpiPort	SPI port descriptor
 * @return true if a character is waiting
 */
BOOL SPIisRXReady(const SPIPORT *SpiPort);
/*!
 * @brief Check if the character transmit buffer register of the SPI bus is empty
 * @param[in] SpiPort	SPI port descriptor
 * @return true if the transmit buffer register is empty
 * @remark A character can be under transmission as the buffer register is empty
 */
BOOL SPIisTXEmpty(const SPIPORT *SpiPort);
/*!
 * @brief Check if all character transmit registers of the SPI bus is empty
 * @param[in] SpiPort	SPI port descriptor
 * @return true is all transmit registers are empty
 */
BOOL SPIisTXFinished(const SPIPORT *SpiPort);

/** }@ */

#endif /* EFM32_MMI_INC_MMI_SPI_H_ */
