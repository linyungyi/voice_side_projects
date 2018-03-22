/*****************************************************************************
* FILE:        wnkHostTcp.cpp
*
* DESCRIPTION:
*              This file contains the:
*
*                - The definition for the following classes:
*                  The class to define call states in wnk0 protocol
*                       Unitialized,        Idle,          Seizure,
*                       ReceiveAddr,        WaitRejAccept, Accept,
*                       Reject,             Answer         WaitPartyLeaveLine,
*                       Seizing,            SeizingAcked1, SeizingAcked2
*                       SendAddr,           Disconnected
*                  The class to define the wnk0 protocol state machine
*                       WnkHostTcp
*****************************************************************************/


/*****************************************************************************
* Class WnkHostTcp implements a simplified wnk0 protocol which is running
* on host. Simplification includes:
*
* A. parameter management is simplified
*
* (1) Natural Access parameter management is not used. WnkHostTcp
*     doesn't load the standard protocol parameters through Natual Access.
*     Modifying the protocol parameters through Natural Access will not
*     influence WnkHostTcp.
*
*     When this wnk0 talks to a standard wnk0 protocol, it requires
*     the the standard wkk0 use NMS default parameters.
*
* (2) In implementation, WnkHostTcp doesn't check the validation for some
*     parameters sent by application.
*     e.g. WnkHostTcp doesn't check the protocol start parms which is sent
*     through nccStartProtocol.
*
* (3) When invoking some ADI SPI functions, ADI default parameters are used.
*     This is done by setting the parm as NULL.
*     e.g. adiSpiStartCCAssist( ctahd, NULL, NULL, NCC_SVCID );
*
* (3) some necessary parameters are hardcoded, e.g. tone frequency, pulse length
*     and the number of digits to be collectted for an incoming call.
*
* B. Limited number of timers are implemented
*
* (1) Three timers are implemented:
*     - The timer to detect the rising edge of wnk (called Rtimer below)
*     - The timer to detect the falling edge of wnk (called Ftimer below)
*     - The timer to detect the remote party asserting signal (called Stimer below)
*
* (2) Any Other timers defined in wnk0 are not implemented.
*
* (3) The way to resolve a few unimplemented timers:
*     - Inter-digit timer is not implemented.
*       To collect the incoming digits for an incoming call, WnkHostTcp waits
*       for 3 digits and only waits for 3 digits.
*
*     - The timer to wait for PC to respond is not implemented
*       WnkHostTcp waits forever.
*
* C. Limited NCC APIs are implemented. They are:
*      nccAcceptCall
*      nccAnswerCall
*      nccBlockCall
*      nccDisconnectCall
*      nccPlaceCall
*      nccRejectCall
*      nccReleaseCall
*      nccStartProtocol
*      nccStopProtocol
*      nccUnblockCalls
*
*   (Note: There are other three NCC APIs which are implemented in hccLineObject:
*      nccGetCallStatus
*      nccGetLineStatus
*      nccQueryCapability )
*
* D: Limited NCC Events are implemented. They are:
*      NCCEVN_ACCEPTING_CALL
*      NCCEVN_ANSWERING_CALL
*      NCCEVN_CALL_CONNECTED
*      NCCEVN_CALL_DISCONNECTED
*      NCCEVN_CALL_PROCEEDING
*      NCCEVN_CALL_RELEASED
*      NCCEVN_CALLS_BLOCKED
*      NCCEVN_CALLS_UNBLOCKED
*      NCCEVN_INCOMING_CALL
*      NCCEVN_PLACING_CALL
*      NCCEVN_PROTOCOL_ERROR
*      NCCEVN_RECEIVED_DIGIT
*      NCCEVN_SEIZURE_DETECTED
*      NCCEVN_START_PROTOCOL_DONE
*      NCCEVN_STOP_PROTOCOL_DONE
*      NCCEVN_READY_FOR_MEDIA
*      NCCEVN_NOT_READY_FOR_MEDIA
*
* E. Limited NCC state transitions are supported.
*    (Please refer to NCC developer's manual for NCC line state machine and
*     call state machine)
*
*    Supported NCC line state transition:
*      Uninitialized   < - >   Idle
*      Idle            < - >   Blocking
*      Idle            < - >   Active
*
*    Supported NCC call state transition:
*      for an outgoing call:
*         OutBoundInitialized -> Placing -> Proceeding
*                            -> Connected -> Disconnected
*         OutBoundInitialized -> Placing -> Proceeding -> Disconnected
*
*      for an incoming call:
*         Seizure -> ReceivingDigits -> Incoming -> Accepting
*                 -> Answering -> Connected -> Disconnected
*         Seizure -> ReceivingDigits -> Incoming -> Answering
*                 -> Connected -> Disconnected
*         Seizure -> ReceivingDigits -> Incoming -> Rejecting -> Disconnected
*
*  (Note: For this demo, local hanging up by invoking nccDisconnecteCall
*         is only allowed in NCC Connected state)
*
* F. Limited functionalities for a few NCC APIs
* (1) nccAnswerCall
*     This function is implemented by asserting signals, Sending rings is not
*     implemented
*
* (2) nccAcceptCall
*     Two modes are supported: NCC_ACCEPT_PLAY_SILENT, NCC_ACCEPT_PLAY_RING
*
* (3) nccRejectCall
*     One mode is supported: NCC_REJECT_PLAY_BUSY
*
* (4) nccBlockCalls
*     One mode is supported: NCC_BLOCK_REJECTALL
*
*****************************************************************************/

#include "wnkHostTcp.h"

/**----------------------------------------------------------------------------
 * Class Unitialized
 *---------------------------------------------------------------------------*/
DWORD Unitialized::gotoNextState ( DISP_COMMAND* cmd, TcpState* & nextState )
{
    DWORD ret = SUCCESS;
    CTABEGIN( "Unitialized::gotoNextState(cmd)");

    switch ( cmd->id )
    {
     case NCCCMD_START_PROTOCOL:
        //when starting CC assist in ADI,
        //set ADI_START_PARMS=NULL to use the default start parameter
        ret = adiSpiStartCCAssist( cmd->ctahd, NULL, NULL, NCC_SVCID );
        if (ret != SUCCESS)
            return CTALOGERROR( cmd->ctahd, ret, NCC_SVCID );

        _lineObject->updateNccStateMachines( cmd, NULL );
        //save protocol name
        strcpy(_lineObject->getLineStatus()->protocol, (char*) cmd->dataptr1);
        return SUCCESS;

     default:
        return CTALOGERROR( cmd->ctahd, CTAERR_INVALID_STATE, NCC_SVCID );
    }
}

