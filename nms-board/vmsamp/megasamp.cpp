/**********************************************************************
*  File - megasamp.cpp
*
*  Description - Video Mail sample application
*
*  2005  NMS Communications Corporation.
*
***********************************************************************/


///////////////////
// Include Files //
///////////////////
#include <pthread.h>

#include "nmstypes.h"

#include "mega_global.h"
#include "gw_types.h"
#include "srv_types.h"   /* tTimsSlot  */
#include "srv_def.h"     /* initServer */
#include "vm_stat.h"     /* initVmPortStat */
#include "vm_trace.h"

///////////////////////////
// GLOBAL DEFINES
///////////////////////////
CMD_LINE_PARAMS         CmdLineParms;   
MEDIA_GATEWAY_CONFIG    GwConfig[MAX_GATEWAYS];
ISDN_CX                 isdnCX[MAX_NAI];

FILE                    *RecCmdFp = NULL;
FILE                    *PlayCmdFp = NULL;
FILE                    *RecH245Fp = NULL;
int                     optind = 1;
int                     opterr = 1;
char                    *optarg;

CTAQUEUEHD              sysQueueHd;
CTAHD                   sysCTAccessHd;

extern int gExiting;

FILE                     *DpfDebugFp;

static pthread_t ThreadCommandLoop( int *pPort );
static void *systemMonitor( void *param );
static void *TestStatistics( void *param );
static void printLoadTestMenu();

int main( int argc, char *argv[] )
{
    DWORD i, ret;
	int port_num[ MAX_GATEWAYS ];
	pthread_t tid[ MAX_GATEWAYS ];
	char key;

    // Zero the GwConfig Structure
    memset (GwConfig, 0, sizeof (MEDIA_GATEWAY_CONFIG ));

    // Get the Command Line Parameters 
    ret = initGetCmdLineParms( argc, argv );
    if( ret == FAILURE )
       exit( 1 );
	   
    //Set up system flow display
    t_FlowObj FlowObjArray[] = {
        { F_NCC,    " NCC " },
        { F_H324,   "H324 " },
        { F_APPL,   "APPL " },                
        { F_TRC,    " TRC " }};
    
    if (VmTraceMask != 0)
    {
      FlowInit( 72, 4, FlowObjArray, "CallFlow.log");
      h324SetTrace( T_ALLERR, 0xFFFFFFFF );
    }
    else 	   
      h324SetTrace( T_ALLERR, 0 );
//	h324SetTrace( 0xFFFFFFFF - T_MUTEX - T_H245CALLB, 0);
           
    //If a configuration file for the server is provided, read in the configuration
	ret = initGatewayConfigFile( CmdLineParms.GwConfigFileName );
    if( ret == FAILURE )
       exit( 1 );   
    	   
    // Initialize the Gateway contexts, endpoints and channels based on the 
    // defined configuration
    ret = initGateway();
    if( ret == FAILURE )
       exit( 1 );

    //If a configuration file for the server is provided, read in the configuration
	ret = initServerConfigFile( CmdLineParms.SrvConfigFileName );
    if( ret == FAILURE )
       exit( 1 );

    // Initialize the Server contexts, endpoints and channels based on the 
    // defined configuration
    ret = initServer();
    if( ret == FAILURE )
       exit( 1 );

	// Zero all statistic counters
	initVmPortStat( gwParm.nPorts );

    // Launch System Monitor thread
    ret = pthread_create(NULL, NULL, systemMonitor, NULL);	    
    usleep(100*1000); //100ms sleep to allow system monitor thread to start    
    
    // Main Loop which waits for commands or events
	if( gwParm.nPorts == 1 ) {
		port_num[0] = 0;
		CommandLoop( &port_num[0] );
	} else {
		ret = pthread_create(NULL, NULL, TestStatistics, NULL);	
	
		for( i=0; i<gwParm.nPorts; i++ ) {
			// Launch a thread per channel
			port_num[i] = i;
			tid[i] = ThreadCommandLoop( &port_num[i] );
//			sleep(1);
		}
		
		printLoadTestMenu();
				
		// Wait untill all the threads will exit
		for( i=0; i<gwParm.nPorts; i++ ) {
			pthread_join(tid[i], NULL);
		}
	}

    shutdown();

#ifdef USE_TRC
    if( gwParm.useVideoXc ) {
        trcShutdown();
    }
#endif    
	
	// Shutdwon the Server's part of the system
	serverShutdown();

    return SUCCESS;
}


