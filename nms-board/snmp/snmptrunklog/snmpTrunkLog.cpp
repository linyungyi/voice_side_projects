#define VERSION_NUM  "3.0 "
#define VERSION_DATE __DATE__

#ifndef   NMSSNMP_H
#include <NmsSnmp.h>
#endif

#ifndef   NMSSNMPTRAP_H
#include <NmsSnmpTrap.h>
#endif

#ifndef   NMSSNMPEXCEPTION_H
#include <NmsSnmpException.h>
#endif

#ifndef   SNMPDEMODEFS_H
#include <SnmpDemoDefs.h>
#endif

#include <NmsNfcUtil.h>

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>


#define CM_SNMP_NO_COMMAND      100
#define CM_SNMP_QUIT            101
#define CM_SNMP_TRUNK_LIST      102
#define CM_SNMP_SYS_INFO        103
#define CM_SNMP_HELP            104
#define CM_SNMP_INVALID_COMMAND 107


class CNmsTrunk
{
public:
    CNmsTrunk();
    virtual ~CNmsTrunk();


public:
    int         dsx1LineIndex;
    OctetStr    dsx1CircuitIdentifier;
    int         dsx1LineStatus;
    TimeTicks   dsx1LineStatusLastChange;
};


// SNMP demo functions definition --------------------------------------------
//
//////////////////////////////////////////////////////////////////
// SNMP functions
//
//
    int  createTrapSession();
    int  createSnmpSession( int argc, char **argv );
    void trap_callback( const CNmsSnmpPdu&      pdu,     // pdu passsed in
                        const Target&   target,  // target passsed in
                        void * );                // optional callback data

    void run();
    int  snmpGetSysInfo();
    int  snmpGetTrunkList( BOOL bEnableTraps = FALSE );
//
//////////////////////////////////////////////////////////////////
// Parse arguments 
//
    void snmpParseACommandLine( int argc, char **argv,
                                int& retries, int& timeout, 
                                OctetStr& community, 
                                unsigned int &port, 
                                char** ppGenAddrStr );

//////////////////////////////////////////////////////////////////
// UI Functions
//

  void demoHeader();
  void printHelp();
  void printUsage();
  int  processCommand();
  void displayData( CNmsTrunk& );
  void displayTrunkStatus( int status, int = 0 );

//
// End SNMP demo functions definition ----------------------------------------


CNmsTrunk::CNmsTrunk()
{
}

CNmsTrunk::~CNmsTrunk()
{
}

// Global Variables ----------------------------------------------------------

int         retries=1;                   // default retries is 1
int         timeout=100;                 // default is 1 second
unsigned int port=161;					 // Default SNMP UDP port is 161
OctetStr    community("public");         // community name
char*       pGenAddrStr   = "127.0.0.1";

static CNmsSnmp     * g_pSnmp=NULL;
static CNmsSnmpTrap * g_pSnmpTrap=NULL;

BOOL        bCommand = TRUE;

// End Global Variables ------------------------------------------------------

int main( int argc, char **argv )
{
    demoHeader();

    try
    {
        THROW_SNMP_EXCEPTION( createSnmpSession( argc, argv ) );
        THROW_SNMP_EXCEPTION( createTrapSession() );
        run();
    }
    catch( CNmsSnmpException e )
    {
        e.ReportError();
    }

    if( g_pSnmp )
        delete g_pSnmp;
    if( g_pSnmpTrap )
        delete g_pSnmpTrap;
return 1;
};

int  createTrapSession()
{
int nStatus;
    g_pSnmpTrap = new CNmsSnmpTrap( nStatus );
    if( nStatus == SNMP_CLASS_SUCCESS )
        nStatus = g_pSnmpTrap->EnableTraps( TRUE );
    if( nStatus == SNMP_CLASS_SUCCESS )
    {
        Target target( GenAddress( g_pSnmp->GetPrintableAddress() ), community, community );
        CNmsSnmpOid    trapOid( SZ_OID_DS1_TRAPS_ENTRY );

        SnmpCallback callback( &target, &trapOid, &trap_callback );
        nStatus = g_pSnmpTrap->notify_register( callback );
    }
    else
        printf("Unable to create trap session\n");

return nStatus;
};

