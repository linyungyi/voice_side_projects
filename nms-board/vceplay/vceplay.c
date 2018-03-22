/***************************************************************************
*  File - vceplay.c
*
*  Description - A utility to play voice files.
*      Demonstrates using the voice service to play messages in voice files.
*      User interface from keyboard or Touch-tones allows control of
*      pause/resume, forward/rewind, louder/softer and faster/slower.
*
* Copyright (c) 1997-2001 NMS Communications.  All rights reserved.
***************************************************************************/
#define REVISION_NUM  "15"
#define VERSION_NUM   "1"
#define REVISION_DATE __DATE__

/*----------------------------- Includes -----------------------------------*/
#include <stdio.h>
#include <string.h>

#if defined (WIN32)
  #include <conio.h>
  #define GETCH() getch()
  #define STRICMP stricmp
#elif defined (UNIX)
  #include <unistd.h>
  #include <termio.h>
  #define GETCH() getchar()
  #define STRICMP strcasecmp
#endif

#include "ctademo.h"
#include "ctadef.h"
#include "vcedef.h"

/*----------------------------------------------------------------------------
                            options
----------------------------------------------------------------------------*/
unsigned   Ag_board    = 0;
unsigned   Mvip_stream = 0;
unsigned   Mvip_slot   = 0;
unsigned   Encoding    = 0;
char      *Protocol    = "lps0";
unsigned   Skiptime    = 2;
unsigned   Maxspeed    = 100;
unsigned   Filetype    = VCE_FILETYPE_AUTO;
unsigned   Startmsg    = 0;
unsigned   Endmsg      = 0xffff;
unsigned   Speedchange = 20;
int        Gain        = 0;
unsigned   Speed       = 100;


#define MINSPEED 40

#define MINGAIN  (-18)
#define MAXGAIN   18


/*----------------------------------------------------------------------------
                            PrintHelp()
----------------------------------------------------------------------------*/
void PrintHelp( void )
{
    printf( "Usage: vceplay [opts] {filename ...}\n" );
    printf( "  where opts are:\n" );
    printf( "  -b n           OAM board number.                 "
            "Default = %d\n", Ag_board );
    printf( "  -e n           Encoding                          "
            "Default = %d %s\n", Encoding,
          Encoding < DemoNumVoiceEncodings ? DemoVoiceEncodings[Encoding] : "");
    printf( "  -e ?           Display encoding choices\n");
    puts(   "  -f type        File type: VOX, WAV or VCE.       "
            "Default = infer from filename");
    printf( "  -g gain        Initial Gain.                     "
            "Default = %d dB\n", Gain );
    printf( "  -m start[:end] Starting/ending message.          "
            "Default = all messages\n" );
    printf( "  -p protocol    Protocol to run                   "
            "Default = %s\n", Protocol);
    printf( "  -s [strm:]slot DSP [stream and] timeslot.        "
            "Default = %d:%d\n", Mvip_stream, Mvip_slot );
    printf( "  -y maxspeed    Maximum Speed.                    "
            "Default = %d%\n", Maxspeed );
    printf( "  -z speed       Initial Speed.                    "
            "Default = %d%\n", Speed );
    printf( "  -A adi_mgr     Natural Access ADI manager to use."
            "Default = %s\n", DemoGetADIManager() );
    printf( "  -F filename    Natural Access Configuration File."
            "Default = None\n");
    puts("");
    puts("Touch-Tone or keyboard commands:");
    puts(   "\t1: replay current or go to previous message");
    puts(   "\t2: next message");
    puts(   "\t3: next file");
    printf( "\t4: back up %d seconds\n", Skiptime );
    puts(   "\t5: pause/resume");
    printf( "\t6: skip forward %d seconds\n", Skiptime );
    printf( "\t7: Slow down %d percent\n", Speedchange);
    puts(   "\t8: Softer 3db");
    printf( "\t9: Speed up %d percent\n", Speedchange);
    puts(   "\t0: Louder 3db");
    puts(   "\t#: quit playing file");
    puts(   "\t*: show this menu");
    puts("These commands work while playing or paused.\n");
    return ;
}

