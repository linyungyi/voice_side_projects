/*****************************************************************************
*  FILE: brd2brd.c
*
*  DESCRIPTION:  This program demonstrates the use of the point-to-point switch
*                API to connect incoming and outgoing calls.
*
*  Copyright (c) 1999-2002 NMS Communications.  All rights reserved.
*****************************************************************************/
#define VERSION_NUM  "3"
#define VERSION_DATE __DATE__

#include "ctademo.h"
#include <ppxdef.h>


/*----------------------------- Application Definitions --------------------*/

typedef struct
{
    CTAQUEUEHD  ctaqueuehd;   /* Natural Access queue handle                 */
    CTAHD       inctahd;      /* Incoming call Natural Access context handle */
    CTAHD       outctahd;     /* Outgoing call Natural Access context handle */

} MYCONTEXT;

/*----------------------------- Application Data ---------------------------*/


static  MYCONTEXT        MyContext = {0};

unsigned   in_swtch          = 0;
unsigned   out_swtch         = 1;
unsigned   in_board          = 0;
unsigned   out_board         = 1;
unsigned   in_slot           = 0;
unsigned   out_slot          = 0;
char      *incoming_protocol = "lps0";
char      *outgoing_protocol = "lps0";
char	  *inctxname		 = "INCOMINGCONTEXT";
char      *outctxname		 = "OUTGOINGCONTEXT";

/* Prototypes */
int InteractWithCaller(CTAHD ctahd, char *digits);


void PrintHelp( void )
{
    printf( "Usage: brd2brd [opts]\n" );
    printf( "  where opts are:\n" );
    printf( "  -A adi_mgr       Natural Access ADI manager to use.               "
            "Default = %s\n", DemoGetADIManager() );
    printf( "  -b n             OAM board number for in bound call. "
            "Default = %d\n", in_board );
    printf( "  -B n             OAM board number for outbound call. "
            "Default = %d\n", out_board );
    printf( "  -x n             PPX switch number for inbound call.   "
            "Default = %d\n", in_swtch );
    printf( "  -X n             PPX switch number for outbound call.  "
            "Default = %d\n", out_swtch );
    printf( "  -s n             Incoming DSP timeslot                 "
            "Default = %d\n", in_slot );
    printf( "  -S n             Outgoing DSP timeslot                 "
            "Default = %d \n", out_slot );
    printf( "  -i protocol      Incoming Protocol to run.             "
            "Default = %s\n", incoming_protocol );
    printf( "  -o protocol      Outgoing Protocol to run.             "
            "Default = %s\n", outgoing_protocol );
    printf( "  -F filename      Natural Access Configuration File name.    "
            "Default = None\n" );

  }


/*-----------------------------------------------------------------------
                           ProcessArgs()

Process command line options.
-----------------------------------------------------------------------*/
void ProcessArgs( int argc, char **argv )
{
    int         c;

    while( ( c = getopt( argc, argv, "A:B:b:F:X:x:S:s:i:o:n:m:M:Hh?" ) ) != -1 )
    {
        switch( c )
        {
        case 'A':
            DemoSetADIManager( optarg );
            break;
        case 'b':
            sscanf( optarg, "%d", &in_board );
            break;
        case 'B':
            sscanf( optarg, "%d", &out_board );
            break;
        case 'F':
            DemoSetCfgFile( optarg );
            break;
        case 'x':
            sscanf( optarg, "%d", &in_swtch );
            break;
        case 'X':
            sscanf( optarg, "%d", &out_swtch );
            break;
        case 's':
            sscanf( optarg, "%d", &in_slot );
            break;
        case 'S':
            sscanf( optarg, "%d", &out_slot );
            break;
        case 'i':
            incoming_protocol = optarg;
            break;
        case 'o':
            outgoing_protocol = optarg;
            break;


        case 'h':
        case 'H':
        case '?':
        default:
            PrintHelp();
            exit( 1 );
            break;
        }
    }
    if (optind < argc)
    {
        printf( "Invalid option.  Use -h for help.\n" );
        exit( 1 );
    }
}

