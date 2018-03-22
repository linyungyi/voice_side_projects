/***********************************************************************
*  File - trunkmon.c
*
*  Description - Digital trunk status display utility and demo
*
*    1997-2001  NMS Communications Corporation.
*
*   Sound on note:
*     Trunkmon has a nasty side effect when the beep_ascending() and
*   beep_descending() functions are enabled with the -S option.  These
*   functions are synchronous and stop trunkmon from handling its CTA queue.
*   If a trunk goes in and out of alarm rapidly for an extended period of
*   time, messages will be left unhandled for an extended period of time.
*   IF CT Access is being run in library mode, the driver will complain about
*   not being serviced and drop messages on the floor (with an error to the
*   system log).  If in client/server mode, CTAccess will start chewing
*   up lots of memory
***********************************************************************/
#define VERSION "1.5"

#include <stdio.h>

/*---------------------------------------------------------------------------
* help - Print usage information.
*--------------------------------------------------------------------------*/
void help(void)
{

    puts("");
    puts("  -@{server}     - server host name or IP address, default = localhost" );
    puts("  -b{board}      - Board to monitor [default 0]");
    puts("  -s             - enable beep when trunk alarm state changes");
    puts("  -?             - shows this help");
    puts("");
    return ;
}

/*---------------------------------------------------------------------------
* banner - print name and version of program.
*--------------------------------------------------------------------------*/
void banner(void)
{
    puts ("TRUNKMON Version " VERSION "   " __DATE__ "\n");
}

/* Screen format */
/*

  Digital Trunk Monitor    NMS Communications    Ver 1.0  Mar 31 1997
  Digital Trunk Monitor Version 1.0   NMS Communications  Mar 31 1997
  (Press ESC or F3 to exit)

    -----------------------------------------------------------------
    Board start time:   Mon Jan 01 00:00:00


        Board #0        Trunk 0       Trunk 1       Trunk 2       Trunk 3
        -----------------------------------------------------------------

          Alarm:             NONE          *RED*        *BLUE*         NONE
          Errored_seconds:     12            12             0             0
          Failed seconds:       1         12345           888             0
          BPVs:              1023            93             0             0
          Slips:               12             2             0             0
          Frame sync:          OK        *LOST*        ALL 1s            OK

*/


#include <stdlib.h>
#include "nmstypes.h"
#include <string.h>
#include <time.h>
#include <signal.h>

#if defined (unix)
  #include <ctype.h>
  #include <fcntl.h>
  #include <unistd.h>
  #include <termio.h>
  #include <sys/time.h>
  #include <errno.h>
#else
  #include <conio.h>
  #include <time.h>
#endif

#if defined (WIN32)
  #include <windows.h>
#endif

#include "dtmdef.h"
#include "ctadef.h"
#include "adidef.h"

#ifndef ARRAY_DIM
  #define ARRAY_DIM(a) (sizeof(a)/sizeof(*(a)))
#endif

/*---------------------------------------------------------------------------
* Forward References
*--------------------------------------------------------------------------*/
void beep_ascending(void) ;
void beep_descending(void);

void initconsole(void) ;
void initialize_display (int board);
void display_data (DTM_TRUNK_STATUS *status) ;
void display_bridata (DTM_BRI_TRUNK_STATUS *status) ;
void process_events (void) ;
void doexit( int xcode, char *errstr );

void init_ct_access (void) ;

int start_monitor (void) ;

/*---------------------------------------------------------------------------
* OS-specific defines and references
*--------------------------------------------------------------------------*/
#if defined (unix)
#define HANDLE int
#define getch getchar
int kbhit (void) ;
static struct termio oldtty ;
int TheyKilledMe = 0;
#define Sleep(x)                        sleep(x)
#define Beep(x,y)
int SetCurPos (int row, int col, int handle) ;
int WrtCharStr (char *msg, int len, int row, int col, int handle);

#else
int SetCurPos (int row, int col, HANDLE handle) ;
int WrtCharStr (char *msg, int len, int row, int col, HANDLE handle);
#if defined (WIN32)
#define getch _getch
#define kbhit _kbhit
#endif
#endif

/*---------------------------------------------------------------------------
* GLOBALS
*--------------------------------------------------------------------------*/
#define MAXTRUNKS  16
#define MAX_SERVER_SIZE     64

unsigned Numtrunks = 0;

