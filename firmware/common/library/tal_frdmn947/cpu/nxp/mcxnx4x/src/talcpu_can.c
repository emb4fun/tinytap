/**************************************************************************
*  This file is part of the TAL project (Tiny Abstraction Layer)
*
*  Copyright (c) 2020-2025 by Michael Fischer (www.emb4fun.de).
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
#define __TALCPU_CAN_C__

/*=======================================================================*/
/*  Include                                                              */
/*=======================================================================*/
#include <string.h>
#include "tal.h"
#include "mcx_cpu.h"

#include "fsl_flexcan.h"

/*=======================================================================*/
/*  All Structures and Common Constants                                  */
/*=======================================================================*/

/*
 * We will want to work with 32 filters, not more and not less.
 * For this 32 filters, 8 mailboxes must be used. That mean in
 * total 14 mailboxes (6 FIFO + 8 Filter) are needed.
 *
 * Mailbox 0 - 13 FIFO & Filter
 * Mailbox 14     Spare
 * Mailbox 15     TX-Mailbox
 *
 * Total mailbox count 16
 */
#if (CAN_MAX_FILTER_COUNT != 32)
   Error: CAN_MAX_FILTER_COUNT must be defined to 32;
#endif    


#define CAN_CLK_FREQ             (CLOCK_GetFlexcanClkFreq(0U))

/*
 * Filter defines
 */
#define FILTER_NOT_USED          (0x7FFFFFFF)
#define FILTER_MASK_ALL_BITS     (0x7FFFFFFF)

#define FILTER_ID_MASK           (0x1FFFFFFF)
#define FILTER_IN_USE_FLAG       (0x80000000)
#define FILTER_IS_EXT_FLAG       (0x40000000)
#define FILTER_IS_ALL_FLAG       (0x20000000)

#define FILTER_ID(_x)            (_x & FILTER_ID_MASK)
#define IS_FILTER_IN_USE(_x)     (_x & FILTER_IN_USE_FLAG)
#define IS_FILTER_EXT(_x)        (_x & FILTER_IS_EXT_FLAG)
#define IS_FILTER_ALL(_x)        (_x & FILTER_IS_ALL_FLAG)

#define TX_MBOX_INDEX            (15)
#define TX_MBOX_IMASK            (1<<TX_MBOX_INDEX)

#define CAN_STD_ENTRY_ID_MASK    (0x7FF)
#define CAN_EXT_ENTRY_ID_MASK    (0x1FFFFFFF)


#define STAT_INC(_x)             if (_x < UINT32_MAX) _x++


/*! @brief FlexCAN Internal State. */
enum _flexcan_state
{
   kFLEXCAN_StateIdle     = 0x0, /*!< MB/RxFIFO idle.*/
   kFLEXCAN_StateRxData   = 0x1, /*!< MB receiving.*/
   kFLEXCAN_StateRxRemote = 0x2, /*!< MB receiving remote reply.*/
   kFLEXCAN_StateTxData   = 0x3, /*!< MB transmitting.*/
   kFLEXCAN_StateTxRemote = 0x4, /*!< MB transmitting remote request.*/
   kFLEXCAN_StateRxFifo   = 0x5  /*!< RxFIFO receiving.*/
};

/*=======================================================================*/
/*  Definition of all local Data                                         */
/*=======================================================================*/

static TAL_CAN_DCB     *DCBArray[TAL_CAN_PORT_MAX];
static flexcan_handle_t HandleArray[TAL_CAN_PORT_MAX];

/*=======================================================================*/
/*  Definition of all local Procedures                                   */
/*=======================================================================*/

/*************************************************************************/
/*  CheckUnhandleInterruptEvents                                         */
/*                                                                       */
/*  Check unhandle interrupt events.                                     */
/*                                                                       */
/*  Return TRUE if unhandled interrupt action exist, FALSE if no         */
/*  unhandlered interrupt action exist.                                  */
/*                                                                       */
/*  In    : base                                                         */
/*  Out   : none                                                         */
/*  Return: true / false                                                 */
/*************************************************************************/
static bool CheckUnhandleInterruptEvents (CAN_Type *base)
{
   uint64_t tempmask;
   uint64_t tempflag;
   bool     ret = false;

   /* Checking exist error flag. */
   if (0U == (FLEXCAN_GetStatusFlags(base) &
              ((uint32_t)kFLEXCAN_TxWarningIntFlag | (uint32_t)kFLEXCAN_RxWarningIntFlag |
               (uint32_t)kFLEXCAN_BusOffIntFlag    | (uint32_t)kFLEXCAN_ErrorIntFlag     | (uint32_t)kFLEXCAN_WakeUpIntFlag)))
   {
      tempmask = (uint64_t)base->IMASK1;
      tempflag = (uint64_t)base->IFLAG1;

#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
      /* Checking whether exist MB interrupt status and legacy RX FIFO interrupt status. */
      tempmask |= ((uint64_t)base->IMASK2) << 32;
      tempflag |= ((uint64_t)base->IFLAG2) << 32;
#endif
      ret = (0U != (tempmask & tempflag));
   }
   else
   {
      ret = true;
   }

   return(ret);
} /* CheckUnhandleInterruptEvents */

