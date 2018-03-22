/*****************************************************************************
 *  FILE: c61t2tIvr.c  
 *           Test of CG6100 board using small 3rd pool of IVR resources to received
 *           a call, play a prompt for the call to dial and then place and outgoing
 *           call on a second trunk.  This is demo is switched in a way that will allow
 *           the user, with some modification, do a follow-on call.
 *           
 *
 *  DESCRIPTION:  This program demonstrates the transfer of a single call received
 *                over an incoming line to an outgoing call placed over
 *                an outgoing line, getting the number to use for placing the call
 *                from a third DSP running IVR/echo resources.  Separate DSPs 
 *                are used for the protocol resources for each part of the call.
 *                The length of the phone number it expects is 7 digits which can
 *                modified by passing it to the command line information.
 *
 *    This test was tested using trunk 0 as the incoming trunk and trunk 10 as
 *    the outgoing trunk.  A cg6000 board was used to place a call to trunk 0
 *    on the cg6100 board and received a call from trunk 10 of the cg6100 board.
 *    Since the CG6100 associates timeslot with pools, timeslot 0, pool 0 resources,
 *    was used for the trunk 0 connection.  Timeslot 256, pool 1 resources,
 *    was used for the outgoing trunk 10 connection.  Timeslot 496, pool 2,
 *    was used to get to the IVR resources.  These hardwired timeslots, trunks
 *    and protocols can be overridden by using the command line. 
 *
 *  Prompts Used from CTADEMO.VOX:
 *                GETEXT     - prompts for extension.
 *
 * NMS Communications.
 *****************************************************************************/
#define VERSION_NUM  "5"
#define VERSION_DATE __DATE__

#include <stdio.h>
#include <memory.h>
#include "ctademo.h"

/* -------------- Application Macros -----------*/
/*Used for switching connections. */
#define SIMPLEX 0
#define DUPLEX  1
#define QUAD    2

#define OUTPUT_MSGS 1

/*----------------------------- Application Definitions --------------------*/
SWIHD     swihd = 0;

typedef struct
{
    CTAQUEUEHD  ctaqueuehd;       /* Natural Access queue handle             */
    CTAHD       Incomingctahd;    /* PSTN Incoming call Natural Access context handle */
    CTAHD       IvrInctahd;       /* IVR incoming call side Natural Access context handle*/
    CTAHD       Outgoingctahd;    /* PSTN Outgoing call Natural Access context handle */
    unsigned    lowlevelflag;     /* flag to get low-level events           */
    unsigned    id;               /* thread number */
} MYCONTEXT;

/*----------------------------- Application Data ---------------------------*/
static MYCONTEXT MyContext = {0};

/* Prototypes */
int InteractWithCaller(CTAHD ctahd, char *digits);
DWORD RunDemo(void);

char       *driver_name            = "agsw";
unsigned   ag_board                = 0;


unsigned   LogicalDSPincoming_slot = 0;  // incomving slot
unsigned   Voiceincoming_slot      = 496; //  IVR SLOT
unsigned   LogicalDSPoutgoing_slot = 256; //  out going slot

unsigned   inbound_trunk           = 0;  /* 0, 1, 2, 3 - first trunk */
unsigned   outbound_trunk          = 10; /* 40, 41, 42, 43 - eleventh trunk */
unsigned   in_trunk_slot           = 0;
unsigned   out_trunk_slot          = 0;

char       *PSTNincoming_protocol  = "mfc0";  
char       *PSTNoutgoing_protocol  = "mfc0";  
char       *IVR_protocol           = "nocc";  // protocol for IVR resources
char       *LogicalDSPinctxname    = "INCOMINGPSTNCONTEXT";  
char       *ivrinctxname           = "INCOMINGIVRCONTEXT";   
char       *LogicalDSPoutctxname   = "OUTGOINGPSTNCONTEXT";  
int        Number_of_Digits        = 7;  // default number of digits to waitfor.

/* macros to convert zero-based trunk number to MVIP95 stream */
#define TRUNK_VOICE_IN(t)  (t*4)
#define TRUNK_VOICE_OUT(t) (t*4+1)
#define TRUNK_SIGNAL_IN(t)  (t*4+2)
#define TRUNK_SIGNAL_OUT(t) (t*4+3)


