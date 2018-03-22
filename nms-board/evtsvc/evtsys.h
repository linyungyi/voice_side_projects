/*****************************************************************************
  NAME:  evtsys.h
  
  PURPOSE:

      This file contains internal (i.e., private) definitions and function
      prototypes for the EVT Service.

  Copyright 2001 NMS Communications.  All rights reserved.
  *****************************************************************************/

#ifndef EVTSYS_INCLUDED
#define EVTSYS_INCLUDED


#include "nmstypes.h"
#include "ctadisp.h"
#include "evtdef.h"


#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------------------------------------------
  EVT Command Codes.
    - These codes are constructed using the EVT_SVCID, the Natural Access
      CTA_CODE_COMMAND typecode, and a sequence number. The formula is
          COMMANDCODE = ((EVT_SVCID<<16) | (CTA_CODE_COMMAND | SEQUENCE))
    - Command codes are used with evtProcessCommand binding function
      to invoke server side implementation function corresponding to
      application invoked API.      
  ---------------------------------------------------------------------------*/
/* evtSetLogFile() */
#define EVTCMD_SET_LOGFILE          0x004E3000

/* evtAddEventAll() */
#define EVTCMD_ADD_EVENTALL         0x004E3001
/* evtAddEventValue() */
#define EVTCMD_ADD_EVENTVALUE       0x004E3002
/* evtAddEvent() */
#define EVTCMD_ADD_EVENT            0x004E3003

/* evtRemoveEventAll() */
#define EVTCMD_REMOVE_EVENTALL      0x004E3004
/* evtRemoveEventValue() */
#define EVTCMD_REMOVE_EVENTVALUE    0x004E3005
/* evtRemoveEvent() */
#define EVTCMD_REMOVE_EVENT         0x004E3006

/* evtLogStart() */
#define EVTCMD_LOGSTART             0x004E3007
/* evtLogStop() */
#define EVTCMD_LOGSTOP              0x004E3008

/* evtShowEvent() */
#define EVTCMD_SHOW_EVENT           0x004E3009
                                

/*-----------------------------------------------------------------------------
  EVT Trace Tag Codes.
    - These codes are constructed using the evt_SVCID, the Natural Access
      CTA_CODE_TRACETAG typecode, and a sequence number. The formula is
         TRACETAGCODE = ((EVT_SVCID<<16) | (CTA_CODE_TRACETAG | SEQUENCE))
    - Tracetag definitions are utilized in evtFormatTraceBuffer to assist
      ctdaemon utility with tracing information to debug service/application.
  ---------------------------------------------------------------------------*/
#define EVT_TRACETAG_ZERO       0x004E4000
#define EVT_TRACETAG_ONE        0x004E4001
#define EVT_TRACETAG_BITX       0x004E4002



/*-----------------------------------------------------------------------------
  Structure definition associated with defined tracetag above.
    -  Structure hold information revelent to service and defined tracetag.
    -  Structure is passed to dispLogTrace for logging trace.
    -  ctdaemon invoke evtFormatTraceBuffer to acquire text translation
       of defined structure below.
  ---------------------------------------------------------------------------*/
typedef struct
{
   DWORD   size;      /* Size of this structure.                              */
                      /* Member data related to tracing data of interest.     */
}  TRACEMASK_ZERO_OBJECT;

typedef struct
{
   DWORD   size;      /* Size of this structure.                              */
                      /* Member data related to tracing data of interest.     */
}  TRACEMASK_ONE_OBJECT;

typedef struct
{
   DWORD   size;      /* Size of this structure.                              */
                      /* Member data related to tracing data of interest.     */
}  TRACEMASK_BITX_OBJECT;




/*-----------------------------------------------------------------------------
  evtGetText() Macros.
    - These macros convert EVT Service command, event, error, and reason codes
      to their corresponding ASCII macro identifier.
  ---------------------------------------------------------------------------*/