CTAHD      Ctahd ;                   /* CT/AG access context hdl  */
DTMHD      Dtmhd[MAXTRUNKS] = {0};
unsigned   Board = 0 ;
int        g_SoundOn = 0 ;           /* global variable to turn on/off beep off by default */

static char s_szServer[ MAX_SERVER_SIZE ] = "";

CTAQUEUEHD Ctaqhd ;                  /* Natural Access queue handle */

DWORD TrunkType = ADI_TRUNKTYPE_T1;

struct  /* A structure to save display state (write to screen only if changed)*/
    {
    unsigned  state;
    unsigned  type;
    unsigned  slips;
    unsigned  errors;
    unsigned  receives;
    unsigned  transmits;
    unsigned  b_channel1;
    unsigned  b_channel2;
    time_t    starttime;

     /* beep on transitions;  1=alarms on, 2=alarms off */
    int          alarm_state   ;
    } BriSaved [MAXTRUNKS], *PBriSaved = {0};

struct  /* A structure to save display state (write to screen only if changed)*/
{
    unsigned  alarm ;
    unsigned  remote_alarm ;
    unsigned  es    ;
    unsigned  fs    ;
    unsigned  bpvs  ;
    unsigned  slips ;
    unsigned  sync  ;
    time_t    starttime ;

    /* beep on transitions;  1=alarms on, 2=alarms off */
    int          alarm_state   ;
} Saved [MAXTRUNKS], *Psaved = {0};




#if defined (WIN32)
HANDLE hStdOut;
HANDLE hStdIn;
#else
int hStdOut = 0;
#endif

/*---------------------------------------------------------------------------
* err - Print an error message and help-screen; then exit.
*--------------------------------------------------------------------------*/
void err(char *errstr)
{
    if (errstr == NULL)
        errstr = "error in command DottedLine";
    printf (" Error: %s.\n", errstr);
    help ();
    exit (1);
}

/*---------------------------------------------------------------------------
* getargs - Process command-DottedLine arguments.
*--------------------------------------------------------------------------*/
void getargs(int argc, char **argv)
{
    while (-- argc > 0)
    {
        if (**++argv == '/' || **argv == '-')
        {
            switch (*++*argv)
            {
            case '?' :
            case 'h' :
            case 'H' :
                help();
                exit (0);

            case '@' :
            {
                char *pc;
                if (*++*argv != '\0')
                    pc = *argv;                  /*  "-@<server>" */
                else if (--argc > 0)
                    pc = *++argv;                /*  "-@ <server>" */
                else
                    err("option requires argument -- @");
                if (strlen(pc) >= MAX_SERVER_SIZE)
                    err("-@ argument is too long");
                strcpy( s_szServer, pc );
                break ;
            }

            case 'b' :
            case 'B' :
                if (*++*argv != '\0')
                    Board = atoi (*argv) ;       /*  "-Bn" */
                else if (--argc > 0)
                    Board = atoi (*++argv) ;     /*  "-B n" */
                else
                    err(NULL);
                if (Board < 0)
                    err("invalid board number");
                break ;

            case 's' :
            case 'S' :
                g_SoundOn = 1;                  /* turn on the beep */
                break ;

            default:
                err(NULL);
                break ;       /* Not reached */
            }
        }
    }/* while */
}


/*---------------------------------------------------------------------------
* sighdlr (unix signal handler)
*--------------------------------------------------------------------------*/
#if defined (unix)
void sighdlr (int sig)
{
	TheyKilledMe = 1;
}
#endif

/*---------------------------------------------------------------------------
* main
*
*--------------------------------------------------------------------------*/
int main(int argc, char **argv)
{

    int ret = 0;

#if defined(unix)
   /* Set a handler to restore keyboard */
   (void) signal(SIGINT, sighdlr) ; 
#endif

    banner();
    getargs (argc, argv);

    init_ct_access();

    ret = start_monitor();


    if (ret)
    {

        initconsole();

        initialize_display (Board);

        process_events() ;   /* Loop until user exits */

        SetCurPos (23, 0, hStdOut); /* set position for exit */

        doexit (0, NULL) ;
    }
    else
    {
        printf("\nSorry, unable to monitor trunks on board %d.\n", Board);
    }
    exit (1);
}


