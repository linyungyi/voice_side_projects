/*****************************************************************************
 *  FILE: inoutcta.c
 *
 *  DESCRIPTION: two-way trunk demo for Natural Access
 *
 *  Voice Files Used -
 *                CTADEMO.VOX     - all the prompts
 *                <temporary>.VOX - scratch file for record and playback
 *
 *
 * Copyright (c) 1997-2002 NMS Communications.  All rights reserved.
 *****************************************************************************/



#define VERSION_NUM  "5"
#define VERSION_DATE __DATE__

/*----------------------------- Constants ----------------------------------*/
#define SINGLE       0       /* operation mode: one port per program        */
#define THREADED     1       /* operation mode: two different threads       */
#define MAX_THREADS  2       /* maximum number of threads                   */

#define NULL_SWIHD 0xFFFFFFFF

#define UNDETERMINED_STREAM 0xffff  /* an invalid MVIP stream number        */

#define KEYBOARD_EVENT CTA_USER_EVENT(1) /* no CTAccess events have this code */

#define CDR_INBOUND         0x01
#define CDR_CONNECTED       0x02
#define CDR_DISCONNECTED    0x04
#define CDR_OUTBOUND        0x08

/*----------------------------- Includes -----------------------------------*/
#include <stdlib.h>

#ifndef UNIX
#include <conio.h>
#endif
#include <time.h>
#include "nccdef.h"

#include "ctademo.h"  /* Natural Access demo library definitions                 */

/*header files for call status extentions and event definions.*/
#include "nccxadi.h"
#include "nccxcas.h"

/*----------------------------- OS-specific definitions --------------------*/

/*
 * Definitions for WaitForAnyEvent().  In UNIX we use the poll() function.
 */
#if defined (UNIX)
#define POLL_TIMEOUT 0
#define POLL_ERROR  -1
#endif

/*----------------------------- Application Definitions --------------------*/
typedef struct
{
    CTAQUEUEHD   ctaqueuehd;        /* Natural Access queue handle                 */
    CTAHD        ctahd;             /* The Line handle                        */
    SWIHD        swihd;             /* Switch context handle                  */
    FILE        *fp;                /* file pointer for play/record           */
    char        *buffer[2];         /* buffers for play/record                */
    char         dialstring[33];    /* current number being dialed (max 32)   */
    char         callingstring[33]; /* calling party number (max 32)          */
    char         protocol[9];       /* the protocol being used by the thread  */
    char         tempfile[12];      /* the temporary file used to play/record */
    unsigned     thread_id;         /* index of the thread (if multithreaded) */
    unsigned     interactive_flag;  /* dial upon user command?                */
    unsigned     dsp_slot;          /* the MVIP timeslot to run on            */
    DWORD        tx_calls;          /* number of placed calls                 */
    DWORD        rx_calls;          /* number of received calls               */
    NCC_CALLHD   callhd;            /* the call handle                        */
} MYCONTEXT;

/*----------------------------- Application Data ---------------------------*/
unsigned   mode           = SINGLE; /* default is single threaded application */
unsigned   ag_board       = 0;      /* the first board on the AG config file  */
unsigned   dsp_stream     = 0;      /* the DSP stream                         */
unsigned   dsp_slot       = 0;      /* the default port                       */
unsigned   interactive_flag = 0;    /* do not prompt the user before dialing  */
unsigned   res_management = FALSE;
unsigned   acceptmode     = 0;      /* none of the legal modes                */
unsigned   rejectmode     = 0;      /* default is answer the call             */
unsigned   gatewaymode    = FALSE;
unsigned   printCDR       = 0;      /* default is no Call Detail Records      */
unsigned   detectDTMFinAccept = FALSE; /* whether to detect DTMF in accepting */
unsigned   ringnumber     = 2;
unsigned   billingrate    = NCC_BILLINGSET_DEFAULT; /* CAS protocols          */
long       answerdelay    = 10;
char       CDRecord[72]   = "\0";
char       dialstring[33] = "123";
char       callingstring[33] = "\0";
char       protocol[9]    = "mfc0"; /* default is MFC-R2                      */
unsigned   verbosity      = 2;      /* display all high-level events          */
unsigned   extsupport     = 0;      /* extended call status supported         */
time_t     ttimeofday;
struct  tm *tmtimeofday;
unsigned    CDRindex;

char       textbuffer[80];

/*----------------------------- Forward References -------------------------*/
THREAD_RET_TYPE THREAD_CALLING_CONVENTION RunDemo (void * pcx);

int   SpeakDigits      (MYCONTEXT *cx, char *digits);
DWORD WaitForAnyEvent  (CTAHD ctahd, CTA_EVENT *eventp);
DWORD MyPlaceCall      (MYCONTEXT *cx);
DWORD MyReceiveCall    (MYCONTEXT *cx, unsigned call_received);
void  CustomRejectCall (MYCONTEXT *cx);
DWORD MyPlayMessage    (MYCONTEXT *cx, char *filename, unsigned  message);

/*----------------------------------------------------------------------------
  PrintHelp()
  ----------------------------------------------------------------------------*/
void PrintHelp(void)
{
    printf("Usage: inoutcta [options]\nwhere options are:\n");

    printf("\n1. general options\n");
    printf("-?          Display this help screen and exits\n");
    printf("-A adi_mgr  Natural Access ADI manager to use (default: %s)\n",
           DemoGetADIManager());
    printf("-b n        OAM board number (default: %d)\n", ag_board);
    printf("-p protocol Protocol to run (default: %s)\n", protocol);
    printf("-s [n:]m    DSP [stream and] timeslot.  Default = %d:%d\n",
           dsp_stream, dsp_slot);
    printf("            You must specify a stream for AG24/30/48/60 boards.\n");
    printf("-t number of threads\n"
           "            This option demonstrates the TSA multithreaded programming model.\n"
           "            Launch any number of threads, as long as the same board is used\n"
           "            for all.  All threads will act the same way, placing or receiving\n"
           "            calls.  The timeslots used are the one specified with -s, and above.\n");
    printf("-v level    Verbosity level of messages printed on screen, where:\n"
           "            level = 0 --> error messages only\n"
           "            level = 1 --> errors and unexpected high-level events\n"
           "            level = 2 --> errors and all high-level events (default)\n");

    printf("\n2. inbound options\n");
    printf("-a mode     Accept the inbound calls before answering or rejecting, where:\n"
           "            mode = 1  --> accept and play ringback tone\n"
           "            mode = 2  --> accept and remain silent until further command\n"
           "            mode = 3  --> accept with user audio and play a voice file\n"
           "            mode = 4  --> accept with user audio and detect DTMFs\n");
    printf("-B rate     requested Billing rate:\n"
           "            rate = 0  --> free call\n"
           "            rate = N  --> normally billed call\n");
    printf("-C type     print Call Detail Records, where:\n"
           "            type is a string of characters:\n"
           "            I --> print CDR when Incoming call is detected\n"
           "            C --> print CDR when call is Connected\n"
           "            D --> print CDR when call is Disconnected\n"
           "            O --> print CDR when Outbound call is made\n");
    printf("-d delay    delay before answering (in seconds), where -1 means NEVER answer\n");
    printf("-g          emulate a gateway application: the user controls the timing\n"
           "            of the incoming call being accepted or rejected\n");
    printf("-j mode     Reject the inbound calls.\n"
           "            mode = 1  --> use reorder tone or equivalent protocol signal\n"
           "            mode = 2  --> use busy tone or equivalent protocol signal\n"
           "            mode = 3  --> play ring forever\n"
           "            mode = 4  --> play user audio to reject\n");
    printf("-i          wait for an inbound call, interactively initiate an outbound call\n");
    printf("-r rings    number of rings tones to play before answering a call (default: %d)\n",
           ringnumber);

    printf("\n3. outbound options\n");
    printf("-n           number   Number to dial.\n");
    printf("-c           number   calling party number.\n");
    printf("-F           filename Natural Access Configuration File name (default: None)\n.");
}

