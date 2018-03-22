/******************************************************************************
  NAME:  tikutils.c

  PURPOSE:

      Contains utility functions in support of the TIK Service Manager.
      
  Copyright (c) 1997-2002 NMS Communications.  All rights reserved.
  ****************************************************************************/

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "tiksys.h"
   

/**/
/*-----------------------------------------------------------------------------
  Event Processing from TICK Server
    - Callback invoked when a corresponding registered wait object is
      signalled.
        ctaqueuehd:  Queue on which event are to be queued.
        waitobj:     Pointer to wait object that was signalled.
        arg:         Pointer to DEVICE object (which was registered by
                     tikAttachServiceManager as a "queuecontext").
  ---------------------------------------------------------------------------*/
DWORD NMSAPI tikFetchAndProcess( CTAQUEUEHD    ctaqueuehd, 
                                 CTA_WAITOBJ  *waitobj, 
                                 void         *arg )
{
    TIK_DEVICE_OBJECT     *ptikDevice  = (TIK_DEVICE_OBJECT *) arg;
    TIK_CHANNEL_OBJECT    *ptikChannel = NULL;
    TIK_RSP_COM_OBJ       *rsp = &ptikDevice->response;
    TSIIPC_COMMAND_STATUS  status = { 0 };
    DWORD                  ret = SUCCESS;

    /* Complete IPC read. */
    if ( tikCompleteReadServerResponse( ptikDevice )
         != SUCCESS )
    {
       return ret;
    }

    /* Get Channel object pointer by indexing into the channelTbl array
     * using the client_id field.
     */
    if( rsp->client_id < 0 || rsp->client_id > 9 ||
        (ptikChannel=ptikDevice->channelTbl[rsp->client_id]) == NULL )
    {
       return TIKERR_UNKNOWN_SERVER_RESPONSE;
    }
    
    /* Log trace for server response if requested. Must check
     * both "global" tracemask and local tracemask
     */
    if ( *tikTracePointer & CTA_TRACEMASK_DRIVER_EVENTS ||
         ptikChannel->tracemask & CTA_TRACEMASK_DRIVER_EVENTS )
    {
       if( (ret=dispLogTrace( ptikChannel->ctahd,
                              TIK_SVCID,
                              CTA_TRACE_SEVERITY_INFO,
                              TIK_TRACETAG_RSP,
                              (void *)(rsp),
                              sizeof(TIK_RSP_COM_OBJ) )) != SUCCESS )
       {
          return( ret );
       }
    }

    /* Process incoming message from TICK Server.*/
    switch ( rsp->response )
    {
       case TIKSVR_OPEN_CHANNEL_RSP:
       {
          ret = tikProcessOpenChannelResponse( ptikChannel );
          break;
       }
       case TIKSVR_STOP_TIMER_RSP:
       {
          ret = tikProcessStopTimerResponse( ptikChannel );
          break;
       }
       case TIKSVR_TICK_RSP:
       {
          ret = tikProcessTickResponse( ptikChannel );
          break;
       }
       default:
       {
          /* Unknown server response. */
          ASSERT(FALSE);
          break;
       }
    }

    return ret;
    
}   /* end tikFetchAndProcess() */