/*----------------------------------------------------------------------------
                            ShortHelp()
----------------------------------------------------------------------------*/
void ShortHelp( void )
{
    puts(
"1 Prev msg   2 Next msg   3 Next file   4 Back   5 Pause   6 Fwd\n"
"7 Slower  8 Softer   9 Faster  0 Louder  * This help   # Quit\n");
    return ;
}


/*----------------------------- Application Definitions --------------------*/
typedef struct
{
    CTAHD             ctahd;           /* what we initialize with OpenPort */
    CTAQUEUEHD        ctaqhd;
    VCE_PLAY_PARMS    playparms;
    ADI_COLLECT_PARMS digitparms;
    char            **filename ;       /* Ptr to list of filenames in argv */
    unsigned          filecount ;
    int               paused;
} MYCONTEXT;
MYCONTEXT MyContext = {0} ;

/*----------------------------- Forward References ------------------------*/
void RunDemo( MYCONTEXT *cx );
int SmartPlay (MYCONTEXT *cx, VCEHD vh, unsigned message, unsigned msgsize);
unsigned PlayFile (MYCONTEXT *cx, char *filename);

/*----------------------------------------------------------------------------
                                main
----------------------------------------------------------------------------*/
int main( int argc, char **argv )
{
    int c;
    MYCONTEXT *cx = &MyContext;

    /* Configure Raw Mode, before any printfs */
    DemoSetupKbdIO(1);

    printf( "Natural Access Play file utility V %s.%s (%s)\n",
                                    VERSION_NUM, REVISION_NUM, REVISION_DATE );

    while( ( c = getopt( argc, argv, "A:e:F:f:g:m:s:b:p:y:z:Hh?t" ) ) != -1 )
    {
        switch( c )
        {
                unsigned   value1, value2;

            case 'A':
                DemoSetADIManager( optarg );
                break;
            case 'b':
                sscanf( optarg, "%d", &Ag_board );
                break;
            case 'f':
                Filetype = DemoGetFileType(optarg);
                break;
            case 'F':
                DemoSetCfgFile( optarg );
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
            case 'g':
                sscanf( optarg, "%d", &Gain );
                break;
            case 'm':
                switch (sscanf( optarg, "%d:%d", &Startmsg, &Endmsg ))
                {
                    case 2:
                        break;
                    case 1:
                        Endmsg = Startmsg;
                        break;
                    default:
                        PrintHelp();
                        exit( 1 );
                }
                break;
            case 'p':
                Protocol = optarg;
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
            case 'y':
                sscanf( optarg, "%d", &Maxspeed);
                break;
            case 'z':
                sscanf( optarg, "%d", &Speed);
                break;
            case 'h':
            case 'H':
            case '?':
                PrintHelp();
                exit( 1 );
                break;


            default:
                printf("? Unrecognizable switch %s\n", optarg );
                exit( 1 );
        }
    }
    if (optind == argc)
    {
        printf("filename required\n");

        /* Put back old keyboard mode */
        DemoSetupKbdIO(0);

        exit( 1 );
    }
    cx->filename  = &argv[optind] ;     /* pointer to array of filenames */
    cx->filecount = argc-optind ;

    printf( "\tEncoding   = %d %s\n", Encoding,
            (Encoding<DemoNumVoiceEncodings)?DemoVoiceEncodings[Encoding]:"" );
    printf( "\tBoard      = %d\n",    Ag_board );
    printf( "\tStream:Slot= %d:%d\n", Mvip_stream, Mvip_slot );
    printf( "\tProtocol   = %s\n",    Protocol );
    printf( "\tMax Speed  = %d\n",    Maxspeed );

    RunDemo( cx );
    return 0;
}



