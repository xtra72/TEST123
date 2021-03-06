/*
 * loramac_ex.c
 *
 *  Created on: 2017. 9. 19.
 *      Author: xtra
 */
#include "loramac_ex.h"
#include "LoRaMacTest.h"
#include "utilities.h"
#include "Region.h"
#include "global.h"
#include "trace.h"
static MibRequestConfirm_t mibReq;

bool	LORAMAC_GetAppSKey(uint8_t* pAppSKey)
{
	mibReq.Type = MIB_APP_SKEY;

	if (LoRaMacMibGetRequestConfirm( &mibReq ) == LORAMAC_STATUS_OK)
	{
		memcpy1( pAppSKey, mibReq.Param.AppSKey, LORAMAC_APP_SKEY_SIZE );

		return	true;
	}

	return	false;
}

bool	LORAMAC_GetNwkSKey(uint8_t* pNwkSKey)
{
	mibReq.Type = MIB_NWK_SKEY;

	if (LoRaMacMibGetRequestConfirm( &mibReq ) == LORAMAC_STATUS_OK)
	{
		memcpy1( pNwkSKey, mibReq.Param.NwkSKey, LORAMAC_NWK_SKEY_SIZE );

		return	true;
	}

	return	false;
}

bool	LORAMAC_GetAppNonce(uint32_t* pAppNonce)
{
	mibReq.Type = MIB_APP_NONCE;

	if (LoRaMacMibGetRequestConfirm( &mibReq ) == LORAMAC_STATUS_OK)
	{
		*pAppNonce = mibReq.Param.AppNonce;

		return	true;
	}

	return	false;
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
	return	(LoRaMacMibSetRequestConfirm( &mibReq ) == LORAMAC_STATUS_OK);
}

bool	LORAMAC_IsPublicNetwork(void)
{
	mibReq.Type = MIB_PUBLIC_NETWORK;
	mibReq.Param.EnablePublicNetwork = false;

	LoRaMacMibGetRequestConfirm( &mibReq );

	// We are already attached
	return	mibReq.Param.EnablePublicNetwork;
}

