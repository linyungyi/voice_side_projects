/*****************************************************************************
* FILE:        hccbnd.cpp
*
* DESCRIPTION:
*              This file contains the:
*
*                - ncc service manager binding functions
*****************************************************************************/

#include "hccsys.h"
#include "hccLineObject.h"
#include "hostTcp.h"
#include "wnkHostTcp.h"

/* define the alias of the events used in runtime control */
RTC_ALIAS _nccRtcEvtAliases[] =
{
    { NCCEVN_READY_FOR_MEDIA,     "Media_Ready" },
    { NCCEVN_NOT_READY_FOR_MEDIA, "Media_Not_Ready" }

};

#define NUM_NCC_ALIASES (sizeof(_nccRtcEvtAliases) / sizeof(RTC_ALIAS))

/*-----------------------------------------------------------------------------
 * hccDefineService()
 * Register NCC service. Actions performed include:
 * - declaring version number of service
 * - declaring compatibility level of service
 * - declaring parameter tables (standard, extension, and/or both).
 * - For this NCC demo service manager (hccmgr), no parameters are used
 *---------------------------------------------------------------------------*/
DWORD NMSAPI
    hccDefineService ( char* svcname )
{
    DWORD                   ret = SUCCESS;
    static CTAINTREV_INFO   hccSvcRevInfo = { 0 };

    /* Needed by error logging service provided by Natural Access */
    CTABEGIN( "hccDefineService" );

    /* Set service revision info. */
    hccSvcRevInfo.size         = sizeof(CTAINTREV_INFO);
    hccSvcRevInfo.majorrev     = HCC_MAJORREV;
    hccSvcRevInfo.minorrev     = HCC_MINORREV;
    hccSvcRevInfo.expapilevel  = NCCAPI_COMPATLEVEL;
    hccSvcRevInfo.expspilevel  = NCCSPI_COMPATLEVEL;
    hccSvcRevInfo.reqdisplevel = DISP_COMPATLEVEL;
    strcpy (hccSvcRevInfo.builddate, __DATE__);

    /* The most important phase of initializing a service! */
    ret = dispRegisterService( "NCC",               /* svcname      */
                               NCC_SVCID,           /* svrid        */
                               "HCCMGR",            /* svcmgrname   */
                               &hccSvcRevInfo,      /* svrRevInfo   */
                               NULL,                /* required service*/
                               0,                   /* #of required services */
                               NULL,                /* standard paramaters not used */
                               NULL);               /* extparmdesc not used */

    if ( ret != SUCCESS )
    {
        return CTALOGERROR(NULL_CTAHD, ret, NCC_SVCID);
    }

    return SUCCESS;

}   /* end hccDefineService() */



/*----------------------------------------------------------------------------
 * hccOpenServiceManager()
 * Allocate a ncc service context
 ----------------------------------------------------------------------------*/
DWORD NMSAPI
    hccOpenServiceManager (
        CTAHD       ctahd,         /* <in> parm */
        void*       queuecontext,  /* <in> parm */
        void**      mgrcontext)    /* <out> parm */
{
    DWORD             ret = SUCCESS;

    CTABEGIN ("hccOpenServiceManager");


    /*create NCC service context,which is an instance of class HccMgrContext*/
    HccMgrContext* hccMgrContext = new HccMgrContext( ctahd );
    if ( hccMgrContext == NULL )
        return CTAERROR( ctahd, CTAERR_OUT_OF_MEMORY ) ;

    *mgrcontext = (void *) hccMgrContext;

    return ret;
}   /* end hccOpenServiceManager() */


/*----------------------------------------------------------------------------
 * hccOpenService()
 * - Do NCC service intialization
 * - Setup mvip address in line status
 * - Register the alias for the events used in runtime control
 *--------------------------------------------------------------------------*/
