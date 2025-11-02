/**************************************************************************
*  Copyright (c) 2020-2025 by Michael Fischer (www.emb4fun.de).
*  All rights reserved.
*
*  Based on an example from Freescale and NXP.
*  Therefore partial copyright:
*
*  Copyright (c) 2013-2016, Freescale Semiconductor, Inc.
*  Copyright 2016-2024 NXP
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
*
***************************************************************************
*  History:
*
*  07.06.2020  mifi  First Version for the MIMXRT1050-EVKB Evaluation Kit.
*  11.06.2020  mifi  Tested with the iMX RT1062 Developers Kit.
*  16.03.2025  mifi  Tested with the FRDM-RW612 board.
**************************************************************************/

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#define __ETHERNETIF_C__

/*=======================================================================*/
/*  Include                                                              */
/*=======================================================================*/
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "tal.h"
#include "tcts.h"

#include "ethernetif.h"
#include "project.h"

#include "lwip/tcpip.h"
#include "netif/etharp.h"
#include "lwip/ethip6.h"
#include "lwip/stats.h"
#include "lwip/igmp.h"

#include "arch/sys_arch.h"

#include "fsl_enet.h"
#include "fsl_phy.h"


/* Forward declarations. */
static void ethernetif_input (void *arg);

/*=======================================================================*/
/*  Global                                                               */
/*=======================================================================*/

/*=======================================================================*/
/*  Extern                                                               */
/*=======================================================================*/

extern void                BoardETHConfig (void);
extern struct _phy_handle *BoardGetPhyHandle (int iface);

/*=======================================================================*/
/*  All Structures and Common Constants                                  */
/*=======================================================================*/

#define TX_READY_EVENT  1

#define IFNAME0 'e'
#define IFNAME1 'n'

#define LINK_TMR_INTERVAL     1000   

#define FSL_ENET_BUFF_ALIGNMENT ENET_BUFF_ALIGNMENT

typedef uint8_t rx_buffer_t[SDK_SIZEALIGN(ENET_RXBUFF_SIZE, FSL_ENET_BUFF_ALIGNMENT)];
typedef uint8_t tx_buffer_t[SDK_SIZEALIGN(ENET_TXBUFF_SIZE, FSL_ENET_BUFF_ALIGNMENT)];

/**
 * Helper struct to hold private data used to operate your ethernet interface.
 */
struct ethernetif
{
   ENET_Type           *base;
   struct _phy_handle  *phyHandle;
   uint32_t             instance;
   char                 hostname[16];
   
   enet_handle_t        handle;

   bool                 OldLink;   
   OS_EVENT             TxEvent;
   OS_SEMA              RxSema;

   enet_rx_bd_struct_t *RxBuffDescrip;
   enet_tx_bd_struct_t *TxBuffDescrip;
   rx_buffer_t         *RxDataBuff;
//   tx_buffer_t         *TxDataBuff;
   
   uint32_t               rxBufferStartAddr[ENET_RXBD_NUM];
   enet_tx_reclaim_info_t txDirty[ENET_TXBD_NUM];   
};

/*=======================================================================*/
/*  Definition of all global and local Data                              */
/*=======================================================================*/

static uint32_t InitPinsDone = 0;

/*=======================================================================*/
/*  Definition of all local Procedures                                   */
/*=======================================================================*/

/**
 * Called from ENET ISR.
 */
static void ethernet_callback (ENET_Type *base, enet_handle_t *handle, enet_event_t event, uint8_t channel, enet_tx_reclaim_info_t *txReclaimInfo, void *userData)
{
   struct netif      *netif = (struct netif *)userData;
   struct ethernetif *ethernetif = netif->state;
   
   (void)base;   
   (void)handle;
   (void)channel;
   (void)txReclaimInfo;

   switch (event)
   {
      case kENET_RxIntEvent:
      {
         /* Send an "event" to wakeup the Receiver task */
         OS_SemaSignalFromInt(&ethernetif->RxSema);
         break;
      }
      
      case kENET_TxIntEvent:
      {
         /* Send an "event" for TX ready */
         OS_EventSetFromInt(&ethernetif->TxEvent, TX_READY_EVENT);
         break;
      }
    
      default:
      {
         /* Do nothing here */
         break;
      }                  
   }

} /* ethernet_callback */


