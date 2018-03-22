/*****************************************************************************
 *  File -  csplayrc.c
 *
 *  Description - This demo serves to demonstrate Client-Server Natural Access
 *                functionality. It needs to be run along with incta (in Client-
 *                Server mode). It performs the play and record functions while
 *                incta does just call control. This demonstrates context sharing,
 *                and call handoff.
 *
 *
 * Copyright (c) 1999-2002 NMS Communications.  All rights reserved.
 *****************************************************************************/

/*----------------------------- Includes -----------------------------------*/
#include "ctademo.h"
#include "csplayrc.h"

/*------------------------ Application Definitions -------------------------*/
typedef struct
{
    CTAHD       ctahd;            /* Natural Access context handle               */
    char        ctaname[128]; /* Natural Access context name    */
} MYCONTEXT;

#if defined (UNIX)
#define TEMP_FILE "/tmp/temp.vce"
#else
#define TEMP_FILE "temp.vce"
#endif

#define   DEMO_FILE "ctademo"

/*----------------------------- Application Data ---------------------------*/
static MYCONTEXT MyContext =
{
    NULL_CTAHD,      /* Natural Access context handle */
    "incta-demo",    /* Natural Access context name   */
};

MYCONTEXT *cx = &MyContext;

static MYCONTEXT CommnContext =
{
    NULL_CTAHD,      /* Natural Access context handle */
    "COMMN-CXT", /* Natural Access context name   */
};

MYCONTEXT *commncx = &CommnContext;

CTAQUEUEHD ctaqueuehd = NULL_CTAQUEUEHD;

/*----------------------------- Forward References -------------------------*/
void CmdLineOptions(int argc, char **argv);
void PrintHelp(void);
void SendEvent( CTAHD ctahd, DWORD evtid, DWORD evtvalue);


/*******************************************************
 *
 *  main
 *
 *******************************************************/

