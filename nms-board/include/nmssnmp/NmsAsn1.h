#ifndef _ASN1
#define _ASN1

#ifdef WIN32
    #ifndef __unix
    #include <winsock.h>
    #endif
#endif

//#ifndef _SNMP_CLS		// NACD 2000-1 Linux
//#include <NmsSnmp_pp.h>
//#endif

#include "NmsOid.h"                // snmp++ oid class
#include "NmsVb.h"                 // snbmp++ vb class
#include "NmsTarget.h"             // snmp++ target class
#include "NmsPdu.h"                // snmp++ pdu class
#include "NmsSnmperrs.h"           // error macros and strings 
#include "NmsAddress.h"            // snmp++ address class defs


#ifndef EIGHTBIT_SUBIDS
    typedef DWORD	oid;
    #define MAX_SUBID   0xFFFFFFFF
#else
    typedef unsigned char	oid;
    #define MAX_SUBID   0xFF
#endif

/*
#ifndef MAX_OID_LEN
#define MAX_OID_LEN	    64	
#endif
*/

// asn.1 values
#ifndef ASN_BOOLEAN
    #define ASN_BOOLEAN	     (0x01)
#endif

#ifndef ASN_INTEGER
    #define ASN_INTEGER	     (0x02)
#endif

#ifndef ASN_BIT_STR
    #define ASN_BIT_STR	     (0x03)
#endif

#ifndef ASN_OCTET_STR
    #define ASN_OCTET_STR	 (0x04)
#endif

#ifndef ASN_NULL
    #define ASN_NULL	     (0x05)
#endif

#ifndef ASN_OBJECT_ID
    #define ASN_OBJECT_ID	 (0x06)
#endif

#ifndef ASN_SEQUENCE
    #define ASN_SEQUENCE	 (0x10)
#endif

#ifndef ASN_SET
    #define ASN_SET		     (0x11)
#endif

#ifndef ASN_UNIVERSAL
    #define ASN_UNIVERSAL	 (0x00)
#endif

#ifndef ASN_APPLICATION
    #define ASN_APPLICATION  (0x40)
#endif

#ifndef ASN_CONTEXT
    #define ASN_CONTEXT	     (0x80)
#endif

#ifndef ASN_PRIVATE
    #define ASN_PRIVATE	     (0xC0)
#endif

#ifndef ASN_PRIMITIVE
    #define ASN_PRIMITIVE	 (0x00)
#endif

#ifndef ASN_CONSTRUCTOR
    #define ASN_CONSTRUCTOR	 (0x20)
#endif

#ifndef ASN_LONG_LEN
    #define ASN_LONG_LEN	 (0x80)
#endif

#ifndef ASN_EXTENSION_ID
    #define ASN_EXTENSION_ID (0x1F)
#endif

#ifndef ASN_BIT8
    #define ASN_BIT8	     (0x80)
#endif

#define IS_CONSTRUCTOR(byte)	((byte) & ASN_CONSTRUCTOR)
#define IS_EXTENSION_ID(byte)	(((byte) & ASN_EXTENSION_ID) == ASN_EXTENSION_ID)

#define ASNERROR( string)
#define MAX_NAME_LEN   64 
#define SNMP_VERSION_1 0
#define SNMP_VERSION_2C 1


// defined types (from the SMI, RFC 1065) 
#define SMI_IPADDRESS   (ASN_APPLICATION | 0)
#define SMI_COUNTER	    (ASN_APPLICATION | 1)
#define SMI_GAUGE	    (ASN_APPLICATION | 2)
#define SMI_TIMETICKS   (ASN_APPLICATION | 3)
#define SMI_OPAQUE	    (ASN_APPLICATION | 4)
#define SMI_NSAP        (ASN_APPLICATION | 5)
#define SMI_COUNTER64   (ASN_APPLICATION | 6)
#define SMI_UINTEGER    (ASN_APPLICATION | 7)

#define GET_REQ_MSG	    (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x0)
#define GETNEXT_REQ_MSG	(ASN_CONTEXT | ASN_CONSTRUCTOR | 0x1)
#define GET_RSP_MSG	    (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x2)
#define SET_REQ_MSG	    (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x3)
#define TRP_REQ_MSG	    (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x4)

#define GETBULK_REQ_MSG	(ASN_CONTEXT | ASN_CONSTRUCTOR | 0x5)
#define INFORM_REQ_MSG	(ASN_CONTEXT | ASN_CONSTRUCTOR | 0x6)
#define TRP2_REQ_MSG	(ASN_CONTEXT | ASN_CONSTRUCTOR | 0x7)
#define REPORT_MSG	    (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x8)

#define SNMP_NOSUCHOBJECT    (ASN_CONTEXT | ASN_PRIMITIVE | 0x0)
#define SNMP_NOSUCHINSTANCE  (ASN_CONTEXT | ASN_PRIMITIVE | 0x1)
#define SNMP_ENDOFMIBVIEW    (ASN_CONTEXT | ASN_PRIMITIVE | 0x2)

