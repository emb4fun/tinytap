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
#define __PARA_C__

/*=======================================================================*/
/*  Includes                                                             */
/*=======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "compat.h"
#include "project.h"
#include "para.h"

/*=======================================================================*/
/*  All Structures and Common Constants                                  */
/*=======================================================================*/

typedef struct _para_list_entry_
{
   const char *Var;
   int        (*pFunc)(int index, int argc, char **argv);
} PARA_LIST_ENTRY;

/*=======================================================================*/
/*  Definition of all global Data                                        */
/*=======================================================================*/

PARAM Param;

/*=======================================================================*/
/*  Definition of all local Data                                         */
/*=======================================================================*/

/*=======================================================================*/
/*  Definition of prototypes                                             */
/*=======================================================================*/

/*=======================================================================*/
/*  Definition of all local Procedures                                   */
/*=======================================================================*/

/*************************************************************************/
/*  OutputUsage                                                          */
/*                                                                       */
/*  Output "usage" message.                                              */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
static void OutputUsage (void)
{
  printf("Usage: tinytap [-tb baud] [-td data] [-tp parity] [-ts stop] [-ip a.b.c.d] [-v] [-d] [-h]\n");
  printf("\n");
  printf("  -tb  Baud rate e.g. -tb 9600\n");
  printf("  -td  Data bits e.g. -td 8\n");
  printf("  -tp  Parity e.g. -tp N\n");
  printf("  -ts  Stop bits e.g. -ts 1\n");
  printf("  -ip  Select IP-Address of the TinyTAP server e.g. -ip 192.168.1.200\n");
  printf("  -v   Show version information only\n");
  printf("  -d   Discover, search TinyTAP server only\n");
  printf("  -h   Print version information and usage\n");

} /* OutputUsage */

/*************************************************************************/
/*  Para...                                                              */
/*                                                                       */
/*  In    : index, argc, argv                                            */
/*  Out   : none                                                         */
/*  Return: 1 to skip next argv (parameter value) / 0                    */
/*************************************************************************/
static int ParaHelp (int index, int argc, char **argv)
{
   (void)index;
   (void)argc;
   (void)argv;

   OutputUsage();
   exit(EXIT_OK);

   return(0);
} /* ParaHelp */

static int ParaDiscover (int index, int argc, char **argv)
{
   (void)index;
   (void)argc;
   (void)argv;

   Param.CmdDiscover = 1;

   return(0);
} /* ParaDiscover */

static int ParaVersion (int index, int argc, char **argv)
{
   (void)index;
   (void)argc;
   (void)argv;

   Param.CmdVersion = 1;

   return(0);
} /* ParaVersion */

static int ParaBaudrate (int index, int argc, char **argv)
{
   int rc = 0;

   if ((index + 1) < argc)
   {
      char *endptr = NULL;
      long value = strtol(argv[index + 1], &endptr, 10);

      if (*endptr != '\0' || value <= 0)
      {
         printf("Invalid baud rate : %s\n", argv[index + 1]);
         exit(EXIT_ERR_BAUDRATE);
      }

      Param.CmdBaudrate = 1;

      rc = 1;
      Param.dBaudrate = (uint32_t)value;

      switch (Param.dBaudrate)
      {
         case   1200:
         case   2400:
         case   4800:
         case   9600:
         case  19200:
         case  38400:
         case  57600:
         case 115200:
         {
            /* Supported baud rate */
            break;
         }
         default:
         {
            printf("Invalid baud rate : %d\n", Param.dBaudrate);
            printf("Baud rate not supported\n");
            exit(EXIT_ERR_BAUDRATE);
            break;
         }
      }
   }

   return(rc);
} /* ParaBaudrate */

static int ParaDataBits (int index, int argc, char **argv)
{
   int rc = 0;

   if ((index + 1) < argc)
   {
      char *endptr = NULL;
      long value = strtol(argv[index + 1], &endptr, 10);

      if (*endptr != '\0' || value <= 0)
      {
         printf("Invalid data bits : %s\n", argv[index + 1]);
         exit(EXIT_ERR_DATABITS);
      }

      Param.CmdData = 1;

      rc = 1;
      Param.dData = (uint32_t)value;

      if (Param.dData != 8)
      {
         printf("Invalid data bits : %d\n", Param.dData);
         printf("Data bits not supported\n");
         exit(EXIT_ERR_DATABITS);
      }
   }

   return(rc);
} /* ParaDataBits */

