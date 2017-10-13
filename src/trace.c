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
static bool _bDumpEnable = false;
static bool	_bEnable = false;

uint32_t	TRACE_Dump(char *pModule, uint8_t *pData, uint32_t ulDataLen, const char *pFormat, ...)
{
	uint32_t	nOutputLength = 0;

	if (_bDumpEnable)
	{
		if (pFormat != NULL)
		{
			va_list	xArgs;

			va_start(xArgs, pFormat);

			nOutputLength = SHELL_VPrintf(pModule, pFormat, xArgs);

			va_end(xArgs);
		}

		nOutputLength += SHELL_Dump(pData, ulDataLen);
	}

	return	nOutputLength;
}
/*!
 * @brief Console formatted output
 */
uint32_t	TRACE_Printf(const char* pModule, const char *pFormat, ...)
{
	uint32_t	nOutputLength = 0;

	if (_bEnable)
	{
		va_list	xArgs;

		va_start(xArgs, pFormat);

		nOutputLength = SHELL_VPrintf(pModule, pFormat, xArgs);

		va_end(xArgs);
	}

	return	nOutputLength;
}

bool		TRACE_SetEnable(bool bEnable)
{
	_bEnable = bEnable;

	return	true;
}

bool		TRACE_GetEnable(void)
{
	return	_bEnable;
}

bool		TRACE_SetDumpEnable(bool bEnable)
{
	_bDumpEnable = bEnable;

	return	true;
}

bool		TRACE_GetDumpEnable(void)
{
	return	_bDumpEnable;
}

void		TRACE_SetModule(const char * pModule, bool bEnable)
{
}

bool		TRACE_GetModule(const char * pModule)
{
	return	_bEnable;
}