/*---------------------------------------------------------------------------
* errorhandler - called from CT/AG Access on function error
*--------------------------------------------------------------------------*/
DWORD NMSSTDCALL errorhandler ( CTAHD ctahd, DWORD errorcode,
                               char *errortext, char *func )
{
    if (errorcode == CTAERR_FUNCTION_NOT_AVAIL
        && strcmp(func, "dtmStartTrunkMonitor") == 0)
        fprintf(stderr,
        "\007The monitor function is not available on the specified board.\n");

    else if (errorcode == CTAERR_SVR_COMM)
        fprintf(stderr, *s_szServer
            ? "\007Server communication error with %s.\nVerify that Natural Access Server (ctdaemon) is running."
            : "\007Server communication error.\nVerify that Natural Access Server (ctdaemon) is running.",
                 s_szServer);
    else
        fprintf(stderr, "\007Error in %s: %s (%#x).\n",
        func, errortext, errorcode );
    exit( 1 );
    return 0;    /* satisfy the compiler */
}


/*---------------------------------------------------------------------------
* init_ct_access
*--------------------------------------------------------------------------*/
void init_ct_access (void)
{
    CTA_EVENT      event = { 0 };
    CTA_INIT_PARMS initparms = { 0 };
    unsigned       ret ;

    /* ctaInitialize */
    static CTA_SERVICE_NAME Init_services[] =
        { { "ADI", "ADIMGR" },
          { "DTM", "ADIMGR" },
        };

    /* ctaOpenServices */
    static CTA_SERVICE_DESC Services[] =
    {
        { { "ADI", "ADIMGR" }, { 0 }, { 0 }, { 0 } },
        { { "DTM", "ADIMGR" }, { 0 }, { 0 }, { 0 } },
    };

    /* ctaCreateContext */
    const char szDefaultDescriptor[] = "trunkmon";
    char szDescriptor[ MAX_SERVER_SIZE + sizeof(szDefaultDescriptor) ] = "";

    /* If server was specified, prepend it and a slash to descriptor string. */
    if (*s_szServer)
    {
        strcpy( szDescriptor, s_szServer );
        strcat( szDescriptor, "/" );
    }
    strcat( szDescriptor, szDefaultDescriptor );

    ctaSetErrorHandler( errorhandler, NULL );

    initparms.size = sizeof(initparms);

    ret = ctaInitialize(Init_services, ARRAY_DIM(Init_services), &initparms);
    if( ret != SUCCESS )
    {
        printf("ERROR: ctaInitialize returned 0x%08x (see ctaerr.h)\n", ret);
        exit (1);
    }

    if (!(*s_szServer))
        ret = ctaSetDefaultServer("localhost");
    if( ret != SUCCESS )
    {
        printf("ERROR: ctaSetDefaultServer returned 0x%08x (see ctaerr.h)\n", ret);
        exit (1);
    }

    /* Open the Natural Access application queue with all managers */
    ctaCreateQueue(NULL, 0, &Ctaqhd);

    /* Create a call resource for handling the incoming call */
    ctaCreateContext(Ctaqhd, 0, szDescriptor, &Ctahd);

    /* Open the service managers and associated services */
    Services[0].mvipaddr.board  = ADI_AG_DRIVER_ONLY;

    ctaOpenServices(Ctahd, Services, ARRAY_DIM(Services));

    /* Wait for the service manager and services to be opened asynchronously */
    do
    {
        ctaWaitEvent( Ctaqhd, &event, 3000);
        if (event.id == CTAEVN_WAIT_TIMEOUT)
        {
            ret = CTAEVN_WAIT_TIMEOUT;
            break;
        }
    } while (event.id != CTAEVN_OPEN_SERVICES_DONE) ;

    if (CTA_IS_ERROR((event.value)))
    {
        char errtxt[40];
        char errbuf[120];
        ctaGetText( NULL_CTAHD, event.value,  errtxt, sizeof(errtxt) );
        sprintf (errbuf,
            "\007Error %s waiting for CTAEVN_OPEN_SERVICES_DONE", errtxt);
        doexit (1, errbuf);
    }

} // end init_ct_access

