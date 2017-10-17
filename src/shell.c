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
#include <time.h>
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
#include "SKTApp.h"
#undef	__MODULE__
#define	__MODULE__ "TRACE"

static char pBuffer[512];
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
		//S47_SHELL_BAUDRATE_DEFAULT,
		9600,/* 9600 bits/s. */                                                     \
		S47_SHELL_DATABITS_DEFAULT, /* 8 databits. */                                                      \
		S47_SHELL_PARITY_DEFAULT,  /* No parity. */                                                       \
		S47_SHELL_STOPBITS_DEFAULT  /* 1 stopbit. */                                                       \
	  };

  /* Enable CORE LE clock in order to access LE modules */
  CMU_ClockEnable(cmuClock_CORELE, true);

  /* Select LFXO for LEUARTs (and wait for it to stabilize) */
  //CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_HFCLKLE);
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

  /* Finally enable it */
  LEUART_Enable(LEUART0, leuartEnable);
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
			vTaskDelay(0);
		}
		else
		{
			vTaskDelay(1);
		}
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

static	char pLine[256];
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

int	AT_CMD_Join(char *ppArgv[], int nArgc)
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

int	AT_CMD_CTM(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("GET CYCLIC MODE\n");
		SHELL_Printf("- %16s : %s\n", "Status", (SUPERVISOR_IsCyclicTaskRun())?"run":"stop");
		SHELL_Printf("- %16s : %d\n", "Period", SUPERVISOR_GetRFPeriod());
	}
	else
	{
		bool	ret = false;

		if (nArgc == 2)
		{
			if (strcasecmp(ppArgv[1], "on") == 0)
			{
				SUPERVISOR_SetPeriodicMode(true);
				SUPERVISOR_StartCyclicTask(0, SUPERVISOR_GetRFPeriod());

				ret = true;
			}
			else if (strcasecmp(ppArgv[1], "off") == 0)
			{
				SUPERVISOR_StopCyclicTask();
				SUPERVISOR_SetPeriodicMode(false);

				ret = true;
			}
			else if (strcasecmp(ppArgv[1], "period") == 0)
			{
				SHELL_Printf("%d\n", SUPERVISOR_GetRFPeriod());

				ret = true;
			}
			else if (strcasecmp(ppArgv[1], "now") == 0)
			{
				DevicePostEvent(PERIODIC_RESEND);

				ret = true;
			}
		}
		else if (nArgc == 3)
		{
			if (strcasecmp(ppArgv[1], "period") == 0)
			{
				uint32_t ulPeriod = atoi(ppArgv[2]);

				if (SUPERVISOR_SetRFPeriod(ulPeriod))
				{
					ret = true;
				}
				else
				{
					SHELL_Printf("Invalid period!\n");
				}
			}
		}

		if (!ret)
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
	}


	return	0;
}


int	AT_CMD_Mac(char *ppArgv[], int nArgc)
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

int	AT_CMD_Task(char *ppArgv[], int nArgc)
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

int	AT_CMD_Status(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("%16s : %s\n", "LoRaWAN", LORAWAN_GetStatusString());
	}

	return	0;
}

int	AT_CMD_Trace(char *ppArgv[], int nArgc)
{
	int	nRet = -1;

	if (nArgc == 1)
	{
		nRet = 0;
	}
	else if (nArgc == 2)
	{
		if (strcmp(ppArgv[1], "enable") == 0)
		{
			TRACE_SetEnable(true);
			nRet = 0;
		}
		else if (strcmp(ppArgv[1], "disable") == 0)
		{
			TRACE_SetEnable(false);
			nRet = 0;
		}
		else if (strcmp(ppArgv[1], "dump") == 0)
		{
			TRACE_SetDumpEnable(true);
			nRet = 0;
		}
		else if (strcmp(ppArgv[1], "undump") == 0)
		{
			TRACE_SetDumpEnable(false);
			nRet = 0;
		}
	}

	if (nRet == 0)
	{
		SHELL_Printf("%16s : %s\n", "Mode", (TRACE_GetEnable())?"Enable":"Disabled");
		SHELL_Printf("%16s : %s\n", "Dump", (TRACE_GetDumpEnable())?"Enable":"Disabled");
	}

	return	nRet;
}

