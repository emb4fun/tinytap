/**************************************************************************
*  This file is part of the TAL project (Tiny Abstraction Layer)
*
*  Copyright (c) 2024 by Michael Fischer (www.emb4fun.de).
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*  1. Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*
*  2. Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
*
*  3. Neither the name of the author nor the names of its contributors may
*     be used to endorse or promote products derived from this software
*     without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
*  THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
*  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
*  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
*  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
*  SUCH DAMAGE.
**************************************************************************/
#if defined(USE_BOARD_FRDMN947)
#define __TALBOARD_C__

/*=======================================================================*/
/*  Include                                                              */
/*=======================================================================*/
#include <string.h>
#include "tal.h"

#if defined(TAL_ENABLE_ETH)
#include "ipstack_conf.h"
#include "fsl_silicon_id.h"
#endif

#include "fsl_common.h"
#include "fsl_port.h"

/*=======================================================================*/
/*  All Structures and Common Constants                                  */
/*=======================================================================*/

/*=======================================================================*/
/*  Definition of all global Data                                        */
/*=======================================================================*/

/*=======================================================================*/
/*  Definition of all local Data                                         */
/*=======================================================================*/

/*=======================================================================*/
/*  Definition of all local Procedures                                   */
/*=======================================================================*/

/*=======================================================================*/
/*  All code exported                                                    */
/*=======================================================================*/

/*************************************************************************/
/*  tal_BoardEnableCOMx                                                  */
/*                                                                       */
/*  Board and hardware depending functionality to enable the COM port.   */
/*  If this port is not supported by the board, return an error.         */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: TAL_OK / error cause                                         */
/*************************************************************************/
TAL_RESULT tal_BoardEnableCOM1 (void)  /* USART0 */
{
   /* UART4 */

   /* Attach FRO 12M to FLEXCOMM4 (LPUART0) */
   CLOCK_SetClkDiv(kCLOCK_DivFlexcom0Clk, 1u);
   CLOCK_AttachClk(kFRO12M_to_FLEXCOMM0);
   
   /* Enables the clock for PORT0: Enables clock */
   CLOCK_EnableClock(kCLOCK_Port0);

   const port_pin_config_t port0_14_pinE11_config = {/* Internal pull-up/down resistor is disabled */
                                                     kPORT_PullDisable,
                                                     /* Low internal pull resistor value is selected. */
                                                     kPORT_LowPullResistor,
                                                     /* Fast slew rate is configured */
                                                     kPORT_FastSlewRate,
                                                     /* Passive input filter is disabled */
                                                     kPORT_PassiveFilterDisable,
                                                     /* Open drain output is disabled */
                                                     kPORT_OpenDrainDisable,
                                                     /* Low drive strength is configured */
                                                     kPORT_LowDriveStrength,
                                                     /* Pin is configured as FC4_P0 */
                                                     kPORT_MuxAlt3,
                                                     /* Digital input enabled */
                                                     kPORT_InputBufferEnable,
                                                     /* Digital input is not inverted */
                                                     kPORT_InputNormal,
                                                     /* Pin Control Register fields [15:0] are not locked */
                                                     kPORT_UnlockRegister};
   /* PORT0_14 (pin E11) is configured as FC0_P2 */
   PORT_SetPinConfig(PORT0, 14U, &port0_14_pinE11_config);

   const port_pin_config_t port0_15_pinG13_config = {/* Internal pull-up/down resistor is disabled */
                                                     kPORT_PullDisable,
                                                     /* Low internal pull resistor value is selected. */
                                                     kPORT_LowPullResistor,
                                                     /* Fast slew rate is configured */
                                                     kPORT_FastSlewRate,
                                                     /* Passive input filter is disabled */
                                                     kPORT_PassiveFilterDisable,
                                                     /* Open drain output is disabled */
                                                     kPORT_OpenDrainDisable,
                                                     /* Low drive strength is configured */
                                                     kPORT_LowDriveStrength,
                                                     /* Pin is configured as FC4_P1 */
                                                     kPORT_MuxAlt3,
                                                     /* Digital input enabled */
                                                     kPORT_InputBufferEnable,
                                                     /* Digital input is not inverted */
                                                     kPORT_InputNormal,
                                                     /* Pin Control Register fields [15:0] are not locked */
                                                     kPORT_UnlockRegister};
   /* PORT0_15 (pin G13) is configured as FC0_P3 */
   PORT_SetPinConfig(PORT0, 15U, &port0_15_pinG13_config);   
   
   return(TAL_OK);
} /* tal_BoardEnableCOM1 */

