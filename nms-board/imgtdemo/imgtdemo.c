/****************************************************************************
*  FILE: imgtdemo.c
*
* Copyright (c) 1999-2001  NMS Communication.   All rights reserved.
*
*****************************************************************************/

#include "imgtdemo.h"

/* $$Revision "%v" */
#define VERSION_NUM  "1"
#define VERSION_DATE __DATE__


/*----------------- Global Variables --------------------------------------*/

CTAQUEUEHD  ctaqueuehd = NULL_CTAQUEUEHD;   /* Natural Access queue handle      */
CTAHD       ctahd = NULL_CTAHD;             /* Natural Access context handle    */

unsigned board = 0;
unsigned nai = 0;


unsigned imgt_mask = IMGT_SERVICE_MASK |     /* Service mask */
                     IMGT_RESTART_MASK | 
                     IMGT_D_CHANNEL_STATUS_MASK;

unsigned mon_mask  = IMGT_REPORT_MASK;       /* Monitor mask */

int cx_d_channel = -1;         /* Number of the context carrying D-channel */

unsigned channel_number = 0;
unsigned trunk_type = 0;

unsigned nfas_group = 0;

unsigned nfas_flag   = 0;
unsigned nfas_config = 0;           /* the NFAS configuration
                                       (first argument in NFAS array) */

unsigned d_channel_status = IMGT_D_CHANNEL_STATUS_AWAITING;

unsigned NumberOfNFASTrunks;

MYCONTEXT NFAS[][2] =              /* if NFAS is set */
    {
      {
        { 0, 0, 0, 0, 1},         /* group, board, trunk, nai, D-Ch flag */
        { 0, 0, 1, 1, 0}
      },
      {
        { 1, 1, 0, 0, 1},         /* group, board, trunk, nai, D-Ch flag */
        { 1, 1, 1, 1, 0}
      }
    };

MYCONTEXT cx_array[MX_NFAS_NAI];      /* Number of trunks per group from 1 to 20 */

/*--------------------------------------------------------------------------*
 *  CommandLineHelp()
 *  print the command line help
 *--------------------------------------------------------------------------*/
void CommandLineHelp( void )
{
    printf( "=============================================================\n");
    printf( "Usage: imgtdemo [opts], where opts are\n" );
    printf( "-------------------------------------------------------------\n");
    printf( "-a <nai>               Network Access Indentifier. Default=0\n");
    printf( "-b <board number>      Board number. Default=0\n");
    printf( "-g <nfas_group>        Nfas Group number. Default=0\n");
    printf( "-n <nfas_config#>      Enable internal NFAS configuration with\n"
            "                       duplicate NAI values assigned. Default=NONE\n"
            "                       (Must match the board configuration file)\n");
    printf( "-h or -?               Display this help screen\n");
    printf( "=============================================================\n");
}

/*--------------------------------------------------------------------------*
 *  PrintHellp()
 *  print the runtime help
 *--------------------------------------------------------------------------*/

void PrintHelp( void )
{
    printf( "\n\n=========================================================\n" );
    printf( "si             Start IMGT service\n" );
    printf( "sm             Set event masks\n" );
    printf( "   The current masks are : \n" );
    printf( "       Maintenance message's mask = 0x%08x\n", imgt_mask );
    printf( "       Monitor message's mask     = 0x%08x\n", mon_mask );
    printf( "ss             Stop IMGT service\n" );
    printf( "is <low channel> <high channel> [<nai>]\n"
            "               Put B-channels on <nai> in service.\n"
            "               Specify <nai> for Nfas only.\n" );
    printf( "tis <nai>      Put the whole interface in service\n"
            "               Specify <nai> for Nfas only.\n" );
    printf( "oos <low channel> <high channel> [<nai>]\n"
            "               Put B-channels on <nai> out of service.\n"
            "               Specify <nai> for Nfas only.\n" );
    printf( "toos <nai>     Put the whole interface out of service\n"
            "               Specify <nai> for Nfas only.\n" );
    printf( "sr <low channel> <high channel> [<nai>]\n"
            "               Request for B-channel's status on <nai>.\n"
            "               Specify <nai> for Nfas only.\n" );
    printf( "sh             Show the current status of B-channels\n" );
    printf( "dsr            Request for D-channel status.\n" );
    printf( "dsh            Show the current status of D-channel\n" );
    printf( "dis		    Put the D-Channel in service\n" );
    printf( "doos		    Put the D-Channel out of service\n" );
    printf( "help, h or ?   Display this help screen\n" );
    printf( "quit or q      Finish the program\n" );
    printf( "\n=================================================\n\n" );
}

/*--------------------------------------------------------------------------*
 *  main()
 *--------------------------------------------------------------------------*/
