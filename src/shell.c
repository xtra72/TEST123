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
#include "supervisor.h"
#include "global.h"
#include "lorawan_task.h"
#include "loramac_ex.h"
#include "utilities.h"

#undef	__MODULE__
#define	__MODULE__ "TRACE"

static char pBuffer[256];
xSemaphoreHandle	SHELLSemaphore;
//static StaticSemaphore_t xSHELLSemaphoreBuffer;
extern SHELL_CMD	pShellCmds[];

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
uint32_t	SHELL_Dump(const uint8_t *pData, uint32_t ulDataLen)
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
//		SHELL_Printf("S47> ");
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
		else if (strcasecmp(ppArgv[1], "pseudo") == 0)
		{
			DevicePostEvent(PSEUDO_JOIN_NETWORK);
		}
		else if (strcasecmp(ppArgv[1], "real") == 0)
		{
			DevicePostEvent(REAL_JOIN_NETWORK);
		}

	}

	return	0;
}


int	SHELL_CMD_Req(char *ppArgv[], int nArgc)
{
	if (nArgc == 2)
	{
		if (strcasecmp(ppArgv[1], "0") == 0)
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

int	SHELL_CMD_Set(char *ppArgv[], int nArgc)
{
	if (nArgc == 3)
	{
		if (strcasecmp(ppArgv[1], "appkey") == 0)
		{
			uint8_t pAppKey[16];
			if (HexString2Array(ppArgv[2], pAppKey, sizeof(pAppKey)) != 16)
			{
				SHELL_Printf("Invalid appkey!\n");
			}
			else
			{
				DeviceUserDataSetAppKey(pAppKey);
			}
		}
		else if (strcasecmp(ppArgv[1], "realappkey") == 0)
		{
			uint8_t pRealAppKey[16];
			if (HexString2Array(ppArgv[2], pRealAppKey, sizeof(pRealAppKey)) != 16)
			{
				SHELL_Printf("Invalid realappkey!\n");
			}
			else
			{
				DeviceUserDataSetSKTRealAppKey(pRealAppKey);
			}
		}
	}

	return	0;
}

int	SHELL_CMD_Get(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("%16s : ", "DevEUI");	SHELL_Dump(UNIT_DEVEUID, 8);
		SHELL_Printf("%16s : ", "Pseudo App Key");	SHELL_Dump(UNIT_APPKEY, 16);
		SHELL_Printf("%16s : ", "Real App Key");	SHELL_Dump(UNIT_REALAPPKEY, 16);
	}
	else if (nArgc == 2)
	{
		if (strcasecmp(ppArgv[1], "deui") == 0)
		{
			SHELL_Dump(UNIT_DEVEUID, 8);
		}
		else if (strcasecmp(ppArgv[1], "appkey") == 0)
		{
			SHELL_Dump(UNIT_APPKEY, 16);
		}
		else if (strcasecmp(ppArgv[1], "realappkey") == 0)
		{
			SHELL_Dump(UNIT_REALAPPKEY, 16);
		}
	}

	return	0;
}

int	SHELL_CMD_Report(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("%16s : %s\n", "Status", (SUPERVISOR_IsCyclicTaskRun())?"run":"stop");
		SHELL_Printf("%16s : %d\n", "Period", SUPERVISOR_GetRFPeriod());
	}
	else if (nArgc == 2)
	{
		if (strcasecmp(ppArgv[1], "on") == 0)
		{
			SUPERVISOR_StartCyclicTask(0, SUPERVISOR_GetRFPeriod());
			DeviceUserDataSetFlag(FLAG_TRANS_ON, FLAG_TRANS_ON);
		}
		else if (strcasecmp(ppArgv[1], "off") == 0)
		{
			SUPERVISOR_StopCyclicTask();
			DeviceUserDataSetFlag(FLAG_TRANS_ON,0);
		}
		else if (strcasecmp(ppArgv[1], "period") == 0)
		{
			SHELL_Printf("%d\n", SUPERVISOR_GetRFPeriod());
		}
		else if (strcasecmp(ppArgv[1], "now") == 0)
		{
			DevicePostEvent(PERIODIC_RESEND);
		}
	}
	else if (nArgc == 3)
	{
		if (strcasecmp(ppArgv[1], "period") == 0)
		{
			uint32_t ulPeriod = atoi(ppArgv[2]);

			if (!SUPERVISOR_SetRFPeriod(ulPeriod))
			{
				SHELL_Printf("Invalid period!\n");
			}
		}
	}

	return	0;
}


