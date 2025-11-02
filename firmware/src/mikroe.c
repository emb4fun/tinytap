/**************************************************************************
*  Copyright (c) 2025 by Michael Fischer (www.emb4fun.de).
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
#define __MIKROE_C__

/*=======================================================================*/
/*  Includes                                                             */
/*=======================================================================*/
#include <stdint.h>
#include "tal.h"
#include "tcts.h"

#include "fsl_common.h"
#include "fsl_port.h"
#include "fsl_gpio.h"


#if !defined(USE_RS485_MIKROE_CLICK_8)
#define _USE_RS485_MIKROE_CLICK_8      0
#else
#define _USE_RS485_MIKROE_CLICK_8      USE_RS485_MIKROE_CLICK_8
#endif

#if !defined(USE_RS485_MIKROE_CLICK_ISO_2)
#define _USE_RS485_MIKROE_CLICK_ISO_2  0
#else
#define _USE_RS485_MIKROE_CLICK_ISO_2  USE_RS485_MIKROE_CLICK_ISO_2
#endif

#if !defined(USE_RS485_MIKROE_CLICK_3V3)
#define _USE_RS485_MIKROE_CLICK_3V3    0
#else
#define _USE_RS485_MIKROE_CLICK_3V3    USE_RS485_MIKROE_CLICK_3V3
#endif

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

#if (_USE_RS485_MIKROE_CLICK_8 >= 1)
/*************************************************************************/
/*  Click_8_RS485Setup                                                   */
/*                                                                       */
/*  Setup "RS485 8 Click" (MIKROE-5752)                                  */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void Click_8_RS485Setup (TAL_COM_DCB *pDCB)
{
   gpio_pin_config_t gpio_config;

   (void)pDCB;

   CLOCK_EnableClock(kCLOCK_Port1);
   CLOCK_EnableClock(kCLOCK_Gpio1);
   
   /*
    * Enable RS485 support for a MIKROE-5752 "RS485 8 Click" board.
    * Here a THVD1426 Transceiver with Auto-direction Control is used.
    *
    * For the Auto-direction the "EN" pin (P1_3 RST) must be set to HIGH.
    */
   gpio_config.pinDirection = kGPIO_DigitalOutput;
   gpio_config.outputLogic  = 1;  /* Configure pin to high */
   GPIO_PinInit(GPIO1, 3, &gpio_config);

} /* Click_8_RS485Setup */
#endif /* (_USE_RS485_MIKROE_CLICK_8 >= 1) */

#if (_USE_RS485_MIKROE_CLICK_ISO_2 >= 1)
/*************************************************************************/
/*  Click_Iso_2_RS485TxEnable                                            */
/*                                                                       */
/*  Enable TX in RS485 mode.                                             */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void Click_Iso_2_RS485TxEnable (void)
{
   /*
    * Receiver Enable   nRE  = P1_3
    * Driver Enable      DE  = P3_23
    */
    
   /* Disable RX, set pin to HIGH */ 
   GPIO1->PSOR = (1<<3);    
   
   /* Enable TX, set pin to HIGH */
   GPIO3->PSOR = (1<<23);

} /* Click_Iso_2_RS485TxEnable */

/*************************************************************************/
/*  Click_Iso_2_RS485TxDisable                                           */
/*                                                                       */
/*  Disable TX in RS485 mode.                                            */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void Click_Iso_2_RS485TxDisable (void)
{
   /*
    * Receiver Enable   nRE  = P1_3
    * Driver Enable      DE  = P3_23
    */

   /* Disable TX, set pin to LOW */
   GPIO3->PCOR = (1<<23);
    
   /* Enable RX, set pin to LOW */ 
   GPIO1->PCOR = (1<<3);    

} /* Click_Iso_2_RS485TxDisable */

/*************************************************************************/
/*  Click_Iso_2_RS485Setup                                               */
/*                                                                       */
/*  Setup "RS485 Isolator 2 Click" (MIKROE-3863)                         */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void Click_Iso_2_RS485Setup (TAL_COM_DCB *pDCB)
{
   gpio_pin_config_t gpio_config;
   port_pin_config_t port_config;
   
   gpio_config.pinDirection = kGPIO_DigitalOutput;
   
   /*
    * Setup RS485 Hardware to RX only
    */
   
   CLOCK_EnableClock(kCLOCK_Port1);
   CLOCK_EnableClock(kCLOCK_Gpio1);
   
   CLOCK_EnableClock(kCLOCK_Port3);
   CLOCK_EnableClock(kCLOCK_Gpio3);
   
   /*
    * Enable RS485 support for a MIKROE-3863 "RS485 Isolator 2 Click" board.
    * Here a ADM2867E Transceiver is used.
    *
    * Receiver Enable   nRE  = P1_3
    * Driver Enable      DE  = P3_23
    * Receiver Inversion INR = P3_19
    * Driver Inversion   IND = P5_7
    *
    * Here we want only the receiver enabled and driver disabled.
    * And no receiver and driver inversion.
    */
    
   /* Configure nRE, pin P1_3 to low (active) */    
   gpio_config.outputLogic = 0;
   GPIO_PinInit(GPIO1, 3, &gpio_config);

   /* Configure DE, pin P3_23 to low (inactive) */
   gpio_config.outputLogic = 0;
   GPIO_PinInit(GPIO3, 23, &gpio_config);

   /* Configure INR, pin P3_19 to low (inactive) */
   gpio_config.outputLogic = 0;
   GPIO_PinInit(GPIO3, 19, &gpio_config);
   
   /* Configure IND, pin P5_7 to low (inactive) */
   memset(&port_config, 0x00, sizeof(port_config));
   PORT_SetPinConfig(PORT5, 7U, &port_config);
   /* Set direction to output */
   GPIO5->PDDR |= GPIO_FIT_REG((1UL << 7));
   /* Set pin to low */
   GPIO5->PCOR  = (1<<7);
   

   /*
    * Setup RS485 software mode
    */
   if (pDCB != NULL)
   {
      uint32_t param;   
      
      param = 1;
      tal_COMIoctl(pDCB, TAL_COM_IOCTL_RS485_MODE, &param);
   
      param = (uint32_t)Click_Iso_2_RS485TxEnable;
      tal_COMIoctl(pDCB, TAL_COM_IOCTL_RS485_TX_ENABLE_FUNC, &param);
   
      param = (uint32_t)Click_Iso_2_RS485TxDisable;
      tal_COMIoctl(pDCB, TAL_COM_IOCTL_RS485_TX_DISABLE_FUNC, &param);
   }      

} /* Click_Iso_2_RS485Setup */
#endif /* (_USE_RS485_MIKROE_CLICK_ISO_2 >= 1) */

