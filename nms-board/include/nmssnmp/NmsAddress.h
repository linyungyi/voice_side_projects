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

 
  SNMP++ A D D R E S S . H

  ADDRESS CLASS DEFINITION

  VERSION:
  2.6

  RCS INFO:
  $Header: address.h,v 1.36 96/09/11 14:01:20 hmgr Exp $

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
  CNmsSnmpAddress class definition. Encapsulates various network
  addresses into easy to use, safe and portable classes.

=====================================================================*/
#ifndef _ADDRESS
#define _ADDRESS

#ifdef WIN32
    #ifndef _WINDOWS_
    #include <windows.h>
    #endif
#endif

//----[ includes ]-----------------------------------------------------
extern "C"
{
#include <string.h>
#include <memory.h>
}

#include "NmsSmival.h"
#include "NmsCollect.h"

// include sockets header files
// for Windows16 and Windows32 include Winsock
// otherwise assume UNIX
#ifdef __unix
// The /**/ stuff below is meant to fool MS C++ into skipping these 
// files when creating its makefile.  8-Dec-95 TM
#include /**/ <unistd.h>
#include /**/ <sys/socket.h>
#include /**/ <netinet/in.h>
#include /**/ <netdb.h>

// 991209: BC: On our UnixWare 2 platforms, we have trouble including 
//             arpa/inet.h due to a conflict with gcc's byteorder.h.
//             We should revisit this in a later release!
#if defined(SOLARIS) || defined(UNIXWARE7) 
   #include /**/ <arpa/inet.h>
#endif

typedef in_addr IN_ADDR;

#ifdef SOLARIS
extern int h_errno;  // defined in WinSock & UW, but not for Solaris!
#endif

#endif // __unix


#ifdef WIN32
#ifndef __unix         // __unix overrides WIN32 if both options are present
#include <winsock.h>
#endif
#endif

//----[ macros ]-------------------------------------------------------
#define BUFSIZE 40     // worst case of address lens
#define OUTBUFF 80     // worst case of output lens

#define IPLEN      4
#define UDPIPLEN   6
#define IPXLEN     10
#define IPXSOCKLEN 12
#define MACLEN     6
#define MAX_FRIENDLY_NAME 80
#define HASH0 19
#define HASH1 13
#define HASH2 7

//----[ enumerated types for address types ]---------------------------
enum addr_type {
  type_ip,
  type_ipx,
  type_udp,
  type_ipxsock,
  type_mac,
  type_invalid
};

//---[ forward declarations ]-----------------------------------------
class GenAddress; 
class UdpAddress;
class IpxSockAddress;

//--------------------------------------------------------------------
//----[ CNmsSnmpAddress class ]-----------------------------------------------
//--------------------------------------------------------------------
class _SNMPCLASS CNmsSnmpAddress: public  SnmpSyntax {

public:

  // allow destruction of derived classes
  virtual ~CNmsSnmpAddress();

  // overloaded equivlence operator, are two addresses equal?
  _SNMPCLASS friend int operator==( const CNmsSnmpAddress &lhs,const CNmsSnmpAddress &rhs);

  // overloaded not equivlence operator, are two addresses not equal?
  _SNMPCLASS friend int operator!=( const CNmsSnmpAddress &lhs,const CNmsSnmpAddress &rhs);

  // overloaded > operator, is a1 > a2
  _SNMPCLASS friend int operator>( const CNmsSnmpAddress &lhs,const CNmsSnmpAddress &rhs);

  // overloaded >= operator, is a1 >= a2
  _SNMPCLASS friend int operator>=( const CNmsSnmpAddress &lhs,const CNmsSnmpAddress &rhs);

  // overloaded < operator, is a1 < a2
  _SNMPCLASS friend int operator<( const CNmsSnmpAddress &lhs,const CNmsSnmpAddress &rhs);

  // overloaded <= operator, is a1 <= a2
  _SNMPCLASS friend int operator<=( const CNmsSnmpAddress &lhs,const CNmsSnmpAddress &rhs);

  // equivlence operator overloaded, are an address and a string equal?
  _SNMPCLASS friend int operator==( const CNmsSnmpAddress &lhs,const char *rhs);

  // overloaded not equivlence operator, are an address and string not equal?
  _SNMPCLASS friend int operator!=( const CNmsSnmpAddress &lhs,const char *rhs);

  // overloaded < , is an address greater than a string?
  _SNMPCLASS friend int operator>( const CNmsSnmpAddress &lhs,const char *rhs);

  // overloaded >=, is an address greater than or equal to a string?
  _SNMPCLASS friend int operator>=( const CNmsSnmpAddress &lhs,const char *rhs);

  // overloaded < , is an address less than a string?
  _SNMPCLASS friend int operator<( const CNmsSnmpAddress &lhs,const char *rhs);

  // overloaded <=, is an address less than or equal to a string?
  _SNMPCLASS friend int operator<=( const CNmsSnmpAddress &lhs,const char *rhs);

  // const char * operator overloaded for streaming output
  virtual operator const char *() const = 0;

  // is the address object valid?
  virtual int valid() const;            

