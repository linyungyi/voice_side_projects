/*****************************************************************************
  FILE:  tiktst.c

  PURPOSE:

      To test functionality of extension "TIK" api functions added to CT
      Access vi the TIK service manager and service.

  USAGE:

      See text in Usage() routine.

 Copyright (c) 1997-2002 NMS Communications.  All rights reserved.
*****************************************************************************/

/******************************************************************************
  Revision info.
  ****************************************************************************/
#define VERSION_NUM  "1"
#define VERSION_DATE __DATE__

/******************************************************************************
  Standard include files.
  ****************************************************************************/
#include <stdlib.h>
#include <stdio.h>

#include "ctademo.h"
#include "tikdef.h"

/******************************************************************************
  Error buffer size.
  ****************************************************************************/
#define  ERR_BUFFER_SIZE   64
#define  MSG_BUFFER_SIZE   ERR_BUFFER_SIZE

/******************************************************************************
  Object for running instance of demo.
  ****************************************************************************/
typedef struct _DEMO_OBJ
{
        unsigned          Size;        /* Size of this structure.             */
   unsigned          Mode;        /* Demo running mode.(See tikTstUsage) */
   unsigned          TraceOn;     /* 1 if -t specified, 0 if not.        */
   TIK_START_PARMS  *Start;       /* Option line supplied parameters     */
}       DEMO_OBJ;

/******************************************************************************
  Service list for ctaOpenServices.
  ****************************************************************************/
CTA_SERVICE_DESC tikOpenSvcLst[] = {{{"TIK", "TIKMGR"}, {0}, {0}, {0}}};

/******************************************************************************
  Service list for ctaCloseServices.
  ****************************************************************************/
char *tikCloseSvcLst[] = { "TIK" };

/******************************************************************************
  Forward references.
  ****************************************************************************/
static void tikTstUsage( void );

static void tikTstCreateStartParameterObject( DEMO_OBJ *Demo );

static void tikTstParseOption( int argc, char **argv, DEMO_OBJ *Demo );

static void tikTstErrorAndExit( CTAHD Ctahd, DWORD ErrId, char *Fcn );

static void tikTstWaitForEvent( CTAQUEUEHD Queue, CTA_EVENT *Event,
                                DWORD Timeout );

static void tikTstWaitForEventWithReason( CTAQUEUEHD Queue, DWORD EventId,
                                          DWORD Reason );

static void tikTstRunDemoMode1( DEMO_OBJ *Demo );

static void tikTstRunDemoMode2( DEMO_OBJ *Demo );

static void tikTstRunDemoMode3( DEMO_OBJ *Demo );


#define  TIKTST_DEFAULT_DEMO_MODE   1
/******************************************************************************
  Prints program usage.
  ****************************************************************************/
static void tikTstUsage( void )
{
   DEMO_OBJ  Demo;

   Demo.Mode = TIKTST_DEFAULT_DEMO_MODE;

   tikTstCreateStartParameterObject( &Demo );

   printf( "Usage: tiktst [opts]\n"
           "  where opts are:\n"
           "  -n (tick count to request)      currently set to %i   \n"
           "  -f (tick frequency to request)  currently set to %i   \n"
           "  -m (demo mode)                                        \n"
           "     1 - Multiple contexts(10) per queue                \n"
           "         (Default demo mode)                            \n"
           "     2 - Single context per queue w/ multiple queues(2) \n"
           "     3 - Single context per queue                       \n"
           "  -t (turn tracing on if ctdaemon is running)           \n"
           "         Demo CTA_TRACEMASK_DRIVER_COMMANDS             \n"
           "              CTA_TRACEMASK_DRIVER_EVENTS               \n"
           "              CTA_TRACEMASK_DEBUG_BIT_0                 \n",
           Demo.Start->NumTicks, Demo.Start->Frequency
         );
}

/******************************************************************************
  Allocate TIK_START_PARMS object and initialize default parameters.
  ****************************************************************************/
static void tikTstCreateStartParameterObject( DEMO_OBJ *Demo )
{
   DWORD            Ret;
   TIK_START_PARMS *Start;

   if ( Demo == NULL )
   {
      return;
   }

   Demo->Start = NULL;

   Start = (TIK_START_PARMS *)calloc( 1, sizeof( TIK_START_PARMS ) );
   if ( Start == NULL )
   {
      return;
   }

    /*
    *   Acquire default TIK_START_PARMS parameter
    *   prior to command line argument processing.
    */
    Ret = ctaGetParms( NULL_CTAHD, TIK_START_PARMID,
                       Start, sizeof(TIK_START_PARMS) );
    if ( Ret != SUCCESS )
    {
       char  Err[ERR_BUFFER_SIZE];
       ctaGetText( NULL_CTAHD, Ret, Err, ERR_BUFFER_SIZE );
       printf("Failed to acquire default start parameters.");
       printf("  Reason (%s).\n", Err);
       free( Start );
       exit( 1 );
    }

    Demo->Start = Start;
}

