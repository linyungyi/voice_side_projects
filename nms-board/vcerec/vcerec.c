/***************************************************************************
*  File - vcerec.c
*
*  Description - demo/utility to record voice file(s).
*     This utility records one or more messages to a voice file. You
*     can Use the keyboard or touch tones to control recording.
*
* Copyright (c) 1997-2001 NMS Communications.  All rights reserved.
***************************************************************************/

#define REVISION_NUM  "11"
#define VERSION_NUM   "1"
#define REVISION_DATE __DATE__

/*----------------------------- Includes -----------------------------------*/
#include <stdio.h>
#include <string.h>
#include <limits.h>

#if defined (WIN32)
  #include <conio.h>
  #define GETCH() getch()
  #define STRICMP stricmp
  #include <io.h>
#elif defined (UNIX)
  #include <unistd.h>
  #include <termio.h>
  #define GETCH() getchar()
  #define STRICMP strcasecmp
#endif

#include "ctademo.h"
#include "ctadef.h"
#include "adidef.h"
#include "vcedef.h"

/*----------------------------------------------------------------------------
                            options
----------------------------------------------------------------------------*/
unsigned   Ag_board    = 0;
unsigned   Mvip_stream = 0;
unsigned   Mvip_slot   = 0;
unsigned   Encoding    = 0;
char*      Protocol    = "lps0";
unsigned   Skiptime    = 2;
unsigned   Maxspeed    = 200 ;
unsigned   Filetype    = VCE_FILETYPE_AUTO ;
unsigned   Startmsg    = 0 ;
unsigned   Endmsg      = 0 ;
unsigned   Maxidx      = VCEVOX_DFLTIDX ;
unsigned   Maxmsec     = 0 ;
unsigned   Clobber     = 0 ;
int        Gain        = INT_MAX ;


/*----------------------------------------------------------------------------
                            PrintHelp()
----------------------------------------------------------------------------*/
void PrintHelp( void )
{
    printf( "Usage: vcerec [opts] {filename}\n" );
    printf( "  where opts are:\n" );
    printf( "  -A adi_mgr  Natural Access manager to use for ADI service. "
            "Default = %s\n", DemoGetADIManager() );           
    printf( "  -F filename Natural Access Configuration File name.  "
            "Default = None\n" );
    printf( "  -b n        OAM board number.                   "
            "Default = %d\n", Ag_board );
    printf( "  -e n        Encoding                            "
            "Default = %d %s\n",
            Encoding,
            (Encoding<DemoNumVoiceEncodings)?DemoVoiceEncodings[Encoding]:"" );
    puts(   "  -e ?        Display encoding choices");
    puts(   "  -f type     File type: VOX, WAV or VCE.         "
            "Default = infer from filename");
    puts(   "  -g gain     Record gain in decibels.            "
            "Default = 0.");
    printf( "  -i nn       Number of indices in a newly created VOX file "
            "(48 to 6400).\n"
            "              The default is %d.\n", Maxidx);
    puts(   "  -m start[:end]\n"
            "              Record messages beginning at <start> and ending "
            "with <end>." );
    puts(   "              If end is not specified, it defaults to <start>.");
    puts(   "  -p protocol Protocol to run.                    "
            "Default = lps0" );
    puts(   "  -r nnn      Maximum nnn seconds per message.    "
            "Default = unlimited." );
    printf( "  -s [strm:]slot\n"
            "              DSP [stream and] timeslot.          "
            "Default = %d:%d\n", Mvip_stream, Mvip_slot );
    puts(   "  -x          Overwrite existing destination file." );
    puts(   "              Default is to merge into existing file, if any." );

    puts("");
    puts("Touch-Tone or keyboard commands:");
    puts(   "\t1: re-record");
    puts(   "\t2: next message");
    puts(   "\t5: pause/resume");
    puts(   "\t# or <ESC>: quit");
    return ;
}

/*----------------------------------------------------------------------------
                            ShortHelp()
----------------------------------------------------------------------------*/
void ShortHelp( void )
{
    if( Endmsg > Startmsg )
    {
        puts( "อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ");
        puts( "  Touch-Tone/Keyboard Commands:");
        puts( "       1: Restart message  2: Next message  5: Pause/Resume   "
              "#/ESC: Quit.\n" );
        puts( "อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ");
    }
    else if( Endmsg == Startmsg )
    {
        puts( "อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ");
        puts( "  Touch-Tone/Keyboard Commands:");
        puts( "       1: Restart message   5: Pause/Resume   #/ESC: Quit.\n" );
        puts( "อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ");
    }

    return ;
}


