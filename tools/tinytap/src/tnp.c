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
#define __TNP_C__

/*=======================================================================*/
/*  Includes                                                             */
/*=======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compat.h"
#include "tnp.h"

/*=======================================================================*/
/*  All Structures and Common Constants                                  */
/*=======================================================================*/

#define MAX_IFACE_CNT   8
#define MAX_SERVER_CNT  8

#define GOTO_END(_a)    { rc = _a; goto end; }


/*
 * Added support for SIO_GET_INTERFACE_LIST
 */
#if defined(_MSC_VER) || defined(__MINGW64__)

#define SIO_GET_INTERFACE_LIST   _IOR ('t', 127, ULONG)

typedef struct in6_addr
{
   union
   {
      uint8_t  Byte[16];
      uint16_t Word[8];
   } u;
} IN6_ADDR, *PIN6_ADDR, *LPIN6_ADDR;

struct sockaddr_in6_old
{
   int16_t  sin6_family;
   uint16_t sin6_port;
   uint32_t sin6_flowinfo;
   IN6_ADDR sin6_addr;
};

typedef union sockaddr_gen
{
   struct sockaddr         Address;
   struct sockaddr_in      AddressIn;
   struct sockaddr_in6_old AddressIn6;
} sockaddr_gen;

typedef struct _INTERFACE_INFO
{
   uint32_t        iiFlags;
   sockaddr_gen iiAddress;
   sockaddr_gen iiBroadcastAddress;
   sockaddr_gen iiNetmask;
} INTERFACE_INFO, *LPINTERFACE_INFO;
#endif /* defined(_MSC_VER) || defined(__MINGW64__) */

/*=======================================================================*/
/*  Definition of all local Data                                         */
/*=======================================================================*/

typedef struct _iface_
{
   socket_t  Socket;
   uint32_t dAddress;
   char      IfName[16];
} IFACE;

#if defined(_MSC_VER) || defined(__MINGW64__)
static int        nWSAInitDone = 0;
#endif

static int        nIfaceCount;
static IFACE       IfaceList[MAX_IFACE_CNT];

static int        nServerCount;
static TNP_SERVER  ServerList[MAX_SERVER_CNT];

/*=======================================================================*/
/*  Definition of prototypes                                             */
/*=======================================================================*/

/*=======================================================================*/
/*  Definition of all local Procedures                                   */
/*=======================================================================*/