/******************************************************************************
  Parse command line options.
  ****************************************************************************/
static void tikTstParseOption( int argc, char **argv, DEMO_OBJ *Demo )
{
   int              i;

   if ( Demo == NULL )
   {
      return;
   }

   /*
   *   Default demo mode is TIKTST_DEFAULT_DEMO_MODE.
   */
   Demo->Mode = TIKTST_DEFAULT_DEMO_MODE;

   /*
   *   Process command line arguments.
   */
   while ( (i = getopt( argc, argv, "tn:f:m:Hh?")) != -1 )
   {
      switch (i)
      {
         case 'n':
            if ( Demo->Start == NULL )
            {
               tikTstCreateStartParameterObject( Demo );
            }
            Demo->Start->NumTicks  = atoi(optarg);
            break;
         case 'f':
            if ( Demo->Start == NULL )
            {
               tikTstCreateStartParameterObject( Demo );
            }
            Demo->Start->Frequency = atoi(optarg);
            break;
         case 't':
            Demo->TraceOn = 1;
            break;
         case 'm':
            Demo->Mode = atoi(optarg);
            if ( (Demo->Mode != 0) && (Demo->Mode <= 3) )
            {
               break;
            }
            printf( "Invalid demo mode specification.\n" );
         case 'h':
         case 'H':
         case '?':
         default:
            tikTstUsage();
            exit(1);
            break;
      }
   }
}

/******************************************************************************
  Print Natural Access error and exit.

  ErrId - Error Id.
  Fcn   - Function responsible for ErrId.
  ****************************************************************************/
static void tikTstErrorAndExit( CTAHD Ctahd, DWORD ErrId, char *Fcn )
{
   char  Err[ERR_BUFFER_SIZE];

   ctaGetText( Ctahd, ErrId, Err, ERR_BUFFER_SIZE );
   printf( "Error : (%s) error returned in (%s) call.\n", Err, Fcn );
   exit( 1 );
}

/******************************************************************************
  Wait for a specified event with reason.
  ****************************************************************************/
static void tikTstWaitForEvent( CTAQUEUEHD Queue, CTA_EVENT *Event,
                                DWORD Timeout )
{
   DWORD        Ret;

   Ret = ctaWaitEvent( Queue, Event, Timeout );
   if ( Ret != SUCCESS )
   {
      char  Err[ERR_BUFFER_SIZE];
      ctaGetText( Event->ctahd, Ret, Err, ERR_BUFFER_SIZE );
      printf( "CTAHD(0x%08X) : ctaWaitEvent failed with (%s) "
              "error.\n",
              Event->ctahd, Err );
      exit( 1 );
   }
}

/******************************************************************************
  Wait for a specified event with reason.
  ****************************************************************************/
static void tikTstWaitForEventWithReason( CTAQUEUEHD Queue, DWORD EventId,
                                          DWORD Reason )
{
   CTA_EVENT    Event = {0};

   tikTstWaitForEvent( Queue, &Event, CTA_WAIT_FOREVER );

   if ( Event.id != EventId )
   {
      char  Err[ERR_BUFFER_SIZE];
      char  EvtTxt[ERR_BUFFER_SIZE];
      ctaGetText( Event.ctahd, EventId, EvtTxt, ERR_BUFFER_SIZE );
      ctaGetText( Event.ctahd, Event.id, Err, ERR_BUFFER_SIZE );
      printf( "CTAHD(0x%08X) : (%s) Event is returned while waiting "
              "for (%s) event.\n",
              Event.ctahd, EvtTxt, Err );
      exit( 1 );
   }

   if ( Event.value != Reason )
   {
      char  Err[ERR_BUFFER_SIZE];
      char  RsnTxt[ERR_BUFFER_SIZE];

      ctaGetText( Event.ctahd, Reason, RsnTxt, ERR_BUFFER_SIZE );
      ctaGetText( Event.ctahd, Event.value, Err, ERR_BUFFER_SIZE );
      printf( "CTAHD(0x%08X) : (%s) reason returned while waiting "
              "for (%s) reason.\n",
              Event.ctahd, RsnTxt, Err );
      exit( 1 );
   }
}

/******************************************************************************
  Routine to test TIK service & service manager functionality for demo mode 1.
  ****************************************************************************/
