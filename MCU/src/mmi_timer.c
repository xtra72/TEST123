/*
 * mmi_timer.c
 *
 */

#include "mmi_timer.h"
#include <em_cmu.h>
#include <em_letimer.h>
/** \addtogroup MMI MyMeterInfo add-on functions
 *  @{
 */

static const LETIMER_Init_TypeDef  LETIMER_INIT =
{
  false,               /* Enable timer when init complete. */
  false,               /* Stop counter during debug halt. */
  false,               /* Do not load COMP0 into CNT on underflow. */
  false,               /* Do not load COMP1 into COMP0 when REP0 reaches 0. */
  0,                   /* Idle value 0 for output 0. */
  0,                   /* Idle value 0 for output 1. */
  letimerUFOANone,     /* No action on underflow on output 0. */
  letimerUFOANone,     /* No action on underflow on output 1. */
  letimerRepeatOneshot /* Count once and stop. */
};

/*!
 * @brief System Low Energy Timer 0 IRQ handler
 */
__attribute__((interrupt)) __attribute__((used)) void LETIMER0_IRQHandler(void) {
	LETIMER0->IFC = _LETIMER_IFC_MASK;
	TimerIrqHandler();
}

__weak void TimerIrqHandler(void) { }

void TIMERInit(void) {
	// Initialize LETIMER
    CMU_ClockEnable(cmuClock_HFLE,true);
    CMU_ClockEnable(cmuClock_LETIMER0, true);
    CMU_ClockDivSet(cmuClock_LETIMER0,cmuClkDiv_32); // 32 / 32768 = 1 / 1024 = 0.976 ms period
    LETIMER_IntClear(LETIMER0,_LETIMER_IFC_MASK);
    SystemIRQEnable(LETIMER0_IRQn);
    LETIMER_IntEnable(LETIMER0,LETIMER_IEN_REP0);
    LETIMER_Init(LETIMER0,&LETIMER_INIT);
}

void TIMERStart(int duration) {
	// As letimer.c has a bug that ignores register sync
	// we use our own implementation.
	// Make sure we can write in the CMD register
	while(LETIMER0->SYNCBUSY & LETIMER_SYNCBUSY_CMD) __nop();
	LETIMER0->CMD = LETIMER_CMD_STOP | LETIMER_CMD_CLEAR;
	// Make sure STOP CMD has been processed before changing any other register
	while(LETIMER0->SYNCBUSY & LETIMER_SYNCBUSY_CMD) __nop();
	// Cancel any pending IRQ
    LETIMER0->IFC = _LETIMER_IFC_MASK;
    // Compute duration in hardware timer increments
	duration = ((duration * 1024) / 1000);
	if (duration > 1) duration--; // Remove one as we decrement down to 0
	// Store computed duration
	// CNT is a 16 bits counter
	LETIMER0->CNT = (duration & 0xFFFF) ? (duration & 0xFFFF) : 1;
	// REP0 is an 8 bits counter
	duration >>= 16;
	duration &= 0x0FF;
	// Check for rollover
	if (duration == 0xFF) duration--;
	LETIMER0->REP0 = (duration) ? (1 + duration) : 1;
	LETIMER0->CMD = LETIMER_CMD_START;		// Start timer
}

void TIMERStop(void) {
	// As letimer.c has a bug that ignores register sync
	// we use our own implementation.
	// Make sure we can write in the CMD register
	while(LETIMER0->SYNCBUSY & LETIMER_SYNCBUSY_CMD) __nop();
	LETIMER0->CMD = LETIMER_CMD_STOP;
}

/** }@ */
