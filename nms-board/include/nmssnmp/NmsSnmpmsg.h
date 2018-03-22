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


  SNMP++ S N M P M E S S A G E . H   
      
  SNMPMESSAGE CLASS DEFINITION
       
  VERSION:
  2.6
  
  RCS INFO:
  $Header: Exp $
       
  DESIGN:
  Peter E Mellquist
                
  AUTHOR:      
  Peter E Mellquist
              
  LANGUAGE:
  ANSI C++ 
      
  OPERATING SYSTEMS:
  Win 32
  BSD UNIX
      
  DESCRIPTION:
  ASN.1	encoding / decoding class
      		 
=====================================================================*/ 

#ifndef _SNMPMSG
#define _SNMPMSG

#include "NmsAsn1.h"                	// asn.1 header file

// SnmpMessage Class
class _SNMPCLASS SnmpMessage {

public:

	// construct a SnmpMessage object
	SnmpMessage( void)
	   { bufflen=SNMP_MSG_LENGTH; valid_flag=FALSE; };

	// load up using a CNmsSnmpPdu, community and SNMP version
	// performs ASN.1 serialization
	// result status returned
	int load( CNmsSnmpPdu pdu,                      // CNmsSnmpPdu to serialize
		      OctetStr community,           // community name to use
			  snmp_version version);        // SNMP version, v1 or v2


	// load up message using ASN.1 data stream
	// status is returned
	int load( unsigned char *data,         // data to be loaded  
		      DWORD len);		   // len of data to be loaded

	// unload ( unserialize ) into SNMP++ CNmsSnmpPdu, community and version
	// status is returned
	int unload( CNmsSnmpPdu &pdu,                  // CNmsSnmpPdu returned
		        OctetStr &community,       // community name
				snmp_version &version);    // version

	// return the validity of the message
	// returns Boolean
	int valid() 
	   { return valid_flag;};

	// return raw data
	// check validity
	unsigned char *data() 
	   { return databuff; };

	// returns len
	// check validity
	DWORD len() 
	   { return bufflen; };
	

protected:
	unsigned char databuff[SNMP_MSG_LENGTH];
	DWORD bufflen;
	int valid_flag;

};
#endif  // _SNMPMSG

