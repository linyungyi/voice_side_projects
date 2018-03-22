/*****************************************************************************
*  FILE: isdnncc.c
*
*  DESCRIPTION: demo for channelized ISDN using the NCC service
*
*  Voice Files Used -
*                CTADEMO.VOX     - all the prompts
*                <temporary>.VOX - scratch file for record and playback
*
*
* Copyright 1999-2001 NMS Communications.
*****************************************************************************/

/* $$Revision "%v" */
#define VERSION_NUM  "1.3"
#define VERSION_DATE __DATE__

/*----------------------------- Constants ----------------------------------*/
                             /* if this is the first received digit:        */
#define REJECT_IMMEDIATE_DIGIT '0' /* send immediate disconnect             */
#define REJECT_BUSY_DIGIT      '9' /* reject playing busy tone              */
#define REJECT_REORDER_DIGIT   '8' /* reject playing reorder tone           */
#define REJECT_SIT_DIGIT       '7' /* reject playing SIT tone (LSL - Vacant)*/

#define SINGLE       0       /* operation mode: one port per program        */
#define THREADED     1       /* operation mode: two different threads       */
#define MAX_THREADS  2       /* maximum number of threads                   */

#define NULL_SWIHD NULL_CTAHD

#define UNDETERMINED_STREAM 0xffff  /* an invalid MVIP stream number        */

#define KEYBOARD_EVENT 0xabcd /* no CT / AG Access events have this code    */

/*----------------------------- Includes -----------------------------------*/
#include <stdlib.h>
#ifndef UNIX
#include <conio.h>
#endif

#include "ctademo.h"  /* Natural Access demo library definitions                 */
#include "isdnval.h"  /* ISDN protocol-specific constants                 */
#include "nccdef.h"
#include "nccxadi.h"
#include "nccxisdn.h"

/*----------------------------- OS-specific definitions --------------------*/

/*
 *  Definitions for WaitForAnyEvent().  In UNIX we use the poll() function.
 */

#if defined (UNIX)
  #define POLL_TIMEOUT 0
  #define POLL_ERROR  -1
#endif

/*----------------------------- Application Definitions --------------------*/
typedef struct
{
    CTAQUEUEHD   ctaqueuehd;       /* Natural Access queue handle                  */
    CTAHD        ctahd;            /* Natural Access context handle                */
    SWIHD        swihd;            /* Switch context handle                   */
    FILE        *fp;               /* file pointer for play/record            */
    char        *buffer[2];        /* buffers for play/record                 */
    char         dialstring[33];   /* current number being dialed - the       */
    char         calling_nb[33];   /* current calling number */
    /* buffer max capacity is 32               */
    char         tempfile[12];     /* the temporary file used to play/record  */
    unsigned     thread_id;        /* index of the thread (if multithreaded)  */
    unsigned     interactive_flag; /* dial upon user command?                 */
    unsigned     slot;             /* the MVIP timeslot to run on             */
    NCC_CALLHD   callhd;           /* an NCC call handle: only 1 in this demo */
} MYCONTEXT;

typedef struct
{
    NCC_ISDN_TRANSPARENT_BUFFER_INVOKE x_msg;
    char fill[15];
} SEND_CALL_MSG;
    SEND_CALL_MSG xpar_msg;

/*----------------------------- Application Data ---------------------------*/
unsigned   mode        = SINGLE; /* default is: one port per process        */
unsigned   ag_board    = 0;      /* the first board                         */
unsigned   mvip_stream = UNDETERMINED_STREAM; /* the DSP stream             */
unsigned   mvip_slot   = 0;      /* the default port                        */
unsigned   mvip_bus    = 0;      /* bus type                                */
unsigned   ISDN_prefmode   = FALSE;  /* PREFERRED mode for ISDN (def EXCLUSIVE) */
unsigned   interactive_flag = 0; /* do not prompt the user before dialing   */
unsigned   tcp_lowlevel_msg = 0x0;
unsigned   loadparms = TRUE;
unsigned   sendUUI = FALSE;
unsigned   sendredirecting = FALSE;
unsigned   callingname = FALSE;
unsigned   res_management = FALSE;
unsigned   acceptmode = 0;       /* none of the legal modes                 */
unsigned   ringnumber = 2;
unsigned   mvip_skip = 0;        /* used by the switching functions to
                                    to discriminate beetwen MVIP 90 and
                                    MVIP 95                                 */
char       dialstring[33] = "123";
char       calling_nb[33] = "987";
BOOL       provokeReject = FALSE;/* if TRUE, allows the call rejection      */
                                 /* based on nccstatus.calledaddr[0]        */
DWORD      rx_calls = 0;         /* number of received calls                */
DWORD      sx_calls = 0;         /* number of seizured calls                */
DWORD      ix_calls = 0;         /* number of initiated calls               */
DWORD      ex_calls = 0;         /* number of ended calls                   */
DWORD      userid = 0;

char       textbuffer[80];
unsigned char s_xpar_ie [16];
int        s_xpar_ie_size = 0;
unsigned char i_xpar_ie [16];
int        i_xpar_ie_size = 0;
unsigned char n_xpar_ie [16];
int        n_xpar_ie_size = 0;
unsigned char a_xpar_ie [16];
int        a_xpar_ie_size = 0;
unsigned char c_xpar_ie [16];
int        c_xpar_ie_size = 0;
int        ie_list_index;

/*----------------------------- Forward References -------------------------*/
THREAD_RET_TYPE THREAD_CALLING_CONVENTION RunDemo ( void * pcx );

int   SpeakDigits      (MYCONTEXT *cx, char *digits);
DWORD WaitForAnyEvent  (CTAHD ctahd, CTA_EVENT *eventp);
DWORD MyPlaceCall      (MYCONTEXT *cx);
DWORD MyReceiveCall    (MYCONTEXT *cx, unsigned call_received);
void  ConnectBChannel  (MYCONTEXT *cx, NCC_ISDN_EXT_CALL_STATUS *status);
DWORD MyPlayMessage    (MYCONTEXT *cx, char *filename, unsigned  message);
int   MyHangUp         (CTAHD ctahd, NCC_CALLHD callhd, void* hangparms, void* disparms);
void  ShowCapabilities (NCC_PROT_CAP *capabilities);
void  MyRejectCall     ( CTAHD ctahd, NCC_CALLHD callhd, unsigned method, void* params );
/*----------------------------------------------------------------------------
PrintHelp()
----------------------------------------------------------------------------*/
void PrintHelp( void )
{
    printf( "Usage: isdnncc [options]\nwhere options are:\n" );

    printf( "\n1. general options\n" );
    printf( "-? or -h    Display this help screen and exit\n");
    printf( "-b n        AG board number (default: %d)\n", ag_board );
    printf( "-d          use protocol Default parameters (without loading the\n"
            "            country-specific parameter file)\n");
    printf( "-P          use PREFERRED mode (default: EXCLUSIVE) - ISDN only\n");
    printf( "-s timeslot MVIP timeslot to use. (default: 0)\n"
            "            The MVIP stream is the DSP stream of the board you are using\n");
    printf( "-t number of threads\n"
            "            This option demonstrates the TSA multithreaded programming model.\n"
            "            Launch any number of threads, as long as the same board is used\n"
            "            for all.  All threads will act the same way, placing or receiving\n"
            "            calls.  The timeslots used are the one specified with -s, and above.\n");
    printf( "-u          send user-to-user information (UUI)\n");
    printf( "-F          send name delivery\n");
    printf( "-v level    Verbosity level of messages printed on screen, where:\n"
            "            level = 0 --> error messages only\n"
            "            level = 1 --> errors and unexpected high-level events\n"
            "            level = 2 --> errors and all high-level events (default)\n"
            "            level = 3 --> errors and  all events\n" );

    printf( "\n2. inbound options\n" );
    printf( "-a mode     Accept the inbound calls before answering or rejecting, where:\n"
            "            mode = 1  --> accept and play ringback tone\n"
            "            mode = 2  --> accept and remain silent until further command\n"
            "            mode = 3  --> accept with user audio and play a voice file\n");
    printf( "-i          interactive dialing or inbound in two-way trunks\n");
    printf( "-r rings    number of rings tones to play before answering a call (default: %d)\n",
                         ringnumber);

    printf( "\n3. outbound options\n" );
    printf( "-n number   Number to dial.\n" );
    printf( "-D number   calling number.\n" );
    printf( "-R          redirecting number in Setup.\n" );
    printf( "-S string   transparent ie list for Setup. Quoted list of hex bytes\n"
            "            separated by spaces. e.g. -S\"96 1 1 C\"\n");
    printf( "-I string   transparent ie list for Info. Quoted list of hex bytes\n"
            "            separated by spaces. e.g. -I\"2C 3 32 33 35\"\n");
    printf( "-N string   transparent ie list for Notify. Quoted list of hex bytes\n"
            "            separated by spaces. e.g. -N\"27 1 F1 28 4 B6 41 42 43\"\n");
    printf( "-A string   transparent ie list for nccAcceptCall. Quoted list of hex bytes\n"
            "            separated by spaces. e.g. -A\"7E 4 4 55 55 49\"\n");
    printf( "-C string   transparent ie list for nccAnswerCall. Quoted list of hex bytes\n"
            "            separated by spaces. e.g. -C\"7E 4 4 75 75 69\"\n");
}

