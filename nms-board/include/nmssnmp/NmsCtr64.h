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

		
  SNMP++ C O U N T E R 6 4 . H   
      
  COUNTER64 CLASSES DEFINITION
      
  VERSION:
  2.6

  RCS INFO:
  $Header: ctr64.h,v 1.17 96/09/11 14:01:23 hmgr Exp $
       
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
  SNMP Counter64 class definition.
      
=====================================================================*/  
#ifndef _CTR64
#define _CTR64

#include "NmsSmival.h"

#define CTR64OUTBUF 30  // maximum ascii string for a 64-bit ctr

                
//---------[ 64 bit Counter Class ]--------------------------------
// Counter64 Class encapsulates two unsigned integers into a
// a single entity. This type has is available in SNMPv2 but
// may be used anywhere where needed.
//
class _SNMPCLASS Counter64: public  SnmpSyntax
{ 
   protected:
      char output_buffer[CTR64OUTBUF];
      
      
   public:
   
     // convert a Counter 64 to a long double
     long double c64_to_ld( Counter64 &c64);
      
     // convert a long double to a Counter64
     Counter64 ld_to_c64( long double ld);
     
     // constructor, no values
     Counter64( void);
        
     // constructor with values   
     Counter64( DWORD hiparm, DWORD loparm);
        
     // constructor with only one arg, defaults to lower
     Counter64( DWORD loparm);
     
     // copy constructor
     Counter64( const Counter64 &ctr64);
     
     // destructor (ensure that SnmpSyntax::~SnmpSyntax() is overridden)
     ~Counter64();

     // syntax type
     SmiUINT32 get_syntax();

     // return the high part   
     DWORD high() const;  
        
     // return the low part   
     DWORD low() const;
        
     // set the high part   
     void set_high( const DWORD h);
        
     // set the low part   
     void set_low( const DWORD l);
        
     //------[ overloaded assignment ]-------------------
     
     // assign a Counter64 to a Counter64 
     Counter64& operator=( const Counter64 &ctr64);
        
     // assign a ul to a ctr64, clears the high part
     // and assugns the low part
     Counter64& operator=( const DWORD i);
        
     //------[ overloaded addition ]---------------------
     
     // add two Counter64s
     Counter64 operator+( const Counter64 &c); 
     
     // add a DWORD and a Counter64
     _SNMPCLASS friend Counter64 operator+( DWORD ul, const Counter64 &c64);
        
     
     //------[ overloaded subtraction ]------------------
     
     // subtract two Counter64s
     Counter64 operator-( const Counter64 &c);
     
     // subtract a DWORD and a Counter64
     _SNMPCLASS friend Counter64 operator-( DWORD ul, const Counter64 &c64);
        
     //-------[ overloaded multiply ]---------------------
     
     // multiply two Counter64s
     Counter64 operator*( const Counter64 &c); 
     
     // multiply a DWORD and a Counter64
     _SNMPCLASS friend Counter64 operator*( DWORD ul, const Counter64 &c64);
     
     //-------[ overloaded division ]---------------------
     
     // divide two Counter64s
     Counter64 operator/( const Counter64 &c); 
     
     // divide a DWORD and a Counter64
     _SNMPCLASS friend Counter64 operator/( DWORD ul, const Counter64 &c64);
     
     //-------[ overloaded equivlence test ]--------------
     _SNMPCLASS friend int operator==( Counter64 &lhs, Counter64 &rhs);
     
     //-------[ overloaded not equal test ]----------------
     _SNMPCLASS friend int operator!=( Counter64 &lhs, Counter64 &rhs);
     
     //--------[ overloaded less than ]--------------------
     _SNMPCLASS friend int operator<( Counter64 &lhs, Counter64 &rhs);
     
     //---------[ overloaded less than or equal ]----------
     _SNMPCLASS friend int operator<=( Counter64 &lhs, Counter64 &rhs);
     
     //---------[ overloaded greater than ]----------------
     _SNMPCLASS friend int operator>( Counter64 &lhs, Counter64 &rhs);
     
     //----------[ overloaded greater than or equal ]------
     _SNMPCLASS friend int operator>=( Counter64 &lhs, Counter64 &rhs);
     
     // get a printable ASCII value
     char *get_printable();

     // create a new instance of this Value
     SnmpSyntax *clone() const; 

     // copy an instance of this Value
     SnmpSyntax& operator=( SnmpSyntax &val);

     // general validity test, always true 
     int valid() const;

}; 

#endif 

