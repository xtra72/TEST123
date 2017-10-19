/*
 * event.c
 *
 *  Created on: 2017. 10. 19.
 *      Author: inhyuncho
 */

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

/*
 * Device Event Handling
 */

static xQueueHandle xEventQueue = 0;
static StaticQueue_t xStaticEventQueue;
static uint8_t ucEventQueueStorageArea[ 16 * sizeof(uint32_t) ];

void EVENT_Init(void)
{
	xEventQueue = xQueueCreateStatic(16,sizeof(uint32_t), ucEventQueueStorageArea, &xStaticEventQueue);
}

uint32_t EVENT_WaitForEvent(portTickType duration)
{
	uint32_t	evt;
	if (xEventQueue)
	{
		if (xQueueReceive(xEventQueue,&evt,duration) == pdTRUE)
		{
			return evt;
		}
	}
	return 0;
}
void EVENT_Send(uint32_t event)
{
	if (xEventQueue)
	{
		xQueueSendToBack(xEventQueue,&event,0);
	}
}
void EVENT_Post(uint32_t event)
{
	if (xEventQueue)
	{
		uint32_t e;
		// Ignore already waiting event (avoid redondency)
		if ((xQueuePeek(xEventQueue,&e,0) == pdTRUE) && (e == event)) return;
		xQueueSendToFront(xEventQueue,&event,0);
	}
}

void EVENT_PostFromISR(uint32_t event)
{
	if (xEventQueue)
	{
		uint32_t e;
		// Ignore already waiting event (avoid redondency)
		if ((xQueuePeekFromISR(xEventQueue,&e) == pdTRUE) && (e == event)) return;
		xQueueSendToFrontFromISR(xEventQueue,&event,NULL);
	}
}

signed portBASE_TYPE EVENT_SendFromISR(uint32_t event)
{
	signed portBASE_TYPE sHigherPriorityTaskWoken = pdFALSE;
	if (xEventQueue)
	{
		xQueueSendToBackFromISR(xEventQueue,&event,&sHigherPriorityTaskWoken);
	}
	return sHigherPriorityTaskWoken;
}
