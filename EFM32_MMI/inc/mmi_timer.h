/**
 * mmi_timer.h
 *
 * @date 24 juin 2017
 * @author omogenot
 */

#ifndef EFM32_MMI_INC_MMI_TIMER_H_
#define EFM32_MMI_INC_MMI_TIMER_H_
#include "system.h"
/** \addtogroup MMI MyMeterInfo add-on functions
 *  @{
 */

/*!
 * @brief Weak definition of Timer IRQ handler
 * @remark This function must be re-defined by the user
 */
__weak void TimerIrqHandler(void);

/*!
 * @brief Timer hardware initialization
 * @remark the hardware timer is based upon milliseconds values
 */
void TIMERInit(void);

/*!
 * @brief Starts the hardware timer for duration milliseconds
 * The TimerIrqHandler will be triggered when timer expires
 * @remark The timer is limited to less than 16,320,000 ms (about 4533 hours) which shall be enough
 * @param duration
 */
void TIMERStart(int duration);

/*!
 * @brief Stops hardware time before it expired
 */
void TIMERStop(void);

/** }@ */
#endif /* EFM32_MMI_INC_MMI_TIMER_H_ */
