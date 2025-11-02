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
#define __MBRTU_TAP_C__

/*=======================================================================*/
/*  Includes                                                             */
/*=======================================================================*/
#include <stdint.h>
#include <string.h>
#include "tal.h"
#include "tcts.h"
#include "terminal.h"
#include "project.h"
#include "ipstack.h"
#include "mbrtu_tap.h"
#include "ipstack.h"
#include "ipweb.h"

/*=======================================================================*/
/*  All Structures and Common Constants                                  */
/*=======================================================================*/

#define CHECK_JSON_COMMA(_a,_b)     \
{                                   \
   if (1 == _a)                     \
   {                                \
      _a = 0;                       \
   }                                \
   else                             \
   {                                \
      s_puts(",", (_b)->s_stream);  \
   }                                \
}

#define MAX_TAP_CLIENT        4
#define MODBUS_CRC16_POLY     0xA001
#define TAB_RX_BUFFER_SIZE    256
#define CLI_CLIENT_PORT       32502

typedef struct _client_info_
{
   OS_TCB             TCB;
   uint8_t           *Stack;
   uint16_t           StackSize;
   int                TCPSocket;
   int                UDPSocket;
   struct sockaddr_in UDPDest;   
   uint32_t           Addr;
} client_info_t;

typedef struct
{
   uint32_t dLastTime;
   uint8_t   RxBuffer[TAB_RX_BUFFER_SIZE];
   uint8_t   RxIndex;
} MB_TAP;

/*=======================================================================*/
/*  Definition of all global Data                                        */
/*=======================================================================*/

/*=======================================================================*/
/*  Definition of all extern Data                                        */
/*=======================================================================*/

/*=======================================================================*/
/*  Definition of all local Data                                         */
/*=======================================================================*/

/* 
 * Stack and task definition.
 */
static OS_STACK(TAPServerStack, TASK_IP_MB_TAP_SERVER_STK_SIZE);
static OS_STACK(TAPTaskStack,   TASK_MB_TAP_STK_SIZE);

static OS_TCB              TCBTAPServer;
static OS_TCB              TCBTAPTask;

static client_info_t       ClientArray[MAX_TAP_CLIENT];
static uint64_t            ClientStack[MAX_TAP_CLIENT][TASK_IP_MB_TAP_CLIENT_STK_SIZE/8];
static int8_t              ClientCount;

static uint8_t             TAPTaskEnd;
static uint8_t             TAPTaskRunning;

static TAL_COM_DCB         COMPort;
static TAL_COM_SETTINGS    COMSettings;
static TAL_COM_OBJECT_TIME RxRingBuffer[256];
static uint8_t             TxRingBuffer[256];

static uint8_t             LastQueryTable[256][8];

/*=======================================================================*/
/*  Definition of all local Procedures                                   */
/*=======================================================================*/

/*************************************************************************/
/*  CalcCRC16                                                            */
/*                                                                       */
/*  In    : pBuffer, bSize                                               */
/*  Out   : none                                                         */
/*  Return: CRC16                                                        */
/*************************************************************************/
static uint16_t CalcCRC16 (uint8_t *pBuffer, uint8_t bSize)
{
   uint16_t wCRC = 0xFFFF;
   uint8_t  bShiftCnt;
   int      nFlag;

   while (bSize > 0)
   {
      bSize--;
      wCRC     ^= (uint16_t)*pBuffer++;
      bShiftCnt = 8;
      
      do 
      {
         nFlag   = (wCRC & 0x0001) ? 1 : 0;  /* Determine if the shift out of rightmost bit is 1     */
         wCRC  >>= 1;                        /* Shift CRC to the right one bit.                      */
         if (1 == nFlag)                     /* If (bit shifted out of rightmost bit was a 1)        */
         {
            wCRC ^= MODBUS_CRC16_POLY;       /* Exclusive OR the CRC with the generating polynomial. */
         }
         bShiftCnt--;
      } while (bShiftCnt > 0);
   }
   
   return(wCRC);
} /* CalcCRC16 */