/*************************************************************************/
/*  UpdateAcceptanceFilter                                               */
/*                                                                       */
/*  Update the acceptance filter.                                        */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static TAL_RESULT UpdateAcceptanceFilter (TAL_CAN_DCB *pDCB)
{
   uint8_t    bIndex;
   uint32_t  *FilterArray = pDCB->HW.FilterArray;
   CAN_Type  *pCANx;
   uint32_t  *pFilterBase;
   uint32_t   dIdentifier;
   uint32_t   dFilterValue;

   /* Get CAN pointer */
   pCANx = (CAN_Type*)pDCB->HW.dBaseAddress;

   /* Enter Freeze Mode. */
   FLEXCAN_EnterFreezeMode(pCANx);
   
   /*lint -save -e661 -e662 -e831 */

   /* Get filter base */
   pFilterBase = (uint32_t*)&(pCANx->MB[6].CS);
   
   /* Setup the complete filter */
   for (bIndex = 0; bIndex < CAN_MAX_FILTER_COUNT; bIndex++)
   {
      if (IS_FILTER_IN_USE(FilterArray[bIndex]))
      {
         dIdentifier = FILTER_ID(FilterArray[bIndex]);
      
         if (IS_FILTER_EXT(FilterArray[bIndex]))
         {
            dFilterValue = FLEXCAN_RX_FIFO_EXT_MASK_TYPE_A(dIdentifier, 0, 1);
         }
         else
         {
            dFilterValue = FLEXCAN_RX_FIFO_STD_MASK_TYPE_A(dIdentifier, 0, 0);
         }
         
         pFilterBase[bIndex] = dFilterValue;
         
         /* Check if it is an ALL filter */
         if (IS_FILTER_ALL(FilterArray[bIndex]))
         {
            /* Check only the IDE bit */
            pCANx->RXIMR[bIndex] = 0x40000000;
         }
         else
         {
            pCANx->RXIMR[bIndex] = FILTER_MASK_ALL_BITS;
         }
      }
      else
      {
         pFilterBase[bIndex]  = FILTER_NOT_USED;
         pCANx->RXIMR[bIndex] = FILTER_MASK_ALL_BITS;
      }         
   }

   /*lint -restore */
   
   /* Exit Freeze Mode. */
   FLEXCAN_ExitFreezeMode(pCANx);

   return(TAL_OK);
} /* UpdateAcceptanceFilter */

/*************************************************************************/
/*  CANIdentRegister                                                     */
/*                                                                       */
/*  Register a CAN identifier in the acceptance filter.                  */
/*                                                                       */
/*  In    : pDCB, dIdentifier, Type                                      */
/*  Out   : none                                                         */
/*  Return: TAL_OK / error cause                                         */
/*************************************************************************/
static TAL_RESULT CANIdentRegister (TAL_CAN_DCB   *pDCB,
                                    uint32_t       dIdentifier,
                                    TAL_CAN_TYPE_ID Type)
{
   TAL_RESULT  Error = TAL_ERROR;
   uint16_t   wIndex;
   uint32_t  *FilterArray = pDCB->HW.FilterArray;
   uint8_t    bRegsiterOK = TAL_FALSE;
   uint32_t   dAllFlag    = 0;
   
   
   /* 
    * Handle special case TAL_CAN_ID_RX_ALL, all identifier
    * from type STD or EXT should be received.
    */
   if (TAL_CAN_ID_RX_ALL == dIdentifier)
   {
      dIdentifier = 0;
      dAllFlag    = FILTER_IS_ALL_FLAG;
   } /* end if (TAL_CAN_ID_RX_ALL == dIdentifier) */


   /*
    * Check for STD or EXT identifier
    */   
   if (TAL_CAN_TYPE_STD_ID == Type)
   {
      /* Check valid range */
      if (dIdentifier > CAN_STD_ENTRY_ID_MASK)
      {
         Error = TAL_ERR_CAN_ID_RANGE;
         goto IdentRegisterEnd;  /*lint !e801*/
      } 
   }
   else
   {
      /* Check valid range */
      if (dIdentifier > CAN_EXT_ENTRY_ID_MASK)
      {
         Error = TAL_ERR_CAN_ID_RANGE;
         goto IdentRegisterEnd;  /*lint !e801*/
      }

      dIdentifier |= FILTER_IS_EXT_FLAG; 
   }
   
   dIdentifier |= FILTER_IN_USE_FLAG;
   dIdentifier |= dAllFlag;
   
   /*
    * Check if this identifier is registered
    */
   for (wIndex = 0; wIndex < CAN_MAX_FILTER_COUNT; wIndex++)
   {
      if (FilterArray[wIndex] == dIdentifier)
      {
         Error = TAL_ERR_CAN_ID_USED;
         goto IdentRegisterEnd;  /*lint !e801*/
      }
   }
   
   
   /*
    * At this point the identifier is not registered.
    * Find a free entry which can be used.
    */
   for (wIndex = 0; wIndex < CAN_MAX_FILTER_COUNT; wIndex++)
   {
      if (0 == IS_FILTER_IN_USE(FilterArray[wIndex]))
      {
         FilterArray[wIndex] = dIdentifier;
         bRegsiterOK         = TAL_TRUE;
         break;
      }
   }      
   if (TAL_FALSE == bRegsiterOK)
   {
      Error = TAL_ERR_CAN_ID_NO_ENTRY;
      goto IdentRegisterEnd;  /*lint !e801*/
   }


   /*
    * At this point an update of the Acceptance Filter is needed
    */
   Error = UpdateAcceptanceFilter(pDCB);    
   
IdentRegisterEnd:   
   return(Error);
} /* CANIdentRegister */   

