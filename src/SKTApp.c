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
#include "supervisor.h"
#include "trace.h"

#undef	__MODULE__
#define	__MODULE__	"SKT"


static void SKTAPP_SendPeriodicDataExt(uint8_t messageType, bool retry);	// Forward definition

static bool DEVICEAPP_ExecSKTNetwork(LORA_MESSAGE* msg) {
bool rc = false;
	if (msg->Version > LORA_MESSAGE_VERSION) return rc; 	// Ignore malformed message
	switch(msg->MessageType) {
/* This is an uplink message only
	case 0:
		break;
*/
	case 1:
		// Add your code here
		break;
/* This is an uplink message only
	case 2:
		break;
*/
	case 3:
		// Add your code here
		break;
	case 4:
		if (msg->PayloadLen > 0)
			LORAWAN_SetMaxRetries(msg->Payload[0]);
		break;
	}
	return rc;
}

static bool DEVICEAPP_ExecSKTDevice(LORA_MESSAGE* msg) {
bool rc = false;
	if (msg->Version > LORA_MESSAGE_VERSION) return rc; 	// Ignore malformed message
	switch(msg->MessageType) {
	case 0x00:
		// Add your code here
		break;
	case 0x80:
		// WARNING: Upon factory reset the unit will not communicated with the LoRaWAN anymore
		// until it is re-installed using the magnet.
		// If a factory reset is requested, uncomment next line
//		DeviceUserDataSetFlag(FLAG_INSTALLED, 0);
		SystemReboot();
		// This code will never return
		break;
	case 0x81:
		// I don't know the expected size of this information nor the endian type
		// As an example I assume it's a long as the data is transfered in seconds
		if (msg->PayloadLen >= sizeof(unsigned long))
			SUPERVISORStartCyclicTask(0, *((unsigned long*)(msg->Payload)) / 60);
		break;
	case 0x82:
		// Just force Supervisor to send standard uplink message
		DevicePostEvent(PERIODIC_RESEND);
		break;
	}
	return rc;
}

static bool DEVICEAPP_ExecDaliworks(LORA_MESSAGE* msg) {
bool rc = false;
	if (msg->Version > LORA_MESSAGE_VERSION) return rc; 	// Ignore malformed message
	switch(msg->MessageType) {
/* This is an uplink message only
	case 01:
		break;
*/
	case 0x81:
		// WARNING: Upon factory reset the unit will not communicated with the LoRaWAN anymore
		// until it is re-installed using the magnet.
		// If a factory reset is requested, uncomment next line
//		DeviceUserDataSetFlag(FLAG_INSTALLED, 0);
		SystemReboot();
		// This code will never return
		break;
	case 0x83:
		// I don't know the expected size of this information nor the endian type
		// As an example I assume it's a long as the data is transfered in seconds
		if (msg->PayloadLen >= sizeof(unsigned long)) {
			SUPERVISORStartCyclicTask(0, *((unsigned long*)(msg->Payload)) / 60);
			LocalMessage.Port = LORAWAN_APP_PORT;
			LocalMessage.Request = MCPS_UNCONFIRMED;
			LocalMessage.Message->MessageType = 0x84;
			LocalMessage.Message->PayloadLen = sizeof(unsigned long);
			*((unsigned long*)(msg->Payload)) = SUPERVISORGetRFPeriod() * 60;
			rc = true;
		}
		break;
/* This is an uplink message only
	case 0x84:
		break;
*/
	case 0x85:
		SKTAPP_SendPeriodicDataExt(0x86,true);
		break;
/* This is an uplink message only
	case 0x86:
		break;
*/
	case 0x87:
	{
		/*
		 * For the battery estimation, I let you do the math, knowing that:
		 * The battery original power is 8500 mA/h
		 * The consumption during TX is about 120 mA
		 * The consumption during RX is about 10 mA
		 * The idle consumption is about 0.01 mA
		 * Now, you have to get the total amount of time spent in TX, the total amount of time
		 * spent in RX (I don't know whether this information is available in standard in the
		 * LoRaMac program). Estimate the total amount of time in Idle mode (or estimate a loss
		 * of 1mA every 100 hours (= 4 days and 4 hours).
		 * Subtract all these values to the original power and you get the info of the remaining power.
		 * But, this will not give you the remaining time as you don't really know in advance how
		 * many transmission retries, receiver retries, etc. will occur in the future.
		 * Now, the main issue is that all these accumulated values are lost if the unit resets,
		 * you then have to save them permanently in Flash. But, you need to do a balance between
		 * the loss of this data and the maximum number of Flash write...
		 * I personally prefer to use the battery low level detection, when the battery drops under
		 * a certain voltage (3.00 V for instance) then you set a flag stating that the battery will
		 * soon (a couple of months) be dead.
		 */
		unsigned long BatteryEstimation = 0;
		LocalMessage.Port = LORAWAN_APP_PORT;
		LocalMessage.Request = MCPS_UNCONFIRMED;
		LocalMessage.Message->MessageType = 0x88;
		LocalMessage.Message->PayloadLen = sprintf((char*)LocalMessage.Message->Payload,
				"%04X,%04X,DW-S47-%08ld,%ld",
				DeviceHWVersion(),
				DeviceVersion(),
				UNIT_SERIALNUMBER,
				BatteryEstimation);
		rc = true;
	}
		break;
/* This is an uplink message only
	case 0x88:
		break;
*/
	}
	return rc;
}

/*
 * @brief Send standard data using a specific messageType
 */
static uint32_t PeriodicCount = 0;
static void SKTAPP_SendPeriodicDataExt(uint8_t messageType, bool retry) {
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

void SKTAPP_SendPeriodic(bool retry) {
	SKTAPP_SendPeriodicDataExt(1, retry);
}

void SKTAPP_ParseMessage(McpsIndication_t* ind) {
bool rc = false;
	LocalMessage.Buffer = LocalBuffer;	/* Just to be sure */
	if (ind) {
		if (ind->Status == LORAMAC_EVENT_INFO_STATUS_OK) {
			if (ind->AckReceived) {
				DeviceFlashLed(1);	// Show that we received an acknowledgment to a confirmed message
			}
			if (ind->RxData) {
				LORA_PACKET *msg = LORAWAN_GetMessage();
				if (msg) {
					switch(msg->Port) {
					case LORAWAN_APP_PORT:
						if (msg->Size > 0) {
							rc = DEVICEAPP_ExecSKTNetwork(msg->Message);
						}
						break;
					case SKT_DEVICE_SERVICE_PORT:
						if (msg->Size > 0) {
							rc = DEVICEAPP_ExecSKTDevice(msg->Message);
						}
						break;
					case SKT_NETWORK_SERVICE_PORT:
						if (msg->Size > 0) {
							rc = DEVICEAPP_ExecDaliworks(msg->Message);
						}
						break;
#if (INCLUDE_COMPLIANCE_TEST > 0)
					case 224: // 0xE0
						DEVICEAPP_RunComplianceTest(ind);
						break;
#endif
					default: break;
					}
				}
				/* If rc is true, a message has been prepared and we need to transmit it) */
				if (rc) {
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
				}
			}
		}
	}
}

void SKTAPP_ParseMlme(MlmeConfirm_t *m) {
#if (INCLUDE_COMPLIANCE_TEST > 0)
	if (Compliance_ParseMlme(m)) return;
#endif
}


/** }@ */