  // syntax type
  virtual SmiUINT32 get_syntax() = 0;

  // for non const [], allows reading and writing
  unsigned char& operator[]( const int position);

  // get a printable ASCII value
  virtual char *get_printable() = 0;

  // create a new instance of this Value
  virtual SnmpSyntax *clone() const = 0;

  // return the type of address
  virtual addr_type get_type() const = 0; 

  // overloaded assignment operator
  virtual SnmpSyntax& operator=( SnmpSyntax &val) = 0;

  // return a hash key
  virtual unsigned int hashFunction() const { return 0;};

protected:
  int valid_flag;
  unsigned char address_buffer[BUFSIZE]; // internal representation

  // parse the address string
  // redefined for each specific address subclass
  virtual int parse_address( const char * inaddr) =0;

  // format the output
  // redefined for each specific address subclass
  virtual void format_output() =0;

  // a reused trimm white space method
  void trim_white_space( char * ptr);

};


//-----------------------------------------------------------------------
//---------[ IP CNmsSnmpAddress Class ]------------------------------------------
//-----------------------------------------------------------------------
class _SNMPCLASS IpAddress : public CNmsSnmpAddress {

public:
  // construct an IP address with no agrs
  IpAddress( void);

  // construct an IP address with a string
  IpAddress( const char *inaddr);

  // construct an IP address with another IP address
  IpAddress( const IpAddress  &ipaddr);

  // construct an IP address with a GenAddress
  IpAddress( const GenAddress &genaddr);

  // destructor (ensure that SnmpSyntax::~SnmpSyntax() is overridden)
  ~IpAddress();

  // copy an instance of this Value
  SnmpSyntax& operator=( SnmpSyntax &val);

  // assignment to another IpAddress object overloaded
  IpAddress& operator=( const IpAddress &ipaddress);

  // create a new instance of this Value
  SnmpSyntax *clone() const;

  // return the friendly name
  // returns a NULL string if there isn't one
  char *friendly_name(int &status);

  virtual char *get_printable() ;

  // const char * operator overloaded for streaming output
  virtual operator const char *() const;

  // logically AND two IPaddresses and
  // return the new one
  void mask( const IpAddress& ipaddr);

  // return the type
  virtual addr_type get_type() const;

  // syntax type
  virtual SmiUINT32 get_syntax();

protected:
  char output_buffer[OUTBUFF];           // output buffer

  // friendly name storage
  char iv_friendly_name[MAX_FRIENDLY_NAME];
  int  iv_friendly_name_status;

  // redefined parse address
  // specific to IP addresses
  virtual int parse_address( const char *inaddr);
   

  // redefined format output
  // specific to IP addresses
  virtual void format_output();

  // parse a dotted string
  int parse_dotted_ipstring( const char *inaddr);

  // using the currently defined address, do a DNS
  // and try to fill up the name
  int addr_to_friendly();

};

//------------------------------------------------------------------------
//---------[ UDP CNmsSnmpAddress Class ]------------------------------------------
//------------------------------------------------------------------------
class _SNMPCLASS UdpAddress : public IpAddress {

public:

  // constructor, no args
  UdpAddress( void);

  // constructor with a dotted string
  UdpAddress( const char *inaddr);

  // construct an Udp address with another Udp address
  UdpAddress( const UdpAddress &udpaddr);

  // construct a Udp address with a GenAddress
  UdpAddress( const GenAddress &genaddr);

  // construct a Udp address with an IpAddress
  // default port # to zero
  UdpAddress( const IpAddress &ipaddr);

  // destructor
  ~UdpAddress();

  // syntax type
  SmiUINT32 get_syntax();

  // copy an instance of this Value
  SnmpSyntax& operator=( SnmpSyntax &val);

  // assignment to another IpAddress object overloaded
  UdpAddress& operator=( const UdpAddress &udpaddr);

  // create a new instance of this Value
  SnmpSyntax *clone() const;

  virtual char *get_printable() ;

  // const char * operator overloaded for streaming output
  virtual operator const char *() const;

  // set the port number
  void set_port( const unsigned short p);

  // get the port number
  unsigned short get_port() const; 

  // return the type
  virtual addr_type get_type() const;

protected:
  char output_buffer[OUTBUFF];           // output buffer

  // redefined parse address
  // specific to IP addresses
  virtual int parse_address( const char *inaddr);

  // redefined format output
  // specific to IP addresses
  virtual void format_output();
};

//-------------------------------------------------------------------------
//---------[ 802.3 MAC CNmsSnmpAddress Class ]-------------------------------------
//-------------------------------------------------------------------------
class _SNMPCLASS MacAddress : public CNmsSnmpAddress {

public:
  // constructor, no arguments
  MacAddress( void);

  // constructor with a string argument
  MacAddress( const char  *inaddr);

  // constructor with another MAC object
  MacAddress( const MacAddress  &macaddr);

  // construct a MacAddress with a GenAddress
  MacAddress( const GenAddress &genaddr);

  // destructor 
  ~MacAddress();

  // syntax type
  SmiUINT32 get_syntax();

  // copy an instance of this Value
  SnmpSyntax& operator=( SnmpSyntax &val);

