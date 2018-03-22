/******************************************************************************
NAME:  tikbnd.c

PURPOSE:

Implements the TIK Service Manager code (i.e., all the
TIK binding functions).

Copyright (c) 1997-2002 NMS Communications.  All rights reserved.
 ****************************************************************************/

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "tiksys.h"


/*-----------------------------------------------------------------------------
  Forward declarations of Life Cycle Binding Functions for
  the TIK Service Manager. "Life Cycle" refers to:
  - service registration
  - event processing initialization and shutdown
  - service startup and shutdown
  ----------------------------------------------------------------------------*/

STATIC DWORD NMSAPI 
tikDefineService( char *svcname );

STATIC DWORD NMSAPI 
tikAttachServiceManager( CTAQUEUEHD   ctaqueuehd,
                         unsigned     mode,
                         void       **queuecontext);

STATIC DWORD NMSAPI 
tikDetachServiceManager( CTAQUEUEHD  ctaqueuehd,
                         void       *queuecontext);

STATIC DWORD NMSAPI 
tikOpenServiceManager( CTAHD   ctahd,
                       void   *queuecontext,
                       void  **mgrcontext );

STATIC DWORD NMSAPI 
tikCloseServiceManager( CTAHD  ctahd,
                        void  *mgrcontext );

STATIC DWORD NMSAPI 
tikOpenService( CTAHD             ctahd, 
                void             *mgrcontext,
                char             *svcname,
                unsigned          svcid,
                CTA_MVIP_ADDR    *mvipaddr,
                CTA_SERVICE_ARGS *svcargs );

STATIC DWORD NMSAPI 
tikCloseService( CTAHD     ctahd, 
                 void     *mgrcontext,
                 char     *svcname, 
                 unsigned  svcid );


/**/
/*-----------------------------------------------------------------------------
  Forward declarations of Runtime Binding Functions for
  the TIK Service Manager. "Runtime" refers to:
  - upcall processing
  - Error handling
  - tracing
  ---------------------------------------------------------------------------*/

STATIC DWORD NMSAPI
tikProcessCommand( CTAHD         ctahd, 
                   void         *mgrcontext,
                   DISP_COMMAND *ctacmd );

STATIC const char* NMSAPI
tikGetText( unsigned code );

STATIC DWORD NMSAPI 
tikFormatMessage( DISP_MESSAGE *pmsg,
                  char         *s,
                  unsigned      size,
                  char         *indent );

STATIC DWORD NMSAPI
tikSetTraceLevel( CTAHD         ctahd, 
                  void         *mgrcontext,  
                  unsigned      svcid,
                  unsigned      tracemask );

STATIC DWORD NMSAPI
tikFormatTraceBuffer( unsigned  tracetag,
                      void     *inbuffer,
                      unsigned  insize,
                      char     *outbuffer, 
                      unsigned  outsize );


/**/
/*----------------------------------------------------------------------------
  Natural Access required Static Data.
  - Service Manager Binding Table (contains pointers to Tik specific
  binding functions). Note that the order of functions in the binding
  table is MANDATORY.
  - Service Manager Revision Information
  - Service Revision Information
  ---------------------------------------------------------------------------*/

STATIC CTASVCMGR_FNS tikSvcmgrFns =
{
    sizeof(CTASVCMGR_FNS),
    tikDefineService,            /* xxxDefineService()                       */
    tikAttachServiceManager,     /* xxxAttachServiceManager()                */
    tikDetachServiceManager,     /* xxxDetachServiceManager()                */
    tikOpenServiceManager,       /* xxxOpenServiceManager()                  */
    tikCloseServiceManager,      /* xxxCloseServiceManager()                 */
    tikOpenService,              /* xxxOpenService()                         */
    tikCloseService,             /* xxxCloseService()                        */
    NULL,                        /* xxxProcessEvent()                        */
    tikProcessCommand,           /* xxxProcessCommand()                      */
    NULL,                        /* xxxAddRTC()             - Future Enh.    */
    NULL,                        /* xxxRemoveRTC()          - Future Enh.    */
    tikGetText,                  /* xxxGetText()                             */
    tikFormatMessage,            /* xxxFormatMessage()                       */
    tikSetTraceLevel,            /* xxxSetTraceLevel()                       */
    tikFormatTraceBuffer,        /* xxxFormatTraceBuffer()                   */
    NULL,                        /* xxxGetFunctionPointer() - Future Enh.    */
};