/*****************************************************/

void PrintHelp( void )
{
    printf( "Usage: prt2prt [opts]\n" );
    printf( "  where opts are:\n" );
    printf( "  -A adi_mgr         Natural Access manager to use for ADI service.        "
            "Default = %s\n", DemoGetADIManager() );
    printf( "  -b n               OAM board number.                          "
            "Default = %d\n", ag_board );
    printf( "  -d driver_name     Name of the driver/dll.                    "
            "Default = %s\n", driver_name );
    printf( "  -s i,o             Incoming and outgoing DSP timeslots.       "
            "Default = %d,%d\n", LogicalDSPincoming_slot, LogicalDSPoutgoing_slot);
    printf( "  -i protocol        Incoming Protocol to run.                  "
            "Default = %s\n", PSTNincoming_protocol );
    printf( "  -o protocol        Outgoing Protocol to run.                  "
            "Default = %s\n", PSTNoutgoing_protocol );
    printf( "  -F filename        Natural Access Configuration File name.         "
            "Default = None\n" );
    printf( "  -t inbound_trunk   Inbound trunk to use.                      "
            "Default = %d\n", inbound_trunk);
    printf( "  -T outbound_trunk  Outbound trunk to use.                     "
            "Default = %d\n", outbound_trunk);
    printf( "  -n Number_of_Digits  Number of digits to get from user.       "
            "Default = %d\n", Number_of_Digits);
    printf( "  -N Number_of_Digits  Number of digits to get from user.       "
            "Default = %d\n", Number_of_Digits);

}


/********************************************************
 Process command line options.
 There are defaults set up in this file, but the user
 can override the defaults with command line parameters.
 ********************************************************/
void ProcessArgs( int argc, char **argv )
{
    int c;

    while( ( c = getopt( argc, argv, "A:F:b:d:s:i:o:I:O:T:t:V:m:M:n:N:Hh?" ) ) != -1 )
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
                if (sscanf( optarg, "%d,%d", &LogicalDSPincoming_slot, &LogicalDSPoutgoing_slot) != 2)
                {
                    PrintHelp();
                    exit( 1 );
                }
                break;
            case 'i':
                PSTNincoming_protocol = optarg;
                break;
            case 'o':
                PSTNoutgoing_protocol = optarg;
                break;

            case 'I':
                LogicalDSPincoming_slot = atoi(optarg);
                break;
            case 'O':
                LogicalDSPoutgoing_slot = atoi(optarg);
                break;
            case 'V':
                Voiceincoming_slot = atoi(optarg);
                break;

            case 't':
                inbound_trunk = atoi(optarg);
                break;
            case 'T':
                outbound_trunk = atoi(optarg);
                break;
            case 'n':
            case 'N':
                Number_of_Digits = atoi(optarg);
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

/*****************************************************
 * Fatal error handler used in myMakeConnection.
 *****************************************************/
void MyErrorHandler(SWIHD swihd, char *text, DWORD e)
{
    DWORD status, errorcode;

    printf("Error (%d): %s\n", e, text);
    status = swiGetLastError(swihd, &errorcode);
    if (status != SUCCESS)
        return;
    else
        printf("MVIP Device Error: %d\n", errorcode);
}  /* MyErrorHandler */

/******************************************************************
 * Non fatal error handler registered with CTAccess to no stop
 ******************************************************************/

DWORD NMSSTDCALL NonFatalErrorHandler( CTAHD ctahd, DWORD errorcode,
                                   char *errortext, char *func )
{
    if ( errorcode == CTAERR_FILE_NOT_FOUND )
        return errorcode;                    /* needs to be handled in app */

    if (ctahd == NULL_CTAHD)
        printf( "\007Error in %s [ctahd=null]: %s (%#x)\n",
                func, errortext, errorcode );
    else
        printf( "\007Error in %s [ctahd=%#x]: %s (%#x)\n",
                func, ctahd, errortext, errorcode );

    /* NOTE that if if you don't end the process within this routing,
       you MUST return the error code that caused this routine to be called.
       Refer to the commented line of code below. */

    /* If the error handler returns, the return code is passed back to
     * the API function, which will return the error code to the caller.
     */
    return errorcode;
}  /* NonFatalErrorHandler */


/*-------------------------------------------------------------------------
  My Switching Functions

  -------------------------------------------------------------------------*/
/****************************************************/

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
        printf("in: stream = %d slot = %d out: stream = %d slot = %d\n",
                inputs[i].stream, inputs[i].timeslot, outputs[i].stream, outputs[i].timeslot);
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
    int ret = -1;
    int i;
    int maxtries = 1;

    printf( "\tPlaying prompt for digits...\n");
    for( i = 0; i < maxtries; i++ )
    {
        if( DemoPlayMessage( ctahd, "ctademo", PROMPT_GETEXT,
                             ADI_ENCODE_NMS_24, NULL ) !=
            SUCCESS )
        {
            ret = FAILURE; 
            break; ;
        }

        printf( "\tcollecting digits to dial...\n" );
        if( ( ret = DemoGetDigits( ctahd, count, digits ) ) == SUCCESS )
        {
            break;
        }

        if( ret == DISCONNECT )
        {
            break; ;
        }
    }

    return ret;
}  /* GetExtension */