int	SHELL_CMD_Mac(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		switch(LORAMAC_GetClassType())
		{
		case	CLASS_A:	SHELL_Printf("%16s : Class A\n", "Device Class"); break;
		case	CLASS_B:	SHELL_Printf("%16s : Class B\n", "Device Class"); break;
		case	CLASS_C:	SHELL_Printf("%16s : Class C\n", "Device Class"); break;
		default:			SHELL_Printf("%16s : Unknown\n", "Device Class"); break;
		}

		SHELL_Printf("%16s : %d\n", "RSSI", LORAWAN_GetRSSI());
		SHELL_Printf("%16s : %d\n", "SNR", LORAWAN_GetSNR());
		SHELL_Printf("%16s : %d\n", "DATARATE", LORAMAC_GetDatarate());
		SHELL_Printf("%16s : %d\n", "Tx Power", LORAMAC_GetTxPower());
		SHELL_Printf("%16s : %d\n", "Retransmission", LORAWAN_GetMaxRetries());

		uint32_t	AppNonce;
		LORAMAC_GetAppNonce(&AppNonce);
		SHELL_Printf("%16s : %06x\n", "App Nonce", AppNonce);

	}
	else if (nArgc == 3)
	{

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

		vTaskGetInfo( hSuperTask, &xStatus, true, eInvalid);
		SHELL_Printf("%16s : %d\n", xStatus.pcTaskName, xStatus.usStackHighWaterMark);

		vTaskGetInfo( hShellTask, &xStatus, true, eInvalid);
		SHELL_Printf("%16s : %d\n", xStatus.pcTaskName, xStatus.usStackHighWaterMark);
	}

	return	0;
}

int	SHELL_CMD_LoRaWAN(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("%16s : %s\n", "Status", LORAWAN_GetStatusString());
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

int SHELL_CMD_AT(char *ppArgv[], int nArgc)
{
	SHELL_CMD*	pCmd = pShellCmds;

	while(pCmd->pName != NULL)
	{
		if ((pCmd->pName[0] == 'A') && (pCmd->pName[1] == 'T') && (pCmd->pName[2] == '+'))
		{
			SHELL_Printf("%-16s : %s\n", &pCmd->pName[3], pCmd->pHelp);
		}
		pCmd++;
	}

	return	0;
}

int SHELL_CMD_AT_Reset(char *ppArgv[], int nArgc)
{
	SHELL_Printf("RESET OK\n");
	vTaskDelay(configTICK_RATE_HZ);
	SystemReboot();
	return	0;
}

int SHELL_CMD_AT_PS(char *ppArgv[], int nArgc)
{
	CLEAR_USERFLAG(UNIT_INSTALLED);
	return	0;
}

int SHELL_CMD_AT_GetConfig(char *ppArgv[], int nArgc)
{
	return	0;
}

int SHELL_CMD_AT_FirmwareInfo(char *ppArgv[], int nArgc)
{
	return	0;
}

int SHELL_CMD_AT_FirmwareUpgrade(char *ppArgv[], int nArgc)
{
	return	0;
}


int SHELL_CMD_AT_DevEUI(char *ppArgv[], int nArgc)
{
	SHELL_Printf("Device EUI : ");	SHELL_Dump(UNIT_DEVEUID, 8);
	return	0;
}

int SHELL_CMD_AT_AppKey(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("GET APPLICATION KEY\n");
		SHELL_Printf("- Application Key : ");	SHELL_Dump(UNIT_APPKEY, LORAMAC_APP_KEY_SIZE);
	}
	else
	{
		uint8_t pAppKey[16];

		SHELL_Printf("SET APPLICATION KEY\n");

		if ((nArgc == 2) && (HexString2Array(ppArgv[1], pAppKey, sizeof(pAppKey)) == LORAMAC_APP_KEY_SIZE))
		{
			DeviceUserDataSetAppKey(pAppKey);
			SHELL_Printf("- Application Key : ");	SHELL_Dump(UNIT_APPKEY, LORAMAC_APP_KEY_SIZE);
		}
		else
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
	}

	return	0;
}