/*-------------------------------------------------------------------------
                            ErrorHandler
-------------------------------------------------------------------------*/
DWORD NMSSTDCALL ErrorHandler( CTAHD ctahd, DWORD errorcode,
                                               char *errortext, char *func )
{
    if (errorcode != VCEERR_INVALID_MESSAGE
     && errorcode != CTAERR_FILE_NOT_FOUND
     && errorcode != CTAERR_BAD_ARGUMENT)
    {
        printf( "\007Error in %s: %s (%#x)\n", func, errortext, errorcode );
        exit( errorcode );
    }
    return errorcode;
}


/*-------------------------------------------------------------------------
                            RunDemo
-------------------------------------------------------------------------*/
void RunDemo( MYCONTEXT *cx )
{
    unsigned i;

     /* Turn off display of events and function comments */
    DemoSetReportLevel(DEMO_REPORT_UNEXPECTED);

    /* Register an error handler to display errors returned from API functions;
     * no need to check return values in the demo.
     */
    ctaSetErrorHandler( ErrorHandler, NULL );

    /* Open a port, which maps to a telephone line interface.
     * On this port, we'll accept calls and interact with the caller.
     */
    DemoSetup(Ag_board, Mvip_stream, Mvip_slot, &cx->ctaqhd, &cx->ctahd);

    /* Initiate a protocol on the port so we can accept calls. */
    DemoStartProtocol( cx->ctahd, Protocol, NULL, NULL );

    /* if the protocol isn't nocc, wait for a call */
    if( STRICMP( Protocol, "nocc" ) != 0)
    {
        puts("\n"
             "Waiting for call...\n"
             "\n");
        DemoWaitForCall( cx->ctahd, NULL, NULL );

        if( DemoAnswerCall( cx->ctahd, 1 ) != SUCCESS )
        {
            puts( "Incoming call abandoned; retry." );
            exit (0);
        }
    }

    ShortHelp();

    adiFlushDigitQueue( cx->ctahd );

    /* Flush keyboard */
    while (kbhit())
    {
        char c = GETCH();
    }

    Speed = MIN (Speed, Maxspeed) ;
    Gain = MIN (Gain, MAXGAIN) ;

    Speed = MAX (Speed, MINSPEED) ;
    Gain = MAX (Gain, MINGAIN) ;


    vceSetPlaySpeed (cx->ctahd, Speed);
    vceSetPlayGain (cx->ctahd, Gain);

    ctaGetParms( cx->ctahd, VCE_PLAY_PARMID,
                 &cx->playparms, sizeof cx->playparms );
    cx->playparms.gain     = VCE_CURRENT_VALUE;
    cx->playparms.speed    = VCE_CURRENT_VALUE;
    cx->playparms.maxspeed = Maxspeed;

    ctaGetParms( cx->ctahd, ADI_COLLECT_PARMID,
                 &cx->digitparms, sizeof cx->digitparms );
    cx->digitparms.firsttimeout = 15000 ;
    cx->digitparms.intertimeout = 15000 ;
    cx->digitparms.validDTMFs   = ADI_DIGIT_ANY ;
    cx->digitparms.terminators  = 0;

    /* Play all messages in each file */
    for (i=0;i<cx->filecount; i++)
        if (PlayFile (cx, cx->filename[i]) == 0)
            break;

    /* hangup if necessary */
    if( STRICMP( Protocol, "nocc" ) != 0 )
        /* don't wait for caller to hang up */
        adiReleaseCall( cx->ctahd );

    /* All done - synchronously close services, destroy context, destroy queue*/
    ctaDestroyQueue (cx->ctaqhd);

    /* Put back old keyboard mode */
    DemoSetupKbdIO(0);


    exit (0);
}


