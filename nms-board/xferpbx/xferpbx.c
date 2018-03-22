/*****************************************************************************
*  File -  xferpbx.c
*
*  Description - Simple demo showing the xfer of an inbound call to
*                another extension on a PBX.
*
*  Voice Files Used -
*                XFERPBX.VOX    - xferpbx demo VOX file.
*
* Copyright (c) 1999-2002 NMS Communications.  All rights reserved.
*****************************************************************************/
#define VERSION_NUM  "11"
#define VERSION_DATE __DATE__

/*----------------------------- Includes -----------------------------------*/
#include <stdio.h>

#include "ctademo.h"
#include "adidef.h"

/* case displays of constants: */
#define _SHOWCASE_(x) case x: printf(" %s",#x); break

/*----------------------------- Application Definitions --------------------*/
#define QUEUE_SIZE 5

typedef struct
{
    CTAQUEUEHD queuehd;    /* Natural Access queue handle associated with port. */
    CTAHD      ctahd;      /* Natural Access handle associated with port.       */
    char       protocol[20];      /* name of protocol running on port       */
    int        iterations;        /* call iterations, 0=infinite            */
    unsigned   method;            /* method of transfer:                    */
      #define BLIND_PROCEEDING 1  /*  transfers on 'PROCEEDING'; after dial */
      #define BLIND_ALERTING   2  /*  transfers on 'ALERTING'  ; ringing    */
      #define BLIND_CONNECTED  3  /*  transfers on 'CONNECTED' ; first voice*/
      #define SUPERVISED       4  /*  manual transfer (call-screening)      */
} MYCONTEXT;

/* Misc. global state and config info */
MYCONTEXT MyContext = {
    NULL_CTAQUEUEHD,
    NULL_CTAHD,
    "lps0",
    0,
    BLIND_PROCEEDING
};

/* For ctaOpenServices */
CTA_SERVICE_DESC Services[] = {
    { {"adi", NULL},     {0}, {0}, {0} },
    { {"vce", "vcemgr"}, {0}, {0}, {0} }
};

/*----------------------------- Application Data ---------------------------*/
static unsigned TraceLevel = 0;


/*   Index                  - Message
*
*    XFERPBX_WELCOME_MSG    - "Thank you for calling."
*    XFERPBX_GETEXT_MSG     - "Please enter the extension to try."
*    XFERPBX_YOURNAME_MSG   - "Who may I say is calling?"
*    XFERPBX_YOUHAVE_MSG    - "You have a call from..."
*    XFERPBX_ACCEPT_MSG     - "Do you want to accept the call?"
*    XFERPBX_YESNO_MSG      - "Press 1 for yes, 2 for no."
*    XFERPBX_WILLTRY_MSG    - "I'll try that extension now."
*    XFERPBX_SORRY_MSG      - "Sorry. The call could not be completed."
*    XFERPBX_CONNECT_MSG    - "You are now being connected."
*    XFERPBX_HANGUP_MSG     - "Please hangup, returning to caller."
*    XFERPBX_GOODBYE_MSG    - "Good-bye."
*
*/
static char *vox_file = "xferpbx.vox";

#define  XFERPBX_WELCOME_MSG     1
#define  XFERPBX_GETEXT_MSG      2
#define  XFERPBX_YOURNAME_MSG    3
#define  XFERPBX_YOUHAVE_MSG     4
#define  XFERPBX_ACCEPT_MSG      5
#define  XFERPBX_YESNO_MSG       6
#define  XFERPBX_WILLTRY_MSG     7
#define  XFERPBX_SORRY_MSG       8
#define  XFERPBX_CONNECT_MSG     9
#define  XFERPBX_HANGUP_MSG      10
#define  XFERPBX_GOODBYE_MSG     11

#if defined (UNIX)
static char *temp_vox = "/tmp/temp.vox";
#else
static char *temp_vox = "temp.vox";
#endif

/*----------------------------- Forward References -------------------------*/
void PrintHelp(          MYCONTEXT *cx );