/*----------------------------- Application Definitions --------------------*/
ADI_COLLECT_PARMS Digitparms;
VCE_RECORD_PARMS  Recordparms;


static CTAHD      Ctahd ;
static CTAQUEUEHD Ctaqhd ;


/*----------------------------- Forward References ------------------------*/
void Setup(unsigned,unsigned,unsigned,CTAQUEUEHD *,CTAHD *);
int GetOptions (int argc, char *argv[]);
void RunDemo( char *filename );
DWORD OpenRecordFile (CTAHD ctahd, char *filename, unsigned filetype,
           unsigned encoding, int deleteifexists, void *createinfo, VCEHD *vh);
void RecordFile (char *filename);
int  RecordMessage (VCEHD vh, unsigned message) ;
void doexit (int exitcode);
char *getErrorText(DWORD errcode );

/*----------------------------------------------------------------------------
                                main
----------------------------------------------------------------------------*/
int main( int argc, char **argv )
{
    char *filename = NULL ;
    char fullname [CTA_MAX_PATH];

    /* Configure Raw Mode, before any printfs */
    DemoSetupKbdIO(1);

    printf( "Natural Access Record file utility V %s.%s (%s)\n",
                                    VERSION_NUM, REVISION_NUM, REVISION_DATE );

    if (argc == 1)
    {
        PrintHelp ();
        /* Put back old keyboard mode */
        DemoSetupKbdIO(0);
        exit (0);
    }

    if (argv[1][0] != '-' && argv[1][0] != '/')
    {
         /* File name first */
        if (argc >= 2)
        {
            filename  = argv[1] ;
            (void) GetOptions (argc-1, argv+1);
        }
    }
    else
    {
         /* File name last */
        int filearg ;
        if ((filearg = GetOptions (argc, argv)) == argc-1)
            filename = argv[filearg] ;
    }
    if (filename == NULL)
    {
        printf("filename required\n");
        /* Put back old keyboard mode */
        DemoSetupKbdIO(0);
        exit( 1 );
    }
    DemoDetermineFileType (filename, 1 /* prepend "./" */, fullname, &Filetype);

    if (Endmsg < Startmsg)
        Endmsg = Startmsg ;

    if ((Endmsg != Startmsg) && (Filetype != VCE_FILETYPE_VOX))
    {
        printf("Multiple messages may be specified only for NMS VOX files\n");
        exit (1);
    }
    printf( "\tBoard      = %d\n",    Ag_board );
    printf( "\tStream:Slot= %d:%d\n", Mvip_stream, Mvip_slot );
    printf( "\tProtocol   = %s\n",    Protocol );
    printf( "\tStartmsg   = %d\n",    Startmsg );
    printf( "\tEndmsg     = %d\n",    Endmsg );

    RunDemo( fullname );
    return 0;
}

/*-------------------------------------------------------------------------
                            GetOptions
-------------------------------------------------------------------------*/
int GetOptions (int argc, char *argv[])
{
    int        c;
    char       junk[256];

    while ( (c=getopt(argc, argv, "A:b:e:F:f:g:i:I:m:M:p:r:R:s:xXHh?tT")) != -1)
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
        case 'e':
            if (*optarg != '?')
                sscanf( optarg, "%d", &Encoding );
            else
            {
                unsigned i ;
                for (i=1; i<DemoNumVoiceEncodings; i++)
                    if (*DemoVoiceEncodings[i] != '\0')
                        printf( " %d %s\n", i, DemoVoiceEncodings[i] );
                /* Put back old keyboard mode */
                DemoSetupKbdIO(0);
                exit(0) ;
            }
            break;
        case 'f':
            Filetype = DemoGetFileType(optarg);
            break;
        case 'F':
            DemoSetCfgFile( optarg );
            break;

        case 'g':
            sscanf( optarg, "%d", &Gain );
            break;

        case 'i':
        case 'I':
            sscanf( optarg, "%d", &Maxidx );
            break;

        case 'm':
        case 'M':
            if( sscanf( optarg, "%d:%d%s", &Startmsg, &Endmsg, junk ) != 2
             && sscanf( optarg, "%d,%d%s", &Startmsg, &Endmsg, junk ) != 2
             && sscanf( optarg, "%d%s", &Startmsg, junk ) != 1 )
            {
                PrintHelp();
                /* Put back old keyboard mode */
                DemoSetupKbdIO(0);
                exit( 1 );
            }
            break;
        case 'p':
            Protocol = optarg;
            break;
        case 'r':
        case 'R':
            sscanf( optarg, "%d", &Maxmsec );
            Maxmsec *= 1000;
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
                /* Put back old keyboard mode */
                DemoSetupKbdIO(0);
                exit( 1 );
            }
            break;
        case 'x':
        case 'X':
            Clobber = 1;
            break;


        case 'h':
        case 'H':
        case '?':
            PrintHelp();
            /* Put back old keyboard mode */
            DemoSetupKbdIO(0);
            exit( 1 );
            break;
        default:
            printf("? Unrecognizable switch %s\n", optarg );
            /* Put back old keyboard mode */
            DemoSetupKbdIO(0);
            exit( 1 );
        }
    }
    return optind;
}


