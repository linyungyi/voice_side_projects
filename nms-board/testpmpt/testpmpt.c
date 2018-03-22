/*************************************************************************
* FILE:         tstprmpt.c
* DESCRIPTION:  This program allows prompt tables to be tested without the
*               hassle of pre-recording all of the voice prompts.
*
* Copyright (c) 1996-2002 NMS Communications.  All rights reserved.
***************************************************************************/
#define REVISION_NUM  "2"
#define REVISION_DATE __DATE__
#define VERSION_NUM   "1"

/*----------------------------- Includes -----------------------------------*/

#include "ctademo.h"
#include "ctadef.h"
#include "vcedef.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*----------------------------- Application Definitions --------------------*/
typedef struct
{
    CTAHD ctahd;                  /* what we initialize with OpenPort       */
    CTAQUEUEHD  ctaqueuehd;
    char   table[CTA_MAX_PATH];   /* prompt rules file */
    char   voxfile[CTA_MAX_PATH]; /* prompt voice file (for text) */
    char   text[CTA_MAX_PATH];    /* prompt text file (if separate) */
} MYCONTEXT;
MYCONTEXT MyContext = {0} ;


/*----------------------------- Forward References ------------------------*/
DWORD NMSSTDCALL ErrorHandler( CTAHD ctahd, DWORD errorcode,
                               char *errortext, char *func );

void RunDemo( MYCONTEXT *cx );
void Setup( CTAQUEUEHD *ctaqueuehd, CTAHD *ctahd );


/*----------------------------------------------------------------------------
                            PrintHelp()
----------------------------------------------------------------------------*/
void PrintHelp( MYCONTEXT *cx )
{
    printf( "Usage: testpmpt [opts]\n" );
    printf( "  where opts are:\n" );
    printf( "  -p table      prompt table name (assumed .tbl ext).    "
           "Default = %s\n", cx->table );
    printf( "  -v voicefile  contains message text (assume .vox ext). "
           "Default = <table-name>\n" );
    printf( "  -t textfile   prompts in text form (assume .txt ext).\n");
}

