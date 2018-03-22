/*****************************************************************************
 *  File -  incta.c
 *
 *  Description - Simple inbound demo using Natural Access and
 *                the VCE and ADI Services.
 *                Single process per call resource programming model.
 *
 *  Voice Files Used -
 *                CTADEMO.VOX  - all the prompts
 *                TEMP.VOX     - scratch file for record and playback
 *
 * Copyright (c) 1999-2002 NMS Communications.  All rights reserved.
 *****************************************************************************/

#define VERSION_NUM  "7"
#define VERSION_DATE __DATE__

/*----------------------------- Includes -----------------------------------*/
#include "ctademo.h"

#include "nccdef.h"
#include "nccxadi.h"
#include "nccxisdn.h"
#include "nccxcas.h"

#include "csplayrc.h"

/*----------------------------- Application Definitions --------------------*/
typedef struct
{
    CTAHD       ctahd;            /* Natural Access context handle               */
    char        ctaname[CTA_CONTEXT_NAME_LEN]; /* Natural Access context name    */
    unsigned    lowlevelflag;     /* flag to get low-level events           */
    NCC_CALLHD  callhd;
} MYCONTEXT;

#if defined (UNIX)
#define TEMP_FILE "/tmp/temp.vox"
#else
#define TEMP_FILE "temp.vox"
#endif

/*----------------------------- Application Data ---------------------------*/
static MYCONTEXT MyContext =
{
    NULL_CTAHD,      /* Natural Access context handle */
    "INCTA-DEMO",    /* Natural Access context name   */
    0,               /* lowlevelflag             */
    NULL_NCC_CALLHD
};

static MYCONTEXT CommnContext =
{
    NULL_CTAHD,      /* Natural Access context handle */
    "COMMN-CXT",    /* Natural Access context name   */
    0,               /* lowlevelflag             */
    NULL_NCC_CALLHD
};

MYCONTEXT *commncx = &CommnContext;

unsigned   Loop_count  = 0;
unsigned   ag_board    = 0;
unsigned   mvip_stream = 0;
unsigned   mvip_slot   = 0;
char      *protocol    = "lps0";
unsigned   nccInUse    = 1;
unsigned   extsupport  = 0;
unsigned   clientServer  = 0;
char      *svraddr = NULL;

CTAQUEUEHD ctaqueuehd = NULL_CTAQUEUEHD;

/*----------------------------- Forward References -------------------------*/
void RunDemo( MYCONTEXT *cx );
int  SpeakDigits( CTAHD ctahd, char *digits );
void CustomRejectCall( CTAHD ctahd, NCC_CALLHD callhd );
int CSDemoPlayMessage( unsigned file_type, unsigned message_num );
int CSDemoRecordMessage( unsigned file_type, unsigned message_num);
int CSDemoPromptDigit( CTAHD ctahd, unsigned file_type, unsigned message,
                       char *digit, int maxtries );
int CSDemoPlayList( unsigned file_type, unsigned message[],
                    unsigned count );
void SendEvent( CTAHD ctahd, DWORD evtid, DWORD evtvalue, char *pBuf );

/*----------------------------------------------------------------------------
  PrintHelp()
  ----------------------------------------------------------------------------*/
void PrintHelp( void )
{
    printf( "Usage: incta [opts]\n" );
    printf( "  where opts are:\n" );
    printf( "  -A adi_mgr      Natural Access ADI manager to use.            "
            "Default = %s\n", DemoGetADIManager() );
    printf( "  -b n            OAM board number.                  "
            "Default = %d\n", ag_board );
    printf( "  -s [strm:]slot  DSP [stream and] timeslot.         "
            "Default = %d:%d\n", mvip_stream, mvip_slot );
    printf( "  -p protocol     Protocol to run.                   "
            "Default = %s\n", protocol );
    printf( "  -l              Have ADI send Low Level events.\n" );
    printf( "  -i              Iteration count before exit.\n");
    printf( "  -F filename     Natural Access Configuration File name  "
            "Default = None\n" );
    printf( "  -c              Use Legacy ADI Call Control        "
            "Default = NCC Call Control\n");
    printf( "  -m contextname  Communication Context Name         "
            "Default = COMMN-CXT\n");
    printf( "  -x contextname  Natural Access demo context Name        "
            "Default = INCTA-DEMO\n");
    printf( "  -C              Run in conjunction with csplayrec\n"
            "                  demo to demonstrate call handoff\n" );

}


