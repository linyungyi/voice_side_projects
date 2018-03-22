/*****************************************************************************
 *  File -  hostp2p.c
 *
 *  Description - Demos a live voice connection using simultanous play and
 *                record with small buffers.
 *
 * Copyright (c) 1997-2002 NMS Communications.  All rights reserved.
 *****************************************************************************/
#define REVISION_NUM  "1"
#define VERSION_NUM   "1"
#define VERSION_DATE __DATE__

/*----------------------------- Includes -----------------------------------*/
#include "ctademo.h"
#ifdef SOLARIS_SPARC
#define stricmp strcasecmp
#endif
/*----------------------------- Application Definitions --------------------*/
typedef struct
{
    CTAQUEUEHD  ctaqueuehd;        /* Natural Access queue handle                 */
    CTAHD       ctahd1;            /* Natural Access context handle               */
    CTAHD       ctahd2;            /* Natural Access context handle               */
    unsigned    lowlevelflag;      /* flag to get low-level events           */
    unsigned    callcount;         /* Track active call state                */
} MYCONTEXT;

/*----------------------- Application Global Data ---------------------------*/
static MYCONTEXT MyContext =
{
    NULL_CTAQUEUEHD, /* Natural Access queue handle   */
    NULL_CTAHD,      /* Natural Access context handle */
    NULL_CTAHD,      /* Natural Access context handle */
    0,               /* lowlevelflag             */
    0                /* callcount                */
};

unsigned   Ag_board    = 0;
unsigned   Ag_board2   = (unsigned)-1;
unsigned   Mvip_stream = 0;
unsigned   Mvip_slot   = 0;
unsigned   Mvip_stream2= 0;
unsigned   Mvip_slot2  = 1;
char      *Protocol1   = "LPS0";
char      *Protocol2   = NULL;
unsigned   Encoding    = ADI_ENCODE_MULAW ;
unsigned   Buftime     = 60;
char      *Digits      = "";
int        Verbose     = 0;
int        Echolen     = 4;
int        Echotime    = 100;

/*----------------------------------------------------------------------------
  PrintHelp()
  ----------------------------------------------------------------------------*/
void PrintHelp( void )
{
    printf( "Usage: hostp2p [opts]\n" );
    printf( "  where opts are:\n" );
    printf( "  -A adi_mgr     Natural Access manager to use for ADI service.    "
            "Default = %s\n", DemoGetADIManager() );
    printf( "  -b n           OAM board number.                      "
            "Default = %d\n", Ag_board );
    printf( "  -B n           Second OAM board number (if different).\n");
    printf( "  -s [strm:]slot 1st DSP [stream and] timeslot.         "
            "Default = %d:%d\n", Mvip_stream, Mvip_slot );
    printf( "  -S [strm:]slot 2nd DSP [stream and] timeslot.         "
            "Default = %d:%d\n", Mvip_stream2, Mvip_slot2 );
    printf( "  -p protocol    Protocol to run.                       "
            "Default = %s\n", Protocol1 );
    printf( "  -P protocol    2nd port's Protocol (if different).\n");
    printf( "  -e n           Encoding                               "
            "Default = %d %s\n", Encoding,
            (Encoding<DemoNumVoiceEncodings)?DemoVoiceEncodings[Encoding]:"" );
    printf( "  -e ?           Display encoding choices\n");
    printf( "  -f n           Buffer size (msec)                     "
            "Default = %d\n", Buftime  );
    printf( "  -d digits      Digits to dial on port 2 (if not NOCC).\n");
    printf( "  -E{len:tim}    Echo cancel length:adapttime           "
            "Default = %d:%d\n",  Echolen, Echotime);
    printf( "  -F filename    Natural Access Configuration File name.     "
            "Default = None\n" );

}

