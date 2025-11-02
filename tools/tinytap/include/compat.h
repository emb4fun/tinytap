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
#if !defined(__COMPAT_H__)
#define __COMPAT_H__

/**************************************************************************
*  Includes
**************************************************************************/

#if defined(_MSC_VER)
#include <windows.h>
#endif


#if defined(__MINGW64__)
#include <winsock2.h>
#include <windows.h>
#include <pthread.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#endif


#if defined(__unix__) || defined(__APPLE__)
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <signal.h>
#endif

/**************************************************************************
*  Global Definitions
**************************************************************************/

#if defined(_MSC_VER)
typedef DWORD64   uint64_t;
typedef LONG64    int64_t;
typedef DWORD     uint32_t;
typedef LONG      int32_t;
typedef WORD      uint16_t;
typedef SHORT     int16_t;
typedef BYTE      uint8_t;

#define SOCKET_INVALID  INVALID_SOCKET
typedef SOCKET    socket_t;
typedef int       socklen_t;

#define ETIMEDOUT    WSAETIMEDOUT
#define NET_ERRNO    WSAGetLastError()

#define PACKED(_a)   _a
#endif /* defined(_MSC_VER) */


#if defined(__MINGW64__)
typedef SOCKET    socket_t;
typedef int       socklen_t;

#define SOCKET_INVALID  INVALID_SOCKET
typedef SOCKET          socket_t;

#define NET_ERRNO       WSAGetLastError()

#define PACKED(_a)      __attribute__((__packed__)) _a
#endif /* defined(__MINGW64__) */


#if defined(__unix__) || defined(__APPLE__)
typedef int socket_t;
#define SOCKET_INVALID  -1
#define SOCKET_ERROR    -1

#define NET_ERRNO       errno

#define closesocket(a)  close(a)
#define _snprintf       snprintf

#define PACKED(_a)      __attribute__((__packed__)) _a
#endif

/**************************************************************************
*  Macro Definitions
**************************************************************************/

/**************************************************************************
*  Funtions Definitions
**************************************************************************/

#endif /* !__COMPAT_H__ */

/*** EOF ***/