/*************************************************************************/
/*  CANIdentDeRegister                                                   */
/*                                                                       */
/*  DeRegister a CAN identifier from the acceptance filter.              */
/*                                                                       */
/*  In    : pDCB, dIdentifier, Type                                      */
/*  Out   : none                                                         */
/*  Return: TAL_OK / error cause                                         */
/*************************************************************************/
static TAL_RESULT CANIdentDeRegister (TAL_CAN_DCB   *pDCB,
                                      uint32_t       dIdentifier,
                                      TAL_CAN_TYPE_ID Type)
{
   TAL_RESULT  Error = TAL_ERROR;     
   uint16_t   wIndex;
   uint32_t  *FilterArray   = pDCB->HW.FilterArray;
   uint8_t    bDeRegsiterOK = TAL_FALSE;
   uint32_t   dAllFlag      = 0;
  

   /* 
    * Handle special case TAL_CAN_ID_RX_ALL, all identifier
    * from type STD or EXT should be received.
    */
   if (TAL_CAN_ID_RX_ALL == dIdentifier)
   {
      dIdentifier = 0;
      dAllFlag    = FILTER_IS_ALL_FLAG;
   } /* end if (TAL_CAN_ID_RX_ALL == dIdentifier) */

  
   /*
    * Check for STD or EXT identifier
    */   
   if (TAL_CAN_TYPE_STD_ID == Type)
   {
      /* Check valid range */
      if (dIdentifier > CAN_STD_ENTRY_ID_MASK)
      {
         Error = TAL_ERR_CAN_ID_RANGE;
         goto IdentDeRegisterEnd;   /*lint !e801*/
      }
   }
   else
   {
      /* Check valid range */
      if (dIdentifier > CAN_EXT_ENTRY_ID_MASK)
      {
         Error = TAL_ERR_CAN_ID_RANGE;
         goto IdentDeRegisterEnd;   /*lint !e801*/
      }
      
      dIdentifier |= FILTER_IS_EXT_FLAG; 
   }
   
   dIdentifier |= FILTER_IN_USE_FLAG;
   dIdentifier |= dAllFlag;
   
   /*
    * Check if this identifier is registered,
    * and remove it from the list if available.
    */
   for (wIndex = 0; wIndex < CAN_MAX_FILTER_COUNT; wIndex++)
   {
      if (FilterArray[wIndex] == dIdentifier)
      {
         FilterArray[wIndex] = 0;
         bDeRegsiterOK       = TAL_TRUE;
         break;
      }
   }
   if (TAL_FALSE == bDeRegsiterOK)
   {
      Error = TAL_ERR_CAN_ID_NOT_USED;
      goto IdentDeRegisterEnd;   /*lint !e801*/
   }

   /*
    * At this point an update of the Acceptance Filter is needed
    */
   Error = UpdateAcceptanceFilter(pDCB);    

IdentDeRegisterEnd:   
   return(Error);
} /* CANIdentDeRegister */   

/*************************************************************************/
/*  SendObject                                                           */
/*                                                                       */
/*  Try to send the given Object.                                        */
/*                                                                       */
/*  In    : pDCB, pCANx, pCANObject                                      */
/*  Out   : none                                                         */
/*  Return: TAL_OK                                                       */
/*************************************************************************/
static void SendObject (TAL_CAN_DCB *pDCB, CAN_Type *pCANx, TAL_CAN_OBJECT *pCANObject)
{
   static flexcan_frame_t frame; /* Use static to putting frame in bss and not at local stack */

   if ((uint8_t)kFLEXCAN_StateIdle == pDCB->HW.pHandle->mbState[TX_MBOX_INDEX])
   {
      pDCB->HW.pHandle->mbState[TX_MBOX_INDEX] = (uint8_t)kFLEXCAN_StateTxData;
      
      memset(&frame, 0x00, sizeof(frame));
      
      /* Convert CANObject to frame */
      if (pCANObject->bFlags & TAL_CAN_OBJECT_FLAGS_STD_ID)
      {
         frame.id     = FLEXCAN_ID_STD(pCANObject->dIdentifier);
         frame.format = (uint8_t)kFLEXCAN_FrameFormatStandard;
      }
      else
      {
         frame.id     = FLEXCAN_ID_EXT(pCANObject->dIdentifier);
         frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
      }
      frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
      frame.length = (uint8_t)pCANObject->bDLC;
      
      frame.dataByte0 = pCANObject->Data[0];
      frame.dataByte1 = pCANObject->Data[1];
      frame.dataByte2 = pCANObject->Data[2];
      frame.dataByte3 = pCANObject->Data[3];
      frame.dataByte4 = pCANObject->Data[4];
      frame.dataByte5 = pCANObject->Data[5];
      frame.dataByte6 = pCANObject->Data[6];
      frame.dataByte7 = pCANObject->Data[7];

      /* Send frame */      
      if (kStatus_Success == FLEXCAN_WriteTxMb(pCANx, TX_MBOX_INDEX, &frame))
      {
         /* Enable Message Buffer Interrupt. */
         FLEXCAN_EnableMbInterrupts(pCANx, (uint64_t)TX_MBOX_IMASK);
      }
      else
      {
         pDCB->HW.pHandle->mbState[TX_MBOX_INDEX] = (uint8_t)kFLEXCAN_StateIdle;
      }      
   }
   
} /* SendObject */

