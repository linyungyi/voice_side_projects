/*===================================================================

   Copyright (c) 1996
   Hewlett-Packard Company

  ATTENTION: USE OF THIS SOFTWARE IS SUBJECT TO THE FOLLOWING TERMS.
  Permission to use, copy, modify, distribute and/or sell this software 
  and/or its documentation is hereby granted without fee. User agrees 
  to display the above copyright notice and this license notice in all 
  copies of the software and any documentation of the software. User 
  agrees to assume all liability for the use of the software; Hewlett-Packard 
  makes no representations about the suitability of this software for any 
  purpose. It is provided "AS-IS without warranty of any kind,either express 
  or implied. User hereby grants a royalty-free license to any and all 
  derivatives based upon this software code base. 

     

   SNMP++ S N M P_PP . H

   SNMP CLASS DEFINITION

   VERSION
    2.6

   RCS INFO:
   $Header: snmp.h,v 1.22 96/09/11 14:01:35 hmgr Exp $

   DESCRIPTION:
   SNMP class defintion. The Snmp class provides an object oriented
   approach to SNMP. The SNMP class is an encapsulation of SNMP
   sessions, gets, sets and get nexts. The class manages all SNMP
   resources and provides complete retry and timeout capability.
   This file is portable. It does not rely on any WinSnmp or any
   other snmp libraries. The matching implementation of this file
   is platform specific.

   DESIGN:
   Peter E Mellquist

  AUTHOR:
  Peter E Mellquist

  LANGUAGE:
  ANSI C++

  OPERATING SYSTEMS:
   Win32
   BSD UNIX

=====================================================================*/
#ifndef _SNMP_CLS
#define _SNMP_CLS

//#pragma pack(1)
//------[ C externals ]------------------------------------------------
#ifdef WIN32
    #ifndef _WINDOWS_
    #include <windows.h>
    #endif
#endif

#include "NmsSocket.h"

extern "C"
{
	#include <memory.h>               // memcpy's
	#include <string.h>               // strlen, etc..
}

//-----[ snmp++ classes ]------------------------------------------------
#include "NmsOid.h"                // snmp++ oid class
#include "NmsVb.h"                 // snbmp++ vb class
#include "NmsTarget.h"             // snmp++ target class
#include "NmsPdu.h"                // snmp++ pdu class
#include "NmsSnmperrs.h"           // error macros and strings 
#include "NmsAddress.h"            // snmp++ address class defs

#ifndef   SNMPCALLBACK_H
#include <NmsCallback.h>
#endif

#ifndef   CNMSTEMPL_H
#include <CNmsTempl.h>
#endif

//-----[ macros ]------------------------------------------------------
#define DEFAULT_TIMEOUT 1000  // one second defualt timeout
#define DEFAULT_RETRIES 1     // no retry default
#define SNMP_PORT	161	      // port # for SNMP
#define MA_PORT		49212	// for the MUX
#define COM_PORT	49213	// for the Communication between Mux and agents
#define SNMP_TRAP_PORT 162    // port # for SNMP traps
#define TICK_TIMEOUT 10         // By default a tick each 10 sec is sent to NMS subagents to check if they are running

#if defined (UW7)
#define MAX_SNMP_PACKET 1024  // maximum snmp packet len
#else
#define MAX_SNMP_PACKET 1024 // maximum snmp packet len
#endif

#define MANY_AT_A_TIME 20     // many at a time size
#define MAX_GET_MANY 51	      // maximum get many size
#define MAX_RID 32767	      // max rid to use
#define MIN_RID 1000	      // min rid to use

//-----[ internally used defines ]----------------------------------------
#define MAXNAME 80                       // maximum name length
#define MAX_ADDR_LEN 10                  // maximume address len, ipx is 4+6
#define SNMP_SHUTDOWN_MSG 0x0400+177     // shut down msg for stoping a blocked message

//-----[ async defines for engine ]---------------------------------------
#define sNMP_PDU_GET_ASYNC       21
#define sNMP_PDU_GETNEXT_ASYNC   22
#define sNMP_PDU_SET_ASYNC       23
#define sNMP_PDU_GETBULK_ASYNC   24
#define sNMP_PDU_INFORM_ASYNC    25

//-----[ trap / notify macros ]-------------------------------------------
#define IP_NOTIFY  162     // IP notification
#define IPX_NOTIFY 0x2121  // IPX notification


/*
//-----------[ async methods callback ]-----------------------------------
// async methods require the caller to provide a callback
// address of a function with the following typedef
typedef void (*snmp_callback)(// int,             // reason
                               CNmsSnmpPdu &,           // pdu passsed in
                               SnmpTarget &,    // source target
                               void * );        // optional callback data

*/


//------------[ SNMP Trap Class Def ]----------------------------------------
//

class _SNMPCLASS SnmpTrap
{
public:
    SnmpTrap( int& status );
    ~SnmpTrap();

  int  enable_traps( BOOL bEnable );
  int  enable_traps( BOOL bEnable, DWORD port );
  void listen_traps();

  //-----------------------[ register to get informs]-------------------------
  virtual int notify_register( const SnmpCallback& );

  //-----------------------[ get notify register info ]---------------------
  virtual int get_notify_filter( const SnmpCallback& );

  //-----------------------[ un-register to get traps]----------------------
  virtual int notify_unregister();

protected:
   NmsSocket iv_trap_session;
   int    construct_status;
	SnmpCallback  * m_pSnmpCallback;

	// inform receive member variables
	// SnmpCallback* m_pSnmpCallback;
	// int  receive_snmp_notification( CNmsSnmpPdu &pdu, Target &target );

};

//
//------------[ End SNMP Trap Class Def ]------------------------------------

typedef void (*agent_send_trap_callback)( const CNmsSnmpPdu&  pdu);

class CNmsThread;
class _SNMPCLASS SnmpAgentTraps
{
public:
    SnmpAgentTraps();
  virtual ~SnmpAgentTraps();

  void           PutTrap( CNmsSnmpPdu& pdu );
  CNmsSnmpPdu    GetTrap();

  CNmsEvent&     GetTrapEvent();
  BOOL           IsEmpty();

  void           RegisterSendTrapCallback( agent_send_trap_callback );

  virtual void   SendTrap( CNmsSnmpPdu& pPdu );

private:

  CNmsQueue< CNmsSnmpPdu >&  m_TrapQueue;
  CNmsThread*                m_pTrapThread;
  CNmsEvent                  m_TrapSignal;



  agent_send_trap_callback   m_pCallbackFunc;

};

#endif //_SNMP_CLS

