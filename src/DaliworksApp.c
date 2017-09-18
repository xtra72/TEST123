/*
 * DALIApp.c
 *
 *  Created on: 2017. 9. 15.
 *      Author: inhyuncho
 */

#include "global.h"
#include "deviceApp.h"
#include "supervisor.h"
#include "trace.h"


#undef	__MODULE__
#define	__MODULE__	"DALIWORKS"

 void SKTAPP_SendPeriodicDataExt(uint8_t messageType, bool retry);	// Forward definition

bool DEVICEAPP_ExecDaliworks(LORA_MESSAGE* msg) {
bool rc = false;
	if (msg->Version > LORA_MESSAGE_VERSION) return rc; 	// Ignore malformed message
	TRACE("Exec Daliworks : %02x\n", msg->MessageType);
	switch(msg->MessageType) {
	case 01:

		TRACE("%16s : %02x %02x %02x\n", "App Nonce", msg->Payload[0], msg->Payload[1], msg->Payload[2]);
		break;

	case 0x81:
		// WARNING: Upon factory reset the unit will not communicated with the LoRaWAN anymore
		// until it is re-installed using the magnet.
		// If a factory reset is requested, uncomment next line
//		DeviceUserDataSetFlag(FLAG_INSTALLED, 0);
		SystemReboot();
		// This code will never return
		break;
	case 0x83:
		{
			// I don't know the expected size of this information nor the endian type
			// As an example I assume it's a long as the data is transfered in seconds
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
				LocalMessage.Port = LORAWAN_APP_PORT;
				LocalMessage.Request = MCPS_UNCONFIRMED;
				LocalMessage.Message->MessageType = 0x84;
				LocalMessage.Message->PayloadLen = sizeof(unsigned long);
				*((unsigned long*)(msg->Payload)) = SUPERVISOR_GetRFPeriod();
				rc = true;
			}
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


