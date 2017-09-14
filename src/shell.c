/*
 * leuart.c
 *
 *  Created on: 2017. 9. 11.
 *      Author: inhyuncho
 */

#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_leuart.h"
#include "em_ldma.h"
#include "retargetserial.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "trace.h"
#include "device_def.h"

#undef	__MODULE__
#define	__MODULE__ "TRACE"

static char pBuffer[256];
xSemaphoreHandle	SHELLSemaphore;
//static StaticSemaphore_t xSHELLSemaphoreBuffer;

#define	SHELL_TIMEOUT	0 //(5 * configTICK_RATE_HZ)
/***************************************************************************//**
 * @brief  Setting up LEUART
 ******************************************************************************/
void SHELL_Init(void)
{
  /* Enable peripheral clocks */
  CMU_ClockEnable(cmuClock_HFPER, true);
  /* Configure GPIO pins */
  CMU_ClockEnable(cmuClock_GPIO, true);
  /* To avoid false start, configure output as high */
  GPIO_PinModeSet(RETARGET_TXPORT, RETARGET_TXPIN, gpioModePushPull, 1);
  GPIO_PinModeSet(RETARGET_RXPORT, RETARGET_RXPIN, gpioModeInput, 0);

  LEUART_Init_TypeDef init = {                                                                                         \
	    leuartEnable,    /* Enable RX/TX when init completed. */                                \
	    0,               /* Use current configured reference clock for configuring baudrate. */ \
	    9600,            /* 9600 bits/s. */                                                     \
	    leuartDatabits8, /* 8 databits. */                                                      \
	    leuartNoParity,  /* No parity. */                                                       \
	    leuartStopbits1  /* 1 stopbit. */                                                       \
	  };

  /* Enable CORE LE clock in order to access LE modules */
  CMU_ClockEnable(cmuClock_CORELE, true);

  /* Select LFXO for LEUARTs (and wait for it to stabilize) */
  CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFXO);
  CMU_ClockEnable(cmuClock_LEUART0, true);

  /* Do not prescale clock */
  CMU_ClockDivSet(cmuClock_LEUART0, cmuClkDiv_1);

  /* Configure LEUART */
  init.enable = leuartDisable;

  LEUART_Init(LEUART0, &init);

  /* Enable pins at default location */
  LEUART0->ROUTELOC0 = (LEUART0->ROUTELOC0 & ~(_LEUART_ROUTELOC0_TXLOC_MASK
                                               | _LEUART_ROUTELOC0_RXLOC_MASK))
                       | (RETARGET_TX_LOCATION << _LEUART_ROUTELOC0_TXLOC_SHIFT)
                       | (RETARGET_RX_LOCATION << _LEUART_ROUTELOC0_RXLOC_SHIFT);

  LEUART0->ROUTEPEN  = USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN;

  /* Set RXDMAWU to wake up the DMA controller in EM2 */
//  LEUART_RxDmaInEM2Enable(LEUART0, true);

  /* Finally enable it */
  LEUART_Enable(LEUART0, leuartEnable);

//SHELLSemaphore = xSemaphoreCreateBinaryStatic( &xSHELLSemaphoreBuffer );

#if 0
  /* LDMA transfer configuration for LEUART */
  const LDMA_TransferCfg_t periTransferRx =
    LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_LEUART0_RXDATAV);

  xfer.xfer.dstInc  = ldmaCtrlDstIncNone;
  xfer.xfer.doneIfs = 1;

  /* LDMA initialization mode definition */
  LDMA_Init_t init = LDMA_INIT_DEFAULT;

  /* LDMA initialization */
  LDMA_Init(&init);
  LDMA_StartTransfer(0, (LDMA_TransferCfg_t *)&periTransferRx, &xfer);
#endif
}

/*!
 * @brief Console formatted output
 */
uint32_t	SHELL_PrintString(const char *pString)
{
	uint32_t	nOutputLength = 0;

	if (SHELLSemaphore)
	{
		if (!xSemaphoreTake( SHELLSemaphore, SHELL_TIMEOUT ))
		{
			return	nOutputLength;
		}
	}

	for(uint32_t i = 0 ; i < strlen(pString) ; i++)
	{
		LEUART_Tx(LEUART0, pString[i]);
	}

	if (SHELLSemaphore) xSemaphoreGive( SHELLSemaphore);

	return	nOutputLength;
}

/*!
 * @brief Console output
 */
