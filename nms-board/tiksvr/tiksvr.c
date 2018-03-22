/******************************************************************************

FILE:  tiksvr.c

PURPOSE:  

"Tick" server.  See below interface explanation.

- Server waits for connection requests to.  For each requested
connection a thread is spawned.  

- Commands can be sent from the client to the server which

 * request a logical TIK channel connection with server
 ~ client command supply logical channel number
 * request start timer on a connected logical channel
 ~ client command supply number of ticks to 
 notify, and duration between ticks
 * request stop timer on a connected logical channel
 ~ client command supply logical channel number
 * request a logical TIK channel teardown with server
 ~ client command supply logical channel number
 * request an IPC tear down with server.

 - "Tick" message strings are sent from the server to the client, with 
 each tick notification.

 - When ticking is done, logical TIK channel connection is left active so 
 that another start timer request can be serviced.

 - If client sends "stop" command to the server, the server will
 stop any ticking that is taking place.

- While ticking is in place, another start command over-write previous
ticking command.

USAGE:

tiksvr [-m message]
message:      Message to send with ticks.    
(default is "TIKSVR is ticking you!")

Copyright (c) 1997-2002 NMS Communications.  All rights reserved.
*****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tsidef.h"    /* OS independent function library. */
#include "tiksvr.h"

/******************************************************************************
  Timer Object.
 *****************************************************************************/
typedef struct _TIKSVR_TIMER_OBJ
{
    unsigned Occupancy;                /* Flag for channel occupancy           */
    unsigned ReadPending;              /* Flag for pending client command read */
    unsigned Count;                    /* Requested tick count in channel      */
    unsigned Sleep;                    /* Requested tick sleep duration        */
    unsigned Sequence;                 /* Pending tick left                    */
    unsigned Elapsed;                  /* Time elapsed since last tick         */
    unsigned ClientId;                 /* Id of requesting client              */
} TIKSVR_TIMER_OBJ;


/******************************************************************************
  Maximum allowed logical channel per Communication Device Object.
 *****************************************************************************/
#define  TIKSVR_MAX_LOGICAL_CHANNEL   10

/******************************************************************************
  Server Communication-Device Object.
 *****************************************************************************/
typedef struct _TIKSVR_COMM_PAIR
{
    TSIIPC_STREAM_HD     ReadStream;  /* OS Independent Read IPC connection.  */
    TSIIPC_STREAM_HD     WriteStream; /* OS Independent Write IPC connection. */
} TIKSVR_COMM_PAIR;
typedef struct _TIKSVR_COMM_OBJ
{
    TIKSVR_COMM_PAIR     Stream;      /* OS Indepent IPC Comm. pair           */
    TIK_CMD_COM_OBJ      Command;     /* Client command object.               */
    TIK_RSP_COM_OBJ      Response;    /* Server response object.              */
    /* Channel table array associated with 
       current communication object.        */
    TIKSVR_TIMER_OBJ     ChannelMap[TIKSVR_MAX_LOGICAL_CHANNEL];
    unsigned             PendingRead; /* Flag for IPC read pending.           */
    unsigned             Disconnected; /* Client initiated a disconnect.      */
} TIKSVR_COMM_OBJ;


/******************************************************************************
  Forward Reference.
 *****************************************************************************/

static void PrintError( unsigned bSuccess, char* szApi, unsigned line );

static TIKSVR_COMM_OBJ* tikSvrAddClient( void );

static void tikSvrRemoveClient( TIKSVR_COMM_OBJ *ptikCommObject );

static unsigned tikSvrConnectChannel( TIKSVR_COMM_OBJ *ptikCommObject,
                                      unsigned ClientId, unsigned ChannelSlot );

static void tikSvrTearDowntChannel( TIKSVR_COMM_OBJ *ptikCommObject,
                                    unsigned ClientId, unsigned ChannelSlot );

static unsigned tikSvrReadCommand( TIKSVR_COMM_OBJ* ptikCommObject,
                                   unsigned *TimeLeft );

static void tikSvrSendResponse( TIKSVR_COMM_OBJ* ptikCommObject,
                                unsigned *TimeLeft );

static void tikSvrProcessChannelTable( TIKSVR_COMM_OBJ *ptikCommObject,
                                       unsigned *TimeLeft );

