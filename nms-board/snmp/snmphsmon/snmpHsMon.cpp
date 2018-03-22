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
#define CM_SNMP_REFRESH         102
#define CM_SNMP_WRONG_KEY       103
#define CM_SNMP_HELP            104
#define CM_SNMP_EXTRACT         105
#define CM_SNMP_INSERT          106
#define CM_SNMP_INVALID_COMMAND 107


class CNmsBoard
{
public:
    CNmsBoard();
  virtual ~CNmsBoard();


  BOOL  IsPCI() const;

public:
    int              m_nBoardIndex;
    CNmsString       m_szBoardModelText;
    int              m_BoardFamilyNumber;
    SnmpBusType      m_BusSegmentType;
    int              m_nBusSegmentNumber;
    int              m_nSlotNumber;
    SnmpBoardStatus  m_BoardStatus;
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
    int  snmpGetBoardList( BOOL bEnableTraps = FALSE );
    int  extractBoard( int nBoard );
    int  insertBoard( int nBoard );

        void walkByBus( int nBusIndex, int nBusType, int nSlotCount );
        void getBoardInfo( CNmsBoard& board  );
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
  int  processCommand( int &);

//
// End SNMP demo functions definition ----------------------------------------


CNmsBoard::CNmsBoard()
{
}

CNmsBoard::~CNmsBoard()
{
}

// Global Variables ----------------------------------------------------------

int         retries=1;                   // default retries is 1
int         timeout=1000;                 // default is 1 second
unsigned int port=161;					 // Default SNMP UDP port is 161
OctetStr    community("public");         // community name
char*       pGenAddrStr   = "127.0.0.1";

static CNmsSnmp     * g_pSnmp;
static CNmsSnmpTrap * g_pSnmpTrap;

BOOL        bCommand = TRUE;

// End Global Variables ------------------------------------------------------

int main( int argc, char **argv )
{
    demoHeader();

    try
    {
        THROW_SNMP_EXCEPTION( createSnmpSession( argc, argv ) );
		printf("SnmpSession OK\n");
        THROW_SNMP_EXCEPTION( createTrapSession() );
		printf("TrapSession OK\n");
        run();
    }
    catch( CNmsSnmpException e )
    {
        e.ReportError();
    }

    if( g_pSnmp )
        delete g_pSnmp;
	printf("OK\n");
    if( g_pSnmpTrap )
        delete g_pSnmpTrap;
return 1;
};

int  createTrapSession()
{
int nStatus;
	printf("creating the trap\n");
    g_pSnmpTrap = new CNmsSnmpTrap( nStatus );

    if( nStatus == SNMP_CLASS_SUCCESS )
	{
		printf("enabling traps\n");
        nStatus = g_pSnmpTrap->EnableTraps( TRUE );
		printf("traps enabled\n");
	}
    if( nStatus == SNMP_CLASS_SUCCESS )
    {
        Target target( GenAddress( g_pSnmp->GetPrintableAddress() ), community, community );
		printf("target created\n");
        CNmsSnmpOid    trapOid( SZ_OID_NMSCHASSIS_TRAP_ENTRY );
		printf("oid created\n");
        SnmpCallback callback( &target, &trapOid, &trap_callback );
		printf("Snmpcallback created\n");
        nStatus = g_pSnmpTrap->notify_register( callback );
		printf("notified\n");
    }

return nStatus;
};

int  createSnmpSession( int argc, char **argv )
{
int nStatus;

printf(" debut session\n");

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
int     nStatus;
int     nBoardNo;        
int     BoardStatus;
DWORD   dwTime;
CNmsString szTime;

        Vb vb;
        if( pdu.get_vb( vb, 0 ) )
        {
            CNmsSnmpOid oid;
            vb.get_oid( oid );
            int nBoardIndex = oid[ oid.len() - 1 ];
            oid = SnmpMakeFieldOid( SZ_OID_BOARD_TABLE_ENTRY, BOARD_FAMILY_NUMBER, nBoardIndex );
            nStatus = g_pSnmp->SnmpGet( nBoardNo, oid );

            vb.get_value( BoardStatus );
        }
        if( pdu.get_vb( vb, 1 ) )
        {
            vb.get_value( dwTime );
            TimeTicks time( dwTime );
            szTime = time.get_printable();
        }
        if( bCommand )
            printf("\n");
        printf("< %s   ", szTime.c_str() );
        printf("%d Board %s\n", nBoardNo, SnmpBoardStatusToStr( (SnmpBoardStatus)BoardStatus ) );
        bCommand = FALSE;
};



void run()
{
int nCommand = CM_SNMP_NO_COMMAND;
    if( snmpGetSysInfo() == SNMP_CLASS_SUCCESS )
        snmpGetBoardList( TRUE );
int nBoard;
    do
    {
        printf("> ");
        nCommand = processCommand( nBoard );
        bCommand = TRUE;
        switch( nCommand )
        {
            case CM_SNMP_REFRESH:
//                if( snmpGetSysInfo() != SNMP_CLASS_SUCCESS )
//                    break;
                snmpGetBoardList( TRUE );
                break;
            case CM_SNMP_QUIT:
                break;
            case CM_SNMP_HELP:
                printHelp();
                break;
            case CM_SNMP_EXTRACT:
                extractBoard( nBoard );
                break;
            case CM_SNMP_INSERT:
                insertBoard( nBoard );
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


int snmpGetBoardList( BOOL bEnableTraps )
{
int nStatus = SNMP_CLASS_SUCCESS;
    printf("SEND A REQUEST FOR NMS BOARDS TO: %s\n", g_pSnmp->GetPrintableAddress() );
    try
    { 
        // Enable traps for all new boards
        if( bEnableTraps )
            THROW_SNMP_EXCEPTION( g_pSnmp->SnmpSet( (int)TrapOn, SZ_OID_CHASSIS_BOARD_TRAP_ENABLE ) );

        Vb vb;
        vb.set_oid( SZ_OID_BUS_SEGMENT_TABLE_ENTRY );
        THROW_SNMP_EXCEPTION( g_pSnmp->SnmpGetFirst( vb ) );

        int nBusIndex, nBusType, nUsedSlots;  // Get a bus type
        do
        {
            vb.get_value( nBusIndex );

            Vb vbl[2];
            CNmsSnmpOid oid = SnmpMakeFieldOid( SZ_OID_BUS_SEGMENT_TABLE_ENTRY, BUS_SEGMENT_TYPE, nBusIndex );
            vbl[0].set_oid( oid );
            vbl[1].set_oid( SnmpMakeFieldOid( SZ_OID_BUS_SEGMENT_TABLE_ENTRY, BUS_SEGMENT_SLOTS_OCCUPIED, nBusIndex ));
            THROW_SNMP_EXCEPTION( g_pSnmp->SnmpGet( vbl, 2 ) );
            vbl[0].get_value( nBusType );
            vbl[1].get_value( nUsedSlots );

            printf("             %s bus\n", SnmpBusTypeToStr( (SnmpBusType)nBusType ) );
            walkByBus( nBusIndex, nBusType, nUsedSlots );

        } while ( g_pSnmp->SnmpGetNextInColumn( vb, SZ_OID_BUS_SEGMENT_TABLE_ENTRY, BUS_SEGMENT_INDEX ) == SNMP_CLASS_SUCCESS );
    }
    catch ( CNmsSnmpException e )
    {
        nStatus = e.m_nStatus;
        printf("Error %s\n", (LPCSTR) g_pSnmp->GetPrintableError( nStatus ) );
    }
    printf("\n");

return nStatus;
};


void walkByBus( int nBusIndex, int nBusType, int nSlotCount )
{
CNmsSnmpOid BoardAccessTableEntry( SZ_OID_BOARD_ACCESS_TABLE_ENTRY );
int nBoardIndex;

    CNmsSnmpOid oid;

    for( int nSlotIndex = 1; nSlotIndex <= nSlotCount; nSlotIndex++ )
    {
        oid = BoardAccessTableEntry;
        oid += BOARD_ACCESS_BOARD_INDEX; // Add column
        oid += nBusIndex;                // Add indexes
        oid += nSlotIndex;

        THROW_SNMP_EXCEPTION( g_pSnmp->SnmpGet( nBoardIndex, oid ) );

        CNmsBoard aBoard;
        aBoard.m_nBoardIndex = nBoardIndex;

        getBoardInfo( aBoard  );

        printf("Board %d:", aBoard.m_BoardFamilyNumber);
        printf("       %s", (LPCSTR)aBoard.m_szBoardModelText);
        if( aBoard.m_BusSegmentType != 1 )
        {
            printf(" Segment:%d", aBoard.m_nBusSegmentNumber);
            printf(" Slot:%d", aBoard.m_nSlotNumber);
        }
        printf(" Status:%s\n", SnmpBoardStatusToStr( aBoard.m_BoardStatus ) );
    }
};


void getBoardInfo( CNmsBoard& board )
{
    THROW_SNMP_EXCEPTION( g_pSnmp->SnmpGet( board.m_szBoardModelText, 
                          SnmpMakeFieldOid(SZ_OID_BOARD_TABLE_ENTRY, BOARD_MODEL_TEXT, board.m_nBoardIndex ) ));
    
    THROW_SNMP_EXCEPTION( g_pSnmp->SnmpGet( board.m_nBusSegmentNumber, 
                          SnmpMakeFieldOid(SZ_OID_BOARD_TABLE_ENTRY, BOARD_BUS_SEGMENT_NUMBER, board.m_nBoardIndex ) ));

    THROW_SNMP_EXCEPTION( g_pSnmp->SnmpGet( board.m_nSlotNumber, 
                          SnmpMakeFieldOid(SZ_OID_BOARD_TABLE_ENTRY, BOARD_SLOT_NUMBER, board.m_nBoardIndex ) ));

    THROW_SNMP_EXCEPTION( g_pSnmp->SnmpGet( board.m_BoardFamilyNumber, 
                          SnmpMakeFieldOid(SZ_OID_BOARD_TABLE_ENTRY, BOARD_FAMILY_NUMBER, board.m_nBoardIndex ) ));

    THROW_SNMP_EXCEPTION( g_pSnmp->SnmpGet( (int &)board.m_BoardStatus, 
                          SnmpMakeFieldOid(SZ_OID_BOARD_TABLE_ENTRY, BOARD_STATUS, board.m_nBoardIndex ) ));

    THROW_SNMP_EXCEPTION( g_pSnmp->SnmpGet( (int &)board.m_BusSegmentType, 
                          SnmpMakeFieldOid(SZ_OID_BOARD_TABLE_ENTRY, BOARD_BUS_SEGMENT_TYPE, board.m_nBoardIndex ) ));

    SnmpTrapEnable bTrapEnable;
    THROW_SNMP_EXCEPTION( g_pSnmp->SnmpGet( (int &)bTrapEnable, 
                          SnmpMakeFieldOid(SZ_OID_BOARD_TABLE_ENTRY, BOARD_TRAP_ENABLE, board.m_nBoardIndex ) ));
    if( bTrapEnable != TrapOn )
    THROW_SNMP_EXCEPTION( g_pSnmp->SnmpSet( (int )TrapOn, 
                          SnmpMakeFieldOid(SZ_OID_BOARD_TABLE_ENTRY, BOARD_TRAP_ENABLE, board.m_nBoardIndex ) ));
};


int extractBoard( int nBoard )
{
int nStatus = SNMP_CLASS_SUCCESS;
    // Find this board in a list;

    try
    {
        int nBoardFamilyNumber;
        CNmsSnmpOid oid( SnmpMakeFieldOid(SZ_OID_BOARD_TABLE_ENTRY, BOARD_FAMILY_NUMBER, 1 ) );
        THROW_SNMP_EXCEPTION( g_pSnmp->SnmpGet( nBoardFamilyNumber, oid ));

        Vb vb;
        vb.set_oid( oid );

        while( nBoardFamilyNumber != nBoard )
        {
            nStatus = g_pSnmp->SnmpGetNextInColumn( vb, SZ_OID_BOARD_TABLE_ENTRY, BOARD_FAMILY_NUMBER );
            if( nStatus != SNMP_CLASS_SUCCESS )
                break;
            else
                vb.get_value( nBoardFamilyNumber );
        }

        if( nStatus == SNMP_CLASS_SUCCESS )
        {
            // Board was found;
            vb.get_oid( oid );
            CNmsBoard board;
            board.m_nBoardIndex = oid[oid.len() - 1];
            getBoardInfo( board );

            if( board.m_BusSegmentType == ISA )
                printf("< Enable to extract ISA board\n");
            else
            {   // Extract board
                nStatus = g_pSnmp->SnmpSet( BoardCommand_Off, 
                    SnmpMakeFieldOid(SZ_OID_BOARD_TABLE_ENTRY, BOARD_COMMAND, board.m_nBoardIndex ) );
                if( nStatus != SNMP_CLASS_SUCCESS )
                    printf("< Enable to extract the board from not a CPCI chassis\n");

            }
        }
    }
    catch( CNmsSnmpException e )
    {
        e.ReportError();
    }
return 1;
};


int insertBoard( int nBoard )
{
int nStatus = SNMP_CLASS_SUCCESS;
    // Find this board in a list;

    try
    {
        int nBoardFamilyNumber;
        CNmsSnmpOid oid( SnmpMakeFieldOid(SZ_OID_BOARD_TABLE_ENTRY, BOARD_FAMILY_NUMBER, 1 ) );
        THROW_SNMP_EXCEPTION( g_pSnmp->SnmpGet( nBoardFamilyNumber, oid ));

        Vb vb;
        vb.set_oid( oid );

        while( nBoardFamilyNumber != nBoard )
        {
            nStatus = g_pSnmp->SnmpGetNextInColumn( vb, SZ_OID_BOARD_TABLE_ENTRY, BOARD_FAMILY_NUMBER );
            if( nStatus != SNMP_CLASS_SUCCESS )
                break;
            else
                vb.get_value( nBoardFamilyNumber );
        }

        if( nStatus == SNMP_CLASS_SUCCESS )
        {
            // Board was found;
            vb.get_oid( oid );
            CNmsBoard board;
            board.m_nBoardIndex = oid[oid.len() - 1];
            getBoardInfo( board );

            if( board.m_BusSegmentType == ISA )
                printf("< Enable to insert ISA board\n");
            else
            {   // Insert board
                nStatus = g_pSnmp->SnmpSet( BoardCommand_On, 
                    SnmpMakeFieldOid(SZ_OID_BOARD_TABLE_ENTRY, BOARD_COMMAND, board.m_nBoardIndex ) );
                if( nStatus != SNMP_CLASS_SUCCESS )
                    printf("< Enable to extract the board from not a CPCI chassis\n");
            }
        }
    }
    catch( CNmsSnmpException e )
    {
        e.ReportError();
    }

return 1;
};



void demoHeader()
{
    printf("SNMP Demonstration and Test Program  V. %s %s\n",  VERSION_NUM , VERSION_DATE);
    printf("NMS Communications Corporation.\n\n");
};

void printHelp()
{
    printf("Usage:\n");
    printf("snmpHsMon [Address | DNSName] [options]\n");
    printf("Address:  default is 127.0.0.1\n");
    printf("options: -cCommunity_name, specify community default is 'public'\n");
    printf("         -pPort, specify SNMP port default is 161 \n");
    printf("         -rN , retries default is N = 1 retry\n");
    printf("         -tN , timeout in hundredths-seconds default is N = 100 = 1 second\n\n");
    printUsage();
}

void printUsage()
{
    printf("h  Help     r  Refresh    i<N> Insert    e<N> Extract  Q  Quit\n\n");
};

int  processCommand( int& nBoard )
{
int nCommand = CM_SNMP_NO_COMMAND;
char szCmdBuf[256];
int i = 0;
    if( fgets( szCmdBuf, sizeof(szCmdBuf), stdin ) )
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
            case 'R':
                nCommand = CM_SNMP_REFRESH;
                break;
            case 'E':
                {
                    nCommand = CM_SNMP_EXTRACT;
                    int c = 0;
                    while( isdigit( szCmdBuf[c+1] ) )
                    {
                        szCmdBuf[c] = szCmdBuf[c+1];
                        c++;
                    }
                    szCmdBuf[c] = '\0';
                    if( c == 0 )
                        nCommand = CM_SNMP_INVALID_COMMAND;
                    nBoard = atoi( szCmdBuf );
                    break;
                }
            case 'I':
                {
                    nCommand = CM_SNMP_INSERT;
                    int c = 0;
                    while( isdigit( szCmdBuf[c+1] ) )
                    {
                        szCmdBuf[c] = szCmdBuf[c+1];
                        c++;
                    }
                    szCmdBuf[c] = '\0';
                    if( c == 0 )
                        nCommand = CM_SNMP_INVALID_COMMAND;
                    nBoard = atoi( szCmdBuf );
                    break;
                }
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
                if (( retries<1)|| (retries>5)) 
                    retries=1;
            }
            if ( strstr( argv[x], "-t")!=0) 
            {                 // parse for timeout
                ptr = argv[x]; ptr++; ptr++;
                timeout = atoi( ptr);
                if (( timeout < 100)||( timeout>500)) 
                    timeout=100;
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
