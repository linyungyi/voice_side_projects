/******************************************************************************
  NAME:  evtbnd.c

  PURPOSE:

      Implements the EVT Service Manager (i.e., all the EVT binding functions).

  Copyright 2001 NMS Communications.  All rights reserved.
  ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "evtsys.h"
#include "evtspi.h"
/* #include "evtdvc.h" */

/* Device API header file. *-
#include "evtdevice.h" */

/*-----------------------------------------------------------------------------
  Forward declarations of Life Cycle Binding Functions for
  the EVT Service Manager. "Life Cycle" refers to:
    - service registration
    - event processing initialization and shutdown
    - service startup and shutdown
  ----------------------------------------------------------------------------*/

STATIC DWORD NMSAPI 
  evtDefineService( char *svcname );
  
STATIC DWORD NMSAPI 
  evtAttachServiceManager( CTAQUEUEHD   ctaqueuehd,
                           unsigned     mode,
                           void       **queuecontext);
                          
STATIC DWORD NMSAPI 
  evtDetachServiceManager( CTAQUEUEHD  ctaqueuehd,
                           void       *queuecontext);

STATIC DWORD NMSAPI 
  evtOpenServiceManager( CTAHD   ctahd,
                         void   *queuecontext,
                         void  **mgrcontext );
                         
STATIC DWORD NMSAPI 
  evtCloseServiceManager( CTAHD  ctahd,
                          void  *mgrcontext );

STATIC DWORD NMSAPI 
  evtOpenService( CTAHD             ctahd, 
                  void             *mgrcontext,
                  char             *svcname,
                  unsigned          svcid,
                  CTA_MVIP_ADDR    *mvipaddr,
                  CTA_SERVICE_ARGS *svcargs );

STATIC DWORD NMSAPI 
  evtCloseService( CTAHD     ctahd, 
                   void     *mgrcontext,
                   char     *svcname, 
                   unsigned  svcid );


/**/
/*-----------------------------------------------------------------------------
  Forward declarations of Runtime Binding Functions for
  the EVT Service Manager. "Runtime" refers to:
    - upcall processing
    - Error handling
    - tracing
  ---------------------------------------------------------------------------*/

STATIC DWORD NMSAPI
  evtProcessCommand( CTAHD         ctahd, 
                     void         *mgrcontext,
                     DISP_COMMAND *ctacmd );

STATIC const char* NMSAPI
  evtGetText( unsigned code );

STATIC DWORD NMSAPI 
  evtFormatMessage( DISP_MESSAGE *pmsg,
                    char         *s,
                    unsigned      size,
                    char         *indent );

STATIC DWORD NMSAPI
  evtSetTraceLevel( CTAHD         ctahd, 
                    void         *mgrcontext,  
                    unsigned      svcid,
                    unsigned      tracemask );

STATIC DWORD NMSAPI
  evtFormatTraceBuffer( unsigned  tracetag,
                        void     *inbuffer,
                        unsigned  insize,
                        char     *outbuffer, 
                        unsigned  outsize );

static EVT_PERQUE* evtCreatePerQue( void );
static void evtDestroyPerQue( EVT_PERQUE* pevtPerQue );

static EVT_PERCTX *evtCreatePerCtx( CTAHD ctahd, EVT_PERQUE *pevtPerQue );
static void evtDestroyPerCtx( EVT_PERCTX *pevtPerCtx );

/**/
/*----------------------------------------------------------------------------
  Natural Access required Static Data.
    - Service Manager Binding Table (contains pointers to service specific
      binding functions). Note that the order of functions in the binding
      table is MANDATORY.
    - Service Manager Revision Information
    - Service Revision Information
  ---------------------------------------------------------------------------*/

STATIC CTASVCMGR_FNS evtSvcmgrFns =
{
    sizeof(CTASVCMGR_FNS),
    evtDefineService,            /* evtDefineService()                       */
    evtAttachServiceManager,     /* evtAttachServiceManager()                */
    evtDetachServiceManager,     /* evtDetachServiceManager()                */
    evtOpenServiceManager,       /* evtOpenServiceManager()                  */
    evtCloseServiceManager,      /* evtCloseServiceManager()                 */
    evtOpenService,              /* evtOpenService()                         */
    evtCloseService,             /* evtCloseService()                        */
    NULL,                        /* evtProcessEvent()                        */
    evtProcessCommand,           /* evtProcessCommand()                      */
    NULL,                        /* evtAddRTC()                              */
    NULL,                        /* evtRemoveRTC()                           */
    evtGetText,                  /* evtGetText()                             */
    evtFormatMessage,            /* evtFormatMessage()                       */
    evtSetTraceLevel,            /* evtSetTraceLevel()                       */
    evtFormatTraceBuffer,        /* evtFormatTraceBuffer()                   */
    NULL,                        /* evtGetFunctionPointer() - Future Enh.    */
};