static void CopyMessage(char *in, char *out );

static char *tikSvrInterpretCommand( DWORD command );

static void tikSvrProcessIPCCommand( TIKSVR_COMM_OBJ *ptikCommObject,
                                     unsigned *TimeLeft );

/******************************************************************************
  Maximum client connection per server.
 *****************************************************************************/
#define  TIKSVR_MAX_CLIENTS      16

/******************************************************************************
  Client connection table.
 *****************************************************************************/
static TIKSVR_COMM_OBJ *ClientMap[TIKSVR_MAX_CLIENTS] = { 0 };

/******************************************************************************
  Panic function.  If (bSuccess) is FALSE, a panic message is printed, and
  the program exits with code 1.

bSuccess:  Value for which to check for success.
szApi:     Name of function responsible for bSuccess.  For example, 
if bSuccess was the result of calling "calloc", szApi
would be the string "calloc".
line:      Line number of in the calling function.
 ****************************************************************************/
static void PrintError( unsigned bSuccess, char* szApi, unsigned line )
{
    if ( !(bSuccess) ) 
    {
        printf( "%s: Error from %s on line %d\n", 
                __FILE__, szApi, line );
        exit( 1 );
    }
}

/******************************************************************************
  Acquire port number from environment variable(IPCPort) if available.
 *****************************************************************************/
static void tikSvrResolvePortNumber( char *IPCPort, WORD *IPCPortNumber )
{
    char *Env = NULL;
    DWORD Port = 0;

    if ( IPCPortNumber == NULL || IPCPort == NULL )
    {
        PrintError( FALSE, "InternalPortNumberResolution", __LINE__ );
    }

    Env = getenv( IPCPort );

    if ( Env == NULL )
    {
        return;
    }

    Port = atol( Env );

    PrintError( Port != 0, "InvalidPortNumberInEnv", __LINE__ );

    *IPCPortNumber = (WORD)Port;

    return;
}

/******************************************************************************
  Add a connection to ClientMap.
 *****************************************************************************/
static TIKSVR_COMM_OBJ* tikSvrAddClient( void )
{
    TIKSVR_COMM_OBJ *ptikCommObject;
    unsigned            Cnt;

    ptikCommObject = (TIKSVR_COMM_OBJ *)calloc( 1, sizeof(TIKSVR_COMM_OBJ) );
    PrintError( ptikCommObject != NULL, "calloc", __LINE__ );

    /*
     *   Initialize Comm. structure.
     */
    ptikCommObject->Response.size = sizeof( TIK_RSP_COM_OBJ );

    for ( Cnt = 0; Cnt < TIKSVR_MAX_CLIENTS; Cnt++ )
    {
        TIKSVR_COMM_OBJ *Src = ClientMap[Cnt];

        if ( Src == NULL )
        {
            ClientMap[Cnt] = ptikCommObject;
            return ptikCommObject;
        }
    }

    free( ptikCommObject );
    printf( "Cannot make any more connection.\n" );

    return NULL;
}

/******************************************************************************
  Remove client from ClientMap.
 *****************************************************************************/
static void tikSvrRemoveClient( TIKSVR_COMM_OBJ *ptikCommObject )
{
    unsigned            Cnt;

    if ( ptikCommObject == NULL )
    {
        return;
    }

    for ( Cnt = 0; Cnt < TIKSVR_MAX_CLIENTS; Cnt++ )
    {
        TIKSVR_COMM_OBJ *Src = ClientMap[Cnt];

        if ( Src == ptikCommObject )
        {
            ClientMap[Cnt] = NULL;
            free( ptikCommObject );
            return;
        }
    }

    PrintError( 0, "BadClientHandle", __LINE__ );
}

/******************************************************************************
  Perform a logical channel connection.
 *****************************************************************************/
