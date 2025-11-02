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
#define __TALCPU_COM_C__

/*=======================================================================*/
/*  Include                                                              */
/*=======================================================================*/
#include "tal.h"
#include "mcx_cpu.h"

#include "fsl_lpuart.h"

/*=======================================================================*/
/*  All Structures and Common Constants                                  */
/*=======================================================================*/

#define USART0_IRQHandler   LP_FLEXCOMM0_IRQHandler
#define USART1_IRQHandler   LP_FLEXCOMM1_IRQHandler
#define USART2_IRQHandler   LP_FLEXCOMM2_IRQHandler
#define USART3_IRQHandler   LP_FLEXCOMM3_IRQHandler
#define USART4_IRQHandler   LP_FLEXCOMM4_IRQHandler
#define USART5_IRQHandler   LP_FLEXCOMM5_IRQHandler
#define USART6_IRQHandler   LP_FLEXCOMM6_IRQHandler
#define USART7_IRQHandler   LP_FLEXCOMM7_IRQHandler
#define USART8_IRQHandler   LP_FLEXCOMM8_IRQHandler
#define USART9_IRQHandler   LP_FLEXCOMM9_IRQHandler

/*=======================================================================*/
/*  Definition of all local Data                                         */
/*=======================================================================*/

static TAL_COM_DCB *DCBArray[TAL_COM_PORT_MAX];

/*=======================================================================*/
/*  Definition of all local Procedures                                   */
/*=======================================================================*/

/*************************************************************************/
/*  RS485TxEnable                                                        */
/*                                                                       */
/*  Enable RS485 TX mode.                                                */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void RS485TxEnable (TAL_COM_DCB *pDCB)
{
   if (pDCB->HW.RS485TXEnable != NULL)
   {
      pDCB->HW.RS485TXEnable();
   }

} /* RS485TxEnable */

/*************************************************************************/
/*  RS485TxDisable                                                       */
/*                                                                       */
/*  Disable RS485 TX mode.                                               */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void RS485TxDisable (TAL_COM_DCB *pDCB)
{
   uint32_t     dSTAT;
   TAL_COM_HW  *pHW    = &pDCB->HW;
   LPUART_Type *pUARTx = (LPUART_Type*)pHW->dBaseAddress;

   if (pDCB->HW.RS485TXDisable != NULL)
   {
      /* Wait for Transmitter idle, check STAT, TC must be 1 */
      dSTAT = pUARTx->STAT;
      while (0 == (dSTAT & LPUART_STAT_TC_MASK))
      {
         dSTAT = pUARTx->STAT;
      }   

      pDCB->HW.RS485TXDisable();
   }

} /* RS485TxEnable */

/*************************************************************************/
/*  IRQHandler                                                           */
/*                                                                       */
/*  This is the generic IRQ handler.                                     */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void IRQHandler (TAL_COM_DCB *pDCB)
{
   TAL_RESULT          Error;
   TAL_COM_HW        *pHW    = &pDCB->HW;
   LPUART_Type       *pUARTx = (LPUART_Type*)pHW->dBaseAddress;
   uint8_t           *pData;
   uint8_t            bData;
   TAL_COM_OBJECT_TIME TimeData;

   /*
    * RX Interrupt
    */
   if (kLPUART_RxDataRegFullFlag & LPUART_GetStatusFlags(pUARTx))
   {
      bData = (uint8_t)pUARTx->DATA;
      
      /* If we have no overflow... */
      if (TAL_FALSE == pDCB->bRxOverflow)
      {
         if      (TAL_COM_BUFFER_RX == pDCB->eBufferRx)
         {
            pData = &bData;
         }
         else if (TAL_COM_BUFFER_RX_TIME == pDCB->eBufferRx)
         {
            if (pDCB->GetRxTimestamp != NULL)
            {
               TimeData.dTimestamp = pDCB->GetRxTimestamp();
            }
            else
            {
               TimeData.dTimestamp = 0;
            }   
            TimeData.dData = bData;
         
            pData = (uint8_t*)&TimeData;
         }
         else
         {
            pData = NULL;
         }
         
         if (pData != NULL)
         {
            /* ... put it into the ring buffer */
            Error = tal_MISCRingAdd(&pDCB->RxRing, pData);
            if (TAL_OK == Error)
            {
               /* Signal counting semaphore */
               OS_SemaSignalFromInt(&pDCB->RxRdySema);
            }
            else
            {
               /* Ups, overflow */
               pDCB->bRxOverflow = TAL_OK;
            }
         }            
      }
   } /* end RX interrupt */      


   /* 
    * Check for TX interrupt, but only if enabled 
    */
   if ((LPUART_GetEnabledInterrupts(pUARTx) & kLPUART_TxDataRegEmptyInterruptEnable) &&  /* <= enabled ? */
       (kLPUART_TxDataRegEmptyFlag & LPUART_GetStatusFlags(pUARTx)))                     /* <= TX interrupt ? */
   {
      /* Read Data from the ring buffer */
      Error = tal_MISCRingGet(&pDCB->TxRing, &bData);
      if (Error != TAL_OK)
      {
         /* Ups, no data available, disable interrupt */
         LPUART_DisableInterrupts(pUARTx, kLPUART_TxDataRegEmptyInterruptEnable);
         
         /* Disable RS485 TX mode */
         RS485TxDisable(pDCB);
      }
      else
      {
         /* Send data */
         pUARTx->DATA = bData;
      }   
   } /* end "TX interrupt */
   
} /* IRQHandler */