int	AT_CMD_Help(char *ppArgv[], int nArgc)
{
	SHELL_CMD	*pCmd = pShellCmds;

	while(pCmd->pName)
	{
		SHELL_Printf("%16s %s\n", pCmd->pName, pCmd->pHelp);
		pCmd++;
	}

	return	0;
}

int AT_CMD(char *ppArgv[], int nArgc)
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

int AT_CMD_Reset(char *ppArgv[], int nArgc)
{
	SHELL_Printf("RESET OK\n");
	DevicePostEvent(SYSTEM_RESET);
	return	0;
}

int AT_CMD_PS(char *ppArgv[], int nArgc)
{
	CLEAR_USERFLAG(UNIT_USE_RAK);
	return	0;
}

int AT_CMD_GetConfig(char *ppArgv[], int nArgc)
{
	SHELL_Printf("Get Configuration\n");

	SHELL_Printf("- Current Channel : %d, Channel Tx Power : %d\n", 922100000 + 200000 * LoRaMacTestGetChannel(), LoRaMacTestGetChannelsTxPower());

	SHELL_Printf("- MaxDCycle : %d, AggregatedDCycle : %d\n", LoRaMacTestGetMaxDCycle(), LoRaMacTestGetAggreagtedDCycle());
	SHELL_Printf("- Current Rx1 DR Offset : %d, Rx2 DataRate : %d\n", LoRaMacTestGetRx1DrOffset(), LoRaMacTestGetRx2Datarate());

	for(uint32_t i = 0 ; i < 16 ; i++)
	{
		ChannelParams_t channel;

		if (!LORAMAC_GetChannel(i, &channel))
		{
			break;
		}
		SHELL_Printf("- Channels[%d] : %d, Band : %d\n", i, channel.Frequency, channel.Band);
	}
#if 0
	for(uint32_t i = 0 ; i <= LORAMAC_TX_CHANNEL_MAX - LORAMAC_TX_CHANNEL_MIN ; i++)
	{
		SHELL_Printf("- Channels[%d] : %d, Band : 0\n", i, 922100000 + i * 200000);
	}
#endif

	SHELL_Printf("- Channels Mask : %04x\n", LORAMAC_GetChannelsMask());
	SHELL_Printf("- Rx1 Delay : %d, Rx2 Delay : %d\n", LORAMAC_GetRx1Delay(), LORAMAC_GetRx2Delay());
	SHELL_Printf("- Join Delay1 : %d, Join Delay2 : %d\n", LORAMAC_GetJoinDelay1(), LORAMAC_GetJoinDelay2());
	SHELL_Printf("- Retransmission Count : %d\n", LORAMAC_GetRetries());

	return	0;
}

int AT_CMD_FirmwareInfo(char *ppArgv[], int nArgc)
{
	SHELL_Printf("Firmware Information\n");
	SHELL_Printf("- Version : %s\n", DeviceVersion());

	return	0;
}

int AT_CMD_FirmwareUpgrade(char *ppArgv[], int nArgc)
{
	return	0;
}


int AT_CMD_DevEUI(char *ppArgv[], int nArgc)
{
	SHELL_Printf("Device EUI : ");	SHELL_Dump(UNIT_DEVEUID, 8);
	return	0;
}

int AT_CMD_AppKey(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("GET APPLICATION KEY\n");
		SHELL_Printf("- Application Key : ");	SHELL_Dump(UNIT_APPKEY, LORAMAC_APP_KEY_SIZE);
	}
	else
	{
		bool	ret = false;
		uint8_t pAppKey[16];

		SHELL_Printf("SET APPLICATION KEY\n");

		if ((nArgc == 2) && (HexString2Array(ppArgv[1], pAppKey, sizeof(pAppKey)) == LORAMAC_APP_KEY_SIZE))
		{
			DeviceUserDataSetAppKey(pAppKey);
			ret = true;
		}

		if (ret)
		{
			SHELL_Printf("- Application Key : ");	SHELL_Dump(UNIT_APPKEY, LORAMAC_APP_KEY_SIZE);
		}
		else
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
	}

	return	0;
}

