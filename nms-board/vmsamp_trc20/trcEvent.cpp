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

extern CTAHD       sysCTAccessHd;
extern CTAQUEUEHD  sysQueueHd;


int ProcessTrcEvent( CTA_EVENT *pevent, DWORD nGw )
{
	MEDIA_GATEWAY_CONFIG *pCfg;
	DWORD ret;	
	tTrcChInfo trcChInfo;
       
	// Get a pointer to the gateway's configuration
	pCfg = &GwConfig[nGw];    
    
    switch( pevent->id ) {

		case TRCEVN_CREATE_CHANNEL_DONE:
            mmPrint(nGw, T_GWAPP, "Event: received TRCEVN_CREATE_CHANNEL_DONE, value=%d\n",
                     pevent->value);
            Flow(nGw, F_TRC, F_APPL, "TRCEVN_CREATE_CHANNEL_DONE");            
			if( pevent->value == TRCERR_NORESOURCES )
			{
				mmPrintPortErr(nGw, "TRCEVN_CREATE_CHANNEL_DONE failed - No resources\n");
				mmPrint(nGw, T_GWAPP, "Destroying the channel...\n");
				// Destroy the channel
				ret = trcDestroyVideoChannel( pCfg->VideoXC.trcChHandle );
	    		if( ret != TRC_SUCCESS )
                    mmPrintPortErr( nGw, "trcDestroyVideoChannel failed. rc = 0x%x\n, [%s]",
								    ret, trcValueName(TRCVALUE_RESULT,ret) );  
                              
                Flow(nGw, F_APPL, F_TRC, "trcDestroyVideoChannel");
	            change_xc_state( nGw, XCSTATE_DESTROYING );				
			}
			
			else
			{
				// Obtain detailed information about the allocated channel
				ret = trcInfoVideoChannel( pCfg->VideoXC.trcChHandle, &trcChInfo );
				Flow(nGw, F_APPL, F_TRC, "trcInfoVideoChannel");             
				if( ret != SUCCESS )
				{
                    mmPrintPortErr( nGw, "trcInfoVideoChannel failed. rc = 0x%x\n, [%s]",
								    ret, trcValueName(TRCVALUE_RESULT,ret) );  				
				}
				else
				{
					// Initialize the channel parameters
					strcpy( pCfg->VideoXC.trcRes.ipAddr, trcChInfo.res.ipAddr );
					pCfg->VideoXC.trcRes.rxPort[TRC_DIR_SIMPLEX] = trcChInfo.res.rxPort[TRC_DIR_SIMPLEX];
					pCfg->VideoXC.vtpId = trcChInfo.summary.vtpId;

				}
			}

			break;
	

        case TRCEVN_START_CHANNEL_DONE:   
            mmPrint(nGw, T_GWAPP, "Event: received TRCEVN_START_CHANNEL_DONE, value=%d\n",
                     pevent->value);
            Flow(nGw, F_TRC, F_APPL, "TRCEVN_START_CHANNEL_DONE");   
			change_xc_state( nGw, XCSTATE_STARTED );         
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

    		ret = trcDestroyVideoChannel( pCfg->VideoXC.trcChHandle );
	    	if( ret != TRC_SUCCESS )
                    mmPrintPortErr( nGw, "trcDestroyVideoChannel failed. rc = %d\n", ret);
            Flow(nGw, F_APPL, F_TRC, "trcDestroyVideoChannel");             
          
            change_xc_state( nGw, XCSTATE_DESTROYING );
            
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
        case TRCEVN_IFRAME_CHANNEL_DONE:   
            mmPrint(nGw, T_GWAPP, "Event: received TRCEVN_IFRAME_CHANNEL_DONE\n");
            Flow(nGw, F_TRC, F_APPL, "TRCEVN_IFRAME_CHANNEL_DONE");            
            break;

        case TRCEVN_CHANNEL_LOST:  
            mmPrint(nGw, T_GWAPP, "Event: received TRCEVN_CHANNEL_LOST\n");
            Flow(nGw, F_TRC, F_APPL, "TRCEVN_CHANNEL_LOST");            
			break;

        case TRCEVN_CHANNEL_FAILED:                    
            mmPrintPortErr(nGw, "Event: received TRCEVN_CHANNEL_FAILED.\n");		
            Flow(nGw, F_TRC, F_APPL, "TRCEVN_CHANNEL_FAILURE"); 
                        
            mmPrint(nGw, T_GWAPP, "Destroying TRC channel because of TRCEVN_CHANNEL_FAILURE\n");
  			ret = trcDestroyVideoChannel( pCfg->VideoXC.trcChHandle );
    		if( ret != TRC_SUCCESS )
	            mmPrintPortErr( nGw, "trcDestroyVideoChannel failed. rc = %d\n", ret);  
                              
            Flow(nGw, F_APPL, F_TRC, "trcDestroyVideoChannel");
			change_xc_state( nGw, XCSTATE_DESTROYING );
            break;
        
		case TRCEVN_DESTROY_CHANNEL_DONE:
            mmPrintPortErr(nGw, "Event: received TRCEVN_DESTROY_CHANNEL_DONE.\n");		
            Flow(nGw, F_TRC, F_APPL, "TRCEVN_DESTROY_CHANNEL_DONE"); 
			change_xc_state( nGw, XCSTATE_NONE );		
			break;
		
        default:
            break;    
    }
    
    return SUCCESS;
}

