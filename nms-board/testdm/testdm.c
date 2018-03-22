/****************************************************************************
 *  File - testdm.c
 *
 *  Description -    This is Natural Access (NA) utility program to test an
 *                   application's ability to dictate where each Natural
 *                   Access context is created and processed. That is, either 
 *                   in the process itself ("inproc"), on the local host's 
 *                   NA server, ctdaemon, ("localhost"), or on a remote 
 *                   system's NA server ("hostname", or IP address).
 *
 * Copyright (c) 2000-1  NMS Communications Corp.  All rights reserved.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctadef.h>
#include <oamdef.h>
#include <adidef.h>
#include <vcedef.h>
#include <swidef.h>

#if defined (UNIX)
#include <strings.h>
#else
#include <string.h>
#endif

#include "ctademo.h"


/* Global Natural Access Information */

char             *svcNames[]        = { "OAM" };
char             *svcNamesLib[]     = { "ADI", "SWI", "VCE" };

CTA_SERVICE_NAME  svcList[]         = {
    { "OAM", "OAMMGR" },
    { "ADI", "ADIMGR" },
    { "SWI", "SWIMGR" },
    { "VCE", "VCEMGR" },
};
CTA_SERVICE_DESC  svcDescriptions[] = {{ { "OAM", "OAMMGR" }, {0},{0},{0} }};
CTA_SERVICE_DESC  svcDescriptionsLib[] =
{   { {"ADI", "ADIMGR"}, { 0 }, { 0 }, { 0 } },
    { {"SWI", "SWIMGR"}, { 0 }, { 0 }, { 0 } },
    { {"VCE", "VCEMGR"}, { 0 }, { 0 }, { 0 } }
};


/******************************************************************************
 *
 *   Global information about startup options
 */

#define XL_STRING  1024
#define L_STRING    256
#define M_STRING    100
#define S_STRING     50
#define XS_STRING    10

#define MAX_SERVER_SIZE  64

#ifndef UNIX
#define STRICMP stricmp
#else
#define STRICMP strcasecmp
#endif

/* Data for caller's request */
typedef struct
{
    /* Natural Access Overhead */

    CTAQUEUEHD ctaqueuehd;
    CTAQUEUEHD ctaqueuehdout;
    CTAHD      ctahd;
    char       server[ MAX_SERVER_SIZE ];    

    CTAHD      ctahdin;
    int        slotIn;
    CTAHD      ctahdout;
    int        slotOut;

    char       protocol[ 128 ];
    char       dialstring[ 64 ];

    /* Managed Object */
    HMOBJ      hObj;
    char       objName[ M_STRING  ];
    DWORD      boardNumber;

    /* Particular request */
    char       field[ M_STRING ];
    char       keyword[ M_STRING ];
    char       value[ M_STRING ];
    unsigned   bSearchText;
    unsigned   bShowQualifiers;
    DWORD      index;
    DWORD      mode;
} INFO_REQ;

typedef enum { UNKNOWN_TYPE, STR_TYPE, INT_TYPE, STRUCT_TYPE, ARRAY_TYPE, \
    STRUCT_ARR_TYPE, FILE_TYPE, OBJ_TYPE, ADDR_TYPE } KW_TYPES;

#define IS_STRING( s )  !STRICMP( s, "STRING" )
#define IS_INTEGER( s ) !STRICMP( s, "INTEGER" )
#define IS_STRUCT( s ) !STRICMP( s, "STRUCT" )
#define IS_ARRAY( s ) !STRICMP( s, "ARRAY" )
#define IS_STRUCTANDARRAY( s ) !STRICMP( s, "STRUCTANDARRAY" )


#define IS_FILENAME( s ) !STRICMP( s, "FILENAME" )
#define IS_OBJECT( s ) !STRICMP( s, "OBJECT" )
#define IS_IPADDRESS( s ) !STRICMP( s, "IPADDRESS" )


    /* global if name option selected */
    WORD name_opt = 0;

    void showNextLevel( INFO_REQ *req, char *keyword );
    void openObject( INFO_REQ *req );
    void printHelp(void);
    void onError( INFO_REQ *, char *, char *, DWORD );

/******************************************************************************
 *
 * exitFromCTA()
 *
 *   - closes Natural Access services
 *   - destroys Natural Access context
 *   - destroys Natural Access queue
 *
 ******************************************************************************/