/*=======================================================================*/
/*  All code exported                                                    */
/*=======================================================================*/

/*************************************************************************/
/*  LPUARTx_IRQHandler                                                   */
/*                                                                       */
/*  This is the Cortex USARTx IRQ handler.                               */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/

void USART0_IRQHandler (void)
{
   TAL_CPU_IRQ_ENTER();
   IRQHandler(DCBArray[TAL_COM_PORT_1]);
   TAL_CPU_IRQ_EXIT();

   SDK_ISR_EXIT_BARRIER;
} /* USART0_IRQHandler */

void USART4_IRQHandler (void)
{
   TAL_CPU_IRQ_ENTER();
   IRQHandler(DCBArray[TAL_COM_PORT_5]);
   TAL_CPU_IRQ_EXIT();

   SDK_ISR_EXIT_BARRIER;
} /* USART4_IRQHandler */

void USART5_IRQHandler (void)
{
   TAL_CPU_IRQ_ENTER();
   IRQHandler(DCBArray[TAL_COM_PORT_6]);
   TAL_CPU_IRQ_EXIT();

   SDK_ISR_EXIT_BARRIER;
} /* USART5_IRQHandler */    

void USART8_IRQHandler (void)
{
   TAL_CPU_IRQ_ENTER();
   IRQHandler(DCBArray[TAL_COM_PORT_9]);
   TAL_CPU_IRQ_EXIT();

   SDK_ISR_EXIT_BARRIER;
} /* USART8_IRQHandler */    