int SHELL_CMD_AT_AppEUI(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("GET APPLICATION EUI\n");
		SHELL_Printf("- Application EUI : ");	SHELL_Dump(UNIT_APPEUID, LORAMAC_APP_EUI_SIZE);
	}
	else
	{
		uint8_t pAppEUI[LORAMAC_APP_EUI_SIZE];

		SHELL_Printf("SET APPLICATION EUI\n");
		if ((nArgc == 2) && (HexString2Array(ppArgv[1], pAppEUI, sizeof(pAppEUI)) == sizeof(pAppEUI)))
		{
			DeviceUserDataSetAppEUI(pAppEUI);
			SHELL_Printf("- Application EUI : ");	SHELL_Dump(pAppEUI, sizeof(pAppEUI));
		}
		else
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
	}

	return	0;
}

int SHELL_CMD_AT_SetTxDR(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("GET TX DATA RATE\n");
		SHELL_Printf("- Data Rate : %d\n", LORAMAC_GetDatarate());
	}
	else
	{
		SHELL_Printf("SET TX DATA RATE\n");
		if (nArgc == 2)
		{
			int8_t	nDatarate = atoi(ppArgv[1]);
			if ((LORAMAC_TX_DATARATE_MIN <= nDatarate) && (nDatarate <= LORAMAC_TX_DATARATE_MAX))
			{
				LORAMAC_SetDatarate(nDatarate);
				SHELL_Printf("- Data Rate : %d\n", nDatarate);
				return	0;
			}
		}

		SHELL_Printf("- ERROR, Invalid Arguments\n");
	}

	return	0;
}

int SHELL_CMD_AT_SetTxPower(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("GET TX POWER\n");
		SHELL_Printf("- Tx Power Index : %d\n", LORAMAC_GetTxPower());
	}
	else
	{
		SHELL_Printf("SET TX POWER\n");

		if (nArgc == 2)
		{
			int8_t	nTxPower = atoi(ppArgv[1]);
			if ((LORAMAC_TX_POWER_MIN <= nTxPower) && (nTxPower <= LORAMAC_TX_POWER_MAX))
			{
				LORAMAC_SetTxPower(nTxPower);
				SHELL_Printf("- Tx Power Index : %d\n", LORAMAC_GetTxPower());
				return	0;
			}
		}

		SHELL_Printf("- ERROR, Invalid Arguments\n");
	}

	return	0;
}

int SHELL_CMD_AT_SetChannelAndTxPower(char *ppArgv[], int nArgc)
{

	return	0;
}

int SHELL_CMD_AT_Channel(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("GET TX CHANNEL\n");

	}
	else
	{
		SHELL_Printf("SET TX CHANNEL\n");

		if (nArgc == 2)
		{
			int8_t	nTxChannel = atoi(ppArgv[1]);
			if ((LORAMAC_TX_CHANNEL_MIN <= nTxChannel) && (nTxChannel <= LORAMAC_TX_CHANNEL_MAX))
			{
				SHELL_Printf("- Tx Channel : %d\n", nTxChannel);
				return	0;
			}
		}

		SHELL_Printf("- ERROR, Invalid Arguments\n");
	}

	return	0;
}

int SHELL_CMD_AT_ADR(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("GET ADAPTIVE DATA RATE FLAG\n");
		SHELL_Printf("Adaptive Data Rate Flag : ");

	}
	else if (nArgc == 2)
	{
		SHELL_Printf("SET ADAPTIVE DATA RATE FLAG\n");

		int8_t	bEnable = atoi(ppArgv[1]);
		if (bEnable != 0 && bEnable != 1)
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
		else
		{
			SHELL_Printf("Adaptive Data Rate Flag : ");
		}
	}
	else
	{
		SHELL_Printf("SET ADAPTIVE DATA RATE FLAG\n");
		SHELL_Printf("- ERROR, Invalid Arguments\n");
	}

	return	0;
}

int SHELL_CMD_AT_CLS(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("GET CLASS\n");
		SHELL_Printf("Class  : %c\n", LORAMAC_GetClassType() + 'A');

	}
	else
	{
		bool	ret = false;

		SHELL_Printf("SET CLASS\n");

		if(nArgc == 2)
		{
			if  (strcasecmp(ppArgv[1], "A") == 0)
			{
				LORAMAC_SetClassType(CLASS_A);
				ret = true;
			}
			else if (strcasecmp(ppArgv[1], "C") == 0)
			{
				LORAMAC_SetClassType(CLASS_C);
				ret = true;
			}
		}

		if (ret == true)
		{
			SHELL_Printf("Class  : %c\n", LORAMAC_GetClassType() + 'A');
		}
		else
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
	}

	return	0;
}

