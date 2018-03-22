/*****************************************************************************
 *  File -  cnfjoin.c
 *
 *  Description - Simple demo showing the join of a member to a conference
 *                using Natural Call Control and SWI service.
 *
 *  with option -m <n> this demo create one mother conference and n daughter
 *  conferences in order to increase the members number
 *
 *  Voice Files Used -
 *                CTADEMO.VOX    - welcome demo VOX file.
 *
 * 2002 NMS Communications
 *****************************************************************************
 *
 *  Functions defined here are:
 *
 * main-+-PrintHelp
 *      |-RunDemo--+-Setup-+-InitMyContext
 *                 |       +-InitResource
 *                 |       +-PrintSettings
 *                 |       +-InitPorts--------BoardInfo
 *                 |       +-InitSwitch
 *                 |       \-InitConference--InitConferenceLink-+-JoinMember
 *                 |                                            +-SwitchMember
 *                 |                                            +-EnableMember
 *                 +-Keyboard-----------------SendCommand
 *                 *** forever ***
 *                         +---WaitForNCCCall-------+-PlayMessage
 *                                                  +-RecordMessage
 *                 *** if new call ***
 *                         +-JoinMember
 *                         +-SwitchMember
 *                         +-EnableMember
 *                         +-PlayBeep
 *                 *** else on Hook ***
 *                         +-PlayBeep
 *                         +-LeaveMember
 *                         +-SwitchRestore
 *
 * Shutdown
 * Comment
 *
 *****************************************************************************
 * Links between conferences with option m
 *
 *                           +-----------------------+
 *                           | Mother conference  0  |
 *                           |  loudest speaker = m  |
 *                           |  EC = FALSE           |
 *                           |  AGC Input = FALSE    |
 *                           |  AGC Output= FALSE    |
 *                           |  DTMF Clamping = FALSE|
 *                           +-----------------------+
 *                          /           |           . \
 *                        /             |            .  \
 *                      /               |             .   \
 * +----------------------+  +-----------------------+ ... +-----------------------+
 * | Daughter conference 1|  | Daughter conference 2 |     | Daughter conference m |
 * |                      |  |                       | ... |                       |
 * |                      |  |                       |     |                       |
 * |                      |  |                       |     |                       |
 * +----------------------+  +-----------------------+ ... +-----------------------+
 *      |         |                |         |                   |         |
 *   member_0 member_m+1 ...    member_1 member_m+2 ...   ...  member_m member_2m ...
 *
 *
 ****************************************************************************/


#define VERSION_NUM  "2.1"
#define VERSION_DATE __DATE__

/*----------------------------- Includes -----------------------------------*/
#include <stdio.h>
#include <stdarg.h>

#include "ctademo.h"
#include "adidef.h"
#include "nccdef.h"
#include "cnfdef.h"


/*----------------------------- Application Definitions --------------------*/

/* members max on mother conference */
#define CNF_MAX 32

/* status of channel */
#define CNF_CC              0
#define CNF_CONNECTED       1
#define CNF_NOCC            2
#define CNF_PLAY            3
#define CNF_RECORD          4

/* user events  */
#define APPEVN_SHUTDOWN     CTA_USER_EVENT( 0x1 )
#define APPEVN_PLAY         CTA_USER_EVENT( 0x2 )
#define APPEVN_RECORD       CTA_USER_EVENT( 0x3 )
#define APPEVN_STOP_PLAY    CTA_USER_EVENT( 0x4 )
#define APPEVN_STOP_RECORD  CTA_USER_EVENT( 0x5 )

/* default channel count */
#define DEFAULT (DWORD)(-1)

char *cnf_status[] = {"IDLE", "CONNECTED", "NOCC", "PLAY", "RECORD"};


typedef struct link_context
{
    CNFRESOURCEHD   resourcehd;         /* conferencing resource handle     */
    DWORD           memberid;           /* member handle                    */
    DWORD           membertimeslot;     /* cnf member timeslot number       */
    DWORD           memberstream;       /* cnf member stream number         */
} LINK_CONTEXT;

struct channel_context;                 /* forward declaration              */
typedef struct
{
    CTAQUEUEHD      queuehd;     /* cta queue handle associated with port.  */
    CTAHD           ctahd;       /* cta handle for conferencing and switch  */
    CNFRESOURCEHD   resourcehd[ CNF_MAX + 1 ];   /* resource handle         */
    DWORD           conferenceid[ CNF_MAX + 1 ]; /* conference ID           */
    unsigned        resourceid[ CNF_MAX + 1 ];   /* resource ID             */
    unsigned        confnum;                     /* conference number       */
    unsigned        virtualnb;                   /* virtual member number   */
    DWORD           swihd;                       /* switch ID               */
    DWORD           board;                       /* bord number             */
    DWORD           channelnb;                   /* channel count           */
    DWORD           channelstart;                /* 1st channel timeslot    */
    BOOL            h100;                        /* Switch to H100          */
    unsigned        numtrunks;                   /* number of trunks        */
    unsigned        maxchannelnb;                /* maximum channel count   */
    unsigned        dspstream;                   /* dsp stream              */
    unsigned        dspstreamoffset; /* dsp input timeslots offset per physical trunk */
    char            swiname [ 16 ];              /* switch name             */
    char            protocol[ 16 ];     /* name of protocol running on port */
    struct channel_context  *firstchannel;       /* first channel context   */
    LINK_CONTEXT    link[ CNF_MAX ][ 2 ]; /* Link of 2 members between conferences */
    ADI_BOARD_INFO  boardinfo;

} MYCONTEXT;

typedef struct channel_context
{
    CTAQUEUEHD queuehd;           /* queue handle associated with port.     */
    CTAHD      ctahd;             /* cta handle associated with port.       */
    NCC_CALLHD callhd;            /* NCC call handle                        */
    VCEHD      vh;                /* VCE handle                             */
    char       filename[ 16 ];    /* file name for playing or recording     */
    int        status;            /* status from cnf_status[]               */
    DWORD      memberid;          /* member handle                          */
    DWORD      trunktimeslot;     /* trunk timeslot number                  */
    DWORD      trunkstream;       /* trunk output stream number             */
    DWORD      dspstream;         /* dsp input stream number                */
    DWORD      dsptimeslot;       /* dsp input timeslot number              */
    DWORD      membertimeslot;    /* cnf member timeslot number             */
    DWORD      memberstream;      /* cnf member stream number               */
    DWORD      resourceindex;     /* cnf resource index                     */
    MYCONTEXT *mycontext;         /* my context address                     */
} CHANNEL_CONTEXT;

MYCONTEXT mycontext =
{
    NULL_CTAQUEUEHD,              /* cta queue handle associated with port. */
    NULL_CTAHD,                   /* cta handle for conferencing and switch */
    { 0 },                        /* resource handle                        */
    { 0 },                        /* conference ID                          */
    { 0 },                        /* resource ID                            */
    1,                            /* conference number                      */
    0,                            /* virtual member number                  */
    0,                            /* switch ID                              */
    DEFAULT,                      /* board number                           */
    DEFAULT,                      /* channel count                          */
    0,                            /* channel start                          */
    FALSE,                        /* Switch to H100                         */
    0,                            /* number of trunks                       */
    0,                            /* maximum channel count                  */
    0,                            /* dsp stream                             */
    0,                     /* dsp input timeslots offset per physical trunk */
    "agsw",                       /* switch name                            */
    "",                           /* name of protocol running on port       */
    NULL,                         /* first channel context                  */
    { 0 }                         /* Link of 2 members between conferences  */
};

/*----------------------------- Application Data ---------------------------*/
static unsigned TraceLevel = 0;
static unsigned ReportLevel = DEMO_REPORT_COMMENTS;