int AT_CMD_AppEUI(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("GET APPLICATION EUI\n");
		SHELL_Printf("- Application EUI : ");	SHELL_Dump(UNIT_APPEUID, LORAMAC_APP_EUI_SIZE);
	}
	else
	{
		bool	ret = false;
		uint8_t pAppEUI[LORAMAC_APP_EUI_SIZE];

		SHELL_Printf("SET APPLICATION EUI\n");
		if ((nArgc == 2) && (HexString2Array(ppArgv[1], pAppEUI, sizeof(pAppEUI)) == sizeof(pAppEUI)))
		{
			DeviceUserDataSetAppEUI(pAppEUI);
			ret = true;
		}

		if (ret)
		{
			SHELL_Printf("- Application EUI : ");	SHELL_Dump(pAppEUI, sizeof(pAppEUI));
		}
		else
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
	}

	return	0;
}


int AT_CMD_RealAppKey(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("GET REAL APPLICATION KEY\n");
		SHELL_Printf("- Real Application Key : ");	SHELL_Dump(UNIT_REALAPPKEY, LORAMAC_APP_KEY_SIZE);
	}
	else
	{
		SHELL_Printf("- ERROR, Invalid Arguments\n");
	}

	return	0;
}

int AT_CMD_SetTxDR(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("GET TX DATA RATE\n");
		SHELL_Printf("- Data Rate : %d\n", LORAMAC_GetDatarate());
	}
	else
	{
		bool	ret = false;

		SHELL_Printf("SET TX DATA RATE\n");
		if (nArgc == 2)
		{
			int8_t	nDatarate = atoi(ppArgv[1]);
			if ((LORAMAC_TX_DATARATE_MIN <= nDatarate) && (nDatarate <= LORAMAC_TX_DATARATE_MAX))
			{
				ret = LORAMAC_SetDatarate(nDatarate);
			}
		}

		if (ret)
		{
			SHELL_Printf("- Data Rate : %d\n", LORAMAC_GetDatarate());
		}
		else
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
	}

	return	0;
}

int AT_CMD_TxPower(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("GET TX POWER\n");
		SHELL_Printf("- Tx Power Index : %d\n", LORAMAC_GetTxPower());
	}
	else
	{
		bool	ret = false;

		SHELL_Printf("SET TX POWER\n");

		if (nArgc == 2)
		{
			int8_t	nTxPower = atoi(ppArgv[1]);
			if ((LORAMAC_TX_POWER_MIN <= nTxPower) && (nTxPower <= LORAMAC_TX_POWER_MAX))
			{
				ret = LORAMAC_SetTxPower(nTxPower);
			}
		}

		if (ret)
		{
			SHELL_Printf("- Tx Power Index : %d\n", LORAMAC_GetTxPower());
		}
		else
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
	}

	return	0;
}

int AT_CMD_SetChannelAndTxPower(char *ppArgv[], int nArgc)
{
	bool	ret = false;

	SHELL_Printf("Set Channel & Tx Power\n");
	if (nArgc == 3)
	{
		int8_t	nTxChannel = atoi(ppArgv[1]);
		if (nTxChannel == 25)
		{
			int8_t	nTxPower = atoi(ppArgv[1]);
			if ((2 <= nTxPower) && (nTxPower <= LORAMAC_TX_POWER_MAX))
			{
				ret = LoRaMacTestSetChannel( nTxChannel );
				ret &= LORAMAC_SetTxPower(nTxPower);
			}
		}
		else if ((LORAMAC_TX_CHANNEL_MIN <= nTxChannel) && (nTxChannel <= LORAMAC_TX_CHANNEL_MAX))
		{
			int8_t	nTxPower = atoi(ppArgv[1]);
			if ((LORAMAC_TX_POWER_MIN <= nTxPower) && (nTxPower <= LORAMAC_TX_POWER_MAX))
			{
				ret = LoRaMacTestSetChannel( nTxChannel );
				ret &= LORAMAC_SetTxPower(nTxPower);
			}
		}
	}

	if (ret)
	{
		SHELL_Printf("- Set Tx Channel : %d, PWR index : %d\n", LoRaMacTestGetChannel(), LORAMAC_GetTxPower());
	}
	else
	{
		SHELL_Printf("- ERROR, Invalid Arguments\n");
	}
	return	0;
}

