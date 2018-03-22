/*****************************************************************************
* FILE:        hostTcp.cpp
*
* DESCRIPTION:
*              This file contains the:
*
*                - The definition for class TcpState, HostTcp and
*                  HccMgrContext
*****************************************************************************/

#include "hostTcp.h"


/**----------------------------------------------------------------------------
 * Class TcpState
 *---------------------------------------------------------------------------*/
TcpState::TcpState ( DWORD id )
{
    _stateId = id;
    _mediaEnabled = false;
    _lineObject = NULL;
}

TcpState::TcpState ( DWORD id, bool enableMedia )
{
    _stateId = id;
    _mediaEnabled = enableMedia;
    _lineObject = NULL;
}

TcpState::TcpState ( DWORD id, HccLineObject* lineObject )
{
    _stateId = id;
    _mediaEnabled = false;
    _lineObject = lineObject;
}

TcpState::TcpState ( DWORD id, bool enableMedia, HccLineObject* lineObject )
{
    _stateId = id;
    _mediaEnabled = enableMedia;
    _lineObject = lineObject;
}

/**
 * A utility function to set 3 fields in an event:
 *  source addre = NCC_SVCID (NCC service)
 *  destionation = CTA_APP_SVCID (application)
 *  objHd        = server side handle of a nonNULL call object
 */
void TcpState::_setEvtAddrObjhd ( DISP_EVENT* evt, HccCallObject* callObj )
{
    evt->addr.destination = CTA_APP_SVCID;
    evt->addr.source = NCC_SVCID;
    if ( callObj != NULL )
        evt->objHd = callObj->getServerCallhd();
}

/**
 * A utility function to handle remote hanging up.
 * return
 *      true: if the passed in event reflects a remote hanging up.
 *              in this case, nextState is updated with disconState.
 *      false: if the passed event doesn't reflect a remote hanging up
 */
bool TcpState::_handleRemoteHangUp ( DISP_EVENT* evt,
                                     HccCallObject* callObj,
                                     TcpState* & nextState,
                                     TcpState* disconState )
{
    ASSERT (evt->id == ADIEVN_SIGNALBIT_CHANGED );

    if ( (!(evt->value ^ 0xA0) || !(evt->value ^ 0xB0)) && !(evt->size ^ 0x0) )
    {   //remote hang up
        evt->id = NCCEVN_CALL_DISCONNECTED;
        evt->value = NCC_DIS_SIGNAL;
        _lineObject->updateNccStateMachines( evt, callObj );
        dispQueueEvent( evt );
        nextState = disconState;
        return true;
    }
    else //swallow other digit change event
        return false;
}

/**
 * A utility function to change an event to a board error event and queue it
 * to Natural Access.
 */
void TcpState::_sendBoardErrorEvent( DISP_EVENT* evt, DWORD boardErrorReason )
{
    evt->id = NCCEVN_PROTOCOL_ERROR;
    evt->value = CTAERR_BOARD_ERROR;
    evt->size = boardErrorReason;
    evt->objHd = 0;   //this event doesn't need to associate with any callobj
    dispQueueEvent( evt );
}

/**---------------------------------------------------------------------------
 * Class HostTcp
 *---------------------------------------------------------------------------*/

/**
 * This functions handles the NCC commands dispatched by hccProcessCommand()
 * It continues to dispatch the NCC command to a call state object to handle
 */
DWORD HostTcp::handleNccCmd (DISP_COMMAND* cmd )
{
    CTABEGIN("HostTcp::handleNccCmd");
    DWORD ret = SUCCESS;

    ASSERT( (cmd->ctahd) == _linehd );

    if ( (ret = _lineObject->capabilityChecking( cmd->id )) != 0 )
        return CTALOGERROR ( cmd->ctahd, ret, NCC_SVCID );

    TcpState* nextState = _currentState;

    ret = _currentState->gotoNextState( cmd, nextState );
    if ( ret == SUCCESS )
        ret = setCurrentState( nextState );

    return (ret == SUCCESS) ? ret : CTALOGERROR( cmd->ctahd, ret, NCC_SVCID );
}


/**
 * This functions handles the ADI events dispatched by hccProcessEvnet()
 * It continues to dispatch the ADI events to a call state object to handle
 */
DWORD HostTcp::handleAdiEvt (DISP_EVENT* evt)
{
    CTABEGIN("HostTcp::handleAdiEvt");
    DWORD ret;

    ASSERT( evt->ctahd == _linehd );

    TcpState* nextState = _currentState;

    ret = _currentState->gotoNextState( evt, nextState );
    if ( ret == SUCCESS )
        ret = setCurrentState( nextState );

    return (ret == SUCCESS) ? ret : CTALOGERROR( evt->ctahd, ret, NCC_SVCID );
}

