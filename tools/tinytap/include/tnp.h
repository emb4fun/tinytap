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
#if !defined(__TNP_H__)
#define __TNP_H__

/**************************************************************************
*  Includes
**************************************************************************/
#include "compat.h"

/**************************************************************************
*  Global Definitions
**************************************************************************/

#define TNP_HEADER_MAGIC_1    0x54504E54
#define TNP_HEADER_MAGIC_2    0x4E54504E
#define TNP_HEADER_VERSION    1

#define TNP_UDP_PORT          54321
#define TNP_NAME_LEN          16
#define TNP_LOCATION_LEN      16
#define TNP_MDNS_NAME_LEN     32

#define TNP_MAX_NAME_LEN      (TNP_NAME_LEN+1)
#define TNP_MAX_LOCATION_LEN  (TNP_LOCATION_LEN+1)
#define TNP_MAX_MDNS_NAME_LEN (TNP_MDNS_NAME_LEN+1)

enum
{
  TNP_SETUP_REQUEST = 0,
  TNP_SETUP_RESPONSE,
  TNP_SETUP_SET,
  TNP_SETUP_RESPONSE_ES
};

#ifdef _MSC_VER
#pragma pack(1)
#endif
typedef struct _tnp_setup_
{
  uint32_t dMagic1;
  uint32_t dMagic2;
  uint16_t wSize;
  uint16_t wVersion;

  uint8_t  bReserve;
  uint8_t  bMode;
  uint8_t  bUseDHCP;
  uint8_t  bMACAddress[6];
  uint32_t dAddress;
  uint32_t dMask;
  uint32_t dGateway;
  uint32_t dFWVersion;
  char      Name[TNP_MAX_NAME_LEN];
  char      Location[TNP_MAX_LOCATION_LEN];
  char      MDNSName[TNP_MAX_MDNS_NAME_LEN];
} PACKED(TNP_SETUP);
#ifdef _MSC_VER
#pragma pack()
#endif


typedef struct _tnp_server_
{
  uint8_t  bMACAddress[6];
  uint32_t dAddress;
  uint32_t dFWVersion;
  char      Name[TNP_MAX_NAME_LEN];
  char      Location[TNP_MAX_LOCATION_LEN];
} TNP_SERVER;

/**************************************************************************
*  Macro Definitions
**************************************************************************/

/**************************************************************************
*  Funtions Definitions
**************************************************************************/

int  tnp_Start (void);
void tnp_Stop (void);

int  tnp_Search (const char *ServerName);
int  tnp_GetServerCount (void);

int  tnp_GetServer (int nIndex, TNP_SERVER *pServer);

#endif /* !__TNP_H__ */

/*** EOF ***/