/*---------------------------------------------------------------------------
* start_monitor
*--------------------------------------------------------------------------*/
int start_monitor (void)
{
    unsigned         trunk ;
    DTM_START_PARMS  startparms = {0};
    ADI_BOARD_INFO   boardinfo = {0};

    ctaGetParms( Ctahd, DTM_START_PARMID, &startparms, sizeof startparms);

    startparms.maxevents   = 20;           /* HARD CODED MAX 20 events/sec */
    startparms.reportmask  = 0xffffffff;

    /* Determine the board type and numnber of trunks */
    if (adiGetBoardInfo (Ctahd, Board, sizeof boardinfo, &boardinfo) != SUCCESS)
        goto error;

    switch(boardinfo.trunktype)
    {
    case ADI_TRUNKTYPE_T1:
    case ADI_TRUNKTYPE_E1:
    case ADI_TRUNKTYPE_BRI:
        TrunkType = boardinfo.trunktype;
        break;
    default:
        goto error;
    }
    Numtrunks = boardinfo.numtrunks;


    for (trunk=0; trunk<Numtrunks; trunk++)
    {
        char             tmpbuf[80];
        CTA_EVENT        event = {0};

        dtmStartTrunkMonitor (Ctahd, Board, trunk, &Dtmhd[trunk], &startparms);
        while (1)
        {
            ctaWaitEvent( Ctaqhd, &event, 2000);
            switch (event.id)
            {
            case DTMEVN_MONITOR_STARTED:
                goto acked;

            case CTAEVN_WAIT_TIMEOUT:
                fprintf(stderr, "Timed out waiting for Monitor-Started "
                    "event for trunk %d\n", trunk);
                /* instead of ending the application, continue onto the next trunk */
                goto acked;

            case DTMEVN_MONITOR_DONE:
                ctaGetText(Ctahd, event.value, tmpbuf, sizeof tmpbuf);
                fprintf(stderr, "Monitor not started for trunk %d, "
                    "reason = %s\n",
                    trunk, tmpbuf);
                Dtmhd[trunk] = 0;
                goto acked;

            case ADIEVN_BOARD_ERROR:
            default:
                fprintf(stderr, "Unexpected event: 0x%x\n", event.id);
                ctaFormatEvent ("", &event, tmpbuf, sizeof tmpbuf);
                fprintf (stderr, "%s\n", tmpbuf);
                goto error;

            case DTMEVN_TRUNK_STATUS:
                break;
            }
        }
acked:
        continue;                   /* Go on to next trunk. */

    }
    return TRUE;

error:
    return FALSE;
}



/*---------------------------------------------------------------------------
* process_events
*--------------------------------------------------------------------------*/
void process_events (void)
{
    CTA_EVENT        event;
    DTM_TRUNK_STATUS status ;
    DTM_BRI_TRUNK_STATUS bri_status ;
    DTMHD            dtmhd ;
    unsigned         trunk;


    memset (&Saved, 0xFF, sizeof Saved) ;    /* Force initial display */
    memset (&BriSaved, 0xFF, sizeof BriSaved) ;    /* Force initial display */

    for (trunk=0; trunk<Numtrunks;trunk++)
    {
        if( Dtmhd[trunk] != 0 )
        {
            if (TrunkType == ADI_TRUNKTYPE_BRI)
            {
                dtmGetBriTrunkStatus (Dtmhd[trunk], &bri_status, sizeof bri_status) ;
                display_bridata(&bri_status) ;
            }
            else
            {
                dtmGetTrunkStatus (Dtmhd[trunk], &status, sizeof status) ;
                display_data(&status) ;
            }
        }
    }

    for(;;)
    {

#if defined(unix)
	if (TheyKilledMe) goto quit;
#endif
        while (kbhit()) /*- check for ESC or F3 */
        {
            int  c ;
            if ((c=getch()) == 0) c = 0x100 + getch() ;
            if (c == 0x1b || c == 0x13d)
                goto quit ;

            /* Undocumented: alt-F1 resets error counters */
            if (c == 0x168)
                for (trunk=0; trunk<Numtrunks;trunk++)
                    if (Dtmhd[trunk] != 0)
                        dtmResetCounters(Dtmhd[trunk]);
        }

        ctaWaitEvent( Ctaqhd, &event, 1000); /* check kbd once a second */
        switch (event.id)
        {
        case DTMEVN_TRUNK_STATUS:
            dtmhd = event.size;
            if (TrunkType == ADI_TRUNKTYPE_BRI)     /* this is for the BRI Card */
            {
                dtmGetBriTrunkStatus (dtmhd, &bri_status, sizeof bri_status) ;
                display_bridata(&bri_status) ;
            }
            else
            {
                dtmGetTrunkStatus (dtmhd, &status, sizeof status) ;
                display_data(&status) ;
            }
            break;

        case CTAEVN_WAIT_TIMEOUT:
            break;

        case DTMEVN_MONITOR_DONE:
        {
            char tmpbuf[50];
            ctaGetText(Ctahd, event.value, tmpbuf, sizeof tmpbuf);
            printf("Monitor terminated, reason = %s\n", tmpbuf);
            break;
        }

        default:
            printf("Unexpected event: 0x%x\n", event.id);
            break;
        }

    } /* for(;;) */
quit:
    return ;
}