/*----------------------------------------------------------------------------
                            main
----------------------------------------------------------------------------*/
int main( int argc, char **argv )
{
    int c, level = 2;
    DWORD ret;
    unsigned servicenumber, threadsnumber, printhelp = FALSE, i;
    CTA_INIT_PARMS initparms = { 0 };
    CTA_ERROR_HANDLER hdlr;
    ADI_BOARD_INFO boardinfo;          /* ADI structure for board info     */
    DWORD temp_ctaqueuehd, temp_ctahd; /* for adiGetBoardInfo()            */
    MYCONTEXT  *cx;
    CTA_SERVICE_DESC services[] =        /* for ctaOpenServices */
    {   { {"ADI", "ADIMGR" }, { 0 }, { 0 }, { 0 } },
        { {"NCC", "ADIMGR" }, { 0 }, { 0 }, { 0 } },
        { {"VCE", "VCEMGR"}, { 0 }, { 0 }, { 0 } },
        { {"SWI", "SWIMGR"}, { 0 }, { 0 }, { 0 } }
    };
    char contextname[15];

    CTA_SERVICE_NAME servicelist[] =   /* for ctaInitialize                */
    {  { "ADI", "ADIMGR" },
       { "NCC", "ADIMGR" },
       { "VCE", "VCEMGR" },
       { "SWI", "SWIMGR" }
    };

    /* allocate and initialize the first context, that we certainly need   */
    cx = (MYCONTEXT *)malloc (sizeof(MYCONTEXT));

    cx->ctaqueuehd = NULL_CTAQUEUEHD;          /* initialize to safe value */
    cx->ctahd = NULL_CTAHD;                    /* initialize to safe value */
    cx->swihd = NULL_SWIHD;                    /* initialize to safe value */
    cx->thread_id = 0;                         /* thread id                */
    cx->callhd = NULL_NCC_CALLHD;

    printf ("Natural Access Two-way trunk Demo V.%s (%s)\n", VERSION_NUM, VERSION_DATE);

    while( ( c = getopt( argc, argv, "a:b:n:r:s:t:v:S:I:N:A:C:D:hdipPuFrR?" ) ) != -1 )
    {
      switch( c )
      {
      case 'a':
          switch (optarg[0])
          {
          case '1':
              acceptmode = NCC_ACCEPT_PLAY_RING;
              break;
          case '2':
              acceptmode = NCC_ACCEPT_PLAY_SILENT;
              break;
          case '3':
              acceptmode = NCC_ACCEPT_USER_AUDIO;
              break;
          case '4':
              acceptmode = NCC_ACCEPT_USER_AUDIO_INTO_CONNECT;
              break;
          default:
              printf ("? unsupported AcceptCall mode\n");
              exit (1);
          }
          break;
      case 'b':
          sscanf( optarg, "%d", &ag_board );
          break;
      case 'd':
          loadparms = FALSE;
          break;
      case 'i':
          interactive_flag = 1;
          break;
      case 'n':
          strncpy (dialstring, optarg, sizeof(dialstring));
          break;
      case 'p':
          provokeReject=TRUE;
          break;
      case 'D':
          strncpy(calling_nb, optarg, sizeof(calling_nb));
          break;
      case 'P':
          ISDN_prefmode = TRUE;
          break;
      case 'r':
          sscanf( optarg, "%d", &ringnumber );
          break;
      case 's':
          sscanf( optarg, "%d", &mvip_slot);
          cx->slot = mvip_slot;                 /* re-assign the timeslot */
          break;
      case 't':
          mode = THREADED;
          sscanf( optarg, "%d", &threadsnumber );
          break;
      case 'u':
          sendUUI = TRUE;
          break;
      case 'R':
          sendredirecting = TRUE;
          break;
      case 'F':
          callingname = TRUE;
          break;
      case 'v':
          sscanf( optarg, "%d", &level );
          break;
      case 'h':
      case '?':
          printhelp = TRUE;
          break;
      case 'S':
          s_xpar_ie_size = sscanf (optarg, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
                                 &s_xpar_ie[0], &s_xpar_ie[1], &s_xpar_ie[2], &s_xpar_ie[3],
                                 &s_xpar_ie[4], &s_xpar_ie[5], &s_xpar_ie[6], &s_xpar_ie[7],
                                 &s_xpar_ie[8], &s_xpar_ie[9], &s_xpar_ie[10], &s_xpar_ie[11],
                                 &s_xpar_ie[12], &s_xpar_ie[13], &s_xpar_ie[14], &s_xpar_ie[15]);
          break;
      case 'I':
          i_xpar_ie_size = sscanf (optarg, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
                                 &i_xpar_ie[0], &i_xpar_ie[1], &i_xpar_ie[2], &i_xpar_ie[3],
                                 &i_xpar_ie[4], &i_xpar_ie[5], &i_xpar_ie[6], &i_xpar_ie[7],
                                 &i_xpar_ie[8], &i_xpar_ie[9], &i_xpar_ie[10], &i_xpar_ie[11],
                                 &i_xpar_ie[12], &i_xpar_ie[13], &i_xpar_ie[14], &i_xpar_ie[15]);
          break;
      case 'N':
          n_xpar_ie_size = sscanf (optarg, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
                                 &n_xpar_ie[0], &n_xpar_ie[1], &n_xpar_ie[2], &n_xpar_ie[3],
                                 &n_xpar_ie[4], &n_xpar_ie[5], &n_xpar_ie[6], &n_xpar_ie[7],
                                 &n_xpar_ie[8], &n_xpar_ie[9], &n_xpar_ie[10], &n_xpar_ie[11],
                                 &n_xpar_ie[12], &n_xpar_ie[13], &n_xpar_ie[14], &n_xpar_ie[15]);
          break;
      case 'A':
          a_xpar_ie_size = sscanf (optarg, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
                                 &a_xpar_ie[0], &a_xpar_ie[1], &a_xpar_ie[2], &a_xpar_ie[3],
                                 &a_xpar_ie[4], &a_xpar_ie[5], &a_xpar_ie[6], &a_xpar_ie[7],
                                 &a_xpar_ie[8], &a_xpar_ie[9], &a_xpar_ie[10], &a_xpar_ie[11],
                                 &a_xpar_ie[12], &a_xpar_ie[13], &a_xpar_ie[14], &a_xpar_ie[15]);
          break;
      case 'C':
          c_xpar_ie_size = sscanf (optarg, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
                                 &c_xpar_ie[0], &c_xpar_ie[1], &c_xpar_ie[2], &c_xpar_ie[3],
                                 &c_xpar_ie[4], &c_xpar_ie[5], &c_xpar_ie[6], &c_xpar_ie[7],
                                 &c_xpar_ie[8], &c_xpar_ie[9], &c_xpar_ie[10], &c_xpar_ie[11],
                                 &c_xpar_ie[12], &c_xpar_ie[13], &c_xpar_ie[14], &c_xpar_ie[15]);
          break;
      default:
          printf("? Unrecognizable option -%c %s\n", c, optarg );
          exit( 1 );
          break;
      }
    }

    /* assigning the final values to the main context cx */

    strcpy (cx->dialstring, dialstring);       /* dialstring               */
    strcpy (cx->calling_nb, calling_nb);       /* dialstring               */
    cx->slot = mvip_slot;                      /* timeslot                 */

    /*
     * mvip_slot + ag_board*1000 is a unique number
     * in a Natural Access application: this is our temporary file name
     */
    sprintf (cx->tempfile, "%5.5d", mvip_slot + ag_board*1000);
    //printf("tempfile = %s\n", cx->tempfile );
    /* thus, the tempfile is "01004.vox", if slot is 4 on board 1 */

    /* assign the interactive behavior */
    cx->interactive_flag = interactive_flag;

    DemoSetReportLevel( DEMO_REPORT_MIN ); /* turn off CTAdemo printouts */

    /*
     * Natural Access initialization:
     * Register an error handler to display errors returned from API functions;
     * no need to check return values in the demo.
     */

    ctaSetErrorHandler( DemoErrorHandlerContinue, NULL );

    /* Initialize size of init parms structure */
    initparms.size = sizeof(CTA_INIT_PARMS);
    initparms.ctacompatlevel = CTA_COMPATLEVEL;

    /* If daemon running then initialize tracing
     * and system global default parameters.
     */
    initparms.traceflags = CTA_TRACE_ENABLE;
    initparms.parmflags  = CTA_PARM_MGMT_SHARED;

    /* Set error handler to NULL and remember old handler */
    ctaSetErrorHandler( NULL, &hdlr );

    /*
     * For ISDN, the switching service is needed (should the calls be placed
     * or received in PREFERRED mode), and of course the ISDN service.
     */
    servicenumber = 4;

    if ( ( ret = ctaInitialize( servicelist, servicenumber,
                               &initparms ) ) != SUCCESS )
    {
        printf("Unable to invoke ctaInitialize(). Error = 0x%x "
               "(ref ctaerr.h)\n",ret);
        exit (1);
    }
    else
      ctaSetErrorHandler( hdlr, NULL );  /* restore error handler */

    /*
     * Find out what type of board we have.
     * Assign the board's DSP voice stream as the one to open our ports on.
     */
    services[0].mvipaddr.board    = ag_board;
    services[0].mvipaddr.mode     = 0;
    services[1].mvipaddr.board    = ag_board;
    services[1].mvipaddr.mode     = 0;
    strcpy (contextname, "isdnncc");
    /* use the function in ctademo to open and initialize the context        */
    DemoOpenPort( 0, contextname, services, 1,
                  &temp_ctaqueuehd, &temp_ctahd );

    adiGetBoardInfo( temp_ctahd, ag_board, sizeof(ADI_BOARD_INFO), &boardinfo);

    if( boardinfo.trunktype == ADI_TRUNKTYPE_T1 || boardinfo.trunktype == ADI_TRUNKTYPE_E1 )
    {
        /* T1 or E1 board */
        if( boardinfo.numtrunks == 1 )
        {
            /* Single trunk board
             * These boards still support the MVIP 90 switching model.
             * Programs running on them must conform to MVIP 90 conventions.
             */
            mvip_stream = 18;
        }
        else
        {
            /* Dual or Quad trunk board
             * These boards instead only support the MVIP 95 switching model.
             * Programs running on them must conform to MVIP 95 conventions.
             * In particular, the type of bus must be declared.
             */
            mvip_bus = MVIP95_LOCAL_BUS;
            mvip_skip = 1;
            mvip_stream = 16;
        }
    }
    else if(  boardinfo.trunktype == ADI_TRUNKTYPE_BRI )
    {
            mvip_bus = MVIP95_LOCAL_BUS;
            mvip_skip = 1;
            mvip_stream = 4;       
    }
    else
    {
        printf( "Illegal board for this demo, board %d\n", ag_board );
        exit ( 1 );
    }
    /* close the temporary port we opened. */
    DemoShutdown (temp_ctahd);
    /* set the verbosity level now that all initialization tasks are finished */
    switch (level)
    {
    case 0:             /* errors only */
        DemoSetReportLevel( DEMO_REPORT_MIN );
        break;
    case 1:             /* errors + unexpected high-level events */
        DemoSetReportLevel( DEMO_REPORT_UNEXPECTED );
        break;
    case 2:             /* errors + all high-level events (default) */
        DemoSetReportLevel( DEMO_REPORT_COMMENTS );
        break;
    case 3:             /* errors + high- and low-level events */
        DemoSetReportLevel( DEMO_REPORT_ALL );
        tcp_lowlevel_msg = 0xffff;  /* everything */
        break;
    default:
        printf ("? WARNING: unsupported verbosity level - using default\n");
        break;
    }

    /*
     * now we know what MVIP stream we would get if we were to continue.
     * If the user asked to print a help text and exit, do it
     */
    if (printhelp)
    {
        PrintHelp();
        exit (1);
    }

    /*
     * now launch the RunDemo threads (if using threads) or call the RunDemo
     * function otherwise
     */
    if (mode == SINGLE)
    {
        if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
        {
            printf( "\tBoard      = %d\n",    ag_board );
            printf( "\tStream:Slot= %d:%d\n", mvip_stream, mvip_slot );
        }
        RunDemo( cx );
    }
    else
    {
        /* launch the first thread, that is already initialized */
        if (DemoSpawnThread( RunDemo, (void *)(cx) ) == FAILURE)
        {
            printf("ERROR during _beginthread... exiting\n");
            exit (1);
        }
        if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
        {
            printf( "\tBoard      = %d\n",    ag_board );
            printf( "\tStream:Slot= %d:%d\n", mvip_stream, cx->slot );
        }

        /* then do the other threads... */
        for (i=1; i<threadsnumber; i++)
        {
    /* allocate and initialize more cx's */
    cx = (MYCONTEXT *)malloc (sizeof(MYCONTEXT));

    cx->ctaqueuehd = NULL_CTAQUEUEHD;         /* initialize to null   */
    cx->ctahd = NULL_CTAHD;                   /* initialize to null   */
    cx->swihd = NULL_SWIHD;                   /* initialize to null   */
    cx->callhd = NULL_NCC_CALLHD;
    strcpy (cx->dialstring, dialstring);      /* dialstring           */
    strcpy (cx->calling_nb, calling_nb);      /* dialstring           */
    cx->thread_id = i;                        /* thread id            */
    cx->interactive_flag = interactive_flag;  /* interactive behavior */
    cx->slot = mvip_slot + i;                 /* timeslot             */

    /* assign a temp file name */
    sprintf (cx->tempfile, "%5.5d", cx->slot + ag_board*1000);
    /* use the OS-dependent function DemoSpawnThread to launch more threads */
    if (DemoSpawnThread( RunDemo, (void *)(cx) ) == FAILURE)
    {
        printf("ERROR during _beginthread... exiting\n");
        exit (1);
    }
    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
        printf( "\tStream:Slot= %d:%d\n", mvip_stream, cx->slot );
        }

        /* now the main thread sleeps forever */
        for(;;) DemoSleep( 1000 );
    }
    return 0;
}


/*-------------------------------------------------------------------------
  RunDemo
  -------------------------------------------------------------------------*/