/*-------------------------------------------------------------------------
                                GetExtension

  Play prompt, and get digits from user.
-------------------------------------------------------------------------*/
int GetExtension( CTAHD ctahd, int count, char *digits )
{
    int ret;
    int i;
    int maxtries = 2;

    printf( "\tGetting extension...\n");
    for( i = 0; i < maxtries; i++ )
    {
        if( DemoPlayMessage( ctahd, "ctademo", PROMPT_GETEXT,
                             ADI_ENCODE_NMS_24, NULL ) !=
            SUCCESS )
            return FAILURE;

        printf( "\tCollecting digits to dial...\n" );
        if( ( ret = DemoGetDigits( ctahd, count, digits ) ) == SUCCESS )
            return SUCCESS;

        if( ret == DISCONNECT )
            return ret;
    }

    if( DemoPlayMessage( ctahd, "ctademo", PROMPT_GOODBYE,
                         ADI_ENCODE_NMS_24, NULL ) ==
        DISCONNECT )
        return DISCONNECT;

    return -1;
}
/*-----------------------------------------------------------------------
                      InteractWithCaller

Greet caller.
-----------------------------------------------------------------------*/
int InteractWithCaller(CTAHD ctahd, char *digits)
{
    int ret;

    /* Play message to the caller. */
    adiFlushDigitQueue( ctahd );
    ret = DemoPlayMessage(ctahd, "ctademo", PROMPT_WELCOME,
                          ADI_ENCODE_NMS_24, NULL);
    if (ret != SUCCESS )
    {
        return ret;
    }

    /* Prompt the caller for the extension to try.  This
     * subroutine will return the digits.
     */
    ret = GetExtension( ctahd, 3, digits );
    if (ret != SUCCESS)
    {
        return ret;
    }

    printf("\tGot extension: %s\n", digits);

    /* Tell the caller that we're going to try making the call.
     * "I'll try that extension now."
     */
    ret = DemoPlayMessage(ctahd, "ctademo", PROMPT_WILLTRY,
                          ADI_ENCODE_NMS_24, NULL );
    if (ret != SUCCESS )
    {
        return ret;
    }

    return SUCCESS;
}
/*-----------------------------------------------------------------------
                           SetupInOutConx

Create/Break  a QUAD/DUPLEX connections for the local line interfaces.
-----------------------------------------------------------------------*/
void SetupInOutConx( CTAHD ctahd, DWORD swi, DWORD stream, DWORD timeslot,
                     int makeconn, PPX_CX_MODE mode )
{
    PPX_MVIP_ADDR listener;
    PPX_MVIP_ADDR talker;

    listener.switch_number = swi ;
    listener.bus = MVIP95_LOCAL_BUS;
    listener.stream = 1 + stream;     /* DSP input */
    listener.timeslot = timeslot;

    talker.switch_number = swi;
    talker.bus = MVIP95_LOCAL_BUS;
    talker.stream = 0;                /* line output */
    talker.timeslot = timeslot;

    if ( makeconn )
    {
        ppxConnect( ctahd, &talker, &listener, mode );
    }
    else
    {
        /* Break the voice connections but maintain the signal connections. */
        ppxDisconnect( ctahd, &talker, &listener, mode );
    }

    return ;
}

/*-----------------------------------------------------------------------
                           VoiceConx()

Connect or Disconnect DUPLEX streams between the voice lines of
the two ports.
-----------------------------------------------------------------------*/
void VoiceConx( CTAHD ctahd, int makeconn )
{
    PPX_MVIP_ADDR listener;
    PPX_MVIP_ADDR talker;

    listener.switch_number = out_swtch ;
    listener.bus = MVIP95_LOCAL_BUS;
    listener.stream = 1;               /* line input */
    listener.timeslot = out_slot ;

    talker.switch_number = in_swtch ;
    talker.bus = MVIP95_LOCAL_BUS;
    talker.stream = 0;                 /* line output */
    talker.timeslot = in_slot;

    if ( makeconn )
    {
        ppxConnect( ctahd, &talker, &listener, PPX_DUPLEX);
    }
    else
    {
        ppxDisconnect( ctahd, &talker, &listener, PPX_DUPLEX);
    }
    return;
}