#define SNMP_MSG_LENGTH 1500

typedef struct sockaddr_in  ipaddr;

// pdu
struct snmp_pdu {
    int	    command;	     // pdu type
    DWORD  reqid;    // Request id 
    DWORD  errstat;	 // Error status 
    DWORD  errindex; // Error index 

    // Trap information 
    oid	    *enterprise;     // System OID 
    int	    enterprise_length;
    ipaddr  agent_addr;	    // address of object generating trap 
    int	    trap_type;	    // trap type 
    int	    specific_type;  // specific type 
    DWORD  time;	// Uptime 

	// vb list
    struct variable_list *variables;
};

// vb list
struct variable_list {
    struct variable_list *next_variable;    // NULL for last variable 
    oid	    *name;                          // Object identifier of variable 
    int	    name_length;                    // number of subid's in name 
    unsigned char   type;                   // ASN type of variable 
    union {                                 // value of variable 
	INT32	*integer;
	unsigned char 	*string;
	oid	*objid;
	unsigned char   *bitstring;
	struct counter64 *counter64;
    } val;
    int	    val_len;
};



// prototypes for encoding routines
unsigned char * asn_parse_int( unsigned char *data, 
			                   int *datalength, 
			                   unsigned char *type, 
			                   INT32 *intp, 
			                   int intsize);


unsigned char * asn_parse_unsigned_int( unsigned char *data,	
                                        int *datalength,
                                        unsigned char *type,
                                        DWORD *intp,
                                        int	intsize);

unsigned char * asn_build_int( unsigned char *data,
                               int *datalength,
                               unsigned char type,
                               INT32 *intp,
                               int intsize);

unsigned char * asn_build_unsigned_int( unsigned char *data,
                                        int *datalength,
                                        unsigned char type,
                                        DWORD *intp,
                                        int intsize);

unsigned char * asn_parse_string( unsigned char	*data,
                                  int *datalength,
                                  unsigned char *type,
                                  unsigned char *string,
                                  int *strlength);

unsigned char * asn_build_string( unsigned char *data,
                                  int *datalength,
                                  unsigned char type,
                                  unsigned char *string,
                                  int strlength);

unsigned char *asn_parse_header( unsigned char *data,
								 int *datalength,
                                 unsigned char *type);

unsigned char * asn_build_header( unsigned char *data,
                                  int *datalength,
                                  unsigned char type,
                                  int length);

unsigned char * asn_build_sequence( unsigned char *data,
                                    int *datalength,
                                    unsigned char type,
                                    int length);

unsigned char * asn_parse_length( unsigned char *data,
                                  DWORD  *length);

unsigned char *asn_build_length( unsigned char *data,
                                 int *datalength,
                                 int length);

unsigned char *asn_parse_objid( unsigned char *data,
                                int *datalength,
                                unsigned char *type,
                                oid *objid,
                                int *objidlength);

unsigned char *asn_build_objid( unsigned char *data,
                                int *datalength,
                                unsigned char type,
                                oid *objid,
                                int objidlength);

unsigned char *asn_parse_null(unsigned char	*data,
                              int *datalength,
                              unsigned char *type);

unsigned char *asn_build_null( unsigned char *data,
                               int *datalength,
                               unsigned char type);

unsigned char *asn_parse_bitstring( unsigned char *data,
                                    int *datalength,
                                    unsigned char *type,
                                    unsigned char *string,
                                    int *strlength);

unsigned char *asn_build_bitstring( unsigned char *data,
                                    int *datalength,
                                    unsigned char type,	
                                    unsigned char *string,
                                    int strlength);

unsigned char * asn_parse_unsigned_int64(  unsigned char *data,
                                           int *datalength,
                                           unsigned char *type,
                                           struct counter64 *cp,
                                           int countersize);

unsigned char * asn_build_unsigned_int64( unsigned char *data,
                                          int *datalength,
                                          unsigned char	type,
                                          struct counter64 *cp,
                                          int countersize);

struct counter64 {
    DWORD high;
    DWORD low;
};

struct snmp_pdu * snmp_pdu_create( int command);

void snmp_free_pdu( struct snmp_pdu *pdu);

int snmp_build( struct snmp_pdu	*pdu, 
			    unsigned char *packet, 
				int *out_length, 
				INT32 version,
				unsigned char* community,
				int community_len);

void snmp_add_var(struct snmp_pdu *pdu, 
			      oid *name, 
			      int name_length,
			      SmiVALUE *smival);

int snmp_parse( struct snmp_pdu *pdu,
                unsigned char  *data,
				unsigned char *community_name,
				DWORD &community_len,
				snmp_version &version,
                int length);

#endif  // _ASN1