/**
 * This function handle the IGMP filter ADD/DEL functionality.
 */
#if defined(LWIP_IGMP) && (LWIP_IGMP >= 1)
static err_t ethernetif_igmp_mac_filter (struct netif *netif, const ip4_addr_t *group, enum netif_mac_filter_action action)
{
   struct ethernetif *ethernetif = netif->state;
   err_t result;
   
   (void)group;

   switch (action)
   {
      case IGMP_ADD_MAC_FILTER:
      {
         /* LPC ENET does not accept multicast selectively,
          * so all multicast has to be passed through. */
         ENET_AcceptAllMulticast(ethernetif->base);
         result = ERR_OK;
         break;
      }         
      case IGMP_DEL_MAC_FILTER:
      {
         /*
          * Moves the ENET device from a multicast group.
          * Since we don't keep track of which multicast groups
          * are still to enabled, the call is commented out.
          */
         /* ENET_RejectAllMulticast(ethernetif->base); */
         result = ERR_OK;
         break;
      }         
      default:
      {
         result = ERR_IF;
         break;
      }         
   }

   return(result);
} /* ethernetif_igmp_mac_filter */
#endif

/**
 * This function handle the MLD filter ADD/DEL functionality.
 */
#if defined(LWIP_IPV6) && (LWIP_IPV6 >= 1)
static err_t ethernetif_mld_mac_filter (struct netif *netif, const ip6_addr_t *group, enum netif_mac_filter_action action)
{
   struct ethernetif *ethernetif = netif->state;
   err_t result;
   
   (void)group;

   switch (action)
   {
      case IGMP_ADD_MAC_FILTER:
      {
         /* LPC ENET does not accept multicast selectively,
          * so all multicast has to be passed through. */
         ENET_AcceptAllMulticast(ethernetif->base);
         result = ERR_OK;
         break;
      }         
      case IGMP_DEL_MAC_FILTER:
      {
         /*
          * Moves the ENET device from a multicast group.
          * Since we don't keep track of which multicast groups
          * are still to enabled, the call is commented out.
          */
         /* ENET_RejectAllMulticast(ethernetif->base); */
         result = ERR_OK;
         break;
      }         
      default:
      {
         result = ERR_IF;
         break;
      }         
   }

   return(result);
} /* ethernetif_mld_mac_filter */
#endif

/**
 * Get ENET base address by index.
 */
static ENET_Type *get_enet_base (const uint8_t enetIdx)
{
   ENET_Type *Type = ENET;

   (void)enetIdx;
   
   return(Type);
} /* get_enet_base */

/**
 * Initialize the PHY the first time.
 */
static void enet_phy_init (struct _phy_handle *phyHandle)
{
   status_t status;
   
   phy_config_t phyConfig = {
      .phyAddr = phyHandle->phyAddr,
      .resource = phyHandle->resource,
      .ops      = phyHandle->ops,
      .autoNeg = true,
   };

   status = PHY_Init(phyHandle, &phyConfig);
   (void)status;

} /* ethernetif_phy_init */


/**
 * Initializes ENET driver.
 */