  // assignment to another IpAddress object overloaded
  MacAddress& operator=( const MacAddress &macaddress);

  // create a new instance of this Value
  SnmpSyntax *clone() const; 

  virtual char *get_printable();

  // const char * operator overloaded for streaming output
  virtual operator const char *() const;

  // return the type
  virtual addr_type get_type() const;

  // return a hash key
  unsigned int hashFunction() const;


protected:
  char output_buffer[OUTBUFF];           // output buffer

  // redefined parse address for macs
  virtual int parse_address( const char *inaddr);

  // redefined format output for MACs
  virtual void format_output();
};

//------------------------------------------------------------------------
//---------[ IPX CNmsSnmpAddress Class ]------------------------------------------
//------------------------------------------------------------------------
class _SNMPCLASS IpxAddress : public CNmsSnmpAddress {

public:
  // constructor no args
  IpxAddress( void);

  // constructor with a string arg
  IpxAddress( const char  *inaddr);

  // constructor with another ipx object
  IpxAddress( const IpxAddress  &ipxaddr);

  // construct with a GenAddress
  IpxAddress( const GenAddress &genaddr);

  // destructor 
  ~IpxAddress();

  // syntax type
  virtual SmiUINT32 get_syntax();

  // copy an instance of this Value
  SnmpSyntax& operator=( SnmpSyntax &val); 

  // assignment to another IpAddress object overloaded
  IpxAddress& operator=( const IpxAddress &ipxaddress);

  // get the host id portion of an ipx address
  int get_hostid( MacAddress& mac);

  // create a new instance of this Value
  SnmpSyntax *clone() const; 

  virtual char *get_printable();

  // const char * operator overloaded for streaming output
  virtual operator const char *() const;

  // return the type
  virtual addr_type get_type() const;

protected:
  // ipx format separator
  char separator;
  char output_buffer[OUTBUFF];           // output buffer

  // redefined parse address for ipx strings
  virtual int parse_address( const char  *inaddr);

  // redefined format output for ipx strings
  // uses same separator as when constructed
  virtual void format_output();
 
};



//------------------------------------------------------------------------
//---------[ IpxSock CNmsSnmpAddress Class ]--------------------------------------
//------------------------------------------------------------------------
class _SNMPCLASS IpxSockAddress : public IpxAddress {

public:
  // constructor, no args
  IpxSockAddress( void);

  // constructor with a dotted string
  IpxSockAddress( const char *inaddr);

  // construct an Udp address with another Udp address
  IpxSockAddress( const IpxSockAddress &ipxaddr);

  //constructor with a GenAddress
  IpxSockAddress( const GenAddress &genaddr);

  //constructor with a IpxAddress
  // default socket # is 0
  IpxSockAddress( const IpxAddress &ipxaddr);

  // destructor
  ~IpxSockAddress();

  // syntax type
  virtual SmiUINT32 get_syntax();

  // copy an instance of this Value
  SnmpSyntax& operator=( SnmpSyntax &val); 

  // assignment to another IpAddress object overloaded
  IpxSockAddress& operator=( const IpxSockAddress &ipxaddr);

  // create a new instance of this Value
  SnmpSyntax *clone() const;

  // set the socket number
  void set_socket( const unsigned short s);

  // get the socket number
  unsigned short get_socket() const;

  virtual char *get_printable();

  // const char * operator overloaded for streaming output
  virtual operator const char *() const;

  // return the type
  virtual addr_type get_type() const;

protected:
  char output_buffer[OUTBUFF];           // output buffer

  // redefined parse address for ipx strings
  virtual int parse_address( const char  *inaddr);

  // redefined format output
  // specific to IP addresses
  virtual void format_output();

};




//-------------------------------------------------------------------------
//--------[ Generic CNmsSnmpAddress ]----------------------------------------------
//-------------------------------------------------------------------------
class _SNMPCLASS GenAddress : public CNmsSnmpAddress {

public:
  // constructor, no arguments
  GenAddress( void);

  // constructor with a string argument
  GenAddress( const char  *addr);

  // constructor with an CNmsSnmpAddress
  GenAddress( const CNmsSnmpAddress &addr);

  // constructor with another GenAddress
  GenAddress( const GenAddress &addr);

  // destructor
  ~GenAddress();

  // get the snmp syntax of the contained address
  SmiUINT32 get_syntax();

  // create a new instance of this Value
  SnmpSyntax *clone() const;

  // assignment of a GenAddress
  GenAddress& operator=( const GenAddress &addr);

  // copy an instance of this Value
  SnmpSyntax& operator=( SnmpSyntax &val);

  virtual char *get_printable();

  // const char * operator overloaded for streaming output
  virtual operator const char *() const;

  // return the type
  virtual addr_type get_type() const;

protected:
  // pointer to a a concrete address
  CNmsSnmpAddress *address;
  char output_buffer[OUTBUFF];           // output buffer

  // redefined parse address for macs
  virtual int parse_address( const char *addr);

  // format output for a generic address
  virtual void format_output();

};

// create OidCollection type
//typedef SnmpCollection <GenAddress> AddressCollection;

#endif  //_ADDRESS