/**/
/*---------------------------------------------------------------------------*/
DWORD tikProcessOpenChannelResponse( TIK_CHANNEL_OBJECT *ptikChannel )
{
   TIK_RSP_COM_OBJ *rsp        = &(ptikChannel->ptikDevice->response);
   DWORD            prev_state = ptikChannel->state;
   DWORD            reason;
   DWORD            ret;

   ptikChannel->ChannelInfo.calls     = 
   ptikChannel->ChannelInfo.ticked    = 
   ptikChannel->ChannelInfo.requested = 
   ptikChannel->ChannelInfo.remaining = 
   ptikChannel->ChannelInfo.ms        = 0;
   strcpy( ptikChannel->ChannelInfo.tick_string, "" );

   /* Translate reason code from incoming message to 
    * associated Natural Access reason.
    */
   switch ( rsp->reason )
   {
      case TIKSVR_CHANNEL_OPENED:
      {
         ptikChannel->state = CHANNEL_TIMER_IDLE;
         reason             = CTA_REASON_FINISHED;
         break;
      }
      case TIKSVR_CHANNEL_ACTIVE:
      {
         ptikChannel->state = CHANNEL_NOT_ALIVE;
         reason             = TIK_REASON_CHANNEL_ACTIVE;
         break;
      }
      case TIKSVR_CHANNEL_INVALID:
      {
         ptikChannel->state = CHANNEL_NOT_ALIVE;
         reason             = TIK_REASON_INVALID_CHANNEL;
         break;
      }
      default:
      {
         ptikChannel->state = CHANNEL_NOT_ALIVE;
         reason             = TIK_REASON_UNKNOWN_SERVER_REASON;
         break;
      }
   }

   /*  If CTA_TRACEMASK_DEBUG_BIT0 is enabled,
    *  log state transition information.
    */
   if ( TRACEMASK_BIT0_SET(ptikChannel->tracemask) )
   {
      char   txt[128] = "TIKSVR_OPEN_CHANNEL_RSP : ";
      char  *reasontxt = tikTranslateCmdRsp( rsp->reason );
      strcat( txt, reasontxt );
      if ( (ret=tikLogStateTransition( ptikChannel, 
                                       prev_state,
                                       ptikChannel->state,
                                       txt )) != SUCCESS )
      {
         return ret;
      }
   }

   /* Create and enqueue open service DONE event. */
   ret = dispMakeAndQueueEvent( ptikChannel->ctahd,
                                 CTAEVN_DISP_OPEN_SERVICE_DONE,
                                 reason,
                                 TIK_SVCID,
                                 CTA_SYS_SVCID);

   return( ret );                                    

}  /* end tikProcessOpenChannelResponse() */


/**/
/*---------------------------------------------------------------------------*/
DWORD tikProcessStopTimerResponse( TIK_CHANNEL_OBJECT *ptikChannel )
{
   TIK_RSP_COM_OBJ *rsp   = &(ptikChannel->ptikDevice->response);
   DWORD            ret;
   DWORD            reason;

   ptikChannel->ChannelInfo.remaining = 0;

   /* Convert reason code from TICK Server to appropriate
    * TIK Service reason code as defined by the TIK Service API.
    * (see tikdef.h)
    */
   if ( rsp->reason == TIKSVR_TIMER_STOPPED )  
      reason = CTA_REASON_STOPPED;
   else if ( rsp->reason == TIKSVR_TIMER_INACTIVE ) 
      reason = CTA_REASON_FINISHED;
   else
      reason = TIK_REASON_UNKNOWN_SERVER_REASON;

   /* Create and enqueue TIKEVN_TIMER_DONE event with the 
    * appropriate reason code as determined above.
    */
   if( (ret=dispMakeAndQueueEvent( ptikChannel->ctahd,
                                   TIKEVN_TIMER_DONE,
                                   reason,
                                   TIK_SVCID,
                                   ptikChannel->owner )) != SUCCESS )
   {
      return( ret );
   }
   
   else if ( ptikChannel->state == CHANNEL_CLOSE_PENDING_TIMER_STOPPING )
   {
      /* Send close channel command, and return event. */
      if ((ret=tikSendCloseChannelCommand( ptikChannel )) != SUCCESS )
      {
         return( ret );
      }
      
      /* Initiate asynchronous read of TICK Server for response to
       * Close Channel command
       */
      else if ( (ret=tikReadServerResponse( ptikChannel )) != SUCCESS )
      {
         return( ret );
      }
 
      else
      {
         /*  Make channel state transition.
          *
          *  If CTA_TRACEMASK_DEBUG_BIT0 is enabled,
          *  log state transition information.
          */
         if ( TRACEMASK_BIT0_SET(ptikChannel->tracemask) )
         {
            char   txt[128] = "TIKEVN_TIMER_DONE : ";
            char  *reasontxt = tikTranslateCmdRsp( rsp->reason );
            strcat( txt, reasontxt );
            ret = tikLogStateTransition( ptikChannel, 
                                         ptikChannel->state,
                                         CHANNEL_NOT_ALIVE,
                                         txt );
         }
         ptikChannel->state = CHANNEL_NOT_ALIVE;

         /* Create and enqueue an event to close the service */
         
         ret = dispMakeAndQueueEvent( ptikChannel->ctahd,
                                      CTAEVN_DISP_CLOSE_SERVICE_DONE,
                                      CTA_REASON_FINISHED,
                                      TIK_SVCID,
                                      CTA_SYS_SVCID );
      }
   }
      
   else if ( ptikChannel->state == CHANNEL_TIMER_STOPPING )
   {

      /* Asked to stop timer, set state.
       *
       *  If CTA_TRACEMASK_DEBUG_BIT0 is enabled,
       *  log state transition information.
       */
      if ( TRACEMASK_BIT0_SET(ptikChannel->tracemask) )
      {
         char   txt[128] = "TIKEVN_TIMER_DONE : ";
         char  *reasontxt = tikTranslateCmdRsp( rsp->reason );
         strcat( txt, reasontxt );
         ret = tikLogStateTransition( ptikChannel, 
                                      ptikChannel->state,
                                      CHANNEL_TIMER_IDLE,
                                      txt );
      }
      ptikChannel->state = CHANNEL_TIMER_IDLE;
   }
  
   return( ret );
   
}  /* end tikProcessStopTimerResponse() */