/*------------------------------------------------------------------------------
  Global service trace mask.
  ----------------------------------------------------------------------------*/
volatile DWORD *tikTracePointer;



/**/
/*-----------------------------------------------------------------------------
  tikInitializeManager
  - Entry point required by Natural Access to register this new service manager.
  - Actions performed (via dispRegisterServiceManager) include:
  - declaring version number of service manager
  - declaring compatibility level of service manager
  - declaring service manager binding functions
  ---------------------------------------------------------------------------*/
DWORD NMSAPI tikInitializeManager( void )
{
    DWORD                 ret;
    static CTAINTREV_INFO tikMgrRevInfo = { 0 };
    static BOOL           initialized = FALSE;

    /* Needed by Natural Access provided error logging service */
    CTABEGIN("tikInitializeManager");

    /* Checked when service is initialized. */
    if (initialized)
    {
        return CTALOGERROR(NULL_CTAHD, CTAERR_ALREADY_INITIALIZED, TIK_SVCID);
    }

    /* Set manager revision information. */
    tikMgrRevInfo.size         = sizeof(CTAINTREV_INFO);
    tikMgrRevInfo.majorrev     = TIK_MAJORREV;
    tikMgrRevInfo.minorrev     = TIK_MINORREV;
    tikMgrRevInfo.expapilevel  = TIKAPI_COMPATLEVEL;
    tikMgrRevInfo.expspilevel  = TIKSPI_COMPATLEVEL;
    tikMgrRevInfo.reqdisplevel = DISP_COMPATLEVEL;
    strcpy (tikMgrRevInfo.builddate, __DATE__);

    /*
     *  Get the address of the system wide trace pointer for trace checking.
     *  If tracing is not enabled then ctaGlobalTracePointer will be set
     *  to NULL and no tracing should be done by any service.
     */
    dispGetTracePointer(&tikTracePointer);

    if( (ret=
         dispRegisterServiceManager(
                                    "TIKMGR", &tikSvcmgrFns, &tikMgrRevInfo)) != SUCCESS )
    {
        return CTALOGERROR(NULL_CTAHD, ret, TIK_SVCID);
    }
    else
        initialized = TRUE;

    return SUCCESS;

}   /* end tikInitializeManager() */



/**/
/*-----------------------------------------------------------------------------
  tikDefineService
  - Register this new service.
  - Actions performed include: 
  - declaring version number of service 
  - declaring compatibility level of service 
  - declaring parameter tables (standard, extension, and/or both).
  - For the TIK Service, only standard parameters are used (no extension
  parameters). Also note that the "parameter table" source code is
  generated via the pf2src utility from the tikparm.pf input file.
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI tikDefineService( char* svcname )
{
    DWORD                 ret;
    static CTAINTREV_INFO tikSvcRevInfo = { 0 };

    /* Needed by Natural Access provided error logging service */
    CTABEGIN("tikDefineService");

    /* Set service revision info. */
    tikSvcRevInfo.size         = sizeof(CTAINTREV_INFO);
    tikSvcRevInfo.majorrev     = TIK_MAJORREV;      
    tikSvcRevInfo.minorrev     = TIK_MINORREV;      
    tikSvcRevInfo.expapilevel  = TIKAPI_COMPATLEVEL;
    tikSvcRevInfo.expspilevel  = TIKSPI_COMPATLEVEL;
    tikSvcRevInfo.reqdisplevel = DISP_COMPATLEVEL;
    strcpy (tikSvcRevInfo.builddate, __DATE__);

    /* The most important phase of initializing a service! */
    ret = dispRegisterService(  "TIK",    
                                TIK_SVCID,
                                "TIKMGR", 
                                &tikSvcRevInfo, 
                                NULL,
                                0,
                                _tikParmDescTable,
                                NULL);
    if ( ret != SUCCESS )
    {
        return CTALOGERROR(NULL_CTAHD, ret, TIK_SVCID);
    }

    return SUCCESS;

}   /* end tikDefineService() */



