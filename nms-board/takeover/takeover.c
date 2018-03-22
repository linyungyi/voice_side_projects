/*************************************************************************
 *  FILE: takeover.c
 *
 *  DESCRIPTION:  This program illustrates an application's ability to 
 *                provide robust behavior when operating in the Multi-
 *                Server Natural Access development environment. This
 *                is achieved by using the shared context and shared object
 *                handle capabilities provided within this development 
 *                environment
 *
 * Copyright (c) 2001-2002 NMS Communications.  All rights reserved.
 *************************************************************************/
#define VERSION_NUM  "1"
#define VERSION_DATE __DATE__
#include <ctademo.h>
#include <oamdef.h>
#include <dtmdef.h>
#include <nccdef.h>
#include <nccxadi.h>
#include <nccxisdn.h>
#include <nccxcas.h>
#include <swidef.h>
#include <clkdef.h>
#include <hswdef.h>

#include "takeover.h"

#define OK( f ) printf("%s -- SUCCESSFUL\n", f)
#define NO_MEMORY() {printf("FATAL Error - Unable to malloc space\n");exit( 1 );}

/******************************************************************************
* Process GLOBAL Data
******************************************************************************/
/* Process startup arguments */
CALL_ARGS      gArgs = {0};
     
/* Global process state information (see takeover.h) */
TAKEOVER_STATE gState = {   TOVR_MODE_BACKUP, 0, 
                            FALSE, FALSE, FALSE, 
                            BRD_UNKNOWN, 
                            NULL_CTAHD,
                            0, (DTM_TRUNK *) NULL, 
                            0, 0, (OAM_DATA *) NULL,
                            {0}
                        };

/* Inter-thread communications array. Each thread has an entry in the array. */
TAKECMD_ENTRY  gCommands[ NUM_TAKEOVER_THRDS ] = {0};

/* general use buffer used in events */
char gbuffer[ MAX_BUFFER_SIZE ];

/**************************************
*  Global Natural Access Information. *
**************************************/

/* These SETUP_IDX_xxx_MGR defines are index values into gsvcMgrList[]
*  indicative of which service manager is associated with the particular 
*  service. For example, the DTM service uses the ADIMGR.
*/
#define SETUP_IDX_OAM_MGR 0
#define SETUP_IDX_ADI_MGR 1
#define SETUP_IDX_NCC_MGR 1
#define SETUP_IDX_DTM_MGR 1
#define SETUP_IDX_SWI_MGR 2
#define SETUP_ALL_MGRS 3

/* These SETUP_IDX_xxx defines are index values into gsvcNames[]
*  for the service text string identifiers.
*/
#define SETUP_IDX_OAM 0
#define SETUP_IDX_ADI 1
#define SETUP_IDX_NCC 2
#define SETUP_IDX_DTM 3
#define SETUP_IDX_SWI 4

char             *gsvcMgrList[]      = { "OAMMGR", "ADIMGR", "SWIMGR" };
char             *gsvcNames[]        = { "OAM", "ADI", "NCC", "DTM", "SWI" };                        

/* Global process initialization with Natural Access, i.e., used in call to
*  ctaInitialize.
*/
CTA_SERVICE_NAME  gsvcList[]         = {{ "OAM", "OAMMGR" },
                                       { "ADI", "ADIMGR" },
                                       { "NCC", "ADIMGR" },
                                       { "DTM", "ADIMGR" },
                                       { "SWI", "SWIMGR" }};
CTA_SERVICE_DESC  gsvcDescriptions[] = {{ { "OAM", "OAMMGR" }, {0},{0},{0} },
                                       { { "ADI", "ADIMGR" }, {0},{0},{0} },
                                       { { "NCC", "ADIMGR" }, {0},{0},{0} },
                                       { { "DTM", "ADIMGR" }, {0},{0},{0} },
                                       { { "SWI", "SWIMGR" }, {0},{0},{0} } };
unsigned          gNumSvcs = 5;

/******************************************************************************
* Thread Data Array.
*
*   Each thread is represented by a structure (CTA_THRD_INFO) containing all 
*   Natural Access specific data, e.g., the context handle, the queue handle,
*   etc.
******************************************************************************/
#define CCV_THRD_IDX  0
#define OAM_THRD_IDX  1
#define DTM_THRD_IDX  2
#define CHK_THRD_IDX  3
#define CMD_THRD_IDX  4

CTA_THRD_INFO ctaThrdData[ NUM_TAKEOVER_THRDS ] = {0};

/*************************
*  FUNCTION Declarations *
*************************/
BOOL serverAvailable( char *host );

/* Support functions and data within DTM thread */
void        myDtmAdd( unsigned brd, unsigned trunk, DTMHD dtmhd );
void        myDtmRemove( DTMHD dtmhd );
DTMHD       myDtmFindHandle( unsigned brd, unsigned trunk );
DTM_TRUNK  *myDtmFindTrunk( DTMHD dtmhd );
void        myDtmShowStatus( DTM_TRUNK_STATUS *status );
void        myDtmShowState( DWORD board, DWORD trunk, DTM_TRUNK_STATE *state );
void        myDtmStopBoard( unsigned brd );
void        myDtmIdle();
void        myDtmResetFromPrimary( CTA_EVENT *pevent, CTA_THRD_INFO *pctaInfo );
DWORD       myDtmStateData( char *pbuf, DWORD bufSize );
void        myDtmShow();
/* Support functions and data within OAM thread */
DWORD       myOamGetKW( CTA_THRD_INFO *pctaInfo, char *pName, DWORD brd, char *pKW, 
                 char *value, int size ); 
HMOBJ       myOamOpenObject ( char *pboardName, DWORD boardNumber, 
                     CTA_THRD_INFO *pctaInfo );
DWORD       myOamAdd( OAM_CMD_KW *poamReq, OAM_DATA *pfromPeer );
void        myOamRemove( OAM_DATA *poamData );
void        showOamData( CTA_THRD_INFO *pctaInfo );
DWORD       myOamStateData( char *pbuf, DWORD bufSize );
void        myOamResetFromPrimary( CTA_EVENT *pevent, CTA_THRD_INFO *pctaInfo );
void        myOamShow();
void        myOamSendToPeer( DWORD entryID, DWORD cmd );
OAM_DATA   *myOamFindEntry( DWORD entryID );
DWORD       myOamBrdNum( CTA_THRD_INFO *pctaInfo, int *pBrdNum, 
                       OAM_MSG *pOamMsg );
OAM_DATA   *myOamFind( char *keyWord, char *objectName );
/* Support functions and data within CCV thread */
BOOL        boardBooted( CTA_THRD_INFO *pctaInfo );
void        ccvSendToPeer( CCV_DATA *pccv, DWORD cmd );
void        ccvOpenServices( CTA_THRD_INFO *pctaInfo, CCV_DATA *pccv );
DWORD       ccvStateData( char *pbuf, DWORD bufSize );
void        ccvResetFromPrimary( CTA_EVENT *pevent, CTA_THRD_INFO *pctaInfo );
void        ccvShow();

/******************************************************************
* Conversion of #defines into text strings for display purposes.  *
******************************************************************/

#define _TEXTCASE_(e) case e: return #e

char *ccvDisplayThrdIdx( DWORD e )
{
    switch( e )
    {
        _TEXTCASE_(CCV_THRD_IDX); 
        _TEXTCASE_(OAM_THRD_IDX);
        _TEXTCASE_(DTM_THRD_IDX);
        _TEXTCASE_(CHK_THRD_IDX);
        _TEXTCASE_(CMD_THRD_IDX);
        default: return NULL;
    }
}

char *ccvDisplayState( DWORD e )
{
    switch( e )
    {
        _TEXTCASE_(CCV_CONNECTED); 
        _TEXTCASE_(CCV_PLAYING); 
        _TEXTCASE_(CCV_IDLE);
        default: return NULL;
    }
}


char *ccvDisplayBrdState( DWORD e )
{
    switch( e )
    {
        _TEXTCASE_(BRD_IDLE); 
        _TEXTCASE_(BRD_BOOTED);
        _TEXTCASE_(BRD_UNKNOWN);
        _TEXTCASE_(BRD_ERROR);
        default: return NULL;
    }
}

/******************************************************************************
*  OS Independent functions for dealing with critical sections of code
******************************************************************************/

#if defined (UNIX)

  static  MUTEX_T  init_mutex = { 0 };

  void EnterCritSection( void )
  {
      MUTEX_LOCK( &init_mutex );
  }

  void ExitCritSection( void )
  {
      MUTEX_UNLOCK( &init_mutex );
  }


#elif defined (WIN32)

  typedef  void*         MTXHDL;      /* Mutex Sem Handle */

  static BOOL             CriticalSectionInitialized = FALSE;
  static CRITICAL_SECTION CritSection = { 0 };

  void EnterCritSection ( void )
  {
      if ( CriticalSectionInitialized == FALSE )
      {
          InitializeCriticalSection( &CritSection );
          CriticalSectionInitialized = TRUE;
      }

      EnterCriticalSection (&CritSection);
  }

  void ExitCritSection ( void )
  {
     LeaveCriticalSection ( &CritSection );
  }

#endif

/******************************************************************************
* getMyPid
*
*    This function returns the process identifier for this application.
******************************************************************************/
int getMyPid()
{
#ifdef WIN32
    return _getpid ();
#else
    return getpid ();
#endif
}
/******************************************************************************
* myCompare
*
*   Perform a case insenstive compare. This assumes the "pValue" string is
*   in UPPER case.
******************************************************************************/
int myCompare( char *pKWval, char *pValue )
{
    char Ukw[ MAX_KW_VALUE_SIZE ];

    strcpy( Ukw, pKWval );

    /* Convert KW value to UPPER case */
#if defined (WIN32)
    strupr( Ukw );
#else
    {
    char *pUkw = Ukw;

    for( ; *pUkw; pUkw++ )
        *pUkw = (char) toupper( *pUkw );
    }
#endif
    return ( !strcmp( Ukw, pValue ) );
}