static void enet_init (struct netif *netif)
{
   enet_config_t        config;
   uint32_t             sysClock;
   enet_buffer_config_t buffCfg;
   struct ethernetif   *ethernetif = netif->state;
   status_t             status;

   /* Prepare the buffer configuration. */
   memset(&buffCfg, 0x00, sizeof(buffCfg));
   
   /* prepare the buffer configuration. */
   buffCfg.rxRingLen            = ENET_RXBD_NUM;                                 /* The length of receive buffer descriptor ring. */
   buffCfg.txRingLen            = ENET_TXBD_NUM;                                 /* The length of transmit buffer descriptor ring. */
   buffCfg.txDescStartAddrAlign = &(ethernetif->TxBuffDescrip[0]);               /* Aligned transmit descriptor start address. */
   buffCfg.txDescTailAddrAlign  = &(ethernetif->TxBuffDescrip[0]);               /* Aligned transmit descriptor tail address. */
   buffCfg.txDirtyStartAddr     = &ethernetif->txDirty[0];                       /* Start address of the dirty tx frame information. */
   buffCfg.rxDescStartAddrAlign = &(ethernetif->RxBuffDescrip[0]);               /* Aligned receive descriptor start address. */
   buffCfg.rxDescTailAddrAlign  = &(ethernetif->RxBuffDescrip[ENET_RXBD_NUM]);   /* Aligned receive descriptor tail address. */
   buffCfg.rxBufferStartAddr    = &ethernetif->rxBufferStartAddr[0];             /* Holds addresses of the rx buffers. */
   buffCfg.rxBuffSizeAlign      =  SDK_SIZEALIGN(ENET_RXBUFF_SIZE, FSL_ENET_BUFF_ALIGNMENT); /* Aligned receive data buffer size. */

   ENET_GetDefaultConfig(&config);

   /* Initialize PHY */   
   enet_phy_init(ethernetif->phyHandle);

   /* 
    * @@MF: No link detection here, will be done later by CheclLink.
    * Set default speed, mode, callback and user data.
    */
   config.miiMode   = kENET_RmiiMode;
   config.miiSpeed  = kENET_MiiSpeed100M;
   config.miiDuplex = kENET_MiiFullDuplex;
   
   /*
    * Configure the interrupts
    */
   config.interrupt = kENET_DmaTx | kENET_DmaRx;

   NVIC_SetPriority(ETHERNET_IRQn,  ENET_PRIORITY);

#if defined(ENET_ENHANCEDBUFFERDESCRIPTOR_MODE)
   NVIC_SetPriority(ENET_1588_Timer_IRQn, ENET_1588_PRIORITY);
#endif

   /* Initialize the ENET module.*/
   sysClock = CLOCK_GetMainClkFreq();
   ENET_Init(ethernetif->base, &config, netif->hwaddr, sysClock);
   
   status = ENET_DescriptorInit(ethernetif->base, &config, &buffCfg);
   LWIP_ASSERT("ENET_DescriptorInit() failed", status == kStatus_Success);

   /* Create the handler. */
   ENET_CreateHandler(ethernetif->base, &ethernetif->handle, &config, &buffCfg, ethernet_callback, netif);

   /* Active TX/RX. */
   ENET_StartRxTx(ethernetif->base, 1, 1);
   
} /* enet_init */


/**
 * Timer callback function for checking the Ethernet Link.
 *
 * @param arg contain the PHYDriver
 */
static void CheckLink (struct netif *netif)
{
   //bool               Autoneg;
   bool               Link;
   
   phy_speed_t        speed;
   phy_duplex_t       duplex;
   struct ethernetif *ethernetif = netif->state;;


   /* Wait for auto-negotiation success and link up */
   //PHY_GetAutoNegotiationStatus(ethernetif->phyHandle, &Autoneg);
   
   /* Get actual Link status */
   PHY_GetLinkStatus(ethernetif->phyHandle, &Link);
   if (ethernetif->OldLink != Link) /*lint !e731*/
   {      
      /* Check for DOWN/UP */
      if (false == Link)   /*lint !e731*/
      {  
         /* Down */
         tcpip_try_callback((tcpip_callback_fn)netif_set_link_down, netif);
         netif->link_speed       = 0;
         netif->link_duplex_full = 0;
      }
      else
      {
         /* Up */
         PHY_GetLinkSpeedDuplex(ethernetif->phyHandle, &speed, &duplex);
         
         switch(speed)
         {
            case kPHY_Speed10M:   netif->link_speed = 10;   break;
            case kPHY_Speed100M:  netif->link_speed = 100;  break;
            case kPHY_Speed1000M: netif->link_speed = 1000; break;
            default:              netif->link_speed = 1;    break; 
         }

         switch(duplex)
         {
            case kPHY_HalfDuplex: netif->link_duplex_full = 0; break;
            case kPHY_FullDuplex: netif->link_duplex_full = 1; break;
            default:              netif->link_duplex_full = 0; break;
         }

         /* Change the MII speed and duplex for actual link status. */
         ENET_SetMII(ethernetif->base, (enet_mii_speed_t)speed, (enet_mii_duplex_t)duplex);
      
         tcpip_try_callback((tcpip_callback_fn)netif_set_link_up, netif);
      }
   } /* end if (LinkStatus != OldLinkStatus) */
   
   ethernetif->OldLink = Link;      
} /* CheckLink */


