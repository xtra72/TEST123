/*
 * deviceApp.h
 */

#ifndef INC_DEVICEAPP_H_
#define INC_DEVICEAPP_H_
#include "lorawan_task.h"

/** @cond */
// External definition of internally used buffers
extern LORA_PACKET LocalMessage;
extern uint8_t LocalBuffer[255];
/** @endcond */

/*!
 * @brief Set this macro to 1 to enable compliance test support
 */
#define INCLUDE_COMPLIANCE_TEST			( 1 )
/*!
 * @brief Send Periodic message whilst in compliance test mode
 * @return true if in compliance test mode
 */
bool Compliance_SendPeriodic(void);
/*!
 * @brief Execute compliance test command
 * @param mcpsIndication Received LoRaWAN packet to proceed
 */
void DEVICEAPP_RunComplianceTest(McpsIndication_t* mcpsIndication);
/*!
 * @brief Execute compliance test Mlme test
 * @param m MlmeConfirm information received from LoRaWAN network
 * @return true if in compliance test
 */
bool Compliance_ParseMlme(MlmeConfirm_t *m);

/*!
 * @brief Send an up link message to the LoRaWAN network at every RF period
 * @param retry Set if last historical value shall be sent
 */
void DEVICEAPP_SendPeriodic(bool retry);
void SKTAPP_SendPeriodic(bool retry);
/*!
 * @brief Decode a down link message received from the LoRaWAN network
 * @param[in] McpsIndication pointer to a McpsIndication_t structure containing the last indication
 * event information
 */
void DEVICEAPP_ParseMessage(McpsIndication_t* McpsIndication);
void SKTAPP_ParseMessage(McpsIndication_t* McpsIndication);
/*!
 * @brief Decode a Mlme link confirm message received from the LoRaWAN network or LoRa MAC
 * @param[in] MlmeConfirm pointer to a MlmeConfirm_t structure containing the last Mlme confirm
 * event information
 */
void DEVICEAPP_ParseMlme(MlmeConfirm_t *MlmeConfirm);
void SKTAPP_ParseMlme(MlmeConfirm_t *MlmeConfirm);

#endif /* INC_DEVICEAPP_H_ */
