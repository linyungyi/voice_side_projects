#define VERSION_NUM  "3.0 "
#define VERSION_DATE __DATE__

#ifndef   NMSSNMP_H
#include <NmsSnmp.h>
#endif

#ifndef   SNMPDEMODEFS_H
#include <SnmpDemoDefs.h>
#endif

#ifndef   NMSSNMPEXCEPTION_H
#include <NmsSnmpException.h>
#endif

#ifndef   CNMSTHREAD_H
#include <CNmsThread.h>
#endif

#include <NmsNfcUtil.h>

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>


#define CM_SNMP_NO_COMMAND      100
#define CM_SNMP_QUIT            101
#define CM_SNMP_SYSINFO         102
#define CM_SNMP_BOARDLIST       103
#define CM_SNMP_WRONG_KEY       104
#define CM_SNMP_HELP            105
#define CM_SNMP_SETPOLLINTERVAL 106
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



class CPollingThread : public CNmsThread
{
public:
    CPollingThread();
  virtual ~CPollingThread();

  void  SetPollingInterval( int nTimeout );
protected:
  virtual void OnIdle();

private:
  void refresh();

private:
    int         m_nInterval;
    CNmsEvent   m_SetIntervalEvent;
};

// SNMP demo functions definition --------------------------------------------
//
//////////////////////////////////////////////////////////////////
// SNMP functions
//
//
    int  createSnmpSession( int argc, char **argv );

    void run();
    int  snmpGetSysInfo();
    int  snmpGetBoardList();

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
  void setPollInterval( int nInterval );

  int  processCommand( int &);

//
// End SNMP demo functions definition ----------------------------------------


// Global Variables ----------------------------------------------------------

int         retries=1;                   // default retries is 1
int         timeout=1000;                 // default is 1 second
unsigned int port=161;					 // Default SNMP UDP port is 161
OctetStr    community("public");         // community name
char*       pGenAddrStr   = "127.0.0.1";

static CNmsSnmp         * g_pSnmp;
static CPollingThread   * g_pPollingThread;

BOOL        bCommand = TRUE;

// End Global Variables ------------------------------------------------------


CNmsBoard::CNmsBoard()
{
}

CNmsBoard::~CNmsBoard()
{
}


CPollingThread::CPollingThread()
{
    m_nInterval = INFINITE;
};
 
CPollingThread::~CPollingThread()
{
};

void CPollingThread::SetPollingInterval( int nTimeout )
{
    m_nInterval = nTimeout;
    m_SetIntervalEvent.SetEvent();
};

void CPollingThread::OnIdle()
{
CNmsEvent evArray[2] = { m_evExit, m_SetIntervalEvent };

int nTimeout;
    if( m_nInterval != INFINITE )
        nTimeout = m_nInterval * 1000;
    else
        nTimeout = m_nInterval;

    DWORD dwWaitResult = SyncWaitForMultipleEvents(
                            sizeof( evArray ) / sizeof( CNmsEvent ),
                            evArray,
                            nTimeout );
    switch( dwWaitResult )
    {
        case NMS_WAIT_OBJECT_0:         //Second interval
            ExitThread( 101 );
            break;

        case NMS_WAIT_OBJECT_0 + 1:     //Exit
            break;

        case NMS_WAIT_TIMEOUT:
            refresh();
            break;
    }
};


void CPollingThread::refresh()
{
    snmpGetBoardList();
};


int main( int argc, char **argv )
{
    demoHeader();

    g_pPollingThread = (CPollingThread *)NmsBeginThread(
        NMSRUNTIME_CLASS( CPollingThread ),
        NMS_THREAD_PRIORITY_NORMAL,
        0 );

    try
    {
        THROW_SNMP_EXCEPTION( createSnmpSession( argc, argv ) );
        run();
    }
    catch( CNmsSnmpException e )
    {
        e.ReportError();
    }


    if( g_pPollingThread )
    {
        TSITHREADHDL hThread = g_pPollingThread->m_hThread;
        g_pPollingThread->EndThread();
        tsiWaitForDestroyThread( hThread, INFINITE );
    }
    if( g_pSnmp )
        delete g_pSnmp;
return 1;
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


void run()
{
int nCommand = CM_SNMP_NO_COMMAND;
    if( snmpGetSysInfo() == SNMP_CLASS_SUCCESS )
        snmpGetBoardList();
int nBoard;
    do
    {
        printf("> ");
        nCommand = processCommand( nBoard );
        bCommand = TRUE;
        switch( nCommand )
        {
            case CM_SNMP_SYSINFO:
                snmpGetSysInfo();
                break;
            case CM_SNMP_BOARDLIST:
                snmpGetBoardList();
                break;
            case CM_SNMP_QUIT:
                break;
            case CM_SNMP_HELP:
                printHelp();
                break;
            case CM_SNMP_NO_COMMAND:
                break;
            case CM_SNMP_SETPOLLINTERVAL:
                setPollInterval( nBoard );
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

    printf("SEND A REQUEST FOR SYSTEM INFO TO: %s\n", g_pSnmp->GetPrintableAddress());

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


int snmpGetBoardList()
{
int nStatus = SNMP_CLASS_SUCCESS;
    printf("SEND A REQUEST FOR NMS BOARDS TO: %s\n", g_pSnmp->GetPrintableAddress() );
    try
    {
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

            printf("              %s bus\n", SnmpBusTypeToStr( (SnmpBusType)nBusType ) );
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

};


void demoHeader()
{
    printf("SNMP Demonstration and Test Program  V. %s %s\n",  VERSION_NUM , VERSION_DATE);
    printf("NMS Communications Corporation.\n\n");
};

void printHelp()
{
    printf("Usage:\n");
    printf("snmpChassScan [Address | DNSName] [options]\n");
    printf("Address:  default is 127.0.0.1\n");
    printf("options: -cCommunity_name, specify community default is 'public'\n");
    printf("         -pPort, specify SNMP port default is 161 \n");
    printf("         -rN , retries default is N = 1 retry\n");
    printf("         -tN , timeout in hundredths-seconds default is N = 100 = 1 second\n\n");
    printUsage();
}

void printUsage()
{
    printf("H Help  S Sys info  L Board list  P<N> Poll Interval  Q Quit\n\n");
};


void setPollInterval( int nInterval )
{
    g_pPollingThread->SetPollingInterval( nInterval );
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
                nCommand = CM_SNMP_QUIT;
                break;
            case 'H':
                nCommand = CM_SNMP_HELP;
                break;
            case 'S':
                nCommand = CM_SNMP_SYSINFO;
                break;
            case 'L':
                nCommand = CM_SNMP_BOARDLIST;
                break;
            case 'P':
                {
                    nCommand = CM_SNMP_SETPOLLINTERVAL;
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
