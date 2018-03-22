/******************************************************************************
  NAME:  tikcomms.c

  PURPOSE:

      Contains functions for communicating with the TICK Server using TSIIPC.
      
  Copyright (c) 1997-2002 NMS Communications.  All rights reserved.
  ****************************************************************************/

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "tiksys.h"

/******************************************************************************
  Acquire port number from environment variable(IPCPort) if available.
 *****************************************************************************/
DWORD tikResolvePortNumber( char *IPCPort, WORD *IPCPortNumber )
{
   char *Env = NULL;
   DWORD Port = 0;

   if ( IPCPortNumber == NULL || IPCPort == NULL )
   {
      return TIKERR_CAN_NOT_CONNECT;
   }

   /* Acquire environmental variable. */
   Env = getenv( IPCPort );

   /* Return if environmental variable is not available. */
   if ( Env == NULL )
   {
      return SUCCESS;
   }

   /* Otherwise, translate and overwrite build-in IPC port number. */
   if ( (Port = atol( Env )) == 0 )
   {
      return TIKERR_CAN_NOT_CONNECT;
   }

   *IPCPortNumber = (WORD)Port;

   return SUCCESS;
}  /* tikResolvePortNumber() */

/**/
/*-----------------------------------------------------------------------------
  IPC Setup/Tear Down Routine.
  ---------------------------------------------------------------------------*/
DWORD tikSetupIPC( TIK_DEVICE_OBJECT *ptikDevice )
{
   TIK_CMD_COM_OBJ  cmd     = { 0 };
   WORD             port    = TIKSVR_PORT_NUMBER;
   char             env[32] = TIKSVR_OVERWRITE_PORT_ENV;
   DWORD            ret;

   if ( ptikDevice == NULL )
   {
      return CTAERR_BAD_ARGUMENT;
   }

   /* Resolve IPC port number to use. */
   ret = tikResolvePortNumber( env, &port );
   if ( ret!= SUCCESS )
   {
      return ret;
   }

   /* Establish client read and write connections to local machine. */
   /*
   *   Note : Server read stream is client's write stream.
   */
   if ( (ret=tsiIPCCreateConnectorStream(
               NULL, 
               port,
               TSIIPC_ASYNCH,
               TIK_CLIENT_CONNECTION_TIMEOUT, 
               &ptikDevice->writehd ) ) != SUCCESS )
   {
      ret = TIKERR_CAN_NOT_CONNECT;
   }
   /*
   *   Note : Server write stream is client's read stream.
   */
   else if ( (ret = tsiIPCCreateConnectorStream(
                     NULL, 
                     port,
                     TSIIPC_ASYNCH,
                     10000, 
                     &ptikDevice->readhd ) ) != SUCCESS )
   {
      tsiIPCDestroyStream( ptikDevice->writehd );
      ret = TIKERR_CAN_NOT_CONNECT;
   }

   return ret;
}  /* tikSetupIPC() */


/**/
DWORD tikTeardownIPC(TIK_DEVICE_OBJECT *ptikDevice)
{
   TIK_CMD_COM_OBJ         cmd = { 0 };
   TSIIPC_COMMAND_STATUS   status = {0};
   int                     cnt;
   DWORD                   ret;

   if ( ptikDevice == NULL )
   {
      return CTAERR_BAD_ARGUMENT;
   }

   /* Send TIKSVR_COMM_TEARDOWN prior to IPC disconnect. */
   cmd.size      = sizeof( TIK_CMD_COM_OBJ );
   cmd.timestamp = time(NULL);
   cmd.client_id = 0;
   cmd.command   = TIKSVR_COMM_TEARDOWN;
   cmd.data1     = 0;
   cmd.data2     = 0;
   cmd.data3     = 0;

   /*  Logical channel has been destroyed on at this point
    *  (i.e., the "manager context has been destroyed in
    *  tikCLoseServiceManager()). Therefore, the tikSendClientCommand
    *  function cannot be used; use low level TSIIPC calls instead.
    *
    *   Log trace if requested.
    */
   if ( *tikTracePointer & CTA_TRACEMASK_DRIVER_COMMANDS )
   {
      DWORD res;
      res = dispLogTrace( NULL_CTAHD,
                          TIK_SVCID,
                          CTA_TRACE_SEVERITY_INFO,
                          TIK_TRACETAG_CMD,
                          (void *)&cmd,
                          sizeof(TIK_CMD_COM_OBJ) );
      if ( res != SUCCESS )
      {
         return res;
      }
   }
   
   /* Send command via IPC. */
   ret = tsiIPCWriteStream( ptikDevice->writehd,
                            (void *)&cmd,
                            sizeof(TIK_CMD_COM_OBJ),
                            0,
                            &cnt );
   switch ( ret )
   {
      case TSIIPC_WOULD_BLOCK:
         {
            DWORD res;
            TSIIPC_WAIT_OBJ_HD waitobj;
            
            if( (res=
                 tsiIPCGetWaitObject( 
                    (void *)(ptikDevice->writehd),
                    &waitobj )) != SUCCESS )
            {
               return CTAERR_OS_INTERNAL_ERROR;
            }

            else if( (res=
                      tsiIPCWait( 
                        waitobj,
                        TIK_CLIENT_CONNECTION_TIMEOUT )) != SUCCESS )
            {
               return TIKERR_COMM_FAILURE;
            }

            else if( (ret=
                      tsiIPCCommandComplete(
                        (void *)(ptikDevice->writehd),
                        &status )) != SUCCESS )
            {
               return CTAERR_OS_INTERNAL_ERROR;
            }
            break;
         }
      default:
         {
            return TIKERR_COMM_FAILURE;
         }
   }

   /* Destroy client read and write stream to complete IPC teardown. */
   tsiIPCDestroyStream( ptikDevice->writehd );
   tsiIPCDestroyStream( ptikDevice->readhd );

   return SUCCESS;
   
}  /* end tikTeardownIPC() */



