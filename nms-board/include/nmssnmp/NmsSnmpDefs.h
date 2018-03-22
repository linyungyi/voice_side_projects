#ifndef NMSSNMPDEFS_H
#define NMSSNMPDEFS_H

#ifndef   NMSNFCTYPES_H
#include <NmsNfcTypes.h>
#endif

#if defined (WIN32)
    #ifdef NMSSNMP_EXPORTS
        #define _SNMPCLASS      __declspec( dllexport )
    #else
        #define _SNMPCLASS      __declspec( dllimport )
    #endif
#else
    #define _SNMPCLASS
#endif

#ifdef UNIX
	typedef int  SOCKET;
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined (UNIX)
    #include <unistd.h>		    // unix
    #include <sys/socket.h>		// bsd socket stuff
    #include <netinet/in.h>		// network types
	#if defined(SOLARIS) || defined(UW7) 
		#include  <arpa/inet.h>
	#endif
    #include <sys/types.h>		// system types
    #include <sys/time.h>		// time stuff
    #include <errno.h>          // ux errs
#else
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

    int     _SNMPCLASS NmsSnmpGetEnumValue( const char* szLabel );
    LPCSTR  _SNMPCLASS NmsSnmpGetEnumString( int value );
    // NACD 2000-1 Linux snmp_initialize is done globally 
    // in the main() of the sub-agent or the multiplexor ==> it must be exported

int   _SNMPCLASS snmp_initialize( void );

#ifdef __cplusplus
}
#endif

DWORD _SNMPCLASS open_multicast_socket(SOCKET &sock, int com_port);

#define NMS_SNMP_FIRST_ERR         25
#define NMS_SNMP_INVALID_ADDRESS   NMS_SNMP_FIRST_ERR + 1
#define NMS_SNMP_TABLE_IS_EMPTY    NMS_SNMP_FIRST_ERR + 2
#define NMS_SNMP_PENDING           NMS_SNMP_FIRST_ERR + 3

#endif // ifndef NMSSNMPDEFS_H //

