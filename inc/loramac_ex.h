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

bool	LORAMAC_IsPublicNetwork(void);
bool	LORAMAC_SetPublicNetwork(bool bPublic);

int8_t 	LORAMAC_GetDatarate(void);
bool	LORAMAC_SetDatarate(int8_t datarate);

int8_t	LORAMAC_GetTxPower(void);
bool	LORAMAC_SetTxPower(int8_t power);

uint8_t	LORAMAC_GetRetries(void);
bool	LORAMAC_SetRetries(uint8_t nRetries);

uint8_t	LORAMAC_GetPeriodMode(void);
bool	LORAMAC_SetPeriodMode(bool bEnable);

uint32_t LORAMAC_GetDownLinkCounter(void);
bool 	LORAMAC_SetDownLinkCounter(uint32_t ulCounter);

uint32_t LORAMAC_GetUpLinkCounter(void);
bool 	LORAMAC_SetUpLinkCounter(uint32_t ulCounter);

bool	LORAMAC_SetDutyCycle(uint32_t ulDutyCycle);

uint32_t	LORAMAC_GetChannelsNbRepeat(void);
bool 	LORAMAC_SetChannelsNbRepeat(uint32_t ulCounter);

uint16_t	LORAMAC_GetChannelsMask(void);

uint32_t	LORAMAC_GetRx1Delay(void);
uint32_t	LORAMAC_GetRx2Delay(void);

uint32_t	LORAMAC_GetJoinDelay1(void);
uint32_t	LORAMAC_GetJoinDelay2(void);

bool	LORAMAC_GetADR(void);
bool	LORAMAC_SetADR(bool bADR);

bool	LORAMAC_AddACK(void);

#endif