/*************************************************************************/
/*  GetInterfaceList                                                     */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
#if defined(_MSC_VER) || defined(__MINGW64__)
static void GetInterfaceList (void)
{
   int                  rc;
   socket_t             Socket;
   INTERFACE_INFO       InterfaceList[MAX_IFACE_CNT];
   struct sockaddr_in *pAddress;
   int                 nInterfacesCnt;
   DWORD               dBytesReturn;
   uint32_t            dValue;
   int                 nIndex;
   int                 nOptionValue;

   /* Default, clear data */
   nIfaceCount = 0;
   memset(&IfaceList, 0x00, sizeof(IfaceList));

   /* Get interface list */
   Socket = socket(AF_INET, SOCK_DGRAM, 0);
   if (Socket != SOCKET_INVALID)
   {
      /* Try to get the Interface List info */
      rc = WSAIoctl(Socket, SIO_GET_INTERFACE_LIST, NULL, 0,
                    InterfaceList, sizeof(InterfaceList),
                    &dBytesReturn, NULL, NULL);
      if (0 == rc)
      {
         /* The socket is not needed anymore */
         closesocket(Socket);

         /* Get interface count */
         nInterfacesCnt = dBytesReturn / sizeof(INTERFACE_INFO);

         nIfaceCount = 0;
         for (nIndex = 0; nIndex < nInterfacesCnt; nIndex++)
         {
            /* Address*/
            pAddress = (struct sockaddr_in *)&(InterfaceList[nIndex].iiAddress);
            dValue = ntohl(pAddress->sin_addr.s_addr);

            /* Check for 127.0.0.1, this is not needed here */
            if ((dValue != 0x7F000001) && (nIfaceCount < MAX_IFACE_CNT))
            {
               /* Create interface socket */
               IfaceList[nIfaceCount].Socket = socket(AF_INET, SOCK_DGRAM, 0);
               if (SOCKET_INVALID == IfaceList[nIfaceCount].Socket)
               {
                  /* Fatal error */
                  exit(0);
               }

               /* Add socket option BROADCAST */
               nOptionValue = 1;
               setsockopt(IfaceList[nIfaceCount].Socket, SOL_SOCKET, SO_BROADCAST, (char *)&nOptionValue, sizeof(nOptionValue));

               /* Add socket option REUSEADDR */
               nOptionValue = 1;
               setsockopt(IfaceList[nIfaceCount].Socket, SOL_SOCKET, SO_REUSEADDR, (char *)&nOptionValue, sizeof(nOptionValue));

               /* Set socket option RCVTIMEO to 200ms*/
               nOptionValue = 200;
               setsockopt(IfaceList[nIfaceCount].Socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&nOptionValue, sizeof(nOptionValue));

               /* Save address info */
               IfaceList[nIfaceCount].dAddress = dValue;

               nIfaceCount++;
            }
         }
      }
   }

} /* GetInterfaceList */
#endif /* defined(_MSC_VER) || defined(__MINGW64__) */
#if defined(__unix__) || defined(__APPLE__)
static void GetInterfaceList (void)
{
   int nOptionValue;

   /* Default, clear data */
   nIfaceCount = 0;
   memset(&IfaceList, 0x00, sizeof(IfaceList));

   /* Get interface list */
   struct ifaddrs *ifaddr, *ifa;
   if (getifaddrs(&ifaddr) == -1)
   {
      perror("getifaddrs");
      return;
   }

   for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
   {
      if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET)
         continue;

      /* No Loopback needed */
      if (ifa->ifa_flags & IFF_LOOPBACK)
         continue;

      /* No bridges needed */
      if (strncmp(ifa->ifa_name, "bridge", 6) == 0)
         continue;

      struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
      //printf("Interface: %s\tIP: %s\n", ifa->ifa_name, inet_ntoa(sa->sin_addr));

      if (nIfaceCount < MAX_IFACE_CNT)
      {
         struct timeval timeout;
         timeout.tv_sec = 0;
         timeout.tv_usec = 200000; // 200 ms

         /* Create interface socket */
         IfaceList[nIfaceCount].Socket = socket(AF_INET, SOCK_DGRAM, 0);
         if (SOCKET_INVALID == IfaceList[nIfaceCount].Socket)
         {
            /* Fatal error */
            exit(0);
         }

         snprintf(IfaceList[nIfaceCount].IfName,
                  sizeof(IfaceList[nIfaceCount].IfName),
                  "%s", ifa->ifa_name);

         /* Add socket option BROADCAST */
         nOptionValue = 1;
         setsockopt(IfaceList[nIfaceCount].Socket, SOL_SOCKET, SO_BROADCAST, (char *)&nOptionValue, sizeof(nOptionValue));

         /* Add socket option REUSEADDR */
         nOptionValue = 1;
         setsockopt(IfaceList[nIfaceCount].Socket, SOL_SOCKET, SO_REUSEADDR, (char *)&nOptionValue, sizeof(nOptionValue));

         /* Add socket option REUSEPORT */
         nOptionValue = 1;
         setsockopt(IfaceList[nIfaceCount].Socket, SOL_SOCKET, SO_REUSEPORT, (char *)&nOptionValue, sizeof(nOptionValue));

         /* Set socket option RCVTIMEO to 200ms*/
         setsockopt(IfaceList[nIfaceCount].Socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

         /* Save address info */
         IfaceList[nIfaceCount].dAddress = ntohl(sa->sin_addr.s_addr);

         nIfaceCount++;
      }
   }

   freeifaddrs(ifaddr);

} /* GetInterfaceList */
#endif /* defined(__unix__) || defined(__APPLE__) */

/*=======================================================================*/
/*  All code exported                                                    */
/*=======================================================================*/