int main( int argc, char **argv )
{
    ParseCommandLine( argc, argv );

    /* initialize the services */
    InitServices();

    /* initialize the context(s) */
    InitContext();

    if( cx_d_channel == -1 )
    {
        printf( "ERROR missing the trunk which's bearing the D channel\n" );
        exit( 1 );
    }

    /* use the OS-dependent function DemoSpawnThread to launch
       the RunDemo thread */
    if ( DemoSpawnThread( RunDemo, &cx_array[cx_d_channel] ) == FAILURE )
    {
        printf( "ERROR during _beginthread... exiting\n" );
        exit ( 1 );
    }


    /* use the OS-dependent function DemoSpawnThread to launch
       the UserInput thread */
    if ( DemoSpawnThread( UserInput, NULL ) == FAILURE )
    {
        printf("ERROR during _beginthread... exiting\n");
        exit (1);
    }

    /* do nothing on this thread */
    while( 1 )
    {
        DemoSleep(10000);
    }
}

/*--------------------------------------------------------------------------*
 *  ParseCommandLine()
 *  read the user command line input
 *--------------------------------------------------------------------------*/
void ParseCommandLine( int argc, char **argv )
{
    int c;
    unsigned i;

    while( (c = getopt( argc, argv, "b:a:g:n:?hH")) != -1 )
    {
        switch( c )
        {
        case 'b':
            sscanf( optarg, "%d", &board );
            break;
        case 'a':
            sscanf( optarg, "%d", &nai );
            break;
        case 'g':
            sscanf( optarg, "%u", &nfas_group );
            break;
        case 'n':
            nfas_flag = 1;
            sscanf( optarg, "%d", &nfas_config );
            i = sizeof ( NFAS ) / sizeof( NFAS[0] );
            if ( nfas_config >= i )
            {
                printf( "Invalid nfas configuration %d\n", nfas_config );
                printf( "Enabled nfas configuration(s) from 0-%d\n", i-1 );
                exit( 1 );
            }
            break;
        case '?':
        case 'h':
        case 'H':
            CommandLineHelp();
            exit( 1 );
        default:
            printf( "Invalid option %s.  Use -h for help.\n", optarg );
            exit( 1 );
        }
    }
}

/*--------------------------------------------------------------------------*
 *  UserInput()
 *  handle the input from the user
 *--------------------------------------------------------------------------*/
THREAD_RET_TYPE THREAD_CALLING_CONVENTION UserInput ( void * pcx )
{
    char userinput[MAX_INPUT];              /* line entered by the user     */
    COMMAND usercommand;                    /* command                      */

    while(1)
    {

        printf("Enter command\n");

        memset( userinput, 0, MAX_INPUT * sizeof( char ));
        memset( &usercommand, 0, sizeof( COMMAND ));

        gets( userinput );

        GetArgs(userinput, &usercommand);

        /* parse usercommand    */

        if( stricmp(usercommand.arg[0], "si" ) == 0 )
        { /* Start IMGT Service                                     */
            StartImgt( &usercommand );
        }
        else if( stricmp(usercommand.arg[0], "sm" ) == 0 )
        { /* Set the masks to specify the range of events/messages  */
            SetImgtConfiguration( &usercommand );
        }
        else if( stricmp(usercommand.arg[0], "ss" ) == 0 )
        { /* Stop IMGT Service                                      */
            StopImgt( &usercommand );
        }
        else if( stricmp(usercommand.arg[0], "is" ) == 0 )
        { /* Build IMGT_SERVICE_RQ to put B channel In Service      */
            CreateBChannelRq(&usercommand, IMGT_SERVICE_RQ,
                          PREFERENCE_BCHANNEL, IMGT_IN_SERVICE);
        }
        else if( stricmp(usercommand.arg[0], "tis" ) == 0 )
        { /* Build IMGT_SERVICE_RQ to put the whole interface In Service */
            CreateBChannelRq( &usercommand, IMGT_SERVICE_RQ,
                          PREFERENCE_TRUNK, IMGT_IN_SERVICE );
        }
        else if( stricmp(usercommand.arg[0], "oos" ) == 0 )
        { /* Build IMGT_SERVICE_RQ to put B channel Out of Service  */
            CreateBChannelRq(&usercommand, IMGT_SERVICE_RQ,
                          PREFERENCE_BCHANNEL, IMGT_OUT_OF_SERVICE);
        }
        else if( stricmp(usercommand.arg[0], "toos" ) == 0 )
        { /* Build IMGT_SERVICE_RQ to put the whole interface Out of Service  */
            CreateBChannelRq(&usercommand, IMGT_SERVICE_RQ,
                          PREFERENCE_TRUNK, IMGT_OUT_OF_SERVICE);
        }
        else if( stricmp(usercommand.arg[0], "sr" ) == 0 )
        { /* Build IMGT_B_CHANNEL_STATUS_RQ to get B channel status */
            CreateBChannelRq( &usercommand, IMGT_B_CHANNEL_STATUS_RQ, PREFERENCE_BCHANNEL, 0);
        }
        else if( stricmp(usercommand.arg[0], "sh" ) == 0 )
        { /* Show the current status of B channels                  */
            ShowBChannelStatus();
        }
        else if( stricmp(usercommand.arg[0], "dsr" ) == 0 )
        { /* Build IMGT_D_CHANNEL_STATUS_RQ to get D channel status */
            CreateDChannelRq( &usercommand, IMGT_D_CHANNEL_STATUS_RQ);
        }
        else if( stricmp(usercommand.arg[0], "dsh" ) == 0 )
        { /* Show the current status of D-channel                   */
            printf( "On Board %d NAI %d D-channel status - %s \n",
                cx_array[cx_d_channel].board, cx_array[cx_d_channel].nai,
                (d_channel_status == IMGT_D_CHANNEL_STATUS_RELEASED) ? "released" :
                (d_channel_status == IMGT_D_CHANNEL_STATUS_ESTABLISHED) ? 
                    "established" : "awaiting establishment" );
        }
        else if( stricmp(usercommand.arg[0], "dis" ) == 0 )
        { /* Build IMGT_D_CHANNEL_EST_RQ to put D channel in service */
            CreateDChannelRq( &usercommand, IMGT_D_CHANNEL_EST_RQ);
        }
        else if( stricmp(usercommand.arg[0], "doos" ) == 0 )
        { /* Build IMGT_D_CHANNEL_REL_RQ to put D channel out of service */
            CreateDChannelRq( &usercommand, IMGT_D_CHANNEL_REL_RQ);
        }
        else if( stricmp(usercommand.arg[0], "help" ) == 0 ||
                 tolower(usercommand.arg[0][0]) == 'h' ||
                 usercommand.arg[0][0] == '?' )
        {
            PrintHelp();
        }
        else if( stricmp(usercommand.arg[0], "quit" ) == 0 ||
                 tolower(usercommand.arg[0][0]) == 'q' )
        {
            exit( 0 );
        }
    }

    UNREACHED_STATEMENT( THREAD_RETURN; )
}

