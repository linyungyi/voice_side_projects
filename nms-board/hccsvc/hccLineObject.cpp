/*****************************************************************************
* FILE:        hccLineObject.cpp
*
* DESCRIPTION:
*              This file contains the:
*
*                - The definition of class HccCallObject and HccLineObject
*****************************************************************************/

#include "hccLineObject.h"

/**----------------------------------------------------------------------------
 * Class HccCallObject
 *---------------------------------------------------------------------------*/
DWORD HccCallObject::_availablehd = 1;

/**
 * The constructor of a call object
 * - initialize the call status
 * - assign a sever call object handle to the call object
 * - set the addr of the object itself as the previous call object and the
 *   next call in the linked list.
 */
HccCallObject::HccCallObject ( CTAHD linehd, DWORD dir )
{
    _prev = this;                         //initialize these to object itself
    _next = this;

    memset(&_callStatus, '\0', sizeof (NCC_CALL_STATUS) );
    _callStatus.size = sizeof ( NCC_CALL_STATUS );
    _callStatus.linehd = linehd;
    _callStatus.direction = dir;

    _servercallhd = _availablehd++;

    if (_availablehd == 0)       //0 is invalid server side call object handle
        _availablehd++;
}

/**
 * A utility function to set calledaddr, callingaddr, and callingname fields
 * in the call status
 */
void HccCallObject::setAddrsName ( char* calledAddr, char* callingAddr,
                                   char* callingName )
{
    strcpy( _callStatus.calledaddr, calledAddr );
    if ( callingAddr != NULL )
        strcpy( _callStatus.callingaddr, callingAddr );
    if ( callingName != NULL )
        strcpy( _callStatus.callingname, callingName );
}


/**----------------------------------------------------------------------------
 * Class HccLineObject
 *---------------------------------------------------------------------------*/
const DWORD HccLineObject::apiToPendingCmd[NCCCMD_QTY] =
{
  NCC_PENDINGCMD_ACCEPT_CALL,        // NCCCMD_ACCEPT_CALL
  NCC_PENDINGCMD_ANSWER_CALL,        // NCCCMD_ANSWER_CALL
  NCC_PENDINGCMD_AUTOMATIC_TRANSFER, // NCCCMD_AUTOMATIC_TRANSFER
  NCC_PENDINGCMD_BLOCK_CALLS,        // NCCCMD_BLOCK_CALLS
  NCC_PENDINGCMD_DISCONNECT_CALL,    // NCCCMD_DISCONNECT_CALL
  0,                                 // NCCCMD_GET_CALL_STATUS
  0,                                 // NCCCMD_GET_EXTENDED_CALL_STATUS
  0,                                 // NCCCMD_GET_LINE_STATUS
  NCC_PENDINGCMD_HOLD_CALL,          // NCCCMD_HOLD_CALL
  NCC_PENDINGCMD_PLACE_CALL,         // NCCCMD_PLACE_CALL
  0,                                 // NCCCMD_QUERY_CAPABILITY
  NCC_PENDINGCMD_REJECT_CALL,        // NCCCMD_REJECT_CALL
  NCC_PENDINGCMD_RELEASE_CALL,       // NCCCMD_RELEASE_CALL
  NCC_PENDINGCMD_RETRIEVE_CALL,      // NCCCMD_RETRIEVE_CALL
  0,                                 // NCCCMD_SEND_CALL_MESSAGE
  0,                                 // NCCCMD_SEND_DIGITS
  0,                                 // NCCCMD_SEND_LINE_MESSAGE
  NCC_PENDINGCMD_SET_BILLING,        // NCCCMD_SET_BILLING
  NCC_PENDINGCMD_START_PROTOCOL,     // NCCCMD_START_PROTOCOL
  NCC_PENDINGCMD_STOP_PROTOCOL,      // NCCCMD_STOP_PROTOCOL
  NCC_PENDINGCMD_TRANSFER_CALL,      // NCCCMD_TRANSFER_CALL
  NCC_PENDINGCMD_UNBLOCK_CALLS,      // NCCCMD_UNBLOCK_CALLS
  NCC_PENDINGCMD_TRANSFER_CALL,      // NCCCMD_TCT_TRANSFER_CALL
  0,                                 // NCCCMD_GET_CALLID
  0                                  // NCCCMD_HOLD_RESPONSE
};

