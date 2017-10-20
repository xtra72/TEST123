/*
 * lorawan_task.c
 *
 */
#include "global.h"
#include "device_def.h"
#include "loramac_ex.h"
#include "lorawan_task.h"
#include "Commissioning.h"
#include "trace.h"
#include "SKTApp.h"
/** \addtogroup S40 S40 Main Application
 *  @{
 */
#undef	__MODULE__
#define	__MODULE__	FLAG_TRACE_LORAWAN

static LORA_PACKET messageIn;
static xSemaphoreHandle LORAWANSemaphore;

#define LORAWAN_TIMEOUT			(50 * configTICK_RATE_HZ)	//!< LORAWAN_SendMessage Timeout value
#define LORAWAN_JOIN_TIMEOUT	(60 * configTICK_RATE_HZ)	//!< LORAWAN_SendMessage Timeout value
static struct
{
	MlmeConfirm_t		mlme;
	McpsConfirm_t		confirm;
	McpsIndication_t	indication;
} LocalMcps;

static uint16_t LoRaDownLinkCounter = 0;
static LoRaMacPrimitives_t LoRaMacPrimitives;
static LoRaMacCallback_t LoRaMacCallbacks;
static MibRequestConfirm_t mibReq;

/** @cond */
#define MLME_EVENT			(0x01 << 0)
#define CONFIRM_EVENT		(0x01 << 1)
#define INDICATION_EVENT	(0x01 << 2)

#define RF_EVENT_STACK		( configMINIMAL_STACK_SIZE * 8)
static StackType_t RFEventStack[RF_EVENT_STACK];
static StaticTask_t RFEventTask;
static TaskHandle_t LORAWANEventTask;
static uint8_t LoRaWAN_Retries = LORAWAN_RETRIES;
static uint8_t	LoRaWAN_DefaultDR = LORAWAN_DEFAULT_DATARATE;
static	LoRaWANStatus_t	LoRaWAN_Status = LORAWAN_STATUS_IDLE;

static uint8_t		nSNR = 0;
static int16_t		nRSSI = 0;

/** @endcond */