/*--------------------------------------------------------------------------*
 *  InitServices()
 *  initialize the services
 *--------------------------------------------------------------------------*/
void InitServices( void )
{
    DWORD ret;
    CTA_INIT_PARMS initparms = { 0 };        /* for ctaInitialize           */
    CTA_SERVICE_NAME servicelist[] =         /* for ctaInitialize           */
    { { "ADI",  "ADIMGR" },
      { "IMGT", "ADIMGR" },
    };
    unsigned servicenumber = sizeof(servicelist)/sizeof(servicelist[0]);
    /*
     * Natural Access initialization:
     * Register an error handler to display errors returned from API functions;
     * no need to check return values
     */

    /* Initialize size of init parms structure */
    initparms.size = sizeof(CTA_INIT_PARMS);

    DemoReportComment( "Initializing and opening the Natural Access context..." );

    /* ctaInitialize */
    ret = ctaInitialize( servicelist, servicenumber, &initparms );
    if (ret != SUCCESS)
    {
       printf("\tctaInitialize failure");
       exit( 1 );
    }
}

/*--------------------------------------------------------------------------*
 *  InitContext()
 *  open the contexts
 *--------------------------------------------------------------------------*/
void InitContext( void )
{
    unsigned cx_nb;

    memset( cx_array, 0, MX_NFAS_NAI * sizeof( MYCONTEXT ) );

    if ( nfas_flag )
    {
        NumberOfNFASTrunks =  sizeof( NFAS[0] ) / sizeof( MYCONTEXT );

        for ( cx_nb = 0; cx_nb < NumberOfNFASTrunks; cx_nb++ )
        {
            cx_array[cx_nb] = NFAS[nfas_config][cx_nb];

            printf( "NFAS Element %d\n", cx_nb );
            printf( "\tGroup = %d\n", cx_array[cx_nb].NFAS_group );
            printf( "\tBoard = %d\n", cx_array[cx_nb].board );
            printf( "\tTrunk = %d\n", cx_array[cx_nb].trunk );
            printf( "\tNAI   = %d\n", cx_array[cx_nb].nai );
            if ( cx_array[cx_nb].d_channel == 1 )
            {
                nfas_group = cx_array[cx_nb].NFAS_group;
                cx_d_channel = cx_nb;
                printf( "\tDChannel on this trunk\n" );
            }
            else
            {
                printf( "\tDChannel not on this trunk\n" );
            }
        }
    } /* end of if nfas */
    else /* single D channel */
    {
        NumberOfNFASTrunks = 1;

        cx_array[0].NFAS_group = nfas_group;
        cx_array[0].board = board;
        cx_array[0].trunk = nai;
        cx_array[0].nai = nai;
        cx_array[0].d_channel = 1;
        cx_d_channel = 0;
        printf("\tOpen context on Board %d NAI %d\n",
                cx_array[0].board, cx_array[0].nai);
    }
}

/*--------------------------------------------------------------------------*
 *  RunDemo()
 *  handle the incoming events
 *--------------------------------------------------------------------------*/
