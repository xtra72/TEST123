/***************************************************************************//**
 * @file
 * @brief Provide stdio retargeting configuration parameters.
 * @version 5.2.2
 *******************************************************************************
 * # License
 * <b>Copyright 2015 Silicon Labs, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/

#ifndef __SILICON_LABS_RETARGETSERIALCONFIG_H__
#define __SILICON_LABS_RETARGETSERIALCONFIG_H__

#include "bsp.h"

/***************************************************************************//**
 *
 * When retargeting serial output the user can choose which peripheral
 * to use as the serial output device. This choice is made by configuring
 * one or more of the following defines: RETARGET_USART0, RETARGET_LEUART0,
 * RETARGET_VCOM.
 *
 * This table shows the supported configurations and the resulting serial
 * output device.
 *
 * +----------------------------------------------------------------------+
 * | Defines                            | Serial Output (Locations)       |
 * |----------------------------------------------------------------------+
 * | None                               | USART0 (Rx #18, Tx #18)         |
 * | RETARGET_USART0                    | USART0 (Rx #18, Tx #18)         |
 * | RETARGET_VCOM                      | VCOM using USART0 #0            |
 * | RETARGET_LEUART0                   | LEUART0 (Rx #18, Tx #18)        |
 * | RETARGET_LEUART0 and RETARGET_VCOM | VCOM using LEUART0              |
 * +----------------------------------------------------------------------+
 *
 * Note that the default configuration is the same as RETARGET_USART0.
 *
 ******************************************************************************/

  #define RETARGET_PERIPHERAL_ENABLE() \
  GPIO_PinModeSet(BSP_BCC_ENABLE_PORT, \
                  BSP_BCC_ENABLE_PIN,  \
                  gpioModePushPull,    \
                  1);
    #undef RETARGET_TX_LOCATION
    #undef RETARGET_RX_LOCATION
    #undef RETARGET_TXPORT
    #undef RETARGET_TXPIN
    #undef RETARGET_RXPORT
    #undef RETARGET_RXPIN
    #define RETARGET_TX_LOCATION _USART_ROUTELOC0_TXLOC_LOC0 /* Location of of USART TX pin */
    #define RETARGET_RX_LOCATION _USART_ROUTELOC0_RXLOC_LOC0 /* Location of of USART RX pin */
    #define RETARGET_TXPORT      gpioPortA                   /* UART transmission port */
    #define RETARGET_TXPIN       0                           /* UART transmission pin */
    #define RETARGET_RXPORT      gpioPortA                   /* UART reception port */
    #define RETARGET_RXPIN       1                           /* UART reception pin */

#endif