/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static err_t low_level_init (struct netif *netif, const uint8_t enetIdx)
{
   struct ethernetif *ethernetif = netif->state;
   
   (void)enetIdx;
   
   netif->flags = 0;
   
   /* Set netif maximum transfer unit */
   netif->mtu = 1500;

   /* Accept broadcast address, ARP traffic and Multicast */
   netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;

#if defined(LWIP_IPV6) && (LWIP_IPV6 != 0)
   netif->flags |= NETIF_FLAG_MLD6;
#endif

#if (LWIP_IGMP >= 1)   
   /* Add IGMP support */   
   netif->flags |= NETIF_FLAG_IGMP;
   netif->igmp_mac_filter = ethernetif_igmp_mac_filter;
#endif   

#if (LWIP_IPV6 >= 1) && (LWIP_IPV6_MLD >= 1)
   /* Add IGMP support */
   netif->flags |= NETIF_FLAG_MLD6;
   netif->mld_mac_filter = ethernetif_mld_mac_filter;
#endif


   /* 
    * Init Ethernet pins if not already done 
    */
   if (0 == InitPinsDone)
   {
      InitPinsDone = 1;
      BoardETHConfig();
   }

   /* 
    * Initialize the ENET hardware
    */

   /* Create our semaphore */
   OS_EventCreate(&ethernetif->TxEvent);
   OS_SemaCreate(&ethernetif->RxSema, 0, 1);
   
   /* Initialize the ethernet hardware */
   enet_init(netif);

   /* 
    * Create the IPRX thread 
    *
    * PHY init, ETH start and the LinkTimer will be 
    * handled by the ethernetif_input thread.
    */ 
   if      (1 == ethernetif->instance)
   {      
      sys_thread_new("IPRX-1", ethernetif_input, netif,
                     TASK_IP_RX_STK_SIZE, TASK_IP_RX_PRIORITY);
   }
#if defined(TASK_IP_RX2_STK_SIZE)   
   else if (2 == ethernetif->instance)
   {
      sys_thread_new("IPRX-2", ethernetif_input, netif,
                     TASK_IP_RX2_STK_SIZE, TASK_IP_RX2_PRIORITY);
   
   }
#endif   
   else
   {
      return(ERR_MEM);
   }
                 
   return(ERR_OK);
} /* low_level_init */


/**
 * Returns next buffer for TX.
 * Can wait if no buffer available.
 */
static unsigned char *enet_get_tx_buffer (struct ethernetif *ethernetif)
{
   static unsigned char ucBuffer[ENET_FRAME_MAX_FRAMELEN];
   
   (void)ethernetif;
   
   return(ucBuffer);
} /* enet_get_tx_buffer */


/**
 * Sends frame via ENET.
 */