/*----------------------------------------------------------------------------
  main
  ----------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
    int c;
    DWORD             ret;
    unsigned          threadsnumber, i;
    unsigned          printhelp = FALSE;
    CTA_INIT_PARMS    initparms = { 0};
    CTA_ERROR_HANDLER hdlr;
    MYCONTEXT        *cx;

    /* allocate and initialize the first context, that we certainly need   */
    cx = (MYCONTEXT *)malloc (sizeof(MYCONTEXT));

    cx->ctaqueuehd = NULL_CTAQUEUEHD;          /* initialize to safe value */
    cx->ctahd      = NULL_CTAHD;               /* initialize to safe value */
    cx->swihd      = NULL_SWIHD;               /* initialize to safe value */
    cx->callhd     = NULL_NCC_CALLHD;
    cx->thread_id  = 0;                        /* thread id                */
    cx->rx_calls   = 0;                        /* number of received calls */
    cx->tx_calls   = 0;                        /* number of placed calls   */

    printf ("Natural Access Two-way trunk Demo V.%s (%s)\n", VERSION_NUM, VERSION_DATE);

    while ((c = getopt(argc, argv, "A:a:b:B:c:C:d:F:j:l:n:p:r:s:t:v:giPr?"))
           != -1)
    {
        switch (c)
        {
            unsigned          value1, value2;

            case 'A':
                DemoSetADIManager(optarg);
                break;
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
                        acceptmode = NCC_ACCEPT_USER_AUDIO;
                        detectDTMFinAccept = TRUE;
                        break;
                    default:
                        printf ("? unsupported NCCAcceptCall mode\n");
                        exit (1);
                }
                break;
            case 'b':
                sscanf(optarg, "%d", &ag_board);
                break;
            case 'B':
                sscanf(optarg, "%d", &billingrate);
                break;
            case 'c':
                strcpy (callingstring, optarg);
                break;
            case 'C':
                strcpy (CDRecord, optarg);
                break;
            case 'd':
                sscanf(optarg, "%d", &answerdelay);
                break;
            case 'F':
                DemoSetCfgFile(optarg);
                break;
            case 'j':
                switch (optarg[0])
                {
                    case '1':
                        rejectmode = NCC_REJECT_PLAY_REORDER;
                        break;
                    case '2':
                        rejectmode = NCC_REJECT_PLAY_BUSY;
                        break;
                    case '3':
                        rejectmode = NCC_REJECT_PLAY_RINGTONE;
                        break;
                    case '4':
                        rejectmode = NCC_REJECT_USER_AUDIO;
                        break;
                    default:
                        printf ("? unsupported NCCRejectCall mode\n");
                        exit (1);
                }
                break;
            case 'g':
                gatewaymode = TRUE;
                break;
            case 'i':
                interactive_flag = 1;
                break;
            case 'n':
                strcpy (dialstring, optarg);
                break;
            case 'p':
                strcpy (protocol, optarg);
                break;
            case 'r':
                sscanf(optarg, "%d", &ringnumber);
                break;
            case 's':
                switch (sscanf(optarg, "%d:%d", &value1, &value2))
                {
                    case 2:
                        dsp_stream = value1;
                        dsp_slot   = value2;
                        break;
                    case 1:
                        dsp_slot   = value1;
                        break;
                    default:
                        PrintHelp();
                        exit(1);
                }
                break;
            case 't':
                mode = THREADED;
                sscanf(optarg, "%d", &threadsnumber);
                break;
            case 'v':
                sscanf(optarg, "%d", &verbosity);
                break;
            case '?':
                printhelp = TRUE;
                break;
            default:
                printf("? Unrecognizable option -%c %s\n", c, optarg);
                exit(1);
                break;
        }
    }

    /*
       Now we know what the options are, if the user asked to print a
       help text and exit, do it.
     */
    if (printhelp)
    {
        PrintHelp();
        exit (1);
    }

    /* assigning the final values to the main context cx */
    strcpy (cx->dialstring, dialstring);       /* dialstring               */
    strcpy (cx->callingstring, callingstring); /* callingstring            */
    strcpy (cx->protocol, protocol);           /* protocol                 */
    cx->dsp_slot = dsp_slot;                   /* timeslot                 */
    cx->interactive_flag = interactive_flag;   /* interactive flag         */
    printCDR = 0;
    if (strchr (CDRecord, 'I') != NULL) printCDR |= CDR_INBOUND;
    if (strchr (CDRecord, 'i') != NULL) printCDR |= CDR_INBOUND;
    if (strchr (CDRecord, 'C') != NULL) printCDR |= CDR_CONNECTED;
    if (strchr (CDRecord, 'c') != NULL) printCDR |= CDR_CONNECTED;
    if (strchr (CDRecord, 'D') != NULL) printCDR |= CDR_DISCONNECTED;
    if (strchr (CDRecord, 'd') != NULL) printCDR |= CDR_DISCONNECTED;
    if (strchr (CDRecord, 'O') != NULL) printCDR |= CDR_OUTBOUND;
    if (strchr (CDRecord, 'o') != NULL) printCDR |= CDR_OUTBOUND;
    /*
     * slot + ag_board*1000 is a unique number
     * in a Natural Access application: this is our temporary file name
     */
    sprintf (cx->tempfile, "%5.5d", dsp_slot + ag_board*1000);
    /* thus, the tempfile is "01004.vox", if slot is 4 on board 1 */

    DemoSetReportLevel(DEMO_REPORT_MIN); /* turn off CTAdemo printouts */

    /*
       Natural Access initialization: Register an error handler to display errors
       returned from API functions; no need to check return values in the demo.
     */

    ctaSetErrorHandler(DemoErrorHandlerContinue, NULL);

    /* Initialize init parms structure */
    initparms.size = sizeof(CTA_INIT_PARMS);
    initparms.ctacompatlevel = CTA_COMPATLEVEL;

    /* If daemon running then initialize tracing
     * and system global default parameters.
     */
    initparms.traceflags = CTA_TRACE_ENABLE;
    initparms.parmflags  = CTA_PARM_MGMT_SHARED;

    /* Initialize the Natural Access configuration file (if any) */
    initparms.filename   = DemoGetCfgFile();

    /* Set error handler to NULL and remember old handler */
    ctaSetErrorHandler(NULL, &hdlr);

    /*
     * Prepare to initialize Natural Access.
     */
    {

        CTA_SERVICE_NAME  servicelist[] =   /* for ctaInitialize */
        { {"ADI", ""},
            {"VCE", "VCEMGR"},
            {"NCC", ""}
        };

        unsigned numservices = 3;
        servicelist[0].svcmgrname = DemoGetADIManager();
        servicelist[2].svcmgrname = DemoGetADIManager();

        if ((ret = ctaInitialize(servicelist, numservices,
                                 &initparms)) != SUCCESS)
        {
            printf("Unable to invoke ctaInitialize().\n");
            exit (1);
        }
        else
            ctaSetErrorHandler(hdlr, NULL);  /* restore error handler */
    }

    /* set the verbosity level now that all initialization tasks
       are finished */
    switch (verbosity)
    {
        case 0:             /* errors only */
            DemoSetReportLevel(DEMO_REPORT_MIN);
            break;
        case 1:             /* errors + unexpected high-level events */
            DemoSetReportLevel(DEMO_REPORT_UNEXPECTED);
            break;
        case 2:             /* errors + all high-level events (default) */
            DemoSetReportLevel(DEMO_REPORT_ALL);
            break;
        default:
            printf ("? WARNING: unsupported verbosity level - using default\n");
            break;
    }

    /*
     * now launch the RunDemo threads (if using threads) or call the RunDemo
     * function otherwise
     */
    if (mode == SINGLE)
    {
        if (DemoShouldReport(DEMO_REPORT_COMMENTS))
        {
            printf("\tProtocol   = %s\n",    protocol);
            printf("\tBoard      = %d\n",    ag_board);
            printf("\tStream:Slot= %d:%d\n", dsp_stream, dsp_slot);
        }
        RunDemo(cx);
    }
    else
    {
        /* launch the first thread, that is already initialized */
        if (DemoSpawnThread(RunDemo, (void *)(cx)) == FAILURE)
        {
            printf("ERROR during _beginthread... exiting\n");
            exit (1);
        }
        if (DemoShouldReport(DEMO_REPORT_COMMENTS))
        {
            printf("\tProtocol   = %s\n",    protocol);
            printf("\tBoard      = %d\n",    ag_board);
            printf("\tStream:Slot= %d:%d\n", dsp_stream, cx->dsp_slot);
        }

        /* then do the other threads... */
        for (i=1; i<threadsnumber; i++)
        {
            /* allocate and initialize more cx's */
            cx = (MYCONTEXT *)malloc (sizeof(MYCONTEXT));

            cx->ctaqueuehd = NULL_CTAQUEUEHD;       /* initialize to null   */
            cx->ctahd      = NULL_CTAHD;            /* initialize to null   */
            cx->swihd      = NULL_SWIHD;            /* initialize to null   */
            strcpy (cx->dialstring, dialstring);    /* dialstring           */
            strcpy (cx->callingstring, callingstring); /* calling party     */
            strcpy (cx->protocol, protocol);        /* protocol             */
            cx->thread_id  = i;                     /* thread id            */
            cx->interactive_flag = interactive_flag;/* interactive behavior */
            cx->dsp_slot   = dsp_slot + i;          /* timeslot             */
            cx->rx_calls   = 0;                     /* # of received calls  */
            cx->tx_calls   = 0;                     /* # of placed calls    */

            /* assign a temp file name (see above) */
            sprintf (cx->tempfile, "%5.5d", cx->dsp_slot + ag_board*1000);

            /* use the OS-dependent DemoSpawnThread to launch more threads */
            if (DemoSpawnThread(RunDemo, (void *)(cx)) == FAILURE)
            {
                printf("ERROR during _beginthread... exiting\n");
                exit (1);
            }
            if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                printf("\tStream:Slot= %d:%d\n", dsp_stream, cx->dsp_slot);
        }

        /* now the main thread sleeps forever */
        for (;;) DemoSleep(1000);
    }
    return 0;
}


/*-------------------------------------------------------------------------
  RunDemo
  -------------------------------------------------------------------------*/
