/*****************************************************************************
  NAME:  tiksys.h
  
  PURPOSE:

      This file contains internal (i.e., private) definitions and function
      prototypes for the TIK Service.

  Copyright (c) 1997-2002 NMS Communications.  All rights reserved.
  *****************************************************************************/

#ifndef TIKSYS_INCLUDED
#define TIKSYS_INCLUDED

#include "nmstypes.h"
#include "ctadisp.h"
#include "tsidef.h"
#include "tiksvr.h"
#include "tikdef.h"
#include "tikspi.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*-----------------------------------------------------------------------------
  TIK Command Codes.
    - These codes are constructed using the TIK_SVCID, the Natural Access
      CTA_CODE_COMMAND id, and a 1-based sequence number. The formula is
          COMMANDCODE = ((TIK_SVCID<<16) | (CTA_CODE_COMMAND | SEQUENCE))
    - The actual command codes are "hardcoded" for source browsing purposes. 
  ---------------------------------------------------------------------------*/

#define TIKCMD_START            0x403000    /* Represents tikStartTimer()    */
#define TIKCMD_STOP             0x403001    /* Represents tikStopTimer()     */
#define TIKCMD_GET_CHANNEL_INFO 0x403002    /* Represents tikGetChannelInfo()*/




/*-----------------------------------------------------------------------------
  TIK Trace Tag Codes.
    - These codes are constructed using the TIK_SVCID, the Natural Access
      CTA_CODE_TRACETAG id, and a 1-based sequence number. The formula is
         TRACETAGCODE = ((TIK_SVCID<<16) | (CTA_CODE_TRACETAG | SEQUENCE))
    - The actual event codes are "hardcoded" for source browsing purposes.   
  ---------------------------------------------------------------------------*/

#define TIK_TRACETAG_CMD        0x404000    /* Trace Outgoing commands       */
#define TIK_TRACETAG_RSP        0x404001    /* Trace incoming events         */
#define TIK_TRACETAG_BIT0       0x404002    /* Trace state changes- see below*/

/*-----------------------------------------------------------------------------
  Trace tag TIK_TRACETAG_BIT0 definition.
    -  TIK_TRACETAG_BIT0 demonstrates the usage of service specific tracetags 
       (as opposed to Natural Access generic tracetags).
    -  In the TIK Service, TIK_TRACETAG_BIT0 will log TIK Service state
       transitions (as determined by incoming messages from the Tick Server).,
  ---------------------------------------------------------------------------*/
/* pointer to trace word denoting what (if anything) is to be traced */
extern volatile DWORD *tikTracePointer;

#define  MSG_TXT_SIZE   128
typedef struct
{
   DWORD    size;      /* Size of this structure.                             */
   unsigned channel;   /* Reporting Channel Object                            */
                       /* Text presentation of previous state.                */
   char     prev_state[MSG_TXT_SIZE];
                       /* Text presentation of current state.                 */
   char     curr_state[MSG_TXT_SIZE];
                       /* Text presentation of agent causing this transition. */
   char   agent[MSG_TXT_SIZE];
}  STATE_TRAN_CTCX;

/*-----------------------------------------------------------------------------
  Macro to detect if TIK_TRACETAG_BIT0 is set.
  ---------------------------------------------------------------------------*/
#define TRACEMASK_BIT0_SET(x) \
    ( (((x)&CTA_TRACEMASK_DEBUG_BIT0)==CTA_TRACEMASK_DEBUG_BIT0) || \
      ((*tikTracePointer)&CTA_TRACEMASK_DEBUG_BIT0)==CTA_TRACEMASK_DEBUG_BIT0 )




/*-----------------------------------------------------------------------------
  tikGetText() Macros.
    - These macros convert TIK Service command, event, error, and reason codes
      to their corresponding ASCII macro identifier.
  ---------------------------------------------------------------------------*/

#define TEXTCASE(e) case e: return #e 

#define TIK_COMMANDS() \
TEXTCASE(TIKCMD_START); \
TEXTCASE(TIKCMD_STOP); \
TEXTCASE(TIKCMD_GET_CHANNEL_INFO); \

#define TIK_EVENTS() \
TEXTCASE(TIKEVN_TIMER_TICK); \
TEXTCASE(TIKEVN_TIMER_DONE); \

#define TIK_ERRORS() \
TEXTCASE(TIKERR_COMM_FAILURE); \
TEXTCASE(TIKERR_CHANNEL_NOT_OPENED); \
TEXTCASE(TIKERR_OWNER_CONFLICT); \
TEXTCASE(TIKERR_UNKNOWN_SERVER_RESPONSE); \
TEXTCASE(TIKERR_CAN_NOT_CONNECT); \