/*************************************************************************/
/*  TX_IRQHandler                                                        */
/*                                                                       */
/*  This is the generic CAN TX IRQHandler.                               */
/*                                                                       */
/*  In    : pCANx, pDCB, dFlags                                          */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void TX_IRQHandler (CAN_Type *pCANx, TAL_CAN_DCB *pDCB, uint32_t dFlags)
{
   TAL_RESULT     Error;
   TAL_CAN_OBJECT CANObject;
   
   (void)dFlags;

   /* Solve Tx Data Frame. */
   FLEXCAN_TransferAbortSend(pCANx, pDCB->HW.pHandle, TX_MBOX_INDEX);
   
   /* Clear resolved Message Buffer IRQ. */
   FLEXCAN_ClearMbStatusFlags(pCANx, (uint64_t)TX_MBOX_IMASK);
   
   STAT_INC(pDCB->HW.Stat.dTXCount);
   
   /* 
    * Check for TX interrupt, but only if it is anabled 
    */
   if (1 == pDCB->HW.nTxRunning)
   {
      /* Read Object from the ring buffer */
      Error = tal_MISCRingGet(&pDCB->TxRing, (uint8_t*)&CANObject);
      if (Error != TAL_OK)
      {
         /* Ups, no data available, disable interrupt */
         pDCB->HW.nTxRunning = 0;
      }
      else
      {
         /* Send Object */
         SendObject(pDCB, pCANx, &CANObject); 
      }   
   } /* end "TX interrupt */

} /* TX_IRQHandler */

/*************************************************************************/
/*  RX_IRQHandler                                                        */
/*                                                                       */
/*  This is the generic CAN RX IRQHandler.                               */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void RX_IRQHandler (CAN_Type *pCANx, TAL_CAN_DCB *pDCB, uint32_t dFlags)
{
   TAL_RESULT     Error;
   TAL_CAN_OBJECT CANObject;
   uint32_t       CS;
   uint32_t       ID;
   uint32_t       WORD0;
   uint32_t       WORD1;
   uint32_t       IDHIT;
   uint8_t       bRXValid;

   do
   {
      /* Check FIFO overflow */
      if (dFlags & CAN_IFLAG1_BUF7I_MASK) STAT_INC(pDCB->HW.Stat.dFIFOOverflow);
      if (dFlags & CAN_IFLAG1_BUF6I_MASK) STAT_INC(pDCB->HW.Stat.dFIFOWarning);

      CS    = pCANx->MB[0].CS;
      ID    = pCANx->MB[0].ID;
      WORD0 = pCANx->MB[0].WORD0;
      WORD1 = pCANx->MB[0].WORD1;
      IDHIT = (pCANx->RXFIR & CAN_RXFIR_IDHIT_MASK) & (CAN_MAX_FILTER_COUNT - 1);

      /* Read free-running timer to unlock Rx Message Buffer. */
      (void)pCANx->TIMER;
      
      /* ACK interrupt */
      pCANx->IFLAG1 = (CAN_IFLAG1_BUF7I_MASK | CAN_IFLAG1_BUF6I_MASK | CAN_IFLAG1_BUF5I_MASK);
      
      /* Get Identifier */
      if (CS & CAN_CS_IDE_MASK)
      {
         /* 29 bit Identifier */
         CANObject.bFlags      = TAL_CAN_OBJECT_FLAGS_EXT_ID;
         CANObject.dIdentifier = ID & (CAN_ID_EXT_MASK | CAN_ID_STD_MASK);
      }
      else
      {
         /* 11 bit Identifier */
         CANObject.bFlags      = TAL_CAN_OBJECT_FLAGS_STD_ID;
         CANObject.dIdentifier = (ID & CAN_ID_STD_MASK) >> CAN_ID_STD_SHIFT;
      }
      
      /* Check if we should receive this identifier */
      bRXValid = 0;
      if      ( (FILTER_ID(pDCB->HW.FilterArray[IDHIT]) == CANObject.dIdentifier) &&
                (TAL_CAN_OBJECT_FLAGS_EXT_ID            == CANObject.bFlags)      &&
                (IS_FILTER_EXT(pDCB->HW.FilterArray[IDHIT]))                      )
      {
         bRXValid = 1;
      }
      else if ( (FILTER_ID(pDCB->HW.FilterArray[IDHIT]) == CANObject.dIdentifier) &&
                (TAL_CAN_OBJECT_FLAGS_STD_ID            == CANObject.bFlags)      &&
                (!IS_FILTER_EXT(pDCB->HW.FilterArray[IDHIT]))                     )
      {
         bRXValid = 1;
      }
      else if ( (IS_FILTER_ALL(pDCB->HW.FilterArray[IDHIT]))                      &&
                (TAL_CAN_OBJECT_FLAGS_EXT_ID            == CANObject.bFlags)      &&
                (IS_FILTER_EXT(pDCB->HW.FilterArray[IDHIT]))                      )
      {                
         bRXValid = 1;
      }
      else if ( (IS_FILTER_ALL(pDCB->HW.FilterArray[IDHIT]))                      &&
                (TAL_CAN_OBJECT_FLAGS_STD_ID            == CANObject.bFlags)      &&
                (!IS_FILTER_EXT(pDCB->HW.FilterArray[IDHIT]))                     )
      {                
         bRXValid = 1;
      }
      
      /* Complete only if this is valid */
      if (1 == bRXValid)
      {
         /* Get the message length. */
         CANObject.bDLC = (uint8_t)((CS & CAN_CS_DLC_MASK) >> CAN_CS_DLC_SHIFT);

         /* Get data */
         CANObject.Data[3] = (uint8_t)(WORD0 & 0xFF);
         WORD0 = WORD0 >> 8;
         CANObject.Data[2] = (uint8_t)(WORD0 & 0xFF);
         WORD0 = WORD0 >> 8;
         CANObject.Data[1] = (uint8_t)(WORD0 & 0xFF);
         WORD0 = WORD0 >> 8;
         CANObject.Data[0] = (uint8_t)(WORD0 & 0xFF);

         CANObject.Data[7] = (uint8_t)(WORD1 & 0xFF);
         WORD1 = WORD1 >> 8;
         CANObject.Data[6] = (uint8_t)(WORD1 & 0xFF);
         WORD1 = WORD1 >> 8;
         CANObject.Data[5] = (uint8_t)(WORD1 & 0xFF);
         WORD1 = WORD1 >> 8;
         CANObject.Data[4] = (uint8_t)(WORD1 & 0xFF);

         /* If we have no overflow... */
         if (TAL_FALSE == pDCB->bRxOverflow)
         {
            /* ... put it into the ring buffer */
            Error = tal_MISCRingAdd(&pDCB->RxRing, (uint8_t*)&CANObject);
            if (TAL_OK == Error)
            {
               /* Signal counting semaphore */
               OS_SemaSignalFromInt(&pDCB->RxRdySema);
            }
            else
            {
               /* Ups, overflow */
               pDCB->bRxOverflow = TAL_OK;

               STAT_INC(pDCB->HW.Stat.dOverunCount);
            }
         } 
   
         STAT_INC(pDCB->HW.Stat.dRXCount);
      } /* end if (1 == bRXValid) */         
      
      /* Check if still more messages are availabe in the FIFO */      
      dFlags = pCANx->IFLAG1 & (CAN_IFLAG1_BUF7I_MASK | CAN_IFLAG1_BUF6I_MASK | CAN_IFLAG1_BUF5I_MASK); 
      
   } while (dFlags & CAN_IFLAG1_BUF5I_MASK);     
   
} /* RX_IRQHandler */

