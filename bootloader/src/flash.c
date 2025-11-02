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
#define __FLASH_C__

/*
 * Flash programming is based on the following SDk example:
 * boards\frdmmcxn947\driver_examples\romapi\flashiap
 */

/*=======================================================================*/
/*  Includes                                                             */
/*=======================================================================*/
#include <stdio.h>
#include "tcts.h"
#include "flash.h"
#include "terminal.h"

#include "fsl_flash.h"
#include "fsl_flash_ffr.h"
#include "fsl_common.h"
#include "fsl_cache_lpcac.h"

/*=======================================================================*/
/*  All Structures and Common Constants                                  */
/*=======================================================================*/

#define BUFFER_LEN   (512 / 4)

/*=======================================================================*/
/*  Definition of all local Data                                         */
/*=======================================================================*/

/* Flash driver Structure */
static flash_config_t s_flashDriver;

static uint32_t pflashBlockBase  = 0U;
static uint32_t pflashTotalSize  = 0U;
static uint32_t pflashSectorSize = 0U;
static uint32_t PflashPageSize   = 0U;

/*=======================================================================*/
/*  Definition of all local Procedures                                   */
/*=======================================================================*/

/*
 * Clear speculation buffer.
 */
static void speculation_buffer_clear (void)
{
   /* Clear Flash/Flash data speculation. */
   if(((SYSCON->NVM_CTRL & SYSCON_NVM_CTRL_DIS_MBECC_ERR_INST_MASK) == 0U) 
       && ((SYSCON->NVM_CTRL & SYSCON_NVM_CTRL_DIS_MBECC_ERR_DATA_MASK) == 0U))
   {
      if((SYSCON->NVM_CTRL & SYSCON_NVM_CTRL_DIS_FLASH_SPEC_MASK) == 0U)
      {
         /* Disable flash speculation first. */
         SYSCON->NVM_CTRL |= SYSCON_NVM_CTRL_DIS_FLASH_SPEC_MASK;          
         /* Re-enable flash speculation. */
         SYSCON->NVM_CTRL &= ~SYSCON_NVM_CTRL_DIS_FLASH_SPEC_MASK;
      }
      if((SYSCON->NVM_CTRL & SYSCON_NVM_CTRL_DIS_DATA_SPEC_MASK) == 0U)
      {
         /* Disable flash data speculation first. */
         SYSCON->NVM_CTRL |= SYSCON_NVM_CTRL_DIS_DATA_SPEC_MASK;          
         /* Re-enable flash data speculation. */
         SYSCON->NVM_CTRL &= ~SYSCON_NVM_CTRL_DIS_DATA_SPEC_MASK;
      }
   }
} /* speculation_buffer_clear */

/*
 * Clear flash cache.
 */
static void flash_cache_clear (void)
{
   /* Clear Flash cache. */
   if((SYSCON->NVM_CTRL & SYSCON_NVM_CTRL_DIS_FLASH_CACHE_MASK) == 0U)
   {
      SYSCON->NVM_CTRL |= SYSCON_NVM_CTRL_CLR_FLASH_CACHE_MASK;
      SYSCON->NVM_CTRL &= ~SYSCON_NVM_CTRL_CLR_FLASH_CACHE_MASK;
   }
} /* flash_cache_clear */

/*
 * Clear L1 low power cache.
 */
static void lpcac_clear (void)
{
   /* Clear L1 low power cache. */
   if((SYSCON->LPCAC_CTRL & SYSCON_LPCAC_CTRL_DIS_LPCAC_MASK) == 0U)
   {
      L1CACHE_InvalidateCodeCache();
   }
} /* lpcac_clear */

/*=======================================================================*/
/*  All code exported                                                    */
/*=======================================================================*/

/*************************************************************************/
/*  flash_Init                                                           */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: 0 == OK / error cause                                        */
/*************************************************************************/
int flash_Init (void)
{
   int      rc = -1;
   status_t status;

   /* Clean up Flash, Cache driver Structure*/
   memset(&s_flashDriver, 0, sizeof(flash_config_t));

   /* Initialize flash driver */   
   status = FLASH_Init(&s_flashDriver);  
   if (status != kStatus_Success) goto end;  /*lint !e801*/

   /* Initializing FFR driver */
   status = FFR_Init(&s_flashDriver);
   if (status != kStatus_Success) goto end;  /*lint !e801*/

   /* Get flash properties kFLASH_ApiEraseKey */
   FLASH_GetProperty(&s_flashDriver, kFLASH_PropertyPflashBlockBaseAddr, &pflashBlockBase);
   FLASH_GetProperty(&s_flashDriver, kFLASH_PropertyPflashSectorSize, &pflashSectorSize);
   FLASH_GetProperty(&s_flashDriver, kFLASH_PropertyPflashTotalSize, &pflashTotalSize);
   FLASH_GetProperty(&s_flashDriver, kFLASH_PropertyPflashPageSize, &PflashPageSize);

   if (FLASH_SECTOR_SIZE == pflashSectorSize)
   {
      rc = 0;
   }
       
end:    
   
   return(rc);
} /* flash_Init */

/*************************************************************************/
/*  flash_EraseSector                                                    */
/*                                                                       */
/*  In    : dAddress                                                     */
/*  Out   : none                                                         */
/*  Return: 0 == OK / error cause                                        */
/*************************************************************************/
int flash_EraseSector (uint32_t dAddress)
{
   int      rc = -1;
   status_t status;
  
   status = FLASH_Erase(&s_flashDriver, dAddress, pflashSectorSize, kFLASH_ApiEraseKey);
   if (kStatus_Success == status)
   {
      /* Clear speculation buffer, flash cache and lpcac. */
      speculation_buffer_clear();
      flash_cache_clear();
      lpcac_clear();
      
      status = FLASH_VerifyErase(&s_flashDriver, dAddress, pflashSectorSize);
      if (kStatus_Success == status)
      {
         rc = 0;
      }
   }
   
   return(rc);
} /* flash_EraseSector */

/*************************************************************************/
/*  flash_Program                                                        */
/*                                                                       */
/*  In    : dAddress, pBuffer                                            */
/*  Out   : none                                                         */
/*  Return: 0 == OK / error cause                                        */
/*************************************************************************/
int flash_Program (uint32_t dAddress, uint8_t *pBuffer)
{
   int       rc   = -1;
   status_t  status;
   
   status = FLASH_Program(&s_flashDriver, dAddress, pBuffer, pflashSectorSize);
   if (kStatus_Success == status)
   {
      /* Clear speculation buffer, flash cache and lpcac. */
      speculation_buffer_clear();
      flash_cache_clear();
      lpcac_clear();

      /* Verify programming by reading back from flash directly */
      if (0 == memcmp((uint8_t*)dAddress, pBuffer, pflashSectorSize))
      {
         rc = 0;
      }
   }
   
   return(rc);
} /* flash_Program */

/*************************************************************************/
/*  flash_End                                                            */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void flash_End (void)
{
   /* Nothing to do */
} /* flash_End */

/*** EOF ***/