uint32_t	SHELL_Dump(uint8_t *pData, uint32_t ulDataLen)
{
	uint32_t	nOutputLength = 0;
	int 		bBinary = 0;

	for(uint32_t i = 0 ; i < ulDataLen ; i++)
	{
		if (!isprint(pData[i]))
		{
			bBinary = 1;
			break;
		}
	}

	char	pBuff[4];
	memset(pBuff, 0, sizeof(pBuff));
	pBuff[2] = ' ';

	if (bBinary)
	{
		for(uint32_t i = 0 ; i < ulDataLen ; i++)
		{
			uint8_t	nValue;

			nValue = pData[i] >> 4;
			if (nValue < 10)
			{
				pBuff[0] = nValue + '0';
			}
			else
			{
				pBuff[0] = nValue + 'a' -  10;
			}

			nValue = pData[i] & 0x0F;
			if (nValue < 10)
			{
				pBuff[1] = nValue + '0';
			}
			else
			{
				pBuff[1] = nValue + 'a' -  10;
			}

			nOutputLength += SHELL_PrintString(pBuff);
		}
		nOutputLength += SHELL_PrintString("\n");
	}
	else
	{
		if (SHELLSemaphore)
		{
			if (!xSemaphoreTake( SHELLSemaphore, SHELL_TIMEOUT ))
			{
				return	nOutputLength;
			}
		}

		for(uint32_t i = 0 ; i < ulDataLen ; i++)
		{
			LEUART_Tx(LEUART0, pData[i]);
		}
		LEUART_Tx(LEUART0, '\n');

		if (SHELLSemaphore) xSemaphoreGive( SHELLSemaphore);
	}
	return	nOutputLength;
}

/*!
 * @brief Console formatted output
 */
uint32_t	SHELL_Printf(const char *pFormat, ...)
{
	uint32_t	nOutputLength = 0;

	va_list	xArgs;

	va_start(xArgs, pFormat);

	nOutputLength = vsnprintf(&pBuffer[nOutputLength], sizeof(pBuffer) - nOutputLength - 1, pFormat, xArgs);

	va_end(xArgs);

	if (SHELLSemaphore)
	{
		if (!xSemaphoreTake( SHELLSemaphore, SHELL_TIMEOUT ))
		{
			return	nOutputLength;
		}
	}

	for(uint32_t i = 0 ; i < nOutputLength ; i++)
	{
		LEUART_Tx(LEUART0, pBuffer[i]);
	}

	if (SHELLSemaphore) xSemaphoreGive( SHELLSemaphore);

	return	nOutputLength;
}

/*!
 * @brief Console formatted output
 */
uint32_t	SHELL_VPrintf(const char* pModule, const char *pFormat, va_list xArgs)
{
	uint32_t	nOutputLength = 0;
	uint32_t	ulTime = xTaskGetTickCount();

	if (pModule == 0)
	{
		nOutputLength = snprintf(pBuffer, sizeof(pBuffer) - 1, "[%8lu][%16s] ", ulTime, "global");
	}
	else
	{
		nOutputLength = snprintf(pBuffer, sizeof(pBuffer) - 1, "[%8lu][%16s] ", ulTime, pModule);
	}

	nOutputLength += vsnprintf(&pBuffer[nOutputLength], sizeof(pBuffer) - nOutputLength - 1, pFormat, xArgs);

	if (SHELLSemaphore)
	{
		if (!xSemaphoreTake( SHELLSemaphore, SHELL_TIMEOUT ))
		{
			return	nOutputLength;
		}
	}

	for(uint32_t i = 0 ; i < nOutputLength ; i++)
	{
		LEUART_Tx(LEUART0, pBuffer[i]);
	}

	if (SHELLSemaphore) xSemaphoreGive( SHELLSemaphore);

	return	nOutputLength;
}

uint32_t	SHELL_GetLine(uint8_t* pBuffer, uint32_t ulBufferLen)
{
	uint32_t	ulLineLen = 0;

	while(ulLineLen < ulBufferLen)
	{
		if (LEUART_StatusGet(LEUART0) & LEUART_STATUS_RXDATAV)
		{
			uint8_t nData = LEUART_Rx(LEUART0);
			if (nData == '\n')
			{
				break;
			}
			else if (nData == '\r')
			{
			}
			else if (nData == '\b')
			{
				if (ulLineLen > 0)
				{
					pBuffer[--ulLineLen] = '\0';
				}
			}
			else
			{
				pBuffer[ulLineLen++] = nData;
			}
		}

		vTaskDelay(1);
	}

	if (ulLineLen < ulBufferLen)
	{
		pBuffer[ulLineLen] = '\0';
	}

	return	ulLineLen;
}

