/*****************************************************************************
 *  File -  nccxfer.c
 *
 *  Description - Simple demo showing the xfer of an inbound call to
 *                another extension on a PBX, using Natural Call Control.
 *
 *  Voice Files Used -
 *                NCCXFER.VOX    - nccxfer demo VOX file.
 *
 * Copyright (c) 1999-2002 NMS Communications.  All rights reserved.
 *****************************************************************************/
#define VERSION_NUM  "11"
#define VERSION_DATE __DATE__

/*----------------------------- Includes -----------------------------------*/
#include <stdio.h>

#include "ctademo.h"
#include "adidef.h"
#include "nccdef.h"

/* case displays of constants: */
#define _SHOWCASE_(x) case x: printf(" %s",#x); break

/*----------------------------- Application Definitions --------------------*/
#define QUEUE_SIZE 5

typedef struct
{
    CTAQUEUEHD queuehd;    /* Natural Access queue handle associated with port. */
    CTAHD      ctahd;      /* Natural Access handle associated with port.       */
    NCC_CALLHD callhd;     /* primary call handle                    */
    NCC_CALLHD callhd2;           /* call handle for second calls           */
    char       protocol[20];      /* name of protocol running on port       */
    int        iterations;        /* call iterations, 0=infinite            */
    unsigned   method;            /* method of transfer:                    */
#define BLIND_PROCEEDING 1    /*  transfers on 'PROCEEDING'; after dial */
#define BLIND_ALERTING   2    /*  transfers on 'ALERTING'  ; ringing    */
#define BLIND_CONNECTED  3    /*  transfers on 'CONNECTED' ; first voice*/
#define SUPERVISED       4    /*  manual transfer (call-screening)      */
} MYCONTEXT;

/* Misc. global state and config info */
MYCONTEXT MyContext = {
    NULL_CTAQUEUEHD,
    NULL_CTAHD,
    NULL_NCC_CALLHD,
    NULL_NCC_CALLHD,
    "lps0",
    0,
    BLIND_PROCEEDING
};

/* For ctaOpenServices */
CTA_SERVICE_DESC Services[] = {
    { {"adi", NULL},     {0}, {0}, {0} },
    { {"ncc", NULL},     {0}, {0}, {0} },
    { {"vce", "vcemgr"}, {0}, {0}, {0} }
};

/*----------------------------- Application Data ---------------------------*/
static unsigned TraceLevel = 0;


/*   Index                  - Message
 *
 *    NCCXFER_WELCOME_MSG    - "Thank you for calling."
 *    NCCXFER_GETEXT_MSG     - "Please enter the extension to try."
 *    NCCXFER_YOURNAME_MSG   - "Who may I say is calling?"
 *    NCCXFER_YOUHAVE_MSG    - "You have a call from..."
 *    NCCXFER_ACCEPT_MSG     - "Do you want to accept the call?"
 *    NCCXFER_YESNO_MSG      - "Press 1 for yes, 2 for no."
 *    NCCXFER_WILLTRY_MSG    - "I'll try that extension now."
 *    NCCXFER_SORRY_MSG      - "Sorry. The call could not be completed."
 *    NCCXFER_CONNECT_MSG    - "You are now being connected."
 *    NCCXFER_HANGUP_MSG     - "Please hangup, returning to caller."
 *    NCCXFER_GOODBYE_MSG    - "Good-bye."
 *
 */
static char *vox_file = "nccxfer.vox";

#define  NCCXFER_WELCOME_MSG     1
#define  NCCXFER_GETEXT_MSG      2
#define  NCCXFER_YOURNAME_MSG    3
#define  NCCXFER_YOUHAVE_MSG     4
#define  NCCXFER_ACCEPT_MSG      5
#define  NCCXFER_YESNO_MSG       6
#define  NCCXFER_WILLTRY_MSG     7
#define  NCCXFER_SORRY_MSG       8
#define  NCCXFER_CONNECT_MSG     9
#define  NCCXFER_HANGUP_MSG      10
#define  NCCXFER_GOODBYE_MSG     11

#if defined (UNIX)
static char *temp_vox = "/tmp/temp.vox";
#else
static char *temp_vox = "temp.vox";
#endif

/*----------------------------- Forward References -------------------------*/
void PrintHelp(          MYCONTEXT *cx );
void PrintSettings(      MYCONTEXT *cx );

