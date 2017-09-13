/*******************************************************************
**                                                                **
** CRC16 computation functions.                                   **
**                                                                **
*******************************************************************/

#ifndef __CRC16_H__
#define __CRC16_H__

/** \addtogroup MMI MyMeterInfo add-on functions
 *  @{
 */

#define CRC16_CCITT		0x1021  //!< @brief CCITT Standard CRC16 polynomial for X25/XMODEM/SD/HDLC/Bluetooth etc...
#define CRC16_ANSI      0x8005 	//!< @brief ANSI/IBM CRC16 polynomial for BiSync/MODBUS/USB etc...
//! Macro to calculate standard CCITT CRC16 value
#define CRC16_Calculate(b,c)  CRC16_CalculatePolynomial((b),(c),CRC16_CCITT)
//! Macro to calculate standard CCITT CRC16 on memory range
#define CRC16_CalculateRange(b,l,c) CRC16_CalculateRangePolynomial((b),(l),(c),CRC16_CCITT)

/*!
 * @brief Calculates a CRC value on 16bits, using any CRC polynomial
 * @param [in] Byte			Byte to add to the CRC value
 * @param [in] prevCrc		Previous value of the CRC
 * @param [in] Polynomial	Polynomial value to use in CRC calculation
 * @return 					New CRC value
 */
unsigned short CRC16_CalculatePolynomial(const unsigned char Byte, const unsigned short prevCrc, const unsigned short Polynomial);
/*!
 * @brief Calculates a memory range CRC value on 16 bits, using any CRC polynom
 * @param [in] Buffer		Buffer address to calculate CRC on
 * @param [in] length		Length of the provided buffer
 * @param [in] prevCrc		Previous value of the CRC
 * @param [in] Polynomial	Polynomial value to use in CRC Calculation
 * @return 					New CRC value
 */
unsigned short CRC16_CalculateRangePolynomial(const unsigned char* Buffer, unsigned short length, unsigned short prevCrc, const unsigned short Polynomial);

/** }@ */

#endif