void exitFromCTA( INFO_REQ *req )
{
    CTA_EVENT event;
    DWORD     ret;

    if ( ( ret = ctaCloseServices( req->ctahd, svcNames, sizeof svcNames / sizeof svcNames[0]) )
         != SUCCESS )
    {
        printf("Natural Access Close Service failed; returned 0x%x\n", ret );
        exit( 1 );
    }
    else if ( (ret = ctaWaitEvent( req->ctaqueuehd, &event, CTA_WAIT_FOREVER))
              != SUCCESS )
    {
        printf("Natural Access Open Service wait event returned 0x%x\n", ret );
        exit( 1 );
    }
    else if ( ( event.id != CTAEVN_CLOSE_SERVICES_DONE ) ||
              ( event.value != CTA_REASON_FINISHED ) )
    {
        exit( 1 );
    }

    /* Cleanup */
    if ( ( ret = ctaDestroyContext( req->ctahd) ) != SUCCESS )
    {
        printf("Natural Access Destroy context failed; returned 0x%x\n", ret );
        exit( 1 );
    }
    else if ( (ret = ctaWaitEvent( req->ctaqueuehd, &event, CTA_WAIT_FOREVER))
              != SUCCESS )
    {
        printf("Natural Access Destroy Context wait event returned 0x%x\n", ret );
        exit( 1 );
    }
    else if ( ( event.id != CTAEVN_DESTROY_CONTEXT_DONE ) )
        exit( 1 );

    /* Library part */
    if ( ( ret = ctaCloseServices( req->ctahdin, svcNamesLib,
                                   sizeof svcNamesLib / sizeof svcNamesLib[0]) )
         != SUCCESS )
    {
        printf("Natural Access Close Service failed; returned 0x%x\n", ret );
        exit( 1 );
    }
    else if ( (ret = ctaWaitEvent( req->ctaqueuehd, &event, CTA_WAIT_FOREVER))
              != SUCCESS )
    {
        printf("Natural Access Open Service wait event returned 0x%x\n", ret );
        exit( 1 );
    }
    else if ( ( event.id != CTAEVN_CLOSE_SERVICES_DONE ) ||
              ( event.value != CTA_REASON_FINISHED ) )
    {
        exit( 1 );
    }
    else if ( ( ret = ctaCloseServices( req->ctahdout, svcNamesLib,
                                        sizeof svcNamesLib / sizeof svcNamesLib[0]) )
              != SUCCESS )
    {
        printf("Natural Access Close Service failed; returned 0x%x\n", ret );
        exit( 1 );
    }
    else if ( (ret = ctaWaitEvent( req->ctaqueuehdout, &event, CTA_WAIT_FOREVER))
              != SUCCESS )
    {
        printf("Natural Access Open Service wait event returned 0x%x\n", ret );
        exit( 1 );
    }
    else if ( ( event.id != CTAEVN_CLOSE_SERVICES_DONE ) ||
              ( event.value != CTA_REASON_FINISHED ) )
    {
        exit( 1 );
    }

    if ( ( ret = ctaDestroyContext( req->ctahdin) ) != SUCCESS )
    {
        printf("Natural Access Destroy context failed; returned 0x%x\n", ret );
        exit( 1 );
    }
    else if ( (ret = ctaWaitEvent( req->ctaqueuehd, &event, CTA_WAIT_FOREVER))
              != SUCCESS )
    {
        printf("Natural Access Destroy Context wait event returned 0x%x\n", ret );
        exit( 1 );
    }
    else if ( ( event.id != CTAEVN_DESTROY_CONTEXT_DONE ) )
    {
        exit( 1 );
    }
    else if ( ( ret = ctaDestroyContext( req->ctahdout) ) != SUCCESS )
    {
        printf("Natural Access Destroy context failed; returned 0x%x\n", ret );
        exit( 1 );
    }
    else if ( (ret = ctaWaitEvent( req->ctaqueuehdout, &event, CTA_WAIT_FOREVER))
              != SUCCESS )
    {
        printf("Natural Access Destroy Context wait event returned 0x%x\n", ret );
        exit( 1 );
    }
    else if ( ( event.id != CTAEVN_DESTROY_CONTEXT_DONE ) )
    {
        exit( 1 );
    }

    /* Destroy queue */
    if ( (ret = ctaDestroyQueue( req->ctaqueuehd )) != SUCCESS )
    {
        printf("Natural Access Destroy Queue failed; returned 0x%x\n", ret );
        exit( 1 );
    }
    /* Destroy queue */
    if ( (ret = ctaDestroyQueue( req->ctaqueuehdout )) != SUCCESS )
    {
        printf("Natural Access Destroy Queue failed; returned 0x%x\n", ret );
        exit( 1 );
    }
}

/*******************************************************************************
 *
 *  onError
 *
 *   Show error information and exit.
 *
 *
 *******************************************************************************/
void onError( INFO_REQ *req, char *funcName, char *extra, DWORD ret )
{
    char buf[ L_STRING ];

    ctaGetText( req->ctahd, ret, buf, L_STRING );
    if ( extra )
        printf("%s: error 0x%x: %s for %s\n", funcName, ret, buf, extra);
    else
        printf("%s: error 0x%x: %s\n", funcName, ret, buf );

    /* Clean up OAM resources */
    if ( req->hObj != NULL_CTAHD )
        oamCloseObject( req->hObj );

    exitFromCTA( req );
    free( req );

    exit( 1 );
}

