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
#if !defined(__MBRTU_TAP_H__)
#define __MBRTU_TAP_H__

/**************************************************************************
*  Includes
**************************************************************************/

#ifdef _MSC_VER
#include <windows.h>
#include "stdint.h"

#pragma pack(1)
#define PACKED(_a)   _a
#else
#include <stdint.h>
#endif


#if !defined(PACKED)
#define PACKED(_a)    __attribute__((__packed__)) _a
#endif   

/**************************************************************************
*  Global Definitions
**************************************************************************/

/*
 * TAP server port
 */
#define TAP_TCP_SERVER_PORT   54323


/*
 * TAP header infos
 */
#define TAP_HEADER_MAGIC_1    0x5452424D  // "MBRTUTAP"
#define TAP_HEADER_MAGIC_2    0x50415455
#define TAP_SIZEVER           ((((uint32_t)sizeof(tap_header_t)) << 16) | TAP_VERSION)
#define TAP_HEADER_SIZE       sizeof(tap_header_t)
#define TAP_VERSION           1


/*
 * TAP error
 */
#define TAP_OK                0
#define TAP_ERROR             -1
#define TAP_ERR_OPEN          -2
#define TAP_ERR_START_PARAM   -3    /* Wrong parameter to start */
#define TAP_ERR_RUN_PARAM     -4    /* Wrong parameter, still running */

#define TAP_ERR_BAUDRATE      (1 <<  0)
#define TAP_ERR_DATA_BITS     (1 <<  1)
#define TAP_ERR_PARITY        (1 <<  2)
#define TAP_ERR_STOP_BITS     (1 <<  3)

/*************************************************************************/

typedef enum _tap_com_length_
{
   TAP_COM_LENGTH_8 = 0,
   TAP_COM_LENGTH_9
} TAP_COM_LENGTH;

typedef enum _tap_com_parity_
{
   TAP_COM_PARITY_NONE = 0,
   TAP_COM_PARITY_EVEN,
   TAP_COM_PARITY_ODD
} TAP_COM_PARITY;

typedef enum _tap_com_stop_
{
   TAP_COM_STOP_0_5 = 0,
   TAP_COM_STOP_1_0,
   TAP_COM_STOP_1_5,
   TAP_COM_STOP_2_0
} TAP_COM_STOP;

/*************************************************************************/

typedef struct
{
  uint32_t  dBaudrate;
  uint8_t   eLength;
  uint8_t   eParity;
  uint8_t   eStop;
} PACKED(tap_msg_connect_t);
#define TAP_MSG_CONNECT_SIZE  sizeof(tap_msg_connect_t)

/*************************************************************************/

typedef union
{
   tap_msg_connect_t Connect;
   
} PACKED(tap_data_t);

/*************************************************************************/

typedef enum
{
   TAP_MSG_CONNECT = 0,
   
   /**************************/
   TAP_MSG_END = 0xFFFFFFFF
} tap_msg_func;

/*************************************************************************/

typedef struct _tap_header_
{
   uint32_t     Magic1; 
   uint32_t     Magic2; 
   uint32_t     SizeVer;
   tap_msg_func Func;
   uint32_t     Len;
   int32_t      Result;
   uint32_t     ErrorCode;
} PACKED(tap_header_t);
#define TAP_MSG_HEADER_SIZE   sizeof(tap_header_t)

typedef struct _tap_msg_
{
   tap_header_t Header;
   tap_data_t   Data;
} PACKED(tap_msg_t); 


#ifdef _WINDOWS
#pragma pack()
#endif

/**************************************************************************
*  Macro Definitions
**************************************************************************/

/**************************************************************************
*  Functions Definitions
**************************************************************************/

void mbrtu_TapInitRS485 (void);
void mbrtu_TapInit (void);

void mbrtu_TapStart (void);

#endif /* !__MBRTU_TAP_H__ */

/*** EOF ***/
