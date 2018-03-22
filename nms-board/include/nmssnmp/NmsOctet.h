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

		
  SNMP++ O C T E T . H

  OCTETSTR CLASS DEFINITION

  VERSION:
  2.6

  RCS INFO:
  $Header: octet.h,v 1.18 96/09/11 14:01:30 hmgr Exp $

  DESIGN:
  Peter E Mellquist

  AUTHOR:
  Peter E Mellquist

  DATE:
  July 07, 1995

  LANGUAGE:
  ANSI C++

  OPERATING SYSTEMS:
  MS-WINDOWS Win32
  BSD UNIX

  DESCRIPTION:
  This class is fully contained and does not rely on or any other
  SNMP libraries. This class is portable across any platform
  which supports C++.

=====================================================================*/
#ifndef _OCTET_CLS
#define _OCTET_CLS

//------------------------------------------------------------------------


//----------[ extern C libraries Needed ]---------------------------------
extern "C"
{
#include <memory.h>		  // memcpy's
#include <string.h>		  // strlen, etc..
}
#include "NmsSmival.h"


//-----------------------------------------------------------------------
//------------[ SNMP++ OCTETSTR CLASS DEF  ]-----------------------------
//-----------------------------------------------------------------------
class _SNMPCLASS OctetStr: public  SnmpSyntax
{


public:

  // constructor using no arguments
  OctetStr( void);

  // constructor using a  string
  OctetStr( const char * string);

  // constructor using an unsigned char *
  OctetStr( const unsigned char * string, DWORD size);

  // constructor using another octet object
  OctetStr ( const OctetStr &octet);

  // destructor
  ~OctetStr();

  // syntax type
  SmiUINT32 get_syntax();

  // set the data on an already constructed Octet
  void set_data( const unsigned char * string,
		         DWORD size);

  // assignment to a string operator overloaded
  OctetStr& operator=( const char *string);

  // assignment to another oid object overloaded
  OctetStr& operator=( const OctetStr &octet);

  // equivlence operator overloaded
  _SNMPCLASS friend int operator==( const OctetStr &lhs, const OctetStr &rhs);

  // not equivlence operator overloaded
  _SNMPCLASS friend int operator!=( const OctetStr &lhs, const OctetStr &rhs);

  // less than < overloaded
  _SNMPCLASS friend int operator<( const OctetStr &lhs, const OctetStr &rhs);

  // less than <= overloaded
  _SNMPCLASS friend int operator<=( const OctetStr &lhs,const OctetStr &rhs);

  // greater than > overloaded
  _SNMPCLASS friend int operator>( const OctetStr &lhs, const OctetStr &rhs);

  // greater than >= overloaded
  _SNMPCLASS friend int operator>=( const OctetStr &lhs, const OctetStr &rhs);

  // equivlence operator overloaded
  _SNMPCLASS friend int operator==( const OctetStr &lhs,const char *rhs);

  // not equivlence operator overloaded
  _SNMPCLASS friend int operator!=( const OctetStr &lhs,const char  *rhs);

  // less than < operator overloaded
  _SNMPCLASS friend int operator<( const OctetStr &lhs,const char  *rhs);

  // less than <= operator overloaded
  _SNMPCLASS friend int operator<=( const OctetStr &lhs,char  *rhs);

  // greater than > operator overloaded
  _SNMPCLASS friend int operator>( const OctetStr &lhs,const char  *rhs);

  // greater than >= operator overloaded
  _SNMPCLASS friend int operator>=( const OctetStr &lhs,const char  *rhs);

  // append operator, appends a string
  OctetStr& operator+=( const char  *a);

  // appends an int
  OctetStr& operator+=( const unsigned char c);

  // append one octetStr to another
  OctetStr& operator+=( const OctetStr& octetstr);

  // for non const [], allows reading and writing
  unsigned char& operator[]( int position);

  // compare n elements of an octet
  int nCompare( const DWORD n,
		const OctetStr &o) const;

  // return the len of the oid
  DWORD len() const ;

  // returns validity
  int valid() const;

  // returns pointer to internal data
  unsigned char  * data() const;

  // get a printable ASCII value
  char  *get_printable();

  // get an ASCII formattted hex dump of the contents
  char  *get_printable_hex();

  // create a new instance of this Value
  SnmpSyntax  *clone() const; 

  // copy an instance of this Value
  SnmpSyntax& operator=( SnmpSyntax &val);


protected:
  //----[ instance variables ]
  char *output_buffer;	   // formatted Octet value
  int validity;		       // validity boolean

};
//-----------[ End OctetStr Class ]-------------------------------------

#endif // _OCTET_CLS