static __attribute__((noreturn)) void LORAWAN_EventTask(void* pvParameter)
{
	uint32_t ulNotificationValue;
	(void)pvParameter;

	for(;;)
	{
		// Stop task forever, waiting for notification
		// No bits will be cleared upon enter
		// All bits will be cleared upon exit
		xTaskNotifyWait(0,-1,&ulNotificationValue,portMAX_DELAY);
		if (ulNotificationValue &  MLME_EVENT)
		{
			// MlmeConfirm event
			// Perform any specific action
			// and Give Semaphore to unlock waiting task
		    switch( LocalMcps.mlme.MlmeRequest )
		    {
		        case MLME_JOIN:
		        {
		            if( LocalMcps.mlme.Status == LORAMAC_EVENT_INFO_STATUS_OK )
		            {
						/*
						 * Remove the comment in order to copy OTAA parameters back to Flash memory
						 *
												USERDATA UData;
												memcpy((unsigned char*)&UData,(unsigned char*)USERDATAPTR,sizeof(USERDATA));
												mibReq.Type = MIB_DEV_ADDR;
												LoRaMacMibGetRequestConfirm( &mibReq );
												UData.DeviceSerialNumber = mibReq.Param.DevAddr;
												mibReq.Type = MIB_NWK_SKEY;
												mibReq.Param.NwkSKey = (uint8_t*)UNIT_NETSKEY;
												LoRaMacMibGetRequestConfirm( &mibReq );
												memcpy(UData.LoRaWAN.NwkSKey,mibReq.Param.NwkSKey,sizeof(UData.LoRaWAN.NwkSKey));
												mibReq.Type = MIB_APP_SKEY;
												mibReq.Param.AppSKey = (uint8_t*)UNIT_APPSKEY;
												LoRaMacMibGetRequestConfirm( &mibReq );
												memcpy(UData.LoRaWAN.AppSKey,mibReq.Param.AppSKey,sizeof(UData.LoRaWAN.AppSKey));
												DeviceUserDataSave(&UData);
						*/
						TRACE("Node has joined the network.\n");
		                // Status is OK, node has joined the network
		            }
		            else
		            {
		            	TRACE("Join failed[%d].\n", LocalMcps.mlme.Status);
		            }
		            break;
		        }
		        case MLME_LINK_CHECK:
		        {
		            if( LocalMcps.mlme.Status == LORAMAC_EVENT_INFO_STATUS_OK )
		            {
		    			DevicePostEvent(RF_MLME);
		                // Check DemodMargin
		                // Check NbGateways
		            }
		            break;
		        }
		        default:
		            break;
		    }
		    if (LORAWANSemaphore) xSemaphoreGive( LORAWANSemaphore);
		}
		if (ulNotificationValue & CONFIRM_EVENT)
		{
        	TRACE("Confirm event.\n");
			// Confirm event
			// Perform any specific action
			// and Give Semaphore to unlock waiting task
		    if( LocalMcps.confirm.Status == LORAMAC_EVENT_INFO_STATUS_OK )
		    {
		    	LoRaDownLinkCounter++;
				switch( LocalMcps.confirm.McpsRequest )
				{
					case MCPS_UNCONFIRMED:
					{
						// Check Datarate
						// Check TxPower
						break;
					}
					case MCPS_CONFIRMED:
					{
						TRACE("MCPS_CONFIRMED confirmed.\n");
						// Check Datarate
						// Check TxPower
						// Check AckReceived
						// Check NbTrials
						break;
					}
					case MCPS_PROPRIETARY:
					{
						TRACE("MCPS_PROPRIETARY confirmed.\n");
						break;
					}
					default:
						break;
				}
		    }
		    else
		    {
		    	ERROR("Error : %d\n", LocalMcps.confirm.Status );
		    }

		    if (LORAWANSemaphore) xSemaphoreGive( LORAWANSemaphore);
		}
		if (ulNotificationValue & INDICATION_EVENT)
		{
			nRSSI = LocalMcps.indication.Rssi;
			nSNR  = LocalMcps.indication.Snr;
	       	TRACE("Indication event.\n");
			// Indication event
			// Perform any specific action
			// and post event to let other tasks know
	       	if (SKTAPP_ParseMessage(LORAWAN_GetIndication()) == false)
	       	{
				DevicePostEvent(RF_INDICATION);
	       	}
		}
	}
	__builtin_unreachable();
}

#if 0	// Remove unused static function
static inline void TimerEmpty(void) {

}
#endif

/*
 * The event callbacks are kept as small as possible as they are called inside an ISR
 * Real work is deferred to the LORAWAN_EventTask function as a separate task
 */
/*!
 * \brief   MLME-Confirm event function
 *
 * \param   [IN] mlmeConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
static void MlmeConfirm( MlmeConfirm_t *mlmeConfirm )
{
	memcpy(&LocalMcps.mlme,mlmeConfirm,sizeof(MlmeConfirm_t));
	if (LORAWANEventTask) xTaskNotifyFromISR(LORAWANEventTask,MLME_EVENT,eSetBits,NULL);
}
/*!
 * \brief   MCPS-Confirm event function
 *
 * \param   [IN] mcpsConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
static void McpsConfirm( McpsConfirm_t *mcpsConfirm )
{
	memcpy(&LocalMcps.confirm,mcpsConfirm,sizeof(McpsConfirm_t));
	if (LORAWANEventTask) xTaskNotifyFromISR(LORAWANEventTask,CONFIRM_EVENT,eSetBits,NULL);
}
/*!
 * \brief   MCPS-Indication event function
 *
 * \param   [IN] mcpsIndication - Pointer to the indication structure,
 *               containing indication attributes.
 */static void McpsIndication( McpsIndication_t *mcpsIndication )
{
	memcpy(&LocalMcps.indication,mcpsIndication,sizeof(McpsIndication_t));
	if (LORAWANEventTask) xTaskNotifyFromISR(LORAWANEventTask,INDICATION_EVENT,eSetBits,NULL);
}

