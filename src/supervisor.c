/*******************************************************************
** supervisor.c                                                   **
**                                                                **
*******************************************************************/
/** \addtogroup S40 S40 Main Application
 *  @{
 */

#include "global.h"
#include "supervisor.h"
#include "lorawan_task.h"
#include "deviceApp.h"
#include "SKTApp.h"
#include "system.h"
#include "trace.h"

#undef	__MODULE__
#define	__MODULE__	"Supervisor"

static xTaskHandle xCyclicHandle = 0;
static uint32_t		RFPeriod = SUPERVISOR_CYCLIC_TASK_DEFAULT;
static portTickType CycleStart = 0;
static bool			bStepByStep = false;

void	SUPERVISOR_startReport(void)
{
}

void	SUPERVISOR_stopReport(void)
{
}

bool	SUPERVISOR_setReportInterval(uint32_t ulSecs)
{
	return	true;
}

/**************************** Private functions ********************/
/** @cond */
#define MS_DELAY_MIN		3L		// loose 3 ms per min. (in fact 2.941 ms)
/** @endcond */
__attribute__((noreturn)) static void SUPERVISORCyclicTask(void* pvParameters) {
	(void)pvParameters;
	static portTickType xNextWakeUp = 0;
	portTickType xDelay = configTICK_RATE_HZ;
	if (!CycleStart) CycleStart = xTaskGetTickCount();
	for(;;) {
#if (INCLUDE_COMPLIANCE_TEST > 0)
		if (RFPeriod)
#endif
			xDelay = ((portTickType)(RFPeriod*configTICK_RATE_HZ - (RFPeriod / 60 * 3 * configTICK_RATE_HZ / 1000)));
#if (INCLUDE_COMPLIANCE_TEST > 0)
		else
			xDelay = configTICK_RATE_HZ * 15;	// 15 seconds delay for compliance test
#endif
		// Update Start if needed
		if (CycleStart) {
			if (CycleStart > xNextWakeUp) {
				xNextWakeUp = CycleStart;
#if (DELAY_READ_DURATION > 0)
				/* Delay loop start to allow sensor reading */
				if (xDelay > (DELAY_READ_DURATION * configTICK_RATE_HZ))
					xDelay -= (DELAY_READ_DURATION * configTICK_RATE_HZ);
#endif
			}
			CycleStart = 0;	// Reset start value
		}
		vTaskDelayUntil(&xNextWakeUp, xDelay);
		// Check if we were woke up by timer or cancelled
		if (xNextWakeUp <= xTaskGetTickCount())
		{
			DevicePostEvent(PERIODIC_EVENT);
		}
	}
	__builtin_unreachable();
}

/* Get default periodic delay from User Data FLASH memory region */
#define SUPERVISOR_ReadRFPeriod() (UNIT_RFPERIOD)

static void SUPERVISORUpdatePulseValue(void) {
    CLEAR_FLAG(DEVICE_COMM_ERROR);	/* Reset Communication Status */
#if (NODE_TEMP > 0)
	DeviceCaptureAnalogValue(0);
	if (GET_FLAG(DEVICE_COMM_ERROR)) {
		/* Give another trial */
		CLEAR_FLAG(DEVICE_COMM_ERROR);
		/* Wait some time for sensor to power down */
		SysTimerWait1ms(100);
		DeviceCaptureAnalogValue(0);
	}
#elif (NODE_HYGRO > 0)
	DeviceCaptureAnalogValue(0);
#elif (NODE_ANALOG > 0)
	DeviceCaptureAnalogValue(0);
#else
	DevicePulseInCheck();			// Verify any sensor interface problem
#endif
}

static void SUPERVISORRunAttach()
{
	DevicePostEvent(RUN_ATTACH);
}

static void SUPERVISORResetUnit(void) {
	TRACE("Reset Unit\n");
	if (!UNIT_DISALLOW_RESET) {
		DeviceUserDataSetFlag(FLAG_INSTALLED, 0);
		DeviceFlashLed(20);
		SystemReboot();
	}

	TRACE("Reset disallowed.\n");
	DeviceFlashLed(LED_FLASH_OFF);
}

