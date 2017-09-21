/*
 * SKTApp.h
 *
 *  Created on: 2017. 9. 15.
 *      Author: inhyuncho
 */

#ifndef INC_SKTAPP_H_
#define INC_SKTAPP_H_

#define	MSG_SKT_NET_REAL_APP_KEY_ALLOC_REQ		0x00
#define	MSG_SKT_NET_REAL_APP_KEY_ALLOC_ANS		0x01
#define	MSG_SKT_NET_REAL_APP_KEY_RX_REPORT_REQ	0x02
#define	MSG_SKT_NET_REAL_APP_KEY_RX_REPORT_ANS	0x03
#define MSG_SKT_NET_CONFIRMED_UP_NB_RETRANS		0x04

#define	MSG_SKT_DEV_EXT_DEVICE_MANAGEMENT		0x00
#define MSG_SKT_DEV_RESET						0x80
#define MSG_SKT_DEV_SET_UPLINK_DATA_INTERVAL	0x81
#define MSG_SKT_DEV_UPLINK_DATA_REQ				0x82


typedef	enum
{
	SKTAPP_STATUS_IDLE,
	SKTAPP_STATUS_JOIN,
	SKTAPP_STATUS_PSEUDO_JOIN,
	SKTAPP_STATUS_PSEUDO_JOIN_COMPLETED,
	SKTAPP_STATUS_REQ_REAL_APP_KEY_ALLOC,
	SKTAPP_STATUS_REQ_REAL_APP_KEY_ALLOC_COMPLETED,
	SKTAPP_STATUS_REQ_REAL_APP_KEY_RX_REPORT,
	SKTAPP_STATUS_REQ_REAL_APP_KEY_RX_REPORT_COMPLETED,
	SKTAPP_STATUS_REAL_JOIN,
	SKTAPP_STATUS_REAL_JOIN_COMPLETED
} SKTAppStatus_t;

/*!
 * @brief Send an up link message to the LoRaWAN network at every RF period
 * @param retry Set if last historical value shall be sent
 */
void SKTAPP_SendPeriodic(bool retry);
/*!
 * @brief Decode a down link message received from the LoRaWAN network
 * @param[in] McpsIndication pointer to a McpsIndication_t structure containing the last indication
 * event information
 */
bool SKTAPP_ParseMessage(McpsIndication_t* McpsIndication);
/*!
 * @brief Decode a Mlme link confirm message received from the LoRaWAN network or LoRa MAC
 * @param[in] MlmeConfirm pointer to a MlmeConfirm_t structure containing the last Mlme confirm
 * event information
 */
void SKTAPP_ParseMlme(MlmeConfirm_t *MlmeConfirm);


bool	SKTAPP_Init(void);

bool SKTAPP_SendRealAppKeyAllocReq(void);
bool SKTAPP_SendRealAppKeyRxReportReq(void);

bool SKTAPP_GetPeriodicMode(void);
bool SKTAPP_SetPeriodicMode(bool bEnable);

bool SKTAPP_SetConfirmedMsgType(bool bConfirmed);
bool SKTAPP_IsConfirmedMsgType(void);

bool SKTAPP_Send(uint8_t port, uint8_t messageType, uint8_t *pFrame, uint32_t ulFrameLen);
bool SKTAPP_SendAck(void);
bool SKTAPP_LinkCheck(void);

#endif /* INC_SKTAPP_H_ */