/*----------------------------- Forward References -------------------------*/
void InitializeDemo( MYCONTEXT *cx );
void SetupCalls (MYCONTEXT *cx);
void RunDemo( MYCONTEXT *cx );
int  SpeakDigits( CTAHD ctahd, char *digits );
void CustomRejectCall( CTAHD ctahd );
unsigned compute_bufsize (MYCONTEXT *cx);

/*----------------------------------------------------------------------------
  main
  ----------------------------------------------------------------------------*/
int main( int argc, char **argv )
{
    int        c;
    MYCONTEXT *cx = &MyContext;

    printf( "Natural Access host port to port voice Demo V %s.%s (%s)\n",
            VERSION_NUM, REVISION_NUM,
            VERSION_DATE );

    while( ( c = getopt( argc, argv, "A:d:E:e:F:f:s:S:b:B:p:P:t:lvHh?" ) ) != -1 )
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

            case 'd':
            Digits = optarg;
            break;

            case 's':
            switch (sscanf( optarg, "%d:%d", &value1, &value2 ))
            {
                case 2:
                    Mvip_stream = value1;
                    Mvip_slot   = value2;
                    break;
                case 1:
                    Mvip_slot   = value1;
                    break;
                default:
                    PrintHelp();
                    exit( 1 );
            }
            break;
            case 'S':
            switch (sscanf( optarg, "%d:%d", &value1, &value2 ))
            {
                case 2:
                    Mvip_stream2 = value1;
                    Mvip_slot2   = value2;
                    break;
                case 1:
                    Mvip_slot2   = value1;
                    break;
                default:
                    PrintHelp();
                    exit( 1 );
            }
            break;
            case 'b':
            sscanf( optarg, "%d", &Ag_board );
            break;
            case 'B':
            sscanf( optarg, "%d", &Ag_board2);
            break;
            case 'e':
            if (*optarg != '?')
                sscanf( optarg, "%d", &Encoding );
            else
            {
                unsigned i ;
                for (i=1; i<DemoNumVoiceEncodings; i++)
                    if (*DemoVoiceEncodings[i] != '\0')
                        printf( " %d %s\n", i, DemoVoiceEncodings[i] );
                exit(0) ;
            }
            break;
            case 'E':
            if (sscanf( optarg, "%d:%d", &Echolen, &Echotime ) != 2)
            {
                PrintHelp();
                exit( 1 );
            }
            break;
            case 'f':
            sscanf( optarg, "%d", &Buftime );
            break;
            case 'p':
            Protocol1 = optarg;
            break;
            case 'P':
            Protocol2 = optarg;
            break;
            case 'l':
            cx->lowlevelflag = 1;
            break;
            case 'v':
            Verbose = 1;
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
    if (Ag_board2 == (unsigned)-1)
        Ag_board2 = Ag_board;
    if (Protocol2 == NULL)
        Protocol2 = Protocol1;

    if (Verbose)
        DemoSetReportLevel( DEMO_REPORT_ALL );
    else
        DemoSetReportLevel( DEMO_REPORT_COMMENTS );


    printf( "\tPort #1: Board %d Stream %d Slot %d   Protocol = %s\n",
            Ag_board, Mvip_stream, Mvip_slot, Protocol1);
    printf( "\tPort #2: Board %d Stream %d Slot %d   Protocol = %s\n",
            Ag_board2, Mvip_stream2, Mvip_slot2, Protocol2 );
    printf( "\tEncoding = %d      %s\n",    Encoding,
            (Encoding<DemoNumVoiceEncodings)?DemoVoiceEncodings[Encoding]:"" );
    printf( "\tBuffer time = %d msec\n", Buftime);
    printf( "\tEchocanceling length = %d msec, adapt time= %d msec\n",
            Echolen, Echotime);
    printf("\n");
    InitializeDemo( cx );
    RunDemo( cx );
    ctaDestroyQueue( cx->ctaqueuehd );
    return 0;
}



/*-------------------------------------------------------------------------
  InitializeDemo
  -------------------------------------------------------------------------*/
