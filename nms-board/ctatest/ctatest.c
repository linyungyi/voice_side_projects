/*****************************************************************************
 *                                  CTATEST.C
 *
 *  Stand-alone Natural Access and ADI Service test program
 *
 *  Functions:
 *      main()
 *      WaitForAnyEvent()    waits for an event from the board or keyboard
 *      FetchKeys()          gets double-key commands from user
 *      MyEventHandler()     supplied to adiOpenDriver
 *      HelpKeys()           displays list of all commands
 *      PerformFunc()        performs function specified by 'FetchKey' input
 *      ClearScreen()        simply clears the screen
 *      ShowParm()           used by ShowDefaultParm
 *      ShowDefaultParm()
 *      SetParm()            used by SetDefaultParm
 *      SetDefaultParm()
 *      ShowCallStatus()     interprets adiGetCallStatus information
 *      ShowAdiState()
 *      ShowNccCallState()
 *      ShowISDNExtCallStatus()
 *      ShowCASExtCallStatus()
 *      ShowNccLineState()
 *      ShowCapabilities()
 *
 *  Copyright NMS Communications Corp. 1996-2002
 *****************************************************************************/
#define VERSION_NUM  "2.3"
#define VERSION_DATE __DATE__

/*----------------------------- Includes -----------------------------------*/
#if defined (WIN32)
#include <conio.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>
#define GETCH() getche()
#elif defined (UNIX)
#include <unistd.h>
#include <termio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define GETCH() getchar()
#ifdef SCO
#include <fcntl.h>
#endif
#endif

#include "ctademo.h"
#include "nccdef.h"
#include "nccxadi.h"
#include "nccxisdn.h"
#include "nccxcas.h"
#include "oamdef.h"
#include "hswdef.h"

/*---------------- macros and defines for this module ----------------------*/

#define PAIR( A, B ) ((A<<8)+B)

typedef struct
{
    CTAHD   ctahd;
    char    cName[256];
    VCEHD   vcehd;
    VCEHD   vcememhd;
} CNXT_INFO;

/* case displays of constants: */
#define _SHOWCASE_(x) case x: printf("%s\n",#x); break
#define MAX_STRUCT_SIZE 2000
#define SVC_NAME_BUFFER_SIZE    128
#define MAX_SVC_NAME_LEN         16
#define MAX_SERVICES             64
#define MAX_CONTEXTS             20

/*------------------------ forward references ------------------------------*/

DWORD NMSSTDCALL ErrorHandler(CTAHD ctahd, DWORD ret, char *errtxt, char *func);
void Exit( int exit_code );

unsigned FetchKeys( void );
unsigned FetchKeysFromFile( void );

void MyEventHandler( CTA_EVENT *pm );

void PrintUsage( void );
void HelpKeys( void );
void PerformFunc( CTAQUEUEHD ctaqueuehd, unsigned k );
unsigned PromptForEncoding( unsigned mode );
unsigned PromptForFileType( void );
void ClearScreen( void );

int  StartProtocol( );
void ShowParm( void *pvalue, CTA_PARM_INFO *f ) ;
int  ShowDefaultParm( CTAHD ctahd, char *name );
int  SetParm( CTAHD ctahd, void *pvalue, CTA_PARM_INFO *f ) ;
int  SetDefaultParm( CTAHD ctahd, char *name );
void ShowDisconnectReason (unsigned reason) ;
void ShowCallStatus( ADI_CALL_STATUS *ci );
int  ShowAdiState( CTAHD ctahd );
int  ShowNccCallState( NCC_CALLHD callhd );
int  ShowISDNExtCallStatus( NCC_CALLHD callhd );
int  ShowCASExtCallStatus( NCC_CALLHD callhd );
int  ShowNccLineState( CTAHD ctahd );
void ShowCapabilities( NCC_PROT_CAP *capabilities );
void ShowMVIP( char *str, unsigned value );
void ShowReason( int reason );
void promptstr (char *text, char *target, unsigned max ) ;
void prompt (char *text, int *target);
void promptdw (char *text, DWORD *target);
void promptw (char *text, WORD *target);
void prompthex (char *text, DWORD *target);
void promptuns(char *text, unsigned *target);
void promptchar(char *text, char *target);
void promptline (char *text, char *target, unsigned max );
int  PromptForStrings(char *promptstring, char *strarr[], int maxstrlen);
void PrintStringofStrings(char *stringlist);
int  PromptServiceOpenList(CTA_SERVICE_DESC *ServiceList);
int  ContextAdd(CNXT_INFO cxinfo);
int  ContextRemove(CTAHD ctahd);
int  ContextGet(CNXT_INFO *cxinfo);
void ContextBackup(CTAHD ctahd, VCEHD vcehd, VCEHD vcememhd);
void ContextArrayInitialize(void);
void PrintServiceList(char *svcnames);
void GetLineFromFile(char *line, int len);
int StrNoCaseCmp(char *str1, char *str2);
BOOL ReconnectServer(char *name, CTAQUEUEHD ctaqueuehd);
int  SendISDNCallMessage( NCC_CALLHD callhd );

/* Undocumented function that takes a service name and optional service
   manager and returns an array of unsigned integers containing the
   parameter ids of all parameter structures registered by the service.
   It also returns the service id of the service in retsvcid. */
DWORD NMSAPI ctaGetParmIds(
         char     *svcname,                   /* service name                 */
         char     *svcmgrname,                /* optional service manager     */
         unsigned *retsvcid,                  /* returned service id          */
         unsigned *buffer,                    /* array of parmids to return   */
         unsigned  maxsize,                   /* size of the array            */
         unsigned *retsize);                  /* number of parmids returned   */

/*------------------------ global variables --------------------------------*/

int      Board  = 0;           /* Command line args to place the context */
int      Stream = 0;
int      Slot   = 0;
int      nSlots = 8;
unsigned Openmode = ADI_FULL_DUPLEX ;
unsigned wait_on_exit = 0;

BOOL     wait_daemon = FALSE;

char     DigitsCollected[40] = "";
char     ProtocolName[40]    = "nocc";
#if defined (UNIX)
char     VoiceFileName[512]   = "/tmp/test.vox";
#else
char     VoiceFileName[_MAX_PATH]   = "test.vox";
#endif
unsigned LowLevel            = 0;
unsigned SharedMemParms      = 1;
unsigned EventFetch          = 2;
unsigned adiMemoryFunctions  = 0;
unsigned adiMemoryDataSize   = 0;
unsigned NccInUseFlag        = 1;
unsigned CallOnHold          = 0;

unsigned ToggleAutoDisconnect  = 0;
unsigned AutoStart           = 0;  /* Automatically open port and start protocol */

char callerID[32] = "";


VCEHD    Vcehd     = 0;            /* Current play or record (file) */

BYTE     MemoryBuffer[ 20000 ];    /* PlayFrom/RecordTo memory buf */
VCEHD    VceMemhd     = 0;

unsigned Repeatentry   = 0;
char     EraseChar = 0;

BOOL     Batch = FALSE;
FILE     *InputFileStream;

CNXT_INFO contextarray[20] = { 0 };

NCC_CALLHD callhd = NULL_NCC_CALLHD;
NCC_CALLHD heldcallhd = NULL_NCC_CALLHD;

CTAHD    ctahd       = NULL_CTAHD;  /* One main context              */
CTAHD    ctahd_oam   = NULL_CTAHD;  /* Need a separate one for OAM service  */
                                    /*  because the service is on the server*/

char    ncc_mgr[10] = { 0 };

/* Variables used to initialize/open services without NCC ------------ */

static CTA_SERVICE_NAME pInitServicesWithoutNcc[] = /* for ctaInitialize */
{   { "ADI", NULL },
    { "VCE", "VCEMGR" },
    { "OAM", "OAMMGR" }
};
static int NumInitServicesWithoutNcc = sizeof pInitServicesWithoutNcc /
                                       sizeof pInitServicesWithoutNcc[0];

static CTA_SERVICE_DESC pServicesWithoutNcc[] =     /* for ctaOpenServices */
{   { {"ADI", NULL},     { 0 }, { 0 }, { 0 } },
    { {"VCE", "VCEMGR"}, { 0 }, { 0 }, { 0 } }
};

/* Variables used to initialize/open services with NCC ---------------- */

static CTA_SERVICE_NAME pInitServicesWithNcc[] =    /* for ctaInitialize */
{   { "ADI", NULL },
    { "NCC", NULL },
    { "VCE", "VCEMGR" },
    { "OAM", "OAMMGR" }
};
static int NumInitServicesWithNcc = sizeof pInitServicesWithNcc /
                                    sizeof pInitServicesWithNcc[0];

static CTA_SERVICE_DESC pServicesWithNcc[] =        /* for ctaOpenServices */
{   { {"ADI", NULL},     { 0 }, { 0 }, { 0 } },
    { {"NCC", NULL},     { 0 }, { 0 }, { 0 } },
    { {"VCE", "VCEMGR"}, { 0 }, { 0 }, { 0 } }
};

static CTA_SERVICE_DESC pServicesOAM[] =        /* for ctaOpenServices */
{
    { {"OAM", "OAMMGR"}, { 0 }, { 0 }, { 0 } }
};

/* Default is to use NCC. */
static CTA_SERVICE_NAME* pInitServices = pInitServicesWithNcc;
static int           *pNumInitServices = &NumInitServicesWithNcc;

static CTA_SERVICE_DESC* pServices = pServicesWithNcc;
static unsigned SizeofServices = sizeof(pServicesWithNcc);

#if defined (UNIX)
/*----------------------------------------------------------------------------
  UnixGets

  Read characters from the standard input stream.
  ----------------------------------------------------------------------------*/
char UnixGets(char *s)
{
    char c;
    int i = 0;

    while((c = getchar()) != -1)
    {
        if( c == EraseChar  || c == 0x08 )
        {
            if( i > 0 )
            {
                c = 0x08;
                putchar(c);
                putchar(' ');
                putchar(c);
                i--;
            }
            continue;
        }

        switch( c )
        {
            case 0x1b:
                continue;
                break;
            case 0x0a:
            case 0x0d:
                putchar( c );
                s[i] = '\0';
                return c;
                break;
            default:
                putchar( c );
                s[i++] = c;
                break;
         }
    }
    return c;
}

/*----------------------------------------------------------------------------
  DisableKbdEcho()

  Disable keyboard echo.
  ----------------------------------------------------------------------------*/
char DisableKbdEcho( void )
{
    static struct termio tio;
    static int fd;

    fd = open( "/dev/tty", O_RDWR );
    ioctl( fd, TCGETA, &tio );
    tio.c_lflag &= ~(ICANON | ISIG | ECHO | ECHOCTL | ECHOE | \
                     ECHOK | ECHOKE | ECHONL | ECHOPRT );

    tio.c_iflag |= IGNBRK;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
    ioctl( fd, TCSETA, &tio );

    return (tio.c_cc[VERASE]);
}
#endif

/*----------------------------------------------------------------------------
  TestShowError

  Print a message for a Natural Access error.
  ----------------------------------------------------------------------------*/
void TestShowError( char *preface, DWORD errcode )
{
    char text[40] = "";

    ctaGetText( NULL_CTAHD, errcode, text, sizeof(text) );
    printf( "\t%s: %s\n", preface, text );
}

/*----------------------------------------------------------------------------
  TestSetup

  ----------------------------------------------------------------------------*/
void TestSetup(
    CTA_INIT_PARMS initparms,
    char       *adi_mgr,                   /* Natural Access ADI manager      */
    unsigned    ag_board,                  /* AG board number                 */
    unsigned    mvip_stream,               /* MVIP stream                     */
    unsigned    mvip_slot,                 /* MVIP timeslot                   */
    CTAQUEUEHD *ctaqueuehd)                /* returned Natural Access queue handle */
{
    DWORD ret;
    CTA_ERROR_HANDLER hdlr;

    if (initparms.parmflags == CTA_PARM_MGMT_SHARED)
        SharedMemParms       = 1;
    else
        SharedMemParms       = 0;

    initparms.filename = DemoGetCfgFile();

    /* Initialize the ADI service (index 0) MVIP address argument */
    pInitServices[0].svcmgrname    = adi_mgr;
    pServices[0].name.svcmgrname   = adi_mgr;
    pServices[0].mvipaddr.board    = ag_board;
    pServices[0].mvipaddr.stream   = mvip_stream;
    pServices[0].mvipaddr.timeslot = mvip_slot;
    pServices[0].mvipaddr.mode     = Openmode;

    if (NccInUseFlag)
    {
        /* Initialize the NCC service (index 1) MVIP address with the same
           address as the ADI service. THIS MAY BE CHANGED in the future,
           to allow call-control and media to be done on different
           boards??? */
        if ( ncc_mgr[0] == '\0' )
        {
            pInitServices[1].svcmgrname    = adi_mgr;
            pServices[1].name.svcmgrname   = adi_mgr;
        }
        else
        {
            pInitServices[1].svcmgrname    = ncc_mgr;
            pServices[1].name.svcmgrname   = ncc_mgr;
        }

        pServices[1].mvipaddr.board    = ag_board;
        pServices[1].mvipaddr.stream   = mvip_stream;
        pServices[1].mvipaddr.timeslot = mvip_slot;
        pServices[1].mvipaddr.mode     = Openmode;
    }

    printf("Initializing and creating the queue...\n");

    /* Set error handler to NULL and remember old handler */
    ctaSetErrorHandler( NULL, &hdlr );

    /* If an input filename was specified, then don't use
     * compiled-in service list (ctaInitialize would ignore it anyway).
     */
    ret = ( (initparms.filename != NULL)
            ? ctaInitialize( NULL, 0, &initparms )
            : ctaInitialize( pInitServices, *pNumInitServices, &initparms) );

    if( ret != SUCCESS )
    {
        switch( ret )
        {
            case CTAERR_WRONG_COMPATLEVEL:
                printf("ERROR: Wrong compatibility level.\n");
                printf("       Check Natural Access version.\n");
                Exit( -1 );
                break;
            default:
                printf("ERROR: ctaInitialize returned 0x%08x (see ctaerr.h)\n",
                        ret);
                break;
        }
        /* Put back old keyboard mode */
        Exit( -1 );
    }

    ctaSetErrorHandler( hdlr, NULL ); /* restore error handler */

    /* Open the Natural Access application queue */
    if ((ret = ctaCreateQueue( NULL, 0, ctaqueuehd )) != SUCCESS)
    {
        Exit( -1 );
    }

    /* Create the OAM context */
    ctaCreateContext( *ctaqueuehd, 0, "localhost/CTATESTOAM", &ctahd_oam);

    ctaOpenServices( ctahd_oam,
                     pServicesOAM,
                     sizeof pServicesOAM /sizeof(CTA_SERVICE_DESC) );

    ContextArrayInitialize();

    printf("ok.\n");
}

/*----------------------------------------------------------------------------
  TestShutdown

  Synchronously closes everything we setup above.
  ----------------------------------------------------------------------------*/
void TestShutdown(CTAQUEUEHD ctaqueuehd)
{
    DWORD ret;

    printf("Destroying the queue...\n");
    ret = ctaDestroyQueue( ctaqueuehd );

    if (ret != SUCCESS)
    {
        printf("Destroy queue failed...\n");
    }
    return;
}


/*----------------------------------------------------------------------------
  main()
  ----------------------------------------------------------------------------*/
int main( int argc, char *argv[] )
{
    unsigned     ret;
    unsigned     eventtype;
    char         InputFileName[40] = "";
    char         c = '\0';
    CTA_INIT_PARMS initparms = { 0 };
    CTAQUEUEHD   ctaqueuehd = 0xffffffff;
    CTA_EVENT    event = { 0 };

    initparms.size = sizeof(CTA_INIT_PARMS);
    initparms.parmflags  = CTA_PARM_MGMT_SHARED;
    initparms.traceflags = CTA_TRACE_ENABLE;
    initparms.filename = NULL;
    initparms.ctacompatlevel = CTA_COMPATLEVEL;


    /* Configure Raw Mode, before any printfs */
    DemoSetupKbdIO(1);
    atexit( RestoreSetupKbdIO );

#if defined (UNIX)
    EraseChar = DisableKbdEcho();
#endif

    while( (c = getopt( argc, argv, "m:t:cF:vf:i:A:o:s:b:n:p:wlaM:?hHdS")) != -1 )
    {
        switch( c )
        {
            unsigned   value1, value2;

            case 'A':
                DemoSetADIManager( optarg );
                break;
            case 'f':
            case 'F':
                DemoSetCfgFile( optarg );
                break;
            case 'm':
            {
                char parmflag='\0';

                sscanf( optarg, "%c", &parmflag);
                switch( parmflag )
                {
                    case 'l':
                        initparms.parmflags = CTA_PARM_MGMT_LOCAL;
                        break;
                    case 'c':
                        // this option is obsolete because ctdaemon is always runnning
                        initparms.parmflags = CTA_PARM_MGMT_SHARED_IF_PRESENT;
                        break;
                    case 's':
                    default:
                        initparms.parmflags = CTA_PARM_MGMT_SHARED;
                        break;
                }
                break;
            }
            case 't':
            {
                char traceflag='\0';

                sscanf( optarg, "%c", &traceflag);
                switch( traceflag )
                {
                    case 'd':
                        initparms.traceflags = 0;
                        break;
                    case 'r':
                        initparms.traceflags |= CTA_TRACE_ON_ERROR;
                        break;
                    default:
                        break;
                }
                break;
            }
            case 'v':
                initparms.ctacompatlevel = 0;
                break;
            case 'i':
                sscanf( optarg, "%s", &InputFileName);
                Batch = TRUE;
                if ((InputFileStream = fopen( InputFileName, "r")) == NULL)
                {
                    printf("Cannot open Input File %s\n", InputFileName);
                    exit( -1 );
                }
                break;
            case 's':
                switch (sscanf( optarg, "%d:%d", &value1, &value2 ))
                {
                    case 2:
                        Stream = value1;
                        Slot   = value2;
                        break;
                    case 1:
                        Slot = value1;
                        break;
                    default:
                        PrintUsage();
                        exit( -1 );
                }
                break;
            case 'b':
                sscanf( optarg, "%d", &Board );
                break;
            case 'n':
                sscanf( optarg, "%d", &nSlots );
                break;
            case 'p':
                strcpy( ProtocolName, optarg );
                break;
            case 'l':
                LowLevel = 1;
                break;
            case 'o':
                Openmode = (unsigned) strtoul(optarg, NULL, 0);
                break;
            case 'w':
                wait_on_exit = 1;
                break;
            case 'a':
                adiMemoryFunctions = 1;
                break;
            case 'c':
                NccInUseFlag = 0;
                pInitServices = pInitServicesWithoutNcc;
                pNumInitServices = &NumInitServicesWithoutNcc;
                pServices = pServicesWithoutNcc;
                SizeofServices = sizeof(pServicesWithoutNcc);
                break;
            case 'd':
                wait_daemon = TRUE;
                break;
            case 'M':
                strcpy( ncc_mgr, optarg );
                break;
            case 'S':
                AutoStart = 1;
                break;
            case '?':
            case 'h':
            case 'H':
            default:
                printf( "NA Demonstration and Test Program  V.%s (%s)\n",
                        VERSION_NUM, VERSION_DATE );
                puts ("NMS Communications Corporation.\n");
                PrintUsage();
                Exit( -1 ) ;
                break;
        }
    }
    if (optind < argc)
    {
        printf( "Invalid option.  Use -h for help.\n" );
        Exit( 1 );
    }

    ctaSetErrorHandler( ErrorHandler, NULL );

    HelpKeys();                 /* display list of commands */

    /* Initialize Natural Access, open a Natural Access context, open default services */
    TestSetup( initparms, DemoGetADIManager(), Board, Stream, Slot, &ctaqueuehd );


    if ( AutoStart )
    {
        int ret;
        
        printf("\tAutomatically open port ...\n");
        ret = ctaCreateContext( ctaqueuehd, 0, "CTATEST", &ctahd );

        if (ret == SUCCESS)
        {
            ctaOpenServices( ctahd,
                                   pServices,
                                   SizeofServices/sizeof(CTA_SERVICE_DESC) );
        }
    }
    
    /* Wait for either a Natural Access event or for a keyboard event.  The
     * return code indicates the type of event, 0=AG, 1=keyboard
     */
    for (;;)
    {
        BOOL reconnecting = FALSE;
        static char cnxtName[256] = {0};

        if( EventFetch != 0 )
        {
            for (;;)
            {
                ret = ctaWaitEvent( ctaqueuehd, &event, 100);
                if (ret != SUCCESS)
                {
                    if (wait_daemon)
                    {
                        if (!reconnecting)
                        {
                            BOOL found = FALSE;
                            int i;

                            for ( i = 0; i < MAX_CONTEXTS; i++)
                            {
                                if (contextarray[i].ctahd == ctahd)
                                {
                                    strcpy(cnxtName, contextarray[i].cName);
                                    found = TRUE;
                                    break;
                                }
                            }
                            if (!found)
                            {
                                if (cnxtName[0] == 0)
                                {
                                    printf("No Contexts found to reconnect\n");
                                    break;
                                }
                            }

                            ContextRemove(ctahd);

                            printf("Waiting for Natural Access Server...\n");
                            reconnecting = TRUE;
                        }

                        if (ReconnectServer(cnxtName, ctaqueuehd) == TRUE)
                        {
                            ret = SUCCESS;
                            reconnecting = FALSE;
                        }

                    }
                    else
                    {
                        TestShowError( "Error from ctaWaitEvent", ret );

                        Exit( 1 );
                    }
                }
                else if (event.id == CTAEVN_WAIT_TIMEOUT)
                {
                    if (Batch)
                    {
                        eventtype = 1;
                        break;
                    }
                    else if (kbhit())
                    {
                        eventtype = 1;
                        break;
                    }
                    else if (reconnecting)
                    {
                        if (ReconnectServer(cnxtName, ctaqueuehd) == FALSE)
                        {
                            continue;
                        }
                        else
                        {
                            ret = SUCCESS;
                            reconnecting = FALSE;
                            continue;
                        }

                    }
                    else
                    {
                        continue;
                    }
                }
                else /* got a real event */
                {
                    eventtype = 0;
                    break;
                }
            }
        }
        else
            eventtype = 1;

        if( eventtype != 0 )         /* a keyboard event is ready */
        {
            int key;

            if (Batch)
                key = FetchKeysFromFile();
            else
                key = FetchKeys();
            if( key != 0)
            {
                PerformFunc( ctaqueuehd, key );
            }
        }
        else                     /* an AG event needs to be processed */
        {
            int  freeCtaBuff = 0;

            if( event.id != 0 )  /* event returned; call event handler */
            {
                if ( event.buffer )
                {
                    /* Determine whether this buffers needs to be released
                     *  after processing.
                     */
                    if ( freeCtaBuff = event.size & CTA_INTERNAL_BUFFER )
                        event.size &= ~ CTA_INTERNAL_BUFFER;
                }

                MyEventHandler( &event );

                if ( freeCtaBuff )
                    /* When running in C/S mode, internal Natural Access buffers
                     *  were allocated that must now be freed.
                     */
                    ctaFreeBuffer( event.buffer );
            }

            /* If single step event fetch, clear EventFetch flag */
            if( EventFetch == 1 )
            {
                EventFetch = 0;
                printf( "use GE to get another event\n" );
            }
        }
    }

    UNREACHED_STATEMENT( return 0; )
}