/*---------------------------------------------------------------------------
* initialize_display -
*--------------------------------------------------------------------------*/
void initialize_display(int board)
{
    static char ProgLine[] =
        "   Digital Trunk Monitor      NMS Communications      Ver " VERSION "  "
        __DATE__ ;
    static char ExitLine[] =
#if ! defined (unix)
        "                           (Press F3 or ESC to exit, ALT-F1 to reset)";
#else
    "                           (Press ESC to exit)      ";
#endif
    static char EmptyLine[] = "" ;
    static char DottedLine[] =
        "   -------------------------------------------------------------------------";

    static char StartTime[] = "   Monitor start time:";
    static char Header1[]   = "            Alarms   Remote    Errored  Failed     Code      Slips    Frame";
    static char Header2[]   = "                     alarms      sec      sec   violations             sync";

    static char briHeader1[]   = "            State    Type   Slips   Errors  Receives   Transmits  B1  B2";
    static char briHeader2[]   = "                                                                        ";

    int      row = 0;
    char     buf[80] = {0};
    unsigned i;

#define TOP_ROW    0
#define TRUNK_ROW  TOP_ROW + 10

#if defined ( unix )
    printf ("\x1B[2J\n") ;
#else
    system ("CLS");         /* best way? */
#endif
    /* str, size, row, col, 0 */
    WrtCharStr( ProgLine, sizeof ProgLine, TOP_ROW,   0, hStdOut );
    WrtCharStr( ExitLine, sizeof ExitLine, TOP_ROW+1, 0, hStdOut );
//    WrtCharStr( EmptyLine, sizeof EmptyLine, TOP_ROW+2, 0, hStdOut );

    sprintf (buf,"   BOARD # %d", board) ;
    WrtCharStr( buf,     strlen(buf),    TOP_ROW+2, 0, hStdOut );
    WrtCharStr( DottedLine, sizeof DottedLine, TOP_ROW+3, 0, hStdOut );
    WrtCharStr(StartTime, sizeof(StartTime), TOP_ROW+4,   0, hStdOut );
    WrtCharStr(EmptyLine, sizeof(EmptyLine), TOP_ROW+5, 0, hStdOut );
    if (TrunkType != ADI_TRUNKTYPE_BRI)
    {
        WrtCharStr(Header1, sizeof(Header1), TOP_ROW+6, 0, hStdOut );
        WrtCharStr(Header2, sizeof(Header2), TOP_ROW+7, 0, hStdOut );
    }
    else
    {
        WrtCharStr(briHeader1, sizeof(Header1), TOP_ROW+6, 0, hStdOut );
        WrtCharStr(briHeader2, sizeof(Header2), TOP_ROW+7, 0, hStdOut );
    }
    WrtCharStr(DottedLine, sizeof(Header2), TOP_ROW+8, 0, hStdOut );

    for (i=0; i<Numtrunks; i++)
    {
        char trunk[10]  ;
        int len = 0;
        sprintf(trunk, "Trunk %d", i);
        len = strlen(trunk);
        WrtCharStr(trunk, len, TRUNK_ROW + i, 3, hStdOut );
    }

    SetCurPos(row, 0, hStdOut);
    return ;
}