/*******************************************************************************
 *
 *  printHelp
 *
 *******************************************************************************/

void printHelp()
{
    printf( "\n         Natural Access utility program to show remote server mode.");
    printf( "\nUsage: "
            "testdm \n");

    printf( "  -@ server\tServer host name or IP address, default = localhost.\n" );

    printf( "  -b num\tBoard number; may be used instead of -n option. "
            "Default = 0\n" );

    printf( "  -p protocol\tProtocol to run. "
            "Default = nocc\n" );

    printf( "  -d number\tNumber to dial. "
            "Default = 555\n" );

    printf( "  -m in_slot,\tInbound and outgoing TDM timeslots. "
            "Default = 0,2\n" );
    printf( "     out_slot\n");

    printf( "  -n name\tName of managed object to find information about.\n");

    printf( "  -k keyword\tKeyword name. "
            "If unspecified, shows all keywords.\n" );

    printf( "  -f field\tDesired subfield within Keyword name.\n");

    printf( "  -i num  \tArray index of keyword name. "
            "Default = all\n" );

    printf( "  -q      \tShow keyword qualifiers. \n");

    printf( "  -s text\tDisplay all keywords containing specified text. \n");

    printf("\n");
}

/*******************************************************************************
 *
 *  cmdLineOptions
 *
 *   Analyse calling parameters.
 *
 *******************************************************************************/

INFO_REQ *cmdLineOptions(int argc, char **argv)
{
    int         c;
    INFO_REQ    *req = NULL;

    if ( req = malloc( sizeof( INFO_REQ ) ) )
    {
        memset( req, 0, sizeof( INFO_REQ ) );
        req->mode = OAM_READONLY;
        req->index = -1;
        req->hObj = NULL_CTAHD;
        strcpy(req->protocol, "nocc");
        strcpy(req->dialstring, "555");
        req->slotIn = 0;
        req->slotOut = 2;

        while( ( c = getopt( argc, argv, "@:s:v:f:i:n:b:p:m:d:k:qHh?" ) ) != -1 )
        {
            switch( c )
            {
                case '@':   /* server host name */
                    if (strlen(optarg) >= MAX_SERVER_SIZE)
                    {
                        printf( "-%c argument is too long.\n", c );
                        exit(1);
                    }
                    strcpy( req->server, optarg );
                    break;

                case 'b':
                    sscanf( optarg, "%d", &req->boardNumber );
                    break;

                case 'p':
                    sscanf( optarg, "%s", req->protocol );
                    break;

                case 'd':
                    sscanf( optarg, "%s", req->dialstring );
                    break;

                case 'm':
                    if( sscanf( optarg, "%d,%d", &req->slotIn, &req->slotOut ) != 2 )
                    {
                        printHelp();
                        exit(1);
                    }
                    break;

                case 'f':
                    sscanf( optarg, "%s", req->field );
                    break;

                case 'i':
                    sscanf( optarg, "%d", &req->index );
                    break;

                case 'k':
                    sscanf( optarg, "%s", req->keyword );
                    break;

                case 'n':
                    name_opt = 1;
                    strncpy( req->objName, optarg, (sizeof( req->objName ) - 1));
                    break;

                case 'q':

                    req->bShowQualifiers = 1;
                    break;

                case 's':

                    req->bSearchText = 1;
                    sscanf( optarg, "%s", req->value );
                    break;

                case 'h':
                case 'H':
                case '?':
                default:

                    printHelp();
                    exit(1);
            }
        }

        if (optind < argc)
        {
            printHelp();
            exit(1);
        }
    }
    else
    {
        printf("Fatal Error: no memory\n");
        exit(1);
    }
    return req;
}


/******************************************************************************
 *
 * showEvent()
 *
 ******************************************************************************/
void showEvent(CTA_EVENT *e)
{
    char buffer[1024];

    if( ctaFormatEvent( "\t", e, buffer, sizeof buffer ) == SUCCESS )
    {
        printf( "%s", buffer );
    }
}

/******************************************************************************
 *
 * openPort()
 *
 *   - creates a Natural Access context
 *   - opens Natural Access services and waits for completion
 *
 ******************************************************************************/
void openPort( CTAQUEUEHD queuehd, CTA_SERVICE_DESC svcList[],
              int numSvcs, char *ctxName, CTAHD *ctahd, char *server )
{
    DWORD ret;
    CTA_EVENT event;

    if( (ret = ctaCreateContext( queuehd, 0, ctxName,
                                 ctahd )) != SUCCESS )
    {
        printf("Unable to create context - returns 0x%x\n", ret);
        if ( ret == CTAERR_SVR_COMM )
            printf(*server ? 
                 "Verify that NA Server (ctdaemon) is running on %s.\n"
               : "Verify that NA Server (ctdaemon) is running.\n",
               server);
           
        exit( 1 );
    }

    if( (ret = ctaOpenServices( *ctahd, svcList,  numSvcs )) != SUCCESS )
    {
        printf("Unable to open services - returns 0x%x\n", ret);
        exit( 1 );
    }

    if( (ret = ctaWaitEvent( queuehd, &event, CTA_WAIT_FOREVER))
        != SUCCESS )
    {
        printf("Natural Access Open Service wait event returned 0x%x\n", ret );
        exit( 1 );
    }
    else if ( ( event.id != CTAEVN_OPEN_SERVICES_DONE ) ||
              ( event.value != CTA_REASON_FINISHED ) )
    {
        printf( "Failed to open services\n" );
        showEvent( &event );
        exit( 1 );
    }
}

