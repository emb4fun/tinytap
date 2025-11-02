/**************************************************************************
*  This file is part of the TAL project (Tiny Abstraction Layer)
*
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

/*
   Most of the sources used here come from the following project:

   https://community.nxp.com/t5/MCX-Microcontrollers-Knowledge/Using-DDR-Octal-PSRAM-with-the-MCXN947/ta-p/1892917
   https://github.com/wavenumber-eng/mcxn947_octal_psram

   Hence the following copyright too:
   
   MIT License

   Copyright (c) 2024 Wavenumber

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.    
*/

#define __PSRAM_C__

/*=======================================================================*/
/*  Include                                                              */
/*=======================================================================*/
#include <stdint.h>
#include "tal.h"
#include "fsl_common.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "fsl_flexspi.h"
#include "fsl_cache.h"
#include "fsl_edma.h"

#if !defined(CPU_OVERDRIVE_MODE)
   #error: CPU_OVERDRIVE_MODE must be defined to 1;
#else
   #if (CPU_OVERDRIVE_MODE != 1)
      #error: CPU_OVERDRIVE_MODE must be defined to 1;
   #endif
#endif   

/*=======================================================================*/
/*  Extern                                                               */
/*=======================================================================*/

/*=======================================================================*/
/*  All Structures and Common Constants                                  */
/*=======================================================================*/

#define PCR_IBE_ibe1 0x01u /*!<@brief Input Buffer Enable: Enables */

/*=======================================================================*/
/*  Definition of all local Data                                         */
/*=======================================================================*/

edma_transfer_config_t transferConfig;
edma_config_t userConfig;
volatile bool g_Transfer_Done   = false;

flexspi_device_config_t psram_config =
{
   .flexspiRootClk       = 133333,
   .isSck2Enabled        = false,
   .flashSize            = 0x2000,
   .CSIntervalUnit       = kFLEXSPI_CsIntervalUnit1SckCycle,
   .CSInterval           = 2,
   .CSHoldTime           = 2,
   .CSSetupTime          = 2,
   .dataValidTime        = 2, //Important for APS6408L-3OBM-BA
   .enableWordAddress    = false,
   .AWRSeqIndex          = 1,
   .AWRSeqNumber         = 1,
   .ARDSeqIndex          = 0,
   .ARDSeqNumber         = 1,
   .AHBWriteWaitUnit     = kFLEXSPI_AhbWriteWaitUnit2AhbCycle,
   .AHBWriteWaitInterval = 0,
   .enableWriteMask      = true,
};

uint32_t psram_octal_lut[64] =
{
   /* Read Data */
   [0] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_8PAD, 0x20, kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x20),

   [1] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_RWDS_DDR, kFLEXSPI_8PAD, 0x07, kFLEXSPI_Command_READ_DDR,
                         kFLEXSPI_8PAD, 0x04),

   /* Write Data */
   [4] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_8PAD, 0xA0, kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x20),
   [5] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_RWDS_DDR, kFLEXSPI_8PAD, 0x07, kFLEXSPI_Command_WRITE_DDR,
                         kFLEXSPI_8PAD, 0x04),

   /* Read Register */
   [8] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_8PAD, 0x40, kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x20),
   [9] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_RWDS_DDR, kFLEXSPI_8PAD, 0x07, kFLEXSPI_Command_READ_DDR,
                         kFLEXSPI_8PAD, 0x04),

   /* Write Register */
   [12] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_8PAD, 0xC0, kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x20),
   [13] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_DDR, kFLEXSPI_8PAD, 0x08, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD,
                          0x00),

   /* Reset */
   [16] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_8PAD, 0xFF, kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_8PAD, 0x03),

};

/*=======================================================================*/
/*  Definition of all local Procedures                                   */
/*=======================================================================*/