/*************************************************************************/
/*  FindFreeClient                                                       */
/*                                                                       */
/*  In    : task parameter                                               */
/*  Out   : none                                                         */
/*  Return: NULL / Free client                                           */
/*************************************************************************/
static client_info_t *FindFreeClient (void)
{
   client_info_t *Client = NULL;
   
   for (int i=0; i<MAX_TAP_CLIENT; i++)
   {
      if ( OS_TaskTestStateNotInUsed(&ClientArray[i].TCB) )
      {
         Client = &ClientArray[i];
         break;
      }
   }
   
   return(Client);
} /* FindFreeClient */

/*************************************************************************/
/*  TAPSendRAW                                                           */
/*                                                                       */
/*  In    : pBuffer, Size, bQuery                                        */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void TAPSendRAW (uint8_t *pBuffer, size_t Size, uint8_t bQuery)
{
   uint8_t  Buffer[1 + TAB_RX_BUFFER_SIZE];
   
   Buffer[0] = bQuery;
   memcpy(&Buffer[1], pBuffer, Size);

   for (int i=0; i<MAX_TAP_CLIENT; i++)
   {
      if( (OS_TASK_STATE_READY   == ClientArray[i].TCB.State) ||
          (OS_TASK_STATE_RUNNING == ClientArray[i].TCB.State) ||
          (OS_TASK_STATE_WAITING == ClientArray[i].TCB.State) )
      {
         if (ClientArray[i].UDPSocket != SOCKET_ERROR)
         {
            sendto(ClientArray[i].UDPSocket, (const char *)Buffer, 1 + Size, 0, 
                   (struct sockaddr *)&ClientArray[i].UDPDest, sizeof(struct sockaddr)); /*lint !e740*/
         }                   
      }
   }
   
} /* TAPSendRAW */

#if 0
/*************************************************************************/
/*  OutputTapData                                                        */
/*                                                                       */
/*  In    : pTap, bQuery                                                 */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void OutputTapData (MB_TAP *pTap, uint8_t bQuery)
{
   uint16_t wAddr;
   uint16_t wSize;
   uint16_t wLineSize;
   
   term_printf("\r\n");

   if (1 == bQuery)
   {
      term_printf("Query\r\n");
   }
   else
   {
      term_printf("Response\r\n");
   }
   
   wAddr = 0;
   wSize = pTap->RxIndex;
   
   while (wSize > 0)
   {
      wLineSize = MIN(16, wSize);

      /* Output address and byte data */
      term_printf("%04X ", wAddr);
      for (int i = 0; i < wLineSize; i++)
      {
         term_printf(" %02X", pTap->RxBuffer[wAddr + i]);
      }
      
      /* Output space */
      if (wLineSize < 16)
      {
         for (int i = 16 - wLineSize; i > 0; i--)
         {
            term_printf("   ");
         }
      }
      term_printf("  ");
      
      /* Output "text" data */
      for (int i = 0; i < wLineSize; i++)
      {
         if( (pTap->RxBuffer[wAddr + i] >= 0x20) && 
             (pTap->RxBuffer[wAddr + i] <= 0x7E) )
         {
            term_printf("%c", pTap->RxBuffer[wAddr + i]);
         }
         else
         {
            term_printf(".");
         }
      }
      
      wSize -= wLineSize;    
      wAddr += 16;
      
      term_printf("\r\n");
   }

} /* OutputTapData */
#endif