/******************************************************************************
 *
 * initializeWithCTA()
 *
 *   - invoke ctaInitialize
 *   - create a Natural Access queue
 *   - create a Natural Access context
 *   - open the OAM service on the context
 *
 ******************************************************************************/

void initializeWithCTA( INFO_REQ *req )
{
    DWORD           ret;
    DWORD           board_num;
    CTA_INIT_PARMS  initparms = { 0 };
    char            ctxWithOam[ CTA_CONTEXT_NAME_LEN + MAX_SERVER_SIZE + 1 ];
    char            noServer[]= "";

    /* Initialize Natural Access. Applications contexts which use the OAM
    *  service must be run with the NA server, ctdaemon. Other contexts can be
    *  run in process.
    */
    initparms.size = sizeof(CTA_INIT_PARMS);
    initparms.parmflags  = 0;
    initparms.ctacompatlevel = CTA_COMPATLEVEL;
 
    if( (ret = ctaInitialize(svcList, sizeof svcList / sizeof svcList[0], &initparms) )
        != SUCCESS )
    {
        printf("Unable to Initialize with Natural Access - returns 0x%x\n", ret);
        if ( ret == CTAERR_SVR_COMM )
            printf("Check that ctdaemon is running.\n");

        exit( 1 );
    }
    else if (( ret = ctaCreateQueue( NULL, 0, &req->ctaqueuehd ))
             != SUCCESS )
    {
        printf("Unable to create Natural Access queue - returns 0x%x\n", ret);
        exit( 1 );
    }
    else if (( ret = ctaCreateQueue( NULL, 0, &req->ctaqueuehdout ))
             != SUCCESS )
    {
        printf("Unable to create Natural Access queue - returns 0x%x\n", ret);
        exit( 1 );
    }

    /* If user has specified a hostname, use it. Otherwise, indicate that the
    *  OAM context should be created on the local host's ctdaemon.
    */
    if ( req->server[ 0 ] != '\0' )
    {
        strcpy( ctxWithOam, req->server );
        strcat( ctxWithOam, "/" );
    }
    else
    {
        /* Use the default local host designation, "localhost" */
        strcpy( ctxWithOam, "localhost/" );
    }
    strcat( ctxWithOam, "testdm" );

    openPort( req->ctaqueuehd, svcDescriptions,
              sizeof svcDescriptions / sizeof svcDescriptions[0],
              ctxWithOam, &req->ctahd, req->server );

    /* Initialize library part */
    if(name_opt == 1)
    {
        name_opt = 0;
        ret = oamBoardGetNumber(req->ctahd, req->objName, &board_num);
        if(ret != SUCCESS)
        {
            onError( req, "oamBoardGetNumber", "", ret );
        }
        else
            req->boardNumber = board_num;
    }

    /* AG board number                  */
    svcDescriptionsLib[0].mvipaddr.board = req->boardNumber;
    /* MVIP stream                      */
    svcDescriptionsLib[0].mvipaddr.stream = 0;
    /* MVIP time slot                   */
    svcDescriptionsLib[0].mvipaddr.timeslot = req->slotIn;
    /* ADI_FULL_DUPLEX or ADI_VOICE_DUPLEX*/
    svcDescriptionsLib[0].mvipaddr.mode = ADI_FULL_DUPLEX;

    /* Open the voice input and output port contexts in process, i.e.,
    *  do not use the NA server.
    */
    openPort( req->ctaqueuehd, svcDescriptionsLib,
              sizeof svcDescriptionsLib / sizeof svcDescriptionsLib[0],
              "inproc/dmin", &req->ctahdin, noServer );

    svcDescriptionsLib[0].mvipaddr.timeslot = req->slotOut;

    openPort( req->ctaqueuehdout, svcDescriptionsLib,
              sizeof svcDescriptionsLib / sizeof svcDescriptionsLib[0],
              "inproc/dmout", &req->ctahdout, noServer );
}


/******************************************************************************
 *
 * isSubstring
 *
 *   Perform a case insenstive substring search.
 *
 ******************************************************************************/