U32 trcCallback(tTrcMessage *pMsg, S32 size)
{
    CTA_EVENT NewEvent = { 0 };  
    int nGw, i;       
	VIDEO_XC *pVideoXc;
	
    S8 *eventName = trcValueName(TRCVALUE_EVENT, pMsg->event);
    S8 *resultName = trcValueName(TRCVALUE_RESULT, pMsg->result);   	
    mmPrint( -1, T_GWAPP, "trcCallback. TRC Event [%s]: result [%s]\n",eventName, resultName);

	NewEvent.id     = pMsg->event;
	NewEvent.value  = pMsg->result;  
	
    if( pMsg->event == TRCEVN_RESOURCE_CHANGE || 
		pMsg->event == TRCEVN_SHUTDOWN_DONE ) {
        NewEvent.size   = 0;
        NewEvent.ctahd  = sysCTAccessHd;
        NewEvent.buffer = NULL;
		
        ctaQueueEvent(&NewEvent);    
        return 0;
    }           

	pVideoXc = (VIDEO_XC *)pMsg->userKey;
            
    nGw = -1;
	for (i=0; i<gwParm.nPorts ; i++) {
        if( pVideoXc == &GwConfig[i].VideoXC ) {
            nGw = i;
            break;
        }
    }    
    
    if( nGw == -1 ) {
        mmPrintErr("trcCallback: Invalid message from the TRC. user key =  0x%x\n", pMsg->userKey );
        return -1;
    }

	// Protect the application from the useless events
	if( (pVideoXc->state == XCSTATE_DESTROYING) && (pMsg->event != TRCEVN_DESTROY_CHANNEL_DONE) )
	{
	    mmPrint( nGw, T_GWAPP, "trcCallback. state=XCSTATE_DESTROYING. Ignote this event [%s]\n",eventName);	
		return 0;
	}

	if( pVideoXc->state == XCSTATE_NONE )
	{
	    mmPrint( nGw, T_GWAPP, "trcCallback. state=XCSTATE_NONE. Ignote this event [%s]\n",eventName);	
		return 0;
	}
   
    NewEvent.size   = 0;
    NewEvent.ctahd  = GwConfig[nGw].MuxEp.ctahd;
    NewEvent.buffer = NULL;
    
	DWORD rc;
    rc = ctaQueueEvent(&NewEvent);
	       
    return 0;
}