/*----------------------------- Forward References -------------------------*/
void PrintHelp      ( MYCONTEXT *mx );
void PrintSettings  ( MYCONTEXT *mx );
void RunDemo        ( MYCONTEXT *mx );
void Setup          ( MYCONTEXT *mx );
void InitMyContext  ( MYCONTEXT *mx );
void InitResource   ( MYCONTEXT *mx );
void InitPorts      ( MYCONTEXT *mx );
void InitSwitch     ( MYCONTEXT *mx);
void InitConference ( MYCONTEXT *mx );
void InitConferenceLink ( MYCONTEXT *mx );
THREAD_RET_TYPE THREAD_CALLING_CONVENTION Keyboard ( void *arg );
void SendCommand    ( MYCONTEXT *mx, int channel, DWORD Command );
int  WaitForNCCCall ( MYCONTEXT *mx , CHANNEL_CONTEXT **cx);
void JoinMember     ( CHANNEL_CONTEXT *cx );
void LeaveMember    ( CHANNEL_CONTEXT *cx );
void SwitchMember   ( MYCONTEXT *mx,CHANNEL_CONTEXT *cx , int mode );
void SwitchRestore  ( CHANNEL_CONTEXT *cx );
void EnableMember   ( CHANNEL_CONTEXT *cx );
void PlayBeep       ( CHANNEL_CONTEXT *cx , INT32 repeat_count );
int  PlayMessage    ( CHANNEL_CONTEXT *cx, char *filename, unsigned message, unsigned encoding, VCE_PLAY_PARMS  *parmsp );
int  RecordMessage  ( CHANNEL_CONTEXT *cx, char *filename, unsigned message,  unsigned encoding, VCE_RECORD_PARMS  *parmsp );
void Shutdown       ( MYCONTEXT *mx, int error );
void BoardInfo          ( CHANNEL_CONTEXT* cx );
void Comment        ( char *Format, ...);
void WaitForSpecificEvent( CTAQUEUEHD ctaqueuehd, DWORD desired_event,
                           CTA_EVENT *eventp );

int sslot = 0;
/*----------------------------------------------------------------------------
                                main
----------------------------------------------------------------------------*/
int main( int argc, char **argv )
{
    int c;
    MYCONTEXT *mx = &mycontext;

    printf( "NaturalConference Console Demonstration Program V.%s (%s)\n",
            VERSION_NUM, VERSION_DATE );

    mx->resourceid[0] = -1;
    while ((c = getopt(argc, argv, "F:l:t:b:r:p:n:N:m:v:eHh?"))
            != -1 )
    {
        switch( c )
        {
        case 'F':
            DemoSetCfgFile( optarg );
            break;
        case 'b':
            sscanf( optarg, "%d", &mx->board);
            break;
        case 'r':
            sscanf( optarg, "%d", &mx->resourceid[0]);
            break;
        case 'm':
            sscanf( optarg, "%d", &mx->confnum );
            break;
        case 'p':
            strncpy( mx->protocol, optarg, sizeof(mx->protocol) - 1 );
            break;
        case 't':
            sscanf( optarg, "%x", &TraceLevel );
            break;
        case 'l':
            sscanf( optarg, "%d", &ReportLevel );
            break;
        case 'n':
            sscanf( optarg, "%d", &mx->channelnb );
            break;
        case 's':
            sscanf( optarg, "%d", &sslot );
            break;
        case 'N':
            sscanf( optarg, "%d", &mx->channelstart );
            break;
        case 'v':
            sscanf( optarg, "%d", &mx->virtualnb );
            break;
        case 'e':
            mx->h100 = TRUE;
            break;
        case 'H':
        case '?':
        default:
            PrintHelp( mx );
            exit( 1 );
            break;
        }
    }
    if (optind < argc)
    {
        printf( "Invalid option.  Use -h for help.\n" );
        exit( 1 );
    }
    
    RunDemo( mx );

    return 0;
}

/*----------------------------------------------------------------------------
                            PrintSettings
----------------------------------------------------------------------------*/
void PrintSettings( MYCONTEXT *mx )
{
    printf( "\tBoard          = %d\n",    mx->board );
    if(mx->resourceid[0] != -1)
        printf( "\tResource       = %d\n",    mx->resourceid[0] );
    printf( "\tTraceLevel     = 0x%X\n",  TraceLevel );
    printf( "\tReportLevel    = %d\n",    ReportLevel );
    if (mx->confnum != 1)
    {
       printf( "\tconference number = %d\n",    mx->confnum );
    }
}

/*----------------------------------------------------------------------------
                            PrintHelp
----------------------------------------------------------------------------*/
void PrintHelp( MYCONTEXT *mx )
{
    printf("Usage: cnfjoin [opts]\n");
    printf("  where opts are:\n");
    printf( "  -F filename  CT Access Configuration File name.       "
            "Default = None\n" );
    printf( "  -l number    report level from 0 to 4                 "
        "Default = %d\n", ReportLevel );
    printf( "  -t mask      trace level                              "
        "Default = %d\n", TraceLevel );
    printf( "  -b n         AG/CG board number.                      "
        "Default = %d\n", mx->board );
    printf( "  -r n         resource number in cnf.cfg.              "
        "Default = %d\n", 0 );
    printf( "  -p protocol  Protocol to run (usually loop-start).    "
        "Default |   depends\n" );
    printf( "  -n count     channel count.                           "
        "Default | on the board\n");
    printf( "  -N number    first channel.                           "
        "Default =%d\n", mx->firstchannel);
    printf( "  -m count     conference number.                       "
        "Default = %d\n", mx->confnum );
    printf( "  -v count     virtual member.                          "
        "Default = %d\n", mx->virtualnb );
    printf( "  -e           switch members to H100 rather than trunk\n");
}

/*-------------------------------------------------------------------------
                            RunDemo
-------------------------------------------------------------------------*/
void RunDemo( MYCONTEXT *mx )
{
    unsigned i;
    CHANNEL_CONTEXT *cx;

    /* open all resources */

    Setup( mx );

    /* start call control on all required ports */

    for ( cx = mx->firstchannel, i = 0; i < (mx->channelnb); i++, cx++ )
    {
        if ( cx->status == CNF_CC )
        {
            nccStartProtocol( cx->ctahd, mx->protocol, NULL, NULL, NULL);
        }
    }

    /* user commands thread */

    DemoSpawnThread(Keyboard, mx);


    /* Loop, receiving calls and processing them. */

    for(;;)
    {

        /* wait connected or disconnected call */

        if ( WaitForNCCCall( mx, &cx ) == TRUE )  /* new member */
        {
            /* join a member to the conference */
            JoinMember( cx );

            /* switch the member to the conference */
            SwitchMember(mx, cx , 0);

            /* enable talker attribute
             */
            EnableMember( cx );

            /* play a beep to signal other members
             *  that a new member has joined the conference
             */
            PlayBeep( cx , 1 );
        }

        /* leave member */

        else
        {
            /* play a beep to signal other members
             * that the member has leaved the conference
             */
            PlayBeep( cx , 2 );

            /* leave the conference */
            LeaveMember ( cx );

            /* restore the link between DSP and Trunk */
            SwitchRestore( cx );
        }

    }
}