/*----------------------------------------------------------------------------
  main
  ----------------------------------------------------------------------------*/
int main( int argc, char **argv )
{
    int        c;
    MYCONTEXT *cx = &MyContext;

    printf( "Natural Access Inbound Demo V.%s (%s)\n", VERSION_NUM, VERSION_DATE );

    while( ( c = getopt( argc, argv, "A:F:i:s:b:p:t:cCm:x:lHh?" ) ) != -1 )
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
            case 'b':
                sscanf( optarg, "%d", &ag_board );
                break;
            case 'i':
                sscanf( optarg, "%d", &Loop_count );
                break;
            case 'p':
                protocol = optarg;
                break;
            case 'l':
                cx->lowlevelflag = 1;
                break;
            case 'c':
                nccInUse = 0;
                break;
            case 'C':
                clientServer = 1;
                break;
            case 'm':
                clientServer = 1;
                sscanf( optarg, "%s", commncx->ctaname );
                break;
            case 'x':
                clientServer = 1;
                sscanf( optarg, "%s", cx->ctaname );
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

    printf( "\tProtocol   = %s\n",    protocol );
    printf( "\tBoard      = %d\n",    ag_board );
    printf( "\tStream:Slot= %d:%d\n", mvip_stream, mvip_slot );

    RunDemo( cx );
    return 0;
}



/*-------------------------------------------------------------------------
  RunDemo
  -------------------------------------------------------------------------*/
