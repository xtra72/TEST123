/*******************************************************************
**                                                                **
** Project global definitions                                     **
**                                                                **
*******************************************************************/

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

/*!
 * @brief Setting this macro to 1 will set by default USERDATA device flag FLAG_USE_SKT_APP to
 * force the S47 device to use SKT/Daliworks LoRaWAN messages
 */
#define USE_SKT_FORMAT	( 1 )

#ifndef NODE_PULSE
/*!
 * @brief Defines the number of pulse input(s) handled by the device
 * @remark Mutually exclusive with other types of input.
 */
#define NODE_PULSE		( 1 )
#endif
#ifndef NODE_TEMP
/*!
 * @brief Defines that 1 or 2 temperature sensor(s) is(are) attached to the device
 * @remark Mutually exclusive with other types of input.
 */
#define NODE_TEMP		( 0 )
#endif
#ifndef NODE_HYGRO
/*!
 * @brief Defines that 1 temperature and humidity sensor is attached to the device
 * @remark Mutually exclusive with other types of input.
 */
#define NODE_HYGRO		( 0 )
#endif
#ifndef NODE_ANALOG
/*!
 * @brief Defines that an analog input (0-10V or 4-20mA) is attached to the device
 * @remark Mutually exclusive with other types of input.
 */
#define NODE_ANALOG		( 0 )
#endif

#if ((NODE_TEMP > 0) && (NODE_ANALOG > 0)) || ((NODE_TEMP > 0) && (NODE_HYGRO > 0)) || ((NODE_HYGRO > 0) && (NODE_ANALOG > 0))
#error "ERROR: Check device configuration"
#endif
#if (NODE_PULSE > 0) && ((NODE_TEMP > 0) || (NODE_ANALOG > 0) || (NODE_HYGRO > 0))
#warning "WARNING: NODE_PULSE value ignored"
#endif
#if (NODE_TEMP > 2)
#error "ERROR: NODE_TEMP cannot be > 2"
#endif
#if (NODE_HYGRO > 1)
#error "ERROR: NODE_HYGRO cannot be > 1"
#endif
#if (NODE_ANALOG > 1)
#error "ERROR: NODE_ANALOG cannot be > 1"
#endif

/*!
 * @brief Defines the maximum number of historical values kept in memory. Put 0 not to keep any.
 */
#define HISTORICAL_DATA			(100)

#if NODE_PULSE
#define DEVICETYPE_DEFAULT	(0 + NODE_PULSE)
#elif NODE_TEMP
#define DEVICETYPE_DEFAULT	(15 + NODE_TEMP)
#elif NODE_HYGRO
#define DEVICETYPE_DEFAULT	(18)
#elif NODE_ANALOG
#define DEVICETYPE_DEFAULT	(19)
#endif

#define	SUPERVISOR_CYCLIC_TASK_DEFAULT	(1*60)
#define	SUPERVISOR_CYCLIC_TASK_MIN		(1)
#define	SUPERVISOR_CYCLIC_TASK_MAX		(30 * 24 * 60)

#define	LORAMAC_DEV_EUI_SIZE	8
#define	LORAMAC_APP_EUI_SIZE	8

#define	LORAMAC_APP_KEY_SIZE	16

#define	LORAMAC_NWK_SKEY_SIZE	16
#define	LORAMAC_APP_SKEY_SIZE	16

#define	LORAMAC_TX_DATARATE_MIN	1
#define	LORAMAC_TX_DATARATE_MAX	5

#define	LORAMAC_TX_POWER_MIN	0
#define	LORAMAC_TX_POWER_MAX	6

#define	LORAMAC_TX_CHANNEL_MIN	26
#define	LORAMAC_TX_CHANNEL_MAX	32

#define	LORAMAC_TX_RETRANSMISSION_COUNT_MIN	1
#define	LORAMAC_TX_RETRANSMISSION_COUNT_MAX	15

#define	LORAWAN_RETRANSMISSION_MIN	1
#define	LORAWAN_RETRANSMISSION_MAX	8

#define	S47_SKT_CERT_ENABLED		1

#define	SHELL_USED_POLLING_LEUART	0

/* Hardware Init */
#include "device_def.h"
#include "board.h"

 /* RTOS includes. */
/* Definitions specific to the port being used. */
/** \addtogroup RTOS FreeRTOS Implementation
 *  @{
 */
#include <FreeRTOS.h>
#include <croutine.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include <timers.h>
#include "EFMEnergy.h"
/** }@ */

/* MMI Lib includes */
#include <mcu_rtc.h>
#include <datetime.h>
#include <mmi_spi.h>

#define	S47_SHELL_BAUDRATE_DEFAULT	38400
#define	S47_SHELL_DATABITS_DEFAULT	leuartDatabits8
#define	S47_SHELL_PARITY_DEFAULT	leuartNoParity
#define	S47_SHELL_STOPBITS_DEFAULT	leuartStopbits1

#endif