const DWORD HccLineObject::eventToPendingCmd[NCCEVN_QTY] =
{
  0,                                 // NCCEVN_PROTOCOL_ERROR
  NCC_PENDINGCMD_ACCEPT_CALL,        // NCCEVN_ACCEPTING_CALL
  NCC_PENDINGCMD_ANSWER_CALL,        // NCCEVN_ANSWERING_CALL
  0,                                 // NCCEVN_BILLING_INDICATION
  NCC_PENDINGCMD_SET_BILLING,        // NCCEVN_BILLING_SET
  NCC_PENDINGCMD_BLOCK_CALLS,        // NCCEVN_CALLS_BLOCKED
  NCC_PENDINGCMD_UNBLOCK_CALLS,      // NCCEVN_CALLS_UNBLOCKED
  0,                                 // NCCEVN_CALL_CONNECTED
  NCC_PENDINGCMD_DISCONNECT_CALL,    // NCCEVN_CALL_DISCONNECTED
  NCC_PENDINGCMD_HOLD_CALL,          // NCCEVN_CALL_HELD
  0,                                 // NCCEVN_PROTOCOL_EVENT
  0,                                 // NCCEVN_CALL_PROCEEDING
  0,                                 // NCCEVN_CALL_STATUS_UPDATE
  NCC_PENDINGCMD_HOLD_CALL,          // NCCEVN_HOLD_REJECTED
  0,                                 // NCCEVN_INCOMING_CALL
  0,                                 // NCCEVN_LINE_IN_SERVICE
  0,                                 // NCCEVN_LINE_OUT_OF_SERVICE
  0,                                 // NCCEVN_REMOTE_ALERTING
  0,                                 // NCCEVN_REMOTE_ANSWERED
  NCC_PENDINGCMD_PLACE_CALL,         // NCCEVN_PLACING_CALL
  NCC_PENDINGCMD_REJECT_CALL,        // NCCEVN_REJECTING_CALL
  0,                                 // NCCEVN_CALL_RELEASED
  NCC_PENDINGCMD_RETRIEVE_CALL,      // NCCEVN_CALL_RETRIEVED
  NCC_PENDINGCMD_RETRIEVE_CALL,      // NCCEVN_RETRIEVE_REJECTED
  0,                                 // NCCEVN_SEIZURE_DETECTED
  NCC_PENDINGCMD_START_PROTOCOL,     // NCCEVN_START_PROTOCOL_DONE
  NCC_PENDINGCMD_STOP_PROTOCOL,      // NCCEVN_STOP_PROTOCOL_DONE
  0,                                 // NCCEVN_CAPABILITY_UPDATE
  0,                                 // NCCEVN_EXTENDED_CALL_STATUS_UPDATE
  0,                                 // NCCEVN_RECEIVED_DIGITS
  0,                                 // NCCEVN_READY_FOR_MEDIA
  0,                                 // NCCEVN_NOT_READY_FOR_MEDIA
  NCC_PENDINGCMD_BLOCK_CALLS,        // NCCEVN_BLOCK_FAILED
  NCC_PENDINGCMD_UNBLOCK_CALLS,      // NCCEVN_UNBLOCK_FAILED
  0,                                 // NCCEVN_CALLID_AVAILABLE
  0                                  // NCCEVN_HOLD_INDICATION
};