/******************************************************************************
* PrintHelp
*******************************************************************************/
void PrintHelp( void )
{
    printf( "Natural Access Multi-Server Object Sharing Demo (TAKEOVER) V.%s (%s)\n",
            VERSION_NUM, VERSION_DATE );

    printf( "\nUsage: takeover [opts]\n" );
    printf( "  where opts are:\n\n" );
    printf( "  -@ name      Server host name where services are running     \n"
            "                 Default = localhost\n");
    printf( "  -c name      Server host name of where the command and       \n"
            "               checkpoint contexts are created                 \n"
            "                 Default = localhost\n");
    printf( "  -b #         OAM board #                                     \n"
            "                 Default = %d\n", gArgs.board );
    printf( "  -p protocol  Call Control protocol to run.                   \n"
            "                 Default = %s\n", gArgs.protocol );
    
    /* Switching setup */
    printf( "  -d name      Board's switching driver name.                   \n"
            "                 Default = %s\n",  gArgs.swiDriver );
    printf( "  -s [n:]m     Caller Line/DSP [stream and] timeslot.           \n"
            "                 Default = 0:0\n");
    printf( "  -r [n:]m     Called DSP resource [stream and] timeslot.       \n"
            "                 Default = 16:0 or 4:0 -- board dependent\n");
    printf( "  -1           Initial runtime mode is primary                  \n"
            "                 Default = %s\n", (gArgs.bPrimary ? "PRIMARY" : "BACKUP" ) );
    printf( "  -f name      Voice file used for play                         \n"
            "                 Default = %s\n", gArgs.voiceFile );
    printf( "  -e #         Voice file encoding, e.g., ADI_ENCODE_NMS_32 is 3\n"
            "               See adidef.h for other encodings.                \n"
            "                 Default = %d\n", gArgs.encoding );
}
/******************************************************************************
* ProcessArgs
*
*   Process command line options.
*******************************************************************************/
void ProcessArgs( int argc, char **argv )
{
    int         c;
    unsigned   tmp1, tmp2;
    
    /* Setup defaults */
    strcpy( gArgs.protocol, "wnk0" );
    /* Default play file. */
    strcpy( gArgs.voiceFile, "american.vox" );
    gArgs.encoding = ADI_ENCODE_NMS_32;   
    /* Indicators of which stream and timesots have been provided at startup. */
    gArgs.inStream = gArgs.inSlot = gArgs.dspStream = gArgs.dspSlot = -1;  
    strcpy( gArgs.swiDriver, "agsw" );      

    while( ( c = getopt( argc, argv, "h?v1@:c:f:b:p:e:r:d:s:" ) ) != -1 )
    {
        switch( c )
        {
            case '@':   
                /* server host name where services are running */
                if (strlen(optarg) >= MAX_SERVER_SIZE)
                {
                    printf( "-%c argument is too long.\n", c );
                    exit( -1 );
                }
                strcpy( gArgs.svrHost, optarg );
                break;

            case 'c':   
                /* server host name where command app (takecmd) is running  */
                if (strlen(optarg) >= MAX_SERVER_SIZE)
                {
                    printf( "-%c argument is too long.\n", c );
                    exit( -1 );
                }
                strcpy( gArgs.cmdHost, optarg );
                break;

            case 'h':
            case '?':
                PrintHelp();
                exit( 0 );
                break;

            case 'p':
                sscanf( optarg, "%s", gArgs.protocol );
                break;

            case 'b':
                sscanf( optarg, "%d", &gArgs.board );
                break;

            case 'd': /* switching driver */
                if ( strlen( optarg ) >= MAX_DRIVER_NAME )
                {
                    printf( "-%c argument is too long.\n", c );
                    exit( -1 );
                }
                strcpy( gArgs.swiDriver, optarg );
                break;

            case 's':
                /* The stream and timeslot of the caller, e.g., 0:4 */
                switch (sscanf( optarg, "%d:%d", &tmp1, &tmp2 ))
                {
                    case 2:
                        gArgs.inStream = tmp1;
                        gArgs.inSlot   = tmp2;
                        break;
                    case 1:
                        gArgs.inSlot = tmp1;
                        break;
                    default:
                        PrintHelp();
                        exit( 1 );
                }
                break;

            case 'r':
                /* The  stream and timeslot of the DSP resource, e.g., 4:4 */
                /* Default is the base DSP resource stream, e.g., 16 */
                switch (sscanf( optarg, "%d:%d", &tmp1, &tmp2 ))
                {
                    case 2:
                        gArgs.dspStream = tmp1;
                        gArgs.dspSlot   = tmp2;
                        break;
                    case 1:
                        gArgs.dspSlot = tmp1;
                        break;
                    default:
                        PrintHelp();
                        exit( 1 );
                }
                break;
          
            case '1':
                gArgs.bPrimary = TRUE;
                break;

            case 'v':
                gArgs.bVerbose = TRUE;
                break;

            case 'f':
                /* default file to play */
                sscanf( optarg, "%s", gArgs.voiceFile );
                break;

            case 'e': 
                /* encoding */
                sscanf( optarg, "%d", &gArgs.encoding );
                break;

            default:
                PrintHelp();
                exit( 1 );
                break;
        }
    }
    if (optind < argc)
    {
        printf( "Invalid option.  Use -? for help.\n" );
        exit( 1 );
    }
    /* Adjust default input/DSP streams and timeslots base upon passed in 
    *  parameters. The DSP stream cannot be determined until after the
    *  ADI service has been opened.
    */
    if ( gArgs.inStream == -1 )
        /* Use line interface */
        gArgs.inStream = 0;
 
    if ( gArgs.inSlot == -1 )
        gArgs.inSlot = 0;
    if ( gArgs.dspSlot == -1 )
        gArgs.dspSlot =  gArgs.inSlot;

}
/******************************************************************************
* closeCtaSvs
*
*    This function closes NA services on the context.
******************************************************************************/
void closeCtaSvs( CTA_THRD_INFO *pctaInfo )
{
    DWORD     ret = SUCCESS;
    CTA_EVENT event;

    if ( ( ret = ctaCloseServices( pctaInfo->ctahd, pctaInfo->svcNames, 
          pctaInfo->numSvcs ) ) 
          != SUCCESS )
    {
        printf("Natural Access Close Service failed; returned 0x%x\n", ret );
    }
    if ( (ret = ctaWaitEvent( pctaInfo->ctaqueuehd, &event, CTA_WAIT_FOREVER)) 
         != SUCCESS )
    {
        printf("%s Natural Access Close Service wait event returned 0x%x\n", 
                    pctaInfo->ctxName, ret );
    }
    else if ( ( event.id != CTAEVN_CLOSE_SERVICES_DONE ) ||
                  ( event.value != CTA_REASON_FINISHED ) )
    {
        printf("%s Natural Access Close Service failed.\n", pctaInfo->ctxName);
        DemoShowEvent( &event );
    }

    pctaInfo->bsvcReady = FALSE;
}
/******************************************************************************
* cleanupCtaContext
*
*    This function closes the NA context.
******************************************************************************/
void cleanupCtaContext( CTA_THRD_INFO *pctaInfo )
{
    DWORD     ret = SUCCESS;
    
    pctaInfo->bsvcReady = pctaInfo->bctxReady = FALSE;
    pctaInfo->ctahd = NULL_CTAHD;
    pctaInfo->breset = TRUE;
}
/******************************************************************************
* setupCtaServices
*
*    This function opens NA services for the context if the calling thread
*    was responsible for creating it. If the context was attached to, it is
*    not necessary to open services because it is created in "common access 
*    mode".
******************************************************************************/
DWORD setupCtaServices ( CTA_THRD_INFO *pctaInfo )
{
    DWORD     ret = SUCCESS;
    CTA_EVENT event;

    if ( pctaInfo->battached == FALSE )
    {
        /* This thread is responsible for creating the context. Therefore,
        *  open the services.
        */
        if ( (ret = ctaOpenServices( pctaInfo->ctahd, pctaInfo->svcDescriptions,
                                     pctaInfo->numSvcs )) 
            != SUCCESS )
        {
            printf("%s Unable to open Natural Access services - returns 0x%x\n", 
                    pctaInfo->ctxName, ret);
            if ( ret != CTAERR_SVR_COMM )
                exit( 1 );
        }
        else
        {                                        
            if ( (ret = ctaWaitEvent( pctaInfo->ctaqueuehd, &event, CTA_WAIT_FOREVER)) 
                 != SUCCESS )
            {
                printf("%s Natural Access Open Service wait event returned 0x%x\n", 
                        pctaInfo->ctxName, ret );
                if ( ret != CTAERR_SVR_COMM )
                    exit( 1 );
            }
            else if ( ( event.id != CTAEVN_OPEN_SERVICES_DONE ) ||
                      ( event.value != CTA_REASON_FINISHED ) )
            {
                printf("%s Natural Access Open Service failed.\n", pctaInfo->ctxName);
                DemoShowEvent( &event );
                exit( 1 );
            }
        }
    
        if ( ret != SUCCESS )
        {
            /* Communictaion problems with server. */
            cleanupCtaContext( pctaInfo );
        }
        else
            pctaInfo->bsvcReady = TRUE;
    }
    else
    {
        /* This thread attached to an existing shared context. It needs to
        *  check if the the services have been opened yet.
        */
        CTA_CONTEXT_INFOEX ctxInfo = {0};

        /* Allocate space for the 3 character service names, separated by a
        *  '0', and then terminated by a final '0'.
        */ 
        ctxInfo.size = (((sizeof(char) * 4) * pctaInfo->numSvcs) + 1);
        ctxInfo.svcnames = (char *) malloc(ctxInfo.size);

        /* Check if services have been opened yet by the creator of the 
        *  context.
        */
        ret = ctaGetContextInfoEx( pctaInfo->ctahd, &ctxInfo );
        if ( ret == SUCCESS )
        {
            /* Since all services are opened at once, just check that
            *  at least one service name has been returned, rather than parsing
            *  the whole list for particular names.
            */
            if ( strlen( ctxInfo.svcnames ) == 3 )
            {
                /* Services are opened. */
                pctaInfo->bsvcReady = TRUE;
                if ( gArgs.bVerbose )
                    printf("%15s: Services have been opened\n", pctaInfo->ctxName );
            }
        }
        else
        {
            DemoShowError("Unable to determine if services opened.", ret);
            exit( 1 );
        }
    }
    return ret;
}
/******************************************************************************
* serverAvailable
*
*    This function determines whether the Natural Access server is running on 
*    the specified host.
******************************************************************************/
BOOL serverAvailable( char *host )
{
    BOOL   bavail = FALSE;
    CTAHD  voidHandle;
    DWORD  ret = SUCCESS;
    char   server[ MAX_SERVER_SIZE +1 ];

    if ( *host )
    {
        /* If context name starts with '//', the name is interpreted as a 
        *  server name.
        */
        strcpy( server, "//" );
        strcat( server, host );
        
        /* There are Natural Access functions that are not context specific,
        *  for example, ctaGetVersion and ctaFormatEvent. But, it is often
        *  desirable to run these functions on particular servers in the 
        *  system. In order to designate which server is desired, apps
        *  need a "void context handle" and need to use the xxxxEx versions
        *  of these functions.
        *
        *  To obtain a server's unique void context handle, use NULL_CTAQUEUEHD
        *  as the queue handle and supply a server only context name to 
        *  ctaCreateContext.
        */
        ret = ctaCreateContext( NULL_CTAQUEUEHD, 0, server, &voidHandle );
    }
    else
        /* NULL_CTAHD handle refers to the default server (localhost). */
         voidHandle = NULL_CTAHD;

    if ( ret == SUCCESS )
    {
        CTA_REV_INFO info;

        /* Use the void context handle to see if the server is up and running */
        ret = ctaGetVersionEx( voidHandle, &info, sizeof( CTA_REV_INFO ) );
        if ( ret == SUCCESS )
            bavail = TRUE;            
    }

    return bavail;
}
/******************************************************************************
* setupCtaOnThread
*
*    This function establishes a NA context on the specified server for the
*    calling thread. If services need to be opened, this is also done at this
*    time.
******************************************************************************/
DWORD setupCtaOnThread( CTA_THRD_INFO *pctaInfo, char *host )
{
    BOOL  retry = FALSE;
    DWORD ret = CTAERR_RESERVED;
    char  cntxName[ MAX_DESCRIPTOR_SIZE ] = {0};
    
    pctaInfo->battached = FALSE;

    /* The context name to be used needs to be prepended with the host
    *  name if present, i.e., "hostname/context-name"
    */
    if ( *host )
    {
        strcpy( cntxName, host );
        strcat( cntxName, "/" );
    }
    strcat( cntxName, pctaInfo->ctxName );

    if ( serverAvailable( host ) )
    { 
        if ( pctaInfo->ctaqueuehd == NULL_CTAQUEUEHD )
        {
            if (( ret = ctaCreateQueue(  pctaInfo->svcMgrList, pctaInfo->numSvcs, 
                                        &pctaInfo->ctaqueuehd ))
                  != SUCCESS )
            {
                printf("Context: %s, Unable to create Natural Access queue - returns 0x%x\n", 
                        cntxName, ret);
                exit( 1 );
            }
        }
            

        /* Attempt to connect to common context until successful. 
        *  The other process (takecmd) may not have yet
        *  initialized.
        */
        if ((ret = ctaAttachContext( pctaInfo->ctaqueuehd, 0, cntxName, 
                  &pctaInfo->ctahd )) != SUCCESS ) 
        {
            /* Will get CTAERR_BAD_ARGUMENT when named context does not exist */
            if(    (ret != CTAERR_SVR_COMM) 
                && (ret != CTAERR_BAD_ARGUMENT) )
            {
                printf("Unable to attach to context %s\n", cntxName );
                exit(-1);
            }
        }
        else
        {
            pctaInfo->battached = TRUE;
            printf("Have attached to context %s.\n", cntxName );
        }
        if ( ret == CTAERR_BAD_ARGUMENT )
        {
            /* Can't attach because it doesn't exits. Create the context */
            ret = ctaCreateContext( pctaInfo->ctaqueuehd, 0, cntxName, 
                                    &pctaInfo->ctahd );
    
        }
        if ( ret == CTAERR_BAD_ARGUMENT )
        {
            /* There's a race condition with the other instance of "takeover"
            *  following the starting of the server. Try the attach, again...
            */
            ret = ctaAttachContext(  pctaInfo->ctaqueuehd, 0, cntxName, 
                                    &pctaInfo->ctahd ); 
            if ( ret != SUCCESS )
            {
                printf("Unable to create or attach to context: %s\n", cntxName);
                exit( 1 );
            }
            pctaInfo->battached = TRUE;
        }
    }
    else
    {
        if ( *host )
            printf( "%15s: Verify the NA Server ctdaemon is running on %s.\n",
                    pctaInfo->ctxName, host );
        else
            printf( "%15s: Verify the NA Server ctdaemon is running.\n", 
                    pctaInfo->ctxName );
    }
       
    if ( ret == SUCCESS )
    {
        if ( pctaInfo->breset )
        {
            printf("\n%15s: Have reestablished context.\n", pctaInfo->ctxName );
            pctaInfo->breset = FALSE;
        }
        pctaInfo->bctxReady = TRUE;
    }
    return ret;
}
/*******************************************************************************
 *  sendEvent
 * 
 *      This function queues our application event to the Exchange queue.
 *******************************************************************************/
void sendEvent( CTA_THRD_INFO *pctaInfo, DWORD evtId, DWORD evtValue, void *pbuffer, 
                 DWORD size )
{
    DWORD     ret = SUCCESS;
    CTA_EVENT event = { 0 };

    event.id = evtId;
    event.ctahd = pctaInfo->ctahd;
    event.value = evtValue;
    event.size = size;

    if ( pbuffer )
    {
        event.buffer = pbuffer;
    }

    /* If there have been communication problems with the server, then the 
    *  context will get reset (NULL_CTAHD). The context will need to get
    *  reestablished before any further events can be queued.
    */
    if ( pctaInfo->ctahd != NULL_CTAHD )
    {
        if ( (ret = ctaQueueEvent( &event )) != SUCCESS )
        {
            printf("ctaQueueEvent failed 0x%x\n", ret );
            /* Reset context and service ready flags. */
            cleanupCtaContext( pctaInfo );
        }
    }
}
/*******************************************************************************
*  showEvent
* 
*     This function shows the event if the "verbose" flag has been set.
*******************************************************************************/
void showEvent( CTA_EVENT *pEvent )
{
    const OAM_MSG * const pOamMsg = (OAM_MSG *)pEvent->buffer;
    char  txt[ MAX_TEXT_BUFFER_SIZE ];

    if ( gArgs.bVerbose )
    {
        /* Never show timeouts and heartbeat events. There's too many  */
        if (   (pEvent->id != CTAEVN_WAIT_TIMEOUT)
            && (pEvent->id != APPEVN_XCHG_HEARTBEAT) )
        {
            if ( OAM_IS_OAM_EVENT( pEvent->id ) ) 
            {
                /* OAM events are a superset of many events and need to be
                *  handled specially.
                */
                if ( !pOamMsg )
                    printf("WARNING: Invalid OAM Event received: 0x%x\n",
                            pEvent->id );
                else
                {
                    char *pName = ((char *) pOamMsg) + pOamMsg->dwOfsSzName;

                    if ( ctaGetText( pEvent->ctahd, pOamMsg->dwCode, txt, 
                         MAX_TEXT_BUFFER_SIZE ) == SUCCESS )
                    {
                        printf("%s", txt );
                        if(    CTA_IS_DONE_EVENT( pOamMsg->dwCode )
                            && ( ctaGetText( pEvent->ctahd, pEvent->value, 
                                 txt, MAX_TEXT_BUFFER_SIZE ) == SUCCESS ) )
                            printf( " : %s --", txt );
                        printf(" %s \n", pName );
                    }
                }
            }
            else /* Not and OAM event */
            {
                DemoSetReportLevel( DEMO_REPORT_ALL );
                DemoShowEvent( pEvent );
                DemoSetReportLevel( DEMO_REPORT_COMMENTS );
            }
        }
    }
}
/******************************************************************************
* setCmd
*
*   This function is used by one thread to record a new command event for one 
*   of the other threads to process. (Inter-thread communictaion)
******************************************************************************/
void setCmd( CTA_EVENT *pevent, int threadId )
{
    TAKECMD_ENTRY *pCmd;
    BOOL           done = FALSE;
    int            retry = 0;

    pCmd = &gCommands[ threadId ];
    do
    {
        if ( retry ) DemoSleep( 1 );
        ENTERCRITSECT();
        if ( !pCmd->newCmd )
        {
            /* New command for thread */
            pCmd->newCmd = TRUE;
            memcpy( &pCmd->cmd, pevent, sizeof(pCmd->cmd) );

            /* Copy over contents of buffer into new buffer space. By 
            *  doing this, caller is able to reuse and/or free orignal
            *  event and buffer space.
            */
            if ( pevent->buffer )
            {
                if ( pCmd->cmd.buffer = malloc( pevent->size ) )
                    memcpy( pCmd->cmd.buffer, pevent->buffer, pevent->size );
                else
                    NO_MEMORY();
            }
            done = TRUE;
        }
        EXITCRITSECT();
    } while( !done && (retry++ < 3) );

    if ( !done )
        printf("%s: Unable to process command: 0x%x\n", 
                ccvDisplayThrdIdx( threadId ), pevent->id );
}
/******************************************************************************
* getCmd
*
*   This function is used by the individual threads to get back an event
*   structure filled with a command message from the user (sent from takecmd).
******************************************************************************/
BOOL getCmd( CTA_EVENT *pevent, int id )
{
    TAKECMD_ENTRY *pCmd;
    BOOL           ret = FALSE;


    pCmd = &gCommands[ id ];

    ENTERCRITSECT();
    if ( ret = pCmd->newCmd )
    {
        /* New command for thread */
        pCmd->newCmd = FALSE;
        memcpy( pevent, &pCmd->cmd, sizeof(pCmd->cmd) );
    }
    EXITCRITSECT();

    return ret;
}
/******************************************************************************
* getRunMode --  This function returns the state in which the process is 
*                currently running.
******************************************************************************/
int getRunMode()
{
    int mode;
    ENTERCRITSECT();
    mode = gState.runMode;
    EXITCRITSECT();
    return mode;
}
/******************************************************************************
* setRunMode -- This function sets the state in which the process is 
*               currently running.
******************************************************************************/
void setRunMode( int mode )
{
    ENTERCRITSECT();
    gState.runMode = mode;
    EXITCRITSECT();
}
/******************************************************************************
* setBoardStopped -- This function returns whether or not we've stopped the
*                   board.
******************************************************************************/
void setBoardStopped( BOOL state )
{
    ENTERCRITSECT();
    gState.bboardStopped = state;
    EXITCRITSECT();
}
/******************************************************************************
* boardIsStopped -- This function returns whether or not we've stopped the
*                   board.
******************************************************************************/
BOOL boardIsStopped()
{
    BOOL ret;
    ENTERCRITSECT();
    ret = gState.bboardStopped;
    EXITCRITSECT();
    return ret;
}
/******************************************************************************
* getBoardState -- This function returns the state of the board.
******************************************************************************/
BRD_STATE getBoardState()
{
    BRD_STATE state;
    ENTERCRITSECT();
    state = gState.boardState;
    EXITCRITSECT();
    return state;
}
/******************************************************************************
* setBoardState -- This function records the state of the board.
******************************************************************************/
void setBoardState( BRD_STATE state )
{
    ENTERCRITSECT();
    gState.boardState = state;
    EXITCRITSECT();
}
/******************************************************************************
* getCallHandle -- This function gets the call handle
******************************************************************************/
NCC_CALLHD getCallHandle()
{
    NCC_CALLHD callhd;
    ENTERCRITSECT();
    callhd = gState.voiceInfo.callhd;
    EXITCRITSECT();
    return callhd;
}
/******************************************************************************
* setCallHandle -- This function records the call handle
******************************************************************************/
void setCallHandle( NCC_CALLHD callhd )
{
    ENTERCRITSECT();
    gState.voiceInfo.callhd = callhd;
    EXITCRITSECT();
}
/******************************************************************************
* getSwiHandle -- This function gets the call handle
******************************************************************************/
SWIHD getSwiHandle()
{
    SWIHD swihd;
    ENTERCRITSECT();
    swihd = gState.swihd;
    EXITCRITSECT();
    return swihd;
}
/******************************************************************************
* setSwiHandle -- This function records the call handle
******************************************************************************/
void setSwiHandle( SWIHD swihd )
{
    ENTERCRITSECT();
    gState.swihd = swihd;
    EXITCRITSECT();
}
/******************************************************************************
* getProtStarted -- This function gets protocol start flag.
******************************************************************************/
BOOL getProtStarted()
{
    BOOL state;
    ENTERCRITSECT();
    state = gState.voiceInfo.bprotocolStarted;
    EXITCRITSECT();
    return state;
}
/******************************************************************************
* setProtStarted -- This function records protocol start flag.
******************************************************************************/
void setProtStarted( BOOL state )
{
    ENTERCRITSECT();
    gState.voiceInfo.bprotocolStarted = state;
    EXITCRITSECT();
}
/******************************************************************************
* getCallState -- this function returns the call state
******************************************************************************/
CCV_STATE getCallState()
{
    CCV_STATE state;
    ENTERCRITSECT();
    state = gState.voiceInfo.state;
    EXITCRITSECT();
    return state;
}
/******************************************************************************
* setCallState -- this function sets the call state
******************************************************************************/
void setCallState( CCV_STATE state )
{
    ENTERCRITSECT();
    gState.voiceInfo.state = state;
    EXITCRITSECT();
}
/******************************************************************************
* myWaitEvent
*
*   This function invokes ctaWaitEvent on behalf of the calling thread.
******************************************************************************/
void myWaitEvent( CTA_THRD_INFO *pctaInfo, CTA_EVENT *pevent, 
                  int *pFreeBuf, unsigned tmo )
{
    DWORD ret;
    *pFreeBuf = 0;

    memset( pevent, '0', sizeof(CTA_EVENT) );
    pevent->ctahd = pctaInfo->ctahd;

    if ( (ret = ctaWaitEvent( pctaInfo->ctaqueuehd, pevent, tmo )) != SUCCESS )
    {
        if (   (ret == CTAERR_WAIT_FAILED) 
            || (ret == CTAERR_SVR_COMM) )
        {
            /* Reset context and service ready flags. */
            cleanupCtaContext( pctaInfo );
            pevent->id = CTAEVN_NULL_EVENT;
        }
        else
        {
            printf( "\tctaWaitEvent returned %x -- %s\n", ret, 
                    pctaInfo->ctxName );
            exit( 1 );
        }
    }
    else
    {
        /* In Natural Access Client/Server  mode, certain event buffers are 
         * allocated by Natural Access and must be released back to 
         * Natural Access when we're done using it.
        */
        if( pevent->buffer && (*pFreeBuf = pevent->size & CTA_INTERNAL_BUFFER))
            pevent->size &= ~CTA_INTERNAL_BUFFER;
        showEvent( pevent ); 
    }
}
/******************************************************************************
* showRunState
*
*       This function displays our runtime state information.
******************************************************************************/
void showRunState()
{
    char *modeText;
    
    printf("\n");
    switch( getRunMode() )
    {
        case TOVR_MODE_PRIMARY: modeText = TOVR_MODE_PRIMARY_TEXT; break;
        case TOVR_MODE_BACKUP:  modeText = TOVR_MODE_BACKUP_TEXT;  break;
        default:                modeText = TOVR_MODE_UNKNOWN_TEXT; break;
    }
    printf("Running mode: %s\n", modeText );

    /* OAM */
    myOamShow();
    /* DTM */
    myDtmShow();
    /* CCV */
    ccvShow();

    printf("\n");
}
/******************************************************************************
* mySwiConns
*
*       This function either switches the inputs to the outputs or disables
*       the outputs.
******************************************************************************/
void mySwiConns (SWIHD swihd, DWORD lineIn, DWORD talkSlot,
                              DWORD dsp,    DWORD listenSlot,
                 BOOL  bmakeConnect )
{
    SWI_TERMINUS  doesCall[4]; 
    SWI_TERMINUS  isCalled[4];
    DWORD ret = SUCCESS;

    if ( lineIn != dsp )
    {
        /* Connect line interfaces to DSP resources 
        *  MVIP95_LOCAL_BUS, lineIn,   talkSlot    -- l:0:4 V    
        *  MVIP95_LOCAL_BUS, lineIn+2, talkSlot    -- l:2:4 S
        *  MVIP95_LOCAL_BUS, dsp,      talkSlot    -- l:4:4 V 
        *  MVIP95_LOCAL_BUS, dsp+2,    talkSlot    -- l:6:4 S 
        */
        doesCall[0].bus = MVIP95_LOCAL_BUS;
        doesCall[0].stream = lineIn;
        doesCall[0].timeslot = talkSlot;

        doesCall[1].bus = MVIP95_LOCAL_BUS;
        doesCall[1].stream = lineIn+2;
        doesCall[1].timeslot = talkSlot;

        doesCall[2].bus = MVIP95_LOCAL_BUS;
        doesCall[2].stream = dsp;
        doesCall[2].timeslot = talkSlot;

        doesCall[3].bus = MVIP95_LOCAL_BUS;
        doesCall[3].stream = dsp+2;
        doesCall[3].timeslot = talkSlot;
        
        /* MVIP95_LOCAL_BUS, dsp+1,    listenSlot -- l:5:4 V 
        *  MVIP95_LOCAL_BUS, dsp+3,    listenSlot -- l:7:4 S 
        *  MVIP95_LOCAL_BUS, lineIn+1, listenSlot -- l:1:4 V   
        *  MVIP95_LOCAL_BUS, lineIn+3, listenSlot -- l:3:4 S 
        */
        isCalled[0].bus = MVIP95_LOCAL_BUS;
        isCalled[0].stream = dsp+1;
        isCalled[0].timeslot = listenSlot;

        isCalled[1].bus = MVIP95_LOCAL_BUS;
        isCalled[1].stream = dsp+3;
        isCalled[1].timeslot = listenSlot;

        isCalled[2].bus = MVIP95_LOCAL_BUS;
        isCalled[2].stream = lineIn+1;
        isCalled[2].timeslot = listenSlot;

        isCalled[3].bus = MVIP95_LOCAL_BUS;
        isCalled[3].stream = lineIn+3;
        isCalled[3].timeslot = listenSlot;
    }
    else
    {
        /* Cross-connecting between two DSP resources 
		*  MVIP95_LOCAL_BUS, lineIn,   talkSlot    -- l:4:5 V     
        *  MVIP95_LOCAL_BUS, lineIn+2, talkSlot    -- l:6:5 S 
        *  MVIP95_LOCAL_BUS, dsp,      listenSlot  -- l:4:4 V 
        *  MVIP95_LOCAL_BUS, dsp+2,    listenSlot  -- l:6:4 S 
        */
        doesCall[0].bus = MVIP95_LOCAL_BUS;
        doesCall[0].stream = lineIn;
        doesCall[0].timeslot = talkSlot;

        doesCall[1].bus = MVIP95_LOCAL_BUS;
        doesCall[1].stream = lineIn+2;
        doesCall[1].timeslot = talkSlot;

        doesCall[2].bus = MVIP95_LOCAL_BUS;
        doesCall[2].stream = dsp;
        doesCall[2].timeslot = listenSlot;

        doesCall[3].bus = MVIP95_LOCAL_BUS;
        doesCall[3].stream = dsp+2;
        doesCall[3].timeslot = listenSlot;

        /* MVIP95_LOCAL_BUS, dsp+1,    listenSlot -- l:5:4 V 
        *  MVIP95_LOCAL_BUS, dsp+3,    listenSlot -- l:7:4 S 
        *  MVIP95_LOCAL_BUS, lineIn+1, talkSlot   -- l:5:5 V   
        *  MVIP95_LOCAL_BUS, lineIn+3, talkSlot   -- l:7:5 S 
        */
        isCalled[0].bus = MVIP95_LOCAL_BUS;
        isCalled[0].stream = dsp+1;
        isCalled[0].timeslot = listenSlot;

        isCalled[1].bus = MVIP95_LOCAL_BUS;
        isCalled[1].stream = dsp+3;
        isCalled[1].timeslot = listenSlot;

        isCalled[2].bus = MVIP95_LOCAL_BUS;
        isCalled[2].stream = lineIn+1;
        isCalled[2].timeslot = talkSlot;

        isCalled[3].bus = MVIP95_LOCAL_BUS;
        isCalled[3].stream = lineIn+3;
        isCalled[3].timeslot = talkSlot;
    }		
    
    if ( bmakeConnect )
    {
        /* L{04/05}:04  <-> L{05/04}:05 */
        /* L{06/07}:04  <-> L{07/06}:05 */
        ret = swiMakeConnection( swihd, &doesCall[0], &isCalled[0], 4);
    }
    else
    {
        ret = swiDisableOutput( swihd, &isCalled[0], 4);
    }

    if( ret != SUCCESS )
        printf("CCV: Unable to %s outputs\n", ( bmakeConnect ? "connect to" :
                                                               "disable" ) );
}
/******************************************************************************
* setupSwitches
*
*       This initializes the streams and timeslots used to connect the calling
*       party to the DSP resources.
******************************************************************************/
void setupSwitches( CTA_THRD_INFO *pctaInfo )
{
    DWORD   ret = SUCCESS;
    SWIHD   swihd;

    ret = swiOpenSwitch( pctaInfo->ctahd, gArgs.swiDriver, gArgs.board,
                         0, &swihd );

    if ( ret == SUCCESS )
    {
        setSwiHandle( swihd );
        mySwiConns( swihd, gArgs.inStream, gArgs.inSlot, 
                           gArgs.dspStream, gArgs.dspSlot,
                    TRUE );
    }
    else
        printf("CCV: Warning: unable to open switch: 0x%x\n", ret );
}
/******************************************************************************
* unconnectSwitches
*
*       This function disables the outputs and closes the switch.
******************************************************************************/
void unconnectSwitches( CTA_THRD_INFO *pctaInfo )
{
    DWORD ret = SUCCESS;
    SWIHD swihd;

    /* Disable the outputs */
    mySwiConns( (swihd = getSwiHandle()), gArgs.inStream,  gArgs.inSlot, 
                                          gArgs.dspStream, gArgs.dspSlot,
                FALSE );

    ret = swiCloseSwitch( swihd );

    if ( ret != SUCCESS )
        printf("CCV: Warning: unable to close switch: 0x%x\n", ret );

    setSwiHandle( NULL_CTAHD );
}
/******************************************************************************
* 
* THREAD: dispatchCommand
*
*   This thread is responsible for handing off events that come in from the
*   takecmd process to the appropriate worker thread.
******************************************************************************/