void RunDemo( MYCONTEXT *cx )
{
    char      called_digits[20];
    char      calling_digits[20];
    char      option[2];   /* 1 digit +1 for null-terminator */
    unsigned  i;
    int       retry = 0;
    char      commndescriptor[256] = { 0 };
    char      cxdescriptor[256] = { 0 };
    unsigned  userid = 0;
    int       ret ;
    char      fullname[256] = { 0 };
    NCC_PROT_CAP protcap = {0};  /*capability struct...to determine extended call status */

    NCC_ADI_START_PARMS nccadiparms = {0}; /* NCC ADI parameters                         */
    NCC_START_PARMS     nccparms = {0};    /* NCC Call Control specific parameters       */
    ADI_START_PARMS     adiparms = {0};    /* ADI Call Control specific parameters       */
    NCC_ADI_ISDN_PARMS  isdparms = {0};    /* ISDN specific parameters                   */


    /* Register an error handler to display errors returned from API functions;
     * no need to check return values in the demo.
     */
    ctaSetErrorHandler( DemoErrorHandler, NULL );

    /* Initialize Natural Access, open a call resource, open default services */
    if (nccInUse == 1)
        DemoNCCSetup( ag_board, mvip_stream, mvip_slot, &ctaqueuehd, &cx->ctahd,
                      cx->ctaname );
    else
        DemoCSSetup( ag_board, mvip_stream, mvip_slot, &ctaqueuehd, &cx->ctahd,
                     cx->ctaname );

    if (clientServer == 1)
    {
        /* create communicaiton context descriptor */
        if( svraddr == NULL  ||  *svraddr == 0 )
        {
            if( commncx->ctaname != NULL )
                strcpy( commndescriptor, commncx->ctaname );
        }
        else
        {
            sprintf( commndescriptor,  "//%s/%s", svraddr, commncx->ctaname );
        }

        do
        {
            if ( retry ) DemoSleep( 5 ); /* Sleep for 5 seconds and try again. */

            /* If ctaAttachContext returns error application exits
             * So I remove error hander */
            ctaSetErrorHandler( NULL, NULL );

            /* Continuously attempt to connect to common context until
            *  successful. The other process (csplayrc) may not have yet
            *  initialized.
            */

            if ((ret = ctaAttachContext( ctaqueuehd, userid, commndescriptor, &commncx->ctahd ))
                 != SUCCESS)
            {
                if( ret == CTAERR_NOT_IMPLEMENTED )
                {
                    printf("Unable to create context %s\n", commncx->ctaname);
                    printf("Ensure that CTAmode=1 in cta.cfg.\n");
                    exit(-1);
                }
                if ( !retry )
                {
                    printf("\nCurrently unable to attach to context %s; will keep trying.\n",
                            commncx->ctaname);
                    printf("Verify that 'csplayrc' is running and has created the same\n");
                    printf("named context\n");
                }
            }
            else
            {
                if ( retry )
                    printf("\nHave now been able to attach to context %s.\n\n",
                            commncx->ctaname);
            }

            /* If attach fails,  try to attach for up to a minute. */
            
            /* restore error handler */
            ctaSetErrorHandler( DemoErrorHandler, NULL );

        } while ( ( ret != SUCCESS ) && ( ++retry < 12 ) );

        if ( ret != SUCCESS )
        {
            /* Must have timed out attempting to attach to the
            *  context shared with csplayrc.
            */
            printf("\nUnable to attach to shared context. Exiting...\n");
            exit(-1);
        }

        /* create context descriptor */
        if( svraddr == NULL  ||  *svraddr == 0 )
        {
            if( cx->ctaname != NULL )
                strcpy( cxdescriptor, cx->ctaname );
        }
        else
        {
            sprintf( cxdescriptor,  "//%s/%s", svraddr, cx->ctaname );
        }

        /* Send the context descriptor as part of the event to csplayrc
        *  to attach to intcta's play and record context.
        */
        SendEvent( commncx->ctahd, APPEVN_CONTEXT_NAME, SUCCESS, cxdescriptor );
    }


    /* Initiate a protocol on the call resource so we can accept calls. */
    if ( cx->lowlevelflag )
    {
        nccparms.eventmask = 0xffff;
        adiparms.callctl.eventmask = 0xffff;  /* show low-level evs */
    }
    /* Retrieve the NCC_START_PARMID structure if in NCC mode, and the
     * ADI_START_PARMID if in ADI mode
     */
    if (nccInUse == 1)
    {
        ctaGetParms( cx->ctahd, NCC_START_PARMID, &nccparms,
                     sizeof(nccparms) );
        ctaGetParms( cx->ctahd, NCC_ADI_START_PARMID, &nccadiparms,
                     sizeof(nccadiparms) );
        if (strncmp(protocol, "isd", 3) == 0)
            ctaGetParms(cx->ctahd, NCC_ADI_ISDN_PARMID, &isdparms,
                        sizeof(isdparms));
    }
    else
    {
        ctaGetParms( cx->ctahd, ADI_START_PARMID, &adiparms,
                     sizeof(adiparms) );
    }

    /*  Here we have to start the protocol with the correct parameters, depending on
        what call control model and what protocol is being used.
     */

    if ((strncmp(protocol, "isd", 3) == 0) && (nccInUse == 1))
        DemoStartNCCProtocol(cx->ctahd, protocol, &isdparms.isdn_start,
                             &nccadiparms, &nccparms);
    else if ((strncmp(protocol, "isd", 3) != 0) && (nccInUse == 1))
        DemoStartNCCProtocol(cx->ctahd, protocol, NULL, &nccadiparms,
                             &nccparms);
    else if ((strncmp(protocol, "isd", 3) == 0) && (nccInUse != 1))
        DemoStartProtocol( cx->ctahd, protocol, (WORD*)&isdparms,
                           &adiparms );
    else if ((strncmp(protocol, "isd", 3) != 0) && (nccInUse != 1))
        DemoStartProtocol( cx->ctahd, protocol, NULL, &adiparms );

    if (nccInUse == 1)
    {
        nccQueryCapability(cx->ctahd, &protcap, sizeof(protcap));

        if ((protcap.capabilitymask & NCC_CAP_EXTENDED_CALL_STATUS)
            == NCC_CAP_EXTENDED_CALL_STATUS)
        {
            extsupport = 1;
            DemoReportComment("Extended Call Status supported...");
        }

        else
            DemoReportComment("Extended Call Status not supported...");
    }

    /* Loop forever, receiving calls and processing them.
     * In this loop, if any function returns a failure or disconnect,
     * simply loop again for the next call.
     */
    for ( i = 0; ; i++ )
    {
        int ret;

        if ( Loop_count != 0 && Loop_count == i )
        {
            DemoShutdown( cx->ctahd );
            return;
        }
        if (nccInUse == 1)
            DemoWaitForNCCCall( cx->ctahd, &cx->callhd, called_digits,
                                calling_digits );
        else
            DemoWaitForCall( cx->ctahd, called_digits, calling_digits );

        /* If DID/DNIS line, demo the rejection of a call based on the
         *  address information: if the first digit is '9', play busy;
         *  if '8', play reorder tone.  If '0' play a reject message.
         * For other protocols, which don't provide any useful address
         *  information, just answer.
         * Answer will occur after 1 ring.
         */
        switch( called_digits[0] )
        {
            case '0':
                CustomRejectCall( cx->ctahd, cx->callhd );
                goto hangup;
            case '9':
                if (nccInUse == 1)
                    DemoRejectNCCCall( cx->ctahd, cx->callhd,
                                       NCC_REJECT_PLAY_BUSY, NULL);
                else
                    DemoRejectCall( cx->ctahd, ADI_REJ_PLAY_BUSY );
                goto hangup;
            case '8':
                if (nccInUse == 1)
                    DemoRejectNCCCall( cx->ctahd, cx->callhd,
                                       NCC_REJECT_PLAY_REORDER, NULL);
                else
                    DemoRejectCall( cx->ctahd, ADI_REJ_PLAY_REORDER );
                goto hangup;
            default:
                if (nccInUse == 1)
                {
                    if ( DemoAnswerNCCCall( cx->ctahd, cx->callhd,
                                            1 /* #rings */, NULL ) != SUCCESS )
                        goto hangup;
                }
                else if ( DemoAnswerCall( cx->ctahd, 1 /* #rings */ ) != SUCCESS )
                    goto hangup;
        }

        /* Play a welcome message to the caller. */
        adiFlushDigitQueue( cx->ctahd );

        if (clientServer == 1)
            ret = CSDemoPlayMessage( CS_DEMO_FILE, PROMPT_WELCOME );
        else
            ret = DemoPlayMessage( cx->ctahd, "ctademo", PROMPT_WELCOME,
                                   0, NULL );
        if( ret != SUCCESS )
            goto hangup;

        /* Announce the called number, if available. */
        if( strlen( called_digits ) > 0 )
        {
            if (clientServer == 1)
                ret = CSDemoPlayMessage( CS_DEMO_FILE, PROMPT_CALLED );
            else
                ret = DemoPlayMessage( cx->ctahd, "ctademo", PROMPT_CALLED,
                                       0, NULL );
            if( ret != SUCCESS )
                goto hangup;
            if( SpeakDigits( cx->ctahd, called_digits ) != SUCCESS )
                goto hangup;
        }

        /* Announce the calling number, if available */
        if( isdigit(*calling_digits) )
        {
            if (clientServer == 1)
                ret = CSDemoPlayMessage( CS_DEMO_FILE, PROMPT_CALLING );
            else
                ret = DemoPlayMessage( cx->ctahd, "ctademo", PROMPT_CALLING,
                                       0, NULL );
            if( ret != SUCCESS )
                goto hangup;
            if( SpeakDigits( cx->ctahd, calling_digits ) != SUCCESS )
                goto hangup;
        }

        /* Loop, giving the user three options: record, playback, hangup. */
        for(;;)
        {
            if (clientServer == 1)
                ret = CSDemoPromptDigit( cx->ctahd, CS_DEMO_FILE, PROMPT_OPTION,
                                         option, 2 );
            else
                ret = DemoPromptDigit( cx->ctahd, "ctademo", PROMPT_OPTION,
                                       0, NULL, option, 2 );
            if ( ret != SUCCESS )
                break;

            switch( *option )
            {
                VCE_RECORD_PARMS rparms;

                case '1':
                    /* Record the caller, overriding default parameters for
                     * quick recognition of silence once voice input has begun.
                     */
                    ctaGetParms( cx->ctahd, VCE_RECORD_PARMID,
                                 &rparms, sizeof(rparms) );

                    rparms.silencetime = 500; /* 1/2 sec */

                    if (clientServer == 1)
                        ret = CSDemoRecordMessage( CS_TEMP_FILE, 0 );
                    else
                        ret = DemoRecordMessage( cx->ctahd, TEMP_FILE, 0,
                                                 ADI_ENCODE_NMS_24, &rparms );
                    if( ret != SUCCESS )
                        goto hangup;
                    break;

                case '2':
                    /* Play back anything the caller recorded.
                     * This might be something recorded last time
                     * the demo was run, or the file might not exist.
                     */
                    if( ret != SUCCESS )
                        goto hangup;

                    if (clientServer == 1)
                        ret = CSDemoPlayMessage( CS_TEMP_FILE, 0 );
                    else
                        ret = DemoPlayMessage( cx->ctahd, TEMP_FILE, 0,
                                               ADI_ENCODE_NMS_24, NULL );
                    if( ret == FAILURE )
                    {
                        if (clientServer == 1)
                            ret = CSDemoPlayMessage( CS_DEMO_FILE,
                                                     PROMPT_NOTHING );
                        else
                            ret = DemoPlayMessage( cx->ctahd, "ctademo",
                                                   PROMPT_NOTHING, 0, NULL );
                    }

                    if( ret != SUCCESS )
                        goto hangup;
                    break;

                case '3':
                    goto hangup;

                default:
                    printf( "\t Invalid selection: '%c'\n", *option );
                    break;
            }
        }
hangup:
        if (nccInUse == 1)
            DemoHangNCCUp(cx->ctahd, cx->callhd, NULL, NULL );
        else
            DemoHangUp(cx->ctahd);
    }

    if (clientServer == 1)
    {
        CTA_EVENT event = { 0 };

        event.id = APPEVN_SHUT_DOWN;
        event.ctahd = cx->ctahd;

        if ( ctaQueueEvent( &event ) != SUCCESS )
            DemoShowError( "ctaQueueEvent failed", event.value );
    }

    UNREACHED_STATEMENT( ctaDestroyQueue( ctaqueuehd ); )
        UNREACHED_STATEMENT( return; )
}


