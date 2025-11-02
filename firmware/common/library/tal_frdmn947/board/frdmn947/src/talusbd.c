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
#define __TALUSBD_C__

/*=======================================================================*/
/*  Include                                                              */
/*=======================================================================*/
#include <stdint.h>
#include <string.h>
#include "tal.h"
#include "terminal.h"

#include "clock_config.h"
#include "usb.h"
#include "usb_phy.h"

void USB_X_IRQHandler (void);

/*=======================================================================*/
/*  All Structures and Common Constants                                  */
/*=======================================================================*/

/* USB PHY condfiguration */
#define BOARD_USB_PHY_D_CAL            (0x04U)
#define BOARD_USB_PHY_TXCAL45DP        (0x07U)
#define BOARD_USB_PHY_TXCAL45DM        (0x07U)

#define CONTROLLER_ID                  kUSB_ControllerEhci0
#define USB_DEVICE_INTERRUPT_PRIORITY  (3U)

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
/*  USB_ControllerIdGet                                                  */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: CONTROLLER_ID                                                */
/*************************************************************************/
uint8_t USB_ControllerIdGet (void)
{
   return(CONTROLLER_ID);
} /* USB_ControllerIdGet */

/*************************************************************************/
/*  USB_DeviceClockInit                                                  */
/*                                                                       */
/*  Initialize the USB clock.                                            */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void USB_DeviceClockInit (void)
{
   usb_phy_config_struct_t phyConfig = {
      BOARD_USB_PHY_D_CAL,
      BOARD_USB_PHY_TXCAL45DP,
      BOARD_USB_PHY_TXCAL45DM,
   };
   
   SPC0->ACTIVE_VDELAY = 0x0500;
   /* Change the power DCDC to 1.8v (By deafult, DCDC is 1.8V), CORELDO to 1.1v (By deafult, CORELDO is 1.0V) */
   SPC0->ACTIVE_CFG &= ~SPC_ACTIVE_CFG_CORELDO_VDD_DS_MASK;
   SPC0->ACTIVE_CFG |= SPC_ACTIVE_CFG_DCDC_VDD_LVL(0x3) | SPC_ACTIVE_CFG_CORELDO_VDD_LVL(0x3) |
                       SPC_ACTIVE_CFG_SYSLDO_VDD_DS_MASK | SPC_ACTIVE_CFG_DCDC_VDD_DS(0x2u);
   /* Wait until it is done */
   while (SPC0->SC & SPC_SC_BUSY_MASK)
      ;
   if (0u == (SCG0->LDOCSR & SCG_LDOCSR_LDOEN_MASK))
   {
      SCG0->TRIM_LOCK = 0x5a5a0001U;
      SCG0->LDOCSR |= SCG_LDOCSR_LDOEN_MASK;
      /* wait LDO ready */
      while (0U == (SCG0->LDOCSR & SCG_LDOCSR_VOUT_OK_MASK))
         ;
   }
   SYSCON->AHBCLKCTRLSET[2] |= SYSCON_AHBCLKCTRL2_USB_HS_MASK | SYSCON_AHBCLKCTRL2_USB_HS_PHY_MASK;
   SCG0->SOSCCFG &= ~(SCG_SOSCCFG_RANGE_MASK | SCG_SOSCCFG_EREFS_MASK);
   /* xtal = 20 ~ 30MHz */
   SCG0->SOSCCFG = (1U << SCG_SOSCCFG_RANGE_SHIFT) | (1U << SCG_SOSCCFG_EREFS_SHIFT);
   SCG0->SOSCCSR |= SCG_SOSCCSR_SOSCEN_MASK;
   while (1)
   {
      if (SCG0->SOSCCSR & SCG_SOSCCSR_SOSCVLD_MASK)
      {
         break;
      }
   }
   SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_CLKIN_ENA_MASK | SYSCON_CLOCK_CTRL_CLKIN_ENA_FM_USBH_LPT_MASK;
   CLOCK_EnableClock(kCLOCK_UsbHs);
   CLOCK_EnableClock(kCLOCK_UsbHsPhy);
   CLOCK_EnableUsbhsPhyPllClock(kCLOCK_Usbphy480M, 24000000U);
   CLOCK_EnableUsbhsClock();
   USB_EhciPhyInit(CONTROLLER_ID, BOARD_XTAL0_CLK_HZ, &phyConfig);
    
} /* USB_DeviceClockInit */

/*************************************************************************/
/*  USB_DeviceIsrEnable                                                  */
/*                                                                       */
/*  Enable USB interrupt.                                                */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void USB_DeviceIsrEnable (void)
{
   uint8_t irqNumber;
   
   uint8_t usbDeviceEhciIrq[] = USBHS_IRQS;
   irqNumber                  = usbDeviceEhciIrq[CONTROLLER_ID - kUSB_ControllerEhci0];

   /* Install isr, set priority, and enable IRQ. */
   NVIC_SetPriority((IRQn_Type)irqNumber, USB_DEVICE_INTERRUPT_PRIORITY);
   EnableIRQ((IRQn_Type)irqNumber);
   
} /* USB_DeviceIsrEnable */

/*************************************************************************/
/*  USB1_HS_IRQHandler                                                   */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void USB1_HS_IRQHandler (void)
{
    TAL_CPU_IRQ_ENTER();

    USB_X_IRQHandler();
    
    __DSB();
    
    TAL_CPU_IRQ_EXIT();    
} /* USB1_HS_IRQHandler */

#endif /* USE_BOARD_FRDMN947 */

/*** EOF ***/