/*********************************************************
 * Generic make connection routine.
 *
 *  Input:  swihd - switch handle
 *          incoming_stream -
 *          incoming_slot -  Incoming stream and timeslot
 *          outgoing_stream
 *          outgoing_slot   - Outgoing stream and timeslot
 *          connect_type - SIMPLEX, DUPLEX or QUAD
 *********************************************************/

DWORD MakeConx( SWIHD swihd, 
                 unsigned incoming_stream, 
                 unsigned incoming_slot, 
                 unsigned outgoing_stream,
                 unsigned outgoing_slot,
                 unsigned connect_type)           
{
    SWI_TERMINUS incoming, outgoing;
    DWORD result;

    printf("incom stream %d In slot = %d  out stream = %d out slot = %d\n",
                incoming_stream, incoming_slot, outgoing_stream, outgoing_slot);
    /* Connect Local voice and signalling streams if quad is specified*/
    incoming.bus = MVIP95_LOCAL_BUS;
    incoming.stream = incoming_stream;
    incoming.timeslot = incoming_slot;

    outgoing.bus = MVIP95_LOCAL_BUS;
    outgoing.stream = outgoing_stream;
    outgoing.timeslot = outgoing_slot;

    /*  swihd - handle to switch
        incoming - incoming terminus data
        outgoing - outgoing terminus data
        1 is number of connects to make
        connect_type - SIMPLEX, DUPLEX, QUAD */
    result = MyMakeConnection(swihd, incoming, outgoing, 1, connect_type);
    if (result != SUCCESS)
        return result;

    return SUCCESS;
}  /* MakeConx */


/*********************************************************/

DWORD BreakVoiceConx(SWIHD swihd, unsigned stream, unsigned slot)
{
    SWI_TERMINUS currentconn;

    /*
     * We want to disable output to the voice streams (1) that were
     * previously connected in DUPLEX mode.
     */

    printf("Breakvoiceconx: stream = %d timeslot = %d\n",stream, slot);
    currentconn.bus = MVIP95_LOCAL_BUS;
    currentconn.stream = stream;

    currentconn.timeslot = slot;
    /*  1st param is switch handle, second is pointer to SWI_TERMINUS and the third
        is the number of connections to break. */
    swiDisableOutput(swihd, &currentconn, 1);

    return SUCCESS;
}  /* BreakVoiceConx */

/******************************************************************
 *  DestroyContext is called to return DSP resources on the board.
 *  Since there are less IVR resources on the CG6100 board, 
 *  They are returned as soon as they are no longer needed.
 ******************************************************************/