THREAD_RET_TYPE THREAD_CALLING_CONVENTION RunDemo( void *pcx )
{
    MYCONTEXT *cx;
    DWORD ret;
    char errortext[50] = "";
    CTA_EVENT event;
    unsigned cx_nb, i;

    CTA_SERVICE_DESC services[] =           /* for ctaOpenServices */
    { { {"ADI",  "ADIMGR"}, { 0 }, { 0 }, { 0 } },
      { {"IMGT", "ADIMGR"}, { 0 }, { 0 }, { 0 } },
    };
    unsigned numservices = sizeof(services)/sizeof(services[0]);

    cx = (MYCONTEXT *) pcx;

    /* open services */
    /* fill in some stuff for services */
    for ( i = 0; i < numservices; i++ )
    {
        services[i].mvipaddr.board = cx[cx_d_channel].board;
        services[i].mvipaddr.mode = 0;
    }

    /* use the function in ctademo to open and initialize the context       */
    DemoOpenPort( (DWORD) cx[cx_d_channel].board, /*use the number of the board as userid*/
                "imgt",
                services,
                numservices,
                &ctaqueuehd,
                &ctahd );

    /* start the IMGT manager on the trunk bearing the D channel */
    if(  cx->d_channel == 1 )
    {
        IMGT_MSG_PACKET *pimgt_pkt;

        StartImgt( NULL );

        //  IMGT returns a message for both start and stop.

        DemoWaitForSpecificEvent( ctahd, IMGTEVN_RCV_MESSAGE, &event );
        ImgtFormatEvent( event.buffer );

        //  pick up pointers...

        pimgt_pkt = (IMGT_MSG_PACKET *) event.buffer;

        if ( pimgt_pkt->message.status == SUCCESS )
        {
            for ( cx_nb = 0; cx_nb < NumberOfNFASTrunks; cx_nb++ )
            {
                /* query the current B-channel status for the current nai */
                QueryBChannelStatus( &cx_array[cx_nb], PREFERENCE_TRUNK, 0 );
            }

            /* get the status of the D-channel for the current nai */
            CreateDChannelRq( NULL, IMGT_D_CHANNEL_STATUS_RQ);
        }
    }

    while( 1 )
    {
        /* wait 100 ms */
        ret = ctaWaitEvent( ctaqueuehd, &event,100 );

        if (ret != SUCCESS)
        {
          printf("ctaWaitEvent failure 0x%x\n", ret);
          /* if there is an event and we fail to get it, exit */
          exit( 1 );
        }

        switch( event.id )
        {
            case CTAEVN_WAIT_TIMEOUT:
                break;

            case IMGTEVN_SEND_MESSAGE:
                /* come up if the message failed */
                DemoShowEvent( &event );
                break;

            case IMGTEVN_RCV_MESSAGE:
                /* process the incoming buffer */
                ImgtFormatEvent( event.buffer );
                break;

            default:
                DemoShowEvent( &event );
                break;
        }
    }

  UNREACHED_STATEMENT( THREAD_RETURN; )
}

/*--------------------------------------------------------------------------*
 *  StartImgt()
 *  start IMGT service
 *--------------------------------------------------------------------------*/
void StartImgt( COMMAND *comm )
{
    DWORD ret;
    IMGT_CONFIG config;
    char errortext[50] = "";

    memset( &config, 0, sizeof(IMGT_CONFIG));
    config.imgt_mask = imgt_mask;
    config.mon_mask = mon_mask;

    config.nfas_group = nfas_group;

    printf( "\tThe event masks are: \n" );
    printf( "\t\tMaintenance message's mask 0x%08x\n", imgt_mask );
    printf( "\t\tMonitor message's mask     0x%08x\n", mon_mask );

    ret = imgtStart( ctahd, cx_array[cx_d_channel].nai, &config );

    if (ret != SUCCESS)
    {
        ctaGetText( ctahd, ret, (char *) errortext, 40);
        printf( "imgtStart failure: %s\n",errortext );
    }
}

/*--------------------------------------------------------------------------*
 *  SetImgtConfiguration()
 *  set the range of messages/incoming events
 *--------------------------------------------------------------------------*/
void SetImgtConfiguration( COMMAND *comm )
{
    char userinput[MAX_INPUT];              /* line entered by the user     */

    memset( userinput, 0, MAX_INPUT );
    printf( "The current masks are : \n" );
    printf( "   Maintenance message's mask = 0x%08x\n", imgt_mask );
    printf( "   Monitor message's mask     = 0x%08x\n", mon_mask );
    printf( "Enter new maintenance mask \n" );
    printf( "   IMGT_SERVICE_MASK           0x%08x\n", IMGT_SERVICE_MASK );
    printf( "   IMGT_RESTART_MASK           0x%08x\n", IMGT_RESTART_MASK );
    printf( "   IMGT_D_CHANNEL_STATUS_MASK  0x%08x\n", IMGT_D_CHANNEL_STATUS_MASK );
    gets( userinput );
    sscanf( userinput,"%x", &imgt_mask );
    memset( userinput, 0, MAX_INPUT );

    printf( "Enter new monitor mask \n" );
    printf( "   IMGT_REPORT_MASK            0x%08x\n", IMGT_REPORT_MASK );
    gets( userinput );
    sscanf( userinput,"%x", &mon_mask );
    memset( userinput, 0, MAX_INPUT );

    printf( "The new masks are : \n" );
    printf( "   Maintenance message's mask = 0x%08x\n", imgt_mask );
    printf( "   Monitor message's mask     = 0x%08x\n", mon_mask );
}

/*--------------------------------------------------------------------------*
 *  StopImgt()
 *  stop IMGT service
 *--------------------------------------------------------------------------*/
