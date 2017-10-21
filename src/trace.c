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

static TRACE_LEVEL	xOutputLevel = TRACE_LEVEL_DEBUG_3;

uint32_t	TRACE_Dump(TRACE_LEVEL xLevel, uint16_t xModule, uint8_t *pData, uint32_t ulDataLen, const char *pFormat, ...)
{
	uint32_t	nOutputLength = 0;

	if ((xLevel >= xOutputLevel) && UNIT_TRACE_DUMP && UNIT_TRACE_ENABLE && UNIT_TRACE(xModule))
	{
		if (pFormat != NULL)
		{
			va_list	xArgs;

			va_start(xArgs, pFormat);

			nOutputLength = SHELL_VPrintf(xModule, pFormat, xArgs);

			va_end(xArgs);
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

	if (xLevel >= xOutputLevel)
	{
		if (UNIT_TRACE_ENABLE && UNIT_TRACE(xModule))
		{
			va_list	xArgs;

			va_start(xArgs, pFormat);

			nOutputLength = SHELL_VPrintf(xModule, pFormat, xArgs);

			va_end(xArgs);
		}
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
	if (strcasecmp(pLevel, "debug0") == 0)
	{
		xOutputLevel = TRACE_LEVEL_DEBUG_0;
	}
	else if (strcasecmp(pLevel, "debug1") == 0)
	{
		xOutputLevel = TRACE_LEVEL_DEBUG_1;
	}
	else if (strcasecmp(pLevel, "debug2") == 0)
	{
		xOutputLevel = TRACE_LEVEL_DEBUG_2;
	}
	else if (strcasecmp(pLevel, "debug3") == 0)
	{
		xOutputLevel = TRACE_LEVEL_DEBUG_3;
	}
	else if (strcasecmp(pLevel, "debug4") == 0)
	{
		xOutputLevel = TRACE_LEVEL_DEBUG_4;
	}
	else if (strcasecmp(pLevel, "debug5") == 0)
	{
		xOutputLevel = TRACE_LEVEL_DEBUG_5;
	}
	else if (strcasecmp(pLevel, "info") == 0)
	{
		xOutputLevel = TRACE_LEVEL_INFO;
	}
	else if (strcasecmp(pLevel, "warn") == 0)
	{
		xOutputLevel = TRACE_LEVEL_WRANING;
	}
	else if (strcasecmp(pLevel, "error") == 0)
	{
		xOutputLevel = TRACE_LEVEL_ERROR;
	}
	else if (strcasecmp(pLevel, "FATAL") == 0)
	{
		xOutputLevel = TRACE_LEVEL_FATAL;
	}
	else
	{
		return	false;
	}

	return	true;
}

TRACE_LEVEL	TRACE_GetLevel(void)
{
	return	xOutputLevel;
}

const char*	TRACE_GetLevelName(TRACE_LEVEL xLevel)
{
	switch(xLevel)
	{
	case	TRACE_LEVEL_DEBUG_0:	return	"DEBUG0";
	case	TRACE_LEVEL_DEBUG_1:	return	"DEBUG1";
	case	TRACE_LEVEL_DEBUG_2:	return	"DEBUG2";
	case	TRACE_LEVEL_DEBUG_3:	return	"DEBUG3";
	case	TRACE_LEVEL_DEBUG_4:	return	"DEBUG4";
	case	TRACE_LEVEL_DEBUG_5:	return	"DEBUG5";
	case	TRACE_LEVEL_INFO:		return	"INFO";
	case	TRACE_LEVEL_WRANING:	return	"WARNING";
	case	TRACE_LEVEL_ERROR:		return	"ERROR";
	case	TRACE_LEVEL_FATAL:		return	"FATAL";

	}

	return	"UNKNOWN";
}