static unsigned tikSvrConnectChannel( TIKSVR_COMM_OBJ *ptikCommObject,
                                      unsigned ClientId, unsigned ChannelSlot )
{
    TIKSVR_TIMER_OBJ  *ptikTimerObject;

    if ( ptikCommObject == NULL )
    {
        PrintError( 0, "BadArgument", __LINE__ );
    }

    if ( ChannelSlot >= TIKSVR_MAX_LOGICAL_CHANNEL )
    {
        return TIKSVR_CHANNEL_INVALID;
    }

    ptikTimerObject = &(ptikCommObject->ChannelMap[ChannelSlot]);

    if ( ptikTimerObject->Occupancy == 1 )
    {
        return TIKSVR_CHANNEL_ACTIVE;
    }
    else
    {
        ptikTimerObject->Occupancy   = 1;
        ptikTimerObject->ReadPending = 
            ptikTimerObject->Count       = 
            ptikTimerObject->Sleep       = 
            ptikTimerObject->Sequence    = 
            ptikTimerObject->Elapsed     = 0;
        ptikTimerObject->ClientId    = ClientId;
        return TIKSVR_CHANNEL_OPENED;
    }
}

/******************************************************************************
  Perform a logical channel teardown.
 *****************************************************************************/
static void tikSvrTearDowntChannel( TIKSVR_COMM_OBJ *ptikCommObject,
                                    unsigned ClientId, unsigned ChannelSlot )
{
    TIKSVR_TIMER_OBJ  *ptikTimerObject;

    if ( ptikCommObject == NULL )
    {
        PrintError( 0, "BadArgument", __LINE__ );
    }

    if ( ChannelSlot >= TIKSVR_MAX_LOGICAL_CHANNEL )
    {
        return;
    }

    ptikTimerObject = &(ptikCommObject->ChannelMap[ChannelSlot]);

    if ( ptikTimerObject->Occupancy == 1 )
    {
        ptikTimerObject->Occupancy   = 
            ptikTimerObject->ReadPending = 
            ptikTimerObject->Count       = 
            ptikTimerObject->Sleep       = 
            ptikTimerObject->Sequence    = 
            ptikTimerObject->Elapsed     = 
            ptikTimerObject->ClientId    = 0;
    }
}

/******************************************************************************
  IPC read timeout.
 ******************************************************************************/
#define TIK_READ_TIMEOUT   10

/******************************************************************************
  Possible return values for tikReadCommand(below).
 ******************************************************************************/
#define TIKSVR_COMMAND_AVAIL        0
#define TIKSVR_COMMAND_NOT_AVAIL    1
/******************************************************************************
  This function read a command structure from IPC.
 ******************************************************************************/
static unsigned tikSvrReadCommand( TIKSVR_COMM_OBJ* ptikCommObject,
                                   unsigned *TimeLeft )
{
    TSIIPC_COMMAND_STATUS Status;
    DWORD                 Ret;

    if ( ptikCommObject->PendingRead )
    {
        /*
         *   If a read is pending, query for command.
         */
        Ret = TSIIPC_WOULD_BLOCK;
    }
    else
    {
        /*
         *   Else, initiate a read.
         */
        Ret = tsiIPCReadStream( ptikCommObject->Stream.ReadStream,
                                (void *)&ptikCommObject->Command,
                                sizeof(TIK_CMD_COM_OBJ), 0, 
                                &Status.ReadBytes );
        ptikCommObject->PendingRead = TRUE;
    }

    switch ( Ret )
    {
        case TSIIPC_WOULD_BLOCK:
            {
                TSIIPC_WAIT_OBJ_HD WaitObj;

                Ret = tsiIPCGetWaitObject( 
                                          (void *)(ptikCommObject->Stream.ReadStream),
                                          &WaitObj
                                         );
                PrintError( Ret == SUCCESS, "tsiIPCGetWaitObject", __LINE__ );

                Ret = tsiIPCWait( WaitObj,TIK_READ_TIMEOUT );
                if( Ret != SUCCESS )
                {
                    *TimeLeft = ( (*TimeLeft) > TIK_READ_TIMEOUT )?
                        ( (*TimeLeft) - TIK_READ_TIMEOUT ) : 0;
                    return TIKSVR_COMMAND_NOT_AVAIL;
                }
                else
                {
                    Ret = tsiIPCCommandComplete( 
                                                (void *)ptikCommObject->Stream.ReadStream,
                                                &Status 
                                               );
                    *TimeLeft = ( (*TimeLeft) > TIK_READ_TIMEOUT )?
                        ( (*TimeLeft) - TIK_READ_TIMEOUT ) : 0;

                    PrintError( (Ret == SUCCESS ||
                                 Ret == TSIIPC_CLOSED_STREAM),
                                "tsiIPCCommandComplete", __LINE__ );


                    ptikCommObject->PendingRead = FALSE;

                    if ( Ret == TSIIPC_CLOSED_STREAM )
                    {
                        ptikCommObject->Disconnected = TRUE;
                        return TIKSVR_COMMAND_NOT_AVAIL;
                    }

                    return TIKSVR_COMMAND_AVAIL;
                }
                /* will never reach here */
                /* break; */
            }
        case SUCCESS:
            {
                return TIKSVR_COMMAND_AVAIL;
            }
        default:
            {
                PrintError( 0, "tsiIPCReadStream", __LINE__ );
            }
    }
    /*
     *  Unreached statement.
     *  Return error.
     */
    return 3;
}