/*------------------------------------------------------------------------------
  Global service trace mask.
  ----------------------------------------------------------------------------*/
volatile DWORD *evtTracePointer;

/**/
/*-----------------------------------------------------------------------------
  evtInitializeManager
    - Entry point required by Natural Access to register this new service manager.
    - Actions performed (via dispRegisterServiceManager) include:
      - declaring version number of service manager
      - declaring compatibility level of service manager
      - declaring service manager binding functions
  ---------------------------------------------------------------------------*/
DWORD NMSAPI evtInitializeManager( void )
{
    DWORD                 ret;
    static CTAINTREV_INFO evtMgrRevInfo = { 0 };
    static BOOL           initialized = FALSE;
    
    /* Needed by Natural Access provided error logging service */
    CTABEGIN("evtInitializeManager");

    /* Checked when service is initialized. */
    if (initialized)
    {
       return CTALOGERROR(NULL_CTAHD, CTAERR_ALREADY_INITIALIZED, EVT_SVCID);
    }

    /* Set manager revision information. */
    evtMgrRevInfo.size         = sizeof(CTAINTREV_INFO);
    evtMgrRevInfo.majorrev     = EVT_MAJORREV;      
    evtMgrRevInfo.minorrev     = EVT_MINORREV;      
    evtMgrRevInfo.expapilevel  = EVTAPI_COMPATLEVEL;
    evtMgrRevInfo.expspilevel  = EVTSPI_COMPATLEVEL;
    evtMgrRevInfo.reqdisplevel = DISP_COMPATLEVEL;
    strcpy (evtMgrRevInfo.builddate, __DATE__);

    /*
     *  Get the address of the system wide trace pointer for trace checking.
     *  If tracing is not enabled then ctaGlobalTracePointer will be set
     *  to NULL and no tracing should be done by any service.
     */
    dispGetTracePointer(&evtTracePointer);

    if( (ret=
         dispRegisterServiceManager(
           "EVTMGR", &evtSvcmgrFns, &evtMgrRevInfo)) != SUCCESS )
    {
       return CTALOGERROR(NULL_CTAHD, ret, EVT_SVCID);
    }
    else
       initialized = TRUE;

    return SUCCESS;
    
}   /* end evtInitializeManager() */

/**/
/*-----------------------------------------------------------------------------
  evtDefineService
    - Register this new service.
    - Actions performed include: 
      - declaring version number of service 
      - declaring compatibility level of service 
      - declaring parameter tables (standard, extension, and/or both).
    - Also note that the "parameter table" source code is
      generated via the pf2src utility from the evtparm.pf input file.
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI evtDefineService( char* svcname )
{
    DWORD ret;
    
    static CTAINTREV_INFO evtSvcRevInfo = { 0 };

    /* Needed by Natural Access provided error logging service */
    CTABEGIN("evtDefineService");

    ASSERT( stricmp(svcname, "EVT") == 0 );

    /* Set service revision info. */
    evtSvcRevInfo.size         = sizeof(CTAINTREV_INFO);
    evtSvcRevInfo.majorrev     = EVT_MAJORREV;      
    evtSvcRevInfo.minorrev     = EVT_MINORREV;      
    evtSvcRevInfo.expapilevel  = EVTAPI_COMPATLEVEL;
    evtSvcRevInfo.expspilevel  = EVTSPI_COMPATLEVEL;
    evtSvcRevInfo.reqdisplevel = DISP_COMPATLEVEL;
    strcpy (evtSvcRevInfo.builddate, __DATE__);

    /* The most important phase of initializing a service! */
    ret = dispRegisterService(  "EVT",
                                EVT_SVCID,
                                "EVTMGR",
                                &evtSvcRevInfo,
                                NULL,
                                0,  
                                0, /* _evtParmDescTable, no service parameters */
                                NULL );
    if ( ret != SUCCESS )
    {
       return CTALOGERROR(NULL_CTAHD, ret, EVT_SVCID);
    }
    
    return SUCCESS;
    
}   /* end evtDefineService() */