/*-------------------------------------------------------------------------
  CustomRejectCall

  Reject the call with a custom tone sequence.
  -------------------------------------------------------------------------*/
void CustomRejectCall( CTAHD ctahd, NCC_CALLHD callhd )
{
    CTA_EVENT  event;
    int        tonesdone = 0;
    int        disconnected = 0;
    int        i;

    ADI_TONE_PARMS p[4];

    ctaGetParms(ctahd, ADI_TONE_PARMID, &p[0], sizeof(p[0]) );

    p[0].iterations = 1;
    for( i=1; i<4; i++ )
        p[i] = p[0];

    p[0].freq1      = 914;
    p[0].ampl1      = -24;
    p[0].ontime     = 274;

    p[1].freq1      = 1429;
    p[1].ampl1      = -24;
    p[1].ontime     = 380;

    p[2].freq1      = 1777;
    p[2].ampl1      = -24;
    p[2].ontime     = 380;
    p[2].iterations = 1;

    p[3].freq1      = 0;
    p[3].ampl1      = 0;
    p[3].ontime     = 300;

    printf( "\tPlaying reject tone\n" );

    nccRejectCall( callhd, NCC_REJECT_USER_AUDIO, NULL );

    /*
     *  Need both TONES_DONE and DISCONNECTED events before returning.
     */

    while ( ! ( disconnected && tonesdone ) )
    {
        DemoWaitForEvent( ctahd, &event );

        switch ( event.id )
        {
            case NCCEVN_REJECTING_CALL:
                adiStartTones( ctahd, 4, p );
                break;

            case ADIEVN_TONES_DONE:
                if (event.value == CTA_REASON_RELEASED )
                {
                    tonesdone = 1;     /* Wait for DISCONNECTED event */
                }
                else if ( CTA_IS_ERROR(event.value) )
                {
                    DemoShowError( "Play Tones failed", event.value );

                    tonesdone = 1;     /* Wait for DISCONNECTED event */
                }
                else
                {
                    /* The remote party is still connected.  Continue playing
                     * the SIT tone.
                     */
                    adiStartTones( ctahd, 4, p );
                }
                break;

            case NCCEVN_CALL_DISCONNECTED:
                printf( "\tCaller hung up.\n" );
                disconnected = 1;
                /* Expect TONES_DONE, reason RELEASED */
                break;

            case ADIEVN_DIGIT_BEGIN:
            case ADIEVN_DIGIT_END:
                break;

            default:
                printf( "\t\t\t  --unexpected event.\n" );
                break;
        }

        /* In Natural Access Client/Server  mode, ensure proper release of 
         * Natural Access owned buffers. */
        if ( event.buffer && (event.size & CTA_INTERNAL_BUFFER) )
        {
            ctaFreeBuffer( event.buffer );
        }
    }
}