int	SHELL_ParseLine(char* pLine, char* pArgv[], uint32_t nMaxArgs)
{
	int			nArgc = 0;

	while(*pLine != '\0')
	{
		while(*pLine != '\0')
		{
			if ((*pLine != ' ') && (*pLine != '\t'))
			{
				break;
			}
			pLine++;
		}

		if (*pLine == '\0')
		{
			break;
		}

		pArgv[nArgc++] = pLine;

		while(*pLine != '\0')
		{
			if ((*pLine == ' ') || (*pLine == '\t'))
			{
				break;
			}
			pLine++;
		}

		if (*pLine != '\0')
		{
			*pLine = '\0';
			pLine++;
		}
	}

	return	nArgc;
}

extern	SHELL_CMD	pShellCmds[];
static	char pLine[128];
static char*	ppArgv[16];

__attribute__((noreturn)) void SHELL_Task(void* pvParameters)
{
	int		nArgc;

	memset(pLine, 0, sizeof(pLine));
	while(1)
	{
		SHELL_Printf("S47> ");
		uint32_t ulLineLen = SHELL_GetLine((uint8_t*)pLine, sizeof(pLine) - 1);
		if (ulLineLen != 0)
		{
			nArgc = SHELL_ParseLine(pLine, ppArgv, 16);
			if (nArgc != 0)
			{
				SHELL_CMD*	pCmd = pShellCmds;
				while(pCmd->pName != NULL)
				{
					if (strcmp(pCmd->pName, ppArgv[0]) == 0 )
					{
						pCmd->fCommand(ppArgv, nArgc);
						break;
					}
					pCmd++;
				}

				if (pCmd->pName == NULL)
				{
					SHELL_Printf("Unknown command : %s\n", ppArgv[0]);
				}
			}
		}
	}

	__builtin_unreachable();

}

int	SHELL_CMD_Join(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		DevicePostEvent(RUN_ATTACH);
	}
	else if (nArgc == 2)
	{
		if (strcasecmp(ppArgv[1], "otta") == 0)
		{
			DevicePostEvent(RUN_ATTACH_USE_OTTA);
		}
		else if (strcasecmp(ppArgv[1], "abp") == 0)
		{
			DevicePostEvent(RUN_ATTACH_USE_ABP);
		}

	}


	return	0;
}


int	SHELL_CMD_Req(char *ppArgv[], int nArgc)
{
	if (nArgc == 2)
	{
		if (strcasecmp(ppArgv[1], "1") == 0)
		{
			DevicePostEvent(REQ_REAL_APP_KEY_ALLOC);
		}
		else if (strcasecmp(ppArgv[1], "2") == 0)
		{
			DevicePostEvent(REQ_REAL_APP_KEY_RX_REPORT);
		}
	}


	return	0;
}

extern	TaskHandle_t	hSuperTask;
extern	TaskHandle_t	hShellTask;

int	SHELL_CMD_Task(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		TaskStatus_t	xStatus;

		vTaskGetInfo( hShellTask, &xStatus, true, eInvalid);

		SHELL_Printf("%16s : %d\n", "Water Mark", xStatus.usStackHighWaterMark);
	}

	return	0;
}

int	SHELL_CMD_Trace(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("%16s : %s\n", "Mode", (TRACE_GetEnable())?"Enable":"Disabled");
	}
	else if (nArgc == 2)
	{
		if (strcmp(ppArgv[1], "enable") == 0)
		{
			TRACE_SetEnable(true);
			SHELL_Printf("%16s : %s\n", "Mode", (TRACE_GetEnable())?"Enable":"Disabled");
		}
		else if (strcmp(ppArgv[1], "disable") == 0)
		{
			TRACE_SetEnable(false);
			SHELL_Printf("%16s : %s\n", "Mode", (TRACE_GetEnable())?"Enable":"Disabled");
		}

	}

	return	0;
}

int	SHELL_CMD_Help(char *ppArgv[], int nArgc)
{
	SHELL_CMD	*pCmd = pShellCmds;

	while(pCmd->pName)
	{
		SHELL_Printf("%16s %s\n", pCmd->pName, pCmd->pHelp);
		pCmd++;
	}

	return	0;
}

SHELL_CMD	pShellCmds[] =
{
		{	"join", "join",	SHELL_CMD_Join},
		{	"req", "req",	SHELL_CMD_Req},
		{	"task", "task", SHELL_CMD_Task},
		{   "trace", "trace", SHELL_CMD_Trace},
		{	"?", "Help", SHELL_CMD_Help},
		{	NULL, NULL, NULL}
};
