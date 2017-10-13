/*
 * DaliworksApp.h
 *
 *  Created on: 2017. 9. 15.
 *      Author: inhyuncho
 */

#ifndef INC_DALIWORKSAPP_H_
#define INC_DALIWORKSAPP_H_

#include <stdbool.h>

#define MSG_DALIWORKS_RESET						0x81
#define MSG_DALIWORKS_UPLINK_DATA_REQ			0x83

bool DEVICEAPP_ExecDaliworks(LORA_MESSAGE* msg);

#endif /* SRC_DALIWORKSAPP_H_ */
