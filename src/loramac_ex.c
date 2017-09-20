/*
 * loramac_ex.c
 *
 *  Created on: 2017. 9. 19.
 *      Author: xtra
 */
#include "loramac_ex.h"
#include "utilities.h"
#include "global.h"

static MibRequestConfirm_t mibReq;

bool	LORAMAC_GetAppSKey(uint8_t* pAppSKey)
{
	mibReq.Type = MIB_APP_SKEY;

	if (LoRaMacMibGetRequestConfirm( &mibReq ) != LORAMAC_STATUS_OK)
	{
		return	false;
	}

	memcpy1( pAppSKey, mibReq.Param.AppSKey, LORAMAC_APP_SKEY_SIZE );

	return	true;
}

bool	LORAMAC_GetNwkSKey(uint8_t* pNwkSKey)
{
	mibReq.Type = MIB_NWK_SKEY;

	if (LoRaMacMibGetRequestConfirm( &mibReq ) != LORAMAC_STATUS_OK)
	{
		return	false;
	}

	memcpy1( pNwkSKey, mibReq.Param.NwkSKey, LORAMAC_NWK_SKEY_SIZE );

	return	true;

}

bool	LORAMAC_GetAppNonce(uint32_t* pAppNonce)
{
	mibReq.Type = MIB_APP_NONCE;

	if (LoRaMacMibGetRequestConfirm( &mibReq ) != LORAMAC_STATUS_OK)
	{
		return	false;
	}

	*pAppNonce = mibReq.Param.AppNonce;

	return	true;
}


DeviceClass_t 	LORAMAC_GetClassType(void)
{
	// Did we already join the network ?
	mibReq.Type = MIB_DEVICE_CLASS;
	mibReq.Param.Class = 0;
	LoRaMacMibGetRequestConfirm( &mibReq );

	return	mibReq.Param.Class;
}

bool	LORAMAC_SetClassType(DeviceClass_t class)
{
	// Did we already join the network ?
	mibReq.Type = MIB_DEVICE_CLASS;
	mibReq.Param.Class = class;
	if (LoRaMacMibSetRequestConfirm( &mibReq ) != LORAMAC_STATUS_OK)
	{
		return	false;
	}

	return	true;
}

int8_t 	LORAMAC_GetDatarate(void)
{
	// Did we already join the network ?
	mibReq.Type = MIB_CHANNELS_DATARATE;
	mibReq.Param.ChannelsDatarate = 0;
	LoRaMacMibGetRequestConfirm( &mibReq );
	// We are already attached
	return	mibReq.Param.ChannelsDatarate;
}

bool	LORAMAC_SetDatarate(int8_t datarate)
{
	// Did we already join the network ?
	mibReq.Type = MIB_CHANNELS_DATARATE;
	mibReq.Param.ChannelsDatarate = datarate;
	if (LoRaMacMibSetRequestConfirm( &mibReq ) != LORAMAC_STATUS_OK)
	{
		return	false;
	}

	return	true;
}


int8_t	LORAMAC_GetTxPower(void)
{
	// Did we already join the network ?
	mibReq.Type = MIB_CHANNELS_TX_POWER;
	mibReq.Param.ChannelsTxPower = 0;

	LoRaMacMibGetRequestConfirm( &mibReq );

	// We are already attached
	return	mibReq.Param.ChannelsTxPower;
}

bool	LORAMAC_SetTxPower(int8_t power)
{
	// Did we already join the network ?
	mibReq.Type = MIB_CHANNELS_TX_POWER;
	mibReq.Param.ChannelsTxPower = power;
	if (LoRaMacMibSetRequestConfirm( &mibReq ) != LORAMAC_STATUS_OK)
	{
		return	false;
	}
	return	true;
}

uint8_t	LORAMAC_GetRetries(void)
{
	// Did we already join the network ?
	mibReq.Type = MIB_CHANNELS_NB_REP;
	mibReq.Param.ChannelNbRep = 0;

	LoRaMacMibGetRequestConfirm( &mibReq );

	// We are already attached
	return	mibReq.Param.ChannelNbRep;
}

bool	LORAMAC_SetRetries(uint8_t nRetries)
{
	// Did we already join the network ?
	mibReq.Type = MIB_CHANNELS_NB_REP;
	mibReq.Param.ChannelNbRep = nRetries;

	return	(LoRaMacMibSetRequestConfirm( &mibReq )  == LORAMAC_STATUS_OK);
}

bool	LORAMAC_AddACK(void)
{
	mibReq.Type = MIB_ADD_ACK;
	LoRaMacMibSetRequestConfirm( &mibReq );

	return	true;
}