DWORD NMSAPI
    hccOpenService (
        CTAHD           ctahd,
        void            *mgrcontext,
        char            *svcname,
        unsigned        svcid,
        CTA_MVIP_ADDR   *mvipaddr,
        CTA_SERVICE_ARGS *svccargs )
{
    CTABEGIN("hccOpenService");
    ASSERT( mgrcontext != NULL );
    DWORD ret = SUCCESS;

    HccLineObject* lineObject = ((HccMgrContext* ) mgrcontext)->getLineObject();
    memcpy(&(lineObject->getLineStatus()->port), mvipaddr, sizeof (CTA_MVIP_ADDR));


    for ( unsigned i = 0; i < NUM_NCC_ALIASES; i++ )
    {
        if ( (ret = dispRegisterRTCAlias( ctahd, &_nccRtcEvtAliases[i] ))
                  != SUCCESS )
        {
            delete (HccMgrContext*) mgrcontext;
            mgrcontext = NULL;
            return CTAERROR(ctahd, ret);
        }
    }

    dispMakeAndQueueEvent(ctahd,
                          CTAEVN_DISP_OPEN_SERVICE_DONE, CTA_REASON_FINISHED,
                          svcid, CTA_SYS_SVCID);
    return SUCCESS;

}

/*----------------------------------------------------------------------------
 * hccCloseServiceManager()
 * - Close Service
 * - Free local ncc service context
 *--------------------------------------------------------------------------*/
DWORD NMSAPI
    hccCloseServiceManager (
        CTAHD       ctahd,
        void*       mgrcontext )
{

    CTABEGIN ("hccCloseServiceManger");
    ASSERT( mgrcontext != NULL );

    HccMgrContext* hccMgrContext = (HccMgrContext*) mgrcontext;
    DWORD ret = SUCCESS;

    delete hccMgrContext;
    hccMgrContext = NULL;

    return SUCCESS;

}   /* end hccCloseServiceManager() */

/*----------------------------------------------------------------------------
 * hccCloseService()
 * - Close service
 * - Free local objects
 * - Unregister the alias for the events used in runtime control
 * - Enqueue close service done event
 *--------------------------------------------------------------------------*/
DWORD NMSAPI
    hccCloseService (
        CTAHD       ctahd,
        void        *mgrcontext,
        char        *svcname,
        unsigned    svcid )
{
    CTABEGIN ("hccCloseService" );
    DISP_COMMAND cmd = { 0 };
    DWORD ret = SUCCESS;

    /* unregister broadcast events */
    for ( unsigned i = 0; i < NUM_NCC_ALIASES; i++ )
    {
        if ( (ret = dispUnregisterRTCAlias( ctahd, &_nccRtcEvtAliases[i] ))
                  != SUCCESS )
        {
            return CTAERROR(ctahd, ret);
        }
    }

    HccMgrContext* hccMgrContext = (HccMgrContext*) mgrcontext;
    HostTcp* hostTcp = hccMgrContext->getHostTcp();
    if ( hostTcp != NULL )
    {
        /*
         * When a protocol is running, the protocol object must be deleted
         * when closing service.
         * For enhancement, a service writer may consider the state of the
         * protocol and invoke corresponding adi function to stop the funtions
         * running in ADI
         */
        delete hostTcp;
        hccMgrContext->setHostTcp( NULL );
    }

    dispMakeAndQueueEvent(ctahd,
                          CTAEVN_DISP_CLOSE_SERVICE_DONE,
                          CTA_REASON_FINISHED,
                          svcid, CTA_SYS_SVCID);
    return SUCCESS;


}
/*----------------------------------------------------------------------------
 * hccProcessEvent()
 * - Performs the logic to process ADI events
 *--------------------------------------------------------------------------*/
DWORD NMSAPI
    hccProcessEvent (
        CTAHD           ctahd,
        void*           mgrcontext,
        DISP_EVENT*     ctaevt )
{
    ASSERT( mgrcontext != NULL );
    ASSERT( ctaevt->ctahd == ctahd );

    DWORD ret;

    CTABEGIN ("hccProcessEvent");

    HccMgrContext* hccMgr = (HccMgrContext*) mgrcontext;
    ASSERT (hccMgr->getLineHd() == ctahd );

    HostTcp* hostTcp = hccMgr->getHostTcp();

    // When hostTcp is equal to NULL, no protocol is started yet.
    // In this case, no ADI event can be handled.
    if (hostTcp == NULL )
        return CTALOGERROR( ctahd,
                            CTAERR_INVALID_STATE,
                            NCC_SVCID );

    //all ADI events are dispatched to the protocol object to handle,
    ret = hostTcp->handleAdiEvt( ctaevt );

    if ( ret == SUCCESS )
    {
        switch ( ctaevt->id )
        {
            /* If the ADI evnet evolves to NCCEVN_STOP_RPOTOCOL_DONE
             * the protocol object (hostTcp) should be deleted.
             */
            case NCCEVN_STOP_PROTOCOL_DONE:
                delete hostTcp;
                hccMgr->setHostTcp( NULL );
                break;

            /* If protocol is not started successfully, delete the
             * protocol object
             */
            case NCCEVN_START_PROTOCOL_DONE:
                if (ctaevt->value != CTA_REASON_FINISHED )
                {
                    delete hostTcp;
                    hccMgr->setHostTcp( NULL );
                }
                break;
            default:
                break;
        }
        return SUCCESS;
    }
    else
        return CTAERROR( ctahd, ret );
}