/*-------------------------------------------------------------------------
                            Setup
-------------------------------------------------------------------------*/
void Setup( MYCONTEXT *mx )
{
    /*
     * Initialize CT Access.
     */
    {
        CTA_SERVICE_NAME init_svc[] = { { "adi", "adimgr" },
                                        { "ncc", "adimgr" },
                                        { "cnf", "cnfmgr" },
                                        { "swi", "swimgr" },
                                        { "vce", "vcemgr" } };

        /* Initialize ADI and NCC Service Manager names */
        init_svc[ 0 ].svcmgrname = DemoGetADIManager();
        init_svc[ 1 ].svcmgrname = DemoGetADIManager();

        DemoInitialize( init_svc, ARRAY_DIM( init_svc ) );
    }

    DemoSetReportLevel( ReportLevel );

    /* From now on, the registered error handler will be called.
     * No need to check return values.
     */
    ctaSetErrorHandler( DemoErrorHandler, NULL );

    /* initialisation of one context CTA for switching and conferencing
     */

    InitMyContext( mx );

    /*
     * Open Conferencing resources on that board.
     * Open all ports, which maps to a telephone line interface.
     * On these ports, we'll accept calls, and perform the
     * join member.
     */

    InitResource( mx );

    PrintSettings( mx );

    InitPorts( mx );

    InitSwitch( mx );

    InitConference( mx );

}


/*----------------------------------------------------------------------------
                            InitMyContext
    open a CTA queue , one context and open the services cnf and swi
----------------------------------------------------------------------------*/
void InitMyContext( MYCONTEXT *mx )
{
    CTA_SERVICE_DESC services[] = {
    { { "cnf", "cnfmgr" }, {0}, {0}, {0} },
    { { "swi", "swimgr" }, {0}, {0}, {0} } };

    services[ 0 ].mvipaddr.board = mx->board;
    services[ 1 ].mvipaddr.board = mx->board;

    /* Open one queue and conferencing and switching services */
    DemoOpenPort( 0, "DEMOCONTEXT", services, ARRAY_DIM( services ), &mx->queuehd , &mx->ctahd );

    /* Set debugging trace for the CNF service only */
    ctaSetTraceLevel( mx->ctahd, "cnf", TraceLevel );

}

/*----------------------------------------------------------------------------
                            InitResource

  Open confnum resources to host confnum conferences on the board, open one
  more resource to host the mother conference (if required).
  Failed if not enough resources are available on this board.
----------------------------------------------------------------------------*/
void InitResource ( MYCONTEXT* mx )
{
    CTA_EVENT Event;
    unsigned resnumx,     /* Index to wresourcenumlist */
             *wresourcenumlist,
             resourcenum,
             rhdx = 0,          /* Index to mx->resourcehd - resource already opened on that board.*/
             confnum,           /* Number of resource to open */
             userresource;      /* Resource index with option -R */;


    /* First get the maximum number of resources defined in the entire system */
    cnfGetResourceList ( mx->ctahd, 0, NULL, &resourcenum);

    /* add one mother conference, for chaining multiple conferences */
    if ( (confnum = mx->confnum) > 1 )
    {
        confnum++;
    }

   /* Allocate a resource number list large enough */
    wresourcenumlist = (unsigned*)malloc(resourcenum*sizeof(unsigned));

    /* Second get all the resources defined */
    cnfGetResourceList ( mx->ctahd, resourcenum, wresourcenumlist, &resourcenum );

    /* If the user specified a resource though option -R */
    if ( (userresource = mx->resourceid[0]) != -1 )
    {
        CNF_RESOURCE_INFO resourceinfo;

        /* Open this one */
        cnfOpenResource ( mx->ctahd, userresource, &mx->resourcehd[0]);

        DemoWaitForSpecificEvent( mx->ctahd, CNFEVN_OPEN_RESOURCE_DONE, &Event);

        cnfGetResourceInfo ( mx->resourcehd[ 0 ], &resourceinfo , sizeof(CNF_RESOURCE_INFO));

        if ( mx->board == DEFAULT )
             mx->board = resourceinfo.board;

        if ( mx->board == resourceinfo.board )
        {
            Comment( "Open conferencing resource %d.", userresource );
        }
        else
        {
            /* Open a valid resource but on the wrong board */
            cnfCloseResource ( mx->resourcehd[ 0 ]);
            DemoWaitForSpecificEvent( mx->ctahd, CNFEVN_CLOSE_RESOURCE_DONE, &Event);
            free(wresourcenumlist);
            Comment( "A resource on board %u has been specified.", resourceinfo.board );
            Comment( "You choose however to work on board %u.", mx->board );
            Shutdown( mx, 1 );
         }

        // increment rhdx
        rhdx++;
    }


    /*
     * Try to open confnum resources available on board mx->board.
     */
    for ( resnumx = 0;
          resnumx < resourcenum  && rhdx < confnum; resnumx++)
    {
        DWORD errorcode;
        CNF_RESOURCE_INFO resourceinfo;

        /*
         * Skip the resource already opened
         */
        if ( wresourcenumlist[ resnumx ] == userresource )
            continue;

        /*
         * Here we need to change the resource handler.
         * We want to continue if the current resource isn't available.
         * The current error handler exits on all errors
         */

        ctaSetErrorHandler( NULL, NULL);

        errorcode = cnfOpenResource ( mx->ctahd, wresourcenumlist[ resnumx ], &mx->resourcehd[ rhdx ]);

        if ( errorcode != SUCCESS )
        {
            if ( errorcode != CNFERR_RESOURCE_NOT_AVAILABLE )
            {
                char buffer[ CTA_MAX_PATH ];
                ctaGetText( mx->ctahd, errorcode, buffer, CTA_MAX_PATH );
                Comment("Trying to open resource %u ", wresourcenumlist[ resnumx ] );
                DemoErrorHandlerContinue( mx->ctahd, errorcode, buffer , "cnfOpenResource" );
                Shutdown( mx, 1 );
            }
        }

        /*
         * Get the CNFEVN_OPEN_RESOURCE_DONE event
         */
       DemoWaitForSpecificEvent( mx->ctahd, CNFEVN_OPEN_RESOURCE_DONE, &Event);

        /*
         * Restore the previous error handler that exits on all errors
         */
        ctaSetErrorHandler( DemoErrorHandler, NULL);

        cnfGetResourceInfo (  mx->resourcehd[ rhdx ], &resourceinfo , sizeof(CNF_RESOURCE_INFO));

        if ( mx->board == DEFAULT )
             mx->board = resourceinfo.board;

        if ( mx->board == resourceinfo.board )
        {
            /* Eh, we've got an available Conferencing Resource on this board */
            Comment( "Open conferencing resource %d.", wresourcenumlist[ resnumx ] );
            mx->resourceid[rhdx++] = wresourcenumlist[ resnumx ];

        }
        else
        {
            /* Open a valid resource but on the wrong board */
            cnfCloseResource (  mx->resourcehd[ rhdx ]);
            DemoWaitForSpecificEvent( mx->ctahd, CNFEVN_CLOSE_RESOURCE_DONE, &Event);
        }
    }


    if (rhdx < confnum )
    {
        /* Not enough resources opened on this board */
        Comment("not enough available conferencing resources on this board ");
        Comment("%u resources are available. %u resources needed.", rhdx, confnum);
        free(wresourcenumlist);
        Shutdown( mx, 1 );
    }

    free(wresourcenumlist);
} // end of InitResource