static void tikTstRunDemoMode1( DEMO_OBJ *Demo )
{
   #define TIKSVR_MAX_CHANNEL    10

   /* Need one queue. */
   CTAQUEUEHD        FstQueue = 0;

   /* Need array of 10 context handles. */
   CTAHD             CtaHd[TIKSVR_MAX_CHANNEL] = { 0 };

   /* Event structure to wait for TIK events. */
   CTA_EVENT         Event;

   /* Structure to hold TIK context information. */
   TIK_CHANNEL_INFO  ChanInfo;
   unsigned          OutSize;

   /* Utility varaibles. */
   unsigned          Ctr;
   DWORD             Ret;

   /* Message translation buffer */
   char              Msg[MSG_BUFFER_SIZE];

   Ret = ctaCreateQueue( NULL, 0, &FstQueue );
   if ( Ret != SUCCESS )
   {
      tikTstErrorAndExit( NULL_CTAHD, Ret, "ctaCreateQueue" );
   }

   /*
   *   Create 10 contexts and open TIK service on all contexts.
   */
   for ( Ctr = 0; Ctr < TIKSVR_MAX_CHANNEL; Ctr++ )
   {
      Ret = ctaCreateContext( FstQueue, 0, NULL, &CtaHd[Ctr] );

      if ( Ret != SUCCESS )
      {
         tikTstErrorAndExit( NULL_CTAHD, Ret, "ctaCreateContext" );
      }

      tikOpenSvcLst[0].svcargs.args[0] = (DWORD)(Ctr);

      Ret = ctaOpenServices( CtaHd[Ctr], tikOpenSvcLst, 1 );
      if ( Ret != SUCCESS )
      {
         tikTstErrorAndExit( NULL_CTAHD, Ret, "ctaOpenServices" );
      }

      /* Wait for service open to complete. */
      tikTstWaitForEventWithReason( FstQueue, CTAEVN_OPEN_SERVICES_DONE,
                                    CTA_REASON_FINISHED );

      /* Set trace level. */
      if ( Demo->TraceOn )
      {
         Ret = ctaSetTraceLevel( CtaHd[Ctr], NULL,
                                 CTA_TRACEMASK_DRIVER_COMMANDS |
                                 CTA_TRACEMASK_DRIVER_EVENTS   |
                                 CTA_TRACEMASK_DEBUG_BIT0 );
         if ( Ret != SUCCESS )
         {
            tikTstErrorAndExit( CtaHd[Ctr], Ret, "ctaSetTraceLevel" );
         }
      }

      printf( "Context created and service opened on channel(%d).\n", Ctr );
   }

   /*
   *   Start timer on all contexts.
   */
   for ( Ctr = 0; Ctr < TIKSVR_MAX_CHANNEL; Ctr++)
   {
      /* Start the ticker. */

      /*
      *   NOTE : If -n or -f is not specified in
      *   command line option, Demo->Start will
      *   be NULL and context default Start parameters
      *   will be used.
      */
      Ret = tikStartTimer( CtaHd[Ctr], Demo->Start );
      if ( Ret != SUCCESS )
      {
         tikTstErrorAndExit( CtaHd[Ctr], Ret, "tikStartTimer" );
      }
      printf( "Timer started on channel(%d).\n", Ctr );
   }

   /*
   *   Wait for events until the ticker on all context is done.
   */
   Ctr = 0;
   while ( TRUE )
   {
      tikTstWaitForEvent( FstQueue, &Event, CTA_WAIT_FOREVER );
      switch ( Event.id )
      {
         case TIKEVN_TIMER_TICK:
            /*
            *   Get information regarding the current context.
            */
            tikGetChannelInfo( Event.ctahd,
                               &ChanInfo,
                               sizeof(TIK_CHANNEL_INFO),
                               &OutSize );
            /*
            *   Size(%d) is printed here to demostrate usage of
            *   CTA_VIRTUAL_BUF during parameter marshalling
            *   and how service can return synchronous result
            *   to SPI and API.
            */
            printf( "CtaHd(0x%08X) : Size(%d) Tick (%i) of (%i) with"
                    " string \"%s\".  Call(#%i).\n",
                    Event.ctahd, OutSize, ChanInfo.ticked,
                    ChanInfo.requested, ChanInfo.tick_string,
                    ChanInfo.calls );
            break;


         case TIKEVN_TIMER_DONE:
            ctaGetText( Event.ctahd, Event.value, Msg,
                        MSG_BUFFER_SIZE );
            printf( "CTAHD(0x%08X) : Ticker done, reason %s.\n",
                    Event.ctahd, Msg );
            break;

         default:
            printf( "CTAHD(0x%08X) : Unhandled event id (0x%x) during "
                    "tick wait.\nExiting...\n", Event.ctahd,
                    Event.id );
      }

      if ( (Event.id == TIKEVN_TIMER_DONE) && (++Ctr == TIKSVR_MAX_CHANNEL) )
      {
         break;
      }
   }

   /*
   *   Testing tikStopTimer return event.
   */
   for ( Ctr = 0; Ctr < TIKSVR_MAX_CHANNEL; Ctr++ )
   {
      tikStopTimer( CtaHd[Ctr] );
      printf( "Stopping timer on context(%d).\n", Ctr );
   }

   /* Wait for stop result. */
   Ctr = 0;
   while ( TRUE )
   {
      tikTstWaitForEventWithReason( FstQueue, TIKEVN_TIMER_DONE,
                                    CTA_REASON_FINISHED );

      printf( "Timer stopped on (%d) contexts.\n", Ctr++ );

      if ( Ctr == TIKSVR_MAX_CHANNEL )
      {
         break;
      }
   }

   /*
   *   Close service and destroy context.
   */
   for ( Ctr = 0; Ctr < TIKSVR_MAX_CHANNEL; Ctr++ )
   {
      Ret = ctaCloseServices( CtaHd[Ctr], tikCloseSvcLst, 1 );
      if ( Ret != SUCCESS )
      {
         tikTstErrorAndExit( CtaHd[Ctr], Ret, "ctaCloseServices" );
      }

      tikTstWaitForEventWithReason( FstQueue, CTAEVN_CLOSE_SERVICES_DONE,
                                    CTA_REASON_FINISHED );

      printf( "Services closed on channel(%d).\n", Ctr );

      Ret = ctaDestroyContext( CtaHd[Ctr] );

      if ( Ret != SUCCESS )
      {
         tikTstErrorAndExit( CtaHd[Ctr], Ret, "ctaDestroyContext" );
      }

      tikTstWaitForEventWithReason( FstQueue, CTAEVN_DESTROY_CONTEXT_DONE,
                                    CTA_REASON_FINISHED );

      printf( "Context(%d) destroyed.\n", Ctr );
   }

   Ret = ctaDestroyQueue( FstQueue );
   if ( Ret != SUCCESS )
   {
      tikTstErrorAndExit( NULL_CTAHD, Ret, "ctaDestroyQueue" );
   }

   printf( "Natural Access queue destroyed.\n" );
}