/*************************************************************************/
/*  ERR_IRQHandler                                                       */
/*                                                                       */
/*  This is the generic CAN Error IRQHandler.                            */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void ERR_IRQHandler (CAN_Type *pCANx, TAL_CAN_DCB *pDCB, uint32_t dFlags)
{
   TAL_RESULT     Error;
   TAL_CAN_OBJECT CANObject;
   uint32_t dACK;
   
   /* Clear FlexCAN Error and Status Interrupt. */
   dACK = (CAN_ESR1_TWRNINT_MASK |
           CAN_ESR1_RWRNINT_MASK | 
           CAN_ESR1_BOFFINT_MASK |
           CAN_ESR1_ERRINT_MASK  );
           
   /* Check for ACK_ERR */
   if (dFlags & CAN_ESR1_ACKERR_MASK)
   {
      dACK |= CAN_ESR1_ACKERR_MASK;

      STAT_INC(pDCB->HW.Stat.dACKError);

      /*
      * This bit indicates that an Acknowledge Error has been detected by
      * the transmitter node, i.e., a dominant bit has not been detected
      * during the ACK SLOT.
      *
      * Therefore abort a transmitting frame if available.
      */
      if (dFlags & CAN_ESR1_TX_MASK)
      {      
         /* Abort frame */
         FLEXCAN_TransferAbortSend(pCANx, pDCB->HW.pHandle, TX_MBOX_INDEX);
         FLEXCAN_ClearMbStatusFlags(pCANx, (uint64_t)TX_MBOX_IMASK);

         /* Check if a new frame is available to send */
         /* Read Object from the ring buffer */
         Error = tal_MISCRingGet(&pDCB->TxRing, (uint8_t*)&CANObject);
         if (Error != TAL_OK)
         {
            /* Ups, no data available, disable interrupt */
            pDCB->HW.nTxRunning = 0;
         }
         else
         {
            /* Send Object */
            SendObject(pDCB, pCANx, &CANObject); 
         }   
      }
   }
           
   /* Acknowledge error interrupt */
   pCANx->ESR1 = dACK;

} /* ERR_IRQHandler */

/*************************************************************************/
/*  CAN_IRQHandler                                                       */
/*                                                                       */
/*  This is the Cortex CAN IRQ handler.                                  */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void CAN_IRQHandler (TAL_CAN_DCB *pDCB)
{
   CAN_Type *pCANx = (CAN_Type*)pDCB->HW.dBaseAddress;
   uint32_t  dESRStatus;
   uint32_t  dValue;
   
   do 
   {   
      /* Get Current FlexCAN Module Error and Status. */
      dESRStatus = pCANx->ESR1;
   
      /* To handle FlexCAN Error and Status Interrupt first. */
      if (0U != (dESRStatus & (CAN_ESR1_TWRNINT_MASK | CAN_ESR1_RWRNINT_MASK |
                               CAN_ESR1_BOFFINT_MASK | CAN_ESR1_ERRINT_MASK) ))
      {
         /* Handle ERROR interrupt */
         ERR_IRQHandler(pCANx, pDCB, dESRStatus);
      }
      else if (0U != (dESRStatus & CAN_ESR1_WAKINT_MASK))
      {
         /* Acknowledge wakeup interrupt */
         pCANx->ESR1 = CAN_ESR1_WAKINT_MASK;
      }
      else
      {
         /* Handle RX/TX interrupt */
         dValue = pCANx->IFLAG1 & (TX_MBOX_IMASK | CAN_IFLAG1_BUF7I_MASK | CAN_IFLAG1_BUF6I_MASK | CAN_IFLAG1_BUF5I_MASK);
         if (dValue & CAN_IFLAG1_BUF5I_MASK)
         {
            RX_IRQHandler(pCANx, pDCB, dValue);     
         }
         
         dValue = pCANx->IFLAG1 & TX_MBOX_IMASK;
         if (dValue & TX_MBOX_IMASK)
         {
            TX_IRQHandler(pCANx, pDCB, dValue);     
         }
      }
      
   } while ( CheckUnhandleInterruptEvents(pCANx) );      
   
} /* CAN_IRQHandler */