int AT_CMD_Channel(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("GET TX CHANNEL\n");
		SHELL_Printf("- Tx Channel : %d\n", 921900000 + 200000 * LoRaMacTestGetChannel());
	}
	else
	{
		bool	ret = false;

		SHELL_Printf("SET TX CHANNEL\n");

		if (nArgc == 2)
		{
			int8_t	nTxChannel = atoi(ppArgv[1]);
			if ((LORAMAC_TX_CHANNEL_MIN <= nTxChannel) && (nTxChannel <= LORAMAC_TX_CHANNEL_MAX))
			{
				ret = LoRaMacTestSetChannel( nTxChannel - LORAMAC_TX_CHANNEL_MIN + 1);
			}
		}

		if (ret)
		{
			SHELL_Printf("- Tx Channel : %d\n", LoRaMacTestGetChannel());
		}
		else
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
	}

	return	0;
}

int AT_CMD_ADR(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("GET ADAPTIVE DATA RATE FLAG\n");
		SHELL_Printf("Adaptive Data Rate Flag : %s\n", (LORAMAC_GetADR()?"Enable":"Disable"));

	}
	else
	{
		bool	ret = false;

		if (nArgc == 2)
		{
			SHELL_Printf("SET ADAPTIVE DATA RATE FLAG\n");

			int8_t	bEnable = atoi(ppArgv[1]);
			if (bEnable == 0)
			{
				ret = LORAMAC_SetADR(false);
			}
			else if (bEnable == 1)
			{
				ret = LORAMAC_SetADR(true);
			}
		}

		if (ret)
		{
			SHELL_Printf("Adaptive Data Rate Flag : %s\n", (LORAMAC_GetADR()?"Enable":"Disable"));
		}
		else
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
	}

	return	0;
}

int AT_CMD_CLS(char *ppArgv[], int nArgc)
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
				ret = LORAMAC_SetClassType(CLASS_A);
			}
			else if (strcasecmp(ppArgv[1], "C") == 0)
			{
				ret = LORAMAC_SetClassType(CLASS_C);
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

int AT_CMD_LatestSignal(char *ppArgv[], int nArgc)
{
	SHELL_Printf("GET Latest Signal\n");
	SHELL_Printf("- RSSI  : %d\n", LORAWAN_GetRSSI());
	SHELL_Printf("- SNR   : %d\n", LORAWAN_GetSNR());

	return	0;
}

int AT_CMD_TxRetransmissionNumber(char *ppArgv[], int nArgc)
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
				ret = LORAWAN_SetMaxRetries(nRetries);
			}
		}

		if (ret)
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

int AT_CMD_Send(char *ppArgv[], int nArgc)
{
	static	uint8_t pData[256];
	uint8_t			nDataLen = 0;

	SHELL_Printf("SEND PACKET\n");
	if (nArgc == 2)
	{
		uint32_t	ulLen = strlen(ppArgv[1]);

		nDataLen = HexString2Array( ppArgv[1], pData, sizeof(pData));
		if (ulLen > nDataLen * 2)
		{
			SHELL_Printf("- ERROR, Max Payload Size : %d Byte\n", sizeof(pData) - 1);
		}
		else if (nDataLen == 0)
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
		else
		{
			if (SKTAPP_Send(pData[0], 0, &pData[1], nDataLen - 1) == false)
			{
				SHELL_Printf("- ERROR, Failed to send packet!\n");
			}
		}
	}
	else
	{
		SHELL_Printf("- ERROR, Invalid Arguments\n");
	}

	return	0;
}