const DWORD HccLineObject::apiToRequiredCapability[NCCCMD_QTY] =
{
  NCC_CAP_ACCEPT_CALL,               // NCCCMD_ACCEPT_CALL
  0,                                 // NCCCMD_ANSWER_CALL
  NCC_CAP_AUTOMATIC_TRANSFER,        // NCCCMD_AUTOMATIC_TRANSFER
  0,                                 // NCCCMD_BLOCK_CALLS
  0,                                 // NCCCMD_DISCONNECT_CALL
  0,                                 // NCCCMD_GET_CALL_STATUS
  NCC_CAP_EXTENDED_CALL_STATUS,      // NCCCMD_GET_EXTENDED_CALL_STATUS
  0,                                 // NCCCMD_GET_LINE_STATUS
  NCC_CAP_HOLD_CALL,                 // NCCCMD_HOLD_CALL
  0,                                 // NCCCMD_PLACE_CALL
  0,                                 // NCCCMD_QUERY_CAPABILITY
  0,                                 // NCCCMD_REJECT_CALL
  0,                                 // NCCCMD_RELEASE_CALL
  NCC_CAP_HOLD_CALL,                 // NCCCMD_RETRIEVE_CALL
  NCC_CAP_SEND_CALL_MESSAGE,         // NCCCMD_SEND_CALL_MESSAGE
  NCC_CAP_OVERLAPPED_SENDING,        // NCCCMD_SEND_DIGITS
  NCC_CAP_SEND_LINE_MESSAGE,         // NCCCMD_SEND_LINE_MESSAGE
  NCC_CAP_SET_BILLING,               // NCCCMD_SET_BILLING
  0,                                 // NCCCMD_START_PROTOCOL
  0,                                 // NCCCMD_STOP_PROTOCOL
  NCC_CAP_SUPERVISED_TRANSFER,       // NCCCMD_TRANSFER_CALL
  0,                                 // NCCCMD_UNBLOCK_CALLS
  NCC_CAP_TWOCHANNEL_TRANSFER,       // NCCCMD_TCT_TRANSFER_CALL
  NCC_CAP_TWOCHANNEL_TRANSFER,       // NCCCMD_GET_CALLID
  NCC_CAP_HOLD_RESPONSE              // NCCCMD_HOLD_RESPONSE
};

const DWORD HccLineObject::eventToRequiredCapability[NCCEVN_QTY] =
{
  0,                                 // NCCEVN_PROTOCOL_ERROR
  NCC_CAP_ACCEPT_CALL,               // NCCEVN_ACCEPTING_CALL
  0,                                 // NCCEVN_ANSWERING_CALL
  NCC_CAP_GET_BILLING,               // NCCEVN_BILLING_INDICATION
  NCC_CAP_SET_BILLING,               // NCCEVN_BILLING_SET
  0,                                 // NCCEVN_CALLS_BLOCKED
  0,                                 // NCCEVN_CALLS_UNBLOCKED
  0,                                 // NCCEVN_CALL_CONNECTED
  0,                                 // NCCEVN_CALL_DISCONNECTED
  NCC_CAP_HOLD_CALL,                 // NCCEVN_CALL_HELD
  0,                                 // NCCEVN_PROTOCOL_EVENT
  0,                                 // NCCEVN_CALL_PROCEEDING
  NCC_CAP_STATUS_UPDATE,             // NCCEVN_CALL_STATUS_UPDATE
  NCC_CAP_HOLD_CALL,                 // NCCEVN_HOLD_REJECTED
  0,                                 // NCCEVN_INCOMING_CALL
  0,                                 // NCCEVN_LINE_IN_SERVICE
  0,                                 // NCCEVN_LINE_OUT_OF_SERVICE
  0,                                 // NCCEVN_REMOTE_ALERTING
  0,                                 // NCCEVN_REMOTE_ANSWERED
  0,                                 // NCCEVN_PLACING_CALL
  0,                                 // NCCEVN_REJECTING_CALL
  0,                                 // NCCEVN_CALL_RELEASED
  NCC_CAP_HOLD_CALL,                 // NCCEVN_CALL_RETRIEVED
  NCC_CAP_HOLD_CALL,                 // NCCEVN_RETRIEVE_REJECTED
  0,                                 // NCCEVN_SEIZURE_DETECTED
  0,                                 // NCCEVN_START_PROTOCOL_DONE
  0,                                 // NCCEVN_STOP_PROTOCOL_DONE
  NCC_CAP_CAPABILITY_UPDATE,         // NCCEVN_CAPABILITY_UPDATE
  NCC_CAP_EXTENDED_CALL_STATUS,      // NCCEVN_EXTENDED_CALL_STATUS_UPDATE
  NCC_CAP_OVERLAPPED_RECEIVING,      // NCCEVN_RECEIVED_DIGITS
  0,                                 // NCCEVN_READY_FOR_MEDIA
  0,                                 // NCCEVN_NOT_READY_FOR_MEDIA
  0,                                 // NCCEVN_BLOCK_FAILED
  0,                                 // NCCEVN_UNBLOCK_FAILED
  NCC_CAP_TWOCHANNEL_TRANSFER,       // NCCEVN_CALLID_AVAILABLE
  0                                  // NCCEVN_HOLD_INDICATION
};