/*----------------------------------------------------------------------------
 * hccProcessCommand()
 * - process ncc cmd issued by an applicaiton
 *--------------------------------------------------------------------------*/
DWORD NMSAPI
    hccProcessCommand (
        CTAHD           ctahd,
        void*           mgrcontext,
        DISP_COMMAND*   ctacmd )
{
    ASSERT( mgrcontext != NULL );
    ASSERT( CTA_HIWORD( ctacmd->id ) == NCC_SVCID );
    CTABEGIN ("hccProcessCommand");

    DWORD ret = SUCCESS;
    /* type cast NCC service context */
    HccMgrContext* hccMgr = (HccMgrContext*) mgrcontext;

    ASSERT (hccMgr->getLineHd() == ctahd );

    HostTcp* hostTcp = hccMgr->getHostTcp();
    HccLineObject* lineObject = hccMgr->getLineObject();
    ASSERT( lineObject != NULL );

    switch ( ctacmd->id )
    {
        case NCCCMD_START_PROTOCOL:
        /* specially handle NCCCMD_START_PROTOCOL command */
        {
            /* if hostTcp is not equal to NULL, there's a protocol running already
             * should return an error
             */
            if ( hostTcp != NULL )
                return CTALOGERROR( ctahd, CTAERR_INVALID_STATE, NCC_SVCID );
            else
            {
                /* check the passed-in argument */
                if ( ctacmd->dataptr1 == NULL )
                    return CTALOGERROR ( ctahd, CTAERR_BAD_ARGUMENT, NCC_SVCID );

                /* Depends on the protocol name, create a protocol object.
                 * Only wnk protocol is supported in this demo, return error
                 * if application is trying to start other protocols.
                 *
                 * For enhancement: If a new protocol is implemented, the
                 * following three lines can be modified to include the new protocol.
                 */
                if ( strcmp( "wnk0", (char*) ctacmd->dataptr1 ) )
                    return CTAERR_NOT_IMPLEMENTED;
                hostTcp = new WnkHostTcp( ctahd, lineObject );

                if ( hostTcp == NULL )
                    return CTALOGERROR( ctahd, CTAERR_OUT_OF_MEMORY, NCC_SVCID );

                /* set the pointer of the newly created protocol object
                 * in NCC service context
                 */
                hccMgr->setHostTcp( hostTcp );

                /* send NCCCMD_START_PROTOCOL to protocol object to handle */
                ret = hostTcp->handleNccCmd( ctacmd );
                if ( ret != SUCCESS )
                {
                    delete hostTcp;   //delete hostTCP if starting protocol fails
                    hccMgr->setHostTcp( NULL );
                    return CTALOGERROR( ctahd, ret, NCC_SVCID );
                }
                else
                    return SUCCESS;   //protocol started successfully
            }
        }
        /* The following NCC cmds are handled by line object */
        case NCCCMD_GET_LINE_STATUS:
            ret = lineObject->getLineStatus( ctacmd );
            return (ret == SUCCESS) ? SUCCESS_RESPONSE : CTALOGERROR( ctahd, ret, NCC_SVCID );
        case NCCCMD_GET_CALL_STATUS:
            ret = lineObject->getCallStatus( ctacmd );
            return (ret == SUCCESS) ? SUCCESS_RESPONSE : CTALOGERROR( ctahd, ret, NCC_SVCID );
        case NCCCMD_QUERY_CAPABILITY:
            ret = lineObject->getCapability( ctacmd );
            return (ret == SUCCESS) ? SUCCESS_RESPONSE : CTALOGERROR( ctahd, ret, NCC_SVCID );
        case NCCCMD_GET_EXTENDED_CALL_STATUS:
            //nccGetExtendedCallStatus() is not implemented in this demo.
            return CTAERR_NOT_IMPLEMENTED;
        default: //send other NCC cmds to protocol object to handle
        {
            if ( hostTcp == NULL )      // no protocol running, return error
                return CTALOGERROR( ctahd, CTAERR_INVALID_STATE, NCC_SVCID );
            ret = hostTcp->handleNccCmd( ctacmd );

            if ( ret != SUCCESS )
                return CTALOGERROR( ctahd, ret, NCC_SVCID );
            return SUCCESS;
        }
    }

}   /* end hccProcessCommand() */