/**/
/*-----------------------------------------------------------------------------
  evtAttachServiceManager
    - Declare Wait object
    - Allocate "Device" object which represents the service device.
    - Register the wait object with Natural Access via dispRegisterWaitObject
    - Set the device object to the "queuecontext".  "queuecontext" will
      appear as argument in subsequent binding functions
      ( i.e. evtOpenServiceManager )
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI evtAttachServiceManager( CTAQUEUEHD   ctaqueuehd,
                                             unsigned     mode,
                                             void       **queuecontext)
{
    EVT_PERQUE          *pevtPerQue = NULL;

    /* Needed by Natural Access provided error logging service */
    CTABEGIN("evtAttachServiceManager");
    
    /* Initialize "queuecontext" */
    *queuecontext = NULL;
    
    /* Create "device" object. */
    if ( (pevtPerQue=evtCreatePerQue()) == NULL )
    {
       return CTALOGERROR( NULL_CTAHD, CTAERR_OUT_OF_MEMORY, EVT_SVCID );
    }
    else
       *queuecontext = (void *)pevtPerQue;

    return SUCCESS;
    
}   /* end evtAttachServiceManager() */


/**/
/*-----------------------------------------------------------------------------
  evtDetachServiceManager
    - Undeclare Wait object
    - Deallocate "Device" object
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI evtDetachServiceManager( CTAQUEUEHD  ctaqueuehd,
                                             void       *queuecontext)
{
   EVT_PERQUE           *pevtPerQue = (EVT_PERQUE *)queuecontext;
   
   /* Needed by Natural Access provided error logging service */
   CTABEGIN("evtDetachServiceManager");

   evtDestroyPerQue( pevtPerQue );

   return SUCCESS;
   
}  /* end evtDetachServiceManager() */

/**/
/*-----------------------------------------------------------------------------
  evtOpenServiceManager
    - Allocate Service Manager specific data (a manager context)
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI evtOpenServiceManager( CTAHD   ctahd,
                                           void   *queuecontext,
                                           void  **mgrcontext )
{
    EVT_PERQUE          *pevtPerQue = (EVT_PERQUE *)queuecontext;
    EVT_PERCTX          *pevtPerCtx = NULL;

    /* Needed by Natural Access provided error logging service */
    CTABEGIN("evtOpenServiceManager");
    
    /* Create Channel Object which is associated with "queuecontext" Object. */
    if ( (pevtPerCtx=evtCreatePerCtx(ctahd, pevtPerQue)) == NULL )
    {
       *mgrcontext = NULL;
       return CTALOGERROR( NULL_CTAHD, CTAERR_OUT_OF_MEMORY, EVT_SVCID );
    }
    
    /*   Assign mgrcontext to be the Channel Object.
     *   A "manager context" is supposed to be data that is
     *   specific to a single instance of a Natural Access context (ctahd);
     *   a Channel Object is just that.
     */
    else
       *mgrcontext = (void *)pevtPerCtx;

    return SUCCESS;
    
}   /* end evtOpenServiceManager() */


/**/
/*-----------------------------------------------------------------------------
  evtCloseServiceManager
    - Deallocate Service Manager specific data (a manager context)
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI evtCloseServiceManager( CTAHD  ctahd,
                                            void   *mgrcontext )
{
    EVT_PERCTX  *pevtPerCtx  = (EVT_PERCTX *)mgrcontext;
    
    evtDestroyPerCtx( pevtPerCtx );

    return SUCCESS;
    
}   /* end evtCloseServiceManager() */


