/*****************************************************************************
* FILE:        wnkHostTcp.h
*
* DESCRIPTION:
*              This file contains the:
*
*                - The declaration for the following classes:
*                  The class to define a call state in wnk0 protocol
*                       Unitialized,        Idle,          Seizure,
*                       ReceiveAddr,        WaitRejAccept, Accept,
*                       Reject,             Answer         WaitPartyLeaveLine,
*                       Seizing,            SeizingAcked1, SeizingAcked2
*                       SendAddr,           Disconnected
*                  The class to define the wnk0 protocol state machine
*                       WnkHostTcp
*****************************************************************************/

#ifndef WNKHOSTTCP_INCLUDED
#define WNKHOSTTCP_INCLUDED

#include "hostTcp.h"


/**----------------------------------------------------------------------------
 * Class Unitialized
 * Desc: Define a call state in wnk0 protocol
 *---------------------------------------------------------------------------*/
class Unitialized : public TcpState
{
private:
    TcpState* _idleNext;    //addr of possible next state: Idle

public:
    Unitialized () : TcpState ( HOSTTCP_STATE_UNINITIALIZED )
    {
        _idleNext = NULL;
    }

    void setNextStates ( TcpState* idle) { _idleNext = idle; }

    DWORD gotoNextState ( DISP_COMMAND* cmd, TcpState* & nextState );
    DWORD gotoNextState ( DISP_EVENT* evt, TcpState* & nextState );
};

/**----------------------------------------------------------------------------
 * Class Idle
 * Desc: Define a call state in wnk0 protocol
 *---------------------------------------------------------------------------*/
class Idle : public TcpState
{
private:
    TcpState* _seizingNext; //addr of possible next state: Seizing
    TcpState* _seizureNext; //addr of possible next state: Seizure

    DWORD _blockState;
    DWORD _busyToneState;

public:
    Idle () : TcpState ( HOSTTCP_STATE_IDLE )
    {
        _seizingNext = NULL;
        _seizureNext = NULL;
        _blockState  = 0;
        _busyToneState = 0;          //indicate if busy tone is being played
    }

    void setNextStates ( TcpState* seizing, TcpState* seizure )
    {
        _seizingNext = seizing;
        _seizureNext = seizure;
    }

    DWORD getBlockState ()                      { return _blockState; }
    void  setBlockState( DWORD blockState )     { _blockState = blockState; }
    void  setBusyToneState( DWORD state )       { _busyToneState = state; }
    DWORD getBusyToneState()                    { return _busyToneState; }

    DWORD gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState );
    DWORD gotoNextState( DISP_EVENT* evt, TcpState* & nextState );
};


/**----------------------------------------------------------------------------
 * Class Seizure
 * Desc: Define a call state in wnk0 protocol
 *---------------------------------------------------------------------------*/
 class Seizure : public TcpState
 {
 private:
    TcpState* _receiveAddrNext;   //addr of possible next state: ReceiveAddr
    TcpState* _waitRejAcceptNext; //addr of possible next state: WaitRejAccept
    TcpState* _idleNext;          //addr of possible next state: Idle

 public:
    Seizure () : TcpState ( HOSTTCP_STATE_SEIZURE )
    {
        _receiveAddrNext = NULL;
        _waitRejAcceptNext = NULL;
        _idleNext = NULL;

    }

    void setNextStates ( TcpState* receiveAddr, TcpState* idle)
    {
        _receiveAddrNext = receiveAddr;
        _idleNext = idle;
    }

    DWORD gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState );
    DWORD gotoNextState( DISP_EVENT* evt, TcpState* & nextState );
 };

/**----------------------------------------------------------------------------
 * Class ReceiveAddr
 * Desc: Define a call state in wnk0 protocol
 *---------------------------------------------------------------------------*/
 class ReceiveAddr : public TcpState
 {
 private:
    TcpState* _receiveAddrNext;  //addr of possible next state: ReceiveAddr
    TcpState* _waitRejAcceptNext;//addr of possible next state: WaitRejAccept
    TcpState* _idleNext;         //addr of possible next state: Idle

    DWORD _numOfDigits;

 public:
    ReceiveAddr () : TcpState ( HOSTTCP_STATE_RECEIVE_ADDR )
    {
        _receiveAddrNext = NULL;
        _waitRejAcceptNext = NULL;
        _idleNext = NULL;
        _numOfDigits = 0;
    }

    void setNextStates ( TcpState* receiveAddr, TcpState* waitRejAccept,
                         TcpState* idle )
    {
        _receiveAddrNext = receiveAddr;
        _waitRejAcceptNext = waitRejAccept;
        _idleNext = idle;
    }

    void resetNumOfDigits()    { _numOfDigits = 0; }
    DWORD gotoNextState ( DISP_COMMAND* cmd, TcpState* & nextState );
    DWORD gotoNextState ( DISP_EVENT* evt, TcpState* & nextState );

 };