THREAD_RET_TYPE THREAD_CALLING_CONVENTION dispatchCommand( void *arg )
{
    DWORD ret = SUCCESS;
    CTA_THRD_INFO *pctaInfo = &ctaThrdData[ CMD_THRD_IDX ];

    printf("Thread started: dispatchCommand\n");

    /* Setup NA information required for this thread.Since this thread
    *  is only responsible for processing commands from takecmd via a
    *  shared context, no special NA services are required. This process
    *  attached to the context created by takecmd.
    */
    strcpy ( pctaInfo->ctxName, CMD_CNTX_NAME );
    pctaInfo->svcMgrList[0] = NULL;
    pctaInfo->numSvcs = 0;
    pctaInfo->ctaqueuehd = NULL_CTAQUEUEHD;

    for(;;)
    {
        if ( !pctaInfo->bctxReady )
        {
            /* Need to have a context on the server */
            if ( (ret = setupCtaOnThread( pctaInfo, gArgs.cmdHost ))
                != SUCCESS )
                /* Sleep a bit and try again later */
                DemoSleep( 2 );
        }
        else
        {
            CTA_EVENT   event = { 0 };
            int         ctaFreeBuf;

            /*************************************
            * WAIT for event to be sent to QUEUE *
            *************************************/

            myWaitEvent( pctaInfo, &event, &ctaFreeBuf, CTA_WAIT_FOREVER );  
            
            if ( event.id != CTAEVN_NULL_EVENT )
            {
                /* The value field indicates which thread is responsible for 
                *  processing the event.
                */

                switch ( event.value )
                {
                    case APPEVN_CMD_FOR_OAM: setCmd(&event, OAM_THRD_IDX); break;
                    case APPEVN_CMD_FOR_CCV: setCmd(&event, CCV_THRD_IDX); break;
                    case APPEVN_CMD_FOR_DTM: setCmd(&event, DTM_THRD_IDX); break;
                    /* General process control commands are handled in the thread
                    *  performing checkpoint operations.
                    */
                    case APPEVN_CMD_FOR_GENERAL: 
                                             setCmd(&event, CHK_THRD_IDX); break;
                    default:
                        break;
                }
            }
            else
                /* Wait abit for server to come back. */
                DemoSleep(1);

            /* Release resources owned by Natural Access */
            if ( ctaFreeBuf )
            {
                ctaFreeBuffer( event.buffer );
                ctaFreeBuf = 0;
            }
        }
    }

    UNREACHED_STATEMENT( THREAD_RETURN; )
}

/******************************************************************************
* 
* THREAD: checkPoint
*
*   This thread is responsible for managing the exchange of checkpoint data 
*   with its running peer. This thread operates in both primary and back mode.
* 
******************************************************************************/
THREAD_RET_TYPE THREAD_CALLING_CONVENTION checkPoint( void *arg )
{
    CTA_EVENT event = { 0 };
    DWORD     ret = SUCCESS;
    BOOL      bnewCmdEvent = FALSE;
    int       waitResetCount = 0;
    DWORD     size;
    DWORD     ourPid = (DWORD) getMyPid();

    CTA_THRD_INFO *pctaInfo = &ctaThrdData[ CHK_THRD_IDX ];

    printf("Thread started: checkPoint\n");

    /* Setup NA information required for this thread.Since this thread
    *  is responsible for processing events from the takover peer containing
    *  state information, no special NA services are required.
    */
    strcpy ( pctaInfo->ctxName, CHKPNT_CNTX_NAME );
    pctaInfo->svcMgrList[0] = NULL;
    pctaInfo->numSvcs = 0;
    pctaInfo->ctaqueuehd = NULL_CTAQUEUEHD;    
    
    if ( ! gArgs.bPrimary )
    {
        gState.bInitialize = TRUE;
        setRunMode( TOVR_MODE_BACKUP );
    }
    else
    {
        printf("-------------------------------- Running as Primary\n");
        setRunMode( TOVR_MODE_PRIMARY ); 
    }

    for (;;)
    {
        if ( !pctaInfo->bctxReady )
        {
            /* Need to have a context on the server */
            if ( (ret = setupCtaOnThread( pctaInfo, gArgs.cmdHost ))
                != SUCCESS )
                /* Sleep a bit and try again later */
                DemoSleep( 1 );
        }
        else
        {        
            int ctaFreeBuf;
            if ( !bnewCmdEvent )
            {
                /*************************************
                * WAIT for event to be sent to QUEUE *
                *************************************/
                myWaitEvent( pctaInfo, &event, &ctaFreeBuf, 
                             CHKPNT_HEARTBEAT_TMO );
            }

            if ( event.id == APPEVN_CMD_RUNSTATE )
            {
                /* Display what we're thinking... */
                showRunState();
            }
            else if ( getRunMode() == TOVR_MODE_PRIMARY )
            {
                /**********
                * PRIMARY *
                ***********/
                switch ( event.id )
                {
                    case CTAEVN_WAIT_TIMEOUT:
                        /* Inform backup that we are still alive. The process
                        *  ID is used to arbitrate which process should act as
                        *  primary in certain race conditions.
                        */
                        sendEvent( pctaInfo, APPEVN_XCHG_HEARTBEAT, 0, 
                                   NULL, ourPid );
                        break;

                    case APPEVN_CMD_EXIT:
                    case APPEVN_CMD_EXIT_P:
                        printf("\tCMD: Exiting...\n");
                        exit( 0 );
                        break;

                    case APPEVN_XCHG_GET_DATA:
                        /* Send all active data to peer */
                        size = myDtmStateData( gbuffer, sizeof(gbuffer) );
                        sendEvent( pctaInfo, APPEVN_XCHG_DTM_DATA, 0, 
                                   gbuffer, size );
                        
                        size = myOamStateData( gbuffer, sizeof(gbuffer) );
                        sendEvent( pctaInfo, APPEVN_XCHG_OAM_DATA, 0,
                                   gbuffer, size );
                        
                        size = ccvStateData( gbuffer, sizeof(gbuffer) );
                        sendEvent( pctaInfo, APPEVN_XCHG_CCV_DATA, 0,
                                   gbuffer, size );
                        break;

                    /********************************
                    *  Checkpoint data from thread  *
                    ********************************/
                    case APPEVN_XCHG_OAM:
                        sendEvent( pctaInfo, APPEVN_XCHG_OAM_UPDATE, 
                                   event.value, event.buffer, event.size );
                        break;
                    case APPEVN_XCHG_CCV:
                        sendEvent( pctaInfo, APPEVN_XCHG_CCV_UPDATE, 
                                   event.value, event.buffer, event.size );
                        break;

                    case APPEVN_XCHG_HEARTBEAT:
                        /* If peer is sending a heartbeat, then it is running as
                        *  the primary (race condition). If its PID is < than 
                        *  our PID, then go to backup role.
                        */
                        if ( event.size < ourPid )
                        {
                            printf("\tDetected another active PRIMARY; going to BACKUP mode.\n");
                            setRunMode( TOVR_MODE_BACKUP );
                            gState.missedHeartbeat = 0;
                        }
                        break;
                    
                    case APPEVN_CMD_SWITCH:

                        printf("\tCMD: Leaving PRIMARY and going to BACKUP mode\n");
                        setRunMode( TOVR_MODE_BACKUP );
 
                        /* Allow backup time to takeover, if necesary. */
                        ENTERCRITSECT();
                        DemoSleep( STOP_HEARTBEATS_FOR_IDLE );
                        EXITCRITSECT();
                        break;

                    default:
                        break;
                }
            }
            else 
            {
                /**********/
                /* BACKUP */
                /**********/
                switch ( event.id )
                {
                    case CTAEVN_WAIT_TIMEOUT:
                        /* Check that primary is still alive. */
                        if ( ++gState.missedHeartbeat > HEARTBEAT_MISSED_P )
                        {
                            gState.missedHeartbeat = 0;
                            printf("\n\tPrimary no longer sending heartbeat messages.\n");
                            printf("\tBackup now TAKING OVER primary role.\n\n"); 
                            setRunMode( TOVR_MODE_PRIMARY );
                            /* Don't request data from backup */
                            gState.bInitialize = FALSE;
                        }
                        break;

                    case APPEVN_CMD_EXIT:
                    case APPEVN_CMD_EXIT_B:
                        printf("\tCMD: Exiting...\n");
                        exit( 0 );
                        break;
                    
                    case APPEVN_XCHG_DTM_DATA:
                        /* DTM state data from primary; update DTM thread */
                        setCmd( &event, DTM_THRD_IDX );
                        break;

                    case APPEVN_XCHG_OAM_DATA:
                        /* OAM state from primary; update OAM thread */
                        setCmd( &event, OAM_THRD_IDX );
                        break;

                    case APPEVN_XCHG_CCV_DATA:
                       /* CCV state from primary; update CCV thread */
                        setCmd( &event, CCV_THRD_IDX );
                        break;
                 
                    case APPEVN_XCHG_HEARTBEAT:
                
                        if ( gState.bInitialize )
                        {
                            /* First heartbeat from primary after initial
                            *  startup in this mode. Request current state info 
                            *  from primary.
                            */
                            gState.bInitialize = FALSE;
                            sendEvent( pctaInfo, APPEVN_XCHG_GET_DATA, 0,
                                       NULL, 0 );
                        }
                        /* Primary still active */
                        gState.missedHeartbeat = 0;
                        break;

                    case APPEVN_XCHG_OAM_UPDATE:
                        /* There is an update to the OAM data. The "value" 
                        *  field contains the actual commad for the OAM thread.
                        */
                        event.id = event.value;
                        setCmd( &event, OAM_THRD_IDX );
                        break;
                    
                    case APPEVN_XCHG_CCV_UPDATE:
                        /* There is an update to the CCV data. The "value" 
                        *  field contains the actual command for the CCV thread.
                        */
                        event.id = event.value;
                        setCmd( &event, CCV_THRD_IDX );
                        break;
                        
                    default:
                        break;
                }
            }
            /* Release resources owned by Natural Access */
            if ( ctaFreeBuf )
            {
                ctaFreeBuffer( event.buffer );
                ctaFreeBuf = 0;
            }
            else if ( bnewCmdEvent )
            {
                /* Free up locally malloc'd buffer */
                bnewCmdEvent = FALSE;
                event.id = 0;
                if ( event.buffer )
                    free( event.buffer );
            }

            /******************************************************************
            *
            * Check for USER commands that would change the way we are 
            * operating
            *
            ******************************************************************/
            if ( pctaInfo->bctxReady )
                bnewCmdEvent = getCmd( &event, CHK_THRD_IDX );
        }
    }
    UNREACHED_STATEMENT( THREAD_RETURN; )
}
/******************************************************************************
* 
* THREAD: ccVoice
*
******************************************************************************/
THREAD_RET_TYPE THREAD_CALLING_CONVENTION ccVoice( void *arg )