/*************************************************************************/
/*  HandleTapData                                                        */
/*                                                                       */
/*  In    : pTap                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void HandleTapData (MB_TAP *pTap)
{
   uint16_t wCRCCalc;
   uint16_t wCRCData;
   uint8_t  bCRCValid = 0;
   uint8_t  bQuery    = 0;
   uint8_t  bAddr;
   uint8_t  bFunc;

   /* Check if data is available */
   if (0 == pTap->RxIndex)
   {
      return;
   }
   
   /* Check checksum if it could be a frame */
   if (pTap->RxIndex >= 5)
   {
      /* Check checksum */
      wCRCCalc  = CalcCRC16(pTap->RxBuffer, pTap->RxIndex - 2);
      wCRCData  = pTap->RxBuffer[pTap->RxIndex - 2];
      wCRCData |= pTap->RxBuffer[pTap->RxIndex - 1] << 8;
      
      if (wCRCCalc == wCRCData)
      {
         bCRCValid = 1;
      }
   }
   
   /* 
    * Check if this is a "Query" or "Response"
    *
    * A query has a size of 8 but in case of a "SingleWriteRegister"
    * (Func = 6), the response is identical to the query. To determine
    * whether it is a response, the data is compared with the last query.
    * If the data is identical, it is a response; if not, it is a query.
    */
   if (1 == bCRCValid)
   {
      bAddr = pTap->RxBuffer[0];
      bFunc = pTap->RxBuffer[1];
      
      /*
       * Valid frames with a length of 8 could be a query
       * and need further checking.
       */
      if (8 == pTap->RxIndex)
      {
         bQuery = 1;
         if (0 == memcmp(&LastQueryTable[bAddr][0], pTap->RxBuffer, 8))
         {
            /* It is a response */
            bQuery = 0;
         }
         else
         {
            /* This is a query, save it for a later compare */
            memcpy(&LastQueryTable[bAddr][0], pTap->RxBuffer, 8);
         }
      }
      
      if (0 == bQuery)
      {
         /* This is not a "Query" and the last query can be deleted */
         memset(&LastQueryTable[bAddr][0], 0x00, 8);
      }
   }

   //OutputTapData(pTap, bQuery);

   TAPSendRAW(pTap->RxBuffer, pTap->RxIndex, bQuery); 
   
   pTap->RxIndex = 0;
      
} /* HandleTapData */

/*************************************************************************/
/*  AddTapData                                                           */
/*                                                                       */
/*  In    : pTap, bData                                                  */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void AddTapData (MB_TAP *pTap, uint8_t bData)
{
   if (pTap->RxIndex < TAB_RX_BUFFER_SIZE - 1)
   {
      pTap->RxBuffer[pTap->RxIndex++] = bData;
   }   

} /* AddTapData */

/*************************************************************************/
/*  TAPTask                                                              */
/*                                                                       */
/*  This is the LED task.                                                */
/*                                                                       */
/*  In    : task parameter                                               */
/*  Out   : none                                                         */
/*  Return: never                                                        */
/*************************************************************************/
static void TAPTask (void *p)
{
   TAL_RESULT          Error;
   MB_TAP              Tap;
   uint8_t            bData;
   TAL_COM_OBJECT_TIME TimeData;
   uint32_t           dTimeout = 4;   

   (void)p;
   
   term_printf("TAPTask started\r\n");
   
   memset(&Tap, 0x00, sizeof(Tap));
   Tap.RxIndex = 0;
   
   TAPTaskRunning = 1;
   while (0 == TAPTaskEnd)
   {
      Error = tal_COMReceiveCharWait(&COMPort, (uint8_t*)&TimeData, 1);     
      if (TAL_OK == Error)
      {
         /* Check time between two characters */
         if (OS_TEST_TIMEOUT(TimeData.dTimestamp, Tap.dLastTime , dTimeout))
         {
            HandleTapData(&Tap);
         }   
      
         Tap.dLastTime = TimeData.dTimestamp;
         bData = (uint8_t)(TimeData.dData & 0x000000FF);
         AddTapData(&Tap, bData);
      }
      else
      {
         /* Check time between last character and actual time */
         if (OS_TEST_TIMEOUT(OS_TimeGet(), Tap.dLastTime , dTimeout))
         {
            HandleTapData(&Tap);
         }   
      
         if( (TAL_ERR_COM_OVERFLOW       == Error) || 
             (TAL_ERR_COM_OVERFLOW_EMPTY == Error) )
         {
            tal_COMClearOverflow(&COMPort);
         }             
      } /* end if (TAL_OK == Error) */ 
   
   } /* end while (0 == TAPTaskEnd) */ 
   TAPTaskRunning = 0;
   TAPTaskEnd     = 0;
   
   memset(&COMSettings, 0x00, sizeof(COMSettings));

   tal_COMClose(&COMPort);
   
   term_printf("TAPTask stopped\r\n");

   OS_TaskExit();
} /* TAPTask */

