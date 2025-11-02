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
#define __MAIN_C__

/*=======================================================================*/
/*  Includes                                                             */
/*=======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compat.h"
#include "project.h"
#include "tnp.h"
#include "para.h"
#include "mbrtu_tap.h"

/*=======================================================================*/
/*  All Structures and Common Constants                                  */
/*=======================================================================*/

#define SERVER_NAME     "TinyTAP"


#define GOTO_END(_a)    { rc = _a; goto end; }

#if !defined(MIN)
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#define MBRTU_CLIENT_PORT  61502
#define MBRTU_SERVER_PORT  1502
#define MBRTU_MAX_SIZE     256
#define CLI_CLIENT_PORT    32502

#define SOCKET_TIMEOUT_MS  100
#define TINYTAP_TIMEOUT_MS 5000

/*=======================================================================*/
/*  Definition of all local Data                                         */
/*=======================================================================*/

static socket_t    TCPSocket       = SOCKET_INVALID;
static socket_t    UDPClientSocket = SOCKET_INVALID;
static socket_t    UDPServerSocket = SOCKET_INVALID;
static socket_t    UDPCliSocket    = SOCKET_INVALID;
struct sockaddr_in UDPClientDest;
struct sockaddr_in UDPServerDest;

static int nEndRequestEnd = 0;

/*=======================================================================*/
/*  Definition of prototypes                                             */
/*=======================================================================*/

/*=======================================================================*/
/*  Definition of all local Procedures                                   */
/*=======================================================================*/

/*************************************************************************/
/*  OutputTapData                                                        */
/*                                                                       */
/*  Output data on the terminal.                                         */
/*                                                                       */
/*  In    : pBuffer, Size                                                */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void OutputTapData (uint8_t *pBuffer, int Size)
{
   int       i;
   uint16_t wAddr = 0;
   uint16_t wSize = (uint16_t)Size;
   uint16_t wLineSize;

   while (wSize > 0)
   {
      wLineSize = MIN(16, wSize);

      /* Output address and byte data */
      printf("%04X ", wAddr);
      for (i = 0; i < wLineSize; i++)
      {
         printf(" %02X", pBuffer[wAddr + i]);
      }

      /* Output space */
      if (wLineSize < 16)
      {
         for (i = 16 - wLineSize; i > 0; i--)
         {
            printf("   ");
         }
      }
      printf("  ");

      /* Output "text" data */
      for (i = 0; i < wLineSize; i++)
      {
         if( (pBuffer[wAddr + i] >= 0x20) &&
             (pBuffer[wAddr + i] <= 0x7E) )
         {
            printf("%c", pBuffer[wAddr + i]);
         }
         else
         {
            printf(".");
         }
      }

      wSize -= wLineSize;
      wAddr += 16;

      printf("\n");
   }
   printf("\n");

} /* OutputTapData */

/*************************************************************************/
/*  WaitMS                                                               */
/*                                                                       */
/*  In    : millisec                                                     */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void WaitMS (uint32_t millisec)
{
#if defined(__MINGW64__)
   Sleep(millisec);
#else
   usleep(millisec*1000);
#endif

} /* millisec */

/*************************************************************************/
/*  CloseTCPSocket                                                       */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void CloseTCPSocket (void)
{
   if (TCPSocket != SOCKET_INVALID)
   {
      closesocket(TCPSocket);
      TCPSocket = SOCKET_INVALID;
   }
   
} /* CloseTCPSocket */

/*************************************************************************/
/*  CloseUDPSocket                                                       */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void CloseUDPSocket (void)
{
   if (UDPClientSocket != SOCKET_INVALID)
   {
      closesocket(UDPClientSocket);
      UDPClientSocket = SOCKET_INVALID;
   }
   
   if (UDPServerSocket != SOCKET_INVALID)
   {
      closesocket(UDPServerSocket);
      UDPServerSocket = SOCKET_INVALID;
   }
   
   if (UDPCliSocket != SOCKET_INVALID)
   {
      closesocket(UDPCliSocket);
      UDPCliSocket = SOCKET_INVALID;
   }
   
} /* CloseUDPSocket */

