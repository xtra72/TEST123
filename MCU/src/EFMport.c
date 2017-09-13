/*
    FreeRTOS V7.3.0 - Copyright (C) 2012 Real Time Engineers Ltd.

    FEATURES AND PORTS ARE ADDED TO FREERTOS ALL THE TIME.  PLEASE VISIT
    http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?"                                     *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************


    http://www.FreeRTOS.org - Documentation, training, latest versions, license
    and contact details.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool.

    Real Time Engineers ltd license FreeRTOS to High Integrity Systems, who sell
    the code with commercial support, indemnification, and middleware, under
    the OpenRTOS brand: http://www.OpenRTOS.com.  High Integrity Systems also
    provide a safety engineered and independently SIL3 certified version under
    the SafeRTOS brand: http://www.SafeRTOS.com.
*/
/** \addtogroup RTOS FreeRTOS Implementation
 *  @{
 */

/*-----------------------------------------------------------
 * Implementation of functions defined in portable.h for the ARM CM3 port.
 *----------------------------------------------------------*/

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "em_cmu.h"
#include "em_rtcc.h"
#include "EFMEnergy.h"


#if configMAX_SYSCALL_INTERRUPT_PRIORITY == 0
	#error configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to 0.  See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
#endif

#ifndef configSYSTICK_CLOCK_HZ
	#define configSYSTICK_CLOCK_HZ configCPU_CLOCK_HZ
#endif

/* Constants required to manipulate the core.  Registers first... */
#ifndef portNVIC_INT_CTRL_REG
#define portNVIC_INT_CTRL_REG				( * ( ( volatile unsigned long * ) 0xe000ed04 ) )
#undef configSUPPORT_STATIC_ALLOCATION
#else
#define vTaskIncrementTick xTaskIncrementTick
#endif
#define portNVIC_SYSPRI2_REG				( * ( ( volatile unsigned long * ) 0xe000ed20 ) )
/* ...then bits in the registers. */
#define portNVIC_PENDSVSET_BIT				( 1UL << 28UL )
#define portNVIC_PENDSVCLEAR_BIT 			( 1UL << 27UL )

#define portNVIC_PENDSV_PRI			( ( ( unsigned long ) configKERNEL_INTERRUPT_PRIORITY ) << 16 )

/* Constants required to set up the initial stack. */
#define portINITIAL_XPSR			( 0x01000000 )

/* For backward compatibility, ensure configKERNEL_INTERRUPT_PRIORITY is
defined.  The value 255 should also ensure backward compatibility.
FreeRTOS.org versions prior to V4.3.0 did not include this definition. */
#ifndef configKERNEL_INTERRUPT_PRIORITY
	#define configKERNEL_INTERRUPT_PRIORITY 0
#endif

#define xPortSysTickHandler     RTCC_IRQHandler

/*
 * Setup the timer to generate the tick interrupts.  The implementation in this
 * file is weak to allow application writers to change the timer used to
 * generate the tick interrupt.
 */
void vPortSetupTimerInterrupt( void );

/*
 * Exception handlers.
 */
void xPortSysTickHandler( void );

#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
/*!
 * Provide static memory allocation to FreeRTOS Idle task
 * @param [OUT] ppxIdleTaskTCBBuffer	Static allocated TCB struct buffer
 * @param [OUT] ppxIdleTaskStackBuffer	Static allocated Stack array
 * @param [OUT] pulIdleTaskStackSize	Size of allocated Stack array
 */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
/*!
 * Provide static memory allocation to FreeRTOS timer task
 * @param [OUT] ppxTimerTaskTCBBuffer		Static allocated TCB struct buffer
 * @param [OUT] ppxTimerTaskStackBuffer	Static allocated Stack array
 * @param [OUT] pulTimerTaskStackSize		Size of allocated Stack array
 */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
    task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

#endif

/*-----------------------------------------------------------*/

/*
 * The number of SysTick increments that make up one tick period.
 */

#if configUSE_TICKLESS_IDLE == 1
	#define ONE_TICK	( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ )
	#define xMaximumPossibleSuppressedTicks ((0xffffffffUL / ONE_TICK) - 1)
	#define GET_TICK_CNT()			((RTCC_CounterGet() + 1) / ONE_TICK)
	#define SET_TICK_COMP0(n)		RTCC_ChannelCCVSet(1,(n)-1); /* Remove 1 to COMP0 since RTC->CNT starts from 0 */
	#define GET_TICK_COMP0()		(RTCC_ChannelCCVGet(1) + 1)
#endif /* configUSE_TICKLESS_IDLE */

	/* This exists purely to allow the const to be used from within the
	port_asm.asm assembly file. */
	const unsigned long ulMaxSyscallInterruptPriorityConst = configMAX_SYSCALL_INTERRUPT_PRIORITY;

/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/
/* weak definition of RTC Irq hook to be used by external    */
/* programs to maintain Real Time information                */
/*-----------------------------------------------------------*/
__attribute__((weak))  void INTRTC_IRQHandler(unsigned long n) {
}