#define TIK_REASONS() \
TEXTCASE(TIK_REASON_INVALID_CHANNEL); \
TEXTCASE(TIK_REASON_CHANNEL_ACTIVE); \
TEXTCASE(TIK_REASON_UNKNOWN_SERVER_REASON); \




/*-----------------------------------------------------------------------------
  Macros to translate Tick Server command, response, and reason codes to
  corresponding ASCII macro identifier.
  ---------------------------------------------------------------------------*/
#define TIKSVC_COMMANDS() \
TEXTCASE(TIKSVR_COMM_TEARDOWN); \
TEXTCASE(TIKSVR_OPEN_CHANNEL); \
TEXTCASE(TIKSVR_CLOSE_CHANNEL); \
TEXTCASE(TIKSVR_START); \
TEXTCASE(TIKSVR_STOP)

#define TIKSVR_RESPONSES() \
TEXTCASE(TIKSVR_OPEN_CHANNEL_RSP); \
TEXTCASE(TIKSVR_STOP_TIMER_RSP); \
TEXTCASE(TIKSVR_TICK_RSP)

#define TIKSVR_REASONS() \
TEXTCASE(TIKSVR_CHANNEL_OPENED); \
TEXTCASE(TIKSVR_CHANNEL_ACTIVE); \
TEXTCASE(TIKSVR_CHANNEL_INVALID); \
TEXTCASE(TIKSVR_TIMER_STOPPED); \
TEXTCASE(TIKSVR_TIMER_INACTIVE); \
TEXTCASE(TIKSVR_TICK); \
TEXTCASE(TIKSVR_TICK_WITH_DONE)



/*-----------------------------------------------------------------------------
  Internal Tik Service States.
    - Define States
    - Define macros to convert state to corresponding ASCII macro identifier
  ---------------------------------------------------------------------------*/
#define CHANNEL_NOT_ALIVE                             0
#define CHANNEL_OPENING                               1
#define CHANNEL_TIMER_IDLE                            2
#define CHANNEL_TIMER_STARTED                         3
#define CHANNEL_TIMER_STOPPING                        4
#define CHANNEL_CLOSE_PENDING_TIMER_STOPPING          5

#define TIK_CHANNEL_STATES() \
TEXTCASE(CHANNEL_NOT_ALIVE); \
TEXTCASE(CHANNEL_OPENING); \
TEXTCASE(CHANNEL_TIMER_IDLE); \
TEXTCASE(CHANNEL_TIMER_STARTED); \
TEXTCASE(CHANNEL_TIMER_STOPPING); \
TEXTCASE(CHANNEL_CLOSE_PENDING_TIMER_STOPPING)



  
/*-----------------------------------------------------------------------------
  "TICK DEVICE" Object 
    - Used as a "queuecontext"
    - refers to an instance of a "ticker" which can support
      up to 10 logical channels
  ---------------------------------------------------------------------------*/

struct _TIK_CHAN;

typedef struct
{
    DWORD             size;           /* Size of this structure              */
    TSIIPC_STREAM_HD  writehd;        /* Client-to-Server write IPC handle.  */
    TSIIPC_STREAM_HD  readhd;         /* Server-to-Client read IPC handle.   */
    BOOL              pendingread;    /* Flag for pending IPC read           */
    unsigned          readcount;      /* Outstanding requested read count    */
    TIK_RSP_COM_OBJ   response;       /* Server response via comm channel    */
    struct _TIK_CHAN  *channelTbl[10];/* Array of Logical channels for this 
                                         device */
} TIK_DEVICE_OBJECT;

/*-----------------------------------------------------------------------------
  "TICK CHANNEL" Object
    - used as a "mgrcontext"
    - only one TICK CHANNEL object per Natural Access context (ctahd)
    - a TICK CHANNEL is one of the up to 10 logical channels
      supported by a TICK DEVICE
  ---------------------------------------------------------------------------*/
typedef struct _TIK_CHAN
{
   DWORD               size;          /* Size of this structure              */
   DWORD               timestamp;     /* Channel creation timestamp.         */
   CTAHD               ctahd;         /* Opening Natural Access context handle.   */
   DWORD               channelId;     /* Opened channel number.              */
   DWORD               state;         /* current state of channel.           */
   DWORD               owner;         /* Current owner of this channel.      */
   DWORD               tracemask; /* Natural Access context specific tracemask */
   TIK_DEVICE_OBJECT   *ptikDevice;   /* pointer to "tick device" object.    */
   TIK_CHANNEL_INFO    ChannelInfo;   /* channel specific information.       */
} TIK_CHANNEL_OBJECT;