static void InitPins (void)
{
   /* Enables the clock for GPIO3: Enables clock */
   CLOCK_EnableClock(kCLOCK_Gpio3);
   /* Enables the clock for PORT3: Enables clock */
   CLOCK_EnableClock(kCLOCK_Port3);

   /* PORT3_0 (pin B17) is configured as FLEXSPI0_A_SS0_b */
   PORT_SetPinMux(PORT3, 0U, kPORT_MuxAlt8);

   PORT3->PCR[0] = ((PORT3->PCR[0] &
                     /* Mask bits to zero which are setting */
                     (~(PORT_PCR_IBE_MASK)))

                    /* Input Buffer Enable: Enables. */
                    | PORT_PCR_IBE(PCR_IBE_ibe1));

   /* PORT3_10 (pin F17) is configured as FLEXSPI0_A_DATA2 */
   PORT_SetPinMux(PORT3, 10U, kPORT_MuxAlt8);

   PORT3->PCR[10] = ((PORT3->PCR[10] &
                      /* Mask bits to zero which are setting */
                      (~(PORT_PCR_IBE_MASK)))

                     /* Input Buffer Enable: Enables. */
                     | PORT_PCR_IBE(PCR_IBE_ibe1));

   /* PORT3_11 (pin F16) is configured as FLEXSPI0_A_DATA3 */
   PORT_SetPinMux(PORT3, 11U, kPORT_MuxAlt8);

   PORT3->PCR[11] = ((PORT3->PCR[11] &
                      /* Mask bits to zero which are setting */
                      (~(PORT_PCR_IBE_MASK)))

                     /* Input Buffer Enable: Enables. */
                     | PORT_PCR_IBE(PCR_IBE_ibe1));

   /* PORT3_12 (pin G16) is configured as FLEXSPI0_A_DATA4 */
   PORT_SetPinMux(PORT3, 12U, kPORT_MuxAlt8);

   PORT3->PCR[12] = ((PORT3->PCR[12] &
                      /* Mask bits to zero which are setting */
                      (~(PORT_PCR_IBE_MASK)))

                     /* Input Buffer Enable: Enables. */
                     | PORT_PCR_IBE(PCR_IBE_ibe1));

   /* PORT3_13 (pin H16) is configured as FLEXSPI0_A_DATA5 */
   PORT_SetPinMux(PORT3, 13U, kPORT_MuxAlt8);

   PORT3->PCR[13] = ((PORT3->PCR[13] &
                      /* Mask bits to zero which are setting */
                      (~(PORT_PCR_IBE_MASK)))

                     /* Input Buffer Enable: Enables. */
                     | PORT_PCR_IBE(PCR_IBE_ibe1));

   /* PORT3_14 (pin H17) is configured as FLEXSPI0_A_DATA6 */
   PORT_SetPinMux(PORT3, 14U, kPORT_MuxAlt8);

   PORT3->PCR[14] = ((PORT3->PCR[14] &
                      /* Mask bits to zero which are setting */
                      (~(PORT_PCR_IBE_MASK)))

                     /* Input Buffer Enable: Enables. */
                     | PORT_PCR_IBE(PCR_IBE_ibe1));

   /* PORT3_15 (pin H15) is configured as FLEXSPI0_A_DATA7 */
   PORT_SetPinMux(PORT3, 15U, kPORT_MuxAlt8);

   PORT3->PCR[15] = ((PORT3->PCR[15] &
                      /* Mask bits to zero which are setting */
                      (~(PORT_PCR_IBE_MASK)))

                     /* Input Buffer Enable: Enables. */
                     | PORT_PCR_IBE(PCR_IBE_ibe1));

   /* PORT3_6 (pin D17) is configured as FLEXSPI0_A_DQS */
   PORT_SetPinMux(PORT3, 6U, kPORT_MuxAlt8);

   PORT3->PCR[6] = ((PORT3->PCR[6] &
                     /* Mask bits to zero which are setting */
                     (~(PORT_PCR_IBE_MASK)))

                    /* Input Buffer Enable: Enables. */
                    | PORT_PCR_IBE(PCR_IBE_ibe1));

   /* PORT3_7 (pin D14) is configured as FLEXSPI0_A_SCLK */
   PORT_SetPinMux(PORT3, 7U, kPORT_MuxAlt8);

   PORT3->PCR[7] = ((PORT3->PCR[7] &
                     /* Mask bits to zero which are setting */
                     (~(PORT_PCR_IBE_MASK)))

                    /* Input Buffer Enable: Enables. */
                    | PORT_PCR_IBE(PCR_IBE_ibe1));

   /* PORT3_8 (pin E14) is configured as FLEXSPI0_A_DATA0 */
   PORT_SetPinMux(PORT3, 8U, kPORT_MuxAlt8);

   PORT3->PCR[8] = ((PORT3->PCR[8] &
                     /* Mask bits to zero which are setting */
                     (~(PORT_PCR_IBE_MASK)))

                    /* Input Buffer Enable: Enables. */
                    | PORT_PCR_IBE(PCR_IBE_ibe1));

   /* PORT3_9 (pin F15) is configured as FLEXSPI0_A_DATA1 */
   PORT_SetPinMux(PORT3, 9U, kPORT_MuxAlt8);

   PORT3->PCR[9] = ((PORT3->PCR[9] &
                     /* Mask bits to zero which are setting */
                     (~(PORT_PCR_IBE_MASK)))

                    /* Input Buffer Enable: Enables. */
                    | PORT_PCR_IBE(PCR_IBE_ibe1));

} /* InitPins */