/*!
 * @brief Initializes the LORAWAN primitives
 */
void LORAWAN_Init(void)
{
	LORAWANEventTask = xTaskCreateStatic( LORAWAN_EventTask, (const char*)"LW_EVENT", RF_EVENT_STACK, NULL, tskIDLE_PRIORITY + 2, RFEventStack, &RFEventTask );

	static StaticSemaphore_t xRFSemaphoreBuffer;
	LORAWANSemaphore = xSemaphoreCreateBinaryStatic( &xRFSemaphoreBuffer );
	LoRaMacPrimitives.MacMcpsConfirm = McpsConfirm;
	LoRaMacPrimitives.MacMcpsIndication = McpsIndication;
	LoRaMacPrimitives.MacMlmeConfirm = MlmeConfirm;
	LoRaMacCallbacks.GetBatteryLevel = BoardGetBatteryLevel;
	LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks,UNIT_REGION );

	LORAMAC_SetADR(LORAWAN_ADR_ON);
	LORAMAC_SetPublicNetwork(LORAWAN_PUBLIC_NETWORK);
	LORAMAC_SetDutyCycle(PHY_DUTY_CYCLE);

#if( USE_SEMTECH_DEFAULT_CHANNEL_LINEUP == 1 )
	if (UNIT_REGION == LORAMAC_REGION_EU868)
	{
		LoRaMacChannelAdd( 3, ( ChannelParams_t )LC4 );
		LoRaMacChannelAdd( 4, ( ChannelParams_t )LC5 );
		LoRaMacChannelAdd( 5, ( ChannelParams_t )LC6 );
		LoRaMacChannelAdd( 6, ( ChannelParams_t )LC7 );
		LoRaMacChannelAdd( 7, ( ChannelParams_t )LC8 );
		LoRaMacChannelAdd( 8, ( ChannelParams_t )LC9 );
		LoRaMacChannelAdd( 9, ( ChannelParams_t )LC10 );

		mibReq.Type = MIB_RX2_DEFAULT_CHANNEL;
		mibReq.Param.Rx2DefaultChannel = ( Rx2ChannelParams_t ){ 869525000, DR_3 };
		LoRaMacMibSetRequestConfirm( &mibReq );

		mibReq.Type = MIB_RX2_CHANNEL;
		mibReq.Param.Rx2Channel = ( Rx2ChannelParams_t ){ 869525000, DR_3 };
		LoRaMacMibSetRequestConfirm( &mibReq );
	}
	else if (UNIT_REGION == LORAMAC_REGION_KR920)
	{
		mibReq.Type = MIB_RX2_DEFAULT_CHANNEL;
		mibReq.Param.Rx2DefaultChannel = ( Rx2ChannelParams_t ){ 921900000, DR_0 };
		LoRaMacMibSetRequestConfirm( &mibReq );

		mibReq.Type = MIB_RX2_CHANNEL;
		mibReq.Param.Rx2Channel = ( Rx2ChannelParams_t ){ 921900000, DR_0 };
		LoRaMacMibSetRequestConfirm( &mibReq );
	}
#endif

    memset(&LocalMcps,0,sizeof(McpsIndication_t));
}


LoRaWANStatus_t LORAWAN_GetStatus(void)
{
	return	LoRaWAN_Status;
}

bool LORAWAN_SetStatus(LoRaWANStatus_t xStatus)
{
	LoRaWAN_Status = xStatus;
	return	true;
}