/*---------------------------------------------------------------------------
* display_data - display data on screen
*--------------------------------------------------------------------------*/
void display_data (DTM_TRUNK_STATUS *status)
{

    char     *alarm_text;                     /* pointer to display text */
    char     *remote_alarm_text;              /* pointer to display text */
    char     *sync_text ;                     /* pointer to display text */
    unsigned  alarm               = 0 ;
    unsigned  remote_alarm        = 0 ;
    int       row ;                           /* row to display data on */
    char      displayString[120];
    BOOL      printFlag           = FALSE;    /* flag to update display */

    if (status->trunk >= MAXTRUNKS)
        return ;

    Psaved = &Saved[status->trunk] ;

    if (Psaved->starttime != (time_t)status->starttime)
    {
        Psaved->starttime = (time_t)status->starttime ;
        WrtCharStr( asctime(localtime(&Psaved->starttime)),
            24,
            TOP_ROW+5,
            26,
            hStdOut);
    }

    row = TRUNK_ROW + status->trunk;
    if (TrunkType == ADI_TRUNKTYPE_T1)
    {
        /* process alarm field (T1) */
        if (status->state.alarms & DTM_ALARM_AIS)
        {
            alarm = 1;
            alarm_text = "  BLUE";
        }
        else if (status->state.alarms & DTM_ALARM_LOF)
        {
            alarm = 3;
            alarm_text = "   RED";
        }
        else
        {
            alarm = 0 ;
            alarm_text = "  NONE";
        }
        /* process remote alarm field */
        if (status->state.alarms & DTM_ALARM_FAR_END)
        {
            remote_alarm = 1;
            remote_alarm_text = "YELLOW";
        }
        else
        {
            remote_alarm = 0 ;
            remote_alarm_text = "  NONE";
        }
    }
    else
    {
        /* process alarm field (E1) */
        if (status->state.alarms & DTM_ALARM_AIS)
        {
            alarm = 3;
            alarm_text = "   AIS";
        }
        else if (status->state.alarms & DTM_ALARM_LOF)
        {
            alarm = 2 ;
            alarm_text = "NO_FRM";
        }
        else if (status->state.alarms & DTM_ALARM_TS16AIS)
        {
            alarm = 5;
            alarm_text = "16 AIS";
        }
        else
        {
            alarm = 0 ;
            alarm_text = "  NONE";
        }
        /* process remote alarm field */
        if (status->state.alarms & DTM_ALARM_FAR_END)
        {
            remote_alarm = 1;
            remote_alarm_text = " RAI";
        }
        else if (status->state.alarms & DTM_ALARM_FAR_LOMF )
        {
            remote_alarm = 2;
            remote_alarm_text = "NO_SMF";
        }
        else
        {
            remote_alarm = 0 ;
            remote_alarm_text = "  NONE";
        }
    }
    if (alarm != Psaved->alarm)       // Alarms
    {
        Psaved->alarm = alarm ;
        printFlag=TRUE;
    }

    if (remote_alarm != Psaved->remote_alarm)
    {
        Psaved->remote_alarm = remote_alarm ;
        printFlag=TRUE;
    }

    /* play ascending or descending tone if DottedLine just came up or went down */
    if (Psaved->alarm_state && !alarm && !remote_alarm)
    {
        beep_ascending();
        Psaved->alarm_state = 0;
    }
    else if (!Psaved->alarm_state && (alarm || remote_alarm))
    {
        beep_descending();
        Psaved->alarm_state = 1;
    }


    if (Psaved->es != status->es)             // errored seconds
    {
        Psaved->es = status->es ;
        printFlag=TRUE;
    }

    if (Psaved->fs != status->uas)            // Failed seconds
    {
        Psaved->fs = status->uas;
        printFlag=TRUE;
    }

    if (Psaved->bpvs != status->lineerrors)   //Code Violations
    {
        Psaved->bpvs = status->lineerrors;
        printFlag=TRUE;
    }

    if (Psaved->slips != status->slips)       // slips
    {
        Psaved->slips = status->slips;
        printFlag=TRUE;
    }

    if (status->state.sync == 0)
        sync_text = "    OK" ;
    else if (status->state.sync & DTM_SYNC_NO_SIGNAL)
        sync_text = "NoSgnl" ;
    else if (status->state.sync & DTM_SYNC_NO_FRAME)
        sync_text = "No Frm" ;
    else if (status->state.sync & DTM_SYNC_NO_MULTIFRAME)
        sync_text = " No MF" ;
    else if (status->state.sync & DTM_SYNC_NO_CRC_MF)
        sync_text = "NoCRCF" ;
    else
        sync_text = "??????" ;

    if (Psaved->sync != status->state.sync)
        {
        Psaved->sync = status->state.sync ;
        printFlag=TRUE;
    }

    if (printFlag)
    {
        //  Alarms     Remote      Errored     Failed      Code       Slips       Frame
        //             alarms      seconds     seconds     violations             Sync
        sprintf(displayString,"%-6s  %-6s     %6d   %6d   %6d    %6d    %-6s"
            ,alarm_text,remote_alarm_text,Psaved->es,Psaved->fs,Psaved->bpvs,Psaved->slips,sync_text);
        WrtCharStr(displayString, strlen(displayString), row, 12, hStdOut );
    }
    return ;
}