void DestroyContext( CTAHD ctahndl, CTAQUEUEHD ctaQhndl )
{
    CTA_EVENT event;
    
    ctaDestroyContext( ctahndl );    /* this will close services automatically */

    /* Wait for CTAEVN_DESTROY_CONTEXT_DONE */
    for (;;)
    {
        DWORD ctaret = ctaWaitEvent( ctaQhndl, &event, CTA_WAIT_FOREVER);
        if (ctaret != SUCCESS)
        {
            printf( "\007ctaWaitEvent returned %x\n", ctaret);
            exit( 1 );
        }

        if ( DemoShouldReport( DEMO_REPORT_ALL ) )
        {
            DemoShowEvent( &event );
        }

        if (ctahndl == event.ctahd && event.id == CTAEVN_DESTROY_CONTEXT_DONE)
        {
            break;
        }
        DemoReportUnexpectedEvent( &event );
    }

    /* Stop protocol on IVR ctahd */
    /* DemoStopProtocol( cx->IvrInctahd );  */
} /* end DestoryContext*/

/****************************************************************
 *
 *  ShutdownPort closes the contexts for the input and output 
 *  PSTN part of the call and then destroys the queue.
 *
 ****************************************************************/
void ShutdownPort( MYCONTEXT *context)
{
    CTA_EVENT event;

    /* for "place call" part of port */
    ctaDestroyContext( context->Outgoingctahd );    /* this will close services automatically */

    /* Wait for CTAEVN_DESTROY_CONTEXT_DONE */
    for (;;)
    {
        DWORD ctaret = ctaWaitEvent( context->ctaqueuehd, &event, CTA_WAIT_FOREVER);
        if (ctaret != SUCCESS)
        {
            printf( "\007ctaWaitEvent returned %x\n", ctaret);
            exit( 1 );
        }

        if ( DemoShouldReport( DEMO_REPORT_ALL ) )
        {
            DemoShowEvent( &event );
        }

        if (context->Outgoingctahd == event.ctahd && event.id == CTAEVN_DESTROY_CONTEXT_DONE)
        {
            break;
        }
        DemoReportUnexpectedEvent( &event );
    }


    /* for "place call" part of port */
    ctaDestroyContext( context->Incomingctahd );    /* this will close services automatically */

    /* Wait for CTAEVN_DESTROY_CONTEXT_DONE */
    for (;;)
    {
        DWORD ctaret = ctaWaitEvent( context->ctaqueuehd, &event, CTA_WAIT_FOREVER);
        if (ctaret != SUCCESS)
        {
            printf( "\007ctaWaitEvent returned %x\n", ctaret);
            exit( 1 );
        }

        if ( DemoShouldReport( DEMO_REPORT_ALL ) )
        {
            DemoShowEvent( &event );
        }

        if (context->Incomingctahd == event.ctahd && event.id == CTAEVN_DESTROY_CONTEXT_DONE)
        {
            break;
        }
        DemoReportUnexpectedEvent( &event );
    }
    

    ctaDestroyQueue( context->ctaqueuehd );
    DemoReportComment( "Destroying the Natural Access context and queue...");
}

/******************************************************************
 * Open a ctahd, start protocol and answer an incoming call.
 * The needed parameters are taken from globals that can be
 * set at the command line.  This code is a sample of one call which
 * make use of the IVR resources.
 ******************************************************************/