static err_t enet_send_frame (struct ethernetif *ethernetif, unsigned char *data, const uint32_t length)
{
   int                      rc;
   err_t                    res = ERR_OK;
   status_t                 result;
   uint32_t                 event;
   enet_tx_frame_struct_t txFrame;
   enet_buffer_struct_t   txBuffArray;

   /* Clear event first */
   OS_EventClr(&ethernetif->TxEvent, TX_READY_EVENT);

   txBuffArray.buffer = data;
   txBuffArray.length = (uint16_t)length;
   
   memset(&txFrame, 0x00, sizeof(txFrame));
   txFrame.txBuffArray        = &txBuffArray;
   txFrame.txBuffNum          = 1;
   txFrame.txConfig.intEnable = 1;
   txFrame.context            = NULL;

   do
   {
      /* Send frame */
      result = ENET_SendFrame(ethernetif->base, &ethernetif->handle, &txFrame, 0);
      if (result == kStatus_ENET_TxFrameBusy)
      {
         /* Wait for TX ready event */
         rc = OS_EventWait(&ethernetif->TxEvent, TX_READY_EVENT, OS_EVENT_MODE_OR, &event, OS_MS_2_TICKS(2000));
         if (OS_RC_TIMEOUT == rc)
         {
            /* Error, timeout */
            res = ERR_TIMEOUT;
            break;
         }   
         else if (OS_RC_OK == rc)
         {
            /* OK, no timeout */
            res = ERR_OK;
            break;
         }   
      }
   } while (result == kStatus_ENET_TxFrameBusy);
   
   return(res);
} /* enet_send_frame */


/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */
static err_t low_level_output (struct netif *netif, struct pbuf *p)
{
   err_t              result;
   struct ethernetif *ethernetif = netif->state;
   struct pbuf       *q;
   unsigned char     *pucBuffer;
   unsigned char     *pucChar;

   pucBuffer = enet_get_tx_buffer(ethernetif);
   if (pucBuffer == NULL)
   {
      return(ERR_BUF);
   }

   /* Initiate transfer. */

#if (ETH_PAD_SIZE)
   pbuf_header(p, -ETH_PAD_SIZE); /* Drop the padding word */
#endif

   if (p->len == p->tot_len)
   {
      /* No pbuf chain, don't have to copy -> faster. */
      pucBuffer = (unsigned char *)p->payload;
   }
   else
   {
      /* pbuf chain, copy into contiguous ucBuffer. */
      if (p->tot_len >= ENET_FRAME_MAX_FRAMELEN)
      {
         return(ERR_BUF);
      }
      else
      {
         pucChar = pucBuffer;

         for (q = p; q != NULL; q = q->next)
         {
            /* Send the data from the pbuf to the interface, one pbuf at a
               time. The size of the data in each pbuf is kept in the ->len
               variable. */
            /* Send data from(q->payload, q->len); */
            if (q == p)
            {
               memcpy(pucChar, q->payload, q->len);
               pucChar += q->len;
            }
            else
            {
               memcpy(pucChar, q->payload, q->len);
               pucChar += q->len;
            }
         }
      }
   }

   /* Send frame. */
   result = enet_send_frame(ethernetif, pucBuffer, p->tot_len);

#if (ETH_PAD_SIZE)
   pbuf_header(p, ETH_PAD_SIZE); /* Reclaim the padding word */
#endif

   LINK_STATS_INC(link.xmit);

   return(result);
} /* low_level_output */


/**
 * Gets the length of received frame (if any).
 */
static status_t enet_get_rx_frame_size (struct ethernetif *ethernetif, uint32_t *length)
{
   return( ENET_GetRxFrameSize(ethernetif->base, &ethernetif->handle, length, 0) );  
} /* enet_get_rx_frame_size */


/**
 * Reads frame from ENET.
 */
static void enet_read_frame (struct ethernetif *ethernetif, uint8_t *data, uint32_t length)
{
   ENET_ReadFrame(ethernetif->base, &ethernetif->handle, data, length, 0, NULL);
   
} /* enet_read_frame */


/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
static struct pbuf *low_level_input (struct netif *netif)
{
   struct ethernetif *ethernetif = netif->state;
   struct pbuf       *p = NULL;
   struct pbuf       *q;
   uint32_t           len;
   status_t           status;

   /* Obtain the size of the packet and put it into the "len" variable. */
   status = enet_get_rx_frame_size(ethernetif, &len);

   /* Check for frame error */
   while (kStatus_ENET_RxFrameError == status)
   {
      /* Update the receive buffer. */
      enet_read_frame(ethernetif, NULL, 0U);
      LINK_STATS_INC(link.drop);

      status = enet_get_rx_frame_size(ethernetif, &len);
   }
   
