/*******************************************************************
**                                                                **
** RTC management functions                                       **
**                                                                **
*******************************************************************/

#ifndef __SYSRTC_H__
#define __SYSRTC_H__
/** \addtogroup MMI MyMeterInfo add-on functions
 *  @{
 */

//! Timestamp difference between 01/01/2000 and Unix constant
#define UNIX_TIMESTAMP_DIFF	 	946598400UL
#define JAN1900_TIMESTAMP_DIFF 3155587200UL		//!< Timestamp difference between 01/01/2000 and 01/01/1900 constant

/*!
 * @brief Get current RTC (Real Time Clock) elapsed time since 01/01/2000
 * @return Number of seconds elapsed since 01/01/2000
 */
unsigned long RTCGetSeconds(void);
/*!
 * @brief Set current RTC (Real Time Clock) elapsed time since 01/01/2000
 * @param[in] seconds Number of elapsed seconds to set
 */
void RTCSetSeconds(const unsigned long seconds);

/** }@ */
#endif