THREAD_RET_TYPE THREAD_CALLING_CONVENTION RunDemo (void *pcx)
{
    DWORD            ret;

    NCC_ADI_START_PARMS       adiparms;   /* ADI specific start parameters */
    NCC_START_PARMS           nccparms;   /* NCC specific start parameters */
    NCC_CAS_EXT_CALL_STATUS   casstatus;  /* CAS protocol parameters */

    CTA_EVENT        nextevent;
    MYCONTEXT       *cx;
    char             parfile[15];
    char             contextname[15];
    unsigned         numservices;
    NCC_CALL_STATUS  status;
    NCC_PROT_CAP     protcap = {0};       /* contains the capability mask */

    /*
     * Define the Natural Access service description list for ctaOpenServices
     */
    CTA_SERVICE_DESC services[] =
    {   { {"ADI", ""}, { 0}, { 0}, { 0}},
        { {"VCE", "VCEMGR"}, { 0}, { 0}, { 0}},
        { {"NCC", ""}, { 0}, { 0}, { 0}},
        { {"SWI", "SWIMGR"}, { 0}, { 0}, { 0}}
    };
    services[0].name.svcmgrname = DemoGetADIManager();
    services[2].name.svcmgrname = DemoGetADIManager();

    cx = (MYCONTEXT *) pcx;
    numservices = 3;                /* it's CAS  */

    /* Fill in ADI service (index 0) MVIP address information                */
    services[0].mvipaddr.board    = ag_board;
    services[0].mvipaddr.stream   = dsp_stream;
    services[0].mvipaddr.timeslot = cx->dsp_slot;
    services[0].mvipaddr.bus      = 0;
    services[0].mvipaddr.mode     = ADI_FULL_DUPLEX;

    /* Fill in NCC service (index 2) MVIP address information                */
    services[2].mvipaddr.board    = ag_board;
    services[2].mvipaddr.stream   = dsp_stream;
    services[2].mvipaddr.timeslot = cx->dsp_slot;
    services[2].mvipaddr.bus      = 0;
    services[2].mvipaddr.mode     = ADI_FULL_DUPLEX;

    sprintf (contextname, "inoutcta%d", cx->thread_id);

    /* use the function in ctademo to open and initialize the context        */
    DemoOpenPort(cx->thread_id, contextname, services, numservices,
                 &cx->ctaqueuehd, &cx->ctahd);

    /* Initiate a protocol on the port so we can  place and accept calls     */

    /* Try to load the optional parameters from the .par file.  If the .par
     * file is not there, use the defaults.  Defaults are either from .pf files
     * (automatically loaded while initializing Natural Access with ctaInitialize(),
     * or are embedded in the ADI code. It makes no difference now.
     * The .par file is expected to be either in the current directory,
     * or in
     * \nms\ctaccess\cfg        (Windows NT)
     * /opt/nms/ctaccess/cfg    (UNIX)
     */

    /* try to load the protocol-specific parameter file                      */
    memset (parfile, 0, sizeof (parfile));      /* initialize to '\0'        */

    /* the protocol name is the first three characters of the TCP name       */
    sprintf (parfile, "nccx%.3s.par", cx->protocol);

    if (ctaLoadParameterFile (cx->ctahd, parfile) != SUCCESS)
    {
        if (DemoShouldReport(DEMO_REPORT_COMMENTS))
            printf ("\n\tRunDemo: protocol text parameter file not found: "
                    "using defaults\n\n");
    }

    /* First retrieve the NCC_ADI_START_PARMID structure                     */
    ctaGetParms(cx->ctahd, NCC_ADI_START_PARMID, &adiparms, sizeof(adiparms));
    ctaGetParms(cx->ctahd, NCC_START_PARMID, &nccparms, sizeof(nccparms));

    /* set mediamask to detect DTMF while accepting the call, if so desired  */
    if (detectDTMFinAccept)
    {
        adiparms.mediamask |= NCC_ACCEPT_DTMF;
    }

    nccStartProtocol (cx->ctahd, cx->protocol, &nccparms, &adiparms, NULL);

    /* enable all informational events in the eventmask */
    nccparms.eventmask = NCC_REPORT_ALERTING | NCC_REPORT_ANSWERED |
        NCC_REPORT_BILLING | NCC_REPORT_STATUSINFO;

    /*
     * example of use of the CTADEMO function DemoWaitForSpecificEvent to
     * wait for a specific event. In this case it's appropriate because
     * NCCEVN_START_PROTOCOL_DONE is the only event that can come after
     * nccStartProtocol.
     */

    DemoWaitForSpecificEvent (cx->ctahd,
                              NCCEVN_START_PROTOCOL_DONE,
                              &nextevent);
    switch (nextevent.value)
    {
        case CTA_REASON_FINISHED:
            if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                printf ("\tRunDemo: protocol successfully started\n");
            break;

        case NCCREASON_OUT_OF_RESOURCES:
            /*
             * This means that the board requires resource management. If resource
             * management is required, the application cannot ask the TCP to start
             * services to run in the connected state, but instead must start the
             * service itself, once the connected state is reached. The NCC ADI
             * parameter mediamask in this case must be zero.
             * The services the application must manage are:
             * - DTMF detector     \
             * - silence detector   | started with adiStartDTMFDetector
             * - energy detector   /
             */
            res_management = TRUE;
            /* later we'll start services or assume already started  depending on
             * the flag value */

            if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                printf ("\tRunDemo: mediamask not supported by protocol %s on this\n"
                        "\t         board - setting mediamask to zero and trying "
                        "again\n",
                        cx->protocol);
            adiparms.mediamask = 0;

            /* The Start parameters for the ADI service contain more than just
             * call control, so they are still necessary, while the NCC Start
             * Parameters will manage the Call Control aspect of the demo.
             */
            nccStartProtocol (cx->ctahd, cx->protocol, &nccparms,
                              &adiparms, NULL);

            DemoWaitForSpecificEvent (cx->ctahd, NCCEVN_START_PROTOCOL_DONE,
                                      &nextevent);
            if (nextevent.value != CTA_REASON_FINISHED)
            {
                DemoShowError("Start protocol failed", nextevent.value);
                exit(1);
            }
            else
            {
                if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                    printf ("\tRunDemo: protocol successfully started with mediamask "
                            "= 0x%x\n", adiparms.mediamask);
            }
            break;

        default:
            printf ("\tRunDemo: error in starting the protocol, value 0x%x\n",
                    nextevent.value);
            exit (1);
    }


    /*
     * This is to determine whether or not the protocol can support extended
     * call status.  If so, it will report certain values later in the
     * app.
     */
    ret = nccQueryCapability(cx->ctahd, &protcap, sizeof(protcap));

    if ((protcap.capabilitymask & NCC_CAP_EXTENDED_CALL_STATUS)
        == NCC_CAP_EXTENDED_CALL_STATUS)
    {
        extsupport = 1;
        DemoReportComment("Extended Call Status supported...");
    }
    else
    {
        DemoReportComment("Extended Call Status not supported...");
    }

    while (1) /* repeat until stopped */
    {
        if (cx->interactive_flag == 1)
        {
            if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                if (mode == THREADED)
                    printf ("\tThread %d - Waiting for a call...\n",
                            cx->thread_id);
                else
                    printf ("\tWaiting for a call, or press <enter> to dial\n");

            ret = WaitForAnyEvent(cx->ctahd, &nextevent);

            if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                printf ("\n");

            if (ret == SUCCESS)
            {
                switch (nextevent.id)
                {
                    case KEYBOARD_EVENT:
                        printf ("\n");
                        if (mode != THREADED)
                        {
                            MyPlaceCall(cx);
                        }
                        break;

                    case NCCEVN_SEIZURE_DETECTED: /* the line has been seized       */

                        if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                            if (mode == THREADED)
                                printf("\tThread %d - Seizure detected\n",
                                       cx->thread_id);
                            else
                                printf("\tSeizure detected\n");

                        cx->callhd = nextevent.objHd;
                        MyReceiveCall (cx, FALSE);
                        break;

                    case NCCEVN_CALL_STATUS_UPDATE:
                        /* Get NCC Call Control specific call status */
                        nccGetCallStatus(cx->callhd, &status, sizeof(status));
                        break;

                    case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
                        if (extsupport == 1)
                        {
                            nccGetExtendedCallStatus(cx->callhd, &casstatus,
                                                     sizeof(casstatus));
                            if (nextevent.value == CALL_STATUS_UUI)
                            {
                                if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                    printf("\tRunDemo: Incoming UUI arrived.");
                            }
                            else
                                printf ("\tRunDemo, unexpected status info %d\n",
                                        nextevent.value);
                        }
                        else
                            printf("\tRunDemo: Extended Call Status not supported"
                                   "on protocol\n");
                        break;

                    case NCCEVN_PROTOCOL_ERROR:
                        /*
                         * an error occurred in the inbound TCP before the incoming
                         * call event: do nothing and wait for the next call
                         */
                        if (nextevent.value ==  NCC_PROTERR_NO_CS_RESOURCE)
                        {
                            if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                printf("\tRunDemo: call failed because of lack of"
                                       "  resource\n");
                        }
                        continue;

                    case NCCEVN_LINE_OUT_OF_SERVICE:
                        /* the line went out of service */
                        /* nothing can be done until it goes back in service again   */
                        if (DemoShouldReport(DEMO_REPORT_UNEXPECTED))
                        {
                            printf("\tRunDemo: line out of service "
                                   "- waiting for in-service...\n");
                        }
                        do
                        {
                            ret = WaitForAnyEvent(cx->ctahd, &nextevent);
                            if (ret != SUCCESS)
                            {
                                printf ("\tRunDemo: WaitForAnyEvent FAILED\n");
                                exit(1);
                            }
                            if (nextevent.id != NCCEVN_LINE_IN_SERVICE)
                            {
                                printf ("\tRunDemo: unexpected event id = 0x%x value ="
                                        "0x%x\n", nextevent.id, nextevent.value);
                            }
                        } /* while not back in service */
                        while (nextevent.id != NCCEVN_LINE_IN_SERVICE);

                        printf("\tRunDemo: line back in service\n");
                        break;

                    default:
                        if (DemoShouldReport(DEMO_REPORT_UNEXPECTED))
                        {
                            printf ("\tRunDemo: unexpected event id = 0x%x value = "
                                    "0x%x\n", nextevent.id, nextevent.value);
                        }
                        continue;
                } /* switch (nextevent.id) */
            } /* if (ret == SUCCESS) */
            else
            {
                ctaGetText(cx->ctahd, nextevent.id, textbuffer, sizeof(textbuffer));
                printf ("Error detected: %s\n", textbuffer);
                continue;
            }
        }
        else
        {
            if (mode == THREADED)
            {
                DemoSleep (1);  /* wait some to re-synchronize */
            }
            MyPlaceCall(cx);
        }
    }  /*end of while (1)*/


    UNREACHED_STATEMENT(ctaDestroyQueue(cx->ctaqueuehd);)
        UNREACHED_STATEMENT(THREAD_RETURN;)
}

/*-------------------------------------------------------------------------
  MyReceiveCall

  Receive a call.
  -------------------------------------------------------------------------*/