char*			LORAWAN_GetStatusString(void)
{
	switch(LoRaWAN_Status)
	{
	case	LORAWAN_STATUS_IDLE:	return	"LORAWAN_STATUS_IDLE";
	case	LORAWAN_STATUS_JOIN:	return	"LORAWAN_STATUS_JOIN";
	case	LORAWAN_STATUS_PSEUDO_JOIN:	return	"LORAWAN_STATUS_PSEUDO_JOIN";
	case	LORAWAN_STATUS_PSEUDO_JOIN_COMPLETED:	return	"LORAWAN_STATUS_PSEUDO_JOIN_COMPLETED";
	case	LORAWAN_STATUS_REQ_REAL_APP_KEY_ALLOC:	return	"LORAWAN_STATUS_REQ_REAL_APP_KEY_ALLOC";
	case	LORAWAN_STATUS_REQ_REAL_APP_KEY_ALLOC_CONFIRMED:	return	"LORAWAN_STATUS_REQ_REAL_APP_KEY_ALLOC_CONFIRMED";
	case	LORAWAN_STATUS_REQ_REAL_APP_KEY_RX_REPORT:	return	"LORAWAN_STATUS_REQ_REAL_APP_KEY_RX_REPORT";
	case	LORAWAN_STATUS_REQ_REAL_APP_KEY_RX_REPORT_CONFIRMED:	return	"LORAWAN_STATUS_REQ_REAL_APP_KEY_RX_REPORT_CONFIRMED";
	case	LORAWAN_STATUS_REAL_JOIN:	return	"LORAWAN_STATUS_REAL_JOIN";
	case	LORAWAN_STATUS_REAL_JOIN_COMPLETED:	return	"LORAWAN_STATUS_REAL_JOIN_COMPLETED";
	}

	return	"LORAWAN_STATUS_UNKNOWN";
}


bool LORAWAN_JoinNetworkUseOTTA(uint8_t* pDevEUI, uint8_t* pAppEUI, uint8_t* pAppKey)
{
	MlmeReq_t mlmeReq;
	TRACE("Use OTTA\n");

	mlmeReq.Type = MLME_JOIN;

	mlmeReq.Req.Join.DevEui = pDevEUI;
	mlmeReq.Req.Join.AppEui = pAppEUI;
	mlmeReq.Req.Join.AppKey = pAppKey;
	mlmeReq.Req.Join.NbTrials = 3;
	TRACE_DUMP(pDevEUI, 8, "%16s - ", "Dev EUI");
	TRACE_DUMP(pAppEUI, 8, "%16s - ", "App EUI");
	TRACE_DUMP(pAppKey, 16, "%16s - ", "App Key");

	if (LORAWANSemaphore) xSemaphoreTake( LORAWANSemaphore, 0 );

	if (LoRaMacMlmeRequest( &mlmeReq ) != LORAMAC_STATUS_OK)
	{
		ERROR("LoRaMacMlmeRequest failed.\n");
		return	false;
	}

	if (LORAWANSemaphore) xSemaphoreTake( LORAWANSemaphore, LORAWAN_JOIN_TIMEOUT );

	// Did we join the network ?
	mibReq.Type = MIB_NETWORK_JOINED;
	mibReq.Param.IsNetworkJoined = true;
	LoRaMacMibGetRequestConfirm( &mibReq );

	return ( mibReq.Param.IsNetworkJoined );
}


bool LORAWAN_JoinNetworkUseABP(void)
{
	TRACE("Use ABP\n");

	// Choose a random device address if not already defined in Commissioning.h
	if( UNIT_SERIALNUMBER == 0 )
	{
		// Random seed initialization
		srand1( BoardGetRandomSeed( ) );
		// Choose a random device address
		DeviceUserDateSetSerialNumber(randr( 0, 0x01FFFFFF ));
	}

	mibReq.Type = MIB_NET_ID;
	mibReq.Param.NetID = UNIT_NETWORKID;
	LoRaMacMibSetRequestConfirm( &mibReq );

	mibReq.Type = MIB_DEV_ADDR;
	mibReq.Param.DevAddr = UNIT_SERIALNUMBER;
	LoRaMacMibSetRequestConfirm( &mibReq );

	mibReq.Type = MIB_NWK_SKEY;
	mibReq.Param.NwkSKey = (uint8_t*)UNIT_NETSKEY;
	LoRaMacMibSetRequestConfirm( &mibReq );

	mibReq.Type = MIB_APP_SKEY;
	mibReq.Param.AppSKey = (uint8_t*)UNIT_APPSKEY;
	LoRaMacMibSetRequestConfirm( &mibReq );

	mibReq.Type = MIB_NETWORK_JOINED;
	mibReq.Param.IsNetworkJoined = true;
	LoRaMacMibSetRequestConfirm( &mibReq );

	return true;	// Consider that we succeeded
}

