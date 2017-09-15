/*
 * SKTApp.h
 *
 *  Created on: 2017. 9. 15.
 *      Author: inhyuncho
 */

#ifndef INC_SKTAPP_H_
#define INC_SKTAPP_H_

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
void SKTAPP_ParseMessage(McpsIndication_t* McpsIndication);
/*!
 * @brief Decode a Mlme link confirm message received from the LoRaWAN network or LoRa MAC
 * @param[in] MlmeConfirm pointer to a MlmeConfirm_t structure containing the last Mlme confirm
 * event information
 */
void SKTAPP_ParseMlme(MlmeConfirm_t *MlmeConfirm);


bool SKTAPP_SendRealAppKeyAllocReq(void);
bool SKTAPP_SendRealAppKeyRxReportReq(void);

#endif /* INC_SKTAPP_H_ */
