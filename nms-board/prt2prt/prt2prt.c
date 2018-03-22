/*****************************************************************************
 *  FILE: prt2prt.c
 *
 *  DESCRIPTION:  This program demonstrates the transfer of a call received
 *                over an incoming line to an outgoing call placed over
 *                an outgoing line.  A separate DSP resource is used for each.
 *
 *  Prompts Used from CTADEMO.VOX:
 *                GETEXT     - prompts for extension.
 *                GOODBYE    - Goodbye message.
 *                WELCOME    - Welcome message.
 *                WILLTRY    - Will try extension message.
 *
 * Copyright (c) 1996-2002 NMS Communications.  All rights reserved.
 *****************************************************************************/
#define VERSION_NUM  "5"
#define VERSION_DATE __DATE__

#include "ctademo.h"


/*----------------------------- Application Definitions --------------------*/
SWIHD     swihd = 0;

typedef struct
{
    CTAQUEUEHD  ctaqueuehd;       /* Natural Access queue handle                 */
    CTAHD       inctahd;          /* Incoming call Natural Access context handle */
    CTAHD       outctahd;         /* Outgoing call Natural Access context handle */
    VCEHD       vh;               /* voice handle for play/record           */
    unsigned    lowlevelflag;     /* flag to get low-level events           */
    unsigned    id;               /* thread number */
} MYCONTEXT;

/*----------------------------- Application Data ---------------------------*/
static MYCONTEXT MyContext = {0};

/* Prototypes */
int InteractWithCaller(CTAHD ctahd, char *digits);
DWORD RunDemo(void);

char       *driver_name = "agsw";
unsigned   ag_board    = 0;
unsigned   incoming_slot   = 0;
unsigned   outgoing_slot   = 2;
char      *incoming_protocol    = "lps0";
char      *outgoing_protocol    = "lps0";
char      *inctxname            = "INCOMINGCONTEXT";
char      *outctxname           = "OUTGOINGCONTEXT";

void PrintHelp( void )
{
    printf( "Usage: prt2prt [opts]\n" );
    printf( "  where opts are:\n" );
    printf( "  -A adi_mgr       Natural Access manager to use for ADI service.         "
            "Default = %s\n", DemoGetADIManager() );
    printf( "  -b n             OAM board number.                           "
            "Default = %d\n", ag_board );
    printf( "  -d driver_name   Name of the driver/dll.                     "
            "Default = %s\n", driver_name );
    printf( "  -s i,o           Incoming and outgoing DSP timeslots.        "
            "Default = %d,%d\n", incoming_slot, outgoing_slot);
    printf( "  -i protocol      Incoming Protocol to run.                   "
            "Default = %s\n", incoming_protocol );
    printf( "  -o protocol      Outgoing Protocol to run.                   "
            "Default = %s\n", outgoing_protocol );
    printf( "  -F filename      Natural Access Configuration File name.          "
            "Default = None\n" );

}


/* Process command line options.
 */