/*---------------------------------------------------------------------------
 * display_bridata - display data on screen
 *--------------------------------------------------------------------------*/
void display_bridata (DTM_BRI_TRUNK_STATUS *status)
{
    char     *state_text ;
    char     *type_text ;
    char      tmpbuf[20];
    int       row ;

    int StateCol = 11;
    int TypeCol = 19;
    int SlipsCol = 27;
    int ErrorsCol = 36;
    int RxCol = 46;
    int TxCol = 58;
    int B1Col = 67;
    int B2Col = 71;

    if (status->trunk >= MAXTRUNKS)
        return ;

    PBriSaved = &BriSaved[status->trunk] ;

    if (PBriSaved->starttime != (time_t)status->starttime)
    {
        PBriSaved->starttime = (time_t)status->starttime ;
        WrtCharStr( asctime(localtime(&PBriSaved->starttime)),
                    24,
                    TOP_ROW+5,
                    26,
                    hStdOut);
    }

    row = TRUNK_ROW + status->trunk;

    /* Type field  */
    switch ( status->type )
    {
        case DTM_BRI_TYPE_TE:       type_text = "    TE"; break;
        case DTM_BRI_TYPE_NT:       type_text = "    NT"; break;
        default :                   type_text = "    ??"; break;
    }

    /* State field  */
    switch ( status->state )
    {
        case DTM_BRI_STATE_NO_USED: state_text = "  NONE"; break;

        case DTM_BRI_STATE_F1:      state_text = "    F1"; break;
        case DTM_BRI_STATE_F2:      state_text = "    F2"; break;
        case DTM_BRI_STATE_F3:      state_text = "    F3"; break;
        case DTM_BRI_STATE_F4:      state_text = "    F4"; break;
        case DTM_BRI_STATE_F5:      state_text = "    F5"; break;
        case DTM_BRI_STATE_F6:      state_text = "    F6"; break;
        case DTM_BRI_STATE_F7:      state_text = "    F7"; break;
        case DTM_BRI_STATE_F8:      state_text = "    F8"; break;

        case DTM_BRI_STATE_G1:      state_text = "    G1"; break;
        case DTM_BRI_STATE_G2:      state_text = "    G2"; break;
        case DTM_BRI_STATE_G3:      state_text = "    G3"; break;
        case DTM_BRI_STATE_G4:      state_text = "    G4"; break;

        default :                   state_text = "  ????"; break;
    }

    if ( status->state != PBriSaved->state )
    {
        PBriSaved->state = status->state ;
        WrtCharStr( state_text, 6, row, StateCol, hStdOut );
    }
    if ( status->type != PBriSaved->type )
    {
        PBriSaved->type = status->type;
        WrtCharStr( type_text, 6, row, TypeCol, hStdOut );
    }
    if ( status->slips != PBriSaved->slips )
    {
        PBriSaved->slips = status->slips;
        sprintf(tmpbuf, "%6d", PBriSaved->slips);
        WrtCharStr( tmpbuf, 6, row, SlipsCol, hStdOut );
    }
    if ( status->errors != PBriSaved->errors )
    {
        PBriSaved->errors = status->errors;
        sprintf(tmpbuf, "%6d", PBriSaved->errors);
        WrtCharStr( tmpbuf, 6, row, ErrorsCol, hStdOut );
    }
    if ( status->receives != PBriSaved->receives )
    {
        PBriSaved->receives = status->receives;
        sprintf(tmpbuf, "%6d", PBriSaved->receives);
        WrtCharStr( tmpbuf, 6, row, RxCol, hStdOut );
    }
    if ( status->transmits != PBriSaved->transmits )
    {
        PBriSaved->transmits = status->transmits;
        sprintf(tmpbuf, "%6d", PBriSaved->transmits);
        WrtCharStr( tmpbuf, 6, row, TxCol, hStdOut );
    }
    if ( status->b_channel1 != PBriSaved->b_channel1 )
    {
        PBriSaved->b_channel1 = status->b_channel1;
        sprintf(tmpbuf, "%d", PBriSaved->b_channel1);
        WrtCharStr( tmpbuf, 1, row, B1Col, hStdOut );
    }
    if ( status->b_channel2 != PBriSaved->b_channel2 )
    {
        PBriSaved->b_channel2 = status->b_channel2;
        sprintf(tmpbuf, "%d", PBriSaved->b_channel2);
        WrtCharStr( tmpbuf, 1, row, B2Col, hStdOut );
    }
} // end display_bridata

