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

 
  SNMP++ G A U G E. H

  GAUGE32 CLASS DEFINITION

  VERSION:
  2.6

  RCS INFO:
  $Header: gauge.h,v 1.18 96/09/11 14:01:25 hmgr Exp $

  DESIGN:
  Peter E Mellquist

  AUTHOR:
  Peter E Mellquist

  DATE:
  June 15, 1994

  LANGUAGE:
  ANSI C++

  OPERATING SYSTEMS:
  MS-Windows Win32
  BSD UNIX

  DESCRIPTION:
  Class definition for SMI Gauge32 class.


=====================================================================*/
#ifndef _GAUGE
#define _GAUGE

#include "NmsInteger.h"

//------------[ Gauge32 Class ]------------------------------------------
// The gauge class allows all the functionality of unsigned
// integers but is recognized as a distinct SMI type. Gauge32
// objects may be set or get into Vb objects.
//
class _SNMPCLASS Gauge32: public SnmpUInt32
{

  public:
     // constructor, no value
     Gauge32( void);

     // constructor with a value
     Gauge32( const DWORD i);

     // copy constructor
     Gauge32 ( const Gauge32 &g);

     // destructor for a Gauge32 (ensure that Value::~Value() is overridden)
     ~Gauge32();

     // syntax type
     SmiUINT32 get_syntax();

     // create a new instance of this Value
     SnmpSyntax *clone() const;

     // overloaded assignment
     Gauge32& operator=( const Gauge32 &uli);

     // overloaded assignment
     Gauge32& operator=( const DWORD i);

     // otherwise, behave like an unsigned int
     operator DWORD();

     // copy an instance of this Value
     SnmpSyntax& operator=( SnmpSyntax &val);

};
#endif //_GAUGE