DWORD Unitialized::gotoNextState ( DISP_EVENT* evt, TcpState* & nextState )
{
    DWORD ret = SUCCESS;
    CTABEGIN( "Unitialized::gotoNextState(evt)" );

    //change evt->addr.dest to application. evt->addr.source to NCC
    _setEvtAddrObjhd( evt, NULL );

    switch ( evt->id )
    {
    case ADIEVN_START_CC_ASSIST_DONE:
        evt->id = NCCEVN_START_PROTOCOL_DONE;

        if ( evt->value = CTA_REASON_FINISHED ) //CC Assist mode started correctly
        {
            ret = adiSpiStartSignalDetector( evt->ctahd,
                                             0xC,        //wait for WNK_BIT=0xC,
                                             0xC,        //to detect WNK_BIT=0xC,
                                             100,
                                             100,
                                             (WORD)NCC_SVCID);
            if ( ret != SUCCESS )
            {   //queue NCCEVN_START_PROTOCOL_DONE with reason= CTAERR_BOARD_ERROR
                //to indicate signal detection can't be started,
                // NCC state machine will not be updated.
                evt->value = CTAERR_BOARD_ERROR;
                evt->size = ret;
                dispQueueEvent( evt );
                return CTALOGERROR( evt->ctahd, ret, NCC_SVCID );
            }
            else //to this point Tcp is started correcty
            {
                //update NCC state machine, protocol capablity
                //and set the next possible state of Host TCP
                _lineObject->updateNccStateMachines( evt, NULL );
                _lineObject->getProtCap()->capabilitymask = HCC_WNK_CAPABILITY;
                nextState = _idleNext;

                //queue NCCEVN_START_PROTOCOL_DONE to APP, reason=CTA_REASON_FINISHED
                dispQueueEvent( evt );

                //DTMF detector is started when CC assist started
                //stopping the default-started DTMF detector here gives wnkHostTcp the
                //flexibility to start and stop DTMF detector on its needs
                ret = adiSpiStopDTMFDetector( evt->ctahd, NCC_SVCID );
                if ( ret != SUCCESS )
                {
                    _sendBoardErrorEvent( evt, ret );
                    CTALOGERROR( evt->ctahd, ret, NCC_SVCID );
                }
                return SUCCESS;
            }
        }
        else// CC assist is not started
        {   // queue NCCEVN_START_PROTOCOL_DONE to APP with
            // evt.value=CTAERR_BOARD_ERROR, nccevt.size = adievt.value
            // NCC state machine will not be updated
            evt->size = evt->value;
            evt->value = CTAERR_BOARD_ERROR;
            dispQueueEvent( evt );
            return SUCCESS;
        }
    default:
        evt->id = NCCEVN_PROTOCOL_ERROR;
        evt->value = NCC_PROTERR_EVENT_OUT_OF_SEQUENCE;
        dispQueueEvent( evt );
        return CTALOGERROR( evt->ctahd,
                            NCC_PROTERR_EVENT_OUT_OF_SEQUENCE, NCC_SVCID );
    }
}

/**----------------------------------------------------------------------------
 * Class Idle
 *---------------------------------------------------------------------------*/
DWORD Idle::gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState )
{
    DWORD ret = SUCCESS;
    CTABEGIN( "Idle::gotoNextState(cmd)");

    //make sure no active call on line
    ASSERT( _lineObject->getCurrentCall() == NULL );

    switch ( cmd->id )
    {
        case NCCCMD_PLACE_CALL:
        {
            if ( _blockState == HCC_CALL_BLOCKED )
                return CTALOGERROR( cmd->ctahd, CTAERR_INVALID_STATE, NCC_SVCID );

            char* calledAddr = (char*) cmd->dataptr1;
            char* callingAddr = (char*) cmd->dataptr2;

            if ((calledAddr == NULL) || (strlen(calledAddr) > NCC_MAX_DIGITS))
                return CTALOGERROR( cmd->ctahd, CTAERR_BAD_ARGUMENT, NCC_SVCID );

            // NULL is acceptable for callingAddr
            if ((callingAddr != NULL) && (strlen(callingAddr) > NCC_MAX_DIGITS))
                return CTALOGERROR( cmd->ctahd, CTAERR_BAD_ARGUMENT, NCC_SVCID );

            ret = adiSpiAssertSignal( cmd->ctahd, 0xC, (WORD)NCC_SVCID );

            if ( ret != SUCCESS )
                return CTALOGERROR( cmd->ctahd, ret, NCC_SVCID );

            //start timer (Rtimer) to detect the rising edge of wnk pulse
            //Note: NCC.X.ADI_WNK.winkwaittime = 10000
            ret = adiSpiStartTimer( cmd->ctahd, 10000, 0, (WORD)NCC_SVCID );
            if ( ret != SUCCESS )
                return CTALOGERROR( cmd->ctahd, ret, NCC_SVCID );

            HccCallObject* callObj = _lineObject->createCallObject( NCC_CALL_OUTBOUND );
            if ( callObj == NULL )
                return CTALOGERROR( cmd->ctahd, CTAERR_OUT_OF_MEMORY, NCC_SVCID );

            _lineObject->setCurrentCall( callObj );
            cmd->objHd = callObj->getServerCallhd();

            // set up call object status.
            // Note: calling name is not implemented
            callObj->setAddrsName( calledAddr, callingAddr, NULL );

            _lineObject->updateNccStateMachines(cmd, callObj );

            // assign the next state of state machine
            nextState = _seizingNext;
            return SUCCESS;
        }
        default:
            return CTALOGERROR( cmd->ctahd, CTAERR_INVALID_STATE, NCC_SVCID );
    }
}

DWORD Idle::gotoNextState( DISP_EVENT* evt, TcpState* & nextState )
{
    DWORD ret = SUCCESS;
    CTABEGIN( "Idle::gotoNextState(evt)");

    // change the dest/source of event.
    _setEvtAddrObjhd( evt, NULL );
    DWORD reason = evt->value;


    switch (evt->id)
    {
        case ADIEVN_SIGNALBIT_CHANGED:              //incoming call
            if ((!(reason ^ 0xA1) || !(reason ^ 0xB1)) && !(evt->size ^ 0xC))
            {
                // The mode of blocking call is simplified in this demo
                // The way to block a call is to play busy tone and not present the call
                // to application.
                // For a real protocol, more work should be done.
                if ( _blockState == HCC_CALL_BLOCKED )
                {
                    //send Wnk to calling party
                    ret = adiSpiStartPulse( evt->ctahd, 0xC, 200, 0, (WORD)NCC_SVCID );
                    if ( ret != SUCCESS )
                    {
                        _sendBoardErrorEvent( evt, ret );
                        return CTALOGERROR( evt->ctahd, ret, NCC_SVCID );
                    }
                    //send busy tone to calling party to reject the incoming call
                    ADI_TONE_PARMS p[1] = {0};
                    p[0].freq1   = 480;    p[0].ampl1   = -24;
                    p[0].freq2   = 620;    p[0].ampl2   = -24;
                    p[0].ontime  = 500;    p[0].offtime = 500;
                    p[0].iterations = -1;
                    ret = adiSpiStartTones( evt->ctahd, 1 , p, NCC_SVCID );//tonecount = 1;
                    if ( ret != SUCCESS )
                    {
                        _sendBoardErrorEvent( evt, ret );
                        return CTALOGERROR( evt->ctahd, ret, NCC_SVCID );
                    }
                    _busyToneState = 1;//indicate busy tone is playing.
                                       //This flag will be reset upon the receipt of
                                       //tone done evt
                    return ret;
                }

                // shouldn't happen: there's an incoming call when an active call on line.
                // log error, swallow this event
                if ( _lineObject->getCurrentCall() != NULL )
                    return CTALOGERROR( evt->ctahd, CTAERR_RESOURCE_CONFLICT, NCC_SVCID );

                evt->id = NCCEVN_SEIZURE_DETECTED;
                HccCallObject* callObj = _lineObject->createCallObject( NCC_CALL_INBOUND );
                _lineObject->setCurrentCall( callObj );

                if ( callObj == NULL )
                    return CTALOGERROR( evt->ctahd, CTAERR_OUT_OF_MEMORY, NCC_SVCID );
                else
                {
                     evt->objHd = callObj->getServerCallhd();
                     evt->value = 0;
                     evt->size = 0;
                }

                //send NCCEVN_SEIZURE_DETECTED event to APP
                dispQueueEvent( evt );
                //assign the next state to Seizure state,
                nextState = _seizureNext;
                _lineObject->updateNccStateMachines( evt, callObj );

                //send Wnk to calling party
                ret = adiSpiStartPulse( evt->ctahd, 0xC, 200, 0, (WORD)NCC_SVCID );
                if ( ret != SUCCESS )
                {
                    _sendBoardErrorEvent( evt, ret );
                    return CTALOGERROR( evt->ctahd, ret, NCC_SVCID );
                }
                else
                {
                    return ret;
                }
            }
            else    //swallow other signal bit change evt.
                return SUCCESS;
        case ADIEVN_PULSE_DONE: //catch the pulse done event from adiSpiStartPulse
            return SUCCESS;
        case ADIEVN_TONES_DONE: //this evt from stoppping busy tone when
                                //leaving reject state
                                //or from playing busy tone when calls are blocked
             _busyToneState = 0;
             return SUCCESS;
        case ADIEVN_DTMF_DETECT_DONE:  //this is from stopping DTMF collection
            return SUCCESS;            // swaloow this event

        default:    // log CTAERR_INVALID_STATE simply
            return CTALOGERROR( evt->ctahd, CTAERR_INVALID_STATE, NCC_SVCID );
    }
}