/**/
/*****************************************************************************
  Utility function to send a command via IPC synchronously.
  A Natural Access error code is returned to indicate success status.
  ***************************************************************************/
DWORD tikSendClientCommand( TIK_CHANNEL_OBJECT *ptikChannel,
                            TIK_CMD_COM_OBJ    *cmd )
{
   int                     cnt;
   DWORD                   ret;
   TSIIPC_COMMAND_STATUS   status = {0};

   if ( cmd == NULL || ptikChannel == NULL )
   {
      return CTAERR_BAD_ARGUMENT;
   }

   /* Log trace if requested. Must check
    * both "global" tracemask and local tracemask
    */
   if ( *tikTracePointer & CTA_TRACEMASK_DRIVER_COMMANDS ||
         ptikChannel->tracemask & CTA_TRACEMASK_DRIVER_COMMANDS )
   {
      if( (ret=dispLogTrace( ptikChannel->ctahd,
                          TIK_SVCID,
                          CTA_TRACE_SEVERITY_INFO,
                          TIK_TRACETAG_CMD,
                          (void *)cmd,
                          sizeof(TIK_CMD_COM_OBJ) )) != SUCCESS )
      {
         return ret;
      }
   }
   
   /* Send command via IPC. */
   ret = tsiIPCWriteStream( ptikChannel->ptikDevice->writehd,
                            (void *)cmd, 
                            sizeof(TIK_CMD_COM_OBJ),
                            0,
                            &cnt );
   switch ( ret )
   {
      case TSIIPC_WOULD_BLOCK:
         {
            TSIIPC_WAIT_OBJ_HD waitobj;
            if( (ret=
                 tsiIPCGetWaitObject( 
                   (void *)(ptikChannel->ptikDevice->writehd),
                   &waitobj )) != SUCCESS )
            {
               return CTAERR_OS_INTERNAL_ERROR;
            }

            if( (tsiIPCWait(
                   waitobj,
                   TIK_CLIENT_CONNECTION_TIMEOUT )) != SUCCESS )
            {
               return TIKERR_COMM_FAILURE;
            }

            if( (ret=
                 tsiIPCCommandComplete(
                   (void *)(ptikChannel->ptikDevice->writehd),
                   &status )) != SUCCESS )
            {
               return CTAERR_OS_INTERNAL_ERROR;
            }
            break;
         }
      default:
         {
            return TIKERR_COMM_FAILURE;
         }
   }

   return SUCCESS;
}  /* end tikSendClientCommand() */


/**/
/*----------------------------------------------------------------------------
  Queues a tick request asynchronously, given a particular 
  Channel object.  A Natural Access success status is returned.

      ptikContextObject:  Pointer to a tick channel object for which to 
                          request a tick from the server.
  --------------------------------------------------------------------------*/