DWORD RunDemo()
{
    MYCONTEXT      *cx = &MyContext;
    char            called_digits[20];
    char            calling_digits[20];
    int             ncalls = 0;
    char            digitBuf[256];
    CTA_EVENT       event;
    unsigned        dsp_stream = 0;
    ADI_START_PARMS stparms;
    DWORD result    = SUCCESS;  
    /*  The 3 adi context info were set up to make debugging a little
        easier.  If connections are to be broken and reconnected, this
        would eliminate multiple calls to adigetcontextinfo.*/
    ADI_CONTEXT_INFO In_context_info;
    ADI_CONTEXT_INFO Out_context_info;
    ADI_CONTEXT_INFO Vce_context_info;

    /* for ctaOpenServices */
    CTA_SERVICE_DESC services[] =
    {   { {"ADI", ""      }, { 0 }, { 0 }, { 0 } },
        { {"SWI", "SWIMGR"}, { 0 }, { 0 }, { 0 } },
        { {"VCE", "VCEMGR"}, { 0 }, { 0 }, { 0 } }
    };

    memset( digitBuf, 0, sizeof(digitBuf));
    /* Get Correct ADI Service Manager */
    services[0].name.svcmgrname = DemoGetADIManager();

    /* Fill in ADI service MVIP address information for incoming connection */
    services[0].mvipaddr.board    = ag_board;
    /* The DSP stream on the cg6100 board is 64.  ADI knows to make DSP 0 to 64
       because it checks board type. */
    services[0].mvipaddr.stream   = dsp_stream;
    services[0].mvipaddr.timeslot = LogicalDSPincoming_slot;
    services[0].mvipaddr.mode     = ADI_FULL_DUPLEX;

    /* Initialize Natural Access, open two call control resources with the default services */
    DemoOpenPort( 0, LogicalDSPinctxname, services,
                  sizeof(services)/sizeof(services[0]),
                  &(cx->ctaqueuehd), &(cx->Incomingctahd) );

    /* address information for outgoing connection differs only in timeslot */
    services[0].mvipaddr.timeslot = LogicalDSPoutgoing_slot;

    DemoOpenAnotherPort( 2, LogicalDSPoutctxname, services, ARRAY_DIM(services),
                         cx->ctaqueuehd, &(cx->Outgoingctahd) );

    /*
     * It doesn't matter which ctahd the switch is opened on because there
     * is only one switch handle we are opening on the board.
     */
    if (OUTPUT_MSGS)
    {
        printf("Open Switch\n");
    }
    result = swiOpenSwitch(cx->Incomingctahd,
                      driver_name,
                      ag_board,
                      SWI_ENABLE_RESTORE,
                      &swihd);
    if (result != SUCCESS)
        return result;


    if (OUTPUT_MSGS)
    {
        printf("adiGetContextInfo\n");
    }
    result = adiGetContextInfo( cx->Incomingctahd,
                                &In_context_info,
                                sizeof(ADI_CONTEXT_INFO));
    if (result != SUCCESS)
    {
        swiCloseSwitch(swihd);
        return result;
    }

    /* connect input call control dsp to trunk */
    if (OUTPUT_MSGS)
    {
        printf("MakeConx\n");
    }
    result = MakeConx( swihd, 
                       In_context_info.stream95,
                       LogicalDSPincoming_slot,
                       TRUNK_VOICE_OUT(inbound_trunk),  /* voice out stream on trunk */
                       in_trunk_slot,                   /* voice out timeslot */
                       QUAD);
                            
    if (result != SUCCESS)
    {
        swiCloseSwitch(swihd);
        printf( "MakeConx error = 0x%x\n",result);
        return result;
    }


        /*  Get another CAS resource for placing outbound call.*/
    if (OUTPUT_MSGS)
    {
        printf("adiGetContextInfo2\n");
    }
        result = adiGetContextInfo(cx->Outgoingctahd,
                                   &Out_context_info,
                                   sizeof(ADI_CONTEXT_INFO));
    if (result != SUCCESS)
    {
        swiCloseSwitch(swihd);
        return result;
    }

    /* Connect second call control resource to trunk */
    result = MakeConx( swihd,
                       Out_context_info.stream95,
                       LogicalDSPoutgoing_slot,
                       TRUNK_VOICE_OUT(outbound_trunk),     
                       out_trunk_slot,
                       QUAD);
    if (result != SUCCESS)
    {
        swiCloseSwitch(swihd);
        return result;
    }

    /* Start a protocol on the first call control resource to receive a call.
     */
    if (OUTPUT_MSGS)
    {
        printf("ctaGetParms\n");
    }
    ctaGetParms( cx->Incomingctahd,    /* get call resource parameters    */
                 ADI_START_PARMID,
                 &stparms,
                 sizeof(stparms) );

    /* Notice DTMF detector is started here for possible follow on calls */
    stparms.callctl.eventmask = 0xffff;  /* show low-level evs */
    stparms.callctl.mediamask = 0x000f;  /* Turn on DTMF in connected state*/

    if (OUTPUT_MSGS)
    {
        printf("Start Protocol\n");
    }
    DemoStartProtocol( cx->Incomingctahd, PSTNincoming_protocol, NULL, &stparms );

    if (OUTPUT_MSGS)
    {
        printf("WaitForCall\n");
    }
    DemoWaitForCall( cx->Incomingctahd, called_digits, calling_digits );

    /* Don't want to answer call until you know you have IVR resources.
       So only accept call playing ring.
    */
    /* On place call side need to connect on proceeding.*/

    if (OUTPUT_MSGS)
    {
        printf("AcceptCallwith Ring\n");
    }
    if ( adiAcceptCall( cx->Incomingctahd, ADI_ACCEPT_PLAY_RING, NULL) != SUCCESS)
    {
        goto hangup;
    }



    /* address information for IVR connection differs in timeslot and mode, so change timeslot
       and mode and open an IVR resource for playing prompt and getting digits for dialing.  */
    services[0].mvipaddr.timeslot = Voiceincoming_slot;
    services[0].mvipaddr.mode     = ADI_VOICE_DUPLEX;

    if (OUTPUT_MSGS)
    {
        printf("OpenAnotherPort\n");
    }
    DemoOpenAnotherPort( 1, ivrinctxname, services, ARRAY_DIM(services),
                        cx->ctaqueuehd, &(cx->IvrInctahd) );

    if (OUTPUT_MSGS)
    {
        printf("adiGetContextInfo3\n");
    }
    result = adiGetContextInfo(cx->IvrInctahd,
                              &Vce_context_info,
                              sizeof(ADI_CONTEXT_INFO));
    if (result != SUCCESS)
    {
        swiCloseSwitch(swihd);
        return result;
    }

    /*  Now that have IVR resource Answer the call */

    if (OUTPUT_MSGS)
    {
        printf("AnswerCall\n");
    }
    /* The 1 in this parameter list is the number of rings on which to answer */
    if ( DemoAnswerCall( cx->Incomingctahd, 1  ) != SUCCESS )
    {
        goto hangup;
    }

    /* Setup new voice stream on IVR DSP*/
    if (OUTPUT_MSGS)
    {
        printf("MakeConx2\n");
    }
    result = MakeConx( swihd,
                       Vce_context_info.stream95,
                       Voiceincoming_slot,
                       TRUNK_VOICE_OUT(inbound_trunk),    /* stream on trunk 1 */
                       in_trunk_slot,    /* timeslot on trunk 1*/
                       DUPLEX);
    if (result != SUCCESS)
    {

        /* Stop protocol on PSTN In ctahd */
        DemoStopProtocol( cx->Incomingctahd );

        swiCloseSwitch(swihd);
        return result;
    }
    /*
      Do a start protocol and then do a get digits from other side.
    */
    if (OUTPUT_MSGS)
    {
        printf("ctaGetParms2\n");
    }
    ctaGetParms( cx->IvrInctahd,    /* get call resource parameters    */
                 ADI_START_PARMID,
                 &stparms,
                 sizeof(stparms) );

    stparms.callctl.eventmask = 0xffff;  /* show low-level evs */
    stparms.callctl.mediamask = 0x001f;  /* set media mask to start echo and dtmf */
    /* if echo is needed in connected state, set start parms so echo with start with 
       a start protocol.  Uncomment the following line of stparms.echocancel.mode
       to start the echo canceler.*/
    /* enable echo canceller and set parameters */
    /* Let the board determine what the best echo canceller to use is. Any DPF
       that needs to be canceled by echo, must be on the same DSP as echo.*/
    /* stparms.echocancel.mode = ADI_ECHOCANCEL_DEFAULT; */

    if (OUTPUT_MSGS)
    {
        printf("StartProtocol2\n");
    }
    DemoStartProtocol( cx->IvrInctahd, IVR_protocol, NULL, NULL );
       
        
    /* Out getExtension example plays a prompt and waits for Number_of_Digits digits.  */
    if ( GetExtension( cx->IvrInctahd, Number_of_Digits, digitBuf ) != SUCCESS)
    {
        goto hangup;
    }

       /* 
       Now Free IVR resources and start the protocol on the calling resource
       to place a call using the digits we received.
       */

     ctaGetParms( cx->Outgoingctahd,    /* get call resource parameters    */
                  ADI_START_PARMID,
                  &stparms,
                  sizeof(stparms) );

     stparms.callctl.eventmask = 0xffff;  /* show low-level evs */
     stparms.callctl.mediamask = 0x000f; /* f;  /* set media mask to DTMF.  Make sure 80F is in 
                                             resource definition string.*/

     /* Stop protocol on IVR ctahd and free IVR resources*/
     DemoStopProtocol( cx->IvrInctahd );
     DestroyContext( cx->IvrInctahd, cx->ctaqueuehd);

     DemoStartProtocol( cx->Outgoingctahd, PSTNoutgoing_protocol,
                        NULL, &stparms );


    /* listen in on call progress -- connect audio from outbound to inbound */

     MakeConx(swihd, 
                TRUNK_VOICE_IN(outbound_trunk), out_trunk_slot,
                TRUNK_VOICE_OUT(inbound_trunk), in_trunk_slot, 
                SIMPLEX);

     /* Place second call on the line using digits entered by caller. */
     if ( DemoPlaceCall( cx->Outgoingctahd, digitBuf, NULL ) != SUCCESS )
     {
         goto hangup;
     }

        /* 
            This connection switches the voice channels of two trunks together.
            Since no connections have been specifically broken, except that we have
            released the IVR resource, all remaining resources are getting the
            data for this call.
        */
     result = MakeConx( swihd, 
                        TRUNK_VOICE_IN(inbound_trunk),
                        in_trunk_slot,
                        TRUNK_VOICE_OUT(outbound_trunk),
                        out_trunk_slot,
                        DUPLEX);
     if (result != SUCCESS)
     {
         return result;
     }


     for (;;)
     {
         DWORD ctaret;
         char buf[201];

         ctaret = ctaWaitEvent( cx->ctaqueuehd, &event, CTA_WAIT_FOREVER);

         /* display the events being received */
         ctaFormatEvent("event: ", &event, buf, 200);
         fprintf(stderr, buf);
         

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
                 /*
                    If follow on call is required, add another case to this case statement
                    looking for the '#' digit and take appropriate action.
                 */
                 break;
         }
     }