/******************************************************************************
  Assumed time consumption for synchronous IPC send.
 ******************************************************************************/
#define  TIKSVR_ASSUMED_SYNC_SEND   10

/******************************************************************************
  This function send a response structure via IPC.
  return non-zero value on write failure, and 0 for success.
 ******************************************************************************/
static void tikSvrSendResponse( TIKSVR_COMM_OBJ* ptikCommObject,
                                unsigned *TimeLeft )
{

    TSIIPC_COMMAND_STATUS Status;
    DWORD                 Ret;

    /*
     *   Do not attempt to disconnect a
     *   disconnected IPC.
     */
    if ( ptikCommObject->Disconnected )
    {
        return;
    }

    Ret = tsiIPCWriteStream( ptikCommObject->Stream.WriteStream,
                             (void *)&ptikCommObject->Response,
                             sizeof(TIK_RSP_COM_OBJ), 0,
                             &Status.WrittenBytes );
    switch ( Ret )
    {
        case TSIIPC_WOULD_BLOCK:
            {
                TSIIPC_WAIT_OBJ_HD WaitObj;

                Ret = tsiIPCGetWaitObject( 
                                          (void *)(ptikCommObject->Stream.WriteStream),
                                          &WaitObj
                                         );
                PrintError( Ret == SUCCESS, "tsiIPCGetWaitObject", __LINE__ );

                Ret = tsiIPCWait( WaitObj, -1 );
                PrintError( Ret == SUCCESS, "tsiIPCWait", __LINE__ );

                Ret = tsiIPCCommandComplete(
                                            (void *)ptikCommObject->Stream.WriteStream, 
                                            &Status
                                           );

                PrintError( (Ret == SUCCESS || Ret == TSIIPC_CLOSED_STREAM),
                            "tsiIPCCommandComplete", __LINE__ );

                if ( Ret == TSIIPC_CLOSED_STREAM )
                {
                    ptikCommObject->Disconnected = TRUE;
                }

                *TimeLeft = ( (*TimeLeft) > TIKSVR_ASSUMED_SYNC_SEND )?
                    ( (*TimeLeft) - TIKSVR_ASSUMED_SYNC_SEND ) : 0;

                break;
            }
        case SUCCESS:
            {
                break;
            }
        default:
            {
                PrintError( 0, "tsiIPCWriteStream", __LINE__ );
            }
    }
}

/******************************************************************************
  Perform 1 sec update on communication device timer objects.
 *****************************************************************************/
