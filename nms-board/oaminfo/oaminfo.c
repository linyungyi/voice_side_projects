/****************************************************************************
*  File - oaminfo.c
*
*  Description -    Natural Access utility program to demonstrate the use of the
*                   OAM Service API and provide information on system
*                   configuration keywords.
*
* Copyright (c) 2000  NMS Communications Corp.  All rights reserved.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctadef.h>
#include <oamdef.h>
#include <string.h>
#include <ctype.h>

#if defined (UNIX)
#include <strings.h>
#endif

/* Global Natural Access Information */


char             *svcMgrList[]      = { "OAMMGR" };
char             *svcNames[]        = { "OAM" };

CTA_SERVICE_NAME  svcList[]         = {{ "OAM", "OAMMGR" }};
CTA_SERVICE_DESC  svcDescriptions[] = {{ { "OAM", "OAMMGR" }, {0},{0},{0} }};

unsigned          gNumSvcs = 1;



/******************************************************************************
*
*   Global information about startup options
*/

#define L_STRING    2048
#define M_STRING    100
#define S_STRING     50
#define XS_STRING    10
#define MAX_SERVER_SIZE     64

#ifndef UNIX
#define STRICMP stricmp
#else
#define STRICMP strcasecmp
#endif

/* Data for caller's request */
typedef struct
{
    /* Natural Access Overhead */
    char       szServer[ MAX_SERVER_SIZE ];
    CTAQUEUEHD ctaqueuehd;
    CTAHD      ctahd;

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
    unsigned   bSetValue;
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


void printHelp(void);
INFO_REQ *cmdLineOptions(int argc, char **argv);
void initializeWithCTA( INFO_REQ *req );
void exitFromCTA( INFO_REQ *req );
int isField( char *pKW, char *pField );
char *isSubstring( char *pStr, char *pSubStr );
void onError( INFO_REQ *req, char *funcName, char *extra, DWORD ret );
KW_TYPES getType( INFO_REQ *req, char *keyword );
DWORD getCount( INFO_REQ *req, char *keyword );
void showCommonQualifiers( INFO_REQ *req, char *keyword );
void showIntegerQualifiers( INFO_REQ *req, char *keyword );
void showStringQualifiers( INFO_REQ *req, char *keyword );
void showInteger( INFO_REQ *req, char *keyword );
void showBasic( INFO_REQ *req, char *keyword, KW_TYPES type );
void showStruct( INFO_REQ *req, char *structName );
void showArray( INFO_REQ *req, char *BaseKw );
void showStructArray( INFO_REQ *req, char *keyword );
void showNextLevel( INFO_REQ *req, char *keyword );
void setArray( INFO_REQ *req, char *keyword, KW_TYPES type );
void doSet( INFO_REQ *req );
void doQuery( INFO_REQ *req );
void openObject( INFO_REQ *req );


/*****************************************************************************
                                getopt

 This routine is based on a UNIX standard for parsing options in command
 lines, and is provided because some system libraries do not include it.
 'argc' and 'argv' are passed repeatedly to the function, which returns
 the next option letter in 'argv' that matches a letter in 'optstring'.

 'optstring' must contain the option letters the command will recognize;
 if a letter in 'optstring' is followed by a colon, the option is expected
 to have an argument, or group of arguments, which must be separated from
 it by white space.  Option arguments cannot be optional.

 'optarg' points to the start of the option-argument on return from getopt.

 getopt places in 'optind' the 'argv' index of the next argument to be
 processed.  'optind' has an initial value of 1.

 When all options have been processed (i.e., up to the first non-option
 argument, or the special option '--'), getopt returns -1.  All options
 must precede other operands on the command line.

 Unless 'opterr' is set to 0, getopt prints an error message on stderr
 and returns '?' if it encounters an option letter not included in
 'optstring', or no option-argument after an option that expects one.
*****************************************************************************/

#if defined (LINUX)
#include <getopt.h>
#elif ! defined (UNIX)

char *optarg = NULL;
int optind = 1;
int opterr = 1;

int getopt( int argc, char * const *argv, const char *optstring )
{
    static int   multi = 0;  /* remember that we are in the middle of a group */
    static char *pi = NULL;  /* .. and where we are in the group */

    char *po;
    int c;

    if (optind >= argc)
        return -1;

    if (!multi)
    {
        pi = argv[optind];
        if (*pi != '-' && *pi != '/')
            return -1;
        else
        {
            c = *(++pi);

            /* test for null option - usually means use stdin
             * but this is treated as an arg, not an option
             */
            if (c == '\0')
                return -1;
            else if (c == '-')
            {
                /*/ '--' terminates options */
                ++ optind;
                return -1;
            }
        }

    }
    else
        c = *pi;

    ++pi;

    if (! (isalnum (c) || (c == '@'))
        || (po=strchr( optstring, c )) == NULL)
    {
        if (opterr && c != '?')
            fprintf (stderr, "%s: illegal option -- %c\n", argv[0], c);
        c = '?';
    }
    else if (*(po+1) == ':')
    {
        /* opt-arg required.
         * Note that the standard requires white space, but current
         * practice has it optional.
         */
        if (*pi != '\0')
        {
            optarg = pi;
            multi = 0;
            ++ optind;
        }
        else
        {
            multi = 0;
            if (++optind >= argc)
            {
                if (opterr)
                    fprintf (stderr,
                        "%s: option requires an argument -- %c\n", argv[0],c);
                return '?';
            }
            optarg = argv[optind++];
        }
        return c;
    }

    if (*pi == '\0')
    {
        multi = 0;
        ++ optind;
    }
    else
    {
        multi = 1;
    }
    return c;
}

#endif

/*******************************************************************************
*
*  printHelp
*
*******************************************************************************/

void printHelp()
{
    printf( "OAM utility program to get and set keywords.\n");
    printf( "\nUsage: "
            "oaminfo \n");

    printf( "  -@ server\tServer host name or IP address, default = localhost.\n" );

    printf( "  -b num\tOAM Board number. "
            "Default = 0\n" );

    printf( "  -n name\tName of managed object to find information about.\n");

    printf( "  -k keyword\tKeyword name. "
            "If unspecified, shows all keywords.\n" );

    printf( "  -f field\tDesired subfield within Keyword name.\n");

    printf( "  -i num  \tArray index of keyword name. "
            "Default = all\n" );

    printf( "  -v value\tSet keyword name to value. \n");

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

        while( ( c = getopt( argc, argv, "@:s:v:f:i:n:b:k:qHh?" ) ) != -1 )
        {
            switch( c )
            {
            case '@':   // server host name
                if (strlen(optarg) >= MAX_SERVER_SIZE)
                {
                    printf( "-%c argument is too long.\n", c );
                    exit(1);
                }
                strcpy( req->szServer, optarg );
                break;

            case 'b':
                sscanf( optarg, "%d", &req->boardNumber );
                break;

            case 'f':
                strncpy( req->field, optarg, (sizeof( req->field ) - 1));
                break;

            case 'i':
                sscanf( optarg, "%d", &req->index );
                break;

            case 'k':
                strncpy( req->keyword, optarg, (sizeof( req->keyword ) - 1));
                break;

            case 'n':

                strncpy( req->objName, optarg, (sizeof( req->objName ) - 1));
                break;

            case 'q':

                req->bShowQualifiers = 1;
                break;

            case 's':

                req->bSearchText = 1;
                strncpy( req->value, optarg, (sizeof( req->value ) - 1));
                break;

            case 'v':

                req->mode = OAM_READWRITE;
                req->bSetValue = 1;
                strncpy( req->value, optarg, (sizeof( req->value ) - 1));
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
    CTA_EVENT       event;
    DWORD           ret;
    CTA_INIT_PARMS  initparms = { 0 };
    const char szDefaultDescriptor[] = "oaminfo";
    char szDescriptor[ MAX_SERVER_SIZE + sizeof(szDefaultDescriptor) ] = "";

    /* Initialize Natural Access. Applications which interface with the OAM
    *  service must be run in Natural Access Server mode. Therefore, override
    *  the system default mode specified in cta.cfg and instead specifically
    *  indicate that this application is to run in server mode, i.e.,
    *  set initparms.ctaflags.
    */
    initparms.size = sizeof(CTA_INIT_PARMS);
    initparms.parmflags  = 0;

    /* If server was specified, prepend it and a slash to descriptor string. */
    if (*req->szServer)
    {
        strcpy( szDescriptor, req->szServer );
        strcat( szDescriptor, "/" );
    }

    strcat( szDescriptor, szDefaultDescriptor );

    if ( (ret = ctaInitialize( svcList, gNumSvcs, &initparms )) != SUCCESS )
    {
        printf("Unable to Initialize with Natural Access - returns 0x%x\n", ret);
        exit( 1 );
    }
    else if (!( *req->szServer ))
    {
        ret = ctaSetDefaultServer("cta://localhost/");
        if ( ret != SUCCESS )
        {
            printf("Unable to set default server - return 0x%x\n", ret);
            exit(1);
        }
    }

    if (( ret = ctaCreateQueue( svcMgrList, gNumSvcs, &req->ctaqueuehd ))
                  != SUCCESS )
    {
        printf("Unable to create Natural Access queue - returns 0x%x\n", ret);
        exit( 1 );
    }
    else if ( (ret = ctaCreateContext( req->ctaqueuehd, 0, szDescriptor,
                                       &req->ctahd )) != SUCCESS )
    {
        printf("Unable to create Natural Access context - returns 0x%x\n", ret);
        if ( ret == CTAERR_SVR_COMM )
            printf(*req->szServer ? "Verify that Natural Access Server (ctdaemon) is running on %s.\n"
                                  : "Verify that Natural Access Server (ctdaemon) is running.\n",
                   req->szServer);
        exit( 1 );
    }
    else if ( (ret = ctaOpenServices( req->ctahd, svcDescriptions, gNumSvcs ))
                  != SUCCESS )
    {
        printf("Unable to open Natural Access services - returns 0x%x\n", ret);
        exit( 1 );
    }
    else
    {
        if ( (ret = ctaWaitEvent( req->ctaqueuehd, &event, CTA_WAIT_FOREVER))
                 != SUCCESS )
        {
            printf("Natural Access Open Service wait event returned 0x%x\n", ret );
            exit( 1 );
        }
        else if ( ( event.id != CTAEVN_OPEN_SERVICES_DONE ) ||
                  ( event.value != CTA_REASON_FINISHED ) )
            exit( 1 );
    }
}

/******************************************************************************
*
* exitFromCTA()
*
*   - closes Natural Access services
*   - destroys Natural Access context
*   - detroys a Natural Access queue
*
******************************************************************************/

void exitFromCTA( INFO_REQ *req )
{
    CTA_EVENT event;
    DWORD     ret;

    if ( ( ret = ctaCloseServices( req->ctahd, svcNames, gNumSvcs ) )
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
            exit( 1 );

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

    if ( (ret = ctaDestroyQueue( req->ctaqueuehd )) != SUCCESS )
    {
        printf("Natural Access Destroy Queue failed; returned 0x%x\n", ret );
        exit( 1 );
    }
}

/******************************************************************************
*
* isField
*
*   Perform a case insenstive search for a structure field.
*
******************************************************************************/

int isField( char *pKW, char *pField )
{
    char Ukw[ L_STRING ], *pChar;
    char Ufield[ L_STRING ];

    strcpy( Ukw, pKW );
    strcpy( Ufield, pField );

    /* Convert both main keyword and field to same case */
#if defined (WIN32)
    strupr( Ukw );
    strupr( Ufield );
#else
    {
    char *pUkw = Ukw;
    char *pUfield = Ufield;

    for( ; *pUkw; pUkw++ )
        *pUkw = (char) toupper( *pUkw );

    for( ; *pUfield; pUfield++ )
       *pUfield = (char) toupper( *pUfield );
    }
#endif


    /* Locate the '.' preceeding the last keyword structure field */
    if ( pChar = strrchr( Ukw, '.' ) )
    {
        char  *pc;

        /* Skip the '.' and compare the keyword's struct field against
        *  the one desired by caller.
        */
        ++pChar;

        /* Stip off [i] from keyword in case the desired field is an array but
        *  the caller has not asked for a specific array element.
        */
        if ( !strrchr( Ufield, '[' ) && (pc = strrchr( pChar, '['  )) )
        {
            /* Terminate the  at the '[' and append the ".Count"
            *  special array keyword.
            */
            *pc = '\0';
        }

        return (!strcmp( pChar, (const char *) &Ufield));
    }

    return 0;
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
        fprintf(stderr, "%s: error 0x%x: %s for %s\n", funcName, ret, buf, extra);
    else
        fprintf(stderr, "%s: error 0x%x: %s\n", funcName, ret, buf );

    /* Clean up OAM resources */
    if ( req->hObj != NULL_CTAHD )
        oamCloseObject( req->hObj );

    exitFromCTA( req );
    free( req );

    exit( 1 );
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
    else if ( IS_STRING( info ) )            ret = STR_TYPE;
    else if ( IS_INTEGER( info ) )           ret = INT_TYPE;
    else if ( IS_STRUCT( info ) )            ret = STRUCT_TYPE;
    else if ( IS_ARRAY( info ) )             ret = ARRAY_TYPE;
    else if ( IS_STRUCTANDARRAY( info ) )    ret = STRUCT_ARR_TYPE;
    else if ( IS_FILENAME( info ) )          ret = FILE_TYPE;
    else if ( IS_OBJECT( info ) )            ret = OBJ_TYPE;
    else if ( IS_IPADDRESS( info ) )         ret = ADDR_TYPE;
    else                                     ret = UNKNOWN_TYPE;

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
    {
        if ( strlen( info ) )
            printf("\tDescription: %s\n", info);
    }
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
        printf("\tMin value: %s;\t", info );
    }
    ret = oamGetQualifier( req->hObj, keyword, "Max", info, sizeof(info) );
    if ( ret != SUCCESS )
         onError( req, "showIntegerQualifiers", "Min", ret );
    else
    {
        printf("Max value: %s \n", info );
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

    /* If only a specific subfield has been requested, skip display of
    *  information if not the requested one.
    */
    if ( req->field[0] != '\0' )
    {
        if ( !isField( keyword, req->field ) )

            /* Does not contain desired keyword field */
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

    /* If only a specific subfield has been requested, skip display of
    *  information if not the requested one.
    */
    if ( req->field[0] != '\0' )
    {
        if ( !isField( keyword, req->field ) )

            /* Does not contain desired keyword field */
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
        /* If only a specific subfield has been requested, skip display of
        *  information if not the requested one.
        */
        if ( req->field[0] != '\0' )
        {
            if ( !isField( BaseKw, req->field ) )

                /* Does not contain desired keyword field */
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
*  setArray
*
*   Set array entries to a value.
*
*******************************************************************************/

void setArray( INFO_REQ *req, char *keyword, KW_TYPES type )
{
    DWORD    ret = SUCCESS;
    DWORD    i;
    char     info[ L_STRING];
    char     kwField[ L_STRING ];
    DWORD    max;

    max = getCount( req, keyword );

    if ( req->index != -1 )
    {
        /* Set one specific array entry */
        if ( req->field[0] != '\0' )
            sprintf( kwField, "%s[%d].%s", keyword, req->index, req->field );
        else
            sprintf( kwField, "%s[%d]", keyword, req->index );

        if ( max <= req->index )
        {
            printf("\tCurrently %s = <empty>\n", kwField, req->index );
        }
        else
        {
            ret = oamGetKeyword( req->hObj, kwField, info, sizeof(info) );
            if ( ret == SUCCESS )
            {
                printf("\tCurrently %s = %s\n", kwField, info );
            }
        }

        if ( ret == SUCCESS )
        {
            ret = oamSetKeyword( req->hObj, kwField, req->value );
            if ( ret != SUCCESS )
                onError( req, "oamSetKeyword", kwField, ret );
            else
            {
                ret = oamGetKeyword( req->hObj, kwField, info,
                             sizeof(info) );
                if ( ret == SUCCESS )
                    printf("\tNow       %s = %s\n", kwField, info );
                else
                    onError( req, "oamGetKeyword", kwField, ret );
            }
        }
        else
            onError( req, "oamGetKeyword", kwField, ret );
    }
    else
    {
        DWORD ret;

        ret =  oamGetKeyword( req->hObj, keyword, info, sizeof(info) );
        if ( ( type == ARRAY_TYPE ) ||
             (( type == STRUCT_ARR_TYPE ) && (ret != SUCCESS)) )
        {
            /* Set all array elements to specified value */
            for( i = 0; i < max; i++ )
            {
                if ( req->field[0] != '\0' )
                    sprintf( kwField, "%s[%d].%s", keyword, i, req->field );
                else
                    sprintf( kwField, "%s[%d]", keyword, i );

                ret = oamGetKeyword( req->hObj, kwField, info, sizeof(info) );
                if ( ret == SUCCESS )
                {
                    printf("\tCurrently %s = %s\n", kwField, info );
                    ret = oamSetKeyword( req->hObj, kwField, req->value );
                    if ( ret != SUCCESS )
                        onError( req, "oamSetKeyword", kwField, ret );
                    else
                    {
                        ret = oamGetKeyword( req->hObj, kwField, info, sizeof(info));
                        if ( ret == SUCCESS )
                            printf("\tNow       %s = %s\n", kwField, info );
                        else
                            onError( req, "oamGetKeyword", kwField, ret );
                    }
                }
                else
                    onError( req, "oamGetKeyword", kwField, ret );
            }
        }
        else if( ret == SUCCESS )
        {
            /* Set structure keyword. */
            printf("\tCurrently %s = %s\n", keyword, info );

            ret = oamSetKeyword( req->hObj, keyword, req->value );
            if ( ret != SUCCESS )
                onError( req, "oamSetKeyword", keyword, ret );
            else
            {
                ret = oamGetKeyword( req->hObj, keyword, info, sizeof(info) );
                if ( ret == SUCCESS )
                {
                    printf("\tNow       %s = %s\n", keyword, info );
                }
                else
                    onError( req, "oamGetKeyword", keyword, ret );
            }
        }
        else
             onError( req, "oamGetKeyword", keyword, ret );
    }
}

/*******************************************************************************
*
*  doSet
*
*   Set keword to specified value.
*
*******************************************************************************/

void doSet( INFO_REQ *req )
{
    DWORD    ret;
    char     info[ L_STRING ];
    KW_TYPES type;

    if ( ( req->keyword[0] == '\0' ) || ( req->value[0] == '\0' ) )
    {
        printf("Need both a keyword and a value for a set operation.\n");
    }
    else
    {
        /* Open the OAM managed object; this function will not return on
        *  failure
        */
        openObject( req );

        type = getType( req, req->keyword );
        if( (type == ARRAY_TYPE) || (type == STRUCT_ARR_TYPE) )
        {
            setArray( req, req->keyword, type );
        }
        else
        {
            ret = oamGetKeyword( req->hObj, req->keyword, info, sizeof(info) );
            if ( ret == SUCCESS )
            {
                /* This is a basic keyword. */
                printf("\tCurrently %s = %s\n", req->keyword, info );

                ret = oamSetKeyword( req->hObj, req->keyword, req->value );
                if ( ret != SUCCESS )
                    onError( req, "oamSetKeyword", req->keyword, ret );
                else
                {
                    ret = oamGetKeyword( req->hObj, req->keyword, info,
                                         sizeof(info));
                    if ( ret == SUCCESS )
                    {
                        printf("\tNow       %s = %s\n", req->keyword, info );
                    }
                }
            }
            else
            {
                /* Unable to get the current keyword value */
                onError( req, "oamGetKeyword", req->keyword, ret );
            }
        }


        if ( req->hObj != NULL_CTAHD )
            oamCloseObject( req->hObj );
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
    char  err[ M_STRING ];


    if ( req->objName[0] == '\0' )
    {
        /* Don't have a name; need to find it in the database. */
        ret = oamBoardLookupByNumber( req->ctahd, req->boardNumber,
                                      req->objName, sizeof(req->objName) );
        if ( ret != SUCCESS )
        {
            sprintf( err, "Board Number %d.", req->boardNumber );
            onError( req, "oamBoardLookupByNumber", err, ret );
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

        if ( req->bSetValue )

            /* Go set a keyword in the database */
            doSet( req );

        else
            /* Go look at the OAM configuration database */
            doQuery( req );

        /* nothing else needs to be done... */
        exitFromCTA( req );
    }

    exit( 0 );
}

