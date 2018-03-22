/*
  snmpGet.cpp 

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

  Peter E. Mellquist
*/
#include "CSnmp.h"
#include <stdlib.h>

#define DEFAULT_OID	"1.3.6.1.2.1.1.1.0"

int main( int argc, char **argv)  {

   CNmsSnmpPdu pdu;                                // construct a CNmsSnmpPdu object

   //---------[ check the arg count ]----------------------------------------
   if ( argc < 2) {
	  printf("Usage:\n");
	  printf("snmpGet Address | DNSName [CNmsSnmpOid] [options]\n");
	  printf("CNmsSnmpOid: (default value is %s)\n", DEFAULT_OID) ;
	  printf("options: -v1 , use SNMPV1, default\n");
	  printf("         -v2 , use SNMPV2\n");
	  printf("         -cCommunity_name,(default community is 'public') \n");
  	  printf("         -pPort, specify SNMP port default is 161 \n");
	  printf("         -rN , retries default is N = 1 retry\n");
	  printf("         -tN , timeout in hundredths-seconds default is N = 100 = 1 second\n");
	  return 1;
   }


   //---------[ make a GenAddress and CNmsSnmpOid object to retrieve ]---------------
   GenAddress address( argv[1]);      // make a SNMP++ Generic address
   if ( !address.valid()) {           // check validity of address
	  printf("Invalid Address or DNS Name %s\n", argv[1]);
	  return 1;
   }

 
   CNmsSnmpOid oid(DEFAULT_OID);      // set the default value for oid
   if ( argc >= 3)                    // if 3 args, then use the callers CNmsSnmpOid
   {
	  if ( strstr( argv[2],"-")==0) 
	  {
		  // Check if we ask for several oids. They should be separated by ","
		  // like that: "1.3.6.1.2.1.1.1.0,1.3.6.1.2.1.1.2.0,1.3.6.1.4.1"
	    char *ptr, *ptr1;
		char oiddef[255];
		ptr1 = argv[2];
		while( (ptr = strchr(ptr1, (int) ',')) != NULL)
		{
			*ptr='\0';
			sscanf(ptr1, "%s", oiddef);
			oid = oiddef;
			if ( !oid.valid()) {         // check validity of user oid
				printf("Invalid CNmsSnmpOid %s \n", ptr1);
				return 1;
			}
 			Vb vb;                                  // construct a Vb object
			vb.set_oid( oid);                       // set the CNmsSnmpOid portion of the Vb
			pdu += vb;                              // add the vb to the CNmsSnmpPdu
			ptr1 = ptr+1;
		}
		// Get the last oid
		sscanf(ptr1, "%s", oiddef);
		oid = oiddef;
		if ( !oid.valid()) {         // check validity of user oid
			printf("Invalid CNmsSnmpOid %s \n", ptr1);
			return 1;
		}
 		Vb vb;                                  // construct a Vb object
		vb.set_oid( oid);                       // set the CNmsSnmpOid portion of the Vb
		pdu += vb;                              // add the vb to the CNmsSnmpPdu
		ptr1 = ptr+1;
		
      }
	   else
	   {
 			 Vb vb;                                  // construct a Vb object
			 vb.set_oid( oid);                       // set the CNmsSnmpOid portion of the Vb
			  pdu += vb;                              // add the vb to the CNmsSnmpPdu
	   }

   }
   else
   {
 		 Vb vb;                                  // construct a Vb object
		 vb.set_oid( oid);                       // set the CNmsSnmpOid portion of the Vb
		  pdu += vb;                              // add the vb to the CNmsSnmpPdu
   }



   //---------[ determine options to use ]-----------------------------------
   snmp_version version=version1;                       // default is v1
   int retries=1;                                       // default retries is 1
   int timeout=100;                                     // default is 1 second
   unsigned int port=161;								// Default SNMP UDP port is 161
   OctetStr community("public");                        // community name
   char *ptr;
   for(int x=1;x<argc;x++) {                           // parse for version
      if ( strstr( argv[x],"-v2")!= 0)   
         version = version2c;
      if ( strstr( argv[x],"-r")!= 0) {                 // parse for retries
         ptr = argv[x]; ptr++; ptr++;
		 retries = atoi(ptr);
		 if (retries<1) retries=1; 
      }
	  if ( strstr( argv[x], "-t")!=0) {                 // parse for timeout
		 ptr = argv[x]; ptr++; ptr++; 
		 timeout = atoi( ptr);
		 if (timeout < 100) timeout=100;
      }
	  if ( strstr( argv[x],"-c")!=0) {
		 ptr = argv[x]; ptr++; ptr++;
		 community = ptr;
      }
	  if ( strstr( argv[x], "-p")!=0) {                 // parse for port
		 ptr = argv[x]; ptr++; ptr++; 
		 port = atoi( ptr);
      }
   }

   //----------[ create a SNMP++ session ]-----------------------------------
   int status; 
   Snmp snmp( status);                // check construction status
   if ( status != SNMP_CLASS_SUCCESS) {
      printf("SNMP++ Session Create Fail, %s\n", (LPCSTR) snmp.error_msg(status));
      return 1;
   }

   //--------[ build up SNMP++ object needed ]-------------------------------
   Target target( address);               // make a target using the address
   target.set_version( version);           // set the SNMP version SNMPV1 or V2
   target.set_retry( retries);             // set the number of auto retries
   target.set_timeout( timeout);           // set timeout
   target.set_readcommunity( community);   // set read community
   target.set_port( port);				   // set the SNMP port
 
   //-------[ issue the request, blocked mode ]-----------------------------
   printf("SNMP++ Get to %s SNMPV%d Retries=%d", argv[1], (version+1), retries);
   printf(" Port=%d Timeout=%d ms Community=%s\n",target.get_port(), timeout, community.get_printable());
   if (( status = snmp.get( pdu,target))== SNMP_CLASS_SUCCESS) 
   {
	  Vb vb;                                  // construct a Vb object
	  int nCount = pdu.get_vb_count();
	  for(int i=0; i<nCount; i++)			
	  {
		pdu.get_vb( vb,i);
		printf("CNmsSnmpOid = %s\n", vb.get_printable_oid());
		printf("Value = %s\n", vb.get_printable_value());
	  }
   }
   else {
	  int nErrorIndex = pdu.get_error_index();
	  Vb vb;                                  // construct a Vb object
	  if(nErrorIndex>0)
		pdu.get_vb( vb,nErrorIndex-1);
	  else
		pdu.get_vb( vb,0);
	  printf("SNMP++ Get Error [error_index=%d related with %s]\n",nErrorIndex, (LPCSTR) vb.get_printable_oid());
      if ( status == SNMP_CLASS_ERR_STATUS_SET)
	     status = pdu.get_error_status();
      printf("%s\n", (LPCSTR) snmp.error_msg( status));
   }
   return 0;
}  // end get