/*----------------------------------------------------------------------------
                            InitPorts
    open a context for each port and start the protocol
----------------------------------------------------------------------------*/
void InitPorts( MYCONTEXT *mx )
{
    CTA_EVENT event;
    unsigned i;
    CTA_SERVICE_DESC services[] = { { { "adi", "adimgr" }, {0}, {0}, {0} },
                                    { { "ncc", "adimgr" }, {0}, {0}, {0} },
                                    { { "vce", "vcemgr" }, {0}, {0}, {0} } };
    CHANNEL_CONTEXT mycx = {0}, *newcx;

    /* Specify manager to use for ADI and NCC service. */
    services[ 0 ].mvipaddr.board   = mx->board;
    services[ 0 ].mvipaddr.mode    = ADI_FULL_DUPLEX;
    services[ 1 ].mvipaddr.board   = mx->board;
    services[ 1 ].mvipaddr.mode    = ADI_FULL_DUPLEX;
    services[ 2 ].mvipaddr.board   = mx->board;
    services[ 2 ].mvipaddr.mode    = ADI_FULL_DUPLEX;

    /* port number */
    services[ 0 ].mvipaddr.timeslot = mx->channelstart;
    services[ 1 ].mvipaddr.timeslot = mx->channelstart;

    /*
     * Open a first channel to get the number of ports,
     * and other info.
     * then go along
     */
    mycx.mycontext = mx;
    mycx.queuehd = mx->queuehd;

    DemoOpenAnotherPort(0, "DEMOCONTEXT",
                        services, ARRAY_DIM(services),
                        mycx.queuehd, &mycx.ctahd);

    BoardInfo( &mycx );

    ctaDestroyContext( mycx.ctahd ); /* this will close services automatically */
    WaitForSpecificEvent( mx->queuehd, CTAEVN_DESTROY_CONTEXT_DONE, &event );
    if( event.value != CTA_REASON_FINISHED )
    {
        DemoShowError( "Destroying the CT Access context failed", event.value );
        exit( 1 );
    }

    mx->firstchannel = (CHANNEL_CONTEXT*) malloc((mx->channelnb + mx->virtualnb)*sizeof( CHANNEL_CONTEXT ) );


    /* for all members if NOCC is selected */
    newcx = mx->firstchannel;
    i = 0;
    if ( strcmp( mx->protocol,  "nocc" ) == 0 )
    {
        //
        //   Loop through live caller ports (from the '-n' option on command line)
        //
        for ( ; i < mx->channelnb; i++, newcx++ )
        {
            /* init context */
            memset( newcx , 0 , sizeof( CHANNEL_CONTEXT ) );

            /* my context */
            newcx->mycontext = mx;

            /* my queue hd */
            newcx->queuehd = mx->queuehd;

            /* port number */
            if (mx->h100)
            {
                //
                //  H.100 and H.110 bus streams are configurable for
                //  32, 64 or 128 timeslots per stream. This example
                //  assumes that bus streams are configured
                //  for 128 timeslots.
                //
                newcx->trunkstream = 2 * ((i + mx->channelstart ) / 128);  // transmit and receive
                newcx->trunktimeslot = (i + mx->channelstart) % 128;
            }
            else
            {
                //
                //  Pick a trunk port.
                //
                //  Stream number will determine the trunk.  Each trunk has four streams.
                //  Timeslot will determine the channel within the trunk.
                //
                //  mx->dspstreamoffset is the number of timeslots per trunk.
                //
                //
                newcx->trunkstream = ((i + mx->channelstart) / mx->dspstreamoffset) * 4;  // each trunk has four streams
                newcx->trunktimeslot = (i + mx->channelstart) % mx->dspstreamoffset;
            }

            newcx->status = CNF_NOCC;

            /* the resource is 0 or 1 to n */
            if ( mx->confnum > 1 )
                newcx->resourceindex = ( i % mx->confnum ) + 1;

        }
    }
    /* for all other channels (virtual included) */
    for ( ; i < (mx->channelnb + mx->virtualnb); i++, newcx++)
    {
        /* init context */
        memset( newcx , 0 , sizeof( CHANNEL_CONTEXT ) );

        /* my context */
        newcx->mycontext = mx;

        /* my queue hd */
        newcx->queuehd = mx->queuehd;


        newcx->dspstream = mx->dspstream;
        newcx->dsptimeslot = (i + mx->channelstart);

        newcx->trunkstream = ((i + mx->channelstart) / mx->dspstreamoffset) * 4;
        newcx->trunktimeslot = (i + mx->channelstart) % mx->dspstreamoffset;

        /* port number */
        services[ 0 ].mvipaddr.timeslot = newcx->dsptimeslot;
        services[ 1 ].mvipaddr.timeslot = newcx->dsptimeslot;

        /* the resource is 0 or 1 to n */
        if ( mx->confnum > 1 )
            newcx->resourceindex = ( i % mx->confnum ) + 1;

        /*
         * Open all the port on the same queue
         *  the userid is the channel context
         */

        DemoOpenAnotherPort(i, "DEMOCONTEXT",
                            services, ARRAY_DIM(services),
                            newcx->queuehd, &newcx->ctahd);


        /* start protocol with default values */
        if ( i >= mx->channelnb )
        {
            /* NOCC */
            DemoStartNCCProtocol( newcx->ctahd, "nocc", NULL, NULL, NULL );
            DemoWaitForSpecificEvent( newcx->ctahd, NCCEVN_CALL_CONNECTED, &event );
            newcx->status = CNF_NOCC;
        }
        else
        {
            /* Don't start call control now, should be ready to receive
               line events as soon as call control get started */
            newcx->status = CNF_CC;
        }
    }
}

/*----------------------------------------------------------------------------
                                InitSwitch
----------------------------------------------------------------------------*/
void InitSwitch( MYCONTEXT *mx )
{
    if (mx->boardinfo.boardtype != OAM_PRODUCT_NO_CGHOST)
    {
        swiOpenSwitch( mx->ctahd, mx->swiname , mx->board , SWI_ENABLE_RESTORE, &mx->swihd );
    }
}

/*----------------------------------------------------------------------------
                                InitConference

  create conference[s] and link them together if multiple conferences have
  been created
----------------------------------------------------------------------------*/
void InitConference( MYCONTEXT *mx )
{
    unsigned i;
    CHANNEL_CONTEXT *cx;

    /*
     * create one conference if confnum == 1 or
     *  1 mother conference and confnum daughter conferences
     */
    //(mx->confnum) --;
    for ( i = 0; i < (( mx->confnum == 1 ) ? 1 : ( (mx->confnum + 1 ))); i++ )
    {
        cnfCreateConference ( mx->resourcehd[ i ], NULL, &mx->conferenceid[ i ] );
        Comment( "Create conference  %u on resource %u.", i , mx->resourceid[i]);
    }

    /*
     * Link between conferences (only for multiple conferences)
     */
    if ( mx->confnum > 1 )
        InitConferenceLink( mx );

    /*
     * join conference for virtual member
     */
    Comment("Starting joining virtual and nocc members.");

    cx = mx->firstchannel;
    for ( i = 0; i < (mx->channelnb + mx->virtualnb); i++ )
    {
        /* for No Call Control we allocate a seat */
        if ( cx->status == CNF_NOCC )
        {
            JoinMember( cx );
            /* Switch to the trunk for NOCC members, join to dsp for virtual ones */
            SwitchMember(mx, cx , ( (i < mx->channelnb) ? 0 : 1 ) );
            EnableMember( cx );
            /* for playing or recording file the echo cancellor is unnecessary */
            cnfSetMemberAttribute(mx->resourcehd[ cx->resourceindex ],
                                  cx->memberid,
                                  MEMBER_ATTR_EC_ENABLE,
                                  FALSE);

        }
        cx++;
    }
}