/*----------------------------------------------------------------------------
  FetchKeys()

  Gets either one or two keys.  Returns a PAIR() or 0 if no PAIR() is
  ready.  A PAIR() is either a special single key, or two keys.
  ----------------------------------------------------------------------------*/
unsigned FetchKeys()
{
    static int keyscollected = 0;       /* all must be static */
    static int old = 0;
    int key;

    key = GETCH();

#if defined (UNIX)
    if (key == EraseChar || key == 0x08)
    {
      key = 0x08;
      putchar( key );
      putchar( ' ' );
      putchar( key );
      keyscollected = 0 ;
      return 0;                       /* no key PAIR yet */
    }
    else
      putchar( key );
#else
    if (key == '\033' || key == '\010') /* ESC or BS */
    {
        keyscollected = 0 ;
        return 0;                       /* no key PAIR yet */
    }
#endif
    key = toupper( key );               /* make it upper case */

    if( keyscollected > 0 )
    {
        keyscollected = 0;
        printf("\n");
        return PAIR( old, key );        /* processing a PAIR  */
    }
    else if( strchr( "?!HXQW \x0D\x0A\x1B", key ) != (char *)NULL )
    {
        /* make a message with one character and return to the caller */
        keyscollected = 0;
        printf("\n");
        return PAIR( key, 0 );          /* special one key PAIR */
    }
    else
    {
        old = key;
        keyscollected = 1;
        return 0;                       /* no key PAIR yet */
    }
}

/*----------------------------------------------------------------------------
  FetchKeysFromFile()

  Read one- or two- letter command from one line of the input file
  ----------------------------------------------------------------------------*/
unsigned FetchKeysFromFile()
{
    char line[80];
    char keyword[8];
    int key1 = 0;
    int key2 = 0;

    GetLineFromFile(line, sizeof line);

    if (sscanf(line, "%s", keyword) == -1)
    {
        return 0;
    }

    key1 = (keyword[0] != '\0') ? toupper((int)keyword[0]) : 0;

    if (key1 == 0)
        return  0;

    key2 = (keyword[1] != '\0') ? toupper((int)keyword[1]) : 0;

    keyword[3] = '\0';
    printf("%s\n", keyword);

    return PAIR( key1, key2 );

}

/*----------------------------------------------------------------------------
  MyEventHandler()

  Event handler routine.
  ----------------------------------------------------------------------------*/
void MyEventHandler( CTA_EVENT *event )
{
    int showreason = 1 ;
    DWORD i;

    // Test for OAM event; the only one we care about is notification of a
    //  hot swap removal.
    if (event->ctahd == ctahd_oam)
    {
        switch( event->id )
        {
            case CTAEVN_OPEN_SERVICES_DONE:
                // Must sign up for *All* OAM events in order to get hot swap notification
                oamAlertRegister( event->ctahd );
                break;

            case HSWEVN_REMOVAL_REQUESTED:
            {
                const OAM_MSG * const pOamMsg = (OAM_MSG *)event->buffer;
                if (pOamMsg)
                {
                    // get board #
                    const char * const pszName = (char*)pOamMsg + pOamMsg->dwOfsSzName;
                    DWORD boardnumber = (DWORD) -1;

                    if (SUCCESS == oamBoardGetNumber( event->ctahd, pszName, &boardnumber ))
                    {
                        printf("Hot swap Removal Requested For Board %d\n", boardnumber);
                    }

                    if ((int)boardnumber == Board && ctahd != NULL_CTAHD)
                    {
                        printf("Destroying Context\n");
                        ctaDestroyContext(ctahd);
                        ctahd  = NULL_CTAHD;
                    }
                }
                break;
            }
            default:
                // Ignore all other OAM events
                break;
        }
        // Any event buffer will be freed by caller of this function
        return;
    }

    printf("ctahd 0x%x ", event->ctahd);
    DemoShowEvent( event ) ;

    /* Note: also in the message are:
     *       event->ctahd      context handle
     *       event->timestamp  event time stamp
     *       event->userid     user supplied value
     *                          (could be ptr to user context)
     */

    switch( event->id )
    {
        case VCEEVN_RECORD_DONE:
        case VCEEVN_PLAY_DONE:
            if (Vcehd != 0)
            {
                vceClose( Vcehd );
                Vcehd = 0;
            }
            break;

        case ADIEVN_RECORD_DONE:
            adiMemoryDataSize = event->size;
            break;

        case CTAEVN_OPEN_SERVICES_DONE:
            if ( AutoStart )
            {
                printf("\tAutomatically start protocol...\n");
                StartProtocol ( );
            }
            break;

        case CTAEVN_DESTROY_CONTEXT_DONE:
        {
            CallOnHold = 0;
            callhd = NULL_NCC_CALLHD;
            heldcallhd = NULL_NCC_CALLHD;
            Vcehd = 0;
            break;
        }

        case ADIEVN_INCOMING_CALL:
        {
            ADI_CALL_STATUS call_status;

            adiGetCallStatus( event->ctahd, &call_status, sizeof(call_status) );

            if( *call_status.calledaddr )
                printf( "\t\tCalled number = '%s'\n", call_status.calledaddr );

            if( *call_status.callingaddr )
                printf( "\t\tCaller number = '%s'\n", call_status.callingaddr );

            if( *call_status.callingname )
                printf( "\t\tCaller name = '%s'\n", call_status.callingname );
            break;
        }

        case NCCEVN_INCOMING_CALL:
        {
            NCC_CALL_STATUS call_status;

            nccGetCallStatus( callhd, &call_status, sizeof( call_status ) );

            if( *call_status.calledaddr )
                printf( "\t\tCalled number = '%s'\n", call_status.calledaddr );

            if( *call_status.callingaddr )
                printf( "\t\tCaller number = '%s'\n", call_status.callingaddr );

            if( *call_status.callingname )
                printf( "\t\tCaller name = '%s'\n", call_status.callingname );
            break;
        }

        case NCCEVN_SEIZURE_DETECTED:
        {
            callhd = event->objHd;
            printf( "\t\tCallhd = 0x%x\n", callhd );
            break;
        }

        case NCCEVN_PLACING_CALL:
        {
            callhd = event->objHd;
            printf( "\t\tCallhd = 0x%x\n", callhd );
            break;
        }

        case NCCEVN_CALL_HELD:
        {
            CallOnHold = 1;
            heldcallhd = callhd;
            callhd = NULL_NCC_CALLHD;
            break;
        }

        case NCCEVN_CALL_RETRIEVED:
        {
            CallOnHold = 0;
            callhd = heldcallhd;
            heldcallhd = NULL_NCC_CALLHD;
            break;
        }

        case NCCEVN_CALL_RELEASED:
        {
            /* For automatic tranfer. */
            if (callhd == event->objHd)
            {
                if (heldcallhd != NULL_NCC_CALLHD)
                {
                    callhd = heldcallhd;
                }
                else
                {
                    callhd = 0;
                }
            }
            else if (heldcallhd == event->objHd)
            {
                heldcallhd = NULL_NCC_CALLHD;
            }

            break;
        }

        case NCCEVN_STOP_PROTOCOL_DONE:
        {
            CallOnHold = 0;
            callhd = NULL_NCC_CALLHD;
            heldcallhd = NULL_NCC_CALLHD;
            break;
        }

        case ADIEVN_FSK_RECEIVE_DONE:
            if (event->value == CTA_REASON_FINISHED)
                if (event->buffer != NULL)
                {
                    printf ("\t\tData (%d bytes) = ", event->size);
                    for (i = 0; i < event->size; i++)
                    {
                        if ((((char*)event->buffer)[i] < ' ') ||
                            (((char*)event->buffer)[i] > 0x7E))
                            printf ("[%02X]", ((unsigned char*)event->buffer)[i]);
                        else
                            printf ("%c",((char*)event->buffer)[i]);
                    }
                    printf("\n");
                }
            break;

        case NCCEVN_CALL_DISCONNECTED:          // automatically disconnect call...
            if (ToggleAutoDisconnect)
            {
                printf("\tnccReleaseCall automatically called...\n");
                nccReleaseCall( callhd, NULL );
            }
            break;
            
        case NCCEVN_PROTOCOL_EVENT:
            if (StrNoCaseCmp(ProtocolName, "ISD0") == 0)
            {
                NCC_ISDN_SEND_CALL_MESSAGE *p = (NCC_ISDN_SEND_CALL_MESSAGE *)event->buffer;
                switch ( p->message_id )
                {
                    case NCC_ISDN_TRANSPARENT_BUFFER:
                        {
                            NCC_ISDN_TRANSPARENT_BUFFER_INVOKE *ex = (NCC_ISDN_TRANSPARENT_BUFFER_INVOKE *) p;
                            char * name = "<UNKNOWN>";
                            unsigned i;
                            
                            switch ( ex->isdn_message )
                            {
                            case NCC_ISDN_FACILITY: name = "FACILITY";  break;     
                            case NCC_ISDN_NOTIFY:   name = "NOTIFY";    break; 
                            case NCC_ISDN_INFO:     name = "INFO";      break;
                            }
                            printf ("\t\tTransparent ISDN message %s (%d bytes):\n", name, ex->size );

                            for ( i = 0; i < ex->size; i++ )
                            {
                                if ( ( i & 0x0F ) == 0 ) printf ("\n\t\t");
                                printf("%02X ", (unsigned char) ex->buffer[i] );
                                if ( ( i & 0x03 ) == 3 ) printf ("   " );
                            } 
                            printf ("\n");
                        }
                        break;
                   
                    case NCC_ISDN_TRANSFER:
                        {
                            int i;
                            NCC_ISDN_TRANSFER_PARAM *ex = (NCC_ISDN_TRANSFER_PARAM *) p;
                            printf("\t\tISDN Transfer Indication,  CallID = {" );
                           
                            for ( i=0; i<sizeof(ex->callid)/sizeof(ex->callid[0]); i++ ) 
                            {
                                printf (" %04X", ex->callid[i] );
                            }

                            printf(" }\n");
                        }
                        break;     
                }
                                
            }
 
            break;

        default:
            break;
    }
    return ;
}


/*----------------------------------------------------------------------------
  Key table and table operation routines
  ----------------------------------------------------------------------------*/

typedef struct { unsigned value; char *help; } KEYS;

#define SKIPLINE '\n',""

/* Note - single key commands have to be included in the string in fetchkeys()*/
static KEYS keys[] =
{
    PAIR( 'H',  0  ),  "help",
    PAIR( 'Q',  0  ),  "quit",
    PAIR( '!',  0  ),  "repeat command",
    PAIR( 'W',  0  ),  "trace",

    PAIR( 'P', 'I' ),  "get parm id",
    PAIR( 'R', 'P' ),  "refresh prm",
    PAIR( 'P', 'D' ),  "set parms",
    PAIR( 'V', 'D' ),  "view parms",
    SKIPLINE,

    PAIR( 'C', 'P' ),  "close ctx & svcs",
    PAIR( 'O', 'P' ),  "open cntx & svcs",
    PAIR( 'X',  0  ),  "show state",
    PAIR( '_', '_' ),  "undefined",
    SKIPLINE,

    PAIR( 'A', 'X' ),  "attach context",
    PAIR( 'C', 'X' ),  "create shrd cntx",
    PAIR( 'D', 'X' ),  "destroy context",
    PAIR( 'E', 'C' ),  "enumerate cntxts",

    PAIR( 'P', 'X' ),  "create pers. ctx",
    PAIR( 'V', 'X' ),  "create void cntx",
    PAIR( 'C', 'V' ),  "close services",
    PAIR( 'O', 'V' ),  "open services",

    PAIR( 'C', 'N' ),  "context info",
    PAIR( 'C', 'H' ),  "change context",
    PAIR( '_', '_' ),  "undefined",
    PAIR( '_', '_' ),  "undefined",
    SKIPLINE,

    PAIR( 'G', 'M' ),  "get evnt sources",
    PAIR( 'S', 'M' ),  "set evnt sources",
    PAIR( 'C', 'E' ),  "cont event fetch",
    PAIR( 'G', 'E' ),  "get 1 event",

    PAIR( 'S', 'E' ),  "stop event fetch",
    PAIR( 'L', 'E' ),  "loopback event",
    PAIR( 'F', 'E' ),  "format event",
    PAIR( 'E', 'H' ),  "set event hdlr",
    SKIPLINE,

    PAIR( 'S', 'P' ),  "start protocol",
    PAIR( 'U', 'P' ),  "stop protocol",
    PAIR( 'A', 'C' ),  "answer call",
    PAIR( 'P', 'C' ),  "place call",

    PAIR( 'R', 'C' ),  "release call",
    PAIR( 'B', 'C' ),  "block calls",
    PAIR( 'U', 'C' ),  "unblock calls",
    PAIR( 'C', '?' ),  "call status",

    PAIR( 'J', 'C' ),  "reject call",
    PAIR( 'C', 'C' ),  "accept call",
    PAIR( 'C', 'I' ),  "accept address",
    PAIR( 'A', 'S' ),  "assert signal",
    SKIPLINE,

    PAIR( 'P', 'F' ),  "play file",
    PAIR( 'P', 'M' ),  "play memory",
    PAIR( 'P', '?' ),  "play/rec status",
    PAIR( 'P', 'S' ),  "play/rec stop",

    PAIR( 'R', 'F' ),  "record file",
    PAIR( 'R', 'M' ),  "record memory",
    PAIR( 'R', '?' ),  "play/rec status",
    PAIR( 'R', 'S' ),  "play/rec stop",

    PAIR( 'M', 'G' ),  "modify play gain",
    PAIR( 'M', 'S' ),  "modify playspeed",
    PAIR( 'E', 'I' ),  "encoding info",
    PAIR( '_', '_' ),  "undefined",
    SKIPLINE,

    PAIR( 'D', 'G' ),  "digit get",
    PAIR( 'D', 'F' ),  "digit flush",
    PAIR( 'C', 'D' ),  "collect digits",
    PAIR( 'S', 'C' ),  "collect stop",

    PAIR( 'D', 'P' ),  "digit peek",
    PAIR( 'F', 'S' ),  "FSK Send",
    PAIR( 'F', 'R' ),  "FSK Receive",
    PAIR( 'F', 'A' ),  "FSK Abort rcv",

    PAIR( 'G', 'N' ),  "gen net tone",
    PAIR( 'G', 'T' ),  "gen user tone",
    PAIR( 'G', 'D' ),  "gen DTMFs",
    PAIR( 'G', 'S' ),  "generation stop",
    SKIPLINE,

    PAIR( 'T', 'P' ),  "toggle pattern",
    PAIR( 'G', 'W' ),  "generate wink",
    PAIR( 'S', 'T' ),  "start timer",
    PAIR( 'A', 'T' ),  "abort timer",

    PAIR( 'E', 'B' ),  "enable bit det",
    PAIR( 'D', 'B' ),  "disabl bit det",
    PAIR( 'S', 'Q' ),  "signal bit query",
    PAIR( 'D', 'S' ),  "define signals",

    PAIR( 'E', 'E' ),  "enable energydet",
    PAIR( 'D', 'E' ),  "disabl energydet",
    PAIR( 'E', 'D' ),  "enable DTMF det",
    PAIR( 'D', 'D' ),  "disabl DTMF det",

    PAIR( 'E', 'T' ),  "enable tone det",
    PAIR( 'D', 'T' ),  "disabl tone det",
    PAIR( 'E', 'M' ),  "enable MF det",
    PAIR( 'D', 'M' ),  "disabl MF det",

    PAIR( 'S', 'D' ),  "start dial",
    PAIR( 'A', 'D' ),  "abort dial",
    PAIR( 'C', 'B' ),  "callprog begin",
    PAIR( 'C', 'S' ),  "callprog stop",
    SKIPLINE,

    /* Commands for server control functions*/
    PAIR( 'S', 'V' ),  "get svc version",
    PAIR( 'R', 'V' ),  "get svr version",
    PAIR( 'F', 'F' ),  "find file",
    PAIR( 'T', 'R' ),  "set srvr trace m",

    PAIR( 'T', 'S' ),  "set time slot",
    PAIR( 'S', 'S' ),  "set default srv",
    PAIR( 'E', 'S' ),  "query services",
    PAIR( 'S', 'H' ),  "server shutdown",

    PAIR( 'C', 'L' ),  "get context list",
    PAIR( '_', '_' ),  "undefined",
    PAIR( '_', '_' ),  "undefined",
    PAIR( '_', '_' ),  "undefined",
    SKIPLINE,

    /* New commands for NCC functions */

    PAIR( 'O', 'C' ),  "hold call",
    PAIR( 'R', 'R' ),  "retrieve call",
    PAIR( 'D', 'C' ),  "disconnect call",
    PAIR( 'N', 'D' ),  "send digits",

    PAIR( 'I', 'H' ),  "set call handle",
    PAIR( 'L', '?' ),  "get line status",
    PAIR( 'C', 'Q' ),  "query capability",
    PAIR( 'V', 'T' ),  "supervised xfer",
    PAIR( 'T', '2' ),  "2-channel xfer",
    PAIR( 'C', 'M' ),  "send call msg",
    
    PAIR( 'I', '?' ),  "ext call status",
    PAIR( 'I', 'D' ),  "set caller ID  ",
    PAIR( 'O', 'R' ),  "hold response  ",
    PAIR( 'S', 'B' ),  "set billing",
    SKIPLINE,

    /* New commands for service handle testing */
    PAIR( 'A', 'O' ),  "attach object",
    PAIR( 'D', 'O' ),  "detach object",
    PAIR( 'D', 'H' ),  "get object descr",
    SKIPLINE,

    PAIR( 'T', 'D' ),  "toggle auto disconnect",
    SKIPLINE,
    0,""
};