void InitializeDemo( MYCONTEXT *cx )
{
    CTA_SERVICE_DESC services[] =        /* for ctaOpenServices */
    {   { {"ADI", ""}, { 0 }, { 0 }, { 0 } },
    };

    /* Get Correct ADI Service Manager */
    services[0].name.svcmgrname = DemoGetADIManager();

    /* Get Correct ADI Service Manager */
    services[0].name.svcmgrname = DemoGetADIManager();

    /* Register an error handler to display errors returned from API functions;
     * no need to check return values in the demo.
     */
    ctaSetErrorHandler( DemoErrorHandler, NULL );

    /* Initialize Natural Access, open a call resource, open default services */
    DemoSetup( Ag_board, Mvip_stream, Mvip_slot, &cx->ctaqueuehd, &cx->ctahd1);

    /* Open a 2nd call resource, same queue, new ctahd */
    services[0].mvipaddr.board    = Ag_board2;
    services[0].mvipaddr.stream   = Mvip_stream2;
    services[0].mvipaddr.timeslot = Mvip_slot2;
    services[0].mvipaddr.mode     = ADI_FULL_DUPLEX;

    DemoOpenAnotherPort( 1, "DEMOCONTEXT2", services, ARRAY_DIM(services),
                         cx->ctaqueuehd, &cx->ctahd2 );

    /* start Call control on both ports */
    /* Setup calls (if not NOCC) */
    SetupCalls (cx);
    return;
}


/*-------------------------------------------------------------------------
 *
 *-------------------------------------------------------------------------*/
void SetupCalls (MYCONTEXT *cx)
{
    ADI_START_PARMS stparms;
    ctaGetParms (cx->ctahd1, ADI_START_PARMID, &stparms, sizeof stparms);

    stparms.echocancel.filterlength = Echolen;
    stparms.echocancel.adapttime    = Echotime;
    stparms.echocancel.mode         = ADI_ECHOCANCEL_CUSTOM;

    DemoStartProtocol( cx->ctahd1, Protocol1, NULL ,&stparms );
    DemoStartProtocol( cx->ctahd2, Protocol2, NULL ,&stparms );

    if (stricmp(Protocol1, "NOCC") != 0)
    {
        DemoWaitForCall( cx->ctahd1, NULL, NULL );
        if (DemoAnswerCall( cx->ctahd1, 0 ) != SUCCESS)
            exit (0);
        ++ cx->callcount;
    }

    if (stricmp(Protocol2, "NOCC") != 0)
    {
        if (DemoPlaceCall( cx->ctahd2, Digits, NULL ) != SUCCESS)
            exit (0);
        ++ cx->callcount;
    }
    return ;
}


/*-------------------------------------------------------------------------
 * start async record on both ports.  When the SECOND buffer is returned
 * on a port, start async play on the other port, passing the buffers
 * filled by record.
 *
 * As each buffer is filled by record, pass it to play.  Normally play
 * should consume buffers at exactly the same rate as they are recorded; this
 * could vary if this app falls behind submitting either play or record buffers.
 *
 -------------------------------------------------------------------------*/
void RunDemo( MYCONTEXT *cx )
{
    CTA_EVENT event;
    unsigned bufsize ;
    ADI_RECORD_PARMS recparms ;
    ADI_PLAY_PARMS playparms ;

    void *voice_buffer1;
    void *voice_buffer2;
    void *voice_buffer3;
    void *voice_buffer4;
    unsigned counter1 = 0;   /* total record buffers from port 1 */
    unsigned counter2 = 0;
    unsigned need1 = 0;      /* Play buffers needed from port 1*/
    unsigned need2 = 0;

    void *pending1 = NULL;          /* Pending buffer from record1 to play2 */
    void *pending2 = NULL;          /* Pending buffer from record2 to play1 */

    int playstarted1 = 0;
    int playstarted2 = 0;

    bufsize = compute_bufsize (cx) ;

    /*
     *  Malloc 4 buffers.  (Double buffering in each direction)
     */

    voice_buffer1 = malloc( bufsize ) ;
    voice_buffer2 = malloc( bufsize ) ;
    voice_buffer3 = malloc( bufsize ) ;
    voice_buffer4 = malloc( bufsize ) ;
    if (voice_buffer1 == NULL
        || voice_buffer2 == NULL
        || voice_buffer3 == NULL
        || voice_buffer4 == NULL)
    {
        printf( "failed malloc of %d bytes\n", bufsize );
        exit( -1 );
    }

    /*
     *  Start recording in each direction and submit 1st buffer.  The event
     *  handler will write and submit any subsequent
     *  buffers.
     */

    /* Must disable the beep or ADI library will not allow play */

    ctaGetParms (cx->ctahd1, ADI_RECORD_PARMID, &recparms, sizeof recparms);

    recparms.DTMFabort   = 0 ;
    recparms.novoicetime = 0 ;   /* disable silence detect */
    recparms.silencetime = 0 ;
    recparms.beeptime    = 0 ;

    adiRecordAsync( cx->ctahd1, Encoding, 0, voice_buffer1, bufsize, &recparms);
    adiRecordAsync( cx->ctahd2, Encoding, 0, voice_buffer3, bufsize, &recparms);

    ctaGetParms (cx->ctahd2, ADI_PLAY_PARMID, &playparms, sizeof playparms);
    playparms.DTMFabort = 0 ;

    for (;;)
    {
        /* Pass either ctahd to DemoWaitForEvent - both point to same queue */
        DemoWaitForEvent( cx->ctahd1, &event );

        switch ( event.id )
        {
            case ADIEVN_RECORD_BUFFER_FULL:
                /* If this is the first buffer, save it.
                 * If this is the second buffer, Start play on other side.
                 * Otherwise, if play needs a buffer pass it; if play does not
                 * need one, make this the pending buffer.
                 */

                if (event.ctahd == cx->ctahd1)
                {
                    if (counter1 < 2)
                    {
                        if (++counter1 == 2)
                        {
                            /* Have 2 record buffers, start play. Pass the
                             * first record buffer, save this one for next play
                             * request (forthcoming).
                             */
                            adiPlayAsync( cx->ctahd2, Encoding, pending1,
                                          event.size, 0, &playparms );
                            pending1 = event.buffer;
                            need1 = 1;   /* play needs one more buffer */
                        }
                        else  /* first record buffer*/
                        {
                            pending1 = event.buffer;
                        }
                    }
                    else  /* fully started */
                    {
                        if (need1 != 0 && playstarted1)
                        {
                            adiSubmitPlayBuffer( cx->ctahd2,
                                                 event.buffer, bufsize, 0 );
                            -- need1 ;
                        }
                        else  /* play is full or not started - hang on to buf */

                            pending1 = event.buffer;
                    }
                }
                else  /* same code, other direction */
                {
                    if (counter2 < 2)
                    {
                        if (++counter2 == 2)
                        {
                            adiPlayAsync( cx->ctahd1, Encoding, pending2,
                                          event.size, 0, &playparms );
                            pending2 = event.buffer;
                            need2 = 1;
                        }
                        else
                        {
                            pending2 = event.buffer;
                        }
                    }
                    else
                    {
                        if (need2 != 0 && playstarted2)
                        {
                            adiSubmitPlayBuffer( cx->ctahd1,
                                                 event.buffer, bufsize, 0 );
                            -- need2 ;
                        }
                        else
                            pending2 = event.buffer;
                    }
                }
                /* re-Submit the buffer for record. We assume that it will be
                 * played before it is recorded!   (It might be safer to have
                 * more buffers).
                 */
                if (event.value & ADI_RECORD_UNDERRUN)
                    printf("record underrun\n");
                adiSubmitRecordBuffer( event.ctahd, event.buffer, bufsize);
                break;


            case ADIEVN_RECORD_STARTED:

                /* Submit 2nd buffer */
                adiSubmitRecordBuffer( event.ctahd,
                                       event.ctahd == cx->ctahd1 ?
                                       voice_buffer2 :
                                       voice_buffer4,
                                       bufsize);
                break;

            case ADIEVN_PLAY_BUFFER_REQ:
                if (event.ctahd == cx->ctahd2)
                {
                    if (pending1 != NULL)
                    {
                        adiSubmitPlayBuffer( event.ctahd, pending1, bufsize, 0);
                        pending1 = NULL ;
                    }
                    else
                        ++need1;
                    playstarted1 = 1;
                }
                else  /* same code, other direction */
                {
                    if (pending2 != NULL)
                    {
                        adiSubmitPlayBuffer( event.ctahd, pending2, bufsize, 0);
                        pending2 = NULL ;
                    }
                    else
                        ++need2;
                    playstarted2 = 1;
                }

                if (event.value & ADI_PLAY_UNDERRUN)
                    printf("play underrun\n");
                break;


            case ADIEVN_RECORD_DONE:
                if (event.value != CTA_REASON_RELEASED)
                {
                    if (event.value == CTAERR_FUNCTION_NOT_AVAIL)
                    {
                        static int warned= 0;
                        if (!warned)
                            puts ("DSP Function not found.  Note: Default"
                                  " encoding requires RVOICE.DSP");
                        warned = 1;
                    }
                    else
                    {
                        puts("Unexepected RECORD DONE event");
                        if (!Verbose)
                            DemoShowEvent ( &event) ;
                    }
                }

                /* It's safe to free the record buffers now that the record
                 * function is stopped.  (Don't need to worry about play,
                 * buffers are copied synchronously by the play functions.)
                 */
                if (event.ctahd == cx->ctahd1)
                {
                    puts ("Calling party disconnected");

                    free( voice_buffer1 );
                    free( voice_buffer2 );

                    /* Hang up the other side (if not NOCC)*/
                    if (stricmp(Protocol2, "NOCC") != 0)
                        adiReleaseCall( cx->ctahd2 );
                }
                else
                {
                    puts ("Called party disconnected");

                    free( voice_buffer3 );
                    free( voice_buffer4 );

                    /* Hang up the other side (if not NOCC)*/
                    if (stricmp(Protocol1, "NOCC") != 0)
                        adiReleaseCall( cx->ctahd1 );
                }
                break;

            case ADIEVN_PLAY_DONE:
                if (event.value != CTA_REASON_RELEASED)
                {
                    printf("Unexepected PLAY DONE event");
                    if (!Verbose)
                        DemoShowEvent ( &event) ;
                }
                break;

            case ADIEVN_CALL_DISCONNECTED:
                adiReleaseCall( event.ctahd );
                break;

            case ADIEVN_CALL_RELEASED:
                printf("RELEASED, count %d\n",cx->callcount);
                if (--cx->callcount == 0)
                    return ;
                break;

            default:
                if (!Verbose)
                    DemoShowEvent ( &event) ;
                break;
        }
    }
    UNREACHED_STATEMENT( return; )
}


/*-------------------------------------------------------------------------
 * Compute the AG buffer size for the selected encoding and buffer time
 *-------------------------------------------------------------------------*/
unsigned compute_bufsize (MYCONTEXT *cx)
{
    unsigned  framesize;
    unsigned  datarate ;
    unsigned  frametime ;
    unsigned  nframes;

    adiGetEncodingInfo( cx->ctahd1, Encoding, &framesize, &datarate, NULL );

    /* compute frame time in msec */
    frametime = (framesize*1000) / datarate ;

    /* Round up requested buf time to whole frames */
    nframes = ((Buftime -1) / frametime) + 1;

    /* compute buf size needed */
    return nframes * framesize ;
}