/**/
/*---------------------------------------------------------------------------*/
DWORD tikProcessTickResponse( TIK_CHANNEL_OBJECT *ptikChannel )
{
   TIK_RSP_COM_OBJ *rsp   = &(ptikChannel->ptikDevice->response);
   DWORD            ret;

   /* Update channel information. */
   ptikChannel->ChannelInfo.remaining--;
   ptikChannel->ChannelInfo.ticked++;
   
   /* Copy incoming tick string to channel object... */
   memcpy( ptikChannel->ChannelInfo.tick_string,
           rsp->msg,
           TIKSVR_MSG_SIZE );

   /* Enqueue a TIKEVN_TIMER _TICK event ... */   
   if( (ret=dispMakeAndQueueEvent( ptikChannel->ctahd,
                                   TIKEVN_TIMER_TICK,
                                   SUCCESS,
                                   TIK_SVCID, 
                                   ptikChannel->owner )) != SUCCESS )
   {
      return( ret );
   }
   
   else if ( rsp->reason == TIKSVR_TICK_WITH_DONE )
   {
      /*  Tik done, modify service channel state send done event.
       *
       *  If CTA_TRACEMASK_DEBUG_BIT0 is enabled,
       *  log state transition information.
       */
      if ( TRACEMASK_BIT0_SET(ptikChannel->tracemask) )
      {
         char   txt[128] = "TIKEVN_TIMER_TICK : ";
         char  *reasontxt = tikTranslateCmdRsp( rsp->reason );
         strcat( txt, reasontxt );
         ret = tikLogStateTransition( ptikChannel, 
                                      ptikChannel->state,
                                      CHANNEL_TIMER_IDLE,
                                      txt );
      }

      ptikChannel->state = CHANNEL_TIMER_IDLE;

      /* Since this is the last of the requested ticks,
       * also enqueue a TIKEVN_TIMER_DONE event 
       */
      ret = dispMakeAndQueueEvent( ptikChannel->ctahd,
                                   TIKEVN_TIMER_DONE,
                                   CTA_REASON_FINISHED,
                                   TIK_SVCID,
                                   ptikChannel->owner );
   }
   
   else
   {
      /*  More tiks to come, initiate next asynchronous read from server.*/
      ret = tikReadServerResponse( ptikChannel );
   }
   
   return( ret );
   
}  /* end tikProcessTickResponse() */



/**/
/*-----------------------------------------------------------------------------
  Function to translate client-server communication command and response
  to text presentation.
  ---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
char* tikTranslateCmdRsp( DWORD code )
{
   switch ( code )
   {
      TIKSVC_COMMANDS();
      TIKSVR_RESPONSES();
      TIKSVR_REASONS();
      default:
         return NULL;
   }
   
}  /* end tikTranslateCmdRsp() */