int AT_CMD_SendAck(char *ppArgv[], int nArgc)
{
	SHELL_Printf("SEND ACK\n");

	if (LORAWAN_SendAck() == false)
	{
		SHELL_Printf("- ERROR, Failed to send ACK!\n");
	}

	return	0;
}


int AT_CMD_LinkCheck(char *ppArgv[], int nArgc)
{
	LORAWAN_SendLinkCheckRequest();

	return	0;
}

int AT_CMD_DeviceTimeRequest(char *ppArgv[], int nArgc)
{
	LORAWAN_SendDevTimeReq();

	return	0;
}

int AT_CMD_DutyCycleTime(char *ppArgv[], int nArgc)
{
	return	0;
}

int AT_CMD_UnconfirmedRetransmissionNumber(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("Get Tx Unconfirmed Retransmission Count\n");
		SHELL_Printf("- Count : %d\n", LORAMAC_GetChannelsNbRepeat());
	}
	else
	{
		bool	ret = false;

		SHELL_Printf("Set Tx Unconfirmed Retransmission Count\n");
		if (nArgc == 2)
		{
			uint32_t	nCount = atoi(ppArgv[1]);
			if ((LORAMAC_TX_RETRANSMISSION_COUNT_MIN <= nCount) && (nCount <= LORAMAC_TX_RETRANSMISSION_COUNT_MAX))
			{
				ret = LORAMAC_SetChannelsNbRepeat(nCount);
			}
		}

		if (ret)
		{
			SHELL_Printf("- Count : %d\n", LORAMAC_GetChannelsNbRepeat());
		}
		else
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
	}

	return	0;
}

int AT_CMD_Confirmed(char *ppArgv[], int nArgc)
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
				ret = SKTAPP_SetConfirmedMsgType(true);
			}
			else if (strcmp(ppArgv[1], "0") == 0)
			{
				ret = SKTAPP_SetConfirmedMsgType(false);
			}

		}

		if (ret)
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

int AT_CMD_Log(char *ppArgv[], int nArgc)
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
				ret = TRACE_SetEnable(false);
			}
			if (strcmp(ppArgv[1], "1") == 0)
			{
				ret = TRACE_SetEnable(true);
			}
		}

		if (ret)
		{
			SHELL_Printf("- LOG Message %s.\n", (TRACE_GetEnable()?"Enabled":"Disabled"));
		}
		else
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
	}

	return	0;
}

int AT_CMD_PRF(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("GET PERIOD REPORT FLAG\n");
		SHELL_Printf("- Period Report Status : %s Mode\n", (SKTAPP_GetPeriodicMode()?"Timer":"Event"));
	}
	else
	{
		bool	ret = false;

		SHELL_Printf("SET PERIOD REPORT FLAG\n");

		if (nArgc == 2)
		{
			if (strcmp(ppArgv[1], "0") == 0)
			{
				ret = SKTAPP_SetPeriodicMode(false);
			}
			if (strcmp(ppArgv[1], "1") == 0)
			{
				ret = SKTAPP_SetPeriodicMode(true);
			}
		}

		if (ret)
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

int AT_CMD_FCNT(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("GET UP/DOWN LINK COUNTER\n");
		SHELL_Printf("- Up Link Counter : %d\n", LORAMAC_GetUpLinkCounter());
		SHELL_Printf("- Down Link Counter : %d\n", LORAMAC_GetDownLinkCounter());
	}
	else
	{
		bool	rc = false;

		if (nArgc == 3)
		{
			uint32_t	ulCounter = atoi(ppArgv[2]);

			if (strcasecmp(ppArgv[1], "up") == 0)
			{
				LORAMAC_SetUpLinkCounter(ulCounter);
				rc = true;
			}
			else if (strcasecmp(ppArgv[1], "down") == 0)
			{
				LORAMAC_SetDownLinkCounter(ulCounter);
				rc = true;
			}
		}

		if (rc)
		{
			SHELL_Printf("- Up Link Counter : %d\n", LORAMAC_GetUpLinkCounter());
			SHELL_Printf("- Down Link Counter : %d\n", LORAMAC_GetDownLinkCounter());
		}
		else
		{
			SHELL_Printf("- ERROR, Invalid Arguments\n");
		}
	}

	return	0;
}