TAL_RESULT tal_BoardEnableCOM2 (void)  /* USART1 */
{
   return(TAL_ERR_COM_PORT_NOHW);
} /* tal_BoardEnableCOM2 */

TAL_RESULT tal_BoardEnableCOM3 (void)  /* USART2 */
{
   return(TAL_ERR_COM_PORT_NOHW);
} /* tal_BoardEnableCOM3 */

TAL_RESULT tal_BoardEnableCOM4 (void)
{
   /* USART3 */
   return(TAL_ERR_COM_PORT_NOHW);
} /* tal_BoardEnableCOM4 */

TAL_RESULT tal_BoardEnableCOM5 (void)
{
   /* UART4 */
   
   /* Attach FRO 12M to FLEXCOMM4 (LPUART4) */
   CLOCK_SetClkDiv(kCLOCK_DivFlexcom4Clk, 1u);
   CLOCK_AttachClk(kFRO12M_to_FLEXCOMM4);
   
   /* Enables the clock for PORT1: Enables clock */
   CLOCK_EnableClock(kCLOCK_Port1);

   const port_pin_config_t port1_8_pinA1_config = {/* Internal pull-up/down resistor is disabled */
                                                   kPORT_PullDisable,
                                                   /* Low internal pull resistor value is selected. */
                                                   kPORT_LowPullResistor,
                                                   /* Fast slew rate is configured */
                                                   kPORT_FastSlewRate,
                                                   /* Passive input filter is disabled */
                                                   kPORT_PassiveFilterDisable,
                                                   /* Open drain output is disabled */
                                                   kPORT_OpenDrainDisable,
                                                   /* Low drive strength is configured */
                                                   kPORT_LowDriveStrength,
                                                   /* Pin is configured as FC4_P0 */
                                                   kPORT_MuxAlt2,
                                                   /* Digital input enabled */
                                                   kPORT_InputBufferEnable,
                                                   /* Digital input is not inverted */
                                                   kPORT_InputNormal,
                                                   /* Pin Control Register fields [15:0] are not locked */
                                                   kPORT_UnlockRegister};
   /* PORT1_8 (pin A1) is configured as FC4_P0 */
   PORT_SetPinConfig(PORT1, 8U, &port1_8_pinA1_config);

   const port_pin_config_t port1_9_pinB1_config = {/* Internal pull-up/down resistor is disabled */
                                                   kPORT_PullDisable,
                                                   /* Low internal pull resistor value is selected. */
                                                   kPORT_LowPullResistor,
                                                   /* Fast slew rate is configured */
                                                   kPORT_FastSlewRate,
                                                   /* Passive input filter is disabled */
                                                   kPORT_PassiveFilterDisable,
                                                   /* Open drain output is disabled */
                                                   kPORT_OpenDrainDisable,
                                                   /* Low drive strength is configured */
                                                   kPORT_LowDriveStrength,
                                                   /* Pin is configured as FC4_P1 */
                                                   kPORT_MuxAlt2,
                                                   /* Digital input enabled */
                                                   kPORT_InputBufferEnable,
                                                   /* Digital input is not inverted */
                                                   kPORT_InputNormal,
                                                   /* Pin Control Register fields [15:0] are not locked */
                                                   kPORT_UnlockRegister};
   /* PORT1_9 (pin B1) is configured as FC4_P1 */
   PORT_SetPinConfig(PORT1, 9U, &port1_9_pinB1_config);   
   
   return(TAL_OK);
} /* tal_BoardEnableCOM5 */

