/***************************************************************************
*  File - trunklog.c
*
*  Description - Digital trunk status monitor demo for Natural Access.
*    Uses the DTM service.
*
*  1997-1998  NMS Communications Corporation.
***************************************************************************/

/* This program writes a line to stdout whenever the alarm state of a
 * any digital trunk changes.  Sample output:
 *
 *   15:52:26  Board 1 Trunk 0: In service
 *   15:52:37  Board 1 Trunk 0: Loss of frame
 *   15:52:38  Board 1 Trunk 0: Loss of frame Far end alarm indication
 *   15:52:54  Board 1 Trunk 0: Far end alarm indication
 *   15:52:55  Board 1 Trunk 0: In service
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined (unix)
  #include <signal.h>
  #include <unistd.h>
  #include <termio.h>
  #include <sys/time.h>
  #ifdef SCO
     #include <time.h>
  #endif /* SCO */

  #define getch getchar
  static struct termio Oldtty ;
#else
  #include <conio.h>
  #include <time.h>

  #if defined (WIN32)
    #define getch _getch
  #endif
#endif

#include "ctadef.h"
#include "adidef.h"
#include "dtmdef.h"

#ifdef unix
  #include "ctademo.h"
#endif /* unix */

#ifndef ARRAY_DIM
  #define ARRAY_DIM(a) (sizeof(a)/sizeof(*(a)))
#endif

#define VERSION_MAJOR   1
#define VERSION_MINOR   1

/*---------------------------------------------------------------------------
 * Forward References
 *--------------------------------------------------------------------------*/
void initconsole(void);
void display_data (DTM_TRUNK_STATUS *status) ;
void process_events (void) ;
void doexit( int xcode );
void init_ct_access (void) ;
int  start_monitor (void) ;
void display_bri_data (DTM_BRI_TRUNK_STATUS *briStatus) ;

/*---------------------------------------------------------------------------
 * GLOBALS
 *--------------------------------------------------------------------------*/
#define MAXBOARDS 16
#define MAXTRUNKS 16

CTAHD      Ctahd ;                   /* Natural Access context hdl  */
CTAQUEUEHD Ctaqhd ;                  /* Natural Access queue handle */
DTMHD      Dtmhd[MAXBOARDS][MAXTRUNKS] = {0};



/*---------------------------------------------------------------------------
 * main
 *
 *--------------------------------------------------------------------------*/
