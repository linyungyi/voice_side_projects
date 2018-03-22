/******************************************************************************
  NAME:  tikcmd.c

  PURPOSE:

      Server side implementation of the TIK service API functions.

  Copyright (c) 1997-2002 NMS Communications.  All rights reserved.
  ***************************************************************************/


#include <string.h>
#include <time.h>
#include <stdio.h>

#include "tiksys.h"


/*-----------------------------------------------------------------------------
  tikCmdStartTimer
    - Implementation function for tikStartTimer
  ---------------------------------------------------------------------------*/
DWORD tikCmdStartTimer( TIK_CHANNEL_OBJECT *ptikChannel, DISP_COMMAND* m )
{
   TIK_CMD_COM_OBJ    cmd = { 0 };
   TIK_START_PARMS    start = { 0 };
   DWORD              ret = SUCCESS;

   if ( ptikChannel == NULL || m == NULL )
   {
      return CTAERR_BAD_ARGUMENT;
   }

   /*  Check channel owner.  */
   if ( ptikChannel->owner != CTA_NULL_SVCID &&
        ptikChannel->owner != m->addr.source )
   {
      return TIKERR_OWNER_CONFLICT;
   }

   /*  channel state must be either IDLE or STARTED. */
   if ( !(ptikChannel->state == CHANNEL_TIMER_IDLE ||
          ptikChannel->state == CHANNEL_TIMER_STARTED) )
   {
      return CTAERR_INVALID_SEQUENCE;
   }

   /* Check if this function call included explicit TIK_START_PARMS
    * structure. 
    *  - If present, use passed in structure
    *  - If not present (i.e., NULL passed), get the default
    *    TIK_START_PARMS structure via dispGetParms and use it.
    */
   if ( m->size1 == 0 && m->dataptr1 == NULL )
   {
      if( (ret=
           dispGetParms( ptikChannel->ctahd, 
                         TIK_START_PARMID,
                         &start,
                         sizeof(TIK_START_PARMS) )) != SUCCESS )
      {
         return ret;
      }
   }
   else
   {
      DWORD size = ( m->size1 < sizeof(TIK_START_PARMS) )
                   ? m->size1
                   : sizeof(TIK_START_PARMS);
      memcpy( &start, m->dataptr1, size );
   }

   /* Prepare command (message buffer) to send to TICK server. */
   cmd.size      = sizeof( TIK_CMD_COM_OBJ );
   cmd.timestamp = time(NULL);
   cmd.client_id = ptikChannel->channelId;
   cmd.command   = TIKSVR_START;
   cmd.data1     = ptikChannel->channelId;
   cmd.data2     = start.Frequency;
   cmd.data3     = start.NumTicks;

   /* Update channel_info structure. */
   ptikChannel->ChannelInfo.requested += cmd.data3;

   /* If CTA_TRACEMASK_DEBUG_BIT0 is enabled,
    * log state transition information.
    */
   if ( TRACEMASK_BIT0_SET(ptikChannel->tracemask) )
   {
      ret = tikLogStateTransition( ptikChannel, 
                                   ptikChannel->state,
                                   CHANNEL_TIMER_STARTED,
                                   "tikCmdStartTimer" );
   }

   /* Set channel owner.  Also used as reply address. */
   if ( ptikChannel->owner == CTA_NULL_SVCID )
   {
      ptikChannel->owner = m->addr.source;
   }

   if( (ret=
        tikSendClientCommand( ptikChannel, &cmd )) != SUCCESS )
   {
      return ret;
   }

   /* Expecting ticks, queue an asynchronous read request. */
   if( (ret=tikReadServerResponse( ptikChannel )) != SUCCESS )
   {
      return ret;
   }

   /* Set channel state to STARTED. */
   ptikChannel->state = CHANNEL_TIMER_STARTED;

   ptikChannel->ChannelInfo.calls++;

   return SUCCESS;
   
}  /* end tikCmdStartTimer() */


/**/
/*-----------------------------------------------------------------------------
  tikCmdStopTimer
    - Implementation function for tikStopTimer
  ---------------------------------------------------------------------------*/