/**/
/*-----------------------------------------------------------------------------
  tikAttachServiceManager
  - Declare Wait object
  - Allocate "Device" object which represents the "tick device".
  - Register the wait object with Natural Access via dispRegisterWaitObject
  - Set the device object to the "queuecontext" since demultiplexing
  of events is done a per "tick device" basis.
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI tikAttachServiceManager( CTAQUEUEHD   ctaqueuehd,
                                             unsigned     mode,
                                             void       **queuecontext)
{
    TIK_DEVICE_OBJECT  *ptikDevice = NULL;
    TSIIPC_WAIT_OBJ_HD  waitobj;
    CTA_WAITOBJ        *ctawaitobj;
    DWORD               ret;

    /* Needed by Natural Access provided error logging service */
    CTABEGIN("tikAttachServiceManager");

    /* Initialize queuecontext */
    *queuecontext = NULL;

    /* Create Tick "device" object. */
    if ( (ptikDevice=tikCreateDeviceObject()) == NULL )
    {
        return CTALOGERROR( NULL_CTAHD, CTAERR_OUT_OF_MEMORY, TIK_SVCID );
    }


    /*  Establish IPC connection. */

    /*  In the process of establishing IPC connection,
     *   OS specific server-to-client communication
     *   wait object is created within NMS tsi library.
     *   Under NT: wait object(manual reset event) is created via
     *             Window Socket API(WSACreateEvent).
     *   Under UNIX : wait object(pollfd structure) is allocated via
     *                alloc().
     */
    else if ( (ret=tikSetupIPC( ptikDevice )) != SUCCESS )
    {
        tikDestroyDeviceObject( ptikDevice );
        return CTALOGERROR( NULL_CTAHD, ret, TIK_SVCID );
    }

    /* Get tsi library created waitobject to register with Natural Access. */
    else if ( tsiIPCGetWaitObject(ptikDevice->readhd, &waitobj) != SUCCESS )
    {
        tikDestroyDeviceObject( ptikDevice );
        return CTALOGERROR( NULL_CTAHD, CTAERR_OS_INTERNAL_ERROR, TIK_SVCID );
    }

    /* Register created waitobject with Natural Access. */
    else if ( (ret=dispRegisterWaitObject( ctaqueuehd,
                                           (CTA_WAITOBJ *)waitobj,
                                           tikFetchAndProcess,
                                           (void *)ptikDevice )) != SUCCESS )
    {
        tikDestroyDeviceObject( ptikDevice );
        return CTALOGERROR( NULL_CTAHD, ret, TIK_SVCID );
    }

    /*  The follow two function calls(dispFindWaitObject,
     *   tsiIPCRegisterWaitObjectCopy) are required in order to
     *   make use of NMS's abstracted IPC communications layer. 
     */
    /* Acquire registered Natural Access WaitObject to register with TSIIPC. */
    else if ( (ret=dispFindWaitObject( ctaqueuehd,
                                       (CTA_WAITOBJ *)waitobj,
                                       &ctawaitobj) ) != SUCCESS )
    {
        tikDestroyDeviceObject( ptikDevice );
        return CTALOGERROR( NULL_CTAHD, ret, TIK_SVCID );
    }
    /* Register Natural Access WaitObject with TSIIPC. */
    else if ( (ret=tsiIPCRegisterWaitObjectCopy(ptikDevice->readhd,
                                                (TSIIPC_WAIT_OBJ_HD)ctawaitobj )
              ) != SUCCESS )
    {
        tikDestroyDeviceObject( ptikDevice );
        return CTALOGERROR( NULL_CTAHD, TIKERR_CAN_NOT_CONNECT, TIK_SVCID );
    }

    /* Assign the device object to the "queuecontext" so that event
     * handling can be performed on a "device" basis.
     * Using Natural Access terminology,"Mux Handling will be on a per queue basis
     * (as opposed to a per context basis)".
     */
    else
        *queuecontext = (void *)ptikDevice;

    return SUCCESS;

}   /* end tikAttachServiceManager() */