char *isSubstring( char *pStr, char *pSubStr )
{
    char s[ L_STRING ];
    char ss[ L_STRING ];

    strcpy( s, pStr );
    strcpy( ss, pSubStr );

    /* Convert both main string and substring to same case */
#if defined (WIN32)
    strupr( s );
    strupr( ss );
#else
    {
        char *ps = s;
        char *pss = ss;

        for( ; *ps; ps++ )
            *ps = (char) toupper( *ps );

        for( ; *pss; pss++ )
            *pss = (char) toupper( *pss );
    }
#endif

    return( strstr( s, ss ) );
}

/*******************************************************************************
 *
 *  getType
 *
 *       Determine type of keyword
 *
 *******************************************************************************/

KW_TYPES getType( INFO_REQ *req, char *keyword )
{
    KW_TYPES ret = UNKNOWN_TYPE;
    char     info[ S_STRING ];

    ret = oamGetQualifier( req->hObj, keyword, "Type",
                           info, sizeof(info) );

    if ( ret != SUCCESS )
    {
        ret = STR_TYPE;
    }
    else if ( IS_STRING( info ) )                       ret = STR_TYPE;
    else if ( IS_INTEGER( info ) )                      ret = INT_TYPE;
    else if ( IS_STRUCT( info ) )                       ret = STRUCT_TYPE;
    else if ( IS_ARRAY( info ) )                        ret = ARRAY_TYPE;
    else if ( IS_STRUCTANDARRAY( info ) )               ret = STRUCT_ARR_TYPE;
    else if ( IS_FILENAME( info ) )                     ret = FILE_TYPE;
    else if ( IS_OBJECT( info ) )                       ret = OBJ_TYPE;
    else if ( IS_IPADDRESS( info ) )                    ret = ADDR_TYPE;
    else                                                ret = UNKNOWN_TYPE;

    return ret;
}

/*******************************************************************************
 *
 *  getCount
 *
 *       Returns number of entries in an ARRAY.
 *
 *******************************************************************************/
DWORD getCount( INFO_REQ *req, char *keyword )
{
    char  kwCount[ L_STRING ];
    char  info[ XS_STRING ];
    char *pChar;
    DWORD ret;
    DWORD count = 0, len;

    if ( (len = strlen( keyword )) >= sizeof(kwCount) )
    {
        onError( req, "getCount; string length", keyword, CTAERR_INTERNAL_ERROR );
    }
    else
    {
        strcpy( kwCount, keyword );
        /* Determine if keyword contains array index specification. If it does,
         *  strip it off prior to getting the array's current count.
         */
        if ( kwCount[ len - 1 ] == ']' )
        {
            if ( pChar = strrchr( kwCount, '[' ) )
            {
                /* Terminate the string at the '[' and append the ".Count"
                 *  special array keyword.
                 */
                *pChar = '\0';
                strcat( kwCount, ".Count" );
            }
        }
        else
            sprintf( kwCount, "%s.Count", keyword );

        ret = oamGetKeyword( req->hObj, kwCount, info, sizeof(info) );
        if ( ret == SUCCESS )
            sscanf( info, "%d", &count );
    }
    return count;
}

/*******************************************************************************
 *
 *  showCommonQualifiers
 *
 *   Displays qualifiers common to both INTERGER and STRING type keywords.
 *
 ******************************************************************************/

void showCommonQualifiers( INFO_REQ *req, char *keyword )
{
    DWORD ret = SUCCESS;
    char  info[ L_STRING ];
    char  Choices[ S_STRING ];
    DWORD howMany, i;

    ret = oamGetQualifier( req->hObj, keyword, "ReadOnly", info, sizeof(info) );
    if ( ret == SUCCESS )
    {
        if ( isSubstring( info, "Yes" ) )
            printf("\tRead-only\n");
        else
            printf("\tRead-Write\n");
    }
    else
        onError( req, "showCommonQualifiers", "ReadOnly", ret );

    ret = oamGetQualifier( req->hObj, keyword,"Description", info, sizeof(info) );
    if ( ret == SUCCESS )
        printf("\tDescription: %s\n", info);
    else
        onError( req, "showCommonQualifiers", "Description", ret );

    /* How many different values can this keyword contain */
    ret = oamGetQualifier( req->hObj, keyword, "Choices.Count", info, sizeof( info ) );
    if ( ret == SUCCESS )
    {
        sscanf( info, "%d", &howMany );
        if ( howMany != 0 )
        {
            printf("\tThere are %d possible values:\n", howMany);
            for ( i = 0; i < howMany; i++ )
            {
                sprintf( Choices, "Choices[%d]", i );
                ret = oamGetQualifier( req->hObj, keyword, Choices, info,
                                       sizeof( info ) );
                if ( ret == SUCCESS )
                {
                    printf("\t\t%s\n", info );
                }
            }
        }
    }
    else
        onError( req, "showCommonQualifiers", "Choices.Count", ret );

}
/*******************************************************************************
 *
 *  showIntegerQualifiers
 *
 *   Displays qualifiers applicable to INTEGER type keywords.
 *
 ******************************************************************************/

