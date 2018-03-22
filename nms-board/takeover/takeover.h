/*****************************************************************************
*  FILE: takeover.h
*
*  DESCRIPTION: A common include file for the Multi-Server, shared service
*               object demonstration programs.
*
* Copyright (c) 2001-2002 NMS Communications.  All rights reserved.
*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#if defined (UNIX)
#define _MAX_PATH   256
#define _MAX_DIR    256
#define _MAX_DRIVE  256
#define _MAX_FNAME  256
#define _MAX_EXT    128
#endif

#define MAX_SERVER_SIZE          64
#define MAX_PROTOCOL_NAME_SIZE   40
#define MAX_THREAD_NAME_SIZE     40 

#define MAX_DESCRIPTOR_SIZE     128
#define MAX_BUFFER_SIZE        1024
#define MAX_TEXT_BUFFER_SIZE    120

#define MAX_OBJ_NAME_SIZE        40 

#define MAX_KW_SIZE             100
#define MAX_KW_VALUE_SIZE       100

#define MAX_DRIVER_NAME          20

#define ENTERCRITSECT()      EnterCritSection()
#define EXITCRITSECT()       ExitCritSection()

/******************************************************************************
*
*  Support definitions for OS Independent functions for dealing with critical 
*  sections of code
*
******************************************************************************/

#if defined (UNIX)

    #include <thread.h>
  
    #include  <synch.h>
    typedef  void *        MTXHDL;      /* Mutex Sem Handle*/

      /*
      *  Solaris has no recursive mutices; we have to emulate them.
      */
      typedef struct
      {
          thread_t   tID;          /* Thread ID                           */
          int        useCount;     /* Recursion count                     */
          mutex_t    realMutex;    /* Native (non-recursive) mutex struct */
      } Rmtx;
      #define MUTEX_T                 Rmtx
      #define MUTEX_LOCK(mtx)         mutex_lock(&((mtx)->realMutex))
      #define MUTEX_UNLOCK(mtx)       mutex_unlock(&((mtx)->realMutex))

#endif /* UNIX */


/*
* Startup arguments for "takeover"
*/

typedef struct
{
    BOOL     bPrimary;
    BOOL     bVerbose;

    char     cmdHost[ MAX_SERVER_SIZE ];
    char     svrHost[ MAX_SERVER_SIZE ];
    char     protocol[ MAX_PROTOCOL_NAME_SIZE ];
    char     voiceFile[ _MAX_FNAME + _MAX_PATH ];
    char     brdName[ MAX_OBJ_NAME_SIZE ];
    DWORD    board;
    /* Caller and DSP stream:slot designations */
    char     swiDriver[ MAX_DRIVER_NAME ];
    int      inStream;
    int      inSlot;
    int      dspStream;
    int      dspSlot;
    unsigned encoding;
    char     inputFileName[ _MAX_FNAME ];

} CALL_ARGS;


/*
*  General thread management information
*/

#define NUM_TAKEOVER_THRDS 5

#define CCV_THRD_IDX  0
#define OAM_THRD_IDX  1
#define DTM_THRD_IDX  2
#define CHK_THRD_IDX  3
#define CMD_THRD_IDX  4

/*
*  Natural Access Context specific information
*/

#define MAX_SVCS_ON_THREAD 6

typedef struct
{
    BOOL              bctxReady;
    BOOL              bsvcReady;
    BOOL              battached;
    BOOL              breset;
    char              ctxName[ CTA_CONTEXT_NAME_LEN ];
    CTAQUEUEHD        ctaqueuehd;
    CTAHD             ctahd;

    char             *svcMgrList[ MAX_SVCS_ON_THREAD];     /* ctaCreateQueue */
    CTA_SERVICE_DESC  svcDescriptions[MAX_SVCS_ON_THREAD ];/* ctaOpenService */
    char             *svcNames[ MAX_SVCS_ON_THREAD ];     /* ctaCloseService */
    unsigned          numSvcs;                      /* CreateQ, OpenServices */

} CTA_THRD_INFO;

#define CMD_CNTX_NAME    "TakeCmd"
#define CHKPNT_CNTX_NAME "Exchange"

#define OAM_CNTX_NAME "TakeOverOam"
#define DTM_CNTX_NAME "TakeOverDtm"
#define CCV_CNTX_NAME "TakeOverCcV"

#define CHKPNT_HEARTBEAT_TMO    200 /* milliseconds */
#define HEARTBEAT_MISSED_P        1
#define STOP_HEARTBEATS_FOR_IDLE  2 /* #seconds to let backup see primary is gone */


#define CMD_WAIT_TMO 500 /* milliseconds */


