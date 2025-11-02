/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*            (c) 2014 - 2025 SEGGER Microcontroller GmbH             *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* Redistribution and use in source and binary forms, with or         *
* without modification, are permitted provided that the following    *
* condition is met:                                                  *
*                                                                    *
* - Redistributions of source code must retain the above copyright   *
*   notice, this condition and the following disclaimer.             *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL SEGGER Microcontroller BE LIABLE FOR *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------

File      : MCXN947_cm33_core0_Vectors.s
Purpose   : Exception and interrupt vectors for MCXN947_cm33_core0 devices.

Additional information:
  Preprocessor Definitions
    __NO_EXTERNAL_INTERRUPTS
      If defined,
        the vector table will contain only the internal exceptions
        and interrupts.
    __VECTORS_IN_RAM
      If defined,
        an area of RAM, large enough to store the vector table,
        will be reserved.

    __OPTIMIZATION_SMALL
      If defined,
        all weak definitions of interrupt handlers will share the
        same implementation.
      If not defined,
        all weak definitions of interrupt handlers will be defined
        with their own implementation.
*/
        .syntax unified

/*********************************************************************
*
*       Macros
*
**********************************************************************
*/

//
// Directly place a vector (word) in the vector table
//
.macro VECTOR Name=
        .section .vectors, "ax"
        .code 16
        .word \Name
.endm

//
// Declare an exception handler with a weak definition
//
.macro EXC_HANDLER Name=
        //
        // Insert vector in vector table
        //
        .section .vectors, "ax"
        .word \Name
        //
        // Insert dummy handler in init section
        //
        .section .init.\Name, "ax"
        .thumb_func
        .weak \Name
        .balign 2
\Name:
        1: b 1b   // Endless loop
.endm

//
// Declare an interrupt handler with a weak definition
//
.macro ISR_HANDLER Name=
        //
        // Insert vector in vector table
        //
        .section .vectors, "ax"
        .word \Name
        //
        // Insert dummy handler in init section
        //
#if defined(__OPTIMIZATION_SMALL)
        .section .init, "ax"
        .weak \Name
        .thumb_set \Name,Dummy_Handler
#else
        .section .init.\Name, "ax"
        .thumb_func
        .weak \Name
        .balign 2
\Name:
        1: b 1b   // Endless loop
#endif
.endm

//
// Place a reserved vector in vector table
//
.macro ISR_RESERVED
        .section .vectors, "ax"
        .word 0
.endm

//
// Place a reserved vector in vector table
//
.macro ISR_RESERVED_DUMMY
        .section .vectors, "ax"
        .word Dummy_Handler
.endm

/*********************************************************************
*
*       Externals
*
**********************************************************************
*/
        .extern __stack_end__
        .extern Reset_Handler
        .extern HardFault_Handler

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*  Setup of the vector table and weak definition of interrupt handlers
*
*/
        .section .vectors, "ax"
        .code 16
        .balign 256
        .global _vectors
        .global __Vectors
_vectors:
__Vectors:
        //
        // Internal exceptions and interrupts
        //
        VECTOR __stack_end__
        VECTOR Reset_Handler
        EXC_HANDLER NMI_Handler
        VECTOR HardFault_Handler
#ifdef __ARM_ARCH_6M__
        ISR_RESERVED
        ISR_RESERVED
        ISR_RESERVED
#else
        EXC_HANDLER MemManage_Handler
        EXC_HANDLER BusFault_Handler
        EXC_HANDLER UsageFault_Handler
#endif
        ISR_RESERVED
        ISR_RESERVED
        ISR_RESERVED
        ISR_RESERVED
        EXC_HANDLER SVC_Handler
#ifdef __ARM_ARCH_6M__
        ISR_RESERVED
#else
        EXC_HANDLER DebugMon_Handler
#endif
        ISR_RESERVED
        EXC_HANDLER PendSV_Handler
        EXC_HANDLER SysTick_Handler
        //
        // External interrupts
        //