/**----------------------------------------------------------------------------
 * Class Seizure
 *---------------------------------------------------------------------------*/
DWORD Seizure::gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState )
{
    CTABEGIN( "Seizure::gotoNextState(cmd)");

    return CTALOGERROR( cmd->ctahd, CTAERR_INVALID_STATE, NCC_SVCID );
}

DWORD Seizure::gotoNextState( DISP_EVENT* evt, TcpState* & nextState )
{
    CTABEGIN( "Seizure::gotoNextState(evt)");
    DWORD ret = SUCCESS;

    HccCallObject* callObj = _lineObject->getCurrentCall();
    ASSERT( callObj != NULL );
    _setEvtAddrObjhd( evt, callObj );

    switch (evt->id)
    {
    case ADIEVN_SIGNALBIT_CHANGED:
        _handleRemoteHangUp( evt, callObj, nextState, _idleNext );
        return SUCCESS;

    case ADIEVN_PULSE_DONE:
        if ( evt->value != CTA_REASON_FINISHED )
        {
            _sendBoardErrorEvent( evt, evt->value );
            return SUCCESS;
        }
        else //progress to ReceiveAddr state
        {
            // prepare to receive digits
            ((ReceiveAddr*)_receiveAddrNext)->resetNumOfDigits();
            ret = adiSpiFlushDigitQueue( evt->ctahd, NCC_SVCID );
            if ( ret != SUCCESS )
            {
                _sendBoardErrorEvent( evt, ret );
                CTALOGERROR( evt->ctahd, ret, NCC_SVCID );
            }

            ret = adiSpiStartDTMFDetector( evt->ctahd, NULL, NCC_SVCID );
            if ( ret != SUCCESS )
            {
                _sendBoardErrorEvent( evt, ret );
                CTALOGERROR( evt->ctahd, ret, NCC_SVCID );
            }

            nextState = _receiveAddrNext;
            return ret;
        }
    default:
        return CTALOGERROR( evt->ctahd, CTAERR_INVALID_STATE, NCC_SVCID );
    }
}


/**----------------------------------------------------------------------------
 * Class ReceiveAddr
 *---------------------------------------------------------------------------*/
DWORD ReceiveAddr::gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState )
{
    CTABEGIN( "ReceiveAddr::gotoNextState(cmd)");

    return CTALOGERROR( cmd->ctahd, CTAERR_INVALID_STATE, NCC_SVCID );
}

DWORD ReceiveAddr::gotoNextState( DISP_EVENT* evt, TcpState* & nextState )
{
    CTABEGIN( "ReceiveAddr::gotoNextState(evt)");
    DWORD ret = SUCCESS;

    HccCallObject* callObj = _lineObject->getCurrentCall();
    ASSERT( callObj != NULL );
    _setEvtAddrObjhd( evt, callObj );

    switch ( evt->id )
    {
    case ADIEVN_SIGNALBIT_CHANGED:
         _handleRemoteHangUp( evt, callObj, nextState, _idleNext );
         return SUCCESS;

    case ADIEVN_DIGIT_BEGIN:
        return SUCCESS;        //swallow this event

    case ADIEVN_DIGIT_END:
        // collect the incoming digits and save them
        (callObj->getCallStatus())->calledaddr[_numOfDigits] = (char)evt->value;
        _numOfDigits++;
        //Note: overlapping receiving digits is implemented
        evt->id = NCCEVN_RECEIVED_DIGIT;
        //Note: evt.value = the comming digit
        evt->size = 0;
        dispQueueEvent( evt );
        if ( _numOfDigits == 3 )
        {
            //once getting enough digits, present NCCEVN_INCOMING_CALL evt
            evt->id = NCCEVN_INCOMING_CALL;
            evt->value = 0;
            evt->size = 0;
            dispQueueEvent( evt );
            _lineObject->updateNccStateMachines( evt, callObj );
            nextState = _waitRejAcceptNext;

            //stop DTMF detection
            ret = adiSpiStopDTMFDetector( evt->ctahd, NCC_SVCID );
            if ( ret != SUCCESS )
            {
                _sendBoardErrorEvent( evt, ret );
                CTALOGERROR( evt->ctahd, ret, NCC_SVCID );
            }

            //flush the digit queue in ADI service, otherwise, play/record
            //function will take those digits as preentered digits and play/record
            //function will stop immediatly after started.
            ret = adiSpiFlushDigitQueue( evt->ctahd, NCC_SVCID );
            if ( ret != SUCCESS )
            {
                _sendBoardErrorEvent( evt, ret );
                CTALOGERROR( evt->ctahd, ret, NCC_SVCID );
            }

        }
        return ret;
    default:
        return CTALOGERROR( evt->ctahd, CTAERR_INVALID_STATE, NCC_SVCID );
    }
}

/**----------------------------------------------------------------------------
 * Class WaitRejAccept
 *---------------------------------------------------------------------------*/
