/*************************************************************************
 *  FILE: takecmd.c
 *
 *  DESCRIPTION:  This program illustrates an application's ability to
 *                provide robust behavior when operating in the Multi-
 *                Server Natural Access development environment. This
 *                is achieved by using the shared context and shared object
 *                handle capabilities provided in this development
 *                environment
 *
 *                This program works in conjunction with the program
 *                takeover. It is modeled after the ctatest program and
 *                allows users to interactively enter commands that modify
 *                the runtime behavior of takeover.
 *
 * Copyright (c) 2001-2002 NMS Communications.  All rights reserved.
 *************************************************************************/

#define VERSION_NUM  "1"
#define VERSION_DATE __DATE__

#if defined (WIN32)
#include <conio.h>
#include <process.h>
#include <stdlib.h>
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

#include <ctademo.h>
#include <dtmdef.h>
#include <nccdef.h>
#include <nccxadi.h>
#include <nccxisdn.h>
#include <nccxcas.h>

#include <takeover.h>

/******************************************************************************
*
* Process GLOBAL Data
*
******************************************************************************/

CALL_ARGS      gArgs = {0};

/* General process initialization with Natural Access */
CTA_SERVICE_NAME  svcList[] = {0};
unsigned          gNumSvcs  = 0;

CTA_THRD_INFO ctaThrdData[ 1 ] = {0};

/* Flag that input from file is being used. */
BOOL      Ioffline = FALSE;
unsigned  Repeatentry   = 0;
FILE     *InputFileStream;

/* Buffer information sent in command event to takeover */

char geventNameBuffer[ MAX_BUFFER_SIZE ] = {0};
char geventTrunkBuffer[ MAX_BUFFER_SIZE ] = {0};
char geventCCVBuffer[ MAX_BUFFER_SIZE ] = {0};

/* Message used to request get keywords  */
typedef union
{
    char       buf[ MAX_BUFFER_SIZE ];
    OAM_CMD_KW cmdParams;
} APP_KW_CMD_PARAM;

APP_KW_CMD_PARAM geventOAM = {0};

/*----------------------------------------------------------------------------
  Key table and table operation routines
  ----------------------------------------------------------------------------*/

typedef struct { unsigned value; char *help; } KEYS;

#define SKIPLINE '\n',""
#define PAIR( A, B ) ((A<<8)+B)

/* Note - single key commands have to be included in the string in fetchkeys()*/
static KEYS keys[] =
{
    PAIR( 'H',  0  ),  "help",
    PAIR( 'Q',  0  ),  "quit",
    PAIR( 'D', 'C' ),  "delay command",
    SKIPLINE,
    SKIPLINE,

    PAIR( 'E', 'B' ),  "exit backup",
    PAIR( 'E', 'P' ),  "exit primary",
    PAIR( 'E', 'T' ),  "exit takeover",
    PAIR( 'D', 'R' ),  "display runstate",
    PAIR( 'S', 'B' ),  "switch backup",
    SKIPLINE,
    SKIPLINE,

    PAIR( 'S', 'M' ),  "start monitor",
    PAIR( 'P', 'M' ),  "stop monitor",
    PAIR( 'G', 'S' ),  "get status",
    SKIPLINE,
    SKIPLINE,

    PAIR( 'B', 'B' ),  "boot board",
    PAIR( 'P', 'B' ),  "stop board",
    PAIR( 'G', 'K' ),  "get keyword",
    PAIR( 'P', 'G' ),  "stop get",

    SKIPLINE,
    SKIPLINE,

    PAIR( 'E', 'C' ),  "establish call",
    PAIR( 'P', 'C' ),  "hangup call",
    PAIR( 'P', 'F' ),  "play file",
    PAIR( 'P', 'P' ),  "stop play",
    PAIR( 'C', 'D' ),  "collect digits",
    SKIPLINE,
    /* END */
    0,""
};


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
  HelpKeys()
  Prints help info.
  ----------------------------------------------------------------------------*/
#define NULL_TO_SPACE(c) ( (c)==0 ? ' ' : c )