/*************************************************************************/
/*  CheckConnect                                                         */
/*                                                                       */
/*  In    : Msg, ErrorCode                                               */
/*  Out   : ErrorCode                                                    */
/*  Return: 0 = OK / -1 = Error                                          */
/*************************************************************************/
static int CheckConnect (tap_msg_t *Msg, uint32_t *ErrorCode)
{
   int              rc = TAP_ERROR;
   TAL_COM_SETTINGS TAPSettings;
   
   *ErrorCode = 0;
   memset(&TAPSettings, 0x00, sizeof(TAPSettings));

   if( (TAP_HEADER_MAGIC_1 == Msg->Header.Magic1)  &&
       (TAP_HEADER_MAGIC_2 == Msg->Header.Magic2)  &&
       (TAP_SIZEVER        == Msg->Header.SizeVer) &&
       (TAP_MSG_CONNECT    == Msg->Header.Func)    )
   {
      /* 
       * Check valid parameter
       */
      switch (Msg->Data.Connect.dBaudrate)
      {
         case   1200:
         case   2400:
         case   4800:
         case   9600:
         case  19200:
         case  38400:
         case  57600:
         case 115200:
         {
            TAPSettings.dBaudrate = Msg->Data.Connect.dBaudrate;   
            break;
         }
         default: *ErrorCode |= TAP_ERR_BAUDRATE; break;
      }
      
      switch (Msg->Data.Connect.eLength)
      {
         case  TAP_COM_LENGTH_8: TAPSettings.eLength = TAL_COM_LENGTH_8; break;
         default               : *ErrorCode |= TAP_ERR_DATA_BITS;        break;
      }
      
      switch (Msg->Data.Connect.eParity)
      {
         case TAP_COM_PARITY_NONE: TAPSettings.eParity = TAL_COM_PARITY_NONE; break;
         case TAP_COM_PARITY_EVEN: TAPSettings.eParity = TAL_COM_PARITY_EVEN; break;
         case TAP_COM_PARITY_ODD : TAPSettings.eParity = TAL_COM_PARITY_ODD;  break;
         default                 : *ErrorCode |= TAP_ERR_PARITY;              break;
      }
      
      switch (Msg->Data.Connect.eStop)
      {
         case TAP_COM_STOP_1_0: TAPSettings.eStop = TAL_COM_STOP_1_0; break;
         default              : *ErrorCode |= TAP_ERR_STOP_BITS;      break;
      }
   
      /*
       * Check if this is the first connect
       */
      if (1 == ClientCount)
      {
         /* No error, start TAPTask */
         if (0 == *ErrorCode)
         {
            TAL_RESULT Error;
            
            /* Init the Device Control Block */      
            tal_COMInitDCB(&COMPort, TAL_COM_PORT_6);

            /* Set Rx and Tx ring buffer */
            tal_COMSetRingBuffer(&COMPort, TAL_COM_BUFFER_RX_TIME, (uint8_t*)RxRingBuffer, sizeof(RxRingBuffer)); 
            tal_COMSetRingBuffer(&COMPort, TAL_COM_BUFFER_TX, TxRingBuffer, sizeof(TxRingBuffer)); 
      
            tal_COMSetRxTimestampFunc(&COMPort, OS_TimeGet);
      
            /* Prepare communication settings */
            COMSettings = TAPSettings;

            /* Now, the port can be opened */   
            Error = tal_COMOpen(&COMPort, &COMSettings);
            if (TAL_OK == Error)
            {
               rc = TAP_OK;
               
               TAPTaskEnd = 0;
               OS_TaskCreate(&TCBTAPTask, TAPTask, NULL, TASK_MB_TAP_PRIORITY,
                             TAPTaskStack, sizeof(TAPTaskStack), 
                             "TAPTask");
            }
            else
            {
               rc = TAP_ERR_OPEN;
            }                             
         }
         else
         {
            rc = TAP_ERR_START_PARAM;
         }
      }
      else
      {
         /* 
          * TAPTask still running, check COM settings
          */
         if (0 == memcmp(&COMSettings, &TAPSettings, sizeof(COMSettings)))
         {
            rc = TAP_OK;
         }
         else
         {
            /* TAPTask running with other settings */
            rc = TAP_ERR_RUN_PARAM;  
            if (COMSettings.dBaudrate != TAPSettings.dBaudrate) *ErrorCode |= TAP_ERR_BAUDRATE;
            if (COMSettings.eLength   != TAPSettings.eLength)   *ErrorCode |= TAP_ERR_DATA_BITS;
            if (COMSettings.eParity   != TAPSettings.eParity)   *ErrorCode |= TAP_ERR_PARITY;
            if (COMSettings.eStop     != TAPSettings.eStop)     *ErrorCode |= TAP_ERR_STOP_BITS;
         }
      }
   }
   
   return(rc);
} /* CheckConnect */