TAL_RESULT tal_BoardEnableCOM6 (void)
{
   /* UART5 */
   
   /* Attach FRO 12M to FLEXCOMM5 (LPUART5) */
   CLOCK_SetClkDiv(kCLOCK_DivFlexcom5Clk, 1u);
   CLOCK_AttachClk(kFRO12M_to_FLEXCOMM5);
   
   /* Enables the clock for PORT1: Enables clock */
   CLOCK_EnableClock(kCLOCK_Port1);

   const port_pin_config_t port1_16_pinF6_config = {/* Internal pull-up/down resistor is disabled */
                                                    kPORT_PullDisable,
                                                    /* Low internal pull resistor value is selected. */
                                                    kPORT_LowPullResistor,
                                                    /* Fast slew rate is configured */
                                                    kPORT_FastSlewRate,
                                                    /* Passive input filter is disabled */
                                                    kPORT_PassiveFilterDisable,
                                                    /* Open drain output is disabled */
                                                    kPORT_OpenDrainDisable,
                                                    /* Low drive strength is configured */
                                                    kPORT_LowDriveStrength,
                                                    /* Pin is configured as FC4_P0 */
                                                    kPORT_MuxAlt2,
                                                    /* Digital input enabled */
                                                    kPORT_InputBufferEnable,
                                                    /* Digital input is not inverted */
                                                    kPORT_InputNormal,
                                                    /* Pin Control Register fields [15:0] are not locked */
                                                    kPORT_UnlockRegister};
   /* PORT1_16 (pin F6) is configured as FC5_P0 */
   PORT_SetPinConfig(PORT1, 16U, &port1_16_pinF6_config);

   const port_pin_config_t port1_17_pinF4_config = {/* Internal pull-up/down resistor is disabled */
                                                    kPORT_PullDisable,
                                                    /* Low internal pull resistor value is selected. */
                                                    kPORT_LowPullResistor,
                                                    /* Fast slew rate is configured */
                                                    kPORT_FastSlewRate,
                                                    /* Passive input filter is disabled */
                                                    kPORT_PassiveFilterDisable,
                                                    /* Open drain output is disabled */
                                                    kPORT_OpenDrainDisable,
                                                    /* Low drive strength is configured */
                                                    kPORT_LowDriveStrength,
                                                    /* Pin is configured as FC4_P1 */
                                                    kPORT_MuxAlt2,
                                                    /* Digital input enabled */
                                                    kPORT_InputBufferEnable,
                                                    /* Digital input is not inverted */
                                                    kPORT_InputNormal,
                                                    /* Pin Control Register fields [15:0] are not locked */
                                                    kPORT_UnlockRegister};
   /* PORT1_17 (pin F4) is configured as FC5_P1 */
   PORT_SetPinConfig(PORT1, 17U, &port1_17_pinF4_config);   
   
   return(TAL_OK);
} /* tal_BoardEnableCOM6 */

TAL_RESULT tal_BoardEnableCOM7 (void)
{
   /* UART6 */
   return(TAL_ERR_COM_PORT_NOHW);
} /* tal_BoardEnableCOM7 */

TAL_RESULT tal_BoardEnableCOM8 (void)
{
   /* UART7 */
   return(TAL_ERR_COM_PORT_NOHW);
} /* tal_BoardEnableCOM8 */

