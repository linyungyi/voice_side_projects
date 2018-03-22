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


  SNMP++ T I M E T I C K. H   
      
  TIMETICK CLASS DEFINITION
   
  VERSION:
  2.6

  RCS INFO:
  $Header: timetick.h,v 1.15 96/09/11 14:01:39 hmgr Exp $
       
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
  Class definition for SMI Timeticks class.
        
=====================================================================*/

#ifndef _TIMETICKS
#define _TIMETICKS

#include "NmsInteger.h"

#define TICKOUTBUF 30 // max formatted time string

//------------[ TimeTicks Class ]-----------------------------------
// The timeticks class allows all the functionality of unsigned
// integers but is recognized as a distinct SMI type. TimeTicks
// objects may be get or set into Vb objects.
//
class _SNMPCLASS TimeTicks: public SnmpUInt32
{
    
  public:
     // constructor no args 
     TimeTicks( void);
        
     // constructor with a value   
     TimeTicks( const DWORD i);
        
     // copy constructor
     TimeTicks( const TimeTicks &t);

     // destructor 
     ~TimeTicks();

     // syntax type
     SmiUINT32 get_syntax();

     // get a printable ASCII value
     char *get_printable();

     // create a new instance of this Value
     SnmpSyntax *clone() const;

     // copy an instance of this Value
     SnmpSyntax& operator=(SnmpSyntax &val);
    
     // overloaded assignment
     TimeTicks& operator=( const TimeTicks &uli); 
        
     // overloaded assignment
     TimeTicks& operator=( const DWORD i); 
        
     // otherwise, behave like a DWORD    
     operator DWORD();

  protected:
    char output_buffer[TICKOUTBUF];  // for storing printed form
            
}; 
#endif