/******************************************************************************
  Routine to test TIK service & service manager functionality for demo mode 2.
  ****************************************************************************/
static void tikTstRunDemoMode2( DEMO_OBJ *Demo )
{
   /* Need two queue handles. */
   CTAQUEUEHD      FstQueue = 0;
   CTAQUEUEHD      SndQueue = 0;

   /* Need two contexts. */
   CTAHD           FstCtaHd = NULL_CTAHD;
   CTAHD           SndCtaHd = NULL_CTAHD;

   /* Event structure to wait for TIK events. */
   CTA_EVENT         Event;

   /* Structure to hold TIK context information. */
   TIK_CHANNEL_INFO  ChanInfo;
   unsigned          OutSize;

   /* Utility varaibles. */
   unsigned        FstCtaHdDone = FALSE;
   unsigned        SndCtaHdDone = FALSE;
   DWORD           Ret;

   /* Message translation buffer */
   char              Msg[MSG_BUFFER_SIZE];

   Ret = ctaCreateQueue( NULL, 0, &FstQueue );
   if ( Ret != SUCCESS )
   {
      tikTstErrorAndExit( NULL_CTAHD, Ret, "ctaCreateQueue" );
   }
   printf( "First queue created.\n" );

   Ret = ctaCreateQueue( NULL, 0, &SndQueue );
   if ( Ret != SUCCESS )
   {
      tikTstErrorAndExit( NULL_CTAHD, Ret, "ctaCreateQueue" );
   }
   printf( "Second queue created.\n" );

   /* Open FstCtaHd on FstQueue                   */
   /* Open logical channel 0 on context.          */
   {
      Ret = ctaCreateContext( FstQueue, 0, NULL, &FstCtaHd );

      if ( Ret != SUCCESS )
      {
         tikTstErrorAndExit( NULL_CTAHD, Ret, "ctaCreateContext" );
      }

      tikOpenSvcLst[0].svcargs.args[0] = 0;

      Ret = ctaOpenServices( FstCtaHd, tikOpenSvcLst, 1 );
      if ( Ret != SUCCESS )
      {
         tikTstErrorAndExit( FstCtaHd, Ret, "ctaOpenServices" );
      }

      /* Wait for service open to complete. */
      tikTstWaitForEventWithReason( FstQueue,
                                    CTAEVN_OPEN_SERVICES_DONE,
                                    CTA_REASON_FINISHED );

      /* Set trace level. */
      if ( Demo->TraceOn )
      {
         Ret = ctaSetTraceLevel( FstCtaHd, NULL,
                                 CTA_TRACEMASK_DRIVER_COMMANDS |
                                 CTA_TRACEMASK_DRIVER_EVENTS   |
                                 CTA_TRACEMASK_DEBUG_BIT0 );
         if ( Ret != SUCCESS )
         {
            tikTstErrorAndExit( FstCtaHd, Ret, "ctaSetTraceLevel" );
         }
      }

      printf( "Context created and service opened on FstQueue.\n" );
   }

   /* Open SndCtaHd on SndQueue                   */
   /* Open logical channel 0 on context.          */
   {
      Ret = ctaCreateContext( SndQueue, 0, NULL, &SndCtaHd );

      if ( Ret != SUCCESS )
      {
         tikTstErrorAndExit( NULL_CTAHD, Ret, "ctaCreateContext" );
      }

      tikOpenSvcLst[0].svcargs.args[0] = 0;

      Ret = ctaOpenServices( SndCtaHd, tikOpenSvcLst, 1 );
      if ( Ret != SUCCESS )
      {
         tikTstErrorAndExit( SndCtaHd, Ret, "ctaOpenServices" );
      }

      /* Wait for service open to complete. */
      tikTstWaitForEventWithReason( SndQueue,
                                    CTAEVN_OPEN_SERVICES_DONE,
                                    CTA_REASON_FINISHED );

      /* Set trace level. */
      if ( Demo->TraceOn )
      {
         /* Set trace level on context. */
         Ret = ctaSetTraceLevel( SndCtaHd, NULL,
                                 CTA_TRACEMASK_DRIVER_COMMANDS |
                                 CTA_TRACEMASK_DRIVER_EVENTS   |
                                 CTA_TRACEMASK_DEBUG_BIT0 );
         if ( Ret != SUCCESS )
         {
            tikTstErrorAndExit( SndCtaHd, Ret, "ctaSetTraceLevel" );
         }
      }

      printf( "Context created and service opened on SndQueue.\n" );
   }

   /* Start timer on both contexts.
   *
   *   NOTE : If -n or -f is not specified in
   *   command line option, Demo->Start will
   *   be NULL and context default Start parameters
   *   will be used.
   */

   Ret = tikStartTimer( FstCtaHd, Demo->Start );
   if ( Ret != SUCCESS )
   {
      tikTstErrorAndExit( FstCtaHd, Ret, "tikStartTimer" );
   }
   printf( "Timer started on FstCtaHd.\n" );

   Ret = tikStartTimer( SndCtaHd, Demo->Start );
   if ( Ret != SUCCESS )
   {
      tikTstErrorAndExit( SndCtaHd, Ret, "tikStartTimer" );
   }
   printf( "Timer started on SndCtaHd.\n" );

   /*
   *   Alternate and wait on two queues until
   *   all tickings are done.
   */
   while ( TRUE )
   {
      DWORD Timeout = 50;  /* In milli second. */
      tikTstWaitForEvent( FstQueue, &Event, Timeout );

      if ( Event.id != CTAEVN_WAIT_TIMEOUT )
      {
         switch ( Event.id )
         {
            case TIKEVN_TIMER_TICK:
               /*
               *   Get information regarding the current context.
               */
               tikGetChannelInfo( Event.ctahd,
                                  &ChanInfo,
                                  sizeof(TIK_CHANNEL_INFO),
                                  &OutSize );
               /*
               *   Size(%d) is printed here to demostrate usage of
               *   CTA_VIRTUAL_BUF during parameter marshalling
               *   and how service can return synchronous result
               *   to SPI and API.
               */
               printf( "CtaHd(0x%08X) : Size(%d) Tick (%i) of (%i) with"
                       " string \"%s\".  Call(#%i).\n",
                       Event.ctahd, OutSize, ChanInfo.ticked,
                       ChanInfo.requested, ChanInfo.tick_string,
                       ChanInfo.calls );
               break;

            case TIKEVN_TIMER_DONE:
               ctaGetText( Event.ctahd, Event.value, Msg,
                           MSG_BUFFER_SIZE );
               printf( "CTAHD(0x%08X) : Ticker done, reason %s.\n",
                       Event.ctahd, Msg );
               FstCtaHdDone = TRUE;
               break;

            default:
               printf( "CTAHD(0x%08X) : Unhandled event id (0x%x) during "
                       "tick wait.\nExiting...\n", Event.ctahd,
                       Event.id );
         }
      }
      tikTstWaitForEvent( SndQueue, &Event, Timeout );

      if ( Event.id != CTAEVN_WAIT_TIMEOUT )
      {
         switch ( Event.id )
         {
            case TIKEVN_TIMER_TICK:
               /*
               *   Get information regarding the current context.
               */
               tikGetChannelInfo( Event.ctahd,
                                  &ChanInfo,
                                  sizeof(TIK_CHANNEL_INFO),
                                  &OutSize );
               /*
               *   Size(%d) is printed here to demostrate usage of
               *   CTA_VIRTUAL_BUF during parameter marshalling
               *   and how service can return synchronous result
               *   to SPI and API.
               */
               printf( "CtaHd(0x%08X) : Size(%d) Tick (%i) of (%i) with"
                       " string \"%s\".  Call(#%i).\n",
                       Event.ctahd, OutSize, ChanInfo.ticked,
                       ChanInfo.requested, ChanInfo.tick_string,
                       ChanInfo.calls );
               break;
            case TIKEVN_TIMER_DONE:
               ctaGetText( Event.ctahd, Event.value, Msg,
                           MSG_BUFFER_SIZE );
               printf( "CTAHD(0x%08X) : Ticker done, reason %s.\n",
                       Event.ctahd, Msg );
               SndCtaHdDone = TRUE;
               break;

            default:
               printf( "CTAHD(0x%08X) : Unhandled event id (0x%x) during "
                       "tick wait.\nExiting...\n", Event.ctahd,
                       Event.id );
         }
      }

      if ( FstCtaHdDone && SndCtaHdDone )
      {
         break;
      }
   }

   /* Stop timer on both contexts. */
   Ret = tikStopTimer( FstCtaHd );
   if ( Ret != SUCCESS )
   {
      tikTstErrorAndExit( FstCtaHd, Ret, "tikStopTimer" );
   }
   printf( "Stopping timer on FstCtaHd.\n" );

   Ret = tikStopTimer( SndCtaHd );
   if ( Ret != SUCCESS )
   {
      tikTstErrorAndExit( SndCtaHd, Ret, "tikStopTimer" );
   }
   printf( "Stopping timer on SndCtaHd.\n" );


   /*
   *   Wait on two queues
   *   until both tick stop
   *   is done.
   */
   tikTstWaitForEventWithReason( FstQueue, TIKEVN_TIMER_DONE,
                                 CTA_REASON_FINISHED );

   printf( "Timer stopped on FstCtaHd.\n" );

   tikTstWaitForEventWithReason( SndQueue, TIKEVN_TIMER_DONE,
                                 CTA_REASON_FINISHED );

   printf( "Timer stopped on SndCtaHd.\n" );

   /*
   *   Close service and destroy context.
   */
   Ret = ctaCloseServices( FstCtaHd, tikCloseSvcLst, 1 );
   if ( Ret != SUCCESS )
   {
      tikTstErrorAndExit( FstCtaHd, Ret, "ctaCloseServices" );
   }

   tikTstWaitForEventWithReason( FstQueue, CTAEVN_CLOSE_SERVICES_DONE,
                                 CTA_REASON_FINISHED );

   printf( "Services closed on FstCtaHd.\n" );

   Ret = ctaDestroyContext( FstCtaHd );

   if ( Ret != SUCCESS )
   {
      tikTstErrorAndExit( FstCtaHd, Ret, "ctaDestroyContext" );
   }

   tikTstWaitForEventWithReason( FstQueue, CTAEVN_DESTROY_CONTEXT_DONE,
                                 CTA_REASON_FINISHED );

   printf( "FstCtaHd destroyed.\n" );


   Ret = ctaCloseServices( SndCtaHd, tikCloseSvcLst, 1 );
   if ( Ret != SUCCESS )
   {
      tikTstErrorAndExit( SndCtaHd, Ret, "ctaCloseServices" );
   }

   tikTstWaitForEventWithReason( SndQueue, CTAEVN_CLOSE_SERVICES_DONE,
                                 CTA_REASON_FINISHED );

   printf( "Services closed on SndCtaHd.\n" );

   Ret = ctaDestroyContext( SndCtaHd );

   if ( Ret != SUCCESS )
   {
      tikTstErrorAndExit( SndCtaHd, Ret, "ctaDestroyContext" );
   }

   tikTstWaitForEventWithReason( SndQueue, CTAEVN_DESTROY_CONTEXT_DONE,
                                 CTA_REASON_FINISHED );

   printf( "SndCtaHd destroyed.\n" );

   Ret = ctaDestroyQueue( FstQueue );
   if ( Ret != SUCCESS )
   {
      tikTstErrorAndExit( NULL_CTAHD, Ret, "ctaDestroyQueue" );
   }

   printf( "FstQueue destroyed.\n" );

   Ret = ctaDestroyQueue( SndQueue );
   if ( Ret != SUCCESS )
   {
      tikTstErrorAndExit( NULL_CTAHD, Ret, "ctaDestroyQueue" );
   }

   printf( "SndQueue destroyed.\n" );
}