TAL_RESULT tal_BoardEnableCOM9 (void)
{
   /* UART8 */
   
   /* Attach FRO 12M to FLEXCOMM8 (LPUART8) */
   CLOCK_SetClkDiv(kCLOCK_DivFlexcom8Clk, 1u);
   CLOCK_AttachClk(kFRO12M_to_FLEXCOMM8);
   
   /* Enables the clock for PORT3: Enables clock */
   CLOCK_EnableClock(kCLOCK_Port3);

   const port_pin_config_t port3_16_pinJ5_config =  {/* Internal pull-up/down resistor is disabled */
                                                     kPORT_PullDisable,
                                                     /* Low internal pull resistor value is selected. */
                                                     kPORT_LowPullResistor,
                                                     /* Fast slew rate is configured */
                                                     kPORT_FastSlewRate,
                                                     /* Passive input filter is disabled */
                                                     kPORT_PassiveFilterDisable,
                                                     /* Open drain output is disabled */
                                                     kPORT_OpenDrainDisable,
                                                     /* Low drive strength is configured */
                                                     kPORT_LowDriveStrength,
                                                     /* Pin is configured as FC4_P0 */
                                                     kPORT_MuxAlt2,
                                                     /* Digital input enabled */
                                                     kPORT_InputBufferEnable,
                                                     /* Digital input is not inverted */
                                                     kPORT_InputNormal,
                                                     /* Pin Control Register fields [15:0] are not locked */
                                                     kPORT_UnlockRegister};
   /* PORT3_16 (pin J5) is configured as FC8_P2 */
   PORT_SetPinConfig(PORT3, 16U, &port3_16_pinJ5_config);

   const port_pin_config_t port3_17_pinK15_config = {/* Internal pull-up/down resistor is disabled */
                                                     kPORT_PullDisable,
                                                     /* Low internal pull resistor value is selected. */
                                                     kPORT_LowPullResistor,
                                                     /* Fast slew rate is configured */
                                                     kPORT_FastSlewRate,
                                                     /* Passive input filter is disabled */
                                                     kPORT_PassiveFilterDisable,
                                                     /* Open drain output is disabled */
                                                     kPORT_OpenDrainDisable,
                                                     /* Low drive strength is configured */
                                                     kPORT_LowDriveStrength,
                                                     /* Pin is configured as FC4_P1 */
                                                     kPORT_MuxAlt2,
                                                     /* Digital input enabled */
                                                     kPORT_InputBufferEnable,
                                                     /* Digital input is not inverted */
                                                     kPORT_InputNormal,
                                                     /* Pin Control Register fields [15:0] are not locked */
                                                     kPORT_UnlockRegister};
   /* PORT3_17 (pin K15) is configured as FC8_P3 */
   PORT_SetPinConfig(PORT3, 17U, &port3_17_pinK15_config);   
   
   return(TAL_OK);
} /* tal_BoardEnableCOM8 */

/*************************************************************************/
/*  tal_BoardEnableCANx                                                  */
/*                                                                       */
/*  Board and hardware depending functionality to enable the CAN port.   */
/*  If this port is not supported by the board, return an error.         */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: TAL_OK / error cause                                         */
/*************************************************************************/
TAL_RESULT tal_BoardEnableCAN1 (void)
{
   /* FlexCAN0 */
   
   /* Enables the clock for PORT1: Enables clock */
   CLOCK_EnableClock(kCLOCK_Port1);

   const port_pin_config_t port1_10_pinC3_config = {/* Internal pull-up/down resistor is disabled */
                                                    kPORT_PullDisable,
                                                    /* Low internal pull resistor value is selected. */
                                                    kPORT_LowPullResistor,
                                                    /* Fast slew rate is configured */
                                                    kPORT_FastSlewRate,
                                                    /* Passive input filter is disabled */
                                                    kPORT_PassiveFilterDisable,
                                                    /* Open drain output is disabled */
                                                    kPORT_OpenDrainDisable,
                                                    /* Low drive strength is configured */
                                                    kPORT_LowDriveStrength,
                                                    /* Pin is configured as CAN0_TXD */
                                                    kPORT_MuxAlt11,
                                                    /* Digital input enabled */
                                                    kPORT_InputBufferEnable,
                                                    /* Digital input is not inverted */
                                                    kPORT_InputNormal,
                                                    /* Pin Control Register fields [15:0] are not locked */
                                                    kPORT_UnlockRegister};
   /* PORT1_10 (pin C3) is configured as CAN0_TXD */
   PORT_SetPinConfig(PORT1, 10U, &port1_10_pinC3_config);

   const port_pin_config_t port1_11_pinD3_config = {/* Internal pull-up/down resistor is disabled */
                                                    kPORT_PullDisable,
                                                    /* Low internal pull resistor value is selected. */
                                                    kPORT_LowPullResistor,
                                                    /* Fast slew rate is configured */
                                                    kPORT_FastSlewRate,
                                                    /* Passive input filter is disabled */
                                                    kPORT_PassiveFilterDisable,
                                                    /* Open drain output is disabled */
                                                    kPORT_OpenDrainDisable,
                                                    /* Low drive strength is configured */
                                                    kPORT_LowDriveStrength,
                                                    /* Pin is configured as CAN0_RXD */
                                                    kPORT_MuxAlt11,
                                                    /* Digital input enabled */
                                                    kPORT_InputBufferEnable,
                                                    /* Digital input is not inverted */
                                                    kPORT_InputNormal,
                                                    /* Pin Control Register fields [15:0] are not locked */
                                                    kPORT_UnlockRegister};
   /* PORT1_11 (pin D3) is configured as CAN0_RXD */
   PORT_SetPinConfig(PORT1, 11U, &port1_11_pinD3_config);
   
   return(TAL_OK);
} /* tal_BoardEnableCAN1 */

