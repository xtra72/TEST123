/**
 * rtc-board.c
 *
 * @date 01 june 2017
 * @author omogenot
 */
/** \addtogroup LW LoRaWAN Implementation
 *  @{
 */

#include "board.h"
#include "rtc-board.h"
#include "global.h"
#include "mmi_timer.h"

static uint32_t LastSetTime = 0;	// To keep track of last Timer start tick time

void RtcInit( void )
{
	TIMERInit();
}

void RtcSetTimeout( uint32_t timeout )
{
	LastSetTime = RtcGetTimerValue();		// Store tick counts for later use
	TIMERStart(timeout);
}

TimerTime_t RtcGetAdjustedTimeoutValue( uint32_t timeout )
{
    return (timeout > 1) ? (timeout - 1) : timeout;
}

TimerTime_t RtcGetTimerValue( void )
{
// FreeRTOS Tick count CANNOT be used as it is possibly not yet updated upon sleep mode exit
/*
//	register uint32_t timer = xTaskGetTickCount();
	// Compute the number of seconds in milliseconds
	register uint32_t value = (timer / 1024) * 1000;
	// Add remaining milliseconds
	value += ((timer % 1024) * 1000) / 1024;
	return value;
*/
// Use System Ticks instead as it's implementing free run timer
// That is not stopped during low power mode
	return SystemGetSystemTicks();
}

TimerTime_t RtcGetElapsedAlarmTime( void )
{
/* This function is equivalent to RtcComputeElapsedTime with LastSetTime as a reference */
/*
    TimerTime_t currentTime = xTaskGetTickCount();

    if( currentTime < LastSetTime )
    {
        return(((currentTime + ( 0xFFFFFFFF - LastSetTime ) ) * 1000) / configTICK_RATE_HZ);
    }
    else
    {
        return((( currentTime - LastSetTime ) * 1000) / configTICK_RATE_HZ);
    }
*/
	return RtcComputeElapsedTime( LastSetTime );
}

TimerTime_t RtcComputeFutureEventTime( TimerTime_t futureEventInTime )
{
    return( RtcGetTimerValue( ) + futureEventInTime );
}

TimerTime_t RtcComputeElapsedTime( TimerTime_t eventInTime )
{
    // Needed at boot, cannot compute with 0 or elapsed time will be equal to current time
    if( eventInTime == 0 )
    {
        return 0;
    }

    TimerTime_t elapsedTime = RtcGetTimerValue();

    if( elapsedTime < eventInTime )
    { // roll over of the counter
        return( elapsedTime + ( 0xFFFFFFFF - eventInTime ));
    }
    else
    {
        return( elapsedTime - eventInTime);
    }
}

/** }@ */
