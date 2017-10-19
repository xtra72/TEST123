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

uint32_t	TRACE_Dump(uint16_t xModule, uint8_t *pData, uint32_t ulDataLen, const char *pFormat, ...);
uint32_t	TRACE_Printf(uint16_t xModule, const char *pFormat, ...);
bool		TRACE_SetEnable(bool bEnable);
bool		TRACE_GetEnable(void);
bool		TRACE_SetDump(bool bEnable);
bool		TRACE_GetDump(void);
void		TRACE_SetModule(uint16_t xModule, bool bEnable);
bool		TRACE_GetModule(uint16_t xModule);
const char*	TRACE_GetModuleName(unsigned short xModuleFlag);

#define	TRACE(format, ...)				TRACE_Printf(__MODULE__, format, ## __VA_ARGS__)
#define	ERROR(format, ...)				TRACE_Printf(__MODULE__, format, ## __VA_ARGS__)
#define	TRACE_DUMP(pData, ulDataLen, format, ...)	TRACE_Dump(__MODULE__, (uint8_t *)pData, ulDataLen, format, ## __VA_ARGS__)

#endif /* INC_TRACE_H_ */