void StopImgt( COMMAND *comm )
{
    DWORD ret;
    char errortext[50] = "";

    ret = imgtStop( ctahd );
    if (ret != SUCCESS)
    {
        ctaGetText( ctahd, ret, (char *) errortext, 40);
        printf( "imgtStop failure: %s\n",errortext );
    }

}

/*--------------------------------------------------------------------------*
 *  QueryBChannelStatus()
 *  query the status of B channels
 *--------------------------------------------------------------------------*/
void QueryBChannelStatus( MYCONTEXT *cx, unsigned type, unsigned channel )
{
    DWORD ret;
    CTA_EVENT event;
    ADI_BOARD_INFO  boardinfo;

    IMGT_MSG_PACKET *pimgt_pkt;
    IMGT_MESSAGE imgt_msg, *pimgt_msg;
    struct imgt_service imgt_svc, *pimgt_svc;
    char errortext[50] = "";
    unsigned i, size, request_confirmed, cx_nb;

    if( !channel_number )
    {
        /* Determine the board type and number of trunks */
        ret = adiGetBoardInfo(ctahd, cx->board, sizeof boardinfo, &boardinfo);
        if ( ret != SUCCESS )
        {
            ctaGetText( ctahd, ret, (char *) errortext,40);
            printf("adiGetBoardInfo failure: %s\n",errortext);
            exit( 1 );
        }
        switch( boardinfo.trunktype )
        {
            case ADI_TRUNKTYPE_T1:
                trunk_type = ADI_TRUNKTYPE_T1;
                channel_number = 24;
                break;
            case ADI_TRUNKTYPE_E1:
                trunk_type = ADI_TRUNKTYPE_E1;
                channel_number = 30;
                break;
            default:
                printf("\nTrunktype invalid for this demo");
                exit( 1 );
                break;
        }
    }

    memset( &imgt_msg, 0, sizeof( IMGT_MESSAGE ));
    memset( &imgt_svc, 0, sizeof(struct imgt_service));

    for( i = ((type==PREFERENCE_TRUNK)?1:channel);
         i <= ((type==PREFERENCE_TRUNK)?channel_number:channel); i++ )
    {
        imgt_msg.nai = cx->nai;
        imgt_msg.code = IMGT_B_CHANNEL_STATUS_RQ;
        imgt_msg.nfas_group = nfas_group;
        imgt_svc.nai = cx->nai;
        imgt_svc.type = PREFERENCE_BCHANNEL;
        size = sizeof( struct imgt_service );
        /* get status for every channel in the current trunk (nai) */
        imgt_svc.BChannel = i;
        request_confirmed = 0;
        ret = imgtSendMessage( ctahd, &imgt_msg, size, (char *)&imgt_svc );
        if( ret != SUCCESS )
        {
            ctaGetText( ctahd, ret, (char *) errortext, 40);
            printf( "imgtSendMessage failure: %s\n",errortext );
            return;
        }

        /* don't print the incoming expected event: IMGTEVN_RCV_MESSAGE */
        DemoSetReportLevel(DEMO_REPORT_COMMENTS);
        while( !request_confirmed ) /* Wait for all requests to be confirmed */
        {
            /* receive the event with attached buffer */
            DemoWaitForSpecificEvent( ctahd, IMGTEVN_RCV_MESSAGE, &event );
            if( event.buffer != NULL )
            {
                pimgt_pkt= (IMGT_MSG_PACKET *) event.buffer;
                pimgt_msg= (IMGT_MESSAGE *) &pimgt_pkt->message;

                if ( pimgt_msg->code != IMGT_B_CHANNEL_STATUS_CO ) 
                    ImgtFormatEvent( event.buffer );
                else
                {
                    DWORD RR;

                    pimgt_svc= (struct imgt_service *) pimgt_pkt->databuff;

                    for ( cx_nb = 0; cx_nb < NumberOfNFASTrunks; cx_nb++ )
                        if ( cx_array[cx_nb].nai == pimgt_svc->nai )
                            break;

                    if ( cx_nb >= NumberOfNFASTrunks )
                    {
                        printf( "Received Invalid NAI\n" );
                        break;
                    }

                    request_confirmed = 1;
                    cx_array[cx_nb].channel_status[pimgt_svc->BChannel]=pimgt_svc->status;
                    /* release the attached buffer */
                    RR = imgtReleaseBuffer( ctahd, event.buffer );
                    if (RR != SUCCESS)
                        printf("imgtReleaseBuffer failure %lx\n", RR);
                }
            }
        }
    }

    if( channel_number == 24 )
        for( i = 25; i <= 30; i++ )
            cx->channel_status[i] = 0;

    return;
}

/*--------- Build Imgt Primitivies -----------------------------------------*/
/*--------------------------------------------------------------------------*
 *  CreateBChannelRq()
 *  Create buffer with B-channel service or status request
 *--------------------------------------------------------------------------*/