int  RunDemo(            MYCONTEXT *cx );
void InitPort(           MYCONTEXT *cx );
int  GetExtension(       CTAHD ctahd, int count, char *digits );
int  GetYesNo(           CTAHD ctahd, char *digit );
int  TransferCall(       MYCONTEXT *cx, char *dialstring );
int  TransferSupervised( MYCONTEXT *cx, char *dialstring );
int  Place2ndCall(       CTAHD ctahd, char *dialstring );
int  HoldCall(           MYCONTEXT *cx );
int  RetrieveCall(       MYCONTEXT *cx );


/*----------------------------------------------------------------------------
  main
  ----------------------------------------------------------------------------*/
int main( int argc, char **argv )
{
    int c;
    MYCONTEXT *cx = &MyContext;

    printf( "Natural Access PBX Transfer Demo V.%s (%s)\n",
            VERSION_NUM, VERSION_DATE );

    while( ( c = getopt( argc, argv, "A:B:b:F:P:p:S:s:T:t:I:i:M:m:Hh?" ) ) 
           != -1 )
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
            sscanf( optarg, "%d", &(Services[1].mvipaddr.board) );
            break;
            case 'S':
            switch (sscanf( optarg, "%d:%d", &value1, &value2 ))
            {
                case 2:
                    Services[0].mvipaddr.stream   = value1;
                    Services[0].mvipaddr.timeslot = value2;
                    Services[1].mvipaddr.stream   = value1;
                    Services[1].mvipaddr.timeslot = value2;
                    break;
                case 1:
                    Services[0].mvipaddr.timeslot = value1;
                    Services[1].mvipaddr.timeslot = value1;
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

    PrintSettings( cx );

    return RunDemo( cx );
}

/*----------------------------------------------------------------------------
  PrintSettings()
  ----------------------------------------------------------------------------*/
void PrintSettings( MYCONTEXT *cx )
{
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

    return;
}


/*----------------------------------------------------------------------------
  PrintHelp()
  ----------------------------------------------------------------------------*/
void PrintHelp( MYCONTEXT *cx )
{
    printf("Usage: nccxfer [opts]\n");
    printf("  where opts are:\n");
    printf( "  -A adimgr     Natural Access manager to use for ADI service.     "
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
  Setup
  -------------------------------------------------------------------------*/
void Setup( MYCONTEXT *cx )
{
    /*
     * Initialize Natural Access.
     */
    {
        CTA_SERVICE_NAME init_svc[] =   {
            { "adi", ""       },
            { "ncc", ""       },
            { "vce", "vcemgr" }
        };

        /* Initialize ADI and NCC Service Manager names */
        init_svc[0].svcmgrname = DemoGetADIManager();
        init_svc[1].svcmgrname = DemoGetADIManager();

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

    return;
}


/*-------------------------------------------------------------------------
  RunDemo
  -------------------------------------------------------------------------*/
int RunDemo( MYCONTEXT *cx )
{
    char     digits[20];
    int      i;
    NCC_PROT_CAP    capabilities = {0};
    NCC_CALL_STATUS callstatus   = {0};

    Setup( cx );

    /* Loop, receiving calls and processing them.
     * If any function returns failure or disconnect, go on to next iteration.
     */
    for( i=0; (i < cx->iterations) || (cx->iterations == 0); i++ )
    {
        int ret;

        DemoWaitForNCCCall( cx->ctahd, &cx->callhd, NULL, NULL );

        /* Always answer.  Loop-start lines don't provide any
         * useful address information. Answer after 1 ring.
         */
        if( DemoAnswerNCCCall( cx->ctahd, cx->callhd, 1, NULL ) != SUCCESS )
            goto hangup;

        /* Play a welcome message to the caller.
         */
        adiFlushDigitQueue( cx->ctahd );
        ret = DemoPlayMessage( cx->ctahd, vox_file, NCCXFER_WELCOME_MSG,
                               0, NULL );
        if( ret != SUCCESS )
            goto hangup;

        /* Prompt the caller for the extension to try.  This
         * subroutine will return the digits.
         */
        if( GetExtension( cx->ctahd, 4, digits ) != SUCCESS )
            goto hangup;

        /*
         * Get capabilities, to check if protocol supports transfer.
         */

        nccQueryCapability( cx->ctahd, &capabilities, 
                            sizeof( capabilities ) );

        /* Try to transfer the call.  On successful transfer, loop
         * again waiting for the next call.
         */

        if( cx->method == SUPERVISED )
        {
            /* Check if supervised transfer is supported */
            if( !(capabilities.capabilitymask & NCC_CAP_SUPERVISED_TRANSFER) )
            {
                DemoReportComment( "Supervised transfer not supported." );
                goto hangup;
            }
            if( TransferSupervised( cx, digits ) == DISCONNECT )
                continue;
        }
        else
        {
            /* Check if automatic transfer is supported */
            if( !(capabilities.capabilitymask & NCC_CAP_AUTOMATIC_TRANSFER) )
            {
                DemoReportComment( "Automatic transfer not supported." );
                goto hangup;
            }
            if( TransferCall( cx, digits ) == DISCONNECT )
                continue;
        }

        if (cx->callhd != NULL_NCC_CALLHD)
        {
            /* See if it's OK to play a voice file. */
            nccGetCallStatus(cx->callhd, &callstatus, sizeof(callstatus));
            if (callstatus.state == NCC_CALLSTATE_CONNECTED &&
                callstatus.held == 0)
            {
                /* Let the user know that the transfer failed. */ 
                ret = DemoPlayMessage( cx->ctahd, vox_file, NCCXFER_SORRY_MSG,
                                       0, NULL );
            }
        }

hangup:
        /* Hangup.  Could add facility to try another
         * extension, too.
         */
        DemoHangNCCUp( cx->ctahd, cx->callhd, NULL, NULL );
        cx->callhd = NULL_NCC_CALLHD;
    }

    UNREACHED_STATEMENT( DemoShutdown( cx->ctahd ); )
        UNREACHED_STATEMENT( return 0; )
}



/*----------------------------------------------------------------------------
  InitPort

  ----------------------------------------------------------------------------*/
void InitPort( MYCONTEXT *cx )
{
    NCC_START_PARMS stparms;

    /* Set ADI port mode to full duplex. */
    Services[0].mvipaddr.mode = ADI_FULL_DUPLEX;
    Services[1].mvipaddr.mode = ADI_FULL_DUPLEX;

    /* Specify manager to use for ADI and NCC service. */
    Services[0].name.svcmgrname = DemoGetADIManager();
    Services[1].name.svcmgrname = DemoGetADIManager();

    /* Open the port. */
    DemoOpenPort( 0, "DEMOCONTEXT",
                  Services, ARRAY_DIM(Services),
                  &(cx->queuehd),
                  &(cx->ctahd) );

    /* Start the protocol, changing default parameters. */

    ctaGetParms( cx->ctahd, NCC_START_PARMID, &stparms, sizeof(stparms) );
    /* stparms.eventmask=0xffff; report low-level call-control events*/
    DemoStartNCCProtocol( cx->ctahd, cx->protocol, NULL, NULL, &stparms );
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
        ret = DemoPlayMessage( ctahd, vox_file, NCCXFER_GETEXT_MSG, 0, NULL );
        if( ret != SUCCESS )
            return FAILURE;

        DemoReportComment( "Collecting digits to dial..." );
        if( ( ret = DemoGetDigits( ctahd, count, digits ) ) != FAILURE )
            return ret;
    }

    /* Give up. */
    DemoPlayMessage( ctahd, vox_file, NCCXFER_GOODBYE_MSG, 0, NULL );
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
        ret = DemoPlayMessage( ctahd, vox_file, NCCXFER_YESNO_MSG, 0, NULL );
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
int TransferCall( MYCONTEXT *cx, char *dialstring )
{
    CTA_EVENT event;
    unsigned xfer_method;
    int ret;

    /* Tell the caller that we're going to try making the call.
     */
    ret = DemoPlayMessage( cx->ctahd, vox_file, NCCXFER_WILLTRY_MSG, 0, NULL );
    if( ret != SUCCESS )
    {
        return DemoHangNCCUp( cx->ctahd, cx->callhd, NULL, NULL );
    }

    switch( cx->method )
    {
        case BLIND_PROCEEDING: xfer_method = NCC_TRANSFER_PROCEEDING; break;
        case BLIND_ALERTING  : xfer_method = NCC_TRANSFER_ALERTING  ; break;
        case BLIND_CONNECTED : xfer_method = NCC_TRANSFER_CONNECTED ; break;
    }

    DemoReportComment( "--------\nTransfering call..." );
    nccAutomaticTransfer( cx->callhd, dialstring, xfer_method, 
                          NULL, NULL, NULL );

    for(;;)
    {
        DemoWaitForEvent( cx->ctahd, &event );

        switch( event.id )
        {
            case NCCEVN_CALL_HELD:
                {
                    /* ack of nccAutomaticTransfer; wait for a disconnected */
                    break;
                }

            case NCCEVN_CALL_DISCONNECTED:
                {
                    if( event.value == NCC_DIS_TRANSFER )
                    {
                        DemoReportComment( "Call disconnected.  Transfer complete." );
                        ret = DemoHangNCCUp( cx->ctahd, cx->callhd, NULL, NULL );
                        cx->callhd = NULL_NCC_CALLHD;
                    }
                    else
                    {
                        DemoReportComment( "Transfer not successful." );
                        ret = DemoHangNCCUp( cx->ctahd, cx->callhd, NULL, NULL );
                        cx->callhd = NULL_NCC_CALLHD;
                    }

                    return ret; 
                }

            case NCCEVN_CALL_RETRIEVED:
                {
                    DemoReportComment( "Transfer not successful." );
                    return FAILURE;
                }

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
int TransferSupervised( MYCONTEXT *cx, char *dialstring )
{
    CTA_EVENT event;
    VCE_RECORD_PARMS recparms;
    NCC_CALL_STATUS callstatus = {0};
    char digit[2];
    int ret;

    /* Prompt the caller for their recorded name.
     */
    ret = 
        DemoPlayMessage( cx->ctahd, vox_file, NCCXFER_YOURNAME_MSG, 0, NULL );
    if( ret != SUCCESS )
        goto failure;

    /* Record callers name, overriding default record parameters for
     * quick recognition of silence afterwards.
     */
    ctaGetParms( cx->ctahd, VCE_RECORD_PARMID, &recparms, sizeof(recparms) );
    recparms.silencetime = 500; /* 1/2 sec */
    ret = DemoRecordMessage( cx->ctahd, temp_vox, 0, 
                             VCE_ENCODE_NMS_24, &recparms );
    if( ret != SUCCESS )
        goto failure;

    /* Tell the caller that we're going to try making the call.
     * "I'll try that extension now."
     */
    ret = DemoPlayMessage( cx->ctahd, vox_file, NCCXFER_WILLTRY_MSG, 0, NULL );
    if( ret != SUCCESS )
        goto failure;

    /* Put first call on hold */

    ret = HoldCall( cx );
    if( ret != SUCCESS )
        goto failure;

    /* Place second call on the line.
     */
    ret = DemoPlaceNCCCall( cx->ctahd, dialstring, "5086209300", 
                            NULL, NULL, &(cx->callhd2) );

    /* Deal with failure modes */
    if( ret != SUCCESS )
        goto failure;

    /* Otherwise, we're in the Connected state.  Prompt for instruction. */

    /* "You have a call from ...xxxxxx. Do you want to accept the call?"
     * Then prompt for yes or no.
     */

    adiFlushDigitQueue( cx->ctahd );
    ret = DemoPlayMessage( cx->ctahd, vox_file, NCCXFER_YOUHAVE_MSG, 0, NULL );
    if( ret != SUCCESS )
        goto failure;

    ret = DemoPlayMessage( cx->ctahd, temp_vox, 0, 0, NULL );
    if( ret != SUCCESS )
        goto failure;

    ret = DemoPlayMessage( cx->ctahd, vox_file, NCCXFER_ACCEPT_MSG, 0, NULL );
    if( ret != SUCCESS )
        goto failure;

    if( GetYesNo( cx->ctahd, digit ) != SUCCESS )
        goto failure;

    if( *digit == '1' /*YES*/ )
    {
        ret = DemoPlayMessage( cx->ctahd, vox_file, NCCXFER_CONNECT_MSG, 
                               0, NULL );
        if( ret != SUCCESS )
            goto failure;
        ret = nccTransferCall( cx->callhd, cx->callhd2, NULL );

        if( ret != SUCCESS )
            goto failure;

        /* Both calls will be disconnected. Immediately release each call.
         * When calls set to NULL_NCC_CALLHD, cleanup has been finished.
         */
        while (cx->callhd != NULL_NCC_CALLHD ||
               cx->callhd2 != NULL_NCC_CALLHD)
        {
            DemoWaitForEvent( cx->ctahd, &event );

            switch( event.id )
            {
                case NCCEVN_CALL_DISCONNECTED:
                    {
                        if( event.objHd == cx->callhd )
                        {
                            DemoReportComment( "Call 1 disconnected." );
                            nccReleaseCall( cx->callhd, NULL );
                        }
                        else
                        {
                            DemoReportComment( "Call 2 disconnected." );
                            nccReleaseCall( cx->callhd2, NULL );
                        }

                        break;
                    }

                case NCCEVN_CALL_RELEASED:
                    {
                        if( event.objHd == cx->callhd )
                        {
                            DemoReportComment( "Call 1 released." );
                            cx->callhd = NULL_NCC_CALLHD;
                        }
                        else
                        {
                            DemoReportComment( "Call 2 released." );
                            cx->callhd2 = NULL_NCC_CALLHD;
                        }

                        break;
                    }

                default:
                    DemoReportUnexpectedEvent( &event );
                    break;

            }
        }

        return DISCONNECT;
    }

failure:

    if (cx->callhd2 != NULL_NCC_CALLHD)
    {
        /* Check call state. */
        nccGetCallStatus(cx->callhd2, &callstatus, sizeof(callstatus));

        if (callstatus.state == NCC_CALLSTATE_CONNECTED)
        {
            /* Please hang up - returning to caller... */
            DemoPlayMessage( cx->ctahd, vox_file, NCCXFER_HANGUP_MSG, 
                             0, NULL );
        }

        if (callstatus.state != NCC_CALLSTATE_DISCONNECTED)
        {
            DemoReportComment( "Waiting for remote disconnect..." );

            /* If cleardown is not supported by the PBX or CO, 
               this function will block forever! */
            DemoWaitForSpecificEvent( cx->ctahd,
                                      NCCEVN_CALL_DISCONNECTED, 
                                      &event );
        }

        DemoHangNCCUp( cx->ctahd, cx->callhd2, NULL, NULL );
        cx->callhd2 = NULL_NCC_CALLHD;
    }

    /* Retrieve call if necessary. */
    nccGetCallStatus(cx->callhd, &callstatus, sizeof(callstatus));
    if (callstatus.state == NCC_CALLSTATE_CONNECTED &&
        callstatus.held != 0)
    {
        /* Retrieve first call. */
        RetrieveCall( cx );
    }

    /* Indicate that first call may still be on the line. */
    return FAILURE;
}


int HoldCall( MYCONTEXT *cx )
{
    CTA_EVENT event;
    int ret;

    ret = nccHoldCall( cx->callhd, NULL );
    if( ret != SUCCESS )
    {
        DemoReportComment( "Call not held." );
        return ret;
    }

    for (;;)
    {
        DemoWaitForEvent( cx->ctahd, &event );

        switch( event.id )
        {
            case NCCEVN_CALL_DISCONNECTED:
                DemoReportComment( "Call disconnected." );
                return FAILURE;

            case NCCEVN_HOLD_REJECTED:
                DemoReportComment( "Hold rejected." );
                return FAILURE;

            case NCCEVN_CALL_HELD:
                return SUCCESS;
                /* this is commented out b/c it is never reached */
                /*  break; */

            default:
                DemoReportUnexpectedEvent( &event );

                break;
        }
    }

    UNREACHED_STATEMENT( return SUCCESS; )
}


int RetrieveCall( MYCONTEXT *cx )
{
    CTA_EVENT event;
    int ret;

    ret = nccRetrieveCall( cx->callhd, NULL );
    if( ret != SUCCESS )
    {
        DemoReportComment( "Call not retrieved." );
        return ret;
    }

    for (;;)
    {
        DemoWaitForEvent( cx->ctahd, &event );

        switch( event.id )
        {
            case NCCEVN_CALL_DISCONNECTED:
                {
                    /* Clean up the first call if it was disconnected. */
                    DemoHangNCCUp( cx->ctahd, event.objHd, NULL, NULL );
                    /* We can be certain that this is the first call, because
                       we can only retrieve a held call if all other calls
                       on the line are disconnected or on hold. */
                    cx->callhd = NULL_NCC_CALLHD;
                    return FAILURE;
                }

            case NCCEVN_RETRIEVE_REJECTED:
                DemoReportComment( "Retrieve rejected." );
                return FAILURE;

            case NCCEVN_CALL_RETRIEVED:
                return SUCCESS;
                /* this is commented out b/c it is never reached */ 
                /* break; */ 

            default:
                DemoReportUnexpectedEvent( &event );
                break;
        }
    }

    UNREACHED_STATEMENT( return SUCCESS; )
}