DWORD MyReceiveCall (MYCONTEXT *cx, unsigned call_received)
{

    DWORD             ret;
    char              option[2];
    VCE_RECORD_PARMS  rparms;
    ADI_COLLECT_PARMS cparms;
    NCC_CALL_STATUS   status;
    CTA_EVENT         nextevent;
    unsigned          connected=FALSE, timer_done = FALSE;
    unsigned          recording;
    unsigned          collecting;
    unsigned          call_accepted;
    unsigned          billing_capable = 0;
    VCEHD             vh;

    NCC_PROT_CAP      protcap = {0};
    NCC_CAS_EXT_CALL_STATUS  casstatus;

    nccQueryCapability(cx->ctahd, &protcap, sizeof(protcap));

    if ((protcap.capabilitymask & NCC_CAP_SET_BILLING) == NCC_CAP_SET_BILLING)
    {
        billing_capable = 1;
    }

    /*
     * If we are here because of a NCCEVN_SEIZURE_DETECTED, the call is still
     * in its setup phase. We need to receive an NCCEVN_INCOMING_CALL, that
     * tells us that the call is ready for us to decide what to do.
     * Otherwise, the call is ready for us to accept or reject
     */

    if (call_received == FALSE)  /* call setup is in progress */
    {
        while (!call_received)
        {
            ret = WaitForAnyEvent(cx->ctahd, &nextevent);
            if (ret == SUCCESS)
            {
                switch (nextevent.id)
                {
                    case NCCEVN_PROTOCOL_ERROR:
                        if (nextevent.value == NCC_PROTERR_FALSE_SEIZURE)
                        {
                            if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                printf("\tMyReceiveCall: false seizure\n");
                            continue;
                        }
                        /* A protocol error occurred. On a AG Quad board used with
                           resource management, this might mean that the TCP failed
                           to get the call setup resource on time. In all cases, a
                           protocol error at this stage means that the call will
                           not be presented: return. */
                        if (nextevent.value == NCC_PROTERR_NO_CS_RESOURCE)
                        {
                            if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                printf("\tMyReceiveCall: call failed because of "
                                       "lack of resource\n");
                        }
                        ret = FAILURE;
                        return ret;
                    case NCCEVN_INCOMING_CALL:    /* an incoming call has arrived */
                        if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                            printf ("\tMyReceiveCall: Incoming call\n");
                        call_received = TRUE;       /* exit the loop */
                        break;

                    case NCCEVN_RECEIVED_DIGIT:   /* a digit has arrived */
                        if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                            printf("\tMyReceiveCall: Digit detected:\t%c\n",
                                   nextevent.value);
                        break;

                    case NCCEVN_CALL_DISCONNECTED:
                        /*
                         * a disconnect in this phase means that the call failed.
                         * Hangup and wait for another call
                         */
                        if (DemoShouldReport(DEMO_REPORT_UNEXPECTED))
                        {
                            printf("\tMyReceiveCall: call setup failed - "
                                   "hanging up\n");
                        }
                        goto hangup_in;

                    default:
                        if (DemoShouldReport(DEMO_REPORT_UNEXPECTED))
                        {
                            printf ("MyReceiveCall: unexpected event id = 0x%x "
                                    "value = 0x%x\n", nextevent.id,
                                    nextevent.value);
                        }
                        break;
                } /* switch (nextevent.id) */
            } /* if (ret == SUCCESS) */
            else
            {
                ctaGetText(cx->ctahd, nextevent.id, textbuffer, sizeof(textbuffer));
                printf ("MyReceiveCall: Error detected: %s\n", textbuffer);
                return ret;
            }
        } /* while (!call_received) */
    } /* if (call_received == FALSE) */

    ++cx->rx_calls;

    if (DemoShouldReport(DEMO_REPORT_COMMENTS))
    {
        if (mode == THREADED)
        {
            printf("\tMyReceiveCall[%2d]: got incoming call %d\n",
                   cx->thread_id, cx->rx_calls);
        }
        else
        {
            printf("\tMyReceiveCall: got incoming call %d\n", cx->rx_calls);
        }
    }
    /*
     * At this point, a call is ready to be processed.
     * Use nccGetCallStatus() to get the information associated with it.
     * And use nccGetExtendedCallStatus() to get the information associated
     * with the CAS protocols.
     */

    /* Get NCC Call Control specific call status             */
    nccGetCallStatus(cx->callhd, &status, sizeof(status));
    if (extsupport == 1)
    {
        nccGetExtendedCallStatus(cx->callhd, &casstatus, sizeof(casstatus));
    }

    if (printCDR & CDR_INBOUND)
    {
        time (&ttimeofday);
        tmtimeofday = localtime(&ttimeofday);
        printf("\nDate             Time     Ln  Call Type     Called_DN Calling_Dn Calling_Name\n");
        strftime(CDRecord, 26,"%a %b %d, %Y %H:%M:%S ",tmtimeofday);
        sprintf(CDRecord+26,"%2d %5d I %16s ",cx->thread_id+1,cx->rx_calls,status.calledaddr);
        sprintf(CDRecord+54,"%10s ",status.callingaddr);
        printf("%s%s\n",CDRecord, status.callingname);
    }
    
    if (DemoShouldReport(DEMO_REPORT_COMMENTS))
    {
        /* user-friendly initialization of some text fields     */
        if (strlen (status.calledaddr) == 0)
            strcpy (status.calledaddr, "not available");
        if (strlen (status.callingaddr) == 0)
            strcpy (status.callingaddr, "not available");
        if (strlen (status.callingname) == 0)
            strcpy (status.callingname, "not available");
        if (extsupport == 1)
        {
            printf("\tMyReceiveCall: (call n.%d)\n"
                   "\t\tcalled number:\t\t%s\n"
                   "\t\tcalling category:\t%c\n"
                   "\t\tcalling number:\t\t%s\n"
                   "\t\tcaller's name:\t\t%s\n"
                   "\t\tcalling toll category:\t%c\n\n",
                   ++cx->rx_calls,
                   status.calledaddr,  casstatus.usercategory,
                   status.callingaddr,
                   status.callingname,
                   casstatus.tollcategory);
        }
    }

    /* Print a message informing the user that the protocol does not
     * support billing
     */
    if ((billingrate != NCC_BILLINGSET_DEFAULT) && (billing_capable == 0))
    {
        printf("\tMyReceiveCall: billing not supported by protocol %s\n",
               cx->protocol);
    }

    /*
     * This is a good time to set the billing rate of the call, if the
     * command line option is set
     */
    if ((billingrate != NCC_BILLINGSET_DEFAULT) && (billing_capable == 1))
    {
        nccSetBilling (cx->callhd, billingrate, NULL);
        WaitForAnyEvent(cx->ctahd, &nextevent);
        switch (nextevent.id)
        {
            case NCCEVN_BILLING_SET:
                if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                {
                    if (nextevent.value == NCC_BILLINGSET_DEFAULT)
                    {
                        /* note: event if we never requested the billing
                         * rate to be equal to the network default, the
                         * billing set operation can fail, and this is
                         * what we have...  */
                        printf ("\tMyReceiveCall: billing rate set to network "
                                "default\n");
                    }
                    else
                        printf ("\tMyReceiveCall: billing rate set to %d "
                                "cents/minute\n", nextevent.value);
                }
                break;

            case NCCEVN_PROTOCOL_ERROR:
                /* if the event is an NCCEVN_PROTOCOL_ERROR, the operation
                 * to set the billing rate is not supported by the
                 * protocol */
                if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                    printf ("\tMyReceiveCall: set billing not supported by "
                            "protocol\n");
                break;

            case NCCEVN_CALL_DISCONNECTED:
                /* during the set billing operation, the network hung up */
                if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                    printf("\tMyReceiveCall: call disconnected\n");
                ret = DISCONNECT;
                goto hangup_in;

            default:
                printf ("\tMyReceiveCall: unexpected event during nccSetBilling\n");
                break;
        } /* switch (nextevent.id) */
    } /* if ((billingrate... */

    /*
     * We can decide to put the call on hold for the moment. This might be
     * because, based on the associated information, we want to do something
     * that might require some time, i.e. a database search, or placing an
     * outbound call to be then connected with this call.
     * Or we might want to play a voice file before answering.
     * This is captured by the global variable acceptmode (set with a command
     * line option).
     * To do this we call nccAcceptCall().
     */
    if (acceptmode != 0)
    {
        nccAcceptCall (cx->callhd, acceptmode, NULL);

        call_accepted = FALSE;
        while (!call_accepted)
        {
            WaitForAnyEvent(cx->ctahd, &nextevent);
            switch (nextevent.id)
            {
                case NCCEVN_CALL_STATUS_UPDATE:
                    /* Some asynchrounous information regarding the call has
                       arrived from the line. This could be ANI information. The
                       ANI are detached from the DID for the Russian R1.5 protocol
                       but come later. Look for it here. You can tell the kind of
                       information looking at the event's value field. */
                    /* Get NCC Call Control specific call status */
                    nccGetCallStatus(cx->callhd, &status, sizeof(status));
                    break;

                case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
                    /* This is the event which will tell the program to receive
                       the protocol-specific extended call status.*/

                    if (extsupport == 1)
                    {
                        nccGetExtendedCallStatus(cx->callhd, &casstatus,
                                                 sizeof(casstatus));
                        if (nextevent.value == NCC_CALL_STATUS_CALLINGADDR)
                        {
                            if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                printf("\tMyReceiveCall: calling number arrived:\n"
                                       "\t               %s\n", status.callingaddr);
                            else
                                printf ("\tMyReceiveCall, unexpected status info "
                                        "%d\n", nextevent.value);
                        }
                    }
                    else
                    {
                        printf("\tMyReceiveCall: Extended Call Status not "
                               "supported on protocol\n");
                    }
                    break;

                case NCCEVN_ACCEPTING_CALL:
                    if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                        printf("\tMyReceiveCall: call accepted with mode %d\n",
                               nextevent.value);

                    call_accepted = TRUE;

                    if (nextevent.value == NCC_ACCEPT_USER_AUDIO)
                    {
                        if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                            printf("\tMyReceiveCall: playing message before "
                                   "answering\n");

                        if ((ret = MyPlayMessage (cx, "ctademo", PROMPT_WELCOME))
                            != SUCCESS)
                        {
                            goto hangup_in;
                        }

                        /* at this point, collecting DTMFs is also enabled
                         * on the board if the mediamask &
                         * NCC_ACCEPT_DTMF */
                        if (detectDTMFinAccept)
                        {
                            ctaGetParms(cx->ctahd, ADI_COLLECT_PARMID, &cparms,
                                        sizeof(cparms));
                            /* 10 seconds to wait for first digit */
                            cparms.firsttimeout = 10000;
                            /* 3 seconds between digits */
                            cparms.intertimeout = 3000;
                            adiCollectDigits(cx->ctahd, option, 1, &cparms);

                            if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                printf("\tMyReceiveCall: collecting DTMF while "
                                       "accepting...\n");
                            collecting = TRUE;

                            while (collecting)
                            {
                                WaitForAnyEvent(cx->ctahd, &nextevent);
                                if (CTA_IS_ERROR (nextevent.id))
                                {
                                    ret = FAILURE;
                                    goto hangup_in;
                                }
                                switch (nextevent.id)
                                {
                                    case ADIEVN_COLLECTION_DONE:
                                        if (nextevent.value == CTA_REASON_RELEASED)
                                        {
                                            break;
                                        }
                                        else if
                                            (nextevent.value == CTA_REASON_TIMEOUT)
                                            {
                                                if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                                    printf("\tMyReceiveCall: timeout in "
                                                           "collecting DTMF\n");
                                            }
                                        else
                                        {
                                            if(DemoShouldReport(DEMO_REPORT_COMMENTS))
                                                printf("\tMyReceiveCall: received "
                                                       "DTMF '%c'\n", option[0]);
                                        }
                                        collecting = FALSE;
                                        break;

                                    case NCCEVN_CALL_DISCONNECTED:
                                        if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                            printf("\tMyReceiveCall: call "
                                                   "disconnected\n");
                                        ret = DISCONNECT;
                                        goto hangup_in;

                                    case ADIEVN_DIGIT_BEGIN:
                                    case ADIEVN_DIGIT_END:
                                        break;

                                    default:
                                        if (DemoShouldReport(DEMO_REPORT_UNEXPECTED))
                                        {
                                            if (mode == THREADED)
                                            {
                                                printf ("MyReceiveCall[%2.2d]: "
                                                        "unexpected event id = 0x%x "
                                                        "value = 0x%x\n",
                                                        cx->thread_id,
                                                        nextevent.id,
                                                        nextevent.value);
                                            }
                                            else
                                            {
                                                printf ("MyReceiveCall: unexpected "
                                                        "event id = 0x%x value = 0x%x\n",
                                                        nextevent.id, nextevent.value);
                                            }
                                        }
                                        break;
                                } /* switch (nextevent.id) */
                            } /* while (collecting) */
                        } /* if (detectDTMFinAccept) */
                    } /* if (nextevent.value == NCC_ACCEPT_USER_AUDIO) */
                    else
                    {
                        /* it's either silence, and we wait silently for a
                         * while, or the TCP is playing rings, and we also
                         * wait for a while and then answer.
                         *
                         * Note that if the application acts as a gateway
                         * (for instance between different protocols),
                         * this is the time to place the outbound leg of
                         * this call. This is demoed here by the user
                         * pressing the <ENTER> key, representing the
                         * outbound call being accepted
                         * (NCCEVN_REMOTE_ALERTING). The inbound call may
                         * then be answered (with 0 rings, immediate
                         * answer) when the NCCEVN_CALL_CONNECTED event is
                         * received from the outbound call.  */
                        if (gatewaymode)
                        {
                            if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                printf ("\tMyReceiveCall: Accepting... Press "
                                        "<enter> to answer\n");
                        }
                        if (answerdelay == 0)
                            timer_done = TRUE;
                        else
                            if (answerdelay > 0)
                                adiStartTimer(cx->ctahd, answerdelay * 1000, 1); /* wait before answering */
                        while (!timer_done)
                        {
                            WaitForAnyEvent(cx->ctahd, &nextevent);
                            switch (nextevent.id)
                            {
                                case ADIEVN_TIMER_DONE:
                                    if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                        printf ("\tMyReceiveCall: timer done: "
                                                "exiting loop\n");
                                    timer_done = TRUE;
                                    break;

                                case NCCEVN_CALL_DISCONNECTED:
                                    if (answerdelay > 0)
                                    {
                                        adiStopTimer (cx->ctahd);
                                        do
                                        {
                                            WaitForAnyEvent(cx->ctahd, &nextevent);
                                        }
                                        while (nextevent.id != ADIEVN_TIMER_DONE);
                                    }
                                    if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                        printf("\tMyReceiveCall: call disconnected\n");
                                    ret = DISCONNECT;
                                    goto hangup_in;

                                case KEYBOARD_EVENT:
                                    if (answerdelay > 0)
                                        adiStopTimer (cx->ctahd);
                                    do
                                    {
                                        WaitForAnyEvent(cx->ctahd, &nextevent);
                                    }
                                    while (nextevent.id != ADIEVN_TIMER_DONE);
                                    timer_done = TRUE;
                                    break;

                                default:
                                    printf("\tMyReceiveCall, bad event while "
                                           "accepting: 0x%x\n", nextevent.id);
                                    break;
                            }  /* switch */
                        }  /* while (!timer_done)  */
                    }  /* else: if (nextevent.value == NCC_ACCEPT_USER_AUDIO) */
                    break;

                case NCCEVN_CALL_DISCONNECTED:
                    if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                        printf("\tMyReceiveCall: remote abandoned\n");
                    ret = DISCONNECT;
                    goto hangup_in;

                default:
                    printf("\tMyReceiveCall, bad event from nccAcceptCall: "
                           "0x%x\n", nextevent.id);
                    ret = FAILURE;
                    goto hangup_in;
            } /* switch (nextevent.id) */
        } /* while (!call_accepted) */
    } /* if (acceptmode != 0) */


    /*
     * Demo the rejection of a call based on the corresponding user option:
     * either playing busy, or fast busy (reorder), or playing ring forever,
     * or playing user audio (a SIT tone).
     * If the option to reject is not set, answer the call.
     */

    switch (rejectmode)
    {

        case NCC_REJECT_USER_AUDIO:
            CustomRejectCall(cx);
            goto hangup_in;

        case NCC_REJECT_PLAY_REORDER:
        case NCC_REJECT_PLAY_BUSY:
        case NCC_REJECT_PLAY_RINGTONE:
            DemoRejectNCCCall(cx->ctahd, cx->callhd, rejectmode, NULL);
            goto hangup_in;

        default: /* answer the call */
            nccAnswerCall(cx->callhd, ringnumber, NULL);  /* ring the phone */

            while (!connected)
            {
                WaitForAnyEvent(cx->ctahd, &nextevent);
                switch (nextevent.id)
                {
                    case NCCEVN_CALL_STATUS_UPDATE:
                        /* Get NCC Call Control specific call status */
                        nccGetCallStatus(cx->callhd, &status, sizeof(status));

                        /* Get Protocol-specific call status */
                        if (extsupport == 1)
                        {
                            nccGetExtendedCallStatus(cx->callhd, &casstatus,
                                                     sizeof(casstatus));
                        }

                        if (nextevent.value == CALL_STATUS_CALLINGADDR)
                        {
                            if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                printf ("\tMyReceiveCall: calling number arrived:\n"
                                        "\t               %s\n", status.callingaddr);
                        }
                        else
                        {
                            printf ("\tMyReceiveCall, unexpected status info %d\n",
                                    nextevent.value);
                        }
                        break;

                    case NCCEVN_ANSWERING_CALL:
                        if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                            printf ("\tMyReceiveCall: Answering... \n");
                        break;

                    case NCCEVN_CALL_DISCONNECTED:
                        if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                            printf ("\tMyReceiveCall: Remote abandoned - "
                                    "hanging up\n");
                        goto hangup_in;

                    case NCCEVN_CALL_CONNECTED:
                        if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                            printf("\tMyReceiveCall: Call connected\n");
                        connected = TRUE;
                        break;

                    default:
                        if (DemoShouldReport(DEMO_REPORT_UNEXPECTED))
                        {
                            printf ("MyReceiveCall: unexpected event id = 0x%x "
                                    "value = 0x%x\n", nextevent.id, nextevent.value);
                        }
                        break;
                }
            } /* while (!connected) */
            break;
    } /* switch (rejectmode) */

    if (printCDR & CDR_CONNECTED)
    {
        nccGetCallStatus(cx->callhd, &status, sizeof(status));
        time (&ttimeofday);
        tmtimeofday = localtime(&ttimeofday);
        strftime(CDRecord, 26,"%a %b %d, %Y %H:%M:%S ",tmtimeofday);
        sprintf(CDRecord+26,"%2d %5d C %16s ",cx->thread_id+1,cx->rx_calls,status.calledaddr);
        sprintf(CDRecord+54,"%10s ",status.callingaddr);
        printf("%s%s\n",CDRecord, status.callingname);
    }
    /*
     * Start caller interaction.
     * We do the following:
     * - we announce the digits dialed
     * - we play another prompt, giving the user three options:
     *     record (1), play (2), or hangup (3)
     * - we wait for the dtmf, and we do what we're asked for
     * - we hangup
     */

    adiFlushDigitQueue(cx->ctahd);

    /*
     * If resource management is needed, start the DTMF detector.
     * If not, the TCP already did it.
     */
    if (res_management)
    {
        if ((ret = adiStartDTMFDetector (cx->ctahd, NULL))!= SUCCESS)
        {
            printf("\tMyReceiveCall: could not start the DTMF detector\n");
            goto hangup_in;
        }
    }

    /* Announce the calling number, if available. (we had manipulated it) */
   
    if ((strlen(status.callingaddr) != 0) &&
        (strcmp(status.callingaddr , "not available") != 0))
    {
        if ((ret = MyPlayMessage (cx, "ctademo", PROMPT_CALLING)) != SUCCESS)
        {
            goto hangup_in;
        }   
        if ((ret = SpeakDigits(cx, status.callingaddr)) != SUCCESS)
        {
            goto hangup_in;
        }
    }

    /* Announce the called number, if available. (we had manipulated it) */

    if ((strlen(status.calledaddr) != 0) &&
        (strcmp(status.calledaddr , "not available") != 0))
    {
        if ((ret = MyPlayMessage (cx, "ctademo", PROMPT_CALLED)) != SUCCESS)
        {
            goto hangup_in;
        }   
        if ((ret = SpeakDigits(cx, status.calledaddr)) != SUCCESS)
        {
            goto hangup_in;
        }
    }

    /* Loop, giving the user three options: record, playback, hangup. */
    for (;;)
    {
        if ((ret = MyPlayMessage (cx, "ctademo", PROMPT_OPTION)) != SUCCESS)
        {
            goto hangup_in;
        }

        ctaGetParms(cx->ctahd, ADI_COLLECT_PARMID, &cparms, sizeof(cparms));
        cparms.firsttimeout = 3000;  /* 3 seconds to wait for first digit */
        cparms.intertimeout = 3000;  /* 3 seconds between digits */

        adiCollectDigits(cx->ctahd, option, 1, &cparms);

        if (DemoShouldReport(DEMO_REPORT_COMMENTS))
            printf("\tMyReceiveCall: collecting DTMF...\n");

        collecting = TRUE;
        while (collecting)
        {
            WaitForAnyEvent(cx->ctahd, &nextevent);

            if (CTA_IS_ERROR (nextevent.id))
            {
                ret = FAILURE;
                goto hangup_in;
            }

            switch (nextevent.id)
            {
                case ADIEVN_COLLECTION_DONE:
                    if (nextevent.value == CTA_REASON_RELEASED)
                    {
                        goto hangup_in;
                    }
                    else if (CTA_IS_ERROR (nextevent.value))
                    {
                        ret = FAILURE;
                        goto hangup_in;
                    }
                    else if (strlen (option) == 0)
                    {
                        ret = FAILURE;
                        goto hangup_in;
                    }
                    else
                    {
                        if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                            printf("\tMyReceiveCall: received DTMF '%c'\n",
                                   option[0]);
                    }
                    collecting = FALSE;
                    break;

                case NCCEVN_CALL_DISCONNECTED:
                    if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                        printf("\tMyReceiveCall: remote hung up while "
                               "collecting DTMF\n");
                    ret = DISCONNECT;
                    goto hangup_in;

                case ADIEVN_DTMF_DETECT_DONE:
                    /* this comes if the other side disconnects
                       (reason: Released) */
                    break;

                case ADIEVN_DIGIT_BEGIN:
                case ADIEVN_DIGIT_END:
                    break;

                default:
                    if (DemoShouldReport(DEMO_REPORT_UNEXPECTED))
                    {
                        printf ("MyReceiveCall: unexpected event id = 0x%x "
                                "value = 0x%x\n", nextevent.id, nextevent.value);
                    }
                    break;
            } /* switch (nextevent.id) */
        } /* while (collecting) */

        switch (*option)
        {
            case '3':
                /* this is what outbound will play, in back-to-back mode */
                goto hangup_in;

            case '1':
                /* Record the caller, overriding default parameters for
                 * quick recognition of silence once voice input has begun.
                 */
                ctaGetParms(cx->ctahd, VCE_RECORD_PARMID,
                            &rparms, sizeof(rparms));
                rparms.silencetime = 1000; /* 1 sec */

                /*
                 * Record called party, overriding default parameters for
                 * quick recognition of silence once voice input has begun.
                 */

                if (vceOpenFile(cx->ctahd, cx->tempfile, VCE_FILETYPE_VOX,
                                VCE_PLAY_RECORD, ADI_ENCODE_NMS_24,
                                &vh) != SUCCESS)
                {
                    unlink(cx->tempfile);
                    if(vceCreateFile (cx->ctahd, cx->tempfile, VCE_FILETYPE_VOX,
                                      ADI_ENCODE_NMS_24, NULL,
                                      &vh) != SUCCESS)
                    {
                        printf ("\tMyReceiveCall: could not open file %s for "
                                "recording\n",cx->tempfile);
                        ret = FAILURE;
                        goto hangup_in;
                    }
                }

                if (vceRecordMessage (vh, 0, 0, &rparms) != SUCCESS)
                {
                    vceClose(vh);
                    printf ("\tMyReceiveCall: could not record message\n");
                    ret = FAILURE;
                    goto hangup_in;
                }

                if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                    printf("\tMyReceiveCall: recording file %s.vox\n",
                           cx->tempfile);

                recording = TRUE ;
                while (recording)
                {
                    WaitForAnyEvent(cx->ctahd, &nextevent);

                    if (CTA_IS_ERROR (nextevent.id))
                    {
                        ret = FAILURE;
                        vceClose(vh);
                        goto hangup_in;
                    }

                    switch (nextevent.id)
                    {
                        case VCEEVN_RECORD_DONE:
                            vceClose(vh);
                            if (nextevent.value == CTA_REASON_RELEASED)
                            {
                                if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                    printf ("\tMyReceiveCall: remote hung up during "
                                            "recording\n");
                                goto hangup_in;
                            }
                            if (CTA_IS_ERROR (nextevent.value))
                            {
                                printf ("\tMyReceiveCall: record function failed\n");
                                ret = FAILURE;
                                goto hangup_in;
                            }
                            else
                            {
                                if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                    printf("\tMyReceiveCall: record done\n");
                                recording = FALSE;
                            }
                            break;

                        case NCCEVN_CALL_DISCONNECTED:
                            if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                printf ("\tMyReceiveCall: caller hung up during "
                                        "recording\n");
                            vceClose(vh);
                            goto hangup_in;

                        case ADIEVN_DIGIT_BEGIN:
                        case ADIEVN_DIGIT_END:
                            break;

                        case KEYBOARD_EVENT: /* do nothing - it's accidental */
                            break;

                        default:
                            if (DemoShouldReport(DEMO_REPORT_UNEXPECTED))
                            {
                                printf ("\tMyPlaceCall: unexpected event id = 0x%x "
                                        "value = 0x%x\n",
                                        nextevent.id, nextevent.value);
                            }
                            break;
                    }
                } /* while (recording) */
                break;

            case '2':
                /* Play back anything the caller recorded.  This might be
                   something recorded last time the demo was run, or the file
                   might not exist.
                 */
                MyPlayMessage (cx, cx->tempfile, 0);
                break;

            default:
                if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                    printf("\t Invalid selection: '%c'\n", *option);
                break;
        } /* switch (*option) */
    } /* for (;;) */

hangup_in:
    if (printCDR & CDR_DISCONNECTED)
    {
        nccGetCallStatus(cx->callhd, &status, sizeof(status));
        time (&ttimeofday);
        tmtimeofday = localtime(&ttimeofday);
        strftime(CDRecord, 26,"%a %b %d, %Y %H:%M:%S ",tmtimeofday);
        sprintf(CDRecord+26,"%2d %5d D %16s ",cx->thread_id+1,cx->rx_calls,status.calledaddr);
        sprintf(CDRecord+54,"%10s ",status.callingaddr);
        printf("%s%s\n",CDRecord, status.callingname);
    }
    
    DemoHangNCCUp(cx->ctahd, cx->callhd, NULL, NULL);
    return ret;
} /* MyReceiveCall */