/*----------------------------------------------------------------------------
                                InitConferenceLink

 create : 1 link between conference 0 and conference m
----------------------------------------------------------------------------*/
void InitConferenceLink(  MYCONTEXT *mx )
{
    CNF_MEMBER_PARMS parms          = { 0 };
    CNF_MEMBER_INFO memberinfo;
    unsigned i, j;
    INT32 loudest;

    parms.size = sizeof(CNF_MEMBER_PARMS);

    /* n members in conference 0 */
    for ( i = 0; i < mx->confnum; i++ )
    {
        cnfJoinConference ( mx->resourcehd[ 0 ],
                            mx->conferenceid[ 0 ],
                            &parms,
                            &mx->link[ i ][ 0 ].memberid );


        /* retrieve member info to know the used stream/timeslot */
        cnfGetMemberInfo ( mx->resourcehd[ 0 ],
                           mx->link[ i ][ 0 ].memberid,
                           &memberinfo,
                           sizeof( memberinfo ) );

        mx->link[ i ][ 0 ].memberstream     = memberinfo.stream;
        mx->link[ i ][ 0 ].membertimeslot   = memberinfo.timeslot;
        mx->link[ i ][ 0 ].resourcehd       = mx->resourcehd[ 0 ];
    }

    /* 1 members in each conference n */
    for ( i = 0; i < mx->confnum; i++ )
    {
        cnfJoinConference ( mx->resourcehd[ 1 + i ],
                            mx->conferenceid[ 1 + i ],
                            &parms,
                            &mx->link[i][1].memberid );

        /* retrieve member info to know the used stream/timeslot */
        cnfGetMemberInfo ( mx->resourcehd[ 1 + i ],
                           mx->link[ i ][ 1 ].memberid,
                           &memberinfo,
                           sizeof( memberinfo ) );

        mx->link[ i ][ 1 ].memberstream     = memberinfo.stream;
        mx->link[ i ][ 1 ].membertimeslot   = memberinfo.timeslot;
        mx->link[ i ][ 1 ].resourcehd       = mx->resourcehd[ 1 + i ];
    }

    /* link conference 0 and n in Duplex */
    for ( i = 0; i < mx->confnum ; i++ )
    {
        if (mx->boardinfo.boardtype != OAM_PRODUCT_NO_CGHOST)
        {
            SWI_TERMINUS input = { MVIP95_LOCAL_BUS, 0, 0 };
            SWI_TERMINUS output = { MVIP95_LOCAL_BUS, 0, 0 };

            input.stream = mx->link[ i ][ 0 ].memberstream;
            input.timeslot = mx->link[ i ][ 0 ].membertimeslot;

            output.stream = mx->link[ i ][ 1 ].memberstream + 1;
            output.timeslot = mx->link[ i ][ 1 ].membertimeslot;

            swiMakeConnection( mx->swihd, &input, &output, 1 );

            output.stream--; /* the input stream is output stream minus one */
            input.stream ++; /* the output stream is input stream plus one  */

            swiMakeConnection( mx->swihd, &output, &input, 1 );
        }
        /*
         * members attributs for the n conferences
         */
        for ( j = 0; j < 2; j++ )
        {
            DWORD attributes[ ] = { MEMBER_ATTR_TALKER_ENABLE,
                                    MEMBER_ATTR_EC_ENABLE,
                                    MEMBER_ATTR_INPUT_AGC_ENABLE,
                                    MEMBER_ATTR_OUTPUT_AGC_ENABLE,
                                    MEMBER_ATTR_TALKER_PRIVILEGE,
                                    MEMBER_ATTR_DTMF_CLAMPING_ENABLE };

            INT32 values[ sizeof( attributes ) / sizeof( attributes[ 0 ] ) ]
                = { TRUE, FALSE, FALSE, FALSE, TRUE, FALSE };

            cnfSetMemberAttributeList ( mx->link[ i ][ j ].resourcehd,
                                        mx->link[ i ][ j ].memberid,
                                        attributes,
                                        values,
                                        sizeof( attributes ) / sizeof( attributes[ 0 ] ) );
        }
    }

    /* get the default loudest speaker */
    cnfGetConferenceAttribute ( mx->resourcehd[ 0 ],
                                mx->conferenceid[ 0 ],
                                CONF_ATTR_NUM_LOUDEST_SPEAKERS,
                                &loudest);

    /* the loudest speaker is the number of daughter conferences */
    cnfSetConferenceAttribute ( mx->resourcehd[ 0 ],
                                mx->conferenceid[ 0 ],
                                CONF_ATTR_NUM_LOUDEST_SPEAKERS,
                                mx->confnum );

    /* add one to the loudest speaker for the link with mother conference */
    loudest++;

    for ( i = 0; i < mx->confnum; i++ )
    {
        cnfSetConferenceAttribute ( mx->resourcehd[ i ],
                                    mx->conferenceid[ i ],
                                    CONF_ATTR_NUM_LOUDEST_SPEAKERS,
                                    loudest );

        Comment( "Link between conference 0 and %d.", i + 1 );
    }
}

/*----------------------------------------------------------------------------
                                Keyboard Thread

 This thread wait a command
----------------------------------------------------------------------------*/
THREAD_RET_TYPE THREAD_CALLING_CONVENTION Keyboard( void *arg )
{
    MYCONTEXT *mx = (MYCONTEXT*)arg;
    CHANNEL_CONTEXT *cx;
    char command[ 80 ];
    int argument;
    int ret;
    char Line[ 80 ];

    /* User event */
    CTA_EVENT event;
    memset( &event, 0, sizeof( event ) );
    event.id = APPEVN_SHUTDOWN;
    event.ctahd = mx->ctahd;

    /* wait a command */
    for(;;)
    {
        *command = 0;
        if ( fgets( Line, sizeof( Line ), stdin ) == NULL)  break;
        if ( *Line == '\n' ) { printf( "Type 'h' to help.\n" ); continue; }

        ret = sscanf( Line, "%s %d\n", command, &argument );
        if ( *command == 'q') break;
        if ( *command == 's')
        {
            unsigned membernb = 0;
            unsigned channelnb = 0;
            cx = mx->firstchannel;
            while ( channelnb < (mx->channelnb + mx->virtualnb) )
            {

                if ( cx->memberid != 0 )
                {
                    printf( "\tchannel %d : %s \t: memberid = %08X\n",
                            channelnb, cnf_status[ cx->status ] , cx->memberid );
                    membernb++;
                }
                else
                {
                    printf( "\tchannel %d : %s\n",
                            channelnb, cnf_status[ cx->status ] );
                }
                cx++; channelnb++;
            }
            printf( "\tnumber of channels = %d\n", channelnb );
            printf( "\tnumber of members  = %d\n", membernb );
            continue;
        }

        if ( ret > 1 )
        {
            if ( strcmp( command, "pf") == 0 )
            {
                SendCommand( mx, argument, APPEVN_PLAY );
                continue;
            }
            else if ( strcmp( command, "rf" ) == 0)
            {
                SendCommand( mx, argument, APPEVN_RECORD );
                continue;
            }
            else if ( strcmp( command, "ps" ) == 0)
            {
                SendCommand( mx, argument, APPEVN_STOP_PLAY );
                continue;
            }
            else if ( strcmp( command, "rs" ) == 0 )
            {
                SendCommand( mx, argument, APPEVN_STOP_RECORD );
                continue;
            }
        }
        printf("\tpf <n> : play file sample<n>.pcm on the channel n (-1 for all)\n"
               "\trf <n> : record file cnfjoin<n>.pcm on the channel n (-1 for all)\n"
               "\tps <n> : stop play file on the channel n (-1 for all)\n"
               "\trs <n> : stop record file on the channel n(-1 for all)\n"
               "\ts      : status\n"
               "\th      : help\n"
               "\tq      : quit\n");
    }

    /* send the user event */
    event.id = APPEVN_SHUTDOWN;
    ctaQueueEvent( &event );

    THREAD_RETURN;
}