DWORD WaitRejAccept::gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState )
{
    DWORD ret = SUCCESS;
    CTABEGIN( "WaitRejAccept::gotoNextState(cmd)");

    HccCallObject* callObj = _lineObject->objhdToCallObject( cmd->objHd );
    if ( callObj == NULL )
        return CTALOGERROR( cmd->ctahd, CTAERR_INVALID_HANDLE, NCC_SVCID );


    DISP_EVENT evt = {0};
    evt.ctahd = cmd->ctahd;
    _setEvtAddrObjhd( &evt, callObj );

    switch ( cmd->id )
    {
    case NCCCMD_ACCEPT_CALL:
        switch ( cmd->size1 )           //accept mode = cmd->size1
        {
            case NCC_ACCEPT_PLAY_SILENT:
            {
                _lineObject->updateNccStateMachines( cmd, callObj );
                evt.id = NCCEVN_ACCEPTING_CALL;
                evt.value = NCC_ACCEPT_PLAY_SILENT;
                _lineObject->updateNccStateMachines( &evt, callObj );
                dispQueueEvent( &evt );

                nextState = _acceptNext;
                return ret;
            }
            case NCC_ACCEPT_PLAY_RING:
            {
                ADI_TONE_PARMS p[1] = {0};
                p[0].freq1   = 440;    p[0].ampl1   = -24;
                p[0].freq2   = 480;    p[0].ampl2   = -24;
                p[0].ontime  = 1000;   p[0].offtime = 3000;
                p[0].iterations = -1;
                ret = adiSpiStartTones( cmd->ctahd, 1 , p, NCC_SVCID );//tonecount = 1;
                if ( ret == SUCCESS )
                {
                    evt.id = NCCEVN_ACCEPTING_CALL;
                    evt.value = NCC_ACCEPT_PLAY_RING;
                    _lineObject->updateNccStateMachines( &evt, callObj );
                    nextState = _acceptNext;
                    ((Accept*) _acceptNext)->setAcceptMode( NCC_ACCEPT_PLAY_RING );
                    dispQueueEvent( &evt ); //queue NCCEVN_ACCEPTING_CALL

                }
                else
                {
                    _sendBoardErrorEvent( &evt, ret );
                    CTALOGERROR( cmd->ctahd, ret, NCC_SVCID );
                }
                return ret;
            }

            case NCC_ACCEPT_USER_AUDIO:
                return CTALOGERROR( cmd->ctahd,
                                    CTAERR_NOT_IMPLEMENTED, NCC_SVCID );
            default:
                return CTALOGERROR( cmd->ctahd,
                                CTAERR_BAD_ARGUMENT, NCC_SVCID );
        }

    case NCCCMD_ANSWER_CALL:

        _lineObject->updateNccStateMachines( cmd, callObj );
        evt.id = NCCEVN_ANSWERING_CALL;
        dispQueueEvent( &evt );
        _lineObject->updateNccStateMachines( &evt, callObj );

        ret = adiSpiAssertSignal( cmd->ctahd, 0xC, NCC_SVCID );
        if ( ret != SUCCESS )
            return CTALOGERROR( cmd->ctahd, ret, NCC_SVCID );

        //generate NCCEVN_CALL_CONNECTED
        evt.id = NCCEVN_CALL_CONNECTED;
        evt.value = NCC_CON_ANSWERED;

        dispQueueEvent( &evt );
        _lineObject->updateNccStateMachines( &evt, callObj );
        nextState = _answerNext; //assign next state to Answer state

        return ret;

    case NCCCMD_REJECT_CALL:
        switch ( cmd->size2 )       //reject method = cmd->size1
        {
            case NCC_REJECT_PLAY_BUSY:
            {
                ADI_TONE_PARMS p[1] = {0};
                p[0].freq1   = 480;    p[0].ampl1   = -24;
                p[0].freq2   = 620;    p[0].ampl2   = -24;
                p[0].ontime  = 500;    p[0].offtime = 500;
                p[0].iterations = -1;
                ret = adiSpiStartTones( cmd->ctahd, 1 , p, NCC_SVCID );//tonecount = 1;
                if ( ret == SUCCESS )
                {
                    evt.id = NCCEVN_REJECTING_CALL;
                    evt.value = NCC_REJECT_PLAY_BUSY;
                    _lineObject->updateNccStateMachines( &evt, callObj );
                    nextState = _rejectNext;
                    dispQueueEvent( &evt ); //queue NCCEVN_REJECTING_CALL

                }
                else
                {
                    _sendBoardErrorEvent( &evt, ret );
                    CTALOGERROR( cmd->ctahd, ret, NCC_SVCID );
                }
                return ret;
            }
            case NCC_REJECT_PLAY_REORDER:
            case NCC_REJECT_PLAY_RINGTONE:
            case NCC_REJECT_USER_AUDIO:
                return CTALOGERROR( cmd->ctahd, CTAERR_NOT_IMPLEMENTED, NCC_SVCID );
            default:
                return CTALOGERROR( cmd->ctahd, CTAERR_BAD_ARGUMENT, NCC_SVCID );
        }

     default:
        return CTALOGERROR( cmd->ctahd,
                            CTAERR_INVALID_STATE, NCC_SVCID );
    }
}

DWORD WaitRejAccept::gotoNextState( DISP_EVENT* evt,
                                    TcpState* & nextState )
{
    DWORD ret = SUCCESS;
    CTABEGIN( "WaitRejAccept::gotoNextState(evt)");
    HccCallObject* callObj = _lineObject->getCurrentCall();
    ASSERT( callObj != NULL );
    _setEvtAddrObjhd( evt, callObj );

    switch ( evt->id )
    {
        case ADIEVN_SIGNALBIT_CHANGED:
            //remote hang up
            _handleRemoteHangUp( evt, callObj, nextState, _idleNext );
            return SUCCESS;

        // DTMF detector is stopped when transition to WaitAppRejAccept state
        case ADIEVN_DTMF_DETECT_DONE: //swallow DTMF done event here
            return SUCCESS;

        default:
            return CTALOGERROR( evt->ctahd,
                                CTAERR_INVALID_STATE, NCC_SVCID );
    }
}

/**----------------------------------------------------------------------------
 * Class Accept
 *---------------------------------------------------------------------------*/
DWORD Accept::gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState )
{
    DWORD ret = SUCCESS;
    CTABEGIN( "Accept::gotoNextState(cmd)");
    HccCallObject* callObj = _lineObject->objhdToCallObject( cmd->objHd );
    if ( callObj == NULL )
        return CTALOGERROR( cmd->ctahd, CTAERR_INVALID_HANDLE, NCC_SVCID );

    DISP_EVENT evt = {0};
    evt.ctahd = cmd->ctahd;
    _setEvtAddrObjhd( &evt, callObj );

    switch ( cmd->id )
    {
        case NCCCMD_ANSWER_CALL:
            _lineObject->updateNccStateMachines( cmd, callObj );

            if ( _acceptMode == NCC_ACCEPT_PLAY_RING )
            {
                _acceptMode = 0;
                adiSpiStopTones( cmd->ctahd, NCC_SVCID );
            }

            evt.id = NCCEVN_ANSWERING_CALL;
            _lineObject->updateNccStateMachines( &evt, callObj );

            dispQueueEvent( &evt );     //queue NCCEVN_ANSWERING_CALL
            //asserting signals to set up the call
            ret = adiSpiAssertSignal( cmd->ctahd, 0xC, NCC_SVCID );
            if ( ret != SUCCESS ) //NCCEVN_CALL_CONNECTED can't be generated
            {
                evt.id = NCCEVN_PROTOCOL_ERROR;
                evt.value = CTAERR_BOARD_ERROR;
                evt.size = ret;
                dispQueueEvent( &evt );
                return CTALOGERROR( cmd->ctahd, ret, NCC_SVCID );
            }
            else                 //queue NCCEVN_CALL_CONNECTED
            {
                evt.id = NCCEVN_CALL_CONNECTED;
                evt.value = NCC_CON_ANSWERED;
                dispQueueEvent( &evt );

                _lineObject->updateNccStateMachines( &evt, callObj );
                nextState = _answerNext;
                return SUCCESS;
            }

        case NCCCMD_REJECT_CALL:
            return CTAERR_NOT_IMPLEMENTED;

        default:
            return CTALOGERROR( cmd->ctahd, CTAERR_INVALID_STATE, NCC_SVCID );
    }
}

DWORD Accept::gotoNextState( DISP_EVENT* evt, TcpState* & nextState )
{
    DWORD ret = SUCCESS;
    CTABEGIN( "Accept::gotoNextState(evt)");
    HccCallObject* callObj = _lineObject->getCurrentCall();
    ASSERT(_lineObject->getCurrentCall() != NULL );

    _setEvtAddrObjhd(evt, _lineObject->getCurrentCall());

    switch (evt->id)
    {
        case ADIEVN_SIGNALBIT_CHANGED:
            _handleRemoteHangUp( evt, callObj, nextState, _idleNext );
            return SUCCESS;
        // DTMF detector is stopped when transtion to WaitAppRejAccept state
        // swallow done event here
        case ADIEVN_DTMF_DETECT_DONE:
            return SUCCESS;
        default:
            return CTALOGERROR( evt->ctahd,
                                NCCERR_INVALID_CALL_STATE, NCC_SVCID );
    }
}