/*-------------------------------------------------------------------------
PlayFile - play all messages in one file.

Returns - 0 to exit, 1 to continue to the next file, -1 if error .
-------------------------------------------------------------------------*/
unsigned PlayFile (MYCONTEXT *cx, char *filename)
{
    unsigned filetype = 0;
    VCEHD    vh ;
    unsigned message, highmessage;
    unsigned msgsize;
    int      retcode;
    unsigned first ;
    int     back ;
    unsigned ret;
    char     fullname [CTA_MAX_PATH];

    DemoDetermineFileType (filename, 0 /* don't prepend "./" */,
                                                 fullname, &Filetype) ;

    /* use the user-provided file type if one is specified */
    if (Filetype != VCE_FILETYPE_AUTO)
        filetype = Filetype ;

    /* If filetype is "flat" and encoding not specified, default to NMS24 */
    if (filetype == VCE_FILETYPE_FLAT && Encoding == 0)
        Encoding = VCE_ENCODE_NMS_24 ;

    /* use "Startmsg, Endmsg" parameters only for VOX files */
    if (filetype != VCE_FILETYPE_VOX)
    {
        Startmsg = 0;
        Endmsg = 0;
    }

    printf ("\nfile: %s\n", fullname);

    if ((ret=vceOpenFile(cx->ctahd, fullname, filetype, VCE_PLAY_ONLY, Encoding,
       &vh)) != SUCCESS)
    {
         /* Assume file-not-found. Note that other errors will be caught by
          * the error handler.
          */
        puts ("    *** Unable to open file");
        return 1;
    }


     /* Play each message message in the file */
    vceGetHighMessageNumber (vh, &highmessage) ;
    if (highmessage == VCE_UNDEFINED_MESSAGE)
        printf("empty voice file");
    else
    {
        highmessage = MIN (highmessage, Endmsg);
        message = Startmsg;
        first = VCE_UNDEFINED_MESSAGE ;
        back  = FALSE ;
        while (message <= highmessage)
        {
            if (back)
                if (message == 0)
                    message = first;
                else
                    --message;

            if (vceGetMessageSize(vh, message, &msgsize) != SUCCESS
                                                               || msgsize == 0)
            {
                if (!back)
                    ++ message;
                continue;
            }
            if (first == VCE_UNDEFINED_MESSAGE)
                first = message;
            printf("\rMessage %3d %s", message, cx->paused ?
                                                "*PAUSED*" : "        ");
            retcode = SmartPlay (cx, vh, message, msgsize);
            back = FALSE ;
            switch (retcode)
            {
                case -1 :
                case 0 :
                    vceClose (vh);
                    return retcode;           /* exit */
                case 1:
                    back = TRUE ;             /* goto previous message */
                    break;
                case 2:
                    ++message;               /* goto next message */
                    break;
                case 3:
                    message = highmessage+1; /* done with file */
                    break;
            }
        }
    }
    vceClose (vh);
    return 1;
}