/*=======================================================================*/
/*  All code exported                                                    */
/*=======================================================================*/

/*************************************************************************/
/*  cpu_CANInit                                                          */
/*                                                                       */
/*  Prepare the hardware for use by the Open function later. Set the HW  */
/*  information depending of ePort and "enable" the CAN port.            */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: TAL_OK / error cause                                         */
/*************************************************************************/
TAL_RESULT cpu_CANInit (TAL_CAN_DCB *pDCB)
{
   TAL_RESULT    Error = TAL_ERR_CAN_PORT_RANGE;
   TAL_CAN_HW  *pHW    = &pDCB->HW;

   switch (pDCB->ePort)
   {
      case TAL_CAN_PORT_1:
      {
         Error = tal_BoardEnableCAN1();
         if (TAL_OK == Error)
         {
            DCBArray[TAL_CAN_PORT_1] = pDCB;
         
            pHW->dBaseAddress = CAN0_BASE;
            pHW->nIrqNumber   = CAN0_IRQn;
            pHW->nIrqPriority = CAN0_PRIO;
            pHW->nTxRunning   = TAL_FALSE;
            
            /* Set irq and priority */
            tal_CPUIrqSetPriority(pHW->nIrqNumber, pHW->nIrqPriority);
         }
         break;
      } /* TAL_CAN_PORT_1 */

      case TAL_CAN_PORT_2:
      {
         Error = tal_BoardEnableCAN2();
         if (TAL_OK == Error)
         {
            DCBArray[TAL_CAN_PORT_2] = pDCB;
         
            pHW->dBaseAddress = CAN1_BASE;
            pHW->nIrqNumber   = CAN1_IRQn;
            pHW->nIrqPriority = CAN1_PRIO;
            pHW->nTxRunning   = TAL_FALSE;
            
            /* Set irq and priority */
            tal_CPUIrqSetPriority(pHW->nIrqNumber, pHW->nIrqPriority);
         }
         break;
      } /* TAL_CAN_PORT_2 */
      
      default:
      {
         /* Do nothing */
         Error = TAL_ERR_CAN_PORT_RANGE;
         break;
      }
   } /* end switch (pDCB->ePort) */
   
   return(Error);
} /* cpu_CANInit */