#define OAM_TICKS_IN_SECOND 5
#define OAM_WAIT_TMO        200 
#define DTM_WAIT_TMO    200
#define CCV_WAIT_TMO    200
#define CCV_WAIT_CLOSE  3000

typedef struct
{
    BOOL	  newCmd;
    CTA_EVENT cmd;
} TAKECMD_ENTRY;


/****************************************************************************
* Application events sent through the command and checkpoint contexts
*/ 
#define     APPEVN_CMD_FOR_OAM      CTA_USER_EVENT(0x1) /* Board Management */
#define     APPEVN_CMD_FOR_CCV      CTA_USER_EVENT(0x2) /* CallControl/Voice*/
#define     APPEVN_CMD_FOR_DTM      CTA_USER_EVENT(0x3) /* Trunk Monitoring */
#define     APPEVN_CMD_FOR_GENERAL  CTA_USER_EVENT(0x4) /* Demo operations  */

#define     APPEVN_CMD_EXIT         CTA_USER_EVENT(0x5) /* application exit */
#define     APPEVN_CMD_EXIT_B       CTA_USER_EVENT(0x6) /* backup exit      */
#define     APPEVN_CMD_EXIT_P       CTA_USER_EVENT(0x7) /* primary  exit    */
#define     APPEVN_CMD_SWITCH       CTA_USER_EVENT(0x8) /* Switch backup    */

#define     APPEVN_CMD_RUNSTATE     CTA_USER_EVENT(0xD) /* show running state */

#define     APPEVN_XCHG_IDLE        CTA_USER_EVENT(0x32) /* Record Failure    */
#define     APPEVN_XCHG_HEARTBEAT   CTA_USER_EVENT(0x33) /* I'm alive         */
#define     APPEVN_XCHG_TAKEOVER    CTA_USER_EVENT(0x34) /* Backup->Primary   */

#define     APPEVN_XCHG_GET_DATA    CTA_USER_EVENT(0x35) /* state data request*/
#define     APPEVN_XCHG_DTM_DATA    CTA_USER_EVENT(0x36) /* DTM state data rsp*/
#define     APPEVN_XCHG_OAM_DATA    CTA_USER_EVENT(0x37) /* OAM state data rsp*/
#define     APPEVN_XCHG_CCV_DATA    CTA_USER_EVENT(0x38) /* CCV state data rsp*/
#define     APPEVN_XCHG_OAM         CTA_USER_EVENT(0x39) /* send OAM data     */
#define     APPEVN_XCHG_OAM_UPDATE  CTA_USER_EVENT(0x3A) /* receive OAM data  */
#define     APPEVN_XCHG_CCV         CTA_USER_EVENT(0x3B) /* send CCV data     */
#define     APPEVN_XCHG_CCV_UPDATE  CTA_USER_EVENT(0x3C) /* receive CCV data  */

#define     APPEVN_OAM_START        CTA_USER_EVENT(0x40) /* Start Board       */
#define     APPEVN_OAM_STOP         CTA_USER_EVENT(0x41) /* Stop Board        */
#define     APPEVN_OAM_GET_KW       CTA_USER_EVENT(0x42) /* Get keyword       */
#define     APPEVN_OAM_GET_STOP     CTA_USER_EVENT(0x43) /* Stop get keyword  */
#define     APPEVN_OAM_ADD          CTA_USER_EVENT(0x44) /* OAM data add      */
#define     APPEVN_OAM_REMOVE       CTA_USER_EVENT(0x45) /* OAM data delete   */
#define     APPEVN_OAM_UPDATE       CTA_USER_EVENT(0x46) /* OAM data change   */
#define     APPEVN_OAM_BOARD_STATE  CTA_USER_EVENT(0x47) /* Get state of board*/

#define     APPEVN_DTM_START        CTA_USER_EVENT(0x50) /* Start Trunk Mon   */
#define     APPEVN_DTM_STOP         CTA_USER_EVENT(0x51) /* Stop Trunk Mon    */
#define     APPEVN_DTM_GETSTATUS    CTA_USER_EVENT(0x52) /* Get Trunk Status  */

