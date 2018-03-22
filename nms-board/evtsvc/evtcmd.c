/*****************************************************************************
  NAME:  evtcmd.c

  PURPOSE:

      Server side implementation of the EVT service API functions.

  Copyright 2001 NMS Communications.  All rights reserved.
  ***************************************************************************/

#include "string.h" /* for memcpy, etc. */
#include "stdio.h"  /* for printf */
#include "evtsys.h"
/* #include "evtdvc.h" */
 
/*----------------------------------------------------------------------------
*  The event data observed by the EVT service is logged to a log file or
*  the standard output file, based on the application's preference.
*
*  The choice of the log file is for the entire application (and all the
*  queues and contexts under it).  In other words, evtSetLogFile() needs to
*  be called at most once per application.
*
*  The standard output file is used to log the events if evtSetLogFile()
*  is not called or is called with a NULL pointer or a null string ""
*  as the log file name.
*
*  To keep it simple in dealing with the case of identical log file name
*  chosen by multiple applications (with the intent to commingle the log),
*  the following is done for logging each event observed by the EVT service:
*
*     fopen, fprintf, fflush and fclose (in the case of using a log file)
*     or
*     printf (in the case of using the standard output file)
*
*  The event tracking criteria and the log starts and stops are also logged
*  into the log file to make it self-documenting.
*---------------------------------------------------------------------------*/
/* Maximum file name size, counting the ending null character */
#define EVT_MAX_FILE_NAME_SIZE  256

/* Choice of EVT log file: 0: stdio; 1: given file name */
static WORD evtLogFileChoice = 0;

/* Log file name */
static char evtLogFileName[EVT_MAX_FILE_NAME_SIZE];

/* Log file pointer */
static FILE *evtLogFilePtr;

/*-----------------------------------------------------------------------------
   Utility function for validating command arguments.
  ---------------------------------------------------------------------------*/
static DWORD evtValidate( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m )
{
   if ( pevtPerCtx == NULL || m == NULL )
   {
      return CTAERR_BAD_ARGUMENT;
   }
   /* Check channel owner. */
   if ( pevtPerCtx->owner != CTA_NULL_SVCID &&
        pevtPerCtx->owner != m->addr.source )
   {
      return EVTERR_OWNER_CONFLICT;
   }
   /*
   *  Set channel owner.
   *  Also used as a reply address.
   */
   if ( pevtPerCtx->owner == CTA_NULL_SVCID )
   {
      pevtPerCtx->owner = m->addr.source;
   }
   return SUCCESS;
}

/*-----------------------------------------------------------------------------
   Implementation function for evtCmdSetLogFile
  ---------------------------------------------------------------------------*/
DWORD evtCmdSetLogFile( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m )
{
   DWORD                ret = SUCCESS;
   RTC_CONDITION        condition = {0};
   RTC_ACTION           action = {0};

   /* Validate the arguments. */
   ret = evtValidate( pevtPerCtx, m );
   if ( ret != SUCCESS )
      return ret;

   /*
   *   If service parameters can be passed as argument,
   *   check if value is NULL.  If so, get context specific
   *   default value.
   */

   /*
   *  Do appropriate processing.
   */
   if (m->size1 <= 0 || m->size1 > EVT_MAX_FILE_NAME_SIZE)
   {
      evtLogFileChoice = 0; /* stdout as log file */
   }
   else
   {
      evtLogFileChoice = 1;
      strncpy( evtLogFileName, m->dataptr1, EVT_MAX_FILE_NAME_SIZE);
   }

   /*
   *  Log trace records according to active trace
   *  category(ies) specified in global tracemask
   *  or context specific tracemask.
   */

   /*
   *  If DISP_COMMAND *m is not used to return
   *  synchronous data to the application, return SUCCESS.
   *  Otherwise, set appropriate fields of DISP_COMMAND *m
   *  and return SUCCESS_RESPONSE.
   */
   return ret;
}

/*-----------------------------------------------------------------------------
   Implementation function for evtCmdAddEventAll
  ---------------------------------------------------------------------------*/
