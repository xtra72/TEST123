/*******************************************************************
**                                                                **
** Supervisor management functions                                **
**                                                                **
*******************************************************************/

#ifndef __SUPERVISOR_H__
#define __SUPERVISOR_H__
/** \addtogroup S40 S40 Main Application
 *  @{
 */

/*!
 * @brief Returns the maximum depth of save historical values
 * @return Maximum number of archived values
 */
unsigned short SUPERVISOR_GetHistoricalCount(void);
/*!
 * @brief Returns an historical value for a given index number
 * @param rank	How deep in time shall we go (0 = latest)
 * @param index Which index value do we need (0 or 1)
 * @return The requested value or 0 if not existing
 */
unsigned long SUPERVISOR_GetHistoricalValue(int rank, int index);
/*!
 * @brief Get RF transmission period cycle
 * @return The period in minutes (from 1 to 240)
 */
unsigned char SUPERVISORGetRFPeriod(void);
/*!
 * @brief Restart RF transmission cyclic loop
 * @param[in] StartTicks	Starting FreeRTOS tick
 * @param[in] period		Period cycle in minutes (from 1 to 240)
 * @remark If period is not in range, the cyclic loop is not restarted
 */
void SUPERVISORStartCyclicTask(int StartTicks, unsigned char period);
/*!
 * @brief Supervisor task entry point
 * @param[in] pvParameters Unused
 */
void SUPERVISOR_Task(void* pvParameters);

/** }@ */
#endif