/**----------------------------------------------------------------------------
 * Class Reject
 *---------------------------------------------------------------------------*/
DWORD Reject::gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState )
{
    CTABEGIN( "Reject::gotoNextState(cmd)");
    return CTALOGERROR( cmd->ctahd, CTAERR_INVALID_STATE, NCC_SVCID );
}

 DWORD Reject::gotoNextState( DISP_EVENT* evt, TcpState* & nextState )
 {
     DWORD ret = SUCCESS;
     CTABEGIN( "Reject::gotoNextState(evt)");

     HccCallObject* callObj = _lineObject->getCurrentCall();
     ASSERT( callObj != NULL );
     _setEvtAddrObjhd( evt, callObj );

     switch (evt->id)
     {
         case ADIEVN_SIGNALBIT_CHANGED:
            if ( _handleRemoteHangUp( evt, callObj, nextState, _idleNext ) )
            {
                // when remote hangs up, stop the busy tone.
                ret = adiSpiStopTones( evt->ctahd, NCC_SVCID );
                if ( ret != SUCCESS )
                    CTALOGERROR( evt->ctahd, ret, NCC_SVCID );
                return SUCCESS;
            }
            else
                return SUCCESS;
         // DTMF detector is stopped when transition to WaitAppRejAccept state,
         // done event should be swallowed here.
         case ADIEVN_DTMF_DETECT_DONE:
            return SUCCESS;
         default:
            return CTALOGERROR( evt->ctahd,
                                CTAERR_INVALID_STATE, NCC_SVCID );
     }
 }

/**----------------------------------------------------------------------------
 * Class Answer
 *---------------------------------------------------------------------------*/
DWORD Answer::gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState )
{
    DWORD ret = SUCCESS;
    CTABEGIN( "Answer::gotoNextState(cmd)");

    HccCallObject* callObj = _lineObject->objhdToCallObject( cmd->objHd );
    if ( callObj == NULL )
        return CTALOGERROR ( cmd->ctahd, CTAERR_INVALID_HANDLE, NCC_SVCID );

    _lineObject->updateNccStateMachines( cmd, callObj );

    switch ( cmd->id )
    {
        case NCCCMD_DISCONNECT_CALL:
            //change AfBf = 11->00
            ret = adiSpiAssertSignal( cmd->ctahd, 0x0, (WORD)NCC_SVCID );

            if ( ret != SUCCESS )
                return CTALOGERROR( cmd->ctahd, ret, NCC_SVCID );
            else
            {
                nextState = _waitPartyLeaveLineNext;
                return ret;
            }
        default:
            return CTALOGERROR( cmd->ctahd, CTAERR_INVALID_STATE, NCC_SVCID );
    }
}

DWORD Answer::gotoNextState( DISP_EVENT* evt, TcpState* & nextState )
{
    DWORD ret = SUCCESS;
    CTABEGIN( "Answer::gotoNextState(evt)");
    HccCallObject* callObj = _lineObject->getCurrentCall();
    ASSERT ( callObj != NULL );
    _setEvtAddrObjhd( evt, callObj );

    switch (evt->id)
    {
    case ADIEVN_SIGNALBIT_CHANGED:
        //remote hang up
        _handleRemoteHangUp( evt, callObj, nextState, _disconnectedNext );
        return SUCCESS;

    // Note: ring tone may be stopped in accept state
    case ADIEVN_TONES_DONE:
    // Note: DTMF detector is stopped when transition to WaitAppRejAccept state
    // swallow done event here
    case ADIEVN_DTMF_DETECT_DONE:
    // Note: Stimer is stopped in SendAddr, swallow done event here.
    case ADIEVN_TIMER_DONE:
        return SUCCESS;
    default:
        return CTALOGERROR( evt->ctahd,
                            CTAERR_INVALID_STATE, NCC_SVCID );
    }
}

/**----------------------------------------------------------------------------
 * Class WaitPartyLeaveLine
 *---------------------------------------------------------------------------*/
DWORD WaitPartyLeaveLine::gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState )
{
    CTABEGIN( "WaitPartyLeaveLine::gotoNextState(cmd)");
    return CTALOGERROR( cmd->ctahd, CTAERR_INVALID_STATE, NCC_SVCID );
}

DWORD WaitPartyLeaveLine::gotoNextState( DISP_EVENT* evt, TcpState* & nextState )
{
    DWORD ret = SUCCESS;
    CTABEGIN( "WaitPartyLeaveLine::gotoNextState(evt)");

    HccCallObject* callObj = _lineObject->getCurrentCall();
    ASSERT ( callObj != NULL );
    _setEvtAddrObjhd( evt, callObj );

    switch (evt->id)
    {
        case ADIEVN_SIGNALBIT_CHANGED:
            _handleRemoteHangUp( evt, callObj, nextState, _idleNext );
            return SUCCESS;
        default:
            return CTALOGERROR( evt->ctahd,
                                CTAERR_INVALID_STATE, NCC_SVCID );
    }
}


/**----------------------------------------------------------------------------
 * Class Seizing
 *---------------------------------------------------------------------------*/
DWORD Seizing::gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState )
{
    CTABEGIN( "Seizing::gotoNextState(cmd)");
    return CTALOGERROR( cmd->ctahd, CTAERR_INVALID_STATE, NCC_SVCID );
}

DWORD Seizing::gotoNextState( DISP_EVENT* evt, TcpState* & nextState )
{
    DWORD ret = SUCCESS;
    CTABEGIN( "Seizing::gotoNextState(evt)");
    HccCallObject* callObj = _lineObject->getCurrentCall();
    ASSERT( callObj != NULL );

    _setEvtAddrObjhd( evt, callObj );

     switch (evt->id)
     {
     case ADIEVN_TIMER_DONE:
        // Timer done with reason CTA_REASON_FINISHED =>
        // rising edge of wink is not received in desired time period
        // ( Rtimer timout)
        if ( evt->value == CTA_REASON_FINISHED )
        {
            evt->id = NCCEVN_CALL_DISCONNECTED;
            evt->value = NCC_DIS_NO_ACKNOWLEDGEMENT;
            _lineObject->updateNccStateMachines( evt, callObj );
            dispQueueEvent( evt ); //queue NCCEVN_CALL_DISCONNECTED
            nextState = _idleNext;
            return ret;
        }
        else
            return SUCCESS;     //swallow timer done event if any other reason

     case ADIEVN_SIGNALBIT_CHANGED:
        if ( (!(evt->value ^ 0xA1) || !(evt->value ^ 0xB1)) && !(evt->size ^ 0xC) )
        {   //get wink in desired time period
            //stop Rtimer, which is to detect the rising edge of wnk
            ret = adiSpiStopTimer( evt->ctahd, NCC_SVCID );
            if ( ret != SUCCESS )
            {
                evt->id = NCCEVN_PROTOCOL_ERROR;
                evt->value = CTAERR_BOARD_ERROR;
                evt->size = 0;
                dispQueueEvent( evt );
                return CTALOGERROR ( evt->ctahd, ret, NCC_SVCID );
            }
            else
            {
                nextState = _seizingAcked1Next;
                return SUCCESS;
            }
        }
        else
            return SUCCESS;     //swallow other bit change event
     default:
        return CTALOGERROR( evt->ctahd,
                            CTAERR_INVALID_STATE, NCC_SVCID );
     }
 }