DWORD evtCmdAddEventAll( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m )
{
   DWORD                ret = SUCCESS;
   RTC_CONDITION        condition = {0};
   RTC_ACTION           action = {0};


   /* Validate the arguments. */
   ret = evtValidate( pevtPerCtx, m );
   if ( ret != SUCCESS )
      return ret;

   if (m->size1 < CTA_MAX_SVCID)
   {
      condition.Attribute = RTC_ALL_PROVIDER_EVENTS;
      condition.EventID = (m->size1 << 16) | CTA_CODE_EVENT;
   }
   else
   {
      return CTAERR_BAD_ARGUMENT;
   }
   /* Log the intended tracking criteria. */
   if (evtLogFileChoice == 1)
   {
      evtLogFilePtr = fopen(evtLogFileName, "a");
      fprintf(evtLogFilePtr,"evtAddEventAll()      CTAHD=x%08x svcid=x%04x\n",
      m->ctahd,m->size1);
      fflush(evtLogFilePtr);
      fclose(evtLogFilePtr);
   }
   else
   {
      printf("evtAddEventAll()      CTAHD=x%08x svcid=x%04x\n",
      m->ctahd,m->size1);
   }
   /*
   *   If service parameters can be passed as argument,
   *   check if value is NULL.  If so, get context specific
   *   default value.
   */

   /*
   *  Do appropriate processing.
   */
   action.Consumer = EVT_SVCID;
   action.ActionCmd = EVTCMD_SHOW_EVENT;
   dispAddRTC(m->ctahd,&condition,&action);

   return ret;
}

/*-----------------------------------------------------------------------------
   Implementation function for evtCmdAddEventValue
  ---------------------------------------------------------------------------*/
DWORD evtCmdAddEventValue( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m )
{
   DWORD                ret = SUCCESS;
   RTC_CONDITION        condition = {0};
   RTC_ACTION           action = {0};

   /* Validate the arguments. */
   ret = evtValidate( pevtPerCtx, m );
   if ( ret != SUCCESS )
      return ret;

   if (((m->size1 >> 16) < CTA_MAX_SVCID)
   &&  ((m->size1 & CTA_CODE_EVENT) == CTA_CODE_EVENT))
   {
      condition.Attribute  = RTC_EVT_AND_ONE_VALUE;
      condition.EventID    = m->size1;
      condition.EventValue = m->size2;
   }
   else
   {
      return CTAERR_BAD_ARGUMENT;
   }
   /* Log the intended tracking criteria. */
   if (evtLogFileChoice == 1)
   {
      evtLogFilePtr = fopen(evtLogFileName, "a");
      fprintf(evtLogFilePtr,"evtAddEventValue()    CTAHD=x%08x EvId=x%08x EvVal=x%08x\n",
      m->ctahd,m->size1,m->size2);
      fflush(evtLogFilePtr);
      fclose(evtLogFilePtr);
   }
   else
   {
      printf("evtAddEventValue()    CTAHD=x%08x EvId=x%08x EvVal=x%08x\n",
      m->ctahd,m->size1,m->size2);
   }
   /*
   *  Do appropriate processing.
   */
   action.Consumer = EVT_SVCID;
   action.ActionCmd = EVTCMD_SHOW_EVENT;
   dispAddRTC(m->ctahd,&condition,&action);

   return ret;
}

/*-----------------------------------------------------------------------------
   Implementation function for evtCmdAddEvent
  ---------------------------------------------------------------------------*/
DWORD evtCmdAddEvent( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m )
{
   DWORD                ret = SUCCESS;
   RTC_CONDITION        condition = {0};
   RTC_ACTION           action = {0};

   /* Validate the arguments. */
   ret = evtValidate( pevtPerCtx, m );
   if ( ret != SUCCESS )
      return ret;

   if (((m->size1 >> 16) < CTA_MAX_SVCID)
   &&  ((m->size1 & CTA_CODE_EVENT) == CTA_CODE_EVENT))
   {
      condition.Attribute  = RTC_EVT_AND_ANY_VALUE;
      condition.EventID    = m->size1;
   }
   else
   {
      return CTAERR_BAD_ARGUMENT;
   }
   /* Log the intended tracking criteria. */
   if (evtLogFileChoice == 1)
   {
      evtLogFilePtr = fopen(evtLogFileName, "a");
      fprintf(evtLogFilePtr,"evtAddEvent()         CTAHD=x%08x EvId=x%08x\n",
      m->ctahd,m->size1);
      fflush(evtLogFilePtr);
      fclose(evtLogFilePtr);
   }
   else
   {
      printf("evtAddEvent()         CTAHD=x%08x EvId=x%08x\n",
      m->ctahd,m->size1);
   }
   /*
   *  Do appropriate processing.
   */
   action.Consumer = EVT_SVCID;
   action.ActionCmd = EVTCMD_SHOW_EVENT;
   dispAddRTC(m->ctahd,&condition,&action);

   return ret;
}
/*-----------------------------------------------------------------------------
   Implementation function for evtCmdRemoveEventAll
  ---------------------------------------------------------------------------*/
