/*
 * lorawan_task.h
 *
 */

#ifndef INC_LORAWAN_TASK_H_
#define INC_LORAWAN_TASK_H_
#include "LoRaMac.h"
#include "Region.h"

/** \addtogroup S40 S40 Main Application
 *  @{
 */

/*!
 * Default datarate
 */
#define LORAWAN_DEFAULT_DATARATE                    DR_0

/*!
 * LoRaWAN confirmed messages
 */
#define LORAWAN_CONFIRMED_MSG_ON                    false

/*!
 * LoRaWAN Adaptive Data Rate
 *
 * \remark Please note that when ADR is enabled the end-device should be static
 */
#define LORAWAN_ADR_ON                              1

#include "LoRaMacTest.h"

#define USE_SEMTECH_DEFAULT_CHANNEL_LINEUP          1

#if( USE_SEMTECH_DEFAULT_CHANNEL_LINEUP == 1 )

#define LC4                { 867100000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC5                { 867300000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC6                { 867500000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC7                { 867700000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC8                { 867900000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC9                { 868800000, 0, { ( ( DR_7 << 4 ) | DR_7 ) }, 2 }
#define LC10               { 868300000, 0, { ( ( DR_6 << 4 ) | DR_6 ) }, 1 }

#endif

typedef	enum
{
	LORAWAN_STATUS_IDLE,
	LORAWAN_STATUS_JOIN,
	LORAWAN_STATUS_PSEUDO_JOIN,
	LORAWAN_STATUS_PSEUDO_JOIN_CONFIRMED,
	LORAWAN_STATUS_REQ_REAL_APP_KEY_ALLOC,
	LORAWAN_STATUS_REQ_REAL_APP_KEY_ALLOC_CONFIRMED,
	LORAWAN_STATUS_REQ_REAL_APP_KEY_RX_REPORT,
	LORAWAN_STATUS_REQ_REAL_APP_KEY_RX_REPORT_CONFIRMED,
	LORAWAN_STATUS_REAL_JOIN,
	LORAWAN_STATUS_REAL_JOIN_CONFIRMED
} LoRaWANStatus_t;

/*!
 * @brief LoRaWAN default application port (Daliworks requirement)
 */
#define LORAWAN_APP_PORT                            1
/*!
 * @brief Device/Network interworking application message requirements (SKT requirement)
 */
#define SKT_NETWORK_SERVICE_PORT					0xDF

#define	MSG_REAL_APP_KEY_ALLOC_REQ					0x00
#define	MSG_REAL_APP_KEY_RX_REPORT_REQ				0x02

/*!
 * @brief Device management application message requirement (SKT requirement)
 */
#define SKT_DEVICE_SERVICE_PORT						0xDE

/*!
 * @brief Service management application message requirement (Daliworks requirement)
 */
#define DALIWORKS_SERVICE_PORT						0xDD

/*!
 * @brief Default number of retries when confirmed message is sent out LoRaWAN network
 */
#define LORAWAN_RETRIES 							8

/*!
 * @brief Maximum LORAWAN Payload size (SKT requirement is 65)
 */
#define LORAWAN_MAX_MESSAGE_SIZE					65
/*!
 * @brief Default LoRaWAN packet exchange periodicity
 */
#ifdef _DEBUG
#define LORAMAC_DEFAULT_RF_PERIOD					2
#else
#define LORAMAC_DEFAULT_RF_PERIOD					5 // 60
#endif
/*!
 * @brief SKT/Daliworks LoRaWAN buffer abstraction
 */
typedef struct __packed__ {
uint8_t Version;
uint8_t MessageType;
uint8_t PayloadLen;
uint8_t Payload[1];
} LORA_MESSAGE;

/*!
 * @brief Size of default empty LORA_MESSAGE
 */
#define LORA_MESSAGE_HEADER_SIZE					3

/*!
 * @brief Current LoRaWAN Message version to use
 */
