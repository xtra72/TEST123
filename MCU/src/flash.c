/*******************************************************************
**                                                                **
** FLASH Self-Programming functions                               **
**                                                                **
*******************************************************************/
/** \addtogroup MMI MyMeterInfo add-on functions
 *  @{
 */

#include "flash.h"
#include <em_msc.h>
#include <em_system.h>
#include "em_assert.h"

unsigned long FLASHGetPageSize(void) { return SYSTEM_GetFlashPageSize(); }
unsigned long FLASHGetTotalSize(void) { return (SYSTEM_GetFlashSize() * 1024UL); }

//! @brief The function will be relocated in RAM memory to run
#define RAMFUNC __attribute__ ((section(".ram")))

RAMFUNC static int checkMSCFlag(uint32_t flag,uint32_t sts) {
	for (int timeOut = MSC_PROGRAM_TIMEOUT; timeOut; timeOut--)
		if ((MSC->STATUS & flag) != sts) return mscReturnOk;
	return mscReturnTimeOut;
}

RAMFUNC static msc_Return_TypeDef MMI_ErasePage(uint32_t *startAddress, int bErase) {

	  /* Load address */
	  MSC->ADDRB    = (uint32_t) startAddress;
	  MSC->WRITECMD = MSC_WRITECMD_LADDRIM;

	  /* Check for invalid address */
	  if (MSC->STATUS & MSC_STATUS_INVADDR)
	  {
	    return mscReturnInvalidAddr;
	  }

	  /* Check for write protected page */
	  if (MSC->STATUS & MSC_STATUS_LOCKED)
	  {
	    return mscReturnLocked;
	  }
	  if (bErase) {
		  /* Send erase page command */
		  MSC->WRITECMD = MSC_WRITECMD_ERASEPAGE;

		  /* Wait for the erase to complete */
		  return checkMSCFlag(MSC_STATUS_BUSY,MSC_STATUS_BUSY);
	  }
	  return mscReturnOk;
}

RAMFUNC static msc_Return_TypeDef MMI_MSC_ErasePage(uint32_t *startAddress)
{
  /* Address must be aligned to pages */
//  EFM_ASSERT((((uint32_t) startAddress) & (FLASH_PAGE_SIZE - 1)) == 0);
  /* Enable writing to the MSC */
  MSC->WRITECTRL |= MSC_WRITECTRL_WREN;
  msc_Return_TypeDef rc = MMI_ErasePage(startAddress,1);
  /* Disable writing to the MSC */
  MSC->WRITECTRL &= ~MSC_WRITECTRL_WREN;
  return rc;
}

RAMFUNC static msc_Return_TypeDef MMI_MscLoadAddress(uint32_t* address)
{
	return MMI_ErasePage(address,0);
}

RAMFUNC static msc_Return_TypeDef MMI_MscLoadData(uint32_t* data, int num)
{
  /* Wait for the MSC to be ready for a new data word.
   * Due to the timing of this function, the MSC should
   * already by ready */
  if (checkMSCFlag(MSC_STATUS_WDATAREADY,0))
    return mscReturnTimeOut;

  /* Load 'num' 32-bit words into write data register. */
	  (void)num;	// to avoid warning
    MSC->WDATA = *data;

  /* Trigger write once */
  MSC->WRITECMD = MSC_WRITECMD_WRITEONCE;

  /* Wait for the write to complete */
  /* Check for timeout */
  return checkMSCFlag(MSC_STATUS_BUSY,MSC_STATUS_BUSY);
}

RAMFUNC msc_Return_TypeDef MMI_MSC_WriteWord(uint32_t *address, void const *data, int numBytes) {
	  int wordCount;
	  int numWords;
	  msc_Return_TypeDef retval = mscReturnOk;

	  /* Check alignment (Must be aligned to words) */
	  EFM_ASSERT(((uint32_t) address & 0x3) == 0);

	  /* Check number of bytes. Must be divided by four */
	  EFM_ASSERT((numBytes & 0x3) == 0);

	  // Round number of bytes to a word boundary
	  while (numBytes & 0x03) {
		  numBytes++;
	  }

	  /* Enable writing to the MSC */
	  MSC->WRITECTRL |= MSC_WRITECTRL_WREN;

	  /* Convert bytes to words */
	  numWords = numBytes >> 2;

	  for (wordCount = 0; (wordCount < numWords)&& (mscReturnOk == retval); wordCount++)
	  {
	    retval = MMI_MscLoadAddress(address + wordCount);
	    if (mscReturnOk == retval)
	    	retval = MMI_MscLoadData(((uint32_t *) data) + wordCount, 1);
	  }
	  /* Disable writing to the MSC */
	  MSC->WRITECTRL &= ~MSC_WRITECTRL_WREN;

	  return retval;
}