DWORD tikCmdStopTimer( TIK_CHANNEL_OBJECT *ptikChannel, DISP_COMMAND* m )
{
   TIK_CMD_COM_OBJ cmd = { 0 };
   DWORD           ret = 0;
   DWORD           ctr = 0;

   if ( ptikChannel == NULL || m == NULL )
   {
      return CTAERR_BAD_ARGUMENT;
   }

   /* Check channel owner. */
   if ( ptikChannel->owner != CTA_NULL_SVCID &&
        ptikChannel->owner != m->addr.source )
   {
      return TIKERR_OWNER_CONFLICT;
   }

   /* channel state must be either IDLE or STARTED, or STOPPING. */
   if ( !(ptikChannel->state == CHANNEL_TIMER_IDLE ||
          ptikChannel->state == CHANNEL_TIMER_STARTED ||
          ptikChannel->state == CHANNEL_TIMER_STOPPING) )
   {
      return CTAERR_INVALID_SEQUENCE;
   }

   /* If CTA_TRACEMASK_DEBUG_BIT0 is enabled,
    * log state transition information.
    */
   if ( TRACEMASK_BIT0_SET(ptikChannel->tracemask) )
   {
      ret = tikLogStateTransition( ptikChannel, 
                                   ptikChannel->state,
                                   CHANNEL_TIMER_STOPPING,
                                   "tikCmdStopTimer" );
   }

   /* Send a stop command to TICK server. */
   ret = tikSendStopCommand( ptikChannel );
   if ( ret != SUCCESS )
   {
      return ret;
   }

   /* Set channel owner.  Also used as reply address. */
   if ( ptikChannel->owner == CTA_NULL_SVCID )
   {
      ptikChannel->owner = m->addr.source;
   }

   /* Set channel state to STOPPING. */
   ptikChannel->state = CHANNEL_TIMER_STOPPING;

   ptikChannel->ChannelInfo.calls++;

   return SUCCESS;
   
}  /* end tikCmdStopTimer() */



/**/
/*-----------------------------------------------------------------------------
  tikCmdGetChannelInfo
    - Implementation function for tikGetChannelInfo
  ---------------------------------------------------------------------------*/
DWORD tikCmdGetChannelInfo(TIK_CHANNEL_OBJECT *ptikChannel, DISP_COMMAND* m)
{
   TIK_CHANNEL_INFO *info = NULL;
   DWORD             size;

   if ( ptikChannel == NULL || m == NULL )
   {
      return CTAERR_BAD_ARGUMENT;
   }

   /* Check channel owner. */
   if ( ptikChannel->owner != CTA_NULL_SVCID &&
        ptikChannel->owner != m->addr.source )
   {
      return TIKERR_OWNER_CONFLICT;
   }

   /* Channel state must be either IDLE or STARTED, or STOPPING. */
   if ( !(ptikChannel->state == CHANNEL_TIMER_IDLE ||
          ptikChannel->state == CHANNEL_TIMER_STARTED ||
          ptikChannel->state == CHANNEL_TIMER_STOPPING) )
   {
      return TIKERR_CHANNEL_NOT_OPENED;
   }

   /* Supports backward compatibility */
   size = (m->size1 < sizeof(TIK_CHANNEL_INFO))
          ? m->size1 : 
          sizeof(TIK_CHANNEL_INFO);

   if ( size < sizeof(DWORD) )
   {
      return CTAERR_BAD_SIZE;
   }

   info = (TIK_CHANNEL_INFO*)m->dataptr1;
   if ( info == NULL )
   {
      return CTAERR_BAD_ARGUMENT;
   }

   /* Keep track of the number of API calls made.  */
   ptikChannel->ChannelInfo.calls++;

   /*  Simple copy. Note that the returned "size" value
    *  can be used for checking the version of the service.
    */
   memcpy(info, 
          &(ptikChannel->ChannelInfo), 
          size);

   /*
   *  Assigned returned outsize value in m->size3.
   *  Assigning CTA_VIRTUAL_BUF does not destroy this
   *  return value when message propagates back to
   *  SPI.
   */
   m->size3 = size;

   /* Set channel owner.  Also used as reply address. */
   if ( ptikChannel->owner == CTA_NULL_SVCID )
   {
      ptikChannel->owner = m->addr.source;
   }

   /* Tell dispatcher we are responding with returned data. */
   return SUCCESS_RESPONSE;
   
}  /* end tikCmdGetChannelInfo() */
