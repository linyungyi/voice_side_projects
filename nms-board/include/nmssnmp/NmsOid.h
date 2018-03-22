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

		
  SNMP++ O I D. H

  OID CLASS DEFINITION

  VERSION:
  2.6

  RCS INFO:
  $Header: oid.h,v 1.28 96/09/11 14:01:32 hmgr Exp $

  DESIGN:
  Peter E Mellquist

  AUTHOR:
  Peter E Mellquist

  LANGUAGE:
  ANSI C++

  OPERATING SYSTEMS:
  MS-Windows Win32
  BSD UNIX

  DESCRIPTION:
  This class is fully contained and does not rely on or any other
  SNMP libraries. This class is portable across any platform
  which supports C++.

=====================================================================*/
#ifndef _OID_CLS
#define _OID_CLS

//------------------------------------------------------------------------


//----------[ extern C libraries Needed ]---------------------------------

extern "C"
{
#include <memory.h>               // memcpy's
#include <string.h>               // strlen, etc..
}

#include <NmsSmi.h>
#include <NmsSmival.h>                // derived class for all values
#include <NmsCollect.h>

//-----------------------------------------------------------------------
//------------[ SNMP++ OID CLASS DEF  ]----------------------------------
//-----------------------------------------------------------------------
class _SNMPCLASS CNmsSnmpOid: public  SnmpSyntax {


public:

  // constructor using no arguments
  // initialize octet ptr and string
  // ptr to null
  CNmsSnmpOid( void);

  // constructor using a dotted string
  CNmsSnmpOid( const char * dotted_oid_string);

  // constructor using another oid object
  CNmsSnmpOid ( const CNmsSnmpOid &oid);

  // constructor from raw form
  CNmsSnmpOid(const DWORD *raw_oid, int oid_len);

  // destructor
  ~CNmsSnmpOid();

  // syntax type
  SmiUINT32 get_syntax();

  // assignment to a string operator overloaded
  CNmsSnmpOid& operator=( const char *dotted_oid_string);

  // assignment to another oid object overloaded
  CNmsSnmpOid& operator=( const CNmsSnmpOid &oid);

  // equal operator overloaded
  _SNMPCLASS friend int operator==( const CNmsSnmpOid &lhs,const CNmsSnmpOid &rhs);

  // not equal operator overloaded
  _SNMPCLASS friend int operator!=( const CNmsSnmpOid &lhs,const CNmsSnmpOid &rhs);

  // less than < overloaded
  _SNMPCLASS friend int operator<( const CNmsSnmpOid &lhs,const CNmsSnmpOid &rhs);

  // less than <= overloaded
  _SNMPCLASS friend int operator<=( const CNmsSnmpOid &lhs,const CNmsSnmpOid &rhs);

  // greater than > overloaded
  _SNMPCLASS friend int operator>( const CNmsSnmpOid &lhs,const CNmsSnmpOid &rhs);

  // greater than >= overloaded
  _SNMPCLASS friend int operator>=( const CNmsSnmpOid &lhs,const CNmsSnmpOid &rhs);

  // equal operator overloaded
  _SNMPCLASS friend int operator==( const CNmsSnmpOid &lhs,const char *rhs);

  // not equal operator overloaded
  _SNMPCLASS friend int operator!=( const CNmsSnmpOid &lhs,const char *rhs);

  // less than < operator overloaded
  _SNMPCLASS friend int operator<( const CNmsSnmpOid &lhs,const char *rhs);

  // less than <= operator overloaded
  _SNMPCLASS friend int operator<=( const CNmsSnmpOid &lhs,char *rhs);

  // greater than > operator overloaded
  _SNMPCLASS friend int operator>( const CNmsSnmpOid &lhs,const char *rhs);

  // greater than >= operator overloaded
  _SNMPCLASS friend int operator>=( const CNmsSnmpOid &lhs,const char *rhs);

  // append operator, appends a string
  CNmsSnmpOid& operator+=( const char *a);

  // appends an int
  CNmsSnmpOid& operator+=( const DWORD i);

  // appends an CNmsSnmpOid
  CNmsSnmpOid& operator+=( const CNmsSnmpOid &o);


  // allows element access
  DWORD & operator[]( int position);
  
  // return the WinSnmp oid part
  SmiLPOID oidval();

  // reset the data from raw form
  void set_data( const DWORD *raw_oid,
                 const unsigned int oid_len);

 
  // return the len of the oid
  DWORD len() const;

  // trim off the n rightmost values of an oid
  void trim( const DWORD n=1);

  // compare the n leftmost bytes (left-to-right)
  // returns 0, equal
  // returns -1, <
  // returns 1 , >
  int nCompare( const DWORD n, const CNmsSnmpOid &o) const; 

  
  // compare the n rightmost bytes (right-to-left)
  // returns 0, equal
  // returns -1, <
  // returns 1 , >
  int RnCompare( const DWORD n, const CNmsSnmpOid &o) const; 


  // is the CNmsSnmpOid object valid
  // returns validity
  int valid() const;

  // get a printable ASCII value
  char * get_printable();

  // get printable 
  // return dotted string value from the right
  // where the user specifies how many positions to print
  //
  char * get_printable( const DWORD n);

  // return a dotted string starting at start,
  // going n positions to the left
  // NOTE, start is 1 based ( the first id is at position #1)
  char * get_printable( const DWORD start, const DWORD n);

  // create a new instance of this Value
  SnmpSyntax *clone() const;

  // copy an instance of this Value
  SnmpSyntax& operator=( SnmpSyntax &val);

protected:
  //----[ instance variables ]
 
  char *iv_str;             // used for returning oid string

  //----[ protected member functions ]

  // convert a string to an smi oid
  int StrToOid( const char *string,  // input string
                    SmiLPOID dstOid);           // destination oid

  // clone an smi oid
  int OidCopy(  SmiLPOID srcOid,            // source oid
                    SmiLPOID dstOid);           // destination oid

  // convert an smi oid to its string representation
  int OidToStr(SmiLPOID srcOid,             // source oid
                   SmiUINT32 size,              // size of string
                   char *string);        // pointer to string

};

//-----------[ End CNmsSnmpOid Class ]-------------------------------------


  CNmsSnmpOid operator - ( CNmsSnmpOid& oid, const CNmsSnmpOid& subOid );
//  CNmsSnmpOid operator + ( const CNmsSnmpOid& oid, const CNmsSnmpOid& subOid );



// create OidCollection type
//typedef SnmpCollection <CNmsSnmpOid> OidCollection;

#endif //_OID_CLS