/*
 * NOTE: Even if no historical data shall be kept (i.e. HISTORICAL_DATA = 0), the last captured
 * values will be stored to allow PERIODIC_RESEND event to send accurate information
 */

#if HISTORICAL_DATA
static unsigned long _historical[HISTORICAL_DATA ][HAL_NB_PULSE_IN];
static int _historical_in = 0;
static bool _historical_overflow = false;
#else
static unsigned long _historical[1][HAL_NB_PULSE_IN];
#endif

static void SUPERVISOR_SaveHistorical(void) {
#if HISTORICAL_DATA
	for (int i=0; i < DeviceGetPulseInNumber(); i++)
		_historical[_historical_in][i] = DeviceGetPulseInValue(i);
	if (++_historical_in >= HISTORICAL_DATA) {
		_historical_in = 0;
		_historical_overflow = true;
	}
#else
	for (int i=0; i < DeviceGetPulseInNumber(); i++)
		_historical[0][i] = DeviceGetPulseInValue(i);
#endif
}

/**************************** PUBLIC Functions *********************/
unsigned short SUPERVISOR_GetHistoricalCount(void) {
#if HISTORICAL_DATA
	return (unsigned short)((_historical_overflow) ? HISTORICAL_DATA : _historical_in);
#else
	return 0;
#endif
}

unsigned long SUPERVISOR_GetHistoricalValue(int rank, int index) {
#if HISTORICAL_DATA
	if (index > DeviceGetPulseInNumber()) return 0;
	int nb = (_historical_overflow) ? HISTORICAL_DATA : _historical_in;
	if (rank >= nb) return 0;
	if (rank >= _historical_in) rank = _historical_in + rank - HISTORICAL_DATA -1;
	else rank = _historical_in - rank - 1;
	return _historical[rank][index];
#else
	if (index > DeviceGetPulseInNumber()) return 0;
	if (rank) return 0;
	return _historical[0][index];
#endif
}

unsigned long SUPERVISOR_GetRFPeriod(void)
{
	return RFPeriod;
}

bool	SUPERVISOR_SetRFPeriod(unsigned long ulRFPeriod)
{
	if ((ulRFPeriod < SUPERVISOR_CYCLIC_TASK_MIN) || (SUPERVISOR_CYCLIC_TASK_MAX < ulRFPeriod))
	{
		return	false;
	}

	RFPeriod = ulRFPeriod;
	// Optionally store permanently in Flash
	DeviceUserDataSetRFPeriod(ulRFPeriod);

	return	true;
}

/** @cond */
#define CYCLE_STACK	40
static StackType_t CycleStack[CYCLE_STACK];
static StaticTask_t CycleTask;
/** @endcond */
void SUPERVISOR_StartCyclicTask(int Start, unsigned long period)
{
	if (xCyclicHandle) vTaskDelete(xCyclicHandle);
#if (INCLUDE_COMPLIANCE_TEST > 0)
	if (period == 0) RFPeriod = 0;	// Used for compliance test
#endif
	// By default we lock up period from 1 to 30*24*60*60 sec. (30 days) for security.
	// if the period is longer that this, it could be long before we can "talk"
	// to the device...
	if ((period >= SUPERVISOR_CYCLIC_TASK_MIN) && (period <=SUPERVISOR_CYCLIC_TASK_MAX))
	{
			RFPeriod = period;
			// Optionally store permanently in Flash
			DeviceUserDataSetRFPeriod(RFPeriod);
	}
	CycleStart = xTaskGetTickCount();
	if (CycleStart < (portTickType)Start)
	{
		CycleStart = (portTickType)Start;
	}

	/* Create new task */
	while ((xCyclicHandle = xTaskCreateStatic( SUPERVISORCyclicTask, (const char*)"PERIODIC", CYCLE_STACK, NULL, tskIDLE_PRIORITY + 3, CycleStack, &CycleTask )) == NULL)
		vTaskDelay(1);
}

void SUPERVISOR_StopCyclicTask(void)
{
	if (xCyclicHandle)
	{
		vTaskDelete(xCyclicHandle);
		xCyclicHandle = 0;
	}
}

bool SUPERVISOR_IsCyclicTaskRun(void)
{
	return	xCyclicHandle != 0;
}