/*----------------------------------------------------------------------------
  HelpKeys()
  Prints help info.
  ----------------------------------------------------------------------------*/
#define NULL_TO_SPACE(c) ( (c)==0 ? ' ' : c )

void HelpKeys( void )
{
    KEYS    *key;
    unsigned i;
    CTA_REV_INFO revinfo = { 0 } ;

    ClearScreen();

    ctaGetVersion( &revinfo, sizeof revinfo );

    printf( "NA Demonstration and Test Program  V.%s (%s)"
            "      NA Version is %d.%02d\n",
            VERSION_NUM, VERSION_DATE, revinfo.majorrev, revinfo.minorrev) ;

    for( i=0, key = &keys[0]; key->value != 0; key++ )
    {
        if( key->value == '\n' )
        {
            printf( "\n" );
            i = 0;
            continue;
        }
        if( key->value != PAIR( '_', '_' ) )
        {
            printf( "%c%c %-16.16s", key->value>>8,
                    NULL_TO_SPACE( key->value & 0xFF ), key->help );
        }
        if( (i++ %4) == 3 )
            printf( "\n" );     /* 4 per line */
        else
            printf( " " );      /* space between */
    }

    puts( "" );
    return;
}


/*----------------------------------------------------------------------------
  PerformFunc()

  Executes two character commands.
  ----------------------------------------------------------------------------*/