/*-------------------------------------------------------------------------
  SpeakDigits
  -------------------------------------------------------------------------*/
int SpeakDigits( CTAHD ctahd, char *digits )
{
    unsigned  ret;
    char     *pc;
    unsigned  realdigits[80];
    unsigned  i;

    for( pc = digits, i = 0; pc && *pc; pc++)
    {
        if( isdigit( *pc ) )
        {
            realdigits[i++] = *pc -'0';
        }
    }

    if( i > 0 )
    {
        if (clientServer == 1)
            ret = CSDemoPlayList( CS_DEMO_FILE, realdigits, i );
        else
            ret = DemoPlayList( ctahd, "ctademo", realdigits, i, 0, NULL );
        if( ret != SUCCESS )
            return ret;
    }
    return SUCCESS;
}


/*-------------------------------------------------------------------------
  CSDemoPlayMessage
  -------------------------------------------------------------------------*/
int CSDemoPlayMessage( unsigned file_type, unsigned message_num )
{
    CTA_EVENT event = { 0 };
    int       ret;

    event.id = APPEVN_PLAY_MESSAGE;
    event.ctahd = commncx->ctahd;
    event.value = file_type;
    event.size = message_num;
    event.buffer = NULL;

    ret = ctaQueueEvent( &event );
    if (ret != SUCCESS)
    {
        DemoShowError( "ctaQueueEvent failed", event.value );
        return ret;
    }

    for(;;)
    {
        event.id = 0;
        event.value = 0;
        DemoWaitForEvent( commncx->ctahd, &event );

        switch ( event.id )
        {
            case ADIEVN_CALL_DISCONNECTED:
                DemoReportComment( "Other party hung up." );
                break;

            case APPEVN_PLAY_FAILURE:
                switch ( event.value )
                {
                    case APP_REASON_UNKNOWN_FILE:
                        printf("Error: Unknown file type\n");
                        break;
                    case APP_REASON_FUNCTION_FAILED:
                        printf("Error: DemoPlayMessage failed\n");
                        break;
                    default:
                        printf("Error; Reason Unknown.\n");
                        break;
                }
                return FAILURE;

            case APPEVN_PLAY_DONE:

                /* In Natural Access Client/Server  mode, ensure proper release 
                 * of Natural Access owned buffers*/
                if ( event.buffer && (event.size & CTA_INTERNAL_BUFFER) )
                    ctaFreeBuffer( event.buffer );

                switch ( event.value )
                {
                    case APP_REASON_DISCONNECT:
                        return DISCONNECT;
                    default:
                        return SUCCESS;
                }

            default:
                break;
        }
        /* In Natural Access Client/Server mode, ensure proper release of 
         * Natural Access owned buffers. */
        if ( event.buffer && (event.size & CTA_INTERNAL_BUFFER) )
            ctaFreeBuffer( event.buffer );
    }

    UNREACHED_STATEMENT( return FAILURE; )

}