int initTrcConfig( int nGw )
{
	// Configure the Video Transcoder
    GwConfig[nGw].VideoXC.bActive       = TRUE;       
    GwConfig[nGw].VideoXC.bFVU_Enabled  = TRUE;
    sprintf( GwConfig[nGw].VideoXC.trcRes.ipAddr , gwParm.destIPAddr_video );
	GwConfig[nGw].VideoXC.state    = XCSTATE_NONE;
       
	tTrcChConfig *pChConfig = &GwConfig[nGw].VideoXC.trcChConfig;
	memset( pChConfig, TRC_CONFIG_DEFAULT, sizeof(tTrcChConfig) );        /* start from all defaults  */

	pChConfig->endpointA.vidType = TRC_VIDTYPE_H263;
	pChConfig->endpointA.profile = TRC_PROFILE_BASELINE;
	pChConfig->endpointA.level = TRC_H263_LEVEL_10;
	pChConfig->endpointA.dataRate  = 43;                           /* kbits/sec                    */
	pChConfig->endpointA.frameRate = 7;                            /* frames/sec                   */
	pChConfig->endpointA.frameRes  = TRC_FRAME_RES_QCIF;
	pChConfig->endpointA.packetizeMode = TRC_PACKETIZE_2190;
	pChConfig->endpointA.chanIn.jitterMode = TRC_JITTER_NONE;    /* don't use jitter             */
	pChConfig->endpointA.chanIn.jitterLatency = 0;               /* milliseconds                 */
		
	pChConfig->endpointB.vidType = TRC_VIDTYPE_MPEG4;
	pChConfig->endpointB.profile = TRC_PROFILE_SIMPLE;
	pChConfig->endpointB.level = TRC_MPEG4_LEVEL_0;
	pChConfig->endpointB.dataRate  = 43; /* kbits/sec */
	pChConfig->endpointB.frameRate = 7; /* frames/sec */
	pChConfig->endpointB.frameRes  = TRC_FRAME_RES_QCIF;	
	pChConfig->endpointB.packetizeMode = TRC_PACKETIZE_3016;
	strcpy( pChConfig->endpointB.chanOut.ipAddr, GwConfig[nGw].vidRtpEp[EP_OUTB].Addr.EP.RtpRtcp.SrcIpAddress );
	pChConfig->endpointB.chanOut.rtpPort = GwConfig[nGw].vidRtpEp[EP_OUTB].Addr.EP.RtpRtcp.nSrcPort;
	pChConfig->endpointB.chanOut.payloadId = 100;
	pChConfig->endpointB.chanOut.tos = 0;


	/* configure the MPEG-4 decoder that is transcoding from MPEG-4 endpoint A
	to H.263 endpoint B                                                                     */

/*
	pChConfig->decoder[TRC_DIR_FDX1].optData = mpeg4DecoderCfg;
	pChConfig->decoder[TRC_DIR_FDX1].optSize = sizeof(mpeg4DecoderCfg);
*/

	return 0;         
}

int initTrc()
{
	DWORD ret;
	CTA_EVENT event = { 0 };

	// Set up TRC logging mask
    trcSetTrace( gwParm.trcTraceMask, gwParm.trcFileTraceMask );
        
    /* Initialize TRC library */        
	ret = trcInitialize( TRC_CTL_VERSION, TRC_CTL_REVISION, "video_mail",
						 "trc.cfg", "trc.log", trcCallback );
    Flow(0, F_APPL, F_TRC, "trcInitialize");
    if( ret != TRC_SUCCESS ) {
    	mmPrintErr("trcInitialize failed. Return code is %d\n", ret);
   		exit(1);
    }
	
	ret = WaitForSpecificEvent(-1, sysQueueHd, TRCEVN_RESOURCE_CHANGE, &event );
	if (ret != SUCCESS)
	{
		mmPrintErr("Close services failed for Context %d...event.value=0x%x\n", 
					   sysCTAccessHd, event.value);     
		return ret;
	}
	mmPrint(-1, T_CTA, "TRCEVN_RESOURCE_CHANGE received - can start creating TRC channels...\n");
	
	return ret;
}

char *lookup_xc_state_name(int state)
{
    switch(state)
    {
    case XCSTATE_NONE:       return "XCSTATE_NONE";
    case XCSTATE_STOPPED:    return "XCSTATE_STOPPED";
    case XCSTATE_STARTING:   return "XCSTATE_STARTING";
    case XCSTATE_STARTED:    return "XCSTATE_STARTED";
    case XCSTATE_STOPPING:   return "XCSTATE_STOPPING";
    case XCSTATE_DESTROYING: return "XCSTATE_DESTROYING";
    case XCSTATE_CREATING:   return "XCSTATE_CREATING";
    case XCSTATE_CREATED:    return "XCSTATE_CREATED";
	
    default:                 return "UNKNOWN";
    }
}     

void change_xc_state(int nGw, int new_state)
{
    mmPrint(nGw, T_XC, "Transcoder state change: %s(%d) ---> %s(%d)\n",
            lookup_xc_state_name(GwConfig[nGw].VideoXC.state), GwConfig[nGw].VideoXC.state,
            lookup_xc_state_name(new_state), new_state);
    GwConfig[nGw].VideoXC.state = new_state;
}
