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

extern CTAHD  sysCTAccessHd;

int ProcessTrcEvent( CTA_EVENT *pevent, DWORD nGw )
{
	MEDIA_GATEWAY_CONFIG *pCfg;
	DWORD ret;		    
       
	// Get a pointer to the gateway's configuration
	pCfg = &GwConfig[nGw];    
    
    switch( pevent->id ) {
        case TRCEVN_START_CHANNEL_DONE:   
            mmPrint(nGw, T_GWAPP, "Event: received TRCEVN_START_CHANNEL_DONE, value=%d\n",
                     pevent->value);
            Flow(nGw, F_TRC, F_APPL, "TRCEVN_START_CHANNEL_DONE");            
            break;
        case TRCEVN_STOP_CHANNEL_DONE: 
        // dynamic VTP channel
	    if(pCfg->bVideoChanRestart==1) {    //bRestart  
                mmPrint(nGw, T_GWAPP, "Event: received TRCEVN_STOP_CHANNEL_DONE, value=%d. And Restarting Channel TRC channel...\n",
                     pevent->value);
                Flow(nGw, F_TRC, F_APPL, "TRCEVN_STOP_CHANNEL_DONE");  
                pCfg->bVideoChanRestart=0; 
    	    }else{
    	              
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
            }  //end of if bRestart
       
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

int trcCallback(tTrcMessage *pMsg, int size)
{
    CTA_EVENT NewEvent = { 0 };  
    int nGw, i;       
    
    if( pMsg->code == TRCEVN_VTP_FAILURE ) {
        NewEvent.id     = pMsg->code;
        NewEvent.value  = pMsg->vtpId;
        NewEvent.size   = sizeof(int);
        NewEvent.ctahd  = sysCTAccessHd;
    
        /* Specify the reason for the VTP failure */
    	ctaAllocBuffer( &(NewEvent.buffer), sizeof(int) );        
        memcpy( NewEvent.buffer, &pMsg->data, sizeof(int) );
        
        ctaQueueEvent(&NewEvent);    
        return 0;
    } else if( pMsg->code == TRCEVN_VTP_RECOVERED ) {
        NewEvent.id     = pMsg->code;
        NewEvent.value  = pMsg->vtpId;
        NewEvent.size   = 0;
        NewEvent.ctahd  = sysCTAccessHd;
        NewEvent.buffer = NULL;
        
        ctaQueueEvent(&NewEvent);    
        return 0;        
    }           
            
    nGw = -1;
	for (i=0; i<gwParm.nPorts ; i++) {
        if( pMsg->channelId == GwConfig[i].VideoXC.xcChanId ) {
            nGw = i;
            break;
        }
    }    
    
    if( nGw == -1 ) {
        mmPrintErr("trcCallback: Invalid message from the TRC. channel ID = %d\n", pMsg->channelId );
        return -1;
    }
    
    NewEvent.id     = pMsg->code;
    NewEvent.value  = pMsg->data;
    NewEvent.size   = 0;
    NewEvent.ctahd  = GwConfig[nGw].MuxEp.ctahd;
    NewEvent.buffer = NULL;
    
    ctaQueueEvent(&NewEvent);
        
    return 0;
}

int initTrcConfig( int nGw )
{
	// Configure the Video Transcoder
    GwConfig[nGw].VideoXC.bActive  = TRUE;
        
    GwConfig[nGw].VideoXC.bFVU_Enabled                = TRUE;
    sprintf( GwConfig[nGw].VideoXC.ipAddr,            gwParm.destIPAddr_video );
    GwConfig[nGw].VideoXC.xcChanId = -1;
//  GwConfig[nGw].VideoXC.state    = XCSTATE_NONE;
        
    tChannelConfig *pChConfig = &GwConfig[nGw].VideoXC.xcChanConfig;           
    memset( pChConfig, 0, sizeof (tChannelConfig)); 
               
    pChConfig->params1.VidType  = 2;  // H263
    pChConfig->params2.VidType  = 0;  // MPEG4
    pChConfig->params1.RxPort   = 0;  // Will be defined after trcCreateVideoChannel
    pChConfig->params1.DestPort = GwConfig[nGw].vidRtpEp[EP_OUTB].Addr.EP.RtpRtcp.nSrcPort; 
    strcpy(pChConfig->params1.DestIP, GwConfig[nGw].vidRtpEp[EP_OUTB].Addr.EP.RtpRtcp.SrcIpAddress);
    pChConfig->params1.outDataRate       = 43; // kbps
    pChConfig->params1.outFrameRate      = 9;  // frame/sec
    pChConfig->params1.outPayloadID      = 100;
    pChConfig->params1.jitterCfg.mode    = 3;                                                                                                        
    pChConfig->params1.jitterCfg.latency = 0;         

	return 0;
}

int initTrc()
{
	DWORD ret;
	
    // Set up TRC logging mask
    trcSetTrace( gwParm.trcTraceMask, gwParm.trcFileTraceMask );
        
    /* Initialize TRC library */               
    ret = trcInitialize( gwParm.sTrcConfigFile, gwParm.sTrcLogFile, trcCallback );
    Flow(0, F_APPL, F_TRC, "trcInitialize");
    if( ret != TRC_SUCCESS ) {
	   	mmPrintErr("trcInitialize failed. Return code is %d\n", ret);
	    if( ret != TRCERR_COMM_PROBLEM )
    		exit(1);
    }

	return 0;	
}