void CreateBChannelRq( COMMAND *comm, char code, unsigned type, unsigned status )
{
    struct imgt_service imgt_svc;
    unsigned i, chnl_number = 0;
    int low_chnl = 1, high_chnl = 0;

    memset( &imgt_svc, 0, sizeof(struct imgt_service) );

    // fill the buffer
    /* set NAI equal to Nai carrying D-channel */
    imgt_svc.nai = cx_array[cx_d_channel].nai;
    if ( nfas_flag )
    {
        if ( type == PREFERENCE_BCHANNEL )
        {
            if (comm != NULL && comm->arg[3][0] != 0 && comm->arg[3][0] != '#')
                sscanf( comm->arg[3], "%d", &imgt_svc.nai );
        }
        else if ( type == PREFERENCE_TRUNK )
        {
            if (comm != NULL && comm->arg[1][0] != 0 && comm->arg[1][0] != '#')
                sscanf( comm->arg[1], "%d", &imgt_svc.nai );
        }
        for ( i = 0; i < NumberOfNFASTrunks; i++ )
            if ( imgt_svc.nai == cx_array[i].nai )
                break;
        if ( i == NumberOfNFASTrunks )
        {
            printf( "Invalid NAI. This message will not be sent\n" );
            return;
        }
    }

    if ( code != IMGT_B_CHANNEL_STATUS_RQ )
    {
        imgt_svc.type    = type;
        imgt_svc.status  = status;  // set the Status
    }

    sscanf( comm->arg[1], "%d", &low_chnl );
    sscanf( comm->arg[2], "%d", &high_chnl );

    imgt_svc.BChannel = low_chnl;
    chnl_number = (high_chnl >= low_chnl) ? high_chnl-low_chnl : 0;
    // send message for each B channel, start from the low channel
    printf( "Send %s request ",
            ( code == IMGT_B_CHANNEL_STATUS_RQ ) ? "status" : "service" );
    if ( !chnl_number )
        printf( "for channel %d on NAI %d\n", imgt_svc.BChannel, imgt_svc.nai );
    else if ( chnl_number == 1 )
        printf( "for channels %d and %d on NAI %d\n", low_chnl, high_chnl, imgt_svc.nai );
    else
        printf( "for channels from %d to %d on NAI %d\n", low_chnl, high_chnl, imgt_svc.nai );

    for( i = 0; i <= chnl_number; i++ )
    {
        SendImgtMessage( code, cx_array[cx_d_channel].nai, (char *)&imgt_svc );
        imgt_svc.BChannel++;
    }
}

/*--------------------------------------------------------------------------*
 *  CreateDChannelRq()
 *  Create buffer with D-channel status request
 *--------------------------------------------------------------------------*/
void CreateDChannelRq( COMMAND *comm, char code )
{
    struct imgt_d_channel_status imgt_dchl_status;
    struct imgt_d_channel_control imgt_dchl_ctrl;
    unsigned int nai;

    if (( code != IMGT_D_CHANNEL_EST_RQ ) && ( code != IMGT_D_CHANNEL_REL_RQ ))
	{
	    memset( &imgt_dchl_status, 0, sizeof(struct imgt_d_channel_status) );

	    /* set NAI equal to Nai carrying D-channel */
		nai = cx_array[cx_d_channel].nai;

	    SendImgtMessage( code, nai, (char *)&imgt_dchl_status );
	}
	else {
	    memset( &imgt_dchl_ctrl, 0, sizeof(struct imgt_d_channel_control) );

	    /* set NAI equal to Nai carrying D-channel */
		nai = cx_array[cx_d_channel].nai;

	    SendImgtMessage( code, nai, (char *)&imgt_dchl_ctrl );
	}
}

/*--------------------------------------------------------------------------*
 *  SendImgtMessage()
 *--------------------------------------------------------------------------*/
void SendImgtMessage( char code, unsigned nai, char *buffer )
{
    DWORD ret;
    IMGT_MESSAGE imgt_msg;
    char errortext[50] = "";
    unsigned size;

    memset( &imgt_msg, 0, sizeof(IMGT_MESSAGE));

    imgt_msg.nai = nai;
    imgt_msg.code = code;
    imgt_msg.nfas_group = nfas_group;
    size = sizeof( struct imgt_service );
    ret = imgtSendMessage( ctahd, &imgt_msg, size, buffer );
    if (ret != SUCCESS)
    {
        ctaGetText( ctahd, ret, (char *) errortext, 40);
        printf( "imgtSendMessage failure: %s\n",errortext );
    }
}

/*---------------------------------------------------------------------------*
 *  ImgtFormatEvent
 *  format the incoming buffer
 *---------------------------------------------------------------------------*/