/*-------------------------------------------------------------------------
  CSDemoRecordMessage
  -------------------------------------------------------------------------*/
int CSDemoRecordMessage( unsigned file_type, unsigned message_num )
{
    CTA_EVENT event = { 0 };
    int       ret;

    event.id    = APPEVN_RECORD_MESSAGE;
    event.ctahd = commncx->ctahd;
    event.value = file_type;
    event.size  = message_num;
    event.buffer = NULL;

    ret = ctaQueueEvent( &event );
    if (ret != SUCCESS)
    {
        DemoShowError( "ctaQueueEvent failed", event.value );
        return ret;
    }

    for(;;)
    {
        event.id = 0;
        event.value = 0;
        DemoWaitForEvent( commncx->ctahd, &event );

        switch ( event.id )
        {
            case ADIEVN_CALL_DISCONNECTED:
                DemoReportComment( "Caller hung up." );
                break;

            case APPEVN_RECORD_FAILURE:
                switch ( event.value )
                {
                    case APP_REASON_UNKNOWN_FILE:
                        printf("Error: Unknown file type\n");
                        break;
                    case APP_REASON_FUNCTION_FAILED:
                        printf("Error: DemoRecordMessage failed\n");
                        break;
                    default:
                        printf("Error; Reason Unknown.\n");
                        break;
                }

                /* In Natural Access Client/Server mode, ensure proper release 
                 * of Natural Access owned buffers. */
                if ( event.buffer && (event.size & CTA_INTERNAL_BUFFER) )
                    ctaFreeBuffer( event.buffer );

                return FAILURE;

            case APPEVN_RECORD_DONE:

                /* In Natural Access Client/Server mode, ensure proper release 
                 * of Natural Access owned buffers. */
                if ( event.buffer && (event.size & CTA_INTERNAL_BUFFER) )
                    ctaFreeBuffer( event.buffer );

                switch ( event.value )
                {
                    case APP_REASON_DISCONNECT:
                        return DISCONNECT;
                    default:
                        return SUCCESS;
                }

            default:
                break;
        }
        /* In Natural Access Client/Server mode, ensure proper release of 
         * Natural Access owned buffers. */
        if ( event.buffer && (event.size & CTA_INTERNAL_BUFFER) )
            ctaFreeBuffer( event.buffer );

    }

    UNREACHED_STATEMENT( return FAILURE; )

}