/*************************************************************************/
/*  cpu_COMInit                                                          */
/*                                                                       */
/*  Prepare the hardware for use by the Open function later. Set the HW  */
/*  information depending of ePort and "enable" the COM port.            */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: TAL_OK / error cause                                         */
/*************************************************************************/
TAL_RESULT cpu_COMInit (TAL_COM_DCB *pDCB)
{
   TAL_RESULT    Error = TAL_ERR_COM_PORT_RANGE;
   TAL_COM_HW  *pHW    = &pDCB->HW;

   /* Set default values for RS485 */
   pHW->bRS485Mode     = FALSE;
   pHW->RS485TXEnable  = NULL;
   pHW->RS485TXDisable = NULL;
   
   switch (pDCB->ePort)
   {
      case TAL_COM_PORT_1:
      {
         Error = tal_BoardEnableCOM1();
         if (TAL_OK == Error)
         {
            DCBArray[TAL_COM_PORT_1] = pDCB;
            
            pHW->dBaseAddress = LPUART0_BASE;   /* Start count by 0 */
            pHW->nIrqNumber   = LP_FLEXCOMM0_IRQn;
            pHW->nIrqPriority = LPUART0_PRIO;
            pHW->dFlexCommID  = 0;

            /* Reset USART */
            LPUART_SoftwareReset((LPUART_Type*)pHW->dBaseAddress);
            
            /* Set irq and priority */
            tal_CPUIrqSetPriority(pHW->nIrqNumber, pHW->nIrqPriority);
         }
         break;
      } /* TAL_COM_PORT_1 */
   
      case TAL_COM_PORT_5:
      {
         Error = tal_BoardEnableCOM5();
         if (TAL_OK == Error)
         {
            DCBArray[TAL_COM_PORT_5] = pDCB;
            
            pHW->dBaseAddress = LPUART4_BASE;   /* Start count by 0 */
            pHW->nIrqNumber   = LP_FLEXCOMM4_IRQn;
            pHW->nIrqPriority = LPUART4_PRIO;
            pHW->dFlexCommID  = 4;

            /* Reset USART */
            LPUART_SoftwareReset((LPUART_Type*)pHW->dBaseAddress);
            
            /* Set irq and priority */
            tal_CPUIrqSetPriority(pHW->nIrqNumber, pHW->nIrqPriority);
         }
         break;
      } /* TAL_COM_PORT_5 */
      
      case TAL_COM_PORT_6:
      {
         Error = tal_BoardEnableCOM6();
         if (TAL_OK == Error)
         {
            DCBArray[TAL_COM_PORT_6] = pDCB;
            
            pHW->dBaseAddress = LPUART5_BASE;   /* Start count by 0 */
            pHW->nIrqNumber   = LP_FLEXCOMM5_IRQn;
            pHW->nIrqPriority = LPUART5_PRIO;
            pHW->dFlexCommID  = 5;

            /* Reset USART */
            LPUART_SoftwareReset((LPUART_Type*)pHW->dBaseAddress);
            
            /* Set irq and priority */
            tal_CPUIrqSetPriority(pHW->nIrqNumber, pHW->nIrqPriority);
         }
         break;
      } /* TAL_COM_PORT_6 */
      
      case TAL_COM_PORT_9:
      {
         Error = tal_BoardEnableCOM9();
         if (TAL_OK == Error)
         {
            DCBArray[TAL_COM_PORT_9] = pDCB;
            
            pHW->dBaseAddress = LPUART8_BASE;   /* Start count by 0 */
            pHW->nIrqNumber   = LP_FLEXCOMM8_IRQn;
            pHW->nIrqPriority = LPUART8_PRIO;
            pHW->dFlexCommID  = 8;

            /* Reset USART */
            LPUART_SoftwareReset((LPUART_Type*)pHW->dBaseAddress);
            
            /* Set irq and priority */
            tal_CPUIrqSetPriority(pHW->nIrqNumber, pHW->nIrqPriority);
         }
         break;
      } /* TAL_COM_PORT_9 */
         
      default:
      {
         /* Do nothing */
         break;
      }
   } /* end switch (pDCB->ePort) */
   
   return(Error);
} /* cpu_COMInit */

/*************************************************************************/
/*  cpu_COMIoctl                                                         */
/*                                                                       */
/*  Call a IOCTL function.                                               */
/*                                                                       */
/*  In    : pDCB, wNum, pParam                                           */
/*  Out   : none                                                         */
/*  Return: TAL_OK / error cause                                         */
/*************************************************************************/
TAL_RESULT cpu_COMIoctl (TAL_COM_DCB *pDCB, TAL_COM_IOCTL eFunc, uint32_t *pParam)
{
   TAL_RESULT Error = TAL_OK;

   switch (eFunc)
   {
      case TAL_COM_IOCTL_RS485_MODE:
      {
         pDCB->HW.bRS485Mode = (uint8_t)*pParam;
         break;
      }

      case TAL_COM_IOCTL_RS485_TX_ENABLE_FUNC:
      {
         pDCB->HW.RS485TXEnable = (void(*)(void))*pParam;
         break;
      }

      case TAL_COM_IOCTL_RS485_TX_DISABLE_FUNC:
      {
         pDCB->HW.RS485TXDisable = (void(*)(void))*pParam;
         break;
      }

      default:
      {
         Error = TAL_ERR_COM_IOCTL_NUM;
         break;
      }
   } /* end switch (wNum) */

   return(Error);
} /* cpu_COMIoctl */