/*************************************************************************/
/*  UDPClientThread                                                      */
/*                                                                       */
/*  Receive data from the server.                                        */
/*                                                                       */
/*  In    : param                                                        */
/*  Out   : none                                                         */
/*  Return: 0                                                            */
/*************************************************************************/
static void *UDPClientThread (void *param)
{
   int                rc;
   fd_set             readfds;
   struct timeval     timeout;
   struct sockaddr_in client;
   struct sockaddr_in source;
   socklen_t          sourceLen;
   char               buffer[MBRTU_MAX_SIZE];
   
   (void)param;
   
   printf("UDP client thread started.\n");
   
   /* Bind client port */
   client.sin_family      = AF_INET;
   client.sin_port        = htons(MBRTU_CLIENT_PORT);
   client.sin_addr.s_addr = INADDR_ANY;
   bind(UDPClientSocket, (struct sockaddr *)&client, sizeof(client));
   
   /* Set destination */
   UDPClientDest.sin_family      = AF_INET;
   UDPClientDest.sin_port        = htons(MBRTU_SERVER_PORT);
   UDPClientDest.sin_addr.s_addr = inet_addr("127.0.0.1");

   while (0 == nEndRequestEnd)
   {
      /* Set NonBlocking socket to SOCKET_TIMEOUT_MS */
      FD_ZERO(&readfds);
      FD_SET(UDPClientSocket, &readfds);
      timeout.tv_sec  = 0;
      timeout.tv_usec = SOCKET_TIMEOUT_MS * 1000;
      
      rc = select(UDPClientSocket + 1, &readfds, NULL, NULL, &timeout);
      if (rc > 0)
      {
         /* Empty buffer if data is available */
         sourceLen = sizeof(source);
         recvfrom(UDPClientSocket, buffer, sizeof(buffer), 0,
                  (struct sockaddr *)&source, &sourceLen);
      }
      else if (0 == rc)
      {
         /* Timeout */
      }
      else
      {
         /* Error, e.g. disconnect */
         break;
      }   
   }

   printf("UDP client thread terminated.\n");

   return(0);
} /* UDPClientThread */

/*************************************************************************/
/*  UDPServerThread                                                      */
/*                                                                       */
/*  Receive data from the client.                                        */
/*                                                                       */
/*  In    : Param                                                        */
/*  Out   : none                                                         */
/*  Return: 0                                                            */
/*************************************************************************/
static void *UDPServerThread (void *param)
{
   int                rc;
   fd_set             readfds;
   struct timeval     timeout;
   struct sockaddr_in server;
   struct sockaddr_in source;
   socklen_t          sourceLen;
   char               buffer[MBRTU_MAX_SIZE];
   
   (void)param;
   
   printf("UDP server thread started.\n");
   
   /* Bind server port */
   server.sin_family      = AF_INET;
   server.sin_port        = htons(MBRTU_SERVER_PORT);
   server.sin_addr.s_addr = INADDR_ANY;
   bind(UDPServerSocket, (struct sockaddr *)&server, sizeof(server));
   
   /* Set destination */
   UDPServerDest.sin_family      = AF_INET;
   UDPServerDest.sin_port        = htons(MBRTU_CLIENT_PORT);
   UDPServerDest.sin_addr.s_addr = inet_addr("127.0.0.1");

   while (0 == nEndRequestEnd)
   {
      /* Set NonBlocking socket to SOCKET_TIMEOUT_MS */
      FD_ZERO(&readfds);
      FD_SET(UDPServerSocket, &readfds);
      timeout.tv_sec  = 0;
      timeout.tv_usec = SOCKET_TIMEOUT_MS * 1000;
      
      rc = select(UDPServerSocket + 1, &readfds, NULL, NULL, &timeout);
      if (rc > 0)
      {
         /* Empty buffer if data is available */
         sourceLen = sizeof(source);
         recvfrom(UDPServerSocket, buffer, sizeof(buffer), 0,
                  (struct sockaddr *)&source, &sourceLen);
      }
      else if (0 == rc)
      {
         /* Timeout */
      }
      else
      {
         /* Error, e.g. disconnect */
         break;
      }   
   }

   printf("UDP server thread terminated.\n");

   return(0);
} /* UDPServerThread */