DWORD tikReadServerResponse( TIK_CHANNEL_OBJECT *ptikChannel )
{

   if ( ptikChannel == NULL )
   {
      return CTAERR_BAD_ARGUMENT;
   }

   if ( (ptikChannel->ptikDevice->readcount > 0) )
   {
      if ( ++(ptikChannel->ptikDevice->readcount) == 0xFFFF )
      {
         /* Too many pending reads. */
         return TIKERR_COMM_FAILURE;
      }
   }
   else
   {
      TIK_RSP_COM_OBJ       *rsp = &(ptikChannel->ptikDevice->response);
      DWORD                  ret;
      int                    cnt;

      ptikChannel->ptikDevice->readcount++;

      ret = tsiIPCReadStream( ptikChannel->ptikDevice->readhd,
                              (void *)rsp,
                              sizeof(TIK_RSP_COM_OBJ),
                              TIK_CLIENT_CONNECTION_TIMEOUT,
                              &cnt );
      switch ( ret )
      {
         case TSIIPC_WOULD_BLOCK:
         case SUCCESS:
            break;
         default:
            return TIKERR_COMM_FAILURE;
      }
   }

   return SUCCESS;
   
}  /* end tikReadServerResponse() */

/**/
/*----------------------------------------------------------------------------
  Complete an asynchronous read on given a particular 
  Channel object.  A Natural Access success status is returned.

      ptikDevice:  Pointer to a tick device object for which to 
                   request a tick from the server.
  --------------------------------------------------------------------------*/
DWORD tikCompleteReadServerResponse( TIK_DEVICE_OBJECT *ptikDevice )
{
   TSIIPC_COMMAND_STATUS  status = { 0 };

   if ( ptikDevice == NULL )
   {
      return CTAERR_BAD_ARGUMENT;
   }

   if ( ptikDevice->readcount == 0 )
   {
      return CTAERR_BAD_ARGUMENT;
   }

   /* Complete IPC read. */
   if ( tsiIPCCommandComplete( ptikDevice->readhd, &status )
        != SUCCESS )
   {
      return TIKERR_COMM_FAILURE;
   }

   if ( --(ptikDevice->readcount) != 0 )
   {
      TIK_RSP_COM_OBJ       *rsp = &(ptikDevice->response);
      DWORD                  ret;
      int                    cnt;

      ptikDevice->readcount++;

      ret = tsiIPCReadStream( ptikDevice->readhd,
                              (void *)rsp,
                              sizeof(TIK_RSP_COM_OBJ),
                              TIK_CLIENT_CONNECTION_TIMEOUT,
                              &cnt );
      switch ( ret )
      {
         case TSIIPC_WOULD_BLOCK:
         case SUCCESS:
            break;
         default:
            return TIKERR_COMM_FAILURE;
      }
   }

   return SUCCESS;
   
}  /* end tikCompleteReadServerResponse() */

/**/
/*----------------------------------------------------------------------------
  Sends a TIK_CMD_STOP over the communication channel.
  --------------------------------------------------------------------------*/
DWORD tikSendStopCommand( TIK_CHANNEL_OBJECT *ptikChannel )
{
   DWORD ret;
   TIK_CMD_COM_OBJ cmd = { 0 };

   if ( ptikChannel == NULL )
   {
      return CTAERR_BAD_ARGUMENT;
   }

   /* Send stop command to TIK server. */
   cmd.size      = sizeof(TIK_CMD_COM_OBJ);
   cmd.timestamp = time(NULL);
   cmd.client_id = ptikChannel->channelId;
   cmd.command   = TIKSVR_STOP;
   cmd.data1     = ptikChannel->channelId;
   cmd.data2     = 0;
   cmd.data3     = 0;

   ret = tikSendClientCommand( ptikChannel, &cmd );

   if ( ret != SUCCESS )
   {
      return ret;
   }

   return( tikReadServerResponse( ptikChannel ) );
   
}  /* end tikSendStopCommand() */


/**/
/*-----------------------------------------------------------------------------
  Graceful Service Close Routine.
  ---------------------------------------------------------------------------*/
DWORD tikSendCloseChannelCommand( TIK_CHANNEL_OBJECT *ptikChannel )
{
   TIK_CMD_COM_OBJ cmd = { 0 };
   
   /* Send TIKSVR_CLOSE_CHANNEL request. */
   cmd.size      = sizeof( TIK_CMD_COM_OBJ );
   cmd.timestamp = time(NULL);
   cmd.client_id = ptikChannel->channelId;
   cmd.command   = TIKSVR_CLOSE_CHANNEL;
   cmd.data1     = ptikChannel->channelId;
   cmd.data2     = 0;
   cmd.data3     = 0;

   return tikSendClientCommand( ptikChannel, &cmd );
   
}  /*end tikSendCloseChannelCommand() */