/*************************************************************************/
/*  cpu_COMOpen                                                          */
/*                                                                       */
/*  Open the COM port.                                                   */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: TAL_OK / error cause                                         */
/*************************************************************************/
TAL_RESULT cpu_COMOpen (TAL_COM_DCB *pDCB)
{
   TAL_RESULT        Error = TAL_ERROR;
   TAL_COM_HW      *pHW    = &pDCB->HW;
   LPUART_Type     *pUARTx = (LPUART_Type*)pHW->dBaseAddress;
   uint32_t         dUartClock;
   lpuart_config_t   config; 
   status_t          status;

   LPUART_GetDefaultConfig(&config);

   if( (TAL_COM_PORT_1 == pDCB->ePort) ||
       (TAL_COM_PORT_9 == pDCB->ePort) )
   {
      config.periph =  LP_FLEXCOMM_PERIPH_LPI2CAndLPUART;
   }

   /* 
    * Check parameter first 
    */

   /* Check word length */
   switch (pDCB->Settings.eLength)
   {
      case TAL_COM_LENGTH_8:
      {
         config.dataBitsCount = kLPUART_EightDataBits;
         break;
      }
      
      default:
      {
         Error = TAL_ERR_COM_LENGTH;
         goto COMOpenEnd;  /*lint !e801*/
         break;   /*lint !e527*/
      }
   } /* switch (pDCB->Settings.eLength) */
   
   /* Check parity settings */
   switch (pDCB->Settings.eParity)
   {
      case TAL_COM_PARITY_NONE:
      {
         config.parityMode = kLPUART_ParityDisabled; 
         break;
      } 
      
      case TAL_COM_PARITY_EVEN:
      {
         config.parityMode = kLPUART_ParityEven; 
         break;
      }
      
      case TAL_COM_PARITY_ODD:
      {
         config.parityMode = kLPUART_ParityOdd; 
         break;
      }
      
      default:
      {
         Error = TAL_ERR_COM_PARITY;
         goto COMOpenEnd;  /*lint !e801*/
         break;   /*lint !e527*/
      }
   } /* switch (pDCB->Settings.eParity) */
    
   /* Check stop bit settings */
   switch (pDCB->Settings.eStop)
   {
      case TAL_COM_STOP_1_0:
      {
         config.stopBitCount = kLPUART_OneStopBit;
         break;
      }
      
      default:
      {
         Error = TAL_ERR_COM_STOP;
         goto COMOpenEnd;  /*lint !e801*/
         break;   /*lint !e527*/
      }
   } /* switch (pDCB->Settings.eStop) */

   /* Check baud rate */
   if (pDCB->Settings.dBaudrate != 0)
   {
      config.baudRate_Bps = pDCB->Settings.dBaudrate;
   }
   else
   {
      Error = TAL_ERR_COM_BAUDRATE;
      goto COMOpenEnd;  /*lint !e801*/
   }
   
   /* 
    * To make it simple, we assume default PLL and divider settings,
    * and the only variable from application is use PLL3 source or OSC source.
    */
   dUartClock = CLOCK_GetLPFlexCommClkFreq(pHW->dFlexCommID); 

   /*
    * Initializes an UART instance with the user configuration structure and the peripheral clock.
    */
   config.txFifoWatermark = 0;

   status = LPUART_Init(pUARTx, &config, dUartClock);
   if (kStatus_Success == status)
   {
      /* Enable RX interrupt. */
      LPUART_EnableInterrupts(pUARTx, kLPUART_RxDataRegFullInterruptEnable);

      /* Enable the interrupt */
      tal_CPUIrqEnable(pHW->nIrqNumber);         
            
      LPUART_EnableTx(pUARTx, (bool)1);
      LPUART_EnableRx(pUARTx, (bool)1);

      /* Disable RS485 TX => Enable RX */
      RS485TxDisable(pDCB);
      
      Error = TAL_OK;
   }   

COMOpenEnd:   
   return(Error);
} /* cpu_COMOpen */