/*************************************************************************/
/*  UDPCliThread                                                         */
/*                                                                       */
/*  Receive data from the TinyTAP server.                                */
/*                                                                       */
/*  In    : Param                                                        */
/*  Out   : none                                                         */
/*  Return: 0                                                            */
/*************************************************************************/
static void *UDPCliThread (void *param)
{
   int                rc;
   fd_set             readfds;
   struct timeval     timeout;
   struct sockaddr_in server;
   struct sockaddr_in source;
   socklen_t          sourceLen;
   uint8_t            rxBuffer[1 + MBRTU_MAX_SIZE];
   uint8_t            txBuffer[1 + MBRTU_MAX_SIZE];

   (void)param;
   
   printf("UDP cli thread started.\n");
   
   /* Bind server port */
   server.sin_family      = AF_INET;
   server.sin_port        = htons(CLI_CLIENT_PORT);
   server.sin_addr.s_addr = INADDR_ANY;
   bind(UDPCliSocket, (struct sockaddr *)&server, sizeof(server));
   
   while (0 == nEndRequestEnd)
   {
      /* Set NonBlocking socket to SOCKET_TIMEOUT_MS */
      FD_ZERO(&readfds);
      FD_SET(UDPCliSocket, &readfds);
      timeout.tv_sec  = 0;
      timeout.tv_usec = SOCKET_TIMEOUT_MS * 1000;
      
      rc = select(UDPCliSocket + 1, &readfds, NULL, NULL, &timeout);
      if (rc > 0)
      {
         /* Empty buffer if data is available */
         sourceLen = sizeof(source);
         rc = recvfrom(UDPCliSocket, (char*)rxBuffer, sizeof(rxBuffer), 0,
                       (struct sockaddr *)&source, &sourceLen);
         if (rc > 0)
         {         
            /* Copy data to the tx buffer */
            memcpy(txBuffer, rxBuffer, rc);
         
            /* Check if it is a "Query" */
            if (1 == txBuffer[0])
            {
               /* Query, send by client */
               if (UDPClientSocket != SOCKET_INVALID)
               {
                  sendto(UDPClientSocket, (const char*)&txBuffer[1], rc - 1, 0,
                         (struct sockaddr*)&UDPClientDest, sizeof(UDPClientDest));
               }                         
            }
            else
            {
               /* Response, send by server */
               if (UDPServerSocket != SOCKET_INVALID)
               {
                  sendto(UDPServerSocket, (const char*)&txBuffer[1], rc - 1, 0,
                         (struct sockaddr*)&UDPServerDest, sizeof(UDPServerDest));
               }                         
            }
            
            /* Output data on the terminal */
            OutputTapData(&txBuffer[1], rc - 1);
         }            
      }
      else if (0 == rc)
      {
         /* Timeout */
      }
      else
      {
         /* Error, e.g. disconnect */
         break;
      }   
   }

   printf("UDP cli thread terminated.\n");

   return(0);
} /* UDPCliThread */

/*************************************************************************/
/*  ConsoleHandler                                                       */
/*                                                                       */
/*  In    : signal                                                       */
/*  Out   : none                                                         */
/*  Return: TRUE / FALSE                                                 */
/*************************************************************************/
void ConsoleHandler (int signal)
{
   if (SIGINT == signal)
   {
      const char msg[] = "Ctrl+C signal received.\n";
      write(STDOUT_FILENO, msg, strlen(msg));

      nEndRequestEnd = 1;
   }

} /* ConsoleHandler */

/*************************************************************************/
/*  OutputStartMessage                                                   */
/*                                                                       */
/*  Output start message.                                                */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void OutputStartMessage (void)
{
   printf("\n");
   printf("TinyTAP v%s compiled "__DATE__" "__TIME__"\n", VERSION);
   printf("(c) 2025 by Michael Fischer (www.emb4fun.de)\n");
   printf("\n");

} /* OutputStartMessage */

/*************************************************************************/
/*  Discover                                                             */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: 0 = OK / error cause                                         */
/*************************************************************************/
static int Discover (void)
{
   int              rc = 0;
   int             nIndex;
   int             nServerCount;
   TNP_SERVER       Server;
   char             String[64];
   struct in_addr iaAddr;

   /*
    * Search TAP server
    */
   printf("Searching TinyTAP Server...\n\n");
   rc = tnp_Search(SERVER_NAME);
   if (rc != 0)
   {
      printf("No server found.\n");
   }
   else
   {
      nServerCount = tnp_GetServerCount();

      printf("MAC-Address        IP-Address       Server - Version  Location\n");
      printf("======================================================================\n");

      for (nIndex = 0; nIndex < nServerCount; nIndex++)
      {
         rc = tnp_GetServer(nIndex, &Server);
         if (0 == rc)
         {
            _snprintf(String, sizeof(String), "%02X:%02X:%02X:%02X:%02X:%02X",
               Server.bMACAddress[0], Server.bMACAddress[1], Server.bMACAddress[2],
               Server.bMACAddress[3], Server.bMACAddress[4], Server.bMACAddress[5]);
            printf("%s  ", String);

            iaAddr.s_addr = Server.dAddress;
            printf("%-15s  ", inet_ntoa(iaAddr));

            _snprintf(String, sizeof(String), "%s - v%d.%02d", Server.Name, Server.dFWVersion/100, Server.dFWVersion%100);
            printf("%s   %s\r\n", String, Server.Location);
         }
      }
   }

   return(rc);
} /* Discover */