/*************************************************************************/
/*  TAPClient                                                            */
/*                                                                       */
/*  In    : task parameter                                               */
/*  Out   : none                                                         */
/*  Return: never                                                        */
/*************************************************************************/
static void TAPClient (void *arg)
{
   int            rc = TAP_ERROR;
   int            Optval;
   client_info_t *Client    = (client_info_t*)arg;
   int            BytesReceived;  
   uint8_t        Buffer[256];
   tap_msg_t     *Msg = (tap_msg_t*)Buffer;
   uint32_t       ErrorCode;
   uint32_t       dLastTime;
   
   ClientCount++;
   
   /*
    * Wait for a TinyTAP client connection...
    */   
   BytesReceived = recv(Client->TCPSocket, Buffer, sizeof(Buffer), 0);
   if (BytesReceived > 0)
   {
      rc = CheckConnect(Msg, &ErrorCode);

      Msg->Header.Result    = rc;
      Msg->Header.ErrorCode = ErrorCode; 
      
      /* Send response */
      send(Client->TCPSocket, Buffer, TAP_MSG_HEADER_SIZE + TAP_MSG_CONNECT_SIZE, 0); 
   }
   
   /*
    * Start receive loop if no error
    */
   if (TAP_OK == rc)
   {
      term_printf("TinyTAP client connected\r\n");
      
      /* Create UDP socket for raw data transfer to TinyTAP client */   
      Client->UDPSocket = socket(AF_INET, SOCK_DGRAM, 0);
      if (Client->UDPSocket != SOCKET_ERROR)
      {
         /* Set address and port for the socket */
         Client->UDPDest.sin_addr.s_addr = Client->Addr;
         Client->UDPDest.sin_port        = htons(CLI_CLIENT_PORT);
         Client->UDPDest.sin_family      = AF_INET;
      }
      else
      {
        term_printf("TinyTAP socket error, client disconnected\r\n");
        goto end;
      }
      
      /* Set receive timeout for the TCP socket */   
      Optval = 100;
      rc = setsockopt(Client->TCPSocket, SOL_SOCKET, SO_RCVTIMEO, &Optval, sizeof(Optval));

      dLastTime = OS_TimeGet();
      while(1)
      {
         BytesReceived = recv(Client->TCPSocket, Buffer, sizeof(Buffer), 0);
         if (BytesReceived <= 0)
         {
            /* Check for timeout */
            if (errno != EAGAIN)
            {
               break;
            }               
         }

         /* Check if we must send a ping */         
         if (OS_TEST_TIMEOUT(OS_TimeGet(), dLastTime, OS_MS_2_TICKS(2500)))         
         {         
            dLastTime = OS_TimeGet();
            
            rc = send(Client->TCPSocket, "ping", 4, 0);
            if (rc <= 0)
            {
               break;
            }
         }
      } /* end while(1) */

      term_printf("TinyTAP client disconnected\r\n");
   } /* end if of connection*/

end:

   shutdown(Client->TCPSocket, SHUT_RDWR);
   closesocket(Client->TCPSocket); 
   Client->TCPSocket = INVALID_SOCKET;

   if (Client->UDPSocket != INVALID_SOCKET)
   {
      shutdown(Client->UDPSocket, SHUT_RDWR);
      closesocket(Client->UDPSocket); 
      Client->UDPSocket = INVALID_SOCKET;
   }      

   /* The last client must stop the TAP task */
   if (ClientCount > 0)
   {
      ClientCount--;
      if (0 == ClientCount)
      {
         TAPTaskEnd = 1;
      }      
   }

   OS_TaskExit();
} /* TAPClient */