void PerformFunc( CTAQUEUEHD ctaqueuehd, unsigned key )
{
    unsigned ret;                          /* return code from ADI calls    */
    unsigned encoding ;
    unsigned filetype;
    static unsigned     userid = 0;
    static char         contextname[256] = "";

    static char     *svraddr = NULL;
    static unsigned prevkey = 0;

    ret = SUCCESS;              /* start with no errors */
    if (key == PAIR( '!',0 ))   /* '!' repeats last good entry */
    {
        if (prevkey != 0)
        {
            Repeatentry   = 1;
            PerformFunc( ctaqueuehd, prevkey ) ;
            Repeatentry   = 0;
        }
        return ;
    }

    switch( key )
    {
        case PAIR( 'H', 0 ):                /* HELP */
        case PAIR( '?', 0 ):
            HelpKeys();
            return;

        case PAIR( 'Q', 0 ):                /* QUIT */
            TestShutdown( ctaqueuehd );
            Exit( 0 );
            break;

        case PAIR( 'W', 0 ):                /* TRACE */
        {
            static DWORD mask=0;
            static char svcname[6] = "CTA";

            promptstr( "Trace service name", svcname, sizeof(svcname) );
            prompthex( "Trace mask (hex)", &mask);

            ret = ctaSetTraceLevel( ctahd, svcname, (unsigned)mask );
            break;
        }
        case PAIR( 'O', 'P' ):              /* OPEN PORT (i.e. context & services) */
            if( ctahd != NULL_CTAHD )
            {
                printf( "Natural Access context already created\n" );
                break ;
            }

            ret = ctaCreateContext( ctaqueuehd, 0, "CTATEST", &ctahd );

            if (ret != SUCCESS) break;

            ret = ctaOpenServices( ctahd,
                                   pServices,
                                   SizeofServices/sizeof(CTA_SERVICE_DESC) );
            break;

        case PAIR( 'C', 'P' ):              /* CLOSE PORT (i.e. context & services) */
            ret = ctaDestroyContext( ctahd );
            ctahd = NULL_CTAHD;
            break;

        case PAIR( 'C', 'X' ):              /* Create shared context */
        case PAIR( 'P', 'X' ):              /* Create Persistent shared context */
        case PAIR( 'V', 'X' ):              /* Create void shared context*/
        {
            CTAHD       tmpctahd;
            char        tmpcName[256];
            char        descriptor[256] = {0};
            char        sharing_flags = 0;
            BOOL        void_context = (key == PAIR( 'V', 'X' ));
            CTAQUEUEHD  tmpctaqueuehd;

            tmpctahd = ctahd;
            strcpy(tmpcName, contextname );
            strcpy(contextname, "");
            promptstr("Enter Context Name (optional)", contextname,
                                                       sizeof(contextname));

            if (! void_context)
            {
                static char sharemode = 'c';
                char  newsharemode = sharemode;

                if (key == PAIR( 'P', 'X' ))
                {
                    sharing_flags = CTA_CONTEXT_PERSISTENT;
                }

                promptchar("Access mode - decla(r)ed , (c)ommon", &newsharemode);

                switch( toupper( newsharemode ) )
                {
                    case 'R': sharing_flags |= CTA_CONTEXT_DECLARED_ACCESS;    break;
                    case 'C': sharing_flags |= 0;   break;
                    default: newsharemode = '\0';   break;
                }
                if (newsharemode == '\0')
                {
                    printf("\t\t\007Illegal Entry\n");
                    break;
                }
                sharemode = newsharemode;
            }

            /* create context descriptor */
            if( svraddr == NULL  ||  *svraddr == 0 )
            {
                if( contextname != NULL )
                    strcpy( descriptor,  contextname );
            }
            else
            {
                sprintf( descriptor,  "//%s/%s", svraddr, contextname );
            }

            tmpctaqueuehd = void_context ? NULL_CTAQUEUEHD : ctaqueuehd;

            ret = ctaCreateContextEx( tmpctaqueuehd, userid, descriptor,
                                      &ctahd,  sharing_flags );
            if (ret == SUCCESS)
            {
                // make this the current context, save previous handles
                CNXT_INFO cxinfo;

                if (tmpctahd  != NULL_CTAHD)
                {
                    ContextBackup(tmpctahd, Vcehd, VceMemhd);
                }
                cxinfo.ctahd = ctahd;
                strcpy(cxinfo.cName, contextname);
                cxinfo.vcehd = Vcehd = 0;
                cxinfo.vcememhd = VceMemhd = 0;
                ContextAdd(cxinfo);
            }
            else
            {
                ctahd = tmpctahd;
                strcpy(contextname, tmpcName);
            }

            break;
        }
        case PAIR( 'A', 'X' ):              /* ATTACH CONTEXT */
        {
            CTAHD       tmpctahd;
            char        tmpcName[256];
            char        descriptor[256] = {0};

            tmpctahd = ctahd;
            strncpy(tmpcName, contextname, 256);
            strcpy(contextname, "");
            promptstr("Enter Context Name", contextname,
                                            sizeof(tmpcName));

            /* create context descriptor */
            if( svraddr == NULL  ||  *svraddr == 0 )
            {
                if( contextname != NULL )
                    strcpy( descriptor,  contextname );
            }
            else
            {
                sprintf( descriptor,  "//%s/%s", svraddr, contextname );
            }

            ret = ctaAttachContext( ctaqueuehd, userid, descriptor,
                                    &ctahd);
            if (ret == SUCCESS)
            {
                CNXT_INFO cxinfo;

                if (tmpctahd  != NULL_CTAHD)
                {
                    ContextBackup(tmpctahd, Vcehd, VceMemhd);
                }
                cxinfo.ctahd = ctahd;
                strcpy(cxinfo.cName, contextname);
                cxinfo.vcehd = Vcehd = 0;
                cxinfo.vcememhd = VceMemhd = 0;
                ContextAdd(cxinfo);
            }
            else
            {
                ctahd = tmpctahd;
                strcpy(contextname, tmpcName);
            }

            break;
        }
        case PAIR( 'O', 'V' ):              /* OPEN SERVICE */
        {
            CTA_SERVICE_DESC    *ServiceList;
            int         nsvcs;

            if (ctahd == NULL_CTAHD)
            {
                printf("Invalid current context\n");
                break;
            }

            ServiceList = (CTA_SERVICE_DESC *)malloc(SizeofServices);
            if ((nsvcs = PromptServiceOpenList( ServiceList )) > 0)
            {
                ret = ctaOpenServices( ctahd, ServiceList, nsvcs );
            }
            free(ServiceList);
            break;
        }
        case PAIR( 'C', 'V' ):              /* CLOSE SERVICE */
        {
            char        *svcarr[MAX_SERVICES];
            int         nsvcs;

            if (ctahd == NULL_CTAHD)
            {
                printf("Invalid current context\n");
                break;
            }

            if (!Batch)
            {
                printf("Enter list of services to close; ");
                printf("Press Enter to end the list\n");
            }
            if ((nsvcs = PromptForStrings("Enter service name", svcarr,
                                          MAX_SVC_NAME_LEN)) > 0)
            {
                int i;
                for ( i = 0; i < nsvcs ; i++ )
                {
                    if (StrNoCaseCmp(svcarr[i], "VCE") == 0)
                    {
                        if (VceMemhd != 0)
                        {
                            vceClose( VceMemhd );
                            VceMemhd = 0;
                        }
                        break;
                    }
                }
                ret = ctaCloseServices(ctahd, svcarr, nsvcs);
            }
            for ( ; nsvcs >= 0; nsvcs--)
            {
                free(svcarr[nsvcs]);
            }
            break;
        }
        case PAIR( 'D', 'X' ):              /* DESTROY CONTEXT */
            ret = ctaDestroyContext( ctahd );
            ContextRemove(ctahd);
            ctahd = NULL_CTAHD;
            break;

        case PAIR( 'E', 'C' ):              /* ENUMERATE CONTEXTS */
        {
            int     i;
            BOOL    found = FALSE;

            for (i = 0; i < MAX_CONTEXTS; i++)
            {
                if (contextarray[i].ctahd != NULL_CTAHD)
                {
                    printf("ctahd = 0x%x\tcontext name = %s\n",
                           contextarray[i].ctahd, contextarray[i].cName);
                    found = TRUE;
                }
            }
            if (!found)
                printf("No Contexts found\n");

            break;
        }
        case PAIR( 'C', 'N' ):              /* GET CONTEXT NAME AND HANDLE */
        {

            if (ctahd == NULL_CTAHD)
                printf("Invalid current context\n");
            else
            {
                CTA_CONTEXT_INFOEX info;

                info.size = sizeof(char) * 64;
                info.svcnames = (char *)malloc(info.size);

                if ((ret = ctaGetContextInfoEx(ctahd, &info)) == SUCCESS)
                {
                    printf("ctahd = 0x%x\tcontext name = %s\tqueuehd = 0x%x"
                           "\tuserid = 0x%x\n", ctahd, info.contextname,
                           info.ctaqueuehd, info.userid);
                    if (info.svcnames != NULL)
                    {
                        printf("Services opened are:\n");
                        PrintServiceList(info.svcnames);
                    }
                }
            }
            break;
        }
        case PAIR( 'C', 'H' ):              /* CHANGE CONTEXT */
        {
            CNXT_INFO cxinfo;

            cxinfo.ctahd = NULL_CTAHD;
            prompthex("Enter Context Handle", &(cxinfo.ctahd));
            if (ContextGet(&cxinfo) < 0)
                printf("Context not found\n");
            else
            {
                ContextBackup( ctahd, Vcehd, VceMemhd );
                ctahd = cxinfo.ctahd;
                strcpy(contextname, cxinfo.cName);
                Vcehd = cxinfo.vcehd;
                VceMemhd = cxinfo.vcememhd;
            }
            break;
        }
        case PAIR('G', 'M' ):               /* Get Event Sources */
        {
                char        *svcarr[MAX_SERVICES];
                int i;

                for (i=0; i<MAX_SERVICES; i++) {
                    svcarr[i] = malloc(sizeof(char) * 5);
                }
                ret = ctaGetEventSources(ctahd, (char**)svcarr, MAX_SERVICES);
                if (ret == SUCCESS)
                {
                    printf("The services which are masked are:\n");
                    for( i=0; i<MAX_SERVICES; i++ )
                    {
                        if( svcarr[i][0] != 0 )
                            printf( "%s\n", svcarr[i]);

                        free(svcarr[i]);
                    }
                }
                break;
        }
        case PAIR('S', 'M' ):               /* Set Event Sources */
        {
            char        *svcarr[MAX_SERVICES];
            int         nsvcs;

            if (!Batch)
            {
                printf("Enter list of services to mask; Press Enter to end the list\n");
            }
            nsvcs = PromptForStrings("Enter service name", svcarr,
                                     MAX_SVC_NAME_LEN);
            ret = ctaSetEventSources(ctahd, svcarr, nsvcs);
            for ( ; nsvcs >= 0; nsvcs--)
            {
                free(svcarr[nsvcs]);
            }
            break;
        }
        case PAIR( 'X', 0 ):                /* SHOW STATE */
            ret = ShowAdiState( ctahd );
            break;

        case PAIR( 'C', 'E' ):              /* CONTINUE (resume) EVENT FETCH */
            if (EventFetch != 2)
            {
                printf( "continuous event fetch enabled\n" );
                EventFetch = 2;
            }
            break;

        case PAIR( 'G', 'E' ):              /* GET (fetch) ONE EVENT */
            if (EventFetch != 1)
            {
                EventFetch = 1;
                printf( "single step get event\n" );
            }
            break;

        case PAIR( 'S', 'E' ):              /* STOP (pause) EVENT FETCH */
            EventFetch = 0;
            printf( "continuous event fetch disabled\n" );
            break;

        case PAIR( 'L', 'E' ):              /* LOOPBACK EVENT */
        {
            CTA_EVENT event;
            memset( &event, 0, sizeof( event ) );
            promptdw( "Enter event id (0-0xfff)", &event.id );
            event.id = CTA_USER_EVENT(event.id); /* convert to user event */

            if( CTA_IS_DONE_EVENT( event.id ) )
                promptdw( "Enter done reason", &event.value );
            else
                promptdw( "Enter value field", &event.value );
            promptdw( "Enter size/2nd value field", &event.size );
            event.ctahd = ctahd;
            ctaQueueEvent( &event );
            break;
        }
        case PAIR( 'S', 'P' ):              /* START PROTOCOL */
        {
            promptstr( "Enter protocol name", ProtocolName,
                                              sizeof(ProtocolName) );

            ret = StartProtocol( );
            
            break;
        }
        case PAIR( 'U', 'P' ):              /* STOP PROTOCOL */
        {
            if( NccInUseFlag == 1 )
            {
                ret = nccStopProtocol( ctahd, NULL );
            }
            else
            {
                ret = adiStopProtocol( ctahd );
            }
            break;
        }
        case PAIR( 'A', 'C' ):              /* ANSWER CALL */
        {
            static unsigned nrings=1;

            promptuns( "Number of rings", &nrings );
            if( NccInUseFlag == 1 )
            {
                ret = nccAnswerCall( callhd, nrings, NULL );
            }
            else
            {
                ret = adiAnswerCall( ctahd, nrings );
            }
            break;
        }
        case PAIR( 'C', 'C' ):              /* ACCEPT CALL */
        {
            int accept;
            static char acceptmode = 'r';

            promptchar( "Accept mode-(r)inging (q)uiet (p)lay user audio (u)ser audio through connect",
                        &acceptmode);

            if( NccInUseFlag == 1 )
            {
                switch( toupper( acceptmode ) )
                {
                    case 'R': accept = NCC_ACCEPT_PLAY_RING;                break;
                    case 'Q': accept = NCC_ACCEPT_PLAY_SILENT;              break;
                    case 'P': accept = NCC_ACCEPT_USER_AUDIO;               break;
                    case 'U': accept = NCC_ACCEPT_USER_AUDIO_INTO_CONNECT;  break;
                    default:  accept = 0;                                   break;
                }
                if( accept != 0 )
                {
                    ret = nccAcceptCall( callhd, accept, NULL );
                }
                else
                {
                    printf( "\t\t\007Illegal entry.\n" );
                }
            }
            else
            {
                switch( toupper( acceptmode ) )
                {
                    case 'R': accept = ADI_ACCEPT_PLAY_RING;                break;
                    case 'Q': accept = ADI_ACCEPT_QUIET;                    break;
                    case 'P': accept = ADI_ACCEPT_USER_AUDIO;               break;
                    case 'U': accept = ADI_ACCEPT_USER_AUDIO_INTO_CONNECT;  break;
                    default:  accept = 0;                                   break;
                }
                if( accept != 0 )
                {
                    ret = adiAcceptCall( ctahd, accept, NULL );
                }
                else
                {
                    printf( "\t\t\007Illegal entry.\n" );
                }
            }
            break;
        }
        case PAIR( 'C', 'I' ):              /* ACCEPT INCOMING ADDRESS */
        {
            if( NccInUseFlag == 1 )
            {
                printf( "\t\t\007Command invalid under NCC.\n" );
            }
            else
            {
                ret = adiAcceptIncomingAddress( ctahd );
            }
            break;
        }
        case PAIR( 'P', 'C' ):              /* PLACE CALL */
        {
            static char digits[40] = "123";
            int index;

            promptstr( "Enter digits to dial", digits, sizeof(digits) );
            for (index = strlen (digits) - 1; (index >= 0) && (*(digits + index) == ' '); *(digits + index--) = '\0');

            if( NccInUseFlag == 1 )
            {
                ret = nccPlaceCall( ctahd, digits, callerID, NULL, NULL, &callhd);
            }
            else
            {
                ret = adiPlaceCall( ctahd, digits, NULL );
            }
            break;
        }
        case PAIR( 'P', '2' ):              /* PLACE 2ND CALL */
        {
            static char digits[40] = "123";

            if( NccInUseFlag == 1 )
            {
                printf( "\t\t\007Command invalid under NCC.\n" );
            }
            else
            {
                promptstr( "Enter digits to dial", digits, sizeof(digits));
                ret = adiPlaceSecondCall( ctahd, digits, NULL );
            }
            break;
        }
        case PAIR( 'R', 'C' ):              /* RELEASE CALL */
        {
            if( NccInUseFlag == 1 )
            {
                NCC_CALLHD tempcallhd = callhd;
                if (tempcallhd == NULL_NCC_CALLHD)
                    tempcallhd = heldcallhd;

                prompthex( "Enter number of callhd to release", &tempcallhd );

                ret = nccReleaseCall( tempcallhd, NULL );
            }
            else
            {
                ret = adiReleaseCall( ctahd );
            }
            break;
        }
        case PAIR( 'R', '2' ):              /* RELEASE 2ND CALL */
        {
            if( NccInUseFlag == 1 )
            {
                printf( "\t\t\007Command invalid under NCC.\n" );
            }
            else
            {
                ret = adiReleaseSecondCall( ctahd );
            }
            break;
        }
        case PAIR( 'T', 'D'):
        {
            ToggleAutoDisconnect ^= 1;
            if (ToggleAutoDisconnect)
                printf("\t\tAutomatic nccReleaseCall ON.\n");
            else
                printf("\t\tAutomatic nccReleaseCall OFF.\n");
            break;
        }
        case PAIR( 'T', 'C' ):              /* TRANSFER CALL */
        {
            static char digits[40] = "123";
            static unsigned when = ADI_XFER_PROCEEDING;
            /* ADI_XFER_* has the same value as NCC_TRANSFER_* */

            promptstr( "Enter digits to dial", digits, sizeof(digits) );
            promptuns( "Transfer on: 1-proceeding, 2-alerting, 3-connected",
                       &when) ;

            if( NccInUseFlag == 1 )
            {
                ret = nccAutomaticTransfer( callhd, digits, when,
                                            NULL, NULL, NULL );
            }
            else
            {
                ret = adiTransferCall( ctahd, digits, when, NULL );
            }
            break;
        }
        case PAIR( 'J', 'C' ):              /* REJECT CALL */
        {
            int reject;
            static char rejmode= 'r' ;

            if( NccInUseFlag == 1 )
            {
                promptchar(
                           "Reject-(b)usy (f)ast-busy (r)ingback (p)lay user audio",
                           &rejmode);

                switch( toupper( rejmode ) )
                {
                    case 'B': reject = NCC_REJECT_PLAY_BUSY;     break;
                    case 'F': reject = NCC_REJECT_PLAY_REORDER;  break;
                    case 'R': reject = NCC_REJECT_PLAY_RINGTONE; break;
                    case 'P': reject = NCC_REJECT_USER_AUDIO;    break;
                    default:  reject = 0;                        break;
                }
                if( reject != 0 )
                    ret = nccRejectCall( callhd, reject, NULL );
                else
                    printf( "\t\t\007Illegal entry.\n" );
            }
            else
            {
                promptchar(
                           "Reject-(b)usy (f)ast-busy (r)ingback (p)lay user audio " \
                           "(i)mmediate termination", &rejmode);

                switch( toupper( rejmode ) )
                {
                    case 'B': reject = ADI_REJ_PLAY_BUSY;     break;
                    case 'F': reject = ADI_REJ_PLAY_REORDER;  break;
                    case 'R': reject = ADI_REJ_PLAY_RINGTONE; break;
                    case 'P': reject = ADI_REJ_USER_AUDIO;    break;
                    case 'I': reject = ADI_REJ_FORCE_IMMEDIATE;break;
                    default:  reject = 0;                     break;
                }
                if( reject != 0 )
                    ret = adiRejectCall( ctahd, reject );
                else
                    printf( "\t\t\007Illegal entry.\n" );
            }
            break;
        }
        case PAIR( 'B', 'C' ):              /* BLOCK CALLS */
        {
            if( NccInUseFlag == 1 )
            {
                char  selection = 'o';
                DWORD blockmode = 0;

                promptchar(
                           "(r)eject all, (o)out of service", &selection);

                switch (toupper(selection))
                {
                    case 'R': blockmode = NCC_BLOCK_REJECTALL; break;
                    case 'O': blockmode = NCC_BLOCK_OUT_OF_SERVICE; break;
                    default:  blockmode = NCC_BLOCK_REJECTALL; break;
                }

                ret = nccBlockCalls( ctahd, blockmode, NULL );
            }
            else
                ret = adiBlockCalls( ctahd );
            break;
        }
        case PAIR( 'U', 'C' ):              /* UNBLOCK CALLS */
        {
            if( NccInUseFlag == 1 )
            {
                ret = nccUnblockCalls( ctahd, NULL );
            }
            else
                ret = adiUnBlockCalls( ctahd );
            break;
        }
        case PAIR( 'C', '?' ):              /* SHOW CALL STATUS */
        {
            if( NccInUseFlag == 1 )
            {
                NCC_CALLHD tempcallhd = callhd;
                
                if (tempcallhd == NULL_NCC_CALLHD)
                {
                    tempcallhd = heldcallhd;
                }

                prompthex( "Enter number of callhd", &tempcallhd );

                ret = ShowNccCallState( tempcallhd );
            }
            else
            {
                ADI_CALL_STATUS   callstatus;

                ret = adiGetCallStatus( ctahd, &callstatus, sizeof(callstatus) );
                if ( ret == SUCCESS )
                    ShowCallStatus( &callstatus );
            }
            break;
        }
        case PAIR( 'I', '?' ):              /* SHOW EXTENDED CALL STATUS INFO */
        {
            if( NccInUseFlag == 1 )
            {
                NCC_PROT_CAP capabilities;
                NCC_CALLHD tempcallhd = callhd;

                ret = nccQueryCapability( ctahd, &capabilities, sizeof(capabilities) );
                if (capabilities.capabilitymask & NCC_CAP_EXTENDED_CALL_STATUS)
                {                
                    prompthex( "Enter number of callhd", &tempcallhd );
                    if (StrNoCaseCmp(ProtocolName, "ISD0") == 0)
                        ret = ShowISDNExtCallStatus( tempcallhd );
                    else
                        ret = ShowCASExtCallStatus( tempcallhd );
                }
                else
                {
                    printf( "\t\t\007Protocol does not support Extended Call Status.\n" );
                }
            }
            else
            {
                printf( "\t\t\007Command valid only under NCC.\n" );
            }
            break;
        }
        case PAIR( 'C', 'M' ):              /* SEND CALL MESSAGE */
        {
            if( NccInUseFlag == 1 )
            {
                if (StrNoCaseCmp(ProtocolName, "ISD0") == 0)
                {
                    ret = SendISDNCallMessage( callhd );
                }
            }
            else
            {
                printf( "\t\t\007Command valid only under NCC.\n" );
            }
            break;
        }
        case PAIR( 'I', 'D' ):              /* SET CALLER ID */
        {
            int index;
            promptstr( "Enter caller ID digits (space for none)", callerID, sizeof(callerID) );
            for (index = strlen (callerID) - 1; (index >= 0) && (*(callerID + index) == ' '); *(callerID + index--) = '\0');
            break;
        }
        case PAIR( 'G', 'N' ):              /* GENERATE NETWORK TONE */
        {
            int tonecnt = 1;
            static char nettone = 'd';
            ADI_TONE_PARMS p[3];

            memset( &p, 0, sizeof( p ) );

            promptchar( "NetTone-(d)ialtone (b)usy (f)ast-busy (r)ing (s)IT",
                        &nettone );
            switch( toupper( nettone ) )
            {
                case 'D':
                    p[0].freq1   = 350;    p[0].ampl1   = -16;
                    p[0].freq2   = 440;    p[0].ampl2   = -16;
                    p[0].ontime  = 10000;  p[0].offtime = 0;
                    p[0].iterations = -1;  break;
                case 'B':
                    p[0].freq1   = 480;    p[0].ampl1   = -24;
                    p[0].freq2   = 620;    p[0].ampl2   = -24;
                    p[0].ontime  = 500;    p[0].offtime = 500;
                    p[0].iterations = -1;  break;
                case 'F':
                    p[0].freq1   = 480;    p[0].ampl1   = -24;
                    p[0].freq2   = 620;    p[0].ampl2   = -24;
                    p[0].ontime  = 250;    p[0].offtime = 250;
                    p[0].iterations = -1;  break;
                case 'R':
                    p[0].freq1   = 440;    p[0].ampl1   = -24;
                    p[0].freq2   = 480;    p[0].ampl2   = -24;
                    p[0].ontime  = 1000;   p[0].offtime = 3000;
                    p[0].iterations = -1;  break;
                case 'S':
                    {
                        static char sittone = '1';

                        tonecnt = 3;
                        promptchar( "SIT- 1.InterCept  2.ReOrder(intra)   "
                                    "3.ReOrder(inter)   4.IneffectiveOther\n"
                                    "     5.VacantCode 6.NoCircuit(intra) "
                                    "7.NoCircuit(inter)", &sittone );

                        /*--------------------------------------------------*/
#define DEFINE_SIT( p, t1,t2,t3, f1,f2,f3, ampl )\
                        p[0].freq1  = f1;          \
                            p[0].ampl1  = ampl;        \
                            p[0].ontime = t1;          \
                            p[0].iterations = 1;           \
                            p[1].freq1  = f2;          \
                            p[1].ampl1  = ampl;        \
                            p[1].ontime = t2;          \
                            p[1].iterations = 1;           \
                            p[2].freq1  = f3;          \
                            p[2].ampl1  = ampl;        \
                            p[2].ontime = t3;          \
                            p[2].iterations = 1

                            /*--------------------------------------------------*/

                            switch( toupper( sittone ) ) /* SIT defined by BellCore */
                            {
                                case '1':  /* "InterCept" - SSL lll */
                                    DEFINE_SIT( p, 274,274,380,  914,1371,1777, -24 );
                                    break;

                                case '2':  /* "ReOrder(intra)" - SLL lhl */
                                    DEFINE_SIT( p, 274,380,380,  914,1429,1777, -24 );
                                    break;

                                case '3':  /* "ReOrder(inter)" - SLL hll */
                                    DEFINE_SIT( p, 274,380,380,  985,1371,1777, -24 );
                                    break;

                                case '4':  /* "IneffectiveOther" - SLL lhl */
                                    DEFINE_SIT( p, 274,380,380,  914,1429,1777, -24 );
                                    break;

                                case '5':  /* "Vacant Code" - LSL hll */
                                    DEFINE_SIT( p, 380,274,380,  985,1371,1777, -24 );
                                    break;

                                case '6':  /* "NoCircuit(intra)" - LLL hhl */
                                    DEFINE_SIT( p, 380,380,380,  985,1429,1777, -24 );
                                    break;

                                case '7':  /* "NoCircuit(inter)" - LLL lll */
                                    DEFINE_SIT( p, 380,380,380,  914,1371,1777, -24 );
                                    break;

                                default:
                                    printf( "\t\t\007Illegal entry.\n" );
                                    tonecnt = 0;
                                    break;
                            }
                    }
                    break;
                default:
                    printf( "\t\t\007Illegal entry.\n" );
                    tonecnt = 0;
                    break;
            }
            if( tonecnt )
            {
                ret = adiStartTones( ctahd, tonecnt, p );
                if( ret == SUCCESS )
                {
                    printf( "\nTones started; you can interrupt with 'gs'.\n" );
                }
            }
            break;
        }
        case PAIR( 'G', 'T' ):              /* GENERATE USER TONE */
        {
            static ADI_TONE_PARMS p;  /* static to remember your last entry */

            if( p.freq1 == 0 )
                ret = ctaGetParms( ctahd,
                                   ADI_TONE_PARMID, &p, sizeof(ADI_TONE_PARMS) );

            promptdw( "Enter freq1"  , &p.freq1 );
            prompt  ( "Enter ampl1"  , &p.ampl1 );
            promptdw( "Enter freq2"  , &p.freq2 );
            prompt  ( "Enter ampl2"  , &p.ampl2 );
            promptdw( "Enter ontime" , &p.ontime );
            promptdw( "Enter offtime", &p.offtime );
            prompt  ( "Enter iterations" , &p.iterations );

            ret = adiStartTones( ctahd, 1, &p );
            if( ret == SUCCESS )
            {
                printf( "\nTones started; you can interrupt with 'gs'.\n" );
            }

            break;
        }
        case PAIR( 'G', 'D' ):              /* GENERATE DTMFs */
        {
            static char dtmfs[40] = "1234567890ABCD*#";

            promptstr( "Enter DTMF string", dtmfs, sizeof(dtmfs) );

            /* dial all DTMFs using default parameters */
            ret = adiStartDTMF( ctahd, dtmfs, NULL );
            break;
        }
        case PAIR( 'G', 'S' ):              /* GENERATION STOP */
            ret = adiStopTones( ctahd );
            break;

        case PAIR( 'E', 'D' ):              /* ENABLE DTMF DETECTOR */
            /* turn on the DTMF detector using default parameters */
            ret = adiStartDTMFDetector( ctahd, NULL );
            break;

        case PAIR( 'D', 'D' ):              /* DISABLE DTMF DETECTOR */
            ret = adiStopDTMFDetector( ctahd );
            break;

        case PAIR( 'E', 'M' ):              /* ENABLE MF DETECTOR */
        {
            int mftype;
            static char mfchoice = 'u';

            promptchar( "MF-(U)S (F)orward CCITT (B)ackward CCITT", &mfchoice );
            switch( toupper( mfchoice ) )
            {
                default:
                case 'U': mftype = ADI_MF_US;             break;
                case 'F': mftype = ADI_MF_CCITT_FORWARD;  break;
                case 'B': mftype = ADI_MF_CCITT_BACKWARD; break;
            }

            /* turn on the MF detector */
            ret = adiStartMFDetector( ctahd, mftype );
            break;
        }
        case PAIR( 'D', 'M' ):              /* DISABLE MF DETECTOR */
            ret = adiStopMFDetector( ctahd );
            break;

        case PAIR( 'D', 'G' ):              /* DIGIT GET */
        {
            char digit;
            ret = adiGetDigit( ctahd, &digit );
            if( ret == SUCCESS )
            {
                if( digit != '\0' )
                    printf( "Got DTMF '%c'.\n", digit );
                else
                    printf( "No digits in the queue.\n" );
            }
            break;
        }
        case PAIR( 'D', 'F' ):              /*  DIGIT FLUSH */
            ret = adiFlushDigitQueue( ctahd );
            break;

        case PAIR( 'C', 'D' ):              /* COLLECT DIGITS */
        {
            static int       numDigits = 5;
            ADI_COLLECT_PARMS parms ;

            ret = ctaGetParms( ctahd,
                               ADI_COLLECT_PARMID, &parms, sizeof parms );
            if (ret != 0)
                break ;

            prompt( "Enter number of digits", &numDigits );
            ret = adiCollectDigits( ctahd, DigitsCollected,
                                    numDigits, &parms );
            break;
        }
        case PAIR( 'S', 'C' ):              /* STOP DIGIT COLLECTION */
            ret = adiStopCollection( ctahd );
            break;

        case PAIR( 'D', 'P' ):              /* DIGIT PEEK */
        {
            char digit;
            ret = adiPeekDigit( ctahd, &digit );
            if( ret == SUCCESS )
            {
                if( digit != '\0' )
                    printf( "Peeked at DTMF '%c'.\n", digit );
                else
                    printf( "No digits in the queue.\n" );
            }
            break;
        }
        case PAIR( 'F', 'S' ):              /* FSK SEND */
        {
            static char textbuf[256] = "Hello world!";

            promptstr( "Enter text to send", textbuf, sizeof(textbuf) );
            ret = adiStartSendingFSK (ctahd, textbuf, strlen(textbuf)+1, NULL);
            break;
        }
        case PAIR( 'F', 'R' ):              /* FSK RECIEVE */
        {
            /* Submit static 'Up' buffer */
            static char FSKbuf[1024] = {0};
            ret = adiStartReceivingFSK (ctahd, FSKbuf, sizeof FSKbuf, NULL) ;
            break;
        }
        case PAIR( 'F', 'A' ):              /* FSK ABORT RECIEVE */
            ret = adiStopReceivingFSK (ctahd);
            break;

        case PAIR( 'E', 'B' ):              /* ENABLE BIT DETECTOR */
            {
            /* assume no bits set; watch all bit transitions */
            static unsigned int bitmask = ADI_A_BIT | ADI_B_BIT | ADI_C_BIT | ADI_D_BIT;

            promptuns( "Enter bit mask to monitor", &bitmask);
            ret = adiStartSignalDetector( ctahd, 0, bitmask, 10, 10 );
            break;
            }

        case PAIR( 'D', 'B' ):              /* DISABLE BIT DETECTOR */
            ret = adiStopSignalDetector( ctahd );
            break;

        case PAIR( 'S', 'Q'):               /* QUERY (show) SIGNAL STATE */
            ret = adiQuerySignalState( ctahd );
            break;

        case PAIR( 'D', 'S'):               /* Define Signal patterns */
        {
            static char  pattern_str[128] = "0 8 0 8 0 8 0 8";
            WORD pattern[8] = { 0, 8, 0, 8, 0, 8, 0, 8 };

            promptstr( "\tidle seize break seize_ack flash answer clear block", pattern_str, sizeof(pattern_str) );
            sscanf( pattern_str, "%u %u %u %u %u %u %u %u", 
                    &pattern[0],
                    &pattern[1],
                    &pattern[2],
                    &pattern[3],
                    &pattern[4],
                    &pattern[5],
                    &pattern[6],
                    &pattern[7]);
            ret = adiDefineSignal ( ctahd, pattern, 8);

            break;
        }   
        case PAIR( 'P', 'F' ):              /* PLAY FILE */
        {
            VCE_PLAY_PARMS playparms;

            if (Vcehd != 0)
            {
                printf ("Voice handle busy\n");
                break ;
            }

            ret = ctaGetParms( ctahd,
                               VCE_PLAY_PARMID, &playparms,
                               sizeof(VCE_PLAY_PARMS) );
            if( ret != SUCCESS )
                break;

            playparms.speed     = VCE_CURRENT_VALUE;

            promptstr( "Enter file name", VoiceFileName,
                                          sizeof(VoiceFileName) );

            filetype = PromptForFileType();
            encoding = PromptForEncoding(VCE_PLAY_ONLY);

            ret = vceOpenFile(ctahd, VoiceFileName, filetype, VCE_PLAY_ONLY,
                              encoding, &Vcehd);
            if (ret != SUCCESS)
                break;

            ret = vcePlayMessage( Vcehd, 0, &playparms );
            if( ret != SUCCESS )
            {
                vceClose(Vcehd);
                Vcehd = 0;
            }
            break;
        }
        case PAIR( 'P', 'M' ):              /* PLAY FROM MEMORY */
        {
            if( adiMemoryFunctions == 0 )
            {
                VCE_PLAY_PARMS playparms;

                if ( VceMemhd == 0)
                {
                    printf( "Nothing recorded\n");
                    break ;
                }

                ret = ctaGetParms( ctahd,
                                   VCE_PLAY_PARMID, &playparms,
                                   sizeof(VCE_PLAY_PARMS) );

                if( ret != SUCCESS )
                    break;

                playparms.DTMFabort = 0;
                playparms.speed     = VCE_CURRENT_VALUE;

                /* Play message #1 - this will be empty unless record has been called */
                ret = vcePlayMessage( VceMemhd, 1, &playparms );
            }
            else
            {
                ADI_PLAY_PARMS playparms;

                ret = ctaGetParms( ctahd,
                                   ADI_PLAY_PARMID, &playparms,
                                   sizeof(ADI_PLAY_PARMS) );
                if( ret == SUCCESS )
                {
                    DWORD   RecordedBytes = adiMemoryDataSize;
                    playparms.DTMFabort = 0;     /* don't abort on DTMF */

                    ret = adiPlayFromMemory( ctahd, ADI_ENCODE_NMS_24,
                                             MemoryBuffer, RecordedBytes,
                                             &playparms );
                }
            }

            break;
        }
        case PAIR( 'R', 'F' ):              /* RECORD FILE */
            if ( Vcehd != 0)
            {
                printf( "Voice handle busy\n");
                break ;
            }

            promptstr( "Enter file name", VoiceFileName,
                                          sizeof(VoiceFileName));

            filetype = PromptForFileType();

            encoding = PromptForEncoding(VCE_PLAY_RECORD);

            remove(VoiceFileName);
            ret = vceCreateFile(ctahd, VoiceFileName, filetype, encoding, NULL,
                                &Vcehd);
            if (ret != SUCCESS)
                break;

            ret = vceRecordMessage( Vcehd, 0, VCE_NO_TIME_LIMIT, NULL);
            if( ret != SUCCESS )
            {
                vceClose(Vcehd);
                Vcehd = 0;
            }
            break;

        case PAIR( 'R', 'M' ):              /* RECORD TO MEMORY */
        {
            if( adiMemoryFunctions == 0 )
            {
                VCE_RECORD_PARMS  recordparms;

                if (VceMemhd == 0)
                {
                    ret = vceCreateMemory (ctahd, sizeof(MemoryBuffer),
                                           VCE_ENCODE_NMS_24, &VceMemhd);
                    if( ret != SUCCESS )
                        break;
                }

                ret = vceEraseMessage (VceMemhd, 0);  /* Enable record to message #1 */
                if( ret != SUCCESS )
                    break;
                ret = ctaGetParms( ctahd,
                                   VCE_RECORD_PARMID, &recordparms,
                                   sizeof(VCE_RECORD_PARMS) );
                if( ret != SUCCESS )
                    break;

                recordparms.DTMFabort = 0;
                ret = vceRecordMessage (VceMemhd, 1, VCE_NO_TIME_LIMIT, &recordparms);
            }
            else
            {
                ADI_RECORD_PARMS recordparms;

                ret = ctaGetParms( ctahd,
                                   ADI_RECORD_PARMID, &recordparms,
                                   sizeof(ADI_RECORD_PARMS) );
                if( ret == SUCCESS )
                {
                    recordparms.DTMFabort = 0;
                    ret = adiRecordToMemory( ctahd, ADI_ENCODE_NMS_24,
                                             MemoryBuffer, sizeof(MemoryBuffer),
                                             &recordparms );
                }
            }

            break;
        }
        case PAIR( 'R', '?' ):              /* SHOW PLAY/RECORD STATUS */
        case PAIR( 'P', '?' ):
        {
            VCE_CONTEXT_INFO contextinfo;

            ret = vceGetContextInfo( ctahd, &contextinfo, sizeof contextinfo );
            if( ret != SUCCESS )
                break;

            printf("Active function:   %s\n", contextinfo.function == VCE_PLAY ?
                   "play" :
                   contextinfo.function == VCE_RECORD?
                   "record":
                   contextinfo.function == 0?
                   "none" :  "???");

            printf("Current Position:  %d ms\n",     contextinfo.position);
            printf("Current Gain:      %d db\n",         contextinfo.currentgain);
            printf("Current Speed:     %d%\n",          contextinfo.currentspeed);
            if (contextinfo.function != 0)
            {
                printf("Current Encoding:  %d\n",        contextinfo.encoding);
                printf("Current Framesize: %d bytes\n", contextinfo.framesize);
                printf("Current Frametime: %d ms\n",    contextinfo.frametime);
            }
            printf( "Last termination condition: " );
            if (contextinfo.reasondone != 0)
                ShowReason( contextinfo.reasondone );
            else
                puts ("none");
            break;
        }
        case PAIR( 'P', 'S' ):              /* STOP PLAY/RECORD */
        case PAIR( 'R', 'S' ):
            ret = vceStop( ctahd );
            break;

        case PAIR( 'M', 'G' ):              /* MODIFY PLAY GAIN */
        {
            static int db = 0;

            prompt ( "Enter play gain (+-db)", &db );
            ret = vceSetPlayGain (ctahd, db) ;
            break ;
        }
        case PAIR( 'M', 'S' ):              /* MODIFY PLAY SPEED */
        {
            static unsigned speed = 100;
            
            VCE_PLAY_PARMS playparms;

            ret = ctaGetParms( ctahd,
                               VCE_PLAY_PARMID, &playparms,
                               sizeof(VCE_PLAY_PARMS) );
            if( ret != SUCCESS )
                break;

            if (playparms.maxspeed <= 100)
            {
                printf("\007"
 "  ** NOTE: VCEPLAY.maxspeed parameter is %d. Speed control is disabled.**\n\n",
                       playparms.maxspeed);
            }
            
            promptuns ( "Enter play speed (percent)", &speed );
            ret = vceSetPlaySpeed (ctahd, speed) ;
            break ;
        }
        case PAIR( 'E', 'I' ):              /* SHOW ENCODING INFO */
        {
            unsigned framesize, frametime ;

            encoding = PromptForEncoding(VCE_PLAY_RECORD);

            ret = vceGetEncodingInfo (ctahd, encoding, &framesize, &frametime);
            if (ret == SUCCESS)
            {
                printf ("Frame size   = %d bytes\n", framesize);
                printf ("Frame time   = %d msec\n", frametime);
            }
            break ;
        }


        case PAIR( 'E', 'T' ):              /* ENABLE TONE DETECTOR */
        {
            unsigned toneid     = 1;
            static unsigned frequency1 = 350;
            static unsigned bandwidth1 =  50;
            static unsigned frequency2 = 440;
            static unsigned bandwidth2 =  50;  /* default to dialtone */

            promptuns( "Enter toneid",     &toneid );
            promptuns( "Enter frequency1", &frequency1 );
            promptuns( "Enter bandwidth1", &bandwidth1 );
            promptuns( "Enter frequency2", &frequency2 );
            promptuns( "Enter bandwidth2", &bandwidth2 );

            ret = adiStartToneDetector( ctahd, toneid,
                                        frequency1, bandwidth1,
                                        frequency2, bandwidth2, NULL );
            break;
        }
        case PAIR( 'D', 'T' ):              /* DISABLE TONE DETECTOR */
        {
            unsigned toneid = 1;

            promptuns( "Enter toneid", &toneid );
            ret = adiStopToneDetector( ctahd, toneid );
            break;
        }
        case PAIR( 'E', 'E' ):              /* ENABLE ENERGY DETECTOR */
        {
            static unsigned qualifyenergy = 100 ;
            static unsigned qualifysilence = 1000 ;

            promptuns( "Enter energy qualification time in ms",
                       &qualifyenergy  );
            promptuns( "Enter silence qualification time in ms",
                       &qualifysilence );

            ret = adiStartEnergyDetector( ctahd,
                                          qualifyenergy ,
                                          qualifysilence,
                                          NULL );
            break;
        }
        case PAIR( 'D', 'E' ):              /* DISABLE ENERGY DETECTOR */
            ret = adiStopEnergyDetector( ctahd );
            break;

        case PAIR( 'S', 'D' ):              /* START DIALING */
        {
            static char  digits[40]      = "123";

            promptstr( "Enter digits to dial", digits, sizeof(digits) );
            ret = adiStartDial( ctahd, digits, NULL );
            break;
        }
        case PAIR( 'A', 'D' ):              /* ABORT DIALING */
            ret = adiStopDial( ctahd );
            break;

        case PAIR( 'C', 'B' ):              /* CALL-PROGRESS BEGIN */
            ret = adiStartCallProgress( ctahd, NULL );
            break;

        case PAIR( 'C', 'S' ):              /* CALL-PROGRESS STOP */
            ret = adiStopCallProgress( ctahd );
            break;

        case PAIR( 'S', 'T' ):              /* START TIMER */
        {
            static unsigned timerDuration = 5000;
            static unsigned timerTicks    = 1;

            promptuns( "Enter duration in ms" , &timerDuration );
            promptuns( "Enter number of ticks", &timerTicks );

            ret = adiStartTimer( ctahd, timerDuration, timerTicks );
            break;
        }

        case PAIR( 'A', 'T' ):              /* ABORT TIMER */
            ret = adiStopTimer( ctahd );
            break;

        case PAIR( 'T', 'P' ):              /* TOGGLE SIGNAL PATTERN */
        {
            static unsigned Pattern = 0;

            /* toggle pattern between A , B , C & D bits and 0 */
            if( Pattern == 0 )
                Pattern = ADI_A_BIT + ADI_B_BIT + ADI_C_BIT + ADI_D_BIT;
            else
                Pattern = 0;

            ret = adiAssertSignal( ctahd, Pattern );

            break;
        }

        case PAIR( 'A', 'S'):               /* ASSERT SIGNAL */
        {
            static unsigned signal, ton, toff;

            promptuns( "Enter signal", &signal );
            ret = adiAssertSignal( ctahd, signal );
            break;
        }

        case PAIR( 'G', 'W'):               /* GENERATE WINK */
            ret = adiStartPulse( ctahd, 0x0C, 200, 0 );
            break;

        case PAIR( 'R', 'P' ):              /* REFRESH PARMS */
            ret = ctaRefreshParms( ctahd );
            break;

        case PAIR( 'P', 'I' ):              /* GET PARMID */
        {
            char            parmname[80] = "";
            unsigned        parmid;

            promptstr("Enter parameter name", parmname, sizeof(parmname));
            if ((ret = ctaGetParmId( ctahd, parmname, &parmid )) == SUCCESS)
                printf("Parameter id for %s is 0x%x\n", parmname, parmid);
            break;
        }
        case PAIR( 'S', 'V' ):              /* GET SERVICE VERSION */
        {
            CTA_SERVICE_NAME        svcname;
            CTA_REV_INFO            revinfo;

            svcname.svcname = (char *)malloc(CTA_CONTEXT_NAME_LEN);
            svcname.svcmgrname = (char *)malloc(CTA_CONTEXT_NAME_LEN);
            *(svcname.svcname) = '\0';
            *(svcname.svcmgrname) = '\0';

            promptstr("Enter service name", svcname.svcname,
                       CTA_CONTEXT_NAME_LEN );
            promptstr("Enter service manager name", svcname.svcmgrname,
                       CTA_CONTEXT_NAME_LEN );

            if ((ret = ctaGetServiceVersionEx( ctahd, &svcname, &revinfo, sizeof(revinfo) ))
                == SUCCESS)
                printf("%s, %s Version %d.%02d %s Compat level %d\n",
                       svcname.svcname, svcname.svcmgrname, revinfo.majorrev,
                       revinfo.minorrev, revinfo.builddate, revinfo.compatlevel);
            break;
        }
        case PAIR( 'V', 'D' ):              /* SHOW PARM */
        {
            static unsigned onCres = 0;
            char temp[80];

            if (ctahd != NULL_CTAHD)
                promptuns("Show Natural Access context values ?" , &onCres);
            else
                onCres = 0;

            if (onCres)
                printf("Showing Natural Access context defaults.\n");
            else
            {
                if (SharedMemParms)
                    printf("Showing system shared memory defaults.\n");
                else
                    printf("Showing process defaults.\n");
            }

            promptline("Enter parameter name or category or ? to see categories: ",
                       temp, sizeof temp);

            if (onCres)
                ret = ShowDefaultParm(ctahd, temp);
            else
                ret = ShowDefaultParm(NULL_CTAHD, temp);
            break;
        }
        case PAIR( 'P', 'D' ):              /* SET PARM */
        {
            static unsigned onCres = 0;
            char temp[80];

            if (ctahd != NULL_CTAHD)
                promptuns("Set Natural Access context values ?" , &onCres);
            else
                onCres = 0;

            if (onCres)
                printf("Setting Natural Access context defaults.\n");
            else
            {
                if (SharedMemParms)
                    printf("Setting system shared memory defaults.\n");
                else
                    printf("Setting process defaults.\n");
            }

            promptline("Enter parameter name to set: ", temp, sizeof temp);

            if (onCres)
                ret = SetDefaultParm(ctahd, temp);
            else
                ret = SetDefaultParm(NULL_CTAHD, temp);
            break;
        }
        case PAIR( 'F', 'F' ):              /* FIND FILE */
        {
            char tempname[256];
            char tempext[50];
            char tempenv[50];
            char tempfull[256];

            promptline("Enter file name: ",
                                                 tempname, sizeof tempname);
            promptline("Enter extension (optional): ",
                                                 tempext,  sizeof tempext);
            promptline("Enter search path environment variable: ",
                                                 tempenv,  sizeof tempenv);

            ret = ctaFindFileEx(ctahd, tempname, tempext, tempenv,
                                tempfull, sizeof tempfull);

            if (ret == SUCCESS)
                printf("%s\n", tempfull);
            break;
        }
        case PAIR( 'E', 'H' ):              /* SET ERROR HANDLER */
        {
            static char errhdlr = 'u';

            promptchar("Error Handler\n (r)-CTA_REPORT_ERRORS,\n "
                       "(x)-CTA_EXIT_ON_ERRORS,\n (n)-NULL,\n (u)-user defined handler",
                       &errhdlr);

            switch( toupper( errhdlr ) )
            {
                case 'R':
                    ctaSetErrorHandler( CTA_REPORT_ERRORS, NULL );
                    break;
                case 'X':
                    ctaSetErrorHandler( CTA_EXIT_ON_ERRORS, NULL );
                    break;
                case 'N':
                    ctaSetErrorHandler( NULL, NULL );
                    break;
                case 'U':
                    ctaSetErrorHandler( ErrorHandler, NULL );
                    break;
                default:
                    printf("\t\t\007Illegal Entry\n");
                    break;
            }
            break;
        }
        case PAIR( 'T', 'S' ):              /* SET TIME SLOT */
            promptdw("Enter timeslot", &(pServices[0].mvipaddr.timeslot));
            if( NccInUseFlag )
            {
                /* The timeslot for the NCC service also needs to be set... */
                pServices[1].mvipaddr.timeslot = pServices[0].mvipaddr.timeslot;
            }
            break;

        case PAIR( 'D', 'H' ):              /* GET CONTEXT DESCRIPTOR */
        {
            char descriptor[128] = {0};
            char callhddesc[128] = {0};
            unsigned c = 2;

            promptuns( "Enter descriptor type, 1=context, 2=object", &c );

            switch ( c )
            {
                case 1:
                {
                    if ( ctaGetObjDescriptor(ctahd, descriptor, 128) == SUCCESS )
                    {
                        printf("Descriptor = %s\n", descriptor);
                    }
                    else
                    {
                        printf("Unable to get descriptor for ctahd 0x%x\n", ctahd);
                    }
                    break;
                }
                case 2:
                {
                    NCC_CALLHD tempcallhd = callhd;
                    prompthex( "Enter callhd", &tempcallhd );

                    if ( ctaGetObjDescriptor(tempcallhd, callhddesc, 128) == SUCCESS )
                    {
                        printf("Descriptor = %s\n", callhddesc);
                    }
                    else
                    {
                        printf("Unable to get descriptor for ctahd 0x%x\n", ctahd);
                    }
                    break;
                }
                default:
                    printf("Invalid choice.\n");
                    break;
            }
            break;
        }

        /* New commands for NCC only */

        case PAIR( 'D', 'C' ):              /* DISCONNECT CALL */
        {
            if( NccInUseFlag == 0 )
            {
                printf( "\t\t\007Command valid only under NCC.\n" );
            }
            else
            {
                if (callhd == NULL_NCC_CALLHD)
                {
                    ret = nccDisconnectCall( heldcallhd, NULL );
                }
                else
                {
                    ret = nccDisconnectCall( callhd, NULL );
                }
            }
            break;
        }
        case PAIR( 'L', '?' ):              /* GET LINE STATUS */
        {
            if( NccInUseFlag == 0 )
            {
                printf( "\t\t\007Command valid only under NCC.\n" );
            }
            else
            {
                ret = ShowNccLineState( ctahd );
            }
            break;
        }
        case PAIR( 'O', 'C' ):              /* HOLD CALL */
        {
            if( NccInUseFlag == 0 )
            {
                printf( "\t\t\007Command valid only under NCC.\n" );
            }
            else
            {
                ret = nccHoldCall( callhd, NULL );
            }
            break;
        }
        case PAIR( 'O', 'R' ):              /* HOLD RESPONSE */
        {
            if( NccInUseFlag == 0 )
            {
                printf( "\t\t\007Command valid only under NCC.\n" );
            }
            else
            {
                static char  selection = 'd';
                DWORD respondhow = 1;

                promptchar("(i)ignore, (d)hold with dial tone and digit collection, (h)hold only, (r)retrieve", &selection);

                switch (toupper(selection))
                {
                    case 'I': respondhow = 0; break;
                    case 'D': respondhow = 1; break;
                    case 'H': respondhow = 2; break;
                    case 'R': respondhow = 3; break;
                    default:  respondhow = 0; break;
                }
                if (callhd == NULL_NCC_CALLHD)
                {
                    ret = nccHoldResponse (heldcallhd, respondhow, NULL);
                }
                else
                {
                    ret = nccHoldResponse (callhd, respondhow, NULL);
                }
            }
            break;
        }
        case PAIR( 'S', 'B' ):              /* SET BILLING */
        {
            if( NccInUseFlag == 0 )
            {
                printf( "\t\t\007Command valid only under NCC.\n" );
            }
            else
            {
                static char  selection = 'd';
                DWORD rate;

                promptchar("(f)free, (d)default, (a)alternate charge, (p)send pulse", &selection);

                switch (toupper(selection))
                {
                    case 'F': rate = NCC_BILLINGSET_FREE;               break;
                    case 'A': rate = NCC_BILLINGSET_CHARGE_ALTERNATE;   break;
                    case 'P': rate = NCC_BILLINGSET_SEND_PULSE;         break;
                    default:  rate = NCC_BILLINGSET_DEFAULT;            break;
                }
                ret = nccSetBilling( callhd, rate, NULL );
            }
            break;
        }
        case PAIR( 'C', 'Q' ):              /* QUERY CAPABILITY */
        {
            NCC_PROT_CAP capabilities;

            if( NccInUseFlag == 0 )
            {
                printf( "\t\t\007Command valid only under NCC.\n" );
                break;
            }

            ret = nccQueryCapability( ctahd, &capabilities, sizeof(capabilities) );

            if( ret == SUCCESS )
                ShowCapabilities( &capabilities );

            break;
        }
        case PAIR( 'R', 'R' ):              /* RETRIEVE CALL */
        {
            if( NccInUseFlag == 0 )
            {
                printf( "\t\t\007Command valid only under NCC.\n" );
                break;
            }
            else
            {
                ret = nccRetrieveCall( heldcallhd, NULL );
                break;
            }
        }
        case PAIR( 'N', 'D' ):              /* SEND DIGITS */
        {
            static char  digits[40]      = "123";

            if( NccInUseFlag == 0 )
            {
                printf( "\t\t\007Command valid only under NCC.\n" );
            }
            else
            {
                promptstr( "Enter digits to send", digits, sizeof(digits) );
                ret = nccSendDigits( callhd, digits, NULL );
            }
            break;
        }
        case PAIR( 'V', 'T' ):              /* SUPERVISED TRANSFER */
        {
            if( NccInUseFlag == 0 )
            {
                printf( "\t\t\007Command valid only under NCC.\n" );
                break;
            }
            else
            {
                ret = nccTransferCall( callhd, heldcallhd, NULL );
            }
            break;
        }
        case PAIR( 'T', '2' ):              /* 2 CHANNEL TRANSFER */
        {
            if( NccInUseFlag == 0 )
            {
                printf( "\t\t\007Command valid only under NCC.\n" );
                break;
            }
            else
            {
                static char  callid_str[128] = "0";
                TRANSFERCALL_EXT ext = { 0 };
                
                ext.size = sizeof(ext);
                
                promptstr( "\tCall ID of 2nd call", callid_str, sizeof(callid_str) );
                sscanf( callid_str, "%hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx", 
                        &ext.id[0], 
                        &ext.id[1], 
                        &ext.id[2], 
                        &ext.id[3], 
                        &ext.id[4], 
                        &ext.id[5], 
                        &ext.id[6], 
                        &ext.id[7], 
                        &ext.id[8], 
                        &ext.id[9], 
                        &ext.id[10], 
                        &ext.id[11], 
                        &ext.id[12], 
                        &ext.id[13], 
                        &ext.id[14] 
               );  
                
                ret = nccTransferCall( callhd, 0, &ext );
            }
            break;
        }
        case PAIR( 'A', 'O' ):      /* ATTACH OBJECT TO NATURAL ACCESS CONTEXT */
        {
            unsigned c = 1;
            char callhddesc[128] = {0};
            unsigned newcallhd;

            promptuns( "Enter call type, 1=active, 2=held", &c );

            promptstr("Enter callobj descriptor:", callhddesc, sizeof(callhddesc));

            switch ( c )
            {
                case 1:
                    ret = ctaAttachObject( &ctahd, callhddesc, &newcallhd );
                    if ( ret == SUCCESS )
                    {
                        callhd = (NCC_CALLHD)newcallhd;
                        printf( "\t\tCallhd = 0x%x\n", callhd );
                    }
                    break;
                case 2:
                    ret = ctaAttachObject( &ctahd, callhddesc, &newcallhd);
                    if ( ret == SUCCESS )
                    {
                        heldcallhd = (NCC_CALLHD)newcallhd;
                        printf( "\t\theldCallhd = 0x%x\n", heldcallhd );
                    }
                    break;
                default:
                    printf("Invalid choice.\n");
                    break;
            }

            break;
        }
        case PAIR ( 'D', 'O' ):    /* DETACH OBJECT FROM NATURAL ACCESS CONTEXT */
        {
            prompthex( "Enter callhd", &callhd );

            ret = ctaDetachObject(callhd);

            break;
        }
        case PAIR( 'R', 'V' ):              /* GET SERVER VERSION */
        {
            CTA_REV_INFO revinfo = { 0 } ;

            ret = ctaGetVersionEx(ctahd, &revinfo, sizeof revinfo);

            if( ret == SUCCESS )
            {
                printf("Natural Access server Version %d.%02d %s Compat level %d\n",
                       revinfo.majorrev, revinfo.minorrev,
                       revinfo.builddate, revinfo.compatlevel);
            }
            break;
        }
        case PAIR( 'F', 'E' ):              /* FORMAT EVENT */
        {
            char buffer[256];
            CTA_EVENT event;

            memset(&event, 0, sizeof event);
            promptdw("Enter event id (0-0xfff)", &event.id);
            event.id = CTA_USER_EVENT(event.id); /* convert to user event */

            if( CTA_IS_DONE_EVENT(event.id) )
                promptdw("Enter done reason", &event.value);
            else
                promptdw("Enter value field", &event.value);
            promptdw("Enter size/2nd value field", &event.size);
            event.ctahd = ctahd;
            ret = ctaQueueEvent(&event);
            if( ret != SUCCESS )
                break;

            ret = ctaWaitEvent(ctaqueuehd, &event, CTA_WAIT_FOREVER);
            if( ret != SUCCESS )
                break;

            ret = ctaFormatEventEx(ctahd, "", &event, buffer, sizeof buffer);
            if( ret == SUCCESS )
            {
                printf("%s\n", buffer);
            }

            break;
        }
        case PAIR( 'S', 'S' ):
        {
            static char server[256] = "localhost";

            promptstr("Enter default server address:", server, sizeof server);

            ret = ctaSetDefaultServer(server);

            break;
        }
        case PAIR( 'E', 'S' ):
        {
            unsigned size, count, i;
            DWORD ret;
            CTA_SERVICE_NAME_BUF *svclist;

            ret = ctaQueryServices(ctahd,NULL,0,&size);
            if ( ret != SUCCESS ) {
                printf ("Error 0x%x\n", ret);
                break;
            }

            svclist = (CTA_SERVICE_NAME_BUF *)
                malloc(size * sizeof(CTA_SERVICE_NAME_BUF));

            ret = ctaQueryServices(ctahd,svclist,size,&count);

            if ( ret != SUCCESS ) {
                printf ("Error 0x%x\n", ret);
                break;
            }
            for (i = 0;i< count; i++)
                printf("{ %s, %s }\n",
                svclist[i].svcname,svclist[i].svcmgrname);

            free(svclist);

            break;
        }
        case PAIR( 'T', 'R' ):
        {
            // set server's tracemask
            static unsigned tracemask = 0;
            if (ctahd == NULL_CTAHD)
            {
                printf("Invalid current context\n");
                break;
            }
            promptuns("Enter trace mask", &tracemask);

            ret = ctaSetGlobalTraceMask(ctahd, tracemask);
            break;
        }
        case PAIR( 'S', 'H' ):
        {
            // Shutdown server
            static char serveropt = 'n';
            unsigned opt = 0;

            if (ctahd == NULL_CTAHD)
            {
                printf("Invalid current context\n");
                break;
            }

            promptchar("Shutdown - (d)aemon, (h)ost, (r)eboot host, (n)one", &serveropt);

            switch( toupper( serveropt ) )
            {
                case 'D': opt = CTA_SERVER_SHUTDOWN; break;
                case 'H': opt = CTA_SYSTEM_SHUTDOWN; break;
                case 'R': opt = CTA_SYSTEM_REBOOT; break;
                default: opt = 0;
            }
            if (opt != 0)
            {
                ret = ctaShutdown(ctahd, opt);
            }
            break;
        }
        case PAIR( 'I', 'H' ):            //SET CALL HANDLE
        {
            prompthex( "Enter callhd for active call", &callhd );
            prompthex( "Enter callhd for held call", &heldcallhd );
            break;
        }
        case PAIR( 'C', 'L' ):
        {
            // Get context list

            unsigned size, count, i;
            DWORD ret;
            CTA_CNXT_INFO* buffer;

            ret = ctaQueryServerContexts(ctahd,NULL,0,&size);
            if ( ret != SUCCESS ) {
                printf ("Error 0x%x\n", ret);
                break;
            }

            buffer = (CTA_CNXT_INFO*)
                malloc(size * sizeof(CTA_CNXT_INFO));

            ret = ctaQueryServerContexts(ctahd,buffer,size,&count);
            if ( ret != SUCCESS ) {
                printf ("Error 0x%x\n", ret);
                break;
            }

            for (i = 0;i < count; i++)
                    printf ("%s ctahd: 0x%08X flags: 0x%08X nclients: %d\n",
                           buffer[i].contextname,buffer[i].ctahd,
                           buffer[i].flags,buffer[i].nclients);
            free(buffer);

            break;
        }
        default:
            /*
               printf("? [%c%c] is an unknown demo command\n", key>>8, key & 0xFF);
             */
            printf( "Type 'h' for help.\n" );
            return;
    }
    prevkey = key ;

    if( ret == SUCCESS )
        printf( "ok\n" );
    else
    {
        printf( "not ok\n" );
        /* Error handler prints the error */
    }
    return;
}