/**/
/*-----------------------------------------------------------------------------
  Function to translate service channel binary state to text presentation.
  ---------------------------------------------------------------------------*/
char* tikTranslateCtcxState( DWORD state )
{
   switch ( state )
   {
      TIK_CHANNEL_STATES();
      default:
         return NULL;
   }
   
}  /* end tikTranslateCtcxState() */


/**/
/*-----------------------------------------------------------------------------
  Function to log trace on CTA_TRACEMASK_DEBUG_BIT0 bit.
  ---------------------------------------------------------------------------*/
DWORD tikLogStateTransition( TIK_CHANNEL_OBJECT *ptikChannel,
                             DWORD prev_state,
                             DWORD next_state,
                             char *trigger )
{
   STATE_TRAN_CTCX statectcx = { 0 };
   DWORD           ret = SUCCESS;

   if ( ptikChannel == NULL )
   {
      return CTAERR_BAD_ARGUMENT;
   }

   else if ( !TRACEMASK_BIT0_SET(ptikChannel->tracemask) )
   {
      /* Don't need to log.  return SUCCESS */
      return SUCCESS;
   }

   else
   {
      statectcx.size = sizeof( STATE_TRAN_CTCX );
   
      statectcx.channel = ptikChannel->channelId;
      strcpy( statectcx.prev_state, tikTranslateCtcxState( prev_state ) );
      strcpy( statectcx.curr_state, tikTranslateCtcxState( next_state ) );
      strcpy( statectcx.agent , trigger );
   
      return ( dispLogTrace( ptikChannel->ctahd,
                             TIK_SVCID,
                             CTA_TRACE_SEVERITY_INFO,
                             TIK_TRACETAG_BIT0,
                             (void *)&statectcx,
                             sizeof(STATE_TRAN_CTCX) ) );
   }
}  /* end tikLogStateTransition() */



/**/
/*-----------------------------------------------------------------------------
  Device Creation/Destruction Routines.
  ---------------------------------------------------------------------------*/
TIK_DEVICE_OBJECT* tikCreateDeviceObject( void )
{
   DWORD i;   
   TIK_DEVICE_OBJECT *ptikDevice;
   
   if( (ptikDevice=
        (TIK_DEVICE_OBJECT *)calloc(1,sizeof(TIK_DEVICE_OBJECT))) == NULL )
   {
      return NULL;
   }
   
   ptikDevice->size        = sizeof(TIK_DEVICE_OBJECT);
   ptikDevice->readcount   = 0;

   for( i=0; i<10; i++ )
   {
      ptikDevice->channelTbl[i] = NULL;
   }
   
   return ptikDevice;
   
}  /* end tikCreateDeviceObject() */


/**/
/*---------------------------------------------------------------------------*/
void tikDestroyDeviceObject( TIK_DEVICE_OBJECT* ptikDevice )
{
   
   free( ptikDevice );
   
}  /* end tikDestroyDeviceObject() */


/**/
/*-----------------------------------------------------------------------------
  CHANNEL Creation/Destruction Routines.
  ---------------------------------------------------------------------------*/
TIK_CHANNEL_OBJECT *tikCreateChannelObject( CTAHD                ctahd, 
                                            TIK_DEVICE_OBJECT   *ptikDevice )
{
   TIK_CHANNEL_OBJECT *ptikChannel = NULL;
   
   if ( ptikDevice == NULL ||
        (ptikChannel=calloc(1, sizeof(TIK_CHANNEL_OBJECT))) == NULL )
   {
      return NULL;
   }
   /* Initialize channel. */
   ptikChannel->size              = sizeof( TIK_CHANNEL_OBJECT );
   ptikChannel->timestamp         = time( NULL );
   ptikChannel->ctahd             = ctahd;
   ptikChannel->channelId         = -1;
   ptikChannel->state             = CHANNEL_NOT_ALIVE;
   ptikChannel->owner             = CTA_NULL_SVCID;
   ptikChannel->tracemask         = (*tikTracePointer);
   ptikChannel->ptikDevice        = ptikDevice;
   ptikChannel->ChannelInfo.size  = sizeof( TIK_CHANNEL_INFO );

   return ptikChannel;
   
}  /* end tikCreateChannelObject() */