static void tikSvrProcessChannelTable( TIKSVR_COMM_OBJ *ptikCommObject,
                                       unsigned *TimeLeft )
{
    TIKSVR_TIMER_OBJ  *ptikTimerObject;
    TIK_RSP_COM_OBJ   *Rsp;
    TIK_CMD_COM_OBJ   *Cmd;
    unsigned           Cnt;

    if ( ptikCommObject == NULL )
    {
        PrintError( 0, "BadArgument", __LINE__ );
    }

    ptikTimerObject = ptikCommObject->ChannelMap;
    Rsp             = &(ptikCommObject->Response);
    Cmd             = &(ptikCommObject->Command);

    for ( Cnt = 0; Cnt < TIKSVR_MAX_LOGICAL_CHANNEL; Cnt++ )
    {
        TIKSVR_TIMER_OBJ  *Tgt = &(ptikTimerObject[Cnt]);
        if ( Tgt != NULL && Tgt->Occupancy == 1 && Tgt->Sequence > 0 )
        {
            if ( (++Tgt->Elapsed) == Tgt->Sleep )
            {
                /*
                 *   Time to tik.
                 *   Send tik event to client.
                 */
                Tgt->Elapsed = 0;

                Tgt->Sequence--;

                Rsp->timestamp   = time(NULL);
                Rsp->response    = TIKSVR_TICK_RSP;
                Rsp->channel     = Cnt;
                Rsp->client_id   = Tgt->ClientId;
                Rsp->seq         = (Tgt->Count - Tgt->Sequence);

                /*
                 *   Print status.
                 */
                printf("Stream(0x%08X):  **TIK** : TikSequence(%i) "
                       " tikCount(%d) tikChannel(%d) tikMessage(%s).\n",
                       (&ptikCommObject->Stream), Rsp->seq,
                       Tgt->Count, Cnt, Rsp->msg );

                if ( Tgt->Sequence == 0 )
                {
                    /*
                     *   Timer is done,
                     *   Send done event.
                     */

                    /*
                     *   Print status.
                     */
                    printf("Stream(0x%08X):  **DONE** tikChannel(%d).\n",
                           (&ptikCommObject->Stream), Cnt );
                    Rsp->reason    = TIKSVR_TICK_WITH_DONE;
                }
                else
                {
                    Rsp->reason    = TIKSVR_TICK;
                }

                tikSvrSendResponse( ptikCommObject, TimeLeft );
            }
        }
    }
}

/******************************************************************************
  Copy command line supplied server message from (in) to (out) parameter.
 ****************************************************************************/
static void CopyMessage(char *in, char *out )
{
    unsigned cnt;

    cnt = strlen( in );
    if ( cnt < TIKSVR_MSG_SIZE )
    {
        strcpy( out, in );
    }
    else
    {
        strncpy( out, in, TIKSVR_MSG_SIZE );
        out[TIKSVR_MSG_SIZE-1] = '\0';
    }
}

/******************************************************************************
  Translate command to its text string presentation.
 ******************************************************************************/
static char *tikSvrInterpretCommand( DWORD command )
{
    switch ( command )
    {
        case TIKSVR_OPEN_CHANNEL:
            return "Open";
        case TIKSVR_CLOSE_CHANNEL:
            return "Close";
        case TIKSVR_START:
            return "Start";
        case TIKSVR_STOP:
            return "Stop";
        default:
            return NULL;
    }
}

/******************************************************************************
  Routine to process a received command via IPC.
 ****************************************************************************/
