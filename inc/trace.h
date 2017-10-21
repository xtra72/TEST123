/*
 * trace.h
 *
 *  Created on: 2017. 9. 11.
 *      Author: inhyuncho
 */

#ifndef INC_TRACE_H_
#define INC_TRACE_H_

#include "shell.h"
#include "stdbool.h"
#define	__MODULE__	0

typedef	enum
{
	TRACE_LEVEL_DEBUG_0,
	TRACE_LEVEL_DEBUG_1,
	TRACE_LEVEL_DEBUG_2,
	TRACE_LEVEL_DEBUG_3,
	TRACE_LEVEL_DEBUG_4,
	TRACE_LEVEL_DEBUG_5,
	TRACE_LEVEL_INFO,
	TRACE_LEVEL_WRANING,
	TRACE_LEVEL_ERROR,
	TRACE_LEVEL_FATAL
}	TRACE_LEVEL;


uint32_t	TRACE_Printf(TRACE_LEVEL xLevel, uint16_t xModule, const char *pFormat, ...);
uint32_t	TRACE_Dump(TRACE_LEVEL xLevel, uint16_t xModule, uint8_t *pData, uint32_t ulDataLen, const char *pFormat, ...);

bool		TRACE_SetEnable(bool bEnable);
bool		TRACE_GetEnable(void);
bool		TRACE_SetDump(bool bEnable);
bool		TRACE_GetDump(void);
bool		TRACE_SetLevel(char* pLevel);
TRACE_LEVEL	TRACE_GetLevel(void);
const char*	TRACE_GetLevelName(TRACE_LEVEL xLevel);
void		TRACE_SetModule(uint16_t xModule, bool bEnable);
bool		TRACE_GetModule(uint16_t xModule);
const char*	TRACE_GetModuleName(unsigned short xModuleFlag);

#if DEBUG == 1
#define	TRACE(level, format, ...)		TRACE_Printf(TRACE_LEVEL_DEBUG_##level, __MODULE__, format, ## __VA_ARGS__)
#define	INFO(format, ...)		TRACE_Printf(TRACE_LEVEL_INFO, __MODULE__, format, ## __VA_ARGS__)
#define	WARN(format, ...)		TRACE_Printf(TRACE_LEVEL_WRANING, __MODULE__, format, ## __VA_ARGS__)
#define	ERROR(format, ...)		TRACE_Printf(TRACE_LEVEL_ERROR, __MODULE__, format, ## __VA_ARGS__)
#define	DUMP(level, pData, ulDataLen, format, ...)	TRACE_Dump(TRACE_LEVEL_DEBUG_##level, __MODULE__, (uint8_t *)pData, ulDataLen, format, ## __VA_ARGS__)
#else
#define	TRACE(format, ...)
#define	ERROR(format, ...)
#define	DUMP(pData, ulDataLen, format, ...)
#endif

#endif /* INC_TRACE_H_ */
