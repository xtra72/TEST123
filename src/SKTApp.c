/*
 * deviceApp.c
 *
 * This file contains the global device Application Layer
 */
/** \addtogroup S40 S40 Main Application
 * @brief S40 module main application files
 *  @{
 */
#include "global.h"
#include "deviceApp.h"
#include "SKTApp.h"
#include "DaliworksApp.h"
#include "supervisor.h"
#include "trace.h"
#include "LoRaMacCrypto.h"
#include "lorawan_task.h"

#undef	__MODULE__
#define	__MODULE__	"SKT"

static uint8_t AppNonce[3];
static StaticSemaphore_t xSemaphoreBuffer;
static xSemaphoreHandle SKTAppSemaphore;
static SKTAppStatus_t	SKTApp_xStatus;
static bool				SKTApp_ConfirmedMsgType = false;

void SKTAPP_SendPeriodicDataExt(uint8_t messageType, bool retry);	// Forward definition
bool DEVICEAPP_ExecDaliworks(LORA_MESSAGE* msg);

static bool DEVICEAPP_ExecSKTNetwork(LORA_MESSAGE* msg)
{
	bool rc = false;

	if (msg->Version > LORA_MESSAGE_VERSION) return rc; 	// Ignore malformed message

	switch(msg->MessageType)
	{
	case MSG_SKT_NET_REAL_APP_KEY_ALLOC_ANS:
		if (msg->PayloadLen == sizeof(AppNonce))
		{
			memcpy(AppNonce, msg->Payload, 3);
			TRACE("%16s - ", "App Nonce"); TRACE_DUMP(AppNonce, sizeof(AppNonce));

			if(SKTAppSemaphore) xSemaphoreGive(SKTAppSemaphore);
		}
		break;

	case MSG_SKT_NET_REAL_APP_KEY_RX_REPORT_ANS:
		{
			uint8_t RealAppKey[16];

			LoRaMacJoinComputeRealAppKey( UNIT_APPKEY, AppNonce, UNIT_NETWORKID, RealAppKey );

			TRACE("%16s : ", "App Nonce"); TRACE_DUMP(AppNonce, sizeof(AppNonce));
			TRACE("%16s : %06x\n", "Net ID", UNIT_NETWORKID);
			TRACE("%16s : ", "Real App Key"); TRACE_DUMP(RealAppKey, sizeof(RealAppKey));

			DeviceUserDataSetSKTRealAppKey(RealAppKey);

			if(SKTAppSemaphore) xSemaphoreGive(SKTAppSemaphore);
		}
		break;

	case MSG_SKT_NET_CONFIRMED_UP_NB_RETRANS:
		{
			if (msg->PayloadLen > 0)
			{
				LORAWAN_SetMaxRetries(msg->Payload[0]);
			}
		}
		break;
	}

	return rc;
}

static bool DEVICEAPP_ExecSKTDevice(LORA_MESSAGE* msg)
{
	bool rc = false;
	if (msg->Version > LORA_MESSAGE_VERSION) return rc; 	// Ignore malformed message
	TRACE("Exec STK : %02x\n", msg->MessageType);
	switch(msg->MessageType)
	{
	case MSG_SKT_DEV_EXT_DEVICE_MANAGEMENT:
		// Add your code here
		LORAWAN_SendAck();
		rc = true;
		break;

	case MSG_SKT_DEV_RESET:
		// WARNING: Upon factory reset the unit will not communicated with the LoRaWAN anymore
		// until it is re-installed using the magnet.
		// If a factory reset is requested, uncomment next line
//		DeviceUserDataSetFlag(FLAG_INSTALLED, 0);
		LORAWAN_SendAck();
		DevicePostEvent(SYSTEM_RESET);
		// This code will never return
		break;

	case MSG_SKT_DEV_SET_UPLINK_DATA_INTERVAL:
		{
			uint32_t	ulPeriod = 0;
			// I don't know the expected size of this information nor the endian type
			// As an example I assume it's a long as the data is transfered in seconds
			if (msg->PayloadLen == 1)
			{
				ulPeriod = *((uint8_t *)(msg->Payload));
			}
			else if (msg->PayloadLen == 2)
			{
				ulPeriod = *((uint16_t *)(msg->Payload));
			}
			if (msg->PayloadLen == 4)
			{
				ulPeriod = *((uint32_t *)(msg->Payload));
			}

			if (ulPeriod != 0)
			{
				SUPERVISOR_StartCyclicTask(0, ulPeriod);
			}

			LORAWAN_SendAck();
		}
		break;

	case MSG_SKT_DEV_UPLINK_DATA_REQ:
		// Just force Supervisor to send standard uplink message
		DevicePostEvent(PERIODIC_RESEND);
		break;
	}
	return rc;
}