int main ( int argc, char *argv[] )
{
    int         ret;
    char        *svraddr = NULL;
    char        commndescriptor[256] = {0};
    unsigned    userid = 0;

    CmdLineOptions( argc, argv );

    DemoInitialize( NULL, 0 );

    if ((ret = ctaCreateQueue( NULL, 0, &ctaqueuehd )) != SUCCESS)
    {
        printf("Error: Unable to create queue\n");
        exit( -1 );
    }

    /* create context descriptor */
    if( svraddr == NULL  ||  *svraddr == 0 )
    {
        if( commncx->ctaname != NULL )
            strcpy( commndescriptor, commncx->ctaname );
    }
    else
    {
        sprintf( commndescriptor,  "//%s/%s", svraddr, commncx->ctaname );
    }

    if ((ret = ctaCreateContextEx( ctaqueuehd, userid, commndescriptor,
                                   &commncx->ctahd, 0 )) != SUCCESS)
    {
        printf("Unable to create context %s\n", commncx->ctaname);
        printf("Ensure that CTAmode=1 in cta.cfg.\n");
        exit(-1);
    }

    for(;;)
    {
        CTA_EVENT   event = { 0 };
        char        *filename;
        unsigned    message_num;
        unsigned    encoding = 0;
        int         ctaFreeBuf = 0;

        event.ctahd = commncx->ctahd;

        DemoWaitForEvent( commncx->ctahd, &event );

        /* In Natural Access Client/Server mode, certain event buffers are 
         * allocated by Natural Access and then must be released back to 
         * Natural Access.
         */
        if ( event.buffer )
        {
            if ( ctaFreeBuf = event.size & CTA_INTERNAL_BUFFER )
            {
                /* This buffer is owned by Natural Access; it will need to
                 *  be released back to CTA. Clear the Natural Access flag
                 *  from the size field.
                 */
                event.size &= ~CTA_INTERNAL_BUFFER;
            }
        }

        switch ( event.id )
        {
            case APPEVN_CONTEXT_NAME:
                {
                    /* This event message contains the name of the context
                     *  to which we should now attach.
                     */

                    if ( event.buffer )
                    {
                        MYCONTEXT newcontext;

                        if ( event.size <= sizeof( cx->ctaname ) )
                        {
                            /* Attach to play-record context */
                            if ((ret = ctaAttachContext( ctaqueuehd, userid,
                                                         event.buffer,  &(newcontext.ctahd)))
                                != SUCCESS)
                            {
                                printf("Unable to attach to context %s\n", cx->ctaname);
                            }
                            else
                            {
                                if (cx->ctahd != NULL_CTAHD)
                                    ctaDestroyContext( cx->ctahd );

                                cx->ctahd = newcontext.ctahd;
                                memcpy( cx->ctaname, event.buffer, event.size );
                            }
                        }
                        else
                        {
                            printf("Context name too long. Must be no longer");
                            printf("than %d\n", sizeof( cx->ctaname ) );
                        }
                    }
                    else
                    {
                        printf("Context name missing in APPEVN_CONTEXT_NAME event.\n");
                    }
                    break;
                }

            case APPEVN_PLAY_MESSAGE:
                if (event.value == CS_DEMO_FILE)
                {
                    filename = DEMO_FILE;
                    encoding = 0;
                }
                else if (event.value == CS_TEMP_FILE)
                {
                    filename = TEMP_FILE;
                    encoding = ADI_ENCODE_NMS_24;
                }
                else
                {
                    printf("Unknown filename request %u\n", event.value);
                    SendEvent( commncx->ctahd, APPEVN_PLAY_FAILURE,
                               APP_REASON_UNKNOWN_FILE );
                    break;
                }
                message_num = event.size;

                ret = DemoPlayMessage( cx->ctahd, filename, message_num,
                                       encoding, NULL );
                switch ( ret )
                {
                    case SUCCESS:
                        SendEvent( commncx->ctahd, APPEVN_PLAY_DONE,
                                   APP_REASON_FINISHED );
                        break;
                    case FAILURE:
                        SendEvent( commncx->ctahd, APPEVN_PLAY_FAILURE,
                                   APP_REASON_FUNCTION_FAILED );
                        break;
                    case DISCONNECT:
                        SendEvent( commncx->ctahd, APPEVN_PLAY_DONE,
                                   APP_REASON_DISCONNECT );
                        break;
                    default:
                        SendEvent( commncx->ctahd, APPEVN_PLAY_FAILURE,
                                   APP_REASON_UNKNOWN_REASON );
                        break;
                }

                break;

            case APPEVN_RECORD_MESSAGE:
                {
                    VCE_RECORD_PARMS rparms;

                    if (event.value == CS_DEMO_FILE)
                    {
                        filename = DEMO_FILE;
                        encoding = 0;
                    }
                    else if (event.value == CS_TEMP_FILE)
                    {
                        filename = TEMP_FILE;
                        encoding = ADI_ENCODE_NMS_24;
                    }
                    else
                    {
                        printf("Unknown filename request %u\n", event.value);
                        SendEvent( commncx->ctahd, APPEVN_RECORD_FAILURE,
                                   APP_REASON_UNKNOWN_FILE );
                        break;
                    }
                    message_num = event.size;

                    ctaGetParms( cx->ctahd, VCE_RECORD_PARMID,
                                 &rparms, sizeof(rparms) );

                    rparms.silencetime = 500;

                    ret = DemoRecordMessage( cx->ctahd, filename, message_num,
                                             encoding, &rparms );
                    switch ( ret )
                    {
                        case SUCCESS:
                            SendEvent( commncx->ctahd, APPEVN_RECORD_DONE,
                                       APP_REASON_FINISHED );
                            break;
                        case FAILURE:
                            SendEvent( commncx->ctahd, APPEVN_RECORD_FAILURE,
                                       APP_REASON_FUNCTION_FAILED );
                            break;
                        case DISCONNECT:
                            SendEvent( commncx->ctahd, APPEVN_RECORD_DONE,
                                       APP_REASON_DISCONNECT );
                            break;
                        default:
                            SendEvent( commncx->ctahd, APPEVN_RECORD_FAILURE,
                                       APP_REASON_UNKNOWN_REASON );
                            break;
                    }


                    break;
                }

            case APPEVN_SHUT_DOWN:
                return 0;

            default:
                break;
        }

        /* In Natural Access Client/Server mode, ensure proper release of 
         * Natural Access owned buffers. 
         */
        if ( ctaFreeBuf )
        {
            ctaFreeBuf = 0;
            ctaFreeBuffer( event.buffer );
        }
    }

    UNREACHED_STATEMENT( return FAILURE; )


}

/*******************************************************
 *
 *  CmdLineOptions
 *
 *******************************************************/

void CmdLineOptions(int argc, char **argv)
{
    int  c;

    while( ( c = getopt( argc, argv, "F:m:Hh?" ) ) != -1 )
    {
        switch( c )
        {
            case 'F':
                DemoSetCfgFile( optarg );
                break;

            case 'm':
                sscanf( optarg, "%s", commncx->ctaname );
                break;
            case 'h':
            case 'H':
            case '?':
                PrintHelp();
                exit(1);

            default:
                PrintHelp();
                exit(1);
        }
    }
    if (optind < argc)
    {
        PrintHelp();
        exit(1);
    }

    return;

}

/*******************************************************
 *
 *  PrintHelp
 *
 *******************************************************/

void PrintHelp()
{
    printf( "\nUsage: "
            "csplayrc [-F filename] [-m contextname]\n");

    printf( "  -F filename Natural Access Configuration File name. "
            "Default = None\n" );

    printf( "  -m contextname Natural Access Communication context name. "
            "Default = COMMN-CXT\n" );

    printf("\n");
}


/*******************************************************
 *
 *  SendEvent
 *
 *******************************************************/
void SendEvent( CTAHD ctahd, DWORD evtid, DWORD evtvalue)
{
    CTA_EVENT event = { 0 };

    event.id = evtid;
    event.ctahd = ctahd;
    event.value = evtvalue;

    if ( ctaQueueEvent( &event ) != SUCCESS )
        DemoShowError( "ctaQueueEvent failed", event.value );

}