/*---------------------------------------------------------------------------
 * ErrorHandler
 *
 *  Called by Natural Access before it returns an error from any function.
 *--------------------------------------------------------------------------*/
DWORD NMSSTDCALL ErrorHandler( CTAHD ctahd, DWORD ret,
                               char *errtxt, char *func )
{
    printf( " %s from %s\n", errtxt, func );
    return ret;
}


/*---------------------------------------------------------------------------
 * Exit
 *
 * Called to exit the program, pausing if wait_on_exit != 0
 * exit_code is returned when program terminates.
 *--------------------------------------------------------------------------*/
void Exit( int exit_code )
{
    if( wait_on_exit )
    {
        fputs( "Press return to exit...", stdout );
        getchar();
    }
    exit( exit_code );
}

/*---------------------------------------------------------------------------
 * PromptForEncoding
 *
 *--------------------------------------------------------------------------*/
unsigned PromptForEncoding( unsigned mode /* play or record */)
{
    static unsigned rate = VCE_ENCODE_NMS_24;   // default is code 2;
    char promptstring[200];

    sprintf (promptstring,
            "Encoding %s (1)16kbps (2)24kbps (3)32kbps (4)64kbps\n"
            "          (10)mulaw (11)Alaw (13)linearPCM (see VCEDEF.H for others):",
            mode == VCE_PLAY_ONLY ?   "(0)auto" : "" );

    promptuns (promptstring, &rate );
    return rate;
}