void showIntegerQualifiers( INFO_REQ *req, char *keyword )
{
    DWORD    ret = SUCCESS;
    unsigned base = 10;
    char     info[ S_STRING ];

    printf("\tINTEGER Type\n");

    showCommonQualifiers( req, keyword );

    ret = oamGetQualifier( req->hObj, keyword, "Base", info, sizeof(info) );
    if ( ret != SUCCESS )
    {
        onError( req, "showIntegerQualifiers", "Base", ret );
    }
    else
    {
        printf("\tBase %s\n", info );
        sscanf( info, "%d", &base );
    }
    ret = oamGetQualifier( req->hObj, keyword, "Min", info, sizeof(info) );
    if ( ret != SUCCESS )
        onError( req, "showIntegerQualifiers", "Min", ret );
    else
    {
        if ( base == 10 )
            printf("\tMin value: %s;\t", info );
        else if ( base == 16 )
            printf("\tMin value: 0x%s;\t", info );
        else
            printf("\tMin value: 0%s;\t", info );
    }
    ret = oamGetQualifier( req->hObj, keyword, "Max", info, sizeof(info) );
    if ( ret != SUCCESS )
        onError( req, "showIntegerQualifiers", "Min", ret );
    else
    {
        if ( base == 10 )
            printf("Max value: %s \n", info );
        else if ( base == 16 )
            printf("Max value: 0x%s \n", info );
        else
            printf("Max value: 0%s \n", info );
    }
    printf("\n");
}

/*******************************************************************************
 *
 *  showStringQualifiers
 *
 *   Displays qualifiers applicable to STRING type keywords.
 *
 ******************************************************************************/

void showStringQualifiers( INFO_REQ *req, char *keyword )
{
    DWORD ret = SUCCESS;

    printf("\tSTRING Type\n");
    showCommonQualifiers( req, keyword );
    printf("\n");
}

/*******************************************************************************
 *
 *  showInteger
 *
 *   The keyword is of type integer. Show keyword=value pair.
 *   Display applicable keyword qualifiers.
 *
 *******************************************************************************/

void showInteger( INFO_REQ *req, char *keyword )
{
    DWORD    ret;
    char     info[ S_STRING ];

    if ( req->bSearchText )
    {
        /* Caller interested in only seeing keywords containing a particular
         *  text substring.
         */
        if ( !isSubstring( keyword, req->value ) )
            return;
    }

    ret = oamGetKeyword( req->hObj, keyword, info, sizeof( info ) );
    if ( (ret == SUCCESS)  )
    {
        printf("%s = %s\n", keyword, info );

        if ( req->bShowQualifiers )
        {
            showIntegerQualifiers( req, keyword );
        }
    }
    else
    {
        onError( req, "oamGetKeyword", keyword, ret );
    }
}


/*******************************************************************************
 *
 *  showBasic
 *
 *   The keyword is of a basic string type. Show keyword=value pair.
 *   Display applicable keyword qualifiers.
 *
 *******************************************************************************/

void showBasic( INFO_REQ *req, char *keyword, KW_TYPES type )
{
    char  info[ L_STRING ];

    DWORD ret = SUCCESS;

    if ( req->bSearchText )
    {
        /* Caller interested in only seeing keywords containing a particular
         *  text substring.
         */
        if ( !isSubstring( keyword, req->value ) )

            return;
    }

    ret = oamGetKeyword( req->hObj, keyword, info, sizeof( info ) );
    if ( ret == SUCCESS )
    {
        if ( info[0] != '\0' )
            printf("%s = %s\n", keyword, info );
        else
            printf("%s = <empty>\n", keyword );

        if ( req->bShowQualifiers )
        {
            switch( type )
            {
                case STR_TYPE:
                    printf("\tSTRING Type\n");
                    break;

                case FILE_TYPE:
                    printf("\tFILE Type\n");
                    break;

                case ADDR_TYPE:
                    printf("\tADDRESS Type\n");
                    break;

                case OBJ_TYPE:
                    printf("\tOAM Managed Object Type\n");
                    break;

                default:
                    break;
            }
            showCommonQualifiers( req, keyword );
            printf("\n");
        }
    }
    else
    {
        onError( req, "oamGetKeyword", keyword, ret );
    }
}

/*******************************************************************************
 *
 *  showStruct
 *
 *   The keyword is of type STRUCT, i.e.,
 *
 *       xxx.Keywords.Count = n
 *       xxx.field_0
 *       xxx.field_1
 *       ...
 *       xxx.field_n-1
 *
 *   To access "field_x" within the STRUCT keyword xxx is through the
 *   qualifier "Keywords[x]" of the keyword "xxx", i.e.,
 *
 *       char buf[]
 *
 *       oamGetQualifier ( ..., "xxx", "Keywords[1]", buf,... )
 *
 *       returns the string "xxx.field1" in buf.
 *
 *   where the "Type" of "xxx.field1" will determine how it is to be
 *   subsequently processed.
 *
 *   Note: A structure name of "" allows access to all keywords
 *         of an opened object.
 *
 *******************************************************************************/