void ImgtFormatEvent( unsigned char *buffer )
{
    IMGT_MSG_PACKET *pimgt_pkt;
    IMGT_MESSAGE *pimgt_msg;
    unsigned cx_nb, i;
    DWORD RR;
    char errortext[40] = "";

    pimgt_pkt = (IMGT_MSG_PACKET *) buffer;
    pimgt_msg = (IMGT_MESSAGE *) &pimgt_pkt->message;

    switch ( pimgt_msg->code )
    {
        case IMGT_STARTED_CO:
            ctaGetText( ctahd, pimgt_msg->status, (char *) errortext, 40);
            printf("\n\tIMGT_STARTED_CO:  status = %s (0x%lx)\n", errortext, pimgt_msg->status);
            break;

        case IMGT_STOPPED_CO:
            ctaGetText( ctahd, pimgt_msg->status, (char *) errortext, 40);
            printf("\n\tIMGT_STOPPED_CO:  status = %s (0x%lx)\n", errortext, pimgt_msg->status);
            break;

        case IMGT_REPORT_IN:
        {
            struct imgt_report_hdr *pimgt_rpt;
            printf( "\nEvent: IMGT_REPORT_INDICATION (Code: %c)\n",
                    IMGT_REPORT_IN );
            pimgt_rpt = (struct imgt_report_hdr *) pimgt_pkt->databuff;
            switch ( pimgt_rpt->OperationID )
            {
                case IMGT_OP_ID_SET_CALL_TAG:
                    {
                        struct imgt_report_CallTag *pimgt_CallTag;
                        pimgt_CallTag =
                            (struct imgt_report_CallTag *) pimgt_pkt->databuff;
                        printf( "Operation  = Set Call Tag\n" );
                        printf( "Slot       = %d\n", pimgt_CallTag->Slot );
                        printf( "Call Tag   = %x\n", pimgt_CallTag->CallTag );
                    break;
                    }
                case IMGT_OP_ID_TRFD_CALL_CLEARING:
                    {
                        struct imgt_report_CallTag *pimgt_CallTag;
                        pimgt_CallTag =
                            (struct imgt_report_CallTag *) pimgt_pkt->databuff;
                        printf( "Operation  = Transferred Call Clearing\n" );
                        printf( "Call Tag   = %x\n", pimgt_CallTag->CallTag );
                    break;
                    }

                default:
                    printf( "Unknown operation %c\n", pimgt_rpt->OperationID );
                    break;
            }
        }
        break;
        case IMGT_RESTART_IN:
        {
            struct imgt_restart *pimgt_rst;

            printf( "\nEvent: IMGT_RESTART_INDICATION (Code: %c)\n",
                    IMGT_RESTART_IN );

            pimgt_rst = (struct imgt_restart *) pimgt_pkt->databuff;
            printf( "Type       = %s\n",
                  ( pimgt_rst->type == PREFERENCE_TRUNK ) ?  "Trunk" :
                  ( pimgt_rst->type == PREFERENCE_BCHANNEL ) ?
                    "Single B Channel" : "Unknown" );
            printf( "NAI        = %d\n", pimgt_rst->nai );
            if ( pimgt_rst->type == PREFERENCE_BCHANNEL )
                printf( "B Channel  = %d\n", pimgt_rst->BChannel );
            /* For some variants (DMS) the RESTART message resets
               the B channel status to "in service".  Every time
               when restart comes up check the status of the channel */
            for ( cx_nb = 0; cx_nb < NumberOfNFASTrunks; cx_nb++ )
                if ( cx_array[cx_nb].nai == pimgt_rst->nai )
                    break;
            if ( cx_nb < NumberOfNFASTrunks )
            {
                if ( pimgt_rst->type == PREFERENCE_BCHANNEL )
                    QueryBChannelStatus( &cx_array[cx_nb], PREFERENCE_BCHANNEL,
                                        pimgt_rst->BChannel );
                else
                    QueryBChannelStatus( &cx_array[cx_nb], PREFERENCE_TRUNK, 0 );
            }
            else
            {
                printf( "Received Invalid NAI\n" );
            }
        }
        break;
        case IMGT_SERVICE_IN:
        case IMGT_SERVICE_CO:
        case IMGT_B_CHANNEL_STATUS_CO:
        {
            struct imgt_service *pimgt_svc;

            printf( "\nEvent: %s (Code: %c)\n",
            (pimgt_msg->code == IMGT_SERVICE_IN) ? "IMGT_SERVICE_INDICATION" :
            (pimgt_msg->code == IMGT_SERVICE_CO) ? "IMGT_SERVICE_CONFIRMATION" :
            "IMGT_B_CHANNEL_STATUS_CONFIRMATION", pimgt_msg->code );

            pimgt_svc = (struct imgt_service *) pimgt_pkt->databuff;
            printf( "Type       = %s\n",
                  ( pimgt_svc->type == PREFERENCE_TRUNK ) ?  "Trunk" :
                  ( pimgt_svc->type == PREFERENCE_BCHANNEL ) ?
                    "Single B Channel" : "Unknown" );
            printf( "NAI        = %d\n", pimgt_svc->nai );
            if( pimgt_svc->type == PREFERENCE_BCHANNEL )
                printf( "B Channel  = %d\n", pimgt_svc->BChannel );
            printf( "Status     = %s\n",
                  ( pimgt_svc->status == IMGT_IN_SERVICE ) ?  "In Service" :
                  ( pimgt_svc->status == IMGT_MAINTENANCE ) ?  "Maintenance" :
                  ( pimgt_svc->status == IMGT_OUT_OF_SERVICE ) ?
                    "Out Of Service" : "Unknown" );

            for ( cx_nb = 0; cx_nb < NumberOfNFASTrunks; cx_nb++ )
                if ( cx_array[cx_nb].nai == pimgt_svc->nai )
                    break;
            if ( cx_nb < NumberOfNFASTrunks )
            {
                if( pimgt_svc->type == PREFERENCE_TRUNK )
                {
                    for ( i = 1; i <= channel_number; i++ )
                        cx_array[cx_nb].channel_status[i] = pimgt_svc->status;
                }
                else
                    cx_array[cx_nb].channel_status[pimgt_svc->BChannel] = pimgt_svc->status;
            }
            else
            {
                printf( "Received Invalid Nai\n" );
            }
        }
        break;

        case IMGT_D_CHANNEL_STATUS_IN:
        case IMGT_D_CHANNEL_STATUS_CO:
        {
            struct imgt_d_channel_status *pimgt_dchl_status;

            printf( "\nEvent: %s (Code: %c)\n",
            (pimgt_msg->code == IMGT_D_CHANNEL_STATUS_IN) ? "IMGT_D_CHANNEL_STATUS_IN" :
            "IMGT_D_CHANNEL_STATUS_CONFIRMATION", pimgt_msg->code );

            pimgt_dchl_status = (struct imgt_d_channel_status *) pimgt_pkt->databuff;

            d_channel_status = pimgt_dchl_status->status;

            printf( "On Board %d NAI %d D-channel status - %s \n",
                cx_array[cx_d_channel].board, pimgt_msg->nai,
                (d_channel_status == IMGT_D_CHANNEL_STATUS_RELEASED) ? "released" :
                (d_channel_status == IMGT_D_CHANNEL_STATUS_ESTABLISHED) ? 
                    "established" : "awaiting establishment" );
        }
        break;

		case IMGT_D_CHANNEL_EST_CO:
		{
            struct imgt_d_channel_control *pimgt_dchl_control;
			unsigned op_status;

            printf( "\nEvent: %s (Code: %c)\n", "IMGT_D_CHANNEL_EST_CO", pimgt_msg->code );

            pimgt_dchl_control = (struct imgt_d_channel_control *) pimgt_pkt->databuff;

            op_status = pimgt_dchl_control->result;

            printf( "On Board %d NAI %d D-channel Establish Request Status - %d \n",
                cx_array[cx_d_channel].board, pimgt_msg->nai, op_status);
		}
		break;

		case IMGT_D_CHANNEL_REL_CO:
		{
            struct imgt_d_channel_control *pimgt_dchl_control;
			unsigned op_status;

            printf( "\nEvent: %s (Code: %c)\n", "IMGT_D_CHANNEL_REL_CO", pimgt_msg->code );

            pimgt_dchl_control = (struct imgt_d_channel_control *) pimgt_pkt->databuff;

            op_status = pimgt_dchl_control->result;

            printf( "On Board %d NAI %d D-channel Release Request Status - %d \n",
                cx_array[cx_d_channel].board, pimgt_msg->nai, op_status);
		}
		break;

        default:
            printf( "\nEvent: Unknown (Code: %c)\n", pimgt_msg->code );
    }

    /* release the attached buffer */
    RR = imgtReleaseBuffer( ctahd, buffer );
    if (RR != SUCCESS)
        printf("imgtReleaseBuffer failure %lx\n", RR);
}