{
    CTA_EVENT  event = { 0 };
    DWORD      ret = SUCCESS;
    BOOL       bnewCmdEvent = FALSE;
    char       called_digits[20];
    char       calling_digits[20];
    NCC_CALLHD callhd;
    CCV_STATE  state;      
    CTA_THRD_INFO *pctaInfo = &ctaThrdData[ CCV_THRD_IDX ];
    CCV_DATA      *pccv = &gState.voiceInfo;

    printf("Thread started: ccVoice\n");
    
    strcpy ( pctaInfo->ctxName, CCV_CNTX_NAME );
    pccv->callhd = NULL_NCC_CALLHD;
 
    /* Setup NA information required for this thread. This thread will
    *  be using the ADI, NCC, and SWI services. Note: Using all service
    *  managers in list; some of the services share managers.
    */
    pctaInfo->svcMgrList[0] = gsvcMgrList[ 0 ]; 
    pctaInfo->svcMgrList[1] = gsvcMgrList[ 1 ]; 
    pctaInfo->svcMgrList[2] = gsvcMgrList[ 2 ]; 

    memcpy( &pctaInfo->svcDescriptions[0],
            &gsvcDescriptions[ SETUP_IDX_NCC ], sizeof(CTA_SERVICE_DESC) );
    memcpy( &pctaInfo->svcDescriptions[1],
            &gsvcDescriptions[ SETUP_IDX_ADI ], sizeof(CTA_SERVICE_DESC) );
    memcpy( &pctaInfo->svcDescriptions[2],
            &gsvcDescriptions[ SETUP_IDX_SWI ], sizeof(CTA_SERVICE_DESC) );

    /* Fill in NCC service MVIP address information */
    pctaInfo->svcDescriptions[ 0 ].mvipaddr.board    = gArgs.board;
    pctaInfo->svcDescriptions[ 0 ].mvipaddr.stream   = 0;
    pctaInfo->svcDescriptions[ 0 ].mvipaddr.timeslot = gArgs.dspSlot;
    pctaInfo->svcDescriptions[ 0 ].mvipaddr.mode     = ADI_FULL_DUPLEX;

    /* Fill in ADI service MVIP address information */
    pctaInfo->svcDescriptions[ 1 ].mvipaddr.board    = gArgs.board;
    pctaInfo->svcDescriptions[ 1 ].mvipaddr.stream   = 0;
    pctaInfo->svcDescriptions[ 1 ].mvipaddr.timeslot = gArgs.dspSlot;
    pctaInfo->svcDescriptions[ 1 ].mvipaddr.mode     = ADI_FULL_DUPLEX;
                
    pctaInfo->svcNames[0] = gsvcNames[ SETUP_IDX_NCC ];
    pctaInfo->svcNames[1] = gsvcNames[ SETUP_IDX_ADI ];
    pctaInfo->svcNames[2] = gsvcNames[ SETUP_IDX_SWI ];
    pctaInfo->numSvcs = 3;
    pctaInfo->ctaqueuehd = NULL_CTAQUEUEHD;
    
    /* Setup initial state of call thread */
    setProtStarted( FALSE );
    setCallState( CCV_IDLE );
    setBoardState( BRD_UNKNOWN );
    
    /* The context may or may not be running on the same host. It may be
    *  on a CG6000eC board. 
    */
    for (;;)
    {
        int       ctaFreeBuf = 0;
        DWORD     svcRet = SUCCESS;
        BOOL      bhangUp = FALSE;

        if ( !pctaInfo->bsvcReady )
        {
            if ( bnewCmdEvent )
            {
                /* Process certain user command input even if services aren't
                *  opened 
                */
                bnewCmdEvent = FALSE;
                switch ( event.id )
                {
                    case APPEVN_BOARD_STOPPED:
                        /* One or more boards are being stopped; release 
                        *  resources
                        */
                        if ( !event.size || ( event.value == gArgs.board ) )
                            setBoardStopped( TRUE );
                        break;
                    
                    case APPEVN_XCHG_CCV_DATA:
                        /* Save CCV data sent from primary */
                        if ( getRunMode() == TOVR_MODE_BACKUP )
                            ccvResetFromPrimary( &event, pctaInfo );
                        break;

                    default:
                        printf("CCV: Services not yet opened; unable to process"
                                " command 0x%x\n", event.id);
                        break;
                }
                event.id = 0;
                if ( event.buffer )
                    free( event.buffer );
            }

            /* Services may not be ready because the context hasn't been 
            *  created yet or the board is not booted.
            */
            ccvOpenServices( pctaInfo, pccv );
        }
        else
        {
            if ( !bnewCmdEvent )
            {
                /*************************************
                * WAIT for event to be sent to QUEUE *
                *************************************/
                myWaitEvent( pctaInfo, &event, &ctaFreeBuf, CCV_WAIT_TMO );
            }
            
            if ( event.id == CTAEVN_NULL_EVENT )
            {
                /* There's been a communcation problem with the NA server. 
                *  The context has been closed. Reset the local book-keeping.
                */
                if ( (state = getCallState()) != CCV_IDLE )
                {
                    if ( state == CCV_PLAYING )
                    {
                        free( pccv->pplayBuf );
                        fclose( pccv->pplayFile );
                        pccv->pplayBuf = NULL;
                    }
                    ctaDetachObject( getSwiHandle() );
                    setCallState( CCV_IDLE );
                    setCallHandle( NULL_NCC_CALLHD );
                    setSwiHandle( NULL_CTAHD );
                }

                /* Will need to determine later if the board is booted and 
                *  restart protocol.
                */
                setProtStarted( FALSE );
                setBoardState( BRD_UNKNOWN );
            }
            else if ( event.id == APPEVN_BOARD_STOPPED )
            {
                /* One or more boards are being stopped; release resources */
                if ( !event.size || ( event.value == gArgs.board ) )
                { 
                    setProtStarted( FALSE );
                    setBoardStopped( TRUE );
                    bhangUp = TRUE;
                }
            }
            else if ( event.id == ADIEVN_PLAY_DONE )
            {
                /* Same processing by both PRIMARY and BACKUP */
                printf("CCV: ADIEVN_PLAY_DONE\n");
                /* The call may have been disconnected, thus causing
                *  the play to end. Check before setting to
                *  connected.
                */
                if ( getCallState() != CCV_IDLE )
                    setCallState( CCV_CONNECTED );

                /* Release the play file buffer space and
                *  close the open file handle.
                */
                if ( pccv->pplayBuf )
                    free( pccv->pplayBuf );
                fclose( pccv->pplayFile );
                pccv->pplayBuf = NULL;
            }
            else if ( getRunMode() == TOVR_MODE_PRIMARY )
            {
                /**********
                * PRIMARY *
                **********/
                switch ( event.id )
                {
                    case APPEVN_CCV_DO_CALL:
                        /******************************/
                        /* Establish a connected call */
                        /******************************/
                        if ( getCallState() != CCV_IDLE)
                        {
                            printf("CCV: Invalid state for establishing call.\n");
                        }
                        else
                        {                   
                            if ( !getProtStarted() )
                            {
                                /* Start up the protocol on the board. */
                                if ( strncmp(gArgs.protocol, "isd", 3) == 0 ) 
                                    DemoStartNCCProtocol( pctaInfo->ctahd, 
                                        gArgs.protocol, &pccv->isdparms.isdn_start, 
                                        &pccv->nccadiparms, &pccv->nccparms );
                                else 
                                    DemoStartNCCProtocol( pctaInfo->ctahd, 
                                                      gArgs.protocol, NULL, 
                                                      &pccv->nccadiparms, 
                                                      &pccv->nccparms );
                                setProtStarted( TRUE );
                                ccvSendToPeer( pccv, APPEVN_CCV_PROTOCOL );
                            }
                            /* Switch the line interfaces to the resources. */
                            setupSwitches( pctaInfo );
                            
                            /* Wait for caller to dial in. */
                            DemoWaitForNCCCall( pctaInfo->ctahd, &callhd, 
                                            called_digits, calling_digits );
                            setCallHandle( callhd );
                           
                            if ( DemoAnswerNCCCall( pctaInfo->ctahd, callhd,
                                                    1, NULL ) )
                            {
                                printf("DemoAnswerNCCCall failed  %d\n", ret); 
                                bhangUp = TRUE;
                            }
                            else
                            {
                                setCallState( CCV_CONNECTED );
                                /* Tell peer that call is connected, i.e., check- 
                                *  point call handle.
                                */
                                ccvSendToPeer( pccv, APPEVN_CCV_CALLCONN );
                            }
                        }        
                        break;

                    case APPEVN_CCV_HANG_UP:
                        printf("\tCMD: Hangup call.\n");
                        if ( getCallState() != CCV_IDLE)
                            /* Hang up call. */
                            bhangUp = TRUE;
                        else
                            printf("\tCCV: Invalid state for hangup.\n");
                        break;

                    case APPEVN_CCV_COLLECT:
                    {
                        char     *pdigits = NULL;
                        unsigned *pbuf;
                        CCV_STATE state;
                        
                        /* Collect Digits */
                        if ( (state = getCallState()) != CCV_IDLE )
                        {
                            adiFlushDigitQueue( pctaInfo->ctahd );
                            
                             
                            /* Allocate a buffer big enough to hold digits */
                            pbuf = (unsigned *) event.buffer;
                            pccv->numDigits = *pbuf;
                            if ( pccv->numDigits )
                            {
                                printf("\tCMD: Collect %d digits.\n", 
                                        pccv->numDigits );  
                                if ( !(pdigits = (char*) malloc( pccv->numDigits + 1)) )
                                    NO_MEMORY();
                           
                                /* Tell Peer */
                                ccvSendToPeer( pccv, APPEVN_CCV_COLLECT_BEG );
                                ret = DemoGetDigits( pctaInfo->ctahd, 
                                                     pccv->numDigits, 
                                                     pdigits );

                                if ( ret == DISCONNECT )
                                    bhangUp = TRUE;
                                else if ( ret == SUCCESS )
                                    printf("CCV: Digits collected were %s\n", 
                                            pdigits ); 

                                if ( state == CCV_PLAYING )
                                {
                                    /* If play file was active, it would have  
                                    *  been automatically stopped and the ADI
                                    *  PLAY_DONE event will have been swallowed
                                    *  by the DemoGetDigits routine. Cleanup.
                                    */
                                    setCallState( CCV_CONNECTED );
                                    free( pccv->pplayBuf );
                                    fclose( pccv->pplayFile );
                                    pccv->pplayBuf = NULL;
                                }
                            
                                free( pdigits );
                                /* Independent of its succeeding, tell backup that 
                                *  collection has stopped.
                                */
                                ccvSendToPeer( pccv, APPEVN_CCV_COLLECT_END );
                            }
                            else
                                printf("CMD: Collect digits; invalid 0 "
                                       "digits passed.\n");
                        }
                        else
                            printf("CCV: Call not in connected state.\n");
                        break;
                    }
                    case APPEVN_CCV_PLAY_FILE:
                    {   /*************/
                        /* Play file */
                        /************/
                        unsigned  flags; 
                        char fullpathname [CTA_MAX_PATH];   
                        
                        if ( getCallState() != CCV_CONNECTED )
                        {
                            printf("CCV: Invalid call state for play.\n");
                            break;
                        }
                        if ( !*pccv->voiceFile )
                            strcpy( pccv->voiceFile, gArgs.voiceFile );

                        pccv->encoding = gArgs.encoding;
                        adiGetEncodingInfo( pctaInfo->ctahd, pccv->encoding, 
                                           NULL, NULL, &pccv->playBufSize );

                        /* Malloc a play buffer */
                        if( !(pccv->pplayBuf = (char*)malloc(pccv->playBufSize)) )                    
                            NO_MEMORY();

                        /* Find and open the voice file */
                        if(   (ctaFindFile( pccv->voiceFile, "vox", "CTA_DPATH",
                                    fullpathname, CTA_MAX_PATH) != SUCCESS )
                           || ((pccv->pplayFile = fopen( fullpathname, "rb" ))
                               == NULL) )
                        {
                            printf("Unable to open file %s\n", fullpathname );
                        }
                        else
                        {   
                            printf("\tCMD: play file %s.\n", fullpathname );  
                            setCallState ( CCV_PLAYING );
                            adiFlushDigitQueue( pctaInfo->ctahd );
                            pccv->playSize = fread( pccv->pplayBuf, 1, 
                                                    pccv->playBufSize, 
                                                    pccv->pplayFile );
                            /* If this is the only buffer (feof() != 0), then
                            *  indicate so in flags.
                            */
                            flags = feof( pccv->pplayFile ) ? 
                                           ADI_PLAY_LAST_BUFFER : 0;
                            /* Start playing and submit 1st buffer.  The event
                            *  handler will read and submit any subsequent
                            *  buffers.
                            */
                            adiPlayAsync( pctaInfo->ctahd, pccv->encoding, 
                                          pccv->pplayBuf, 
                                          pccv->playSize,
                                          flags, NULL );

                            pccv->currPosition = pccv->playSize;
                            ccvSendToPeer( pccv, APPEVN_CCV_PLAY_BEG );
                        }
                        break;
                    }
                                                       
                    case APPEVN_CCV_ABORT_PLAY:
                        printf("\tCMD: Abort play file.\n");
                        if ( getCallState() == CCV_PLAYING )
                        {
                            CTA_EVENT doneEvt = {0};
                            
                            adiStopPlaying( pctaInfo->ctahd );
                            DemoWaitForSpecificEvent( pctaInfo->ctahd, 
                                                  ADIEVN_PLAY_DONE, &doneEvt );
                            /* Peer will receive ADIEVN_PLAY_DONE event and 
                            *  make the appropriate state changes.
                            */
                            if ( pccv->pplayBuf )
                                free( pccv->pplayBuf );
                            fclose( pccv->pplayFile );
                            pccv->pplayBuf = NULL;
                            setCallState( CCV_CONNECTED );
                        }
                        else
                             printf("CCV: Invalid state for abort.\n");
                        break;

 
                    case ADIEVN_PLAY_BUFFER_REQ:
                                                
                        if ( getCallState() == CCV_PLAYING )
                        {
                            unsigned flags;

                            /* Read next voice buffer and submit it. */
                            pccv->playSize = fread( pccv->pplayBuf, 1, 
                                                    pccv->playBufSize, 
                                                    pccv->pplayFile );
                                    
                            flags = feof( pccv->pplayFile ) ? 
                                       ADI_PLAY_LAST_BUFFER : 0;
                            printf("CCV: Submit buffer\n");
                            adiSubmitPlayBuffer( pctaInfo->ctahd, pccv->pplayBuf, 
                                                 pccv->playSize, flags );

                            pccv->currPosition += pccv->playSize;
                            ccvSendToPeer( pccv, APPEVN_CCV_PLAY_BUF );
                        }
                        break;

                    case ADIEVN_COLLECTION_DONE:
                        
                        /* If a takeover switch occurs during the collection, 
                        *  this instance of takeover will get the DONE event, 
                        *  rather than it being caught by the DemoGetDigits 
                        *  function.
                        */
                        if ( CTA_IS_ERROR( event.value ) || !strlen( (char*)event.buffer ) )
                            printf("CCV: Digit collection failed\n");
                        else
                            printf("CCV: Digits collected were %s\n", event.buffer );                                 
                    
                        /* If Natural Access doesn't own this buffer, we need 
                         * to free it 
                         */
                        if ( !ctaFreeBuf )
                            free( event.buffer );    
              
                        /* Need to free buffer we allocated locally */
                        if ( pccv->pdigitBuf )
                            free( pccv->pdigitBuf );
                        pccv->pdigitBuf = NULL;
                        
                        /* Independent of its succeeding, tell backup that 
                        *  collection has stopped.
                        */
                        ccvSendToPeer( pccv, APPEVN_CCV_COLLECT_END );
                        break;

                    case NCCEVN_CALL_DISCONNECTED:
                    case NCCEVN_CALL_RELEASED:
                        bhangUp = TRUE;
                        break;

                    default:
                        break;
                }
            }
            else 
            {
                /*************
                *   BACKUP   *
                *************/
                switch ( event.id )
                {
                    case APPEVN_XCHG_CCV_DATA:
                        /* Save CCV data sent from primary */
                        ccvResetFromPrimary( &event, pctaInfo );
                        break;

                    case APPEVN_CCV_PROTOCOL:
                        /* call control protocol has been started. */
                        setProtStarted( TRUE );
                        break;

                    case APPEVN_CCV_CALLCONN:
                    {
                        CC_CHECKPOINT *pchkData = NULL;
                        SWIHD          swihd;
                         
                        /* Primary has entered call connected state.
                        */
                        pchkData = (CC_CHECKPOINT*)event.buffer;
                        setCallState( pchkData->state );
                        
                        /*  Attach to the active call represented by the call
                        *   handle descriptor that has been sent.
                        */
                        ret = ctaAttachObject( &pctaInfo->ctahd, 
                                               pchkData->callDescriptor,
                                               (unsigned *)&callhd );
                        if ( ret == SUCCESS )
                        {
                            setCallHandle( callhd );
                            printf("CCV: Attachment to active call complete.\n");
                            /*  Attach to the open switch via the switch
                            *   handle descriptor that has been sent.
                            */
                            ret = ctaAttachObject( &pctaInfo->ctahd, 
                                               pchkData->swiDescriptor,
                                               (unsigned  *)&swihd );
                            if ( ret == SUCCESS )
                            {
                                setSwiHandle( swihd );
                                printf("CCV: Attach to switch complete.\n");
                            }
                            else
                            {
                                DemoShowError("CCV: Attach to switch failed.",
                                               ret ); 
                            }
                        }
                        else
                        {
                            DemoShowError("CCV: Attach to call failed.", ret ); 
                            setCallState( CCV_IDLE );
                        }
                        break;
                    }
                    case APPEVN_CCV_COLLECT_BEG:
                        /* Primary has started collection of digits.
                        *  Reserve collection buffer in case we need to
                        *  complete task.
                        */
                        pccv->numDigits = event.size;
                        if ( !(pccv->pdigitBuf = 
                               (char*)malloc( pccv->numDigits + 1)) )
                             NO_MEMORY();
                        
                        break;
                   
                    case APPEVN_CCV_COLLECT_END:
                        pccv->numDigits = 0;
                        if ( pccv->pdigitBuf )
                            free( pccv->pdigitBuf );
                        break;

                    case ADIEVN_COLLECTION_DONE:
                        printf("CCV: ");
                        if ( !gArgs.bVerbose )
                            DemoShowEvent( &event );
                        if (   ( event.value == CTA_REASON_RELEASED )
                            || ( CTA_IS_ERROR( event.value ) )
                            || ( event.size == 0 ) )
                        {
                            printf("CCV: Collection error\n");
                        }
                        free( pccv->pdigitBuf );
                        pccv->pdigitBuf = NULL;
                        break;
 
                    case APPEVN_CCV_PLAY_BEG:
                    {   
                        /* Primary is playing a file. Be ready to take over in 
                        *  case primary fails.
                        */
                        CC_CHECKPOINT  *pprimary = (CC_CHECKPOINT *)event.buffer;
                        char            fullpathname [CTA_MAX_PATH] = {0};   
                                                
                        setCallState ( CCV_PLAYING );
                        adiFlushDigitQueue( pctaInfo->ctahd );
                        pccv->encoding = pprimary->encoding;
                        pccv->playBufSize = pprimary->playBufSize;
                        pccv->playSize = pprimary->playSize;

                        /* Malloc a play buffer */
                        if( !(pccv->pplayBuf = (char*)malloc(pccv->playBufSize)) )                    
                            NO_MEMORY();
 
                        /* Find and open the voice file */
                        if(   (ctaFindFile( pprimary->voiceFile, "vox", "CTA_DPATH",
                                     fullpathname, CTA_MAX_PATH) != SUCCESS )
                           || (pccv->pplayFile = fopen( fullpathname, "rb" )) 
                                     == NULL )
                        {
                            printf("Unable to open file %s\n", fullpathname );
                        }
                        else
                        {
                            printf("CCV: Ready to continue play of %s\n", 
                                    fullpathname  );
                            fread( pccv->pplayBuf, 1, pccv->playBufSize, 
                                   pccv->pplayFile );
                        }
                        break;
                    }
                    case APPEVN_CCV_PLAY_BUF:
                        /* Primary has read and played more of the file. Seek
                        *  to next file position so that we are prepared to
                        *  take over if necessary.
                        */
                        pccv->playSize = event.size;
                        if ( pccv->pplayFile )
                            fseek( pccv->pplayFile, pccv->playSize, SEEK_CUR );
                        break;

                    case NCCEVN_CALL_DISCONNECTED:
                    case NCCEVN_CALL_RELEASED:

                        setCallState( CCV_IDLE );
                        setCallHandle( NULL_NCC_CALLHD );
                        setSwiHandle( NULL_CTAHD );
                        break;

                    case CTAEVN_CLOSE_SERVICES_DONE:
                        /* Primary has closed services on this context */
                        printf("CCV: Services Closed.\n");
                        pctaInfo->bsvcReady = FALSE;
                        break;

                    default:
                        break;
                }
            }
            if ( bhangUp )
            {
                bhangUp = FALSE;
                if ( (state = getCallState()) != CCV_IDLE )
                {
                    callhd = getCallHandle();
                    if ( state == CCV_PLAYING )
                    {
                        if ( getRunMode() == TOVR_MODE_PRIMARY )
                        {
                            CTA_EVENT evtDone = {0};
                            adiStopPlaying( pctaInfo->ctahd );
                            DemoWaitForSpecificEvent( pctaInfo->ctahd, 
                                                  ADIEVN_PLAY_DONE, &evtDone );
                        }
                        free( pccv->pplayBuf );
                        fclose( pccv->pplayFile );
                        pccv->pplayBuf = NULL;
                    }

                    if ( getRunMode() == TOVR_MODE_PRIMARY )
                    {
                        DemoHangNCCUp( pctaInfo->ctahd, callhd, NULL, NULL );
                        unconnectSwitches( pctaInfo );
                        printf("\t--------\n");
                    }
                    setCallHandle( NULL_NCC_CALLHD );
                    setCallState ( CCV_IDLE );
                }
            }

            /* If board has been stopped, need to close services. Because
            *  this is a shared context, only one of the attached clients
            *  needs to close the service. The backup will just mark the 
            *  service as closed even though it's done by the primary.
            */
            if ( boardIsStopped() && (getRunMode() == TOVR_MODE_PRIMARY ) )
            {
                /* Close services */
                closeCtaSvs( pctaInfo );
                printf("CCV: Services Closed.\n");
            }
                                       
            /* Release resources owned by Natural Access */
            if ( ctaFreeBuf )
            {
                ctaFreeBuffer( event.buffer );
                ctaFreeBuf = 0;
            }
            else if ( bnewCmdEvent )
            {
                /* Free up locally malloc'd buffer */
                bnewCmdEvent = FALSE;
                event.id = 0;
                if ( event.buffer )
                    free( event.buffer );
            }
        }
        /******************************************************************
        * Check for USER commands that could change the way we are 
        * operating
        ******************************************************************/
        if ( pctaInfo->bctxReady )
            bnewCmdEvent = getCmd( &event, CCV_THRD_IDX );
    }

    UNREACHED_STATEMENT( THREAD_RETURN; )
}
/******************************************************************************
* boardBooted
*
*   This function determines whether the board being used by this demo has
*   been booted.
******************************************************************************/
BOOL boardBooted( CTA_THRD_INFO *pctaInfo )
{
    CTA_EVENT event = {0};
    BOOL      ret = FALSE;
    int       retry = 0;
    BRD_STATE state;

    if ( boardIsStopped() )
    {
        /* We've stopped the board */
        ret = FALSE;
    }
    else if ( (state = getBoardState()) == BRD_UNKNOWN )
    {
        /* Ask the monitorBoards thread to determine the current state of 
        *  the board. This context does not have the OAM service opened.
        */
        event.id = APPEVN_OAM_BOARD_STATE;
        setCmd(&event, OAM_THRD_IDX);
        DemoSleep( 1 );
        while ( ((state = getBoardState()) == BRD_UNKNOWN)
                && (retry++ < 2))
        {
             DemoSleep( 1 );
        }

        if ( state == BRD_BOOTED )
            ret = TRUE;
        else if ( state == BRD_ERROR )
        {
            printf("CCV: ERROR accessing board %d.\n", gArgs.board );
            /* Give system a bit of time to come up. */
            DemoSleep( 1 );
            setBoardState( BRD_UNKNOWN );
       }
    } 
    else if ( state == BRD_BOOTED )
        ret = TRUE;

    return ret;
}