int  createSnmpSession( int argc, char **argv )
{
int nStatus;

    snmpParseACommandLine( argc, argv,
                           retries, timeout, community, port, &pGenAddrStr );

    g_pSnmp = new CNmsSnmp( nStatus );

    if( nStatus == SNMP_CLASS_SUCCESS )
    {
        g_pSnmp->SetParams( pGenAddrStr,      // IpAddress or DNS name
                            version1,
                            community,    // read community
                            community,    // write community
                            retries,      // retries,
                            timeout,      // timeout ms
                            port          // UDP Snmp Port
                         );
    }
return nStatus;
};


void trap_callback( const CNmsSnmpPdu&      pdu,     // pdu passsed in
                    const Target&   target,  // target passsed in
                    void * )
{
CNmsTrunk   trunk;

        Vb vb;
        if( pdu.get_vb( vb, 0 ) )
        {
            CNmsSnmpOid oid;
            vb.get_oid( oid );

            trunk.dsx1LineIndex = oid[ oid.len() - 1 ];
            vb.get_value( trunk.dsx1LineStatus );
        }
        if( pdu.get_vb( vb, 1 ) )
        {
            vb.get_value( trunk.dsx1LineStatusLastChange );
        }
        printf("Interface:%d ", trunk.dsx1LineIndex );
        displayTrunkStatus( trunk.dsx1LineStatus, trunk.dsx1LineStatusLastChange );
};



void run()
{
int nCommand = CM_SNMP_NO_COMMAND;
    if( snmpGetSysInfo() == SNMP_CLASS_SUCCESS )
        snmpGetTrunkList( TRUE );
    do
    {
        printf( "> ");
        nCommand = processCommand();
        bCommand = TRUE;
        switch( nCommand )
        {
            case CM_SNMP_SYS_INFO:
                snmpGetSysInfo();
                break;
            case CM_SNMP_TRUNK_LIST:
                snmpGetTrunkList();
                break;
            case CM_SNMP_QUIT:
                break;
            case CM_SNMP_HELP:
                printHelp();
                break;
            case CM_SNMP_NO_COMMAND:
                break;
            default:
                printf("?\n");
                printUsage();
                break;
        }
    } while( nCommand != CM_SNMP_QUIT );
}


int snmpGetSysInfo()
{
int nStatus;
const int NUM_SYS_VBS = 5;

    printf("SEND A REQUEST FOR SYSTEM INFO TO: %s\n", g_pSnmp->GetPrintableAddress() );

    Vb vbl[NUM_SYS_VBS];
    vbl[0].set_oid(SZ_OID_SYSDESC);
    vbl[1].set_oid(SZ_OID_SYSUPTIME);
    vbl[2].set_oid(SZ_OID_SYSCONTACT);
    vbl[3].set_oid(SZ_OID_SYSNAME);
    vbl[4].set_oid(SZ_OID_SYSLOCATION);

    nStatus = g_pSnmp->SnmpGet( vbl, NUM_SYS_VBS );

    if( nStatus == SNMP_CLASS_SUCCESS )
    {
        printf("System information:\n");
        printf("System:        %s\n", vbl[0].get_printable_value() );
        printf("SysUpTime:     %s\n", vbl[1].get_printable_value() );
        printf("SysContact:    %s\n", vbl[2].get_printable_value() );
        printf("Computer name: %s\n", vbl[3].get_printable_value() );
        printf("Location:      %s\n", vbl[4].get_printable_value() );
    }
    else
    {
        printf("Error %s\n", (LPCSTR) g_pSnmp->GetPrintableError( nStatus ) );
    }
    printf("\n");

return nStatus;
};

#ifdef WIN32
#pragma optimize( "", off )
#endif