/******************************************************************************
  Routine to test TIK service & service manager functionality for demo mode 3.
  ****************************************************************************/
static void tikTstRunDemoMode3( DEMO_OBJ *Demo )
{
   /* Need one queue handle. */
   CTAQUEUEHD      FstQueue = 0;

   /* Need one contexts. */
   CTAHD           FstCtaHd = NULL_CTAHD;

   /* Event structure to wait for TIK events. */
   CTA_EVENT         Event;

   /* Structure to hold TIK context information. */
   TIK_CHANNEL_INFO  ChanInfo;
   unsigned          OutSize;

   /* Utility varaibles. */
   DWORD           Ret;

   /* Message translation buffer */
   char              Msg[MSG_BUFFER_SIZE];

   Ret = ctaCreateQueue( NULL, 0, &FstQueue );
   if ( Ret != SUCCESS )
   {
      tikTstErrorAndExit( NULL_CTAHD, Ret, "ctaCreateQueue" );
   }
   printf( "Queue created.\n" );


   /* Open FstCtaHd on FstQueue                   */
   /* Open logical channel 0 on context.          */
   {
      Ret = ctaCreateContext( FstQueue, 0, NULL, &FstCtaHd );

      if ( Ret != SUCCESS )
      {
         tikTstErrorAndExit( NULL_CTAHD, Ret, "ctaCreateContext" );
      }

      tikOpenSvcLst[0].svcargs.args[0] = 0;

      Ret = ctaOpenServices( FstCtaHd, tikOpenSvcLst, 1 );
      if ( Ret != SUCCESS )
      {
         tikTstErrorAndExit( FstCtaHd, Ret, "ctaOpenServices" );
      }

      /* Wait for service open to complete. */
      tikTstWaitForEventWithReason( FstQueue,
                                    CTAEVN_OPEN_SERVICES_DONE,
                                    CTA_REASON_FINISHED );

      /* Set trace level. */
      if ( Demo->TraceOn )
      {
         Ret = ctaSetTraceLevel( FstCtaHd, NULL,
                                 CTA_TRACEMASK_DRIVER_COMMANDS |
                                 CTA_TRACEMASK_DRIVER_EVENTS   |
                                 CTA_TRACEMASK_DEBUG_BIT0 );
         if ( Ret != SUCCESS )
         {
            tikTstErrorAndExit( FstCtaHd, Ret, "ctaSetTraceLevel" );
         }
      }

      printf( "Context created and service opened on queue.\n" );
   }

   /* Start timer on context.
   *
   *   NOTE : If -n or -f is not specified in
   *   command line option, Demo->Start will
   *   be NULL and context default Start parameters
   *   will be used.
   */
   Ret = tikStartTimer( FstCtaHd, Demo->Start );
   if ( Ret != SUCCESS )
   {
      tikTstErrorAndExit( FstCtaHd, Ret, "tikStartTimer" );
   }
   printf( "Timer started on context.\n" );

   /*
   *   Wait for events until the ticker on context is done.
   */
   while ( TRUE )
   {
      tikTstWaitForEvent( FstQueue, &Event, CTA_WAIT_FOREVER );
      switch ( Event.id )
      {
         case TIKEVN_TIMER_TICK:
            /*
            *   Get information regarding the current context.
            */
            tikGetChannelInfo( Event.ctahd,
                               &ChanInfo,
                               sizeof(TIK_CHANNEL_INFO),
                               &OutSize );
            /*
            *   Size(%d) is printed here to demostrate usage of
            *   CTA_VIRTUAL_BUF during parameter marshalling
            *   and how service can return synchronous result
            *   to SPI and API.
            */
            printf( "CtaHd(0x%08X) : Size(%d) Tick (%i) of (%i) with"
                    " string \"%s\".  Call(#%i).\n",
                    Event.ctahd, OutSize, ChanInfo.ticked,
                    ChanInfo.requested, ChanInfo.tick_string,
                    ChanInfo.calls );
            break;

         case TIKEVN_TIMER_DONE:
            ctaGetText( Event.ctahd, Event.value, Msg,
                        MSG_BUFFER_SIZE );
            printf( "CTAHD(0x%08X) : Ticker done, reason %s.\n",
                    Event.ctahd, Msg );
            break;

         default:
            printf( "CTAHD(0x%08X) : Unhandled event id (0x%x) during "
                    "tick wait.\nExiting...\n", Event.ctahd,
                    Event.id );
      }

      if ( (Event.id == TIKEVN_TIMER_DONE) )
      {
         break;
      }
   }

   /*
   *   Testing tikStopTimer return event.
   */
   tikStopTimer( FstCtaHd );
   printf( "Stopping timer on context.\n" );

   tikTstWaitForEventWithReason( FstQueue, TIKEVN_TIMER_DONE,
                                 CTA_REASON_FINISHED );

   printf( "Timer stopped on context.\n" );

   /*
   *   Close service and destroy context.
   */
   Ret = ctaCloseServices( FstCtaHd, tikCloseSvcLst, 1 );
   if ( Ret != SUCCESS )
   {
      tikTstErrorAndExit( FstCtaHd, Ret, "ctaCloseServices" );
   }

   tikTstWaitForEventWithReason( FstQueue, CTAEVN_CLOSE_SERVICES_DONE,
                                 CTA_REASON_FINISHED );

   printf( "Services closed on context.\n" );

   Ret = ctaDestroyContext( FstCtaHd );

   if ( Ret != SUCCESS )
   {
      tikTstErrorAndExit( FstCtaHd, Ret, "ctaDestroyContext" );
   }

   tikTstWaitForEventWithReason( FstQueue, CTAEVN_DESTROY_CONTEXT_DONE,
                                 CTA_REASON_FINISHED );

   printf( "Context destroyed.\n" );

   Ret = ctaDestroyQueue( FstQueue );
   if ( Ret != SUCCESS )
   {
      tikTstErrorAndExit( NULL_CTAHD, Ret, "ctaDestroyQueue" );
   }

   printf( "Queue destroyed.\n" );
}