static void tikSvrProcessIPCCommand( TIKSVR_COMM_OBJ *ptikCommObject,
                                     unsigned *TimeLeft )
{
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
    /*
     *  Process command.
     */
    TIK_CMD_COM_OBJ    *Cmd = &ptikCommObject->Command;
    TIK_RSP_COM_OBJ    *Rsp = &ptikCommObject->Response;
    char               *CmdStr;
    unsigned            Ret;

    PrintError( Cmd->size == sizeof( TIK_CMD_COM_OBJ ),
                "IncompatibleClient", __LINE__ );

    CmdStr = tikSvrInterpretCommand( Cmd->command );

    /*
     *  If Client is disconnected.  Consider this
     *  command to be TIKSVR_COMM_TEARDOWN.
     */
    if ( ptikCommObject->Disconnected )
    {
        Cmd->command = TIKSVR_COMM_TEARDOWN;
        Cmd->timestamp =
            Cmd->data1     =
            Cmd->data2     =
            Cmd->data3     = 0;
    }

    switch ( Cmd->command )
    {
        case TIKSVR_OPEN_CHANNEL:
            {
                /*
                 *   Mark request channel busy.
                 */
                Ret = tikSvrConnectChannel( ptikCommObject, Cmd->client_id,
                                            Cmd->data1 );
                /*
                 *   Setup response structure.
                 */
                Rsp->timestamp   = time(NULL);
                Rsp->response    = TIKSVR_OPEN_CHANNEL_RSP;
                Rsp->client_id   = Cmd->client_id;
                Rsp->reason      = Ret;
                Rsp->channel     = Cmd->data1;

                /*
                 *   Print effect to stdout.
                 */
                if ( Ret == TIKSVR_CHANNEL_ACTIVE )
                {
                    printf( "Stream(0x%08X):  **OPEN**"
                            "(CHANNEL_IN_USE) TikChannel(%d) ",
                            (&ptikCommObject->Stream), Cmd->data1);
                }
                else if ( Ret == TIKSVR_CHANNEL_INVALID )
                {
                    printf( "Stream(0x%08X):  **OPEN** "
                            "(INVALID_CHANNEL) tikChannel(%d) ",
                            (&ptikCommObject->Stream), Cmd->data1 );
                }
                else if ( Ret == TIKSVR_CHANNEL_OPENED )
                {
                    printf( "Stream(0x%08X):  **OPEN** "
                            "(SUCCESS) tikChannel(%d) ",
                            (&ptikCommObject->Stream), Cmd->data1 );
                }

                tikSvrSendResponse( ptikCommObject, TimeLeft );

                break;
            }
        case TIKSVR_START:
            {
                TIKSVR_TIMER_OBJ *Timer;

                if ( Cmd->data1 > TIKSVR_MAX_LOGICAL_CHANNEL )
                {
                    printf( "Stream(0x%08X): **STOP** "
                            "(INVALID CHANNEL) TikChannel(%d) ",
                            (&ptikCommObject->Stream), Cmd->data1 );
                }
                else
                {
                    Timer = &(ptikCommObject->ChannelMap[Cmd->data1]);

                    /*
                     *   Print effect to stdout.
                     */
                    if ( Timer->Sequence != 0 )
                    {
                        printf( "Stream(0x%08X):  **RESTART** ",
                                (&ptikCommObject->Stream) );
                    }
                    else
                    {
                        printf( "Stream(0x%08X):  **START** ",
                                (&ptikCommObject->Stream) );
                    }
                    printf( "TikCount(%i) TikSleep(%i sec) "
                            "TikChannel(%d) ",
                            Cmd->data3, Cmd->data2, Cmd->data1 );

                    /*
                     *  Setup context timer.
                     */
                    Timer->Count       = Cmd->data3;
                    Timer->Sequence    = Cmd->data3;
                    /*
                     *  Convert second to millisecond.
                     */
                    Timer->Sleep       = Cmd->data2;
                }

                break;
            }
        case TIKSVR_STOP:
            {
                TIKSVR_TIMER_OBJ *Timer;

                if ( Cmd->data1 > TIKSVR_MAX_LOGICAL_CHANNEL )
                {
                    printf( "Stream(0x%08X): **STOP** "
                            "(INVALID CHANNEL) TikChannel(%d) ",
                            (&ptikCommObject->Stream), Cmd->data1 );
                }
                else
                {
                    Timer = &(ptikCommObject->ChannelMap[Cmd->data1]);

                    /*
                     *   Print effect to stdout.
                     */
                    printf( "Stream(0x%08X): **STOP** TikChannel(%d) ",
                            (&ptikCommObject->Stream), Cmd->data1 );
                    /*
                     *  Setup response structure.
                     */
                    Rsp->timestamp   = time(NULL);
                    Rsp->client_id   = Timer->ClientId;
                    Rsp->channel     = Cmd->data1;
                    Rsp->response    = TIKSVR_STOP_TIMER_RSP;

                    if ( Timer->Sequence == 0 &&
                         Timer->Count    == 0 &&
                         Timer->Sleep    == 0 )
                    {
                        Rsp->reason      = TIKSVR_TIMER_STOPPED ;
                    }
                    else
                    {
                        Rsp->reason      = TIKSVR_TIMER_INACTIVE;
                    }

                    /*
                     *  Reset timer context.
                     */
                    Timer->Sequence    = 0;
                    Timer->Count       = 0;
                    Timer->Sleep       = 0;

                    tikSvrSendResponse( ptikCommObject, TimeLeft );
                }
                break;
            }
        case TIKSVR_CLOSE_CHANNEL:
            {
                TIKSVR_TIMER_OBJ *Timer;

                if ( Cmd->data1 > TIKSVR_MAX_LOGICAL_CHANNEL )
                {
                    printf( "Stream(0x%08X): **CLOSE** "
                            "(INVALID CHANNEL) TikChannel(%d) ",
                            (&ptikCommObject->Stream), Cmd->data1 );
                    break;
                }
                else
                {
                    Timer = &(ptikCommObject->ChannelMap[Cmd->data1]);

                    printf( "Stream(0x%08X):  **CLOSE** TikChannel(%d) ",
                            (&ptikCommObject->Stream), Cmd->data1 );
                    /*
                     *  Close channel.
                     */
                    tikSvrTearDowntChannel( ptikCommObject, Cmd->client_id,
                                            Cmd->data1 );
                }
                break;
            }
        case TIKSVR_COMM_TEARDOWN:
            {
                printf( "Stream(0x%08X):  **DISCONNECT** ",
                        (&ptikCommObject->Stream) );
                tsiIPCDestroyStream( ptikCommObject->Stream.ReadStream );
                tsiIPCDestroyStream( ptikCommObject->Stream.WriteStream );
                tikSvrRemoveClient( ptikCommObject );
                break;
            }
        default:
            {
                printf( "Stream(0x%08X):  **UNKNOWN** tikCommand(0x%X) ",
                        (&ptikCommObject->Stream), Cmd->command );
                break;
            }
    }

    /*
     *  Print Command Issue Time.
     */
    printf("TimeIssued(0x%08X)\n", Cmd->timestamp);
}



