/*
** Energy Management functions for EFM32
**/
/** \addtogroup RTOS FreeRTOS Implementation
 * @brief Local FreeRTOS implementation files including Low Power mode
 *  @{
 */

#include "FreeRTOS.h"
#include "EFMEnergy.h"
#include <em_chip.h>
#include <em_cmu.h>
#include <em_emu.h>

  /* By default deep sleep mode is allowed */
  PRIVILEGED_DATA static volatile portBASE_TYPE LowPowerDisabled = 0;

/**
 * \brief Put EFM32 processor in Low Power mode EM1 or EM2
 * @param expected 	Number of expected ticks to sleep sent by FreeRTOS
 */
void vEFMEnergyEnter(portTickType expected) {
    if (LowPowerDisabled) {
    	EMU_EnterEM1();
//		SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    } else {
    	EMU_EnterEM2(false);
//		SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
	} 
 }
/**
 * \brief Restore EFM32 processor from Low Power Mode
 * \details Nothing to do.
 * @param expected	Number of expected ticks sent by FreeRTOS
 */
void vEFMEnergyRestore(portTickType expected) {
}

/**
 * \brief Allow or disallow EFM32 processor to go in deep sleep mode
 * @param enable	true to allow deep sleep mode
 */
void vEFMEnergyEnableLowPowerMode(int enable) {
	if (enable) {
		if (LowPowerDisabled) LowPowerDisabled--;
		if (LowPowerDisabled == 0) EMU_EM2UnBlock();
	}
	else {
		LowPowerDisabled++;
		EMU_EM2Block();
	}
}
/**  }@
 */