int  RunDemo(            MYCONTEXT *cx );
void InitPort(           MYCONTEXT *cx );
int  GetExtension(       CTAHD ctahd, int count, char *digits );
int  GetYesNo(           CTAHD ctahd, char *digit );
int  TransferCall(       CTAHD ctahd, char *dialstring, unsigned method );
int  TransferSupervised( CTAHD ctahd, char *dialstring );
int  Place2ndCall(       CTAHD ctahd, char *dialstring );


/*----------------------------------------------------------------------------
                                main
----------------------------------------------------------------------------*/
int main( int argc, char **argv )
{
    int c;
    MYCONTEXT *cx = &MyContext;

    printf( "Natural Access PBX Transfer Demo V.%s (%s)\n",
            VERSION_NUM, VERSION_DATE );

    while( ( c = getopt( argc, argv, "A:B:b:F:P:p:S:s:T:t:I:i:M:m:Hh?" ) ) != -1 )
    {
        switch( toupper( c ) )
        {
                unsigned   value1, value2;

            case 'A':
                DemoSetADIManager( optarg );
                break;
            case 'F':
                DemoSetCfgFile( optarg );
                break;
            case 'B':
                sscanf( optarg, "%d", &(Services[0].mvipaddr.board) );
                break;
            case 'S':
                switch (sscanf( optarg, "%d:%d", &value1, &value2 ))
                {
                    case 2:
                        Services[0].mvipaddr.stream   = value1;
                        Services[0].mvipaddr.timeslot = value2;
                        break;
                    case 1:
                        Services[0].mvipaddr.timeslot = value1;
                        break;
                    default:
                        PrintHelp( cx );
                        exit( 1 );
                }
                break;
            case 'P':
                strncpy( cx->protocol, optarg, sizeof(cx->protocol)-1 );
                break;
            case 'M':
                sscanf( optarg, "%d", &(cx->method) );
                break;
            case 'T':
                sscanf( optarg, "%x", &TraceLevel );
                break;
            case 'I':
                sscanf( optarg, "%d", &(cx->iterations) );
                break;
            case 'H':
            case '?':
            default:
                PrintHelp( cx );
                exit( 1 );
                break;
        }
    }
    if (optind < argc)
    {
        printf( "Invalid option.  Use -h for help.\n" );
        exit( 1 );
    }

    printf( "\tProtocol   = %s\n",    cx->protocol );
    printf( "\tBoard      = %d\n",    Services[0].mvipaddr.board );
    printf( "\tStream:Slot= %d:%d\n", Services[0].mvipaddr.stream,
            Services[0].mvipaddr.timeslot );
    printf( "\tTraceLevel = 0x%X\n",  TraceLevel );
    printf( "\tIterations = %d\n",    cx->iterations );
    printf( "\tMethod     = " );
    switch( cx->method )
    {
        case BLIND_PROCEEDING:
            puts( "1=on PROCEEDING (after dial)\n");
            break;
        case BLIND_ALERTING:
            puts( "2=on ALERTING (ringing)\n");
            break;
        case BLIND_CONNECTED:
            puts( "3=on CONNECTED (first voice)\n");
            break;
        case SUPERVISED:
            puts( "4=manual (call-screening)\n");
            break;
        default:
            printf ("%d=???\n", cx->method);
            break;
    }

    return RunDemo( cx );
}


/*----------------------------------------------------------------------------
                            PrintHelp()
----------------------------------------------------------------------------*/
void PrintHelp( MYCONTEXT *cx )
{
    printf("Usage: xferpbx [opts]\n");
    printf("  where opts are:\n");
    printf( "  -A adi_mgr    Natural Access manager to use for ADI service.     "
            "Default = %s\n", DemoGetADIManager() );
    printf( "  -F filename   Natural Access Configuration File name.      "
            "Default = None\n" );
    printf( "  -b n          OAM board number.                       "
           "Default = %d\n", Services[0].mvipaddr.board );
    printf( "  -s [n:]m      DSP [stream and] timeslot.              "
           "Default = %d:%d\n", Services[0].mvipaddr.stream,
            Services[0].mvipaddr.timeslot );
    printf( "  -p protocol   Protocol to run (usually loop-start).   "
           "Default = %s\n", cx->protocol );
    printf( "  -i iterations Number of call iterations (0=infinite)  "
           "Default = %d\n", cx->iterations );

    printf( "  -m n          Method of transfer:                     "
           "Default = %d\n", cx->method );
    printf( "                  1 = on PROCEEDING (after dial)\n" );
    printf( "                  2 = on ALERTING (ringing)\n" );
    printf( "                  3 = on CONNECTED (first voice or out-of-band)\n" );
    printf( "                  4 = manual (call-screening)\n" );
}


