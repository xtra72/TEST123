/*
 * compliance.c
 *
 */
#include "global.h"
#include "deviceApp.h"
#include "supervisor.h"

#if (INCLUDE_COMPLIANCE_TEST > 0)

/*!
 * LoRaWAN compliance tests support data
 */
struct ComplianceTest_s
{
    bool Running;
    uint8_t State;
    bool IsTxConfirmed;
    uint8_t AppPort;
    uint8_t AppDataSize;
    uint8_t *AppDataBuffer;
    bool LinkCheck;
    uint8_t DemodMargin;
    uint8_t NbGateways;
}ComplianceTest;

static uint8_t oldPeriod;

void DEVICEAPP_RunComplianceTest(McpsIndication_t* mcpsIndication) {
	LocalMessage.Buffer = LocalBuffer;
	// Compliance test not started yet
	if( ComplianceTest.Running == false )
	{
		// Check compliance test enable command (i)
		if( ( mcpsIndication->BufferSize == 4 ) &&
			( mcpsIndication->Buffer[0] == 0x01 ) &&
			( mcpsIndication->Buffer[1] == 0x01 ) &&
			( mcpsIndication->Buffer[2] == 0x01 ) &&
			( mcpsIndication->Buffer[3] == 0x01 ) )
		{
			LocalMessage.Request = MCPS_UNCONFIRMED;
			LORAWAN_ResetDownLinkCounter();
			oldPeriod = SUPERVISORGetRFPeriod();
			SUPERVISORStartCyclicTask(0,0);
			ComplianceTest.LinkCheck = false;
			ComplianceTest.DemodMargin = 0;
			ComplianceTest.NbGateways = 0;
			ComplianceTest.Running = true;
			ComplianceTest.State = 1;

			MibRequestConfirm_t mibReq;
			mibReq.Type = MIB_ADR;
			mibReq.Param.AdrEnable = true;
			LoRaMacMibSetRequestConfirm( &mibReq );

#if defined( USE_BAND_868 )
			LoRaMacTestSetDutyCycleOn( false );
#endif
		}
	}
	if( ComplianceTest.Running == true )
	{
		LocalMessage.Port = 224;
		if( ComplianceTest.LinkCheck == true )
		{
			ComplianceTest.LinkCheck = false;
			LocalMessage.Size = 3;
			LocalMessage.Buffer[0] = 5;
			LocalMessage.Buffer[1] = ComplianceTest.DemodMargin;
			LocalMessage.Buffer[2] = ComplianceTest.NbGateways;
			ComplianceTest.State = 1;
		}
		else if (mcpsIndication) {
			ComplianceTest.State = mcpsIndication->Buffer[0];
			switch( ComplianceTest.State )
			{
			case 0: // Check compliance test disable command (ii)
				ComplianceTest.Running = false;
				LocalMessage.Size = 0;
				MibRequestConfirm_t mibReq;
				mibReq.Type = MIB_ADR;
				mibReq.Param.AdrEnable = LORAWAN_ADR_ON;
				LoRaMacMibSetRequestConfirm( &mibReq );
	#if defined( USE_BAND_868 )
				LoRaMacTestSetDutyCycleOn( LORAWAN_DUTYCYCLE_ON );
	#endif
				SUPERVISORStartCyclicTask(0,oldPeriod);
				break;
			case 1: // (iii, iv)
			case 2: // Enable confirmed messages (v)
			case 3:  // Disable confirmed messages (vi)
				if (ComplianceTest.State == 2) LocalMessage.Request = MCPS_CONFIRMED;
				if (ComplianceTest.State == 3) LocalMessage.Request = MCPS_UNCONFIRMED;
				LocalMessage.Size = 2;
				LocalMessage.Buffer[0] = LORAWAN_GetDownLinkCounter() >> 8;
				LocalMessage.Buffer[1] = LORAWAN_GetDownLinkCounter();
				ComplianceTest.State = 1;
				break;
			case 4: // (vii)
				LocalMessage.Size = mcpsIndication->BufferSize;
				LocalMessage.Buffer[0] = 4;
				for( uint8_t i = 1; i < LocalMessage.Size; i++ )
					LocalMessage.Buffer[i] = mcpsIndication->Buffer[i] + 1;
				ComplianceTest.State = 1;
				break;
			case 5: // (viii)
				{
					MlmeReq_t mlmeReq;
					mlmeReq.Type = MLME_LINK_CHECK;
					LoRaMacMlmeRequest( &mlmeReq );
				}
				break;
			case 6: // (ix)
				{
					// Disable TestMode and revert back to normal operation
					ComplianceTest.Running = false;
					LocalMessage.Size = 0;
					MibRequestConfirm_t mibReq;
					mibReq.Type = MIB_ADR;
					mibReq.Param.AdrEnable = LORAWAN_ADR_ON;
					LoRaMacMibSetRequestConfirm( &mibReq );
	#if defined( USE_BAND_868 )
					LoRaMacTestSetDutyCycleOn( LORAWAN_DUTYCYCLE_ON );
	#endif
					SUPERVISORStartCyclicTask(0,oldPeriod);
					LORAWAN_JoinNetwork();
				}
				break;
			case 7: // (x)
				{
					if( mcpsIndication->BufferSize == 3 )
					{
						MlmeReq_t mlmeReq;
						mlmeReq.Type = MLME_TXCW;
						mlmeReq.Req.TxCw.Timeout = ( uint16_t )( ( mcpsIndication->Buffer[1] << 8 ) | mcpsIndication->Buffer[2] );
						LoRaMacMlmeRequest( &mlmeReq );
					}
					else if( mcpsIndication->BufferSize == 7 )
					{
						MlmeReq_t mlmeReq;
						mlmeReq.Type = MLME_TXCW_1;
						mlmeReq.Req.TxCw.Timeout = ( uint16_t )( ( mcpsIndication->Buffer[1] << 8 ) | mcpsIndication->Buffer[2] );
						mlmeReq.Req.TxCw.Frequency = ( uint32_t )( ( mcpsIndication->Buffer[3] << 16 ) | ( mcpsIndication->Buffer[4] << 8 ) | mcpsIndication->Buffer[5] ) * 100;
						mlmeReq.Req.TxCw.Power = mcpsIndication->Buffer[6];
						LoRaMacMlmeRequest( &mlmeReq );
					}
					ComplianceTest.State = 1;
				}
				break;
			default:
				break;
			}
		}
	}
}

bool Compliance_SendPeriodic(void) {
	if( ComplianceTest.Running == true ) {
		LocalMessage.Port = 224;
		if (LocalMessage.Request == MCPS_CONFIRMED)
			LocalMessage.NbTrials = LORAWAN_RETRIES;
		LORAWAN_SendMessage(&LocalMessage);
		if (ComplianceTest.State == 1) {
			LocalMessage.Size = 2;
			LocalMessage.Buffer[0] = LORAWAN_GetDownLinkCounter() >> 8;
			LocalMessage.Buffer[1] = LORAWAN_GetDownLinkCounter();
			return true;
		}
	}
	return false;
}

bool Compliance_ParseMlme(MlmeConfirm_t *m) {
	if( ComplianceTest.Running == true )
	{
		if (m) {
			if (m->Status == LORAMAC_EVENT_INFO_STATUS_OK) {
				if (m->MlmeRequest == MLME_LINK_CHECK) {
						ComplianceTest.LinkCheck = true;
						ComplianceTest.DemodMargin = m->DemodMargin;
						ComplianceTest.NbGateways = m->NbGateways;
						// Force message content update
						DEVICEAPP_RunComplianceTest(NULL);
				}
			}
		}
		return true;
	}
	return false;
}


#endif