/*---------------------------------------------------------------------------
 * doexit - reset kbd (unix) and exit
 *--------------------------------------------------------------------------*/
void doexit( int xcode, char *errstr)
{
    if (xcode != 0)
        printf("%s\n", errstr);
    /* Rely on process exit cleanup to release Natural Access resources! */
#if defined (unix)
    ioctl (0, TCSETAF, &oldtty) ; /* Reset keyboard to default parameters */
#endif
    exit( xcode );
}



/*---------------------------------------------------------------------------
* beep_ascending, beep_descending
*--------------------------------------------------------------------------*/
void beep_ascending(void)
{
    if(g_SoundOn != 0)
    {
        Beep (440, 60);        /* A */
        Beep (494, 60);        /* B */
        Beep (523, 60);        /* C */
        Beep (587, 60);        /* D */
        Beep (659, 60);        /* E */
    }
    return ;
}

void beep_descending(void)
{
    if(g_SoundOn != 0)
    {
        Beep (659, 60);        /* E */
        Beep (587, 60);        /* D */
        Beep (523, 60);        /* C */
        Beep (494, 60);        /* B */
        Beep (440, 60);        /* A */
    }
    return ;
}

/*---------------------------------------------------------------------------
* initconsole
*--------------------------------------------------------------------------*/
void initconsole(void)
{
#if defined (WIN32)
    COORD coord;

    hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdIn == INVALID_HANDLE_VALUE)
    {
        doexit(1, "GetStdHandle(STD_INPUT_HANDLE);");
    }

    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdOut == INVALID_HANDLE_VALUE)
    {
        doexit(1, "GetStdHandle(STD_OUTPUT_HANDLE);");
        }

    // Test if screen is big enough, if not, make it bigger
    coord.X = 1;
    coord.Y = 30;
/*
    if (!SetConsoleCursorPosition( hStdOut, coord ) )
    {
        system("mode 80,30");
    }
*/
#endif

#if defined (unix)
    /* setup kbd */
    struct termio tty ;

    ioctl (0, TCGETA, &tty) ;     /* Get parameters associated with terminal */
    oldtty = tty ;                /* Duplicate to reset later                */

    tty.c_lflag     &= ~ICANON ;  /* Set ICANON off                          */
    tty.c_lflag     &= ~ECHO ;    /* Set ECHO off                            */
    tty.c_cc[VMIN]  =  (char) 1 ; /* Set VMIN to 1                           */
    tty.c_cc[VTIME] =  (char) 0 ; /* Set VTIME to 0                          */

    ioctl (0, TCSETAF, &tty) ;    /* Set new parameters associated with tty  */

    /* Set a handler to restore keyboard */
    /* moved to the beginning of main() in order to avoid race condition */
#endif
    return ;
}


/*---------------------------------------------------------------------------
* SetCurPos, WrtCharStr  (unix and NT)
*--------------------------------------------------------------------------*/

#if defined (WIN32)
int SetCurPos (int row, int col, HANDLE handle)

{
    COORD coord;

    coord.X = col + 1;
    coord.Y = row + 1;

    if (!SetConsoleCursorPosition( handle, coord ) )
    {
        doexit(1, "SetConsoleCursorPosition failed, please increase your window size to run trunkmon");
    }
    return (0) ;
}

int WrtCharStr (char *msg, int len, int row, int col, HANDLE handle)
{
    DWORD cWritten;

    SetCurPos(row, col, handle);
    if (!WriteFile(handle, msg, len, &cWritten, NULL))
    {
        doexit(1, "WriteFile");
    }
    return (0);
}
#elif defined (unix)
int SetCurPos (int row, int col, int handle)
{

    (void)handle;

    printf ("\x1B[%d;%dH\n", row + 1, col + 1) ;
    return (0) ;
}

int WrtCharStr (char *msg, int len, int row, int col, int handle)
{
    char str[82] ;

    (void)handle;

    printf ("\x1B[%d;%dH", row + 1, col + 1) ;
    memset (str, '\0', 80) ;

    if (len > 80)
        len = 80 ;

    memcpy (str, msg, len) ;
    puts (str) ;
    return (0) ;
}
#endif