/*----------------------------------------------------------------------------
  MyPlaceCall

  Place a call.
  ----------------------------------------------------------------------------*/
DWORD MyPlaceCall (MYCONTEXT *cx)
{
    DWORD ret;
    VCEHD vh;                       /* to record and play voice files    */
    VCE_RECORD_PARMS rparms;        /* structure to contain record parms */
    NCC_CALL_STATUS  status;        /* structure to contain call status  */
    CTA_EVENT nextevent;

    unsigned recording;
    unsigned billing = 0;
    unsigned wait_for_voice = TRUE;

    NCC_CAS_EXT_CALL_STATUS  casstatus;

    strcpy(status.calledaddr, dialstring);
    strcpy(status.callingaddr, callingstring);
    *status.callingname = '\0';
    
    if (DemoShouldReport(DEMO_REPORT_COMMENTS))
    {
        if (mode == THREADED)
            printf("\tMyPlaceCall[%2d]: making an outbound call...\n",
                   cx->thread_id);
        else
            printf("\tMyPlaceCall: making an outbound call...\n");
    }
    ret = nccPlaceCall(cx->ctahd, status.calledaddr, status.callingaddr, NULL, NULL,
                       &cx->callhd);
    while (1)
    {
        ret = WaitForAnyEvent(cx->ctahd, &nextevent);
        if (ret != SUCCESS)
        {
            printf ("MyPlaceCall: WaitForAnyEvent FAILED\n");
            exit(1);
        }

        switch (nextevent.id)
        {
            case NCCEVN_PLACING_CALL:        /* Glare resolved         */
                ++cx->tx_calls;

                if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                {
                    if (mode == THREADED)
                        printf("\tMyPlaceCall[%2d]: placing call %d\n",
                               cx->thread_id, cx->tx_calls);
                    else
                        printf("\tMyPlaceCall: placing call %d\n",
                               cx->tx_calls);
                }
                if (printCDR & CDR_OUTBOUND)
                {
                    time (&ttimeofday);
                    tmtimeofday = localtime(&ttimeofday);
                    printf("\nDate             Time     Ln  Call Type     Called_DN Calling_Dn Calling_Name\n");
                    strftime(CDRecord, 26,"%a %b %d, %Y %H:%M:%S ",tmtimeofday);
                    sprintf(CDRecord+26,"%2d %5d O %16s ",cx->thread_id+1,cx->tx_calls,status.calledaddr);
                    sprintf(CDRecord+54,"%10s ",status.callingaddr);
                    printf("%s%s\n",CDRecord, status.callingname);
                }
                break;

            case NCCEVN_PROTOCOL_ERROR:
                /* an error occurred - consider this call a failure           */
                ret = DISCONNECT;
                goto hangup_out;

            case NCCEVN_CALL_PROCEEDING:     /* all digits delivered         */
                if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                    printf ("\tMyPlaceCall: Dialing completed\n");
                break;

            case NCCEVN_REMOTE_ALERTING:     /* confirmation of ringing      */
                if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                    printf ("\tMyPlaceCall: Phone is ringing\n");
                break;

                /* WARNING: the following two situations are illegal if the
                 * trunk is a purely outbound one. */
            case NCCEVN_SEIZURE_DETECTED:    /* the line has been seized     */
                printf ("\tMyPlaceCall: glare detected (seizure event), do inbound\n");
                cx->callhd = nextevent.objHd;   
                MyReceiveCall (cx, FALSE);
                return SUCCESS;

            case NCCEVN_INCOMING_CALL:       /* a call waits to be answered  */
                printf ("\tMyPlaceCall: glare detected (incoming event), do inbound\n");
                MyReceiveCall (cx, TRUE);
                return SUCCESS;

            case NCCEVN_CALL_STATUS_UPDATE:
                /* Get NCC Call Control specific call status */
                nccGetCallStatus(cx->callhd, &status, sizeof(status));

                /* Get Protocol-specific call status */
                if (extsupport == 1)
                {
                    nccGetExtendedCallStatus(cx->callhd, &casstatus,
                                             sizeof(casstatus));
                }
                else
                    printf ("\tMyPlaceCall, unexpected status info %d\n",
                            nextevent.value);
                break;

            case NCCEVN_REMOTE_ANSWERED:     /* remote telephone picked up     */
                if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                    printf ("\tMyPlaceCall: Remote answered\n");
                break;

            case NCCEVN_CALL_CONNECTED:
                {
                    /* Conversation  phase. Now the application can proceed with
                       its processing */
                    if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                        printf("\tMyPlaceCall: Connected.\n");
                    adiFlushDigitQueue(cx->ctahd);

                    if (printCDR & CDR_CONNECTED)
                    {
                        nccGetCallStatus(cx->callhd, &status, sizeof(status));
                        time (&ttimeofday);
                        tmtimeofday = localtime(&ttimeofday);
                        strftime(CDRecord, 26,"%a %b %d, %Y %H:%M:%S ",tmtimeofday);
                        sprintf(CDRecord+26,"%2d %5d C %16s ",cx->thread_id+1,cx->tx_calls,status.calledaddr);
                        sprintf(CDRecord+54,"%10s ",status.callingaddr);
                        printf("%s%s\n",CDRecord, status.callingname);
                    }
                    /*
                     * use call progress detection to look for voice.  This is not
                     * necessary if a call progress event is the reason for the
                     * connected event that was just received
                     */
                    if (nextevent.value != NCC_CON_SIGNAL)
                    {
                        adiStartCallProgress (cx->ctahd, NULL);
                        while (wait_for_voice)
                        {
                            WaitForAnyEvent(cx->ctahd, &nextevent);

                            if (CTA_IS_ERROR (nextevent.id))
                                continue;

                            switch (nextevent.id)
                            {
                                case ADIEVN_CP_RINGTONE:
                                    if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                        printf ("\tMyPlaceCall: ring tone detected\n");
                                    break;

                                case ADIEVN_CP_BUSYTONE:
                                    if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                        printf ("\tMyPlaceCall: busy tone detected, "
                                                "abandoning...\n");
                                    ret = DISCONNECT;
                                    goto hangup_out;

                                case ADIEVN_CP_VOICE:
                                    if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                        printf ("\tMyPlaceCall: voice detected, starting "
                                                "recording...\n");
                                    adiStopCallProgress (cx->ctahd);
                                    do
                                    {
                                        WaitForAnyEvent(cx->ctahd, &nextevent);
                                    }
                                    while (nextevent.id != ADIEVN_CP_DONE);
                                    wait_for_voice = FALSE;
                                    break;

                                case ADIEVN_CP_SIT:
                                    if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                        printf ("\tMyPlaceCall: SIT detected, "
                                                "abandoning...\n");
                                    ret = DISCONNECT;
                                    goto hangup_out;

                                case ADIEVN_CP_DONE:
                                    if (nextevent.value == ADIERR_NOT_ENOUGH_RESOURCES)
                                    {
                                        if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                            printf ("\tMyPlaceCall: no resource for "
                                                    "call progress\n");
                                    }
                                    else if (nextevent.value == CTA_REASON_TIMEOUT)
                                    {
                                        if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                            printf ("\tMyPlaceCall: call progress done, "
                                                    "no voice detected\n");
                                    }
                                    else
                                    {
                                        if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                            printf ("\tMyPlaceCall: call progress done, "
                                                    "reason 0x%x\n",
                                                    nextevent.value);
                                    }
                                    wait_for_voice = FALSE;
                                    break;

                                case NCCEVN_CALL_STATUS_UPDATE:
                                    /* Get NCC Call Control specific call status */
                                    nccGetCallStatus(cx->callhd, &status, sizeof(status));

                                    /*Get Protocol-specific call status */
                                    if (extsupport == 1)
                                    {
                                        nccGetExtendedCallStatus(cx->callhd, &casstatus,
                                                                 sizeof(casstatus));
                                    }
                                    else
                                        printf("\tMyPlaceCall, unexpected status "
                                               "info %d\n", nextevent.value);
                                    break;

                                case NCCEVN_CALL_DISCONNECTED:
                                    if (DemoShouldReport(DEMO_REPORT_UNEXPECTED))
                                        printf("\tMyPlaceCall: hangup received during "
                                               "call progress\n");
                                    ret = DISCONNECT;
                                    goto hangup_out;

                                case KEYBOARD_EVENT:
                                    break;

                                case NCCEVN_BILLING_INDICATION:
                                    billing++;
                                    printf ("\tMyPlaceCall: billing pulse #%d received\n",
                                            billing);
                                    break;

                                default:
                                    if (DemoShouldReport(DEMO_REPORT_UNEXPECTED))
                                        printf ("\tMyPlaceCall: unexpected event id = 0x%x "
                                                "value = 0x%x\n",
                                                nextevent.id, nextevent.value);
                                    break;
                            } /* switch (nextevent.id) */
                        } /* while (wait_for_voice) */
                    } /* if (nextevent.value != NCC_CON_SIGNAL) */

                    /*
                     * Record called party, overriding default parameters for
                     * quick recognition of silence once voice input has begun.
                     */
                    ctaGetParms(cx->ctahd, VCE_RECORD_PARMID, &rparms,
                                sizeof(rparms));
                    rparms.silencetime = 1000; /* 1 sec */

                    if (vceOpenFile(cx->ctahd, cx->tempfile, VCE_FILETYPE_VOX,
                                    VCE_PLAY_RECORD, ADI_ENCODE_NMS_24,
                                    &vh) != SUCCESS)
                    {
                        unlink(cx->tempfile);
                        if (vceCreateFile(cx->ctahd, cx->tempfile, VCE_FILETYPE_VOX,
                                          ADI_ENCODE_NMS_24, NULL,
                                          &vh) != SUCCESS)
                        {
                            printf ("\tMyPlaceCall: could not open file %s for "
                                    "recording\n", cx->tempfile);

                            ret = FAILURE;
                            goto hangup_out;
                        }
                    }

                    if (vceRecordMessage (vh, 0, 0, &rparms) != SUCCESS)
                    {
                        vceClose(vh);
                        printf ("\tMyPlaceCall: could not record message\n");
                        ret = FAILURE;
                        goto hangup_out;
                    }

                    recording = TRUE ;
                    while (recording)
                    {
                        WaitForAnyEvent(cx->ctahd, &nextevent);

                        switch (nextevent.id)
                        {
                            case VCEEVN_RECORD_DONE:
                                vceClose(vh);

                                if (nextevent.value == CTA_REASON_RELEASED)
                                {
                                    printf ("\tMyPlaceCall: caller hung up during "
                                            "recording\n");
                                    ret = DISCONNECT;
                                    goto hangup_out;
                                }

                                if (CTA_IS_ERROR(nextevent.value))
                                {
                                    printf ("\tMyPlaceCall: record function failed\n");
                                    ret = FAILURE;
                                    goto hangup_out;
                                }
                                else
                                {
                                    if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                        printf("\tMyPlaceCall: record done\n");
                                    recording = FALSE;
                                }
                                break;

                            case NCCEVN_CALL_STATUS_UPDATE:
                                nccGetCallStatus(cx->callhd, &status, sizeof(status));
                                break;

                            case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
                                if (extsupport == 1)
                                {
                                    /* Get Protocol-specific call status */
                                    nccGetExtendedCallStatus(cx->callhd, &casstatus,
                                                             sizeof(casstatus));
                                }
                                else
                                    printf ("\tMyPlaceCall, unexpected status info %d\n",
                                            nextevent.value);
                                break;

                            case NCCEVN_CALL_DISCONNECTED:
                                vceClose(vh);
                                if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                    printf ("\tMyPlaceCall: remote hung up during "
                                            "recording\n");
                                ret = DISCONNECT;
                                goto hangup_out;

                            case ADIEVN_DIGIT_BEGIN:
                            case ADIEVN_DIGIT_END:
                                break;

                            case NCCEVN_BILLING_INDICATION:
                                billing++;
                                printf ("\tMyPlaceCall: billing pulse #%d received\n",
                                        billing);
                                break;

                            case KEYBOARD_EVENT:         /* do nothing: it's accidental */
                                break;

                            default:
                                printf ("\tMyPlaceCall: unexpected event id = 0x%x "
                                        "value = 0x%x\n", nextevent.id, nextevent.value);
                                break;
                        } /* switch (nextevent.id) */
                    } /* while (recording) */

                    adiStartDTMF (cx->ctahd, "1", NULL);
                    /* '1' = record for inbound */
                    do
                    {
                        WaitForAnyEvent(cx->ctahd, &nextevent);

                        if (CTA_IS_ERROR (nextevent.id))
                            continue;

                        if (nextevent.id == NCCEVN_BILLING_INDICATION)
                        {
                            billing++;
                            if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                printf ("\tMyPlaceCall: billing pulse #%d received\n",
                                        billing);
                        }

                        if (nextevent.id == NCCEVN_CALL_DISCONNECTED)
                        {
                            if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                                printf ("\tMyPlaceCall: remote hung up during "
                                        "DTMF\n");
                            ret = DISCONNECT;
                            goto hangup_out;
                        }
                    } /* do */
                    while (nextevent.id != ADIEVN_TONES_DONE);

                    if (nextevent.value == CTA_REASON_RELEASED)
                    {
                        if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                            printf ("\tMyPlaceCall: caller hung up during DTMF\n");
                        ret = DISCONNECT;
                        goto hangup_out;
                    }

                    /* Play back what we just recorded.  */
                    /* only one message in this vox file */
                    ret = MyPlayMessage (cx, cx->tempfile, 0);

                    goto hangup_out;
                } /* case NCCEVN_CALL_CONNECTED: */

            case NCCEVN_LINE_OUT_OF_SERVICE:
                /* the line went out of service nothing can be done until
                   it goes back in service again */
                if (DemoShouldReport(DEMO_REPORT_UNEXPECTED))
                    printf("\tMyPlaceCall: Call failed - line out of service.\n"
                           "\t(waiting for in-service)\n");
                do
                {
                    ret = WaitForAnyEvent(cx->ctahd, &nextevent);
                    if (ret != SUCCESS)
                    {
                        printf ("MyPlaceCall: WaitForAnyEvent FAILED\n");
                        exit(1);
                    }
                    if (nextevent.id != NCCEVN_LINE_IN_SERVICE)
                        printf ("MyPlaceCall: unexpected event id = 0x%x "
                                "value = 0x%x\n",
                                nextevent.id, nextevent.value);
                } /* do */
                while (nextevent.id != NCCEVN_LINE_IN_SERVICE);

                return NCCEVN_LINE_OUT_OF_SERVICE;

            case NCCEVN_CALL_DISCONNECTED:
                /* a disconnect in this phase means that the call failed.
                   Hangup and dial another call */

                if (nextevent.value == NCC_DIS_NO_CS_RESOURCE)
                {
                    if (DemoShouldReport(DEMO_REPORT_UNEXPECTED))
                        printf ("MyPlaceCall: call failed for lack of"
                                "resource\n");
                }
                else
                {
                    if (DemoShouldReport(DEMO_REPORT_UNEXPECTED))
                        printf("\tMyPlaceCall: call failed.\n");
                }
                ret = DISCONNECT;
                goto hangup_out;

            case NCCEVN_CALL_RELEASED:
                if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                    printf ("\tMyPlaceCall: placed call released\n");
                cx->callhd = 0;
                break;

            case KEYBOARD_EVENT:
                /* do nothing: it's accidental */
                break;

            default:
                printf ("\tMyPlaceCall: unexpected event id = 0x%x value = 0x%x\n",
                        nextevent.id, nextevent.value);
                break;
        } /* end of switch */

    } /* while (1) */

hangup_out:
    if (printCDR & CDR_DISCONNECTED)
    {
        time (&ttimeofday);
        tmtimeofday = localtime(&ttimeofday);
        strftime(CDRecord, 26,"%a %b %d, %Y %H:%M:%S ",tmtimeofday);
        sprintf(CDRecord+26,"%2d %5d D %16s ",cx->thread_id+1,cx->tx_calls,status.calledaddr);
        sprintf(CDRecord+54,"%10s ",status.callingaddr);
        printf("%s%s\n",CDRecord, status.callingname);
    }
    DemoHangNCCUp(cx->ctahd, cx->callhd, NULL, NULL);
    return ret;

} /* MyPlaceCall */


