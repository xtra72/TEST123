/*
 * event.h
 *
 *  Created on: 2017. 10. 19.
 *      Author: inhyuncho
 */

#ifndef INC_EVENT_H_
#define INC_EVENT_H_


void EVENT_Init(void);
uint32_t EVENT_WaitForEvent(portTickType duration);
void EVENT_Send(uint32_t event);
void EVENT_Post(uint32_t event);
void EVENT_PostFromISR(uint32_t event);
signed portBASE_TYPE EVENT_SendFromISR(uint32_t event);

#endif /* INC_EVENT_H_ */
