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

      SNMP++ O I D _ D E F . H

      OID_DEF DEFINITIONS

      VERSION
        2.6

      RCS INFO:
         $Header: oid_def.h,v 1.8 97/03/26 09:36:12 hmgr Exp $


      DESCRIPTION:
        Some common CNmsSnmpOid definitions.

      DESIGN:
        Peter E Mellquist

      AUTHOR:
        Peter E Mellquist

      DATE:
        December 18, 1995

      LANGUAGE:
        ANSI C++

      OPERATING SYSTEMS:
        DOS / Windows 3.1
        BSD UNIX

      CHANGE HISTORY:

=====================================================================*/



#ifndef _OID_DEF
#define _OID_DEF


#ifndef _OID_CLS
#include <oid.h>
#endif 


// SMI trap oid def
class snmpTrapsOid: public CNmsSnmpOid {
   public: 
   snmpTrapsOid (void):CNmsSnmpOid("1.3.6.1.6.3.1.1.5"){};
}; 

// SMI Enterprose CNmsSnmpOid
class snmpTrapEnterpriseOid: public CNmsSnmpOid {
   public: 
   snmpTrapEnterpriseOid(void):CNmsSnmpOid("1.3.6.1.6.3.1.1.4.3.0"){};
}; 

// SMI Cold Start CNmsSnmpOid
class coldStartOid: public snmpTrapsOid {
   public: 
   coldStartOid( void){*this+=".1";};
};

// SMI WarmStart CNmsSnmpOid
class warmStartOid: public snmpTrapsOid {
   public: 
   warmStartOid( void){*this+=".2";};
};

// SMI LinkDown CNmsSnmpOid
class linkDownOid: public snmpTrapsOid {
   public: 
   linkDownOid( void){*this+=".3";};
}; 


// SMI LinkUp CNmsSnmpOid
class linkUpOid: public snmpTrapsOid {
   public: 
   linkUpOid( void){*this+=".4";};
};

// SMI Authentication Failure CNmsSnmpOid
class authenticationFailureOid: public snmpTrapsOid {
   public: 
   authenticationFailureOid( void){*this+=".5";};
}; 

// SMI egpneighborloss CNmsSnmpOid
class egpNeighborLossOid: public snmpTrapsOid {
   public: 
   egpNeighborLossOid( void){*this+=".6";};
};


#endif // _OID_DEF