/*-------------------------------------------------------------------------
  CSDemoPromptDigit
  -------------------------------------------------------------------------*/
int CSDemoPromptDigit( CTAHD ctahd, unsigned file_type, unsigned message,
                       char *digit, int maxtries )
{

    int i;
    for( i = 0; i < maxtries; i++ )
    {
        int ret;

        if( CSDemoPlayMessage( file_type, message ) != SUCCESS )
            return FAILURE;

        ret = DemoGetDigits( ctahd, 1, digit );
        if( ret==SUCCESS || ret==DISCONNECT )
            return ret;
    }

    return FAILURE;

}


/*-------------------------------------------------------------------------
  CSDemoPlayList
  -------------------------------------------------------------------------*/
int CSDemoPlayList( unsigned file_type, unsigned message[], unsigned count )
{

    unsigned i;

    for (i = 0; i < count; i++)
    {
        int ret;

        if (ret = CSDemoPlayMessage( file_type, message[i] ) != SUCCESS)
            return ret;
    }
    return SUCCESS;

}

/*******************************************************
 *
 *  SendEvent
 *
 *******************************************************/
void SendEvent( CTAHD ctahd, DWORD evtid, DWORD evtvalue, char *pBuf )
{
    CTA_EVENT event = { 0 };

    event.id = evtid;
    event.ctahd = ctahd;
    event.value = evtvalue;

    if ( pBuf )
    {
         /* Send text buffer with the event */
         event.size = strlen( pBuf ) + 1;
         event.buffer = pBuf;
    }

    if ( ctaQueueEvent( &event ) != SUCCESS )
        DemoShowError( "ctaQueueEvent failed", event.value );

}