int snmpGetTrunkList( BOOL bEnableTraps )
{
int nStatus = SNMP_CLASS_SUCCESS;
    printf("SEND A REQUEST FOR TRUNKS TO: %s\n", g_pSnmp->GetPrintableAddress() );
    try
    { 
        Vb vb;
        vb.set_oid( SZ_OID_DSX1_CONFIG_ENTRY );
        THROW_SNMP_EXCEPTION( g_pSnmp->SnmpGetFirst( vb ) );

        do
        {
            CNmsTrunk   trunk;
            vb.get_value( trunk.dsx1LineIndex );

            Vb vbl[3];
            vbl[0].set_oid( SnmpMakeFieldOid( SZ_OID_DSX1_CONFIG_ENTRY, DSX1_CIRCUIT_IDENTIFIER, trunk.dsx1LineIndex ) );
            vbl[1].set_oid( SnmpMakeFieldOid( SZ_OID_DSX1_CONFIG_ENTRY, DSX1_LINE_STATUS, trunk.dsx1LineIndex ));
            vbl[2].set_oid( SnmpMakeFieldOid( SZ_OID_DSX1_CONFIG_ENTRY, DSX1_LINE_STATUSLASTCHANGE, trunk.dsx1LineIndex ));
            THROW_SNMP_EXCEPTION( g_pSnmp->SnmpGet( vbl, 3 ) );
            vbl[0].get_value( trunk.dsx1CircuitIdentifier );
            vbl[1].get_value( trunk.dsx1LineStatus );
            vbl[2].get_value( trunk.dsx1LineStatusLastChange );

            displayData( trunk );

            if( g_pSnmp->SnmpSet( TrapOn, 
				                  SnmpMakeFieldOid( SZ_OID_DSX1_CONFIG_ENTRY, 
								                    DSX1_LINE_STATUSCHANGETRAPENABLE, 
													trunk.dsx1LineIndex ) ) != SNMP_CLASS_SUCCESS )
			{
				printf("Warning Traps from interface %d are disabled\n", trunk.dsx1LineIndex);
			}


        } while ( g_pSnmp->SnmpGetNextInColumn( vb, SZ_OID_DSX1_CONFIG_ENTRY, DSX1_LINE_INDEX ) == SNMP_CLASS_SUCCESS );
    }
    catch ( CNmsSnmpException e )
    {
        nStatus = e.m_nStatus;
        printf("Error %s\n", (LPCSTR) g_pSnmp->GetPrintableError( nStatus ) );
    }
    printf("\n");

return nStatus;
};

#ifdef WIN32
#pragma optimize( "", on ) 
#endif


void displayData( CNmsTrunk& trunk )
{
int nBoard; int nTrunk;
CNmsString szBoard;

    CNmsString str( (LPCSTR)trunk.dsx1CircuitIdentifier.data() );

    int nStart = 0;
    int nStop;
    nStop = str.Find( '_', nStart );
    if( nStop > 0 )
    {
        szBoard = str.Mid(  nStart, nStop - nStart );
        nStop ++;
        nStart = nStop;
    }
    nStop = str.Find( '_', nStart );
    if( nStop > 0)
    {
        nStart = str.ReverseFind( '_' );
        nTrunk = atoi( (LPCSTR)str.Mid( nStart + 1, str.GetLength() - nStart ) );

        str.SetAt(nStart, '\0');
        nStart = str.ReverseFind( '_' );
        nBoard = atoi( (LPCSTR)str.Mid( nStart + 1, str.GetLength() - nStart ) );
    }

    printf("Interface:%d Board:%d (%s)  Trunk:%d Status:", trunk.dsx1LineIndex, nBoard, (LPCSTR)szBoard, nTrunk);
    displayTrunkStatus( trunk.dsx1LineStatus );

};


