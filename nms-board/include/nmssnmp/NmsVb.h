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


  SNMP++ V B . H   
     
  VARIABLE BINDING CLASS DEFINITION
       
  VERSION 
  2.6  
        
  RCS INFO:
  $Header: vb.h,v 1.12 96/09/11 14:01:41 hmgr Exp $         
      
  DESCRIPTION:
  This module contains the class definition for the variable binding 
  class. The VB class is an encapsulation of a SNMP VB. A VB object is 
  composed of an SNMP++ CNmsSnmpOid and an SMI value. The Vb class utilizes CNmsSnmpOid
  objects and thus requires the CNmsSnmpOid class. The Vb class may be used
  stand alone and does not require use of any other snmp library.
        
        
  DESIGN:
  Peter E. Mellquist  
      
  AUTHOR:
  Peter E Mellquist
        
  LANGAUGE:
  ANSI C++
        
  OPERATING SYSTEM:
  MS-Windows Win32 
  BSD UNIX
        
=====================================================================*/ 
 
#ifndef _VB_CLS
#define _VB_CLS
        

#ifdef WIN32
    #ifndef _WINDOWS_
    #include <windows.h>
    #endif
#endif
       
//----[ external C calls ]----------------------------------------------
extern "C"
{
#include <memory.h>               // memcpy's
#include <string.h>               // strlen, etc..
} 

#include "NmsSmival.h"
#include "NmsOid.h"                 // oid class def 
#include "NmsTimetick.h"            // time ticks
#include "NmsCounter.h"             // counter  
#include "NmsGauge.h"               // gauge class
#include "NmsCtr64.h"               // 64 bit counters
#include "NmsOctet.h"               // octet class 
#include "NmsAddress.h"             // address class def
#include "NmsInteger.h"             // integer class

//------------[ VB Class Def ]-------------------------------------
// The Vb class is the encapsulation of the SNMP variable binding.
// Variable binding lists in SNMP++ are represented as arrays of
// Vb objects. Vb objects are passed to and from SNMP objects to
// provide getting or setting MIB values.
// The vb class keeps its own memory for objects and does not 
// utilize pointers to external data structures.
//
class _SNMPCLASS Vb {


//-----[ public members ]     
public:

  //-----[ constructors / destructors ]-------------------------------
     
  // constructor with no arguments  
  // makes an vb, unitialized
  Vb( void);
    
  // constructor to initialize the oid  
  // makes a vb with oid portion initialized
  Vb( const CNmsSnmpOid &oid);
             
  // copy constructor
  Vb( const Vb &vb);
             
  // destructor
  // if the vb has a oid or an octect string then
  // the associated memory needs to be freed
  ~Vb();     
  
  // assignment to another Vb object overloaded
  Vb& operator=( const Vb &vb);

  //-----[ set oid / get oid ]------------------------------------------
    
   // set value oid only with another oid
   void set_oid( const CNmsSnmpOid oid);
   
   
   // get oid portion
   void get_oid( CNmsSnmpOid &oid) const;
   
   //-----[ set value ]--------------------------------------------------
   
   // sets from a general value
   void set_value( const SnmpSyntax &val);
                                                        
   // set the value with an int 
   // C++ int maps to SMI int      
   void set_value( const INT16 i);
                                                        
   // set the value with an INT32 signed int
   // C++ INT32 maps to SMI int 32                                                       
   void set_value( const INT32 i);
    
   // set the value with a DWORD
   // C++ DWORD maps to SMI UINT32
   void set_value( const DWORD i);

   // set value on a string
   // makes the string an octet
   // this must be a null terminates string 
   // maps to SMI octet
   void set_value( const char *ptr); 

   // set a Vb null, if its not already
   void set_null();
                                                        
   //----[ get value ]------------------------------------------------
   
   // gets a general value
   int get_value( SnmpSyntax &val);
                                                        
   // get value int
   // returns 0 on success and value
   int get_value( INT16 &i);

   // get the signed INT32
   // returns 0 on success and a value  
   int get_value( INT32 &i);
 
   // get the DWORD
   // returns 0 on success and a value
   int get_value( DWORD &i);
          
                       
   // get a unsigned char string value
   // destructive, copies into given ptr 
   // also returnd is the len length 
   // 
   // Note! the caller must provide a target string big
   // enough to handle the vb string
   int get_value( unsigned char *ptr, DWORD &len); 

   // get an unsigned char array
   // caller specifies max len of target space   
   int get_value( unsigned char *ptr,  // pointer to target space
                  DWORD &len,          // returned len
                  DWORD maxlen);       // max len of target space
    
   // get a char * from an octet string
   // the user must provide space or
   // memory will be stepped on
   int get_value( char *ptr); 


   //-----[ misc]--------------------------------------------------------

   // return the current syntax 
   // Or.. if a V2 VB exception is present then return the exception value
   SmiUINT32 get_syntax();

   // set the exception status
   _SNMPCLASS friend void set_exception_status( Vb *vb, const SmiUINT32 status);
   
   // returns a formatted version of the value
   char *get_printable_value();
  
   // returns a formatted version of the value
   char *get_printable_oid();

   // return validity of Vb object
   int valid() const;

//-----[ protected members ]
protected:
  CNmsSnmpOid iv_vb_oid;               // a vb is made up of a oid 
  SnmpSyntax *iv_vb_value;     // and a value...
  SmiUINT32 exception_status;  // are there any vb exceptions??
                 
  // free up any mem used               
  void free_vb();
    
};

#endif  