int AT_CMD_BATT(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		SHELL_Printf("BATTERY INFORMATION\n");
		SHELL_Printf("- Battery Level : %d\n", BoardGetBatteryLevel());
	}

	return	0;
}

int AT_CMD_Sleep(char *ppArgv[], int nArgc)
{
	if (nArgc == 1)
	{
		vTaskDelay(5*1024);
	}

	return	0;
}

SHELL_CMD	pShellCmds[] =
{
		{	"AT",	  	"Checking the serial connection status", AT_CMD},
		{	"AT+RST", 	"Reset",	AT_CMD_Reset},
		{	"AT+PS",	"Pseudo Join",	AT_CMD_PS},
		{	"AT+JOIN",	"Join",	AT_CMD_Join},
		{	"AT+GCFG", 	"Get Configuration",	AT_CMD_GetConfig},
		{	"AT+FWI", 	"Firmware Information",	AT_CMD_FirmwareInfo},
		{	"AT+DEUI", 	"Get Device EUI",	AT_CMD_DevEUI},
		{	"AT+AK", 	"Set/Get Application Key",	AT_CMD_AppKey},
		{	"AT+RAK", 	"Get Real Application Key",	AT_CMD_RealAppKey},
		{	"AT+AEUI", 	"Set/Get Application EUI",	AT_CMD_AppEUI},
		{	"AT+DR", 	"Set Tx Data Rate",	AT_CMD_SetTxDR},
		{	"AT+POW", 	"Set Tx Power",	AT_CMD_TxPower},
		{	"AT+CHTX", 	"Set Channel and Tx Power",	AT_CMD_SetChannelAndTxPower},
		{	"AT+CH", 	"Set/Get Channel",	AT_CMD_Channel},
		{	"AT+ADR", 	"Set/Get ADR Flag",	AT_CMD_ADR},
		{	"AT+CLS", 	"Set/Get Class",	AT_CMD_CLS},
		{	"AT+SIG", 	"Latest RF Signal",	AT_CMD_LatestSignal},
		{	"AT+RCNT", 	"Tx Retransmission Number",	AT_CMD_TxRetransmissionNumber},
		{	"AT+SEND", 	"Sending the user defined packet",	AT_CMD_Send},
		{	"AT+ACK", 	"Sending ACK",	AT_CMD_SendAck},
		{	"AT+DUTC", 	"Set/Get Duty Cycle Time",	AT_CMD_DutyCycleTime},
		{	"AT+RUNT", 	"Tx Retransmission Number(Unconfirmed)",	AT_CMD_UnconfirmedRetransmissionNumber},
		{	"AT+CFM", 	"Enable or disable Confirmed message",	AT_CMD_Confirmed},
		{	"AT+LOG", 	"Enable or Disable Log message",	AT_CMD_Log},
		{	"AT+PRF", 	"Select Timer or Event Mode (Period Report)",	AT_CMD_PRF},
		{	"AT+CTM", 	"Set/Get Cyclic Time Mode", AT_CMD_CTM},
		{	"AT+FCNT", 	"changing the down link FCnt for testing",	AT_CMD_FCNT},
		{	"AT+BATT", 	"Battery",	AT_CMD_BATT},
		{	"AT+LCHK", 	"Link Check Request",	AT_CMD_LinkCheck},
		{	"AT+DEVT", 	"Dev Time Request",	AT_CMD_DeviceTimeRequest},
		{	"AT+TASK",	"Get Task Information",	AT_CMD_Task},
		{	"AT+STAT", 	"Get Status",	AT_CMD_Status},
		{   "AT+TRCE", 	"Get/Set Trace", AT_CMD_Trace},
		{	"AT+SLP",	"Sleep",	AT_CMD_Sleep},
		{	"AT+MAC",	"Get/Set MAC",	AT_CMD_Mac},
		{	"AT+HELP", 	"Help", AT_CMD_Help},
		{	NULL, NULL, NULL}
};