/*************************************************************************/
/*  tnp_Start                                                            */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: 0 = OK / error cause                                         */
/*************************************************************************/
int tnp_Start (void)
{
   int                rc = 0;
   int               nIndex;
   struct sockaddr_in saSource;

   /*
    * Initialize Winsocket
    */
#if defined(__MINGW64__) || defined(_MSC_VER)
   WSADATA        WSAData;
   uint16_t      wVersion = WINSOCK_VERSION;

   if (WSAStartup(wVersion, &WSAData) != 0)
   {
      printf("WSAStartup failed. Error: %d\r\n", WSAGetLastError());
      exit(-1);
   }
   nWSAInitDone = 1;
#endif

   /*
    * Get interface list
    */
   GetInterfaceList();
   if(0 == nIfaceCount)
   {
      printf("Error, could not find any ethernet interfaces.\r\n");
      GOTO_END(-1);
   }

   /*
    * Bind to interface
    */
   for (nIndex = 0; nIndex < nIfaceCount; nIndex++)
   {
      /* Set address and port */
      saSource.sin_addr.s_addr = htonl(INADDR_ANY);
      saSource.sin_port        = htons(TNP_UDP_PORT);
      saSource.sin_family      = AF_INET;

#if defined(__APPLE__)
      unsigned int ifindex = if_nametoindex(IfaceList[nIndex].IfName);
      setsockopt(IfaceList[nIndex].Socket, IPPROTO_IP, IP_BOUND_IF, &ifindex, sizeof(ifindex));
#if 0
      printf("Binding socket %d (%s) to INADDR_ANY:%d\n",
             IfaceList[nIndex].Socket,
             IfaceList[nIndex].IfName,
             TNP_UDP_PORT);

      printf("Binding to interface index %u (%s)\n",
             ifindex,
             IfaceList[nIndex].IfName);
#endif
#endif /* defined(__APPLE__) */
#if defined(__unix__)
      setsockopt(IfaceList[nIndex].Socket, IPPROTO_IP, SO_BINDTODEVICE,
                 IfaceList[nIndex].IfName, strlen(IfaceList[nIndex].IfName));
#if 0
      printf("Binding socket %d (%s) to INADDR_ANY:%d\n",
             IfaceList[nIndex].Socket,
             IfaceList[nIndex].IfName,
             TNP_UDP_PORT);

      printf("Binding to interface index %u (%s)\n",
             ifindex,
             IfaceList[nIndex].IfName);
#endif
#endif /* defined(__unix__) */

      /* bind socket */
      rc = bind(IfaceList[nIndex].Socket, (const struct sockaddr*)&saSource, sizeof(struct sockaddr_in));
      if (rc != 0)
      {
#if 0
         perror("bind");
         printf("errno: %d\n", errno);
#endif      
         printf("Error, could not bind interfaces.\r\n");
         printf("Please close the \"Tiny Network Explorer\".\r\n");
         GOTO_END(-2);
      }
   }

   rc = 0;

end:

   return(rc);
} /* tnp_Start */

/*************************************************************************/
/*  tnp_Stop                                                             */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void tnp_Stop (void)
{
#if defined(__MINGW64__) || defined(_MSC_VER)
   if (1 == nWSAInitDone)
   {
      WSACleanup();
   }
#endif

} /* tnp_Stop */