/*****************************************************
*  ccvOpenServices
*
*      Opens up services for the call control
*****************************************************/
void ccvOpenServices( CTA_THRD_INFO *pctaInfo, CCV_DATA *pccv )
{
    DWORD ret = SUCCESS;

    /* Services may not be ready because the context hasn't been 
    *  created yet or the board is not booted.
    */
    if ( !pctaInfo->bctxReady )
    {
        /* Need to have a context on the server */
        if ( (ret = setupCtaOnThread( pctaInfo, gArgs.svrHost ))
            != SUCCESS )
        {
            setBoardState( BRD_UNKNOWN );
            /* Sleep a bit and try again later */
            DemoSleep( 1 );
        }
        else if( boardBooted( pctaInfo ) )
            /* Now open services. Certain services can only be 
            *  opened if the board is up and running.
            */
            setupCtaServices( pctaInfo );
    }
    else if( boardBooted( pctaInfo ) )
        setupCtaServices( pctaInfo );

    if( pctaInfo->bsvcReady )
    {
         printf("CCV: Services are opened on board %d\n", gArgs.board );
       	 ret = ctaGetParms( pctaInfo->ctahd, NCC_START_PARMID, 
                            &pccv->nccparms, sizeof( pccv->nccparms ) );
         if (strncmp( gArgs.protocol, "isd", 3) == 0)
	         ret = ctaGetParms( pctaInfo->ctahd, NCC_ADI_ISDN_PARMID, 
                               &pccv->isdparms, sizeof(pccv->isdparms));
      
         /* Determine the DSP resource stream to use on the board. 
         *  board, if caller hasn't provided it.
         */
         if ( gArgs.dspStream == -1 )
         {
             ADI_CONTEXT_INFO info;
    
            /* Get the DSP's Mvip-95 stream number */
            ret = adiGetContextInfo( pctaInfo->ctahd, &info, sizeof info);
            if ( ret != SUCCESS )
            {
                DemoShowError("Call to adiGetContextInfo fails", ret ); 
                exit( 1 );
            }               
            gArgs.dspStream= info.stream95;
        }
     }
     else
        /* Sleep a bit before trying to reopen the service */
        DemoSleep( 1 );
}
/******************************************************************************
* ccvSendToPeer
*
*   This function sends an event to the backup to indicate that a change has
*   been made to the call-control/voice  state.
*******************************************************************************/
void ccvSendToPeer( CCV_DATA *pccv, DWORD cmd )
{
    CTA_EVENT      event = {0};
    CC_CHECKPOINT  evtBuff = {0};
    DWORD          ret = SUCCESS;
    NCC_CALLHD     callhd;
    SWIHD          swihd;    
    BOOL           bbuffData = FALSE;

    /* Build event that checkpoint thread will send to peer CCV thread.
    */ 
    event.id = APPEVN_XCHG_CCV;
    switch( cmd )
    {
        case APPEVN_CCV_CALLCONN:
            /* Convert the call and switch handles to an object descriptors so 
            *  that backup can attach to them.
            */
            if ( ( callhd = getCallHandle()) != NULL_NCC_CALLHD )
            {
                ret = ctaGetObjDescriptor( callhd, evtBuff.callDescriptor, 
                                          MAX_DESCRIPTOR_SIZE);
                if ( ret != SUCCESS )
                {
                    DemoShowError("Unable to get descriptor for call handle",
                                   ret );
                }
                else
                    bbuffData = TRUE;
            }
            if ( (swihd = getSwiHandle()) != NULL_CTAHD )
            { 
                if ( (ret = ctaGetObjDescriptor( swihd, 
                            evtBuff.swiDescriptor, MAX_DESCRIPTOR_SIZE) )
                    != SUCCESS )
                {
                    DemoShowError("Unable to get descriptor for switch handle", 
                                  ret );
                }
                else
                    bbuffData = TRUE;
            }
            if ( bbuffData )
            { 
                event.size = sizeof( CC_CHECKPOINT );
                event.buffer = &evtBuff;
            }
            break;

        case APPEVN_CCV_COLLECT_BEG:

            /* Pass number of digits that need to be collected */
            event.size = pccv->numDigits;
            break;

        case APPEVN_CCV_PLAY_BEG:

            evtBuff.state = pccv->state;
            strcpy( evtBuff.voiceFile, pccv->voiceFile );
            evtBuff.encoding    = pccv->encoding;
            evtBuff.playBufSize = pccv->playBufSize;
            evtBuff.playSize    = pccv->playSize;
            event.size          = sizeof( CC_CHECKPOINT );
            event.buffer        = &evtBuff;
            break;

        case APPEVN_CCV_PLAY_BUF:
            event.size = pccv->playSize;
            break;

        case APPEVN_CCV_PROTOCOL:
            break;

        default:
            break;
    }

    if ( ret == SUCCESS )
    {
        /* The "value" will be moved to the event id field by the checkpoint
        *  point thread.
        */
        event.value = cmd;
        /* Ask checkpoint thread to do the exchange */
        setCmd( &event, CHK_THRD_IDX);
    }
}
/******************************************************************************
* ccvStateData
*
*   Places active CCV data into a buffer.
*******************************************************************************/
DWORD ccvStateData( char *pbuf, DWORD bufSize )
{
    DWORD          ret = SUCCESS;
    CC_CHECKPOINT  ccvInfo = {0};
    CTA_THRD_INFO *pctaInfo = &ctaThrdData[ CCV_THRD_IDX ];
    CCV_DATA      *pccv = &gState.voiceInfo;
 
    ENTERCRITSECT();

    /* Call state information. */
    ccvInfo.state = pccv->state;
    ccvInfo.bprotocolStarted = pccv->bprotocolStarted;
    if ( pccv->callhd != NULL_NCC_CALLHD )
    {
        ret = ctaGetObjDescriptor( pccv->callhd, ccvInfo.callDescriptor,
                                   MAX_DESCRIPTOR_SIZE);
        if ( ret != SUCCESS )
        {
            DemoShowError("Unable to get descriptor for call handle.", ret );
        }
    }
    if ( gState.swihd != NULL_CTAHD )
    {
        if ( (ret = ctaGetObjDescriptor( gState.swihd, ccvInfo.swiDescriptor,
                                         MAX_DESCRIPTOR_SIZE) )
            != SUCCESS )
        {
            DemoShowError("Unable to get descriptor for switch handle.", 
                           ret );
        }
    }

    /* Play file information */
    if ( pccv->state == CCV_PLAYING )
    {
        strcpy( ccvInfo.voiceFile, pccv->voiceFile );
        ccvInfo.encoding     = pccv->encoding;
        ccvInfo.playBufSize  = pccv->playBufSize;
        ccvInfo.playSize     = pccv->playSize;
        ccvInfo.currPosition = pccv->currPosition;
    }

    EXITCRITSECT();
    memcpy( pbuf, &ccvInfo, sizeof( CC_CHECKPOINT ) );

    return sizeof( CC_CHECKPOINT );
}
/******************************************************************************
* ccvResetFromPrimary
*
*   Info in buffer for all CCV data being being retrieved. 
******************************************************************************/
void ccvResetFromPrimary( CTA_EVENT *pevent, CTA_THRD_INFO *pctaInfo )
{
    DWORD           ret = SUCCESS;
    BOOL            bAttached = FALSE;
    CC_CHECKPOINT  *pprimary = (CC_CHECKPOINT *) pevent->buffer;
    CCV_DATA       *pccv = &gState.voiceInfo;
    NCC_CALLHD      callhd;
    SWIHD           swihd;
    char            fullpathname [CTA_MAX_PATH];       
    
    setCallState( pprimary->state );
    setProtStarted( pprimary->bprotocolStarted );

    /*  Attach to the active call represented by the call
    *   handle descriptor that has been sent.
    */
    if ( bAttached = (*pprimary->callDescriptor != '\0') )
    {
        ret = ctaAttachObject( &pctaInfo->ctahd, pprimary->callDescriptor,
                               (unsigned *)&callhd );
        if ( ret == SUCCESS )
            setCallHandle( callhd );
    }

    if ( bAttached && ret == SUCCESS )
    {
        /* Attach to opened switch handle */
        ret = ctaAttachObject( &pctaInfo->ctahd, pprimary->swiDescriptor,
                              (unsigned *)&swihd );
        if ( ret == SUCCESS )
        {
            setSwiHandle( swihd );
            printf("CCV: Attach to switch complete.\n");
        }
        else
        {
            DemoShowError("CCV: Attach to switch failed.", ret ); 
        }
    }

    if( (ret == SUCCESS) && (pccv->state == CCV_PLAYING) )
    {
        /* Primary is currently playing a voice file. */
        strcpy( pccv->voiceFile, pprimary->voiceFile );
        pccv->encoding     = pprimary->encoding;
        pccv->playBufSize  = pprimary->playBufSize;
        pccv->playSize     = pprimary->playSize;
        pccv->currPosition = pprimary->currPosition;
        
        /* Malloc a play buffer */
        if( !(pccv->pplayBuf = (char*)malloc(pccv->playBufSize)) )                    
            NO_MEMORY();

        /* Find and open the voice file */
        if(   (ctaFindFile( pprimary->voiceFile, "vox", "CTA_DPATH",
                            fullpathname, CTA_MAX_PATH) != SUCCESS )
           || (pccv->pplayFile = fopen( fullpathname, "rb" )) 
              == NULL )
        {
            printf("Unable to open file %s\n", fullpathname );
        }
        else
        {
            /* Be ready to play */
            printf("CCV: Ready to continue play of %s\n", pprimary->voiceFile );
            fseek( pccv->pplayFile, pccv->currPosition, SEEK_SET );
        }              
    }
    
    if ( ret != SUCCESS )
    {
        DemoShowError("Unable to get descriptor for call or switch handle.", ret );
    }
    else
    {
        if( bAttached )
            printf("CCV: Attachment to active call complete.\n");       
    }
}
/******************************************************************************
* ccvShow
*
*   Display active data related to call control and voice thread. 
******************************************************************************/
void ccvShow()
{
    CCV_DATA       *pccv = &gState.voiceInfo;

    if ( boardIsStopped() )
        printf("\t** CCV: Board %d has been STOPPED.\n", gArgs.board );
    else
        printf("\t** CCV: State of board is %s\n", 
                ccvDisplayBrdState( gState.boardState ) );

    printf("\t** CCV: Connection state is %s\n", 
            ccvDisplayState( getCallState() ) );;
    printf("\t** CCV: Protocol has%sbeen started\n ", 
                 pccv->bprotocolStarted ? " " : " not " );
    if ( getCallState() == CCV_PLAYING )
        printf("\t** CCV: Playing voice file %s\n", pccv->voiceFile );

    if ( getSwiHandle() != NULL_CTAHD )
        printf("\t** CCV: Switch on board %d is OPENED\n", gArgs.board );
    else
        printf("\t** CCV: Switch on board %d is CLOSED\n", gArgs.board );

}
/******************************************************************************
* 
* THREAD: monitorTrunks
*
*   This thread is responsible for using the DTM service to show trunks status
*   information.
******************************************************************************/
THREAD_RET_TYPE THREAD_CALLING_CONVENTION monitorTrunks( void *arg )