/*************************************************************************/
/*  TAPServer                                                            */
/*                                                                       */
/*  In    : task parameter                                               */
/*  Out   : none                                                         */
/*  Return: never                                                        */
/*************************************************************************/
static void TAPServer (void *p)
{
   int                  ServerSocket;
   int                  Socket;
   struct sockaddr_in   Server;
   struct sockaddr_in   InAddr;
   socklen_t            InAddrSize;
   client_info_t       *Client = NULL;
   
   (void)p;
   
   while(!IP_IF_IsReady(IFACE_ANY))
   {
      OS_TimeDly(500);
   }
   
   /* Create the TAP server socket */
   ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
   
   /* Set address and port for the socket */
   memset(&Server, 0x00, sizeof(Server));
   Server.sin_addr.s_addr = INADDR_ANY;
   Server.sin_port        = htons(TAP_TCP_SERVER_PORT);
   Server.sin_family      = AF_INET;
   
   /* Assign a name (port) to an unnamed socket */
   bind(ServerSocket, (const struct sockaddr *)&Server, sizeof(Server));   /*lint !e740*/
   
   listen(ServerSocket, MAX_TAP_CLIENT);
   
   while(1)
   {
      InAddrSize = sizeof(InAddr);
      if ((Socket = accept(ServerSocket, (struct sockaddr*)&InAddr, &InAddrSize)) == INVALID_SOCKET) /*lint !e740*/
      {
         continue;   /* Error */
      }

      Client = FindFreeClient();
      if (Client != NULL)
      {
         /* Create the TAP client task */
         Client->TCPSocket = Socket;
         Client->UDPSocket = INVALID_SOCKET;
         Client->Addr      = InAddr.sin_addr.s_addr;

         OS_TaskCreate(&Client->TCB, TAPClient, (void*)Client, (TASK_IP_WEB_TLS_SERVER_PRIORITY + 1),
                       Client->Stack, Client->StackSize, 
                       "TAPClient");
      }
      else
      {
         closesocket(Socket);
      }
   }

} /* TAPServer */


/*************************************************************************/
/*  sys_mb_xxx                                                           */
/*                                                                       */
/*  In    : hs                                                           */
/*  Out   : none                                                         */
/*  Return: 0 = OK / -1 = ERROR                                          */
/*************************************************************************/
static int sys_mb_baud (HTTPD_SESSION *hs)
{
   if (0 == TAPTaskRunning)
   {
      s_printf(hs->s_stream, "0");
   }
   else
   {
      s_printf(hs->s_stream, "%d", COMSettings.dBaudrate);
   }   

   s_flush(hs->s_stream);

   return(0);
} /* sys_mb_baud */

static int sys_mb_data (HTTPD_SESSION *hs)
{
   if (0 == TAPTaskRunning)
   {
      s_printf(hs->s_stream, "0");
   }
   else
   {
      switch (COMSettings.eLength)
      {
         case TAP_COM_LENGTH_8: s_printf(hs->s_stream, "8 bit"); break;
         default: s_printf(hs->s_stream, "8 bit"); break;
      }
   }   

   s_flush(hs->s_stream);

   return(0);
} /* sys_mb_data */

static int sys_mb_parity (HTTPD_SESSION *hs)
{
   if (0 == TAPTaskRunning)
   {
      s_printf(hs->s_stream, "N");
   }
   else
   {
      switch(COMSettings.eParity)
      {
         case TAP_COM_PARITY_NONE: s_printf(hs->s_stream, "N"); break;
         case TAP_COM_PARITY_EVEN: s_printf(hs->s_stream, "E"); break;
         case TAP_COM_PARITY_ODD : s_printf(hs->s_stream, "O"); break;
         default:                  s_printf(hs->s_stream, "N"); break;
      }
   }   

   s_flush(hs->s_stream);

   return(0);
} /* sys_mb_parity */

static int sys_mb_stop (HTTPD_SESSION *hs)
{
   if (0 == TAPTaskRunning)
   {
      s_printf(hs->s_stream, "0");
   }
   else
   {
      switch (COMSettings.eStop)
      {
         case TAP_COM_STOP_1_0: s_printf(hs->s_stream, "1 bit"); break;
         default: s_printf(hs->s_stream, "1 bit"); break;
      }
   }   

   s_flush(hs->s_stream);

   return(0);
} /* sys_mb_stop */


/*
 * SSI variable list
 */
static const SSI_EXT_LIST_ENTRY SSIList[] =  /*lint !e31*/
{
   { "sys_mb_baud",     sys_mb_baud   },
   { "sys_mb_data",     sys_mb_data   },
   { "sys_mb_parity",   sys_mb_parity },
   { "sys_mb_stop",     sys_mb_stop   },
   
   { NULL, NULL }
};