void showStruct( INFO_REQ *req, char *structName )
{
    DWORD ret;
    char  info[ L_STRING ];
    char  Keywords[ S_STRING ];
    DWORD howMany, i;

    /* It's a structure. Will need to get each of it's keywords.
     *  How many keywords are there?
     */
    ret = oamGetQualifier( req->hObj, structName, "Keywords.Count", info,
                           sizeof( info ) );
    if ( ret == SUCCESS )
    {
        sscanf( info, "%d", &howMany );
        for ( i = 0; i < howMany ; i++ )
        {
            /* Retrieve each keyword within the structure */
            sprintf( Keywords, "Keywords[%d]", i );

            ret = oamGetQualifier( req->hObj, structName,
                                   Keywords, info, sizeof( info ) );
            if ( ret == SUCCESS )
            {
                showNextLevel( req, info );
            }
            else
            {
                char err[ L_STRING ];
                sprintf( err, "%s's %s", structName, Keywords );
                onError( req, "oamGetQualifier",  err, ret );
            }
        }
    }
    else
    {
        sprintf( info, "%s's %s", structName, "Keywords.Count");
        onError( req, "oamGetQualifier", info, ret );
    }
}

/*******************************************************************************
 *
 *  showArray
 *
 *   The keyword is of type ARRAY, i.e.,
 *
 *       xxx.Count = n
 *       xxx[ 0 ]
 *       ...
 *       xxx[ n-1 ]
 *
 *  where the "Type" of "xxx[ 0 ]" will determine how it is to be subsequently
 *  processed.
 *
 *******************************************************************************/

void showArray( INFO_REQ *req, char *BaseKw )
{
    char kwCount[ L_STRING ];
    char nextBase[ L_STRING ];
    char info[ S_STRING ];
    DWORD i, count = 0;
    DWORD ret = SUCCESS;

    /* it's an array, need to get it's count */
    sprintf( kwCount, "%s.Count", BaseKw );
    if ( ( ret = oamGetKeyword( req->hObj, kwCount, info,
                                sizeof ( info ) ) ) == SUCCESS )
        sscanf( info, "%d", &count );
    else
        onError( req, "oamGetKeyword", kwCount, ret );

    if ( count )
    {
        for ( i = 0 ; i < count ;  i++ )
        {
            /* What is it an array of? */
            if ( req->index == -1 || STRICMP( BaseKw, req->keyword ) )
            {
                sprintf( nextBase, "%s[%d]", BaseKw, i );
                showNextLevel( req, nextBase );
            }
            else if ( !STRICMP( BaseKw, req->keyword ) && ( i == req->index ) )
            {
                sprintf( nextBase, "%s[%d]", BaseKw, i );
                showNextLevel( req, nextBase );
            }
        }
    }
    else
    {
        if ( req->bSearchText )
        {
            /* Caller interested in only seeing keywords containing a particular
             *  text substring.
             */
            if ( !isSubstring( BaseKw, req->value ) )
                return;
        }

        printf("%s[] -- <empty array> \n", BaseKw );
        if ( req->bShowQualifiers )
        {
            KW_TYPES type;

            sprintf( nextBase, "%s[]", BaseKw );
            type = getType( req, nextBase );
            if ( type == STR_TYPE )
                showStringQualifiers( req, nextBase );
            else if ( type == INT_TYPE )
                showIntegerQualifiers( req, nextBase );
            else
                showCommonQualifiers( req, nextBase );
        }
    }
}

/*******************************************************************************
 *
 *  showStructArray
 *
 *   The keyword is if type STRUCTANDARRAY. As a result, there is both
 *   a STRUCT portion and an ARRAY portion. Display each independently.
 *
 *******************************************************************************/

void showStructArray( INFO_REQ *req, char *keyword )
{
    /* Display components of the structure which are common to
     *  all elements of the array.
     */
    showStruct( req, keyword );

    /* Display all elements of the array.
     */
    showArray( req, keyword );
}

/*******************************************************************************
 *
 *  showNextLevel
 *
 *   The type of keyword will determine how next to process the keyword.
 *
 *******************************************************************************/

void showNextLevel( INFO_REQ *req, char *keyword )
{
    KW_TYPES type;

    type = getType( req, keyword );
    switch( type )
    {
        case STR_TYPE:
        case FILE_TYPE:
        case ADDR_TYPE:
        case OBJ_TYPE:

            showBasic( req, keyword, type );
            break;

        case INT_TYPE:

            showInteger( req, keyword );
            break;

        case STRUCT_TYPE:

            showStruct( req, keyword );
            break;

        case ARRAY_TYPE:

            showArray( req, keyword );
            break;

        case STRUCT_ARR_TYPE:

            showStructArray( req, keyword );
            break;

        case UNKNOWN_TYPE:
        default:

            printf("%s - Type of keyword unknown\n", keyword );
            break;
    }
}