/*
 * This array defines the various functions available depending on the duration
 * of the positioning of a magnet on the magnet detector
 * NOTE: if the duration is less than the first element, then no action is executed
 */
static const BUTTON_TASKLIST buttonTasks[] = {
{3000, 0,SUPERVISORRunAttach },		/* Run Attach after 3000 milliseconds */
{10000,15,SUPERVISORResetUnit},		/* Run ResetUnit after 10000 milliseconds */
{0,100,NULL}						/* End of list */
};

static unsigned long lastButton = 0;
__attribute__((noreturn)) void SUPERVISOR_Task(void* pvParameters)
{
	// Wait 1 sec. for Radio Task to start
	vTaskDelay(configTICK_RATE_HZ);
	if (UNIT_FACTORY_TEST)
	{
		CLEAR_USERFLAG(FLAG_FACTORY_TEST);
		GetPhyParams_t PhyParam;
		PhyParam.Attribute = PHY_CHANNELS;
		SX1276SetTxContinuousWave(RegionGetPhyParam(UNIT_REGION,&PhyParam).Channels[0].Frequency,10,3);
		SystemReboot();
	}

	// Mark device temporary error upon unknown device reset
	DeviceStatus |= (UNIT_INSTALLED) ? DEVICE_COMM_ERROR:  (DEVICE_COMM_ERROR | DEVICE_UNINSTALLED);
	SUPERVISOR_StartCyclicTask(0,SUPERVISOR_ReadRFPeriod());

#if 0
  if (UNIT_INSTALLED)
  {
	  CLEAR_FLAG(DEVICE_UNINSTALLED);
	  DeviceFlashLed(LED_FLASH_ON);

	  if (LORAWAN_JoinNetwork())
	  {
		  DeviceFlashLed(5);
	  }

	  DeviceFlashLed(LED_FLASH_OFF);
  }
#else
  DevicePostEvent(RUN_ATTACH);
#endif

#if (NODE_TEMP > 0)
  DeviceFlashLed(LED_FLASH_ON);
  SUPERVISORUpdatePulseValue();
  DeviceFlashLed(LED_FLASH_OFF);
  if (GET_FLAG(DEVICE_COMM_ERROR))
	  DeviceFlashLed(10);
#endif
  DeviceFlashLed(LED_FLASH_OFF);
  for (;;) {
	DeviceResetAllButtons();
	EVENT_TYPE event = DeviceWaitForEvent(portMAX_DELAY);
	switch(event) {
		/*
		 * DeviceWaitForEvent timed out
		 */
	case IDLE_EVENT:
		break;
		/*
		 * Button was pressed.
		 */
	case BUTTON_EVENT:
    	if (!DevicePerformKeypressTasks(0,buttonTasks))
    	{
    	}
		lastButton = DeviceRTCGetSeconds();
      break;
      /*
       * A system error occurred
       */
	case ERROR_EVENT:
		ERROR("A system error occurred.\n");
        break;
        /*
         * A battery low level was detected
         */
	case BATTERYLOW_EVENT:
		TRACE("A battery low level was detected.\n");
		break;
        /*
         * A pulse occurred
         */
	case PULSE_EVENT:
		TRACE("A pulse occurred.\n");
		// Show Pulse LED if not installed or button was activated for less than 5 minutes
		if (GET_FLAG(DEVICE_UNINSTALLED) || ((DeviceRTCGetSeconds() - lastButton) < (5*60)))
			DeviceFlashOneLedExt(0,1,LED_FLASH_SHORTER);
		break;
		/*
		 * The Cyclic Task triggered
		 */
	case PERIODIC_EVENT:
		if (UNIT_INSTALLED == 0) break;

#if (DELAY_READ_DURATION > 0)
		portTickType now = xTaskGetTickCount();
		SUPERVISORUpdatePulseValue();
		/* Make sure that this function takes exactly DELAY_READ_DURATION seconds */
		vTaskDelayUntil(&now,(DELAY_READ_DURATION * configTICK_RATE_HZ));
#else
		SUPERVISORUpdatePulseValue();
#endif
		SUPERVISOR_SaveHistorical();

		if (UNIT_TRANS_ON == 0) break;
		/* no break */
	case PERIODIC_RESEND:
		if (UNIT_INSTALLED == 0) break;
		DeviceFlashLed(LED_FLASH_ON);
		if (UNIT_USE_SKT_APP)
			SKTAPP_SendPeriodic(event == PERIODIC_RESEND);
		else
			DEVICEAPP_SendPeriodic(event == PERIODIC_RESEND);
		DeviceFlashLed(LED_FLASH_OFF);
		/* Reset Status Flags until next periodic message */
		CLEAR_FLAG(DEVICE_COMM_ERROR | DEVICE_TEMPORARY_ERROR | DEVICE_PERMANENT_ERROR | DEVICE_LOW_BATTERY);
		break;
		/*
		 * Received an indication event from the network.
		 */
	case RF_INDICATION:
		{
			TRACE("Received an indication event from the network.\n");
			if (UNIT_USE_SKT_APP)
				SKTAPP_ParseMessage(LORAWAN_GetIndication());
			else
				DEVICEAPP_ParseMessage(LORAWAN_GetIndication());
		}
		break;
		/*
		 * Received a Mlme confirm event from the network
		 */
	case RF_MLME:
		TRACE("Received a Mlme confirm event from the network.\n");
		if (UNIT_USE_SKT_APP)
			SKTAPP_ParseMlme(LORAWAN_GetMlmeConfirm());
		else
			DEVICEAPP_ParseMlme(LORAWAN_GetMlmeConfirm());
		break;
		/*
		 * An error occurred during last RF transaction
		 */

	case	RUN_ATTACH:
		TRACE("Run attach.\n");

		if (UNIT_USE_SKT_APP)
		{
			if (UNIT_INSTALLED)
			{
				DevicePostEvent(REAL_JOIN_NETWORK);
			}
			else
			{
				DevicePostEvent(PSEUDO_JOIN_NETWORK);
			}
		}
		else
		{
			DevicePostEvent(RUN_ATTACH_USE_OTTA);
		}
		CLEAR_FLAG(DEVICE_COMM_ERROR | DEVICE_TEMPORARY_ERROR | DEVICE_LOW_BATTERY);	/* Reset Communication Status and Low battery indicator */
		break;

	case	RUN_ATTACH_USE_OTTA:
		DeviceFlashLed(1);

		if (LORAWAN_JoinNetworkUseOTTA((uint8_t*)UNIT_DEVEUID, (uint8_t*)UNIT_APPEUID, (uint8_t*)UNIT_APPKEY))
		{
			DeviceFlashLed(5);
			CLEAR_FLAG(DEVICE_UNINSTALLED);
			DeviceUserDataSetFlag(FLAG_INSTALLED,FLAG_INSTALLED);
			DevicePostEvent(PERIODIC_EVENT);		// Force immediate communication
			TRACE("Request to join has been completed.\n");
		}
		else
		{
			TRACE("Request to join failed.\n");
		}

		CLEAR_FLAG(DEVICE_COMM_ERROR | DEVICE_TEMPORARY_ERROR | DEVICE_LOW_BATTERY);	/* Reset Communication Status and Low battery indicator */
		DeviceFlashLed(LED_FLASH_OFF);
		break;


	case	RUN_ATTACH_USE_ABP:
		DeviceFlashLed(1);
		SUPERVISORUpdatePulseValue();
		if (LORAWAN_JoinNetworkUseABP())
		{
			TRACE("Request to join with ABP has been completed.\n");
			DeviceFlashLed(5);
			CLEAR_FLAG(DEVICE_UNINSTALLED);
			DeviceUserDataSetFlag(FLAG_INSTALLED,FLAG_INSTALLED);
			DevicePostEvent(PERIODIC_EVENT);		// Force immediate communication
		}
		else
		{
			TRACE("Request to join with ABP failed.\n");
		}
		CLEAR_FLAG(DEVICE_COMM_ERROR | DEVICE_TEMPORARY_ERROR | DEVICE_LOW_BATTERY);	/* Reset Communication Status and Low battery indicator */
		DeviceFlashLed(LED_FLASH_OFF);
		break;

	case	PSEUDO_JOIN_NETWORK:
		DeviceFlashLed(1);

		if (LORAWAN_JoinNetworkUseOTTA((uint8_t*)UNIT_DEVEUID, (uint8_t*)UNIT_APPEUID, (uint8_t*)UNIT_APPKEY))
		{
			DeviceFlashLed(5);
			TRACE("Request to pseudo join has been completed.\n");
			if (!bStepByStep)
			{
				DevicePostEvent(REQ_REAL_APP_KEY_ALLOC);
			}
		}
		else
		{
			TRACE("Request to pseudo join failed.\n");
		}

		CLEAR_FLAG(DEVICE_COMM_ERROR | DEVICE_TEMPORARY_ERROR | DEVICE_LOW_BATTERY);	/* Reset Communication Status and Low battery indicator */
		DeviceFlashLed(LED_FLASH_OFF);
		break;

	case	REQ_REAL_APP_KEY_ALLOC:
		DeviceFlashLed(1);

		TRACE("Request to real app alloc!\n");
		if (SKTAPP_SendRealAppKeyAllocReq())
		{
			DeviceFlashLed(5);
			if (!bStepByStep)
			{
				DevicePostEvent(REQ_REAL_APP_KEY_RX_REPORT);
			}
			TRACE("Request to real app key alloc has been completed.\n");
		}
		else
		{
			TRACE("Request to real app key alloc failed.\n");
			if (!bStepByStep)
			{
				DevicePostEvent(REQ_REAL_APP_KEY_ALLOC);
			}
		}

		CLEAR_FLAG(DEVICE_COMM_ERROR | DEVICE_TEMPORARY_ERROR | DEVICE_LOW_BATTERY);	/* Reset Communication Status and Low battery indicator */
		DeviceFlashLed(LED_FLASH_OFF);
		break;

	case	REQ_REAL_APP_KEY_RX_REPORT:
		DeviceFlashLed(1);

		TRACE("Request to real app key rx report!\n");
		if (SKTAPP_SendRealAppKeyRxReportReq())
		{
			DeviceFlashLed(5);
			if (!bStepByStep)
			{
				DevicePostEvent(REAL_JOIN_NETWORK);
			}
			TRACE("Request to real app key rx report has been completed.\n");
		}
		else
		{
			TRACE("Request to real app key rx report failed.\n");
			if (!bStepByStep)
			{
				DevicePostEvent(REQ_REAL_APP_KEY_RX_REPORT);
			}
		}

		CLEAR_FLAG(DEVICE_COMM_ERROR | DEVICE_TEMPORARY_ERROR | DEVICE_LOW_BATTERY);	/* Reset Communication Status and Low battery indicator */
		DeviceFlashLed(LED_FLASH_OFF);
		break;

	case	REAL_JOIN_NETWORK:
		TRACE("Real join\n");

		DeviceFlashLed(1);
		if (LORAWAN_JoinNetworkUseOTTA((uint8_t*)UNIT_DEVEUID, (uint8_t*)UNIT_APPEUID, (uint8_t*)UNIT_REALAPPKEY))
		{
			DeviceFlashLed(5);
			TRACE("Request to real join has been completed.\n");
			DevicePostEvent(REAL_JOIN_NETWORK_COMPLETED);
		}
		else
		{
			TRACE("Request to real join failed.\n");
		}

		CLEAR_FLAG(DEVICE_COMM_ERROR | DEVICE_TEMPORARY_ERROR | DEVICE_LOW_BATTERY);	/* Reset Communication Status and Low battery indicator */
		DeviceFlashLed(LED_FLASH_OFF);
		break;

	case REAL_JOIN_NETWORK_COMPLETED:
		CLEAR_FLAG(DEVICE_UNINSTALLED);
		DeviceUserDataSetFlag(FLAG_INSTALLED,FLAG_INSTALLED);

		if (!bStepByStep)
		{
//			DevicePostEvent(PERIODIC_EVENT);		// Force immediate communication
		}
		break;

	case RF_ERROR_EVENT:
		ERROR("An error occurred during last RF transaction.\n");
		break;
	default: break;
	}
  }
	__builtin_unreachable();
}

/** }@ */