/*-------------------------------------------------------------------------
                            RunDemo
-------------------------------------------------------------------------*/
int RunDemo( MYCONTEXT *cx )
{
    char     digits[20];
    int      i;

    /*
     * Initialize Natural Access.
     */
    {
        CTA_SERVICE_NAME init_svc[] =   {
            { "adi", ""       },
            { "vce", "vcemgr" }
        };
        
        /* Initialize ADI Service Manager name */
        init_svc[0].svcmgrname = DemoGetADIManager();

        DemoInitialize( init_svc, ARRAY_DIM(init_svc) );
    }

    /* For debugging, turn on tracing. */
    ctaSetTraceLevel( NULL_CTAHD, "cta", TraceLevel );

    /* From now on, the registered error handler will be called.
     * No need to check return values.
     */
    ctaSetErrorHandler( DemoErrorHandler, NULL );

    /* Open a port, which maps to a telephone line interface.
     * On this port, we'll accept calls, and perform the
     * transfer with the PBX.
     */
    InitPort( cx );

    /* Loop, receiving calls and processing them.
     * If any function returns failure or disconnect, go on to next iteration.
     */
    for( i=0; (i < cx->iterations) || (cx->iterations == 0); i++ )
    {
        int ret;

        DemoWaitForCall( cx->ctahd, NULL, NULL );

        /* Always answer.  Loop-start lines don't provide any
         * useful address information. Answer after 1 ring.
         */
        if( DemoAnswerCall( cx->ctahd, 1 ) != SUCCESS )
            goto hangup;

        /* Play a welcome message to the caller.
         */
        adiFlushDigitQueue( cx->ctahd );
        ret = DemoPlayMessage( cx->ctahd, vox_file, XFERPBX_WELCOME_MSG,
                               0, NULL );
        if( ret != SUCCESS )
            goto hangup;

        /* Prompt the caller for the extension to try.  This
         * subroutine will return the digits.
         */
        if( GetExtension( cx->ctahd, 4, digits ) != SUCCESS )
            goto hangup;

        /* Try to transfer the call.  On successful transfer, loop
         * again waiting for the next call.
         */
        if( cx->method == SUPERVISED )
        {
            if( TransferSupervised( cx->ctahd, digits ) == DISCONNECT )
                continue;
        }
        else
        {
            if( TransferCall( cx->ctahd, digits, cx->method ) == DISCONNECT )
                continue;
        }

        /* If the transfer failed, let the user know, and
         * hangup.  Could add facility to try another
         * extension, too.
         */
        ret = DemoPlayMessage( cx->ctahd, vox_file, XFERPBX_SORRY_MSG,
                               0, NULL );
        if( ret != SUCCESS )
            goto hangup;

    hangup:
        DemoHangUp( cx->ctahd );
    }

    UNREACHED_STATEMENT( DemoShutdown( cx->ctahd ); )
    UNREACHED_STATEMENT( return 0; )
}



/*----------------------------------------------------------------------------
                            InitPort

----------------------------------------------------------------------------*/
void InitPort( MYCONTEXT *cx )
{
    ADI_START_PARMS stparms;

    /* Set ADI port mode to full duplex. */
    Services[0].mvipaddr.mode = ADI_FULL_DUPLEX;

    /* Specify manager to use for ADI service. */
    Services[0].name.svcmgrname = DemoGetADIManager();

    /* Open the port. */
    DemoOpenPort( 0, "DEMOCONTEXT",
                  Services, ARRAY_DIM(Services),
                  &(cx->queuehd),
                  &(cx->ctahd) );

    /* Start the protocol, changing default parameters.
     */
    ctaGetParms( cx->ctahd, ADI_START_PARMID, &stparms, sizeof(stparms) );
    stparms.callctl.eventmask=0xffff; /* report low-level call-control events*/
    DemoStartProtocol( cx->ctahd, cx->protocol, NULL, &stparms );
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

    DemoReportComment( "Getting extension..." );
    for( i = 0; i < maxtries; i++ )
    {
        ret = DemoPlayMessage( ctahd, vox_file, XFERPBX_GETEXT_MSG, 0, NULL );
        if( ret != SUCCESS )
            return FAILURE;

        DemoReportComment( "Collecting digits to dial..." );
        if( ( ret = DemoGetDigits( ctahd, count, digits ) ) != FAILURE )
            return ret;
    }

    /* Give up. */
    DemoPlayMessage( ctahd, vox_file, XFERPBX_GOODBYE_MSG, 0, NULL );
    return FAILURE;
}