/**/
/*---------------------------------------------------------------------------*/
void tikDestroyChannelObject( TIK_CHANNEL_OBJECT *ptikChannel )
{
   if ( ptikChannel == NULL )
   {
      return;
   }
   
   /* Clear the corresponding entry in the "channelTbl" array in the
    * corresponding DEVICE object.
    */
   ptikChannel->ptikDevice->channelTbl[ptikChannel->channelId] = NULL;
 
   /* Free the channel object */
   free( ptikChannel );
   
}  /* end tikDestroyChannelObject() */


/**/
/*---------------------------------------------------------------------------*/
DWORD tikHandleCloseService( TIK_CHANNEL_OBJECT* ptikChannel )
{
   DWORD ret;

   if ( ptikChannel == NULL )
   {
      return CTAERR_BAD_ARGUMENT;
   }

   switch ( ptikChannel->state )
   {
      case CHANNEL_NOT_ALIVE:
      {
         /* Service was never opened or is already closed. */
         ret = CTAERR_INVALID_SEQUENCE;
         break;
      }
      case CHANNEL_OPENING:
      {
         /* Application did not wait for open service done. */
         ret = CTAERR_INVALID_SEQUENCE;
         break;
      }
      case CHANNEL_TIMER_IDLE:
      {
         /*  If CTA_TRACEMASK_DEBUG_BIT0 is enabled,
          *  log state transition information.
          */
         if ( TRACEMASK_BIT0_SET(ptikChannel->tracemask) )
         {
            if( (ret=
                 tikLogStateTransition( ptikChannel,
                                        ptikChannel->state,
                                        CHANNEL_NOT_ALIVE,
                                        "ctaCloseServices" )) != SUCCESS )
            {
               break;
            }
         }
         
         ptikChannel->state = CHANNEL_NOT_ALIVE;

         if ( (ret=tikSendCloseChannelCommand( ptikChannel )) != SUCCESS )
         {
            break;
         }

         ret = dispMakeAndQueueEvent( ptikChannel->ctahd,
                                      CTAEVN_DISP_CLOSE_SERVICE_DONE,
                                      CTA_REASON_FINISHED,
                                      TIK_SVCID, CTA_SYS_SVCID);
         break;
      }
      case CHANNEL_TIMER_STARTED:
      {
         /*  If CTA_TRACEMASK_DEBUG_BIT0 is enabled,
          *  log state transition information.
          */
         if ( TRACEMASK_BIT0_SET(ptikChannel->tracemask) )
         {
            if( (ret=
                 tikLogStateTransition( ptikChannel, 
                                        ptikChannel->state,
                                        CHANNEL_CLOSE_PENDING_TIMER_STOPPING,
                                        "ctaCloseServices" )) != SUCCESS )
            {
               break;
            } 
         }

         ptikChannel->state = CHANNEL_CLOSE_PENDING_TIMER_STOPPING;
         ret = tikSendStopCommand( ptikChannel );
         break;
      }
      case CHANNEL_TIMER_STOPPING:
      {
         /*  If CTA_TRACEMASK_DEBUG_BIT0 is enabled,
          *  log state transition information.
          */
         if ( TRACEMASK_BIT0_SET(ptikChannel->tracemask) )
         {
            if( (ret=
                 tikLogStateTransition( ptikChannel,
                                        ptikChannel->state,
                                        CHANNEL_CLOSE_PENDING_TIMER_STOPPING,
                                        "ctaCloseServices" )) != SUCCESS )
            {
               break;
            }
         }

         /*  ctaCloseServices has been invoke while stopping timer. */
         ptikChannel->state = CHANNEL_CLOSE_PENDING_TIMER_STOPPING;
         break;
      }
      default:
      {
         /* This should never happen. */
         ASSERT(FALSE);
         break;
      }
   }
   
   return ret;
   
}  /* end tikHandleCloseService() */