#ifndef __NO_EXTERNAL_INTERRUPTS
        ISR_HANDLER OR_IRQHandler
        ISR_HANDLER EDMA_0_CH0_IRQHandler
        ISR_HANDLER EDMA_0_CH1_IRQHandler
        ISR_HANDLER EDMA_0_CH2_IRQHandler
        ISR_HANDLER EDMA_0_CH3_IRQHandler
        ISR_HANDLER EDMA_0_CH4_IRQHandler
        ISR_HANDLER EDMA_0_CH5_IRQHandler
        ISR_HANDLER EDMA_0_CH6_IRQHandler
        ISR_HANDLER EDMA_0_CH7_IRQHandler
        ISR_HANDLER EDMA_0_CH8_IRQHandler
        ISR_HANDLER EDMA_0_CH9_IRQHandler
        ISR_HANDLER EDMA_0_CH10_IRQHandler
        ISR_HANDLER EDMA_0_CH11_IRQHandler
        ISR_HANDLER EDMA_0_CH12_IRQHandler
        ISR_HANDLER EDMA_0_CH13_IRQHandler
        ISR_HANDLER EDMA_0_CH14_IRQHandler
        ISR_HANDLER EDMA_0_CH15_IRQHandler
        ISR_HANDLER GPIO00_IRQHandler
        ISR_HANDLER GPIO01_IRQHandler
        ISR_HANDLER GPIO10_IRQHandler
        ISR_HANDLER GPIO11_IRQHandler
        ISR_HANDLER GPIO20_IRQHandler
        ISR_HANDLER GPIO21_IRQHandler
        ISR_HANDLER GPIO30_IRQHandler
        ISR_HANDLER GPIO31_IRQHandler
        ISR_HANDLER GPIO40_IRQHandler
        ISR_HANDLER GPIO41_IRQHandler
        ISR_HANDLER GPIO50_IRQHandler
        ISR_HANDLER GPIO51_IRQHandler
        ISR_HANDLER UTICK0_IRQHandler
        ISR_HANDLER MRT0_IRQHandler
        ISR_HANDLER CTIMER0_IRQHandler
        ISR_HANDLER CTIMER1_IRQHandler
        ISR_HANDLER SCT0_IRQHandler
        ISR_HANDLER CTIMER2_IRQHandler
        ISR_HANDLER LP_FLEXCOMM0_IRQHandler
        ISR_HANDLER LP_FLEXCOMM1_IRQHandler
        ISR_HANDLER LP_FLEXCOMM2_IRQHandler
        ISR_HANDLER LP_FLEXCOMM3_IRQHandler
        ISR_HANDLER LP_FLEXCOMM4_IRQHandler
        ISR_HANDLER LP_FLEXCOMM5_IRQHandler
        ISR_HANDLER LP_FLEXCOMM6_IRQHandler
        ISR_HANDLER LP_FLEXCOMM7_IRQHandler
        ISR_HANDLER LP_FLEXCOMM8_IRQHandler
        ISR_HANDLER LP_FLEXCOMM9_IRQHandler
        ISR_HANDLER ADC0_IRQHandler
        ISR_HANDLER ADC1_IRQHandler
        ISR_HANDLER PINT0_IRQHandler
        ISR_HANDLER PDM_EVENT_IRQHandler
        ISR_RESERVED
        ISR_HANDLER USB0_FS_IRQHandler
        ISR_HANDLER USB0_DCD_IRQHandler
        ISR_HANDLER RTC_IRQHandler
        ISR_HANDLER SMARTDMA_IRQHandler
        ISR_HANDLER MAILBOX_IRQHandler
        ISR_HANDLER CTIMER3_IRQHandler
        ISR_HANDLER CTIMER4_IRQHandler
        ISR_HANDLER OS_EVENT_IRQHandler
        ISR_HANDLER FLEXSPI0_IRQHandler
        ISR_HANDLER SAI0_IRQHandler
        ISR_HANDLER SAI1_IRQHandler
        ISR_HANDLER USDHC0_IRQHandler
        ISR_HANDLER CAN0_IRQHandler
        ISR_HANDLER CAN1_IRQHandler
        ISR_RESERVED
        ISR_RESERVED
        ISR_HANDLER USB1_HS_PHY_IRQHandler
        ISR_HANDLER USB1_HS_IRQHandler
        ISR_HANDLER SEC_HYPERVISOR_CALL_IRQHandler
        ISR_RESERVED
        ISR_HANDLER PLU_IRQHandler
        ISR_HANDLER Freqme_IRQHandler
        ISR_HANDLER SEC_VIO_IRQHandler
        ISR_HANDLER ELS_IRQHandler
        ISR_HANDLER PKC_IRQHandler
        ISR_HANDLER PUF_IRQHandler
        ISR_HANDLER PQ_IRQHandler
        ISR_HANDLER EDMA_1_CH0_IRQHandler
        ISR_HANDLER EDMA_1_CH1_IRQHandler
        ISR_HANDLER EDMA_1_CH2_IRQHandler
        ISR_HANDLER EDMA_1_CH3_IRQHandler
        ISR_HANDLER EDMA_1_CH4_IRQHandler
        ISR_HANDLER EDMA_1_CH5_IRQHandler
        ISR_HANDLER EDMA_1_CH6_IRQHandler
        ISR_HANDLER EDMA_1_CH7_IRQHandler
        ISR_HANDLER EDMA_1_CH8_IRQHandler
        ISR_HANDLER EDMA_1_CH9_IRQHandler
        ISR_HANDLER EDMA_1_CH10_IRQHandler
        ISR_HANDLER EDMA_1_CH11_IRQHandler
        ISR_HANDLER EDMA_1_CH12_IRQHandler
        ISR_HANDLER EDMA_1_CH13_IRQHandler
        ISR_HANDLER EDMA_1_CH14_IRQHandler
        ISR_HANDLER EDMA_1_CH15_IRQHandler
        ISR_HANDLER CDOG0_IRQHandler
        ISR_HANDLER CDOG1_IRQHandler
        ISR_HANDLER I3C0_IRQHandler
        ISR_HANDLER I3C1_IRQHandler
        ISR_HANDLER NPU_IRQHandler
        ISR_HANDLER GDET_IRQHandler
        ISR_HANDLER VBAT0_IRQHandler
        ISR_HANDLER EWM0_IRQHandler
        ISR_HANDLER TSI_END_OF_SCAN_IRQHandler
        ISR_HANDLER TSI_OUT_OF_SCAN_IRQHandler
        ISR_HANDLER EMVSIM0_IRQHandler
        ISR_HANDLER EMVSIM1_IRQHandler
        ISR_HANDLER FLEXIO_IRQHandler
        ISR_HANDLER DAC0_IRQHandler
        ISR_HANDLER DAC1_IRQHandler
        ISR_HANDLER DAC2_IRQHandler
        ISR_HANDLER HSCMP0_IRQHandler
        ISR_HANDLER HSCMP1_IRQHandler
        ISR_HANDLER HSCMP2_IRQHandler
        ISR_HANDLER FLEXPWM0_RELOAD_ERROR_IRQHandler
        ISR_HANDLER FLEXPWM0_FAULT_IRQHandler
        ISR_HANDLER FLEXPWM0_SUBMODULE0_IRQHandler
        ISR_HANDLER FLEXPWM0_SUBMODULE1_IRQHandler
        ISR_HANDLER FLEXPWM0_SUBMODULE2_IRQHandler
        ISR_HANDLER FLEXPWM0_SUBMODULE3_IRQHandler
        ISR_HANDLER FLEXPWM1_RELOAD_ERROR_IRQHandler
        ISR_HANDLER FLEXPWM1_FAULT_IRQHandler
        ISR_HANDLER FLEXPWM1_SUBMODULE0_IRQHandler
        ISR_HANDLER FLEXPWM1_SUBMODULE1_IRQHandler
        ISR_HANDLER FLEXPWM1_SUBMODULE2_IRQHandler
        ISR_HANDLER FLEXPWM1_SUBMODULE3_IRQHandler
        ISR_HANDLER QDC0_COMPARE_IRQHandler
        ISR_HANDLER QDC0_HOME_IRQHandler
        ISR_HANDLER QDC0_WDG_SAB_IRQHandler
        ISR_HANDLER QDC0_IDX_IRQHandler
        ISR_HANDLER QDC1_COMPARE_IRQHandler
        ISR_HANDLER QDC1_HOME_IRQHandler
        ISR_HANDLER QDC1_WDG_SAB_IRQHandler
        ISR_HANDLER QDC1_IDX_IRQHandler
        ISR_HANDLER ITRC0_IRQHandler
        ISR_HANDLER BSP32_IRQHandler
        ISR_HANDLER ELS_ERR_IRQHandler
        ISR_HANDLER PKC_ERR_IRQHandler
        ISR_HANDLER ERM_SINGLE_BIT_ERROR_IRQHandler
        ISR_HANDLER ERM_MULTI_BIT_ERROR_IRQHandler
        ISR_HANDLER FMU0_IRQHandler
        ISR_HANDLER ETHERNET_IRQHandler
        ISR_HANDLER ETHERNET_PMT_IRQHandler
        ISR_HANDLER ETHERNET_MACLP_IRQHandler
        ISR_HANDLER SINC_FILTER_IRQHandler
        ISR_HANDLER LPTMR0_IRQHandler
        ISR_HANDLER LPTMR1_IRQHandler
        ISR_HANDLER SCG_IRQHandler
        ISR_HANDLER SPC_IRQHandler
        ISR_HANDLER WUU_IRQHandler
        ISR_HANDLER PORT_EFT_IRQHandler
        ISR_HANDLER ETB0_IRQHandler
        ISR_RESERVED
        ISR_RESERVED
        ISR_HANDLER WWDT0_IRQHandler
        ISR_HANDLER WWDT1_IRQHandler
        ISR_HANDLER CMC0_IRQHandler
        ISR_HANDLER CTI0_IRQHandler
#endif
        //
        .section .vectors, "ax"
_vectors_end:

#ifdef __VECTORS_IN_RAM
        //
        // Reserve space with the size of the vector table
        // in the designated RAM section.
        //
        .section .vectors_ram, "ax"
        .balign 256
        .global _vectors_ram

_vectors_ram:
        .space _vectors_end - _vectors, 0
#endif

/*********************************************************************
*
*  Dummy handler to be used for reserved interrupt vectors
*  and weak implementation of interrupts.
*
*/
        .section .init.Dummy_Handler, "ax"
        .thumb_func
        .weak Dummy_Handler
        .balign 2
Dummy_Handler:
        1: b 1b   // Endless loop


/*************************** End of file ****************************/