THREAD_RET_TYPE THREAD_CALLING_CONVENTION RunDemo ( void *pcx )
{
    DWORD           ret;
    NCC_ADI_START_PARMS adiparms;
    NCC_START_PARMS     nccparms;
    NCC_ADI_ISDN_PARMS  isdnparms;
    CTA_EVENT       nextevent;
    MYCONTEXT *     cx;
    char            contextname[15];
    unsigned        numservices;
    char            parfile[15] = "nccxisdn.par";

    NCC_ISDN_EXT_CALL_STATUS isdstatus;  /* isdn protocol parameters */

    CTA_SERVICE_DESC services[] =        /* for ctaOpenServices */
    {   { {"ADI", "ADIMGR"}, { 0 }, { 0 }, { 0 } },
        { {"NCC", "ADIMGR"}, { 0 }, { 0 }, { 0 } },
        { {"VCE", "VCEMGR"}, { 0 }, { 0 }, { 0 } },
        { {"SWI", "SWIMGR"}, { 0 }, { 0 }, { 0 } }
    };

    cx = (MYCONTEXT *) pcx;

    numservices = 4;

    /* Fill in ADI service (index 0) MVIP address information                */
    services[0].mvipaddr.board    = ag_board;
    services[0].mvipaddr.stream   = mvip_stream;
    services[0].mvipaddr.timeslot = cx->slot;
    services[0].mvipaddr.bus      = mvip_bus;
    services[0].mvipaddr.mode     = ADI_FULL_DUPLEX;
    services[1].mvipaddr.board    = ag_board;
    services[1].mvipaddr.stream   = mvip_stream;
    services[1].mvipaddr.timeslot = cx->slot;
    services[1].mvipaddr.bus      = mvip_bus;
    services[1].mvipaddr.mode     = ADI_FULL_DUPLEX;
    sprintf (contextname, "isdnncc%d", cx->thread_id);

    /* use the function in ctademo to open and initialize the context        */
    DemoOpenPort( cx->thread_id, contextname, services, numservices,
                  &cx->ctaqueuehd, &cx->ctahd );

    /* Initiate a protocol on the port so we can  place and accept calls     */

    /*
     * Try  to load the optional parameters from the .par file.  If the .par
     * file is not there, use the defaults from the .pf file (automatically
     * loaded while initializing Natural Access with ctaInitialize().
     * The .par file is expected to be either in the current directory,
     * or in
     * \nms\ctaccess\cfg        (Windows NT)
     * /opt/nms/ctaccess/cfg    (UNIX)
     */
    if (loadparms == TRUE)
        /* try to load the protocol-specific parameter file   */
    {
        if (ctaLoadParameterFile (cx->ctahd, parfile) != SUCCESS)
            printf ("\n\tRunDemo: isdn protocol text parameter file nccxisdn.par not found:\n"
                    "\t         using default values from binary parameter file nccxisdn.pf\n\n");
    }

    /*
     * before starting the protocol, set the event mask to comprise some
     * low-level events  (see the inoutcta application notes document for
     * a complete list of the possible low-level events.
     */

    /* First retrieve the NCC_START_PARMID structure                         */

    ctaGetParms(cx->ctahd, NCC_START_PARMID, &nccparms, sizeof(NCC_START_PARMS));

    /* then set the callctl.eventmask parameter to report everything         */
    nccparms.eventmask =
        NCC_REPORT_ALERTING   | /* and alerting      */
        NCC_REPORT_ANSWERED   | /* and answered      */
        NCC_REPORT_STATUSINFO;  /* status updates    */

    /* Retrieve the NCC_ADI_ISDN_PARMID structure                           */
    ctaGetParms( cx->ctahd, NCC_ADI_ISDN_PARMID, &isdnparms, sizeof(NCC_ADI_ISDN_PARMS));

    if( DemoShouldReport( DEMO_REPORT_ALL ) )
        /* set the PROGRESS ISDN event to be captured */
        isdnparms.isdn_start.ISDNeventmask |= ISDN_REPORT_PROGRESS;

    /* if mode is preferred set the appropriate bit in the TCP               */
    if (ISDN_prefmode)
        isdnparms.isdn_start.exclusive = 0;

    nccStartProtocol (cx->ctahd, "isd0", &nccparms, NULL, &isdnparms.isdn_start );

    /* example of use of the CTADEMO function DemoWaitForSpecificEvent to    */
    /* wait for a specific event.                                            */

    do
    {
        DemoWaitForEvent( cx->ctahd, &nextevent );
    } while( nextevent.id != NCCEVN_START_PROTOCOL_DONE );

    switch (nextevent.value) {
        case CTA_REASON_FINISHED:
            if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                printf ("\tRunDemo: protocol successfully started on ctahd = %x\n", cx->ctahd);
            break;

        case NCCREASON_OUT_OF_RESOURCES:
            /*
             * This means that the board requires resource management. If resource
             * management is required, the application cannot ask the TCP to start
             * services to run in the connected state, but instead must start the
             * service itself, once the connected state is reached. The ADI
             * parameter stparms.mediamask in this case must be zero.
             * These services are:
             * - DTMF detector      \
             * - silence detector    | adiStartDTMFDetector
             * - energy detector    /
             */
            res_management = TRUE;  /* later we'll start services or assume already
                                       started  depending on the flag value */

            if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                printf ("\tRunDemo: mediamask not supported by protocol %s on this\n"
                        "\t         board - setting mediamask to zero and trying again\n",
                        "isd0");

            ctaGetParms(cx->ctahd, NCC_ADI_START_PARMID, &adiparms, sizeof(NCC_ADI_START_PARMS));
            adiparms.mediamask = 0;

            nccStartProtocol (cx->ctahd, "isd0", &nccparms, &adiparms, &isdnparms.isdn_start );

            do {
                DemoWaitForEvent( cx->ctahd, &nextevent );
            } while ( nextevent.id != NCCEVN_START_PROTOCOL_DONE );

            if (nextevent.value != CTA_REASON_FINISHED)
            {
                DemoShowError( "Start protocol failed", nextevent.value );
                exit( 1 );
            }
            else
                if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                    printf ("\tRunDemo: protocol successfully started with mediamask = 0x%x\n",
                            adiparms.mediamask);
            break;

          default:
            printf ("\tRunDemo: error in starting the protocol, value 0x%x\n",
                    nextevent.value);
            exit (1);
    }

    while (1) /* repeat until stopped */
    {
      if (cx->interactive_flag == 1)
      {
        if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
        {
          if (mode == THREADED)
            printf ("\tThread %d - Waiting for a call...\n",
                    cx->thread_id);
          else
              printf ("\tWaiting for a call, or press <enter> to dial\n");
          printf ("\n");
        }

        ret = WaitForAnyEvent( cx->ctahd, &nextevent );

        if (ret == SUCCESS)
        {
            switch (nextevent.id) {
                case KEYBOARD_EVENT:
                    printf ("\n");
                    if( mode != THREADED )
                        MyPlaceCall( cx );
                    break;

                case NCCEVN_SEIZURE_DETECTED: /* the line has been seized      */
                    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                    {
                        if( mode == THREADED )
                            printf( "\tThread %d - Seizure detected (seizured calls n.%d)\n",
                            cx->thread_id, ++sx_calls);
                        else
                            printf( "\tSeizure detected (seizured calls n.%d)\n", ++sx_calls );
                    }
                    cx->callhd = nextevent.objHd;   /* Call handle is included in
                                                        the event */
                    MyReceiveCall( cx, FALSE );
                    break;

                case NCCEVN_INCOMING_CALL:    /* a call waits to be answered   */
                    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                    {
                        if (mode == THREADED)
                            printf( "\tThread %d - Incoming call (n. %d). Acting as inbound now\n",
                                    cx->thread_id, ++rx_calls );
                        else
                            printf ( "\tIncoming call (n. %d). Acting as inbound now\n", ++rx_calls );
                    }

                    MyReceiveCall( cx, TRUE );
                    break;

                case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
                    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                    {
                        nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                        printf( "\n\tRunDemo: Extended call status update:\n");
                        if( nextevent.value & NCC_X_STATUS_INFO_UUI )
                            printf( "\tUUI = %s\n", isdstatus.UUI );
                        if( nextevent.value & NCC_X_STATUS_INFO_CONN_NAME )
                            printf( "\tConnected Name = %s\n", isdstatus.connectedname );
                        printf( "\n" );
                    }
                    break;

                case NCCEVN_PROTOCOL_ERROR:
                    /*
                    * an error occurred in the inbound TCP before the incoming
                    * call event: do nothing and wait for the next call
                    */
                    if( nextevent.value == NCC_PROTERR_NO_CS_RESOURCE )
                        if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                             printf( "\tRunDemo: call failed because of lack of"
                                     "  resource\n" );
                    break;

                case NCCEVN_LINE_OUT_OF_SERVICE:   /* the line went out of service  */
                    /* nothing can be done until it goes back in service again     */
                    if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                        printf( "\tRunDemo: line out of service "
                                "- waiting for in-service...\n" );
                    do {
                        if( WaitForAnyEvent( cx->ctahd, &nextevent ) != SUCCESS )
                        {
                            printf("RunDemo: WaitForAnyEvent FAILED\n");
                            exit(1);
                        }
                        if( nextevent.id != NCCEVN_LINE_IN_SERVICE )
                            if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                                    printf( "\tRunDemo: unexpected event id = 0x%x value = 0x%x\n",
                                                nextevent.id, nextevent.value );
                    } while( nextevent.id != NCCEVN_LINE_IN_SERVICE ); /* back in service */
                    if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                        printf( "\tRunDemo: line back in service\n");
                    break;

                default:
                    if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                        printf( "\tRunDemo: unexpected event id = 0x%x value = 0x%x\n",
                                 nextevent.id, nextevent.value );
                    break;
            } /* end of switch */

        } /* end of if (ret == SUCCESS) */
        else
        {
          ctaGetText( cx->ctahd, nextevent.id, textbuffer, sizeof(textbuffer) );
          printf( "Error detected: %s\n", textbuffer );
        }

      } /* end if (cx->interactive_flag == 1) */
      else
      {
        if( mode == THREADED )
          DemoSleep (2);  /* wait some to re-synchronize */
        MyPlaceCall( cx );
      }
    }  /*end of while (1)*/

    UNREACHED_STATEMENT( ctaDestroyQueue( cx->ctaqueuehd ); )
    UNREACHED_STATEMENT( THREAD_RETURN; )
}