/**----------------------------------------------------------------------------
 * Class SeizingAcked1
 *---------------------------------------------------------------------------*/
DWORD SeizingAcked1::gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState )
{
    CTABEGIN( "SeizingAcked1::gotoNextState(cmd)");
    return CTALOGERROR( cmd->ctahd, CTAERR_INVALID_STATE, NCC_SVCID );
}

DWORD SeizingAcked1::gotoNextState( DISP_EVENT* evt, TcpState* & nextState )
{
    DWORD ret = SUCCESS;
    CTABEGIN( "SeizingAcked1::gotoNextState(evt)");
    ASSERT( _lineObject->getCurrentCall() != NULL );
    _setEvtAddrObjhd(evt, _lineObject->getCurrentCall());

    switch (evt->id)
    {
        // Done event for Rtimer, a timer to detect the rising edge of wnk,
        // which is stopped when leaving Seizing state
        case ADIEVN_TIMER_DONE:
            ASSERT ( evt->value == CTA_REASON_STOPPED );
            //NCC.X.ADI_WNK.maxwinktime = 4900,
            //start timer to detect falling edge (Ftimer) of wnk
            ret = adiSpiStartTimer( evt->ctahd, 4900, 0, (WORD)NCC_SVCID );
            if ( ret != SUCCESS )
                return CTALOGERROR( evt->ctahd, ret, NCC_SVCID );
            else
            {
                nextState = _seizingAcked2Next;
                return SUCCESS;
            }
        default:
            return CTALOGERROR( evt->ctahd,
                                CTAERR_INVALID_STATE, NCC_SVCID );
    }

}


/**----------------------------------------------------------------------------
 * Class SeizingAcked2
 *---------------------------------------------------------------------------*/
DWORD SeizingAcked2::gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState )
{
    CTABEGIN( "SeizingAcked2::gotoNextState(cmd)");
    return CTALOGERROR( cmd->ctahd, CTAERR_INVALID_STATE, NCC_SVCID );
}

DWORD SeizingAcked2::gotoNextState( DISP_EVENT* evt, TcpState* & nextState )
{
    DWORD ret = SUCCESS;
    CTABEGIN( "SeizingAcked2::gotoNextState(evt)");
    HccCallObject* callObj = _lineObject->getCurrentCall();
    ASSERT( callObj != NULL );
    _setEvtAddrObjhd( evt, callObj );

    switch (evt->id)
    {
        case ADIEVN_TIMER_DONE:
            // Ftimer timeout, glare happens
            if ( evt->value == CTA_REASON_FINISHED )
            {
                //glare is detected, should drop outbound call and create
                //an inbound call
                evt->id = NCCEVN_CALL_RELEASED;
                evt->value = NCC_RELEASED_GLARE;

                // destroy the outbound call
                _lineObject->destroyCallObject( callObj );
                _lineObject->setCurrentCall( NULL );
                _lineObject->updateNccStateMachines( evt, callObj );
                // queue NCCEVN_CALL_RELEASED
                dispQueueEvent( evt );

                HccCallObject* callObj = _lineObject->createCallObject( NCC_CALL_INBOUND );
                if ( callObj == NULL )
                    return CTALOGERROR( evt->ctahd, CTAERR_OUT_OF_MEMORY, NCC_SVCID );

                _lineObject->setCurrentCall( callObj );

                evt->id = NCCEVN_SEIZURE_DETECTED;
                evt->objHd = callObj->getServerCallhd();
                evt->value = 0;
                _lineObject->updateNccStateMachines( evt, callObj );

                //queue NCCEVN_SEIZURE_DETECTED
                dispQueueEvent( evt );
                nextState = _seizureNext;
            }
            return SUCCESS;     //swallow other ADIEVN_TIMER_DONE events

        case ADIEVN_SIGNALBIT_CHANGED:      //receiving the falling edge of wnk
            if ( (!(evt->value ^ 0xA0) || !(evt->value ^ 0xB0)) && !(evt->size ^ 0x0))
            {
                // stop Ftimer, a timer to detect the following edge of wink
                ret = adiSpiStopTimer( evt->ctahd, NCC_SVCID );
                if ( ret != SUCCESS )
                    CTALOGERROR( evt->ctahd, ret, NCC_SVCID );

                evt->id = NCCEVN_PLACING_CALL;
                evt->value = 0;
                evt->size = 0;
                _lineObject->updateNccStateMachines( evt, _lineObject->getCurrentCall() );

                // present NCCEVN_PLACING_CALL event to application
                dispQueueEvent( evt );

                // send the called address out
                // Note: In current implementation, all address are sent out together.
                //       Current implementation doesn't support overlap sending
                /*ret = adiSpiStartDTMF( evt->ctahd,
                      _lineObject->getCurrentCall()->getCallStatus()->calledaddr,
                      NULL, NCC_SVCID );*/

                ret = adiSpiStartDial( evt->ctahd,
                    _lineObject->getCurrentCall()->getCallStatus()->calledaddr,
                    NULL, NCC_SVCID );

                if ( ret != SUCCESS )
                {   //present protocol error event to APP
                    _sendBoardErrorEvent ( evt, ret );
                    CTALOGERROR( evt->ctahd, ret, NCC_SVCID );
                }

                //manufacture NCCEVN_CALL_PROCEEDING event
                evt->id = NCCEVN_CALL_PROCEEDING;
                _lineObject->updateNccStateMachines( evt, _lineObject->getCurrentCall() );
                dispQueueEvent( evt );
                nextState = _sendAddrNext;
                return SUCCESS;

            }
            return SUCCESS;     //swallow other ADIEVN_SIGNALBIT_CHANGED events

        default:
            return CTALOGERROR( evt->ctahd,
                     CTAERR_INVALID_STATE, NCC_SVCID );
        }
}

/**----------------------------------------------------------------------------
 * Class SendAddr
 * The class can be extended to handle overlap sending with nccSendDigits
 * By checking if the ending digits is '~' or not, TCP can decide if the
 * digits to be sent is completed. Currently, this is not implemented.
 *---------------------------------------------------------------------------*/
DWORD SendAddr::gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState )
{
    CTABEGIN( "SendAddr::gotoNextState(cmd)");
    return CTALOGERROR( cmd->ctahd, CTAERR_INVALID_STATE, NCC_SVCID );
}