/*----------------------------------------------------------------------------
                                main
----------------------------------------------------------------------------*/
int main( int argc, char **argv )
{
    int        c;
    MYCONTEXT *cx = &MyContext;
    int        voxspecified = 0;
    char      *pext;

    printf( "Natural Access Test Prompt Utility V %s.%s (%s)\n",
            VERSION_NUM, REVISION_NUM, REVISION_DATE );

    strcpy( cx->table,   "american" );

    while( ( c = getopt( argc, argv, "p:t:v:Hh?" ) ) != -1 )
    {
        switch( c )
        {
            case 'p':
                strcpy( cx->table, optarg );
                break;
            case 't':
                strcpy( cx->text, optarg );
                break;
            case 'v':
                strcpy( cx->voxfile, optarg );
                voxspecified = 1;
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
        printf( "Invalid option.  Use -h for help.\n" );
        exit( 1 );
    }
    if (voxspecified && cx->text[0] != '\0')
    {
        printf( "\007Please specify either a voice file for message text "
                "or a text file, but not both.");
        exit( 1 );
    }
    if (!voxspecified)
    {
        /* Normally the VOX file has the same base name as the table file */
        strcpy( cx->voxfile, cx->table );
        if ((pext=strchr(cx->voxfile, '.')) != NULL)
            *pext = '\0' ;
    }
    if ((pext=strchr(cx->table, '.')) == NULL)
        strcat (cx->table, ".tbl") ;

    printf( "\tPrompt table =   %s\n", cx->table );
    /* VoX/Text file name printed in RunDemo */

    RunDemo( cx );
    return 0;
}


/*-------------------------------------------------------------------------
                            ErrorHandler
-------------------------------------------------------------------------*/
DWORD NMSSTDCALL ErrorHandler( CTAHD ctahd, DWORD errorcode,
                               char *errortext, char *func )
{
    if (errorcode != VCEERR_INVALID_MESSAGE
     && errorcode != CTAERR_FILE_NOT_FOUND)
    {
        printf( "\007Error in %s: %s (%#x)\n", func, errortext, errorcode );
        exit( errorcode );
    }
    return errorcode;
}

#define MAXENTRIES 500
#define MAXMSGLIST 100
/*-------------------------------------------------------------------------
                            RunDemo
-------------------------------------------------------------------------*/
void RunDemo( MYCONTEXT *cx )
{
    char        filename[CTA_MAX_PATH];
    unsigned    i;
    unsigned    ret;
    char       *msgtxt[MAXENTRIES] = { NULL };
    unsigned    maxtextarray = 0;
    VCEPROMPTHD prompthandle;
    char        buff[255];
    unsigned    msglst[MAXMSGLIST];
    unsigned    msgcnt;

     /* Turn off display of events and function comments */
    DemoSetReportLevel(DEMO_REPORT_UNEXPECTED);

    /* Register an error handler to display errors returned from functions;
     * no need to check return values in the demo.
     */
    ctaSetErrorHandler( ErrorHandler, NULL );

    /* Open a call resource: a Natural Access processing context w/voice manager */
    Setup( &cx->ctaqueuehd, &cx->ctahd );

    /* Load the file containing text representations of the prompts */
    if (cx->text[0] == '\0')
    {
        /* Voice file specified.  Use the message text functionns to read the
         * text for each voice message in the file.
         */

        VCEHD     vh;
        unsigned  highmessage;
        unsigned  message;
        char      textbuf[80];
        unsigned  textsize;
        unsigned  filetype;

         /* If the file has no extension, default to .VOX */
        filetype = 0 ;   /* infer from extension if there is one */
        DemoDetermineFileType (cx->voxfile, 0, filename, &filetype);
        printf( "\tPrompt text from %s\n", filename );

        if ( ( ret = vceOpenFile( cx->ctahd, filename, 0, VCE_PLAY_ONLY, 0,
                                  &vh ) ) != SUCCESS )
        {
            /* Assume file-not-found. Note that other errors will be caught by
             * the error handler.
             */
            fprintf( stderr, "\t*** Unable to open voice file %s\n", filename );
            return;
        }

        vceGetHighMessageNumber( vh, &highmessage );
        for ( message = 0;
              highmessage != VCE_UNDEFINED_MESSAGE
                && message <= highmessage
                && message < MAXENTRIES;
              message++ )
        {
            if (vceReadMessageText (vh, message, textbuf, sizeof textbuf-1,
                                              &textsize) != SUCCESS
                || textsize == 0)
                continue;

             /* Add a terminating nul if necessary.*/
            if (textbuf[textsize-1] != '\0')
                textbuf[textsize] = '\0';

            msgtxt[message] = malloc (strlen(textbuf)+1);
            strcpy( msgtxt[message], textbuf );

            if( message > maxtextarray )
                maxtextarray = message;
        }
    }
    else
    {
        /* Text file specified.  Each line of the file is of the form:
         *     <message number>:<text>.
         * Lines not starting with a digit are ignored.
         */

        FILE *fp;
        char *pc;


        if( ( ret = ctaFindFile( cx->text, "txt", "CTA_DPATH",
                                 filename, sizeof filename ) ) != SUCCESS )
        {
            fprintf( stderr, "Error! Prompt Text file %s not found\n",cx->text);
            return;
        }
        printf( "\tPrompt text from %s\n", filename );

        fp = fopen( filename, "r" );
        if( fp == NULL )
        {
            fprintf( stderr, "Open error on '%s': %s\n", filename,
                      strerror(errno));
            return;
        }

        while( !feof( fp ) )
        {
            fgets( buff, sizeof buff, fp );  /* read a line */

            if( feof( fp ) )
                break;

            if( isdigit( *buff ) )
            {
                i = atoi( buff );
                if( i >= MAXENTRIES )
                    continue;

                pc = strchr( buff, ':' );
                if( pc != NULL )
                {
                    char *ptmp;
                    ++pc;
                    for(ptmp=pc ; *ptmp != 0; ptmp++ )
                        if( iscntrl( *ptmp) )
                        {
                            *ptmp = '\0';
                            break;
                        }
                    ptmp = malloc (strlen(pc)+1);
                    strcpy( ptmp, pc );
                    msgtxt[i] = ptmp;

                    if( i > maxtextarray )
                        maxtextarray = i;
                }
            }
        }
    }

    if (maxtextarray == 0)
    {
        fprintf (stderr, "(Warning) No prompt text found in %s\n", filename);
    }

    /* Load the prompt table */

    ret = vceLoadPromptRules( cx->ctahd, cx->table, &prompthandle );
    if ( ret != SUCCESS )
    {
        char buff[80];
        ctaGetText( NULL_CTAHD, ret, buff, sizeof buff );
        fprintf( stderr, "Error loading prompt tables, %s (0x%x)\n",
                 buff, ret );
    }
    else
    {
        for( ;; )
        {
            printf( "Enter text: " );
            fgets( buff, sizeof buff, stdin );
            if ( strlen( buff ) == 0 || buff[0] == '\n' )  break;

            buff[strlen(buff)-1] = '\0';
            msgcnt = 0;
            ret = vceBuildPromptList( prompthandle, 0,
                                      buff,
                                      msglst, MAXMSGLIST, &msgcnt );

            if ( ret != SUCCESS )
            {
                char buff[80];
                ctaGetText( NULL_CTAHD, ret, buff, sizeof buff );
                fprintf( stderr, "Error in build, %s (0x%x)\n",
                         buff, ret );
            }
            else
            {
                for ( i = 0; i < msgcnt; i++ )
                {
                    if (msglst[i] > maxtextarray || *msgtxt[msglst[i]] == '\0')
                        printf( "<No text: msg %d> ", msglst[i] );
                    else
                        printf( "%s ", msgtxt[msglst[i]] );
                }
                printf( "\n" );
            }
        }
        vceUnloadPromptRules( prompthandle );
    }

    /* All done; close Natural Access context: */
    DemoShutdown( cx->ctahd );
    return;
}


/*****************************************************************************
                                Setup

 Synchronously opens the VCE manager.
*****************************************************************************/
void Setup( CTAQUEUEHD *ctaqueuehd,      /* returned Natural Access queue handle */
            CTAHD      *ctahd)          /* returned call resource handle   */
{
    CTA_EVENT event = { 0 };
    CTA_INIT_PARMS initparms = { 0 };

     /* ctaInitialize */
    static CTA_SERVICE_NAME Init_services[] = { { "VCE", "VCEMGR" } };

     /* ctaOpenServices */
    static CTA_SERVICE_DESC Services[] = { { { "VCE", "VCEMGR"} } };


    DemoReportComment( "Initializing and opening the call resource..." );

    initparms.size = sizeof(CTA_INIT_PARMS);
    initparms.ctacompatlevel = CTA_COMPATLEVEL;
    ctaInitialize(Init_services,
                  sizeof(Init_services)/sizeof(CTA_SERVICE_NAME),
                  &initparms);

    /* Open the Natural Access application queue with all managers */
    ctaCreateQueue(NULL, 0, ctaqueuehd);

    /* Create a call resource for handling the incoming call */
    ctaCreateContext( *ctaqueuehd, 0, "testpmpt", ctahd );

    /* Open the service manager and associated service */
    ctaOpenServices(*ctahd,
                    Services,
                    sizeof(Services)/sizeof(CTA_SERVICE_DESC));

    /* Wait for the service manager and services to be opened asynchronously */
    DemoWaitForSpecificEvent(*ctahd, CTAEVN_OPEN_SERVICES_DONE, &event);
    if (event.value != CTA_REASON_FINISHED)
    {
        DemoShowError( "Opening services failed", event.value );
        exit( 1 );
    }
}