/*-------------------------------------------------------------------------
                            MyReceiveCall

Receive a call.
-------------------------------------------------------------------------*/
DWORD MyReceiveCall ( MYCONTEXT *cx, unsigned call_received)
{

    DWORD             ret;
    char              option[2], fullname[CTA_MAX_PATH];
    VCE_RECORD_PARMS  rparms;
    ADI_COLLECT_PARMS cparms;
    REJECTCALL_EXT    rejectcall_ext;
    NCC_CALL_STATUS   nccstatus;       /* structure to contain call status */
    NCC_ISDN_EXT_CALL_STATUS isdstatus;       /* protocol-specific call status    */
    CTA_EVENT         nextevent;
    unsigned connected, recording, collecting, timer_done, call_accepted;
    VCEHD             vh;
    char              firstDigit;

    /*
     * If we are here because of a ADIEVN_SEIZURE_DETECTED, the call is still
     * in its setu phase. We need to receive an ADIEVN_INCOMING_CALL, that
     * tells us that the call is ready for us to decide what to do.
     * Otherwise, the call is ready for us to accept or reject
     */

    if( call_received == FALSE )  /* call setup is in progress */
    {
        while( !call_received )
        {
            ret = WaitForAnyEvent( cx->ctahd, &nextevent );
            if (ret == SUCCESS)
            {
                switch (nextevent.id)
                {
                case NCCEVN_SEIZURE_DETECTED:
                    /*
                     * expect a seizure event only if we are running a purely inbound
                     * protocol; otherwise we have already gotten it
                     */
                    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                        printf( "\tMyReceiveCall: Seizure detected (seizured calls n.%d)\n", ++sx_calls );
                    cx->callhd = nextevent.objHd;
                    break;

                case NCCEVN_INCOMING_CALL:    /* an incoming call has arrived    */
                    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                        printf( "\tMyReceiveCall: Incoming call (n. %d)\n", ++rx_calls );

                    call_received = TRUE;       /* exit the loop */
                    break;

                case NCCEVN_RECEIVED_DIGIT:
                    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                        printf( "\tMyReceiveCall: Digit detected:\t%c\n", nextevent.value );
                    break;

                case NCCEVN_CALL_DISCONNECTED:
                    /*
                     * a disconnect in this phase means that the call failed.
                     * Hangup and wait for another call
                     */
                    if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                    {
                        printf( "\tMyReceiveCall: call setup failed - hanging up\n" );
                        nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                        printf( "\t               Q.931 reason for disconnect: %d\n",
                                isdstatus.releasecause );
                    }
                    goto hangup_in;

                case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
                    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                    {
                        nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                        printf( "\n\tMyReceiveCall: Extended call status update:\n");
                        if( nextevent.value & NCC_X_STATUS_INFO_UUI )
                            printf( "\tUUI = %s\n", isdstatus.UUI );
                        if( nextevent.value & NCC_X_STATUS_INFO_CONN_NAME )
                            printf( "\tConnected Name = %s\n", isdstatus.connectedname );
                        printf( "\n" );
                    }
                    break;

                case NCCEVN_PROTOCOL_ERROR:   /* an error occurred  */
                    /*
                     * a protocol error occurred. On a AG Quad board used with resource
                     * management, this might mean that the TCP failed to get the call
                     * setup resource on time. In all cases, a protocol error at this
                     * stage means that the call will not be presented: return.
                     */
                    if( nextevent.value == NCC_PROTERR_NO_CS_RESOURCE )
                    {
                        if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                            printf( "\tMyReceiveCall: call failed because of lack of"
                                    " resource\n" );
                    }
                    ret = FAILURE;
                    return ret;

                default:
                    if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                        printf( "\tMyReceiveCall: while waiting for NCCEVN_INCOMING_CALL: \n"
                                "\t\tUnexpected event id = 0x%x value = 0x%x\n",
                                nextevent.id, nextevent.value );
                    break;
                }
            }
            else
            {
                ctaGetText( cx->ctahd, nextevent.id, textbuffer, sizeof(textbuffer) );
                printf( "MyReceiveCall: Error detected: %s\n", textbuffer );
                return ret;
            }
        }
    }

    /*
     * At this point, a call is ready to be processed.  Use nccGetCallStatus()
     * to get the information associated with it.
     */

    nccGetCallStatus( cx->callhd, &nccstatus, sizeof(NCC_CALL_STATUS) );
    nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );

    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
    {
        if( strlen (nccstatus.calledaddr) != 0 )
        {
            printf("\t\tcalled number:                   %s\n"
                   "\t\tcalled number plan:              %d\n"
                   "\t\tcalled number type:              %d\n",
                   nccstatus.calledaddr, isdstatus.callednumplan,
                   isdstatus.callednumtype);
        }
        if( strlen(nccstatus.callingaddr) != 0 )
        {
            printf("\t\tcalling number:                  %s\n"
                   "\t\tcalling number plan:             %d\n"
                   "\t\tcalling number type:             %d\n"
                   "\t\tcalling number screen:           %d\n"
                   "\t\tcalling number presentation:     %d\n",
                   nccstatus.callingaddr, isdstatus.callingnumplan,
                   isdstatus.callingnumtype, isdstatus.callingscreen,
                   isdstatus.callingpres);
        }
        if( isdstatus.origlineinfo >= 0 )
        {
            printf("\t\toriginating line info:           %d\n",
                   isdstatus.origlineinfo);
        }
        if( strlen (isdstatus.redirectingaddr) != 0 )
        {
            printf("\t\tredirecting number:              %s\n"
                   "\t\tredirecting number plan:         %d\n"
                   "\t\tredirecting number type:         %d\n"
                   "\t\tredirecting number screen:       %d\n"
                   "\t\tredirecting number presentation: %d\n",
                   isdstatus.redirectingaddr, isdstatus.redirectingplan,
                   isdstatus.redirectingtype, isdstatus.redirectingscreen,
                   isdstatus.redirectingpres);
        }
        if( strlen (isdstatus.originalcalledaddr) != 0 )
        {
            printf("\t\toriginal called number:              %s\n"
                   "\t\toriginal called number plan:         %d\n"
                   "\t\toriginal called number type:         %d\n"
                   "\t\toriginal called number screen:       %d\n"
                   "\t\toriginal called number presentation: %d\n"
                   "\t\tredirection counter:                 %d\n"
                   "\t\tcall forward no response:            %d\n",
                   isdstatus.originalcalledaddr, isdstatus.origcalledplan,
                   isdstatus.origcalledtype, isdstatus.origcalledscreen,
                   isdstatus.origcalledpres, isdstatus.origcalledcount,
                   isdstatus.origcalledcfnr);
        }

        if( isdstatus.callreference != 0 )
            printf("\t\tcall reference:                  %x\n",
                    isdstatus.callreference );

        if (strlen (isdstatus.UUI) > 0)
          printf("\n\tMyReceiveCall: UUI for the incoming call is\n"
                   "\t               %s\n\n", isdstatus.UUI);

    }

        if( strlen (nccstatus.callingname) != 0 )
        {
            printf("\t\tcalling name:                   %s\n",nccstatus.callingname);
        }

    /* using ISDN if mode = PREFERRED we must do the switching */
    if ( ISDN_prefmode == TRUE )    
        ConnectBChannel( cx, &isdstatus );

    /*
     * We can decide to accept the call for the moment. This might be
     * because, based on the associated information, we want to do something
     * that might require some time, i.e. a database search or placing an
     * outbound call to be then connected with this call.
     * Or we might want to play a voice file before answering.
     * This is captured by the global variable acceptmode (set with a command
     * line option).
     * To do this we call nccAcceptCall().
     */
    if( acceptmode != 0 )
    {
        if (a_xpar_ie_size > 0)
        {
          ACCEPTCALL_EXT  acceptcall_ext;

          memset( &acceptcall_ext, 0, sizeof( acceptcall_ext ));
          acceptcall_ext.size = sizeof(ACCEPTCALL_EXT);
          memcpy (&acceptcall_ext.ie_list, a_xpar_ie, a_xpar_ie_size);
          acceptcall_ext.ie_list [a_xpar_ie_size] = 0;      /* must terminate the ie list with a zero */
          nccAcceptCall( cx->callhd, acceptmode, &acceptcall_ext);
        }
        else
          nccAcceptCall( cx->callhd, acceptmode, NULL );

        call_accepted = FALSE;

        while( !call_accepted )
        {
            WaitForAnyEvent( cx->ctahd, &nextevent );

            switch (nextevent.id)
            {

            case NCCEVN_ACCEPTING_CALL:
                call_accepted = TRUE;

                if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                    printf("\tMyReceiveCall: call accepted with mode %d\n",
                              nextevent.value);

                if(( nextevent.value == NCC_ACCEPT_USER_AUDIO_INTO_CONNECT ) ||
                   ( nextevent.value == NCC_ACCEPT_USER_AUDIO ))
                {
                    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                        printf( "\tMyReceiveCall: playing message before answering\n" );

                    if ((ret = MyPlayMessage( cx, "ctademo", PROMPT_WELCOME) ) != SUCCESS)
                        goto hangup_in;
                }
                else
                {
                /*
                 * it's either silence, and we wait silently for a while, or the TCP
                 * is playing rings, and we also wait for a while and then answer.
                 *
                 */
                  adiStartTimer( cx->ctahd, 8000, 1 );  /* wait eight seconds */

                  timer_done = FALSE;

                  while( !timer_done )
                  {
                    WaitForAnyEvent( cx->ctahd, &nextevent );
                    switch( nextevent.id )
                    {
                      case ADIEVN_TIMER_DONE:

                        if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                            printf ( "\tMyReceiveCall: timer done: exiting loop\n" );
                        timer_done = TRUE;
                        break;

                      case NCCEVN_CALL_DISCONNECTED:

                        adiStopTimer( cx->ctahd );
                        do
                        {
                          WaitForAnyEvent( cx->ctahd, &nextevent );
                          if( nextevent.id == NCCEVN_EXTENDED_CALL_STATUS_UPDATE )
                          {
                            if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                            {
                                nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                                printf( "\n\tMyReceiveCall: Extended call status update:\n");
                                if( nextevent.value & NCC_X_STATUS_INFO_UUI )
                                    printf( "\tUUI = %s\n", isdstatus.UUI );
                                if( nextevent.value & NCC_X_STATUS_INFO_CONN_NAME )
                                    printf( "\tConnected Name = %s\n", isdstatus.connectedname );
                                printf( "\n" );
                            }
                          }
                        } while (nextevent.id != ADIEVN_TIMER_DONE);

                        if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                        {
                          printf( "\tMyReceiveCall: call disconnected\n");
                          nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                          printf( "\t               Q.931 reason for disconnect: %d\n",
                                  isdstatus.releasecause);
                        }
                        ret = DISCONNECT;
                        goto hangup_in;

                      case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
                        if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                        {
                            nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                            printf( "\n\tMyReceiveCall: Extended call status update:\n");
                            if( nextevent.value & NCC_X_STATUS_INFO_UUI )
                                printf( "\tUUI = %s\n", isdstatus.UUI );
                            if( nextevent.value & NCC_X_STATUS_INFO_CONN_NAME )
                                printf( "\tConnected Name = %s\n", isdstatus.connectedname );
                            printf( "\n" );
                        }
                        break;

                      case KEYBOARD_EVENT:
                        break;

                      default:
                        if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                            printf("\tMyReceiveCall, while accepting: \n"
                                    "\t\tUnexpected event 0x%x value = 0x%x\n",
                                   nextevent.id, nextevent.value);
                        break;
                    }  /* switch */
                  }  /* while (timer_done)  */
                }  /* else   */
                break;

            case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
                if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                {
                    nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                    printf( "\n\tMyReceiveCall: Extended call status update:\n");
                    if( nextevent.value & NCC_X_STATUS_INFO_UUI )
                        printf( "\tUUI = %s\n", isdstatus.UUI );
                    if( nextevent.value & NCC_X_STATUS_INFO_CONN_NAME )
                        printf( "\tConnected Name = %s\n", isdstatus.connectedname );
                    printf( "\n" );
                }
                break;

            default:
                if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                    printf("\tMyReceiveCall, while wating for ACCEPTING_CALL: \n"
                           "\t\tUnexpected event id = 0x%x value = 0x%x\n",
                                nextevent.id, nextevent.value);
                ret = FAILURE;
                goto hangup_in;
        } /* switch (nextevent.id) */
      } /* while (call_accepted) */
    } /* if (acceptmode) */

    /************************************************************************
    * Demo the rejection of a call based on the address information:
    * if the first digit is BUSY_DIGIT, play busy; if it's REJECT_DIGIT,
    * play reorder tone.  If CUSTOM_REJECT play SIT tone.
    *************************************************************************/

    // This rejection is available only if provokeReject is TRUE , ie 
    // the input flag '-p' has been set.
    // if provokeReject  is FALSE, firstDigit is arbitrarily set to '1' to avoid rejection

    if (provokeReject == TRUE)
    {
        firstDigit = nccstatus.calledaddr[0];
        memset( &rejectcall_ext, 0, sizeof( rejectcall_ext ) );
        rejectcall_ext.size = sizeof(REJECTCALL_EXT);
    }
    else firstDigit = '1';
    
    switch( firstDigit )
    {
      case REJECT_BUSY_DIGIT:
          rejectcall_ext.cause = 0x11; // USER_BUSY
          MyRejectCall( cx->ctahd, cx->callhd, NCC_REJECT_PLAY_BUSY, &rejectcall_ext );
          goto hangup_in;

      case REJECT_REORDER_DIGIT:
          rejectcall_ext.cause = 0x15; // CALL REJECTED
          MyRejectCall( cx->ctahd, cx->callhd, NCC_REJECT_PLAY_REORDER, &rejectcall_ext );
          goto hangup_in;

      case REJECT_SIT_DIGIT:
          rejectcall_ext.cause = 0x01; // UNALLOCATED NUMBER
          MyRejectCall( cx->ctahd, cx->callhd, NCC_REJECT_USER_AUDIO, &rejectcall_ext );
          goto hangup_in;

      case REJECT_IMMEDIATE_DIGIT:
          goto hangup_in;

      default:
          if (c_xpar_ie_size > 0)
          {
            ANSWERCALL_EXT  answercall_ext;

            memset( &answercall_ext, 0, sizeof( answercall_ext ));
            answercall_ext.size = sizeof(ANSWERCALL_EXT);
            memcpy (&answercall_ext.ie_list, c_xpar_ie, c_xpar_ie_size);
            answercall_ext.ie_list [c_xpar_ie_size] = 0;      /* must terminate the ie list with a zero */
            nccAnswerCall( cx->callhd, ringnumber, &answercall_ext);  /* ring the phone */
          }
          else
            nccAnswerCall( cx->callhd, ringnumber, NULL );  /* ring the phone */

          connected = FALSE;
          while ( !connected )
          {
            WaitForAnyEvent( cx->ctahd, &nextevent );
            switch (nextevent.id)
            {
            case NCCEVN_ANSWERING_CALL:
              if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                printf( "\tMyReceiveCall: Answering... \n" );
              break;

            case NCCEVN_CALL_DISCONNECTED:
              if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
              {
                printf ( "\tMyReceiveCall: Remote abandoned - hanging up\n" );
                nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                printf( "\t               Q.931 reason for disconnect: %d\n",
                        isdstatus.releasecause);
              }
              goto hangup_in;

            case NCCEVN_CALL_CONNECTED:
              if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                printf( "\tMyReceiveCall: Call connected\n");
              connected = TRUE;
              break;

            case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
                if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                {
                    nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                    printf( "\n\tMyReceiveCall: Extended call status update:\n");
                    if( nextevent.value & NCC_X_STATUS_INFO_UUI )
                        printf( "\tUUI = %s\n", isdstatus.UUI );
                    if( nextevent.value & NCC_X_STATUS_INFO_CONN_NAME )
                        printf( "\tConnected Name = %s\n", isdstatus.connectedname );
                    if( nextevent.value & NCC_X_STATUS_INFO_CONN_NUMBER )
                        printf("\t\tconnected number:                %s\n"
                               "\t\tconnected number plan:           %d\n"
                               "\t\tconnected number type:           %d\n"
                               "\t\tconnected number screen:         %d\n"
                               "\t\tconnected number presentation:   %d\n",
                               isdstatus.connectedaddr, isdstatus.connectedplan,
                               isdstatus.connectedtype, isdstatus.connectedscreen,
                               isdstatus.connectedpres);
                    printf( "\n" );
                }
                break;

            default:
              if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                printf( "\tMyReceiveCall: while in connected state:\n"
                        "\t\tUnexpected event id = 0x%x value = 0x%x\n",
                            nextevent.id, nextevent.value );
              break;
          }
        }
        break;
    }

    /* Send INFO or NOTIFY messages */
    
    if (i_xpar_ie_size > 0)
    {
        memset( &xpar_msg, 0, sizeof( xpar_msg ) );
        xpar_msg.x_msg.hdr.message_id = NCC_ISDN_TRANSPARENT_BUFFER;
        xpar_msg.x_msg.isdn_message = NCC_ISDN_INFO;
        xpar_msg.x_msg.size = i_xpar_ie_size;   /* size of xpar data */
        memcpy (&xpar_msg.x_msg.buffer, i_xpar_ie, i_xpar_ie_size);
        if (nccSendCallMessage( cx->callhd,
                                &xpar_msg,
                                (sizeof(NCC_ISDN_TRANSPARENT_BUFFER_INVOKE) - 1) + i_xpar_ie_size) == SUCCESS)
        {
            printf ("\tMyReceiveCall: Sending transparent INFO message\n");
        }
        else
        {
            printf ("\tMyReceiveCall: Could not send transparent INFO message\n");
        }
    }

    if (n_xpar_ie_size > 0)
    {
        memset( &xpar_msg, 0, sizeof( xpar_msg ) );
        xpar_msg.x_msg.hdr.message_id = NCC_ISDN_TRANSPARENT_BUFFER;
        xpar_msg.x_msg.isdn_message = NCC_ISDN_NOTIFY;
        xpar_msg.x_msg.size = n_xpar_ie_size;   /* size of xpar data */
        memcpy (&xpar_msg.x_msg.buffer, n_xpar_ie, n_xpar_ie_size);
        if (nccSendCallMessage( cx->callhd,
                                &xpar_msg,
                                (sizeof(NCC_ISDN_TRANSPARENT_BUFFER_INVOKE) - 1) + n_xpar_ie_size) == SUCCESS)
        {
            printf ("\tMyReceiveCall: Sending transparent NOTIFY message\n");
        }
        else
        {
            printf ("\tMyReceiveCall: Could not send transparent NOTIFY message\n");
        }
    }

    /*************************************************************************
    *  Start caller interaction.  We do the following:
    *  - we announce the digits dialed (digprmpt.vce, and all the digits in
    *    called_digits[])
    *  - if available, we announce the calling number (callingd.vce, and all
    *    the digits in calling_digits[])
    *  - we play another prompt, (option.vce) giving the user three options:
    *    record (dtmf 1), play (dtmf 2), or hangup (dtmf 3)
    *  - we wait for the dtmf, and we do what we're asked for
    *  - we hangup
    **************************************************************************/
    adiFlushDigitQueue( cx->ctahd );

    /*
     * if resource management is needed, start the DTMF detector.  If not, the
     * TCP already did it
     */
    if( res_management )
      if( (ret = adiStartDTMFDetector( cx->ctahd, NULL ) ) != SUCCESS )
      {
        printf("\tMyReceiveCall: could not start the DTMF detector\n");
        goto hangup_in;
      }

    /* Announce the called number, if available.                             */
    if( strlen( nccstatus.calledaddr ) > 0 )
    {
      if ((ret = MyPlayMessage( cx, "ctademo", PROMPT_CALLED) ) != SUCCESS )
        goto hangup_in;

      if( (ret = SpeakDigits( cx, nccstatus.calledaddr ) ) != SUCCESS )
        goto hangup_in;
    }

    /* Loop, giving the user three options: record, playback, hangup.        */
    for(;;)
    {
        if( (ret = MyPlayMessage( cx, "ctademo", PROMPT_OPTION) ) != SUCCESS )
            goto hangup_in;
        memset( &cparms, 0 , sizeof( ADI_COLLECT_PARMS) );
        ctaGetParms( cx->ctahd, ADI_COLLECT_PARMID, &cparms, sizeof( ADI_COLLECT_PARMS ) );
        cparms.firsttimeout = 3000;  /* 3 seconds to wait for first digit */
        cparms.intertimeout = 3000;  /* 3 seconds between digits */

        adiCollectDigits( cx->ctahd, option, 1, &cparms );

        if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
            printf("\tMyReceiveCall: collecting DTMF...\n");

        collecting = TRUE;

        while( collecting )
        {
            WaitForAnyEvent( cx->ctahd, &nextevent );

            if( CTA_IS_ERROR( nextevent.id ) )
            {
                ret = FAILURE;
                goto hangup_in;
            }

            switch( nextevent.id )
            {
            case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
                if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                {
                    nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                    printf( "\n\tMyReceiveCall: Extended call status update:\n");
                    if( nextevent.value & NCC_X_STATUS_INFO_UUI )
                        printf( "\tUUI = %s\n", isdstatus.UUI );
                    if( nextevent.value & NCC_X_STATUS_INFO_CONN_NAME )
                        printf( "\tConnected Name = %s\n", isdstatus.connectedname );
                    if( nextevent.value & NCC_X_STATUS_INFO_CONN_NUMBER )
                        printf("\t\tconnected number:                %s\n"
                               "\t\tconnected number plan:           %d\n"
                               "\t\tconnected number type:           %d\n"
                               "\t\tconnected number screen:         %d\n"
                               "\t\tconnected number presentation:   %d\n",
                               isdstatus.connectedaddr, isdstatus.connectedplan,
                               isdstatus.connectedtype, isdstatus.connectedscreen,
                               isdstatus.connectedpres);
                    printf( "\n" );
                }
                break;

            case ADIEVN_COLLECTION_DONE:
                if( nextevent.value == CTA_REASON_RELEASED )
                    break;

                else if( CTA_IS_ERROR( nextevent.value ) )
                {
                    ret = FAILURE;
                    goto hangup_in;
                }
                else if( strlen( option ) == 0 )
                {
                    ret = FAILURE;
                    goto hangup_in;
                }
                else
                {
                    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                        printf( "\tMyReceiveCall: received DTMF '%c'\n", option[0] );
                }
                collecting = FALSE;
                break;

            case NCCEVN_CALL_DISCONNECTED:
                if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                {
                    printf("\tMyReceiveCall: remote hung up while collecting DTMF\n");
                    nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                    printf( "\t               Q.931 reason for disconnect: %d\n",
                            isdstatus.releasecause);
                }
                ret = DISCONNECT;
                goto hangup_in;

            case ADIEVN_DTMF_DETECT_DONE:
                /* this comes if the other side disconnects (reason: Released) */
                break;

            case ADIEVN_DIGIT_BEGIN:
            case ADIEVN_DIGIT_END:
                break;

            default:
                if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                    printf ("\tMyReceiveCall: while collecting: \n"
                            "\t\tUnexpected event id = 0x%x value = 0x%x\n",
                                nextevent.id, nextevent.value);
                break;
            }
        }

        switch( *option )
        {
        case '1':
            /* Record the caller, overriding default parameters for
             * quick recognition of silence once voice input has begun.
             */
            ctaGetParms( cx->ctahd, VCE_RECORD_PARMID,
                         &rparms, sizeof(rparms) );

            rparms.silencetime = 1000; /* 1 sec */

            /*
             * Record called party, overriding default parameters for
             * quick recognition of silence once voice input has begun.
             */

            sprintf( fullname, "./%s.vox", cx->tempfile);
            remove( fullname );

            if (
              (vceOpenFile (cx->ctahd, cx->tempfile, VCE_FILETYPE_VOX,
                            VCE_PLAY_RECORD, ADI_ENCODE_NMS_24, &vh ) != SUCCESS ) &&
              (vceCreateFile (cx->ctahd, cx->tempfile, VCE_FILETYPE_VOX,
                              ADI_ENCODE_NMS_24, NULL, &vh ) != SUCCESS )
             )


            {
                printf ("\tMyReceiveCall: could not open file %s for recording\n",
                         cx->tempfile);
                ret = FAILURE;
                goto hangup_in;
            }

            if( vceRecordMessage (vh, 0, 0, &rparms) != SUCCESS )
            {              

                vceClose( vh );
                printf( "\tMyReceiveCall: could not record message\n" );
                ret = FAILURE;
                goto hangup_in;
            }

            if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                printf( "\tMyReceiveCall: recording file %s.vox\n", cx->tempfile );

            recording = TRUE;

            while( recording )
            {
                WaitForAnyEvent( cx->ctahd, &nextevent );

                if( CTA_IS_ERROR (nextevent.id) )
                {
                  ret = FAILURE;                


                  vceClose( vh );
                  goto hangup_in;
                }

                switch ( nextevent.id )
                {
                  case VCEEVN_RECORD_DONE:
                    if( nextevent.value == CTA_REASON_RELEASED )
                    {
                      if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                        printf( "\tMyReceiveCall: Remote hung up during recording\n" );
                      break;
                    }

                    if( CTA_IS_ERROR( nextevent.value ) )
                    {
                      printf( "\tMyReceiveCall: record function failed\n" );
                      ret = FAILURE;

                     


                      vceClose( vh );
                      goto hangup_in;
                    }
                    else
                    {
                      if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                        printf( "\tMyReceiveCall: record done\n");
                      recording = FALSE;
                    }
                    break;

                 case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
                    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                    {
                        nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                        printf( "\n\tMyReceiveCall: Extended call status update:\n");
                        if( nextevent.value & NCC_X_STATUS_INFO_UUI )
                            printf( "\tUUI = %s\n", isdstatus.UUI );
                        if( nextevent.value & NCC_X_STATUS_INFO_CONN_NAME )
                            printf( "\tConnected Name = %s\n", isdstatus.connectedname );
                        if( nextevent.value & NCC_X_STATUS_INFO_CONN_NUMBER )
                            printf("\t\tconnected number:                %s\n"
                                   "\t\tconnected number plan:           %d\n"
                                   "\t\tconnected number type:           %d\n"
                                   "\t\tconnected number screen:         %d\n"
                                   "\t\tconnected number presentation:   %d\n",
                                   isdstatus.connectedaddr, isdstatus.connectedplan,
                                   isdstatus.connectedtype, isdstatus.connectedscreen,
                                   isdstatus.connectedpres);
                        printf( "\n" );
                    }
                    break;

                case NCCEVN_CALL_DISCONNECTED:
                   if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                   {
                      printf( "\tMyReceiveCall: caller hung up during recording\n" );
                      nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                      printf( "\t               Q.931 reason for disconnect: %d\n",
                              isdstatus.releasecause );
                   }
                  


                   vceClose( vh );
                   goto hangup_in;

                  case ADIEVN_DIGIT_BEGIN:
                  case ADIEVN_DIGIT_END:
                    break;

                  case KEYBOARD_EVENT:             /* do nothing - it's accidental */
                    break;

                  default:
                    if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                      printf ("\tMyReceiveCall: while recording: \n"
                              "\t\tUnexpected event id = 0x%x value = 0x%x\n",
                                    nextevent.id, nextevent.value);
                    break;
                }
            }
           



            vceClose( vh );
            break;

        case '2':
        /*
         * Play back anything the caller recorded.  This might be something
         * recorded last time the demo was run, or the file might not exist.
         */

          MyPlayMessage (cx, cx->tempfile, 0);
          break;

        case '3':  /* the other side requested the disconnect */
          goto hangup_in;

        default:
          if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
            printf( "\t Invalid selection: '%c'\n", *option );
          break;
      }
    }
        
    hangup_in:
      MyHangUp( cx->ctahd, cx->callhd, NULL, NULL );

    return ret;
}