void displayTrunkStatus( int dsx1LineStatus, int dsx1LineStatusLastChange )
{
    if( dsx1LineStatusLastChange )
    {
        TimeTicks ticks( dsx1LineStatusLastChange );
        printf("%s ", ticks.get_printable() );
    }

    CNmsString szText("");
    if( dsx1LineStatus == 1 )
        szText += "In service, ";
    else
    {
        if( dsx1LineStatus & 2 )
            szText += "Far end alarm indication, ";
        if( dsx1LineStatus & 4 )
            printf("Near end LOF Indication ");
        if( dsx1LineStatus & 8 )
            szText += "All 1's alarm, ";
        if( dsx1LineStatus & 16 )
            szText += "Near end sending AIS, ";
        if( dsx1LineStatus & 32 )
            szText += "Loss of frame, ";
        if( dsx1LineStatus & 64 )
            szText += "NoSgnl, ";
        if( dsx1LineStatus & 256 )
            szText += "Time slot 16 AIS, ";
        if( dsx1LineStatus & 512 )
            printf("Far end loss of multiframe ");
    }
    int nLength = szText.GetLength();
    if( nLength == 0 )
        printf("%d", dsx1LineStatus);
    else
        szText.SetAt( nLength - 2, '\0');
    printf("%s\n", (LPCSTR)szText );
};


void demoHeader()
{
    printf("SNMP Demonstration and Test Program  V. %s %s\n",  VERSION_NUM , VERSION_DATE);
    printf("NMS Communications Corporation.\n\n");
};

void printHelp()
{
    printf("Usage:\n");
    printf("snmpTrunkLog [Address | DNSName] [options]\n");
    printf("Address:  default is 127.0.0.1\n");
    printf("options: -cCommunity_name, specify community default is 'public'\n");
    printf("         -pPort, specify SNMP port default is 161 \n");
    printf("         -rN , retries default is N = 1 retry\n");
    printf("         -tN , timeout in hundredths-seconds default is N = 100 = 1 second\n\n");
    printUsage();
}

void printUsage()
{
    printf("h  Help     S  Sys info    L  Trunk list    Q  Quit\n\n");
};

int  processCommand()
{
int nCommand = CM_SNMP_NO_COMMAND;
char szCmdBuf[256];
int i = 0;
    if( fgets( szCmdBuf , sizeof(szCmdBuf), stdin ) )
    {
        while( szCmdBuf[i] != '\0' )
        {
            if( szCmdBuf[i] == ' ')
                strcpy( &szCmdBuf[i], &szCmdBuf[i+1] );
            else
            {
                szCmdBuf[i] = toupper( szCmdBuf[i] );
                i++;
            }
        }
        switch( szCmdBuf[0] )
        {
            case 'Q':
            case '\033':
            case '\010': /* ESC or BS */
                nCommand = CM_SNMP_QUIT;
                break;
            case 'H':
                nCommand = CM_SNMP_HELP;
                break;
            case 'L':
                nCommand = CM_SNMP_TRUNK_LIST;
                break;
            case 'S':
                nCommand = CM_SNMP_SYS_INFO;
                break;
            default:
                nCommand = CM_SNMP_NO_COMMAND;
        }
    }
return nCommand;
};


void snmpParseACommandLine( int argc, char **argv,
                            int& retries, 
                            int& timeout, 
                            OctetStr& community, 
                            unsigned int& port, 
                            char** ppGenAddrStr )
{
    if( argc > 1 )
    {
        //---------[ make a GenAddress and CNmsSnmpOid object to retrieve ]---------------
        *ppGenAddrStr = argv[1];      // make a SNMP++ Generic address

        char *ptr;
        for(int x = 2; x < argc; x++ ) 
        {  
            if ( strstr( argv[x],"-r")!= 0) 
            {                 // parse for retries
                ptr = argv[x]; ptr++; ptr++;
                retries = atoi(ptr);
                if (retries<1) retries=1;
            }
            if ( strstr( argv[x], "-t")!=0) 
            {                 // parse for timeout
                ptr = argv[x]; ptr++; ptr++;
                timeout = atoi( ptr);
                if (timeout < 100)  timeout=100;
            }
            if ( strstr( argv[x],"-c")!=0) 
            {
                ptr = argv[x]; ptr++; ptr++;
                community = ptr;
            }
		  	if ( strstr( argv[x], "-p")!=0)
			{                 // parse for port
			 	ptr = argv[x]; ptr++; ptr++; 
			 	port = atoi( ptr);
 	       }
        }
        printUsage();
    }
    else
        printHelp();

};