/**/
/*-----------------------------------------------------------------------------
  evtOpenService
    - Initialize any dedicated resources (typically on a per Natural Access Context
      basis)
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI evtOpenService( CTAHD            ctahd,
                                    void             *mgrcontext,
                                    char             *svcname,
                                    unsigned         svcid,
                                    CTA_MVIP_ADDR    *mvipaddr,
                                    CTA_SERVICE_ARGS *svcargs )
{
    DWORD              ret;

    /* Needed by Natural Access provided error logging service */
    CTABEGIN("evtOpenService");

    ASSERT( stricmp(svcname, "EVT") == 0 );
    ASSERT( svcid == EVT_SVCID );

    /* MUST queue event to indicate successful open of service */
    if( (ret=dispMakeAndQueueEvent( ctahd,
                                    CTAEVN_DISP_OPEN_SERVICE_DONE,
                                    CTA_REASON_FINISHED,
                                    EVT_SVCID,
                                    CTA_SYS_SVCID)) != SUCCESS )
    {
        return CTALOGERROR(ctahd, ret, svcid);
    }

    return SUCCESS;
    
}   /* end evtOpenService() */

                                 
/**/
/*-----------------------------------------------------------------------------
  evtCloseService
    - shutdown the resource
    - need to consider graceful unwinding from any
      device resource states.
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI evtCloseService ( CTAHD     ctahd,
                                      void     *mgrcontext,
                                      char     *svcname,
                                      unsigned  svcid )
{
   DWORD               ret;

   /* Needed by Natural Access provided error logging service */
   CTABEGIN("evtCloseService");

   ASSERT( stricmp(svcname, "evt") == 0 );
   ASSERT( svcid == EVT_SVCID );

   /* MUST queue event to indicate successful close of service */
   if( (ret=dispMakeAndQueueEvent( ctahd,
                                   CTAEVN_DISP_CLOSE_SERVICE_DONE,
                                   CTA_REASON_FINISHED,
                                   EVT_SVCID,
                                   CTA_SYS_SVCID)) != SUCCESS )
      return CTALOGERROR(ctahd, ret, svcid);
   
   return SUCCESS;
   
}  /* end evtCloseService() */

/**/
/*-----------------------------------------------------------------------------
  evtProcessCommand
    - declare table of function pointers to implementation functions
    - perform lookup based on incoming id
    - perform upcall to appropriate implementation function
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI evtProcessCommand( CTAHD          ctahd, 
                                       void          *mgrcontext,
                                       DISP_COMMAND  *ctacmd )
{
   typedef DWORD (*EVT_CMD_FCN) ( EVT_PERCTX *, DISP_COMMAND *);
   static const EVT_CMD_FCN evtCmdFcnTbl[]=
   {
       /* 0x000 */    evtCmdSetLogFile,
       /* 0x001 */    evtCmdAddEventAll,
       /* 0x002 */    evtCmdAddEventValue,
       /* 0x003 */    evtCmdAddEvent,
       /* 0x004 */    evtCmdRemoveEventAll,
       /* 0x005 */    evtCmdRemoveEventValue,
       /* 0x006 */    evtCmdRemoveEvent,
       /* 0x007 */    evtCmdLogStart,
       /* 0x008 */    evtCmdLogStop,
       /* 0x009 */    evtCmdShowEvent,
   };
   EVT_PERCTX *pevtPerCtx = (EVT_PERCTX *)mgrcontext;
   DWORD ret;

   /* Needed by Natural Access provided error logging service */
   CTABEGIN("evtProcessCommand");

   /*  Make sure command is defined in this command table, 
    *  before invoking the appropriate command.
    */
   if ((ctacmd->id & 0xfff) < sizeof(evtCmdFcnTbl) / sizeof(evtCmdFcnTbl[0]))
   {
     ret = ((*evtCmdFcnTbl[ ctacmd->id & 0xfff ]) (pevtPerCtx, ctacmd));
     if ( ret != SUCCESS && ret != SUCCESS_RESPONSE )
     {
        return CTALOGERROR( ctahd, ret, EVT_SVCID );
     }
   }
   else
   {
     return CTALOGERROR( ctahd, CTAERR_NOT_IMPLEMENTED, EVT_SVCID );
   }
   
   return ret;
   
}  /* end evtProcessCommand() */


/**/
/*-----------------------------------------------------------------------------
  evtGetText
    - convert error, event, reason, and/or command codes to associated
      ASCII macro identifer
  ---------------------------------------------------------------------------*/