/*************************************************************************/
/*  cpu_COMClose                                                         */
/*                                                                       */
/*  Close the COM port.                                                  */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: TAL_OK / error cause                                         */
/*************************************************************************/
TAL_RESULT cpu_COMClose (TAL_COM_DCB *pDCB)
{
   TAL_RESULT    Error = TAL_OK;
   TAL_COM_HW  *pHW    = &pDCB->HW;
   LPUART_Type *pUARTx = (LPUART_Type*)pHW->dBaseAddress;

   /* Disable UART Module */
   pUARTx->CTRL = 0;
   
   /* Disable the RS485 TX mode */
   RS485TxDisable(pDCB);
   
   /* Reset USART */
   LPUART_SoftwareReset((LPUART_Type*)pHW->dBaseAddress);

   /* Disable the interrupt in the GIC. */
   tal_CPUIrqDisable(pHW->nIrqNumber);
   
   return(Error);
} /* cpu_COMClose */

/*************************************************************************/
/*  cpu_COMStartTx                                                       */
/*                                                                       */
/*  Send the data from the ring buffer if the TX interrupt is disabled.  */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: TAL_OK / error cause                                         */
/*************************************************************************/
TAL_RESULT cpu_COMStartTx (TAL_COM_DCB *pDCB)
{
   TAL_RESULT    Error = TAL_OK;
   TAL_COM_HW  *pHW    = &pDCB->HW;
   LPUART_Type *pUARTx = (LPUART_Type*)pHW->dBaseAddress;
   uint8_t      bData;

   TAL_CPU_DISABLE_ALL_INTS();
   if (LPUART_GetEnabledInterrupts(pUARTx) & kLPUART_TxDataRegEmptyInterruptEnable)
   {
      /* TX interrupt is enabled, do nothing */
   }
   else
   {
      /* Get data from the ring buffer */
      Error = tal_MISCRingGet(&pDCB->TxRing, &bData);
      if (TAL_OK == Error)
      {
         /* Enable RS485 TX mode */
         RS485TxEnable(pDCB);
      
         /* Send data */
         pUARTx->DATA = bData;
      
         /* Enable TX interrupt */
         LPUART_EnableInterrupts(pUARTx, kLPUART_TxDataRegEmptyInterruptEnable);
      }
   }
   TAL_CPU_ENABLE_ALL_INTS();
   
   return(Error);
} /* cpu_COMStartTx */

/*************************************************************************/
/*  cpu_COMTxIsRunning                                                   */
/*                                                                       */
/*  Check if TX is still running.                                        */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: TAL_OK / TAL_ERROR                                           */
/*************************************************************************/
TAL_RESULT cpu_COMTxIsRunning (TAL_COM_DCB *pDCB)
{
   TAL_RESULT    Error = TAL_OK;
   TAL_COM_HW  *pHW    = &pDCB->HW;
   LPUART_Type *pUARTx = (LPUART_Type*)pHW->dBaseAddress;

   TAL_CPU_DISABLE_ALL_INTS();
   if (LPUART_GetEnabledInterrupts(pUARTx) & kLPUART_TxDataRegEmptyInterruptEnable)
   {
      /* TX is still running */
      Error = TAL_OK;
   }
   else
   {
      /* TX is not running */
      Error = TAL_ERROR;
   }
   TAL_CPU_ENABLE_ALL_INTS();
   
   return(Error);
} /* cpu_COMTxIsRunning */

/*************************************************************************/
/*  cpu_COMSendStringASS                                                 */
/*                                                                       */
/*  Send the assertion string.                                           */
/*                                                                       */
/*  In    : pDCB, pString                                                */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void cpu_COMSendStringASS (TAL_COM_DCB *pDCB, char *pString)
{
   TAL_COM_HW  *pHW    = &pDCB->HW;
   LPUART_Type *pUARTx = (LPUART_Type*)pHW->dBaseAddress;

   TAL_CPU_DISABLE_ALL_INTS();
   
#if 1   
   if (pString != NULL)
   {   
      LPUART_WriteBlocking(pUARTx, (uint8_t*)pString, strlen(pString));
   } 
#else        
   while (*pString != 0)
   {
      /* Loop until the end of transmit */ 
      while (0 == (pUARTx->STAT & LPUART_STAT_TDRE_MASK))
      {
      }

      /* Write one byte to the transmit data register */   
      pUARTx->DATA = *pString;
      
      pString++;
   }
#endif   
   
   TAL_CPU_ENABLE_ALL_INTS();
} /* cpu_COMSendStringASS */  

/*** EOF ***/