/*----------------------------------------------------------------------------
 * hccGetText()
 * - Returns a string with text description of the input ncc code.
 *--------------------------------------------------------------------------*/
const char* NMSAPI
    hccGetText ( unsigned code )
{
    switch (code)
    {
       NCC_EVENTS();           /* see nccsys.h */
       NCC_REASONS();          /* see nccsys.h */
       NCC_ERRORS();           /* see nccsys.h */
       NCC_COMMANDS();         /* see nccsys.h */
       CTA_GENERIC_ERRORS();   /* see ctaerr.h */
       CTA_GENERIC_REASONS();  /* see ctaerr.h */
       default:
          return NULL ;
    }

}  /* end hccGetText() */


/*----------------------------------------------------------------------------
 * hccFormatMessage()
 *--------------------------------------------------------------------------*/
DWORD NMSAPI
    hccFormatMessage (
      DISP_MESSAGE*         pmsg,
      char*                 s,          /* dispatcher buffer */
      unsigned              size,       /* buf size          */
      char*                 indent)     /* indent            */
{
    CTABEGIN ("hccFormatMessage");
    ASSERT (s != NULL && size >= 500);

    if (CTA_IS_EVENT(pmsg->id))
    {
        _hccFormatEvent((DISP_EVENT *)pmsg, s, size, indent);
    }
    else if (CTA_IS_COMMAND (pmsg->id) )/* must be a command */
    {
        _hccFormatCommand((DISP_EVENT *)pmsg, s, size, indent);
    }
    else if (CTA_IS_ERROR( pmsg->id ) )
    {
        _hccFormatError((DISP_EVENT *)pmsg, s, size, indent);
    }

    return SUCCESS;

}   /* end hccFormatMessage() */


/*----------------------------------------------------------------------------
 * hccFormatEvent()
 * - Print information about an NCC event.
----------------------------------------------------------------------------*/
void _hccFormatEvent( DISP_EVENT *pevent,
                            char *printbuf,            /* dispatcher buffer */
                            unsigned pbsize,          /* buf size          */
                            char *indent)              /* indent            */
{
        /* ??? WE ASSUME THAT THE PRINT BUFFER SIZE IS BIG ENOUGH ??? */
    const char* eventtext = NULL;
    const char* valuetext = NULL;
    const char* sizetext = NULL;

    char buffer[16];

    /* Buffer should be at least this long to be able to
       format the entire event string. */
    unsigned minbsize = 0;

    minbsize =
        NCC_MAX_EVENT_STRLEN + (2 * NCC_MAX_VALUE_STRLEN) + 12 +
        ((indent == NULL) ? 0 : strlen(indent));

    /* Don't do anything if the buffer is too small. */
    if (pbsize < minbsize)
    {
        /* Buffer can be a little smaller if no information is in the
           size field for the event. */
        if (pevent->buffer == NULL &&
            pevent->size == 0)
        {
            if (pbsize < (minbsize - NCC_MAX_VALUE_STRLEN - 2))
                return;
        }
        else
            return;
    }

    /* Get the event & value text. */
    eventtext = hccGetText((unsigned)pevent->id);
    switch (pevent->id)
    {
    case NCCEVN_START_PROTOCOL_DONE:
    case NCCEVN_STOP_PROTOCOL_DONE:
        valuetext = hccGetText((unsigned)pevent->value);
        break;

    case NCCEVN_CALL_RETRIEVED:
    {
        /* Pretend a CALL_DISCONNECTED event to get the valuetext. */
        pevent->id = NCCEVN_CALL_DISCONNECTED;
        valuetext = _hccGetValueText(pevent, buffer);
        pevent->id = NCCEVN_CALL_RETRIEVED;
        break;
    }

    default:
        valuetext = _hccGetValueText(pevent, buffer);
    }

    /* Get the size text if necessary. */
    if (pevent->buffer == NULL &&
        pevent->size != 0)
    {
        sizetext = _hccGetSizeText(pevent);
    }

    if (eventtext != NULL)
    {
        if (valuetext != NULL)
        {
            /* Print text for both event and value. */
            sprintf(printbuf, "%sEvent: %s, %s\n",
                    indent, eventtext, valuetext);

            /* Tack the size field text to the end if necessary. */
            if (sizetext != NULL)
                sprintf(printbuf + strlen(printbuf), ", %s", sizetext);
        }
        else
        {
            if (pevent->value != 0)
            {
                /* Application may want to see a non-zero value code. */
                sprintf(printbuf, "%sEvent: %s, Value: 0x%08x\n",
                        indent, eventtext, pevent->value);
            }
            else
            {
                /* Print event without value code. */
                sprintf(printbuf, "%sEvent: %s\n",
                        indent, eventtext);
            }
        }
    }
    else
    {
        /* Unknown NCC event. Should be a protocol-specific event. */
        sprintf(printbuf, "%sEvent: NCC 0x%08x, Value: 0x%08x\n",
                indent, pevent->id, pevent->value);
    }

    return;
}   /* end _hccFormatEvent() */



