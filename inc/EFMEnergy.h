/*
** Energy Management functions for EFM32
**/
#ifndef __EFMENERGY_H__
#define __EFMENERGY_H__
/* Definitions specific to the port being used. */
/** \addtogroup RTOS FreeRTOS Implementation
 *  @{
 */

#include "portable.h"

/*!
 * @brief FreeRTOS Low Power Mode entry
 * @param[in] expected number of FreeRTOS ticks that are expected to elapse
 */
void vEFMEnergyEnter(portTickType expected);
/*!
 * @brief FreeRTOS Low Power Mode exit
 * @param[in] expected number of FreeRTOS ticks that are expected to elapse
 */
void vEFMEnergyRestore(portTickType expected);
/*!
 * @brief Allows or disallows deep sleep mode.
 * If a task uses a peripheral function that shall prevent the system to go to deep sleep mode
 * the task shall disable Low Power mode
 * @param[in] enable true to allow deep sleep mode
 */
void vEFMEnergyEnableLowPowerMode(int enable);
/** }@ */
#endif