/*-----------------------------------------------------------------------------
  TIK API Implementation Function Prototypes
    - For each TIK API function, there is an associated implementation function
      that is called via tikProcessCommand().
  ---------------------------------------------------------------------------*/

/* Implementation Function for tikStartTimer */
DWORD 
  tikCmdStartTimer( TIK_CHANNEL_OBJECT *ptikChannel, 
                    DISP_COMMAND       *m );
                        
/* Implementation Function for tikStopTimer */                        
DWORD
  tikCmdStopTimer( TIK_CHANNEL_OBJECT *ptikChannel,
                   DISP_COMMAND       *m );

/* Implementation Function for tikGetChannelInfo */                       
DWORD
  tikCmdGetChannelInfo( TIK_CHANNEL_OBJECT *ptikChannel,
                        DISP_COMMAND       *m);
                            


                            
/*-----------------------------------------------------------------------------
  Communication Function Prototypes
    - for communicating with TICK Service
  ---------------------------------------------------------------------------*/

DWORD 
  tikSetupIPC( TIK_DEVICE_OBJECT *ptikDevice );
  
DWORD 
  tikTeardownIPC(TIK_DEVICE_OBJECT *ptikDevice);
  
/* Function to send client command to Tick Server. */
DWORD
  tikSendClientCommand( TIK_CHANNEL_OBJECT *ptikChannel,
                        TIK_CMD_COM_OBJ    *cmd );

/* Function to send stop command to server. */
DWORD
  tikSendStopCommand( TIK_CHANNEL_OBJECT *ptikChannel );

DWORD
  tikSendCloseChannelCommand( TIK_CHANNEL_OBJECT *ptikChannel );
  
/* Function to initiate asynchronous wait for Tick Server responses. */
DWORD
  tikReadServerResponse( TIK_CHANNEL_OBJECT *ptikChannel );

/* Function to complete asynchronous wait for Tick Server responses. */
DWORD tikCompleteReadServerResponse( TIK_DEVICE_OBJECT *ptikDevice );
  
/*-----------------------------------------------------------------------------
  Event Handling Function Prototypes
    - for supporting TIK binding function operation
    - Processing incoming events from TICK Server
  ---------------------------------------------------------------------------*/

DWORD NMSAPI
  tikFetchAndProcess( CTAQUEUEHD  ctaqueuehd, 
                      CTA_WAITOBJ *waitobj, 
                      void        *arg );
                        
DWORD 
  tikProcessOpenChannelResponse( TIK_CHANNEL_OBJECT *ptikChannel );
                        
DWORD 
  tikProcessStopTimerResponse( TIK_CHANNEL_OBJECT *ptikChannel );

DWORD 
  tikProcessTickResponse( TIK_CHANNEL_OBJECT *ptikChannel );



/*-----------------------------------------------------------------------------
  Constructor / Destructor Function Prototypes
    - for supporting CHANNEL objects
    - for DEVICE objects
  ---------------------------------------------------------------------------*/

TIK_DEVICE_OBJECT  
  *tikCreateDeviceObject( void );
  
void 
  tikDestroyDeviceObject( TIK_DEVICE_OBJECT *ptikDevice );
  
TIK_CHANNEL_OBJECT 
  *tikCreateChannelObject( CTAHD               ctahd,
                           TIK_DEVICE_OBJECT   *ptikDevice );
                    
void 
  tikDestroyChannelObject( TIK_CHANNEL_OBJECT *ptikChannel );


                    
/*-----------------------------------------------------------------------------
  Miscellaneous Function Prototypes
  ---------------------------------------------------------------------------*/
  
char 
  *tikTranslateCmdRsp( DWORD code );
  
char 
  *tikTranslateCtcxState( DWORD state );  
  
/* Function to log trace on CTA_TRACEMASK_DEBUG_BIT0 bit. */
DWORD
  tikLogStateTransition( TIK_CHANNEL_OBJECT *ptikChannel, 
                         DWORD              prev_state,
                         DWORD              next_state, 
                         char              *trigger );
                         
DWORD
  tikHandleCloseService( TIK_CHANNEL_OBJECT *ptikChannel );




#ifdef __cplusplus
}
#endif

#endif