DWORD evtCmdRemoveEventAll( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m )
{
   DWORD                ret = SUCCESS;
   RTC_CONDITION        condition = {0};
   RTC_ACTION           action = {0};

   /* Validate the arguments. */
   ret = evtValidate( pevtPerCtx, m );
   if ( ret != SUCCESS )
      return ret;

   if (m->size1 < CTA_MAX_SVCID)
   {
      condition.Attribute = RTC_ALL_PROVIDER_EVENTS;
      condition.EventID = (m->size1 << 16) | CTA_CODE_EVENT;
   }
   else
   {
      return CTAERR_BAD_ARGUMENT;
   }
   /*
   *   If service parameters can be passed as argument,
   *   check if value is NULL.  If so, get context specific
   *   default value.
   */
   /* Log the intended tracking criteria. */
   if (evtLogFileChoice == 1)
   {
      evtLogFilePtr = fopen(evtLogFileName, "a");
      fprintf(evtLogFilePtr,"evtRemoveEventAll()   CTAHD=x%08x svcid=x%04x\n",
      m->ctahd,m->size1);
      fflush(evtLogFilePtr);
      fclose(evtLogFilePtr);
   }
   else
   {
      printf("evtRemoveEventAll()   CTAHD=x%08x svcid=x%04x\n",
      m->ctahd,m->size1);
   }

   /*
   *  Do appropriate processing.
   */
   action.Consumer = EVT_SVCID;
   action.ActionCmd = EVTCMD_SHOW_EVENT;
   dispRemoveRTC(m->ctahd,&condition,&action);

   return ret;
}

/*-----------------------------------------------------------------------------
   Implementation function for evtCmdRemoveEventValue
  ---------------------------------------------------------------------------*/
DWORD evtCmdRemoveEventValue( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m )
{
   DWORD                ret = SUCCESS;
   RTC_CONDITION        condition = {0};
   RTC_ACTION           action = {0};

   /* Validate the arguments. */
   ret = evtValidate( pevtPerCtx, m );
   if ( ret != SUCCESS )
      return ret;

   if (((m->size1 >> 16) < CTA_MAX_SVCID)
   &&  ((m->size1 & CTA_CODE_EVENT) == CTA_CODE_EVENT))
   {
      condition.Attribute  = RTC_EVT_AND_ONE_VALUE;
      condition.EventID    = m->size1;
      condition.EventValue = m->size2;
   }
   else
   {
      return CTAERR_BAD_ARGUMENT;
   }
   /* Log the intended tracking criteria. */
   if (evtLogFileChoice == 1)
   {
      evtLogFilePtr = fopen(evtLogFileName, "a");
      fprintf(evtLogFilePtr,"evtRemoveEventValue() CTAHD=x%08x EvId=x%08x EvVal=x%08x\n",
      m->ctahd,m->size1,m->size2);
      fflush(evtLogFilePtr);
      fclose(evtLogFilePtr);
   }
   else
   {
      printf("evtRemoveEventValue() CTAHD=x%08x EvId=x%08x EvVal=x%08x\n",
      m->ctahd,m->size1,m->size2);
   }
   /*
   *  Do appropriate processing.
   */
   action.Consumer = EVT_SVCID;
   action.ActionCmd = EVTCMD_SHOW_EVENT;
   dispRemoveRTC(m->ctahd,&condition,&action);

   return ret;
}

/*-----------------------------------------------------------------------------
   Implementation function for evtCmdRemoveEvent
  ---------------------------------------------------------------------------*/
DWORD evtCmdRemoveEvent( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m )
{
   DWORD                ret = SUCCESS;
   RTC_CONDITION        condition = {0};
   RTC_ACTION           action = {0};

   /* Validate the arguments. */
   ret = evtValidate( pevtPerCtx, m );
   if ( ret != SUCCESS )
      return ret;

   if (((m->size1 >> 16) < CTA_MAX_SVCID)
   &&  ((m->size1 & CTA_CODE_EVENT) == CTA_CODE_EVENT))
   {
      condition.Attribute  = RTC_EVT_AND_ANY_VALUE;
      condition.EventID    = m->size1;
   }
   else
   {
      return CTAERR_BAD_ARGUMENT;
   }
   /* Log the intended tracking criteria. */
   if (evtLogFileChoice == 1)
   {
      evtLogFilePtr = fopen(evtLogFileName, "a");
      fprintf(evtLogFilePtr,"evtRemoveEvent()      CTAHD=x%08x EvId=x%08x\n",
      m->ctahd,m->size1);
      fflush(evtLogFilePtr);
      fclose(evtLogFilePtr);
   }
   else
   {
      printf("evtRemoveEvent()      CTAHD=x%08x EvId=x%08x\n",
      m->ctahd,m->size1);
   }
   /*
   *  Do appropriate processing.
   */
   action.Consumer = EVT_SVCID;
   action.ActionCmd = EVTCMD_SHOW_EVENT;
   dispRemoveRTC(m->ctahd,&condition,&action);

   return ret;
}
/*-----------------------------------------------------------------------------
  evtCmdApi1
    - Implementation function for evtApi1
  ---------------------------------------------------------------------------*/