int main(int argc, char **argv)
    {
    initconsole();
    init_ct_access();

    printf("NMS Communications Digital Trunk Monitor, Ver %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
    printf("  Monitoring all digital boards             (Press <Esc> to exit)\n\n");

    if (start_monitor() != 0)
        process_events() ;   /* Loop until user exits */

    doexit (0) ;
    }


/*---------------------------------------------------------------------------
 * start_monitor
 *--------------------------------------------------------------------------*/
int start_monitor (void)
    {
    unsigned         board;
    int              numstarted = 0;
    DTM_START_PARMS  startparms ;

    ctaGetParms( Ctahd, DTM_START_PARMID, &startparms, sizeof startparms);

    startparms.maxevents   = 2;           /* 2 events/sec */
    startparms.reportmask  = DTM_REPORT_ALARMS;

    /* Start a monitor function on each board.   */

    for (board=0; board<MAXBOARDS; board++)
        {
        ADI_BOARD_INFO   boardinfo;
        unsigned         trunk;

        if (adiGetBoardInfo (Ctahd, board, sizeof boardinfo, &boardinfo)
            != SUCCESS)
            continue;

        if ((boardinfo.trunktype != ADI_TRUNKTYPE_T1) &&
            (boardinfo.trunktype != ADI_TRUNKTYPE_E1) &&
            (boardinfo.trunktype != ADI_TRUNKTYPE_BRI))
            continue;
        if (boardinfo.boardtype == ADI_BOARDTYPE_AG2000BRI)
        {
            startparms.reportmask  = DTM_BRI_REPORT_STATE | DTM_BRI_REPORT_SLIPS | DTM_BRI_REPORT_ERRORS;
            startparms.maxevents   = 10;           /* 10 events/sec */
        }
        for (trunk=0; trunk<boardinfo.numtrunks; trunk++)
            {
            if (dtmStartTrunkMonitor (Ctahd, board, trunk, &Dtmhd[board][trunk],
                                      &startparms) != SUCCESS)
                Dtmhd[board][trunk] = 0 ;
            else
                 ++numstarted;
            }
        }

    return numstarted;
    }


/*---------------------------------------------------------------------------
 * process_events
 *--------------------------------------------------------------------------*/
void process_events (void)
    {
    CTA_EVENT        event;
    DTM_TRUNK_STATUS status ;
    DTM_BRI_TRUNK_STATUS briStatus;
    DTMHD            dtmhd ;
    unsigned         board;
    unsigned         trunk;
    char             tmpbuf[80];

    for(;;)
        {
        while (kbhit()) /*- check for ESC or F3 */
            {
            int  c ;
            if ((c=getch()) == 0)
                c = 0x100 + getch() ;
            if (c == 0x1b || c == 0x13d)
                goto quit ;
            }

        ctaWaitEvent( Ctaqhd, &event, 1000); /* check kbd once a second */
        switch (event.id)
            {
            case DTMEVN_MONITOR_STARTED:
            case DTMEVN_TRUNK_STATUS:
                dtmhd = event.size;     /* dtmhd is passed in size field */
                 /* Although the alarm info is contained in the event, we Call
                 * dtmGetTrunkStatus as an easy way to translate the dtmhd to
                 * board and trk.
                 */
                if (DTM_BRI_PROFILE & event.value)
                {
                    dtmGetBriTrunkStatus(dtmhd, &briStatus, sizeof briStatus );
                    display_bri_data(&briStatus) ;
                    break;
                }
                 dtmGetTrunkStatus (dtmhd, &status, sizeof status) ;
                display_data(&status) ;
                break;

            case CTAEVN_WAIT_TIMEOUT:
                break;

            case DTMEVN_MONITOR_DONE:
                /* board and trunk are passed in size field: */
                board = event.size >> 16 ;
                trunk = event.size & 0xffff;
                Dtmhd[board][trunk] = 0 ;
                ctaGetText(Ctahd, event.value, tmpbuf, sizeof tmpbuf);
                printf("Board %d Trunk %d:   Monitor terminated, reason = %s\n",
                           board, trunk, tmpbuf);
                break;

            case ADIEVN_BOARD_ERROR:
            default:
                fprintf(stderr, "Unexpected event: 0x%x\n", event.id);
                ctaFormatEvent ("", &event, tmpbuf, sizeof tmpbuf);
                fprintf (stderr, "%s\n", tmpbuf);
                break;
            }
        } /* for(;;) */
quit:
     return ;
     }


/*---------------------------------------------------------------------------
 * display_data - print alarm on screen
 *--------------------------------------------------------------------------*/
void display_data (DTM_TRUNK_STATUS *status)
    {
    char       alarm_text[100] ;
    time_t     timeval;
    struct tm *t;

    if (status->state.alarms == 0)
        strcpy (alarm_text, "In service");
    else
        {
        alarm_text[0] = '\0';
        if (status->state.alarms & DTM_ALARM_LOF)
            strcat (alarm_text, "Loss of frame ");
        if (status->state.alarms & DTM_ALARM_AIS)
            strcat (alarm_text, "All 1's alarm ");
        if (status->state.alarms & DTM_ALARM_FAR_END )
            strcat (alarm_text, "Far end alarm indication ");

        if (status->state.alarms & DTM_ALARM_FAR_LOMF)
            strcat (alarm_text, "Far end loss of multiframe ");
        else if (status->state.alarms & DTM_ALARM_TS16AIS )
            strcat (alarm_text, "Time slot 16 AIS ");
        }

    timeval = time(NULL);
    t = localtime (&timeval);
    printf("%02d:%02d:%02d  Board %d Trunk %d: %s\n",
                                  t->tm_hour, t->tm_min, t->tm_sec,
                                  status->board, status->trunk,
                                  alarm_text);
    return;
    }
/*---------------------------------------------------------------------------
 * display_bri_data - print bri errors, slips and state on screen
 *--------------------------------------------------------------------------*/
void display_bri_data (DTM_BRI_TRUNK_STATUS *briStatus)
    {
    char     *state_text ;
    time_t     timeval;
    struct tm *t;


    if (briStatus->trunk >= MAXTRUNKS)
        return ;

    /* State field  */
    switch ( briStatus->state )
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

    timeval = time(NULL);
    t = localtime (&timeval);
    printf("%02d:%02d:%02d  Board %d Trunk %d state %s\t: errors=%d\tslips=%d\n",
                                  t->tm_hour, t->tm_min, t->tm_sec,
                                  briStatus->board, briStatus->trunk,
                                  state_text, briStatus->errors, briStatus->slips);
    return;

    }


/*---------------------------------------------------------------------------
 * doexit - reset kbd (unix) and exit
 *--------------------------------------------------------------------------*/
void doexit( int xcode)
    {
    /* Rely on process exit cleanup to release Natural Access resources! */
#ifdef unix
    ioctl (0, TCSETAF, &Oldtty) ; /* Reset keyboard to default parameters */
#endif
    exit( xcode );
    }


/*---------------------------------------------------------------------------
 * sighdlr (unix signal handler)
 *--------------------------------------------------------------------------*/
#if defined (unix)
void sighdlr (int sig)
    {
    ioctl (0, TCSETAF, &Oldtty) ; /* Reset keyboard to default parameters */
    exit(0);
    }
#endif

/*---------------------------------------------------------------------------
 * initconsole
 *--------------------------------------------------------------------------*/
void initconsole(void)
    {
#ifdef unix
     /* setup kbd to raw mode */
    struct termio tty ;

    ioctl (0, TCGETA, &tty) ;     /* Get parameters associated with terminal */
    Oldtty = tty ;                /* Duplicate to reset later                */

    tty.c_lflag     &= ~ICANON ;  /* Set ICANON off                          */
    tty.c_lflag     &= ~ECHO ;    /* Set ECHO off                            */
    tty.c_cc[VMIN]  =  (char) 1 ; /* Set VMIN to 1                           */
    tty.c_cc[VTIME] =  (char) 0 ; /* Set VTIME to 0                          */

    ioctl (0, TCSETAF, &tty) ;    /* Set new parameters associated with tty  */

    /* Set a handler to restore keyboard */
    (void) signal(SIGINT, sighdlr) ;
#endif
    return ;
}

/*---------------------------------------------------------------------------
 * errorhandler - called from Natural Access on function error
 *--------------------------------------------------------------------------*/
DWORD NMSSTDCALL errorhandler ( CTAHD ctahd, DWORD errorcode,
                                   char *errortext, char *func )
    {
    if ( (errorcode == CTAERR_INVALID_BOARD)
         && (strcmp(func, "adiGetBoardInfo") == 0) )
        return errorcode;

    if ( (errorcode == CTAERR_FUNCTION_NOT_AVAIL)
         && (strcmp(func, "dtmStartTrunkMonitor") == 0) )
        return errorcode;

    fprintf( stderr, "\007Error in %s: %s (%#x).\n",
             func, errortext, errorcode );
    doexit( 1 );
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
     /* Note that we need ADI Service only to get board info and
      * to let ctaFormatEvent decode board errors. */
    static CTA_SERVICE_NAME Init_services[] = { { "DTM", "ADIMGR" },
                                                { "ADI", "ADIMGR" } };
     /* ctaOpenServices */
    static CTA_SERVICE_DESC Services[] =
        {
            { { "ADI", "ADIMGR" }, { 0 }, { 0 }, { 0 } },
            { { "DTM", "ADIMGR" }, { 0 }, { 0 }, { 0 } }
        };

    Services[0].mvipaddr.board = ADI_DRIVER_ONLY;
    ctaSetErrorHandler( errorhandler, NULL );

    initparms.size = sizeof(CTA_INIT_PARMS);
    initparms.ctacompatlevel = CTA_COMPATLEVEL;
    ret = ctaInitialize(Init_services, ARRAY_DIM(Init_services), &initparms);
    if( ret != SUCCESS )
    {
        printf("ERROR: ctaInitialize returned 0x%08x (see ctaerr.h)\n", ret);
        exit (1);
    }

    /* Open the Natural Access application queue with all managers */
    ctaCreateQueue(NULL, 0, &Ctaqhd);

    /* Create a call resource for handling the incoming call */
    ctaCreateContext(Ctaqhd, 0, NULL, &Ctahd);

    /* Open the service managers and associated services */
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
        char errtxt[80];
        ctaGetText( NULL_CTAHD, event.value,  errtxt,   sizeof(errtxt) );
        printf ("\007Error %s waiting for CTAEVN_OPEN_SERVICES_DONE\n", errtxt);
        doexit (1);
        }
    }