/*----------------------------------------------------------------------------
                 send a command to the main thread

----------------------------------------------------------------------------*/
void SendCommand( MYCONTEXT *mx, int channel, DWORD Command )
{
    CHANNEL_CONTEXT *cx = mx->firstchannel;
    CTA_EVENT event;
    unsigned i;

    memset(&event, 0, sizeof(event));
    event.ctahd = mx->ctahd;

    if ( channel >= 0 )
    {
        if ( ( channel <   (int)  mx->channelnb ) ||
             ( channel >=  (int)( mx->virtualnb + mx->channelnb ) ) )
        {
            printf( "\tunexpected channel\n" );
            return ;
        }
    }

    for (i = 0; i < (mx->channelnb + mx->virtualnb); i++)
    {
        if ( ( channel == -1 ) || ( cx->dsptimeslot == (unsigned)channel ) )
        {
            event.value = i;
            event.id = Command;
            ctaQueueEvent( &event );
        }
        cx++;
    }
}

/*----------------------------------------------------------------------------
  WaitForNCCCall for all ports

  return TRUE  for an incoming call
  return FALSE for a disconnected call
----------------------------------------------------------------------------*/
int WaitForNCCCall( MYCONTEXT *mx , CHANNEL_CONTEXT **cx )
{
    if ( mx->channelnb == 0 )
        Comment( "--------\n\tWaiting for user command ... (h to help)" );
    else
        Comment( "--------\n\tWaiting for incoming call or user command ... (h to help)" );

    for (;;)
    {
        CTA_EVENT event;
        DemoWaitForEvent( mx->ctahd, &event );
        *cx = mx->firstchannel+event.userid;       /* to retrieve the context */

        switch( event.id )
        {
        case NCCEVN_START_PROTOCOL_DONE:
            if ( event.value != CTA_REASON_FINISHED )
            {
                DemoShowError( "Start protocol failed", event.value );
                printf( "\tCheck board configuration for available protocols.\n" );
                Shutdown( mx, 1 );
            }
            break;

        case NCCEVN_SEIZURE_DETECTED:
            (*cx)->callhd = event.objHd;
            Comment( "Seizure Detected..." );
            break;

        case NCCEVN_INCOMING_CALL:
            /* Get any address information for the application*/
            {
                NCC_CALL_STATUS status;
                nccGetCallStatus( (*cx)->callhd, &status, sizeof(status) );
                Comment("Incoming Call Channel %d calling address : %s\n",
                        event.userid,
                        status.callingaddr);
            }
            nccAnswerCall( (*cx)->callhd, 1, NULL );
        break;

        case NCCEVN_ANSWERING_CALL :
            /* expect CALL_CONNECTED after rings are complete */
            break;

        case NCCEVN_REJECTING_CALL:
            Comment( "Board timed out waiting for answer." );
            break;

        case NCCEVN_CALL_CONNECTED:
            Comment( "Call connected. channel %d", event.userid );
            (*cx)->status = CNF_CONNECTED;  /* welcome */
            adiFlushDigitQueue( (*cx)->ctahd );
            if (PlayMessage(*cx,"ctademo.vox",PROMPT_WELCOME,0,NULL) == SUCCESS)
            {
                Comment("Playing welcome file...");
                break;                      /* wait PLAY_DONE */
            }
            else
            {
                return TRUE;                /* conference without welcome   */
            }

        case NCCEVN_RECEIVED_DIGIT:
        case NCCEVN_PROTOCOL_ERROR :        /* e.g. abandoned after seizure */
        case NCCEVN_LINE_OUT_OF_SERVICE:    /* e.g, permanent signal        */
        case NCCEVN_LINE_IN_SERVICE:        /* back in service after OOS    */
            break;

        case NCCEVN_CALL_DISCONNECTED:
            nccReleaseCall(event.objHd, NULL);
            DemoWaitForSpecificEvent(event.ctahd, NCCEVN_CALL_RELEASED, &event);
            (*cx)->callhd = (NCC_CALLHD)0;
            (*cx)->status = CNF_CC;
            if ( (*cx)->memberid != 0 )     /* linked member ?      */
            return FALSE;                   /* conferencing process */
            break;

        case CNFEVN_ACTIVE_TALKERS_CHANGE:
            Comment("Active talkers change...");
            break;

        case VCEEVN_PLAY_DONE:
            vceClose( (*cx)->vh );
            if ( (*cx)->status == CNF_CONNECTED )
                return TRUE;                    /* conferencing process */
            if ( (*cx)->status == CNF_PLAY )
                PlayMessage( *cx, (*cx)->filename, 0, VCE_ENCODE_PCM8M16, NULL );
            else
                Comment("stop Play file on channel %d.", event.userid);
            break;

        case VCEEVN_RECORD_DONE:
            vceClose( (*cx)->vh );
            Comment( "record file done on channel %d.", event.userid );
            (*cx)->status = CNF_NOCC;
            break;

        case APPEVN_SHUTDOWN:
            Shutdown( mx, 0 );
            break;

        case APPEVN_PLAY:
            *cx = mx->firstchannel + event.value; /* to retrieve the context */

            if ( (*cx)->status == CNF_NOCC )
            {
                FILE *file;
                sprintf( (*cx)->filename, "sample%d.pcm",event.value );
                if ( ( file = fopen( (*cx)->filename, "r") ) == NULL )
                    strcpy( (*cx)->filename , "sample.pcm" );
                else
                    fclose( file );

                adiFlushDigitQueue( (*cx)->ctahd );
                if (PlayMessage( *cx, (*cx)->filename, 0, VCE_ENCODE_PCM8M16, NULL ) == SUCCESS)
                {
                    Comment( "Play file %s on channel %d.",
                             (*cx)->filename, event.value );
                    (*cx)->status = CNF_PLAY;
                }
            }
            break;

        case APPEVN_RECORD:
            *cx =  mx->firstchannel + event.value; /* to retrieve the context */
            if ((*cx)->status == CNF_NOCC )
            {
                sprintf( (*cx)->filename, "cnfjoin%d.pcm", event.value );
                adiFlushDigitQueue( (*cx)->ctahd );
                if (RecordMessage( *cx, (*cx)->filename, 0, VCE_ENCODE_PCM8M16, NULL ) == SUCCESS)
                {
                    Comment( "Recording file %s on channel %d.",
                             (*cx)->filename, event.value );
                    (*cx)->status = CNF_RECORD;
                }
            }
            break;

        case APPEVN_STOP_PLAY:
            *cx = mx->firstchannel + event.value; /* to retrieve the context */
            if ( (*cx)->status == CNF_PLAY )
            {
                vceStop( (*cx)->ctahd );
                (*cx)->status = CNF_NOCC;
            }
            break;

        case APPEVN_STOP_RECORD:
            *cx =  mx->firstchannel + event.value; /* to retrieve the context */
            if ( (*cx)->status == CNF_RECORD )
            {
                vceStop( (*cx)->ctahd );
                (*cx)->status = CNF_NOCC;
            }
            break;

        default:
            DemoReportUnexpectedEvent( &event );
            break;
        }
    }
}

/*----------------------------------------------------------------------------
                                JoinMember

  new member to the conference
----------------------------------------------------------------------------*/
void JoinMember( CHANNEL_CONTEXT *cx )
{
    CNF_MEMBER_PARMS parms          = { 0 };
    int ret;
    CNF_MEMBER_INFO memberinfo;
    MYCONTEXT *mx = cx->mycontext;


    parms.size = sizeof( CNF_MEMBER_PARMS );

    ret = cnfJoinConference(mx->resourcehd[ cx->resourceindex ],
                            mx->conferenceid[ cx->resourceindex ],
                            &parms,
                            &cx->memberid);

    if ( ret != SUCCESS )
    {
        DemoShowError( "Error join member.", ret );
        Shutdown( mx, 1 );
    }

    /* retrieve member info to know the used stream/timeslot */
    ret = cnfGetMemberInfo(mx->resourcehd[ cx->resourceindex ],
                           cx->memberid,
                           &memberinfo,
                           sizeof(memberinfo));

    if ( ret != SUCCESS )
    {
        DemoShowError( "Error member info.", ret );
        Shutdown( mx, 1 );
    }
    cx->memberstream   = memberinfo.stream;
    cx->membertimeslot = memberinfo.timeslot;

    Comment( "Join member. id = 0x%08X" , cx->memberid );
}