/**----------------------------------------------------------------------------
 * Class WaitRejAccept
 * Desc: Define a call state in wnk0 protocol
 *---------------------------------------------------------------------------*/
 class WaitRejAccept : public TcpState
 {
 private:
    TcpState* _answerNext;      //addr of possible next state: Answer
    TcpState* _acceptNext;      //addr of possible next state: Accept
    TcpState* _rejectNext;      //addr of possible next state: Reject
    TcpState* _idleNext;        //addr of possible next state: Idle

 public:
    WaitRejAccept () : TcpState ( HOSTTCP_STATE_WAITAPP_REJACCEPT )
    {
        _answerNext = NULL;
        _acceptNext = NULL;
        _rejectNext = NULL;
        _idleNext = NULL;
    }

    void setNextStates ( TcpState* answer, TcpState* accept,
                         TcpState* reject, TcpState* idle )
    {
        _answerNext = answer;
        _acceptNext = accept;
        _rejectNext = reject;
        _idleNext = idle;
    }

    DWORD gotoNextState ( DISP_COMMAND* cmd, TcpState* & nextState );
    DWORD gotoNextState ( DISP_EVENT* evt, TcpState* & nextState );

 };

/**----------------------------------------------------------------------------
 * Class Accept
 * Desc: Define a call state in wnk0 protocol
 *---------------------------------------------------------------------------*/
 class Accept : public TcpState
 {
 private:
     TcpState* _answerNext;     //addr of possible next state: Answer
     TcpState* _idleNext;       //addr of possible next state: Idle

     DWORD _acceptMode;         //The mode to accept the call

 public:
     Accept () : TcpState ( HOSTTCP_STATE_ACCEPT, true )
     {
          _answerNext = NULL;
          _idleNext = NULL;
          _acceptMode = 0;
     }

     void setNextStates ( TcpState* answer, TcpState* idle )
     {
         _answerNext = answer;
         _idleNext = idle;
         _acceptMode = 0;
     }

     DWORD gotoNextState ( DISP_COMMAND* cmd, TcpState* & nextState );
     DWORD gotoNextState ( DISP_EVENT* evt, TcpState* & nextState );
     void setAcceptMode ( DWORD mode ) { _acceptMode = mode; }
     DWORD getAcceptMode () { return _acceptMode; }
 };

/**----------------------------------------------------------------------------
 * Class Reject
 * Desc: Define a call state in wnk0 protocol
 *---------------------------------------------------------------------------*/
 class Reject : public TcpState
 {
 private:
    TcpState* _idleNext;        //addr of possible next state: Idle

 public:
    Reject () : TcpState ( HOSTTCP_STATE_REJECT, true )
    {
        _idleNext = NULL;
    }

    void setNextStates ( TcpState* idle )
    {
        _idleNext = idle;
    }

    DWORD gotoNextState ( DISP_COMMAND* cmd, TcpState* & nextState );
    DWORD gotoNextState ( DISP_EVENT* evt, TcpState* & nextState );

 };

/**----------------------------------------------------------------------------
 * Class Answer
 * Desc: Define a call state in wnk0 protocol
 *---------------------------------------------------------------------------*/
class Answer : public TcpState
{
private:
    TcpState* _waitPartyLeaveLineNext;  //addr of possible next state:
    TcpState* _disconnectedNext;  //addr of possible next state: Disconnected

public:
    Answer () : TcpState ( HOSTTCP_STATE_ANSWER, true )
    {
        _waitPartyLeaveLineNext = NULL;
        _disconnectedNext = NULL;
    }

    void setNextStates( TcpState* waitPartyLeave, TcpState* discon )
    {
        _waitPartyLeaveLineNext = waitPartyLeave;
        _disconnectedNext = discon;
    }

    DWORD gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState );
    DWORD gotoNextState( DISP_EVENT* evt, TcpState* & nextState );
};

/**----------------------------------------------------------------------------
 * Class WaitPartyLeaveLine
 * Desc: Define a call state in wnk0 protocol
 *---------------------------------------------------------------------------*/
class WaitPartyLeaveLine : public TcpState
{
private:
    TcpState* _idleNext;    //addr of possible next state: Idle

public:
    WaitPartyLeaveLine () : TcpState ( HOSTTCP_STATE_WAITPARTY_LEAVELINE )
    {
        _idleNext = NULL;
    }

    void setNextStates( TcpState* idle ) { _idleNext = idle; }

    DWORD gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState );
    DWORD gotoNextState( DISP_EVENT* evt, TcpState* & nextState );
};

/**----------------------------------------------------------------------------
 * Class Seizing
 * Desc: Define a call state in wnk0 protocol
 *---------------------------------------------------------------------------*/
class Seizing : public TcpState
{
private:
    TcpState* _idleNext;            //addr of possible next state: Idle
    TcpState* _seizingAcked1Next;   //addr of possible next state: SeizingAcked

public:
    Seizing () : TcpState ( HOSTTCP_STATE_SEIZING )
    {
        _idleNext = NULL;
        _seizingAcked1Next = NULL;
    }