/*-------------------------------------------------------------------------
  SpeakDigits
  -------------------------------------------------------------------------*/
int SpeakDigits(MYCONTEXT * cx, char * digits)
{
    unsigned  ret;
    char     *pc;
    unsigned  realdigits[80];
    unsigned  i, j;

    for (pc = digits, i = 0; pc && *pc; pc++)
    {
        if (isdigit(*pc))
            realdigits[i++] = *pc -'0';
    }

    for (j = 0; j < i; j++)
    {
        ret = MyPlayMessage (cx, "ctademo", realdigits[j]);
        if (ret != SUCCESS)
            break;
    }
    return ret;
}


/*-------------------------------------------------------------------------
  CustomRejectCall

  Reject the call with a custom tone sequence.
  -------------------------------------------------------------------------*/
void CustomRejectCall (MYCONTEXT * cx)
{
    CTA_EVENT  event;
    CTAHD      ctahd;
    CTAQUEUEHD ctaqueuehd;
    int        tonesdone = 0;
    int        disconnected = 0;
    int        i;

    ADI_TONE_PARMS p[4];

    ctahd = cx->ctahd;


    ctaGetQueueHandle(ctahd, &ctaqueuehd);

    ctaGetParms(ctahd, ADI_TONE_PARMID, &p[0], sizeof(p[0]));

    p[0].iterations = 1;
    for (i=1; i<4; i++)
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

    printf("\tPlaying reject tone\n");

    nccRejectCall(cx->callhd, NCC_REJECT_USER_AUDIO, NULL);

    /*
     *  Need both TONES_DONE and DISCONNECTED events before returning.
     */

    while (! (disconnected && tonesdone))
    {
        WaitForAnyEvent (ctahd, &event);
        DemoShowEvent(&event);

        switch (event.id)
        {
            case NCCEVN_REJECTING_CALL:
                if (adiStartTones(ctahd, 4, p) != SUCCESS)
                    /* could not start tones, outbound is already clearing */
                    tonesdone = 1;
                break;

            case ADIEVN_TONES_DONE:
                if (event.value == CTA_REASON_RELEASED)
                {
                    tonesdone = 1;     /* Wait for DISCONNECTED event */
                }
                else if (CTA_IS_ERROR(event.value))
                {
                    DemoShowError("Play Tones failed", event.value);
                    tonesdone = 1;     /* Wait for DISCONNECTED event */
                }
                else
                {
                    /* The remote party is still connected.  Continue playing
                       the SIT tone. */
                    adiStartTones(ctahd, 4, p);
                }
                break;

            case NCCEVN_CALL_DISCONNECTED:
                printf("\tCaller hung up.\n");
                disconnected = 1;
                /* Expect TONES_DONE, reason RELEASED */
                break;

            case ADIEVN_DIGIT_BEGIN:
            case ADIEVN_DIGIT_END:
                break;

            default:
                if (DemoShouldReport(DEMO_REPORT_UNEXPECTED))
                    printf("\tCustomRejectCall - unexpected event - id = 0x%x\n",
                           event.id);
                break;
        } /* end of switch */
    }  /* end of while */
}