/**/
/*-----------------------------------------------------------------------------
  tikDetachServiceManager
  - Undeclare Wait object
  - Deallocate Resource object
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI tikDetachServiceManager( CTAQUEUEHD  ctaqueuehd,
                                             void       *queuecontext)
{
    TIK_DEVICE_OBJECT   *ptikDevice = (TIK_DEVICE_OBJECT *)queuecontext;
    TSIIPC_WAIT_OBJ_HD   waitobj;
    DWORD                ret;

    /* Needed by Natural Access provided error logging service */
    CTABEGIN("tikDetachServiceManager");

    /* Get IPC waitobject to un-register. */
    if ( tsiIPCGetWaitObject( ptikDevice->readhd, &waitobj ) != SUCCESS )
    {
        tikDestroyDeviceObject( ptikDevice );
        return CTALOGERROR( NULL_CTAHD, CTAERR_OS_INTERNAL_ERROR, TIK_SVCID );
    }

    /* Unregister waitobject with Natural Access. */
    else if ( (ret=dispUnregisterWaitObject(
                                            ctaqueuehd, (CTA_WAITOBJ *)waitobj)) != SUCCESS)
    {
        tikDestroyDeviceObject( ptikDevice );
        return CTALOGERROR( NULL_CTAHD, ret, TIK_SVCID );
    }

    /* Teardown IPC connection. */
    else if ( (ret=tikTeardownIPC( ptikDevice )) != SUCCESS )
    {
        tikDestroyDeviceObject( ptikDevice );
        return CTALOGERROR( NULL_CTAHD, ret, TIK_SVCID );
    }

    /* Destroy created service manager object. */
    else
        tikDestroyDeviceObject( ptikDevice );

    return SUCCESS;

}  /* end tikDetachServiceManager() */



/**/
/*-----------------------------------------------------------------------------
  tikOpenServiceManager
  - Allocate Service Manager specific data (a manager context)
  - In this case, a manager context corresponds to a Tick CHANNEL object.
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI tikOpenServiceManager( CTAHD   ctahd,
                                           void   *queuecontext,
                                           void  **mgrcontext )
{
    TIK_DEVICE_OBJECT  *ptikDevice  = (TIK_DEVICE_OBJECT *)queuecontext;
    TIK_CHANNEL_OBJECT *ptikChannel = NULL;

    /* Needed by Natural Access provided error logging service */
    CTABEGIN("tikOpenServiceManager");

    if ( (ptikChannel=tikCreateChannelObject(ctahd, ptikDevice)) == NULL )
    {
        *mgrcontext = NULL;
        return CTALOGERROR( NULL_CTAHD, CTAERR_OUT_OF_MEMORY, TIK_SVCID );
    }

    /*   Assign mgrcontext to be the Channel Object.
     *   A "manager context" is supposed to be data that is
     *   specific to a single instance of a Natural Access context (ctahd);
     *   a Channel Object is just that.
     */
    else
        *mgrcontext = (void *)ptikChannel;

    return SUCCESS;

}   /* end tikOpenServiceManager() */


/**/
/*-----------------------------------------------------------------------------
  tikCloseServiceManager
  - Deallocate Service Manager specific data (a manager context)
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI tikCloseServiceManager( CTAHD  ctahd,
                                            void   *mgrcontext )
{
    TIK_CHANNEL_OBJECT  *ptikChannel  = (TIK_CHANNEL_OBJECT *)mgrcontext;

    tikDestroyChannelObject( ptikChannel );

    return SUCCESS;

}   /* end tikCloseServiceManager() */