    void setNextStates( TcpState* idle, TcpState* seizingAck )
    {
        _idleNext = idle;
        _seizingAcked1Next = seizingAck;
    }

    DWORD gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState );
    DWORD gotoNextState( DISP_EVENT* evt, TcpState* & nextState );
};

/**----------------------------------------------------------------------------
 * Class SeizingAcked1
 * Desc: Define a call state in wnk0 protocol
 *---------------------------------------------------------------------------*/
class SeizingAcked1 : public TcpState
{
private:
    TcpState* _seizingAcked2Next;   //addr of possible next state: SeizingAcked2

public:
    SeizingAcked1 () : TcpState ( HOSTTCP_STATE_SEIZING_ACKED1 )
    {
        _seizingAcked2Next = NULL;
    }

    void setNextStates( TcpState* seizingAcked2)
    {
        _seizingAcked2Next = seizingAcked2;
    }

    DWORD gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState );
    DWORD gotoNextState( DISP_EVENT* evt, TcpState* & nextState );
};

/**----------------------------------------------------------------------------
 * Class SeizingAcked2
 * Desc: Define a call state in wnk0 protocol
 *---------------------------------------------------------------------------*/
class SeizingAcked2 : public TcpState
{
private:
    TcpState* _seizureNext;     //addr of possible next state: Seizure
    TcpState* _sendAddrNext;    //addr of possible next state: SendAddr

public:
    SeizingAcked2 () : TcpState ( HOSTTCP_STATE_SEIZING_ACKED2 )
    {
        _seizureNext = NULL;
        _sendAddrNext = NULL;
    }

    void setNextStates( TcpState* seizure, TcpState* sendAddr )
    {
        _seizureNext = seizure;
        _sendAddrNext = sendAddr;
    }

    DWORD gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState );
    DWORD gotoNextState( DISP_EVENT* evt, TcpState* & nextState );
};


/**----------------------------------------------------------------------------
 * Class SendAddr
 * Desc: Define a call state in wnk0 protocol
 *---------------------------------------------------------------------------*/
class SendAddr : public TcpState
{
private:
    TcpState* _sendAddrNext;        //addr of possible next state: SendAddr
    TcpState* _answerNext;          //addr of possible next state: Answer
    TcpState* _idleNext;            //addr of possible next state: Idle

public:
    SendAddr () : TcpState ( HOSTTCP_STATE_SEND_ADDR )
    {
        _sendAddrNext = NULL;
        _answerNext = NULL;
        _idleNext = NULL;
    }

    void setNextStates( TcpState* sendAddr, TcpState* idle, TcpState* answer )
    {
        _sendAddrNext = sendAddr;
        _idleNext = idle;
        _answerNext = answer;
    }

    DWORD gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState );
    DWORD gotoNextState( DISP_EVENT* evt, TcpState* & nextState );
};

/**----------------------------------------------------------------------------
 * Class Disconnected
 * Desc: Define a call state in wnk0 protocol
 *---------------------------------------------------------------------------*/
class Disconnected : public TcpState
{
private:
    TcpState* _idleNext;        //addr of possible next state: Idle

public:
    Disconnected () : TcpState ( HOSTTCP_STATE_ANSWER ) { _idleNext = NULL; }

    void setNextStates ( TcpState* idle )               { _idleNext = idle; }

    DWORD gotoNextState( DISP_COMMAND* cmd, TcpState* & nextState );
    DWORD gotoNextState( DISP_EVENT* evt, TcpState* & nextState );
};

/**----------------------------------------------------------------------------
 * Class WnkHostTcp
 * Desc: Define the state machine of the simplified wnk0 protocol
 *---------------------------------------------------------------------------*/
class WnkHostTcp : public HostTcp
{
private:
    Unitialized             _unitialized;
    Idle                    _idle;
    Seizure                 _seizure;
    ReceiveAddr             _receiveAddr;
    Accept                  _accept;
    Reject                  _reject;
    WaitRejAccept           _waitRejAccept;
    Seizing                 _seizing;
    SeizingAcked1           _seizingAcked1;
    SeizingAcked2           _seizingAcked2;
    SendAddr                _sendAddr;
    Answer                  _answer;
    WaitPartyLeaveLine      _waitPartyLeaveLine;
    Disconnected            _disconnected;

public:

    /**
     * Construtor of WnkHostTcp,
     * The protocol state machine must be intialized in the constructor
     */
    WnkHostTcp ( CTAHD linehd,
                 HccLineObject* lineObject );

    ~WnkHostTcp (){}

    /* The following three function overwrite the same functions in HostTcp */
    DWORD handleNccCmd ( DISP_COMMAND* cmd );
    DWORD handleAdiEvt ( DISP_EVENT* evt );
    DWORD setCurrentState( TcpState* st );

};


#endif //WNKHOSTTCP_INCLUDED


