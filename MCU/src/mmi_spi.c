/*******************************************************************
**                                                                **
** SPI management functions                                      **
**                                                                **
*******************************************************************/
/** \addtogroup MMI MyMeterInfo add-on functions
 *  @{
 */

#include "mmi_spi.h"
#include "system.h"
#include <em_device.h>
#include <em_usart.h>
#include <em_cmu.h>
#include <em_gpio.h>

static const USART_TypeDef* _SPIPorts[] = {
#if USART_COUNT > 0
	USART0,
#if USART_COUNT > 1
	USART1,
#endif
#if USART_COUNT > 2
	USART2,
#endif
#if USART_COUNT > 3
	USART3,
#endif
#if USART_COUNT > 4
	USART4,
#endif
#endif
};
static const CMU_Clock_TypeDef _SPIClock[] = {
#if USART_COUNT > 0
	cmuClock_USART0,
#if USART_COUNT > 1
	cmuClock_USART1,
#endif
#if USART_COUNT > 2
	cmuClock_USART2,
#endif
#if USART_COUNT > 3
	cmuClock_USART3,
#endif
#if USART_COUNT > 4
	cmuClock_USART4,
#endif
#endif
};
/*******************************************************************
**                SPI Helping functions                          **
*******************************************************************/

/*******************************************************************
**                SPI Public functions                           **
**                not related to hardware                         **
*******************************************************************/
void SPIPutString(const SPIPORT *SpiPort, unsigned char const *buffer)
{
  while(*buffer)
    SPIPutChar(SpiPort,*buffer++);
}

void SPIPutBuffer(const SPIPORT *SpiPort, unsigned char const *buffer, unsigned short len)
{
  while(len--)
    SPIPutChar(SpiPort,*buffer++);
}

BOOL SPIGetRXLevel(const SPIPORT *SpiPort)
{
  return (unsigned char)SystemGetPortState(SpiPort->Miso);
}

/*******************************************************************
**                SPI Public functions                           **
**                hardware oriented                               **
*******************************************************************/
short SPIGetChar(const SPIPORT *SpiPort)
{
short c = -1;
	if (SpiPort->spi_name < 0) {

	}
#if USART_COUNT > 0
	else if (SpiPort->spi_name < USART_COUNT) {
		if (SPIisRXReady(SpiPort))
			c = (short)USART_Rx((USART_TypeDef*)_SPIPorts[SpiPort->spi_name]);
	}
#endif
	return c;
}

void SPIPutChar(const SPIPORT *SpiPort, unsigned char c)
{
  if (SPIIsOpen(SpiPort))
  {
		if (SpiPort->spi_name < 0) {
		}
#if USART_COUNT > 0
		else if (SpiPort->spi_name < USART_COUNT) {
			USART_SpiTransfer((USART_TypeDef*)_SPIPorts[SpiPort->spi_name],c);
		}
#endif
  }
}

unsigned char SPITransferChar(const SPIPORT *SpiPort, unsigned char c)
{
	if (SPIIsOpen(SpiPort))
	{
		if (SpiPort->spi_name < 0) {
		}
#if USART_COUNT > 0
		else if (SpiPort->spi_name < USART_COUNT) {
			return USART_SpiTransfer((USART_TypeDef*)_SPIPorts[SpiPort->spi_name],c);
		}
#endif
	}
	return 0;
}

BOOL SPIOpen(SPIPORT* SpiPort, const unsigned long speed)
{
  /* Configure GPIO pins */
  CMU_ClockEnable(cmuClock_GPIO, true);
  SystemSetPortMode(SpiPort->Miso, PortInDown); // This will lower consumption on idle state
  SystemSetPortMode(SpiPort->Mosi, PortOut0);
  SystemSetPortMode(SpiPort->Sclk, PortOut0);

  /* Enable peripheral clocks */
  CMU_ClockEnable(cmuClock_HFPER, true);
  if (SpiPort->spi_name < 0) {
  }
#if USART_COUNT > 0
  else if (SpiPort->spi_name < USART_COUNT) {
	  CMU_ClockEnable(_SPIClock[SpiPort->spi_name], true);
	  USART_InitSync_TypeDef init = USART_INITSYNC_DEFAULT;
	  init.enable = usartDisable;
	  init.baudrate = speed;
	  init.msbf = true;
	  /* Enable pins location */
	  USART_InitSync((USART_TypeDef*)_SPIPorts[SpiPort->spi_name], &init);
	  ((USART_TypeDef*)_SPIPorts[SpiPort->spi_name])->ROUTEPEN = USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN | USART_ROUTEPEN_CLKPEN;
	  ((USART_TypeDef*)_SPIPorts[SpiPort->spi_name])->ROUTELOC0 =  (SpiPort->routeLocation << _USART_ROUTELOC0_RXLOC_SHIFT) | (SpiPort->routeLocation << _USART_ROUTELOC0_CLKLOC_SHIFT) | (SpiPort->routeLocation  << _USART_ROUTELOC0_TXLOC_SHIFT);
	  USART_Enable((USART_TypeDef*)_SPIPorts[SpiPort->spi_name],usartEnable);
  } else return 0;
#endif
  return 1;
}

