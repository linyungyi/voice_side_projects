/******************************************************************************\
* File: rtrace.cpp
* Author:   Askar Rahimberdiev
* Project:  Multi-Remote Server
*
* Description:
*       Monitor program for multiple remote NA servers.
*
* Revision History:
*       (see source code control)
*
*   (c)   2001 Natural MicroSystems Inc.   All rights reserved.
\******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>     // for va_start etc.
#include <ctype.h>
#include <signal.h>

#include "ctademo.h"
#include "ctadef.h"
#include "tsidef.h"

#if defined (UNIX)
    #include <fcntl.h>        // for O_RDWR
    #include <unistd.h>       // for STDIN_FILENO
    #include <termio.h>       // for struct termio, TCSETAW
    #include <sys/time.h>     // for select, FD_SET, FD_CLR, FD_ISSET, FD_ZERO
    #define GETCH() getchar()
#elif defined (WIN32)
    #include <conio.h>
    #define kbhit _kbhit
    #define GETCH() getch()
#endif

////////////////////////////////////////////////////////////////////////////////

#define MAX_SERVER_SIZE     64

static char s_szServer[ MAX_SERVER_SIZE ] = "";
static CTAHD s_ctahd = NULL_CTAHD;
static CTAQUEUEHD s_ctaqueuehd = NULL_CTAQUEUEHD;
static FILE *s_fpLog = NULL;
static char *s_pszLogFileName = NULL;
static BOOL s_bLogfileInUse = FALSE;
static BOOL s_bQuiet = FALSE;

////////////////////////////////////////////////////////////////////////////////
// sighdlr
//
// Unix signal handler.

#if defined (UNIX)
static void setupKbdIO( int init );

void sighdlr( int sig )
{
    // Restore original keyboard settings.
    setupKbdIO( 0 );
}
#endif

////////////////////////////////////////////////////////////////////////////////
// setupKbdIO
//
// Sets up unbuffered keyboard I/O.  This includes support for reading single
// chars with GETCH() on Unix.
//
// init [in] is 1 to init, 0 to restore original settings.

static void setupKbdIO( int init )
{
#if defined (UNIX)
    static struct termio tio, old_tio;
    static int initialized = 0;
    static int fd;

    if ( init == 0 )
    {
        if ( initialized == 0 )
            return;

        ioctl( fd, TCSETAW, &old_tio );  /* Restore original settings */
        initialized = 0;
        return;
    }

    /* Set up UNIX for unbuffered I/O */
    setbuf( stdout, NULL );
    setbuf( stdin, NULL );

    fd = open( "/dev/tty", O_RDWR );
    ioctl( fd, TCGETA, &old_tio );      /* Remember original setting */
    ioctl( fd, TCGETA, &tio );
    tio.c_lflag    &= ~ICANON;
    tio.c_lflag    &= ~ECHO ;    // set ECHO off
    tio.c_cc[VMIN]  = 1;
    tio.c_cc[VTIME] = 0;
    ioctl( fd, TCSETA, &tio );

    initialized = 1;
    signal( SIGINT, sighdlr );

