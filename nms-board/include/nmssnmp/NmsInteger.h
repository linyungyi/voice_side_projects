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

		
  SNMP++ I N T E G E R. H   
      
  INTEGER CLASS DEFINITION
       
  VERSION:
  2.6

  RCS INFO:
  $Header: integer.h,v 1.15 96/09/11 14:01:27 hmgr Exp $
       
  DESIGN:
  Jeff Meyer
                
  AUTHOR: 
  Jeff Meyer     
              
  LANGUAGE:
  ANSI C++ 
      
  OPERATING SYSTEMS:
  MS-Windows Win32
  BSD UNIX
      
  DESCRIPTION:
  Class definition for Integer classes.
        
=====================================================================*/ 
#ifndef _SNMPINTEGER
#define _SNMPINTEGER

#include "NmsSmival.h"

#define INTOUTBUF 15  // largest ASCII formatted integer

//------------[ Integer Classes ]------------------------------------------
// The integer class allows all the functionality of the various
// integers but is contained in a Value object for consistency
// among the various types.
// class objects may be set or get into Vb objects.
// 

// 32 bit unsigned integer class
class _SNMPCLASS SnmpUInt32: public SnmpSyntax
{
     
  public: 
     // constructor no value
     SnmpUInt32( void);
        
     // constructor with value
     SnmpUInt32 (const DWORD i);

     // copy constructor
     SnmpUInt32( const SnmpUInt32 &c);
        
     // destructor (ensure that SnmpSyntax::~SnmpSyntax() is overridden)
     virtual ~SnmpUInt32();

     // syntax type
     virtual SmiUINT32 get_syntax();

     // overloaded assignment   
     SnmpUInt32& operator=( const DWORD i); 
        
     // overloaded assignment   
     SnmpUInt32& operator=( const SnmpUInt32 &uli); 
        
     // otherwise, behave like a DWORD    
     operator DWORD();

     // get a printable ASCII value
     virtual char *get_printable();

     // create a new instance of this Value
     virtual SnmpSyntax *clone() const;

     // copy an instance of this Value
     SnmpSyntax& operator=( SnmpSyntax &val);

     int valid() const;

  protected:
    int valid_flag;
    char output_buffer[INTOUTBUF];
      
}; 


// 32 bit signed integer class
class _SNMPCLASS SnmpInt32: public SnmpSyntax
{
 
  public:
     // constructor no value
     SnmpInt32( void);
        
     // constructor with value
     SnmpInt32 (const INT32 i);

     // constructor with value
     SnmpInt32 (const SnmpInt32 &c);

     // destructor (ensure that SnmpSyntax::~SnmpSyntax() is overridden)
     virtual ~SnmpInt32();

     // syntax type
     virtual SmiUINT32 get_syntax();

     // overloaded assignment   
     SnmpInt32& operator=( const INT32 i); 
        
     // overloaded assignment   
     SnmpInt32& operator=( const SnmpInt32 &li); 
        
     // otherwise, behave like an INT32    
     operator INT32();

     // get a printable ASCII value
     char *get_printable();

     // create a new instance of this Value
     SnmpSyntax *clone() const;

     // copy an instance of this Value
     SnmpSyntax& operator=( SnmpSyntax &val);

     int valid() const;

 protected:
    int valid_flag;
    char output_buffer[INTOUTBUF];
    
};
#endif       