/*-----------------------------------------------------------------------------
                            MyPlaceCall

 Place a call.
-----------------------------------------------------------------------------*/
DWORD MyPlaceCall ( MYCONTEXT *cx )
{
    DWORD ret;
    char *digit_to_play;
    VCEHD vh;                             /* to record and play voice files    */
    VCE_RECORD_PARMS         rparms;      /* structure to contain record parms */
    NCC_ISDN_EXT_CALL_STATUS isdstatus;
    CTA_EVENT nextevent;
    unsigned recording = TRUE, playing = TRUE;
    unsigned wait_for_voice = TRUE;
    char     fullname[CTA_MAX_PATH];

    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
        printf( "\n\tMyPlaceCall: Initiating a new call (calls n. %d)\n", ++ix_calls );

    if ((sendUUI) || (s_xpar_ie_size > 0) || (callingname) || (sendredirecting))
    {
       PLACECALL_EXT  placecall_ext;

       memset( &placecall_ext, 0, sizeof( placecall_ext ) );
       placecall_ext.size = sizeof(PLACECALL_EXT);
       ie_list_index = 0;
       if(sendUUI)
       {
            char *UUI = "ISDNNCC - NMS Communications";

            /* Supported only in 4ess, 5ess, ni2, etsi, vn6, ntt, korea, taiwan */
            placecall_ext.ie_list[0] = 0x7E;           /* UU IE identifier      */
            placecall_ext.ie_list[1] = strlen(UUI)+1;  /* UU IE contents length = string + protocol discriminator */
            placecall_ext.ie_list[2] = 0x04;           /* UU protocol = IA5     */

            strcpy( placecall_ext.ie_list+3,UUI);
            ie_list_index += strlen(UUI) + 3;          /* index where next ie could be placed */
       }
       if (s_xpar_ie_size > 0)
       {
            memcpy (&placecall_ext.ie_list [ie_list_index], s_xpar_ie, s_xpar_ie_size);
            ie_list_index += s_xpar_ie_size;
       }
       placecall_ext.ie_list [ie_list_index] = 0;      /* must terminate the ie list with a zero */

       if (callingname)
            strcpy( placecall_ext.callingname, "NMS Communications" );

       if (sendredirecting)
            strcpy( placecall_ext.redirectingnumber.digits, "4023654748" );

       nccPlaceCall( cx->ctahd, cx->dialstring, cx->calling_nb, NULL, &placecall_ext, &cx->callhd );

    }
    else
        nccPlaceCall( cx->ctahd, cx->dialstring, cx->calling_nb, NULL, NULL, &cx->callhd );

    while (1)
    {
        ret = WaitForAnyEvent( cx->ctahd, &nextevent );
        if( ret != SUCCESS )
        {
          printf( "MyPlaceCall: WaitForAnyEvent FAILED\n" );
          exit(1);
        }

        switch( nextevent.id )
        {
          case NCCEVN_PLACING_CALL:        /* Glare resolved                 */
            if ( ISDN_prefmode == TRUE )   /* if mode = PREFERRED
                                              we must do the swicthing       */
            {
                nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                ConnectBChannel( cx, &isdstatus );
            }
            break;

          case NCCEVN_PROTOCOL_EVENT:     /* event from the ISDN TCP */
              if( nextevent.value == ISDN_PROGRESS )
              {
                  if( DemoShouldReport( DEMO_REPORT_ALL ) )
                        printf ("\tMyPlaceCall: Progress message\n");
              }
              else
              {
                  if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                      printf( "\tMyPlaceCall: Unexpected event id = 0x%x value = 0x%x\n",
                              nextevent.id, nextevent.value );
              }
              break;

          case NCCEVN_CALL_PROCEEDING:     /* all digits delivered           */
            if( DemoShouldReport( DEMO_REPORT_ALL ) )
              printf( "\tMyPlaceCall: Dialing completed\n" );
            break;

          case NCCEVN_REMOTE_ALERTING:     /* confirmation of ringing        */
            if( DemoShouldReport( DEMO_REPORT_ALL ) )
              printf( "\tMyPlaceCall: Phone is ringing\n" );
            break;

          /* WARNING: the following two situations are illegal if the        */
          /* trunk is a purely outbound one                                  */
          case NCCEVN_SEIZURE_DETECTED:    /* the line has been seized       */
            if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                printf( "\tMyPlaceCall: Glare detected, do inbound (call n.%d)\n", ++sx_calls );

            cx->callhd = nextevent.objHd;
            MyReceiveCall( cx, FALSE );
            return SUCCESS;

          case NCCEVN_INCOMING_CALL:       /* a call waits to be answered    */
            if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                printf( "\tMyPlaceCall: Glare detected, do inbound (call n. %d) \n", ++rx_calls );
            MyReceiveCall( cx, TRUE );
            return SUCCESS;

          case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
            if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
            {
                nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                printf( "\n\tMyPlaceCall: Extended call status update:\n" );                              

                if( nextevent.value & NCC_X_STATUS_INFO_UUI )
                    printf( "\tUUI = %s\n", isdstatus.UUI );
                if( nextevent.value & NCC_X_STATUS_INFO_CONN_NAME )
                    printf( "\tConnected Name = %s\n", isdstatus.connectedname );
                if( nextevent.value & NCC_X_STATUS_INFO_PROGRESSDESCR )
                    printf( "\tProgress Description = %d\n", isdstatus.progressdescr );
                if( nextevent.value & NCC_X_STATUS_INFO_CAUSE )
                    printf( "\tCause = %d\n", isdstatus.releasecause );
                printf( "\n" );
            }
            break;

          case NCCEVN_REMOTE_ANSWERED:     /* remote telephone picked up     */
            if( DemoShouldReport( DEMO_REPORT_ALL ) )
                printf( "\tMyPlaceCall: Remote answered\n" );
            break;

          case NCCEVN_CALL_CONNECTED:      /* conversation phase. Now the    */

            /* application can proceed with its processing                   */
            if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                printf( "\tMyPlaceCall: Connected.\n" );

            adiFlushDigitQueue( cx->ctahd );

            /*
             * use call progress detection to look for voice.  This is not
             * necessary if a call progress event is the reason for the
             * connected event that was just received
             */
            if( nextevent.value != NCC_CON_SIGNAL )
            {
              adiStartCallProgress( cx->ctahd, NULL );
              while( wait_for_voice )
              {
                WaitForAnyEvent( cx->ctahd, &nextevent );

                if( CTA_IS_ERROR( nextevent.id ) )
                  continue;

                switch ( nextevent.id )
                {
                  case ADIEVN_CP_RINGTONE:
                    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                      printf ("\tMyPlaceCall: Ring tone detected\n");
                    break;

                  case ADIEVN_CP_BUSYTONE:
                    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                      printf ("\tMyPlaceCall: Busy tone detected, abandoning...\n");
                    MyHangUp( cx->ctahd, cx->callhd, NULL, NULL ); /* this call a failure            */
                    return DISCONNECT;

                  case ADIEVN_CP_VOICE:
                    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                      printf ("\tMyPlaceCall: Voice detected, starting recording...\n");
                    adiStopCallProgress( cx->ctahd );
                    do
                    {
                      WaitForAnyEvent( cx->ctahd, &nextevent );
                      if( nextevent.id == NCCEVN_EXTENDED_CALL_STATUS_UPDATE )
                      {
                        if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                        {
                            nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                            printf( "\n\tMyReceiveCall: Extended call status update:\n");
                            if( nextevent.value & NCC_X_STATUS_INFO_UUI )
                                printf( "\tUUI = %s\n", isdstatus.UUI );
                            if( nextevent.value & NCC_X_STATUS_INFO_CONN_NAME )
                                printf( "\tConnected Name = %s\n", isdstatus.connectedname );
                            if( nextevent.value & NCC_X_STATUS_INFO_PROGRESSDESCR )
                                printf( "\tProgress Description = %d\n", isdstatus.progressdescr );
                            if( nextevent.value & NCC_X_STATUS_INFO_CAUSE )
                                printf( "\tCause = %d\n", isdstatus.releasecause );
                            if( nextevent.value & NCC_X_STATUS_INFO_CONN_NUMBER )
                                printf("\t\tconnected number:                %s\n"
                                       "\t\tconnected number plan:           %d\n"
                                       "\t\tconnected number type:           %d\n"
                                       "\t\tconnected number screen:         %d\n"
                                       "\t\tconnected number presentation:   %d\n",
                                       isdstatus.connectedaddr, isdstatus.connectedplan,
                                       isdstatus.connectedtype, isdstatus.connectedscreen,
                                       isdstatus.connectedpres);
                            printf( "\n" );
                        }
                      }
                    } while (nextevent.id != ADIEVN_CP_DONE);
                    wait_for_voice = FALSE;
                    break;

                  case ADIEVN_CP_SIT:
                    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                      printf ("\tMyPlaceCall: SIT detected, abandoning...\n");
                    MyHangUp( cx->ctahd, cx->callhd, NULL, NULL ); /* this call a failure            */
                    return DISCONNECT;

                  case ADIEVN_CP_DONE:
                    if( nextevent.value == NCCREASON_OUT_OF_RESOURCES )
                    {
                      if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                        printf ("\tMyPlaceCall: No resource for call progress\n");
                    }
                    else if( nextevent.value == CTA_REASON_TIMEOUT )
                    {
                      if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                        printf ("\tMyPlaceCall: Call progress done, no voice detected\n");
                    }
                    else
                    {
                      if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                        printf ("\tMyPlaceCall: Call progress done, reason 0x%x\n",
                                nextevent.value);
                    }
                    wait_for_voice = FALSE;
                    break;

                  case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
                    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                    {
                        nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                        printf( "\n\tMyPlaceCall: Extended call status update:\n");
                        if( nextevent.value & NCC_X_STATUS_INFO_UUI )
                            printf( "\tUUI = %s\n", isdstatus.UUI );
                        if( nextevent.value & NCC_X_STATUS_INFO_CONN_NAME )
                            printf( "\tConnected Name = %s\n", isdstatus.connectedname );
                        if( nextevent.value & NCC_X_STATUS_INFO_PROGRESSDESCR )
                            printf( "\tProgress Description = %d\n", isdstatus.progressdescr );
                        if( nextevent.value & NCC_X_STATUS_INFO_CAUSE )
                            printf( "\tCause = %d\n", isdstatus.releasecause );
                        if( nextevent.value & NCC_X_STATUS_INFO_CONN_NUMBER )
                            printf("\t\tconnected number:                %s\n"
                                   "\t\tconnected number plan:           %d\n"
                                   "\t\tconnected number type:           %d\n"
                                   "\t\tconnected number screen:         %d\n"
                                   "\t\tconnected number presentation:   %d\n",
                                   isdstatus.connectedaddr, isdstatus.connectedplan,
                                   isdstatus.connectedtype, isdstatus.connectedscreen,
                                   isdstatus.connectedpres);
                        printf( "\n" );
                    }
                    break;

                  case NCCEVN_PROTOCOL_EVENT:     /* event from the ISDN TCP */
                      if( nextevent.value == ISDN_PROGRESS )
                      {
                          if( DemoShouldReport( DEMO_REPORT_ALL ) )
                                printf ("\tMyPlaceCall: Progress message\n");
                      }
                      else
                      {
                          if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                              printf ("\tMyPlaceCall: Unexpected event id = 0x%x value = 0x%x\n",
                                      nextevent.id, nextevent.value);
                      }
                      break;

                  case NCCEVN_CALL_DISCONNECTED:
                      if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                      {
                          printf( "\tMyPlaceCall: Hangup received during call progress\n" );
                          nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                          printf( "\t             Q.931 reason for disconnect: %d\n",
                                  isdstatus.releasecause);
                      }
                      MyHangUp( cx->ctahd, cx->callhd, NULL, NULL );
                      return DISCONNECT;

                  case KEYBOARD_EVENT:
                      break;

                  default:
                    if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                      printf ("\tMyPlaceCall: while in call progress: \n"
                            "\t\tUnexpected event id = 0x%x value = 0x%x\n",
                              nextevent.id, nextevent.value);
                    break;
                  }
              }
            }

            if (i_xpar_ie_size > 0)
            {
                memset( &xpar_msg, 0, sizeof( xpar_msg ) );
                xpar_msg.x_msg.hdr.message_id = NCC_ISDN_TRANSPARENT_BUFFER;
                xpar_msg.x_msg.isdn_message = NCC_ISDN_INFO;
                xpar_msg.x_msg.size = i_xpar_ie_size;   /* size of xpar data */
                memcpy (&xpar_msg.x_msg.buffer, i_xpar_ie, i_xpar_ie_size);
                if (nccSendCallMessage( cx->callhd,
                                        &xpar_msg,
                                        (sizeof(NCC_ISDN_TRANSPARENT_BUFFER_INVOKE) - 1) + i_xpar_ie_size) == SUCCESS)
                {
                    printf ("\tMyPlaceCall: Sending transparent INFO message\n");
                }
                else
                {
                    printf ("\tMyPlaceCall: Could not send transparent INFO message\n");
                }
            }

            if (n_xpar_ie_size > 0)
            {
                memset( &xpar_msg, 0, sizeof( xpar_msg ) );
                xpar_msg.x_msg.hdr.message_id = NCC_ISDN_TRANSPARENT_BUFFER;
                xpar_msg.x_msg.isdn_message = NCC_ISDN_NOTIFY;
                xpar_msg.x_msg.size = n_xpar_ie_size;   /* size of xpar data */
                memcpy (&xpar_msg.x_msg.buffer, n_xpar_ie, n_xpar_ie_size);
                if (nccSendCallMessage( cx->callhd,
                                        &xpar_msg,
                                        (sizeof(NCC_ISDN_TRANSPARENT_BUFFER_INVOKE) - 1) + n_xpar_ie_size) == SUCCESS)
                {
                    printf ("\tMyPlaceCall: Sending transparent NOTIFY message\n");
                }
                else
                {
                    printf ("\tMyPlaceCall: Could not send transparent NOTIFY message\n");
                }
            }

            /*
             * Record called party, overriding default parameters for
             * quick recognition of silence once voice input has begun.
             */
            ctaGetParms(cx->ctahd, VCE_RECORD_PARMID,
                        &rparms, sizeof(VCE_RECORD_PARMS) );
            rparms.silencetime = 1000; /* 1 sec */

            sprintf( fullname, "./%s.vox", cx->tempfile);
            remove( fullname );
          


            if((vceOpenFile (cx->ctahd, cx->tempfile, VCE_FILETYPE_VOX,
                     VCE_PLAY_RECORD, ADI_ENCODE_NMS_24, &vh ) != SUCCESS ) &&
               (vceCreateFile (cx->ctahd, cx->tempfile, VCE_FILETYPE_VOX,
                                ADI_ENCODE_NMS_24, NULL, &vh ) != SUCCESS ))
            {
              printf ("\tMyPlaceCall: Could not open file %s for recording\n",
                       cx->tempfile);
              MyHangUp( cx->ctahd, cx->callhd, NULL, NULL );
              return FAILURE;
            }

            if( vceRecordMessage( vh, 0, 0, &rparms ) != SUCCESS )
            {             

              vceClose( vh );
              printf ("\tMyPlaceCall: Could not record message\n");
              MyHangUp( cx->ctahd, cx->callhd, NULL, NULL );

              return FAILURE;
            }

            recording = TRUE;

            while( recording )
            {
              WaitForAnyEvent( cx->ctahd, &nextevent );

              switch( nextevent.id )
              {
                case VCEEVN_RECORD_DONE:
                 
                  vceClose( vh );

                  if( nextevent.value == CTA_REASON_RELEASED )
                  {
                    printf ("\tMyPlaceCall: hung up during recording\n");
                    /* Wait for CALL_DISCONNECTED event */
                    break;
                  }

                  if( CTA_IS_ERROR( nextevent.value) )
                  {
                      printf ("\tMyPlaceCall: Record function failed\n");
                      MyHangUp( cx->ctahd, cx->callhd, NULL, NULL );
                                   
                      return FAILURE;
                  }
                  else
                  {
                    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                      printf( "\tMyPlaceCall: Record done\n");
                      printf( "\tAfter Record done\n");                           

                    recording = FALSE;
                  }
                  break;

                case NCCEVN_CALL_DISCONNECTED:
                  if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                  {
                    printf ("\tMyPlaceCall: Remote hung up during recording\n");
                    nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                    printf( "\t             Q.931 reason for disconnect: %d\n",
                            isdstatus.releasecause);
                  }

                  MyHangUp( cx->ctahd, cx->callhd, NULL, NULL ); /* this call a failure */
                  return DISCONNECT;

                case ADIEVN_DIGIT_BEGIN:
                case ADIEVN_DIGIT_END:
                  break;

                case NCCEVN_PROTOCOL_EVENT:     /* event from the ISDN TCP */
                  if( nextevent.value == ISDN_PROGRESS )
                  {
                      if( DemoShouldReport( DEMO_REPORT_ALL ) )
                            printf ("\tMyPlaceCall: Progress message\n");
                  }
                  else
                  {
                      if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                          printf ("\tMyPlaceCall: Unexpected event id = 0x%x value = 0x%x\n",
                                  nextevent.id, nextevent.value);
                  }
                  break;

                case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
                 if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                 {
                    nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                    printf( "\n\tMyPlaceCall: Extended call status update:\n");
                    if( nextevent.value & NCC_X_STATUS_INFO_UUI )
                        printf( "\tUUI = %s\n", isdstatus.UUI );
                    if( nextevent.value & NCC_X_STATUS_INFO_CONN_NAME )
                        printf( "\tConnected Name = %s\n", isdstatus.connectedname );
                    if( nextevent.value & NCC_X_STATUS_INFO_PROGRESSDESCR )
                        printf( "\tProgress Description = %d\n", isdstatus.progressdescr );
                    if( nextevent.value & NCC_X_STATUS_INFO_CAUSE )
                        printf( "\tCause = %d\n", isdstatus.releasecause );
                    if( nextevent.value & NCC_X_STATUS_INFO_CONN_NUMBER )
                        printf("\t\tconnected number:                %s\n"
                               "\t\tconnected number plan:           %d\n"
                               "\t\tconnected number type:           %d\n"
                               "\t\tconnected number screen:         %d\n"
                               "\t\tconnected number presentation:   %d\n",
                               isdstatus.connectedaddr, isdstatus.connectedplan,
                               isdstatus.connectedtype, isdstatus.connectedscreen,
                               isdstatus.connectedpres);
                    printf( "\n" );
                                               
                 }
                 break;

                case KEYBOARD_EVENT:             /* do nothing: it's accidental */
                  break;

                default:
                  if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                    printf ("\tMyPlaceCall: while recording: \n"
                            "\t\tUnexpected event id = 0x%x value = 0x%x\n",
                                nextevent.id, nextevent.value);                 
                  break;
              }
          
           

            }

            digit_to_play = "3";
            adiStartDTMF( cx->ctahd, digit_to_play, NULL );

            if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                printf ("\tMyPlaceCall: Playing DTMF %s \n", digit_to_play);

            /* '3' = hang-up for inbound */
            do
            {
              WaitForAnyEvent( cx->ctahd, &nextevent );

              if( CTA_IS_ERROR( nextevent.id ) )
                printf ("\tCTA_IS_ERROR\n");
                continue;

              switch( nextevent.id ){

                case ADIEVN_TONES_DONE:
                    break;

                case NCCEVN_CALL_DISCONNECTED:
                    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                    {
                      printf ("\tMyPlaceCall: Remote hung up during DTMF\n");
                      nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                      printf( "\t             Q.931 reason for disconnect: %d\n",
                              isdstatus.releasecause);
                    }
                    MyHangUp( cx->ctahd, cx->callhd, NULL, NULL );
                    return DISCONNECT;

                case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
                    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                    {
                        nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                        printf( "\n\tMyPlaceCall: Extended call status update:\n");
                        if( nextevent.value & NCC_X_STATUS_INFO_UUI )
                            printf( "\tUUI = %s\n", isdstatus.UUI );
                        if( nextevent.value & NCC_X_STATUS_INFO_CONN_NAME )
                            printf( "\tConnected Name = %s\n", isdstatus.connectedname );
                        if( nextevent.value & NCC_X_STATUS_INFO_PROGRESSDESCR )
                            printf( "\tProgress Description = %d\n", isdstatus.progressdescr );
                        if( nextevent.value & NCC_X_STATUS_INFO_CAUSE )
                            printf( "\tCause = %d\n", isdstatus.releasecause );
                        if( nextevent.value & NCC_X_STATUS_INFO_CONN_NUMBER )
                            printf("\t\tconnected number:                %s\n"
                                   "\t\tconnected number plan:           %d\n"
                                   "\t\tconnected number type:           %d\n"
                                   "\t\tconnected number screen:         %d\n"
                                   "\t\tconnected number presentation:   %d\n",
                                   isdstatus.connectedaddr, isdstatus.connectedplan,
                                   isdstatus.connectedtype, isdstatus.connectedscreen,
                                   isdstatus.connectedpres);
                        printf( "\n" );
                    }
                    break;

                case NCCEVN_PROTOCOL_EVENT:    /* event from the ISDN TCP */
                    if( nextevent.value == ISDN_PROGRESS )
                    {
                        if( DemoShouldReport( DEMO_REPORT_ALL ) )
                            printf ("\tMyPlaceCall: Progress message\n");
                        break;
                    }/* else fall through and report unexpected in the default case */

                default:
                    if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                        printf ("\tMyPlaceCall: while playing DTMF tones: \n"
                                "\t\tUnexpected event id = 0x%x value = 0x%x\n",
                                    nextevent.id, nextevent.value);

                    break;
              }
            } while( nextevent.id != ADIEVN_TONES_DONE );

            if( nextevent.value == CTA_REASON_RELEASED )
            {
                if ( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                    printf ("\tMyPlaceCall: Stop playing DTMF - Remote hangup...\n" );
                /* Wait for a disconnect event */
                    break;
            }
            else if( nextevent.value == CTA_REASON_FINISHED )
            {
                if ( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                    printf ("\tMyPlaceCall: Finished playing DTMF...\n" );
                MyHangUp( cx->ctahd, cx->callhd, NULL, NULL ); /* disconnect the call */
                return DISCONNECT;
            }

            break;

          case NCCEVN_LINE_OUT_OF_SERVICE:      /* the line went out of service  */
            /* nothing can be done until it goes back in service again      */
            if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
              printf( "\tMyPlaceCall: Call failed - line out of service.\n"
                      "\t(waiting for in-service)\n" );
            do
            {
              if( WaitForAnyEvent( cx->ctahd, &nextevent ) != SUCCESS )
              {
                printf ("MyPlaceCall: WaitForAnyEvent FAILED\n");
                exit(1);
              }
              if( nextevent.id != ADIEVN_IN_SERVICE)
                        printf ("MyPlaceCall: Unexpected event id = 0x%x value = 0x%x\n",
                                 nextevent.id, nextevent.value);
            } while( nextevent.id != NCCEVN_LINE_IN_SERVICE ); /* back in service */

            return NCCEVN_LINE_OUT_OF_SERVICE;

          case NCCEVN_CALL_DISCONNECTED:   /* a disconnect in this phase     */
            /* means that the call failed.  Hangup and dial another call     */

            if( nextevent.value == NCC_DIS_NO_CS_RESOURCE )
            {
              if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                printf ("\tMyPlaceCall: Call failed for lack of resource\n");
            }
            else
            {
              if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
              {
                printf ("\tMyPlaceCall: Call disconnected.\n");
                nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                printf( "\t             Q.931 reason for disconnect: %d\n",
                        isdstatus.releasecause);
              }
            }
            MyHangUp( cx->ctahd, cx->callhd, NULL, NULL );
            return DISCONNECT;

          case NCCEVN_PROTOCOL_ERROR:      /* an error occurred - consider   */
            if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
              printf("\tNCCEVN_PROTOCOL_ERROR: value = 0x%x\n", nextevent.value);

            MyHangUp( cx->ctahd, cx->callhd, NULL, NULL ); /* this call a failure            */
            return DISCONNECT;

          case NCCEVN_CALL_RELEASED:
              if( nextevent.value == NCC_RELEASED_GLARE )
              {
                if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                    printf( "\tMyPlaceCall: Call released due to glare\n" );
                break;
              }
              else if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                      printf ("\tMyPlaceCall: unexpected release event: \n"
                            "\t\tUnexpected event id = 0x%x value = 0x%x\n",
                              nextevent.id, nextevent.value);


              return DISCONNECT;

          case KEYBOARD_EVENT:             /* do nothing: it's accidental */
            break;

          default:
            if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                printf ("\tMyPlaceCall: Unexpected event id = 0x%x value = 0x%x\n",
                        nextevent.id, nextevent.value);
            break;
        } /* end of switch */

    } /* end of while */
}

/*-------------------------------------------------------------------------
 MyHangUp
                                    
 Hang up the call, waiting for the Call Released event.
-------------------------------------------------------------------------*/
int MyHangUp( CTAHD ctahd, NCC_CALLHD callhd, void* hangparms, void* disparms )
{
    CTA_EVENT nextevent;
    unsigned isdiscon = 0;
    NCC_CALL_STATUS   status;
    NCC_ISDN_EXT_CALL_STATUS isdstatus;  /* protocol-specific call status    */

    nccGetCallStatus(callhd, &status, sizeof(status));

    if (status.state != NCC_CALLSTATE_DISCONNECTED)
    {
        DemoReportComment( "MyHangUp: Initiating disconnect..." );
        nccDisconnectCall( callhd, disparms );

      do
      {
        if( WaitForAnyEvent( ctahd, &nextevent ) != SUCCESS )
        {
          printf ("\tMyHangUp: WaitForAnyEvent FAILED\n");
          exit(1);
        }
        switch ( nextevent.id){
            case ADIEVN_COLLECTION_DONE:
            case VCEEVN_PLAY_DONE:
            case VCEEVN_RECORD_DONE:
            case ADIEVN_DIGIT_END:
            case ADIEVN_TONES_DONE:
                break;
            case NCCEVN_CALL_DISCONNECTED:
                if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                {
                    printf ("\n\tMyHangUp: Disconnected\n");
                    nccGetExtendedCallStatus( callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                    printf( "\t                 Q.931 reason for disconnect: %d\n",
                            isdstatus.releasecause);
                }
                break;
            case KEYBOARD_EVENT:
                break;

            case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
                if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                {
                    nccGetExtendedCallStatus( callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                    printf( "\n\tMyHangUp: Extended call status update:\n");
                    if( nextevent.value & NCC_X_STATUS_INFO_UUI )
                        printf( "\tUUI = %s\n", isdstatus.UUI );
                    if( nextevent.value & NCC_X_STATUS_INFO_CONN_NAME )
                        printf( "\tConnected Name = %s\n", isdstatus.connectedname );
                    if( nextevent.value & NCC_X_STATUS_INFO_PROGRESSDESCR )
                        printf( "\tProgress Description = %d\n", isdstatus.progressdescr );
                    if( nextevent.value & NCC_X_STATUS_INFO_CAUSE )
                        printf( "\tCause = %d\n", isdstatus.releasecause );
                    printf( "\n" );
                }
                break;

            default:
                if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                  printf ("\tMyHangUp: while disconnecting: \n"
                            "\t\tUnexpected event id = 0x%x value = 0x%x\n",
                                nextevent.id, nextevent.value);
            break;
        }
      }  while (nextevent.id != NCCEVN_CALL_DISCONNECTED);
    }

    nccReleaseCall( callhd, hangparms );

    for (;;)
    {
        if( WaitForAnyEvent( ctahd, &nextevent ) != SUCCESS )
        {
            printf ("MyHangUp: WaitForAnyEvent FAILED\n");
            exit(1);
        }

        switch( nextevent.id ){
            case ADIEVN_COLLECTION_DONE:
            case VCEEVN_PLAY_DONE:
            case VCEEVN_RECORD_DONE:
            case ADIEVN_DIGIT_END:
            case ADIEVN_TONES_DONE:
                break;   /* swallow */
    
            case NCCEVN_CALL_RELEASED:
                if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                    printf("\tMyHangUp: Call done. (calls n. %d)\n\n", ++ex_calls);
                return DISCONNECT;

            case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
                if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                {
                    nccGetExtendedCallStatus( callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                    printf( "\n\tMyHangUp: Extended call status update:\n");
                    if( nextevent.value & NCC_X_STATUS_INFO_UUI )
                        printf( "\tUUI = %s\n", isdstatus.UUI );
                    if( nextevent.value & NCC_X_STATUS_INFO_CONN_NAME )
                        printf( "\tConnected Name = %s\n", isdstatus.connectedname );
                    if( nextevent.value & NCC_X_STATUS_INFO_PROGRESSDESCR )
                        printf( "\tProgress Description = %d\n", isdstatus.progressdescr );
                    if( nextevent.value & NCC_X_STATUS_INFO_CAUSE )
                        printf( "\tCause = %d\n", isdstatus.releasecause );
                    printf( "\n" );
                }
                break;
    
            case NCCEVN_CALL_DISCONNECTED:  /* Can happen only in client/server mode */
                if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
                {
                    printf ("\n\tMyHangUp: Disconnected\n");
                    nccGetExtendedCallStatus( callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                    printf( "\t                 Q.931 reason for disconnect: %d\n",
                            isdstatus.releasecause);
                }
                break;
    
            default:
                if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                      printf ("\tMyHangUp: while releasing: \n"
                              "\t\tUnexpected event id = 0x%x value = 0x%x\n",
                                nextevent.id, nextevent.value);
                break;
        }
    }

    UNREACHED_STATEMENT( return FAILURE; )
}

/*-------------------------------------------------------------------------
                    SpeakDigits
-------------------------------------------------------------------------*/
int SpeakDigits( MYCONTEXT * cx, char * digits )
{
    unsigned  ret;
    char     *pc;
    unsigned  realdigits[80];
    unsigned  i, j;

    for( pc = digits, i = 0; pc && *pc; pc++)
    {
      if( isdigit( *pc ) )
        realdigits[i++] = *pc -'0';
    }

    for( j = 0; j < i; j++ )
    {
      ret = MyPlayMessage( cx, "ctademo", realdigits[j] );
      if( ret != SUCCESS )
        break;
    }
    return ret;
}

/*-----------------------------------------------------------------------------
                            WaitForAnyEvent()

   The following function waits for either an AG event or for a
   keyboard event.  It returns the type of event.
-----------------------------------------------------------------------------*/
DWORD WaitForAnyEvent( CTAHD ctahd, CTA_EVENT *eventp )
{
    CTAQUEUEHD ctaqueuehd;
    DWORD ctaret ;

    ctaGetQueueHandle( ctahd, &ctaqueuehd );

    while (1)
    {
      ctaret = ctaWaitEvent( ctaqueuehd, eventp, 100 ); /* wait for 100 ms */
      if (ctaret != SUCCESS)
      {
          printf( "\n\tCTAERR_SVR_COMM 0x%x\n", ctaret);
          exit( 1 );
      }

      if( eventp->id == CTAEVN_WAIT_TIMEOUT )
      {
        if(kbhit())                 /* check if a key has been pressed    */
        {                           /* Note that for UNIXes, kbhit() is   */
                                    /* defined in ctademo.c               */
          getc(stdin);
          eventp->id = KEYBOARD_EVENT;   /* indicate that a key is ready  */
          return SUCCESS;
        }
      }
      else if( eventp->id == NCCEVN_CAPABILITY_UPDATE )
      {
        /* Capability update from the stack */
          if( DemoShouldReport( DEMO_REPORT_ALL ) )
          {
            NCC_PROT_CAP capabilities;

            if( nccQueryCapability( ctahd, &capabilities,
                                    sizeof(NCC_PROT_CAP) ) == SUCCESS  );
                ShowCapabilities( &capabilities );
        }
      }
      else return SUCCESS;
    }

}


/*-------------------------------------------------------------------------
                ConnectBChannel

The following function checks if the B channel has been assigned for this call
and performs the switching
-------------------------------------------------------------------------*/
void ConnectBChannel( MYCONTEXT * cx, NCC_ISDN_EXT_CALL_STATUS *status )
{
  SWI_TERMINUS *outputs, *inputs;
  DWORD ret;

  if ( status->timeslot != 0xff )   
  /* B channel has been assigned */
  {
    /* open a switching device if it's not already opened */
    if ( cx->swihd == NULL_SWIHD )
    {
      if ( mvip_bus == MVIP95_LOCAL_BUS )
        ret = swiOpenSwitch( cx->ctahd, "AGSW", ag_board, 0, &(cx->swihd) );
      else
        ret = swiOpenSwitch( cx->ctahd, "AGSW", ag_board, SWI_MVIP90, &(cx->swihd) );
      if( ret != SUCCESS )
      {
        printf( "swiOpenSwitch failed, %d, 0x%x",ret,ret );
        exit(1);
      }
    }

    /* make a bidirectional connection
       in MVIP 90 mode the same streams are used for the two directions
       i.e. 18:2 -> 16:5   16:5 -> 18:2
       in MVIP 95 mode we have to use different streams
       i.e. 16:2 -> 1:5    0:5 -> 17:2 */

    inputs  = (SWI_TERMINUS *)malloc(sizeof(SWI_TERMINUS)*2);
    outputs = (SWI_TERMINUS *)malloc(sizeof(SWI_TERMINUS)*2);
    inputs[0].stream = status->stream;
    inputs[0].timeslot = status->timeslot;
    inputs[0].bus = mvip_bus;
    outputs[0].stream = mvip_stream + mvip_skip;
    outputs[0].timeslot = cx->slot;
    outputs[0].bus = mvip_bus;
    inputs[1].stream = mvip_stream;
    inputs[1].timeslot = cx->slot;
    inputs[1].bus = mvip_bus;
    outputs[1].stream = status->stream + mvip_skip;
    outputs[1].timeslot = status->timeslot;
    outputs[1].bus = mvip_bus;
    ret = swiMakeConnection( cx->swihd, inputs, outputs, 2 );
    if( ret != SUCCESS )
    {
      printf( "swiMakeConnection failed, %d, 0x%x",ret,ret );
      exit(1);
    }
    else
      if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
        printf( "\t%d:%d connected to %d:%d\n", status->stream,
                status->timeslot, mvip_stream, cx->slot );

    free( inputs );
    free( outputs );
  }
  else   /* B channel not assigned */
  {
    printf( "Error: B channel was not assigned\n" );
    exit(1);
  }

}

/*-------------------------------------------------------------------------
                MyPlayMessage

  This function plays a message from a vox file, also expecting all the
  events that we need to consider
-------------------------------------------------------------------------*/
DWORD MyPlayMessage( MYCONTEXT * cx, char * filename, unsigned  message )
{
    VCEHD           vh;
    DWORD           ret;
    unsigned short  playing;
    CTA_EVENT       nextevent;
    NCC_ISDN_EXT_CALL_STATUS isdstatus;

    /* open the VOX file */
    if ( vceOpenFile( cx->ctahd, filename, VCE_FILETYPE_VOX, VCE_PLAY_ONLY,
                      ADI_ENCODE_NMS_24, &vh )  != SUCCESS )
        return FAILURE;

    if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
      printf ("\tMyPlayMessage: playing message %2.2d from file %s.vox ...",
              message, filename);
    if ( vcePlayList( vh, &message, 1, NULL ) != SUCCESS )
    {

        vceClose( vh );
        return FAILURE;
    }

    ret = SUCCESS;  /* assume playing will succeed */

    playing = TRUE;
    while( playing )
    {
      WaitForAnyEvent( cx->ctahd, &nextevent );

      if( CTA_IS_ERROR( nextevent.id ) )
        continue;

      switch( nextevent.id )
      {
        case VCEEVN_PLAY_DONE:
          playing = FALSE;
          if( nextevent.value == CTA_REASON_RELEASED )
          {
            if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
              printf ("\n\tMyPlayMessage: remote hung up while playing...\n");
            break;
          }
          if( CTA_IS_ERROR( nextevent.value ) )
          {
            printf ("\n\tMyPlayMessage: play function failed\n");
            ret = FAILURE;
          }
          else
          {
            if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
              printf (" done\n");
          }
          break;

        case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
            if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
            {
                nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                printf( "\n\tMyPlaceCall: Extended call status update:\n");
                if( nextevent.value & NCC_X_STATUS_INFO_UUI )
                    printf( "\tUUI = %s\n", isdstatus.UUI );
                if( nextevent.value & NCC_X_STATUS_INFO_CONN_NAME )
                    printf( "\tConnected Name = %s\n", isdstatus.connectedname );
                if( nextevent.value & NCC_X_STATUS_INFO_PROGRESSDESCR )
                    printf( "\tProgress Description = %d\n", isdstatus.progressdescr );
                if( nextevent.value & NCC_X_STATUS_INFO_CAUSE )
                    printf( "\tCause = %d\n", isdstatus.releasecause );
                printf( "\n" );
            }
            break;

        case NCCEVN_CALL_DISCONNECTED:
            if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
            {
                printf ("\n\tMyPlayMessage: remote hung up while playing\n");
                nccGetExtendedCallStatus( cx->callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                printf( "\t                 Q.931 reason for disconnect: %d\n",
                        isdstatus.releasecause);
            }
            playing = FALSE;
            ret = DISCONNECT;
            break;

        case ADIEVN_DIGIT_BEGIN:
        case ADIEVN_DIGIT_END:
          break;

        case KEYBOARD_EVENT:
          break;

        default:
          if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
            printf ("\n\tMyPlayMessage: while playing: \n"
                    "\t\tUnexpected event id = 0x%x value = 0x%x\n",
                        nextevent.id, nextevent.value);
          break;
      }
    }

  


    vceClose( vh );
    return ret;
}

/*----------------------------------------------------------------------------
                MyRejectCall

 Reject the call by playing busy or fast busy.
 Returns when the caller hangs up.
----------------------------------------------------------------------------*/
void MyRejectCall( CTAHD ctahd, NCC_CALLHD callhd, unsigned method, void* params )
{
    CTA_EVENT event;
    NCC_ISDN_EXT_CALL_STATUS isdstatus;
    ADI_TONE_PARMS sit[3];
    
    DemoReportComment( "Rejecting call..." );
    nccRejectCall( callhd, method, params );

    for (;;)
    {
        DemoWaitForEvent(ctahd, &event);

        switch( event.id )
        {
        case NCCEVN_REJECTING_CALL:
            /* if we are rejecting with user audio, generate a SIT tone */
            /* This is the tone for a Vacant Code - hll LSL */
            if (method == NCC_REJECT_USER_AUDIO)
            {
                memset( &sit, 0, sizeof( sit ) );
                sit[0].freq1  = 985;
                sit[0].ampl1  = -24;
                sit[0].ontime = 380;
                sit[0].iterations = 1;
                sit[1].freq1  = 1371;
                sit[1].ampl1  = -24;
                sit[1].ontime = 274;
                sit[1].iterations = 1;
                sit[2].freq1  = 1777;
                sit[2].ampl1  = -24;
                sit[2].ontime = 380;
                sit[2].iterations = 1;
                adiStartTones( ctahd, 3, sit );
            }
            break;

        case ADIEVN_TONES_DONE:
            if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
            {
                printf ("\n\tMyRejectCall: SIT tone transmitted.\n");
            }
            break;

        case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
            if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
            {
                nccGetExtendedCallStatus( callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                printf( "\n\tMyRejectCall: Extended call status update:\n");
                if( event.value & NCC_X_STATUS_INFO_UUI )
                    printf( "\tUUI = %s\n", isdstatus.UUI );
                if( event.value & NCC_X_STATUS_INFO_CONN_NAME )
                    printf( "\tConnected Name = %s\n", isdstatus.connectedname );
                if( event.value & NCC_X_STATUS_INFO_PROGRESSDESCR )
                    printf( "\tProgress Description = %d\n", isdstatus.progressdescr );
                if( event.value & NCC_X_STATUS_INFO_CAUSE )
                    printf( "\tCause = %d\n", isdstatus.releasecause );
                printf( "\n" );
            }
            break;

        case NCCEVN_CALL_DISCONNECTED:
            if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
            {
                printf ("\n\tMyRejectCall: Caller hung up.\n");
                nccGetExtendedCallStatus( callhd, &isdstatus, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
                printf( "\t                 Q.931 reason for disconnect: %d\n",
                        isdstatus.releasecause);
            }
            return;

        default:
            if( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
                        printf( "\tMyRejectCall: while rejecting \n"
                                "\t\tUnexpected event id = 0x%x value = 0x%x\n",
                                    event.id, event.value );
            break;
        }
    }
}

/*----------------------------------------------------------------------------
                        ShowCapabilities()

 Display capabilities.
----------------------------------------------------------------------------*/
void ShowCapabilities( NCC_PROT_CAP *capabilities )
{
    DWORD mask = capabilities->capabilitymask;
    printf( "\n\tCurrent Capabiliy Mask:\n" );

    if( mask & NCC_CAP_ACCEPT_CALL )
        printf( "\tNCC_CAP_ACCEPT_CALL supported\n");
    if( mask & NCC_CAP_SET_BILLING )
        printf( "\tNCC_CAP_SET_BILLING supported\n" );
    if( mask & NCC_CAP_OVERLAPPED_SENDING )
        printf( "\tNCC_CAP_OVERLAPPED_SENDING supported\n" );

    if( mask & NCC_CAP_HOLD_CALL )
        printf( "\tNCC_CAP_HOLD_CALL supported\n" );
    if( mask & NCC_CAP_SUPERVISED_TRANSFER )
        printf( "\tNCC_CAP_SUPERVISED_TRANSFER supported\n" );
    if( mask & NCC_CAP_AUTOMATIC_TRANSFER )
        printf( "\tNCC_CAP_AUTOMATIC_TRANSFER supported\n" );

    if( mask & NCC_CAP_EXTENDED_CALL_STATUS )
        printf( "\tNCC_CAP_EXTENDED_CALL_STATUS supported\n" );
    if( mask & NCC_CAP_SEND_CALL_MESSAGE )
        printf( "\tNCC_CAP_SEND_CALL_MESSAGE supported\n" );
    if( mask & NCC_CAP_SEND_LINE_MESSAGE )
        printf( "\tNCC_CAP_SEND_LINE_MESSAGE supported\n" );

    if( mask & NCC_CAP_HOLD_IN_ANY_STATE )
        printf( "\tNCC_CAP_HOLD_IN_ANY_STATE supported\n" );
    if( mask & NCC_CAP_DISCONNECT_IN_ANY_STATE )
        printf( "\tNCC_CAP_DISCONNECT_IN_ANY_STATE supported\n" );

    if( mask & NCC_CAP_MEDIA_IN_SETUP )
        printf( "\tNCC_CAP_MEDIA_IN_SETUP supported\n" );
    if( mask & NCC_CAP_CALLER_ID )
        printf( "\tNCC_CAP_CALLER_ID supported\n" );
    printf( "\n" );
     return;
}