#if configUSE_TICKLESS_IDLE == 1
static volatile int bSysTickIRQDone;	/* Marker to show whether the RTC Irq was already fired */
#endif
__attribute__((interrupt)) void xPortSysTickHandler( void )
{
	INTRTC_IRQHandler((unsigned long)GET_TICK_COMP0());	/* Call external RTC to update real time clock information */
	RTCC->IFC = _RTCC_IFC_MASK; //RTCC_IFC_CC1;
	if (xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED) {

		/* If using preemption, also force a context switch. */
		#if configUSE_PREEMPTION == 1
			portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
		#endif

		/* Only reset the systick load register if configUSE_TICKLESS_IDLE is set to
		1.  If it is set to 0 tickless idle is not being used.  If it is set to a
		value other than 0 or 1 then a timer other than the SysTick is being used
		to generate the tick interrupt. */
		#if configUSE_TICKLESS_IDLE == 1
			if (GET_TICK_COMP0() != ONE_TICK)
				SET_TICK_COMP0(ONE_TICK);
		#endif
		bSysTickIRQDone = 1;            /* Set RTC Irq fired Marker */
		( void ) portSET_INTERRUPT_MASK_FROM_ISR();
		{
			xTaskIncrementTick();
		}
		portCLEAR_INTERRUPT_MASK_FROM_ISR( 0 );
	}
}
/*-----------------------------------------------------------*/

	void vPortSuppressTicksAndSleep( portTickType xExpectedIdleTime )
	{
#if configUSE_TICKLESS_IDLE == 1
	unsigned long ulCompleteTickPeriods, ElapsedTicks;
		/* If a context switch is pending then abandon the low power entry as
		the context switch might have been pended by an external interrupt that
		requires processing. */
		if( ( portNVIC_INT_CTRL_REG & portNVIC_PENDSVSET_BIT ) != 0 ) return;
		bSysTickIRQDone = 0;            /* Reset RTC Irq Marker*/
		/* Calculate elapsed ticks so far */
		ElapsedTicks = GET_TICK_CNT();

        /* Make sure the SysTick reload value does not overflow the counter. */
		if( xExpectedIdleTime > (xMaximumPossibleSuppressedTicks - ElapsedTicks))
		{
			xExpectedIdleTime = (xMaximumPossibleSuppressedTicks - ElapsedTicks);
		}
		if (xExpectedIdleTime < 5) return; 	/* Don't do all this for so short time */
		if (bSysTickIRQDone) return;		/* Tick Irq occurred in the meantime, just abort */
		/* Calculate the reload value required to wait xExpectedIdleTime
		tick periods. Add elapsed Ticks as RTC value cannot be reset on Gecko */
		SET_TICK_COMP0( ONE_TICK * ( ElapsedTicks + xExpectedIdleTime /*- 1UL*/));

		vTaskSuspendAll();	// Prevent task schedule from running until we're back
		/* Sleep until something happens. */
		configPRE_SLEEP_PROCESSING( xExpectedIdleTime );
//		if( xExpectedIdleTime > 0 ) __WFI();
		configPOST_SLEEP_PROCESSING( xExpectedIdleTime );

		if ( bSysTickIRQDone) /* Test RTC Irq Marker */
		{
				/* The tick interrupt handler will already have pended the tick
				processing in the kernel.  As the pending tick will be
				processed as soon as this function exits, the tick value
				maintained by the tick is stepped forward by one less than the
				time spent waiting. */
				ulCompleteTickPeriods = xExpectedIdleTime - 1UL;
		}
		else
		{
				/* Something other than the tick interrupt ended the sleep.
				Work out how long the sleep lasted. = RTC since it's an upward counter */
				/* How many complete tick periods passed while the processor
				was waiting? */
				ulCompleteTickPeriods = GET_TICK_CNT();

				/* The reload value is set to whatever fraction of a single tick
				period remains. Plus the current value of RTC as we cannot reset the CNT register on Gecko */
				/* Unless the tick IRQ already occurred */
				if (!bSysTickIRQDone)
					SET_TICK_COMP0( ( ulCompleteTickPeriods + 1 ) * ONE_TICK );
				ulCompleteTickPeriods -= ElapsedTicks;
		}

		vTaskStepTick(ulCompleteTickPeriods);
		xTaskResumeAll();	// Restore Scheduler
		// Any task switch shall occur within next regular Tick increment
#endif /* #if configUSE_TICKLESS_IDLE */
	}

/*-----------------------------------------------------------*/

/*
 * Setup the systick timer to generate the tick interrupts at the required
 * frequency.
 */
void vPortSetupTimerInterrupt( void )
{
	CMU_ClockSelectSet(cmuClock_HFLE,(CMU->STATUS & CMU_STATUS_LFXORDY) ? cmuSelect_LFXO : cmuSelect_LFRCO);
	CMU_ClockEnable(cmuClock_HFLE,true);
	CMU_ClockSelectSet(cmuClock_RTCC,(CMU->STATUS & CMU_STATUS_LFXORDY) ? cmuSelect_LFXO : cmuSelect_LFRCO);
	CMU_ClockEnable(cmuClock_LFE, true);	// Use LF for RTCC
	CMU_ClockEnable(cmuClock_RTCC, true);	// Start RTCC
	RTCC_Reset();
	RTCC_CCChConf_TypeDef RtccChannelInit = RTCC_CH_INIT_COMPARE_DEFAULT;
	RTCC_ChannelInit(1,&RtccChannelInit);
	NVIC_ClearPendingIRQ(RTCC_IRQn);
	NVIC_SetPriority (RTCC_IRQn, 1); 	/* Make sure to set priority lowest system priority */
	/* Enable interrupt*/
	RTCC_IntEnable(RTCC_IEN_CC1);
	NVIC_EnableIRQ(RTCC_IRQn);
    SET_TICK_COMP0(ONE_TICK);
	RTCC_Init_TypeDef RtccInit = RTCC_INIT_DEFAULT;
//	RtccInit.presc = rtccCntPresc_8;	// Any prescaler would fail => processor bug
	RtccInit.presc = rtccCntPresc_1;
	RtccInit.cntWrapOnCCV1 = true;
	RTCC_Init(&RtccInit);
}
/*-----------------------------------------------------------*/
/**  }@
 */