/*----------------------------------------------------------------------------
 * _hccGetValueText()
 *  Returns a string with text description of the input ncc code.
 *  Buffer is used to print numbers for certain events. If this buffer
 *  is used, it is returned.
 *--------------------------------------------------------------------------*/
char *
 _hccGetValueText( DISP_EVENT* pevent, char* buffer )
{
    switch (pevent->id)
    {
    case NCCEVN_ACCEPTING_CALL:
    {
        switch (pevent->value)
        {
            _TEXTCASE_(NCC_ACCEPT_PLAY_RING);
            _TEXTCASE_(NCC_ACCEPT_PLAY_SILENT);
            _TEXTCASE_(NCC_ACCEPT_USER_AUDIO);

        default: return NULL;
        }
    }

    case NCCEVN_BILLING_SET:
    {
        switch (pevent->value)
        {
            _TEXTCASE_(NCC_BILLINGSET_DEFAULT);
            _TEXTCASE_(NCC_BILLINGSET_FREE);

        default:
            sprintf(buffer, "%i", pevent->value);
            return buffer;
        }
    }

    case NCCEVN_BILLING_INDICATION:
    {
        sprintf(buffer, "%i", pevent->value);
        return buffer;
    }

    case NCCEVN_BLOCK_FAILED:
    {
        switch (pevent->value)
        {
            _TEXTCASE_(NCC_BLOCK_TIMEOUT);
            _TEXTCASE_(NCC_BLOCK_OUT_OF_SEQUENCE);
            _TEXTCASE_(NCC_BLOCK_CAPABILITY_ERROR);
        default: return NULL;
        }
    }

    case NCCEVN_CALLS_BLOCKED:
    {
        switch (pevent->value)
        {
            _TEXTCASE_(NCC_BLOCK_REJECTALL);
            _TEXTCASE_(NCC_BLOCK_OUT_OF_SERVICE);

        default: return NULL;
        }
    }

    case NCCEVN_CALL_CONNECTED:
    {
        switch (pevent->value)
        {
            _TEXTCASE_(NCC_CON_ANSWERED);
            _TEXTCASE_(NCC_CON_CED);
            _TEXTCASE_(NCC_CON_DIALTONE_DETECTED);
            _TEXTCASE_(NCC_CON_PROCEEDING);
            _TEXTCASE_(NCC_CON_RING_BEGIN);
            _TEXTCASE_(NCC_CON_RING_QUIT);
            _TEXTCASE_(NCC_CON_SIGNAL);
            _TEXTCASE_(NCC_CON_SIT_DETECTED);
            _TEXTCASE_(NCC_CON_TIMEOUT);
            _TEXTCASE_(NCC_CON_VOICE_BEGIN);
            _TEXTCASE_(NCC_CON_VOICE_END);
            _TEXTCASE_(NCC_CON_VOICE_EXTENDED);
            _TEXTCASE_(NCC_CON_VOICE_LONG);
            _TEXTCASE_(NCC_CON_VOICE_MEDIUM);

        default: return NULL;
        }
    }

    case NCCEVN_CALL_DISCONNECTED:
    {
        switch (pevent->value)
        {
            _TEXTCASE_(NCC_DIS_BUSY);
            _TEXTCASE_(NCC_DIS_CED);
            _TEXTCASE_(NCC_DIS_CLEARDOWN_TONE);
            _TEXTCASE_(NCC_DIS_REORDER);
            _TEXTCASE_(NCC_DIS_DIAL_FAILURE);
            _TEXTCASE_(NCC_DIS_DIALTONE);
            _TEXTCASE_(NCC_DIS_GLARE);
            _TEXTCASE_(NCC_DIS_HOST_TIMEOUT);
            _TEXTCASE_(NCC_DIS_INCOMING_FAULT);
            _TEXTCASE_(NCC_DIS_NO_ACKNOWLEDGEMENT);
            _TEXTCASE_(NCC_DIS_NO_CS_RESOURCE);
            _TEXTCASE_(NCC_DIS_NO_DIALTONE);
            _TEXTCASE_(NCC_DIS_NO_LOOP_CURRENT);
            _TEXTCASE_(NCC_DIS_REJECT_REQUESTED);
            _TEXTCASE_(NCC_DIS_REMOTE_ABANDONED);
            _TEXTCASE_(NCC_DIS_REMOTE_NOANSWER);
            _TEXTCASE_(NCC_DIS_RING_BEGIN);
            _TEXTCASE_(NCC_DIS_RING_QUIT);
            _TEXTCASE_(NCC_DIS_SIGNAL);
            _TEXTCASE_(NCC_DIS_UNASSIGNED_NUMBER);
            _TEXTCASE_(NCC_DIS_SIGNAL_UNKNOWN);
            _TEXTCASE_(NCC_DIS_SIT_DETECTED);
            _TEXTCASE_(NCC_DIS_TIMEOUT);
            _TEXTCASE_(NCC_DIS_TRANSFER);
            _TEXTCASE_(NCC_DIS_VOICE_BEGIN);
            _TEXTCASE_(NCC_DIS_VOICE_END);
            _TEXTCASE_(NCC_DIS_VOICE_EXTENDED);
            _TEXTCASE_(NCC_DIS_VOICE_LONG);
            _TEXTCASE_(NCC_DIS_VOICE_MEDIUM);
            _TEXTCASE_(NCC_DIS_PROTOCOL_ERROR);
            _TEXTCASE_(NCC_DIS_CONGESTION);

        default: return NULL;
        }
    }

    case NCCEVN_CALL_RELEASED:
    {
        switch (pevent->value)
        {
            _TEXTCASE_(NCC_RELEASED_GLARE);
            _TEXTCASE_(NCC_RELEASED_ERROR);
        default: return NULL;
        }
    }

    case NCCEVN_CALL_RETRIEVED:
    {
        switch (pevent->value)
        {
            _TEXTCASE_(NCC_RETRIEVED_AUTO_TRANSFER_FAILED);

        default: return NULL;
        }
    }

    case NCCEVN_CALL_STATUS_UPDATE:
    {
        switch (pevent->value)
        {
            _TEXTCASE_(NCC_CALL_STATUS_CALLINGADDR);

        default: return NULL;
        }
    }

    case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
    {
        switch (pevent->value)
        {
            _TEXTCASE_(NCC_EXTENDED_CALL_STATUS_CATEGORY);

        default: return NULL;
        }
    }

    case NCCEVN_LINE_OUT_OF_SERVICE:
    {
        switch (pevent->value)
        {
            _TEXTCASE_(NCC_OUT_OF_SERVICE_DIGIT_TIMEOUT);
            _TEXTCASE_(NCC_OUT_OF_SERVICE_NO_LOOP_CURRENT);
            _TEXTCASE_(NCC_OUT_OF_SERVICE_PERM_SIGNAL);
            _TEXTCASE_(NCC_OUT_OF_SERVICE_REMOTE_BLOCK);
            _TEXTCASE_(NCC_OUT_OF_SERVICE_WINK_STUCK);
            _TEXTCASE_(NCC_OUT_OF_SERVICE_NO_DIGITS);
            _TEXTCASE_(NCC_OUT_OF_SERVICE_LINE_FAULT);

        default: return NULL;
        }
    }

    case NCCEVN_PROTOCOL_ERROR:
    {
        switch (pevent->value)
        {
            _TEXTCASE_(NCC_PROTERR_CAPABILITY_ERROR);
            _TEXTCASE_(NCC_PROTERR_DIGIT_TIMEOUT);
            _TEXTCASE_(NCC_PROTERR_EXTRA_DIGITS);
            _TEXTCASE_(NCC_PROTERR_INVALID_DIGIT);
            _TEXTCASE_(NCC_PROTERR_NO_CS_RESOURCE);
            _TEXTCASE_(NCC_PROTERR_PREMATURE_ANSWER);
            _TEXTCASE_(NCC_PROTERR_COMMAND_OUT_OF_SEQUENCE);
            _TEXTCASE_(NCC_PROTERR_EVENT_OUT_OF_SEQUENCE);
            _TEXTCASE_(NCC_PROTERR_TIMEOUT);
            _TEXTCASE_(NCC_PROTERR_FALSE_SEIZURE);

        default: return NULL;
        }
    }

    case NCCEVN_REJECTING_CALL:
    {
        switch (pevent->value)
        {
            _TEXTCASE_(NCC_REJECT_HOST_TIMEOUT);
            _TEXTCASE_(NCC_REJECT_PLAY_BUSY);
            _TEXTCASE_(NCC_REJECT_PLAY_REORDER);
            _TEXTCASE_(NCC_REJECT_PLAY_RINGTONE);
            _TEXTCASE_(NCC_REJECT_USER_AUDIO);

        default: return NULL;
        }
    }

    case NCCEVN_REMOTE_ANSWERED:
    {
        switch (pevent->value)
        {
            _TEXTCASE_(NCC_ANSWER_MODEM);
            _TEXTCASE_(NCC_ANSWER_SIGNAL);
            _TEXTCASE_(NCC_ANSWER_VOICE);

        default: return NULL;
        }
    }

    case NCCEVN_RECEIVED_DIGIT:
    {
        sprintf(buffer, "'%c'", pevent->value);
        return buffer;
    }

    case NCCEVN_UNBLOCK_FAILED:
    {
        switch (pevent->value)
        {
            _TEXTCASE_(NCC_UNBLOCK_TIMEOUT);
            _TEXTCASE_(NCC_UNBLOCK_OUT_OF_SEQUENCE);
            _TEXTCASE_(NCC_UNBLOCK_CAPABILITY_ERROR);
        default: return NULL;
        }
    }

    default:
        return NULL;
    }
}  /* end _hccGetValueText() */