{
    CTA_EVENT event = { 0 };
    DWORD     ret = SUCCESS;
    BOOL      bnewCmdEvent = FALSE;
    DTMHD     dtmhd;
    unsigned  brd, trunk, *pbuf;
    DTM_TRUNK_STATUS  status;
    DTM_TRUNK        *ptrunk;
    DTM_START_PARMS   gstartparms = {0};

    CTA_THRD_INFO *pctaInfo = &ctaThrdData[ DTM_THRD_IDX ];
    
    printf("Thread started: monitorTrunks\n");
    strcpy ( pctaInfo->ctxName, DTM_CNTX_NAME );
 
    /* Setup NA information required for this thread. This thread will
    *  be using the Digital Trunk monitoring service. 
    */
    pctaInfo->svcMgrList[0] = gsvcMgrList[ SETUP_IDX_DTM_MGR ]; 

    memcpy( &pctaInfo->svcDescriptions[0],
            &gsvcDescriptions[ SETUP_IDX_DTM ], sizeof(CTA_SERVICE_DESC) );
    
    pctaInfo->svcNames[0] = gsvcNames[ SETUP_IDX_DTM ];
    pctaInfo->numSvcs = 1;
    pctaInfo->ctaqueuehd = NULL_CTAQUEUEHD;
    
    /* Set up number of events per second and the mask
    *  for what events to report (all).
    */
    ctaGetParms( pctaInfo->ctahd, DTM_START_PARMID, 
                 &gstartparms, sizeof gstartparms);
    gstartparms.maxevents   = 1;    
    gstartparms.reportmask  = 0x7F;

    /* The context may or may not be running on the same host. It may be
    *  on a CG6000eC board. 
    */
    for (;;)
    {
        if ( !pctaInfo->bctxReady)
        {
            /* Need to have a context on the server */
            if ( (ret = setupCtaOnThread( pctaInfo, gArgs.svrHost ))
                != SUCCESS )
                /* Sleep a bit and try again later */
                DemoSleep( 1 );
            else
                /* Now open services */
                setupCtaServices( pctaInfo );
        }
        else if ( !pctaInfo->bsvcReady )
        {
            setupCtaServices( pctaInfo );
            if( pctaInfo->bsvcReady )
            {
                printf("DTM: Services reopened.\n");
            }
            else
                /* Sleep a bit before trying to reopen the service */
                DemoSleep( 1 );
        }
        else
        {        
            int ctaFreeBuf;
            DWORD dtmRet = SUCCESS;

            if ( !bnewCmdEvent )
            {
                /*************************************
                * WAIT for event to be sent to QUEUE *
                *************************************/
                myWaitEvent( pctaInfo, &event, &ctaFreeBuf, DTM_WAIT_TMO );
            }

            switch ( event.id )
            {
                case APPEVN_DTM_START:
                    /* Only start monitoring from primary instance of 
                    *  takeover. The backup will get the STARTED event.
                    */
                    if ( getRunMode() == TOVR_MODE_PRIMARY )
                    {
                        printf("\tCMD: Start trunk monitor\n");
                        /* Retrieve board and trunk numbers passed in event
                        *  buffer.
                        */
                        pbuf = (unsigned *) event.buffer;
                        brd = *pbuf++;
                        trunk = *pbuf;

                        dtmRet = dtmStartTrunkMonitor( pctaInfo->ctahd, brd,
                                                 trunk, &dtmhd, &gstartparms );

                        if ( dtmRet != SUCCESS )
                            DemoShowError("dtmStartTrunkMonitor failed",dtmRet);
                        else
                        {
                            myDtmAdd( brd, trunk, dtmhd );
                            OK( "dtmStartTrunkMonitor" );
                        }
                    }
                    break;

                case APPEVN_DTM_STOP:

                    /* Only stop monitoring from primary instance of 
                    *  takeover. The backup will get the DONE event.
                    */
                    if ( getRunMode() == TOVR_MODE_PRIMARY )
                    {
                        printf("\tCMD: Stop trunk monitor\n");
                        /* Retrieve board and trunk numbers passed in event
                        *  buffer.
                        */
                        pbuf = (unsigned *) event.buffer;
                        brd = *pbuf++;
                        trunk = *pbuf;

                        if ( ( dtmhd = myDtmFindHandle( brd, trunk ) ) == NULL_CTAHD )
                            printf("DTM: Warning - not monitoring Board %d Trunk %d\n",
                                    brd, trunk );
                        else
                        {
                            if ( (dtmRet = dtmStopTrunkMonitor( dtmhd ) ) 
                                != SUCCESS )
                                DemoShowError( "dtmStopTrunkMonitor failed", 
                                                dtmRet );
                            else
                            {
                                myDtmRemove( dtmhd );
                                OK( "dtmStopTrunkMonitor" );
                            }
                        }
                    }
                    break;

                case APPEVN_DTM_GETSTATUS:
                    /* Retrieve board and trunk numbers passed in event
                    *  buffer.
                    */
                    printf("\tCMD: Get status request.\n");
                    pbuf = (unsigned *) event.buffer;
                    brd = *pbuf++;
                    trunk = *pbuf;

                    if ( ( dtmhd = myDtmFindHandle( brd, trunk ) ) == NULL_CTAHD )
                        printf("DTM: Warning - not monitoring Board %d Trunk %d\n",
                                brd, trunk );
                    else                        
                    {
                        dtmRet = dtmGetTrunkStatus( dtmhd, &status, 
                                                    sizeof( DTM_TRUNK_STATUS ));
                        if ( dtmRet != SUCCESS )
                        {
                            DemoShowError( "DTM: dtmGetTrunkStatus failed", dtmRet );
                        }
                        else
                            myDtmShowStatus( &status );

                    }
                    break;
                
                case DTMEVN_MONITOR_DONE:

                    /* The monitoring of a trunk has been stopped. Remove it
                    *  from our bookkeekping. 
                    */
                    if ( getRunMode() == TOVR_MODE_BACKUP )
                    {
                        if ( event.value == CTA_REASON_STOPPED )
                        {
                            myDtmRemove( event.objHd );
                        }
                    }
                    else
                    {
                        /* When the primary goes to BACKUP, the trunk
                        *  monitoring is stopped and the trunk entry 
                        *  deleted. If we still have a trunk record, restart
                        *  the monitoring function because we've just 
                        *  transitioned from backup to primary.
                        */
                        if ( ptrunk = myDtmFindTrunk( event.objHd ) )
                        {

                            dtmRet = dtmStartTrunkMonitor( pctaInfo->ctahd,
                                               ptrunk->board, ptrunk->trunk,
                                              &ptrunk->dtmhd, &gstartparms );

                            if (dtmRet != SUCCESS )
                                 DemoShowError( "dtmStartTrunkMonitor failed", 
                                                 dtmRet );
                        }
                        else if ( event.value != CTA_REASON_STOPPED )
                            DemoShowError( "DTM: Event reports last function failed",
                                            event.value);
                    }
                    break;

                case DTMEVN_MONITOR_STARTED:

                    if ( getRunMode() == TOVR_MODE_BACKUP )
                    {
                        printf("\tDTM: Using event's DTMHD to get status...");
                        dtmRet = dtmGetTrunkStatus( event.objHd, &status, 
                                                    sizeof( DTM_TRUNK_STATUS ));
                        if ( dtmRet != SUCCESS )
                        {
                            DemoShowError( "DTM: dtmGetTrunkStatus failed", dtmRet );
                        }
                        else
                        {
                            myDtmAdd( status.board, status.trunk, event.objHd );
                            printf("board %d trunk %d\n", status.board, 
                                                          status.trunk );
                            OK( "dtmGetTrunkStatus" );
                        }
                    }
                    break;

                case DTMEVN_TRUNK_STATUS:
                
                    if ( getRunMode() == TOVR_MODE_PRIMARY )
                    {
                        if ( ptrunk = myDtmFindTrunk( event.objHd ) )
                            myDtmShowState( ptrunk->board, ptrunk->trunk,
                                           (DTM_TRUNK_STATE *)&event.value );
                    }
                    break;
                
                case APPEVN_XCHG_DTM_DATA:
                    /* We've just received current active set of data from the 
                    *  primary. 
                    */
                    if ( getRunMode() == TOVR_MODE_BACKUP )
                        myDtmResetFromPrimary( &event, pctaInfo );

                    break;
            
                case APPEVN_BOARD_STOPPED:
                    if ( getRunMode() == TOVR_MODE_PRIMARY )
                    {
                        /* Board is going to be stopped; stop trunk monitoring
                        *  sessions.
                        */
                        if ( !event.size )
                            /* All boards being stopped. */
                            myDtmIdle();
                        else
                            /* Just one of them is being stopped. */
                            myDtmStopBoard( event.value );
                    }
                    break;

                default:
                    break;
            }
        
            /* Release resources owned by Natural Access */
            if ( ctaFreeBuf )
            {
                ctaFreeBuffer( event.buffer );
                ctaFreeBuf = 0;
            }
            else if ( bnewCmdEvent )
            {
                /* Free up locally malloc'd buffer */
                bnewCmdEvent = FALSE;
                event.id = 0;
                if ( event.buffer )
                    free( event.buffer );
            }
            /******************************************************************
            *
            * Check for USER commands that could change the way we are 
            * operating
            *
            ******************************************************************/
            if ( pctaInfo->bctxReady )
                bnewCmdEvent = getCmd( &event, DTM_THRD_IDX );
        }
    }

    UNREACHED_STATEMENT( THREAD_RETURN; )
}
/******************************************************************************
* myDtmAdd
*
*   Adds book-keeping for trunk being monitored to general status structure 
*   for entire process.
******************************************************************************/
void myDtmAdd( unsigned brd, unsigned trunk, DTMHD dtmhd )
{
    DTM_TRUNK *pnewTrunk;
 
    pnewTrunk = (DTM_TRUNK *) malloc( sizeof(DTM_TRUNK) );
    if ( pnewTrunk )
    {
        /* Add new entry to front of list */
        pnewTrunk->next = gState.listTrunks;
        pnewTrunk->board = brd;
        pnewTrunk->trunk = trunk;
        pnewTrunk->dtmhd = dtmhd;
        
        ENTERCRITSECT();
        gState.listTrunks = pnewTrunk;
        ++gState.numTrunks;
        EXITCRITSECT();
    }
    else
        NO_MEMORY();
}
/******************************************************************************
* myDtmRemove
*
*   Removes book-keeping for trunk being monitored from the general status
*   structure. If "dtmhd" is NULL_CTAHD, then all trunk records are deleted.
******************************************************************************/
void myDtmRemove( DTMHD dtmhd )
{
    BOOL       bFound = FALSE;
    DTM_TRUNK *pcurrTrunk, *ptmpTrunk = NULL;

    ENTERCRITSECT();

    pcurrTrunk = gState.listTrunks;
    while ( pcurrTrunk )
    {
        if ( dtmhd == NULL_CTAHD )
        {
            /* Removing all entries */
            --gState.numTrunks;
            ptmpTrunk = pcurrTrunk->next;
            free ( pcurrTrunk );
            pcurrTrunk = ptmpTrunk;
        }
        else if ( pcurrTrunk->dtmhd == dtmhd )
        {
            if ( !--gState.numTrunks )
                gState.listTrunks = NULL;
            else if ( ptmpTrunk )
                /* Link over hole about to be created in list. */
                ptmpTrunk->next = pcurrTrunk->next;
            else /* We're removing the first entry in the list */
                gState.listTrunks = pcurrTrunk->next;

            printf("DTM: Disabling monitoring of Board %d Trunk %d\n", 
                    pcurrTrunk->board, pcurrTrunk->trunk );

            free( pcurrTrunk );
            bFound = TRUE;
            break;
        }
        else
        {
            /* go onto next entry in list */
            ptmpTrunk = pcurrTrunk;
            pcurrTrunk = ptmpTrunk->next;
        }
    }
    
    EXITCRITSECT();
    if ( !bFound && (dtmhd != NULL_CTAHD) )
        printf("DTM Warning: unknown handle encountered - 0x%x", dtmhd );
}
/******************************************************************************
* myDtmFindHandle
*
*   Retrieve DTM handle for board:trunk being monitored.
*******************************************************************************/
DTMHD myDtmFindHandle( unsigned brd, unsigned trunk )
{
    BOOL       bFound = FALSE;
    DTM_TRUNK *pcurrTrunk = gState.listTrunks, *ptmp;

    while ( pcurrTrunk )
    {
        if (   ( pcurrTrunk->board == brd )
            && ( pcurrTrunk->trunk == trunk ) )
        {
            /* Found it */
            return pcurrTrunk->dtmhd;
        }
        else
        {
            /* go onto next entry in list */
            ptmp = pcurrTrunk;
            pcurrTrunk = ptmp->next;
        }
    }
    return NULL_CTAHD;
}
/******************************************************************************
* myDtmFindTrunk
*
*   Find trunk record for DTM handle.
*******************************************************************************/
DTM_TRUNK *myDtmFindTrunk( DTMHD dtmhd )
{
    DTM_TRUNK *pcurrTrunk = gState.listTrunks, *ptmp;

    while ( pcurrTrunk )
    {
        if ( pcurrTrunk->dtmhd == dtmhd )
        {
            /* Found it */
            return pcurrTrunk;
        }
        else
        {
            /* go onto next entry in list */
            ptmp = pcurrTrunk;
            pcurrTrunk = ptmp->next;
        }
    }
    return NULL;
}
/******************************************************************************
* myDtmShowStatus
*
*   Interpret and display the trunk status information.
*******************************************************************************/
void myDtmShowStatus ( DTM_TRUNK_STATUS *status )
{
    myDtmShowState( status->board, status->trunk, &status->state );
}
/******************************************************************************
* myDtmShowState
*
*   Interpret and display the trunk state information.
*******************************************************************************/
void myDtmShowState ( DWORD board, DWORD trunk, DTM_TRUNK_STATE *state )
{
    char       alarm_text[100] ;
    time_t     timeval;
    struct tm *t;

    if ( state->alarms == 0)
        strcpy (alarm_text, "In service");
    else
    {
        alarm_text[0] = '\0';
        if ( state->alarms & DTM_ALARM_LOF)
            strcat (alarm_text, "Loss of frame ");
        if ( state->alarms & DTM_ALARM_AIS)
            strcat (alarm_text, "All 1's alarm ");
        if ( state->alarms & DTM_ALARM_FAR_END )
            strcat (alarm_text, "Far end alarm indication ");

        if ( state->alarms & DTM_ALARM_FAR_LOMF)
            strcat (alarm_text, "Far end loss of multiframe ");
        else if ( state->alarms & DTM_ALARM_TS16AIS )
            strcat (alarm_text, "Time slot 16 AIS ");
    }
    timeval = time(NULL);
    t = localtime (&timeval);
    printf("%02d:%02d:%02d  Board %d Trunk %d: %s\n",
                                  t->tm_hour, t->tm_min, t->tm_sec,
                                  board, trunk,
                                  alarm_text);
}
/******************************************************************************
* myDtmIdle
*
*       Stops trunk monitoring and removes all book-keeping.
*
* Note: Called from within critical section.
*******************************************************************************/
void myDtmIdle()
{
    DWORD dtmRet = SUCCESS;

    DTM_TRUNK *pcurrTrunk = gState.listTrunks, *ptmp;

    while ( pcurrTrunk )
    {
        /* Stop monitoring the trunk. If this isn't done, then
        *  restart of the monitor will fail upon transitioning to
        *  to backup.
        */
        if ( (dtmRet = dtmStopTrunkMonitor( pcurrTrunk->dtmhd ) ) 
              != SUCCESS )
             DemoShowError( "dtmStopTrunkMonitor failed", 
                                                dtmRet );
        else
            printf("DTM: Monitoring stopped for board %d trunk %d\n",
                    pcurrTrunk->board, pcurrTrunk->trunk );

        /* go onto next entry in list */
        ptmp = pcurrTrunk;
        pcurrTrunk = ptmp->next;
    }
    /* Delete active list */
    myDtmRemove( NULL_CTAHD );
    gState.listTrunks = NULL;
}
/******************************************************************************
* myDtmStopBoard
*
*       Stops trunk monitoring for one board.
*******************************************************************************/
void myDtmStopBoard( unsigned brd )
{
    DWORD dtmRet = SUCCESS;
    DTMHD dtmhd;
    BOOL  bremoveEntry = FALSE;

    DTM_TRUNK *pcurrTrunk = gState.listTrunks, *ptmp;

    while ( pcurrTrunk )
    {
        if ( bremoveEntry = (brd == pcurrTrunk->board) )
        {
            dtmhd = pcurrTrunk->dtmhd;
            /* Stop monitoring the trunk. If this isn't done, then
            *  restart of the monitor will fail upon transitioning to
            *  to backup.
            */
            if ( (dtmRet = dtmStopTrunkMonitor( dtmhd ) ) 
                  != SUCCESS )
                DemoShowError( "dtmStopTrunkMonitor failed", 
                                                    dtmRet );
            else
                printf("DTM: Monitoring stopped for board %d trunk %d\n",
                        pcurrTrunk->board, pcurrTrunk->trunk );
        
        }
        /* go onto next entry in list and then remove the current entry*/
        ptmp = pcurrTrunk;
        pcurrTrunk = ptmp->next;
        if ( bremoveEntry )
            myDtmRemove( dtmhd );
    }
}
/******************************************************************************
* myDtmShow
*
*   List trunks being monitored.
*******************************************************************************/
void myDtmShow()
{
    DWORD dtmRet = SUCCESS;

    DTM_TRUNK *pcurrTrunk = gState.listTrunks, *ptmp;

    if ( !gState.numTrunks )
        printf("\t** DTM: No trunks are currently being monitored.\n");
    else
    {
        printf("\t** DTM: %d trunk%c currently being monitored:\n", 
               gState.numTrunks, (gState.numTrunks >1 ? 's' : ' ') );
        while ( pcurrTrunk )
        {
            printf("\t\tBoard %d -- trunk %d\n", pcurrTrunk->board, 
                                               pcurrTrunk->trunk );
            ptmp = pcurrTrunk;
            pcurrTrunk = ptmp->next;
        }
    }
}
/******************************************************************************
* myDtmStateData
*
*   Places in buffer all trunks actively 
*   being monitored by DTM. Format:
*
*   DWORD numTrunks     // = n
*   DWORD board#        // 0
*   DWORD trunk#        // 0
*   ...
*   DWORD board#        // n-1
*   DWORD trunk#        // n-1
*
*******************************************************************************/
DWORD myDtmStateData( char *pbuf, DWORD bufSize )
{
    DWORD size = 0;
    DWORD *pdata = (DWORD *)pbuf;
    DTM_TRUNK *pcurrTrunk = gState.listTrunks, *ptmp;

    memset( pbuf,'\0', bufSize );
    ENTERCRITSECT();
    if ( ( size = (gState.numTrunks * sizeof(DWORD) * 2) + sizeof(DWORD)) 
        <= bufSize )
    {
        /* Will fit in buffer */
        *pdata++ = gState.numTrunks;

        while( pcurrTrunk )
        {
            *pdata++ = pcurrTrunk->board;
            *pdata++ = pcurrTrunk->trunk;
            
            /* go onto next entry in list */
            ptmp = pcurrTrunk;
            pcurrTrunk = ptmp->next;
        }
    }
    else
    {
        printf("DTM: Warning -- Insufficient buffer space for data transfer.\n");
        size = 0;
    }
    EXITCRITSECT();

    return size;
}