/**
 * This function is called to create an inbound or an outbound call object
 * It can only be called by Idle::gotoNextState(cmd/evt)
 *
 * @parm dir  = NCC_INBOUND_CALL or NCC_OUTBOUND_CALL
 */
HccCallObject* HccLineObject::createCallObject ( DWORD dir )
{
    CTABEGIN("HccLineObject::createCallObject");
    HccCallObject* callobj = new HccCallObject( _lineHd, dir );

    if ( callobj == NULL )
        return NULL;

    if (_firstCall == NULL)
    {
        // set the header of linked list to the newly created call if it's
        // the first one
        _firstCall = callobj;
    }
    else
    {
        // Otherwise, add the call object to the end of linked list
        callobj->setNext( _firstCall );
        HccCallObject* temp = _firstCall->getPrev();
        callobj->setPrev( temp );
        _firstCall->setPrev( callobj );
        temp->setNext( callobj );
    }

    _lineStatus.numcallhd++;
    return callobj;
}

/**
 * This function is called when nccReleaseCall() is invoked by appliction
 * This function will destroy a call object
 * @parm callToDelete   the pointer of the call object to be detroyed
 */
DWORD HccLineObject::destroyCallObject ( HccCallObject* callToDelete )
{
    ASSERT( callToDelete != NULL );
    CTABEGIN("HccLineObject::destroyCallObject");
    DWORD ret = SUCCESS;

    //reset _currenctCall if it's the one to be deleted
    if ( callToDelete == _currentCall )
        _currentCall = NULL;

    //reset _heldCall if it's the one to be deleted
    if ( callToDelete == _heldCall )
    {
        _heldCall = NULL;
    }

    //remove this call from the linked list and delete it
    if ( callToDelete == _firstCall )
    {
        //if the call to be deleted is the first call in the linked list
        //_firstCall should be reset with the new header
        if (_firstCall == _firstCall->getNext() ) //for one element list
        {
            delete _firstCall;
            _firstCall = NULL;
        }
        else                                   //for multiple element list
        {
            _firstCall = _firstCall->getNext();
            HccCallObject* prev = callToDelete->getPrev();
            HccCallObject* next = callToDelete->getNext();
            prev->setNext(next);
            next->setPrev(prev);
            delete callToDelete;
        }
    }
    else
    {
        // if the call object to be deleted is not the header of the linked list
        // remove the object from the linked list and delete it.
        HccCallObject* prev = callToDelete->getPrev();
        HccCallObject* next = callToDelete->getNext();
        prev->setNext(next);
        next->setPrev(prev);
        delete callToDelete;
    }
    _lineStatus.numcallhd--;
    return SUCCESS;
}

/**
 * This function destroys all existing call objects
 * It's called when NCC service is shut down or nccStopProtocol is invoked.
 */
DWORD HccLineObject::destroyAllCallObjects ()
{
    CTABEGIN("HccLineObject::destroyAllCallObjects");

    if (_firstCall != NULL )
    {
        //start from the second call object in the linked list
        HccCallObject* tempCurCall = _firstCall->getNext();

        //to delete all other call objects other than the header
        while ( tempCurCall != _firstCall )
        {
            HccCallObject* prev = tempCurCall->getPrev();
            HccCallObject* next = tempCurCall->getNext();
            prev->setNext(next);
            next->setPrev(prev);

            delete tempCurCall;
            tempCurCall = _firstCall->getNext();
        }

        //delete the header object
        delete _firstCall;
        _firstCall = NULL;
        _currentCall = NULL;
        _heldCall = NULL;
    }

    _lineStatus.numcallhd = 0;
    return SUCCESS;
}

