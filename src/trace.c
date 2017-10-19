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

uint32_t	TRACE_Dump(uint16_t xModule, uint8_t *pData, uint32_t ulDataLen, const char *pFormat, ...)
{
	uint32_t	nOutputLength = 0;

	if (UNIT_TRACE_DUMP && UNIT_TRACE_ENABLE && UNIT_TRACE(xModule))
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
uint32_t	TRACE_Printf(uint16_t xModule, const char *pFormat, ...)
{
	uint32_t	nOutputLength = 0;

	if (UNIT_TRACE_ENABLE && UNIT_TRACE(xModule))
	{
		va_list	xArgs;

		va_start(xArgs, pFormat);

		nOutputLength = SHELL_VPrintf(xModule, pFormat, xArgs);

		va_end(xArgs);
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