/******************************************************************************
* myDtmResetFromPrimary
*
*   Data in buffer for all trunks being monitored by DTM. See myDtmStateData() 
*   for format.
*******************************************************************************/
void myDtmResetFromPrimary( CTA_EVENT *pevent, CTA_THRD_INFO *pctaInfo )
{
    int       num, i;
    unsigned  brd, trunk;
    DTMHD     dtmhd;
    DWORD     dtmRet;
    DWORD    *pdata = (DWORD *) pevent->buffer;
    int       retries = 0;

    /* Number of trunks primary is currently monitoring */
    num = *pdata++;
    for ( i = 0; i < num ; i++ )
    {
        brd = *pdata++;
        trunk = *pdata++;

        /* Attach to the existing DTM service object created by the 
        *  primary.
        */
        do
        {
            dtmRet = dtmAttachTrunkMonitor( pctaInfo->ctahd, brd, trunk, 
                                            &dtmhd );
            if ( dtmRet != SUCCESS )
            {
                /* After switching to BACKUP, there's a window during which
                *  the new PRIMARY has not yet restarted the trunk 
                *  monitoring. The window occurs because the primary 
                *  on going to backup stops the monitoring.
                */

                if ( retries++ >= 3 )
                {
                    DemoShowError("DTM RESET: dtmAttachTrunkMonitor failed",
                                  dtmRet);
                    break;
                }
            }
            else
            {
                myDtmAdd( brd, trunk, dtmhd );
                printf("DTM RESET: Attached to board %d trunk %d\n", brd, trunk ); 
            }
        } while ( dtmRet != SUCCESS );
    }
}
/******************************************************************************
* 
* THREAD: manageBoards
*
*       This thread uses OAM to manage the stopping and starting of boards.
*
******************************************************************************/
THREAD_RET_TYPE THREAD_CALLING_CONVENTION manageBoards( void *arg )
{
    CTA_EVENT event = { 0 };
    DWORD     ret = SUCCESS;
    BOOL      bnewCmdEvent = FALSE;
    char      value[ MAX_KW_VALUE_SIZE ];
    
    CTA_THRD_INFO *pctaInfo = &ctaThrdData[ OAM_THRD_IDX ];
  
    printf("Thread started: manageBoards\n");
    strcpy ( pctaInfo->ctxName, OAM_CNTX_NAME );
 
    /* Setup NA information required for this thread. This thread will
    *  be using the OAM service.
    */
    pctaInfo->svcMgrList[0] = gsvcMgrList[ SETUP_IDX_OAM_MGR ]; 
    memcpy( &pctaInfo->svcDescriptions[0],
            &gsvcDescriptions[ SETUP_IDX_OAM ], sizeof(CTA_SERVICE_DESC) );
    pctaInfo->svcNames[0] = gsvcNames[ SETUP_IDX_OAM ];
    pctaInfo->numSvcs = 1;
    pctaInfo->ctaqueuehd = NULL_CTAQUEUEHD;

    for (;;)
    {
        if ( !pctaInfo->bctxReady)
        {
            /* Need to have a context on the server */
            if ( (ret = setupCtaOnThread( pctaInfo, gArgs.svrHost ))
                != SUCCESS )
                /* Sleep a bit and try again later */
                DemoSleep( 1 );
            else
            {
                /* Now open services */
                setupCtaServices( pctaInfo );
                /* Register to receive OAM events */
                if ( !pctaInfo->battached )
                {
                    printf("OAM: Registering to receive OAM alerts.\n");
                    if ( (ret = oamAlertRegister( pctaInfo->ctahd )) != SUCCESS )
                        printf("OAM: Unable to register for alerts 0x%x\n", ret);
                }
            }
        }
        else
        {        
            int   ctaFreeBuf = 0;
            DWORD oamRet = SUCCESS;
  
            if ( !bnewCmdEvent )
                /*************************************
                * WAIT for event to be sent to QUEUE *
                *************************************/
                myWaitEvent( pctaInfo, &event, &ctaFreeBuf, 
                             OAM_WAIT_TMO );

            if ( event.id == APPEVN_OAM_BOARD_STATE )
            {
                /* Determine whether or not the board passed as a
                *  startup argument is running.
                */
                if ( myOamGetKW( pctaInfo, NULL, gArgs.board, "State", value, 
                                 MAX_KW_VALUE_SIZE )  == SUCCESS )
                {
                    if ( myCompare( value, "BOOTED" ) )
                        setBoardState( BRD_BOOTED );
                    else
                        setBoardState( BRD_IDLE );
                }
                else
                    setBoardState( BRD_ERROR );
            }
            else if ( event.id ==  CLKEVN_CONFIGURED )
            {
                unsigned brdNumber;
                /* A board that has been started is now ready to use because
                *  its clocks have been configured. If its the one we're using
                *  for the CCV thread, mark the board booted.
                */
                if( ( myOamBrdNum( pctaInfo, (int*)&brdNumber, (OAM_MSG *)event.buffer )
                      == SUCCESS )
                    && ( brdNumber == gArgs.board ) )
                {
                    setBoardStopped( FALSE );
                    setBoardState( BRD_BOOTED );   
                }
            }
            else if ( event.id == HSWEVN_REMOVAL_REQUESTED )
            {
                unsigned brdNumber;
                /* The handles on a board have been opened so that it can be
                *  extracted. Release resources associated with the board.
                */
                if( ( myOamBrdNum( pctaInfo, (int*)&brdNumber, (OAM_MSG *)event.buffer )
                      == SUCCESS )
                    && ( brdNumber == gArgs.board ) )
                {
                    CTA_EVENT tellEvt = {0};

                    tellEvt.id = APPEVN_BOARD_STOPPED;
                    tellEvt.size = event.size;
                    tellEvt.value = brdNumber;
                    
                    /* Close ADI/NCC/DTM services using this board. */
                    setCmd(&tellEvt, CCV_THRD_IDX);
                    setCmd(&tellEvt, DTM_THRD_IDX);

                    setBoardStopped( TRUE );
                    setBoardState( BRD_UNKNOWN );
                }
            }
            else if ( event.id == APPEVN_OAM_STOP )
            {
                CTA_EVENT tellEvt = {0};
                DWORD brdNum = 0;
                
                /* Board(s) are goingto be stopped. Release resources */
                tellEvt.id = APPEVN_BOARD_STOPPED;
                if ( event.size )
                {
                    /* One specific board is being stopped. Determine
                    *  what its number is. If call is unsuccessfull,
                    *  will stop all boards.
                    */
                    if ( oamBoardGetNumber ( pctaInfo->ctahd, 
                                             (char*) event.buffer, &brdNum )
                         == SUCCESS )
                    {
                        tellEvt.size = event.size;
                        tellEvt.value = brdNum;
                    }                            
                }    
                /* Close ADI/NCC/DTM services using this board. 
                *  Wait a bit to allow it to be completed.
                */
                setCmd(&tellEvt, CCV_THRD_IDX);
                setCmd(&tellEvt, DTM_THRD_IDX);
                DemoSleep( 4 );      
              
                if ( getRunMode() == TOVR_MODE_PRIMARY )
                {
                    if ( event.size )
                    {
                        printf("\tCMD: Stop board %s\n", event.buffer);
                        oamRet = oamStopBoard( pctaInfo->ctahd, (char*)event.buffer );
                    }
                    else
                    {
                        /* Start all boards */
                        oamRet = oamStopBoard( pctaInfo->ctahd, NULL );
                        printf("\tCMD: Stop all board\n");
                    }
                    if ( oamRet != SUCCESS )
                        DemoShowError( "oamStopBoard failed", oamRet );
                    else
                        OK( "oamStopBoard" );
                }
            }
            else if ( getRunMode() == TOVR_MODE_PRIMARY )
            {
                /**********
                * PRIMARY *
                ***********/
                switch ( event.id )
                {
                    case CTAEVN_WAIT_TIMEOUT:
                        /* Process OAM records */
                        if ( gState.numOamData )
                            showOamData( pctaInfo );

                        break;

                    case APPEVN_OAM_START:
                        if ( event.size )
                        {
                            oamRet = oamStartBoard( pctaInfo->ctahd, 
                                                    (char*)event.buffer );
                            printf("\tCMD: Start board %s\n", event.buffer);
                        }
                        else
                        {
                            /* Start all boards */
                            oamRet = oamStartBoard( pctaInfo->ctahd, 
                                                    NULL );
                            printf("\tCMD: Start all boards\n");
                        }
                        if ( oamRet != SUCCESS )
                            DemoShowError( "oamStartBoard failed", oamRet );
                        else
                            OK( "oamStartBoard" );

                        break;

                    case APPEVN_OAM_GET_KW:
                    {
                        /* Get OAM keywords from objects. */
                        OAM_CMD_KW *pkwParams = (OAM_CMD_KW *) event.buffer;

                        printf("\tCMD: Get OAM keyword \"%s\" from %s \n", 
                                pkwParams->keyWord, pkwParams->objName );

                        if ( myOamGetKW( pctaInfo, pkwParams->objName, 0, 
                                         pkwParams->keyWord, value, 
                                         MAX_KW_VALUE_SIZE )  == SUCCESS )
                        {
                            printf("OAM: %s's value of \"%s\" is %s\n", 
                                pkwParams->objName, pkwParams->keyWord, value );
                            /* If periodic gets have been requested, add it to
                            *  the list of OAM keywords that need to be 
                            *  retrieved.
                            */
                            if ( --pkwParams->numGets > 0 )
                            {
                                DWORD entryID;

                                entryID = myOamAdd( pkwParams, NULL );
                                /* Tell peer about new entry */
                                myOamSendToPeer( entryID, APPEVN_OAM_ADD );
                            }
                        }
                        break;
                    }

                    case APPEVN_OAM_GET_STOP:
                    {
                        /* Stop get of OAM keywords  */
                        OAM_CMD_KW *pkwParams = (OAM_CMD_KW *) event.buffer;
                        OAM_DATA   *pentry; 

                        printf("\tCMD: Stop gets of OAM keyword \"%s\" from %s \n", 
                                pkwParams->keyWord, pkwParams->objName );
                        
                        if ( (pentry = myOamFind( pkwParams->keyWord, 
                             pkwParams->objName )) != NULL )
                        {
                            DWORD id = pentry->reqID;
                            myOamRemove( pentry );
                            /* Tell peer entry has been removed. */
                            myOamSendToPeer( id, APPEVN_OAM_REMOVE );
                        }
                        else
                            printf("OAM: Can't find OAM keyword %s, %s \n", 
                                   pkwParams->objName, pkwParams->keyWord );
                        break;
                    }
                    default:
                        break;
                }
            }
            else
            {
                /**********/
                /* BACKUP */
                /**********/
                switch ( event.id )
                {
                    case APPEVN_XCHG_OAM_DATA:

                        /* Save OAM data sent from primary */
                        myOamResetFromPrimary( &event, pctaInfo );
                        break;

                    case APPEVN_OAM_ADD:
                        /* The primary has added a new entry to its list */
                        myOamAdd( NULL, (OAM_DATA *)event.buffer );
                        break;

                    case APPEVN_OAM_REMOVE:
                    case APPEVN_OAM_UPDATE:
                    {
                        OAM_DATA *pentry;
                        /* The primary has deleted an entry from its list or
                        *  decremented the number of remaining loops.
                        *  The "size" field contains the entry ID.
                        */
                        if ( pentry = myOamFindEntry( event.size ) )
                        {
                            if ( event.id == APPEVN_OAM_REMOVE )
                                myOamRemove( pentry );
                            else
                                --pentry->kwReq.numGets;
                        }
                        else if ( gArgs.bVerbose )
                            /* During a state transition, an update may be 
                            *  received before the entry has been added.
                            */
                            printf("OAM: Unable to find entry %d.\n",event.size);
                        break;
                    }
                    default:
                        break;

                }
            }
            /* Release resources owned by Natural Access */
            if ( ctaFreeBuf )
            {
                ctaFreeBuffer( event.buffer );
                ctaFreeBuf = 0;
            }
            else if ( bnewCmdEvent )
            {
                /* Free up locally malloc'd buffer */
                bnewCmdEvent = FALSE;
                event.id = 0;
                if ( event.buffer )
                    free( event.buffer );
            }

            /******************************************************************
            * Check for USER commands that would change the way we are 
            * operating
            ******************************************************************/
            if ( pctaInfo->bctxReady )
                bnewCmdEvent = getCmd( &event, OAM_THRD_IDX );
        }
    }
    UNREACHED_STATEMENT( THREAD_RETURN; )
}
/*******************************************************************************
* openObject
*
*       This function opens the OAM managed  object.
*
*       If the caller has provided a name, this is used for doing the open. 
*       If instead a board number is provided, this number is used to lookup
*       the board's name prior to performing the open.  
*******************************************************************************/
HMOBJ myOamOpenObject ( char *pobjName, DWORD boardNumber, 
                     CTA_THRD_INFO *pctaInfo )
{
    HMOBJ oamhd;
    DWORD ret = SUCCESS;
    char  name[ MAX_OBJ_NAME_SIZE ];
    
    if ( !pobjName  )
    {
        /* Don't have a name; need to find it in the database based on a 
        *  board number.
        */
        ret = oamBoardLookupByNumber( pctaInfo->ctahd, boardNumber, 
                                      name, MAX_OBJ_NAME_SIZE );
        if ( ret != SUCCESS )
        {
            char txt[ 80 ];
            sprintf( txt, "%s [0x%x]: oamBoardLookupByNumber failed for board %d", 
                     pctaInfo->ctxName, pctaInfo->ctahd, boardNumber );
            DemoShowError( txt, ret );
            return NULL_CTAHD;
        }
        else if ( !*gArgs.brdName && (boardNumber == gArgs.board) )
        {
            /* Save the board's name, since we've found it... */
            strcpy( gArgs.brdName, name );
        }
    }
    else
        strcpy( name, pobjName );
    
    /* Get a handle to the managed object (the board) */
    if ( ( ret = oamOpenObject( pctaInfo->ctahd, name, &oamhd, 
                                OAM_READONLY ) )
              != SUCCESS )
    {
        DemoShowError( "oamOpenObject failed", ret );
        return NULL_CTAHD;
    }

    return oamhd;
}
/******************************************************************************
* myOamBrdNum()
*
*    Get a board's OAM number based on it's name which has been passed in the
*    buffer of the CTA_EVENT, containing the OAM message. 
******************************************************************************/
DWORD myOamBrdNum( CTA_THRD_INFO *pctaInfo, int *pBrdNum, OAM_MSG *pOamMsg )
{
    DWORD ret = CTAERR_BAD_ARGUMENT;
    char *pBrdName;
    
    if ( pOamMsg )
    {
        /* Get offset to name string of managed object */
        pBrdName = (char*)pOamMsg + pOamMsg->dwOfsSzName;
            
        /* Find the board's OAM number based upon it's board name */
        ret = oamBoardGetNumber( pctaInfo->ctahd, (const char *) pBrdName, 
                                 (DWORD *) pBrdNum );
    }
    return ret;
}
/*******************************************************************************
*  myOamGetKW
*
*      This function opens the OAM object and retrieves a keyword value.
*******************************************************************************/
DWORD myOamGetKW( CTA_THRD_INFO *pctaInfo, char *pName, DWORD brd, char *pKW, 
                 char *value, int size ) 
{
    DWORD ret = SUCCESS;
    HMOBJ oamhd;

    memset ( value, '\0', size );
    /* Open the OAM object */
    if ( (oamhd = myOamOpenObject( pName, brd, pctaInfo  )) != NULL_CTAHD )
    {
        /* Ket the keyword's value */
        ret = oamGetKeyword( oamhd, pKW, value, size );
        if ( ret != SUCCESS )
            DemoShowError( "oamGetKeyword failed", ret );

        oamCloseObject( oamhd );
    }
    else
        return CTAERR_NOT_FOUND;

    return ret;
}
/******************************************************************************
* myOamAdd
*
*   Adds persistent OAM data to general status structure of process.
*******************************************************************************/
DWORD myOamAdd( OAM_CMD_KW *poamData, OAM_DATA *pfromPeer )
{
    OAM_DATA *pnewOam;

    ENTERCRITSECT();
    pnewOam = (OAM_DATA *) malloc( sizeof(OAM_DATA) );
    if ( pnewOam )
    {
        if( pfromPeer )
        {
            /* Complete entry has been sent from the peer */
            memcpy( pnewOam, pfromPeer, sizeof( OAM_DATA ));
        }
        else
        {
            memcpy( &pnewOam->kwReq, poamData, sizeof( OAM_CMD_KW ));
            /* Set number of ctaWaitTimeouts that have to occur before
            *  next get is performed.
            */
            pnewOam->reqID = ++gState.reqID;
            pnewOam->numTicks = OAM_TICKS_IN_SECOND * poamData->howOften ;
            pnewOam->currTicks = pnewOam->numTicks;
        }
        
        /* Add new OAM entry to front of list */
        pnewOam->next = gState.listOam;

        gState.listOam = pnewOam;
        ++gState.numOamData;
    }
    else
        NO_MEMORY();
    EXITCRITSECT();
    return ( pnewOam->reqID );
}
/******************************************************************************
* myOamRemove
*
*   Removes book-keeping for OAM data being maintained in general status
*   structure.
*******************************************************************************/
void myOamRemove( OAM_DATA *poamEntry )
{
    OAM_DATA *pcurrOAM, *ptmpOAM = NULL;

    ENTERCRITSECT();
    pcurrOAM = gState.listOam;

    while ( pcurrOAM )
    {
        if ( pcurrOAM == poamEntry )
        {
            if ( !--gState.numOamData )
                gState.listOam = NULL;
            else if ( ptmpOAM )
                /* Link over hole about to be created in list. */
                ptmpOAM->next = pcurrOAM->next;
            else /* We're removing the first entry in the list */
                gState.listOam = pcurrOAM->next;

            printf("OAM: Disabling get of %s's keyword \"%s\"\n", 
                    pcurrOAM->kwReq.objName, pcurrOAM->kwReq.keyWord );

            free( pcurrOAM );
            break;
        }
        else
        {
            /* go onto next entry in list */
            ptmpOAM = pcurrOAM;
            pcurrOAM = ptmpOAM->next;
        }
    }
    EXITCRITSECT();
}
/******************************************************************************
* myOamFind
*
*   Removes book-keeping for OAM data being maintained in general status
*   structure.
*******************************************************************************/
OAM_DATA *myOamFind( char *keyWord, char *objectName )
{
    OAM_DATA *pcurrOAM, *ptmpOAM = NULL;

    pcurrOAM = gState.listOam;

    while ( pcurrOAM )
    {
        if (    !strcmp( pcurrOAM->kwReq.keyWord, keyWord )
             && !strcmp( pcurrOAM->kwReq.objName, objectName ) )
        {
            /* Matching entry */
            break;
        }
        else
        {
            /* go onto next entry in list */
            ptmpOAM = pcurrOAM;
            pcurrOAM = ptmpOAM->next;
        }
    }

    return pcurrOAM;
}
/******************************************************************************
* showOamData
*
*   Display OAM data for each record in status structure of process.
******************************************************************************/
void showOamData( CTA_THRD_INFO *pctaInfo )
{
    OAM_DATA   *pcurrOam  = gState.listOam, *ptmpOam;
    OAM_CMD_KW *preq;
    char        value[ MAX_KW_VALUE_SIZE ];
    
    while ( pcurrOam )
    {
        ptmpOam = pcurrOam->next;
        if ( !--pcurrOam->currTicks )
        {
            /* Time to do a get on the OAM keyword */
            preq = &pcurrOam->kwReq;
            if ( myOamGetKW( pctaInfo, preq->objName, 0, preq->keyWord, value, 
                             MAX_KW_VALUE_SIZE )
                 == SUCCESS )
            {
                printf("OAM [%d]: %s's value of \"%s\" is %s\n", preq->numGets, 
                        preq->objName, preq->keyWord, value );                
                
            }
            /* Decrement the number of OAM gets remaining */         
            if ( --preq->numGets <= 0 )
            {
                DWORD id = pcurrOam->reqID;
                myOamRemove( pcurrOam );
                /* Tell peer entry has been removed. */
                myOamSendToPeer( id, APPEVN_OAM_REMOVE );            
            }
            else
            {    
                /* Reset count down value for when to do next OAM get */
                pcurrOam->currTicks = pcurrOam->numTicks;
                /* Tell peer to decrement number of remaining loops */
                myOamSendToPeer( pcurrOam->reqID, APPEVN_OAM_UPDATE );  
            }
        }
        pcurrOam = ptmpOam;
    }
}
/******************************************************************************
* myOamStateData
*
*   Places in buffer all OAM entries of keyword data being retrieved:
*
*   DWORD     reqID
*   DWORD     numEntries   // = n
*   OAM_DATA  entries[n]  
*******************************************************************************/
DWORD myOamStateData( char *pbuf, DWORD bufSize )
{
    DWORD size = 0;
    DWORD *pdata = (DWORD *)pbuf;
    OAM_DATA *pcurrOam, *ptmp, *pbufOam;

    ENTERCRITSECT();
    pcurrOam = gState.listOam;
    if ( ( size = (gState.numOamData * sizeof(OAM_DATA)) + (2 * sizeof(DWORD))) 
        <= bufSize )
    {
        /* Will fit in buffer */
        *pdata++ = gState.reqID;
        *pdata++ = gState.numOamData;
        pbufOam = (OAM_DATA *) pdata;

        /* Copy each entry into exchange buffer */
        while( pcurrOam )
        {
            memcpy( pbufOam, pcurrOam, sizeof(OAM_DATA) );
            ++pbufOam;
            /* go onto next entry in list */
            ptmp = pcurrOam;
            pcurrOam = ptmp->next;
        }
    }
    else
    {
        printf("OAM: Warning -- Insufficient buffer space for data transfer.\n");
        size = 0;
    }
    EXITCRITSECT();

    return size;
}
/******************************************************************************
* myOamResetFromPrimary
*
*   Info in buffer for all OAM data being being retrieved. See myOamStateData() 
*   for format.
******************************************************************************/
void myOamResetFromPrimary( CTA_EVENT *pevent, CTA_THRD_INFO *pctaInfo )
{
    int       num, i;
    OAM_DATA *pxchgData;
    OAM_DATA *pcurrOam, *ptmpOam;
    DWORD    *pdata = (DWORD *) pevent->buffer;

    ENTERCRITSECT(); 
    if ( pdata )
    {
        gState.reqID = *pdata++;
        /* Number of entries that have been sent in the buffer. */
        if ( num = *pdata++ )
            pxchgData = (OAM_DATA *)pdata;

        /* Create entries for each structure sent from primary */
        gState.numOamData = num;
        ptmpOam = NULL;
        for ( i = 0; i < num ; i++, pxchgData++ )
        {
            pcurrOam = (OAM_DATA *) malloc( sizeof(OAM_DATA) );
            if ( pcurrOam )
            {
                memcpy( pcurrOam, pxchgData, sizeof(OAM_DATA) );
                /* Update "next" linkage, overwriting linkage
                *  sent by the primary.
                */
                pcurrOam->next = ptmpOam;
                ptmpOam = pcurrOam;
            }
            else
                NO_MEMORY();
        }
    }
    /* Save the head of the list */
    gState.listOam = ptmpOam;
    EXITCRITSECT();
}
/******************************************************************************
* myOamShow
*
*   List OAM information being retrieved.
******************************************************************************/
void myOamShow()
{
    OAM_DATA *pcurrOam = gState.listOam, *ptmp;

    if ( !gState.numOamData )
        printf("\t** OAM: No OAM keywords currently being retrieved.\n");
    else
    {
        printf("\t** OAM: %d keyword%c currently being retrieved:\n", 
                gState.numOamData, (gState.numOamData > 1 ? 's' : ' '));
        while ( pcurrOam )
        {
            printf("\t\t%s's keyword %s\n", pcurrOam->kwReq.objName,
                                          pcurrOam->kwReq.keyWord );
            ptmp = pcurrOam;
            pcurrOam = ptmp->next;
        }
    }
}
/******************************************************************************
* myOamSendToPeer
*
*   This function sends an event to the backup to inducate that a change has
*   been made to the OAM data entries.
*******************************************************************************/
void myOamSendToPeer( DWORD entryID, DWORD cmd )
{
    CTA_EVENT event;
    OAM_DATA  evtBuff, *pentry;

    /* Build event that checkpoint thread will send to peer OAM thread.
    */ 
    event.id = APPEVN_XCHG_OAM;
    if ( cmd == APPEVN_OAM_ADD )
    {
        if ( pentry = myOamFindEntry( entryID ) )
        {
            event.size = sizeof( OAM_DATA );
            event.buffer = &evtBuff;
            /* The value will be moved to the event id field by the checkpoint
            *  point thread.
            */
            event.value = cmd;
            memcpy( &evtBuff, pentry, sizeof( OAM_DATA ) );
            /* Ask checkpoint thread to do the exchange */
            setCmd(&event, CHK_THRD_IDX);
        }
        else
            if ( gArgs.bVerbose )
                printf("Unable to find OAM entry\n");
    }
    else
    {
        /* Tell peer which entry to remove or update */
        event.size = entryID;
        event.buffer = NULL;
        event.value = cmd;
        setCmd(&event, CHK_THRD_IDX);
    }
}