/*-------------------------------------------------------------------------
                            ErrorHandler
-------------------------------------------------------------------------*/
DWORD NMSSTDCALL ErrorHandler( CTAHD crhd, DWORD errorcode,
                                               char *errortext, char *func )
{
    if ((errorcode != CTAERR_FILE_NOT_FOUND) &&
        (errorcode != CTAERR_FILE_EXISTS) &&
        ! ((errorcode == VCEERR_WRONG_FILE_TYPE)
           && (strcmp(func, "vceReadMessageText") == 0)))
    {
        printf( "\007Error in %s: %s (%#x)\n", func, errortext, errorcode );
        /* Put back old keyboard mode */
        DemoSetupKbdIO(0);
        exit( errorcode );
    }
    return errorcode;
}


/*-------------------------------------------------------------------------
                            RunDemo
-------------------------------------------------------------------------*/
void RunDemo( char *filename )
{
     /* Turn off display of events and function comments */
    DemoSetReportLevel(DEMO_REPORT_UNEXPECTED);

    /* Register an error handler to display errors returned from API functions;
     * no need to check return values in the demo.
     */
    ctaSetErrorHandler( ErrorHandler, NULL );

    /* Open a port, which maps to a telephone line interface.
     * On this port, we'll accept calls and interact with the caller.
     */
    DemoSetup (Ag_board, Mvip_stream, Mvip_slot, &Ctaqhd, &Ctahd);

    /* Initiate a protocol on the port so we can accept calls. */
    DemoStartProtocol( Ctahd, Protocol, NULL, NULL );

    /* if we're not running nocc, wait for a phone call */
    if( STRICMP( "nocc", Protocol ) != 0 )
    {
        puts("\n"
             "Waiting for call...\n");
        DemoWaitForCall( Ctahd, NULL, NULL );

        if( DemoAnswerCall( Ctahd, 1 ) != SUCCESS )
        {
            puts( "Incoming call abandoned; retry." );
            doexit (0);
        }
    }

    adiFlushDigitQueue( Ctahd );
    /* Flush the keyboard */

    while ( kbhit() )
    {
        char c = GETCH();
    }

    ctaGetParms( Ctahd, ADI_COLLECT_PARMID, &Digitparms, sizeof Digitparms);
    Digitparms.firsttimeout = 15000 ;
    Digitparms.intertimeout = 15000 ;
    Digitparms.validDTMFs   = ADI_DIGIT_ANY ;
    Digitparms.terminators  = 0;

    ctaGetParms( Ctahd, VCE_RECORD_PARMID, &Recordparms, sizeof Recordparms);
    if (Gain != INT_MAX)
    {
        Recordparms.gain = Gain;
    }

    RecordFile (filename) ;

    /* All done - synchronously close services, destroy context, destroy queue*/
    doexit (0);
}