/*************************************************************************/
/*  cpu_CANOpen                                                          */
/*                                                                       */
/*  Open the CAN port.                                                   */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: TAL_OK / error cause                                         */
/*************************************************************************/
TAL_RESULT cpu_CANOpen (TAL_CAN_DCB *pDCB)
{
   TAL_RESULT                Error = TAL_ERROR;
   CAN_Type                *pCANx;
   flexcan_config_t          Config;
   flexcan_handle_t        *pHandle;
   uint32_t                 dBaudrate;
   uint8_t                  bIndex;
   uint32_t                *pFilterBase;


   /* Get CAN pointer */
   pCANx = (CAN_Type*)pDCB->HW.dBaseAddress;

   /* 
    * Check baud rate 
    */
   switch (pDCB->Settings.dBitRate)
   {
      case TAL_CAN_BR_500K: dBaudrate = 500000; break;
      case TAL_CAN_BR_250K: dBaudrate = 250000; break;
      case TAL_CAN_BR_125K: dBaudrate = 125000; break;
      case TAL_CAN_BR_100K: dBaudrate = 100000; break;
      case TAL_CAN_BR_50K:  dBaudrate = 50000;  break;
      case TAL_CAN_BR_20K:  dBaudrate = 20000;  break;
      case TAL_CAN_BR_10K:  dBaudrate = 10000;  break;
   
      default:
      {
         Error = TAL_ERR_CAN_BAUDRATE;
         goto CANOpenEnd;  /*lint !e801*/
         break;   /*lint !e527*/
      }
   } /* end switch (pDCB->Settings.dBaudrate) */

   /* Gets the default configuration structure */
   FLEXCAN_GetDefaultConfig(&Config);

   /* Disable self reception */
   Config.disableSelfReception = true;

   /* Set baud rate in bps */
   Config.baudRate = dBaudrate;

   /*
    * We will want to work with 32 filters, not more and not less.
    * For this 32 filters, 8 mailboxes must be used. That mean in
    * total 14 mailboxes (6 FIFO + 8 Filter) are needed.
    *
    * Mailbox 0 - 13 FIFO & Filter
    * Mailbox 14     Spare
    * Mailbox 15     TX-Mailbox
    *
    * Total mailbox count 16
    */
   Config.maxMbNum = 16;

   /* Disable Timer Synchronization */
   Config.enableTimerSync = false;

   /* Enable Rx Individual Mask */
   Config.enableIndividMask = true;

   /* Set Sample-Point point to 87.5 % */
   Config.timingConfig.phaseSeg1 = 7;
   Config.timingConfig.phaseSeg2 = 1;
   Config.timingConfig.propSeg   = 4;

#if 1 /* Calculates the improved timing values */
   flexcan_timing_config_t timing_config;
   memset(&timing_config, 0, sizeof(flexcan_timing_config_t));
   if (FLEXCAN_CalculateImprovedTimingValues(pCANx, Config.bitRate, CAN_CLK_FREQ, &timing_config))
   {
      /* Update the improved timing configuration*/
      memcpy(&(Config.timingConfig), &timing_config, sizeof(flexcan_timing_config_t));
   }
   else
   {
      goto CANOpenEnd;
   }
#endif

   /* Initializes the FlexCAN instance */
   FLEXCAN_Init(pCANx, &Config, CAN_CLK_FREQ);

   /* Create FlexCAN handle structure and set call back function. */
   pHandle = &HandleArray[pDCB->ePort];
   FLEXCAN_TransferCreateHandle(pCANx, pHandle, NULL, NULL);
   pDCB->HW.pHandle = pHandle;

   /* 
    * Setup filter and mask 
    */

   /* Enter Freeze Mode. */
   FLEXCAN_EnterFreezeMode(pCANx);

   /* Set RFFN */
   pCANx->CTRL2 = (pCANx->CTRL2 & ~CAN_CTRL2_RFFN_MASK) | CAN_CTRL2_RFFN(3);
   
   /* Setup ID Fitlter Type. */
   pCANx->MCR = (pCANx->MCR & ~CAN_MCR_IDAM_MASK) | CAN_MCR_IDAM(0x0);  /*lint !e845*/

   /*lint -save -e661 -e662 -e831 */
   
   /* Get filter base */
   pFilterBase = (uint32_t*)&(pCANx->MB[6].CS);
   for (bIndex = 0; bIndex < CAN_MAX_FILTER_COUNT; bIndex++)
   {
      pFilterBase[bIndex] = FILTER_NOT_USED;
   }

   /*lint -restore */

   /* Setting Message Reception Priority. */
   pCANx->CTRL2 = (pCANx->CTRL2 & ~CAN_CTRL2_MRP_MASK);

   /* Enable error interrupts */
   pCANx->CTRL1 |= (CAN_CTRL1_BOFFMSK_MASK | 
                    CAN_CTRL1_ERRMSK_MASK  | 
                    CAN_CTRL1_TWRNMSK_MASK | 
                    CAN_CTRL1_RWRNMSK_MASK );

   /* Enable Rx Message FIFO. */
   pCANx->MCR |= CAN_MCR_RFEN_MASK;

   /* Setting Rx Individual Mask value. */
   for (bIndex = 0; bIndex < CAN_MAX_FILTER_COUNT; bIndex++)
   {
      pCANx->RXIMR[bIndex] = FILTER_MASK_ALL_BITS;
   }

   /* Exit Freeze Mode. */
   FLEXCAN_ExitFreezeMode(pCANx);

   /* Clear filter */   
   memset(pDCB->HW.FilterArray, 0x00, sizeof(pDCB->HW.FilterArray));
   
   /* Clear statistics */
   memset(&pDCB->HW.Stat, 0x00, sizeof(pDCB->HW.Stat));
   
   /* Clear TX running state */
   pDCB->HW.nTxRunning  = TAL_FALSE;

   /* Setup Tx Message Buffer. */
   FLEXCAN_SetTxMbConfig(pCANx, TX_MBOX_INDEX, (bool)true);
   
   /* Update filters */
   UpdateAcceptanceFilter(pDCB);

   /* 
    * Enable interrupts
    */
#if 1 // TODO
   //pCANx->IMASK2 = 0;
   pCANx->IMASK1 = (CAN_IFLAG1_BUF7I_MASK | CAN_IFLAG1_BUF6I_MASK | CAN_IFLAG1_BUF5I_MASK);
#endif
    
   Error = TAL_OK;

CANOpenEnd:   
   return(Error);
} /* cpu_CANOpen */

/*************************************************************************/
/*  cpu_CANClose                                                         */
/*                                                                       */
/*  Close the CAN port.                                                  */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: TAL_OK / error cause                                         */
/*************************************************************************/
TAL_RESULT cpu_CANClose (TAL_CAN_DCB *pDCB)
{
   TAL_RESULT  Error = TAL_OK;
   CAN_Type  *pCANx;

   /* Get CAN pointer */
   pCANx = (CAN_Type*)pDCB->HW.dBaseAddress;

   FLEXCAN_Deinit(pCANx);

   /* Clear filter */   
   memset(pDCB->HW.FilterArray, 0x00, sizeof(pDCB->HW.FilterArray));
   
   return(Error);
} /* cpu_CANClose */

/*************************************************************************/
/*  cpu_CANStartTx                                                       */
/*                                                                       */
/*  Send the data from the ring buffer if the TX interrupt is disabled.  */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: TAL_OK / error cause                                         */
/*************************************************************************/
TAL_RESULT cpu_CANStartTx (TAL_CAN_DCB *pDCB)
{
   TAL_RESULT       Error = TAL_ERROR;
   CAN_Type       *pCANx;
   TAL_CAN_OBJECT   CANObject;
   
   /* Get CAN pointer */
   pCANx = (CAN_Type *)pDCB->HW.dBaseAddress;

   TAL_CPU_DISABLE_ALL_INTS();
   if (1 == pDCB->HW.nTxRunning)
   {
      /* TX interrupt is enabled, do nothing */
   }
   else
   {
      Error = tal_MISCRingGet(&pDCB->TxRing, (uint8_t*)&CANObject);
      if (TAL_OK == Error)
      {
         pDCB->HW.nTxRunning = 1;
         
         SendObject(pDCB, pCANx, &CANObject);
      }
   }
   TAL_CPU_ENABLE_ALL_INTS();
   
   return(Error);
} /* cpu_CANStartTx */