   /* Check if frame is empty */
   if (kStatus_ENET_RxFrameEmpty == status)
   {
      return(NULL);
   }      
   
   /* Check for a good frame */
   if ((kStatus_Success == status) && (len != 0))
   {
#if (ETH_PAD_SIZE)
      len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

      /* We allocate a pbuf chain of pbufs from the pool. */
      p = pbuf_alloc(PBUF_RAW, (u16_t)len, PBUF_POOL);
      if (p != NULL)
      {
#if (ETH_PAD_SIZE)
         pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif
         if (p->next == 0) /* One-chain buffer.*/
         {
            enet_read_frame(ethernetif, p->payload, p->len);
         }
         else    /* Multi-chain buffer.*/
         {
            uint8_t data_tmp[ENET_FRAME_MAX_FRAMELEN];
            uint32_t data_tmp_len = 0;

            enet_read_frame(ethernetif, data_tmp, p->tot_len);

            /* We iterate over the pbuf chain until we have read the entire
             * packet into the pbuf. */
            for (q = p; (q != NULL) && ((data_tmp_len + q->len) <= sizeof(data_tmp)); q = q->next)
            {
               /* Read enough bytes to fill this pbuf in the chain. The
                * available data in the pbuf is given by the q->len
                * variable. */
               memcpy(q->payload,  &data_tmp[data_tmp_len], q->len);
               data_tmp_len += q->len;
            }
         }

#if (ETH_PAD_SIZE)
         pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

         LINK_STATS_INC(link.recv);
      }
      else
      {
         /* Drop packet*/
         enet_read_frame(ethernetif, NULL, 0U);

         LINK_STATS_INC(link.memerr);
         LINK_STATS_INC(link.drop);
      }
   }
   
   return(p);
} /* low_level_input */


/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
static void ethernetif_input (void *arg)
{
   int                rc;
   struct pbuf       *p;
   struct netif      *netif = (struct netif*)arg;
   struct ethernetif *ethernetif;
   struct eth_hdr    *ethhdr;
   uint32_t           Checktime = OS_TimeGet();

   ethernetif = netif->state;
   ethernetif->OldLink = false;
   
   while (1)
   {
      /* Wait for an event */
      rc = OS_SemaWait(&ethernetif->RxSema, 500);
      if (OS_RC_OK == rc)
      {
         while (1)
         {
            /* move received packet into a new pbuf */
            p = low_level_input( netif );
            
            /* no packet could be read, silently ignore this */
            if (p == NULL) break;
            
            /* points to packet payload, which starts with an Ethernet header */
            ethhdr = p->payload;
            
            switch (htons(ethhdr->type)) 
            {
               /* IP or ARP packet? */
               case ETHTYPE_IP:
               case ETHTYPE_IPV6:
               case ETHTYPE_ARP:
               
                  /* full packet send to tcpip_thread to process */
                  if (netif->input(p, netif) != ERR_OK)
                  {
                     LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
                     pbuf_free(p);
                  }
                  
                  /* 
                   * In case we have receive a packet, we can
                   * restart the timeout for the CheckLink.
                   */
                  if (false == ethernetif->OldLink)
                  {
                     CheckLink(netif);
                  }
                  else
                  {
                     Checktime = OS_TimeGet();               
                  }             
                  break;
               
               default:
                  pbuf_free(p);
                  break;                  
            } /* end switch (htons(ethhdr->type)) */               
         } /* while (1), loop over low_level_input */
      } /* if (OS_RC_OK == rc) */
      
      /* Check timeout for CheckLink */
      if ((OS_TimeGet() - Checktime) > LINK_TMR_INTERVAL)
      {
         Checktime = OS_TimeGet();
         CheckLink(netif);
      }
      
   } /* end while (1), task loop */
   
} /* ethernetif_input */