#elif defined (OS2) || defined (WIN32) || defined (__NETWARE_386__)
    if ( init != 0 )
    {
        setbuf( stdout, NULL );        /* use unbuffered i/o */
        setbuf( stdin, NULL );
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////
// detectKeyboardQuit
//
// Looks for keyboard input, and returns TRUE if that input is equal to one
// of the Quit characters
// Turned into a subroutine so that it can be used multiple places,
// TFG 000823

static int detectKeyboardQuit()
{
    // Check for keyboard input.  Exit if exit key hit, else discard.

    while( kbhit() )
    {
        int nKey = GETCH();
        switch (nKey) // handle extended chars (sent as multiple chars)
        {             // and convert normal chars to lower case
        case 0:     nKey = 0x0100 + GETCH();    break;  // Func key
        case 0xe0:  nKey = 0xe000 + GETCH();    break;  // keypad
        default:    nKey = tolower(nKey);       break;  // normal
        }
        switch (nKey) // exit if an exit key was hit
        {             // (note function key support on Unix can vary)
        case 0x0004:    // Ctrl-D
        case 0x001B:    // Esc
        case 0x013d:    // F3
        case 0x016b:    // Alt-F4
        case 'e':
        case 'q':
        case 'x':
            return TRUE;
        }
    }
    
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// printf2
//
// Displays to stdout and also optionally to our output log file.
//
// szFormat [in] is text message format string, as in printf().
// ... [in] represents other optional arguments to display using szFormat.
//
// Returns 0 if success, else non-zero error code.

static int printf2( const char *szFormat, ... )
{
    int nTextLen;
    va_list pva;
    va_start( pva, szFormat );
    nTextLen = vprintf( szFormat, pva );
    if (s_pszLogFileName)
    {
        // reopen file if was closed after timeout
        if (!s_fpLog)
        {
            s_fpLog = fopen( s_pszLogFileName, "a" );
        }
        if (s_fpLog)
        {
            nTextLen = vfprintf( s_fpLog, szFormat, pva );
            s_bLogfileInUse = TRUE;
        }
    }
    va_end( pva );

    return nTextLen;
}

////////////////////////////////////////////////////////////////////////////////
// displayUsage
//
// Displays usage info.

static void displayUsage()
{
    printf( "Usage:\n" );
    printf( "rtrace               Monitor the local server.\n" );
    printf( "rtrace -@ <server>   Specify the server host name or IP address\n" );
    printf( "rtrace -f <file>     Monitor the server, also log to file.\n" );
    printf( "rtrace -?            Print this help info.\n" );
    printf( "rtrace -h            Print this help info.\n" );
}

////////////////////////////////////////////////////////////////////////////////
// getText
//
// Helper function that uses ctaGetText() to format the text for a numeric code
// (error, event, reason, or command code) and returns a pointer to that text.
// If ctaGetText() fails, formats the numeric code value in hex.
// This is actually a pointer to a local static buffer.  It is not thread-safe
// and will be overwritten by subsequent calls.
//
// dwCode [in] is the numeric code.
//
// Returns a pointer to the text.

static const char *getText( DWORD dwCode )
{
    DWORD dwBufSize = 256;
    static char szBuf[ 256 ];

    switch (dwCode)
    {
    case CTAERR_SVR_COMM:       // special case until CT Access supports properly
        sprintf( szBuf, *s_szServer
            ? "server communication error with %s.\nVerify that Natural Access Server (ctdaemon) is running"
            : "server communication error.\nVerify that Natural Access Server (ctdaemon) is running",
                 s_szServer );
        break;

    case CTAERR_WAIT_FAILED:    // special case until CT Access supports properly
        sprintf( szBuf, *s_szServer
            ? "OS specific wait failed with %s.\nVerify that Natural Access Server (ctdaemon) is running"
            : "OS specific wait failed.\nVerify that Natural Access Server (ctdaemon) is running",
                 s_szServer );
        break;


    default:
        if (SUCCESS != ctaGetText( s_ctahd, dwCode, szBuf, dwBufSize ))
            sprintf( szBuf, "0x%08lx", dwCode );
        break;
    }

    return szBuf;
}

////////////////////////////////////////////////////////////////////////////////
// cleanup
//
// Cleans up.

static void cleanup()
{
    // Clean up log file.
    if (s_fpLog)
    {
        fclose( s_fpLog );
        s_fpLog = NULL;
    }

    // Stop receiving trace messages
    if (s_ctahd != NULL_CTAHD)
        ctaStopTrace( s_ctahd );

    // Clean up CT Access connection.
    if (s_ctaqueuehd != NULL_CTAQUEUEHD)
    {
        ctaDestroyQueue( s_ctaqueuehd );
        s_ctaqueuehd = NULL_CTAQUEUEHD;
        s_ctahd = NULL_CTAHD;
    }

    // Restore original keyboard settings.
    setupKbdIO( 0 );
}

////////////////////////////////////////////////////////////////////////////////
// cleanupAndExit
//
// Cleans up and exits.
//
// dwErr [in] is exit code to use.

static void cleanupAndExit( DWORD dwErr )
{
    cleanup();

    printf( "Press <Enter> to exit.\n" );
    GETCH();
    exit( (int)dwErr );
}

////////////////////////////////////////////////////////////////////////////////
// sleepAndCheckKeyboard
//
// Sleeps the specified # seconds, checking keyboard queue in case user is
// trying to quit.  If so, stops waiting and cleans up and exits.
//
// nSecs [in] is # secs to sleep.

static void sleepAndCheckKeyboard( int nSecs )
{
    int i;
    for (i = 0; i < nSecs; ++i) // retry delay time in secs
    {
        DemoSleep( 1 );              // check keyboard each sec
        if( detectKeyboardQuit() )  // check for keyboard quit, TFG 000823
        {
            cleanup();
            exit( (int)SUCCESS );
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// init
//
// Initializes.  Also handles reconnection after server communication lost.
//
// bRetry [in] means retry rather than abort when server not running.

static void init( BOOL bRetry )
{
    DWORD dwErr;
    CTA_INIT_PARMS initparms;
    char szDefaultDescriptor[] = "rtrace";
    char szDescriptor[ MAX_SERVER_SIZE + sizeof(szDefaultDescriptor) + 1] = "";
    // If server communication was lost, must clean up before trying to
    // reconnect.
    BOOL const bReconnecting = (s_ctaqueuehd != NULL_CTAQUEUEHD);
    if (bReconnecting)
        cleanup();

    // Initialize CT Access process.
    // If server is initially not found, we will generally discover and handle
    // it here.
    
    memset( &initparms, 0, sizeof(CTA_INIT_PARMS) );
    initparms.size = sizeof(CTA_INIT_PARMS);
    initparms.ctaflags = CTA_MODE_SERVER;   // force client-server mode
    dwErr = ctaInitialize( NULL, 0, &initparms );

    if (dwErr == CTAERR_SVR_COMM && bRetry) // no server (initially)
    {                                       // and we're supposed to retry
        if (!bReconnecting) // if reconnecting, we already displayed err msg
            printf2( "Can't initialize CT Access, %s.\n", getText(dwErr) );
        printf2( *s_szServer ? "Waiting for Natural Access Server on %s...\n"
                             : "Waiting for Natural Access Server...\n",
                 s_szServer );
        do
        {
            sleepAndCheckKeyboard( 5 ); // retry delay time in secs
            dwErr = ctaInitialize( NULL, 0, &initparms );
        } while (dwErr == CTAERR_SVR_COMM);
    }
    if (dwErr != SUCCESS && dwErr != CTAERR_ALREADY_INITIALIZED)
    {
        printf2( "Can't initialize CT Access, %s.\n", getText(dwErr) );
        cleanupAndExit( dwErr );
    }

    dwErr = ctaCreateQueue( NULL, 0, &s_ctaqueuehd );
    if (dwErr != SUCCESS)
    {
        printf2( "Can't create queue, %s.\n", getText(dwErr) );
        cleanupAndExit( dwErr );
    }

    // Create a CTA context.
    // If we're trying to reconnect after server communication was lost,
    // we will generally discover and handle it here.
    // If server was specified, prepend it and a slash to descriptor string.

    
    if (*s_szServer)
    {
        strcpy( szDescriptor, s_szServer );
        strcat( szDescriptor, "/" );
    }
    strcat( szDescriptor, szDefaultDescriptor );
    dwErr = ctaCreateContext( s_ctaqueuehd, 0, szDescriptor, &s_ctahd );
    if (dwErr == CTAERR_SVR_COMM && bRetry) // no server (while trying to reconnect)
    {                                       // and we're supposed to retry
        if (!bReconnecting) // if reconnecting, we already displayed err msg
            printf2( "Can't create context, %s.\n", getText(dwErr) );
        printf2( *s_szServer ? "Waiting for Natural Access Server on %s...\n"
                             : "Waiting for Natural Access Server...\n",
                 s_szServer );
        do
        {
            sleepAndCheckKeyboard( 5 ); // retry delay time in secs
            dwErr = ctaCreateContext( s_ctaqueuehd, 0, szDescriptor, &s_ctahd );
        } while (dwErr == CTAERR_SVR_COMM);
    }
    if (dwErr != SUCCESS)
    {
        printf2( "Can't create context, %s.\n", getText(dwErr) );
        cleanupAndExit( dwErr );
    }

}

////////////////////////////////////////////////////////////////////////////////
// monitor
//
// Loops monitoring the OA&M system.

static void monitor()
{
    DWORD dwErr;
    BOOL bCtaOwnsBuf;
    char* pszText;

    // Set up unbuffered keyboard I/O.
    setupKbdIO( 1 );

RECONNECT:
    init( TRUE );
    ctaStartTrace( s_ctahd );
    printf2( "Ready (press Esc or q to exit)...\n" );

    for (;;)
    {
        // Retrieve an event from the event queue.
        CTA_EVENT cta_event;
        dwErr = ctaWaitEvent( s_ctaqueuehd, &cta_event, 1000 );
        if (dwErr != SUCCESS)
        {
            printf2( "Error waiting for event, %s.\n", getText(dwErr) );
            goto RECONNECT; // assume server connection lost
        }

        // Check if buffer is owned by CTA and must be freed by us below.
        bCtaOwnsBuf = cta_event.buffer && (cta_event.size & CTA_INTERNAL_BUFFER);
        if (bCtaOwnsBuf)
            cta_event.size &= ~CTA_INTERNAL_BUFFER; // clear flag from size

        // Process the event.
        if (cta_event.id == CTAEVN_WAIT_TIMEOUT)
        {
            // 000823: TFG: Changed to subroutine
            if( detectKeyboardQuit() == TRUE ) 
                return;
            // If log file is open and not in recent use, close (and flush) it
            // when 2 timeouts occur with no intervening output (agmon's logic).
            // This is more efficient than flushing every write and also allows
            // the file to be opened by a viewer.
            if (s_fpLog != NULL)
            {
                if (s_bLogfileInUse)
                    s_bLogfileInUse = FALSE;
                else
                {
                    fclose( s_fpLog );
                    s_fpLog = NULL;
                }
            }
        }
        else if (cta_event.id == CTAEVN_TRACE_MESSAGE) // if it's a trace event
        {
            if (!cta_event.buffer)
            {
                printf2( "Error, trace event has NULL buffer.\n" );
                continue;
            }

            pszText = (char*)cta_event.buffer;

            printf2( "%s", pszText );
        }
        else    
            printf2( "%s\n", getText(cta_event.id) );

        if (bCtaOwnsBuf)
            ctaFreeBuffer( cta_event.buffer ); // our reponsibility to free
    } // end for(;;)
}

////////////////////////////////////////////////////////////////////////////////
// main
//
// Remote Trace Monitor main program.
//
// argc, argv [in] command line switches described in usage function below.
//
// Exits with 0 if success, else non-zero error code.

int main( int argc, char *argv[] )
{
    BOOL bLog = FALSE;
    int c;

    if (argc < 2)   // no command-line args
        goto MONITOR;

    // Handle command-line args.
    switch (*argv[1])
    {
    case '-':
    case '/':
        break;
    default:    // first arg must be an option, i.e. must start with '-' or '/'
        goto DISPLAY_USAGE;
    }

    while ((c = getopt( argc, argv, "@:f:?h" )) != -1)
    {
        switch (c)
        {
            case '@':   // server host name
                if (strlen(optarg) >= MAX_SERVER_SIZE)
                {
                    printf( "-%c argument is too long.\n", c );
                    exit(EXIT_FAILURE);
                }
                strcpy( s_szServer, optarg );
                break;

            case 'f':   // monitor the system, log also to file
                bLog = TRUE;
                break;

            case '?':   // help
            case 'h':   // help
            {
DISPLAY_USAGE:
                displayUsage();
                exit(0);
            }
        }
    }

    if (bLog)   // monitor the system, log also to file
    {
        // 000605: TFG: Changed from write to append.
        s_pszLogFileName = optarg;
        s_fpLog = fopen( s_pszLogFileName, "a" );
        if (!s_fpLog)
        {
            printf( "Can't open log file \"%s\".\n", s_pszLogFileName );
            exit( EXIT_FAILURE );
        }
        printf( "Appending output to log file \"%s\".\n", s_pszLogFileName );
    }

MONITOR:
    monitor();
    cleanup();
    exit( (int)SUCCESS );
    return 0;
}
