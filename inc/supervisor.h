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
 * @return The period in seconds (from 1 to 30*24*60*60)
 */
unsigned long SUPERVISOR_GetRFPeriod(void);

bool	SUPERVISOR_SetRFPeriod(unsigned long ulPeriod);
/*!
 * @brief Restart RF transmission cyclic loop
 * @param[in] StartTicks	Starting FreeRTOS tick
 * @param[in] period		Period cycle in minutes (from 1 to 30*24*60*60)
 * @remark If period is not in range, the cyclic loop is not restarted
 */
void SUPERVISOR_StartCyclicTask(int StartTicks, unsigned long period);

/*!
 * @brief Stop RF transmission cyclic loop
 */
void SUPERVISOR_StopCyclicTask(void);

bool SUPERVISOR_IsCyclicTaskRun(void);

/*!
 * @brief Supervisor task entry point
 * @param[in] pvParameters Unused
 */

void	SUPERVISOR_SetAsyncCall(bool bAsync);
bool	SUPERVISOR_IsAsyncCall();

uint8_t	SUPERVISOR_GetSNR(void);
int16_t	SUPERVISOR_GetRSSI(void);
uint8_t	SUPERVISOR_GetSF(void);

void SUPERVISOR_Task(void* pvParameters);

/** }@ */
#endif
