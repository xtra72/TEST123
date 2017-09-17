/*
 * trace.h
 *
 *  Created on: 2017. 9. 11.
 *      Author: inhyuncho
 */

#ifndef INC_SHELL_H_
#define INC_SHELL_H_

#include <stdarg.h>

typedef	struct
{
	const char*	pName;
	const char* pHelp;
	int	(*fCommand)(char *pArgv[], int nArgc);
}	SHELL_CMD;

void		SHELL_Init(void);
uint32_t	SHELL_Printf(const char* pFormat, ...);
uint32_t	SHELL_Dump(const uint8_t *pData, uint32_t ulDataLen);
uint32_t	SHELL_VPrintf(const char* pModule, const char* pFormat, va_list	xArgs);
uint32_t	SHELL_GetLine(uint8_t* pBuffer, uint32_t ulBufferLen);

void 		SHELL_Task(void* pvParameters);

#endif /* INC_SHELL_H_ */