/*-----------------------------------------------------------------------
                           RunDemo()

  Open a ctahd, start protocol, and loop answering incoming calls.
  The needed parameters are taken from globals.
-----------------------------------------------------------------------*/
void RunDemo( void )
{
    MYCONTEXT       *cx = &MyContext;
    char             called_digits[20];
    char             calling_digits[20];
    char             digitBuf[256];
    CTA_EVENT        event;
    ADI_CONTEXT_INFO info;
    DWORD            in_stream;
    DWORD            out_stream;
    int              bConnected = FALSE;

    /* for ctaOpenServices */
    CTA_SERVICE_DESC services[] =
    {   { {"ADI", ""      }, { 0 }, { 0 }, { 0 } },
        { {"VCE", "VCEMGR"}, { 0 }, { 0 }, { 0 } },
        { {"PPX", "PPXMGR"}, { 0 }, { 0 }, { 0 } }
    };

    /* Get Correct ADI Service Manager */
    services[0].name.svcmgrname = DemoGetADIManager();

    /* Fill in ADI service MVIP address information for
     * incoming connection
     */
    services[0].mvipaddr.board    = in_board;
    services[0].mvipaddr.stream   = 0;
    services[0].mvipaddr.timeslot = in_slot;
    services[0].mvipaddr.mode     = ADI_FULL_DUPLEX;

    DemoOpenPort( 0, inctxname, services,
                  sizeof(services)/sizeof(services[0]),
                  &(cx->ctaqueuehd), &(cx->inctahd) );

    /* Get the DSP's Mvip-95 stream number */
    adiGetContextInfo( cx->inctahd, &info, sizeof info);
    in_stream = info.stream95;

    /*
     * Address information for outgoing connection differs in timeslot and
     * and board number
     */
    services[0].mvipaddr.timeslot = out_slot;
    services[0].mvipaddr.board    = out_board;

    DemoOpenAnotherPort( 0, outctxname, services,
                         sizeof(services)/sizeof(services[0]),
                         cx->ctaqueuehd, &(cx->outctahd) );

    adiGetContextInfo( cx->outctahd, &info, sizeof info);
    out_stream = info.stream95;

    /*
     * Set up  QUAD connections on both the incoming and
     * outgoing ports, connection the line interfaces to the
     * local DSP on the board.
     */
    SetupInOutConx( cx->inctahd, in_swtch, in_stream, in_slot,
                         TRUE, PPX_QUAD );
    SetupInOutConx( cx->outctahd, out_swtch, out_stream, out_slot,
                         TRUE, PPX_QUAD );

    /* Loop forever, receiving calls and processing them.
     * If any function returns a failure or disconnect,
     * simply hang up and wait for the next call.
     */

    for(;;)
    {
         /* Initiate a protocol on the call resource so we can accept calls.
         */
        DemoStartProtocol( cx->inctahd, incoming_protocol, NULL, NULL );

        DemoWaitForCall( cx->inctahd, called_digits, calling_digits );

        if ( DemoAnswerCall( cx->inctahd, 1 /* #rings */ ) != SUCCESS )
            goto hangup;

        if (InteractWithCaller(cx->inctahd, digitBuf) != SUCCESS)
            goto hangup;

        /* Initiate a protocol on the call resource so we can accept calls.
         */
        DemoStartProtocol( cx->outctahd, outgoing_protocol, NULL, NULL );


        /* Place second call on the line. */
        if ( DemoPlaceCall( cx->outctahd, digitBuf, NULL ) != SUCCESS )
        {
            goto hangup;
        }

        /*
         * Disconnect the line interfaces from the local DSPs, but maintain
         * the signal connections.
         */

        SetupInOutConx( cx->inctahd, in_swtch, in_stream, in_slot,
                        FALSE, PPX_DUPLEX );
        SetupInOutConx( cx->outctahd, out_swtch, out_stream, out_slot,
                        FALSE, PPX_DUPLEX );

        /*
         * Connect the line interfaces bewteen the two boards.
         */

        VoiceConx( cx->inctahd, TRUE );
        bConnected = TRUE;

        for (;;)
        {
            DWORD ctaret;

            ctaret = ctaWaitEvent( cx->ctaqueuehd, &event, CTA_WAIT_FOREVER);

            if (ctaret != SUCCESS)
            {
                printf( "\007ctaWaitEvent returned %x\n", ctaret);
                goto hangup;
            }

            switch ( event.id )
            {
            case ADIEVN_CALL_DISCONNECTED:
                
                /* Shut down the previously established voice connection */
                VoiceConx( cx->inctahd, FALSE );
                goto hangup;

            default:
                break;
            }
        }

    hangup:
        DemoHangUp( cx->inctahd );
        DemoHangUp( cx->outctahd );

        /* Stop protocol on ctahd */
        DemoStopProtocol( cx->inctahd );
        DemoStopProtocol( cx->outctahd );

        if (bConnected)
        {
            /* Reconnect the DSPs. */
            /* Signaling connections already established, just connect voice */
            SetupInOutConx( cx->inctahd, in_swtch, in_stream, in_slot,
                            TRUE, PPX_DUPLEX );
            SetupInOutConx( cx->outctahd, out_swtch, out_stream, out_slot,
                            TRUE, PPX_DUPLEX );

            bConnected = FALSE;
        }

    }
  UNREACHED_STATEMENT( ctaDestroyQueue( cx->ctaqueuehd ); )
  UNREACHED_STATEMENT( return; )
}


/*-----------------------------------------------------------------------
                           main()

-----------------------------------------------------------------------*/
int main( int argc, char **argv )
{
    CTA_SERVICE_NAME servicelist[] =     /* for ctaInitialize */
    {   { "ADI", ""       },
        { "VCE", "VCEMGR" },
        { "PPX", "PPXMGR" }
    };

    printf( "Natural Access PPX Board-to-Board Switching Demo V.%s (%s)\n",
            VERSION_NUM, VERSION_DATE );

    ProcessArgs( argc, argv );

    /* Initialize ADI Service Manager name */
    servicelist[0].svcmgrname = DemoGetADIManager();

    printf( "\tBoard for incoming call (operator)       = %d\n", in_board );
    printf( "\tBoard for outgoing call (customer)       = %d\n", out_board );
    printf( "\tPPX switch number for incoming call      = %d\n", in_swtch );
    printf( "\tPPX switch number for outgoing call      = %d\n", out_swtch );

    printf( "\tDSP Incoming TimeSlot, Outgoing TimeSlot = %d,%d\n",
             in_slot, out_slot);
    printf( "\tIncoming Protocol                        = %s\n",
            incoming_protocol );
    printf( "\tOutgoing Protocol                        = %s\n",
            outgoing_protocol );
    printf( "\n" );


    /* Register an error handler to display errors returned from
     * API functions; no need to check return values in the demo.
     */
    ctaSetErrorHandler( DemoErrorHandler, NULL );

    /* initialize Natural Access services for this process */
    DemoInitialize( servicelist, sizeof(servicelist)/sizeof(servicelist[0]) );

    RunDemo();

    UNREACHED_STATEMENT( return 0; )
}
