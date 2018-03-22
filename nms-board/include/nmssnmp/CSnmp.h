#ifndef CSNMP_H
#define CSNMP_H

#ifndef MMSSOCKET_H
#include "NmsSocket.h"
#endif

#include "NmsSnmp_pp.h"

#include "NmsSnmperrs.h"

class _SNMPCLASS Snmp {


 public:
  //------------------[ constructor,blocked usage ]---------------------
  Snmp( int &status, int port = 0 );                    // construction status

  //-------------------[ destructor ]------------------------------------
  ~Snmp();

  //-------------------[ returns error string ]--------------------------
  char * error_msg( const int c);
  
  //----------------------[ cancel a pending request ]--------------------
  virtual int cancel( const DWORD rid);

  //------------------------[ get ]---------------------------------------
  virtual int get( CNmsSnmpPdu &pdu,             // pdu to get
                   SnmpTarget &target);  // get target

  //------------------------[ get async ]----------------------------------
  virtual int get( CNmsSnmpPdu &pdu,                       // pdu to get async
                   SnmpTarget &target,             // destination target
                   const snmp_callback callback,   // async callback to use
                   const void * callback_data=0);  // callback data

  //------------------------[ get next ]-----------------------------------
  virtual int get_next( CNmsSnmpPdu &pdu,             // pdu to get_next
                        SnmpTarget &target);  // get target

  //------------------------[ get next async ]-----------------------------
  virtual int get_next( CNmsSnmpPdu &pdu,                      // pdu to get_next async
                        SnmpTarget &target,            // destination target
                        const snmp_callback callback,  // async callback to use
                        const void * callback_data=0); // callback data

  //-------------------------[ set ]---------------------------------------
  virtual int set( CNmsSnmpPdu &pdu,            // pdu to set
                   SnmpTarget &target); // target address

  //------------------------[ set async ]----------------------------------
  virtual int set( CNmsSnmpPdu &pdu,                       // pdu to set async
                   SnmpTarget &target,             // destination target
                   const snmp_callback callback,   // async callback
                   const void * callback_data=0);  // callback data

  //-----------------------[ get bulk ]------------------------------------
  virtual int get_bulk( CNmsSnmpPdu &pdu,                // pdu to get_bulk
                        SnmpTarget &target,      // destination target
                        const int non_repeaters, // number of non repeaters
                        const int max_reps);     // maximum number of repetitions

  //-----------------------[ get bulk async ]------------------------------
  virtual int get_bulk( CNmsSnmpPdu &pdu,                      // pdu to get_bulk async
                        SnmpTarget &target,            // destination target
                        const int non_repeaters,       // number of non repeaters
                        const int max_reps,            // max repeaters
                        const snmp_callback callback,        // async callback
                        const void *callback_data=0);        // callback data


  //-----------------------[ send a trap ]----------------------------------
  virtual int trap( CNmsSnmpPdu &pdu,                     // pdu to send
                    SnmpTarget &target);          // destination target

  
  //----------------------[ blocking inform, V2 only]------------------------
  virtual int inform( CNmsSnmpPdu &pdu,                // pdu to send
                      SnmpTarget &target);     // destination target

  //----------------------[ asynch inform, V2 only]------------------------
  virtual int inform( CNmsSnmpPdu &pdu,                      // pdu to send
                      SnmpTarget &target,            // destination target
                      const snmp_callback callback,  // callback function
                      const void * callback_data=0); // callback data


  void set_trap_port( int port );


 protected:
   //---[ instance variables ]
   HANDLE pdu_handler;                 // pdu handler win proc
   int construct_status;               // status of construction
   int port;
   int trapPort;

   //-----------[ Snmp Engine ]----------------------------------------
   // gets, sets and get nexts go through here....
   // This mf does all snmp sending (using sendMsg) and reception
   // except for traps which are sent using trap().
   int snmp_engine( CNmsSnmpPdu &pdu,                  // pdu to use
                    INT32 non_reps,         // get bulk only
                    INT32 max_reps,         // get bulk only
                    SnmpTarget &target,        // destination target
                    const snmp_callback cb,    // async callback function
                    const void *cbd);          // callback data

 public:

    NmsSocket iv_snmp_session;             // session handle
	 
	//-----------[ Snmp sendMsg ]---------------------------------
   // send a message using whatever underlying stack is available.
   // 
   int sendMsg( SnmpTarget &target,      // target of send
                CNmsSnmpPdu &pdu,                // the pdu to send
				INT32 non_reps,       // # of non repititions
                INT32 max_reps,       // # of max repititions
    	    	INT32 request_id);    // id to use in send

   int receiveMsg( Target& target , CNmsSnmpPdu& pdu );

};








#endif // CSNMP_H
