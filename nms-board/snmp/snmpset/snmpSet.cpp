/*
  snmpSet.cpp 

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
#include "NmsSnmp.h"
#include <stdlib.h>


// detrmine the smi type and get a value from
// the user
int determine_vb( SmiUINT32 val, Vb &vb) {
   char buffer[255];

   printf("Value Type is ");
   switch (val) {

	  // octet string
      case sNMP_SYNTAX_OCTETS:
	  {
	     printf("Octet String\n");
		 printf("Value ?");
      	 fgets( buffer, sizeof(buffer), stdin );
		 OctetStr octetstr( buffer);
		 if ( octetstr.valid()) {
		    vb.set_value( octetstr);
			return TRUE;
         }
		 else {
		   printf("Invalid OctetStr\n");
		   return FALSE;
         }
      }
	  break;

	  // IP Address
	  case sNMP_SYNTAX_IPADDR:
	  {
        printf("IP Address\n");
        printf("Value ?");
        fgets( buffer, sizeof(buffer), stdin );
        IpAddress ipaddress( buffer);
        if ( ipaddress.valid()) {
            vb.set_value( ipaddress);
            return TRUE;
        }
        else {
            printf("Invalid IP Address\n");
            return FALSE;
        }
	  }
	  break;

	  // CNmsSnmpOid
	  case sNMP_SYNTAX_OID:
	  {
	     printf("CNmsSnmpOid\n");
	     printf("Value ?");
     	 fgets( buffer, sizeof(buffer), stdin );
	     CNmsSnmpOid oid( buffer);
	     if ( oid.valid()) {
		    vb.set_value( oid);
		    return TRUE;
         }
	     else {
		    printf("Invalid CNmsSnmpOid\n");
		    return FALSE;
         }
      }
	  break;

	  // TimeTicks
	  case sNMP_SYNTAX_TIMETICKS:
	  {
	     printf("TimeTicks\n");
		 printf("Value ?");
     	 fgets( buffer, sizeof(buffer), stdin );
         DWORD i;
		 i = atol( buffer);
		 TimeTicks timeticks( i);
		 if ( timeticks.valid()) {
		    vb.set_value( timeticks);
			return TRUE;
         }
		 else {
			printf("Invalid TimeTicks\n");
			return FALSE;
         }
      }
	  break;

	  // Gauge32
      case sNMP_SYNTAX_GAUGE32:
	  {
         printf("Gauge32\n");
         printf("Value ?");
     	 fgets( buffer, sizeof(buffer), stdin );
         DWORD i;
         i = atol( buffer);
         Gauge32 gauge32(i); 
         if ( gauge32.valid()) {
            vb.set_value( gauge32);
            return TRUE;
         }
         else {
            printf("Invalid Gauge32\n");
            return FALSE;
         }
      }
	  break;

	  case sNMP_SYNTAX_CNTR32:
      {
         printf("Counter32\n");
         printf("Value ?");
         fgets( buffer, sizeof(buffer), stdin );
         DWORD i;
         i = atol( buffer);
         Counter32 counter32(i);
         if ( counter32.valid()) {
            vb.set_value( counter32);
            return TRUE;
         }
         else {
            printf("Invalid Counter32\n");
            return FALSE;
         }
      }
	  break;

	  case sNMP_SYNTAX_INT:
      {
         printf("Integer\n");
         printf("Value ?");
         fgets( buffer, sizeof(buffer), stdin );
         DWORD i;
         i = atol( buffer);
         INT32 l ;
		 l = ( INT32) i;
         vb.set_value( l);
         return TRUE;
      }
	  break;

	  case sNMP_SYNTAX_UINT32:
      {
         printf("Integer\n");
         printf("Value ?");
         fgets( buffer, sizeof(buffer), stdin );
         DWORD i;
         i = atol( buffer);
         vb.set_value( i);
         return TRUE;
      }
	  break;

	  default:
		 printf("Unknown Data Type\n");
		 return FALSE;
   }
}


#define DEFAULT_OID	"1.3.6.1.2.1.1.4.0"

int main( int argc, char **argv)  {

   CNmsSnmpPdu pdu;                                // construct a CNmsSnmpPdu object

   //---------[ check the arg count ]----------------------------------------
   if ( argc < 2) {
	  printf("Usage:\n");
	  printf("snmpSet Address | DNSName [CNmsSnmpOid] [options]\n");
	  printf("CNmsSnmpOid: (default value is %s)\n", DEFAULT_OID);
	  printf("options: -v1 , use SNMPV1, default\n");
	  printf("         -v2 , use SNMPV2\n");
	  printf("         -cCommunity_name (default community is 'public')\n");
  	  printf("         -pPort, specify SNMP port default is 161 \n");
	  printf("         -rN , retries default is N = 1 retry\n");
	  printf("         -tN , timeout in hundredths-seconds default is N = 100 = 1 second\n");
	  return 1;
   }

   //---------[ make a GenAddress and CNmsSnmpOid object to retrieve ]---------------
   GenAddress address( argv[1]);      // make a SNMP++ Generic address
   if ( !address.valid()) {           // check validity of address
	  printf("Invalid Address or DNS Name, %s\n", argv[1]);
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
				printf("Invalid CNmsSnmpOid, %s\n", ptr1);
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
			printf("Invalid CNmsSnmpOid, %s\n", ptr1);
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
   printf("SNMP++ Set to %s SNMPV%d Retries=%d", argv[1], (version+1), retries);
   printf(" Port=%d Timeout=%d ms\n", target.get_port(), timeout);

   // first get the variabel to determine its type
   CNmsSnmpPdu setpdu;
   if (( status = snmp.get( pdu,target))== SNMP_CLASS_SUCCESS) 
   {
	  Vb vb;                                  // construct a Vb object
	  int nCount = pdu.get_vb_count();
	  for(int i=0; i<nCount; i++)			
	  {
		pdu.get_vb( vb,i);
		printf("CNmsSnmpOid = %s\n", vb.get_printable_oid());
		printf("Value = %s\n", vb.get_printable_value());
	    if ( determine_vb(vb.get_syntax(), vb)) 
		{
	      // do the Set
		  setpdu += vb; 
		}
	  }
	  nCount = setpdu.get_vb_count();
	  if(nCount >0)
	  {
		  // Send the SNMP SET request  
		  set_error_status( &setpdu, 0);
		  status = snmp.set( setpdu, target);
		  if( status != SNMP_CLASS_SUCCESS)
		  {
			int nErrorIndex = setpdu.get_error_index();
			Vb vb;                                  // construct a Vb object
			if(nErrorIndex>0)
				setpdu.get_vb( vb,nErrorIndex-1);
			else
				setpdu.get_vb( vb,0);
            printf("SNMP++ Set Error [error_index=%d related with %s]\n",nErrorIndex, (LPCSTR) vb.get_printable_oid());
			if ( status == SNMP_CLASS_ERR_STATUS_SET)
				status = setpdu.get_error_status();
            printf("%s\n", (LPCSTR) snmp.error_msg( status));
		  }
	  }
	  else
		 printf("Error => There are no oids to set\n");
   }
   else 
   {
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
}  // end snmpSet

