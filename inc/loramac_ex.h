/*
 * loramac_ex.h
 *
 */

#ifndef INC_LORAMAC_EX_H_
#define INC_LORAMAC_EX_H_


#include "global.h"
#include "LoRaMAC.h"


DeviceClass_t	LORAMAC_GetClass(void);
bool	LORAMAC_SetClass(DeviceClass_t xClass);

bool	LORAMAC_GetAppSKey(uint8_t* pAppSKey);
bool	LORAMAC_GetNwkSKey(uint8_t* pNwkSKey);
bool	LORAMAC_GetAppNonce(uint32_t* pAppNonce);

DeviceClass_t 	LORAMAC_GetClassType(void);
bool	LORAMAC_SetClassType(DeviceClass_t class);

int8_t 	LORAMAC_GetDatarate(void);
bool	LORAMAC_SetDatarate(int8_t datarate);
int8_t	LORAMAC_GetTxPower(void);
bool	LORAMAC_SetTxPower(int8_t power);

uint8_t	LORAMAC_GetRetries(void);
bool	LORAMAC_SetRetries(uint8_t nRetries);

uint8_t	LORAMAC_GetPeriodMode(void);
bool	LORAMAC_SetPeriodMode(bool bEnable);

bool	LORAMAC_AddACK(void);

#endif