pthread_t ThreadCommandLoop( int *pPort )
{
	int ret;
	pthread_t tid;
	
	ret = pthread_create(&tid, NULL, CommandLoop, (void *)pPort);
	if (ret != SUCCESS)
	{
		printf("ThreadCommandLoop: pthread_create returned %d\n",ret);
		exit(1);
	}
	
	return tid;
}

void *TestStatistics( void *param )
{
	char key;
	
	while(1) {
		key = getchar();
		
		if( key == 'p' )
			printVmStat();
	}
    
    return NULL;
}

void printLoadTestMenu()
{
	printf("\t=====================================================================\n");
	printf("\t      LOAD TEST COMMANDS MENU                                        \n");
	printf("\t      ===============================================================\n\n");
	printf("\t  p)  Print statistics                                               \n");
	printf("\t=====================================================================\n");	
}

/************************* System Monitor ****************************/

#ifdef USE_TRC
void vtp_failure_to_string( int *pReason, char *pReasonStr )
{
    switch( *pReason ) {
        case TRCERR_VTP_UNREACHABLE:
            strcpy( pReasonStr, "TRCERR_VTP_UNREACHABLE" );
            break;
        case TRCERR_VTP_APP_PROBLEM:
            strcpy( pReasonStr, "TRCERR_VTP_APP_PROBLEM" );
            break;
        default:
            strcpy( pReasonStr, "REASON_UNKNOWN" );
            break;               
    }
}
#endif // USE_TRC

DWORD handleSystemEvents( CTA_EVENT *pEvent )
{
    char strReason[100];
    int reason;
    
    switch( pEvent->id ) {
#ifdef USE_TRC	
        case TRCEVN_VTP_FAILURE:
            reason = *((int *)(pEvent->buffer));
            vtp_failure_to_string( &reason, strReason );
            mmPrintAll("systemMonitor: TRCEVN_VTP_FAILURE event received. VTP ID=%d, reason=%s\n", 
                    pEvent->value, strReason );            
                    
            if (reason == TRCERR_VTP_APP_PROBLEM)
            {
                DWORD ret;
                mmPrintAll("systemMonitor: trcResetVTP <%d> \n", pEvent->value);
                ret = trcResetVTP( pEvent->value );
                mmPrintAll("systemMonitor: trcResetVTP ret=%d\n", ret);
                break;  
            }   
            break;
        case TRCEVN_VTP_RECOVERED:
            mmPrintAll("systemMonitor: TRCEVN_VTP_RECOVERED event received. VTP ID=%d\n", 
                    pEvent->value );                        
            break;
#endif // USE_TRC
			
        default:
            break;
    }

    if(pEvent->buffer != NULL) {
        ctaFreeBuffer( pEvent->buffer );
    }        
    
    return SUCCESS;    
}

void *systemMonitor( void *param )
{
    DWORD ret;
    CTA_EVENT event;
    
	mmPrint(-1, T_GWAPP, "System Monitor thread started\n");    
        
    ret = ctaCreateQueue(NULL, 0, &sysQueueHd);
    if( ret != SUCCESS ) {
       	mmPrintErr("Failed to create a System Monitor CTA queue, ret = %x", ret);
	    exit( 1 );
   	}      
	mmPrint(-1, T_GWAPP, "System Monitor CT Access queue for events created\n");

	ret = ctaCreateContext( sysQueueHd, 0, "System Monitor thread", &sysCTAccessHd );
    if( ret != SUCCESS ) {
		mmPrintErr("Failed to create System Monitor CT Access context, invalid queue handle %x.\n",
            sysQueueHd );
		exit( 1 );
	}
	mmPrint(-1, T_GWAPP, "System Monitor CT Access context created, CtaHd = 0x%x\n",sysCTAccessHd );    
        
	// Main Loop
	while ( gExiting == FALSE ) {
		memset( &event, 0, sizeof (CTA_EVENT));
        ret = ctaWaitEvent( sysQueueHd, &event, CTA_WAIT_FOREVER ); 
        if (ret != SUCCESS) {
			mmPrintErr("ctaWaitEvent failure, ret= 0x%x, event.value= 0x%x, board error (event.size field) %x",
				 ret, event.value, event.size);
            exit(1);
        }
        
        handleSystemEvents( &event );        
    } // End while       
    
	mmPrint(-1, T_GWAPP, "Exiting System Monitor thread\n");    
    return NULL;    
}

