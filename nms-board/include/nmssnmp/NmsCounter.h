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


  SNMP++ C O U N T E R. H

  COUNTER32 CLASS DEFINITION

  VERSION:
  2.6
  
  RCS INFO:
  $Header: counter.h,v 1.17 96/09/11 14:01:22 hmgr Exp $

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
  Class definition for SMI Counter32 class.

=====================================================================*/
#ifndef _COUNTER
#define _COUNTER

#include "NmsSmival.h"
#include "NmsInteger.h"

//------------[ Counter32 Class ]------------------------------------------
// The counter class allows all the functionality of unsigned
// integers but is recognized as a distinct SMI type. Counter32
// class objects may be set or get into Vb objects.
//
class _SNMPCLASS Counter32: public SnmpUInt32
{
  public:
     // constructor no value
     Counter32( void);

     // constructor with a value
     Counter32( const DWORD i);

     // copy constructor
     Counter32( const Counter32 &c);

     // syntax type
     SmiUINT32 get_syntax();

     // create a new instance of this Value
     SnmpSyntax *clone() const;

     // copy an instance of this Value
     SnmpSyntax& operator=( SnmpSyntax &val);

     // overloaded assignment
     Counter32& operator=( const Counter32 &uli);

     // overloaded assignment
     Counter32& operator=( const DWORD i);

     // otherwise, behave like a DWORD
     operator DWORD();
};

#endif // _COUNTER

