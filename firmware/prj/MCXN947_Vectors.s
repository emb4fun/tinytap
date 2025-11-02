/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*            (c) 2014 - 2023 SEGGER Microcontroller GmbH             *
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

File      : MCXN947_Vectors.s
Purpose   : Exception and interrupt vectors for MCXN947 devices.

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
_vectors:
        //
        // Internal exceptions and interrupts
        //
        VECTOR __stack_end__
        VECTOR Reset_Handler
        EXC_HANDLER NMI_Handler
        VECTOR HardFault_Handler
        EXC_HANDLER MemManage_Handler
        EXC_HANDLER BusFault_Handler
        EXC_HANDLER UsageFault_Handler
        EXC_HANDLER SecureFault_Handler
        ISR_RESERVED
        ISR_RESERVED
        ISR_RESERVED
        EXC_HANDLER SVC_Handler
        ISR_RESERVED
        ISR_RESERVED
        EXC_HANDLER PendSV_Handler
        EXC_HANDLER SysTick_Handler
        //
        // External interrupts
        //
#ifndef __NO_EXTERNAL_INTERRUPTS
       EXC_HANDLER OR_IRQHandler                      /* OR IRQ*/
       EXC_HANDLER EDMA_0_CH0_IRQHandler              /* eDMA_0_CH0 error or transfer complete*/
       EXC_HANDLER EDMA_0_CH1_IRQHandler              /* eDMA_0_CH1 error or transfer complete*/
       EXC_HANDLER EDMA_0_CH2_IRQHandler              /* eDMA_0_CH2 error or transfer complete*/
       EXC_HANDLER EDMA_0_CH3_IRQHandler              /* eDMA_0_CH3 error or transfer complete*/
       EXC_HANDLER EDMA_0_CH4_IRQHandler              /* eDMA_0_CH4 error or transfer complete*/
       EXC_HANDLER EDMA_0_CH5_IRQHandler              /* eDMA_0_CH5 error or transfer complete*/
       EXC_HANDLER EDMA_0_CH6_IRQHandler              /* eDMA_0_CH6 error or transfer complete*/
       EXC_HANDLER EDMA_0_CH7_IRQHandler              /* eDMA_0_CH7 error or transfer complete*/
       EXC_HANDLER EDMA_0_CH8_IRQHandler              /* eDMA_0_CH8 error or transfer complete*/
       EXC_HANDLER EDMA_0_CH9_IRQHandler              /* eDMA_0_CH9 error or transfer complete*/
       EXC_HANDLER EDMA_0_CH10_IRQHandler             /* eDMA_0_CH10 error or transfer complete*/
       EXC_HANDLER EDMA_0_CH11_IRQHandler             /* eDMA_0_CH11 error or transfer complete*/
       EXC_HANDLER EDMA_0_CH12_IRQHandler             /* eDMA_0_CH12 error or transfer complete*/
       EXC_HANDLER EDMA_0_CH13_IRQHandler             /* eDMA_0_CH13 error or transfer complete*/
       EXC_HANDLER EDMA_0_CH14_IRQHandler             /* eDMA_0_CH14 error or transfer complete*/
       EXC_HANDLER EDMA_0_CH15_IRQHandler             /* eDMA_0_CH15 error or transfer complete*/
       EXC_HANDLER GPIO00_IRQHandler                  /* GPIO0 interrupt 0*/
       EXC_HANDLER GPIO01_IRQHandler                  /* GPIO0 interrupt 1*/
       EXC_HANDLER GPIO10_IRQHandler                  /* GPIO1 interrupt 0*/
       EXC_HANDLER GPIO11_IRQHandler                  /* GPIO1 interrupt 1*/
       EXC_HANDLER GPIO20_IRQHandler                  /* GPIO2 interrupt 0*/
       EXC_HANDLER GPIO21_IRQHandler                  /* GPIO2 interrupt 1*/
       EXC_HANDLER GPIO30_IRQHandler                  /* GPIO3 interrupt 0*/
       EXC_HANDLER GPIO31_IRQHandler                  /* GPIO3 interrupt 1*/
       EXC_HANDLER GPIO40_IRQHandler                  /* GPIO4 interrupt 0*/
       EXC_HANDLER GPIO41_IRQHandler                  /* GPIO4 interrupt 1*/
       EXC_HANDLER GPIO50_IRQHandler                  /* GPIO5 interrupt 0*/
       EXC_HANDLER GPIO51_IRQHandler                  /* GPIO5 interrupt 1*/
       EXC_HANDLER UTICK0_IRQHandler                  /* Micro-Tick Timer interrupt*/
       EXC_HANDLER MRT0_IRQHandler                    /* Multi-Rate Timer interrupt*/
       EXC_HANDLER CTIMER0_IRQHandler                 /* Standard counter/timer 0 interrupt*/
       EXC_HANDLER CTIMER1_IRQHandler                 /* Standard counter/timer 1 interrupt*/
       EXC_HANDLER SCT0_IRQHandler                    /* SCTimer/PWM interrupt*/
       EXC_HANDLER CTIMER2_IRQHandler                 /* Standard counter/timer 2 interrupt*/
       EXC_HANDLER LP_FLEXCOMM0_IRQHandler            /* LP_FLEXCOMM0 (LPSPI interrupt or LPI2C interrupt or LPUART Receive/Transmit interrupt)*/
       EXC_HANDLER LP_FLEXCOMM1_IRQHandler            /* LP_FLEXCOMM1 (LPSPI interrupt or LPI2C interrupt or LPUART Receive/Transmit interrupt)*/
       EXC_HANDLER LP_FLEXCOMM2_IRQHandler            /* LP_FLEXCOMM2 (LPSPI interrupt or LPI2C interrupt or LPUART Receive/Transmit interrupt)*/
       EXC_HANDLER LP_FLEXCOMM3_IRQHandler            /* LP_FLEXCOMM3 (LPSPI interrupt or LPI2C interrupt or LPUART Receive/Transmit interrupt)*/
       EXC_HANDLER LP_FLEXCOMM4_IRQHandler            /* LP_FLEXCOMM4 (LPSPI interrupt or LPI2C interrupt or LPUART Receive/Transmit interrupt)*/
       EXC_HANDLER LP_FLEXCOMM5_IRQHandler            /* LP_FLEXCOMM5 (LPSPI interrupt or LPI2C interrupt or LPUART Receive/Transmit interrupt)*/
       EXC_HANDLER LP_FLEXCOMM6_IRQHandler            /* LP_FLEXCOMM6 (LPSPI interrupt or LPI2C interrupt or LPUART Receive/Transmit interrupt)*/
       EXC_HANDLER LP_FLEXCOMM7_IRQHandler            /* LP_FLEXCOMM7 (LPSPI interrupt or LPI2C interrupt or LPUART Receive/Transmit interrupt)*/
       EXC_HANDLER LP_FLEXCOMM8_IRQHandler            /* LP_FLEXCOMM8 (LPSPI interrupt or LPI2C interrupt or LPUART Receive/Transmit interrupt)*/
       EXC_HANDLER LP_FLEXCOMM9_IRQHandler            /* LP_FLEXCOMM9 (LPSPI interrupt or LPI2C interrupt or LPUART Receive/Transmit interrupt)*/
       EXC_HANDLER ADC0_IRQHandler                    /* Analog-to-Digital Converter 0 - General Purpose interrupt*/
       EXC_HANDLER ADC1_IRQHandler                    /* Analog-to-Digital Converter 1 - General Purpose interrupt*/
       EXC_HANDLER PINT0_IRQHandler                   /* Pin Interrupt Pattern Match Interrupt*/
       EXC_HANDLER PDM_EVENT_IRQHandler               /* Microphone Interface interrupt*/
       ISR_RESERVED
       EXC_HANDLER USB0_FS_IRQHandler                 /* Universal Serial Bus - Full Speed interrupt*/
       EXC_HANDLER USB0_DCD_IRQHandler                /* Universal Serial Bus - Device Charge Detect interrupt*/
       EXC_HANDLER RTC_IRQHandler                     /* RTC Subsystem interrupt (RTC interrupt or Wake timer interrupt)*/
       EXC_HANDLER SMARTDMA_IRQHandler                /* SmartDMA_IRQ*/
       EXC_HANDLER MAILBOX_IRQHandler                 /* Inter-CPU Mailbox interrupt0 for CPU0 Inter-CPU Mailbox interrupt1 for CPU1*/
       EXC_HANDLER CTIMER3_IRQHandler                 /* Standard counter/timer 3 interrupt*/
       EXC_HANDLER CTIMER4_IRQHandler                 /* Standard counter/timer 4 interrupt*/
       EXC_HANDLER OS_EVENT_IRQHandler                /* OS event timer interrupt*/
       EXC_HANDLER FLEXSPI0_IRQHandler                /* Flexible Serial Peripheral Interface interrupt*/
       EXC_HANDLER SAI0_IRQHandler                    /* Serial Audio Interface 0 interrupt*/
       EXC_HANDLER SAI1_IRQHandler                    /* Serial Audio Interface 1 interrupt*/
       EXC_HANDLER USDHC0_IRQHandler                  /* Ultra Secured Digital Host Controller interrupt*/
       EXC_HANDLER CAN0_IRQHandler                    /* Controller Area Network 0 interrupt*/
       EXC_HANDLER CAN1_IRQHandler                    /* Controller Area Network 1 interrupt*/
       ISR_RESERVED
       ISR_RESERVED
       EXC_HANDLER USB1_HS_PHY_IRQHandler             /* USBHS DCD or USBHS Phy interrupt*/
       EXC_HANDLER USB1_HS_IRQHandler                 /* USB High Speed OTG Controller interrupt */
       EXC_HANDLER SEC_HYPERVISOR_CALL_IRQHandler     /* AHB Secure Controller hypervisor call interrupt*/
       ISR_RESERVED
       EXC_HANDLER PLU_IRQHandler                     /* Programmable Logic Unit interrupt*/
       EXC_HANDLER Freqme_IRQHandler                  /* Frequency Measurement interrupt*/
       EXC_HANDLER SEC_VIO_IRQHandler                 /* Secure violation interrupt (Memory Block Checker interrupt or secure AHB matrix violation interrupt)*/
       EXC_HANDLER ELS_IRQHandler                     /* ELS interrupt*/
       EXC_HANDLER PKC_IRQHandler                     /* PKC interrupt*/
       EXC_HANDLER PUF_IRQHandler                     /* Physical Unclonable Function interrupt*/
       EXC_HANDLER PQ_IRQHandler                      /* Power Quad interrupt*/
       EXC_HANDLER EDMA_1_CH0_IRQHandler              /* eDMA_1_CH0 error or transfer complete*/
       EXC_HANDLER EDMA_1_CH1_IRQHandler              /* eDMA_1_CH1 error or transfer complete*/
       EXC_HANDLER EDMA_1_CH2_IRQHandler              /* eDMA_1_CH2 error or transfer complete*/
       EXC_HANDLER EDMA_1_CH3_IRQHandler              /* eDMA_1_CH3 error or transfer complete*/
       EXC_HANDLER EDMA_1_CH4_IRQHandler              /* eDMA_1_CH4 error or transfer complete*/
       EXC_HANDLER EDMA_1_CH5_IRQHandler              /* eDMA_1_CH5 error or transfer complete*/
       EXC_HANDLER EDMA_1_CH6_IRQHandler              /* eDMA_1_CH6 error or transfer complete*/
       EXC_HANDLER EDMA_1_CH7_IRQHandler              /* eDMA_1_CH7 error or transfer complete*/
       EXC_HANDLER EDMA_1_CH8_IRQHandler              /* eDMA_1_CH8 error or transfer complete*/
       EXC_HANDLER EDMA_1_CH9_IRQHandler              /* eDMA_1_CH9 error or transfer complete*/
       EXC_HANDLER EDMA_1_CH10_IRQHandler             /* eDMA_1_CH10 error or transfer complete*/
       EXC_HANDLER EDMA_1_CH11_IRQHandler             /* eDMA_1_CH11 error or transfer complete*/
       EXC_HANDLER EDMA_1_CH12_IRQHandler             /* eDMA_1_CH12 error or transfer complete*/
       EXC_HANDLER EDMA_1_CH13_IRQHandler             /* eDMA_1_CH13 error or transfer complete*/
       EXC_HANDLER EDMA_1_CH14_IRQHandler             /* eDMA_1_CH14 error or transfer complete*/
       EXC_HANDLER EDMA_1_CH15_IRQHandler             /* eDMA_1_CH15 error or transfer complete*/
       EXC_HANDLER CDOG0_IRQHandler                   /* Code Watchdog Timer 0 interrupt*/
       EXC_HANDLER CDOG1_IRQHandler                   /* Code Watchdog Timer 1 interrupt*/
       EXC_HANDLER I3C0_IRQHandler                    /* Improved Inter Integrated Circuit interrupt 0*/
       EXC_HANDLER I3C1_IRQHandler                    /* Improved Inter Integrated Circuit interrupt 1*/
       EXC_HANDLER NPU_IRQHandler                     /* NPU interrupt*/
       EXC_HANDLER GDET_IRQHandler                    /* Digital Glitch Detect 0 interrupt  or Digital Glitch Detect 1 interrupt*/
       EXC_HANDLER VBAT0_IRQHandler                   /* VBAT interrupt( VBAT interrupt or digital tamper interrupt)*/
       EXC_HANDLER EWM0_IRQHandler                    /* External Watchdog Monitor interrupt*/
       EXC_HANDLER TSI_END_OF_SCAN_IRQHandler         /* TSI End of Scan interrupt*/
       EXC_HANDLER TSI_OUT_OF_SCAN_IRQHandler         /* TSI Out of Scan interrupt*/
       EXC_HANDLER EMVSIM0_IRQHandler                 /* EMVSIM0 interrupt*/
       EXC_HANDLER EMVSIM1_IRQHandler                 /* EMVSIM1 interrupt*/
       EXC_HANDLER FLEXIO_IRQHandler                  /* Flexible Input/Output interrupt*/
       EXC_HANDLER DAC0_IRQHandler                    /* Digital-to-Analog Converter 0 - General Purpose interrupt*/
       EXC_HANDLER DAC1_IRQHandler                    /* Digital-to-Analog Converter 1 - General Purpose interrupt*/
       EXC_HANDLER DAC2_IRQHandler                    /* 14-bit Digital-to-Analog Converter interrupt*/
       EXC_HANDLER HSCMP0_IRQHandler                  /* High-Speed comparator0 interrupt*/
       EXC_HANDLER HSCMP1_IRQHandler                  /* High-Speed comparator1 interrupt*/
       EXC_HANDLER HSCMP2_IRQHandler                  /* High-Speed comparator2 interrupt*/
       EXC_HANDLER FLEXPWM0_RELOAD_ERROR_IRQHandler   /* FlexPWM0_reload_error interrupt*/
       EXC_HANDLER FLEXPWM0_FAULT_IRQHandler          /* FlexPWM0_fault interrupt*/
       EXC_HANDLER FLEXPWM0_SUBMODULE0_IRQHandler     /* FlexPWM0 Submodule 0 capture/compare/reload interrupt*/
       EXC_HANDLER FLEXPWM0_SUBMODULE1_IRQHandler     /* FlexPWM0 Submodule 1 capture/compare/reload interrupt*/
       EXC_HANDLER FLEXPWM0_SUBMODULE2_IRQHandler     /* FlexPWM0 Submodule 2 capture/compare/reload interrupt*/
       EXC_HANDLER FLEXPWM0_SUBMODULE3_IRQHandler     /* FlexPWM0 Submodule 3 capture/compare/reload interrupt*/
       EXC_HANDLER FLEXPWM1_RELOAD_ERROR_IRQHandler   /* FlexPWM1_reload_error interrupt*/
       EXC_HANDLER FLEXPWM1_FAULT_IRQHandler          /* FlexPWM1_fault interrupt*/
       EXC_HANDLER FLEXPWM1_SUBMODULE0_IRQHandler     /* FlexPWM1 Submodule 0 capture/compare/reload interrupt*/
       EXC_HANDLER FLEXPWM1_SUBMODULE1_IRQHandler     /* FlexPWM1 Submodule 1 capture/compare/reload interrupt*/
       EXC_HANDLER FLEXPWM1_SUBMODULE2_IRQHandler     /* FlexPWM1 Submodule 2 capture/compare/reload interrupt*/
       EXC_HANDLER FLEXPWM1_SUBMODULE3_IRQHandler     /* FlexPWM1 Submodule 3 capture/compare/reload interrupt*/
       EXC_HANDLER ENC0_COMPARE_IRQHandler            /* ENC0_Compare interrupt*/
       EXC_HANDLER ENC0_HOME_IRQHandler               /* ENC0_Home interrupt*/
       EXC_HANDLER ENC0_WDG_SAB_IRQHandler            /* ENC0_WDG_IRQ/SAB interrupt*/
       EXC_HANDLER ENC0_IDX_IRQHandler                /* ENC0_IDX interrupt*/
       EXC_HANDLER ENC1_COMPARE_IRQHandler            /* ENC1_Compare interrupt*/
       EXC_HANDLER ENC1_HOME_IRQHandler               /* ENC1_Home interrupt*/
       EXC_HANDLER ENC1_WDG_SAB_IRQHandler            /* ENC1_WDG_IRQ/SAB interrupt*/
       EXC_HANDLER ENC1_IDX_IRQHandler                /* ENC1_IDX interrupt*/
       EXC_HANDLER ITRC0_IRQHandler                   /* Intrusion and Tamper Response Controller interrupt*/
       EXC_HANDLER BSP32_IRQHandler                   /* CoolFlux BSP32 interrupt*/
       EXC_HANDLER ELS_ERR_IRQHandler                 /* ELS error interrupt*/
       EXC_HANDLER PKC_ERR_IRQHandler                 /* PKC error interrupt*/
       EXC_HANDLER ERM_SINGLE_BIT_ERROR_IRQHandler    /* ERM Single Bit error interrupt*/
       EXC_HANDLER ERM_MULTI_BIT_ERROR_IRQHandler     /* ERM Multi Bit error interrupt*/
       EXC_HANDLER FMU0_IRQHandler                    /* Flash Management Unit interrupt*/
       EXC_HANDLER ETHERNET_IRQHandler                /* Ethernet QoS interrupt*/
       EXC_HANDLER ETHERNET_PMT_IRQHandler            /* Ethernet QoS power management interrupt*/
       EXC_HANDLER ETHERNET_MACLP_IRQHandler          /* Ethernet QoS MAC interrupt*/
       EXC_HANDLER SINC_FILTER_IRQHandler             /* SINC Filter interrupt */
       EXC_HANDLER LPTMR0_IRQHandler                  /* Low Power Timer 0 interrupt*/
       EXC_HANDLER LPTMR1_IRQHandler                  /* Low Power Timer 1 interrupt*/
       EXC_HANDLER SCG_IRQHandler                     /* System Clock Generator interrupt*/
       EXC_HANDLER SPC_IRQHandler                     /* System Power Controller interrupt*/
       EXC_HANDLER WUU_IRQHandler                     /* Wake Up Unit interrupt*/
       EXC_HANDLER PORT_EFT_IRQHandler                /* PORT0~5 EFT interrupt*/
       EXC_HANDLER ETB0_IRQHandler                    /* ETB counter expires interrupt*/
       EXC_HANDLER SM3_IRQHandler                     /* Secure Generic Interface (SGI) SAFO interrupt */
       EXC_HANDLER TRNG0_IRQHandler                   /* True Random Number Generator interrupt*/
       EXC_HANDLER WWDT0_IRQHandler                   /* Windowed Watchdog Timer 0 interrupt*/
       EXC_HANDLER WWDT1_IRQHandler                   /* Windowed Watchdog Timer 1 interrupt*/
       EXC_HANDLER CMC0_IRQHandler                    /* Core Mode Controller interrupt*/
       EXC_HANDLER CTI0_IRQHandler                    /* Cross Trigger Interface interrupt*/
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