TAL_RESULT tal_BoardEnableCAN2 (void)
{
   return(TAL_ERR_CAN_PORT_NOHW);
} /* tal_BoardEnableCAN2 */

#if defined(TAL_ENABLE_ETH)
/*************************************************************************/
/*  tal_BoardGetMACAddress                                               */
/*                                                                       */
/*  Retrieve the MAC address of the board.                               */
/*  In case of an error, a default address will be used.                 */
/*                                                                       */
/*  In    : pAddress                                                     */
/*  Out   : pAddress                                                     */
/*  Return: TAL_OK / TAL_ERROR                                           */
/*************************************************************************/
TAL_RESULT tal_BoardGetMACAddress (int iface, uint8_t *pAddress)
{
   TAL_RESULT      Error         = TAL_ERROR;
   static uint8_t bMACRetrieved  = TAL_FALSE;
   static uint8_t  MACAddress[6] = IP_DEFAULT_MAC_ADDR;

#if !defined(USE_IP_DEFAULT_MAC_ADDR)
   if (TAL_FALSE == bMACRetrieved)
   {
      status_t result = SILICONID_ConvertToMacAddr(&MACAddress);
      if ((kStatus_Success == result) && (0 == iface))
      {
         bMACRetrieved = TAL_TRUE;
         Error         = TAL_OK;
      }  
   }
#else
  (void)iface;
  (void)bMACRetrieved;
  Error = TAL_OK;
#endif

   /* Return MAC address */
   memcpy(pAddress, MACAddress, 6);

   return(Error);
} /* tal_BoardGetMACAddress */
#endif /* defined(TAL_ENABLE_ETH) */

/*************************************************************************/
/*  tal_BoardRTCSetTM                                                    */
/*                                                                       */
/*  Set the onboard RTC time by TM                                       */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void tal_BoardRTCSetTM (struct tm *pTM)
{
   (void)pTM;
} /* tal_BoardRTCSetTM */

/*************************************************************************/
/*  tal_BoardRTCSetUnixtime                                              */
/*                                                                       */
/*  Set the onboard RTC time by Unixtime                                 */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void tal_BoardRTCSetUnixtime (uint32_t Unixtime)
{
   (void)Unixtime;
} /* tal_BoardRTCSetUnixtime */

/*************************************************************************/
/*  tal_BoardRTC2System                                                  */
/*                                                                       */
/*  Set the system time from the RTC.                                    */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void tal_BoardRTC2System (void)
{
} /* tal_BoardRTC2System */

#endif /* USE_BOARD_FRDMN947 */

/*** EOF ***/