/*************************************************************************/
/*  cpu_CANTxIsRunning                                                   */
/*                                                                       */
/*  Check if TX is still running.                                        */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: TAL_OK / TAL_ERROR                                           */
/*************************************************************************/
TAL_RESULT cpu_CANTxIsRunning (TAL_CAN_DCB *pDCB)
{
   TAL_RESULT  Error = TAL_ERROR;

   TAL_CPU_DISABLE_ALL_INTS();
   if (1 == pDCB->HW.nTxRunning)
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
} /* cpu_CANTxIsRunning */

/*************************************************************************/
/*  cpu_CANEnableSilentMode                                              */
/*                                                                       */
/*  Enable "Silent" mode.                                                */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: TAL_OK / error cause                                         */
/*************************************************************************/
TAL_RESULT cpu_CANEnableSilentMode (TAL_CAN_DCB *pDCB)
{
   CAN_Type  *pCANx = (CAN_Type*)pDCB->HW.dBaseAddress;

   /* Enter Freeze Mode. */
   FLEXCAN_EnterFreezeMode(pCANx);
   
   pCANx->CTRL1 |= CAN_CTRL1_LOM_MASK;

   /* Exit Freeze Mode. */
   FLEXCAN_ExitFreezeMode(pCANx);
   
   return(TAL_OK);
} /* cpu_CANEnableSilentMode */

/*************************************************************************/
/*  cpu_CANDisableSilentMode                                             */
/*                                                                       */
/*  Disable "Silent" mode.                                               */
/*                                                                       */
/*  In    : pDCB                                                         */
/*  Out   : none                                                         */
/*  Return: TAL_OK / error cause                                         */
/*************************************************************************/
TAL_RESULT cpu_CANDisableSilentMode (TAL_CAN_DCB *pDCB)
{
   CAN_Type  *pCANx = (CAN_Type*)pDCB->HW.dBaseAddress;

   /* Enter Freeze Mode. */
   FLEXCAN_EnterFreezeMode(pCANx);

   pCANx->CTRL1 &= ~CAN_CTRL1_LOM_MASK;

   /* Exit Freeze Mode. */
   FLEXCAN_ExitFreezeMode(pCANx);
   
   return(TAL_OK);
} /* cpu_CANDisableSilentMode */

/*************************************************************************/
/*  cpu_CANIdentRegister                                                 */
/*                                                                       */
/*  Register a CAN identifier.                                           */
/*                                                                       */
/*  In    : pDCB, dIdentifier, Type                                      */
/*  Out   : none                                                         */
/*  Return: TAL_OK / error cause                                         */
/*************************************************************************/
TAL_RESULT cpu_CANIdentRegister (TAL_CAN_DCB  *pDCB,
                                uint32_t       dIdentifier,
                                TAL_CAN_TYPE_ID Type)
{
   TAL_RESULT  Error;

   Error = CANIdentRegister(pDCB, dIdentifier, Type);
   
   return(Error);
} /* cpu_CANIdentRegister */   

/*************************************************************************/
/*  cpu_CANIdentDeRegister                                               */
/*                                                                       */
/*  DeRegister a CAN identifier.                                         */
/*                                                                       */
/*  In    : pDCB, dIdentifier, Type                                      */
/*  Out   : none                                                         */
/*  Return: TAL_OK / error cause                                         */
/*************************************************************************/
TAL_RESULT cpu_CANIdentDeRegister (TAL_CAN_DCB  *pDCB,
                                  uint32_t       dIdentifier,
                                  TAL_CAN_TYPE_ID Type)
{
   TAL_RESULT  Error;

   Error = CANIdentDeRegister(pDCB, dIdentifier, Type);

   return(Error);
} /* cpu_CANIdentDeRegister */   

/*************************************************************************/
/*  cpu_CANGetSpeed                                                      */
/*                                                                       */
/*  Return the speed of the given port.                                  */
/*                                                                       */
/*  In    : pDCB, dIdentifier, Type                                      */
/*  Out   : none                                                         */
/*  Return: TAL_OK / error cause                                         */
/*************************************************************************/
void cpu_CANGetSpeed (TAL_CAN_PORT ePort, uint32_t *pSpeed)
{
   if (pSpeed != NULL)
   {
      switch (ePort)
      {
         case TAL_CAN_PORT_1: *pSpeed = DCBArray[TAL_CAN_PORT_1]->Settings.dBitRate / 1000; break;
         case TAL_CAN_PORT_2: *pSpeed = DCBArray[TAL_CAN_PORT_2]->Settings.dBitRate / 1000; break;
         default:             *pSpeed = 0; break;
      }
   }

} /* cpu_CANIdentDeRegister */   

/*************************************************************************/
/*  CAN0_IRQHandler                                                      */
/*                                                                       */
/*  This is the Cortex CAN0 IRQ handler.                                 */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void CAN0_IRQHandler (void)
{
   TAL_CPU_IRQ_ENTER();

   CAN_IRQHandler( DCBArray[TAL_CAN_PORT_1] );
   
   TAL_CPU_IRQ_EXIT();
} /* CAN0_IRQHandler */

/*************************************************************************/
/*  CAN1_IRQHandler                                                      */
/*                                                                       */
/*  This is the Cortex CAN1 IRQ handler.                                 */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void CAN1_IRQHandler (void)
{
   TAL_CPU_IRQ_ENTER();

   CAN_IRQHandler( DCBArray[TAL_CAN_PORT_2] );
   
   TAL_CPU_IRQ_EXIT();
} /* CAN1_IRQHandler */

/*** EOF ***/