#define  TIKSVR_CONNECTION_TIMEOUT  30
#define  TIKSVR_MICELL_PROCESS      100
/******************************************************************************
  Program entry point.

  -1 is returned if command line specified is wrong.
  1 is returned on all other failures.

  On successful execution, this program will run until the process is stopped
  externally, either by control break or an explicit kill request by process 
  ID.
 *****************************************************************************/
int main(int argc, char *argv[])
{
    /*
     *   Server message string.
     */
    char    Msg[TIKSVR_MSG_SIZE]  = TIKSVR_DEFAULT_MSG;

    /*
     *   Overwriting evironment variable
     *   name which contain port number.
     */
    char    IPCPort[TIKSVR_MSG_SIZE] = TIKSVR_OVERWRITE_PORT_ENV;
    WORD    IPCPortNumber = TIKSVR_PORT_NUMBER;

    /*
     *   System IPC "acceptor"
     */
    TSIIPC_ACCEPTOR_HD  Acceptor;
    TSIIPC_WAIT_OBJ_HD  WaitObj;
    /*
     *   Structure passed to threads,
     *   created when connection is established.
     */
    TIKSVR_COMM_OBJ*    ptikCommObject = NULL;

    /*
     *   Utility variable.
     */
    unsigned   ReadyConnect = FALSE;
    unsigned   MaxConnection = FALSE;
    unsigned   Ctr = 0;
    DWORD      Ret;

    /*--------------------------------*/
    /* Parse command line variables.  */
    /*--------------------------------*/
    for ( Ctr = 1; Ctr <= (unsigned)( argc-1 ); Ctr += 2 )
    {
        if ( strcmp( argv[Ctr], "-m" ) == 0 )
        {
            /*
             *  Copy tick message
             */
            CopyMessage( argv[Ctr+1], Msg );
        } 
        else
        {
            /*
             *   help on how to use this server!
             */
            printf( "tiksvr [options]\n"
                    "where options are:\n"
                    "   -m MsgStr       Message string to send to clients\n"
                    "                   with tick.\n"
                    "                   (default is \"%s\")\n"
                    "   Example: tiksvr -m tickertick\n",
                    TIKSVR_DEFAULT_MSG );
            return -1;
        }
    }

    tikSvrResolvePortNumber( IPCPort, &IPCPortNumber );

    /*
     *   Create an "Acceptor" object, and acquire system IPC.
     */
    Ret = tsiIPCCreateAcceptor( IPCPortNumber, TSIIPC_ASYNCH, &Acceptor );
    PrintError( Ret == SUCCESS, "tsiIPCCreateAcceptor", __LINE__ );

    Ret = tsiIPCGetWaitObject( (void *)(Acceptor), &WaitObj );
    PrintError( Ret == SUCCESS, "tsiIPCGetWaitObject", __LINE__ );

    puts("Server system IPC created, proceeding to wait for connection...\n");

    /*
     *   Main loop.
     */
    while(1)
    {
        unsigned            TimeLeft = 1000 - TIKSVR_MICELL_PROCESS;

        /*
         *   Pool to see if any clients are trying to connect.
         */
        if ( !ReadyConnect )
        {
            ptikCommObject = tikSvrAddClient();

            if ( ptikCommObject != NULL )
            {
                ReadyConnect  = TRUE;
                MaxConnection = FALSE;
                ptikCommObject->Response.size = sizeof( TIK_RSP_COM_OBJ );
                ptikCommObject->PendingRead   = FALSE;
                ptikCommObject->Disconnected  = FALSE;
                strcpy( (char *)ptikCommObject->Response.msg, Msg );
                /*
                 *   Note : Server read stream is client's write stream.
                 */
                Ret = tsiIPCCreateAcceptorStream(
                                                 Acceptor, 0,
                                                 &ptikCommObject->Stream.ReadStream
                                                );
                PrintError( (Ret == TSIIPC_WOULD_BLOCK),
                            "tsiIPCCreateAcceptorStream(ReadStream)", __LINE__ );
            }
            else
            {
                MaxConnection = TRUE;
            }
        }
        /*
         *   Create an "Acceptor" object acquire system IPC.
         */
        if ( !MaxConnection )
        {
            Ret = tsiIPCWait( WaitObj, TIKSVR_CONNECTION_TIMEOUT );

            TimeLeft -= TIKSVR_CONNECTION_TIMEOUT;

            if ( Ret == SUCCESS )
            {
                Ret = tsiIPCCommandComplete( (void *)Acceptor, NULL );

                PrintError( Ret == SUCCESS, "tsiIPCCommandComplete", __LINE__ );

                {
                    /*  A WriteStream connection should follow. 
                     *   Note : Server write stream is client's read stream.
                     */

                    Ret = tsiIPCCreateAcceptorStream(
                                                     Acceptor, 0,
                                                     &ptikCommObject->Stream.WriteStream
                                                    );
                    PrintError( (Ret == TSIIPC_WOULD_BLOCK),
                                "tsiIPCCreateAcceptorStream(WriteStream)", __LINE__ );

                    Ret = tsiIPCWait( WaitObj, 10000 );

                    TimeLeft -= TIKSVR_CONNECTION_TIMEOUT;

                    /*
                     *  Write IPC connection must follow. Otherwise, client
                     *  connection is considered to have failed entirely.
                     */
                    if ( Ret == SUCCESS )
                    {
                        Ret = tsiIPCCommandComplete( (void *)Acceptor, NULL );

                        PrintError( Ret == SUCCESS, "tsiIPCCommandComplete", __LINE__ );

                        printf( "Stream(0x%08X): **CONNECTED** Await open channel.\n",
                                (&ptikCommObject->Stream) );

                        ptikCommObject = NULL;

                        ReadyConnect   = FALSE;
                    }
                    else
                    {
                        tsiIPCDestroyStream( ptikCommObject->Stream.ReadStream );
                        printf( "WARNING : Client failed to connect "
                                "write stream.\n" );
                    }
                }
            }
        }


        /*
         *   For every communication channel.
         */
        for ( Ctr = 0; Ctr < TIKSVR_MAX_CLIENTS; Ctr++ )
        {
            TIKSVR_COMM_OBJ    *Src = ClientMap[Ctr];
            unsigned            CommandReceived = FALSE;

            /*
             *   If no IPC connection,
             *   then continue.
             */
            if ( Src == NULL )
            {
                continue;
            }

            /*
             *   Process 1 sec tik update.
             */
            tikSvrProcessChannelTable( Src, &TimeLeft );

            /*
             *   Poll for command.
             */
            if ( Src != ptikCommObject )
            {
                Ret = tikSvrReadCommand( Src, &TimeLeft );
                switch ( Ret )
                {
                    case TIKSVR_COMMAND_AVAIL:
                        CommandReceived = TRUE;
                        break;
                    case TIKSVR_COMMAND_NOT_AVAIL:
                        CommandReceived = FALSE;
                        break;
                    default:
                        PrintError( 0, "tikReadCommand", __LINE__ );
                        break;
                }

                if ( CommandReceived || Src->Disconnected )
                {
                    tikSvrProcessIPCCommand( Src, &TimeLeft );
                    CommandReceived = FALSE;
                }
            }
        }

        /*
         *   Sleep for the rest of 1 sec. frame.
         */
        tsiSleepMs( TimeLeft );
    } 

    /*
     *  Unreached statement...
     *  Inserted to shut of compiler warning.
     */
    return 0;
}