bool	SKTAPP_Init(void)
{
	 if (!SKTAppSemaphore)
	 {
		 SKTAppSemaphore = xSemaphoreCreateBinaryStatic( &xSemaphoreBuffer );
	 }

	 SKTApp_xStatus = SKTAPP_STATUS_IDLE;

	 return	true;
}

/*
 * @brief Send standard data using a specific messageType
 */
 bool SKTAPP_SendRealAppKeyAllocReq(void)
{
	 bool	rc = false;
	 LocalMessage.Buffer = LocalBuffer;
	 LocalMessage.Port = SKT_NETWORK_SERVICE_PORT;
	 LocalMessage.Request = MCPS_UNCONFIRMED;
	 LocalMessage.Message->MessageType = MSG_REAL_APP_KEY_ALLOC_REQ;
	 LocalMessage.Message->Version = LORA_MESSAGE_VERSION;
	 LocalMessage.Message->PayloadLen = 0;
	 LocalMessage.Size = LORA_MESSAGE_HEADER_SIZE + LocalMessage.Message->PayloadLen;

	 if (SKTAppSemaphore)	xSemaphoreTake(SKTAppSemaphore, 0);

	 SKTApp_xStatus = SKTAPP_STATUS_REQ_REAL_APP_KEY_ALLOC;
	 if (LORAWAN_SendMessage(&LocalMessage) == LORAMAC_STATUS_OK)
	 {
		 if (SKTAppSemaphore)
		 {
			 if (xSemaphoreTake(SKTAppSemaphore, 7 * configTICK_RATE_HZ + 1))
			 {
				 SKTApp_xStatus = SKTAPP_STATUS_REQ_REAL_APP_KEY_ALLOC_COMPLETED;
				 rc = true;
			 }
		 }
	 }
	 else
	 {
		 switch(LocalMessage.Status)
		 {
		 case LORAMAC_EVENT_INFO_STATUS_ERROR:
			 ERROR("MSG_REAL_APP_KEY_ALLOC_REQ failed.\n");
			 DeviceFlashLed(10);
			 break;

		 case LORAMAC_EVENT_INFO_STATUS_TX_TIMEOUT:
			 ERROR("MSG_REAL_APP_KEY_ALLOC_REQ tx timeout.\n");
			 DeviceFlashLed(3);
			 break;

		 default:
			 ERROR("MSG_REAL_APP_KEY_ALLOC_REQ failed.\n");
			 DeviceFlashLed(5);
			 break;
		 }
	}

	 return	rc;
}

 /*
  * @brief Send standard data using a specific messageType
  */
 bool SKTAPP_SendRealAppKeyRxReportReq(void)
 {
	 bool	rc = false;

	 LocalMessage.Buffer = LocalBuffer;
	 LocalMessage.Port = SKT_NETWORK_SERVICE_PORT;
	 LocalMessage.Request = MCPS_UNCONFIRMED;
	 LocalMessage.Message->MessageType = MSG_REAL_APP_KEY_RX_REPORT_REQ;
	 LocalMessage.Message->Version = LORA_MESSAGE_VERSION;
	 LocalMessage.Message->PayloadLen = 0;
	 LocalMessage.Size = LORA_MESSAGE_HEADER_SIZE + LocalMessage.Message->PayloadLen;

	 if (SKTAppSemaphore)	xSemaphoreTake(SKTAppSemaphore, 0);

	 SKTApp_xStatus = SKTAPP_STATUS_REQ_REAL_APP_KEY_RX_REPORT;
	 if (LORAWAN_SendMessage(&LocalMessage)  == LORAMAC_STATUS_OK)
	 {
		 if (SKTAppSemaphore)
		 {
			 if (xSemaphoreTake(SKTAppSemaphore, 7 * configTICK_RATE_HZ + 1))
			 {
				 SKTApp_xStatus = SKTAPP_STATUS_REQ_REAL_APP_KEY_RX_REPORT_COMPLETED;
				 rc = true;
			 }
		 }
	 }
	 else
 	 {
		 switch(LocalMessage.Status)
 		 {
 		 case LORAMAC_EVENT_INFO_STATUS_ERROR:
  			 ERROR("MSG_REAL_APP_KEY_RX_REPORT_REQ failed.");
   			 DeviceFlashLed(10);
   			 break;

 		 case LORAMAC_EVENT_INFO_STATUS_TX_TIMEOUT:
 			 ERROR("MSG_REAL_APP_KEY_RX_REPORT_REQ tx timeout.");
 			 DeviceFlashLed(3);
 			 break;

 		 default:
  			 ERROR("MSG_REAL_APP_KEY_RX_REPORT_REQ failed.");
 			 DeviceFlashLed(5);
   			 break;
 		 }
 	 }

	 return	rc;
 }


