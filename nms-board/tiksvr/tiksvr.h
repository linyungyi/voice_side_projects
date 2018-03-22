/*****************************************************************************
  NAME:  tiksvr.h
  
  PURPOSE:

      Internal header file for client-server communication.

  Copyright (c) 1997-2002 NMS Communications.  All rights reserved.
  *****************************************************************************/
#ifndef TIKCOM_INCLUDED
#define TIKCOM_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef TRUE
   #define TRUE   1
#endif
   
#ifndef FALSE
   #define FALSE  0
#endif
   
/*-----------------------------------------------------------------------------
  Utility macros.
  ---------------------------------------------------------------------------*/
/* Default arguments.*/
#define TIKSVR_DEFAULT_MSG                "Ticker!"

/* Text message by server to service. */
#ifndef TIKSVR_MSG_SIZE
#define TIKSVR_MSG_SIZE                   (64)
#endif

/* Server specific macros. */
#define TIKSVR_PORT_NUMBER                1667
#define TIKSVR_OVERWRITE_PORT_ENV         "NMS_TIKTST_PORT"

#define TIK_CLIENT_CONNECTION_TIMEOUT     1000

/*-----------------------------------------------------------------------------
  TIK client-to-server command identification.
  ----------------------------------------------------------------------------*/
#define TIKSVR_COMM_TEARDOWN        0x10001
#define TIKSVR_OPEN_CHANNEL		   0x10002
#define TIKSVR_CLOSE_CHANNEL		   0x10003
#define TIKSVR_START		            0x10004
#define TIKSVR_STOP        		   0x10005

/*-----------------------------------------------------------------------------
  TIK server-to-client response identification.
  ----------------------------------------------------------------------------*/
#define TIKSVR_OPEN_CHANNEL_RSP		0x20001
#define TIKSVR_STOP_TIMER_RSP		   0x20002
#define TIKSVR_TICK_RSP			      0x20003

/*-----------------------------------------------------------------------------
  TIK server-to-client reason identification
  ----------------------------------------------------------------------------*/
#define TIKSVR_CHANNEL_OPENED		   0x20010
#define TIKSVR_CHANNEL_ACTIVE		   0x20011
#define TIKSVR_CHANNEL_INVALID	   0x20012
#define TIKSVR_TIMER_STOPPED		   0x20020
#define TIKSVR_TIMER_INACTIVE		   0x20021
#define TIKSVR_TICK			         0x20030
#define TIKSVR_TICK_WITH_DONE		   0x20031

/*-----------------------------------------------------------------------------
  TIK server-to-client response communication object.
  ----------------------------------------------------------------------------*/

typedef struct _TIK_RSP_COM_OBJ
{
    
    DWORD size;                  /* Size of this structure.                  */
    DWORD timestamp;             /* Timestamp of when reponse is issued.     */
    DWORD response;              /* Response identification.                 */
    DWORD client_id;             /* Originating command client id            */
    DWORD reason;                /* Reason identification                    */
    DWORD channel;               /* Reponding channel                        */
    DWORD seq;                   /* TIK sequence(Response to start command)  */
    char  msg[TIKSVR_MSG_SIZE];  /* Tik Server Specific Message              */

} TIK_RSP_COM_OBJ;

/*-----------------------------------------------------------------------------
  TIK client-to-server command communication object.
  ----------------------------------------------------------------------------*/
/*
*                               TIK_CMD_COM_OBJ Interface
*                               -------------------------
*         Command        |  data1 contain  | data2 contain |  data3 contain
*   TIKSVR_OPEN_CHANNEL  |    channel      |   (Not Used)  |    (Not Used)
*   TIKSVR_CLOSE_CHANNEL |    channel      |   (Not Used)  |    (Not Used)
*   TIKSVR_START         |    channel      |    TikSleep   |     TikCount
*   TIKSVR_STOP          |    channel      |   (Not Used)  |    (Not Used)
*   TIKSVR_COMM_TEARDOWN |   (Not Used)    |   (Not Used)  |    (Not Used)
*/
typedef struct _TIK_CMD_COM_OBJ
{
    DWORD size;            /* Size of this structure                          */
    DWORD timestamp;       /* Timestamp of when command is issued.            */
    DWORD client_id;       /* Commanding client id                            */
    DWORD command;         /* Command identification.                         */
    DWORD data1;           /* Data 1 associated with command identification.  */
    DWORD data2;           /* Data 2 associated with command identification.  */
    DWORD data3;           /* Data 3 associated with command identification.  */
                           /* Stub variable to match size with command object.*/
    char  rsv2[TIKSVR_MSG_SIZE];
} TIK_CMD_COM_OBJ;

#ifdef __cplusplus
}
#endif

#endif