DWORD SendAddr::gotoNextState( DISP_EVENT* evt, TcpState* & nextState )
{
    DWORD ret = SUCCESS;
    CTABEGIN( "SendAddr::gotoNextState(evt)");
    ASSERT( _lineObject->getCurrentCall() != NULL);
    _setEvtAddrObjhd( evt, _lineObject->getCurrentCall());

    switch (evt->id)
    {
        case ADIEVN_DIAL_DONE:
            // this evt is associated with adiSpiStartDial().
            // called address is sent out scuccessfully
            if ( evt->value != CTA_REASON_FINISHED )
            {
                _sendBoardErrorEvent( evt, evt->value );
                return CTALOGERROR( evt->ctahd, evt->value, NCC_SVCID );
            }

            // start a timer to wait remote party to assert signal (Stimer)
            // NCC.START_waitforPCtime = 10000
            ret = adiSpiStartTimer( evt->ctahd, 10000, 0, (WORD)NCC_SVCID );
            if ( ret != SUCCESS )
                return CTALOGERROR( evt->ctahd, ret, NCC_SVCID );
            return SUCCESS;

        case ADIEVN_SIGNALBIT_CHANGED:
            if ( (!(evt->value ^ 0xA1) || !(evt->value ^ 0xB1)) && !(evt->size ^ 0xc) )
            {
                //remote party accepts this call
                evt->id = NCCEVN_CALL_CONNECTED;
                evt->value = NCC_CON_SIGNAL;
                _lineObject->updateNccStateMachines( evt,
                              _lineObject->getCurrentCall() );
                dispQueueEvent( evt );

                nextState = _answerNext;
                // try to stop Stimer if it's active
                ret = adiSpiStopTimer( evt->ctahd, NCC_SVCID );
                if ( ret != SUCCESS && ret != CTAERR_FUNCTION_NOT_ACTIVE )
                {
                    _sendBoardErrorEvent( evt, ret );
                    CTALOGERROR( evt->ctahd, ret, NCC_SVCID );
                }
            }
            return SUCCESS;         //swallow other ADIEVN_SIGNALBIT_CHANGED events

        case ADIEVN_TIMER_DONE:
            // Ftimer is stopped when leaving SeizingAcked2 state,
            // catch done event here
            if ( evt->value == CTA_REASON_STOPPED )
                return SUCCESS;
            else if (evt->value == CTA_REASON_FINISHED ) // Stimer timeout
            {   // remote didn't answer the call (e.g. nccAnswerCall is not
                // invoked) in the specifed time, the call gets disconnected
                evt->id = NCCEVN_CALL_DISCONNECTED;
                evt->value = NCC_DIS_TIMEOUT;
                dispQueueEvent( evt );

                _lineObject->updateNccStateMachines( evt,
                            _lineObject->getCurrentCall() );
                nextState = _idleNext;

                //change AfBf = 11->00
                ret = adiSpiAssertSignal( evt->ctahd, 0x0, (WORD)NCC_SVCID );
                if ( ret != SUCCESS )
                    return CTALOGERROR( evt->ctahd, ret, NCC_SVCID );
            }
            return SUCCESS;
        default:
            return CTALOGERROR( evt->ctahd,
                        CTAERR_INVALID_STATE, NCC_SVCID );
    }
}

/**----------------------------------------------------------------------------
 * Class Disconnected
 *---------------------------------------------------------------------------*/
DWORD Disconnected::gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState )
{
    DWORD ret = SUCCESS;
    CTABEGIN( "Disconnected::gotoNextState(cmd)");

    switch ( cmd->id )
    {
        case NCCCMD_RELEASE_CALL:
        {
            HccCallObject* releaseCall = _lineObject->objhdToCallObject( cmd->objHd );
            if ( releaseCall == NULL )
                return CTALOGERROR (cmd->ctahd, CTAERR_INVALID_HANDLE, NCC_SVCID );


            // The released call is still the call holding the line.
            ASSERT ( releaseCall == _lineObject->getCurrentCall() );

            DISP_EVENT evt = {0};
            evt.ctahd = cmd->ctahd;
            _setEvtAddrObjhd( &evt, releaseCall );

            ret = adiSpiAssertSignal( cmd->ctahd, 0 , NCC_SVCID );
            if ( ret != SUCCESS )
                return CTALOGERROR ( cmd->ctahd, ret, NCC_SVCID );
            evt.id = NCCEVN_CALL_RELEASED;
            dispQueueEvent(&evt);
            _lineObject->destroyCallObject( releaseCall );
            nextState = _idleNext;
            return SUCCESS ;
        }
        default:
            return CTALOGERROR( cmd->ctahd, CTAERR_INVALID_STATE, NCC_SVCID );
    }
}

DWORD Disconnected::gotoNextState( DISP_EVENT* evt, TcpState* & nextState )
{
    DWORD ret = SUCCESS;
    CTABEGIN( "Disconnected::gotoNextState(evt)");

    if (evt->id == ADIEVN_TIMER_DONE )
        return SUCCESS;

    return CTALOGERROR( evt->ctahd,
                        CTAERR_INVALID_STATE, NCC_SVCID );

}

/**----------------------------------------------------------------------------
 * Class WnkHostTcp
 *---------------------------------------------------------------------------*/
WnkHostTcp::WnkHostTcp ( CTAHD linehd,
                         HccLineObject* lineObject ) : HostTcp( linehd,
                                                                lineObject )
{
    //initialize Tcp current state and the starting state
    _currentState = &_unitialized;
    _startState = &_unitialized;

    //initialize Tcp state machine
    _unitialized.setNextStates( &_idle );
    _idle.setNextStates( &_seizing, &_seizure );
    _seizure.setNextStates( &_receiveAddr, &_idle );
    _receiveAddr.setNextStates( &_receiveAddr, &_waitRejAccept, &_idle );
    _waitRejAccept.setNextStates( &_answer, &_accept, &_reject, &_idle );
    _accept.setNextStates( &_answer, &_idle );
    _reject.setNextStates( &_idle );
    _seizing.setNextStates( &_idle, &_seizingAcked1 );
    _seizingAcked1.setNextStates( &_seizingAcked2);
    _seizingAcked2.setNextStates( &_seizure, &_sendAddr );
    _sendAddr.setNextStates( &_sendAddr, &_idle, &_answer );

    _answer.setNextStates( &_waitPartyLeaveLine, &_disconnected );
    _waitPartyLeaveLine.setNextStates( &_idle );
    _disconnected.setNextStates( &_idle );

    //set pointer of hccLineObject in each Tcp state object
    _unitialized.setLineObject( lineObject );
    _idle.setLineObject( lineObject );
    _seizure.setLineObject( lineObject );
    _receiveAddr.setLineObject( lineObject );
    _accept.setLineObject( lineObject );
    _reject.setLineObject( lineObject );
    _waitRejAccept.setLineObject( lineObject );
    _seizing.setLineObject( lineObject );
    _seizingAcked1.setLineObject( lineObject );
    _seizingAcked2.setLineObject( lineObject );
    _sendAddr.setLineObject( lineObject );
    _answer.setLineObject( lineObject );
    _waitPartyLeaveLine.setLineObject( lineObject );
    _disconnected.setLineObject( lineObject );

}