static status_t flexspi_hyper_ram_write_mcr (FLEXSPI_Type *base, uint8_t regAddr, uint32_t *mrVal)
{
   flexspi_transfer_t flashXfer;
   status_t           status;

   /* Write data */
   flashXfer.deviceAddress = regAddr;
   flashXfer.port          = kFLEXSPI_PortA1;
   flashXfer.cmdType       = kFLEXSPI_Write;
   flashXfer.SeqNumber     = 1;
   flashXfer.seqIndex      = 3;
   flashXfer.data          = mrVal;
   flashXfer.dataSize      = 1;

   status = FLEXSPI_TransferBlocking(base, &flashXfer);

   return(status);
} /* flexspi_hyper_ram_write_mcr */


static status_t flexspi_hyper_ram_get_mcr (FLEXSPI_Type *base, uint8_t regAddr, uint32_t *mrVal)
{
   flexspi_transfer_t flashXfer;
   status_t           status;

   /* Read data */
   flashXfer.deviceAddress = regAddr;
   flashXfer.port          = kFLEXSPI_PortA1;
   flashXfer.cmdType       = kFLEXSPI_Read;
   flashXfer.SeqNumber     = 1;
   flashXfer.seqIndex      = 2;
   flashXfer.data          = mrVal;
   flashXfer.dataSize      = 2;

   status = FLEXSPI_TransferBlocking(base, &flashXfer);

   return(status);
} /* flexspi_hyper_ram_get_mcr */


static status_t flexspi_hyper_ram_reset (FLEXSPI_Type *base)
{
   flexspi_transfer_t flashXfer;
   status_t           status;

   /* Write data */
   flashXfer.deviceAddress = 0x0U;
   flashXfer.port          = kFLEXSPI_PortA1;
   flashXfer.cmdType       = kFLEXSPI_Command;
   flashXfer.SeqNumber     = 1;
   flashXfer.seqIndex      = 4;

   status = FLEXSPI_TransferBlocking(base, &flashXfer);
   if (status == kStatus_Success)
   {
      for (uint32_t i = 2000000U; i > 0; i--)
      {
         __NOP();
      }
   }
   
   return(status);
} /* flexspi_hyper_ram_reset */