bool LORAWAN_JoinNetwork(void)
{
	TRACE("Start Join Network.\n");
	if( UNIT_USE_OTAA)
	{
		if (UNIT_USE_SKT_APP && UNIT_USE_RAK)
		{
			return	LORAWAN_JoinNetworkUseOTTA((uint8_t*)UNIT_DEVEUID, (uint8_t*)UNIT_APPEUID, (uint8_t*)UNIT_REALAPPKEY);
		}
		else
		{
			return	LORAWAN_JoinNetworkUseOTTA((uint8_t*)UNIT_DEVEUID, (uint8_t*)UNIT_APPEUID, (uint8_t*)UNIT_APPKEY);
		}
	}
	else
	{
		return	LORAWAN_JoinNetworkUseABP();
	}
}

bool LORAWAN_CancelJoinNetwork(void)
{
	if (LoRaWAN_Status == LORAWAN_STATUS_IDLE)
	{
		return	true;
	}

	MlmeReq_t mlmeReq;

	TRACE("Cancel Join N/W\n");

	mlmeReq.Type = MLME_CANCEL;

	if (LoRaMacMlmeRequest( &mlmeReq ) != LORAMAC_STATUS_OK)
	{
		ERROR("LoRaMacMlmeRequest failed.\n");
		return	false;
	}

	LoRaWAN_Status = LORAWAN_STATUS_IDLE;
	return true;
}
/*!
 * \brief Sends a message to the network.
 * \note  The function would block
 * @param[in] message 	Pointer of message to be sent
 * @return	a LORAWAN_Result_t information
 */
LoRaMacStatus_t LORAWAN_SendMessage( LORA_PACKET* message )
{
    McpsReq_t mcpsReq;
    LoRaMacTxInfo_t txInfo;
    LoRaMacStatus_t result = LORAMAC_STATUS_DEVICE_OFF;
    message->Status = LORAMAC_EVENT_INFO_STATUS_OK;
    if( LoRaMacQueryTxPossible( message->Size, &txInfo ) != LORAMAC_STATUS_OK )
    {
        // Send empty frame in order to flush MAC commands
        mcpsReq.Type = MCPS_UNCONFIRMED;
        mcpsReq.Req.Unconfirmed.fBuffer = NULL;
        mcpsReq.Req.Unconfirmed.fBufferSize = 0;
        mcpsReq.Req.Unconfirmed.Datarate = LoRaWAN_DefaultDR;
    }
    else
    {
        if( message->Request == MCPS_UNCONFIRMED )
        {
            mcpsReq.Type = MCPS_UNCONFIRMED;
            mcpsReq.Req.Unconfirmed.fPort = message->Port;
            mcpsReq.Req.Unconfirmed.fBuffer = &message->Buffer[0];
            mcpsReq.Req.Unconfirmed.fBufferSize = message->Size;
            message->NbTrials = 0;
            mcpsReq.Req.Unconfirmed.Datarate = LoRaWAN_DefaultDR;
         	TRACE("Request unconfirmed : PORT = %02x, Size = %d\n",
         			mcpsReq.Req.Unconfirmed.fPort,
					mcpsReq.Req.Unconfirmed.fBufferSize);
        }
        else if( message->Request == MCPS_CONFIRMED )
        {
            mcpsReq.Type = MCPS_CONFIRMED;
            mcpsReq.Req.Confirmed.fPort = message->Port;
            mcpsReq.Req.Confirmed.fBuffer = &message->Buffer[0];
            mcpsReq.Req.Confirmed.fBufferSize = message->Size;
            mcpsReq.Req.Confirmed.NbTrials = LoRaWAN_Retries;
            mcpsReq.Req.Confirmed.Datarate = LoRaWAN_DefaultDR;
         	TRACE("Request confirmed : PORTR  = %02x, Size = %d\n",
         			mcpsReq.Req.Confirmed.fPort,
					mcpsReq.Req.Confirmed.Datarate,
					mcpsReq.Req.Confirmed.fBufferSize);
       }
        else {
        	message->Status = LORAMAC_EVENT_INFO_STATUS_ERROR;
        	ERROR("LORAMAC_EVENT_INFO_STATUS_ERROR\n");
        	return false;
        }
    }

    if (LORAWANSemaphore) xSemaphoreTake( LORAWANSemaphore, 0 );

    result = LoRaMacMcpsRequest( &mcpsReq );
    if (result == LORAMAC_STATUS_OK)
    {
      	TRACE("Request OK\n");
	    if (LORAWANSemaphore)
	    {
	    	if (xSemaphoreTake( LORAWANSemaphore, ((message->NbTrials) ? 2 : 1 ) * LORAWAN_TIMEOUT ) == pdFALSE)
	    	{
	    		ERROR("Request Timeout!\n");
	        	message->Status = LORAMAC_EVENT_INFO_STATUS_TX_TIMEOUT;
	    	}
	    	else
	    	{
	         	TRACE("Request Done\n");
	    		message->Status = LocalMcps.confirm.Status;
	    		message->NbTrials = LocalMcps.confirm.NbRetries;
	    	}
	    }
    } else
    {
    	if (result == LORAMAC_STATUS_BUSY)
    	{
    	}
    	message->Status = LORAMAC_EVENT_INFO_STATUS_ERROR;
    	ERROR("Request error\n");
    }

    return result;
}