void showCmds( void )
{
    KEYS    *key;
    unsigned i;
    CTA_REV_INFO revinfo = { 0 } ;

    ClearScreen();

    ctaGetVersion( &revinfo, sizeof revinfo );

    printf( "NA Failover Demonstration Command Program  V.%s (%s)"
            " Natural Access Version is %d.%02d\n",
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

/* ------------------------------------------------------------------------- */

void PrintHelp( void )
{
    printf( "Natural Access Multi-Server Object Sharing Demo (TESTCMD) V.%s (%s)\n",
            VERSION_NUM, VERSION_DATE );

    printf( "Usage: takecmd [opts]\n" );
    printf( "  where opts are:\n\n" );
    printf( "  -c name             Server host name of used for commands    \n"
            "                        Default = %s\n", *gArgs.cmdHost ?
                                        gArgs.cmdHost : "localhost" );
    printf( "  -i filename         Input file for commands                  \n");

}

void GetLineFromFile(char *line, int len, FILE *fp)
{
    char *ret;
    char *temp;
    char *currptr;
    int  i = 0;

    temp = (char *)malloc(sizeof(char) * len);

    do
    {
        ret = fgets(temp, len, fp);
        if ( !ret )
        {
            /* End of file or error; turn off file input */
            strcpy( temp, "h" );
            Ioffline = FALSE;
        }
    } while (temp[0] == '#');

    currptr = temp;
    while (*currptr != '\0')
    {
        if (*currptr == '\\' && *(currptr + 1) == '#')
        {
            line[i] = '#';
            i++;
            currptr += 2;
        }
        else if (*currptr == '#')
        {
            line[i] = '\n';
            i++;
            break;
        }
        else if ( *currptr < 0x1f )
        {
            /* Skip CTRL char */
            currptr++;
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

}

/* -------------------------------------------------------------------------
  Process command line options.
  ------------------------------------------------------------------------- */
void ProcessArgs( int argc, char **argv )
{
    int         c;

    while( ( c = getopt( argc, argv, "vc:h?i:" ) ) != -1 )
    {
        switch( c )
        {
            case 'c':
                /* server host name where command app (takecmd) is running  */
                if (strlen(optarg) >= MAX_SERVER_SIZE)
                {
                    printf( "-%c argument is too long.\n", c );
                    exit( -1 );
                }
                strcpy( gArgs.cmdHost, optarg );
                break;

            case 'v':
                DemoSetReportLevel( DEMO_REPORT_ALL );
                gArgs.bVerbose = TRUE;
                break;

            case 'i':
                sscanf( optarg, "%s", gArgs.inputFileName);
                Ioffline = TRUE;
                if ((InputFileStream = fopen( gArgs.inputFileName, "r"))
                    == NULL)
                {
                    printf("Cannot open Input File %s\n", gArgs.inputFileName );
                    exit( -1 );
                }
                break;


            case '?':
            default:
                PrintHelp();
                /* Put back old keyboard mode */
                DemoSetupKbdIO(0);
                exit( 1 );
                break;
        }
    }
    if (optind < argc)
    {
        printf( "Invalid option.  Use -? for help.\n" );
        /* Put back old keyboard mode */
        DemoSetupKbdIO(0);
        exit( 1 );
    }
}

/******************************************************************************
*
* setupCtaOnThread
*
*    This function establishes a NA context on the specified server for the
*    calling thread. If services need to be opened, this is also done at this
*    time.
*
******************************************************************************/

DWORD setupCtaOnThread( CTA_THRD_INFO *pctaInfo, char *host )
{
    BOOL  retry = FALSE;
    DWORD ret = SUCCESS;
    char  cntxName[ MAX_DESCRIPTOR_SIZE ] = {0};

    /* The context name to be used needs to be prepended with the host
    *  name if present, i.e., "hostname/context-name"
    */
    if ( *host )
    {
        strcpy( cntxName, host );
        strcat( cntxName, "/" );
    }
    else
        /* Use the localhost's server for creating our context */
        ctaSetDefaultServer( "localhost" );

    strcat( cntxName, pctaInfo->ctxName );

    if ( pctaInfo->ctaqueuehd != NULL_CTAQUEUEHD )
    {
        /* Resetting queue and context after failure */
        retry = TRUE;
        printf("Reestablishing contact with the NA server.\n");
        ctaDestroyQueue( pctaInfo->ctaqueuehd );
        pctaInfo->ctaqueuehd = NULL_CTAQUEUEHD;
    }

    /* First create a queue from which to recieve events and then
    *  create or attcach to the context using the new queue.
    */
    if (( ret = ctaCreateQueue(  pctaInfo->svcMgrList, pctaInfo->numSvcs,
                                &pctaInfo->ctaqueuehd ))
                  != SUCCESS )
    {
        printf("Context: %s, Unable to create Natural Access queue - returns 0x%x\n",
                cntxName, ret);
        exit( 1 );
    }

    do
    {
        if ( ret != SUCCESS )
        {
            retry = TRUE;
            /* Previous context create/attach failed; retry in a little while.*/
            DemoSleep( 2 );
        }

        if ( (ret = ctaCreateContextEx( pctaInfo->ctaqueuehd, 0, cntxName,
                                       &pctaInfo->ctahd,
                                       0 )) != SUCCESS )
        {
            if ( ret == CTAERR_SVR_COMM )
            {
                printf("Currently unable to create command context.\n");
                if ( *host )
                    printf( "Verify the NA Server ctdaemon is running on %s.\n",host);
                else
                    printf( "Verify the NA Server ctdaemon is running.\n");
            }
            else if ( ret == CTAERR_BAD_NAME )
            {
                /* Context already exists; attach to it */
                ret = ctaAttachContext( pctaInfo->ctaqueuehd, 0, cntxName,
                                        &pctaInfo->ctahd );
                if ( ret != SUCCESS )
                {
                    printf("Context: %s, Unable to attach to context - returns 0x%x\n",
                            cntxName, ret);
                    exit( 1 );
                }
                printf("Have attached to existing Natural Access context %s\n", cntxName );
            }
            else
            {
                printf("Unable to create Natural Access context - returns 0x%x\n", ret);
                exit( 1 );
            }
        }
    }   while ( ret != SUCCESS );

    if ( retry )
    {
        printf("\nHave SUCCESSfully created the command context.\n");
        /* Give user chance to see message and refresh the screen. */
        DemoSleep( 2 );
        showCmds();
    }
    return ret;
}

/*******************************************************************************
 *
 *  sendEvent
 *
 *      This function queues our application event to the specified queue.
 *
 *******************************************************************************/

 void sendEvent( CTAHD ctahd, DWORD evtId, DWORD evtValue, void *pbuffer,
                 DWORD size )
{
    CTA_EVENT event = { 0 };

    event.id = evtId;
    event.ctahd = ctahd;
    event.value = evtValue;
    event.buffer = pbuffer;

    if ( pbuffer )
    {
        event.size = size;
        event.buffer = pbuffer;
    }

    if ( ctaQueueEvent( &event ) != SUCCESS )
        DemoShowError( "ctaQueueEvent failed", event.value );
    else
        printf("OK\n");

}

/*---------------------------------------------------------------------------
 * promptuns - prompt for an unsigned value
 *
 *--------------------------------------------------------------------------*/
void promptuns (char *text, unsigned *target)
{
    char     temp[80];                     /* used to get user string input */

    if (Ioffline)
    {
        if (!Repeatentry)
        {
            GetLineFromFile(temp, 80, InputFileStream);
            sscanf(temp, "%i", target);
        }
        printf( "\t%s: %u\n", text, *target );
    }
    else
    {
        printf( "\t%s [%u]: ", text, *target );
        if (!Repeatentry)
        {
            fgets(temp, sizeof temp, stdin);
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

    if (Ioffline)
    {
        if (!Repeatentry)
        {
            GetLineFromFile(temp, 80, InputFileStream);
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
            // gets(temp);
            fgets(temp, sizeof temp, stdin);
            temp[strlen(temp)-1] = '\0';

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

    if (Ioffline)
    {
        if (!Repeatentry)
        {
            GetLineFromFile(temp, 80, InputFileStream);
            sscanf (temp, "%c", target);
        }
        printf( "%s: '%c'\n", text, *target );
    }
    else
    {
        printf( "%s ['%c']: ", text, *target );
        if (!Repeatentry)
        {
            fgets(temp, sizeof temp, stdin);
            sscanf (temp, "%c", target);
        }
        else
            puts( "" );
    }
#else
    char     c;

    if (Ioffline)
    {
        if (!Repeatentry)
        {
            GetLineFromFile(temp, 80, InputFileStream);
            sscanf (temp, "%c", &c);
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
    key = toupper( key );               /* make it upper case */
    if (key == '\033' || key == '\010') /* ESC or BS */
    {
        keyscollected = 0 ;
        return 0;                       /* no key PAIR yet */
    }

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

  Read keys from a file
  ----------------------------------------------------------------------------*/
unsigned FetchKeysFromFile()
{
    char line[80];
    char keyword[8];
    int key1 = 0;
    int key2 = 0;

    GetLineFromFile(line, 80, InputFileStream);
    sscanf(line, "%s", keyword);

    key1 = (keyword[0] != '\0') ? toupper((int)keyword[0]) : 0;

    if (key1 == 0)
        return  0;

    key2 = (keyword[1] != '\0') ? toupper((int)keyword[1]) : 0;

    keyword[3] = '\0';
    printf("%s\n", keyword);

    return PAIR( key1, key2 );

}

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
  TestShutdown

  Synchronously closes everything we setup above.
  ----------------------------------------------------------------------------*/
void TestShutdown(CTAQUEUEHD ctaqueuehd)
{
    DWORD ret;

    /* Put back old keyboard mode */
    DemoSetupKbdIO(0);

    ret = ctaDestroyQueue( ctaqueuehd );

    if (ret == SUCCESS)
        printf("Destroying the queue...\n");
    else
        printf("Destroy queue failed...\n");
}

/*----------------------------------------------------------------------------
  PerformFunc()

  Executes two character commands.
  ----------------------------------------------------------------------------*/
void PerformFunc( CTA_THRD_INFO *pctaInfo, unsigned key )
{
    unsigned ret;                          /* return code from ADI calls    */
    static unsigned prevkey = 0;
    static BOOL     accessmode = FALSE;


    ret = SUCCESS;              /* start with no errors */
    if (key == PAIR( '!',0 ))   /* '!' repeats last good entry */
    {
        if (prevkey != 0)
        {
            Repeatentry   = 1;
            PerformFunc( pctaInfo, prevkey ) ;
            Repeatentry   = 0;
        }
        return ;
    }

    switch( key )
    {
        case PAIR( 'D', 'C' ):   /* delay command */
        {
            /* Delay execution of next command */
            static unsigned sec;
            promptuns( "Sleep seconds", &sec );
            DemoSleep( sec );
            printf("...ready\n");
            break;

        }
        case PAIR( 'H', 0 ):                /* HELP */
        case PAIR( '?', 0 ):
            showCmds();
            return;

        case PAIR( 'Q', 0 ):                /* QUIT */
            TestShutdown( pctaInfo->ctaqueuehd );
            exit( 0 );
            break;

        case PAIR( 'V', 0 ):               /* "verbose output" */
            DemoSetReportLevel( DEMO_REPORT_ALL );
            break;

        case PAIR( 'E', 'B' ):             /* "exit backup" */
            sendEvent( pctaInfo->ctahd, APPEVN_CMD_EXIT_B,
                            APPEVN_CMD_FOR_GENERAL, NULL, 0 );
            break;

        case PAIR( 'E', 'P' ):             /* "exit primary" */
            sendEvent( pctaInfo->ctahd, APPEVN_CMD_EXIT_P,
                            APPEVN_CMD_FOR_GENERAL, NULL, 0 );
            break;

        case PAIR( 'E', 'T' ):             /* "exit takeover" */

            /* Both the primary and backup instances should exit */
            sendEvent( pctaInfo->ctahd, APPEVN_CMD_EXIT,
                       APPEVN_CMD_FOR_GENERAL, NULL, 0);
            TestShutdown( pctaInfo->ctaqueuehd );
            exit( 0 );
            break;


        case PAIR( 'S', 'B' ):             /* "switch backup" */
            sendEvent( pctaInfo->ctahd, APPEVN_CMD_SWITCH,
                            APPEVN_CMD_FOR_GENERAL, NULL, 0 );
            break;

        case PAIR( 'R', 'B' ):             /* "reset backup" */
            /* Have backup instance of takeover get uptodate information from
            *  the primamry.
            */
            break;


       case PAIR( 'D', 'R' ):  /* display runstate */
           sendEvent( pctaInfo->ctahd, APPEVN_CMD_RUNSTATE,
                       APPEVN_CMD_FOR_GENERAL, NULL, 0);
           break;

        /*******/
        /* OAM */
        /*******/

        case PAIR( 'B', 'B' ):              /* boot board */

            promptstr( "Board name [<ALL> for all boards]",
                        &geventNameBuffer[0], MAX_BUFFER_SIZE  );
            if ( !strcmp( geventNameBuffer, "ALL" ) ||
                 !strcmp( geventNameBuffer, "all" ) )

                sendEvent( pctaInfo->ctahd, APPEVN_OAM_START,
                           APPEVN_CMD_FOR_OAM, NULL, 0 );
            else
                sendEvent( pctaInfo->ctahd, APPEVN_OAM_START,
                           APPEVN_CMD_FOR_OAM, geventNameBuffer,
                           strlen( geventNameBuffer ) + 1 );
            break;

        case PAIR( 'P', 'B' ):              /* stop board */

            promptstr( "Board name [<ALL> for all boards]",
                        &geventNameBuffer[0], MAX_BUFFER_SIZE  );

            if ( !strcmp( geventNameBuffer, "ALL" ) ||
                 !strcmp( geventNameBuffer, "all" ) )
                sendEvent( pctaInfo->ctahd, APPEVN_OAM_STOP,
                           APPEVN_CMD_FOR_OAM, NULL, 0 );
            else
                sendEvent( pctaInfo->ctahd, APPEVN_OAM_STOP,
                           APPEVN_CMD_FOR_OAM, geventNameBuffer,
                           strlen( geventNameBuffer ) + 1 );

            break;

        case PAIR( 'G', 'K' ):  /* get keyword */
        case PAIR( 'P', 'G' ):  /* stop get */
        {
            OAM_CMD_KW *pkwParams = &geventOAM.cmdParams;

            unsigned  *pnum = (unsigned *) geventTrunkBuffer;

            promptstr( "\tOAM object name, e.g., board name",
                       pkwParams->objName, sizeof( pkwParams->objName ) );
            promptstr( "\tWhich keyword", pkwParams->keyWord,
                        sizeof( pkwParams->keyWord ) );

            switch( key )
            {
                case PAIR( 'G', 'K' ):
                    promptuns( "How often [# seconds]",
                                (unsigned *)&pkwParams->howOften );
                    promptuns( "How many times [# loops]",
                                (unsigned *)&pkwParams->numGets );
                    sendEvent( pctaInfo->ctahd, APPEVN_OAM_GET_KW,
                                APPEVN_CMD_FOR_OAM,
                                geventOAM.buf, sizeof( OAM_CMD_KW ) );
                    break;
                default:
                    sendEvent( pctaInfo->ctahd, APPEVN_OAM_GET_STOP,
                                APPEVN_CMD_FOR_OAM,
                                geventOAM.buf, sizeof( OAM_CMD_KW ) );
                    break;
            }
            break;
        }


        /*******/
        /* DTM */
        /*******/

        case PAIR( 'S', 'M' ):  /* "start monitor" */
        case PAIR( 'P', 'M' ):  /* "stop monitor" */
        case PAIR( 'G', 'S' ):  /* "get status" */
        {
            unsigned  *pnum = (unsigned *) geventTrunkBuffer;
            int cmd;

            promptuns( "Board number", pnum++);
            promptuns( "Trunk number", pnum );

            switch( key )
            {
                case PAIR( 'S', 'M' ): cmd = APPEVN_DTM_START; break;
                case PAIR( 'P', 'M' ): cmd = APPEVN_DTM_STOP; break;
                case PAIR( 'G', 'S' ): cmd = APPEVN_DTM_GETSTATUS; break;
            }
            sendEvent( pctaInfo->ctahd, cmd, APPEVN_CMD_FOR_DTM, geventTrunkBuffer,
                       2 * sizeof(unsigned) );
            break;
        }

        /*******/
        /* CCV */
        /*******/


        case PAIR( 'E', 'C' ):  /* establish call */
            sendEvent( pctaInfo->ctahd, APPEVN_CCV_DO_CALL,
                            APPEVN_CMD_FOR_CCV, NULL, 0 );
            break;

        case PAIR( 'P', 'C' ):  /* Hangup call */
            sendEvent( pctaInfo->ctahd, APPEVN_CCV_HANG_UP,
                            APPEVN_CMD_FOR_CCV, NULL, 0 );
            break;

        case PAIR( 'P', 'F' ):  /* play file" */
            sendEvent( pctaInfo->ctahd, APPEVN_CCV_PLAY_FILE,
                            APPEVN_CMD_FOR_CCV, NULL, 0 );
            break;

        case PAIR( 'P', 'P' ):  /* stop play " */
            sendEvent( pctaInfo->ctahd, APPEVN_CCV_ABORT_PLAY,
                            APPEVN_CMD_FOR_CCV, NULL, 0 );
            break;

        case PAIR( 'C', 'D' ):  /* "collect digits" */
        {
            unsigned  *pnum = (unsigned *) geventCCVBuffer;

            promptuns( "Number of digits", pnum );
            sendEvent( pctaInfo->ctahd, APPEVN_CCV_COLLECT,
                       APPEVN_CMD_FOR_CCV, pnum, sizeof( unsigned ) );
            break;
        }
        default:

            showCmds();
            break;
    }
}


/******************************************************************************
*
* THREAD: processCommand
*
******************************************************************************/

THREAD_RET_TYPE THREAD_CALLING_CONVENTION processCommand( void *arg )

{
    DWORD     ret = SUCCESS;
    unsigned  eventtype;
    CTA_EVENT event = {0};

    CTA_THRD_INFO *pctaInfo = ctaThrdData;

    /* Setup NA information required for this thread.Since this thread
    *  is only responsible for processing commands from takecmd via a
    *  shared context, no special NA services are required. This process
    *  attached to the context created by takecmd.
    */

    strcpy ( pctaInfo->ctxName, CMD_CNTX_NAME );
    pctaInfo->numSvcs = 0;
    pctaInfo->ctaqueuehd = NULL_CTAQUEUEHD;

    if ( (ret = setupCtaOnThread( pctaInfo, gArgs.cmdHost )) == SUCCESS )
    {
        for (;;)
        {
            /* Wait for CTA_EVENT, keyboard events, or offline input */
            for (;;)
            {
                ret = ctaWaitEvent( pctaInfo->ctaqueuehd, &event, 100);
                if ( ret != SUCCESS )
                {
                    char text[ 40 ] = {0};
                    if (ret == CTAERR_SVR_COMM )
                    {
                        printf("Error from ctaWaitEvent CTAERR_SVR_COMM\n" );
                    }
                    else
                    {
                        ctaGetText( pctaInfo->ctahd, ret, text, sizeof(text) );
                        printf("Error from ctaWaitEvent 0x%x -- %s\n", ret, text );
                    }
                    /* Put back old keyboard mode */
                    DemoSetupKbdIO(0);
                    /* Assume server failure; reestablish context */
                    if( (ret = setupCtaOnThread( pctaInfo, gArgs.cmdHost ))
                         != SUCCESS )
                        exit( 1 );
                }
                else if (event.id == CTAEVN_WAIT_TIMEOUT)
                {
                    if (Ioffline)
                    {
                        eventtype = 1;
                        break;
                    }
                    else if (kbhit())
                    {
                        eventtype = 1;
                        break;
                    }
                    else
                        continue;
                }
                else /* got a real event */
                {
                    eventtype = 0;
                    break;
                }
            }

            if( eventtype != 0 )         /* a keyboard event is ready */
            {
                int key;

                if (Ioffline)
                    key = FetchKeysFromFile();
                else
                    key = FetchKeys();
                if( key != 0)
                {
                    PerformFunc( pctaInfo, key );
                }
            }
            else                       /* a NA event needs to be processed */
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

                    /* MyEventHandler( &event ); */

                    if ( freeCtaBuff )
                        /* When running in Client/Server mode, internal
                         * Natural Access buffers were allocated that must
                         * now be freed.
                        */
                        ctaFreeBuffer( event.buffer );
                }

            }
        } /* 1st for */
    }

    THREAD_RETURN;
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

    /* Configure Raw Mode, before any printfs */
    DemoSetupKbdIO(1);
    DemoSetReportLevel( DEMO_REPORT_UNEXPECTED );

    ProcessArgs( argc, argv );

    printf( "Natural Access Multi-Server Object Sharing Demo (TESTCMD) V.%s (%s)\n",
            VERSION_NUM, VERSION_DATE );

    /* Disable error hander; will deal with errors within this app. */
    ctaSetErrorHandler( NULL, NULL );

    /* Display list of commands */
    showCmds();

    /* initialize Natural Access services needed by this process */
    initparms.size = sizeof(CTA_INIT_PARMS);
    initparms.parmflags  = 0;

    ctaSetDefaultServer( "localhost" );
    initparms.ctaflags = CTA_CONTEXTNAME_REQUIRE_UNIQUE;

    if ( (ret = ctaInitialize( svcList, gNumSvcs, &initparms )) != SUCCESS )
    {
        printf("Unable to Initialize with Natural Access - 0x%x\n", ret);
        exit( 1 );
    }


    /* Start main processing thread */
    if( DemoSpawnThread( processCommand, (void *) NULL) == FAILURE )
    {
            printf("Could not create thread: \n");
            exit(-1);
    }


    /* Sleep forever. */
    for(;;)
        DemoSleep( 1000 );

    UNREACHED_STATEMENT( return 0; )
}