/*--------------------------------------------------------------------------*
 *  ShowBChannelStatus()
 *  show the current status of B-channels
 *--------------------------------------------------------------------------*/
void ShowBChannelStatus( void )
{
    unsigned cx_nb, i;

    for ( cx_nb = 0; cx_nb < NumberOfNFASTrunks; cx_nb++ )
    {
        printf( "Show status on Board %d NAI %d\n",
                 cx_array[cx_nb].board, cx_array[cx_nb].nai );
        for( i = 1; i <= channel_number; i++ )
        {
            if( i % channel_number == 1 )
                printf( "Channels:  " );
            if( (i - 1) % 10 == 0 )
                printf( " " );
            printf( " %d", i % 10 );
        }
        for( i = 1; i<= channel_number; i++ )
        {
            if( i % channel_number == 1 )
                printf( "\nStatus:    " );
            if( (i - 1) % 10 == 0 )
                printf( " " );
            if( cx_array[cx_nb].d_channel == 1 &&
                ( i == 24 && trunk_type == ADI_TRUNKTYPE_T1 ||
                  i == 16 && trunk_type == ADI_TRUNKTYPE_E1 ))
                    printf( " D" );
            else
                printf( " %d", cx_array[cx_nb].channel_status[i] );
        }
        printf( "\n" );
    }
}

/*--------------------------------------------------------------------------*
 *  GetArgs()
 *  extracts arguments from the user command
 *--------------------------------------------------------------------------*/
void GetArgs( char * input, COMMAND *comm)
{
  char sep[6];
  char *token;
  int i = 0;

/*    separator characters */
  sep[0] = ' ';
  sep[1] = '\t';
  sep[2] = '\n';
  sep[3] = '\0';

  memset(comm,0,sizeof(COMMAND));

  token = strtok( input, sep );
  while( token != NULL && i < MAX_ARGS)
  {
     strcpy(comm->arg[i], token );
     i++;
     comm->numargs = i;
     /* Get next token: */
     token = strtok( NULL, sep );
  }

  return;
}