/*
 * @brief Send standard data using a specific messageType
 */
static uint32_t PeriodicCount = 0;
void SKTAPP_SendPeriodicDataExt(uint8_t messageType, bool retry)
{
#if (INCLUDE_COMPLIANCE_TEST > 0)
	if (Compliance_SendPeriodic()) return;
#endif
	LocalMessage.Buffer = LocalBuffer;
	LocalMessage.Port = LORAWAN_APP_PORT;
	LocalMessage.Request = MCPS_UNCONFIRMED;
	LocalMessage.Message->MessageType = messageType;
	LocalMessage.Message->Version = LORA_MESSAGE_VERSION;
	LocalMessage.Message->PayloadLen = sprintf((char*)&(LocalMessage.Message->Payload[0]),
			"%ld,%ld",PeriodicCount++,(retry) ? SUPERVISOR_GetHistoricalValue(0,0) : DeviceGetPulseInValue(0));
	if (DeviceGetPulseInNumber() > 1)
		LocalMessage.Message->PayloadLen += sprintf((char*)&(LocalMessage.Message->Payload[LocalMessage.Message->PayloadLen]),
				",%ld",(retry) ? SUPERVISOR_GetHistoricalValue(0,1) : DeviceGetPulseInValue(1));
	LocalMessage.Size = LORA_MESSAGE_HEADER_SIZE + LocalMessage.Message->PayloadLen;
	TRACE("SendPeriodicData : "); TRACE_DUMP(LocalMessage.Message->Payload, LocalMessage.Message->PayloadLen);
	if (LORAWAN_SendMessage(&LocalMessage)  != LORAMAC_STATUS_OK) {
		switch(LocalMessage.Status) {
			case LORAMAC_EVENT_INFO_STATUS_ERROR:
				ERROR("LORAMAC_EVENT_INFO_STATUS_ERROR");
				DeviceFlashLed(10);
				break;
			case LORAMAC_EVENT_INFO_STATUS_TX_TIMEOUT:
				ERROR("LORAMAC_EVENT_INFO_STATUS_TX_TIMEOUT");
				DeviceFlashLed(3);
				break;
			default:
				DeviceFlashLed(5);
				break;
		}
	}
}

void SKTAPP_SendPeriodic(bool retry)
{
	SKTAPP_SendPeriodicDataExt(1, retry);
}


bool SKTAPP_Send(uint8_t port, uint8_t messageType, uint8_t *pFrame, uint32_t ulFrameLen)
{
	LocalMessage.Buffer = LocalBuffer;
	LocalMessage.Port = port;
	if (SKTApp_ConfirmedMsgType)
	{
		LocalMessage.Request = MCPS_CONFIRMED;
	}
	else
	{
		LocalMessage.Request = MCPS_UNCONFIRMED;
	}
	LocalMessage.Message->MessageType = messageType;
	LocalMessage.Message->Version = LORA_MESSAGE_VERSION;
	LocalMessage.Message->PayloadLen = ulFrameLen;
	memcpy(LocalMessage.Message->Payload, pFrame, ulFrameLen);
	LocalMessage.Size = LORA_MESSAGE_HEADER_SIZE + LocalMessage.Message->PayloadLen;
	TRACE("SendPeriodicData : "); TRACE_DUMP(LocalMessage.Message->Payload, LocalMessage.Message->PayloadLen);
	if (LORAWAN_SendMessage(&LocalMessage)  != LORAMAC_STATUS_OK)
	{
		switch(LocalMessage.Status)
		{
		case LORAMAC_EVENT_INFO_STATUS_ERROR:
			ERROR("LORAMAC_EVENT_INFO_STATUS_ERROR");
			DeviceFlashLed(10);
			break;
		case LORAMAC_EVENT_INFO_STATUS_TX_TIMEOUT:
			ERROR("LORAMAC_EVENT_INFO_STATUS_TX_TIMEOUT");
			DeviceFlashLed(3);
			break;
		default:
			DeviceFlashLed(5);
			break;
		}
		return	false;
	}

	return	true;
}