static status_t bunny_mem_init (bool enable_cache)
{
   /*
    * DMA 1 used for memory tests
    *
    */
   edma_config_t    userConfig;
   uint32_t         mr0mr1[1];
   uint32_t         mr4mr8[1];
   uint32_t         mr0Val[1];
   uint32_t         mr4Val[1];
   uint32_t         mr8Val[1];
   flexspi_config_t config;
   cache64_config_t cacheCfg;
   status_t         status = kStatus_Success;
   
   EDMA_GetDefaultConfig(&userConfig);
   EDMA_Init(DMA1, &userConfig);
   EnableIRQ(EDMA_1_CH0_IRQn);

   RESET_PeripheralReset(kFLEXSPI_RST_SHIFT_RSTn);

   if(enable_cache)
   {
      /* As cache depends on FlexSPI power and clock, cache must be initialized after FlexSPI power/clock is set */
      CACHE64_GetDefaultConfig(&cacheCfg);
      CACHE64_Init(CACHE64_POLSEL0, &cacheCfg);

      CACHE64_EnableWriteBuffer(CACHE64_CTRL0, true);
      CACHE64_EnableCache(CACHE64_CTRL0);
   }

   /* Get FLEXSPI default settings and configure the flexspi. */
   FLEXSPI_GetDefaultConfig(&config);

   config.rxSampleClock = kFLEXSPI_ReadSampleClkExternalInputFromDqsPad;

   /*Set AHB buffer size for reading data through AHB bus. */
   config.ahbConfig.enableAHBPrefetch    = true;
   config.ahbConfig.enableAHBBufferable  = true;
   config.ahbConfig.enableAHBCachable    = true;
   config.ahbConfig.enableReadAddressOpt = true;

   for (uint8_t i = 1; i < FSL_FEATURE_FLEXSPI_AHB_BUFFER_COUNT - 1; i++)
   {
      config.ahbConfig.buffer[i].bufferSize = 0;
   }

   config.ahbConfig.buffer[0].masterIndex    = 4;     /* DMA0 */
   config.ahbConfig.buffer[0].bufferSize     = 512;   /* Allocate 512B bytes for DMA0 */
   config.ahbConfig.buffer[0].enablePrefetch = true;
   config.ahbConfig.buffer[0].priority       = 0;
   
   /* All other masters use last buffer with 512B bytes. */
   config.ahbConfig.buffer[FSL_FEATURE_FLEXSPI_AHB_BUFFER_COUNT - 1].bufferSize = 512;
   
#if !(defined(FSL_FEATURE_FLEXSPI_HAS_NO_MCR0_COMBINATIONEN) && FSL_FEATURE_FLEXSPI_HAS_NO_MCR0_COMBINATIONEN)
   config.enableCombination = true;
#endif

   FLEXSPI_Init(FLEXSPI0, &config);

   /* Configure flash settings according to serial flash feature. */
   FLEXSPI_SetFlashConfig(FLEXSPI0, &psram_config, kFLEXSPI_PortA1);

   /* Update LUT table. */
   FLEXSPI_UpdateLUT(FLEXSPI0, 0, psram_octal_lut, ARRAY_SIZE(psram_octal_lut));

   /* Do software reset. */
   FLEXSPI_SoftwareReset(FLEXSPI0);

   /* Reset hyper ram. */
   status = flexspi_hyper_ram_reset(FLEXSPI0);
   if (status != kStatus_Success)
   {
      return(status);
   }

   status = flexspi_hyper_ram_get_mcr(FLEXSPI0, 0x0, mr0mr1);
   if (status != kStatus_Success)
   {
      return(status);
   }

   status = flexspi_hyper_ram_get_mcr(FLEXSPI0, 0x4, mr4mr8);
   if (status != kStatus_Success)
   {
      return(status);
   }

   /* Enable RBX, burst length set to 1K. - MR8 */
   mr8Val[0] = (mr4mr8[0] & 0xFF00U) >> 8U;
   mr8Val[0] = mr8Val[0] | 0x0F;

   status = flexspi_hyper_ram_write_mcr(FLEXSPI0, 0x8, mr8Val);
   if (status != kStatus_Success)
   {
      return(status);
   }

   /* Set LC code to 0x04(LC=7, maximum frequency 200M) - MR0. */
   mr0Val[0] = mr0mr1[0] & 0x00FFU;
   mr0Val[0] = (mr0Val[0] & ~0x3CU) | (4U << 2U);

   status = flexspi_hyper_ram_write_mcr(FLEXSPI0, 0x0, mr0Val);
   if (status != kStatus_Success)
   {
      return(status);
   }

   /* Set WLC code to 0x01(WLC=7, maximum frequency 200M) - MR4. */
   mr4Val[0] = mr4mr8[0] & 0x00FFU;
   mr4Val[0] = (mr4Val[0] & ~0xE0U) | (1U << 5U);
   
   status = flexspi_hyper_ram_write_mcr(FLEXSPI0, 0x4, mr4Val);
   if (status != kStatus_Success)
   {
      return(status);
   }

   /* Need to reset FlexSPI controller between IP/AHB access. */
   FLEXSPI_SoftwareReset(FLEXSPI0);

   return(status);
} /* bunny_mem_init */

/*=======================================================================*/
/*  All code exported                                                    */
/*=======================================================================*/

/*************************************************************************/
/*  BoardPSRAMInit                                                       */
/*                                                                       */
/*  Configures pin routing and optionally pin electrical features.       */
/*                                                                       */
/*  In    : iface                                                        */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void BoardPSRAMInit (void)
{
   static int InitPinsDone = 0;
   status_t   status; 

   if (0 == InitPinsDone)
   {
      InitPinsDone = 1;
   
      InitPins();   
      
      CLOCK_AttachClk(kPLL1_to_FLEXSPI);  //PLL1 is set to 133MHz
      
      status = bunny_mem_init(1);
      (void)status;
   }
   
} /* BoardPSRAMInit */

/*** EOF ***/