/*-------------------------------------------------------------------------
                                GetYesNo

  Play "yes or no" prompt, and get answer via digit from user.
-------------------------------------------------------------------------*/
int GetYesNo( CTAHD ctahd, char *digit )
{
    int ret;
    int i;
    int maxtries = 2;

    DemoReportComment( "Getting yes or no...");
    for( i = 0; i < maxtries; i++ )
    {
        ret = DemoPlayMessage( ctahd, vox_file, XFERPBX_YESNO_MSG, 0, NULL );
        if( ret != SUCCESS )
            return FAILURE;

        DemoReportComment( "Collecting digit for yes/no..." );
        if( ( ret = DemoGetDigits( ctahd, 1, digit ) ) != FAILURE )
            return ret;
    }

    return FAILURE;
}


/*-------------------------------------------------------------------------
                                TransferCall
-------------------------------------------------------------------------*/
int TransferCall( CTAHD ctahd, char *dialstring, unsigned method )
{
    CTA_EVENT event;
    unsigned xfer_method;
    int ret;

    /* Tell the caller that we're going to try making the call.
     */
    ret = DemoPlayMessage( ctahd, vox_file, XFERPBX_WILLTRY_MSG, 0, NULL );
    if( ret != SUCCESS )
    {
        return DemoHangUp( ctahd );
    }

    switch( method )
    {
        case BLIND_PROCEEDING: xfer_method = ADI_XFER_PROCEEDING; break;
        case BLIND_ALERTING  : xfer_method = ADI_XFER_ALERTING  ; break;
        case BLIND_CONNECTED : xfer_method = ADI_XFER_CONNECTED ; break;
    }

    DemoReportComment( "--------\nTransfering call..." );
    adiTransferCall( ctahd, dialstring, xfer_method, NULL );

    for(;;)
    {
        DemoWaitForEvent( ctahd, &event );

        switch( event.id )
        {
            case ADIEVN_PLACING_CALL2:
                /* ack of adiTransferCall; wait for a disconnected */
                break;

            case ADIEVN_CALL_DISCONNECTED:
                DemoReportComment( "Call released.  Transfer complete." );
                return DemoHangUp( ctahd );

            case ADIEVN_CALL2_DISCONNECTED:
                if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                    printf( "\tCall2 released.  Reason: %d\n", event.value );
                return FAILURE;

            case ADIEVN_CALL_PROCEEDING:
            case ADIEVN_REMOTE_ALERTING:
            case ADIEVN_REMOTE_ANSWERED:
                break;                  /* are expected (low-level events) */

            default:
                DemoReportUnexpectedEvent( &event );
                break;
        }
    }
    UNREACHED_STATEMENT( return SUCCESS; )
}


/*-------------------------------------------------------------------------
                                Place2ndCall
-------------------------------------------------------------------------*/
int Place2ndCall( CTAHD ctahd, char *dialstring )
{
    CTA_EVENT event;

    DemoReportComment( "--------\n\tPlacing second call..." );
    adiPlaceSecondCall( ctahd, dialstring, NULL );

    for (;;)
    {
        DemoWaitForEvent( ctahd, &event );

        switch( event.id )
        {
            case ADIEVN_PLACING_CALL2:
                /* ack of adiPlaceSecondCall */
                break;

            case ADIEVN_CALL2_CONNECTED:
                DemoReportComment( "Call2 connected; 1st party still on hold." );
                return SUCCESS;    /* call connected */

            case ADIEVN_CALL2_DISCONNECTED:
                if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                    printf( "\tCall2 released.  Reason: %d\n", event.value );
                return FAILURE;    /* call failed */

            case ADIEVN_CALL_DISCONNECTED:
                return DemoHangUp( ctahd ); /* should not happen ? */

            case ADIEVN_CALL_PROCEEDING:
            case ADIEVN_REMOTE_ALERTING:
            case ADIEVN_REMOTE_ANSWERED:
                break;                  /* are expected (low-level events) */

            default:
                DemoReportUnexpectedEvent( &event );
                break;
        }
    }
    UNREACHED_STATEMENT( return SUCCESS; )
}