int SHELL_CMD_AT_LatestSignal(char *ppArgv[], int nArgc)
{
	SHELL_Printf("Latest RF Signal  : %d %d\n", LORAWAN_GetRSSI(), LORAWAN_GetSNR());

	return	0;
}

int SHELL_CMD_AT_TxRetransmissionNumber(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("GET Retransmission Number\n");
		SHELL_Printf("- Count  : %d\n", LORAWAN_GetMaxRetries());
	}
	else
	{
		bool	ret = false;

		SHELL_Printf("SET Retransmission Number\n");

		if (nArgc == 2)
		{
			int8_t	nRetries = atoi(ppArgv[1]);
			if ((LORAWAN_RETRANSMISSION_MIN <= nRetries) && ( nRetries <= LORAWAN_RETRANSMISSION_MAX))
			{
				LORAWAN_SetMaxRetries(nRetries);
				ret = true;
			}
		}

		if (ret == true)
		{
			SHELL_Printf("- Count  : %d\n", LORAWAN_GetMaxRetries());
		}
		else
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
	}

	return	0;
}

int SHELL_CMD_AT_Send(char *ppArgv[], int nArgc)
{
	static	uint8_t pData[128];
	uint8_t			nDataLen = 0;

	if (nArgc == 1)
	{
		SKTAPP_Send(0, LORAWAN_APP_PORT, NULL, 0);
	}
	else if (nArgc == 2)
	{
		uint32_t	ulLen = strlen(ppArgv[1]);

		nDataLen = HexString2Array( ppArgv[1], pData, sizeof(pData));
		if (ulLen > nDataLen * 2)
		{
			SHELL_Printf("- ERROR, Max Payload Size : %d Byte\n", sizeof(pData));
		}
		else if (nDataLen == 0)
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
		else
		{
			SKTAPP_Send(0, LORAWAN_APP_PORT, pData, nDataLen);
		}
	}
	else
	{
		SHELL_Printf("- ERROR, Invalid Arguments\n");
	}

	return	0;
}

int SHELL_CMD_AT_ACK(char *ppArgv[], int nArgc)
{
	SKTAPP_SendAck();

	return	0;
}

int SHELL_CMD_AT_DutyCycleTime(char *ppArgv[], int nArgc)
{
	return	0;
}

int SHELL_CMD_AT_UnconfirmedRetransmissionNumber(char *ppArgv[], int nArgc)
{
	return	0;
}

int SHELL_CMD_AT_Confirmed(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("Enable/Disable confirmed message\n");
		SHELL_Printf("GET MSG TYPE\n");
		SHELL_Printf("- MSG TYPE: %s\n", (SKTAPP_IsConfirmedMsgType()?"Confirmed":"Unconfirmed"));
	}
	else
	{
		bool	ret = false;

		SHELL_Printf("Enable/Disable confirmed message\n");
		SHELL_Printf("SET MSG TYPE\n");
		if (nArgc == 2)
		{
			if (strcmp(ppArgv[1], "1") == 0)
			{
				SKTAPP_SetConfirmedMsgType(true);
				ret = true;
			}
			else if (strcmp(ppArgv[1], "0") == 0)
			{
				SKTAPP_SetConfirmedMsgType(false);
				ret = true;
			}

		}

		if (ret == true)
		{
			SHELL_Printf("- MSG TYPE: %s\n", (SKTAPP_IsConfirmedMsgType()?"Confirmed":"Unconfirmed"));
		}
		else
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
	}

	return	0;
}

int SHELL_CMD_AT_Log(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("Get LOG enable/disable\n");
		if (TRACE_GetEnable())
		{
			SHELL_Printf("- LOG Message Enabled.\n");
		}
		else
		{
			SHELL_Printf("- LOG Message Disabled.\n");
		}
	}
	else
	{
		bool	ret = false;

		SHELL_Printf("Set LOG enable/disable\n");

		if (nArgc == 2)
		{
			if (strcmp(ppArgv[1], "0") == 0)
			{
				TRACE_SetEnable(false);
				ret = true;
			}
			if (strcmp(ppArgv[1], "1") == 0)
			{
				TRACE_SetEnable(true);
				ret = true;
			}
		}

		if (ret == true)
		{
			if (TRACE_GetEnable())
			{
				SHELL_Printf("- LOG Message Enabled.\n");
			}
			else
			{
				SHELL_Printf("- LOG Message Disabled.\n");
			}
		}
		else
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
	}

	return	0;
}