/*---------------------------------------------------------------------------
 * PromptForFileType
 *
 *--------------------------------------------------------------------------*/
unsigned PromptForFileType( void )
{
    static unsigned type = VCE_FILETYPE_AUTO ;

    promptuns (
               "File type (0)auto (1)NMS(vox) (2)wave (3)flat:" , &type );
    return type;
}

/*----------------------------------------------------------------------------
  ClearScreen()

  ----------------------------------------------------------------------------*/
void ClearScreen( void )
{
#if defined (WIN32)
    system( "cls" );
#elif defined (UNIX)
    system( "clear" );
#endif
}


/*----------------------------------------------------------------------------
  StartProtocol()

  Start protocol on context.
  ----------------------------------------------------------------------------*/
int StartProtocol( )
{
    int ret;
    
    if( NccInUseFlag == 1 )
    {
        if( LowLevel )
        {
            NCC_START_PARMS startparms = { 0 };

            ctaGetParms( ctahd, NCC_START_PARMID,
                         &startparms, sizeof startparms );

            startparms.eventmask = 0xffff; /* low-level events */

            ret = nccStartProtocol( ctahd, ProtocolName,
                                            &startparms, NULL, NULL );
        }
        else
        {
            ret = nccStartProtocol( ctahd, ProtocolName,
                                            NULL, NULL, NULL );
        }
    }
    else if( LowLevel )
    {
        ADI_START_PARMS startparms;

        ctaGetParms( ctahd, ADI_START_PARMID,
                     &startparms, sizeof startparms );
        startparms.callctl.eventmask = 0xffff ; /* low-level events */
        adiStartProtocol( ctahd, ProtocolName, NULL, &startparms );
    }
    else
    {
        ret = adiStartProtocol( ctahd, ProtocolName, NULL, NULL );
    }
    return ret;
}

/*----------------------------------------------------------------------------
  ShowCapabilities()

  Display capabilities.
  ----------------------------------------------------------------------------*/
void ShowCapabilities( NCC_PROT_CAP *capabilities )
{
    DWORD mask = capabilities->capabilitymask;
    printf( "Capabilities supported by this protocol are:\n" );

    if( mask & NCC_CAP_ACCEPT_CALL )
        printf( "\tNCC_CAP_ACCEPT_CALL supported\n");
    if( mask & NCC_CAP_SET_BILLING )
        printf( "\tNCC_CAP_SET_BILLING supported\n" );
    if( mask & NCC_CAP_OVERLAPPED_SENDING )
        printf( "\tNCC_CAP_OVERLAPPED_SENDING supported\n" );
    if( mask & NCC_CAP_HOLD_RESPONSE )
        printf( "\tNCC_CAP_HOLD_RESPONSE supported\n" );

    if( mask & NCC_CAP_HOLD_CALL )
        printf( "\tNCC_CAP_HOLD_CALL supported\n" );
    if( mask & NCC_CAP_SUPERVISED_TRANSFER )
        printf( "\tNCC_CAP_SUPERVISED_TRANSFER supported\n" );
    if( mask & NCC_CAP_AUTOMATIC_TRANSFER )
        printf( "\tNCC_CAP_AUTOMATIC_TRANSFER supported\n" );
    if( mask & NCC_CAP_TWOCHANNEL_TRANSFER )
        printf( "\tNCC_CAP_TWOCHANNEL_TRANSFER supported\n" );

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

    return;
}

/*----------------------------------------------------------------------------
  ShowParm()

  Display a parameter.  Called by ShowDefaultParm.
  ----------------------------------------------------------------------------*/
void ShowParm( void *pvalue, CTA_PARM_INFO *f )
{
    union
    {
        BYTE  b;
        WORD  w;
        DWORD W;
        INT16 i;
        INT32 I;
        char  s[80];
    } *pdata = pvalue ;
    char *punits;

    switch( f->units )
    {
        case CTA_UNITS_INTERNAL: punits = "Internal";     break;
        case CTA_UNITS_INTEGER : punits = "Integer";      break;
        case CTA_UNITS_COUNT   : punits = "Count" ;       break;
        case CTA_UNITS_MASK    : punits = "Mask";         break;
        case CTA_UNITS_HZ      : punits = "Hz";           break;
        case CTA_UNITS_MS      : punits = "ms";           break;
        case CTA_UNITS_DB      : punits = "dB";           break;
        case CTA_UNITS_DBM     : punits = "dBm";          break;
        case CTA_UNITS_IDU     : punits = "Internal DSP"; break;
        case CTA_UNITS_STRING  : punits = "String";       break;
        case CTA_UNITS_PERCENT : punits = "Percent";      break;
        case CTA_UNITS_SECONDS : punits = "Seconds";      break;
        case CTA_UNITS_CHAR    : punits = "Character";    break;
        default                : punits = "Undefined";    break;
    }

    switch( f->format )
    {
        case CTA_FMT_UNKNOWN:
            {
            DWORD cnt;
            printf( "%s.%-30s = ", f->structname, f->fieldname );
            for ( cnt = 0; cnt < f->size; cnt++ )
            {
                printf( "0x%02x ", pdata->s[cnt] );
            }
            printf( "\t\t# BYTE_ARRAY[%d]\n", f->size );
            break;
            }
        case CTA_FMT_WORD:
            printf( "%s.%-30s = %5u\t\t# (0x%04x) WORD  (%s)\n",
                    f->structname, f->fieldname,
                    pdata->w, pdata->w, punits );
            break;
        case CTA_FMT_DWORD:
            printf( "%s.%-30s = %5u\t\t# (0x%04x) DWORD (%s)\n",
                    f->structname, f->fieldname,
                    pdata->W, pdata->W, punits );
            break;
        case CTA_FMT_INT16:
            printf( "%s.%-30s = %5d\t\t# (0x%04x) INT16 (%s)\n",
                    f->structname, f->fieldname,
                    pdata->i, pdata->i, punits );
            break;
        case CTA_FMT_INT32:
            printf( "%s.%-30s = %5d\t\t# (0x%04x) INT32 (%s)\n",
                    f->structname, f->fieldname,
                    pdata->I, pdata->I, punits );
            break;
        case CTA_FMT_STRING:
            printf( "%s.%-30s = \"%s\"\t\t# STRING[%d]\n",
                    f->structname, f->fieldname, pdata->s, f->size );
            break;
        case CTA_FMT_BYTE:
            printf( "%s.%-30s = %5u\t\t# (0x%02x)   BYTE  (%s)\n",
                    f->structname, f->fieldname,
                    pdata->b, pdata->b, punits );
            break;
        case CTA_FMT_INT8:
            if ((f->units == CTA_UNITS_CHAR) &&
                (pdata->s[0] >= ' ') &&
                (pdata->s[0] <= 0x7E))
                printf( "%s.%-30s =   '%c'\t\t# (0x%02x)   INT8  (%s)\n",
                        f->structname, f->fieldname,
                        pdata->s[0], pdata->s[0], punits );
            else
                printf( "%s.%-30s = %5d\t\t# (0x%02x)   INT8  (%s)\n",
                        f->structname, f->fieldname,
                        pdata->s[0], pdata->s[0], punits );
            break;
        default:
            printf( "Error! Unknown data type: %u\n",
                    f->format );
            break;
    }

    return;
}

/*----------------------------------------------------------------------------
  SetParm()

  Sets a parameter.  Called by SetDefaultParm.
  ----------------------------------------------------------------------------*/
int SetParm( CTAHD ctahd, void *pvalue, CTA_PARM_INFO *f )
{
    DWORD ret;
    union
    {
        BYTE  b;
        WORD  w;
        DWORD W;
        INT16 i;
        INT32 I;
        char  s[80];
        BYTE  B[80];
    } pdata ;
    char *punits;
    char newvalue[100];
    char pname[100];
    char prompt[100];

    switch( f->units )
    {
        case CTA_UNITS_INTERNAL: punits = "Internal";     break;
        case CTA_UNITS_INTEGER : punits = "Integer";      break;
        case CTA_UNITS_COUNT   : punits = "Count" ;       break;
        case CTA_UNITS_MASK    : punits = "Mask";         break;
        case CTA_UNITS_HZ      : punits = "Hz";           break;
        case CTA_UNITS_MS      : punits = "ms";           break;
        case CTA_UNITS_DB      : punits = "dB";           break;
        case CTA_UNITS_DBM     : punits = "dBm";          break;
        case CTA_UNITS_IDU     : punits = "Internal DSP"; break;
        case CTA_UNITS_STRING  : punits = "String";       break;
        case CTA_UNITS_PERCENT : punits = "Percent";      break;
        case CTA_UNITS_SECONDS : punits = "Seconds";      break;
        case CTA_UNITS_CHAR    : punits = "Character";    break;
        default                : punits = "Undefined";    break;
    }

    sprintf(prompt, "Enter new newvalue in %s [<ENTER>=current]: ", punits);
    promptline (prompt, newvalue, sizeof newvalue);

    /* leave current value passed as pvalue if nothing new entered */
    if (newvalue[0] == 0)
        return SUCCESS;

    switch( f->format )
    {
        case CTA_FMT_UNKNOWN:
            {
            DWORD cnt;
            char *ptr = strtok (newvalue, " \t");
            for (cnt = 0; cnt < f->size; cnt++)
            {
                if (ptr == NULL)
                    pdata.s[cnt] = 0;
                else
                {
                    pdata.B[cnt] = (BYTE) strtoul( ptr, NULL, 0 );
                    ptr = strtok( NULL, " \t" );
                }
            }
            break;
            }
        case CTA_FMT_WORD:
            pdata.w = (WORD) strtoul( newvalue, NULL, 0 );
            break;
        case CTA_FMT_INT32:    /* Unsigned is used for INT32 to allow         */
            /* entering DSP 'backdoor' values (0x8000xxxx) */
        case CTA_FMT_DWORD:
            pdata.W = (DWORD)strtoul( newvalue, NULL, 0 );
            break;
        case CTA_FMT_INT16:
            pdata.i = (INT16)strtol ( newvalue, NULL, 0 );
            break;
        case CTA_FMT_STRING:
            memset( (char *)&pdata.s, '\0', sizeof(pdata.s) );
            strncpy( (char *)&pdata.s, newvalue, sizeof(pdata.s)-1 );
            /* size must include null terminator */
            f->size = strlen( (char *)&pdata.s ) + 1;
            break;
        case CTA_FMT_BYTE:
            pdata.b = (BYTE) strtoul( newvalue, NULL, 0 );
            break;
        case CTA_FMT_INT8:
            pdata.s[0] = (char)strtol ( newvalue, NULL, 0 );
            break;
        default:
            printf( "Error! Unknown data type: %u\n", f->format );
            return CTAERR_BAD_ARGUMENT;
    }

    sprintf(pname, "%s.%s", f->structname, f->fieldname);

    if( ( ret = ctaSetParmByName( ctahd, pname, &pdata, f->size ) ) != 0 )
    {
        printf( "write error\n" );
        return ret ;
    }

    return SUCCESS ;
}

/*----------------------------------------------------------------------------
  ShowDefaultParm()
  ----------------------------------------------------------------------------*/
int ShowDefaultParm( CTAHD ctahd, char *name )
{
    unsigned      i;
    unsigned      ret;
    char         *cpos;
    char          parmname[120];
    BYTE          temp[MAX_STRUCT_SIZE];
    CTA_PARM_INFO info;

    if (name[0] == '?')
    {
        unsigned i, j;

        for( i = 0; i < SizeofServices/sizeof(CTA_SERVICE_DESC); i++)
        {
            unsigned ids[100];
            unsigned numids = 0;
            unsigned serviceID = 0;

            ctaGetParmIds( pServices[i].name.svcname,
                           pServices[i].name.svcmgrname,
                           &serviceID,
                           ids, sizeof ids, &numids );

            for (j=0; j < numids; j++)
            {
                ctaGetParmInfoEx( ctahd, ids[j], NULL, 0, &info );
                printf( "%s\n", info.structname );
            }
        }

        return SUCCESS;
    }

    /* Find first '.' indicating service name (i.e. ADI in ADI.ADIPLAY.gain) */
    if ((cpos = strchr(name, '.')) == NULL)  /* no service name */
    {
        printf("Please supply service name (i.e. ADI.play.gain)\n");
        return CTAERR_BAD_ARGUMENT;
    }
    else
    {
        /* Check if extension parameter (i.e. PR.X.RECORD.aglc) */
        if (strlen(cpos) > 1 &&
            tolower(cpos[1]) == 'x' &&
            cpos[2] == '.')
            cpos += 2;
    }

    ret = ctaGetParmByName( ctahd, name, temp, sizeof(temp) );

    if( ret != SUCCESS )
        return ret;

    if (strchr(cpos + 1, '.') == NULL)
    {
        CTA_ERROR_HANDLER hdlr;

        /* do not print out errors while finding valid parameter fields */
        ctaSetErrorHandler( NULL, &hdlr );

        /* if fieldname not specified then show the entire structure */
        for( i = 0 ; ; i++ )
        {
            ret = ctaGetParmInfoEx( ctahd, 0, name, i, &info );
            if( ret != SUCCESS )
                break;

            sprintf( parmname, "%s.%s", name, info.fieldname );
            ret = ctaGetParmByName( ctahd, parmname, &temp,
                                    info.size );
            ShowParm( temp, &info );
        }

        /* restore error handler */
        ctaSetErrorHandler( hdlr, NULL );
    }
    else                                /* need to show just one field */
    {
        ret = ctaGetParmInfoEx( ctahd, 0, name, 0, &info );
        if( ret != SUCCESS )
            return ret;

        ShowParm( temp, &info );
    }
    return SUCCESS;
}

/*----------------------------------------------------------------------------
  SetDefaultParm()
  ----------------------------------------------------------------------------*/
