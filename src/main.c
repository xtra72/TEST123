
/*******************************************************************
**                                                                **
** Program Main entry point                                       **
**                                                                **
*******************************************************************/
/** \addtogroup S40 S40 Main Application
 * @brief S40 module main application files
 *  @{
 */
#include "global.h"
#include "supervisor.h"
#include "lorawan_task.h"
#include "SKTApp.h"
#include "trace.h"
/** @cond */
/* Make sure that we initialize HAL array */
#define DEFINE_HAL
//#define USE_MMIPCB
/** @endcond */

#include "HAL_def.h"
#include <device_impl.h>	/* Define device implementation */

/*!
 * @brief FreeRTOS Idle Task Hook function
 */
void vApplicationIdleHook( void )
{
	/* Use the idle task to place the CPU into a low power mode.  Greater power
	saving could be achieved by not including any demo tasks that never block. */
}

/*!
 * @brief FreeRTOS Task Stack Overflow Hook function
 * This function is called when a task is detected by FreeRTOS as having overflowed its stack
 * (in fact when the stack contains less than 16 bytes left).
 * @param[in] pxTask	The FreeRTOS handle of the faulty task
 * @param[in] pcTaskName	The name of the faulty task
 */
__attribute__((noreturn)) void vApplicationStackOverflowHook( xTaskHandle pxTask, signed char *pcTaskName )
{
	/* This function will be called if a task overflows its stack, if
	configCHECK_FOR_STACK_OVERFLOW != 0.  It might be that the function
	parameters have been corrupted, depending on the severity of the stack
	overflow.  When this is the case pxCurrentTCB can be inspected in the
	debugger to find the offending task. */
#ifdef _DEBUG
		DeviceShowErrorCode(DEVICE_STACK_ERROR);
#endif
		SystemReboot();
		for(;;);
}

/*!
 * Tasks static allocation
 */
/** @cond */
#define SUPER_STACK		500
/** @endcond */
static StackType_t SuperStack[SUPER_STACK];
static StaticTask_t SuperTask;
TaskHandle_t hSuperTask = NULL;

/*!
 * Tasks static allocation
 */
/** @cond */
#define SHELL_STACK		500
/** @endcond */
static StackType_t ShellStack[SHELL_STACK];
static StaticTask_t ShellTask;
TaskHandle_t hShellTask = NULL;

/*!
 * @brief S40 Application starting point
 * Initializes the hardware, allocates tasks and starts FreeRTOS scheduler.
 * @return Never returns
 */
__attribute__((noreturn)) int main()
{
	// Initialize board GPIO and peripherals
	DeviceInitHardware();

	LORAWAN_Init();
	SKTAPP_Init();

 	SHELL_Init();

	/* Create the various tasks */
	/* Create the task that Monitors the system */
	hSuperTask = xTaskCreateStatic( SUPERVISOR_Task, (const char*)"SUPER", SUPER_STACK, NULL, tskIDLE_PRIORITY + 1, SuperStack, &SuperTask );
	hShellTask = xTaskCreateStatic( SHELL_Task, (const char*)"SHELL", SHELL_STACK, NULL, tskIDLE_PRIORITY + 1, ShellStack, &ShellTask );

	/* Start the scheduler. */
	vEFMEnergyEnableLowPowerMode(false);	// Make sure FreeRTOS can go in deep sleep mode
	vTaskStartScheduler();

	/* The scheduler should now be running the tasks so the following code should
	never be reached.  If it is reached then there was insufficient heap space
	for the idle task to be created.  In this case the heap size is set by
	configTOTAL_HEAP_SIZE in FreeRTOSConfig.h. */
	for(;;)
	{
	}
	__builtin_unreachable();
}

void	TaskGetInfo()
{
	TaskStatus_t	xStatus;

	vTaskGetInfo( hShellTask, &xStatus, true, eInvalid);

	SHELL_Printf("%16s : %d\n", "Water Mark", xStatus.usStackHighWaterMark);
}

/** @}*/
