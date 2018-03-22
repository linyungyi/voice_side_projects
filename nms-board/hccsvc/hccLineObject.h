/*****************************************************************************
* FILE:        hccLineObject.h
*
* DESCRIPTION:
*              This file contains the:
*
*                - The declaration of class HccCallObject and HccLineObject
*****************************************************************************/

#ifndef HCCLINEOBJECT_INCLUDED
#define HCCLINEOBJECT_INCLUDED

#include "hccsys.h"

/**----------------------------------------------------------------------------
 * Class HccCallObject
 * Desc: Defines a call object
 *---------------------------------------------------------------------------*/
class HccCallObject
{
private:
    NCC_CALL_STATUS     _callStatus;    //status of the call object
    HccCallObject*      _prev;          //link to previous call object
    HccCallObject*      _next;          //link to next call object
    DWORD               _servercallhd;  //server side handle for this call obj

    static DWORD        _availablehd;   //available server side call handle

public:
    HccCallObject ( CTAHD linehd, DWORD dir );

    virtual ~HccCallObject(){}

    NCC_CALL_STATUS* getCallStatus ()   { return &_callStatus; }
    HccCallObject* getPrev ()           { return _prev; }
    HccCallObject* getNext ()           { return _next; }
    DWORD          getServerCallhd()    { return _servercallhd; }

    void setPrev ( HccCallObject* prev ) { _prev = prev; }
    void setNext ( HccCallObject* next ) { _next = next; }

    void setAddrsName ( char* calledAddr, char* callingAddr, char* callingName );
};

/**----------------------------------------------------------------------------
 * Class HccLineObject
 * Desc: Defines the NCC line Object
 *---------------------------------------------------------------------------*/
class HccLineObject
{
private:

    NCC_LINE_STATUS _lineStatus;  //line status
    NCC_PROT_CAP    _protcap;     //protocol capability

    HccCallObject*  _currentCall; //the addr of the current call object
    HccCallObject*  _heldCall;    //the addr of the call object on hold
    HccCallObject*  _firstCall;   //the header of the linked list of call objects

    CTAHD           _lineHd;      //the handle of the line, assigned when
                                  //NCC service is opened.

protected:
  // Maps command code to a pending command code.
  static const DWORD apiToPendingCmd[NCCCMD_QTY];
  // Maps event code to a pending command that is completed.
  static const DWORD eventToPendingCmd[NCCEVN_QTY];
  // Maps the command to required capality
  static const DWORD apiToRequiredCapability[NCCCMD_QTY];
  // Maps the event to required capability
  static const DWORD eventToRequiredCapability[NCCEVN_QTY];


public:

    HccLineObject ( CTAHD lineHd )
    {
        //basic initialization for line status struct
        memset( &_lineStatus, 0, sizeof ( NCC_LINE_STATUS ));
        _lineStatus.size = sizeof (NCC_LINE_STATUS);
        _lineStatus.state = NCC_LINESTATE_UNINITIALIZED;

        //basic initialization for protocol capability struct
        memset( &_protcap, 0, sizeof (NCC_PROT_CAP) );
        _protcap.size = sizeof (NCC_PROT_CAP);

        _currentCall = NULL;
        _heldCall = NULL;
        _firstCall = NULL;
        _lineHd = lineHd;
    }

    ~HccLineObject ()    { destroyAllCallObjects(); }

    NCC_LINE_STATUS* getLineStatus ()   { return &_lineStatus; }
    NCC_PROT_CAP*    getProtCap ()      { return &_protcap; }

    HccCallObject*  getCurrentCall ()   { return _currentCall;}
    HccCallObject*  getHeldCall ()      { return _heldCall; }
    HccCallObject*  getFirstCall ()     { return _firstCall; }
    CTAHD           getLineHd ()        { return _lineHd; }

    void setCurrentCall ( HccCallObject* call ) { _currentCall = call;}
    void setHeldCall ( HccCallObject* call )    { _heldCall = call; }

    /**
     * This function is called to create an inbound or an outbound call object
     * It can only be called from Idle::gotoNextState(cmd/evt)
     *
     * @parm dir  The direction of the call object, possible value:
     *            NCC_INBOUND_CALL or NCC_OUTBOUND_CALL
     */
    HccCallObject* createCallObject ( DWORD dir );

    /**
     * called when nccReleaseCall() is invoked by appliction
     * This function will destroy a call object
     * @parm call   the pointer of the call object to be detroyed
     */
    DWORD destroyCallObject ( HccCallObject* call );

    /**
     * This function destroys all existing call objects
     * It's called when NCC service is shut down or nccStopProtocol is invoked.
     */
    DWORD destroyAllCallObjects ();

    /**
     * This function is called to convert a call object handle to a call object
     * The linked list of call objects is searched. The address of the matched
     * call object is returned if it's found, otherwise, NULL is returned.
     * @parm svrHdl the server side object handle for a call object
     */
    HccCallObject* objhdToCallObject ( DWORD svrHdl );

    /**
     * Check if an NCC command/event is allowed under the required capabilty
     * @parm cmdOrEvtId  The code number of NCC command/event
     */
    DWORD HccLineObject::capabilityChecking( DWORD cmdid );

    /**
     * Based on NCC command, this function updates NCC line state and the state
     * of a call object. It also sets the pending commands in line status and
     * call status.
     */
    DWORD HccLineObject::updateNccStateMachines( DISP_COMMAND* cmd, HccCallObject* call );

    /**
     * Based on NCC event,this function updates NCC line state and the state
     * of a call object. This function also clears pending command in linestatus
     * and call status.
     */
    DWORD HccLineObject::updateNccStateMachines( DISP_EVENT* evt, HccCallObject* call );

    /**
     * The following 3 functions are invoked to handle 3 NCC information commands:
     * NCCCMD_GET_LINE_STATUS, NCCCMD_GET_CALL_STATUS, NCCCMD_QUERY_CAPABILITY
     * When function returns, the required information is set in the appropricate
     * field in ctacmd.
     */
    DWORD getLineStatus ( DISP_COMMAND * ctacmd );
    DWORD getCallStatus ( DISP_COMMAND * ctacmd );
    DWORD getCapability ( DISP_COMMAND * ctacmd );
};

#endif //HCCLINEOBJECT_INCLUDED

