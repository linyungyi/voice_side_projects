/*****************************************************************************
*  FILE: threads.c
*
*  DESCRIPTION:  A primitive, multi-threaded answering machine using
*      ctademo function library.  Each thread loops on a port, waiting
*      for a call, playing a message, and hanging up.
*
*  Voice Files Used -
*                ANSWER.VOX     - Default answering message.
*
* Copyright (c) 1995-2002 NMS Communications.  All rights reserved.
*****************************************************************************/
#define VERSION_NUM  "13"
#define VERSION_DATE __DATE__

#include "ctademo.h"


unsigned   ag_board    = 0;
unsigned   mvip_stream = 0;
unsigned   mvip_slot   = 0;
unsigned   nports      = 1;
char      *protocol    = "lps0";
char      *vox_file    = "answer.vox";


void PrintHelp( void )
{
    printf( "Usage: threads [opts]\n" );
    printf( "  where opts are:\n" );
    printf( "  -A adi_mgr  Natural Access manager to use for ADI service.        "
            "Default = %s\n", DemoGetADIManager() );
    printf( "  -F filename Natural Access Configuration File name.         "
            "Default = None\n" );
    printf( "  -b n        OAM board number.                          "
            "Default = %d\n", ag_board );
    printf( "  -s [n:]m    DSP [stream and] timeslot.                 "
            "Default = %d:%d\n", mvip_stream, mvip_slot );
    printf( "  -n nports   The number of ports (and threads) to use.  "
            "Default = %d\n", nports );
    printf( "  -p protocol Protocol to run.                           "
            "Default = %s\n", protocol );
    printf( "  -f filename Voice file to use for answering message.   "
            "Default = %s\n", vox_file );
    printf( "              (must be a VOX sound file).\n" );
}


/*******************************************************************************
  ProcessArgs

  Process command line options.
*******************************************************************************/
void ProcessArgs( int argc, char **argv )
{
    int c;

    while( ( c = getopt( argc, argv, "A:F:b:s:n:p:f:vHh?" ) ) != -1 )
    {
        switch( c )
        {
            unsigned   value1, value2;

            case 'A':
                DemoSetADIManager( optarg );
                break;
            case 'F':
                DemoSetCfgFile( optarg );
                break;
            case 'b':
                sscanf( optarg, "%d", &ag_board );
                break;
            case 's':
                switch (sscanf( optarg, "%d:%d", &value1, &value2 ))
                {
                    case 2:
                        mvip_stream = value1;
                        mvip_slot   = value2;
                        break;
                    case 1:
                        mvip_slot   = value1;
                        break;
                    default:
                        PrintHelp();
                        exit( 1 );
                }
                break;
            case 'n':
                sscanf( optarg, "%d", &nports );
                break;
            case 'p':
                protocol = optarg;
                break;
            case 'f':
                vox_file = optarg;
                break;
            case 'v':
                DemoSetReportLevel (DEMO_REPORT_ALL);
                break;
            case 'h':
            case 'H':
            case '?':
                PrintHelp();
                exit( 1 );
                break;
            default:
                printf("? Unrecognized switch %s\n", optarg );
                exit( 1 );
        }
    }
}

/******************************************************************************
  RunDemo

   Open a port, start protocol, and loop answering incoming calls.
   The needed parameters are taken from globals, except for the
   MVIP timeslot, which is passed in as the argument.
*******************************************************************************/
THREAD_RET_TYPE THREAD_CALLING_CONVENTION RunDemo( void *arg )
{
    CTAQUEUEHD  ctaqueuehd;       /* Natural Access queue handle                 */
    CTAHD       ctahd;            /* Natural Access context handle               */

    const unsigned timeslot = (unsigned)arg;

    int ncalls              = 0;

    CTA_SERVICE_DESC services[] =        /* for ctaOpenServices */
    {   { {"ADI", ""      }, { 0 }, { 0 }, { 0 } },
        { {"VCE", "VCEMGR"}, { 0 }, { 0 }, { 0 } }
    };

    /* Get Correct ADI Service Manager */
    services[0].name.svcmgrname = DemoGetADIManager();

    /* Fill in ADI service (index 0) MVIP address information */
    services[0].mvipaddr.board    = ag_board;
    services[0].mvipaddr.stream   = mvip_stream;
    services[0].mvipaddr.timeslot = timeslot;
    services[0].mvipaddr.mode     = ADI_FULL_DUPLEX;

    DemoOpenPort( 0, "DEMOCONTEXT", services, ARRAY_DIM(services),
                  &ctaqueuehd, &ctahd );
    DemoStartProtocol( ctahd, protocol, NULL, NULL );

    /* Loop forever, receiving calls and processing them.
     * If any function returns a failure or disconnect,
     * simply hang up and wait for the next call.
     */

    for(;;)
    {
        printf( "Waiting for call %d in slot %d...\n", ++ncalls, timeslot );

        DemoWaitForCall( ctahd, NULL, NULL );
        printf( "\t(in slot %d)\n", timeslot );

        if( DemoAnswerCall( ctahd, 1 ) != SUCCESS ) /* Answer after 1 ring. */
            goto hangup;

        /* Play message to the caller. */
        adiFlushDigitQueue( ctahd );
        if( DemoPlayMessage(ctahd, vox_file, 0, 0, NULL) != SUCCESS )
             goto hangup;

    hangup:
        DemoHangUp( ctahd );
    }

    /* All done - synchronously close services, destroy context, destroy queue*/
    UNREACHED_STATEMENT( ctaDestroyQueue( ctaqueuehd ); )
    UNREACHED_STATEMENT( THREAD_RETURN; )
}


/*******************************************************************************
 main
*******************************************************************************/
int main( int argc, char **argv )
{
    unsigned i;

    printf( "Natural Access Multi-thread Demo V.%s (%s)\n", VERSION_NUM, VERSION_DATE );
    DemoSetReportLevel (DEMO_REPORT_COMMENTS);

    ProcessArgs( argc, argv );

    printf( "\tBoard      = %d\n",    ag_board );
    printf( "\tStream:Slot= %d:%d\n", mvip_stream, mvip_slot );
    printf( "\tNPorts     = %d\n",    nports );
    printf( "\tProtocol   = %s\n",    protocol );
    printf( "\tvoice file = %s\n",    vox_file );
    printf( "\n" );

    /* Register an error handler to display errors returned from API functions;
     * no need to check return values in the demo.
     */
    ctaSetErrorHandler( DemoErrorHandler, NULL );

    /* initialize Natural Access services for this process */
    {
        CTA_SERVICE_NAME servicelist[] =     /* for ctaInitialize */
        {   { "ADI", ""       },
            { "VCE", "VCEMGR" }
        };

        /* Initialize ADI Service Manager name */
        servicelist[0].svcmgrname = DemoGetADIManager();

        DemoInitialize( servicelist, ARRAY_DIM(servicelist) );
    }

    for ( i=0; i<nports; i++ )
        DemoSpawnThread( RunDemo, (void *)(mvip_slot+i) );

    /* Sleep forever. */
    for(;;)
         DemoSleep( 1000 );

    UNREACHED_STATEMENT( return 0; )
}