#define USERPAGE    0x0FE00000 /**< Address of the user page */
#define FLASHCheckAddress(a,l) (((uint32_t)(a) + (l)) > ((((void*)(a)) >= (void*)USERPAGE) ? (USERPAGE + 512) : FLASHGetTotalSize()))

void FLASHOpen(void)
{
	MSC_Init();
}
void FLASHClose(void)
{
	MSC_Deinit();
}
signed char FLASHEraseBlock(void* BlockAddress)
{
  /* Calculate page starting address */
  uint32_t* add = (uint32_t*)(((uint32_t)BlockAddress / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE);
  if (FLASHCheckAddress(add,0)) return FLASH_PARAMETER_ERROR;
  return (signed char)MMI_MSC_ErasePage(add);
}
signed char FLASHBlankCheck(void* BlockAddress)
{
  /* Calculate page starting address */
  uint32_t* add = (uint32_t*)(((uint32_t)BlockAddress / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE);
  if (FLASHCheckAddress(add,0)) return FLASH_PARAMETER_ERROR;
  return FLASHBlankBufferCheck((unsigned char *)add,FLASH_PAGE_SIZE);
}
signed char FLASHBlankBufferCheck(unsigned char *Buffer, unsigned short Len)
{
  if (FLASHCheckAddress(Buffer,Len)) return FLASH_PARAMETER_ERROR;
  while(Len--)
    if (*(Buffer++) != 0xFF) return 0;
  return 1;
}
signed char FLASHWrite(void* Address, unsigned char* Buffer,unsigned short Count)
{
  if (FLASHCheckAddress(Address,Count)) return FLASH_PARAMETER_ERROR;
  return (signed char)MMI_MSC_WriteWord((uint32_t *) Address, (void *) Buffer, Count);
}
signed char FLASHWriteVerify(void* Address, unsigned char* Buffer,unsigned short Count)
{
  signed char ret = FLASHWrite(Address, Buffer, Count);
  if (ret == FLASH_NO_ERROR) {
	 while(Count--) {
		 if (*((unsigned char*)Address++) != *(Buffer++)) return FLASH_WRITE_VERIFY_ERROR;
	 }
  }
  return ret;
}
signed char FLASHGetProtectedAddress(unsigned char* StartingAddress)
{
  /* Calculate page starting address */
  uint32_t add = ((uint32_t)StartingAddress / FLASH_PAGE_SIZE);
  if (FLASHCheckAddress((void*)(add * FLASH_PAGE_SIZE),0)) return FLASH_PARAMETER_ERROR;
  int lock = *((int*)( 0x0FE04000 + (add >> 5) ));
  return (lock & (0x0001 << (add & 0x001F))) ? FLASH_WRITE_ENABLED : FLASH_WRITE_DISABLED;
}
signed char FLASHSetProtectedAddress(unsigned char *StartingAddress)
{
  /* Calculate page starting address */
  uint32_t add = ((uint32_t)StartingAddress / FLASH_PAGE_SIZE);
  if (FLASHCheckAddress((void*)(add * FLASH_PAGE_SIZE),0)) return FLASH_PARAMETER_ERROR;
  int lock = *((int*)( 0x0FE04000 + (add >> 5) ));
  lock &= ~( 0x0001 << (add & 0x001F));
  signed char rc = (signed char)FLASHWrite((void*)( 0x0FE04000 + (add >> 5) ),(unsigned char*)&lock,sizeof(lock));
  return (rc == FLASH_NO_ERROR) ? FLASH_WRITE_DISABLED : rc;
}

signed char FLASHEraseUserData(void) {
	  return (signed char)MMI_MSC_ErasePage((uint32_t *)USERPAGE);
}
signed char FLASHBlankCheckUserData(void) {
	return FLASHBlankBufferCheck((unsigned char*)USERPAGE,512);
}
signed char FLASHWriteUserData(unsigned short Offset, unsigned char* Buffer, unsigned short Count) {
  if ((Offset + Count) > 512) return FLASH_PARAMETER_ERROR;
  return (signed char)MMI_MSC_WriteWord((uint32_t *) (USERPAGE + Offset), (void *) Buffer, Count);
}
signed char FLASHWriteVerifyUserData(unsigned short Offset, unsigned char* Buffer, unsigned short Count) {
  signed char ret = FLASHWriteUserData(Offset, Buffer, Count);
  if (ret == FLASH_NO_ERROR) {
	 unsigned char*p = (unsigned char*)(USERPAGE + Offset);
	 while(Count--) {
		 if (*(p++) != *(Buffer++)) return FLASH_WRITE_VERIFY_ERROR;
	 }
  }
  return ret;
}

/** }@ */

