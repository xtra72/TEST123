/*
 * mmiApp.c
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
#define	__MODULE__	"MMI"


#ifdef LORAWAN_APP_PORT
#undef LORAWAN_APP_PORT
#endif
#define LORAWAN_APP_PORT 2

/*!
 * @brief Define the LoRaWAN FPort to use to issue device Service Commands
 */
#define SERVICE_PORT	3
/*!
 * @brief List of device Service Commands
 * @remark Expand this list as needed to add new features
 */
typedef enum {
	GetStatus = 0,			//!< Get Device Status
	SetRFPeriod=1, 			//!< Set Device RF periodic delay in min. (from 2 to 240) for periodic message transfers
	GetRFPeriod=2,			//!< Get Device RF Periodic delay in min. (from 2 to 240) for periodic message transfers
	/*!
	 * @brief Number of periodic unconfirmed messages sent between confirmed messages sent
	 * to the LoRaWAN network
	 * @remark Use zero to send only unconfirmed periodic messages
	 */
	SetConfirmed=3,
	GetConfirmed=4,			//!< Get the number of periodic unconfirmed messages before sending confirmed one
	SetMaxRetries=6,		//!< Set the maximum number of retries for confirmed LoRaWAN messages
	GetMaxRetries=7,		//!< Get the maximum number of retries for confirmed LoRaWAN messages
	SetResetFlag=8,			//!< Set this user reset flag to disable magnet device reset feature
	GetResetFlag=9,			//!< Get the user reset flag value. True if disabled
	SetUseOTAA=10,			//!< Set the use OTAA flag for LoRaWAN network join
	GetUseOTAA=11,			//!< Get the use OTAA flag value
	GetBatteryLevel=12,		//!< Get current battery voltage in mV
	GetLastValues = 16,		//!< Re-send last sent values
	GetMaximumArchivedValues=17, //!< Get the maximum depth of historical data saved
	GetArchivedValues=18,	//!< Get a set of historical data values
	FactoryReset=128,		//!< Perform a factory reset of the device -> <b>!! No more communication afterwards !!</b>
	DeviceReset=129			//!< Perform a device reset. The communication restarts with a join network
}SERVICE_CMD;

LORA_PACKET LocalMessage;
uint8_t LocalBuffer[255];
static int _loopUnconfirmed = 0;
static int _loopConfirmed = 12;		// By default, send a confirmed packet every 12 packets (1/2 day @ 60 min. period)

