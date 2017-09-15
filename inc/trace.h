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
#define	__MODULE__	"global"

uint32_t	TRACE_Dump(char *pModule, uint8_t *pData, uint32_t ulDataLen);
uint32_t	TRACE_Printf(const char* pModule, const char *pFormat, ...);
void		TRACE_SetEnable(bool bEnable);
bool		TRACE_GetEnable(void);
void		TRACE_SetModule(const char * pModule, bool bEnable);
bool		TRACE_GetModule(const char * pModule);

#define	TRACE(format, ...)				TRACE_Printf(__MODULE__, format, ## __VA_ARGS__)
#define	ERROR(format, ...)				TRACE_Printf(__MODULE__, format, ## __VA_ARGS__)
#define	TRACE_DUMP(pData, ulDataLen)	TRACE_Dump(__MODULE__, (uint8_t *)pData, ulDataLen)

#endif /* INC_TRACE_H_ */