bool SKTAPP_ParseMessage(McpsIndication_t* ind)
{
	bool rc = false;
	LocalMessage.Buffer = LocalBuffer;	/* Just to be sure */

	if (ind)
	{
		TRACE("ind->Status = %d\n", ind->Status);
		if (ind->Status == LORAMAC_EVENT_INFO_STATUS_OK)
		{
			if (ind->AckReceived)
			{
				DeviceFlashLed(1);	// Show that we received an acknowledgment to a confirmed message
			}
			TRACE("ind->RxData= %08x\n", (uint32_t)ind->RxData);
			if (ind->RxData)
			{
				LORA_PACKET *msg = LORAWAN_GetMessage();
				if (msg)
				{
					switch(msg->Port)
					{
					case SKT_DEVICE_SERVICE_PORT:
						TRACE("SKT Device Service Port received.\n");
						if (msg->Size > 0) {
							rc = DEVICEAPP_ExecSKTDevice(msg->Message);
						}
						break;
					case SKT_NETWORK_SERVICE_PORT:
						TRACE("SKT Network Service Port received.\n");
						if (msg->Size > 0) {
							rc = DEVICEAPP_ExecSKTNetwork(msg->Message);
						}
						break;
					case DALIWORKS_SERVICE_PORT:
						TRACE("Daliworks Service Port received.\n");
						if (msg->Size > 0) {
							rc = DEVICEAPP_ExecDaliworks(msg->Message);
						}
						break;
#if (INCLUDE_COMPLIANCE_TEST > 0)
					case 224: // 0xE0
						DEVICEAPP_RunComplianceTest(ind);
						break;
#endif
					default:
						TRACE("Unknown Port : %d\n", msg->Port);
						break;
					}
				}
				/* If rc is true, a message has been prepared and we need to transmit it) */
				if (rc) {
#if 0
					LocalMessage.Message->Version = LORA_MESSAGE_VERSION;
					LocalMessage.Size = LORA_MESSAGE_HEADER_SIZE + LocalMessage.Message->PayloadLen;

					if (LORAWAN_SendMessage(&LocalMessage)  != LORAMAC_STATUS_OK) {
						switch(LocalMessage.Status) {
							case LORAMAC_EVENT_INFO_STATUS_ERROR:
								ERROR("LORAMAC_EVENT_INFO_STATUS_ERROR");
								DeviceFlashLed(10);
								break;
							case LORAMAC_EVENT_INFO_STATUS_TX_TIMEOUT:
								ERROR("LORAMAC_EVENT_INFO_STATUS_TX_TIMEOUT");
								DeviceFlashLed(3);
								break;
							default:
								DeviceFlashLed(5);
								break;
						}
					}
#else
					LORAWAN_SendAck();
#endif
				}
			}
		}
	}

	return	rc;
}

void SKTAPP_ParseMlme(MlmeConfirm_t *m)
{
#if (INCLUDE_COMPLIANCE_TEST > 0)
	if (Compliance_ParseMlme(m)) return;
#endif
}

bool SKTAPP_GetPeriodicMode(void)
{
	return	SUPERVISOR_IsPeriodicMode();
}

bool SKTAPP_SetPeriodicMode(bool bEnable)
{

	SUPERVISOR_SetPeriodicMode(bEnable);

	return	true;
}

bool SKTAPP_SetConfirmedMsgType(bool bConfirmed)
{
	SKTApp_ConfirmedMsgType = bConfirmed;

	return	true;
}

bool SKTAPP_IsConfirmedMsgType(void)
{
	return	SKTApp_ConfirmedMsgType;
}


/** }@ */