/*****************************************************************************
  Main entry point for program.

  Demo runs to completetion.
  If error occurs, debug info is printed to stdout before termination.
  ***************************************************************************/
void main( int argc, char **argv )
{
   /* Demo object of current running tiktst.exe */
   DEMO_OBJ   Demo  = { 0 };

   /* Natural Access variables. */
   CTA_INIT_PARMS      tikInitparms =                 { 0 };
   CTA_ERROR_HANDLER   oldErrorHandler =              NULL;
   CTA_SERVICE_NAME    tikServiceNames[] =            { { "TIK", "TIKMGR" } };

   /* Utility variable. */
   DWORD      Ret;
   unsigned   DaemonRunning = 1;

   printf( "TIK Service Demo V.%s (%s)\n\n", VERSION_NUM, VERSION_DATE );

   tikInitparms.size = sizeof(CTA_INIT_PARMS);
   tikInitparms.traceflags = CTA_TRACE_ENABLE;
   tikInitparms.parmflags = CTA_PARM_MGMT_SHARED;
   tikInitparms.ctacompatlevel = CTA_COMPATLEVEL;

   /*
   *   Initialize ctaccess with TIK service.
   *   Following intialization detects presence
   *   of running ctdaemon.exe and covers demo
   *   run mode 2, 3.
   */
   Ret = ctaInitialize(tikServiceNames, 1, &tikInitparms);
   if (Ret != SUCCESS)
   {
       printf("ERROR code 0x%08x initializing Natural Access.", Ret);
       exit( 1 );
   }

   /* Server designed to work only with library mode applications. */
   Ret = ctaSetDefaultServer("inproc");
   if (Ret != SUCCESS)
   {
       printf("ERROR code 0x%08x set default server.", Ret);
       exit( 1 );
   }

    /* Parse command line options. */
    tikTstParseOption( argc, argv, &Demo );

    /* Ignore -t option from command line if ctdaemon is not running */
    if ( DaemonRunning == 0 && Demo.TraceOn != 0 )
    {
       printf( "ctdaemon utility is not running.  -t option ignored.\n\n" );
       Demo.TraceOn &= DaemonRunning;
    }

    /*
    *   Tell Natural Access to report errors.
    */
    ctaSetErrorHandler( CTA_REPORT_ERRORS, &oldErrorHandler );

    /*
    *   Run the demo.
    */
    switch ( Demo.Mode )
    {
       case 1:
          tikTstRunDemoMode1( &Demo );
          break;
       case 2:
          tikTstRunDemoMode2( &Demo );
          break;
       case 3:
          tikTstRunDemoMode3( &Demo );
          break;
    }

    printf( "Demo mode (%d) successfully completed.\n", Demo.Mode );
}