bool	LORAMAC_SetPublicNetwork(bool bPublic)
{
	mibReq.Type = MIB_PUBLIC_NETWORK;
	mibReq.Param.EnablePublicNetwork = bPublic;
	return	(LoRaMacMibSetRequestConfirm( &mibReq ) == LORAMAC_STATUS_OK);
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
	return	(LoRaMacMibSetRequestConfirm( &mibReq ) == LORAMAC_STATUS_OK);
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

uint32_t LORAMAC_GetDownLinkCounter(void)
{
	mibReq.Type = MIB_DOWNLINK_COUNTER;
	mibReq.Param.DownLinkCounter = 0;

	LoRaMacMibGetRequestConfirm( &mibReq );

	return	mibReq.Param.DownLinkCounter;
}

bool 	LORAMAC_SetDownLinkCounter(uint32_t ulCounter)
{
	mibReq.Type = MIB_DOWNLINK_COUNTER;
	mibReq.Param.DownLinkCounter = ulCounter;

	return	(LoRaMacMibSetRequestConfirm( &mibReq )  == LORAMAC_STATUS_OK);
}

uint32_t LORAMAC_GetUpLinkCounter(void)
{
	mibReq.Type = MIB_UPLINK_COUNTER;
	mibReq.Param.UpLinkCounter = 0;

	LoRaMacMibGetRequestConfirm( &mibReq );

	return	mibReq.Param.UpLinkCounter;
}

bool 	LORAMAC_SetUpLinkCounter(uint32_t ulCounter)
{
	// Did we already join the network ?
	mibReq.Type = MIB_UPLINK_COUNTER;
	mibReq.Param.UpLinkCounter = ulCounter;

	return	(LoRaMacMibSetRequestConfirm( &mibReq )  == LORAMAC_STATUS_OK);
}

bool	LORAMAC_SetDutyCycle(uint32_t ulDutyCycle)
{
	GetPhyParams_t 	GetPhyParam;
	PhyParam_t		PhyParam;

	GetPhyParam.Attribute = ulDutyCycle;
	PhyParam = RegionGetPhyParam(UNIT_REGION,&GetPhyParam);
	LoRaMacTestSetDutyCycleOn( PhyParam.Value );

	return	true;
}

uint32_t	LORAMAC_GetChannelsNbRepeat(void)
{
	mibReq.Type = MIB_CHANNELS_NB_REP;
	mibReq.Param.ChannelNbRep = 1;

	LoRaMacMibGetRequestConfirm( &mibReq );

	return	mibReq.Param.ChannelNbRep;
}

bool 	LORAMAC_SetChannelsNbRepeat(uint32_t ulCounter)
{
	mibReq.Type = MIB_CHANNELS_NB_REP;
	mibReq.Param.ChannelNbRep = ulCounter;

	return	(LoRaMacMibSetRequestConfirm( &mibReq )  == LORAMAC_STATUS_OK);
}

uint16_t	LORAMAC_GetChannelsMask(void)
{
	mibReq.Type = MIB_CHANNELS_MASK;
	mibReq.Param.ChannelsMask = 0;

	LoRaMacMibGetRequestConfirm( &mibReq );

	if (mibReq.Param.ChannelsMask != 0)
	{
		return	mibReq.Param.ChannelsMask[0];
	}

	return	0;
}

bool	LORAMAC_GetChannel(uint32_t nIndex, ChannelParams_t* pChannel)
{
    GetPhyParams_t getPhy;
    PhyParam_t phyParam;

	// Reset to defaults
	getPhy.Attribute = PHY_MAX_NB_CHANNELS;
	phyParam = RegionGetPhyParam( UNIT_REGION, &getPhy );

	if (nIndex >= phyParam.Value)
	{
		return	false;
	}

	getPhy.Attribute = PHY_CHANNELS;
	phyParam = RegionGetPhyParam( UNIT_REGION, &getPhy );

	memcpy(pChannel, &phyParam.Channels[nIndex], sizeof(ChannelParams_t));

	return	true;
}


uint32_t	LORAMAC_GetRx1Delay(void)
{
	mibReq.Type = MIB_RECEIVE_DELAY_1;
	mibReq.Param.ReceiveDelay1 = 0;

	LoRaMacMibGetRequestConfirm( &mibReq );

	return	mibReq.Param.ReceiveDelay1;
}


uint32_t	LORAMAC_GetRx2Delay(void)
{
	mibReq.Type = MIB_RECEIVE_DELAY_2;
	mibReq.Param.ReceiveDelay2 = 0;

	LoRaMacMibGetRequestConfirm( &mibReq );

	return	mibReq.Param.ReceiveDelay2;
}


uint32_t	LORAMAC_GetJoinDelay1(void)
{
	mibReq.Type = MIB_JOIN_ACCEPT_DELAY_1;
	mibReq.Param.JoinAcceptDelay1 = 0;

	LoRaMacMibGetRequestConfirm( &mibReq );

	return	mibReq.Param.JoinAcceptDelay1;
}


uint32_t	LORAMAC_GetJoinDelay2(void)
{
	mibReq.Type = MIB_JOIN_ACCEPT_DELAY_2;
	mibReq.Param.JoinAcceptDelay2 = 0;

	LoRaMacMibGetRequestConfirm( &mibReq );

	return	mibReq.Param.JoinAcceptDelay2;
}

bool	LORAMAC_GetADR(void)
{
	mibReq.Type =  MIB_ADR;
	mibReq.Param.AdrEnable = false;

	LoRaMacMibGetRequestConfirm( &mibReq );

	return	mibReq.Param.AdrEnable;
}

bool	LORAMAC_SetADR(bool bADR)
{
	mibReq.Type =  MIB_ADR;
	mibReq.Param.AdrEnable = bADR;

	return	(LoRaMacMibSetRequestConfirm( &mibReq )  == LORAMAC_STATUS_OK);
}

bool	LORAMAC_TxCW(uint32_t xTimeout)
{
	MlmeReq_t mlmeReq;

	mlmeReq.Type = MLME_TXCW;
	mlmeReq.Req.TxCw.Timeout = xTimeout;

	if (LoRaMacMlmeRequest( &mlmeReq ) != LORAMAC_STATUS_OK)
	{
		ERROR("LoRaMacMlmeRequest failed.\n");
		return	false;
	}

	return true;
}

bool	LORAMAC_TxCW1(uint32_t xFrequency, uint32_t xPower, uint32_t xTimeout)
{
	MlmeReq_t mlmeReq;

	mlmeReq.Type = MLME_TXCW_1;
	mlmeReq.Req.TxCw.Frequency = xFrequency;
	mlmeReq.Req.TxCw.Power = xPower;
	mlmeReq.Req.TxCw.Timeout = xTimeout;

	if (LoRaMacMlmeRequest( &mlmeReq ) != LORAMAC_STATUS_OK)
	{
		ERROR("LoRaMacMlmeRequest failed.\n");
		return	false;
	}

	return true;
}