#define LORA_MESSAGE_VERSION						0

/*!
 * @brief LORAWAN task message structure definition
 * @remark Uses a GCC extension allowing anonymous union names to access Buffer or Message directly
 */
typedef struct __packed__ {
uint8_t	Port;					//!< LoRaWAN destination port
Mcps_t Request;					//!< LoRaWAN packet type
LoRaMacEventInfoStatus_t Status;//!< LoRaWAN message result code
uint8_t NbTrials;				//!< LoRaWAN number of trials
uint8_t FramePending;			//!< LoRaWAN message frame pending status
uint8_t Size;					//!< LoRaWAN message size
union {
uint8_t *Buffer;				//!< LoRaWAN message pointer
LORA_MESSAGE *Message;
};
} LORA_PACKET;


/*!
 * @brief LORAWAN Task initialization
 */
void LORAWAN_Init(void);

LoRaWANStatus_t LORAWAN_GetStatus(void);
char*			LORAWAN_GetStatusString(void);
/*!
 * @brief Tries to join the LoRaWAN network if not already done
 * @return true if successful
 */
bool LORAWAN_JoinNetwork(bool bWaitForConfirmed);
bool LORAWAN_JoinNetworkUseOTTA(bool bWaitForConfirmed);
bool LORAWAN_JoinNetworkUseABP(bool bWaitForConfirmed);
bool LORAWAN_PseudoJoinNetwork(bool bWaitForConfirmed);
bool LORAWAN_RealJoinNetwork(bool bWaitForConfirmed);
bool LORAWAN_RequestRealAppKeyAlloc(bool bWaitForConfirmed);
bool LORAWAN_RequestRealAppKeyRxReport(bool bWaitForConfirmed);
bool LORAWAN_CancelJoinNetwork(void);

/*!
 * @brief Sends a LORA_MESSAGE to the network
 * @param[in] message
 * @return true if message sent successfully
 */
LoRaMacStatus_t LORAWAN_SendMessage(LORA_PACKET* message);

/*!
 * @brief Check whether the device has joined the LoRaWAN network
 * @return true if network is joined
 */
bool LORAWAN_IsNetworkJoined();

/*!
 * @brief Get a pointer to the last received LoRaWAN message from the network
 * @return a pointer if OK. Null otherwise.
 */
LORA_PACKET* LORAWAN_GetMessage(void);

/*!
 * @brief Get a pointer to the last received Mlme Confirm event from the network
 * @return a pointer if OK. Null otherwise.
 */
MlmeConfirm_t* LORAWAN_GetMlmeConfirm(void);

/*!
 * @brief Get a pointer to the last received Confirm event from the network
 * @return a pointer if OK. Null otherwise.
 */
McpsConfirm_t* LORAWAN_GetConfirm(void);
/*!
 * @brief Get a pointer to the last received Indication event from the network
 * @return a pointer if OK. Null otherwise.
 */
McpsIndication_t* LORAWAN_GetIndication(void);
/*!
 * @brief Return the current LoRaWAN network down link counter
 * @return the number of received down link messages
 * @remark This is needed for compliance test purpose
 */
uint16_t LORAWAN_GetDownLinkCounter(void);
/*!
 * @brief Resets the current LoRaWAN network down link counter
 * @remark This is needed for compliance test purpose
 */
void LORAWAN_ResetDownLinkCounter(void);
/*!
 * @brief Set the maximum number of retries for confirmed up messages
 * @param retries number of retries (from 1 to 8, 0 to use @ref LORAWAN_RETRIES default value)
 */
void LORAWAN_SetMaxRetries(uint8_t retries);
/*!
 * @brief Get the current maximum number of retries for confirmed messages
 * @return the number of retries (from 1 to 8)
 */
uint8_t LORAWAN_GetMaxRetries(void);

/** }@ */

#endif /* INC_LORAWAN_TASK_H_ */