/*************************************************************************/
/*  TAPDecodeConnect                                                     */
/*                                                                       */
/*  In    : Msg                                                          */
/*  Out   : none                                                         */
/*  Return: 0 = OK / -1 = error                                          */
/*************************************************************************/
static int TAPDecodeConnect (tap_msg_t *Msg)
{
   int rc = -1;

   if (TAP_OK == Msg->Header.Result)
   {
      rc = 0;
      printf("Connected to the TinyTAP server.\n");
   }
   else
   {
      printf("Connection Error, ");

      switch (Msg->Header.Result)
      {
         case           TAP_ERROR: printf("internal.\n");                                      break;
         case        TAP_ERR_OPEN: printf("open.\n");                                          break;
         case TAP_ERR_START_PARAM: printf("wrong parameter:\n");                               break;
         case   TAP_ERR_RUN_PARAM: printf("existing connection uses different parameters:\n"); break;
         default: /* Do nothing */ break;
      }

      if( (TAP_ERR_START_PARAM == Msg->Header.Result) ||
          (TAP_ERR_RUN_PARAM   == Msg->Header.Result) )
      {
         if (Msg->Header.ErrorCode & TAP_ERR_BAUDRATE)
         {
            printf("Baud rate  : %d not supported\n", Param.dBaudrate);
         }
         if (Msg->Header.ErrorCode & TAP_ERR_DATA_BITS)
         {
            printf("Data bits  : %d not supported\n", Param.dData);
         }
         if (Msg->Header.ErrorCode & TAP_ERR_PARITY)
         {
            printf("Parity     : %c not supported\n", Param.Parity);
         }
         if (Msg->Header.ErrorCode & TAP_ERR_STOP_BITS)
         {
            printf("Stop bits  : %d not supported\n", Param.dStop);
         }
      }
   }

   return(rc);
} /* TAPDecodeConnect */

/*************************************************************************/
/*  TAPConnect                                                           */
/*                                                                       */
/*  In    : dAddress                                                     */
/*  Out   : none                                                         */
/*  Return: Socket / SOCKET_INVALID                                      */
/*************************************************************************/
static int TAPConnect (uint32_t dAddress)
{
   int            rc;
   socket_t       Socket;
   struct sockaddr_in  saDest;
   uint8_t        Buffer[MBRTU_MAX_SIZE];
   tap_msg_t     *Msg = (tap_msg_t*)Buffer;

   /* Get socket */
   Socket = socket(AF_INET, SOCK_STREAM, 0);
   if (Socket != SOCKET_INVALID)
   {
      /* Set address and port */
      saDest.sin_addr.s_addr = dAddress;
      saDest.sin_port        = htons(TAP_TCP_SERVER_PORT);
      saDest.sin_family      = AF_INET;

      rc = connect(Socket, (const struct sockaddr*)&saDest, sizeof(struct sockaddr_in));
      if (rc < 0)
      {
         closesocket(Socket);
         Socket = SOCKET_INVALID;
      }
      else
      {
         memset(Buffer, 0x00, sizeof(Buffer));

         /* Send Connect */
         Msg->Header.Magic1  = TAP_HEADER_MAGIC_1;
         Msg->Header.Magic2  = TAP_HEADER_MAGIC_2;
         Msg->Header.SizeVer = TAP_SIZEVER;
         Msg->Header.Func    = TAP_MSG_CONNECT;

         switch(Param.dBaudrate)
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
               Msg->Data.Connect.dBaudrate = Param.dBaudrate; 
               break;
            }
            default  : /* Do nothing */                    break;
         }

         switch(Param.dData)
         {
            case   8: Msg->Data.Connect.eLength = TAP_COM_LENGTH_8; break;
            default : /* Do nothing */                              break;
         }

         switch(Param.Parity)
         {
            case   'N': Msg->Data.Connect.eParity = TAP_COM_PARITY_NONE; break;
            case   'E': Msg->Data.Connect.eParity = TAP_COM_PARITY_EVEN; break;
            case   'O': Msg->Data.Connect.eParity = TAP_COM_PARITY_ODD;  break;
            default : /* Do nothing */                                   break;
         }

         switch(Param.dStop)
         {
            case   1: Msg->Data.Connect.eStop = TAP_COM_STOP_1_0; break;
            default : /* Do nothing */                            break;
         }

         rc = send(Socket, (char*)Buffer, TAP_MSG_HEADER_SIZE + TAP_MSG_CONNECT_SIZE, 0);
         if (rc > 0)
         {
            /* Wait for response */
            rc = recv(Socket, (char*)Buffer, sizeof(Buffer), 0);
            if (rc > 0)
            {
				   rc = TAPDecodeConnect(Msg);
               if (rc != 0)
               {
                  /* Error */
                  closesocket(Socket);
                  Socket = SOCKET_INVALID;
               }
            }
            else
            {
               /* Error */
               closesocket(Socket);
               Socket = SOCKET_INVALID;
            }
         }
         else
         {
            /* Error */
            closesocket(Socket);
            Socket = SOCKET_INVALID;
         }
      }
   }

   return(Socket);
} /* TAPConnect */