/**/
/*-----------------------------------------------------------------------------
  tikOpenService
  - Initialize any dedicated resources (typically on a per Natural Access Context
  basis)
  - For the Tik Service, this means allocating a logical channel on the
  "physical" TICK Device.
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI tikOpenService( CTAHD            ctahd,
                                    void             *mgrcontext,
                                    char             *svcname,
                                    unsigned         svcid,
                                    CTA_MVIP_ADDR    *mvipaddr,
                                    CTA_SERVICE_ARGS *svcargs )
{
    TIK_CHANNEL_OBJECT *ptikChannel = (TIK_CHANNEL_OBJECT *)mgrcontext;
    TIK_CMD_COM_OBJ    cmd = { 0 };
    DWORD              ret;

    /* Needed by Natural Access provided error logging service */
    CTABEGIN("tikOpenService");

    ASSERT( svcid == TIK_SVCID );

    /* svcargs->args[0] contain logical number to use in server. */
    ptikChannel->channelId = svcargs->args[0];
    if( ptikChannel->channelId < 0 || ptikChannel->channelId > 9 )
    {
        return( CTALOGERROR( NULL_CTAHD, CTAERR_BAD_ARGUMENT, TIK_SVCID ));
    }
    else
        ptikChannel->ptikDevice->channelTbl[ptikChannel->channelId] = ptikChannel;

    /*  If CTA_TRACEMASK_DEBUG_BIT0 is enabled,
     *  log state transition information.
     */
    if ( TRACEMASK_BIT0_SET(ptikChannel->tracemask) )
    {
        ret = tikLogStateTransition( ptikChannel,
                                     ptikChannel->state,
                                     CHANNEL_OPENING,
                                     "ctaOpenServices" );
        if ( ret != SUCCESS )
        {
            return CTALOGERROR( NULL_CTAHD, ret, TIK_SVCID );
        }
    }
    ptikChannel->state   = CHANNEL_OPENING;

    /* Send TIKSVR_OPEN_CHANNEL request. */
    cmd.size      = sizeof( TIK_CMD_COM_OBJ );
    cmd.timestamp = time(NULL);
    cmd.client_id = ptikChannel->channelId;
    cmd.command   = TIKSVR_OPEN_CHANNEL;
    cmd.data1     = ptikChannel->channelId;
    cmd.data2     = 0;
    cmd.data3     = 0;

    if( (ret=tikSendClientCommand( ptikChannel, &cmd )) != SUCCESS )
    {
        tikDestroyChannelObject( ptikChannel );
        return CTALOGERROR( NULL_CTAHD, ret, TIK_SVCID );
    }

    else if( (ret=tikReadServerResponse( ptikChannel )) != SUCCESS )
    {
        tikDestroyChannelObject( ptikChannel );
        return CTALOGERROR( NULL_CTAHD, ret, TIK_SVCID );
    }

    return SUCCESS;

}   /* end tikOpenService() */


/**/
/*-----------------------------------------------------------------------------
  tikCloseService
  - shutdown the resource
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI tikCloseService ( CTAHD     ctahd,
                                      void     *mgrcontext,
                                      char     *svcname,
                                      unsigned  svcid )
{
    TIK_CHANNEL_OBJECT  *ptikChannel = (TIK_CHANNEL_OBJECT*)mgrcontext;
    DWORD               ret;

    /* Needed by Natural Access provided error logging service */
    CTABEGIN("tikCloseService");

    ASSERT( svcid == TIK_SVCID );

    /* Perform graceful unwinding prior to service close. */
    if( (ret=tikHandleCloseService( ptikChannel )) != SUCCESS )
    {
        return CTALOGERROR( ctahd, ret, TIK_SVCID );
    }

    return SUCCESS;

}  /* end tikCloseService() */



/**/
/*-----------------------------------------------------------------------------
  tikProcessCommand
  - declare table of function pointers to implementation functions
  - perform lookup based on incoming id
  - perform upcall to appropriate implementation function
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI tikProcessCommand( CTAHD          ctahd, 
                                       void          *mgrcontext,
                                       DISP_COMMAND  *ctacmd )
{
    typedef DWORD (*TIK_CMD_FCN) ( TIK_CHANNEL_OBJECT *, DISP_COMMAND *);
    static const TIK_CMD_FCN tikCmdFcnTbl[]=
    {
        /* 0x000 */    tikCmdStartTimer,
        /* 0x001 */    tikCmdStopTimer,
        /* 0x002 */    tikCmdGetChannelInfo,
    };
    TIK_CHANNEL_OBJECT *ptikChannel = (TIK_CHANNEL_OBJECT *)mgrcontext;
    DWORD ret;

    /* Needed by Natural Access provided error logging service */
    CTABEGIN("tikProcessCommand");

    /*  Make sure command is defined in this command table, 
     *  before invoking the appropriate command.
     */
    if ((ctacmd->id & 0xfff) < sizeof(tikCmdFcnTbl) / sizeof(tikCmdFcnTbl[0]))
    {
        ret = ((*tikCmdFcnTbl[ ctacmd->id & 0xfff ]) (ptikChannel, ctacmd));
        if ( ret != SUCCESS && ret != SUCCESS_RESPONSE )
        {
            return CTALOGERROR( ctahd, ret, TIK_SVCID );
        }
    }
    else
    {
        return CTALOGERROR( ctahd, CTAERR_NOT_IMPLEMENTED, TIK_SVCID );
    }

    return ret;

}  /* end tikProcessCommand() */


