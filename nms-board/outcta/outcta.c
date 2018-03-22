/*****************************************************************************
 *  File -  outcta.c
 *
 *  Description - Simple outbound demo using Natural Access and the ADI service.
 *         Single process per port programming model.
 *
 *  Voice Files Used -
 *         CTADEMO.VOX  - all the prompts
 *         TEMP.VOX     - scratch file for record and playback
 *
 *
 * Copyright (c) 1999-2002 NMS Communications.  All rights reserved.
 *****************************************************************************/
#define VERSION_NUM  "5"
#define VERSION_DATE __DATE__

/*----------------------------- Includes -----------------------------------*/
#include "ctademo.h"


#include "nccdef.h"
#include "nccxadi.h"
#include "nccxisdn.h"
#include "nccxcas.h"


/*----------------------------- Application Definitions --------------------*/
typedef struct
{
    CTAQUEUEHD  ctaqueuehd;     /* Natural Access queue handle      */
    CTAHD       ctahd;          /* Natural Access context handle    */
    char        dialstring[20]; /* current number being dialed      */
    char        callingaddr[20];/* current calling party number     */
    unsigned    maxredial;      /* maximum retries on that number   */
    unsigned    lowlevelflag;   /* flag to get low-level events     */
    NCC_CALLHD  callhd;         /* call handle                      */
} MYCONTEXT;

#define DEFAULT_ADDRESS "123"
#define DEFAULT_CALLING ""

#if defined (UNIX)
#define TEMP_FILE "/tmp/temp.vox"
#else
#define TEMP_FILE "temp.vox"
#endif


/*----------------------------- Application Data ---------------------------*/
static MYCONTEXT MyContext =
{
    NULL_CTAQUEUEHD, /* Natural Access queue handle     */
    NULL_CTAHD,      /* Natural Access context handle   */
    DEFAULT_ADDRESS, /* dialstring      */
    DEFAULT_CALLING, /* current calling party number    */
    3,               /* maxredial       */
    0,               /* lowlevelflag    */
    NULL_NCC_CALLHD
};

unsigned   ag_board    = 0;
unsigned   mvip_stream = 0;
unsigned   mvip_slot   = 0;
char      *protocol    = "lps0";
unsigned nccInUse = 1;

/*----------------------------- Forward References -------------------------*/
void RunDemo( MYCONTEXT *cx );


/*----------------------------------------------------------------------------
  PrintHlp()
  ----------------------------------------------------------------------------*/
void PrintHelp( MYCONTEXT *cx )
{
    printf( "Usage: outcta [opts]\n" );
    printf( "  where opts are:\n" );
    printf( "  -A adi_mgr      Natural Access ADI manager to use.             "
            " Default = %s\n", DemoGetADIManager() );
    printf( "  -b n            OAM board number.                   "
            " Default = %d\n", ag_board );
    printf( "  -s [strm:]slot  DSP [stream and] timeslot.          "
            " Default = %d:%d\n", mvip_stream, mvip_slot );
    printf( "  -p protocol     Protocol to run.                    "
            " Default = %s\n", protocol );
    printf( "  -d digits       Number to dial.                     "
            " Default = %s\n", cx->dialstring );
    printf( "  -a digits       Calling Party Number.               "
            " Default = %s\n", cx->callingaddr );
    printf( "  -F filename     Natural Access Configuration File name.  "
            " Default = None\n" );
    printf( "  -r retries      Maximum dial retries on failure.  \n" );
    printf( "  -l              Print low-level call control events.\n" );
    printf( "  -c              Run with Legacy ADI Call Control.   "
            " Default = NCC Call Control\n");
}


/*----------------------------------------------------------------------------
  main
  ----------------------------------------------------------------------------*/
int main( int argc, char **argv )
{
    int         c;
    MYCONTEXT *cx = &MyContext;

    printf( "Natural Access Outbound Demo V.%s (%s)\n", VERSION_NUM, VERSION_DATE );

    while( ( c = getopt( argc, argv, "A:F:s:b:p:d:a:r:clHh?" ) ) != -1 )
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
                        mvip_slot      = value2;
                        break;
                    case 1:
                        mvip_slot      = value1;
                        break;
                    default:
                        PrintHelp( cx );
                        exit( 1 );
                }
                break;
            case 'b':
                sscanf( optarg, "%d", &ag_board );
                break;
            case 'p':
                protocol = optarg;
                break;
            case 'd':
                strncpy( cx->dialstring, optarg, 20 );
                break;
            case 'a':
                strncpy( cx->callingaddr, optarg, 20 );
                break;
            case 'r':
                sscanf( optarg, "%d", &cx->maxredial );
                break;
            case 'l':
                cx->lowlevelflag = 1;
                break;
            case 'c':
                nccInUse = 0;
                break;
            case 'h':
            case 'H':
            case '?':
            default:
                PrintHelp( cx );
                exit( 1 );
                break;
        }
    }
    if (optind < argc)
    {
        printf( "Invalid option.    Use -h for help.\n" );
        exit( 1 );
    }

    printf( "\tProtocol    = %s\n",     protocol );
    printf( "\tBoard    = %d\n",        ag_board );
    printf( "\tStream:Slot= %d:%d\n",   mvip_stream, mvip_slot );
    printf( "\tCalled Number= %s\n",    cx->dialstring );
    printf( "\tCalling Number= %s\n",   cx->callingaddr );
    printf( "\tMax Redial = %d\n",      cx->maxredial );

    RunDemo( cx );
    return 0;
}


/*-------------------------------------------------------------------------
  RunDemo
  -------------------------------------------------------------------------*/