#define TEXTCASE(e) case e: return #e 

#define EVT_COMMANDS() \
TEXTCASE (EVTCMD_SET_LOGFILE); \
TEXTCASE (EVTCMD_ADD_EVENTALL); \
TEXTCASE (EVTCMD_ADD_EVENTVALUE); \
TEXTCASE (EVTCMD_ADD_EVENT); \
TEXTCASE (EVTCMD_REMOVE_EVENTALL); \
TEXTCASE (EVTCMD_REMOVE_EVENTVALUE); \
TEXTCASE (EVTCMD_REMOVE_EVENT); \
TEXTCASE (EVTCMD_LOGSTART); \
TEXTCASE (EVTCMD_LOGSTOP); \
TEXTCASE (EVTCMD_SHOW_EVENT); \

#define EVT_ERRORS() \
TEXTCASE (EVTERR_OWNER_CONFLICT); \

/* The following are not used for now but kept for future extension. */

#define EVT_EVENTS() \
TEXTCASE (EVTEVN_ONE); \
TEXTCASE (EVTEVN_TWO); \
TEXTCASE (EVTEVN_ONE_DONE); \
TEXTCASE (EVTEVN_TWO_DONE); \

#define EVT_REASONS() \
TEXTCASE (EVTREASON_ONE); \
TEXTCASE (EVTREASON_TWO); \

/*-----------------------------------------------------------------------------
  "EVT PERQUE" Object 
    - Used as a "queuecontext"
    - Object holding per-queue, EVT related information.
  ---------------------------------------------------------------------------*/
typedef struct _EVT_PERQUE
{
    DWORD           size;          /* Size of this structure                 */
    DWORD           evcount;       /* # of events tracked under all contexts */
} EVT_PERQUE;

   
/*-----------------------------------------------------------------------------
  "EVT PERCTX" Object
    - used as a "mgrcontext"
    - Object holding per-context, EVT related information.
  ---------------------------------------------------------------------------*/
typedef struct _EVT_PERCTX
{
    DWORD           size;           /* Size of this structure              */
    EVT_PERQUE      *pevtPerQue;    /* pointer to per-queue infomation     */
    CTAHD           ctahd;          /* Opening Natural Access context handle.   */
    DWORD           evcount;        /* # of events tracked under this context */
    DWORD           logStarted;     /* 0: logging started for the context  */
                                    /* 1: logging stopped for the context  */
                                    /* controled by evtCmdLogStart/Stop()  */
    DWORD           owner;          /* Current owner of this context.      
                                       This can be either calling service
                                       id or CTA_APP_SVCID.                */
    DWORD           tracemask;      /* Context specific tracemask.         */
                                    /* May add necessary service variables
                                       to be mapped with Natural Access context */
} EVT_PERCTX;


/*-----------------------------------------------------------------------------
  EVT API Implementation Function Prototypes
    - For each EVT API function, there is an associated implementation function
      that is called via evtProcessCommand().
  ---------------------------------------------------------------------------*/
/* Server Side Implementation Functions. */
DWORD evtCmdSetLogFile      ( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m );
DWORD evtCmdAddEventAll     ( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m );
DWORD evtCmdAddEventValue   ( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m );
DWORD evtCmdAddEvent        ( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m );
DWORD evtCmdRemoveEventAll  ( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m );
DWORD evtCmdRemoveEventValue( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m );
DWORD evtCmdRemoveEvent     ( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m );
DWORD evtCmdLogStart        ( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m );
DWORD evtCmdLogStop         ( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m );
DWORD evtCmdShowEvent       ( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m );

/*-----------------------------------------------------------------------------
  Other function prototyes.  Which may include
  -  Supporting functions from source files other than evtbnd.c, evtcmd.c,
     evtapi.c, and evtspi.c.(i.e. In TIK service function prototypes from
     tikcomms.c and tikutils.c )
  ---------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif
