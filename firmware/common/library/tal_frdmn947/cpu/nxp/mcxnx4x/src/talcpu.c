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
#define __TALCPU_C__

/*=======================================================================*/
/*  Include                                                              */
/*=======================================================================*/
#include <stdlib.h>
#include "tal.h"

#include "clock_config.h"
#include "fsl_wwdt.h"
#include "fsl_spc.h"

/*=======================================================================*/
/*  All Structures and Common Constants                                  */
/*=======================================================================*/

/*=======================================================================*/
/*  Definition of all local Data                                         */
/*=======================================================================*/

static uint32_t dHiResPeriod   = 0;
static uint32_t dHiResPeriod16 = 0;

/*=======================================================================*/
/*  Definition of all local Procedures                                   */
/*=======================================================================*/

/* Update Active mode voltage for OverDrive mode. */
static void BOARD_PowerMode_OD (void)
{
#if defined(CPU_OVERDRIVE_MODE) && (CPU_OVERDRIVE_MODE >= 1)
   spc_active_mode_dcdc_option_t opt = {
       .DCDCVoltage       = kSPC_DCDC_OverdriveVoltage,
       .DCDCDriveStrength = kSPC_DCDC_NormalDriveStrength,
   };
   SPC_SetActiveModeDCDCRegulatorConfig(SPC0, &opt);

   spc_sram_voltage_config_t cfg = {
       .operateVoltage       = kSPC_sramOperateAt1P2V,
       .requestVoltageUpdate = true,
   };
   SPC_SetSRAMOperateVoltage(SPC0, &cfg);
#endif
} /* BOARD_PowerMode_OD */

/*************************************************************************/
/*  NewSysTick_Config                                                    */
/*                                                                       */
/*  Based on the "core_cm4.h" version, but an AHB clock divided by 8 is  */
/*  used for the SysTick clock source.                                   */
/*                                                                       */
/*  The function initializes the System Timer and its interrupt, and     */
/*  starts the System Tick Timer. Counter is in free running mode to     */
/*  generate periodic interrupts.                                        */
/*                                                                       */
/*  In    : ticks                                                        */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static uint32_t NewSysTick_Config (uint32_t ticks)
{
   /* Reload value impossible */
   if ((ticks - 1) > SysTick_LOAD_RELOAD_Msk)  return (1);

   /* Set reload register */
   SysTick->LOAD = ticks - 1;

   /* Set Priority for Systick Interrupt */
   NVIC_SetPriority(SysTick_IRQn, SYSTICK_PRIO);

   /* Load the SysTick Counter Value */
   SysTick->VAL = 0;

   /*
    * SysTick IRQ and SysTick Timer must be
    * enabled with tal_CPUSysTickStart later.
    */

   return(ticks);
} /* NewSysTick_Config */

/*=======================================================================*/
/*  All code exported                                                    */
/*=======================================================================*/

/*************************************************************************/
/*  tal_CPUInit                                                          */
/*                                                                       */
/*  "Initialize" the CPU.                                                */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void tal_CPUInit (void)
{
   static uint8_t bInitDone = 0;

   if (0 == bInitDone)
   {
      bInitDone = 1;

      /* Update Active mode voltage for OverDrive mode. */
      BOARD_PowerMode_OD();
      
      /* Set PLL */
      BOARD_InitBootClocks();
      
      /* Update clock info */
      SystemCoreClockUpdate();

      /*
       * Init SysTick
       */
      dHiResPeriod = NewSysTick_Config(SystemCoreClock / OS_TICKS_PER_SECOND);

      /*
       * dHiResPeriod value must be a 16bit count, but here it is
       * bigger. Therefore dHiResPeriod must be divided by 16.
       */
      dHiResPeriod16 = dHiResPeriod / 16;

      /* Sets the "Priority Grouping" to the default value 0 */
      NVIC_SetPriorityGrouping(0);

   }

} /* tal_CPUInit */

/*************************************************************************/
/*  tal_CPUSysTickStart                                                  */
/*                                                                       */
/*  Start the SysTick.                                                   */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void tal_CPUSysTickStart (void)
{
   /* Enable SysTick IRQ and SysTick Timer */
   SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
} /* tal_CPUSysTickStart */

/*************************************************************************/
/*  tal_CPUIrqEnable                                                     */
/*                                                                       */
/*  Enable the given IRQ.                                                */
/*                                                                       */
/*  In    : IRQ                                                          */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void tal_CPUIrqEnable (int IRQ)
{
   NVIC_EnableIRQ((IRQn_Type)IRQ);
} /* tal_CPUIrqEnable */

/*************************************************************************/
/*  tal_CPUIrqDisable                                                    */
/*                                                                       */
/*  Disable the given IRQ.                                               */
/*                                                                       */
/*  In    : IRQ                                                          */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void tal_CPUIrqDisable (int IRQ)
{
   NVIC_DisableIRQ((IRQn_Type)IRQ);
} /* tal_CPUIrqDisable */

/*************************************************************************/
/*  tal_CPUIrqDisableAll                                                 */
/*                                                                       */
/*  Disable all interrupts.                                              */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void tal_CPUIrqDisableAll (void)
{
   /*lint +rw(_to_semi) */
   /*lint -d__disable_irq=_to_semi */

   __disable_irq();

} /* tal_CPUIrqDisableAll */

/*************************************************************************/
/*  tal_CPUIrqSetPriority                                                */
/*                                                                       */
/*  Set priority of the given IRQ.                                       */
/*                                                                       */
/*  In    : IRQ, Priority                                                */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void tal_CPUIrqSetPriority (int IRQ, int Priority)
{
   NVIC_SetPriority((IRQn_Type)IRQ, (uint32_t)Priority);
} /* tal_CPUIrqSetPriority */

/*************************************************************************/
/*  tal_CPUStatGetHiResPeriod                                            */
/*                                                                       */
/*  Return the HiResPeriod value.                                        */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: HiResPeriod                                                  */
/*************************************************************************/
uint32_t tal_CPUStatGetHiResPeriod (void)
{
   return(dHiResPeriod16);
} /* tal_CPUStatGetHiResPeriod */

/*************************************************************************/
/*  tal_CPUStatGetHiResCnt                                               */
/*                                                                       */
/*  Return the HiRes counter.                                            */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: HiRes counter                                                */
/*************************************************************************/
uint32_t tal_CPUStatGetHiResCnt (void)
{
   uint32_t dValue;

   /* Get milliseconds */
   dValue  = (OS_TimeGet() << 16);

   /* The SysTick counts down from HiResPeriod, therefore HiResPeriod - X */
   /* HiResPeriod is used, therefore divide the time by 16 too */
   dValue |= (uint16_t)((dHiResPeriod - SysTick->VAL) / 16);

   return(dValue);
} /* tal_CPUStatGetHiResCnt */

/*************************************************************************/
/*  tal_CPUGetFrequencyCPU                                               */
/*                                                                       */
/*  Return the clock frequency of the CPU in MHz.                        */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: Frequency                                                    */
/*************************************************************************/
uint32_t tal_CPUGetFrequencyCPU (void)
{
   return( CLOCK_GetMainClkFreq() );
} /* tal_CPUGetFrequencyCPU */

/*************************************************************************/
/*  tal_CPUGetFrequencyPLL1                                              */
/*                                                                       */
/*  Return the clock frequency of the PLL1 in MHz.                       */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: Frequency                                                    */
/*************************************************************************/
uint32_t tal_CPUGetFrequencyPLL1 (void)
{
   return( CLOCK_GetFreq(kCLOCK_Pll1Out) );
} /* tal_CPUGetFrequencyPLL1 */

#if defined(TAL_ENABLE_RNG)
#include "mcux_els.h"
/*
Include:

..\library\tal_frdmn947\cpu\nxp\mcxnx4x\sdk\components\els_pkc\src\compiler
..\library\tal_frdmn947\cpu\nxp\mcxnx4x\sdk\components\els_pkc\src\platforms\mcxn
..\library\tal_frdmn947\cpu\nxp\mcxnx4x\sdk\components\els_pkc\src\platforms\mcxn\inc
..\library\tal_frdmn947\cpu\nxp\mcxnx4x\sdk\components\els_pkc\src\comps\mcuxClBuffer\inc
..\library\tal_frdmn947\cpu\nxp\mcxnx4x\sdk\components\els_pkc\src\comps\mcuxClCore\inc
..\library\tal_frdmn947\cpu\nxp\mcxnx4x\sdk\components\els_pkc\src\comps\mcuxClEls\inc
..\library\tal_frdmn947\cpu\nxp\mcxnx4x\sdk\components\els_pkc\src\comps\mcuxClMemory\inc
..\library\tal_frdmn947\cpu\nxp\mcxnx4x\sdk\components\els_pkc\src\comps\mcuxClRandom\inc
..\library\tal_frdmn947\cpu\nxp\mcxnx4x\sdk\components\els_pkc\src\comps\mcuxClSession\inc
..\library\tal_frdmn947\cpu\nxp\mcxnx4x\sdk\components\els_pkc\src\comps\mcuxCsslCPreProcessor\inc
..\library\tal_frdmn947\cpu\nxp\mcxnx4x\sdk\components\els_pkc\src\comps\mcuxCsslFlowProtection\inc
..\library\tal_frdmn947\cpu\nxp\mcxnx4x\sdk\components\els_pkc\src\comps\mcuxCsslSecureCounter\inc

Source:

components
   els_pkc
      mcuxClEls   
         - mcuxClEls_Common.c
         - mcuxClEls_KeyManagement.c
         - mcuxClEls_Rng.c
      platform
         - mcux_els.c         
*/