/**/
/*-----------------------------------------------------------------------------
  tikGetText
  - convert error, event, reason, and/or command codes to associated
  ASCII macro identifer
  ---------------------------------------------------------------------------*/
STATIC const char* NMSAPI tikGetText( unsigned code )
{

    switch (code)
    {
        CTA_GENERIC_ERRORS()    /* see ctaerr.h */
            CTA_GENERIC_REASONS();  /* see ctaerr.h; need semicolon here */ 
        /* bc CTA_GENERIC_REASONS doesn't end */ 
        /* with semicolon */

        TIK_ERRORS()            /* see tiksys.h */

            TIK_REASONS()           /* see tiksys.h */

            TIK_EVENTS()            /* see tiksys.h */
            TIK_COMMANDS()          /* see tiksys.h */

        default: return NULL ;
    }

}   /* end tikGetText() */



/**/
/*-----------------------------------------------------------------------------
  tikFormatMessage
  - translate commands and events to ASCII
  - use tikGetText to help in the translation
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI tikFormatMessage( DISP_MESSAGE *pmsg,
                                      char         *s,
                                      unsigned     size,
                                      char         *indent)
{
    char *text;
    char  tmp[1024] = "";
    DWORD cnt;

    /* Needed by Natural Access provided error logging service */
    CTABEGIN ("tikFormatMessage");

    ASSERT ( s != NULL && size != 0 );

    /* Switch statement returns pointer to static text. */
    text = (char*)tikGetText(pmsg->id);

    if (CTA_IS_EVENT(pmsg->id))
    {
        if( text == NULL )
        {
            sprintf( tmp, "%sUnknown TIK Event: *** (%08X)",
                     indent, pmsg->id );
        }
        else
        {
            DISP_EVENT *evt = (DISP_EVENT *)pmsg;
            char *reason = (char *)tikGetText(evt->value);

            sprintf( tmp, "%sEvent: %s  *** (%08X)  Reason: %s *** (%08X)\n",
                     indent, text, pmsg->id, reason, evt->value );
        }
    }
    else /* command */
    {
        if( text == NULL )
        {
            sprintf( tmp, "%sUnknown TIK Command: *** (%08X)\n",
                     indent, pmsg->id );
        }
        else
        {
            DISP_COMMAND *cmd = (DISP_COMMAND *)pmsg;
            sprintf( tmp, "%sCommand: %s  *** (%08X) ",
                     indent, text, pmsg->id );
            switch ( pmsg->id )
            {
                case TIKCMD_START:
                    {
                        TIK_START_PARMS  start;
                        DWORD            ret;
                        char             buffer[128];

                        /* Get user specified start pararmeters. */
                        if ( cmd->size1 == 0 && cmd->dataptr1 == NULL )
                        {
                            /* Command is using system default start parameter.
                             *  Get system default start parameter.
                             */
                            strcat( tmp, "(System defaults) " );
                            ret = dispGetParms( pmsg->ctahd, TIK_START_PARMID,
                                                &start, sizeof(TIK_START_PARMS) );
                            if ( ret != SUCCESS )
                            {
                                return ret;
                            }
                        }
                        else
                        {
                            /* Get application specified start parameters. */
                            start = *(TIK_START_PARMS *)(cmd->dataptr1);
                        }
                        sprintf(buffer, "NumTicks = %d, Frequency = %d.\n", 
                                start.NumTicks, start.Frequency );
                        strcpy( tmp, buffer );
                        break;
                    }

                case TIKCMD_STOP:
                case TIKCMD_GET_CHANNEL_INFO:
                default:
                    {
                        strcat( tmp, "\n" );
                        break;
                    }
            }  /* end switch */
        }  /* end if text != NULL */
    }   /* end command processing */


    /* calculate the amout of locally formatted text to copy.
     *  this amount is the lesser of submitted buffer size and
     *  size of locally formatted text.
     */
    cnt = ( (strlen( tmp )+1) > (size)) ? (size) : (strlen( tmp ) + 1);

    /* copy calculated amount of data to outbuffer. */
    strncpy( s, tmp, cnt );
    s[cnt-1] = '\0';

    return SUCCESS;

}   /* end tikFormatMessage() */