BOOL SPIIsLowPowerMode(const SPIPORT *SpiPort) {
	return false;
}

void SPIClose(const SPIPORT *SpiPort, BOOL PowerOffSPI)
{
  if (SpiPort->spi_name >= 0) {
	  if (PowerOffSPI) {
#if USART_COUNT > 0
		  if (SpiPort->spi_name < USART_COUNT)
			  USART_Reset((USART_TypeDef*)_SPIPorts[SpiPort->spi_name]);
#endif
  	  } else {
#if USART_COUNT > 0
		  if (SpiPort->spi_name < USART_COUNT)
			  USART_Enable((USART_TypeDef*)_SPIPorts[SpiPort->spi_name],usartDisable);
#endif
  	  }
  }
  SystemSetPortMode(SpiPort->Miso,PortDisabled);
  SystemSetPortMode(SpiPort->Mosi,PortDisabled);
  SystemSetPortMode(SpiPort->Sclk,PortDisabled);
}

unsigned long SPIGetSpeed(const SPIPORT *SpiPort)
{
#if USART_COUNT > 0
	if ((SpiPort->spi_name >= 0) && (SpiPort->spi_name < USART_COUNT))
		return USART_BaudrateGet((USART_TypeDef*)_SPIPorts[SpiPort->spi_name]);
#endif
	return 0;
}


BOOL SPIisRXReady(const SPIPORT *SpiPort)
{
	if (SpiPort->spi_name < 0) return true;
#if USART_COUNT > 0
	if ((SpiPort->spi_name >= 0) && (SpiPort->spi_name < USART_COUNT))
		return ((_SPIPorts[SpiPort->spi_name]->STATUS & USART_STATUS_RXDATAV) != 0);
#endif
	return false;
}

BOOL SPIisTXEmpty(const SPIPORT *SpiPort)
{
	if (SpiPort->spi_name < 0) return true;
#if USART_COUNT > 0
	if ((SpiPort->spi_name >= 0) && (SpiPort->spi_name < USART_COUNT))
		return ((_SPIPorts[SpiPort->spi_name]->STATUS & USART_STATUS_TXBL) != 0);
#endif
	return false;
}

BOOL SPIisTXFinished(const SPIPORT *SpiPort)
{
	if (SpiPort->spi_name < 0) return true;
#if USART_COUNT > 0
	if ((SpiPort->spi_name >= 0) && (SpiPort->spi_name < USART_COUNT))
		return ((_SPIPorts[SpiPort->spi_name]->STATUS & USART_STATUS_TXC) != 0);
#endif
	return false;
}

BOOL SPIIsOpen(const SPIPORT *SpiPort)
{
	if (SpiPort->spi_name < 0) return true;
#if USART_COUNT > 0
	if ((SpiPort->spi_name >= 0) && (SpiPort->spi_name < USART_COUNT))
		return ((_SPIPorts[SpiPort->spi_name]->STATUS & (USART_STATUS_RXENS | USART_STATUS_TXENS)) != 0);
#endif
	return false;
}

void SPISetTXLevel(const SPIPORT *SpiPort, BOOL Level)
{
	SystemSetPortState(SpiPort->Mosi, Level);
}

void SPISetClockLevel(const SPIPORT *SpiPort, BOOL Level)
{
	SystemSetPortState(SpiPort->Sclk, Level);
}
/** }@ */