/*************************************************************************/
/*  tal_CPURngInit                                                       */
/*                                                                       */
/*  Initialize the random number generator.                              */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: TAL_OK / TAL_ERROR                                           */
/*************************************************************************/
TAL_RESULT tal_CPURngInit (void)
{
   static uint8_t bInitDone = 0;
   TAL_RESULT      Error = TAL_OK;
   
   if (0 == bInitDone)
   {
      if (ELS_PowerDownWakeupInit(ELS) == kStatus_Success)
      {
         bInitDone = 1;
      }
      else
      {
         Error = TAL_ERROR;   
      }
   }

   return(Error);
} /* tal_CPURngInit */

/*************************************************************************/
/*  tal_CPURngDeInit                                                     */
/*                                                                       */
/*  DeInitialize the random number generator.                            */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: TAL_OK / TAL_ERROR                                           */
/*************************************************************************/
TAL_RESULT tal_CPURngDeInit (void)
{
   return(TAL_ERROR);
} /* tal_CPURngDeInit */

/*************************************************************************/
/*  tal_CPURngHardwarePoll                                               */
/*                                                                       */
/*  Generates a 32-bit random number.                                    */
/*                                                                       */
/*  In    : pData, dSize                                                 */
/*  Out   : pData                                                        */
/*  Return: TAL_OK / TAL_ERROR                                           */
/*************************************************************************/
TAL_RESULT tal_CPURngHardwarePoll (uint8_t *pData, uint32_t dSize)
{
   /* Call ELS to get random data */
   MCUX_CSSL_FP_FUNCTION_CALL_BEGIN(result, token, mcuxClEls_Prng_GetRandom(pData, dSize));
   if ((MCUX_CSSL_FP_FUNCTION_CALLED(mcuxClEls_Prng_GetRandom) != token) || (MCUXCLELS_STATUS_OK != result))
   {
      return(TAL_ERROR);
   }
   MCUX_CSSL_FP_FUNCTION_CALL_END();

   return(TAL_OK);
} /* tal_CPURngHardwarePoll */
#endif /* defined(TAL_ENABLE_RNG) */

/*************************************************************************/
/*  tal_CPUInitHWDog                                                     */
/*                                                                       */
/*  Initialize the Hardware Watchdog.                                    */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void tal_CPUInitHWDog (void)
{
   wwdt_config_t   config;
   uint32_t        wdtFreq;
   uint32_t        wdtClockFreq = CLOCK_GetWdtClkFreq(0);
   static uint8_t bInitDone     = 0;

   if (0 == bInitDone)
   {
      bInitDone = 1;

      /* Set clock divider for WWDT clock source. */
      CLOCK_SetClkDiv(kCLOCK_DivWdt0Clk, 1U);
      
      /* Enable FRO 1M clock for WWDT module. */
      SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_FRO1MHZ_CLK_ENA_MASK;

      /* The WDT divides the input frequency into it by 4 */
      wdtFreq = wdtClockFreq / 4;

      WWDT_GetDefaultConfig(&config);
      
      /*
       * Set watchdog feed time constant to approximately 4s
       * Set watchdog warning time to 512 ticks after feed time constant
       * Set watchdog window time to 1s
       */
      config.timeoutValue = wdtFreq * 1;
      config.warningValue = 512;
      config.windowValue  = wdtFreq * 1;
      
      /* Configure WWDT to reset on timeout */
      config.enableWatchdogReset = true;
      
      /* Setup watchdog clock frequency(Hz). */
      config.clockFreq_Hz = wdtClockFreq;
      WWDT_Init(WWDT0, &config);
   }

} /* tal_CPUInitHWDog */

/*************************************************************************/
/*  Name  : tal_CPUTriggerHWDog                                          */
/*                                                                       */
/*  Trigger the Hardware Watchdog here.                                  */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void tal_CPUTriggerHWDog (void)
{
   WWDT_Refresh(WWDT0);
} /* tal_CPUTriggerHWDog */

/*************************************************************************/
/*  tal_CPUReboot                                                        */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void tal_CPUReboot (void)
{
#if defined(__DEBUG__)
   term_printf("\r\n*** Reboot ***\r\n");
   OS_TimeDly(500);
#endif

   /*
    * Init watchdog, if not done before
    */
#if defined(__FLASH__) || defined(ENABLED_WDOG)
   tal_CPUInitHWDog();
#endif

   /*
    * Wait for watchdog reset
    */
   TAL_CPU_DISABLE_ALL_INTS();
   while (1)
   {
      __asm__ ("nop");
   }
   TAL_CPU_ENABLE_ALL_INTS(); /*lint !e527*/

} /* tal_CPUReboot */

#if defined(RTOS_TCTS)
/*************************************************************************/
/*  SysTick_Handler                                                      */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void SysTick_Handler (void)
{
   TAL_CPU_IRQ_ENTER();

   OS_TimerCallback();

   TAL_CPU_IRQ_EXIT();
} /* SysTick_Handler */
#endif /* defined(RTOS_TCTS) */

/*************************************************************************/
/*  HardFault_Handler                                                    */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void HardFault_Handler (void)
{

#if defined(__DEBUG__)
   while (1)
   {
      __asm__ ("nop");
   }
#endif

   tal_CPUReboot();

} /* HardFault_Handler */

/*** EOF ***/