/*-----------------------------------------------------------------------------
  WaitForAnyEvent()

  The following function waits for either an AG event or for a
  keyboard event.  It returns the type of event.
  -----------------------------------------------------------------------------*/
DWORD WaitForAnyEvent(CTAHD ctahd, CTA_EVENT *eventp)
{
    CTAQUEUEHD ctaqueuehd;
    DWORD ctaret ;

    ctaGetQueueHandle(ctahd, &ctaqueuehd);

    while (1)
    {
        ctaret = ctaWaitEvent(ctaqueuehd, eventp, 100); /* wait for 100 ms */
        if (ctaret != SUCCESS)
        {
            printf("\007ctaWaitEvent returned %x\n", ctaret);
            exit(1);
        }

        if (eventp->id == CTAEVN_WAIT_TIMEOUT)
        {
            if (kbhit())    /* check if a key has been pressed    */
            {
                /* Note that for UNIXes, kbhit() is   */
                /* defined in ctademo.c               */
                getc(stdin);
                eventp->id = KEYBOARD_EVENT;/* indicate that a key is ready  */
                return SUCCESS;
            }
        }
        else
        {
            if( DemoShouldReport( DEMO_REPORT_ALL ) )
                DemoShowEvent( eventp );
            return SUCCESS;
        }
    }
}