/*----------------------------------------------------------------------------
                                LeaveMember
    leave a conferencing member and restore the link between dsp interface
    to trunk
----------------------------------------------------------------------------*/
void LeaveMember( CHANNEL_CONTEXT *cx )
{
    MYCONTEXT *mx = cx->mycontext;

    cnfLeaveConference ( mx->resourcehd[ cx->resourceindex ], cx->memberid );

    Comment( "Leave member. Id = 0x%08X", cx->memberid );
    cx->memberid = 0;
}

/*----------------------------------------------------------------------------
                                SwitchMember
    link a member to trunk interface or dsp interface in duplex mode
----------------------------------------------------------------------------*/
void SwitchMember(MYCONTEXT *mx, CHANNEL_CONTEXT *cx , int mode )
{

    if (cx->mycontext->boardinfo.boardtype != OAM_PRODUCT_NO_CGHOST)
    {
        MYCONTEXT *mx = cx->mycontext;
        SWI_TERMINUS input = { MVIP95_LOCAL_BUS, 0, 0 };
        SWI_TERMINUS output = { MVIP95_LOCAL_BUS, 0, 0 };

        input.stream = cx->memberstream;
        input.timeslot = cx->membertimeslot ;

        if ( mode == 1 )
        {
            output.stream   = cx->dspstream + 1;
            output.timeslot = cx->dsptimeslot;
        }
        else
        {
            if (mx->h100)
            {
                output.bus = MVIP95_MVIP_BUS;
            }
            output.stream = cx->trunkstream + 1;
            output.timeslot = cx->trunktimeslot;
        }

        Comment("Switching between member local:{%d/%d}:%d and %s:{%d/%d}:%d.",
                cx->memberstream + 1, cx->memberstream, cx->membertimeslot,
                mx->h100 ? "mvip" : "local",
                output.stream - 1, output.stream, output.timeslot );

        swiMakeConnection( mx->swihd, &input, &output, 1 );

        output.stream--;        /* the input stream is output stream minus one */
        input.stream ++;        /* the output stream is input stream plus one  */
        swiMakeConnection( mx->swihd, &output, &input, 1 );
    }
    else
    {
         cnfSwitchMember(mx->resourcehd[cx->resourceindex],cx->memberid,cx->trunktimeslot);
    }            
}

/*----------------------------------------------------------------------------
                                SwitchRestore
    restore the link from DSP port to trunk interface
    and send a pattern to old conferencing timeslot
----------------------------------------------------------------------------*/
void SwitchRestore( CHANNEL_CONTEXT *cx )
{
    if (cx->mycontext->boardinfo.boardtype != OAM_PRODUCT_NO_CGHOST)
    {
        MYCONTEXT *mx = cx->mycontext;
        SWI_TERMINUS dspinput   = { MVIP95_LOCAL_BUS, 0, 0 };
        SWI_TERMINUS trunkoutput    = { MVIP95_LOCAL_BUS, 0, 0 };
        SWI_TERMINUS cnfoutput  = { MVIP95_LOCAL_BUS, 0, 0 };
        BYTE pattern = 0x5d;    /* pattern A law */

        dspinput.stream = cx->dspstream;
        dspinput.timeslot = cx->dsptimeslot;

        trunkoutput.stream = cx->trunkstream + 1;
        trunkoutput.timeslot = cx->trunktimeslot;

        cnfoutput.stream    = cx->memberstream + 1;
        cnfoutput.timeslot = cx->membertimeslot;

        swiMakeConnection( mx->swihd, &dspinput, &trunkoutput, 1 );

        trunkoutput.stream--;  /* the input stream is output stream minus one */
        dspinput.stream ++;    /* the output stream is input stream plus one  */

        swiMakeConnection( mx->swihd, &trunkoutput, &dspinput, 1 );

        Comment( "Switching between DSP {%d/%d}:%d and Trunk {%d/%d}:%d.",
                 dspinput.stream, dspinput.stream - 1, dspinput.timeslot,
                 trunkoutput.stream, trunkoutput.stream + 1 , trunkoutput.timeslot );

        swiSendPattern( mx->swihd, &pattern, &cnfoutput, 1 );

        Comment( "Old member timeslot %d:%d in pattern mode 0x%x.",
                 cnfoutput.stream, cnfoutput.timeslot, pattern );
    }
}

/*----------------------------------------------------------------------------
                                EnableMember

    enable MEMBER_ATTR_TALKER_ENABLE
----------------------------------------------------------------------------*/
void EnableMember( CHANNEL_CONTEXT *cx )
{
    MYCONTEXT *mx = cx->mycontext;

    cnfSetMemberAttribute ( mx->resourcehd[ cx->resourceindex ],
                            cx->memberid,
                            MEMBER_ATTR_TALKER_ENABLE,
                            TRUE );

    Comment( "Talker enable. Id = 0x%08X", cx->memberid );
}

/*----------------------------------------------------------------------------
                                PlayBeep

    Play a Beep inside the conference
----------------------------------------------------------------------------*/
void PlayBeep( CHANNEL_CONTEXT *cx , INT32 repeat_count )
{
    MYCONTEXT *mx = cx->mycontext;
    CNF_TONE_PARMS parms = {0};

    ctaGetParms( cx->ctahd, CNF_TONE_PARMID, &parms, sizeof(parms) );

    parms.ontime        /= repeat_count;
    parms.offtime       = 50;
    parms.iterations    = repeat_count;

    cnfStartTone ( mx->resourcehd[ cx->resourceindex ],
                   mx->conferenceid[ cx->resourceindex ],
                   &parms );

    Comment( "Play %d Beep on channel %d", repeat_count, cx - mx->firstchannel);
}


/*----------------------------------------------------------------------------

                                PlayMessage

 Aynchronously play message with the queue of conferencing and call control
 the event VCEEVN_PLAY_DONE is received in main waitevent.
----------------------------------------------------------------------------*/
int PlayMessage(
    CHANNEL_CONTEXT *cx,            /* channel context                      */
    char            *filename,      /* name of voice file                   */
    unsigned         message,       /* message number to play               */
    unsigned         encoding,      /* the encoding scheme of the file      */
    VCE_PLAY_PARMS  *parmsp )       /* optional parms for adiStartPlaying   */
{
    unsigned    count = 1;

    if ( vceOpenFile( cx->ctahd, filename, VCE_FILETYPE_AUTO, VCE_PLAY_ONLY,
                      encoding, &cx->vh ) != SUCCESS )
        return FAILURE;

    if ( vcePlayList( cx->vh, &message, count, parmsp ) != SUCCESS )
    {
        vceClose( cx->vh );
        return FAILURE;
    }
    return SUCCESS;
}

/*----------------------------------------------------------------------------
                                RecordMessage

 ASynchronously record a voice file with the queue of conferencing and call
 control the event VCEEVN_RECORD_DONE is received in main waitevent
----------------------------------------------------------------------------*/
int RecordMessage(
    CHANNEL_CONTEXT  *cx,           /* channel context                      */
    char             *filename,     /* name of voice file                   */
    unsigned          message,      /* message number to record             */
    unsigned          encoding,     /* the encoding scheme of the file      */
    VCE_RECORD_PARMS *parmsp )      /* optional parms for adiStartRecording */
{
    if ( ( vceOpenFile( cx->ctahd, filename, VCE_FILETYPE_VOX, VCE_PLAY_RECORD,
                        encoding, &cx->vh ) != SUCCESS )
     && ( vceCreateFile( cx->ctahd, filename, VCE_FILETYPE_VOX,
                         encoding, NULL, &cx->vh ) != SUCCESS ) )
    {
        return FAILURE;
    }

    if ( vceRecordMessage( cx->vh, message, 20 * 60 * 1000, parmsp ) != SUCCESS )
    {
        vceClose( cx->vh );
        return FAILURE;
    }

    return SUCCESS;
}

