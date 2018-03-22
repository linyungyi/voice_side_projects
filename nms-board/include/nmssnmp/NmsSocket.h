#ifndef NMSSOCKET_H
#define NMSSOCKET_H

#include <stdio.h>		    	// standard io
#include <stdlib.h>		    	// need for malloc

#ifdef UNIX
    	#include <unistd.h>	    	// unix
    	#include <sys/socket.h>		// bsd socket stuff
    	#include <netinet/in.h>		// network types
    	#include <errno.h>          	// ux errs
    	
	#if defined(SOLARIS) || defined(UW7) || defined(LINUX)
   		#include  <arpa/inet.h>
	#endif

	typedef int SOCKET;
	
#endif

#ifdef LINUX
	typedef socklen_t 	sockLength;
#elif UW7
	typedef size_t 		sockLength;
#else
	typedef int 		sockLength;
#endif

#ifdef WIN32

    #ifndef _WINDOWS_
    #include <windows.h>
    #endif
    
    #ifndef _WINSOCKAPI_
    #include <winsock.h>
    #endif

    #ifndef _INC_ERRNO
    #include <errno.h>
    #endif

#endif

#ifdef WIN32
    #define ENOBUFS         WSAENOBUFS
    #define EHOSTDOWN       WSAEHOSTDOWN
    #define EADDRINUSE      WSAEADDRINUSE
    #define EAFNOSUPPORT    WSAEAFNOSUPPORT
    #define ENETUNREACH     WSAENETUNREACH
#endif


#ifndef _SNMPMSG
#include "NmsSnmpmsg.h"
#endif


#define SOCKET_SUCCESS		0	// SOCKET remplace SNMP_CLASS
#define SOCKET_RESOURCE_UNAVAIL	1	// penser a verifier les correspondances.
#define SOCKET_TL_FAILED	2
#define SOCKET_TL_IN_USE	3
#define SOCKET_TL_UNSUPPORTED	4
#define SOCKET_INTERNAL_ERROR	5

#if defined (UW7)
#define MAX_PACKET_SIZE 1024  // maximum snmp packet len
#else
#define MAX_PACKET_SIZE 1024  // maximum snmp packet len
#endif

class _SNMPCLASS NmsSocket
{
public:
	SOCKET m_socket;

	NmsSocket();
	static DWORD initSocket(int trace_flag);

	DWORD createSocket ( DWORD & dwPortNumber );
	DWORD createBroadcastSocket( DWORD & dwPortNumber, struct sockaddr_in & addr );

	void destroySocket ( );

	int receiveFrom( char * buffer, int bufferLength, struct sockaddr_in * Addr );
	int receiveFrom( SnmpMessage &msg, struct sockaddr_in * addr2 = NULL );
	int receiveFrom( CNmsSnmpPdu &pdu, Target* targetOpt = NULL );

	int sendTo( char * send_buf, int send_len, struct sockaddr & address);
	int sendTo( char *send_buf, int send_len, CNmsSnmpAddress & address );
	int sendTo( SnmpTarget &target, CNmsSnmpPdu &pdu );

};
#endif      //NMSSOCKET_H

