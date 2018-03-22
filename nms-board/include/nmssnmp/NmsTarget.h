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
  purpose. It is provided "AS-IS" without warranty of any kind,either express 
  or implied. User hereby grants a royalty-free license to any and all 
  derivatives based upon this software code base. 


  SNMP++  T A R G E T . H   
      
  TARGET CLASS DEFINITION
       
  VERSION:
  2.6

  RCS INFO:
  $Header: target.h,v 1.19 96/09/11 14:01:37 hmgr Exp $
       
  DESIGN:
  Peter E Mellquist
                
  AUTHOR:      
  Peter E Mellquist
              
  LANGUAGE:
  ANSI C++ 
      
  OPERATING SYSTEMS:
  DOS/WINDOWS 3.1
  BSD UNIX
      
  DESCRIPTION:
  Target class defines target SNMP agents. 
      
=====================================================================*/ 
#ifndef _TARGET
#define _TARGET

//----[ includes ]----------------------------------------------------- 

#include "NmsAddress.h" 
#include "NmsOid.h"
#include "NmsOctet.h"
#include "NmsCollect.h"  

// external C libraries
extern "C"
{   
#include <string.h>
}

//----[ enumerated types for SNMP versions ]---------------------------
enum snmp_version {
   version1,          // 0
   version2c          // 1
   };
 
//----[ Target class ]------------------------------------------------- 
// Abstract class used to provide a virtual interface into Targets
// 
class _SNMPCLASS SnmpTarget { 

 public:

    // allow destruction of derived classes
    virtual ~SnmpTarget();

    // return validity of target
    int valid() const; 

    // set the retry value       
    void set_retry( const int r);
       
    // get the retry value
    int get_retry();
       
    // set the timeout   
    void set_timeout( const DWORD t);
       
    // get the timeout
    DWORD get_timeout();
    
    // change the default timeout   
    void set_default_timeout( const DWORD t);
                                 
    // change the default retries                             
    void set_default_retries( const int r);
          

    void set_port( const DWORD port );

    unsigned short get_port() const;

    // virtual clone operation for creating a new SnmpTarget from an existing
    // SnmpTarget.  The caller MUST use the delete operation on the return
    // value when done.
    virtual SnmpTarget *clone() const = 0;
 
    // resolve to entity
    // common interface for targets
    // pure virtual
    virtual int resolve_to_C ( OctetStr &read_comm,        // read community
                               OctetStr &write_comm,       // write community
                               GenAddress &address,        // address object
                               DWORD &t,           // timeout
                               int &r,                     // retry
                               unsigned char &v)=0;        // version V1 or v2c 

    // return the address object
    virtual int get_address( GenAddress &address)=0;

    virtual int set_address( CNmsSnmpAddress &address)=0;

	virtual snmp_version get_version() = 0;
                                
   protected:
     int validity; 
     DWORD timeout; // xmit timeout in milli secs
     int retries;           // number of retries 
     unsigned short port;
     
};
 
//----[  Target class ]---------------------------------------------- 
// For explicit definition of Community based targets
//
class _SNMPCLASS Target: public SnmpTarget{ 

  public: 
    // constructor with no args
    Target( void); 
     
    // constructor with all args 
    // can be constructed with any address object
    Target( const CNmsSnmpAddress &address,              // address
             const char *read_community_name,     // read community name
             const char *write_community_name);   // write community name

    // constructor with all args 
    // can be constructed with any address object
    Target( const CNmsSnmpAddress &address,                 // address
             const OctetStr &read_community_name,    // read community
             const OctetStr &write_community_name);  // write community

    // constructor with only address
    // assumes default as public, public              
    // can be constructed with any address object
    Target( const CNmsSnmpAddress &address);
    
    // constructor from existing Target
    Target( const Target &target);

    // destructor
    ~Target();

    // clone from existing Target
    SnmpTarget *clone() const;

    // get the read community name
    char * get_readcommunity();   

    // get the read community as an Octet Str object
    void get_readcommunity( OctetStr& read_community_oct);
    
    // set the read community name
    void set_readcommunity( const char * new_read_community); 
    
    // set the read community using an OctetStr
    void set_readcommunity( const OctetStr& read_community);
      
    // get the write community
    char * get_writecommunity();
    
    // get the write community as an OctetStr
    void get_writecommunity( OctetStr &write_community_oct);
    
    // set the write community
    void set_writecommunity( const char * new_write_community);

    // set the write community using an OctetStr
    void set_writecommunity( const OctetStr& write_community);

    // get the address
    int get_address( GenAddress & address);
    
    // set the address
    int set_address( CNmsSnmpAddress &address);

    // overloaded assignment    
    Target& operator=( const Target& target);

    // compare two C targets
    _SNMPCLASS friend int operator==( const Target &lhs, const Target &rhs);
    
    // resolve to C entity
    // common interface for C targets
    virtual int resolve_to_C( OctetStr&  read_comm,       // get community
                              OctetStr&  write_comm,      // set community
                              GenAddress &address,        // address object
                              DWORD &t,           // timeout
                              int &r,                     // retry
                              unsigned char &v);          // version v1 or v2c
    
    // get the version
    snmp_version get_version();

    // set the version
    void set_version( const snmp_version v);
    
                
  protected:
     OctetStr read_community;        //  get community
     OctetStr  write_community;      //  set community
     GenAddress my_address;          // address object
     snmp_version version;           // v1 or v2C

};

// create OidCollection type
//typedef SnmpCollection<Target> TargetCollection;

#endif //_TARGET

