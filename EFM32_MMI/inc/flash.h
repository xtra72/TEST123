/*******************************************************************
**                                                                **
** FLASH Self-Programming functions                               **
**                                                                **
*******************************************************************/

#ifndef __FLASH_H__
#define __FLASH_H__
#include <system.h>
/** \addtogroup MMI MyMeterInfo add-on functions
 *  @{
 */

/*==============================================================================================*/
/* constant definitions                                                                         */
/*==============================================================================================*/
#define   FLASH_NO_ERROR                0		//!< No error return code
#define   FLASH_WRITE_VERIFY_ERROR		-5		//!< Flash write verify error return code
#define   FLASH_PARAMETER_ERROR         -4		//!< Illegal parameter error return code
#define   FLASH_WRITE_TIMEOUT			-3		//!< Write timeout error return code
#define   FLASH_PROTECTION_ERROR        -2		//!< Flash memory protected error return code
#define   FLASH_WRITE_ERROR             -1		//!< Flash write error return code

/* Security Flag */
#define FLASH_WRITE_ENABLED				0x00	//!< Flash Memory write enabled security flag
#define FLASH_CHIP_ERASE_DISABLED       0x01	//!< Chip Flash Memory erase disabled security flag
#define FLASH_BLOCK_ERASE_DISABLED      0x02	//!< Block Flash Memory erase disabled security flag
#define FLASH_WRITE_DISABLED            0x04	//!< Flash Memory write disabled security flag

/*!
 * @brief Open Flash Memory Write access
 */
void FLASHOpen(void);
/*!
 * @brief Close Flash Memory Write access
 */
void FLASHClose(void);
/*!
 * @brief Get Flash Memory page size
 * @remark This value represents the minimum flash memory block size that can be erased
 */
unsigned long FLASHGetPageSize(void);
/*!
 * @brief Get total chip Flash Memory size
 */
unsigned long FLASHGetTotalSize(void);
/*!
 * @brief Erase a Block of Flash Memory
 * @param[in] BlockAddress start address
 * @return Flash error code value
 */
signed char FLASHEraseBlock(void* BlockAddress);
/*!
 * @brief Check whether a Block of Flash Memory is blank (a.k.a. contains only 0xFF values)
 * @param[in] BlockAddress start address
 * @return Flash error code value
 */
signed char FLASHBlankCheck(void* BlockAddress);
/*!
 * @brief Check whether a Flash Memory range is blank (a.k.a. contains only 0xFF values)
 * @param[in] Buffer Flash Memory address starting address
 * @param[in] Len	length of range to be checked
 * @return Flash error code value
 */
signed char FLASHBlankBufferCheck(unsigned char *Buffer, unsigned short Len);
/*!
 * @brief Permanently write a memory range to the Flash Memory
 * @param[in] Address	Flash Memory address where to write to
 * @param[in] Buffer	Memory buffer address to copy from
 * @param[in] Count		Number of bytes to copy
 * @return Flash error code value
 * @remark A Flash Memory range can be overwritten as long as 1 bits shall be replaced with 0 value
 */
signed char FLASHWrite(void* Address, unsigned char* Buffer,unsigned short Count);
/*!
 * @brief Permanently write a memory range to the Flash Memory and check that data was copied was successfully
 * @param[in] Address	Flash Memory address where to write to
 * @param[in] Buffer	Memory buffer address to copy from
 * @param[in] Count		Number of bytes to copy
 * @return Flash error code value
 * @remark A Flash Memory range can be overwritten as long as 1 bits shall be replaced with 0 value
 */
signed char FLASHWriteVerify(void* Address, unsigned char* Buffer,unsigned short Count);
/*!
 * @brief Erase the Flash Memory region dedicated to User Data
 * @return Flash error code value
 */
signed char FLASHEraseUserData(void);
/*!
 * @brief Check whether the Flash Memory region dedicated to USer Data is blank (a.k.a. contains only 0xFF values)
 * @return Flash error code value
 */
signed char FLASHBlankCheckUserData(void);
/*!
 * @brief Permanently write data to the Flash Memory range dedicated to User Data
 * @param[in] Offset	Offset to User Data region start address to write to
 * @param[in] Buffer	Memory buffer to copy from
 * @param[in] Count		Number of bytes to copy
 * @return Flash error code value
 */
signed char FLASHWriteUserData(unsigned short Offset, unsigned char* Buffer, unsigned short Count);
/*!
 * @brief Permanently write data to the Flash Memory range dedicated to User Data and check that data was copied successfully
 * @param[in] Offset	Offset to User Data region start address to write to
 * @param[in] Buffer	Memory buffer to copy from
 * @param[in] Count		Number of bytes to copy
 * @return Flash error code value
 */
signed char FLASHWriteVerifyUserData(unsigned short Offset, unsigned char* Buffer, unsigned short Count);
/*!
 * @brief Get a Flash Memory block protection status
 * @param[in] StartingAddress Starting address of Flash Memory block
 * @return a protection status flag
 */
signed char FLASHGetProtectedAddress(unsigned char* StartingAddress);
/*!
 * @brief Set a Flash Memory block protection status
 * @param[in] StartingAddress Starting address of Flash Memory block
 * @return a protection status flag
 */
signed char FLASHSetProtectedAddress(unsigned char *StartingAddress);

/** }@ */

#endif
