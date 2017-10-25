/*
 * trace.c
 *
 *  Created on: 2017. 9. 13.
 *      Author: inhyuncho
 */

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
#include "global.h"
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

/*!
 * @brief Console output
 */
typedef	struct
{
	TRACE_LEVEL	xLevel;
	char*		pName;
} TRACE_LEVEL_CONFIG;

const static TRACE_LEVEL_CONFIG pTraceLevelInfo[] =
{
	{	TRACE_LEVEL_DEBUG_0,	"DEBUG0"},
	{	TRACE_LEVEL_DEBUG_1,	"DEBUG1"},
	{	TRACE_LEVEL_DEBUG_2,	"DEBUG2"},
	{	TRACE_LEVEL_DEBUG_3,	"DEBUG3"},
	{	TRACE_LEVEL_DEBUG_4,	"DEBUG4"},
	{	TRACE_LEVEL_DEBUG_5,	"DEBUG5"},
	{	TRACE_LEVEL_INFO,		"INFO"},
	{	TRACE_LEVEL_WRANING,	"WARNING"},
	{	TRACE_LEVEL_ERROR,		"ERROR"},
	{	TRACE_LEVEL_FATAL,		"FATAL"}
};

static TRACE_LEVEL	xTraceLevel = TRACE_LEVEL_DEBUG_3;
static char			pTraceBuffer[256];

void		TRACE_ShowConfig(void)
{
	SHELL_Printf("%16s : %s\n", "Mode", (TRACE_GetEnable())?"Enable":"Disabled");
	SHELL_Printf("%16s : %s\n", "Dump", (TRACE_GetDump())?"Enable":"Disabled");
	SHELL_Printf("%16s : %s\n", "Level", TRACE_GetLevelName(TRACE_GetLevel()));
	SHELL_Printf("%16s : %s\n", TRACE_GetModuleName(FLAG_TRACE_LORAMAC), (TRACE_GetModule(FLAG_TRACE_LORAMAC)?"Enable":"Disable"));
	SHELL_Printf("%16s : %s\n", TRACE_GetModuleName(FLAG_TRACE_LORAWAN), (TRACE_GetModule(FLAG_TRACE_LORAWAN)?"Enable":"Disable"));
	SHELL_Printf("%16s : %s\n", TRACE_GetModuleName(FLAG_TRACE_SKT), (TRACE_GetModule(FLAG_TRACE_SKT)?"Enable":"Disable"));
}

uint32_t	TRACE_Dump(TRACE_LEVEL xLevel, uint16_t xModule, uint8_t *pData, uint32_t ulDataLen, const char *pFormat, ...)
{
	uint32_t	nOutputLength = 0;

	if ((xLevel >= xTraceLevel) && UNIT_TRACE_DUMP && UNIT_TRACE_ENABLE && UNIT_TRACE(xModule))
	{
		if (pFormat != NULL)
		{
			uint32_t	ulTime = xTaskGetTickCount();
			uint32_t	ulLen = 0;
			va_list		xArgs;

			va_start(xArgs, pFormat);

			ulLen = snprintf(pTraceBuffer, sizeof(pTraceBuffer), "[%8lu][%16s] ", ulTime, TRACE_GetModuleName(xModule));
			ulLen +=vsnprintf(&pTraceBuffer[ulLen], sizeof(pTraceBuffer) - ulLen, pFormat, xArgs);

			va_end(xArgs);

			nOutputLength = SHELL_Print(pTraceBuffer, ulLen);
		}

		nOutputLength += SHELL_Dump(pData, ulDataLen);
	}

	return	nOutputLength;
}
/*!
 * @brief Console formatted output
 */
uint32_t	TRACE_Printf(TRACE_LEVEL xLevel, uint16_t xModule, const char *pFormat, ...)
{
	uint32_t	nOutputLength = 0;

	if ((xLevel >= xTraceLevel) && UNIT_TRACE_ENABLE && UNIT_TRACE(xModule))
	{
		uint32_t	ulTime = xTaskGetTickCount();
		uint32_t	ulLen = 0;
		va_list		xArgs;

		va_start(xArgs, pFormat);

		ulLen = snprintf(pTraceBuffer, sizeof(pTraceBuffer), "[%8lu][%16s] ", ulTime, TRACE_GetModuleName(xModule));
		ulLen +=vsnprintf(&pTraceBuffer[ulLen], sizeof(pTraceBuffer) - ulLen, pFormat, xArgs);

		va_end(xArgs);

		nOutputLength = SHELL_Print(pTraceBuffer, ulLen);
	}

	return	nOutputLength;
}

bool		TRACE_SetEnable(bool bEnable)
{
	UPDATE_TRACE_FLAG(FLAG_TRACE_ENABLE, bEnable);

	return	true;
}

bool		TRACE_GetEnable(void)
{
	return	UNIT_TRACE_ENABLE;
}

bool		TRACE_SetDump(bool bEnable)
{
	UPDATE_TRACE_FLAG(FLAG_TRACE_DUMP, bEnable);

	return	true;
}

bool		TRACE_GetDump(void)
{
	return	UNIT_TRACE_DUMP;
}

void		TRACE_SetModule(unsigned short xModuleFlag, bool bEnable)
{
	UPDATE_TRACE_FLAG(xModuleFlag, bEnable);
}

bool		TRACE_GetModule(unsigned short xModuleFlag)
{
	return	UNIT_TRACE(xModuleFlag);
}

const char*	TRACE_GetModuleName(unsigned short xModuleFlag)
{
	switch(xModuleFlag)
	{
	case	FLAG_TRACE_LORAMAC:		return	"LoRaMAC";
	case	FLAG_TRACE_LORAWAN:		return	"LoRaWAN";
	case	FLAG_TRACE_DALIWORKS:	return	"Daliworks";
	case	FLAG_TRACE_SKT:			return	"SKT";
	case	FLAG_TRACE_SUPERVISOR:	return	"Supervisor";

	}

	return	"S47";
}

bool	TRACE_SetLevel(char* pLevel)
{
	for(uint32_t i = 0 ; i < sizeof(pTraceLevelInfo) / sizeof(TRACE_LEVEL_INFO) ; i++)
	{
		if (strcasecmp(pLevel, pTraceLevelInfo[i].pName) == 0)
		{
			xTraceLevel = pTraceLevelInfo[i].xLevel;
			return	true;
		}
	}

	return	false;
}

TRACE_LEVEL	TRACE_GetLevel(void)
{
	return	xTraceLevel;
}

const char*	TRACE_GetLevelName(TRACE_LEVEL xLevel)
{
	for(uint32_t i = 0 ; i < sizeof(pTraceLevelInfo) / sizeof(TRACE_LEVEL_INFO) ; i++)
	{
		if (pTraceLevelInfo[i].xLevel == xLevel)
		{
			return	pTraceLevelInfo[i].pName;
		}
	}

	return	"UNKNOWN";
}