/*-------------------------------------------------------------------------
  SmartPlay - this function plays a specified message with rewind/pause/
  forward, repeat, louder/softer and faster/slower actions.

  It returns one of the following:
   0 - all done
   1 - go to previous message
   2 - go to next message
   3 - go to next file

-------------------------------------------------------------------------*/
int SmartPlay (MYCONTEXT *cx, VCEHD vh, unsigned message, unsigned msgsize)
{

    VCE_CONTEXT_INFO info;
    int              position=0;
    CTA_EVENT        event;
    char             scratch[80];
    char             key ;

    vceSetCurrentMessage (vh, message);

    for (;;)
    {
        key = '\0';
        if (!cx->paused)
        {
            vcePlay(cx->ctahd, VCE_NO_TIME_LIMIT, &cx->playparms);
            do {
                ctaWaitEvent( cx->ctaqhd, &event, 100 );
                if (event.id == CTAEVN_WAIT_TIMEOUT)
                {
                    if (kbhit())
                    {
                        vceStop(cx->ctahd);
                        key = GETCH();
                    }
                    else
                        continue;
                }
            } while ( event.id != VCEEVN_PLAY_DONE );


            switch ( event.value )
            {
                case CTA_REASON_FINISHED:
                    return 2 ;

                case CTA_REASON_RELEASED:
                    return 0 ;

                case CTA_REASON_DIGIT:
                    adiGetDigit (cx->ctahd, &key) ;
                    break;

                default:
                    if (event.value != CTA_REASON_STOPPED)
                    {
                        ctaGetText(cx->ctahd, event.value,
                                   scratch, sizeof(scratch));
                        printf("Play failed (error=%s)\n", scratch);
                        return -1;
                    }
                    break;
            }
        }
        else  /*paused*/
        {
            char             digitbuf[2] ;
            adiCollectDigits (cx->ctahd, digitbuf, 1, &cx->digitparms) ;
            do
            {
                ctaWaitEvent( cx->ctaqhd, &event, 100 );
                if (event.id == CTAEVN_WAIT_TIMEOUT)
                {
                    if (kbhit())
                    {
                        adiStopCollection(cx->ctahd);
                        key = GETCH();
                    }
                    else
                        continue;
                }
            } while ( event.id != ADIEVN_COLLECTION_DONE );

            if (event.value == CTA_REASON_RELEASED)
                return 0 ;
            else if (event.value == CTA_REASON_TIMEOUT)
                continue;
            else if ( event.value == CTA_REASON_FINISHED)
                key = digitbuf[0] ;

            else if (event.value != CTA_REASON_STOPPED)
            {
                ctaGetText(cx->ctahd, event.value, scratch, sizeof scratch);
                printf("Digit Collection failed (error=%s)\n", scratch);
                return -1;
            }
        }

        switch ( key )
        {
            case '1':
                  /* go to previous message if
                   *  - currently paused and at the beginning
                   *  - or current play started at beginning and less than
                   *      ??ms elapsed
                   */
                vceGetContextInfo (cx->ctahd, &info, sizeof info);
                if (cx->paused && info.position == 0
                 || !cx->paused && info.position < 1000)
                    return 1 ;
                else
                    vceSetPosition(cx->ctahd, 0, VCE_SEEK_SET, NULL);
                break;

            case '2':
                return 2;   /* goto next message */

            case '3':
                return 3;   /* goto next file */

            case '4':           /* skip backward */
                vceSetPosition(cx->ctahd, -(int)Skiptime*1000, VCE_SEEK_CUR,
                                                                         NULL);
                break;

            case '5':       /* pause/resume */
                cx->paused = !cx->paused ;
                if( cx->paused )
                {
                    vceStop(cx->ctahd);
                    printf("\rMessage %3d *PAUSED*", message);
                }
                else
                    printf("\rMessage %3d         ", message);
                fflush (stdout);
                break;

            case '6':       /* skip forward */
                vceSetPosition(cx->ctahd, Skiptime*1000, VCE_SEEK_CUR, NULL);
                break;

            case '7':       /* Slower */
                Speed = MAX (Speed-Speedchange, MINSPEED) ;
                vceSetPlaySpeed (cx->ctahd, Speed);
                break;

            case '8':       /* Softer */
                Gain = MAX (Gain-6, MINGAIN) ;
                vceSetPlayGain (cx->ctahd, Gain);
                break;

            case '9':       /* faster */
                Speed = MIN (Speed+Speedchange, Maxspeed) ;
                vceSetPlaySpeed (cx->ctahd, Speed);
                break;

            case '0':       /* louder */
                Gain = MIN (Gain+6, MAXGAIN) ;
                vceSetPlayGain (cx->ctahd, Gain);
                break;

            case '*':       /* Print help */
                puts ("");
                ShortHelp() ;
                break;

            case '#':       /* Exit */
            case '\033':    /* <esc> = Exit */
                return 0;

            default:
                break;
        }
    }
    UNREACHED_STATEMENT( return 0; )
}