STATIC const char* NMSAPI evtGetText( unsigned code )
{

    switch (code)
    {
//      CTA_GENERIC_ERRORS();   /* see ctaerr.h */
        CTA_GENERIC_REASONS();  /* see ctaerr.h */

//      EVT_ERRORS();           /* see evtsys.h */
//      EVT_REASONS();          /* see evtsys.h */

//      EVT_EVENTS();           /* see evtsys.h */
//      EVT_COMMANDS();         /* see evtsys.h */

        default: return NULL ;
    }
    
}   /* end evtGetText() */

/**/
/*-----------------------------------------------------------------------------
  evtFormatMessage
    - translate commands and events to ASCII
    - use evtGetText to help in the translation
    - responsible for service specific DISP_COMMAND and DISP_EVENT translation.
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI evtFormatMessage( DISP_MESSAGE *pmsg,
                                      char         *s,
                                      unsigned     size,
                                      char         *indent)
{
    char *text;
    char  tmp[1024] = "";

    /* Needed by Natural Access provided error logging service */
    CTABEGIN("evtFormatMessage");

    ASSERT ( s != NULL && size != 0 );

    /* Switch statement returns pointer to static text. */
    text = (char*)evtGetText(pmsg->id);

    if (CTA_IS_EVENT(pmsg->id))
    {
       if( text == NULL )
       {
          sprintf( tmp, "%sUnknown EVT Event: *** (%08X)",
                   indent, pmsg->id );
       }
       else
       {
          DISP_EVENT *event = (DISP_EVENT *)pmsg;
          char *reason = (char *)evtGetText(event->value);

          sprintf( tmp, "%sEvent: %s  *** (%08X)  Reason: %s *** (%08X)\n",
                   indent, text, pmsg->id, reason, event->value );
       }
    }
    else /* command */
    {
        if( text == NULL )
        {
            sprintf( tmp, "%sUnknown EVT Command: *** (%08X)\n",
                     indent, pmsg->id );
        }
        else
        {
           DISP_COMMAND *cmd = (DISP_COMMAND *)pmsg;
           sprintf( tmp, "%sCommand: %s  *** (%08X) ",
                    indent, text, pmsg->id );
           switch ( pmsg->id )
           {
              case EVTCMD_SET_LOGFILE:
              {
                 /*
                 *   Text interpretation of EVTCMD_SET_LOGFILE.
                 */
                 break;
              }
              case EVTCMD_ADD_EVENTALL:
              {
                 /*
                 *   Text interpretation of EVTCMD_ADD_EVENTALL.
                 */
                 break;
              }
              case EVTCMD_ADD_EVENTVALUE:
              {
                 /*
                 *   Text interpretation of EVTCMD_ADD_EVENTVALUE.
                 */
                 break;
              }
              case EVTCMD_ADD_EVENT:
              {
                 /*
                 *   Text interpretation of EVTCMD_ADD_EVENT.
                 */
                 break;
              }
              case EVTCMD_REMOVE_EVENTALL:
              {
                 /*
                 *   Text interpretation of EVTCMD_REMOVE_EVENTALL.
                 */
                 break;
              }
              case EVTCMD_REMOVE_EVENTVALUE:
              {
                 /*
                 *   Text interpretation of EVTCMD_REMOVE_EVENTVALUE.
                 */
                 break;
              }
              case EVTCMD_REMOVE_EVENT:
              {
                 /*
                 *   Text interpretation of EVTCMD_REMOVE_EVENT.
                 */
                 break;
              }
              case EVTCMD_LOGSTART:
              {
                 /*
                 *   Text interpretation of EVTCMD_LOGSTART.
                 */
                 break;
              }
              case EVTCMD_LOGSTOP:
              {
                 /*
                 *   Text interpretation of EVTCMD_LOGSTOP.
                 */
                 break;
              }
              case EVTCMD_SHOW_EVENT:
              {
                 /*
                 *   Text interpretation of EVTCMD_SHOW_EVENT.
                 */
                 break;
              }
           }  /* end switch */
        }  /* end if text != NULL */
    }   /* end command processing */

    strncpy(s, tmp, size); 
    
    return SUCCESS;
    
}   /* end evtFormatMessage() */