/*************************************************************************/
/*  TAPStart                                                             */
/*                                                                       */
/*  In    : dAddress                                                     */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void TAPStart (uint32_t dAddress)
{
   int      rc;
   uint8_t  Buffer[MBRTU_MAX_SIZE];
   
   /* Create UDP sockets */
   UDPClientSocket = socket(AF_INET, SOCK_DGRAM, 0);
   UDPServerSocket = socket(AF_INET, SOCK_DGRAM, 0);
   UDPCliSocket    = socket(AF_INET, SOCK_DGRAM, 0);
   
   if( (UDPClientSocket == SOCKET_INVALID) ||
       (UDPServerSocket == SOCKET_INVALID) ||
       (UDPCliSocket    == SOCKET_INVALID) )
   {
      CloseUDPSocket();
      printf("Error, could not create UDP sockets.\n");
      return;
   }       
   
   /* Connect TinyTAP server */
   TCPSocket = TAPConnect(dAddress);
   if (TCPSocket != SOCKET_INVALID)
   {
      fd_set         readfds;
      struct timeval timeout;
      uint16_t       timeoutCnt;
   
      pthread_t thread_id_client;
      pthread_t thread_id_server;

      #if defined(__MINGW64__)
      signal(SIGINT, ConsoleHandler);
      #else
      struct sigaction sa;
      memset(&sa, 0, sizeof(sa));
      sa.sa_handler = ConsoleHandler;
      sa.sa_flags = SA_RESTART;
      sigemptyset(&sa.sa_mask);

      if (sigaction(SIGINT, &sa, NULL) == -1)
      {
         perror("sigaction");
         return;
      }
      #endif
     
      /* Start UDPClientThread, UDPServerThread and UDPCliThread*/
      pthread_create(&thread_id_client, NULL, UDPClientThread, NULL);
      pthread_create(&thread_id_server, NULL, UDPServerThread, NULL);
      pthread_create(&thread_id_server, NULL, UDPCliThread,    NULL);
      
      /* Wait 1 second, give UDPClientThread,UDPServerThread and UDPCliThread some time to end */
      WaitMS(1000);       

      printf("Press Ctrl+C to disconnect.\n\n");
      
      timeoutCnt = 0;
      while (0 == nEndRequestEnd)
      {
         /* Set NonBlocking socket to SOCKET_TIMEOUT_MS */
         FD_ZERO(&readfds);
         FD_SET(TCPSocket, &readfds);
         timeout.tv_sec  = 0;
         timeout.tv_usec = SOCKET_TIMEOUT_MS * 1000;      
      
         /* Wait for data with timeout */
         rc = select(TCPSocket + 1, &readfds, NULL, NULL, &timeout);
         if (rc > 0)
         {
            /* Read data */
            rc = recv(TCPSocket, (char*)Buffer, sizeof(Buffer), 0);
            
            /* Check for a ping */
            if ( (4 == rc) && (0 == memcmp(Buffer, "ping", 4)) )
            {
               /* Do nothing */
               timeoutCnt = 0;
            }
            else
            {
               /* Data was received do nothing */
            }
         }
         else if (0 == rc)
         {
            /* Timeout */
            timeoutCnt++;
            if (timeoutCnt >= TINYTAP_TIMEOUT_MS / SOCKET_TIMEOUT_MS)
            {
               nEndRequestEnd = 1;
               printf("Timeout TinyTAP server.\n");
               break;            
            }
         }
         else
         {
            /* Error, e.g. disconnect by Ctrl+C */
            break;
         }   
      }
      
      /*
       * Give UDPClientThread and UDPServerThread some time to end
       */
      WaitMS(SOCKET_TIMEOUT_MS * 5);

      printf("Disconnected from TinyTAP server.\n");

      CloseTCPSocket();
      CloseUDPSocket();
   }

} /* TAPStart */