/*----------------------------------------------------------------------------
                                Shutdown

  close all resources
----------------------------------------------------------------------------*/
void Shutdown( MYCONTEXT *mx , int Ret )
{
    CTA_EVENT event;
    CHANNEL_CONTEXT *cx;
    unsigned i;

    Comment( "Destroying all CTA Contexts ..." );

    if ((cx = mx->firstchannel) != NULL)
    {
        /* for all ports */
        for ( i = 0; i < (mx->channelnb + mx->virtualnb); i++ )
        {
            /* to restore the link between line and DSP if the connection is aborted */
            if ( cx->callhd != (NCC_CALLHD)0 )
            {
                Comment( "Closing switching context ..." );
                SwitchRestore( cx );

                /* this will close services automatically */
                ctaDestroyContext( cx->ctahd );

                WaitForSpecificEvent( cx->queuehd, CTAEVN_DESTROY_CONTEXT_DONE, &event );
                if( event.value != CTA_REASON_FINISHED )
                {
                    DemoShowError( "Destroying the CT Access context failed", event.value );
                    exit( 1 );
                }
            }
            cx++;
        }
        free( mx->firstchannel );
    }

    /* conference and switching */
    if ( mx->ctahd != NULL_CTAHD )
    {
        ctaDestroyContext( mx->ctahd ); /* this will close services automatically */
        WaitForSpecificEvent( mx->queuehd, CTAEVN_DESTROY_CONTEXT_DONE, &event );
        if( event.value != CTA_REASON_FINISHED )
        {
            DemoShowError( "Destroying the CT Access context failed", event.value );
            exit( 1 );
        }
    }

    if (mx->ctahd != NULL_CTAQUEUEHD)
    {
        Comment( "Destroying the CT Access queue..." );
        ctaDestroyQueue( mx->queuehd );
    }

    Comment( "cnfjoin exit" );
    exit( Ret );
}


/*----------------------------------------------------------------------------
                            Boardinfo
    to known DSP stream number and to set default parameters
----------------------------------------------------------------------------*/
void BoardInfo( CHANNEL_CONTEXT *cx )
{
    MYCONTEXT *mx = cx->mycontext;
    //ADI_BOARD_INFO boardinfo;      // Keep data in application context
    ADI_CONTEXT_INFO contextinfo;

    adiGetBoardInfo( cx->ctahd, mx->board, sizeof( mx->boardinfo ), &(mx->boardinfo) );

    if ( strlen(mx->protocol) == 0 )
    {
        // Pick a default protocol
        switch( mx->boardinfo.trunktype )
        {
        case ADI_TRUNKTYPE_T1:
        case ADI_TRUNKTYPE_E1:
        case ADI_TRUNKTYPE_BRI:
            strcpy( mx->protocol, "isd0" );
            break;
        case ADI_TRUNKTYPE_LOOPSTART:
            strcpy( mx->protocol, "lps0" );
            break;
        default:
            strcpy( mx->protocol, "nocc");
        }
    }
    // Set the number of trunks
    mx->numtrunks = mx->boardinfo.numtrunks;

    // Set the default MVIP95 stream
    adiGetContextInfo( cx->ctahd, &contextinfo, sizeof( contextinfo ));
    mx->dspstream = cx->dspstream = contextinfo.stream95;

    adiGetBoardSlots32( cx->ctahd, mx->board, 0, 0, NULL ,&mx->maxchannelnb);

    if (mx->channelnb == DEFAULT || ((!mx->h100) && mx->channelnb > mx->maxchannelnb))
        mx->channelnb = mx->maxchannelnb;

    //
    //  The switch model pools all the DSP ports together one one stream set.
    //  For digital trunking boards, trunk ports are broken out separately by
    //  trunk (each trunk has its own stream set).  The number of channels per
    //  trunk depends on trunk type and signaling mode:
    //
    //                                     E1                    T1
    //                                    ----                  ----
    //     PRI (ISDN with D channel)       30                    23
    //     RAW (ISDN, no D channel)        31                    24
    //     CAS (mfc0, wnk0, etc)           30                    24
    //
    //  This demonstration guesses at trunk port count by examining protocol, but
    //  this does not cover all possible cases.  For example, it does not account
    //  for E1 trunks in RAW mode.
    //

    switch( mx->boardinfo.trunktype )
    {
    case ADI_TRUNKTYPE_T1:
        if (!stricmp(mx->protocol, "isd0"))
        {
            mx->dspstreamoffset = 23;
        }
        else if (!stricmp(mx->protocol, "nocc"))
        {
            mx->dspstreamoffset = 23;
        }
        else if (!stricmp(mx->protocol, "wnk0"))
        {
            mx->dspstreamoffset = 24;
        }
        else
        {
            printf("Warning: unknown protocol.  Assuming 24 channels per trunk.\n");
            mx->dspstreamoffset = 24;
        }
        break;

    case ADI_TRUNKTYPE_E1:
        if (!stricmp(mx->protocol, "isd0"))
        {
            mx->dspstreamoffset = 30;
        }
        else if (!stricmp(mx->protocol, "nocc"))
        {
            mx->dspstreamoffset = 30;
        }
        else if (!stricmp(mx->protocol, "mfc0"))
        {
            mx->dspstreamoffset = 30;
        }
        else
        {
            printf("Warning: unknown protocol.  Assuming 30 channels per trunk.\n");
            mx->dspstreamoffset = 30;
        }
        break;

    default:
        mx->dspstreamoffset = mx->maxchannelnb;
        printf("Warning: Channels per trunk assumed to be %d.\n", mx->dspstreamoffset);
    }

    printf( "\tProtocol       = %s\n",    mx->protocol );
    printf( "\tChannel count  = %d\n",    mx->channelnb );
    printf( "\tTrunk number   = %d\n",    mx->numtrunks );
    printf( "\tDsp stream     = %d\n",    cx->dspstream );

}

/*----------------------------------------------------------------------------
                                Report comment
 Generate diagnostic outputs according to the current report level.
----------------------------------------------------------------------------*/
void Comment( char *Format, ... )
{
    char Comment[ 128 ];
    va_list Arg;
    va_start( Arg, Format );
    vsprintf( Comment, Format, Arg );
    DemoReportComment( Comment );
}
 

/*----------------------------------------------------------------------------
                          WaitForSpecificEvent
 Generate diagnostic outputs according to the current report level.
----------------------------------------------------------------------------*/
void WaitForSpecificEvent( CTAQUEUEHD ctaqueuehd, DWORD desired_event,
                           CTA_EVENT *eventp )
{
    DWORD ctaret;

    for(;;)
    {
        ctaret = ctaWaitEvent( ctaqueuehd, eventp, CTA_WAIT_FOREVER);

        if (ctaret != SUCCESS)
        {
            printf( "\007ctaWaitEvent returned %x\n", ctaret);
            exit( 1 );
        }

        if( DemoShouldReport( DEMO_REPORT_ALL ) )
            DemoShowEvent( eventp );

        if( (eventp->id == desired_event) )
            break;

        DemoReportUnexpectedEvent( eventp );
    }

    return;
}



/*------------------------- End Of File [ cnfjoin.c ] ----------------------*/