LORA_PACKET* LORAWAN_GetMessage(void)
{
McpsIndication_t *ind = LORAWAN_GetIndication();
	if (ind && (ind->RxData))
	{
		messageIn.Port = ind->Port;
		messageIn.Buffer = ind->Buffer;
		messageIn.Size = ind->BufferSize;
		messageIn.Request = ind->McpsIndication;
		messageIn.Status = ind->Status;
		messageIn.FramePending = ind->FramePending;
		return &messageIn;
	}
	return NULL;
}

bool	LORAWAN_SendAck(void)
{
	TRACE("Send Ack\n");

//	LORAMAC_SetADR(false);
	LORAMAC_AddACK();

#if 0
	MlmeReq_t mlmeReq;

	mlmeReq.Type = MLME_ACK;

	if (LoRaMacMlmeRequest( &mlmeReq ) != LORAMAC_STATUS_OK)
	{
		ERROR("LoRaMacMlmeRequest failed.\n");
		return	false;
	}
#endif

	LORA_PACKET		xMessage;

	xMessage.Port = 0;
	xMessage.Request = MCPS_UNCONFIRMED;
	xMessage.Buffer = NULL;
	xMessage.Size = 0;
	if (LORAWAN_SendMessage(&xMessage)  != LORAMAC_STATUS_OK)
	{
		 LORAWAN_ShowErrorStatus(xMessage.Status);

		return	false;
	}

	return	true;
}


bool	LORAWAN_SendDevTimeReq(void)
{
	TRACE("Send DevTimeReq\n");

	MlmeReq_t mlmeReq;

	mlmeReq.Type = MLME_DEV_TIME;

	if (LoRaMacMlmeRequest( &mlmeReq ) != LORAMAC_STATUS_OK)
	{
		ERROR("LoRaMacMlmeRequest failed.\n");
		return	false;
	}

	LORA_PACKET		xMessage;

	xMessage.Port = 0;
	xMessage.Request = MCPS_CONFIRMED;
	xMessage.Buffer = NULL;
	xMessage.Size = 0;
	if (LORAWAN_SendMessage(&xMessage)  != LORAMAC_STATUS_OK)
	{
		 LORAWAN_ShowErrorStatus(xMessage.Status);

		return	false;
	}

	return	true;
}