DWORD evtCmdLogStart( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m )
{
   DWORD ret = SUCCESS;

   /* Validate the arguments. */
   ret = evtValidate( pevtPerCtx, m );
   if ( ret != SUCCESS )
      return ret;

   pevtPerCtx->logStarted = 1;

   /* Log the log switch call. */
   if (evtLogFileChoice == 1)
   {
      evtLogFilePtr = fopen(evtLogFileName, "a");
      fprintf(evtLogFilePtr,"evtLogStart()         CTAHD=x%08x\n",m->ctahd);
      fflush(evtLogFilePtr);
      fclose(evtLogFilePtr);
   }
   else
   {
      printf("evtLogStart()         CTAHD=x%08x\n",m->ctahd);
   }

   return ret;
}


/**/
/*-----------------------------------------------------------------------------
  evtCmdApi2
    - Implementation function for evtApi2
  ---------------------------------------------------------------------------*/
DWORD evtCmdLogStop( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m )
{
   DWORD ret = SUCCESS;

   /* Validate the arguments. */
   ret = evtValidate( pevtPerCtx, m );
   if ( ret != SUCCESS )
      return ret;

   pevtPerCtx->logStarted = 0;

   /* Log the log switch call. */
   if (evtLogFileChoice == 1)
   {
      evtLogFilePtr = fopen(evtLogFileName, "a");
      fprintf(evtLogFilePtr,"evtLogStop()          CTAHD=x%08x\n",m->ctahd);
      fflush(evtLogFilePtr);
      fclose(evtLogFilePtr);
   }
   else
   {
      printf("evtLogStop()          CTAHD=x%08x\n",m->ctahd);
   }
   return ret;
}


/**/
/*-----------------------------------------------------------------------------
  evtCmdApi3
    - Implementation function for evtApi3
  ---------------------------------------------------------------------------*/
DWORD evtCmdShowEvent( EVT_PERCTX* pevtPerCtx, DISP_COMMAND* m )
{
   DWORD ret = SUCCESS;
   RTC_EVT_CMD_DATA rtcData;

   /* Validate the arguments. */
   ret = evtValidate( pevtPerCtx, m );
   if ( ret != SUCCESS )
      return ret;

   /*
   *  Update the event counts for the queuecontext and the mgrcontext.
   *  Check if the log switch is on.
   */
   pevtPerCtx->pevtPerQue->evcount++;
   pevtPerCtx->evcount++;
   if (!pevtPerCtx->logStarted)
   {
      return SUCCESS;
   }
   /*
   *  Do appropriate processing.
   *  Retrieve the Event ID and Event Value fields.
   *  Print out the essential infomation about the event.
   */
   memcpy(&rtcData,m->dataptr1,m->size1);

   if (evtLogFileChoice == 1)
   {
      /*
         To keep it simple in dealing with possible identical log file
         names chosen by multiple applications, queues, or contexts,
         the following is done for logging each event observed by
         the EVT service:

            fopen, fprintf, fflush and fclose.
       */
      evtLogFilePtr = fopen(evtLogFileName, "a");
   
      fprintf(evtLogFilePtr,
      "evt:EvSrcSvcid=x%04x  CTAHD=x%08x EvId=x%08x EvVal=x%08x EvSize=%d #inCTX %d #inQ %d\n",
      m->addr.source,
      m->ctahd,
      rtcData.EventId,
      rtcData.EventValue,
      m->size1,
      pevtPerCtx->evcount,
      pevtPerCtx->pevtPerQue->evcount);

      fflush(evtLogFilePtr);
      fclose(evtLogFilePtr);
   }
   else
   {
      printf(
      "evt:EvSrcSvcid=x%04x  CTAHD=x%08x EvId=x%08x EvVal=x%08x EvSize=%d #inCTX %d #inQ %d\n",
      m->addr.source,
      m->ctahd,
      rtcData.EventId,
      rtcData.EventValue,
      m->size1,
      pevtPerCtx->evcount,
      pevtPerCtx->pevtPerQue->evcount);
   }

   return ret;
}