#if (_USE_RS485_MIKROE_CLICK_3V3 >= 1)
/*************************************************************************/
/*  Click_3V3_RS485TxEnable                                              */
/*                                                                       */
/*  Enable TX in RS485 mode.                                             */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void Click_3V3_RS485TxEnable (void)
{
   /* Enable TX, set pin to HIGH */
   GPIO3->PSOR = (1<<19);

} /* Click_3V3_RS485TxEnable */

/*************************************************************************/
/*  Click_3V3_RS485TxDisable                                             */
/*                                                                       */
/*  Disable TX in RS485 mode.                                            */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void Click_3V3_RS485TxDisable (void)
{
   /* Disable TX, set pin to LOW */
   GPIO3->PCOR = (1<<19);

} /* Click_3V3_RS485TxDisable */

/*************************************************************************/
/*  Click_Iso_2_RS485Setup                                               */
/*                                                                       */
/*  Setup "RS485 3V3 Click" (MIKROE-989)                                 */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void Click_3V3_RS485Setup (TAL_COM_DCB *pDCB)
{
   gpio_pin_config_t gpio_config;
   
   gpio_config.pinDirection = kGPIO_DigitalOutput;
   
   /*
    * Setup RS485 Hardware to RX only
    */

   CLOCK_EnableClock(kCLOCK_Port3);
   CLOCK_EnableClock(kCLOCK_Gpio3);
   
   /*
    * Enable RS485 support for a MIKROE-989 "RS485 3V3 Click" board.
    * Here a SN75HVD12 Transceiver is used.
    *
    * Receiver/Driver Enable nRE/DE = P3_19
    *
    * Here we want only the receiver enabled and driver disabled.
    */
    
   /* Configure nRE/DE, pin P3_19 to low */
   gpio_config.outputLogic = 0;
   GPIO_PinInit(GPIO3, 19, &gpio_config);
   

   /*
    * Setup RS485 software mode
    */
   if (pDCB != NULL)
   {
      uint32_t param;   
      
      param = 1;
      tal_COMIoctl(pDCB, TAL_COM_IOCTL_RS485_MODE, &param);
   
      param = (uint32_t)Click_3V3_RS485TxEnable;
      tal_COMIoctl(pDCB, TAL_COM_IOCTL_RS485_TX_ENABLE_FUNC, &param);
   
      param = (uint32_t)Click_3V3_RS485TxDisable;
      tal_COMIoctl(pDCB, TAL_COM_IOCTL_RS485_TX_DISABLE_FUNC, &param);
   }      

} /* Click_3V3_RS485Setup */
#endif /* (_USE_RS485_MIKROE_CLICK_3V3 >= 1) */

/*=======================================================================*/
/*  All code exported                                                    */
/*=======================================================================*/

/*************************************************************************/
/*  mikroe_RS485Setup                                                    */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void mikroe_RS485Setup (TAL_COM_DCB *pDCB)
{
   static int SetupDone = 0;

#if (_USE_RS485_MIKROE_CLICK_8 >= 1)
   /*
    * RS485 8 Click
    */
   if (0 == SetupDone)
   {
      SetupDone = 1;
      Click_8_RS485Setup(pDCB);
   }      
#endif 


#if (_USE_RS485_MIKROE_CLICK_ISO_2 >= 1)
   /*
    * RS485 Isolator 2 Click
    */
   if (0 == SetupDone)
   {
      SetupDone = 1;
      Click_Iso_2_RS485Setup(pDCB);
   }      
#endif


#if (_USE_RS485_MIKROE_CLICK_3V3 >= 1)
   /*
    * RS485 3V3 Click
    */
   if (0 == SetupDone)
   {
      SetupDone = 1;
      Click_3V3_RS485Setup(pDCB);
   }      
#endif

   (void)SetupDone;
   (void)pDCB;

} /* mikroe_RS485Setup */

/*** EOF ***/