/*-------------------------------------------------------------------------
openRecordFile - Create or open a voice file for recording

       Function arguments:
              ctahd    - Natural Access context
              fullname - complete file name
              filetype - VCE_FILETYPE_xxx
              encoding - The encoding to create, or expected encoding if the
                          file exists (default is determined from existing
                          file if possible, otherwise is NMS_24).
              deleteifexists - if 1 and file exists, then delete it first
              createinfo - pointer to optional create info structure (used only
                          the file is created)
              vh         - pointer to returned voice handle.

Returns - 0 if success, otherwise error code
-------------------------------------------------------------------------*/
DWORD OpenRecordFile (CTAHD ctahd, char *fullname, unsigned filetype,
           unsigned encoding, int deleteifexists, void *createinfo, VCEHD *vh)
{
    DWORD ret = SUCCESS;

    /* If file exists and delete option not specified, then open, vs create */
    if (access (fullname, 6 /* read/write */) == 0 && ! deleteifexists)
    {

        /* If filetype is "flat" and encoding not specified, default to NMS24 */
        if (filetype == VCE_FILETYPE_FLAT && encoding == 0)
            encoding = VCE_ENCODE_NMS_24 ;

        ret = vceOpenFile ( ctahd, fullname, filetype, VCE_PLAY_RECORD,
                            encoding, vh) ;
    }
    else  /* not exist, or delete specified */
    {
        remove( fullname );  /* ignore failure */

        if (encoding == 0)
            encoding = VCE_ENCODE_NMS_24 ;
        ret = vceCreateFile (ctahd, fullname, filetype, encoding, createinfo,
                             vh);
    }


    return ret;
}


/*-------------------------------------------------------------------------
RecordFile - record multiple messages in one file.

Returns - 0 to exit, -1 if error .
-------------------------------------------------------------------------*/
void RecordFile (char *filename)
{
    VCEHD           vh ;
    unsigned        uret;
    int             ret;
    VCE_CREATE_VOX  voxcreate;
    unsigned        message ;
    VCE_OPEN_INFO   openinfo;
    char cc=0;
    CTA_EVENT event;

    printf ("\n\tFile       = %s\n", filename);

    voxcreate.maxindex = MAX(Maxidx, Endmsg-Startmsg+1);
    voxcreate.size     = sizeof voxcreate;

    uret = OpenRecordFile (Ctahd, filename, Filetype, Encoding, Clobber,
                           &voxcreate, &vh);

    if ( uret != SUCCESS )
    {
        printf( "Could not open or create file.  Error: %d (%s)\n", uret,
                getErrorText(uret));
        doexit(1);
    }

    vceGetOpenInfo (vh, &openinfo, sizeof openinfo);
    Encoding = openinfo.encoding ;

    printf( "\tEncoding   = %d %s\n\n", Encoding,
            (Encoding<DemoNumVoiceEncodings)?DemoVoiceEncodings[Encoding]:"" );

    ShortHelp();
    puts( "\t\t\tPress any key to start record" );

    /*------------------------------------------------*/
    /* unblocked I/O, waiting for key hit either from */
    /* the keyboard or from the telephone handset     */
    /*     4-1-98              Ning                   */
    /*------------------------------------------------*/
    while(1){
        if( kbhit() )
        {
            cc=GETCH();
            if ( cc == 0x1b )
            {
                vceClose ( vh );
                doexit( 0 );
            }
            break;
        }
        else
        {
            do {
                ctaWaitEvent( Ctaqhd, &event, 10 );
            } while( event.id != CTAEVN_WAIT_TIMEOUT );

            ret=adiGetDigit( Ctahd, &cc );

            if( cc != 0 )
            {
                if( cc == '#' )
                {
                    vceClose( vh );
                    doexit( 0 );
                }
                break;
            }
        }
    }


    /* Loop to record all message */
    for( message = Startmsg; message <= Endmsg; message++ )
    {

        /* LATER: Show message text */
        while ((ret= RecordMessage (vh, message)) == 1)
            ;
        switch(ret)
        {
            case 2 :              /* go on to next message */
                break;

            case 0 :              /* Process normal exit request */

                vceClose (vh);
                doexit( 0 );
                break;

            default:
            case -1 :             /* RecordMessage printed error */
                break;
        }
    }
    vceClose (vh);
    doexit( 0 );
}