/******************************************************************************
* myOamFindEntry
*
*   Returns pointer to requested OAM entry.
*******************************************************************************/
OAM_DATA *myOamFindEntry( DWORD entryID )
{
    OAM_DATA *pcurrOam = gState.listOam, *ptmp;

    while ( pcurrOam )
    {
        if ( pcurrOam->reqID == entryID )
        {
            /* Found it */
            return pcurrOam;
        }
        else
        {
            /* go onto next entry in list */
            ptmp = pcurrOam;
            pcurrOam = ptmp->next;
        }
    }
    return NULL;
}
/******************************************************************************
* 
* main()
*
*   1. Process input arguments
*   2. Initialize with Natural Access
*   3. Start each of the processing threads
*
******************************************************************************/

int main( int argc, char **argv )
{
    CTA_INIT_PARMS  initparms = { 0 };
    DWORD           ret = SUCCESS;
    
    ProcessArgs( argc, argv );
    DemoSetReportLevel( DEMO_REPORT_COMMENTS );
    
    printf( "Natural Access Multi-Server Object Sharing Demo (TAKEOVER) V.%s (%s)\n",
            VERSION_NUM, VERSION_DATE );

    /* Disable error hander; will deal with errors ourselves. */
    ctaSetErrorHandler( NULL, NULL );

    /* Initialize Natural Access services needed by this process */
    initparms.size = sizeof(CTA_INIT_PARMS);
    initparms.parmflags  = 0;
    initparms.ctaflags = CTA_CONTEXTNAME_REQUIRE_UNIQUE;

    if ( (ret = ctaInitialize( gsvcList, gNumSvcs, &initparms )) != SUCCESS )
    {
        printf("Unable to Initialize with Natural Access - 0x%x\n", ret);
        exit( 1 );     
    } 

    if ( !*gArgs.cmdHost )
    {
        /* Use the localhost's server for creating our contexts */
        ret = ctaSetDefaultServer( "localhost" );
    }

    /* Start each of the processing threads */
    if( DemoSpawnThread( checkPoint, (void *) NULL) == FAILURE )
    {
        printf("Could not create thread: %s\n", "checkPoint" );
        exit(-1);
    }
    if( DemoSpawnThread( dispatchCommand, (void *) NULL) == FAILURE )
    {
        printf("Could not create thread: %s\n", "dispatchCommand" );
        exit(-1);
    }
    if( DemoSpawnThread( ccVoice, (void *) NULL) == FAILURE )
    {
        printf("Could not create thread: %s\n", "ccVoice" );
        exit(-1);
    }
    if( DemoSpawnThread( manageBoards, (void *) NULL) == FAILURE )
    {
        printf("Could not create thread: %s\n", "manageBoards" );
        exit(-1);
    }
    if( DemoSpawnThread( monitorTrunks, (void *) NULL) == FAILURE )
    {
        printf("Could not create thread: %s\n", "monitorTrunks" );
        exit(-1);
    }
    /* Sleep forever. */
    for(;;)
        DemoSleep( 1000 );

    UNREACHED_STATEMENT( return 0; )
}