/**/
/*-----------------------------------------------------------------------------
  tikSetTraceLevel
  - update local tracemask
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI tikSetTraceLevel( CTAHD    ctahd,
                                      void     *mgrcontext,
                                      unsigned svcid,
                                      unsigned tracemask )
{
    TIK_CHANNEL_OBJECT *ptikChannel = (TIK_CHANNEL_OBJECT *)mgrcontext;

    /* Needed by Natural Access provided error logging service */
    CTABEGIN("tikSetTraceLevel");

    ASSERT( svcid == TIK_SVCID );

    /* Set Channel specific tracemask. */
    ptikChannel->tracemask = tracemask;

    return SUCCESS;

}  /* end tikSetTraceMask() */



/**/
/*-----------------------------------------------------------------------------
  tikFormatTraceBuffer
  - convert binary trace messages to ASCII
  - note that only TIK specific trace buffers will be converted!
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI tikFormatTraceBuffer( unsigned tracetag,
                                          void     *inbuffer,
                                          unsigned insize,
                                          char     *outbuffer,
                                          unsigned outsize )
{
    DWORD ret = SUCCESS;
    DWORD size;
    char  tmp[512] = "";

    /* Needed by Natural Access provided error logging service */
    CTABEGIN("tikFormatTraceBuffer");

    switch (tracetag)
    {
        case TIK_TRACETAG_CMD:
            {
                TIK_CMD_COM_OBJ *cmd = (TIK_CMD_COM_OBJ *)inbuffer ;

                ASSERT( insize >= sizeof(TIK_CMD_COM_OBJ) );

                sprintf( tmp,
                         "TIK Service ClientID(%d) Command:(%s) Data1=(%d) "
                         "Data2=(%d) Data3=(%d) TimeStamp=(0x%08X).\n", cmd->client_id,
                         tikTranslateCmdRsp(cmd->command), cmd->data1, cmd->data2,
                         cmd->data3, cmd->timestamp );
                break;
            }

        case TIK_TRACETAG_RSP:
            {
                TIK_RSP_COM_OBJ *rsp = (TIK_RSP_COM_OBJ *)inbuffer ;

                ASSERT(insize >= sizeof(TIK_RSP_COM_OBJ));

                sprintf( tmp,
                         "TIK Service ClientID(%d) Response:(%s) Reason=(%s) "
                         "Channel=(%d) TikSeq=(%d) SvrMsg=(%s)"
                         " TimeStamp=(%x).\n", rsp->client_id,
                         tikTranslateCmdRsp( rsp->response ),
                         tikTranslateCmdRsp( rsp->reason ), rsp->channel, rsp->seq,
                         rsp->msg, rsp->timestamp );
                break;
            }

        case TIK_TRACETAG_BIT0:
            {
                STATE_TRAN_CTCX *statectcx = (STATE_TRAN_CTCX *)inbuffer;

                ASSERT( statectcx->size >= sizeof(STATE_TRAN_CTCX) );

                sprintf( tmp,
                         "TIK Service ClientID(%d) PrevState:(%s) CurrentState(%s) "
                         "Agent(%s).\n", statectcx->channel, statectcx->prev_state,
                         statectcx->curr_state, statectcx->agent );
                break;
            }

        default:
            {
                ret = CTAERR_NOT_IMPLEMENTED;
                break;
            }

    }  /* end switch */


    /* calculate the amout of locally formatted text to copy.
     *  this amount is the lesser of submitted buffer size and
     *  size of locally formatted text.
     */
    size = ( (strlen( tmp )+1) > (outsize)) ? (outsize) : (strlen( tmp ) + 1);

    /* copy calculated amount of data to outbuffer. */
    strncpy( outbuffer, tmp, size );
    outbuffer[size-1] = '\0';

    return ret;

}  /* end tikFormatTraceBuffer() */