/*-------------------------------------------------------------------------
  MyPlayMessage

  This function plays a message from a vox file, also expecting all the
  events that we need to consider
  -------------------------------------------------------------------------*/
DWORD MyPlayMessage (MYCONTEXT * cx, char * filename, unsigned  message)
{

    VCEHD           vh;
    DWORD           ret;
    unsigned short  playing;
    CTA_EVENT       nextevent;

    /* open the VOX file */
    if (vceOpenFile(cx->ctahd, filename, VCE_FILETYPE_VOX, VCE_PLAY_ONLY,
                    ADI_ENCODE_NMS_24, &vh)  != SUCCESS)
        return FAILURE;

    if (DemoShouldReport(DEMO_REPORT_COMMENTS))
        printf ("\tMyPlayMessage: playing message %2.2d from file %s.vox ...",
                message, filename);
    if (vcePlayList(vh, &message, 1, NULL) != SUCCESS)
    {
        vceClose(vh);
        return FAILURE;
    }

    ret = SUCCESS;  /* assume playing will succeed */
    playing = TRUE;
    while (playing)
    {
        WaitForAnyEvent(cx->ctahd, &nextevent);

        if (CTA_IS_ERROR (nextevent.id))
            continue;

        switch (nextevent.id)
        {
            case VCEEVN_PLAY_DONE:
                playing = FALSE;
                if (nextevent.value == CTA_REASON_RELEASED)
                {
                    if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                        printf ("\n\tMyPlayMessage: remote hung up while "
                                "playing...\n");
                    ret = DISCONNECT;
                    break;
                }

                if (CTA_IS_ERROR (nextevent.value))
                {
                    printf ("\n\tMyPlayMessage: play function failed\n");
                    ret = FAILURE;
                }
                else
                {
                    if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                        printf (" done\n");
                }
                break;

            case NCCEVN_CALL_STATUS_UPDATE:
                printf ("\tMyPlayMessage, unexpected status info %d\n",
                        nextevent.value);
                break;

            case NCCEVN_CALL_DISCONNECTED:
                if (DemoShouldReport(DEMO_REPORT_COMMENTS))
                    printf ("\n\tMyPlayMessage: remote hung up while playing\n");
                playing = FALSE;
                ret = DISCONNECT;
                break;

            case ADIEVN_DIGIT_BEGIN:
            case ADIEVN_DIGIT_END:
                break;

            case KEYBOARD_EVENT:
                break;

            default:
                if (DemoShouldReport(DEMO_REPORT_UNEXPECTED))
                    printf ("\n\tMyPlayMessage: unexpected event id = 0x%x "
                            "value = 0x%x\n",
                            nextevent.id, nextevent.value);
                break;
        } /* end switch */
    } /* while (playing) */

    vceClose (vh);
    return ret;
}
