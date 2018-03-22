/*****************************************************************************
*  FILE: imgtdemo.h
*
* Copyright (c) 1999-2001  NMS Communication.   All rights reserved.
*
*****************************************************************************/

#ifndef _IMGTDEMO_H_
#define _IMGTDEMO_H_

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------- standard include files -------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*----------------------------- adi include files ---------------------------*/

#include "adidef.h"
#include "ctadef.h"

/*----------------------------- isdn include files -------------------------*/

#include "isdndef.h"

/*----------------------------- imgt include files -------------------------*/

#include "imgtdef.h"
#include "imgtsvc.h"

/*----------------------------- Application Definitions --------------------*/



#if defined (WIN32)
  #include <process.h>
  #define THREAD_RET_TYPE             unsigned
  #define THREAD_CALLING_CONVENTION   __stdcall
  #define THREAD_RETURN               return 0
#elif defined (UNIX_SVR4) || defined (LINUX)
  #include <thread.h>
  #define THREAD_RET_TYPE             void *
  #define THREAD_CALLING_CONVENTION
  #define THREAD_RETURN               return NULL
#endif

#include "ctademo.h"

#define MX_NFAS_NAI     20

typedef struct
{
    unsigned    NFAS_group;         /* Nfas group number                    */
    unsigned    board;              /* board number                         */
    unsigned    trunk;              /* trunk number, from 0 - 3             */
    unsigned    nai;                /* Network Access Identifier            */
    unsigned    d_channel;          /* 1 - trunk bearing D-channel          */
                                    /* 0 - trunk not bearing D-channel      */
    unsigned    channel_status[31]; /* store information about channel's
                                       status on this nai (1-30, skip 0)    */
} MYCONTEXT;

#define MAX_ARGS 6
#define MAX_INPUT 132               /* max line len entered by the user     */

typedef struct
{   /* arg[0] = command, the others are optional args */
    char arg[MAX_ARGS][MAX_INPUT];
    char numargs;
}COMMAND;

/*----------------------------- Function Prototypes ------------------------*/
THREAD_RET_TYPE THREAD_CALLING_CONVENTION UserInput( void *pcx );
void GetArgs( char * input, COMMAND *comm);

void CommandLineHelp( void );
void ParseCommandLine( int argc, char **argv );
void PrintHelp( void );


void InitServices( void );
void InitContext( void );

THREAD_RET_TYPE THREAD_CALLING_CONVENTION RunDemo( void *pcx );

void StartImgt( COMMAND *comm );
void StopImgt( COMMAND *comm );
void SetImgtConfiguration( COMMAND *comm );
void CreateBChannelRq( COMMAND *comm, char code, unsigned type, unsigned status );
void CreateDChannelRq( COMMAND *comm, char code);
void SendImgtMessage( char primitive, unsigned nai, char *buffer );
void ShowBChannelStatus( void );
void ImgtFormatEvent( unsigned char *buffer );
void QueryBChannelStatus( MYCONTEXT *cx, unsigned type, unsigned channel );

#ifdef __cplusplus
}
#endif

#endif