/*************************************************************************/
/*  tnp_Search                                                           */
/*                                                                       */
/*  In    : ServerName                                                   */
/*  Out   : none                                                         */
/*  Return: 0 = OK / error cause                                         */
/*************************************************************************/
int tnp_Search (const char *ServerName)
{
   int                 rc = 0;
   int                nIndex;
   int                nAddressLen;
   struct sockaddr_in saDest;
   struct sockaddr_in saSource;
   TNP_SETUP            Setup;

   /* Default, clear data */
   nServerCount = 0;
   memset(&ServerList, 0x00, sizeof(ServerList));

   /*
    * Send request
    */

   /* Fill the packet */
   memset(&Setup, 0x00, sizeof(TNP_SETUP));
   Setup.dMagic1  = TNP_HEADER_MAGIC_1;
   Setup.dMagic2  = TNP_HEADER_MAGIC_2;
   Setup.wSize    = sizeof(TNP_SETUP);
   Setup.wVersion = TNP_HEADER_VERSION;
   Setup.bMode    = TNP_SETUP_REQUEST;

   /* Send to all interfaces */
   for (nIndex = 0; nIndex < nIfaceCount; nIndex++)
   {
      /* Set address and port */
      saDest.sin_addr.s_addr = INADDR_BROADCAST;
      saDest.sin_port        = htons(TNP_UDP_PORT);
      saDest.sin_family      = AF_INET;

      /* Send the packet */
      sendto(IfaceList[nIndex].Socket, (const char *)&Setup, sizeof(TNP_SETUP), 0,
             (const struct sockaddr*)&saDest, sizeof(struct sockaddr_in));
   }

   /*
    * Wait for response
    */

   /* Check all interfaces */
   for (nIndex = 0; nIndex < nIfaceCount; nIndex++)
   {
      while (1)
      {
         nAddressLen = sizeof(struct sockaddr_in);
         rc = recvfrom(IfaceList[nIndex].Socket,
                       (char *)&Setup,
                       sizeof(TNP_SETUP),
                       0,
                       (struct sockaddr*)&saSource,
                       (socklen_t*)&nAddressLen);

         if( (rc             != SOCKET_ERROR)       &&
             (Setup.dMagic1  == TNP_HEADER_MAGIC_1) &&
             (Setup.dMagic2  == TNP_HEADER_MAGIC_2) &&
             (Setup.wSize    == sizeof(TNP_SETUP))  &&
             (Setup.wVersion == TNP_HEADER_VERSION) )
         {
            /* Check if MAC addres is != 0 */
            if ((0 == Setup.bMACAddress[0]) &&
                (0 == Setup.bMACAddress[1]) &&
                (0 == Setup.bMACAddress[2]) &&
                (0 == Setup.bMACAddress[3]) &&
                (0 == Setup.bMACAddress[4]) &&
                (0 == Setup.bMACAddress[5]) )
            {
               /* Do nothing, this was the request we have send */
            }
            else
            {
               /* Check for response */
               if( (Setup.bMode == TNP_SETUP_RESPONSE)    ||
                   (Setup.bMode == TNP_SETUP_RESPONSE_ES) )
               {
                  /* Check if this is a "TinyTAP" server */
                  if ((0 == strcmp(Setup.Name, ServerName)) && (nServerCount < MAX_SERVER_CNT))
                  {
                     /* Check if the server is already in the list */
                     int  i;
                     int nIsAvailable = 0;
                     for (i = 0; i < nServerCount; i++)
                     {
                        if (0 == memcmp(ServerList[i].bMACAddress, Setup.bMACAddress, 6))
                        {
                           /* The server is already in the list */
                           nIsAvailable = 1;
                           break;
                        }
                     }
                     if (0 == nIsAvailable)
                     {
                        /* Copy server data */
                        memcpy(ServerList[nServerCount].bMACAddress, Setup.bMACAddress, 6);
                        ServerList[nServerCount].dAddress   = Setup.dAddress;
                        ServerList[nServerCount].dFWVersion = Setup.dFWVersion;
                        memcpy(ServerList[nServerCount].Name, Setup.Name, TNP_MAX_NAME_LEN);
                        memcpy(ServerList[nServerCount].Location, Setup.Location, TNP_MAX_LOCATION_LEN);

                        nServerCount++;
                     }
                  }
               }
            }
         }
         else
         {
            /* No more data from this interface */
            break;
         }
      }
   }

   if (0 == nServerCount)
   {
      /* Error, no server found */
      rc = -1;
   }
   else
   {
      /* No error */
      rc = 0;
   }

   return(rc);
} /* tnp_Search */

/*************************************************************************/
/*  tnp_GetServerCount                                                   */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: nServerCount                                                 */
/*************************************************************************/
int tnp_GetServerCount (void)
{
   return(nServerCount);
} /* tnp_GetServerCount */

/*************************************************************************/
/*  tnp_GetServer                                                        */
/*                                                                       */
/*  In    : nIndex, pServer                                              */
/*  Out   : pServer                                                      */
/*  Return: 0 = OK / error cause                                         */
/*************************************************************************/
int tnp_GetServer (int nIndex, TNP_SERVER *pServer)
{
   int rc = -1;

   /* Check for valid parameters */
   if ((nIndex >= 0) && (nIndex < nServerCount) && (pServer != NULL))
   {
      memcpy(pServer, &ServerList[nIndex], sizeof(TNP_SERVER));
      rc = 0;
   }

   return(rc);
} /* tnp_GetServer */

/*** EOF ***/