/**
 * This function is called to convert a call object handle to a call object
 * The linked list of call objects is searched. The address of the matched
 * call object is returned if it's found, otherwise, NULL is returned.
 * @parm svrHdl The server side object handle for a call object
 */
HccCallObject* HccLineObject::objhdToCallObject ( DWORD svrHdl )
{
    CTABEGIN("HccLineObject::svrHdlToCallObject");

    if ( _firstCall == NULL )
        return NULL;

    // Search the call object in the linked list
    HccCallObject* tempCurCall = _firstCall;
    if ( tempCurCall->getServerCallhd() == svrHdl )
    {
        return tempCurCall;
    }

    tempCurCall = _firstCall->getNext();
    while ( tempCurCall != _firstCall )
    {
        if (tempCurCall->getServerCallhd() == svrHdl)
        {
            return tempCurCall;
        }
        tempCurCall = tempCurCall->getNext();
    }
    return NULL;
}

/**
 * Check if an NCC command/event is allowed under the required capabilty
 * @parm cmdOrEvtId  The code number of NCC command/event
 */
DWORD HccLineObject::capabilityChecking( DWORD cmdOrEvtId )
{
    DWORD requiredCap = 0;
    // look for the required capability for an NCC command
    if ( (cmdOrEvtId & 0xf000 ) == 0x3000 )
    {
        ASSERT( ( cmdOrEvtId & 0xfff ) < NCCCMD_QTY );
        requiredCap = apiToRequiredCapability[cmdOrEvtId & 0xff];
    }

    // look for the required capability for an NCC event
    else if ( ( cmdOrEvtId & 0xf000 ) == 0x2000 )
    {
        ASSERT( ( cmdOrEvtId & 0xfff ) < NCCEVN_QTY );
        requiredCap = eventToRequiredCapability[cmdOrEvtId & 0xff];
    }
    else
        return CTAERR_BAD_ARGUMENT;

    // check the if the protocol has the required capability
    // return NCCERR_NOT_CAPABLE if it doesn't, otherwise, return SUCCESS
    if ( requiredCap != 0 && ((requiredCap & (_protcap.capabilitymask)) == 0 ) )
        return NCCERR_NOT_CAPABLE;
    else
        return SUCCESS;
}

/**
 * Based on NCC command, this function updates NCC line state and the state
 * of a call. It also sets the pending commands in line status and call status.
 */
DWORD HccLineObject::updateNccStateMachines( DISP_COMMAND* cmd, HccCallObject* call )
{
    //update NCC line state machine and set the pending cmd for the line
    switch ( cmd->id )
    {
        case NCCCMD_PLACE_CALL:
            _lineStatus.state = NCC_LINESTATE_ACTIVE;
            break;
        case NCCCMD_START_PROTOCOL:
        case NCCCMD_STOP_PROTOCOL:
        case NCCCMD_BLOCK_CALLS:
        case NCCCMD_UNBLOCK_CALLS:
            _lineStatus.pendingcmd = apiToPendingCmd[(cmd->id) & 0xff ];
        default:
            break;
    }

    //update the call state and set the pending cmd for a call
    if ( call != NULL )
    {   NCC_CALL_STATUS* callStatus = call->getCallStatus();

        switch ( cmd->id )
        {
            case NCCCMD_PLACE_CALL:
                call->getCallStatus()->state = NCC_CALLSTATE_OUTBOUND_INITIATED;
            case NCCCMD_ACCEPT_CALL:
            case NCCCMD_ANSWER_CALL:
            case NCCCMD_AUTOMATIC_TRANSFER:
            case NCCCMD_DISCONNECT_CALL:
            case NCCCMD_HOLD_CALL:
            case NCCCMD_REJECT_CALL:
            case NCCCMD_RETRIEVE_CALL:
            case NCCCMD_SET_BILLING:
            case NCCCMD_TRANSFER_CALL:
            case NCCCMD_RELEASE_CALL:
                call->getCallStatus()->pendingcmd = apiToPendingCmd[(cmd->id) & 0xff ];
                break;
            default:
                break;
        }
    }
    return SUCCESS;
}