int SetDefaultParm( CTAHD ctahd, char *name )
{
    unsigned      i;
    unsigned      ret;
    char         *cpos;
    char          parmname[80];
    BYTE          temp[MAX_STRUCT_SIZE];
    CTA_PARM_INFO info;

    /* Find first '.' indicating service name (i.e. ADI in ADI.ADIPLAY.gain) */
    if ((cpos = strchr(name, '.')) == NULL)  /* no service name */
    {
        printf("Please supply service name (i.e. ADI.play.gain)\n");
        return CTAERR_BAD_ARGUMENT;
    }
    else
    {
        /* Check if extension parameter (i.e. PR.X.RECORD.aglc) */
        if (strlen(cpos) > 1 &&
            tolower(cpos[1]) == 'x' &&
            cpos[2] == '.')
            cpos += 2;
    }

    ret = ctaGetParmByName( ctahd, name, temp, sizeof(temp) );

    if( ret != SUCCESS )
        return ret;

    if (strchr(cpos + 1, '.') == NULL)
    {
        CTA_ERROR_HANDLER hdlr;

        /* do not print out errors while finding valid parameter fields */
        ctaSetErrorHandler( NULL, &hdlr );

        /* if fieldname not specified then set the entire structure */
        for( i = 0 ; ; i++ )
        {
            ret = ctaGetParmInfo( 0, name, i, &info );
            if( ret != SUCCESS )
                break;

            sprintf( parmname, "%s.%s", name, info.fieldname );
            ret = ctaGetParmByName( ctahd, parmname, &temp,
                                    info.size );
            ShowParm( temp, &info );
            SetParm( ctahd, temp, &info );
        }

        /* restore error handler */
        ctaSetErrorHandler( hdlr, NULL );
    }
    else                                /* can only set one field */
    {
        ret = ctaGetParmInfo( 0, name, 0, &info );
        if( ret != SUCCESS )
            return ret;

        ShowParm(temp, &info);

        return SetParm( ctahd, temp, &info );
    }
    return SUCCESS;
}


/*---------------------------------------------------------------------------
 * ShowDisconnectReason
 *
 *--------------------------------------------------------------------------*/
void ShowDisconnectReason (unsigned reason)
{
    switch( reason )
    {
        _SHOWCASE_( ADI_DIS_REJECT_REQUESTED  );
        _SHOWCASE_( ADI_DIS_SIGNAL            );
        _SHOWCASE_( ADI_DIS_CLEARDOWN_TONE    );
        _SHOWCASE_( ADI_DIS_RING_STUCK        );
        _SHOWCASE_( ADI_DIS_HOST_TIMEOUT      );
        _SHOWCASE_( ADI_DIS_REMOTE_ABANDONED  );
        _SHOWCASE_( ADI_DIS_TRANSFER          );
        _SHOWCASE_( ADI_DIS_DIAL_FAILURE      );
        _SHOWCASE_( ADI_DIS_NO_WINK           );
        _SHOWCASE_( ADI_DIS_NO_DIALTONE       );
        _SHOWCASE_( ADI_DIS_REMOTE_NOANSWER   );
        _SHOWCASE_( ADI_DIS_BUSY_DETECTED     );
        _SHOWCASE_( ADI_DIS_REORDER_DETECTED  );
        _SHOWCASE_( ADI_DIS_DIALTONE_DETECTED );
        _SHOWCASE_( ADI_DIS_SIT_DETECTED      );
        _SHOWCASE_( ADI_DIS_VOICE_BEGIN       );
        _SHOWCASE_( ADI_DIS_VOICE_MEDIUM      );
        _SHOWCASE_( ADI_DIS_VOICE_LONG        );
        _SHOWCASE_( ADI_DIS_VOICE_EXTENDED    );
        _SHOWCASE_( ADI_DIS_VOICE_END         );
        _SHOWCASE_( ADI_DIS_TIMEOUT           );
        _SHOWCASE_( ADI_DIS_RING_QUIT         );
        _SHOWCASE_( ADI_DIS_CED               );
        _SHOWCASE_( ADI_DIS_NO_LOOP_CURRENT   );
        default:    puts("???\n"); break;
    }
    return ;
}

/*----------------------------------------------------------------------------
  ShowCallStatus()

  Routine to print out Call Control status.
  ----------------------------------------------------------------------------*/
void ShowCallStatus( ADI_CALL_STATUS *ci )
{
    printf( "\tCall State:\t\t" );
    switch ( ci->state )
    {
        _SHOWCASE_( ADI_CC_STATE_STOPPED        );
        _SHOWCASE_( ADI_CC_STATE_IDLE           );
        _SHOWCASE_( ADI_CC_STATE_INCOMING_CALL  );
        _SHOWCASE_( ADI_CC_STATE_ANSWERING_CALL );
        _SHOWCASE_( ADI_CC_STATE_PLACING_CALL   );
        _SHOWCASE_( ADI_CC_STATE_DISCONNECTED   );
        _SHOWCASE_( ADI_CC_STATE_BLOCKING       );
        _SHOWCASE_( ADI_CC_STATE_CONNECTED      );
        _SHOWCASE_( ADI_CC_STATE_REJECTING_CALL );
        _SHOWCASE_( ADI_CC_STATE_OUT_OF_SERVICE );
        _SHOWCASE_( ADI_CC_STATE_PLACING_CALL2  );
        _SHOWCASE_( ADI_CC_STATE_CONNECTED2     );
        _SHOWCASE_( ADI_CC_STATE_ACCEPTING_CALL );
        default: puts( "???" ); break;
    }

    printf( "\tDisconnect Reason:\t" );
    ShowDisconnectReason (ci->reason) ;

    printf( "\tPending command:\t") ;
    switch ( ci->pendingcommand )
    {
        case 0                         : puts ("none"          ); break;
        case ADI_PENDCMD_PLACE_CALL    : puts ("Place call"    ); break;
        case ADI_PENDCMD_ANSWER_CALL   : puts ("Answer call"   ); break;
        case ADI_PENDCMD_REJECT_CALL   : puts ("Reject call"   ); break;
        case ADI_PENDCMD_RELEASE_CALL  : puts ("Release call"  ); break;
        case ADI_PENDCMD_TRANSFER_CALL : puts ("Transfer call" ); break;
        case ADI_PENDCMD_PLACE_SECOND  : puts ("Place 2nd call"); break;
        case ADI_PENDCMD_RELEASE_SECOND: puts ("Rls 2nd call"  ); break;
        case ADI_PENDCMD_ACCEPT_CALL   : puts ("Accept call"   ); break;
        default                        : puts ("???"           ); break;
    }

    printf( "\tcalled party digits = %s\n",  ci->calledaddr );
    printf( "\tcalling party digits = %s\n", ci->callingaddr );
    printf( "\tcalling presentation = %2X\n", ci->callingpres );
    printf( "\tcalling party name = %s\n", ci->callingname );
    printf( "\tredirecting party digits = %s\n", ci->redirectingaddr );
    printf( "\tredirecting reason = %2X\n", ci->redirectingreason );
    printf( "\tuser category = %2X\n", (BYTE) ci->usercategory );
    printf( "\ttoll category = %2X\n", (BYTE) ci->tollcategory );
    if (ci->datetime[0])
        printf( "\tDate & Time= %c%c/%c%c %c%c:%c%c\n",
                ci->datetime[0], ci->datetime[1], ci->datetime[2], ci->datetime[3],
                ci->datetime[4], ci->datetime[5], ci->datetime[6], ci->datetime[7] );

}

/*----------------------------------------------------------------------------
  ShowNccCallState()

  Gets call status info and prints the relevant portions.
  ----------------------------------------------------------------------------*/
int  ShowNccCallState( NCC_CALLHD callhd )
{
    NCC_CALL_STATUS info;
    unsigned ret;

    if( callhd == NULL_NCC_CALLHD )
    {
        printf( "NCC on call handle %08X is CLOSED.\n", callhd );
        return SUCCESS;
    }

    ret = nccGetCallStatus( callhd, &info, sizeof( NCC_CALL_STATUS ) );

    if( ret != SUCCESS )
    {
        return ret;
    }

    printf( "       NCC Call State = " );
    switch( info.state )
    {
        _SHOWCASE_(NCC_CALLSTATE_SEIZURE           );
        _SHOWCASE_(NCC_CALLSTATE_RECEIVING_DIGITS  );
        _SHOWCASE_(NCC_CALLSTATE_INCOMING          );
        _SHOWCASE_(NCC_CALLSTATE_ACCEPTING         );
        _SHOWCASE_(NCC_CALLSTATE_ANSWERING         );
        _SHOWCASE_(NCC_CALLSTATE_REJECTING         );
        _SHOWCASE_(NCC_CALLSTATE_CONNECTED         );
        _SHOWCASE_(NCC_CALLSTATE_DISCONNECTED      );
        _SHOWCASE_(NCC_CALLSTATE_OUTBOUND_INITIATED);
        _SHOWCASE_(NCC_CALLSTATE_PLACING           );
        _SHOWCASE_(NCC_CALLSTATE_PROCEEDING        );
        default: puts( "???\n" ); break;
    }

    if (info.held != 0)
    {
        printf ("\tCall is held.\n");
    }

    printf( "\tPending command:\t") ;
    switch ( info.pendingcmd )
    {
        case 0                                : puts ("none"           ); break;
        case NCC_PENDINGCMD_ACCEPT_CALL       : puts ("Accept call"    ); break;
        case NCC_PENDINGCMD_ANSWER_CALL       : puts ("Answer call"    ); break;
        case NCC_PENDINGCMD_AUTOMATIC_TRANSFER: puts ("Automatic Xfer" ); break;
        case NCC_PENDINGCMD_DISCONNECT_CALL   : puts ("Disconnect call"); break;
        case NCC_PENDINGCMD_HOLD_CALL         : puts ("Place call"     ); break;
        case NCC_PENDINGCMD_PLACE_CALL        : puts ("Place call"     ); break;
        case NCC_PENDINGCMD_REJECT_CALL       : puts ("Reject call"    ); break;
        case NCC_PENDINGCMD_RETRIEVE_CALL     : puts ("Retrieve call"  ); break;
        case NCC_PENDINGCMD_TRANSFER_CALL     : puts ("Transfer call"  ); break;
        case NCC_PENDINGCMD_RELEASE_CALL      : puts ("Release call"   ); break;
        default : puts ("???"           ); break;
    }

    printf( "\tcalled party digits = %s\n",  info.calledaddr );
    printf( "\tcalling party digits = %s\n", info.callingaddr );
    printf( "\tcalling party name = %s\n", info.callingname );

    printf("\n");
    return SUCCESS;
}

/*----------------------------------------------------------------------------
  ShowISDNExtCallStatus()

  Routine to print out ISDN Extended Call status.
  ----------------------------------------------------------------------------*/