#define     APPEVN_CCV_DO_CALL      CTA_USER_EVENT(0x60) /* Establish Call    */
#define     APPEVN_CCV_PLAY_FILE    CTA_USER_EVENT(0x61) /* Play a file       */
#define     APPEVN_CCV_HANG_UP      CTA_USER_EVENT(0x62) /* Hangup call       */
#define     APPEVN_CCV_ABORT_PLAY   CTA_USER_EVENT(0x63) /* Abort file play   */
#define     APPEVN_CCV_CALLCONN     CTA_USER_EVENT(0x64) /* Call connected    */
#define     APPEVN_CCV_COLLECT      CTA_USER_EVENT(0x65) /* Collect Digits    */
#define     APPEVN_CCV_COLLECT_BEG  CTA_USER_EVENT(0x66) /* Begin collecting  */
#define     APPEVN_CCV_COLLECT_END  CTA_USER_EVENT(0x67) /* End collecting    */
#define     APPEVN_CCV_PLAY_BEG     CTA_USER_EVENT(0x68) /* Play file done    */
#define     APPEVN_CCV_PLAY_BUF     CTA_USER_EVENT(0x69) /* Play file update  */
#define     APPEVN_CCV_PROTOCOL     CTA_USER_EVENT(0x6A) /* Protocol started  */

#define     APPEVN_BOARD_STOPPED    CTA_USER_EVENT(0x80) /* Board has stopped */

/*---------------- Reasons in Application Generated Events -----------------*/
#define     APP_REASON_FINISHED                 0x0001  /* File not known   */
#define     APP_REASON_DISCONNECT               0x0002  /* File not known   */
#define     APP_REASON_UNKNOWN_FILE             0x0003  /* File not known   */
#define     APP_REASON_FUNCTION_FAILED          0x0004  /* File not known   */
#define     APP_REASON_UNKNOWN_REASON           0x0005  /* File not known   */

/****************************************************************************
*
* Takeover state information.
*
*****************************************************************************/

#define TOVR_MODE_PRIMARY 1
#define TOVR_MODE_BACKUP  2

#define TOVR_MODE_PRIMARY_TEXT  "PRIMARY"
#define TOVR_MODE_BACKUP_TEXT   "BACKUP"
#define TOVR_MODE_UNKNOWN_TEXT  "UNKNOWN"

/****************
*  DTM Specific *
****************/

typedef struct _dtm
{
    unsigned     board;
    unsigned     trunk;
    DTMHD        dtmhd;
    struct _dtm *next;
} DTM_TRUNK;

/****************
*  OAM Specific *
****************/

typedef struct
{
    DWORD   howOften;
    DWORD   numGets;
    char    objName[ MAX_OBJ_NAME_SIZE ];
    char    keyWord[ MAX_KW_SIZE ];
} OAM_CMD_KW;

typedef struct _oam
{
    DWORD        reqID;
    DWORD        numTicks;
    DWORD        currTicks;
    OAM_CMD_KW   kwReq;
    struct _oam *next;
} OAM_DATA;

typedef enum 
{ 
    CCV_CONNECTED, 
    CCV_PLAYING, 
    CCV_IDLE
} CCV_STATE;

typedef struct
{
    CCV_STATE        state;    
    NCC_CALLHD       callhd;
    BOOL             bprotocolStarted;
    FILE            *pplayFile;
    char            *pplayBuf;
    char            *pdigitBuf;
    char             voiceFile[ _MAX_FNAME + _MAX_PATH ];
    unsigned         encoding;
    unsigned         numDigits;
    unsigned         playBufSize;
    unsigned         playSize;
    unsigned         currPosition;
    NCC_ADI_START_PARMS nccadiparms;
    NCC_START_PARMS     nccparms;
    NCC_ADI_ISDN_PARMS  isdparms;

} CCV_DATA;

typedef struct
{
    CCV_STATE       state;
    BOOL            bprotocolStarted;
    char            callDescriptor[ MAX_DESCRIPTOR_SIZE ];
    char            swiDescriptor[  MAX_DESCRIPTOR_SIZE ];
    /* Current info about file being played */
    char            voiceFile[ _MAX_FNAME + _MAX_PATH ];
    unsigned        encoding;
    unsigned        bufCount;
    unsigned        playBufSize;
    unsigned        playSize;
    unsigned        currPosition;
    
} CC_CHECKPOINT;

/*****************************
*  Process state information *
******************************/

typedef enum { BRD_IDLE, BRD_BOOTED, BRD_UNKNOWN, BRD_ERROR } BRD_STATE;

typedef struct
{
    /* General running state of process */
    int         runMode;
    int         missedHeartbeat;
    BOOL        bmodeSwitch;
    BOOL        bInitialize;
    BOOL        bboardStopped;
    BRD_STATE   boardState;
    
    /* Switching */
    SWIHD       swihd;

    /* DTM specific info */
    int         numTrunks;
    DTM_TRUNK  *listTrunks;

    /* OAM specific info */
    DWORD       reqID;
    int         numOamData;
    OAM_DATA   *listOam;

    /* CCV specific info */
    CCV_DATA    voiceInfo;

} TAKEOVER_STATE;

extern CALL_ARGS gArgs;

extern void EnterCritSection( void );
extern void ExitCritSection( void );



#ifdef __cplusplus
}
#endif