/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
static err_t _ethernetif_init (struct netif *netif, struct ethernetif *ethernetif, const uint8_t enetIdx)
{
   err_t error = ERR_MEM;
   
   if (netif != NULL)
   {
      /* Set netif MAC hardware address */
      memcpy(netif->hwaddr, netif->state, ETHARP_HWADDR_LEN); 

      /* Set netif MAC hardware address length */
      netif->hwaddr_len = ETHARP_HWADDR_LEN;

#if (LWIP_NETIF_HOSTNAME)
      if (NULL == netif->hostname)
      {
         /* Initialize interface hostname */
         snprintf(ethernetif->hostname, sizeof(ethernetif->hostname), "%02X%02X%02X%02X%02X%02X", 
                  netif->hwaddr[0], netif->hwaddr[1], netif->hwaddr[2], 
                  netif->hwaddr[3], netif->hwaddr[4], netif->hwaddr[5]);
         netif->hostname = ethernetif->hostname;
      }
#endif /* LWIP_NETIF_HOSTNAME */

      /* Store the ethernet interface data */
      netif->state = ethernetif;

      /* Descriptive abbreviation for this interface */
      netif->name[0] = IFNAME0;
      netif->name[1] = IFNAME1;
  
      /* 
       * We directly use etharp_output() here to save a function call.
       * You can instead declare your own function an call etharp_output()
       * from it if you have to do some checks before sending (e.g. if link
       * is available...) 
       */
      netif->output     = etharp_output;
      netif->linkoutput = low_level_output;

#if defined(LWIP_IPV6) && (LWIP_IPV6 != 0)
      netif->output_ip6 = ethip6_output;
#endif

      /* Init ethernetif parameters.*/
      ethernetif->base = get_enet_base(enetIdx);
      if (NULL == ethernetif->base)
      {
         return(ERR_MEM);
      }
      
      ethernetif->instance = enetIdx + 1;
  
      /* Initialize the hardware */
      error = low_level_init(netif, enetIdx);
   }      

   return(error);
} /* _ethernetif_init */

/*=======================================================================*/
/*  All code exported                                                    */
/*=======================================================================*/

/**
 * Should be called at the beginning of the program to set up the
 * first network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t ethernetif_init (struct netif *netif)
{
   err_t         error = ERR_MEM;
   static struct ethernetif ethernetif;
   
   AT_NONCACHEABLE_SECTION_ALIGN(static enet_rx_bd_struct_t rxBuffDescrip[ENET_RXBD_NUM], FSL_ENET_BUFF_ALIGNMENT);
   AT_NONCACHEABLE_SECTION_ALIGN(static enet_tx_bd_struct_t txBuffDescrip[ENET_TXBD_NUM], FSL_ENET_BUFF_ALIGNMENT);
   AT_NONCACHEABLE_SECTION_ALIGN(static rx_buffer_t rxDataBuff[ENET_RXBD_NUM], FSL_ENET_BUFF_ALIGNMENT);
//   AT_NONCACHEABLE_SECTION_ALIGN(static tx_buffer_t txDataBuff[ENET_TXBD_NUM], FSL_ENET_BUFF_ALIGNMENT);

   ethernetif.RxBuffDescrip = &(rxBuffDescrip[0]);
   ethernetif.TxBuffDescrip = &(txBuffDescrip[0]);
   ethernetif.RxDataBuff    = &(rxDataBuff[0]);   /*lint !e545*/      
//   ethernetif.TxDataBuff    = &(txDataBuff[0]);   /*lint !e545*/
   
   for (int i = 0; i < ENET_RXBD_NUM; i++)
   {
      ethernetif.rxBufferStartAddr[i] = (uint32_t)&rxDataBuff[i][0];
   }

   ethernetif.phyHandle     = BoardGetPhyHandle(0);
   
   if (ethernetif.phyHandle != NULL)
   {
      error = _ethernetif_init(netif, &ethernetif, 0U);
   }

   return(error);
} /* ethernetif_init */


/*
 * Ethernet "hardware" IRQ handler.
 */
void ETHERNET_IRQHandler (void)
{   
   TAL_CPU_IRQ_ENTER();
   ETHERNET_DriverIRQHandler();
   TAL_CPU_IRQ_EXIT();
}

/*** EOF ***/