int  ShowISDNExtCallStatus( NCC_CALLHD callhd )
{
    NCC_CALL_STATUS cs;
    NCC_ISDN_EXT_CALL_STATUS xcs;
    unsigned ret, i, uui_size;

    if( callhd == NULL_NCC_CALLHD )
    {
        printf( "NCC on call handle %08X is CLOSED.\n", callhd );
        return SUCCESS;
    }

    ret = nccGetCallStatus( callhd, &cs, sizeof( NCC_CALL_STATUS ) );
    if( ret != SUCCESS ) return ret;
    
    ret = nccGetExtendedCallStatus( callhd, &xcs, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
    if( ret != SUCCESS ) return ret;

    printf( "\tcalled:      number=%s type=%d plan=%d\n",
            cs.calledaddr, xcs.callednumtype, xcs.callednumplan );
    printf( "\tcalling:     number=%s type=%d plan=%d presentation=%d screen=%d\n",
            cs.callingaddr, xcs.callingnumtype, xcs.callingnumplan, xcs.callingpres, xcs.callingscreen );
    printf( "\t             name=%s\n",  cs.callingname );
    printf( "\t             originating line info/category = %d national category = %d\n",
            xcs.origlineinfo, xcs.national_cpc );
    printf( "\tconnected:   number=%s type=%d plan=%d presentation=%d screen=%d\n",
            xcs.connectedaddr, xcs.connectedtype, xcs.connectedplan, xcs.connectedpres, xcs.connectedscreen );
    printf( "\t             name=%s\n",  xcs.connectedname );
    printf( "\tredirecting: number=%s type=%d plan=%d presentation=%d screen=%d reason=%d\n",
            xcs.redirectingaddr, xcs.redirectingtype, xcs.redirectingplan, xcs.redirectingpres, xcs.redirectingscreen,
            xcs.redirectingreason );
    printf( "\tredirection: number=%s type=%d plan=%d presentation=%d screen=%d reason=%d\n",
            xcs.redirectionaddr, xcs.redirectiontype, xcs.redirectionplan, xcs.redirectionpres, xcs.redirectionscreen,
            xcs.redirectionreason );
    printf( "\torig called: number=%s type=%d plan=%d presentation=%d screen=%d reason=%d count=%d cfnr=%d\n",
            xcs.originalcalledaddr, xcs.origcalledtype, xcs.origcalledplan, xcs.origcalledpres, xcs.origcalledscreen,
            xcs.origcalledreason, xcs.origcalledcount, xcs.origcalledcfnr );
    printf( "\tcause value = %d\n", xcs.releasecause );
    printf( "\tprogress descriptor = %d\n", xcs.progressdescr );
    
    printf( "\tcall id = {" );
    for ( i=0; i<sizeof(xcs.callid)/sizeof(xcs.callid[0]); i++ )
    {
        printf(" %04X", xcs.callid[i] );
    }
    printf ( " }\n" );
    
    printf( "\tUser-User Info =\t");
    if (xcs.UUI[0] == 0x7E)
    {
        uui_size = ((unsigned char) xcs.UUI[1]) + 2;
        for (i = 0; i < uui_size; )
        {
            printf (" %02x",xcs.UUI[i++]);
            if (((i % 16) == 0) && (i < uui_size))
                printf ("\n\t\t\t\t");
        }
    }
    printf( "\n");
    
    printf("\n");
    return SUCCESS;
}

/*----------------------------------------------------------------------------
  ShowCASExtCallStatus()

  Routine to print out ISDN Extended Call status.
  ----------------------------------------------------------------------------*/

int  ShowCASExtCallStatus( NCC_CALLHD callhd )
{
    NCC_CALL_STATUS cs;
    NCC_CAS_EXT_CALL_STATUS xcs;
    unsigned ret;

    if( callhd == NULL_NCC_CALLHD )
    {
        printf( "NCC on call handle %08X is CLOSED.\n", callhd );
        return SUCCESS;
    }

    ret = nccGetCallStatus( callhd, &cs, sizeof( NCC_CALL_STATUS ) );
    if( ret != SUCCESS ) return ret;
    
    ret = nccGetExtendedCallStatus( callhd, &xcs, sizeof(NCC_ISDN_EXT_CALL_STATUS) );
    if( ret != SUCCESS ) return ret;

    printf( "\tcalled:      number=%s\n", cs.calledaddr );
    printf( "\tcalling:     number=%s user category=%2X toll category=%2X presentation=%c\n",
            cs.callingaddr, (BYTE) xcs.usercategory, (BYTE) xcs.tollcategory, xcs.ANIpresentation );
    printf( "\t             name=%s presentation=%c\n",  cs.callingname, xcs.NAMEpresentation );
    printf( "\tredirecting: number=%s reason=%2X\n", xcs.redirectingaddr, (BYTE) xcs.redirectingreason );
    printf( "\tcarrier:     number=%s\n", xcs.carrierid );
    if (xcs.billingrate == NCC_BILLINGSET_FREE)
        printf( "\tbillingrate= FREE\n", xcs.billingrate );
    else if (xcs.billingrate == NCC_BILLINGSET_CHARGE_ALTERNATE)
        printf( "\tbillingrate= CHARGE ALTERNATE\n", xcs.billingrate );
    else if (xcs.billingrate == NCC_BILLINGSET_DEFAULT)
        printf( "\tbillingrate= DEFAULT\n", xcs.billingrate );
    else printf( "\tbillingrate= %d\n", xcs.billingrate );

    printf( "\treleasecause=%2X\n", xcs.releasecause );
    
    if (xcs.MWIndicator == 0)
        printf( "\tTurn Message Waiting Indicator OFF\n");
    else if (xcs.MWIndicator == 0xFF)
            printf( "\tTurn Message Waiting Indicator ON\n");

    if (xcs.datetime[0])
        printf( "\tDate & Time= %c%c/%c%c %c%c:%c%c\n",
                xcs.datetime[0], xcs.datetime[1], xcs.datetime[2], xcs.datetime[3],
                xcs.datetime[4], xcs.datetime[5], xcs.datetime[6], xcs.datetime[7] );

    printf("\n");
    return SUCCESS;
}

/*----------------------------------------------------------------------------
  ShowNccLineState()

  Gets call status info and prints the relevant portions.
  ----------------------------------------------------------------------------*/
int  ShowNccLineState( CTAHD ctahd )
{
    NCC_LINE_STATUS info;
    unsigned ret;
    NCC_CALLHD  callhds[10];
    unsigned i;

    if( ctahd == NULL_CTAHD )
    {
        printf( "NCC on context %08X is CLOSED.\n", ctahd );
        return SUCCESS;
    }

    ret = nccGetLineStatus( ctahd, &info, sizeof( NCC_LINE_STATUS ), callhds,
                            10 * sizeof( NCC_CALLHD ) );

    if( ret != SUCCESS )
    {
        return ret;
    }

    printf( "       NCC Line State = " );
    switch( info.state )
    {
        _SHOWCASE_(NCC_LINESTATE_UNINITIALIZED  );
        _SHOWCASE_(NCC_LINESTATE_IDLE           );
        _SHOWCASE_(NCC_LINESTATE_BLOCKING       );
        _SHOWCASE_(NCC_LINESTATE_OUT_OF_SERVICE );
        _SHOWCASE_(NCC_LINESTATE_ACTIVE         );
        default: puts( "???\n" ); break;
    }

    printf( "\tPending command:\t") ;
    switch ( info.pendingcmd )
    {
        case 0                                : puts ("none"           ); break;
        case NCC_PENDINGCMD_BLOCK_CALLS       : puts ("Block calls"    ); break;
        case NCC_PENDINGCMD_UNBLOCK_CALLS     : puts ("Unblock calls"  ); break;
        case NCC_PENDINGCMD_START_PROTOCOL    : puts ("Start protocol" ); break;
        case NCC_PENDINGCMD_STOP_PROTOCOL     : puts ("Stop Protocol"  ); break;
        default                        : puts ("???"           ); break;
    }

    printf( "\tprotocol  = %s\n", info.protocol );
    printf( "\tnumcallhd = %d\n", info.numcallhd );

    i = 0;
    while ((i < info.numcallhd) && (i < 10))
    {
        printf( "\t\tcallhd%d = %x\n", i+1, callhds[i]);
        i++;
    }

    printf("\n");
    return SUCCESS;
}

/*----------------------------------------------------------------------------
  ShowAdiState()
  Need to have an xxxGetContextInfo function for all services.
  ----------------------------------------------------------------------------*/
int  ShowAdiState( CTAHD ctahd )
{
    ADI_CONTEXT_INFO info;
    unsigned ret;

    if( ctahd == NULL_CTAHD )
    {
        printf("ADI on context %08X is CLOSED.\n", ctahd );
        return SUCCESS;
    }

    ret = adiGetContextInfo( ctahd, &info, sizeof(ADI_CONTEXT_INFO) );
    if( ret != SUCCESS )
        return ret;

    printf("       ADI State = ");
    switch (info.state)
    {
        _SHOWCASE_(ADI_STATE_OPENING   ) ;
        _SHOWCASE_(ADI_STATE_OPENED    ) ;
        _SHOWCASE_(ADI_STATE_OPENFAILED) ;
        _SHOWCASE_(ADI_STATE_CLOSING   ) ;
        _SHOWCASE_(ADI_STATE_STARTING  ) ;
        _SHOWCASE_(ADI_STATE_STARTED   ) ;
        _SHOWCASE_(ADI_STATE_STOPPING  ) ;
        _SHOWCASE_(ADI_STATE_SUPERVISOR) ;
        default: puts( "???\n" ); break;
    }

    printf("    Board Number = %d\n",      info.board      );
    printf("Stream:Slot,Mode = %d:%d,",    info.stream95, info.timeslot );
    switch( info.mode )
    {
        case ADI_FULL_DUPLEX   : puts("ADI_FULL_DUPLEX" );   break;
        case ADI_VOICE_DUPLEX  : puts("ADI_VOICE_DUPLEX" );  break;
        case ADI_SIGNAL_DUPLEX : puts("ADI_SIGNAL_DUPLEX" ); break;
        default:
            if( info.mode & ADI_VOICE_INPUT  ) printf( "+ADI_VOICE_INPUT"  );
            if( info.mode & ADI_VOICE_OUTPUT ) printf( "+ADI_VOICE_OUTPUT" );
            if( info.mode & ADI_SIGNAL_INPUT ) printf( "+ADI_SIGNAL_INPUT" );
            if( info.mode & ADI_SIGNAL_OUTPUT) printf( "+ADI_SIGNAL_OUTPUT");
            printf( "\n" );
            break;
    }
    if( NccInUseFlag == 0 )
    {
        printf("        Protocol = %s\n",      info.tcpname    );
    }
    else
    {
        NCC_LINE_STATUS nccinfo;
        unsigned ret;

        if( ctahd == NULL_CTAHD )
        {
            printf( "NCC on context %08X is CLOSED.\n", ctahd );
            return SUCCESS;
        }

        ret = nccGetLineStatus( ctahd, &nccinfo, sizeof( nccinfo ),
                                NULL, 0 );

        if( ret != SUCCESS )
            return ret;

        printf("        Protocol = %s\n",      nccinfo.protocol  );

    }
    printf("        Queue ID = %d\n",      info.queueid    );
    printf("         User ID = %08Xh\n",   info.userid     );
    printf("  AG Buffer Size = %d\n",      info.maxbufsize );
    printf("      AG Channel = %08Xh\n",   info.channel    );
    printf("Last AGLIB Error = %d \n",     info.agliberr   );

    printf("\n");
    return SUCCESS;
}

/*----------------------------------------------------------------------------
  SendISDNCallMessage()
  Send ISDN specific call message using nccSendCallMessage()
  ----------------------------------------------------------------------------*/

int  SendISDNCallMessage( NCC_CALLHD callhd )
{
    static char msg = 'c';
    
    BYTE buf[256] = {0};
    int  size = 0;
    
    promptchar("TRANSFER_CALLID_RQ (c), TRANSFER (t)", &msg );

    switch ( msg )
    {
        case 'c':
        case 'C':
            {
                NCC_ISDN_SEND_CALL_MESSAGE *m = (NCC_ISDN_SEND_CALL_MESSAGE *)buf;
                
                m->message_id = NCC_ISDN_TRANSFER_CALLID_RQ;
                size = sizeof( NCC_ISDN_SEND_CALL_MESSAGE );
            }
            break;
            
        case 't':
        case 'T':
            {
                static int status = 0;
                NCC_ISDN_TRANSFER_PARAM *m = (NCC_ISDN_TRANSFER_PARAM *)buf;

                prompt("Status", &status );
                                
                m->hdr.message_id = NCC_ISDN_TRANSFER;
                m->status         = status;
                
                size = sizeof( NCC_ISDN_TRANSFER_PARAM );
            }
            break;
            
        default:
            printf("\tMessage '%c' is not a valid choice\n", msg );    
            return SUCCESS;
    }
    
    return nccSendCallMessage( callhd, buf, size );
}

/*---------------------------------------------------------------------------
 *                         ShowMVIP
 *--------------------------------------------------------------------------*/
void ShowMVIP( char *str, unsigned value )
{
    printf( "\t\t%s\tMVIP signalling bits = 0x%x (%c%c%c%c)\n",
            str, (value&0xf),
            (value&0x8)?'A':'-', (value&0x4)?'B':'-',
            (value&0x2)?'C':'-', (value&0x1)?'D':'-' );
}


/*---------------------------------------------------------------------------
 *                         ShowReason
 *--------------------------------------------------------------------------*/
void ShowReason (int reason)
{
    char errstr[40];

    switch( reason )
    {
        _SHOWCASE_( CTA_REASON_FINISHED  );
        _SHOWCASE_( CTA_REASON_STOPPED   );
        _SHOWCASE_( CTA_REASON_TIMEOUT   );
        _SHOWCASE_( CTA_REASON_DIGIT     );
        _SHOWCASE_( CTA_REASON_NO_VOICE  );
        _SHOWCASE_( CTA_REASON_VOICE_END );
        _SHOWCASE_( CTA_REASON_RELEASED  );
        case 0:
        printf( "Not Done\n" );
        break;
        default:
        ctaGetText( NULL_CTAHD, reason, errstr, sizeof(errstr) );
        printf( "%s\n", errstr );
    }

    printf("\n");
    return;
}

/*---------------------------------------------------------------------------
 * prompt - prompt for a signed value
 *
 *--------------------------------------------------------------------------*/
void prompt (char *text, int *target)
{
    char     temp[80];                     /* used to get user string input */

    if (Batch)
    {
        if (!Repeatentry)
        {
            GetLineFromFile(temp, sizeof temp);
            sscanf(temp, "%i", target);
        }
        printf( "\t%s: %d\n", text, *target );
    }
    else
    {
        printf( "\t%s [%d]: ", text, *target );
        if (!Repeatentry)
        {
#if defined (UNIX)
            UnixGets( temp );
#else
            gets( temp );
#endif
            sscanf (temp, "%i", target);
        }
        else
            puts( "" );
    }
    return ;
}


/*---------------------------------------------------------------------------
 * promptdw - prompt for an unsigned long value
 *
 *--------------------------------------------------------------------------*/
void promptdw (char *text, DWORD *target)
{
    char     temp[80];                     /* used to get user string input */

    if (Batch)
    {
        if (!Repeatentry)
        {
            GetLineFromFile(temp, sizeof temp);
            sscanf(temp, "%i", target);
        }
        printf( "\t%s: %u\n", text, *target );
    }
    else
    {
        printf( "\t%s [%u]: ", text, *target );
        if (!Repeatentry)
        {
#if defined (UNIX)
            UnixGets( temp );
#else
            gets( temp );
#endif
            sscanf (temp, "%i", target);
        }
        else
            puts( "" );
    }
    return ;
}

/*---------------------------------------------------------------------------
 * promptw - prompt for an unsigned short value
 *
 *--------------------------------------------------------------------------*/
void promptw (char *text, WORD *target)
{
    char     temp[80];                     /* used to get user string input */
    DWORD tmptarget;

    if (Batch)
    {
        if (!Repeatentry)
        {
            GetLineFromFile(temp, sizeof temp);
            sscanf(temp, "%i", &tmptarget);
            *target = (WORD)tmptarget;
        }
        printf( "\t%s: %u\n", text, *target );
    }
    else
    {
        printf( "\t%s [%u]: ", text, *target );
        if (!Repeatentry)
        {
            gets( temp );
            sscanf (temp, "%i", &tmptarget);
            *target = (WORD)tmptarget;
        }
        else
            puts( "" );
    }
    return ;
}

/*---------------------------------------------------------------------------
 * prompthex - prompt for a hex value
 *
 *--------------------------------------------------------------------------*/
void prompthex (char *text, DWORD *target)
{
    char     temp[80];                     /* used to get user string input */

    if (Batch)
    {
        if (!Repeatentry)
        {
            GetLineFromFile(temp, sizeof temp);
            sscanf(temp, "%x", target);
        }
        printf( "\t%s: 0x%x\n", text, *target );
    }
    else
    {
        printf( "\t%s [0x%x]: ", text, *target );
        if (!Repeatentry)
        {
#if defined (UNIX)
            UnixGets( temp );
#else
            gets( temp );
#endif
            sscanf (temp, "%x", target);
        }
        else
            puts( "" );
    }
    return ;
}


/*---------------------------------------------------------------------------
 * promptuns - prompt for an unsigned value
 *
 *--------------------------------------------------------------------------*/
void promptuns (char *text, unsigned *target)
{
    char     temp[80];                     /* used to get user string input */

    if (Batch)
    {
        if (!Repeatentry)
        {
            GetLineFromFile(temp, sizeof temp);
            sscanf(temp, "%i", target);
        }
        printf( "\t%s: %u\n", text, *target );
    }
    else
    {
        printf( "\t%s [%u]: ", text, *target );
        if (!Repeatentry)
        {
#if defined (UNIX)
            UnixGets( temp );
#else
            gets( temp );
#endif
            sscanf (temp, "%i", target);
        }
        else
            puts( "" );
    }
    return ;
}


/*---------------------------------------------------------------------------
 * promptstr - prompt for a string
 * 000724: limit strcpy by "max"
 *--------------------------------------------------------------------------*/
void promptstr (char *text, char *target, unsigned max )
{
    char     temp[1024];                     /* used to get user string input */

    if (Batch)
    {
        if (!Repeatentry)
        {
            GetLineFromFile(temp, sizeof temp);
            if( !iscntrl(*temp) )
            {
                strncpy( target, temp, max );
                if ( strlen( temp ) >= max )
                {
                    *(target+(max-1)) = '\0';
                    printf("\tInput string length truncated to %d characters:",
                            max - 1);
                    printf("\t%s\n", target );
                }
            }
        }
        printf( "\t%s: %s\n", text, target );
    }
    else
    {
        printf( "%s ['%s']: ", text, target );
        if (!Repeatentry)
        {
#if defined (UNIX)
            UnixGets( temp );
#else
            gets( temp );
#endif
            if( !iscntrl(*temp) )
            {
                strncpy( target, temp, max );
                if( strlen( temp ) >= max )
                {
                    char err[100];
                    *(target+(max-1)) = '\0';
                    sprintf( err,
                            "\tInput string length truncated to %d characters: %s",
                            max - 1, target);
                    puts( err );
                }
            }
        }
        else
            puts( "" );
    }
    return ;
}

/*---------------------------------------------------------------------------
 * promptline - prompt for a string (no default)
 *--------------------------------------------------------------------------*/
void promptline (char *text, char *target, unsigned max )
{
    printf(text);
    if (Batch)
    {
        GetLineFromFile(target, max);
        puts( target );
    }
    else
    {
#if defined (UNIX)
        UnixGets( target );
#else
        gets( target );
#endif
    }
    return ;
}

/*---------------------------------------------------------------------------
 * promptchar - prompt for a char.
 *   Takes a text prompt and a char * pre-loaded with a default value, which
 * is clobbered if and only if the next input char is not a newline.
 *   In UNIX, the input must end with a newline, which is discarded.  If the
 * program is ever converted to raw mode, this will be unnecessary.
 *--------------------------------------------------------------------------*/
void promptchar(char *text, char *target)
{
    char     temp[80];                     /* used to get user string input */

#if defined (UNIX)

    if (Batch)
    {
        if (!Repeatentry)
        {
            GetLineFromFile(temp, sizeof temp);
            sscanf (temp, "%1s", target);
        }
        printf( "%s: '%c'\n", text, *target );
    }
    else
    {
        printf( "%s ['%c']: ", text, *target );
        if (!Repeatentry)
        {
            UnixGets( temp );
            sscanf (temp, "%1s", target);
        }
        else
            puts( "" );
    }
#else
    char     c;

    if (Batch)
    {
        if (!Repeatentry)
        {
            GetLineFromFile(temp, sizeof temp);
            sscanf (temp, "%1s", &c);
            if ( (c != '\n') && (c != '\r') )
                *target = c;
        }
        printf( "%s '%c'\n", text, *target );
    }
    else
    {
        printf( "%s ['%c']: ", text, *target );
        if (!Repeatentry)
        {
            c = GETCH();
            if ( (c != '\n') && (c != '\r') )
                *target = c;
        }
        puts( "" );
    }
#endif
    return ;
}

/*----------------------------------------------------------------------------
 * PromptForStrings - prompt for string list
 *  Prompts for strings and puts them in an array
 *---------------------------------------------------------------------------*/
int PromptForStrings(char *promptstring, char *strarr[], int maxstrlen)
{
    int     i;

    for (i = 0; ; i++)
    {
        strarr[i] = (char *)malloc(sizeof(char) * maxstrlen);
        strcpy(strarr[i], "");
        promptstr(promptstring, strarr[i], maxstrlen);
        if (strcmp(strarr[i], "") == 0)
            return i;
    }

}

/*----------------------------------------------------------------------------
 * PrintStringofStrings - print the strings in the list one by one
 *  stringlist contains strings put together
 *  in the servicelist string (without removing the string terminator) and
 *  terminating the list with another '\0'; Thus, servicelist will have a list
 *  of strings, with an extra '\0' at the end of the last string.
 *---------------------------------------------------------------------------*/
void PrintStringofStrings(char *stringlist)
{
    char *str = stringlist;

    if (stringlist == NULL)
        return;

    while (*str != '\0')
    {
        printf("%s\n", str);
        while (*str != '\0')
            str++;
        str++;
    }

}

/*----------------------------------------------------------------------------
 * PromptServiceOpenList - get list of services to open
 *
 *---------------------------------------------------------------------------*/
int PromptServiceOpenList(CTA_SERVICE_DESC *ServiceList)
{
    int   i;
    int   nosvcs = 0;
    int   ninisvcs;
    int   svcindex = 0;

    ninisvcs = SizeofServices/sizeof(CTA_SERVICE_DESC);

    if (!Batch)
    {
        printf("Services initialized are:\n");
        for (i = 0; i < ninisvcs; i++)
        {
            printf("%d. %s %s\n", i + 1, pServices[i].name.svcname,
                   pServices[i].name.svcmgrname);
        }
        printf("Enter indices (1..n) from the list for the services "
               "to be opened; Press Enter to end the list\n");
    }

    for(;;)
    {
        svcindex = 0;
        prompt("Enter Service Index", &svcindex);
        if (svcindex == 0)
        {
            break;
        }

        if (svcindex > ninisvcs)
        {
            printf("Index out of range\n");
            return -1;
        }
        /* Increment mvipaddr.timeslot for ADI  & NCC service */
        if ((svcindex == 1) || (svcindex == 2))
        {
            if (pServices[0].mvipaddr.timeslot >= (unsigned)(Slot + nSlots))
            {
                printf("No free timeslots available for ADI & NCC services\n");
                return -1;
            }
        }
        ServiceList[nosvcs++] = pServices[svcindex - 1];

        if ((svcindex == 1) || (svcindex == 2))
        {
            pServices[svcindex-1].mvipaddr.timeslot++;
        }
    }
    return nosvcs;
}

/*----------------------------------------------------------------------------
  PrintUsage()
  ----------------------------------------------------------------------------*/
void PrintUsage( void )
{
    printf( "Usage: ctatest [opts]\n" );
    printf( "  where opts are:\n" );
    printf( "  -A adi_mgr      Manager to use for ADI service.          "
            "Default = %s\n",  DemoGetADIManager() );
    printf( "  -b n            Board number.                            "
            "Default = %d\n", Board );
    printf( "  -c              Use Legacy ADI Call Control commands.    "
            "Default = Use NCC\n");
    printf( "  -d              Do not exit on NA server exiting.\n" );

    printf( "  -f filename     CTA config file (to get service names).  "
            "Default = none\n" );
    printf( "  -i filename     Script file providing initial commands.  "
            "Default = none\n" );
    printf( "  -l              Show low-level call-control events.\n" );

    printf( "  -m {l|s}        Parameters: l=local s=shared.            "
            "Default = shared\n");
    printf( "  -M ncc_mgr      Service manager name for NCC service.    "
            "Default = ADI manager\n");
    printf( "  -o open_mode    Open mode.  Use 3 for voice-only.        "
            "Default = 0xf\n" );
    printf( "  -p protocol     Protocol to run.                         "
            "Default = %s\n", ProtocolName );
    printf( "  -s [strm:]slot  DSP [stream and] timeslot.               "
            "Default = %d:%d\n", Stream, Slot );
    printf( "  -t {d|r}        Tracing: d=disable r=trace on error.     "
            "Default = enabled\n");
    printf( "  -v              Omit Natural Access compatibility check. "
            "Default = enabled\n");
    printf( "  -w              Wait before exit.\n" );
    printf( "  -S              Automatically open port and start prot.  "
            "Default = disabled\n");
    printf( "\n" );
}

int ContextAdd(CNXT_INFO cxinfo)
{
    int i;

    if (cxinfo.ctahd == NULL_CTAHD)
        return -1;

    for (i = 0; i < MAX_CONTEXTS; i++)
    {
        if (contextarray[i].ctahd == NULL_CTAHD)
        {
            contextarray[i] = cxinfo;  // struct copy
            return 0;
        }
    }
    return -1;
}

int ContextRemove(CTAHD ctahd)
{
    int i;

    if (ctahd == NULL_CTAHD)
        return -1;

    for (i = 0; i < MAX_CONTEXTS; i++)
    {
        if (contextarray[i].ctahd == ctahd)
        {
            contextarray[i].ctahd = NULL_CTAHD;
            return 0;
        }
    }
    return -1;
}

int ContextGet(CNXT_INFO *cxinfo)
{
    int i;

    if (cxinfo->ctahd == NULL_CTAHD)
        return -1;

    for (i = 0; i < MAX_CONTEXTS; i++)
    {
        if (contextarray[i].ctahd == cxinfo->ctahd)
        {
            strcpy(cxinfo->cName, contextarray[i].cName);
            cxinfo->vcehd = contextarray[i].vcehd;
            cxinfo->vcememhd = contextarray[i].vcememhd;
            return 0;
        }
    }
    return -1;
}

void ContextBackup(CTAHD ctahd, VCEHD vcehd, VCEHD vcememhd)
{
    int i;

    if (ctahd == NULL_CTAHD)
        return;

    for (i = 0; i < MAX_CONTEXTS; i++)
    {
        if (contextarray[i].ctahd == ctahd)
        {
            contextarray[i].vcehd = vcehd;
            contextarray[i].vcememhd = vcememhd;
            return;
        }
    }
}

void ContextArrayInitialize()
{
    int i;

    for (i = 0; i < MAX_CONTEXTS; i++)
        contextarray[i].ctahd  = NULL_CTAHD;

}

void PrintServiceList(char *svcnames)
{
    char *startstr = svcnames;

    while (*startstr != '\0')
    {
        printf("%s\n", startstr);
        do
        {
            startstr++;
        }
        while (*startstr != '\0');
        startstr++;
    }
}

/*---------------------------------------------------------------------------
 * GetLineFromFile - get input in Batch mode.
 *
 * The input file is specified by the -i option.
 *
 * On EOF, Batch mode ends and input reverts to interactive.
 *
 * Lines beginning with '#' are ignored as comments.  Otherwise the line is
 * accepted but all text following a '#' is discarded (along with the '#'
 * itself).
 *
 *--------------------------------------------------------------------------*/
void GetLineFromFile(char *line, int len)
{
    char *temp;
    char *currptr;
    int  i = 0;

    temp = (char *)malloc(sizeof(char) * len);

    do
    {
        if (fgets(temp, len, InputFileStream) == NULL)
        {
            /* If reached end of file, revert to interactive. */
            Batch = FALSE;
            line[0] = '\0';
            return;
        }
    } while (temp[0] == '#');   // skip comment lines

    currptr = temp;
    while (*currptr != '\0')
    {
        // look for literal '#' -- replace '\#' with '#'
        if (*currptr == '\\' && *(currptr + 1) == '#')
        {
            line[i] = '#';
            i++;
            currptr += 2;
        }
        // but ignore everything after un-escaped '#'
        else if (*currptr == '#')
        {
            break;
        }
        // discard the newline
        else if (*currptr == '\n')
        {
            break;
        }
        else
        {
            line[i] = *currptr;
            i++;
            currptr++;
        }
    }
    line[i] = '\0';
    free(temp);
    return;
}


int StrNoCaseCmp(char *str1, char *str2)
{

    for ( ; toupper( *str1 ) == toupper( *str2 ); str1++, str2++ )
        if (*str1 == '\0')
            return 0;
    return toupper( *str1 ) - toupper( *str2 );

}

BOOL ReconnectServer(char *descriptor, CTAQUEUEHD ctaqueuehd)
{
    CTAHD tmp_ctahd;
    static unsigned  userid = 0;
    DWORD ret;


    ret = ctaCreateContextEx( ctaqueuehd, userid, descriptor,
        &tmp_ctahd,  0 );

    if (ret == SUCCESS)
    {
        ret = ctaDestroyContext(tmp_ctahd);
        printf ("Ready.\n");
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