void RunDemo( MYCONTEXT *cx )
{
    NCC_START_PARMS     nccparms = {0};
    NCC_ADI_START_PARMS nccadiparms = {0};
    NCC_ADI_ISDN_PARMS  isdparms = {0};
    ADI_START_PARMS     adiparms = {0};
    unsigned ret;
    unsigned i;
    char       option[2];  /* get one digit +1 for null-terminator */

    /* Register an error handler so we won't have to check return values
     * of ADI functions.
     */
    ctaSetErrorHandler( DemoErrorHandler, NULL );

    /* Initialize Natural Access, open a call resource, open default services */
    if (nccInUse == 1)
        DemoNCCSetup( ag_board, mvip_stream, mvip_slot, &cx->ctaqueuehd, 
                      &cx->ctahd, "outcta-demo" );
    else
        DemoSetup( ag_board, mvip_stream, mvip_slot, &cx->ctaqueuehd, 
                   &cx->ctahd );


    /* Initiate a protocol on the call resource so we can accept calls. */
    if (nccInUse == 1)
    {
        ctaGetParms( cx->ctahd,    /* get call resource parameters    */
                     NCC_START_PARMID,
                     &nccparms,
                     sizeof(nccparms) );
        ctaGetParms( cx->ctahd,    /* get call resource parameters    */
                     NCC_ADI_START_PARMID,
                     &nccadiparms,
                     sizeof(nccadiparms) );
        if (strncmp(protocol, "isd", 3) == 0)
        {
            ctaGetParms( cx->ctahd,    /* get call resource parameters  */
                         NCC_ADI_ISDN_PARMID,
                         &isdparms,
                         sizeof(isdparms) );
        }
        if ( cx->lowlevelflag )
        {
            nccparms.eventmask = 0xfff;
        }
    }
    else
    {
        ctaGetParms( cx->ctahd,    /* get call resource parameters    */
                     ADI_START_PARMID,
                     &adiparms,
                     sizeof(adiparms) );
        if ( cx->lowlevelflag )
        {
            adiparms.callctl.eventmask = 0xffff;    /* show low-level evs */
        }
    }

    if (nccInUse == 1)
    {
        if (strncmp(protocol, "isd", 3) == 0)
        {
            DemoStartNCCProtocol( cx->ctahd, 
                                  protocol, 
                                  &isdparms.isdn_start, 
                                  &nccadiparms, 
                                  &nccparms );
        }
        else
        {
            DemoStartNCCProtocol( cx->ctahd, 
                                  protocol, 
                                  NULL, 
                                  &nccadiparms, 
                                  &nccparms );
        }
    }    
    else
    {
        DemoStartProtocol( cx->ctahd, protocol, NULL, &adiparms);
    }

    /* Place a call (try upto 'maxredial' attempts).
     * Give user opportunity to play and record.
     */
    for( i = 0; i < cx->maxredial; i++ )
    {

        if (nccInUse == 1)
            ret = DemoPlaceNCCCall( cx->ctahd, cx->dialstring, 
                                    cx->callingaddr, NULL, NULL, &cx->callhd );
        else
            ret = DemoPlaceCall(cx->ctahd, cx->dialstring, NULL);

        if( ret == SUCCESS )
            break;
        else if( ret == DISCONNECT )
            if (nccInUse == 1)
                DemoHangNCCUp(cx->ctahd, cx->callhd, NULL, NULL);
            else
                DemoHangUp( cx->ctahd );
    }

    if ( ret == SUCCESS )
    {
        /* Play an introduction message to the callee.
         */
        adiFlushDigitQueue( cx->ctahd );

        ret = DemoPlayMessage( cx->ctahd, "ctademo", PROMPT_INTRODUC,
                               0, NULL );
        if( ret != SUCCESS )
            goto hangup;

        for(;;)
        {
            ret = DemoPromptDigit( cx->ctahd, "ctademo", PROMPT_OPTION,
                                   0, NULL, option, 2 );
            if ( ret != SUCCESS )
                break;

            switch( *option )
            {
                VCE_RECORD_PARMS rparms;

                case '1':
                /* Record called party, overriding default parameters for
                 * quick recognition of silence once voice input has begun.
                 */
                ctaGetParms( cx->ctahd, VCE_RECORD_PARMID,
                             &rparms, sizeof(rparms) );

                rparms.silencetime = 500; /* 1/2 sec */

                ret = DemoRecordMessage( cx->ctahd, TEMP_FILE, 0,
                                         ADI_ENCODE_NMS_24, &rparms );
                if( ret == DISCONNECT )
                    goto hangup;
                break;

                case '2':
                /* Play back anything the called party recorded.
                 * This might be something recorded last time
                 * the demo was run, or the file might not exist.
                 */
                ret = DemoPlayMessage( cx->ctahd, TEMP_FILE, 0,
                                       ADI_ENCODE_NMS_24, NULL );
                if( ret == FAILURE )
                    ret = DemoPlayMessage( cx->ctahd, "ctademo",
                                           PROMPT_NOTHING, 0, NULL );

                if( ret == DISCONNECT )
                    goto hangup;
                break;

                case '3':
                ret = SUCCESS;
                goto hangup;

                default:
                printf( "\t Invalid selection: '%c'\n", *option );
                break;
            }
        }
hangup:
        if (nccInUse == 1)
            DemoHangNCCUp(cx->ctahd, cx->callhd, NULL, NULL);
        else
            DemoHangUp( cx->ctahd );
    }

    printf( "\tTerminating...\n" );
    DemoShutdown( cx->ctahd );
    return;
}