/**
 * Based on an NCC event,this function updates NCC line state and the state of
 * a call. This function also clears pending command in line status and call
 * status.
 */
DWORD HccLineObject::updateNccStateMachines( DISP_EVENT* evt, HccCallObject* call )
{
    //clear pending command in line status and call status
    DWORD pendingcmd = eventToPendingCmd[(evt->id) & 0xff];

    if ( pendingcmd == _lineStatus.pendingcmd )
        _lineStatus.pendingcmd = 0;

    if ( call != NULL && pendingcmd == call->getCallStatus()->pendingcmd )
        call->getCallStatus()->pendingcmd = 0;

    //when call is disonncted, clear all pending cmd in call status struct
    if ( evt->id == NCCEVN_CALL_DISCONNECTED && call != NULL )
        call->getCallStatus()->pendingcmd = 0;

    //update NCC line state machine
    switch ( evt->id )
    {

        case NCCEVN_START_PROTOCOL_DONE:
            _lineStatus.state = NCC_LINESTATE_IDLE; break;
        case NCCEVN_STOP_PROTOCOL_DONE:
            _lineStatus.state = NCC_LINESTATE_UNINITIALIZED;
            _lineStatus.protocol[0] = '\0';     //unset protocol name
            destroyAllCallObjects();            //destroy all call objects
            break;
        case NCCEVN_CALLS_BLOCKED:
            _lineStatus.state = NCC_LINESTATE_BLOCKING; break;
        case NCCEVN_CALLS_UNBLOCKED:
            _lineStatus.state = NCC_LINESTATE_IDLE; break;

        case NCCEVN_LINE_IN_SERVICE:
            _lineStatus.state = NCC_LINESTATE_IDLE; break;
        case NCCEVN_LINE_OUT_OF_SERVICE:
            _lineStatus.state = NCC_LINESTATE_OUT_OF_SERVICE; break;

        case NCCEVN_CALL_DISCONNECTED:
        case NCCEVN_CALL_HELD:
            _lineStatus.state = NCC_LINESTATE_IDLE; break;

        case NCCEVN_CALL_RETRIEVED:
        case NCCEVN_SEIZURE_DETECTED:
            _lineStatus.state = NCC_LINESTATE_ACTIVE; break;

        default: break;
    }

    //update call state if the call is not NULL
    if ( call != NULL )
    {
        switch ( evt->id )
        {
            //for outbound call
            case NCCEVN_PLACING_CALL:
                call->getCallStatus()->state = NCC_CALLSTATE_PLACING; break;
            case NCCEVN_CALL_PROCEEDING:
                call->getCallStatus()->state = NCC_CALLSTATE_PROCEEDING; break;

            //for inbound call
            case NCCEVN_SEIZURE_DETECTED:
                call->getCallStatus()->state = NCC_CALLSTATE_SEIZURE; break;
            case NCCEVN_RECEIVED_DIGIT:
                call->getCallStatus()->state = NCC_CALLSTATE_RECEIVING_DIGITS; break;
            case NCCEVN_INCOMING_CALL:
                call->getCallStatus()->state = NCC_CALLSTATE_INCOMING; break;
            case NCCEVN_ACCEPTING_CALL:
                call->getCallStatus()->state = NCC_CALLSTATE_ACCEPTING; break;
            case NCCEVN_ANSWERING_CALL:
                call->getCallStatus()->state = NCC_CALLSTATE_ANSWERING; break;
            case NCCEVN_REJECTING_CALL:
                call->getCallStatus()->state = NCC_CALLSTATE_REJECTING; break;

            /* the following cases are for both inbound and outbound call */
            //connect and disconnect
            case NCCEVN_CALL_CONNECTED:
                call->getCallStatus()->state = NCC_CALLSTATE_CONNECTED; break;
            case NCCEVN_CALL_DISCONNECTED:
                call->getCallStatus()->state = NCC_CALLSTATE_DISCONNECTED; break;

            //held and retrieve call
            case NCCEVN_CALL_HELD:
                call->getCallStatus()->held = 1; break;
            case NCCEVN_CALL_RETRIEVED:
                call->getCallStatus()->held = 0; break;
            default: break;
        }

    }
    return SUCCESS;
}