/**/
/*-----------------------------------------------------------------------------
  evtSetTraceLevel
    - update local tracemask
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI evtSetTraceLevel( CTAHD    ctahd,
                                      void     *mgrcontext,
                                      unsigned svcid,
                                      unsigned tracemask )
{
   EVT_PERCTX *pevtPerCtx = (EVT_PERCTX *)mgrcontext;

   /* Needed by Natural Access provided error logging service */
   CTABEGIN("evtSetTraceLevel");

   ASSERT( svcid == EVT_SVCID );

   /* Set Channel specific tracemask. */
   pevtPerCtx->tracemask = tracemask;

   return SUCCESS;
   
}  /* end evtSetTraceMask() */

/**/
/*-----------------------------------------------------------------------------
  evtFormatTraceBuffer
    - convert binary trace messages to ASCII
    - responsible for service specific tracetags defined in evtsys.h
  ---------------------------------------------------------------------------*/
STATIC DWORD NMSAPI evtFormatTraceBuffer( unsigned tracetag,
                                          void     *inbuffer,
                                          unsigned insize,
                                          char     *outbuffer,
                                          unsigned outsize )
{
   DWORD ret = SUCCESS;
   
   /* Needed by Natural Access provided error logging service */
   CTABEGIN("evtFormatTraceBuffer");

   switch (tracetag)
   {
      case EVT_TRACETAG_ZERO:
      {
         /*
         *   Text interpretation of structure,
         *   defined in evtsys.h(TRACETAG_ZERO_OBJECT),
         *   associated with EVT_TRACETAG_ZERO.
         */
         break;
      }
      
      case EVT_TRACETAG_ONE:
      {
         /*
         *   Text interpretation of structure,
         *   defined in evtsys.h(TRACETAG_ONE_OBJECT),
         *   associated with EVT_TRACETAG_ONE.
         */
         break;
      }
      
      case EVT_TRACETAG_BITX:
      {
         /*
         *   Text interpretation of structure,
         *   defined in evtsys.h(TRACETAG_BITX_OBJECT),
         *   associated with EVT_TRACETAG_BITX.
         */
         break;
      }
      
      default:
      {
         ret = CTAERR_NOT_IMPLEMENTED;
         break;
      }
      
   }  /* end switch */

   return ret;
   
}  /* end evtFormatTraceBuffer() */

/**/
/*-----------------------------------------------------------------------------
  Queuecontext Creation/Destruction Routines.
  ---------------------------------------------------------------------------*/
EVT_PERQUE* evtCreatePerQue( void )
{
   EVT_PERQUE *pevtPerQue = NULL;
   
   if ((pevtPerQue = (EVT_PERQUE *)calloc(1,sizeof(EVT_PERQUE))) == NULL)
   {
      return NULL;
   }
   
   pevtPerQue->size = sizeof(EVT_PERQUE);
   pevtPerQue->evcount = 0;

   return pevtPerQue;
   
}  /* end evtCreatePerQue() */


/**/
/*---------------------------------------------------------------------------*/
void evtDestroyPerQue( EVT_PERQUE* pevtPerQue )
{
   
   free( pevtPerQue );
   
}  /* end evtDestroyPerQue() */


/**/
/*-----------------------------------------------------------------------------
  Mgrcontext Creation/Destruction Routines.
  ---------------------------------------------------------------------------*/
EVT_PERCTX *evtCreatePerCtx( CTAHD        ctahd, 
                             EVT_PERQUE   *pevtPerQue )
{
   EVT_PERCTX *pevtPerCtx = NULL;
   
   if ( pevtPerQue == NULL ||
        (pevtPerCtx=calloc(1, sizeof(EVT_PERCTX))) == NULL )
   {
      return NULL;
   }
   /* Initialize channel. */
   pevtPerCtx->size              = sizeof( EVT_PERCTX );
   pevtPerCtx->pevtPerQue        = pevtPerQue;
   pevtPerCtx->ctahd             = ctahd;
   pevtPerCtx->evcount           = 0;
   pevtPerCtx->logStarted        = 0;
   pevtPerCtx->owner             = CTA_NULL_SVCID;
   pevtPerCtx->tracemask         = (*evtTracePointer);

   return pevtPerCtx;
   
}  /* end evtCreatePerCtx() */


/**/
/*---------------------------------------------------------------------------*/
void evtDestroyPerCtx( EVT_PERCTX *pevtPerCtx )
{
   if ( pevtPerCtx == NULL )
   {
      return;
   }
   
   /* Free the channel object */
   free( pevtPerCtx );
   
}  /* end evtDestroyPerCtx() */