/*-------------------------------------------------------------------------
  Recordmessage - this function records a specified message with touch-tone
  and keyboard interruption

  It returns one of the following:
   0 - all done
   1 - repeat message
   2 - go to next message
  -1 - error

-------------------------------------------------------------------------*/
int RecordMessage (VCEHD vh, unsigned message)
{

    CTA_EVENT event;
    char      scratch[80];
    char      key ;
    char      messagetext[50];
    unsigned  txtlen;


    static unsigned  Prevlen = 0;
    static int       Paused = FALSE ;

    vceEraseMessage (vh, message);
    txtlen = 0;
    vceReadMessageText(vh, message, messagetext, sizeof messagetext-1, &txtlen);
    if (txtlen == 0 || messagetext[txtlen-1] != '\0')
        messagetext[txtlen++] = '\0';

    for (;;)
    {
        printf("\rMessage %3d %s  %-*s", message,
                                         Paused? "*PAUSED*" : "        ",
                                         MAX(txtlen, Prevlen),
                                         messagetext);
        Prevlen = txtlen;

        key = '\0';
        if (! Paused)
        {
            vceRecord( Ctahd, Maxmsec, VCE_INSERT, &Recordparms );
            do
            {
                ctaWaitEvent( Ctaqhd, &event, 100 );
                if (event.id == CTAEVN_WAIT_TIMEOUT)
                {
                    if (kbhit())
                    {
                        if (key == '\0')
                        {
                            key = GETCH();
                            switch (key)
                            {
                            case '1':
                            case '2':
                            case '5':
                            case '#':
                            case '\033':
                                break;
                            default:
                                key = '\0';
                                continue;
                            }
                        }
                        vceStop(Ctahd);
                    }
                    else
                        continue;
                }
            } while ( event.id != VCEEVN_RECORD_DONE );

            if (event.value == CTA_REASON_FINISHED
             || event.value == CTA_REASON_TIMEOUT
             || event.value == CTA_REASON_VOICE_END)
                return 2 ;
            else if (event.value == CTA_REASON_RELEASED)
                return 0 ;
            else if (event.value == CTA_REASON_NO_VOICE)
            {
                printf("No voice - message deleted\n");
                vceEraseMessage (vh, message);
                return 2;

            }
            else if (event.value == CTA_REASON_DIGIT)
                adiGetDigit (Ctahd, &key) ;

            else if (event.value != CTA_REASON_STOPPED)
            {
                 ctaGetText(Ctahd, event.value, scratch, sizeof scratch);
                 printf("Record failed (error=%s)\n", scratch);
                 return -1;
            }
        }
        else  /*paused*/
        {
            char      digitbuf[2] ;
            adiCollectDigits (Ctahd, digitbuf, 1, &Digitparms) ;
            do
            {
                ctaWaitEvent( Ctaqhd, &event, 100 );
                if (event.id == CTAEVN_WAIT_TIMEOUT)
                {
                    if (kbhit())
                    {
                        if (key == '\0')
                        {
                            key = GETCH();
                            switch (key)
                            {
                            case '1':
                            case '2':
                            case '5':
                            case '#':
                            case '\033':
                                break;
                            default:
                                key = '\0';
                                continue;
                            }
                        }
                        adiStopCollection (Ctahd) ;
                    }
                    else
                        continue;
                }
            } while ( event.id != ADIEVN_COLLECTION_DONE );
            if (event.value == CTA_REASON_RELEASED)
                return 0 ;
            else if ( event.value == CTA_REASON_FINISHED)
                key = digitbuf[0] ;
            else if (event.value != CTA_REASON_STOPPED)
            {
                ctaGetText(Ctahd, event.value, scratch, sizeof scratch);
                printf("Digit Collection failed (error=%s)\n", scratch);
                return -1;
            }
        }

        switch ( key )
        {
        case '1':
            if (Paused)
                Paused = 0;
            return 1;             /* repeat message */

        case '2':
            if (Paused)
                Paused = 0;
            return 2;             /* goto next message */

        case '5':                 /* pause/resume */
            Paused = ! Paused ;
            if (Paused)
                printf("\rMessage %3d *PAUSED*", message);
            else
                printf("\rMessage %3d         ", message);
            fflush (stdout);
            break;

        case '*':                 /* Print help */
            ShortHelp() ;
            break;

        case '#':                 /* Exit */
        case '\033':              /* <esc> = Exit */
            return 0;

        default:
            break;
        }
    }
    UNREACHED_STATEMENT( return 0; )
}


/*------------------------------ doexit ----------------------------------*/
void doexit (int exitcode)
{
    /* hang up phone call if necessary */
    if( STRICMP( Protocol, "nocc" ) != 0 )
        /* don't wait for caller to hang up */
        adiReleaseCall( Ctahd );

    ctaDestroyQueue (Ctaqhd) ;
    /* Put back old keyboard mode */
    DemoSetupKbdIO(0);


    exit (exitcode);
}


/*----------------------------- getErrorText ------------------------------*/
char *getErrorText(DWORD errcode )
{
    static char text[40];

    ctaGetText( NULL_CTAHD, errcode, text, sizeof(text) );
    return text ;
}