/*=======================================================================*/
/*  All code exported                                                    */
/*=======================================================================*/

/*************************************************************************/
/*  main                                                                 */
/*                                                                       */
/*  In    : argc, argv                                                   */
/*  Out   : none                                                         */
/*  Return: 0 = OK / error cause                                         */
/*************************************************************************/
int main (int argc, char **argv)
{
   int              rc = EXIT_OK;
   TNP_SERVER       Server;
   char             String[64];
   uint32_t        dAddress = 0;
   struct in_addr iaAddr;

   /*
    * Output start message
    */
   OutputStartMessage();

   ParaGetDefault();
   ParaDecode(argc, argv);

   /* Version information requested */
   if (1 == Param.CmdVersion)
   {
      /* Only OutputStartMessage was needed */
      exit(EXIT_OK);
   }

   /*
    * Start the TNP protocol
    */
   rc = tnp_Start();
   if (rc != 0)
   {
      printf("Error: tnp_Start() failed with code %d\n", rc);
      GOTO_END(rc);
   }

   /*
    * Check for discover command only
    */
   if (1 == Param.CmdDiscover)
   {
      Discover();
      GOTO_END(EXIT_OK);
   }

   /********************************************/
   /*  At this point all parameter was parsed  */
   /********************************************/

   /*
    * Check if a server should be selected automatically
    */
   if (0 == Param.CmdIP)
   {
      /* Search server */
      printf("Searching TinyTAP Server...\n\n");
      rc = tnp_Search(SERVER_NAME);
      if (rc != 0)
      {
         printf("No server found.\n");
         GOTO_END(EXIT_ERR_NO_SERVER);
      }
   }
   else
   {
      /* Use the given IP-Address */
      dAddress = inet_addr(Param.IPName);
      if (INADDR_NONE == dAddress)
      {
         printf("Error, IP-Address invalid: %s\n", Param.IPName);
         GOTO_END(EXIT_ERR_IPADDRESS);
      }
   }

   printf("TinyTAP parameters\n");
   printf("===================\n");

   if (1 == Param.CmdIP)
   {
      /* Use the given IP-Address */
      iaAddr.s_addr = dAddress;
      printf("Server     : %s\n", inet_ntoa(iaAddr));
   }
   else
   {
      /* Use the first server which was found in the network */
      rc = tnp_GetServer(0, &Server);
      if (0 != rc) GOTO_END(EXIT_ERR_NO_SERVER);

      dAddress = Server.dAddress;
      iaAddr.s_addr = dAddress;
      printf("Server     : %s  ", inet_ntoa(iaAddr));

      /* Output additional server informations */
      _snprintf(String, sizeof(String), "%02X:%02X:%02X:%02X:%02X:%02X",
         Server.bMACAddress[0], Server.bMACAddress[1], Server.bMACAddress[2],
         Server.bMACAddress[3], Server.bMACAddress[4], Server.bMACAddress[5]);
      printf("%s  ", String);

      _snprintf(String, sizeof(String), "%s - v%d.%02d", Server.Name, Server.dFWVersion/100, Server.dFWVersion%100);
      printf("%s  %s\r\n", String, Server.Location);
   }
   printf("Baud rate  : %d\n", Param.dBaudrate);
   printf("Data bits  : %d\n", Param.dData);
   printf("Parity     : %c\n", Param.Parity);
   printf("Stop bits  : %d\n", Param.dStop);
   printf("\n");

   TAPStart(dAddress);

end:

   /*
    * Cleanup: Close all sockets and stop TNP protocol
    */
   CloseTCPSocket();
   CloseUDPSocket();

   tnp_Stop();

   return(rc);
} /* main */

/*** EOF ***/