DWORD WnkHostTcp::handleNccCmd ( DISP_COMMAND* cmd )
{
    CTABEGIN("WnkHostTcp::handleNccCmd");
    DWORD ret = SUCCESS;

    if ( (ret = _lineObject->capabilityChecking( cmd->id )) != 0 )
        return CTALOGERROR ( cmd->ctahd, ret, NCC_SVCID );

    TcpState* nextState = _currentState;

    // The command that drives protocol state transition is dispatched to
    // individule TCP state object
    switch ( cmd->id )
    {
        case NCCCMD_AUTOMATIC_TRANSFER:
        case NCCCMD_GET_EXTENDED_CALL_STATUS:
        case NCCCMD_HOLD_CALL:
        case NCCCMD_RETRIEVE_CALL:
        case NCCCMD_SEND_CALL_MESSAGE:
        case NCCCMD_SEND_DIGITS:
        case NCCCMD_SEND_LINE_MESSAGE:
        case NCCCMD_SET_BILLING:
        case NCCCMD_TRANSFER_CALL:
        case NCCCMD_TCT_TRANSFER_CALL:
        case NCCCMD_GET_CALLID:
            return CTAERR_NOT_IMPLEMENTED;

        case NCCCMD_BLOCK_CALLS:
            if ( cmd->size2 != NCC_BLOCK_REJECTALL )
                return CTAERR_NOT_IMPLEMENTED;

            _lineObject->updateNccStateMachines( cmd, NULL );
            if ( _currentState->getStateId() == HOSTTCP_STATE_IDLE )
            {
                _idle.setBlockState( HCC_CALL_BLOCKED );
                DISP_EVENT evt = {0};
                evt.ctahd = cmd->ctahd;
                evt.id = NCCEVN_CALLS_BLOCKED;
                evt.addr.source = NCC_SVCID;
                evt.addr.destination = CTA_APP_SVCID;
                dispQueueEvent( &evt );
                _lineObject->updateNccStateMachines( &evt, NULL );
                return SUCCESS;
            }
            else
            {
                _idle.setBlockState( HCC_CALL_BLOCKING );
                return SUCCESS;
            }

        case NCCCMD_UNBLOCK_CALLS:
        {
            _lineObject->updateNccStateMachines( cmd, NULL );
            _idle.setBlockState( HCC_CALL_NOT_BLOCKED );
            if ( _idle.getBusyToneState() == 1 )
            {
                adiSpiStopTones( cmd->ctahd, NCC_SVCID );
                _idle.setBusyToneState( 0 );
            }

            DISP_EVENT evt = {0};
            evt.ctahd = cmd->ctahd;
            evt.id = NCCEVN_CALLS_UNBLOCKED;
            evt.addr.source = NCC_SVCID;
            evt.addr.destination = CTA_APP_SVCID;
            dispQueueEvent( &evt );
            _lineObject->updateNccStateMachines( &evt, NULL );

            return SUCCESS;
        }
        case NCCCMD_STOP_PROTOCOL:
        {
            ret = adiSpiStopSignalDetector( cmd->ctahd, NCC_SVCID );
            if ( ret != SUCCESS )
                return CTALOGERROR ( cmd->ctahd, ret, NCC_SVCID );

            ret = adiSpiStopCCAssist( cmd->ctahd, NCC_SVCID );
            if (ret != SUCCESS)
                return CTALOGERROR( cmd->ctahd, ret, NCC_SVCID );

            _lineObject->updateNccStateMachines( cmd, NULL );

            return SUCCESS;
        }
        case NCCCMD_RELEASE_CALL:
        {
            HccCallObject* callObj = _lineObject->objhdToCallObject( cmd->objHd );
            if ( callObj == NULL )
                return CTALOGERROR( cmd->ctahd, CTAERR_INVALID_HANDLE, NCC_SVCID );
            else
            {
                if ( _currentState->getStateId() != HOSTTCP_STATE_ANSWER )
                {
                    if ( _lineObject->getCurrentCall() == callObj )
                        return CTALOGERROR ( cmd->ctahd, CTAERR_INVALID_STATE, NCC_SVCID );

                    DISP_EVENT evt = {0};
                    evt.ctahd = cmd->ctahd;
                    evt.id = NCCEVN_CALL_RELEASED;
                    evt.addr.source = NCC_SVCID;
                    evt.addr.destination = CTA_APP_SVCID;
                    evt.objHd = callObj->getServerCallhd();
                    _lineObject->destroyCallObject( callObj );
                    dispQueueEvent( &evt );
                    return SUCCESS;
                }
                else
                {
                    ret = _currentState->gotoNextState( cmd, nextState );
                    if ( ret == SUCCESS )
                        setCurrentState ( nextState );
                    return ret;
                }
            }
        }
        default:
            ret = _currentState->gotoNextState( cmd, nextState );
            if ( ret == SUCCESS )
                setCurrentState( nextState );

            return ret;
    }
}

DWORD WnkHostTcp::handleAdiEvt (DISP_EVENT* evt)
{
    CTABEGIN("wnkHostTcp::handleAdiEvt");
    DWORD ret;

    ASSERT( evt->ctahd == _linehd );

    TcpState* nextState = _currentState;

    switch ( evt->id )
    {
        // signal detection is stopped when stopping protocol, swallow done
        // event here
        case ADIEVN_SIGNAL_DETECT_DONE:
            return SUCCESS;

        // Handle stopping CC assist done event here
        case ADIEVN_STOP_CC_ASSIST_DONE:
            evt->addr.destination = CTA_APP_SVCID;
            evt->addr.source = NCC_SVCID;
            //keep reason code: CTA_REASON_FINISHED in value field
            evt->size = 0;
            evt->id = NCCEVN_STOP_PROTOCOL_DONE;
            _lineObject->updateNccStateMachines( evt, NULL );
            dispQueueEvent( evt );
            return SUCCESS;

        // Note: the following two events are not supported in WnkHostTcp now
        // There's no corresponding state in wnkHostTcp for NCC line
        // state = OutOfService
        case ADIEVN_OUT_OF_SERVICE:
            evt->addr.destination = CTA_APP_SVCID;
            evt->addr.source = NCC_SVCID;
            evt->size = 0;
            evt->id = NCCEVN_LINE_OUT_OF_SERVICE;
            _lineObject->updateNccStateMachines( evt, NULL );
            dispQueueEvent( evt );
            return SUCCESS;

        case ADIEVN_IN_SERVICE:
            evt->addr.destination = CTA_APP_SVCID;
            evt->addr.source = NCC_SVCID;
            evt->size = 0;
            evt->id = NCCEVN_LINE_IN_SERVICE;
            _lineObject->updateNccStateMachines( evt, NULL );
            dispQueueEvent( evt );
            return SUCCESS;

        default:
            ret = _currentState->gotoNextState( evt, nextState );
            if ( ret == SUCCESS )
                setCurrentState( nextState );
            return (ret == SUCCESS) ? ret : CTALOGERROR ( evt->ctahd, ret, NCC_SVCID );
     }
}



DWORD WnkHostTcp::setCurrentState ( TcpState* st)
{
    CTABEGIN("HostTcp::setCurrentState");
    DWORD ret = SUCCESS;

    // Broadcast media events to indicate if the current call state allows
    // media function.

    HccCallObject* callObj = _lineObject->getCurrentCall();
    DISP_EVENT evt = {0};

    evt.ctahd = _lineObject->getLineHd();
    evt.userid = NCC_SVCID;
    if ( callObj != NULL )
        evt.objHd = callObj->getServerCallhd();
    evt.addr.destination = RTC_BROADCAST;
    evt.addr.source = NCC_SVCID;
    evt.value = 0;

    if ( st->getMediaEnabled() )
        evt.id = NCCEVN_READY_FOR_MEDIA;
    else
        evt.id = NCCEVN_NOT_READY_FOR_MEDIA;

    dispQueueEvent( &evt );

    // The generic action to be done when entering the TCP Idle state.
    if ( st->getStateId() == HOSTTCP_STATE_IDLE )
    {
        // check if there's a pending blocking call request.
        // If there's one, send NCCEVN_CALLS_BLOCKED to application
        if ( ((Idle*) st)->getBlockState() == HCC_CALL_BLOCKING )
        {
            DISP_EVENT evt = {0};
            evt.ctahd = _lineObject->getLineHd();
            evt.id = NCCEVN_CALLS_BLOCKED;
            evt.addr.destination = CTA_APP_SVCID;
            evt.addr.source = NCC_SVCID;
            dispQueueEvent ( &evt );
            ((Idle*)st)->setBlockState( HCC_CALL_BLOCKED );
            _lineObject->updateNccStateMachines( &evt, NULL );
        }
        // Anytime, when TCP state enters Idle state, there's no active
        // call holding the line, the current call in line object must
        // be set to NULL
        _lineObject->setCurrentCall( NULL );
    }

    _currentState = st;
    if ( ret != SUCCESS )
       CTALOGERROR( _lineObject->getLineHd(), ret, NCC_SVCID );
    return ret;
}