/*******************************************************************************
 *
 *  doQuery
 *
 *   Display information about keywords.
 *
 *******************************************************************************/

void doQuery( INFO_REQ *req )
{
    /* Open the OAM managed object; this function will not return on failure */
    openObject( req );

    if ( req->keyword[0] != '\0' )
    {
        showNextLevel( req, req->keyword );
    }
    else
    {
        /* Enumerate all Keywords */
        showNextLevel( req, "" );
    }
    if ( req->hObj != NULL_CTAHD )
        oamCloseObject( req->hObj );
}


/*******************************************************************************
 *
 *  openObject
 *
 *       This function opens an OAM managed object.
 *
 *       If the caller has provided a name, this is used for doing the open.
 *       If instead a board number is provided, this number is used to lookup
 *       the board's name prior to performing the open.
 *
 *******************************************************************************/

void openObject( INFO_REQ *req )
{
    DWORD ret = SUCCESS;


    if ( req->objName[0] == '\0' )
    {
        /* Don't have a name; need to find it in the database. */
        ret = oamBoardLookupByNumber( req->ctahd, req->boardNumber,
                                      req->objName, sizeof(req->objName) );
        if ( ret != SUCCESS )
        {
            onError( req, "oamBoardLookupByNumber", "", ret );
        }
    }

    /* Get a handle to the managed object */
    if ( ( ret = oamOpenObject( req->ctahd, req->objName, &req->hObj,
                                req->mode ) )
         != SUCCESS )
    {
        req->hObj = NULL_CTAHD;
        onError( req, "oamOpenObject", req->objName, ret );
    }
}


/******************************************************************************
 *
 * InboundThread()
 *
 ******************************************************************************/
INT32 g_inRet = FAILURE;

THREAD_RET_TYPE THREAD_CALLING_CONVENTION InboundThread( void *arg )
{
    CTA_EVENT event;
    INFO_REQ *req = (INFO_REQ *) arg;

    printf( "Waiting for call in slot %d...\n", req->slotIn );

    DemoWaitForCall( req->ctahdin, NULL, NULL );
    printf( "\t(in slot %d)\n", req->slotIn );

    if ( DemoAnswerCall ( req->ctahdin, 1 ) == SUCCESS )
    {
        /* Play message to the caller. */
        adiFlushDigitQueue( req->ctahdin );
        if ( DemoPlayMessage( req->ctahdin, "ctademo", PROMPT_WELCOME,
                              0, NULL ) == SUCCESS )
        {
            DemoWaitForSpecificEvent( req->ctahdin, ADIEVN_CALL_DISCONNECTED, &event );
            g_inRet = (DemoHangUp( req->ctahdin ) == DISCONNECT) ? SUCCESS : FAILURE;
        }
    }

    THREAD_RETURN;
}


/******************************************************************************
 *
 * OutgoingThread()
 *
 ******************************************************************************/
DWORD OutgoingThread( INFO_REQ *req )
{
    DWORD ret = (DWORD) FAILURE;

    DemoSleep(2);

    printf( "Placing a call in slot %d...\n", req->slotOut );

    if ( DemoPlaceCall( req->ctahdout, req->dialstring, NULL ) == SUCCESS )
    {
        /* Play a message. */
        adiFlushDigitQueue( req->ctahdout );
        if ( DemoPlayMessage( req->ctahdout, "ctademo", PROMPT_WELCOME, 0, NULL ) == SUCCESS )
        {
            ret = (DemoHangUp( req->ctahdout ) == DISCONNECT) ? SUCCESS : FAILURE;
        }
    }

    DemoSleep(5);

    return ret;
}


/******************************************************************************
 *
 * doMultiModeTest()
 *
 ******************************************************************************/
void doMultiModeTest( INFO_REQ *req )
{
    DWORD ret;

    DemoStartProtocol( req->ctahdin, req->protocol, NULL, NULL );
    DemoStartProtocol( req->ctahdout, req->protocol, NULL, NULL );

    DemoSpawnThread( InboundThread, (void *) req );
    ret = OutgoingThread( req );

    if ( ret == SUCCESS && g_inRet == SUCCESS )
        printf( "Test passed\n" );
    else
        printf( "Test failed\n" );
}

/******************************************************************************
 *
 * main()
 *
 ******************************************************************************/

int main( int argc, char **argv )
{
    INFO_REQ *req;

    /* Determine what caller wants us to do. */
    req = cmdLineOptions( argc, argv);

    if ( req )
    {
        /* Initialize with Natural Access */
        initializeWithCTA( req );

        /* Go look at the OAM configuration database */
        doQuery( req );

        /* Test Multi Mode operation */
        doMultiModeTest( req );

        /* nothing else needs to be done... */
        exitFromCTA( req );
    }

    exit( 0 );
}