hangup:
     DemoHangUp( cx->Incomingctahd );
     DemoHangUp( cx->Outgoingctahd );

     BreakVoiceConx(swihd, TRUNK_VOICE_OUT(inbound_trunk), in_trunk_slot);
     BreakVoiceConx(swihd, TRUNK_VOICE_OUT(outbound_trunk), out_trunk_slot);

     BreakVoiceConx(swihd, TRUNK_SIGNAL_OUT(inbound_trunk), in_trunk_slot);
     BreakVoiceConx(swihd, TRUNK_SIGNAL_OUT(outbound_trunk), out_trunk_slot);


     BreakVoiceConx(swihd, In_context_info.stream95, LogicalDSPincoming_slot);
     BreakVoiceConx(swihd, Vce_context_info.stream95, Voiceincoming_slot);
     BreakVoiceConx(swihd, Out_context_info.stream95, LogicalDSPoutgoing_slot);


     /* Stop protocol on PSTN In ctahd */
     DemoStopProtocol( cx->Incomingctahd );

     /* Stop protocol on PSTN out ctahd */
     DemoStopProtocol( cx->Outgoingctahd );

     ShutdownPort( cx);
  
     return result; 
}

/**********************************************************************/

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
    printf( "\tincoming slot = %d, outgoing slot = %d\n",
            LogicalDSPincoming_slot, LogicalDSPoutgoing_slot);
    printf( "\tIncoming Protocol                        = %s\n",
            PSTNincoming_protocol );
    printf( "\tOutgoing Protocol                        = %s\n",
            PSTNoutgoing_protocol );
    printf( "\n" );

    /* Register an error handler to display errors returned from API functions;
     * no need to check return values in the demo.
     */
    ctaSetErrorHandler( NonFatalErrorHandler, NULL );

    /* initialize Natural Access services for this process */
    DemoInitialize( servicelist, sizeof(servicelist)/sizeof(servicelist[0]) );

    if (ret = RunDemo() != SUCCESS)
    {
        printf("Could not run demo: %d\n", ret);
        exit(-1);
    }

}

