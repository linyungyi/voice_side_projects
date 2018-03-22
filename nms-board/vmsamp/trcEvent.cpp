/***********************************************************************
*  File - trcEvent.cpp
*
*  Description - The functions in this file process inbound TRC events
*
*  2005  NMS Communications Corporation.
*
***********************************************************************/

///////////////////
// Include Files //
///////////////////
#include "mega_global.h"
#include "gw_types.h"
#include "vm_trace.h"
#include "trcapi.h"


int ProcessTrcEvent( CTA_EVENT *pevent, DWORD nGw )
{
	MEDIA_GATEWAY_CONFIG *pCfg;
	DWORD ret;		    
       
	// Get a pointer to the gateway's configuration
	pCfg = &GwConfig[nGw];    
    
    switch( pevent->id ) {
        case TRCEVN_START_CHANNEL_DONE:   
            H324_MEDIA_CAPABILITY *pMP4Cap;
            mmPrint(nGw, T_GWAPP, "Event: received TRCEVN_START_CHANNEL_DONE, value=%d\n",
                     pevent->value);
            Flow(nGw, F_TRC, F_APPL, "TRCEVN_START_CHANNEL_DONE");            
            break;
        case TRCEVN_STOP_CHANNEL_DONE:   
            mmPrint(nGw, T_GWAPP, "Event: received TRCEVN_STOP_CHANNEL_DONE, value=%d. Destroying TRC channel...\n",
                     pevent->value);
            Flow(nGw, F_TRC, F_APPL, "TRCEVN_STOP_CHANNEL_DONE");                         

    		ret = trcDestroyVideoChannel( pCfg->VideoXC.xcChanId );
	    	if( ret != TRC_SUCCESS )
                    mmPrintPortErr( nGw, "trcDestroyVideoChannel failed. rc = %d\n", ret);
            Flow(nGw, F_APPL, F_TRC, "trcDestroyVideoChannel");             
          
            pCfg->VideoXC.xcChanId = -1;
            
   			if( gwParm.auto_place_call ) {
           	    // Starts the timer to place the next call
        		if( adiStartTimer( pevent->ctahd, gwParm.between_calls_time, 1 ) != SUCCESS ) { 
   	   	    		mmPrintPortErr(nGw, "Failed to start the ADI timer.\n");
                } else {
       	        	mmPrint( nGw, T_GWAPP, "adiStartTimer started.\n");
                }
	       	}				
            
            break;
        case TRCEVN_CONFIG_CHANNEL_DONE:   
            mmPrint(nGw, T_GWAPP, "Event: received TRCEVN_CONFIG_CHANNEL_DONE\n");
            Flow(nGw, F_TRC, F_APPL, "TRCEVN_CONFIG_CHANNEL_DONE");            
            break;
        case TRCEVN_CHANNEL_FAILURE:  
            char failureSevStr[ 50 ];
            switch( pevent->value ) {
                case TRCERR_CHANNEL_FAIL_CRITICAL:
                    strcpy( failureSevStr, "TRCERR_CHANNEL_FAIL_CRITICAL" );
                    break;
                case TRCERR_CHANNEL_FAIL_RECOVERABLE:
                    strcpy( failureSevStr, "TRCERR_CHANNEL_FAIL_RECOVERABLE" );
                    break;
                default:
                    strcpy( failureSevStr, "UNKNOWN" );
                    break;                
            } // End switch                   
            mmPrintPortErr(nGw, "Event: received TRCEVN_CHANNEL_FAILURE. ChanId=0x%x, Severity: %s.\n",
                    pCfg->VideoXC.xcChanId, failureSevStr);       
			
            Flow(nGw, F_TRC, F_APPL, "TRCEVN_CHANNEL_FAILURE"); 
                        
            if( pevent->value == TRCERR_CHANNEL_FAIL_CRITICAL ) {
                mmPrint(nGw, T_GWAPP, "Destroying TRC channel because of TRCEVN_CHANNEL_FAILURE\n");
    			ret = trcDestroyVideoChannel( pCfg->VideoXC.xcChanId );
	    		if( ret != TRC_SUCCESS )
                    mmPrintPortErr( nGw, "trcDestroyVideoChannel failed. rc = %d\n", ret);  
                              
                Flow(nGw, F_APPL, F_TRC, "trcDestroyVideoChannel");
            }
			
			// Restart the Video channel 
            if( pevent->value == TRCERR_CHANNEL_FAIL_RECOVERABLE  ) {
                mmPrint(nGw, T_GWAPP, "Re-start Video Channel <%d>. Rx1=%d, Dest1=%d, DestIP1=%s\n",
                        pCfg->VideoXC.xcChanId,
                        pCfg->VideoXC.xcChanConfig.params1.RxPort,
                        pCfg->VideoXC.xcChanConfig.params1.DestPort,
                        pCfg->VideoXC.xcChanConfig.params1.DestIP);            
   		    
    		    ret = trcStartVideoChannel( pCfg->VideoXC.xcChanId, &pCfg->VideoXC.xcChanConfig );
                Flow(nGw, F_APPL, F_TRC, "trcStartVideoChannel");                
    		    if( ret != TRC_SUCCESS ) {
                    mmPrintPortErr( nGw, "trcStartVideoChannel failed to start the channel. rc = %d\n", ret);
                    return FAILURE;
                }   
			
			}
						
            break;
        
        default:
            break;    
    }
    
    return SUCCESS;
}
