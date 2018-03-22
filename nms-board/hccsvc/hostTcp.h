/*****************************************************************************
* FILE:        hostTcp.h
*
* DESCRIPTION:
*              This file contains the:
*
*                - The declaration for class TcpState, HostTcp and
*                  HccMgrContext
*****************************************************************************/

#ifndef HOSTTCP_INCLUDED
#define HOSTTCP_INCLUDED

#include "hccsys.h"
#include "hccLineObject.h"

/**----------------------------------------------------------------------------
 * Class TcpState
 * Desc: An abstract class to define a call state in a protocol state machine
 *       It's the superclass for a real call state class.
 *---------------------------------------------------------------------------*/
class TcpState
{
protected:
    DWORD               _stateId;        //the id of the call state

    /* !!! Note:
     * By cheching _mediaEnabled field, NCC service can decide whether
     * NCCEVN_READY_FOR_MEDIA or NCCEVN_NOT_READY_FOR_MEDIA should be sent
     * to destination = RTC_BROARDCAST or not when entering this state.
     * This field is initialzed as true or false when the call state object
     * is created. "true" = media function is allowed, "false" = not allowed
     */
    bool                _mediaEnabled;


    HccLineObject*      _lineObject;    //The addr of the NCC line object

public:
    TcpState () { _stateId = HOSTTCP_STATE_NULL; }
    TcpState ( DWORD stateId );
    TcpState ( DWORD stateId, bool enableMedia );
    TcpState ( DWORD sateeId, HccLineObject* lineObject );
    TcpState ( DWORD id, bool enableMedia, HccLineObject* lineObject );

    virtual ~TcpState (){}

    DWORD getStateId () const                   { return _stateId; }
    bool getMediaEnabled () const               { return _mediaEnabled; }
    HccLineObject* getLineObject() const        { return _lineObject; }

    void setMediaEnabled ( bool s )             { _mediaEnabled = s; }
    void setLineObject ( HccLineObject* line )  { _lineObject = line; }

    /**
     * This function is invoked when an NCC command is passed to this call
     * object to handle.
     * It interprets NCC cmd to ADI SPI functions, and finds the next
     * call state that the state machine should move to.
     *
     * Overwrite this function when defining your own call state class
     */
    virtual DWORD gotoNextState ( DISP_COMMAND* cmd,
                                  TcpState* & nextState ) = 0;

    /**
     * This function is invoked when an ADI event is passed to this call
     * object to handle.
     * Depends on the type of ADI event, it interprets the ADI event to
     * an NCC event and sends this event to application, or swallows
     * the event if it's a low level event.
     *
     * Overwrite this function when defining your own call state class
     */
    virtual DWORD gotoNextState ( DISP_EVENT * ctaevt,
                                  TcpState* & nextState ) = 0;

    /**
     * A utility function to set source, destionation and object handle
     * for an event.
     */
    void _setEvtAddrObjhd ( DISP_EVENT* evt, HccCallObject* callObj );

    /**
     * A utility function to handle remote hanging up
     */
    bool _handleRemoteHangUp ( DISP_EVENT* evt, HccCallObject* callObj,
                               TcpState* & nextState, TcpState* disconState );
    /**
     * A utility function to change an event to a board error event and queue
     * it to Natural Access
     */
    void _sendBoardErrorEvent( DISP_EVENT* evt, DWORD boardErrorReason );

};


/**----------------------------------------------------------------------------
 * Class HostTcp
 * Desc: An abstract class to define a protocol state machine
 *       Any protocol state machine can be defined by inheriting it.
 *---------------------------------------------------------------------------*/
class HostTcp
{
protected:

    CTAHD               _linehd;            //line handle
    HccLineObject*      _lineObject;        //addr of the NCC line object

    /* currently, protocol capability is defined as one attribute in
     * HccLineObject class.
     * That field may be moved here in your implementation if considering
     * capablity is an attribute of a protocol
     */
    //NCC_PROT_CAP        _protCap;

    /*
     * The following two variables must be initialized in subclass before
     * being used
     */
    TcpState* _startState;   //the addr of the first call state of the state machine
    TcpState* _currentState; //the addr of the current call state

public:
    HostTcp ( CTAHD linehd, HccLineObject* lineObject )
    {
        _linehd             = linehd;
        _lineObject         = lineObject;
        _startState         = NULL;
        _currentState       = NULL;
    }

    virtual ~HostTcp () {}

    CTAHD               getLineHd ()         { return _linehd; }
    HccLineObject*      getLineObject ()     { return _lineObject; }
    TcpState*           getStartState ()     { return _startState; }
    TcpState*           getCurrentState ()   { return _currentState; }


    /**
     * This function is to do state transition. It sets the current state to
     * the addr of the call state passed-in, and broadcasts an
     * NCCEVN_READY_FOR_MEDIA/NCCEVN_NOT_READY_FOR_MEDIA event to Natural Access.
     *
     * This function can be overwritten to implement more functionalities which
     * are specific to a protocol or specific to a call state when doing state
     * transition.
     */
    virtual DWORD setCurrentState ( TcpState* nextstate )
    {
        //set the current state to addr of next call state
        _currentState = nextstate;

        /* depends on the value of _mediaEnable of next call state
         * create NCCEVN_READY_FOR_MEDIA/NCCEVN_NOT_READY_FOR_MEDIA event
         * and send it to destination = RTC_BROADCAST;
         */
        DISP_EVENT evt = {0};
        evt.ctahd = _linehd;
        evt.userid = NCC_SVCID;
        evt.addr.destination = RTC_BROADCAST;
        evt.addr.source = NCC_SVCID;
        evt.value = 0;
        if ( nextstate->getMediaEnabled() )
            evt.id = NCCEVN_READY_FOR_MEDIA;
        else
            evt.id = NCCEVN_NOT_READY_FOR_MEDIA;
        dispQueueEvent( &evt );
        return SUCCESS;
    }

    /**
     * This functions handles the NCC commands dispatched by hccProcessCommand()
     *
     * Overwrite this function for your needs
     */
    virtual DWORD handleNccCmd (DISP_COMMAND* cmd );

    /**
     * This functions handles the ADI events dispatched by hccProcessEvnet()
     *
     * Overwrite this function for your needs
     */
    virtual DWORD handleAdiEvt (DISP_EVENT* evt);

};

/**----------------------------------------------------------------------------
 * Class HccMgrContext
 * Desc: Defines the NCC service context
 *---------------------------------------------------------------------------*/
class HccMgrContext
{
private:
    CTAHD           _lineHd;      //line handle, assigned when openning service.


    HccLineObject*  _lineObject;  //The addr of line object
    HostTcp*        _hostTcp;     //The addr of a protocol object.
                                  //created when nccStartProtocol is invoked

public:
    HccMgrContext ( CTAHD linehd )
    {
        _lineHd = linehd;
        _lineObject = new HccLineObject( linehd );
        _hostTcp = NULL;
    }

    ~HccMgrContext ()
    {
        delete _lineObject;
        _lineObject = NULL;

        if (_hostTcp != NULL )
        {
            delete _hostTcp;
            _hostTcp = NULL;
        }
    }

    CTAHD getLineHd ()                  { return _lineHd; }
    HccLineObject* getLineObject ()     { return _lineObject; }
    HostTcp* getHostTcp ()              { return _hostTcp; }

    void setHostTcp ( HostTcp* tcp )    { _hostTcp = tcp; }
};

#endif //HOSTTCP_INCLUDED