/**
 * This function copies the line status to the appropriate field in ctacmd
 */
DWORD HccLineObject::getLineStatus( DISP_COMMAND* ctacmd )
{
    NCC_LINE_STATUS* status  = (NCC_LINE_STATUS*)(ctacmd->dataptr1);
    NCC_CALLHD*      callObjhds  = (NCC_CALLHD*)(ctacmd->dataptr2);
    DWORD            hdNum       = ctacmd->size2 / sizeof(NCC_CALLHD*);

    /*  Validate these parameters */
    if ((status == NULL) || (ctacmd->size1 != sizeof(NCC_LINE_STATUS)))
        return CTAERR_BAD_ARGUMENT;

    memcpy( status, &_lineStatus , sizeof (NCC_LINE_STATUS) );

    if ( callObjhds != NULL)
    {
        if ( hdNum < _lineStatus.numcallhd )
            return CTAERR_BAD_SIZE;
        else
        {
            if ( _lineStatus.numcallhd > 0 )
            {
                int i = 0;
                HccCallObject* head = getFirstCall();
                callObjhds[i++] = head->getServerCallhd();
                while ( (head = head->getNext()) != getFirstCall() )
                {
                    callObjhds[i++] = head->getServerCallhd();
                }
            }
        }
    }
    return SUCCESS;
}

/**
 * This function copies the call status into appropriate field in ctacmd
 */
DWORD HccLineObject::getCallStatus( DISP_COMMAND* ctacmd )
{
    HccCallObject* callObject = objhdToCallObject( ctacmd->objHd );
    if ( callObject == NULL )
        return CTAERR_INVALID_HANDLE;

    NCC_CALL_STATUS* callstatus          = (NCC_CALL_STATUS*)ctacmd->dataptr1;

    /*  Validate these parameters */
    if ((callstatus == NULL) || (ctacmd->size1 != sizeof(NCC_CALL_STATUS)))
    {
        return CTAERR_BAD_ARGUMENT;
    }

     /* Copy the call status */
    NCC_CALL_STATUS* cst = callObject->getCallStatus();

    callstatus->size = sizeof(NCC_CALL_STATUS);
    callstatus->state = cst->state;
    int i;
    for (i = 0; i <= NCC_MAX_DIGITS; i++)
    {
        callstatus->calledaddr[i] = cst->calledaddr[i];
        callstatus->callingaddr[i] = cst->callingaddr[i];
    }

    for (i = 0; i < NCC_MAX_CALLING_NAME; i++)
        callstatus->callingname[i] = cst->callingname[i];

    callstatus->pendingcmd = cst->pendingcmd;
    callstatus->held = cst->held;
    callstatus->direction = cst->direction;
    callstatus->linehd = cst->linehd;

    return SUCCESS;
}

/**
 * This functions copies the capabilty of the protocol to
 * the appropriate field in ctacmd
 */
DWORD HccLineObject::getCapability( DISP_COMMAND* ctacmd )
{
    NCC_PROT_CAP*   protcap             = (NCC_PROT_CAP*)(ctacmd->dataptr1);
    DWORD           size                = ctacmd->size1;

    if ((protcap == NULL) || (size != sizeof(NCC_PROT_CAP)))
        return CTAERR_BAD_ARGUMENT;

    /*  Copy the protocol capability */
    NCC_PROT_CAP* pc = getProtCap();
    protcap->size = sizeof(NCC_PROT_CAP);
    protcap->capabilitymask = pc->capabilitymask;

    return SUCCESS;
}