static void SendPacket(void) {
	if (LORAWAN_SendMessage(&LocalMessage)  != LORAMAC_STATUS_OK) {
		switch(LocalMessage.Status) {
			case LORAMAC_EVENT_INFO_STATUS_ERROR:
				ERROR("LORAMAC_EVENT_INFO_STATUS_TX_TIMEOUT");
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

static bool DEVICEAPP_ExecCommand(LORA_PACKET *msg) {
bool rc = false;
	if (msg->Size > 0) {
		switch((SERVICE_CMD)msg->Buffer[0]) {
		case GetStatus:
			LocalMessage.Size = sizeof(unsigned char);
			LocalMessage.Buffer[1] = (uint8_t)(DeviceStatus);
			rc = true;
			break;
		case SetRFPeriod:
			if (msg->Size == 1)
			{
				uint8_t	period = msg->Buffer[1];
				SUPERVISOR_StartCyclicTask(0, period);
			}
			else if (msg->Size == 2)
			{
				uint16_t	period = *(uint16_t *)&msg->Buffer[1];
				SUPERVISOR_StartCyclicTask(0, period);
			}
			else if (msg->Size == 4)
			{
				uint32_t	period = *(uint32_t *)&msg->Buffer[1];
				SUPERVISOR_StartCyclicTask(0, period);
			}
			break;
		case GetRFPeriod:
			LocalMessage.Size = sizeof(uint32_t);
			*(uint32_t *)&LocalMessage.Buffer[1] = SUPERVISOR_GetRFPeriod();
			rc = true;
			break;
		case SetConfirmed:
			if (msg->Size > 1) {
				_loopUnconfirmed = 0;
				_loopConfirmed = msg->Buffer[1];
			}
			break;
		case GetConfirmed:
			LocalMessage.Size = sizeof(unsigned char);
			LocalMessage.Buffer[1] = (unsigned char)_loopConfirmed;
			rc = true;
			break;
		case SetMaxRetries:
			if (msg->Size > 1) {
				LORAWAN_SetMaxRetries(msg->Buffer[1]);
			}
			break;
		case GetMaxRetries:
			LocalMessage.Size = sizeof(unsigned char);
			LocalMessage.Buffer[1] = LORAWAN_GetMaxRetries();
			rc = true;
			break;
		case SetResetFlag:
			if (msg->Size > 1) {
				UPDATE_USERFLAG(FLAG_DISALLOW_RESET, msg->Buffer[1]);
			}
			break;
		case GetResetFlag:
			LocalMessage.Size = sizeof(unsigned char);
			LocalMessage.Buffer[1] = UNIT_DISALLOW_RESET ? 1 : 0;
			rc = true;
			break;
		case SetUseOTAA:
			if (msg->Size > 1) {
				UPDATE_USERFLAG(FLAG_USE_OTAA, msg->Buffer[1]);
			}
			break;
		case GetUseOTAA:
			LocalMessage.Size = sizeof(unsigned char);
			LocalMessage.Buffer[1] = UNIT_USE_OTAA ? 1 : 0;
			rc = true;
			break;
		case GetBatteryLevel: {
			LocalMessage.Size = sizeof(unsigned short);
			unsigned long Batt = SystemBatteryGetVoltage();
			LocalMessage.Buffer[1] = (unsigned char)(Batt & 0xFF);
			LocalMessage.Buffer[2] = (unsigned char)((Batt >> 8) & 0xFF);
			rc = true;
			}
			break;
		case GetLastValues:
			DeviceSendEvent(PERIODIC_RESEND);
			break;
		case GetMaximumArchivedValues:
			LocalMessage.Size = sizeof(unsigned short);
			*((unsigned short*)&LocalMessage.Buffer[1]) = SUPERVISOR_GetHistoricalCount();
			rc = true;
			break;
		case GetArchivedValues:
			if (msg->Size > 3) {
				LocalMessage.Size = sizeof(short) + 1;
				unsigned short size = (USERDATAPTR)->DeviceType;
				memcpy(&LocalMessage.Buffer[1],&size, sizeof(short));
				size = (size >= 16) ? sizeof(unsigned short) : sizeof(unsigned long);
				LocalMessage.Buffer[3] = (uint8_t)(DeviceStatus);
				// Copy input request parameters in case a new LORAWAN message would overwrite the buffer
				int rank = msg->Buffer[1] + (msg->Buffer[2] * 256);
				int nb = min(msg->Buffer[3],((LORAWAN_MAX_MESSAGE_SIZE - LocalMessage.Size) / (DeviceGetPulseInNumber()*size)));
				nb = min(nb,SUPERVISOR_GetHistoricalCount());
				// If type > 16 then values can be contained in unsigned short, LoRaWAN message will be shorter
				unsigned long *ptr4 = (unsigned long*)&LocalMessage.Buffer[4];
				unsigned short *ptr2= (unsigned short*)&LocalMessage.Buffer[4];
				// Compute maximum number of messages that can be sent out to avoid buffer overflow attack
				// or LORAWAN maximum message size breach
				while(nb--) {
					for (int i = 0; i < DeviceGetPulseInNumber(); i++) {
						if ( size < 4)
							*(ptr2++) = (unsigned short)SUPERVISOR_GetHistoricalValue(rank,i);
						else
							*(ptr4++) = SUPERVISOR_GetHistoricalValue(rank,i);
						LocalMessage.Size += size;
					}
					rank++;
				}
				rc = true;
			}
			break;
		case FactoryReset:
			DeviceUserDataSetFlag(FLAG_INSTALLED, 0);
			/* no break */
		case DeviceReset:
			SystemReboot();
			break;
		}
	}
	if (rc) {
		// If we send an answer back, put default packet type and add Command Type as first byte
		LocalMessage.Port = SERVICE_PORT;
		LocalMessage.Request = MCPS_UNCONFIRMED;
		LocalMessage.Size += sizeof(unsigned char);
		LocalMessage.Buffer[0] = msg->Buffer[0];
	}
	return rc;
}

void DEVICEAPP_SendPeriodic(bool retry) {
#if (INCLUDE_COMPLIANCE_TEST > 0)
	if (Compliance_SendPeriodic()) return;
#endif
	LocalMessage.Buffer = LocalBuffer;
	LocalMessage.Port = LORAWAN_APP_PORT;
	if (_loopConfirmed && (++_loopUnconfirmed >= _loopConfirmed)) {
		LocalMessage.Request = MCPS_CONFIRMED;
		LocalMessage.NbTrials = LORAWAN_GetMaxRetries();
	} else
	{
		LocalMessage.Request = MCPS_UNCONFIRMED;
	}
	unsigned short size = (USERDATAPTR)->DeviceType;
	memcpy(&LocalMessage.Buffer[0],&size, sizeof(short));
	size = (size >= 16) ? sizeof(unsigned short) : sizeof(unsigned long);
	LocalMessage.Size = sizeof(short) + 1 + size;
	LocalMessage.Buffer[2] = (uint8_t)(DeviceStatus);
	memcpy(&LocalMessage.Buffer[3],DeviceGetPulseInValuePtr(0),size);
	if (DeviceGetPulseInValuePtr(1)) {
		memcpy(&LocalMessage.Buffer[3+size],DeviceGetPulseInValuePtr(1),size);
		LocalMessage.Size += size;
	}
	SendPacket();
}

void DEVICEAPP_ParseMessage(McpsIndication_t* ind) {
	LocalMessage.Buffer = LocalBuffer;	// Just in case
	if (ind) {
		if (ind->Status == LORAMAC_EVENT_INFO_STATUS_OK) {
			if (ind->AckReceived) {
				DeviceFlashLed(1);
				_loopUnconfirmed = 0;
			}
			if (ind->RxData) {
				LORA_PACKET *msg = LORAWAN_GetMessage();
				if (msg) {
					switch(msg->Port) {
					case SERVICE_PORT:
						if (DEVICEAPP_ExecCommand(msg)) SendPacket();
						break;
#if (INCLUDE_COMPLIANCE_TEST > 0)
					case 224:
						DEVICEAPP_RunComplianceTest(ind);
						break;
#endif
					default: break;
					}
				}
			}
		}
	}
}

void DEVICEAPP_ParseMlme(MlmeConfirm_t *m) {
#if (INCLUDE_COMPLIANCE_TEST > 0)
	if (Compliance_ParseMlme(m)) return;
#endif
}


/** }@ */