/*************************************************************************/
/*  ModbusGet                                                            */
/*                                                                       */
/*  In    : hs                                                           */
/*  Out   : none                                                         */
/*  Return: 0 = OK / -1 = ERROR                                          */
/*************************************************************************/
static int ModbusGet (HTTPD_SESSION *hs)
{
   char     *pArg;
   long       Avail;
   int        First = 1;

   IP_WEBS_CGISendHeader(hs);

   Avail = hs->s_req.req_length;
   s_puts("{", hs->s_stream);

   while (Avail)
   {
      pArg = HttpArgReadNext(hs, &Avail);
      if      (strcmp(pArg, "baud") == 0)
      {
         CHECK_JSON_COMMA(First, hs);
         s_printf(hs->s_stream, "\"baud\":\"%d\"", COMSettings.dBaudrate);
      }
      else if (strcmp(pArg, "data") == 0)
      {
         CHECK_JSON_COMMA(First, hs);
         switch (COMSettings.eLength)
         {
            case TAP_COM_LENGTH_8: s_printf(hs->s_stream, "\"data\":\"8 bits\""); break;
            default: s_printf(hs->s_stream, "\"data\":\"8 bits\""); break;
         }
      }
      else if (strcmp(pArg, "parity") == 0)
      {
         CHECK_JSON_COMMA(First, hs);
         switch(COMSettings.eParity)
         {
            case TAP_COM_PARITY_NONE: s_printf(hs->s_stream, "\"parity\":\"none\""); break;
            case TAP_COM_PARITY_EVEN: s_printf(hs->s_stream, "\"parity\":\"even\""); break;
            case TAP_COM_PARITY_ODD : s_printf(hs->s_stream, "\"parity\":\"odd\"");  break;
            default:                  s_printf(hs->s_stream, "\"parity\":\"???\"");  break;
         }
      }
      else if (strcmp(pArg, "stop") == 0)
      {
         CHECK_JSON_COMMA(First, hs);
         switch (COMSettings.eStop)
         {
            case TAP_COM_STOP_1_0: s_printf(hs->s_stream, "\"stop\":\"1 bits\""); break;
            default:               s_printf(hs->s_stream, "\"stop\":\"? bits\""); break;
         }
      }
      else
      {
         /* Unknown argument */
         CHECK_JSON_COMMA(First, hs);
         s_printf(hs->s_stream, "\"%s\":\"---\"", pArg);
      }
   }

   s_puts("}", hs->s_stream);
   s_flush(hs->s_stream);

   return(0);
} /* ModbusGet */


/*
 * CGI variable list
 */
static const CGI_LIST_ENTRY CGIList[] =      /*lint !e31*/
{
   { "cgi-bin/mb_get.cgi", ModbusGet  },

   { NULL, NULL }
};

/*=======================================================================*/
/*  All code exported                                                    */
/*=======================================================================*/

/*************************************************************************/
/*  mbrtu_TapInit                                                        */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void mbrtu_TapInit (void)
{
   IP_WEBS_SSIListAdd((SSI_EXT_LIST_ENTRY*)SSIList);
   IP_WEBS_CGIListAdd((CGI_LIST_ENTRY*)CGIList);
   
} /* mbrtu_TapInit */ 

/*************************************************************************/
/*  mbrtu_TapStart                                                       */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void mbrtu_TapStart (void)
{
   static int nStartDone = 0;
   
   if (0 == nStartDone)
   {
      ClientCount = 0;
      memset(ClientArray,    0x00, sizeof(ClientArray));
      memset(LastQueryTable, 0x00, sizeof(LastQueryTable));

      /* Create stack */   
      for(int i=0; i<MAX_TAP_CLIENT; i++)
      {
         OS_TaskSetStateNotInUsed(&ClientArray[i].TCB);
         ClientArray[i].Stack     = (uint8_t*)&ClientStack[i][0];
         ClientArray[i].StackSize = TASK_IP_MB_TAP_CLIENT_STK_SIZE;
      }

      /* Create the TAP server task */
      OS_TaskCreate(&TCBTAPServer, TAPServer, NULL, TASK_IP_MB_TAP_PRIORITY,
                    TAPServerStack, sizeof(TAPServerStack), 
                    "TAPServer");
   } /* end if (0 == nStartDone) */  
   
} /* mbrtu_TapStart */

/*** EOF ***/
