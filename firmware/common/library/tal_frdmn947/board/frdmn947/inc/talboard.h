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
#if !defined(__TALBOARD_H__) && defined(USE_BOARD_FRDMN947)
#define __TALBOARD_H__

/**************************************************************************
*  Includes
**************************************************************************/
#include <time.h>
#include "tcts.h"
#include "project.h"
#include "taltypes.h"
#include "heap_conf.h"
#include "talcom.h"
#include "mcx_cpu.h"

/**************************************************************************
*  Global Definitions
**************************************************************************/

/*
 * Defines for board and cpu name
 */
#define TAL_BOARD "FRDM-MCXN947"
#define TAL_CPU   "MCXN947"

/*
 * With 3 (__NVIC_PRIO_BITS) bits of interrupt priority 7 levels are
 * available. Priority 0 is the highest one, 7 the lowest.
 */

#if defined(RTOS_FREERTOS)
#define SYSTICK_PRIO       configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
#endif

#ifndef SYSTICK_PRIO
#define SYSTICK_PRIO       0
#endif

#ifndef LPUART0_PRIO
#define LPUART0_PRIO       7
#endif

#ifndef LPUART4_PRIO
#define LPUART4_PRIO       7
#endif

#ifndef LPUART5_PRIO
#define LPUART5_PRIO       7
#endif

#ifndef LPUART8_PRIO
#define LPUART8_PRIO       7
#endif

#ifndef CAN0_PRIO
#define CAN0_PRIO          6
#endif

#ifndef CAN1_PRIO
#define CAN1_PRIO          6
#endif


/*
 * The FRDM-MCXN947 board support 3 LEDs
 */
typedef enum _tal_led_channel_
{
   TAL_LED_CHANNEL_1 = 0,
   TAL_LED_CHANNEL_2,
   TAL_LED_CHANNEL_3,

   /* TAL_LED_CHANNEL_MAX must be the last one */
   TAL_LED_CHANNEL_MAX
} TAL_LED_CHANNEL;

#define LED_RED   TAL_LED_CHANNEL_1
#define LED_GREEN TAL_LED_CHANNEL_2
#define LED_BLUE  TAL_LED_CHANNEL_3


/*
 * Click-1    : TAL_COM_PORT_9
 * Click-2    : TAL_COM_PORT_1
 * Click-3    : TAL_COM_PORT_6
 * J-Link UART: TAL_COM_PORT_5
 */

/*
 * COM port used for the terminal
 */
#if !defined(TERM_COM_PORT)
#define TERM_COM_PORT   TAL_COM_PORT_5
#endif

/**************************************************************************
*  Macro Definitions
**************************************************************************/

/**************************************************************************
*  Functions Definitions
**************************************************************************/

TAL_RESULT tal_BoardEnableCOM1 (void);
TAL_RESULT tal_BoardEnableCOM2 (void);
TAL_RESULT tal_BoardEnableCOM3 (void);
TAL_RESULT tal_BoardEnableCOM4 (void);
TAL_RESULT tal_BoardEnableCOM5 (void);
TAL_RESULT tal_BoardEnableCOM6 (void);
TAL_RESULT tal_BoardEnableCOM7 (void);
TAL_RESULT tal_BoardEnableCOM8 (void);
TAL_RESULT tal_BoardEnableCOM9 (void);

TAL_RESULT tal_BoardEnableCAN1 (void);
TAL_RESULT tal_BoardEnableCAN2 (void);

TAL_RESULT tal_BoardGetMACAddress (int iface, uint8_t *pAddress);

void       tal_BoardRTCSetTM (struct tm *pTM);
void       tal_BoardRTCSetUnixtime (uint32_t Unixtime);
void       tal_BoardRTC2System (void);


/*
 * USB Device
 */
uint8_t USB_ControllerIdGet (void);
void    USB_DeviceClockInit (void);
void    USB_DeviceIsrEnable (void);

extern void USB_DeviceEhciIsrFunction(void *deviceHandle);
#define USB_DeviceIsrFunction    USB_DeviceEhciIsrFunction


/*
 * PSRAM
 */
void BoardPSRAMInit (void);

#endif /* !__TALBOARD_H__ */

/*** EOF ***/