void ProcessArgs( int argc, char **argv )
{
    int c;

    while( ( c = getopt( argc, argv, "A:F:b:d:s:i:o:m:M:Hh?" ) ) != -1 )
    {
        switch( c )
        {
            case 'A':
                DemoSetADIManager( optarg );
                break;
            case 'F':
                DemoSetCfgFile( optarg );
                break;
            case 'b':
                sscanf( optarg, "%d", &ag_board );
                break;
            case 'd':
                driver_name = optarg;
                break;
            case 's':
                if (sscanf( optarg, "%d,%d", &incoming_slot, &outgoing_slot) != 2)
                {
                    PrintHelp();
                    exit( 1 );
                }
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
  My Switching Functions

  -------------------------------------------------------------------------*/
void MyErrorHandler(SWIHD swihd, char *text, DWORD e)
{
    DWORD status, errorcode;

    printf("Error (%d): %s\n", e, text);
    status = swiGetLastError(swihd, &errorcode);
    if (status != SUCCESS)
        return;
    else
        printf("MVIP Device Error: %d\n", errorcode);
}

#define SIMPLEX 0
#define DUPLEX  1
#define QUAD    2
int MyMakeConnection(SWIHD swihd, SWI_TERMINUS input,
                     SWI_TERMINUS output,
                     unsigned count, DWORD mode)
{
    unsigned i;
    DWORD status = SUCCESS;
    DWORD duplex = 0;
    DWORD quad = 0;
    SWI_TERMINUS *outputs = NULL, *inputs = NULL;

    if (mode == DUPLEX)
        duplex = 1;

    if (mode == QUAD)
    {
        duplex = 1;
        quad = 1;
    }

    if ((inputs = (SWI_TERMINUS *)malloc(sizeof(SWI_TERMINUS)*count)) == NULL)
    {
        fprintf(stderr, "Out of memory\n");
        status = CTAERR_OUT_OF_MEMORY;
    }
    else if ((outputs = (SWI_TERMINUS *)malloc(sizeof(SWI_TERMINUS)*count))
             == NULL )
    {
        fprintf(stderr, "Out of memory\n");
        free( inputs );
        status = CTAERR_OUT_OF_MEMORY;
    }

    if ( status != SUCCESS )
        return status;

    for (i = 0; i < count; i++)
    {
        inputs[i].bus = input.bus;
        inputs[i].stream = input.stream;
        inputs[i].timeslot = input.timeslot + i;

        outputs[i].bus = output.bus;
        outputs[i].stream = output.stream;
        outputs[i].timeslot = output.timeslot + i;
    }
    status  = swiMakeConnection(swihd, inputs, outputs, count);
    if (status != SUCCESS)
    {
        MyErrorHandler(swihd, "MakeConnection", status);
    }
    else if( duplex )
    {
        for (i = 0; i < count; i++)
        {
            inputs[i].stream = inputs[i].stream + 1;
            outputs[i].stream = outputs[i].stream - 1;
        }
        status = swiMakeConnection(swihd, outputs, inputs, count);
        if (status != SUCCESS)
        {
            MyErrorHandler(swihd, "MakeConnection", status);
        }
    }

    if( quad && ( status == SUCCESS ) )
    {
        for (i = 0; i < count; i++)
        {
            inputs[i].stream = inputs[i].stream + 1;
            outputs[i].stream = outputs[i].stream + 3;
        }
        status = swiMakeConnection(swihd, inputs, outputs, count);
        if (status != SUCCESS)
        {
            MyErrorHandler(swihd, "MakeConnection", status);
        }

        for (i = 0; i < count; i++)
        {
            inputs[i].stream = inputs[i].stream + 1;
            outputs[i].stream = outputs[i].stream - 1;
        }

        status = swiMakeConnection(swihd, outputs, inputs, count);
        if (status != SUCCESS)
        {
            MyErrorHandler(swihd, "MakeConnection", status);
        }
    }

    free( inputs );
    free( outputs );

    return status;
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

DWORD SetupIncomingConx(SWIHD swihd, unsigned dsp_stream)
{
    SWI_TERMINUS dsp, hybrid;
    DWORD e;

    /* Connect Local voice and signalling streams */
    dsp.bus = MVIP95_LOCAL_BUS;
    dsp.stream = dsp_stream;
    dsp.timeslot = incoming_slot;

    hybrid.bus = MVIP95_LOCAL_BUS;
    /*
     * Hybrids are on streams 0,1, 2,3:
     * The voice stream input to the switch is on stream 0, and the voice
     * stream output is on stream 1. So we give the base of the voice streams
     * of the dsp_stream and 1 to the MakeConnection function and it
     * will make the quad connection.
     */
    hybrid.stream = 1;
    hybrid.timeslot = incoming_slot;

    e = MyMakeConnection(swihd, dsp, hybrid, 1, QUAD);
    if (e != SUCCESS)
        return e;

    return SUCCESS;
}

DWORD SetupOutgoingConx(SWIHD swihd, unsigned dsp_stream)
{
    SWI_TERMINUS dsp, hybrid;
    DWORD e;

    /* Connect voice and signalling streams for outgoing call */
    dsp.bus = MVIP95_LOCAL_BUS;
    dsp.stream = dsp_stream;
    dsp.timeslot = outgoing_slot;

    hybrid.bus = MVIP95_LOCAL_BUS;
    /*
     * Hybrids are on streams 0,1,2,3:
     * The voice stream input to the switch is on stream 0, and the
     * voice stream output is on stream 1. So we give the base of the
     * dsp voice stream and 1 to the MakeConnection function and it will
     * make the quad connection.
     */
    hybrid.stream = 1;
    hybrid.timeslot = outgoing_slot;

    e = MyMakeConnection(swihd, dsp, hybrid, 1, QUAD);
    if (e != SUCCESS)
        return e;

    return SUCCESS;
}

DWORD MakeVoiceConx(SWIHD swihd)
{
    SWI_TERMINUS incoming, outgoing;
    DWORD e;

    /*
     * Hybrids are on streams 0,1,2,3. We want to connect the voice in to the
     * voice out, duplex. That is, we want to connect streams 0,1, DUPLEX
     */
    incoming.bus = MVIP95_LOCAL_BUS;
    incoming.stream = 0;
    incoming.timeslot = incoming_slot;

    outgoing.bus = MVIP95_LOCAL_BUS;
    outgoing.stream = 1;
    outgoing.timeslot = outgoing_slot;

    e = MyMakeConnection(swihd, incoming, outgoing, 1, DUPLEX);
    if (e != SUCCESS)
        return e;

    return SUCCESS;
}

DWORD BreakVoiceConx(SWIHD swihd)
{
    SWI_TERMINUS outgoing;

    /*
     * We want to disable output to the voice streams (1) that were
     * previously connected in DUPLEX mode.
     */

    outgoing.bus = MVIP95_LOCAL_BUS;
    outgoing.stream = 1;
    outgoing.timeslot = outgoing_slot;

    swiDisableOutput(swihd, &outgoing, 1);

    outgoing.timeslot = incoming_slot;

    swiDisableOutput(swihd, &outgoing, 1);

    return SUCCESS;
}

/* Open a ctahd, start protocol, and loop answering incoming calls.
 * The needed parameters are taken from globals, except for the
 * MVIP timeslot, which is passed in as the argument.
 */
DWORD RunDemo()
{
    MYCONTEXT      *cx = &MyContext;
    char            called_digits[20];
    char            calling_digits[20];
    int             ncalls = 0;
    char            digitBuf[256];
    CTA_EVENT       event;
    unsigned        dsp_stream = 0;
    DWORD e;

    /* for ctaOpenServices */
    CTA_SERVICE_DESC services[] =
    {   { {"ADI", ""      }, { 0 }, { 0 }, { 0 } },
        { {"SWI", "SWIMGR"}, { 0 }, { 0 }, { 0 } },
        { {"VCE", "VCEMGR"}, { 0 }, { 0 }, { 0 } }
    };

    /* Get Correct ADI Service Manager */
    services[0].name.svcmgrname = DemoGetADIManager();

    /* Fill in ADI service MVIP address information for incoming connection */
    services[0].mvipaddr.board    = ag_board;
    /* 0 based DSP numbers are ok because ADI takes that, or correct MVIP
       stream is passed here */
    services[0].mvipaddr.stream   = 0;
    services[0].mvipaddr.timeslot = incoming_slot;
    services[0].mvipaddr.mode     = ADI_FULL_DUPLEX;

    /* Initialize Natural Access, open a call resource, open default services */
    DemoOpenPort( 0, inctxname, services,
                  sizeof(services)/sizeof(services[0]),
                  &(cx->ctaqueuehd), &(cx->inctahd) );

    /* address information for outgoing connection differs only in timeslot */
    services[0].mvipaddr.timeslot = outgoing_slot;

    DemoOpenAnotherPort( 1, outctxname, services, ARRAY_DIM(services),
                         cx->ctaqueuehd, &(cx->outctahd) );

    /*
     * It doesn't matter which ctahd the switch is opened on because there
     * is only one switch handle we are opening on the board.
     */
    e = swiOpenSwitch(cx->inctahd,
                      driver_name,
                      ag_board,
                      SWI_ENABLE_RESTORE,
                      &swihd);
    if (e != SUCCESS)
        return e;

    /* Loop forever, receiving calls and processing them.
     * If any function returns a failure or disconnect,
     * simply hang up and wait for the next call.
     */
    for(;;)
    {
        ADI_CONTEXT_INFO context_info;

        e = adiGetContextInfo(cx->inctahd,
                              &context_info,
                              sizeof(ADI_CONTEXT_INFO));
        if (e != SUCCESS)
        {
            swiCloseSwitch(swihd);
            return e;
        }

        e = SetupIncomingConx(swihd, context_info.stream95);
        if (e != SUCCESS)
        {
            swiCloseSwitch(swihd);
            return e;
        }

        e = adiGetContextInfo(cx->outctahd,
                              &context_info,
                              sizeof(ADI_CONTEXT_INFO));
        if (e != SUCCESS)
        {
            swiCloseSwitch(swihd);
            return e;
        }

        e = SetupOutgoingConx(swihd, context_info.stream95);
        if (e != SUCCESS)
        {
            swiCloseSwitch(swihd);
            return e;
        }

        /* Initiate a protocol on the call resource so we can accept calls.
         */
        if (cx->lowlevelflag)
        {
            ADI_START_PARMS stparms;

            ctaGetParms( cx->inctahd,    /* get call resource parameters    */
                         ADI_START_PARMID,
                         &stparms,
                         sizeof(stparms) );

            stparms.callctl.eventmask = 0xffff;  /* show low-level evs */

            DemoStartProtocol( cx->inctahd, incoming_protocol, NULL, &stparms );
        }
        else
        {
            DemoStartProtocol( cx->inctahd, incoming_protocol, NULL, NULL );
        }

        DemoWaitForCall( cx->inctahd, called_digits, calling_digits );

        if ( DemoAnswerCall( cx->inctahd, 1 /* #rings */ ) != SUCCESS )
            goto hangup;

        if (InteractWithCaller(cx->inctahd, digitBuf) != SUCCESS)
            goto hangup;

        /* Initiate a protocol on the call resource so we can accept calls.
         */
        if (cx->lowlevelflag)
        {
            ADI_START_PARMS stparms;

            ctaGetParms( cx->outctahd,    /* get call resource parameters    */
                         ADI_START_PARMID,
                         &stparms,
                         sizeof(stparms) );

            stparms.callctl.eventmask = 0xffff;  /* show low-level evs */

            DemoStartProtocol( cx->outctahd, outgoing_protocol,
                               NULL, &stparms );
        }
        else
        {
            DemoStartProtocol( cx->outctahd, outgoing_protocol, NULL, NULL );
        }

        /* Place second call on the line. */
        if ( DemoPlaceCall( cx->outctahd, digitBuf, NULL ) != SUCCESS )
        {
            goto hangup;
        }

        e = MakeVoiceConx(swihd);
        if (e != SUCCESS)
            return e;

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

                    goto hangup;

                default:
                    break;
            }
        }

hangup:
        DemoHangUp( cx->inctahd );
        DemoHangUp( cx->outctahd );

        BreakVoiceConx(swihd);

        /* Stop protocol on ctahd */
        DemoStopProtocol( cx->inctahd );

        /* Stop protocol on ctahd */
        DemoStopProtocol( cx->outctahd );

    }
    UNREACHED_STATEMENT( swiCloseSwitch(swihd); )
        UNREACHED_STATEMENT( ctaDestroyQueue( cx->ctaqueuehd ); )
        UNREACHED_STATEMENT( return SUCCESS; )
}

int main( int argc, char **argv )
{
    CTA_SERVICE_NAME servicelist[] =     /* for ctaInitialize */
    {   { "ADI", ""       },
        { "SWI", "SWIMGR" },
        { "VCE", "VCEMGR" },
    };

    unsigned ret;

    printf( "Natural Access Port to Port Calling Demo V.%s (%s)\n",
            VERSION_NUM, VERSION_DATE );

    ProcessArgs( argc, argv );

    /* Initialize ADI Service Manager name */
    servicelist[0].svcmgrname = DemoGetADIManager();

    printf( "\tBoard                                    = %d\n", ag_board );
    printf( "\tInSlot, OutSlot                          = %d,%d\n",
            incoming_slot, outgoing_slot);
    printf( "\tIncoming Protocol                        = %s\n",
            incoming_protocol );
    printf( "\tOutgoing Protocol                        = %s\n",
            outgoing_protocol );
    printf( "\n" );

    /* Register an error handler to display errors returned from API functions;
     * no need to check return values in the demo.
     */
    ctaSetErrorHandler( DemoErrorHandler, NULL );

    /* initialize Natural Access services for this process */
    DemoInitialize( servicelist, sizeof(servicelist)/sizeof(servicelist[0]) );

    if (ret = RunDemo() != SUCCESS)
    {
        printf("Could not run demo: %d\n", ret);
        exit(-1);
    }

    /* Sleep forever. */
    for(;;)
        DemoSleep( 1000 );

    UNREACHED_STATEMENT( return 0; )
}