/*-------------------------------------------------------------------------
                                TransferSupervised
-------------------------------------------------------------------------*/
int TransferSupervised( CTAHD ctahd, char *dialstring )
{
    CTA_EVENT event;
    VCE_RECORD_PARMS recparms;
    char digit[2];
    int ret;

    /* Prompt the caller for their recorded name.
     */
    ret = DemoPlayMessage( ctahd, vox_file, XFERPBX_YOURNAME_MSG, 0, NULL );
    if( ret != SUCCESS )
        goto failure;

    /* Record callers name, overriding default record parameters for
     * quick recognition of silence afterwards.
     */
    ctaGetParms( ctahd, VCE_RECORD_PARMID, &recparms, sizeof(recparms) );
    recparms.silencetime = 500; /* 1/2 sec */
    ret = DemoRecordMessage( ctahd, temp_vox, 0, VCE_ENCODE_NMS_24, &recparms );
    if( ret != SUCCESS )
        goto failure;

    /* Tell the caller that we're going to try making the call.
     * "I'll try that extension now."
     */
    ret = DemoPlayMessage( ctahd, vox_file, XFERPBX_WILLTRY_MSG, 0, NULL );
    if( ret != SUCCESS )
        goto failure;

    /* Place second call on the line.
     */
    if( Place2ndCall( ctahd, dialstring ) != SUCCESS )
        return FAILURE;

    /* For these, we need to get back to first caller .........*/

    /* "You have a call from ...xxxxxx. Do you want to accept the call?"
     * Then prompt for yes or no.
     */

    adiFlushDigitQueue( ctahd );
    ret = DemoPlayMessage( ctahd, vox_file, XFERPBX_YOUHAVE_MSG, 0, NULL );
    if( ret != SUCCESS )
        goto failure;

    ret = DemoPlayMessage( ctahd, temp_vox, 0, 0, NULL );
    if( ret != SUCCESS )
        goto failure;

    ret = DemoPlayMessage( ctahd, vox_file, XFERPBX_ACCEPT_MSG, 0, NULL );
    if( ret != SUCCESS )
        goto failure;

    if( GetYesNo( ctahd, digit ) != SUCCESS )
        goto failure;

    if( *digit == '1' /*YES*/ )
    {
        ret = DemoPlayMessage( ctahd, vox_file, XFERPBX_CONNECT_MSG, 0, NULL );
        if( ret != SUCCESS )
            goto failure;
        return DemoHangUp( ctahd );
    }
    else
    {
        ret = DemoPlayMessage( ctahd, vox_file, XFERPBX_HANGUP_MSG, 0, NULL );
        if( ret != SUCCESS )
            goto failure;

        /* Note: If the second caller does not hang-up, the following function
         *       call could result in a three way conference.
         *       If it is possible to get positive disconnect from the first
         *       caller, create a wait-for-event loop until CALL2_DISCONNECTED
         *       event is received before proceeding.
         */
        adiReleaseSecondCall( ctahd );

        for (;;)
        {
            DemoWaitForEvent( ctahd, &event );

            switch( event.id )
            {
                case ADIEVN_CALL_DISCONNECTED:
                    DemoReportComment( "Call disconnected." );
                    return DemoHangUp( ctahd );

                case ADIEVN_CALL2_DISCONNECTED:
                    DemoReportComment( "Second call released." );
                    return SUCCESS;

                case ADIEVN_DIGIT_BEGIN:
                case ADIEVN_DIGIT_END:
                    break;

                default:
                    DemoReportUnexpectedEvent( &event );
                    break;
            }
        }
    }

  failure:
    return DemoHangUp( ctahd );
}