/*----------------------------------------------------------------------------
 * _hccGetSizeText()
 *  Returns a string with text description of the input ncc code.
 *--------------------------------------------------------------------------*/
char *
 _hccGetSizeText( DISP_EVENT* pevent )
{
    switch (pevent->id)
    {
    case NCCEVN_PROTOCOL_EVENT:
    {
        // for future enhancement
    }

    default:
        return NULL;
    }
}

/*----------------------------------------------------------------------------
 * _hccFormatCommand()
 *  Returns a string with text description of the input ncc code.
 *--------------------------------------------------------------------------*/
void
  _hccFormatCommand(
       DISP_EVENT *commandp,
       char *printbuf,
       unsigned pbsize,
       char *indent )
{
    const char *text;

    if( (text=hccGetText((unsigned)commandp->id)) == (char *)NULL )
    {
        sprintf( printbuf, "%sUnknown ncc Command: *** (%08x)\n",
                 indent, commandp->id );
    }
    else
    {
        sprintf( printbuf, "%sCommand: %s  *** (%08x)\n",
                 indent, text, commandp->id );
    }

}   /* end _hccFormatCommand() */

/*----------------------------------------------------------------------------
 * _hccFormatError()
 *  Returns a string with text description of the input ncc code.
 *--------------------------------------------------------------------------*/
void
  _hccFormatError(
       DISP_EVENT *commandp,
       char *printbuf,
       unsigned pbsize,
       char *indent )
{
    const char *text = NULL;

    if( (text=hccGetText((unsigned)commandp->id)) == (char *)NULL )
    {
        sprintf( printbuf, "%sUnknown ncc Error: *** (%08x)\n",
                 indent, commandp->id );
    }
    else
    {
        sprintf( printbuf, "%sError: %s  *** (%08x)\n",
                 indent, text, commandp->id );
    }

}  /* end _hccFormatError() */