bool	LORAWAN_SendLinkCheckRequest(void)
{
	TRACE("Send Link Check\n");

	MlmeReq_t mlmeReq;
	mlmeReq.Type = MLME_LINK_CHECK;

	if (LoRaMacMlmeRequest( &mlmeReq ) != LORAMAC_STATUS_OK)
	{
		ERROR("LoRaMacMlmeRequest failed.\n");
		return	false;
	}

	LORA_PACKET		xMessage;

	xMessage.Buffer = NULL;
	xMessage.Port = 0;
	xMessage.Request = MCPS_CONFIRMED;
	xMessage.Size = 0;
	if (LORAWAN_SendMessage(&xMessage)  != LORAMAC_STATUS_OK)
	{
		 LORAWAN_ShowErrorStatus(xMessage.Status);
	}

	return	true;
}
bool LORAWAN_IsNetworkJoined() {
	// Did we already join the network ?
	mibReq.Type = MIB_NETWORK_JOINED;
	mibReq.Param.IsNetworkJoined = true;
	LoRaMacMibGetRequestConfirm( &mibReq );
	// We are already attached
	return ( mibReq.Param.IsNetworkJoined );
}

MlmeConfirm_t* LORAWAN_GetMlmeConfirm(void) {
	return &LocalMcps.mlme;
}

McpsConfirm_t* LORAWAN_GetConfirm(void) {
	return &LocalMcps.confirm;
}

McpsIndication_t* LORAWAN_GetIndication(void) {
	return &LocalMcps.indication;
}

uint16_t LORAWAN_GetDownLinkCounter(void) {
	return LoRaDownLinkCounter;
}

void LORAWAN_ResetDownLinkCounter(void) {
	LoRaDownLinkCounter = 0;
}

bool LORAWAN_SetDefaultDR(uint8_t nDR)
{
	if (nDR <= LORAMAC_TX_DATARATE_MAX)
	{
		LoRaWAN_DefaultDR = nDR;
		LORAMAC_SetDatarate(nDR);
		return	true;
	}

	return	false;
}

uint8_t LORAWAN_GetDefaultDR(void)
{
	return	LoRaWAN_DefaultDR;
}

bool LORAWAN_SetMaxRetries(uint8_t retries) {
	if (retries < 9)
	{
		LoRaWAN_Retries = (retries) ? retries : LORAWAN_RETRIES;
		return	true;
	}

	return	false;
}

uint8_t LORAWAN_GetMaxRetries(void) {
	return LoRaWAN_Retries;
}

uint8_t	LORAWAN_GetSNR(void)
{
	return	nSNR;
}

int16_t	LORAWAN_GetRSSI(void)
{
	return	nRSSI;
}

uint32_t	LORAWAN_SetMessage(LORA_MESSAGE* pMessage, uint32_t ulMaxSize, uint8_t nType, uint8_t* pPayload, uint8_t nPayloadLen)
{
	if (sizeof(LORA_MESSAGE) + nPayloadLen - 1 > ulMaxSize)
	{
		return	0;
	}

	pMessage->Version = LORA_MESSAGE_VERSION;
	pMessage->MessageType = nType;
	pMessage->PayloadLen = nPayloadLen;
	memcpy(pMessage->Payload, pPayload, nPayloadLen);

	return	sizeof(LORA_MESSAGE) + nPayloadLen - 1;
}

void	LORAWAN_ShowErrorStatus(LoRaMacEventInfoStatus_t xStatus)
{
	switch(xStatus)
	{
	case LORAMAC_EVENT_INFO_STATUS_ERROR:
		ERROR("Error!\n");
		DeviceFlashLed(10);
		break;
	case LORAMAC_EVENT_INFO_STATUS_TX_TIMEOUT:
		ERROR("Tx Timeout!\n");
		DeviceFlashLed(3);
		break;
	default:
		DeviceFlashLed(5);
		break;
	}
}

/** }@ */