int SHELL_CMD_AT_PRF(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("Get Period Report Flag\n");
		SHELL_Printf("- Period Report Status : %s Mode\n", (SKTAPP_GetPeriodicMode()?"Timer":"Event"));
	}
	else
	{
		bool	ret = false;

		SHELL_Printf("Set Period Report Flag\n");

		if (nArgc == 2)
		{
			if (strcmp(ppArgv[1], "0") == 0)
			{
				SKTAPP_SetPeriodicMode(false);
				ret = true;
			}
			if (strcmp(ppArgv[1], "1") == 0)
			{
				SKTAPP_SetPeriodicMode(true);
				ret = true;
			}
		}

		if (ret == true)
		{
			SHELL_Printf("- Period Report Status : %s Mode\n", (SKTAPP_GetPeriodicMode()?"Timer":"Event"));
		}
		else
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
	}

	return	0;
}

int SHELL_CMD_AT_FCNT(char *ppArgv[], int nArgc)
{
	return	0;
}

int SHELL_CMD_AT_BATT(char *ppArgv[], int nArgc)
{
	return	0;
}

SHELL_CMD	pShellCmds[] =
{
		{	"AT",	  "Checking the serial connection status", SHELL_CMD_AT},
		{	"AT+RST", "Reset",	SHELL_CMD_AT_Reset},
		{	"AT+PS",	"Pseudo Join",	SHELL_CMD_AT_PS},
		{	"AT+GCFG", "Get Config",	SHELL_CMD_AT_GetConfig},
		{	"AT+FWI", "Firmware Information",	SHELL_CMD_AT_FirmwareInfo},
		{	"AT+DEUI", "Get Device EUI",	SHELL_CMD_AT_DevEUI},
		{	"AT+AK", "Set/Get Application Key",	SHELL_CMD_AT_AppKey},
		{	"AT+AEUI", "Set/Get Application EUI",	SHELL_CMD_AT_AppEUI},
		{	"AT+DR", "Set Tx Data Rate",	SHELL_CMD_AT_SetTxDR},
		{	"AT+POW", "Set Tx Power",	SHELL_CMD_AT_SetTxPower},
		{	"AT+CHTX", "Set Channel and Tx Power",	SHELL_CMD_AT_SetChannelAndTxPower},
		{	"AT+CH", "Set/Get Channel",	SHELL_CMD_AT_Channel},
		{	"AT+ADR", "Set/Get ADR Flag",	SHELL_CMD_AT_ADR},
		{	"AT+CLS", "Set/Get Class",	SHELL_CMD_AT_CLS},
		{	"AT+SIG", "Latest RF Signal",	SHELL_CMD_AT_LatestSignal},
		{	"AT+RCNT", "Tx Retransmission Number",	SHELL_CMD_AT_TxRetransmissionNumber},
		{	"AT+SEND", "Sending the user defined packet",	SHELL_CMD_AT_Send},
		{	"AT+ACK", "Sending ack",	SHELL_CMD_AT_ACK},
		{	"AT+DUTC", "Set/Get Duty Cycle Time",	SHELL_CMD_AT_DutyCycleTime},
		{	"AT+RUNT", "Tx Retransmission Number(Unconfirmed)",	SHELL_CMD_AT_UnconfirmedRetransmissionNumber},
		{	"AT+CFM", "Enable or disable Confirmed message",	SHELL_CMD_AT_Confirmed},
		{	"AT+LOG", "Enable or Disable Log message",	SHELL_CMD_AT_Log},
		{	"AT+PRF", "Select Timer or Event Mode (Period Report)",	SHELL_CMD_AT_PRF},
		{	"AT+FCNT", "changing the down link fcnt for testing",	SHELL_CMD_AT_FCNT},
		{	"AT+BATT", "Setting battery level",	SHELL_CMD_AT_BATT},

		{	"join", "join",	SHELL_CMD_Join},
		{	"req", "req",	SHELL_CMD_Req},
		{	"task", "task", SHELL_CMD_Task},
		{	"set", "set environment variables", SHELL_CMD_Set},
		{	"get", "get environment variables", SHELL_CMD_Get},
		{	"report", "report", SHELL_CMD_Report},
		{	"mac", "LoRaMAC", SHELL_CMD_Mac},
		{	"lorawan", "lorawan",	SHELL_CMD_LoRaWAN},
		{   "trace", "trace", SHELL_CMD_Trace},
		{	"?", "Help", SHELL_CMD_Help},
		{	NULL, NULL, NULL}
};