static int ParaParity (int index, int argc, char **argv)
{
   int rc = 0;

   if ((index + 1) < argc)
   {
      Param.CmdParity = 1;

      rc = 1;
      Param.Parity = toupper(argv[index + 1][0]);

      if ( (Param.Parity != 'N') &&
           (Param.Parity != 'O') &&
           (Param.Parity != 'E') )
      {
         printf("Invalid parity : %c\n", Param.Parity);
         printf("Parity not supported\n");
         exit(EXIT_ERR_PARITY);
      }
   }

   return(rc);
} /* ParaParity */

static int ParaStopBits (int index, int argc, char **argv)
{
   int rc = 0;

   if ((index + 1) < argc)
   {
      char *endptr = NULL;
      long value = strtol(argv[index + 1], &endptr, 10);

      if (*endptr != '\0' || value <= 0)
      {
         printf("Invalid stop bits : %s\n", argv[index + 1]);
         exit(EXIT_ERR_STOPBITS);
      }

      Param.CmdStop = 1;

      rc = 1;
      Param.dStop = (uint32_t)value;

      if (Param.dStop != 1)
      {
         printf("Invalid stop bits : %d\n", Param.dStop);
         printf("Stop bits not supported\n");
         exit(EXIT_ERR_STOPBITS);
      }
   }

   return(rc);
} /* ParaStopBits */

static int ParaIP (int index, int argc, char **argv)
{
   int rc = 0;

   if ((index + 1) < argc)
   {
      Param.CmdIP = 1;

      rc = 1;
      _snprintf(Param.IPName, IP_NAME_SIZE, "%s", argv[index + 1]);
   }

   return(rc);
} /* ParaIP */


/*
 * Parameter variable list
 */
static const PARA_LIST_ENTRY PARAList[] =   /*lint !e31*/
{
   { "-tb", ParaBaudrate   },
   { "-td", ParaDataBits   },
   { "-tp", ParaParity     },
   { "-ts", ParaStopBits   },
   { "-ip", ParaIP         },
   { "-v",  ParaVersion    },
   { "-d",  ParaDiscover   },
   { "-h",  ParaHelp       },

   { NULL, NULL }
};

/*=======================================================================*/
/*  All code exported                                                    */
/*=======================================================================*/

/*************************************************************************/
/*  ParaGetDefault                                                       */
/*                                                                       */
/*  In    : none                                                         */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void ParaGetDefault (void)
{
   memset(&Param, 0x00, sizeof(Param));

   Param.CmdBaudrate = 1;
   Param.CmdData     = 1;
   Param.CmdParity   = 1;
   Param.CmdStop     = 1;

   Param.dBaudrate = 9600;
   Param.dData     = 8;
   Param.Parity    = 'N';
   Param.dStop     = 1;

} /* ParaGetDefault */

/*************************************************************************/
/*  ParaDecode                                                           */
/*                                                                       */
/*  In    : argc, argv                                                   */
/*  Out   : none                                                         */
/*  Return: none                                                         */
/*************************************************************************/
void ParaDecode (int argc, char **argv)
{
   int               rc;
   int               index;
   int               CmdUnknown;
   PARA_LIST_ENTRY *pPara;

   for (index = 1; index < argc; index++)
   {
      /* Loop over the actual list */
      pPara      = (PARA_LIST_ENTRY*)&PARAList[0];
      CmdUnknown = 1;
      while(pPara->Var != NULL)
      {
         if (0 == strcmp(argv[index], pPara->Var))
         {
            rc = pPara->pFunc(index, argc, argv);
            if (-1 == rc)
            {
               /* Error */
            }
            index += rc;

            CmdUnknown = 0;
            break;
         }

         pPara++;
      }

      /* Check unknow command */
      if (1 == CmdUnknown)
      {
         /* Ups, found an unknown command */
         printf("Unknown parameter: %s\n\n", argv[index]);
         OutputUsage();
         exit(EXIT_ERR_UNKNOWN_PARAM);
      }
   }

} /* ParaDecode */

/*** EOF ***/
