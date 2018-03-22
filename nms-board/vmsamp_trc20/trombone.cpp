/*********************************************************************************
  trombone.cpp

  This file contains functions related to trombone feature commmands.
      ProcessTromboneEvent
      switchGWVideoDestination
      switchGWAudioDestination
      LookForFreePartner
      switchWithoutDisablingGWAudioDestination
      stopGwVideoRx

**********************************************************************************/

///////////////////
// Include Files //
///////////////////
#include <sys/types.h>
#if defined(WIN32)
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif


#include "mega_global.h"
#include "command.h"
#include "gw_types.h"
#include "vm_trace.h"

#include "srv_types.h"
#include "srv_def.h"
#include "mmfi.h"

int   getFileFormat( char *fileName );
DWORD PlayFromPlayList( int nGw, char *fname );

DWORD disableVideoEP( int nGw, MSPHD hd, DWORD index );
DWORD enableVideoEP( int nGw, MSPHD hd, DWORD index );

DWORD disableAudioEP( int nGw, MSPHD hd );
DWORD enableAudioEP( int nGw, MSPHD hd );

DWORD switchGWVideoDestination(DWORD gwIndex, DWORD destPort, char *destIPAddr, WORD newState);
DWORD switchGWAudioDestination(DWORD gwIndex, DWORD destPort, char *destIPAddr);
DWORD LookForFreePartner (DWORD gwIndex);
DWORD switchWithoutDisablingGWAudioDestination(DWORD gwIndex, DWORD destPort, char *destIPAddr);
DWORD stopGwVideoRx (DWORD gwIndex);

DWORD disableVideoChannel( int nGw, MSPHD hd );
DWORD enableVideoChannel( int nGw, MSPHD hd );

int ProcessTromboneEvent( CTA_EVENT *pevent, DWORD nGw )
{
	DWORD ret;		    
	CTA_EVENT   event;
	DWORD newPartner;
   
   ret = SUCCESS;    
 
    
    switch( pevent->id ) {
        case CDE_LOCAL_TROMBONE:   
            mmPrint( nGw, T_SRVAPP, "CDE_LOCAL_TROMBONE\n" ); 
            if (GwConfig[nGw].TromboneState == TROMBONING_STATE)
            {
               mmPrintPortErr(nGw, "Already in Tromboning state.\n");
               return FAILURE;
            }

            if ((GwConfig[GwConfig[nGw].trombonePartner].isdnCall.dCallState != CON) || 
               (GwConfig[GwConfig[nGw].trombonePartner].TromboneState == TROMBONING_STATE))
            {
               mmPrintPortErr(nGw, "Partner <%d> could not be tromboned to.\n", GwConfig[nGw].trombonePartner);
               newPartner = LookForFreePartner (nGw);
               if (newPartner != -1)
               {
                  mmPrint( nGw, T_SRVAPP, "Tromboning to new partner <%d>\n",  newPartner);
                  GwConfig[nGw].trombonePartner = newPartner;
                  GwConfig[newPartner].trombonePartner = nGw;
                  mmPrint( nGw, T_SRVAPP, "Partner <%d> to be tromboned to <%d>.\n", nGw, newPartner);
               }
               else
                  return FAILURE;                              
            }
         	//Stop recording or playing when Tromboning
            if ( (mmParm.Video_Format != NO_VIDEO) && 
               ( (VideoCtx[nGw].pr_mode_r == PR_MODE_ACTIVE1) ||
                 (VideoCtx[nGw].pr_mode_p == PR_MODE_ACTIVE1) ) )
            {
         	    cmdStopVideo( nGw );	
                WaitForSpecificEvent( nGw, GwConfig[nGw].hCtaQueueHd, ADIEVN_PLAY_DONE, &event );	        
            }         	
            
            if ( (mmParm.Audio_RTP_Format != NO_AUDIO) && 
               ( (AudioCtx[nGw].pr_mode_r == PR_MODE_ACTIVE1) ||
                 (AudioCtx[nGw].pr_mode_p == PR_MODE_ACTIVE1) ) )
            {
                cmdStopAudio( nGw );		
                WaitForSpecificEvent( nGw, GwConfig[nGw].hCtaQueueHd, ADIEVN_PLAY_DONE, &event );                    			
            }

            mmPrint( nGw, T_SRVAPP, "Disable Server Video Endpoint\n" );
            disableVideoEP( nGw, VideoCtx[nGw].rtpEp.hd, EP_OUTB );
            mmPrint( nGw, T_SRVAPP, "Disable Server Audio Endpoint\n" );
            disableAudioEP( nGw, AudioCtx[nGw].rtpEp.hd );              
            
            sendCDE(GwConfig[nGw].trombonePartner, CDE_REMOTE_TROMBONE, 0);
            break;
            
        case CDE_REMOTE_TROMBONE:   
            mmPrint( nGw, T_SRVAPP, "CDE_REMOTE_TROMBONE\n" );
            
            if (GwConfig[nGw].TromboneState == TROMBONING_STATE)
            {
               mmPrintPortErr(nGw, "Already in Tromboning state.\n");
               return FAILURE;
            }

         	//Stop recording or playing when Tromboning
            if ( (mmParm.Video_Format != NO_VIDEO) &&
               ( (VideoCtx[nGw].pr_mode_r == PR_MODE_ACTIVE1) ||
                 (VideoCtx[nGw].pr_mode_p == PR_MODE_ACTIVE1) ) )
            {
         	    cmdStopVideo( nGw );	
                WaitForSpecificEvent( nGw, GwConfig[nGw].hCtaQueueHd, ADIEVN_PLAY_DONE, &event );      	        
            }         	
            
            if ( (mmParm.Audio_RTP_Format != NO_AUDIO) &&
               ( (AudioCtx[nGw].pr_mode_r == PR_MODE_ACTIVE1) ||
                 (AudioCtx[nGw].pr_mode_p == PR_MODE_ACTIVE1) ) )
            {
                cmdStopAudio( nGw );				
                WaitForSpecificEvent( nGw, GwConfig[nGw].hCtaQueueHd, ADIEVN_PLAY_DONE, &event );
            }
             
            mmPrint( nGw, T_SRVAPP, "Disable Server Video Endpoint\n" );
            disableVideoEP( nGw, VideoCtx[nGw].rtpEp.hd, EP_OUTB );
            mmPrint( nGw, T_SRVAPP, "Disable Server Audio Endpoint\n" );
            disableAudioEP( nGw, AudioCtx[nGw].rtpEp.hd );            
                        
            sendCDE(GwConfig[nGw].trombonePartner, CDE_CONFIRM_TROMBONE, 0); 
            
            switchGWVideoDestination(nGw, VideoCtx[GwConfig[nGw].trombonePartner].rtpEp.Addr.EP.RtpRtcp.nDestPort, 
                                     VideoCtx[GwConfig[nGw].trombonePartner].rtpEp.Addr.EP.RtpRtcp.DestIpAddress, 
                                     TROMBONING_STATE);
            switchGWAudioDestination(nGw, AudioCtx[GwConfig[nGw].trombonePartner].rtpEp.Addr.EP.RtpRtcp.nDestPort, 
                                     AudioCtx[GwConfig[nGw].trombonePartner].rtpEp.Addr.EP.RtpRtcp.DestIpAddress);            
            
            GwConfig[nGw].TromboneState = TROMBONING_STATE;
            
            break;

        case CDE_CONFIRM_TROMBONE:   
            mmPrint( nGw, T_SRVAPP, "CDE_CONFIRM_TROMBONE\n" );  
            switchGWVideoDestination(nGw, VideoCtx[GwConfig[nGw].trombonePartner].rtpEp.Addr.EP.RtpRtcp.nDestPort, 
                                     VideoCtx[GwConfig[nGw].trombonePartner].rtpEp.Addr.EP.RtpRtcp.DestIpAddress, 
                                     TROMBONING_STATE);
            switchGWAudioDestination(nGw, AudioCtx[GwConfig[nGw].trombonePartner].rtpEp.Addr.EP.RtpRtcp.nDestPort, 
                                     AudioCtx[GwConfig[nGw].trombonePartner].rtpEp.Addr.EP.RtpRtcp.DestIpAddress); 
            
            GwConfig[nGw].TromboneState = TROMBONING_STATE;
            
            // Send FastVideoUpdate once both GW Endpoint are enabled
            mmPrint( nGw, T_SRVAPP, "Send Fast Video Update \n");
            h324VideoFastUpdate( GwConfig[nGw].MuxEp.hd );
            mmPrint( GwConfig[nGw].trombonePartner, T_SRVAPP, "Send Fast Video Update \n");
            h324VideoFastUpdate( GwConfig[GwConfig[nGw].trombonePartner].MuxEp.hd );            
            break;

        case CDE_STOP_VIDEO_RX:   
            mmPrint( nGw, T_SRVAPP, "CDE_STOP_VIDEO_RX\n" ); 
            stopGwVideoRx(nGw);
            sendCDE(GwConfig[nGw].trombonePartner, CDE_STOP_REMOTE_VIDEO_RX, 0);
            break;
            
        case CDE_STOP_REMOTE_VIDEO_RX:   
            mmPrint( nGw, T_SRVAPP, "CDE_STOP_REMOTE_VIDEO_RX\n" ); 
            stopGwVideoRx(nGw);
            sendCDE(GwConfig[nGw].trombonePartner, CDE_STOP_TROMBONE, 0);
            break;                        
            
        case CDE_STOP_TROMBONE:   
            mmPrint( nGw, T_SRVAPP, "CDE_STOP_TROMBONE\n" );  
            if (GwConfig[nGw].TromboneState == MESSAGING_STATE)
            {
               mmPrintPortErr(nGw, "Already in Messaging state.\n");
               return FAILURE;
            }          

            switchGWAudioDestination(nGw, GwConfig[nGw].AudRtpEp[EP_OUTB].Addr.EP.RtpRtcp.nDestPort, 
                                     GwConfig[nGw].AudRtpEp[EP_OUTB].Addr.EP.RtpRtcp.DestIpAddress);            
            switchGWVideoDestination(nGw, GwConfig[nGw].vidRtpEp[EP_OUTB].Addr.EP.RtpRtcp.nDestPort, 
                                     GwConfig[nGw].vidRtpEp[EP_OUTB].Addr.EP.RtpRtcp.DestIpAddress, 
                                     MESSAGING_STATE);

            sendCDE(GwConfig[nGw].trombonePartner, CDE_STOP_REMOTE_TROMBONE, 0);
            break;
            
        case CDE_STOP_REMOTE_TROMBONE:   
            mmPrint( nGw, T_SRVAPP, "CDE_STOP_REMOTE_TROMBONE\n" );   
            if (GwConfig[nGw].TromboneState == MESSAGING_STATE)
            {
               mmPrintPortErr(nGw, "Already in Messaging state.\n");
               return FAILURE;
            }
            
            switchGWVideoDestination(nGw, GwConfig[nGw].vidRtpEp[EP_OUTB].Addr.EP.RtpRtcp.nDestPort, 
                                     GwConfig[nGw].vidRtpEp[EP_OUTB].Addr.EP.RtpRtcp.DestIpAddress, 
                                     MESSAGING_STATE);
            switchGWAudioDestination(nGw, GwConfig[nGw].AudRtpEp[EP_OUTB].Addr.EP.RtpRtcp.nDestPort, 
                                     GwConfig[nGw].AudRtpEp[EP_OUTB].Addr.EP.RtpRtcp.DestIpAddress);
            
            sendCDE(GwConfig[nGw].trombonePartner, CDE_STOP_CONFIRM_TROMBONE, 0);
            
            GwConfig[nGw].vidRtpEp[EP_OUTB].bEnabled = 0;
            mmPrint( nGw, T_SRVAPP, "Enable Server Video Endpoint\n" );
            enableVideoEP( nGw, VideoCtx[nGw].rtpEp.hd, EP_OUTB );
            mmPrint( nGw, T_SRVAPP, "Enable Server Audio Endpoint\n" );
            enableAudioEP( nGw, AudioCtx[nGw].rtpEp.hd );
            
            GwConfig[nGw].TromboneState = MESSAGING_STATE;

      		if ( mmParm.bAutoPlay == 1 )
            {
      		    tVideo_Format fileFormat;
      			mmParm.Operation = PLAY;	
      			                           
                fileFormat = (tVideo_Format)getFileFormat( VideoCtx[nGw].threegp_FileName );	 	

      			PlayRecord( nGw, fileFormat );
      			
      		}
            else if( mmParm.bAutoPlayList == 1 )
            {
                PlayFromPlayList( nGw, mmParm.autoPlayListFileName );
            }
            
                        
            break;

        case CDE_STOP_CONFIRM_TROMBONE:   
            mmPrint( nGw, T_SRVAPP, "CDE_STOP_CONFIRM_TROMBONE\n" );  
            
            GwConfig[nGw].vidRtpEp[EP_OUTB].bEnabled = 0;
            mmPrint( nGw, T_SRVAPP, "Enable Server Video Endpoint\n" );
            enableVideoEP( nGw, VideoCtx[nGw].rtpEp.hd, EP_OUTB );
            mmPrint( nGw, T_SRVAPP, "Enable Server Audio Endpoint\n" );
            enableAudioEP( nGw, AudioCtx[nGw].rtpEp.hd );
            
            GwConfig[nGw].TromboneState = MESSAGING_STATE;
            
            if (GwConfig[nGw].isdnCall.dCallState == CON)
            {
         		if ( mmParm.bAutoPlay == 1 )
                {
         			tVideo_Format fileFormat;
         			mmParm.Operation = PLAY;	
         			                           
                    fileFormat = (tVideo_Format)getFileFormat( VideoCtx[nGw].threegp_FileName );	
                    	
         			PlayRecord( nGw, fileFormat );
         			
         	    }
                else if( mmParm.bAutoPlayList == 1 )
                {
                     PlayFromPlayList( nGw, mmParm.autoPlayListFileName );
                }
            }
            // if disconnected, then AUDIO GW Endpoint destination must be reconfigured to Messaging mode
            else
            {
               mmPrint( nGw, T_SRVAPP, "Reconfigure Audio GW Endpoint to Messaging State\n" );
               // note: the Endpoint is disabled
               switchWithoutDisablingGWAudioDestination(nGw, GwConfig[nGw].AudRtpEp[EP_OUTB].Addr.EP.RtpRtcp.nDestPort, 
                                                        GwConfig[nGw].AudRtpEp[EP_OUTB].Addr.EP.RtpRtcp.DestIpAddress);
             
            }
            break;

        case CDE_REMOTE_FAST_VIDEO_UPDATE:   
            mmPrint( nGw, T_SRVAPP, "CDE_REMOTE_FAST_VIDEO_UPDATE\n" ); 
            h324VideoFastUpdate( GwConfig[nGw].MuxEp.hd );
            break;

        case CDE_REMOTE_VIDEOTEMPORALSPATIALTRADEOFF:   
            mmPrint( nGw, T_SRVAPP, "CDE_REMOTE_VIDEOTEMPORALSPATIALTRADEOFF\n" ); 
            h324VideoTemporalSpatialTradeoffIndication(GwConfig[nGw].MuxEp.hd , pevent->value);
            break;

        case CDE_REMOTE_SKEW_INDICATION:               
            int skewType, skew;
            skewType = (pevent->value)&(H223_SKEW_MAX_MS+1) >> 12;
            skew = (pevent->value)&~(H223_SKEW_MAX_MS+1);
               
            mmPrint( nGw, T_SRVAPP, "CDE_REMOTE_SKEW_INDICATION type <%d> skew <%d>\n", skewType, skew);
             
            h324_h223SkewIndication(GwConfig[nGw].MuxEp.hd, skew, (channelSkewType)skewType);
            break;
                                    
        default:            
            break;
    }
                    
   return ret;
   
}        

DWORD LookForFreePartner (DWORD gwIndex)
{
   
   int i;
   DWORD ret=-1;
   
   for( i=0; i<gwParm.nPorts; i++ ) 
   {
      if (i != (int)gwIndex)
      {
         if ((GwConfig[i].isdnCall.dCallState == CON) && 
               (GwConfig[i].TromboneState == MESSAGING_STATE))
         {
            ret=i;
         }
      }
   }
   return ret;
}



DWORD stopGwVideoRx (DWORD gwIndex)
{
   	CTA_EVENT   event;
   	DWORD       expectedEvent,
                ret;
   	
    mmPrint( gwIndex, T_SRVAPP, "Send STOP Video RX cde to RTP Endpoint \n");      
                                             				
   	ret = mspSendCommand(GwConfig[gwIndex].vidRtpEp[EP_INB].hd,
   					     mspBuildCommand(MSP_ENDPOINT_RTPFDX_VIDEO_H263, MSP_CMD_RTPFDX_STOP_VIDEO_RX),
   						 NULL,
   						 0 );
   	//Verify correct response from mspSendCommand
   	if (ret != SUCCESS)
   	{
   		printf("\n\t ERROR: mspSendCommand returned failure.\n");
   		return FAILURE;
   	}
   	expectedEvent = (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_RTPFDX_STOP_VIDEO_RX);

   	ret = WaitForSpecificEvent( gwIndex, GwConfig[gwIndex].hCtaQueueHd, expectedEvent, &event );
   	if ( ret != 0 )
    {
   		printf("\n\tERROR: mspSendCommand failed to send valid completion event\n");
   		return FAILURE;     
   	}   

    mmPrint( gwIndex, T_SRVAPP, "Expecting Unsollicited Event  MSPEVN_VIDEO_RX_STOPPED \n");      
                                             				
   	expectedEvent = MSPEVN_VIDEO_RX_STOPPED;
   	
    ret = WaitForSpecificEvent( gwIndex, GwConfig[gwIndex].hCtaQueueHd, expectedEvent, &event );
   	if ( ret != 0 )
    {
   		printf("\n\tERROR: mspSendCommand failed to send valid completion event\n");
   		return FAILURE;     
   	}   

    return SUCCESS;   
}



DWORD switchGWVideoDestination(DWORD gwIndex, DWORD destPort, char *destIPAddr, WORD newState)
{
    msp_ENDPOINT_RTPFDX_CONFIG mCONFIG;
    msp_ENDPOINT_RTPFDX_RTP_PKTSZ_CTRL      pktSzCtrl;
	CTA_EVENT   event;
	DWORD expectedEvent, dIP, ret;

   mmPrint( gwIndex, T_SRVAPP, "Switch Video GW to IP <%s> destination port <%d>\n", destIPAddr,  destPort); 
   GwConfig[gwIndex].vidRtpEp[EP_OUTB].bEnabled = 1;
   
   ret = disableVideoEP( gwIndex, GwConfig[gwIndex].vidRtpEp[EP_OUTB].hd, EP_OUTB );
   if (ret != SUCCESS)
      return FAILURE;
   
	memset( &mCONFIG, 0xFF, sizeof( msp_ENDPOINT_RTPFDX_CONFIG ) );						
	mCONFIG.remoteUDPPort = (WORD)H2NMS_WORD(destPort); 

    dIP = inet_addr(destIPAddr);
    memcpy( &mCONFIG.remoteIPAddr, &dIP, 4);

    mmPrint( gwIndex, T_SRVAPP, "Send Address Video dest re-configuration cde to RTP Endpoint \n");        				
	ret = mspSendCommand(GwConfig[gwIndex].vidRtpEp[EP_OUTB].hd,
							mspBuildCommand(MSP_ENDPOINT_RTPFDX_VIDEO_H263, MSP_CMD_RTPFDX_CONFIG),
							&mCONFIG,
							sizeof( msp_ENDPOINT_RTPFDX_CONFIG ) );
	//Verify correct response from mspSendCommand
	if (ret != SUCCESS)
	{
		printf("\n\t ERROR: mspSendCommand returned failure.\n");
		return FAILURE;
	}
	expectedEvent = (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_RTPFDX_CONFIG);

	ret = WaitForSpecificEvent( gwIndex, GwConfig[gwIndex].hCtaQueueHd, expectedEvent, &event );
	if ( ret != 0 )
    {
		printf("\n\tERROR: mspSendCommand failed to send valid completion event\n");
		return FAILURE;     
	}   
	

    if (newState == TROMBONING_STATE)
    {
        mmPrint( gwIndex, T_SRVAPP, "Send Control RTP Packet Size <%d> Agg Threshold <%d> cde to RTP Endpoint \n",
                 gwParm.pktMaxSize, gwParm.aggregationThreshold);      
        memset(&pktSzCtrl, 0, sizeof(pktSzCtrl));
        pktSzCtrl.pktMaxSz          = H2NMS_DWORD(gwParm.pktMaxSize);
        pktSzCtrl.enableAggregation = 1;
        pktSzCtrl.aggThreshold      = H2NMS_DWORD(gwParm.aggregationThreshold);
                                             				
   	ret = mspSendCommand(GwConfig[gwIndex].vidRtpEp[EP_OUTB].hd,
   							mspBuildCommand(MSP_ENDPOINT_RTPFDX_VIDEO_H263, MSP_CMD_RTPFDX_VIDEO_RTP_PKTSZ_CTRL),
   							&pktSzCtrl,
   							 sizeof (pktSzCtrl) );
   	    //Verify correct response from mspSendCommand
   	    if (ret != SUCCESS)
   	    {
   		    printf("\n\t ERROR: mspSendCommand returned failure.\n");
   		    return FAILURE;
   	    }
   	    expectedEvent = (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_RTPFDX_VIDEO_RTP_PKTSZ_CTRL);
   	    
        ret = WaitForSpecificEvent( gwIndex, GwConfig[gwIndex].hCtaQueueHd, expectedEvent, &event );
   	    if ( ret != 0 )
        {
   		    printf("\n\tERROR: mspSendCommand failed to send valid completion event\n");
       		return FAILURE;     
   	    }   
    }
    else
    {
        mmPrint( gwIndex, T_SRVAPP, "Send Control RTP Packet Size <1440> Agg Threshold <700> cde to RTP Endpoint \n");      
        memset(&pktSzCtrl, 0, sizeof(pktSzCtrl));
        pktSzCtrl.pktMaxSz          = H2NMS_DWORD(1440);
        pktSzCtrl.enableAggregation = 1;
        pktSzCtrl.aggThreshold      = H2NMS_DWORD(700);
                                             				
   	ret = mspSendCommand(GwConfig[gwIndex].vidRtpEp[EP_OUTB].hd,
   							mspBuildCommand(MSP_ENDPOINT_RTPFDX_VIDEO_H263, MSP_CMD_RTPFDX_VIDEO_RTP_PKTSZ_CTRL),
   							&pktSzCtrl,
   							 sizeof(pktSzCtrl) );
   	    //Verify correct response from mspSendCommand
   	    if (ret != SUCCESS)
   	    {
   		    printf("\n\t ERROR: mspSendCommand returned failure.\n");
       		return FAILURE;
   	    }
       	expectedEvent = (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_RTPFDX_VIDEO_RTP_PKTSZ_CTRL);
   	
        ret = WaitForSpecificEvent( gwIndex, GwConfig[gwIndex].hCtaQueueHd, expectedEvent, &event );
       	if ( ret != 0 )
        {
   		    printf("\n\tERROR: mspSendCommand failed to send valid completion event\n");
       		return FAILURE;     
   	    }         
    }

    mmPrint( gwIndex, T_SRVAPP, "Send Discard pending partial video frames cde to RTP Endpoint \n");      
                                             				
   	ret = mspSendCommand(GwConfig[gwIndex].vidRtpEp[EP_OUTB].hd,
   							mspBuildCommand(MSP_ENDPOINT_RTPFDX_VIDEO_H263, MSP_CMD_RTPFDX_DISCARD_PENDING_PFRAMES),
   							NULL,
   							0 );
   	//Verify correct response from mspSendCommand
   	if (ret != SUCCESS)
   	{
   		printf("\n\t ERROR: mspSendCommand returned failure.\n");
   		return FAILURE;
   	}
   	expectedEvent = (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_RTPFDX_DISCARD_PENDING_PFRAMES);
   	
    ret = WaitForSpecificEvent( gwIndex, GwConfig[gwIndex].hCtaQueueHd, expectedEvent, &event );
   	if ( ret != 0 )
    {
   		printf("\n\tERROR: mspSendCommand failed to send valid completion event\n");
   		return FAILURE;     
   	}   


/*
   disableVideoChannel( gwIndex, GwConfig[gwIndex].VidChan.hd );
   enableVideoChannel( gwIndex, GwConfig[gwIndex].VidChan.hd );
*/
   
   ret = enableVideoEP( gwIndex, GwConfig[gwIndex].vidRtpEp[EP_OUTB].hd, EP_OUTB );
   if (ret != SUCCESS)
      return FAILURE;
   

	return SUCCESS;
}



DWORD switchGWAudioDestination(DWORD gwIndex, DWORD destPort, char *destIPAddr)
{
    msp_ENDPOINT_RTPFDX_CONFIG mCONFIG;
	CTA_EVENT   event;
	DWORD expectedEvent, dIP, ret;
   
    mmPrint( gwIndex, T_SRVAPP, "Switch Audio GW to IP <%s> destination port <%d>\n",
             destIPAddr,  destPort); 
   
   ret = disableAudioEP( gwIndex, GwConfig[gwIndex].AudRtpEp[EP_OUTB].hd );
   if (ret != SUCCESS)
      return FAILURE;
   
	memset( &mCONFIG, 0xFF, sizeof ( msp_ENDPOINT_RTPFDX_CONFIG ) );						
	mCONFIG.remoteUDPPort = (WORD)H2NMS_WORD(destPort); 

    dIP = inet_addr(destIPAddr);
    memcpy( &mCONFIG.remoteIPAddr, &dIP, 4);
   
   mmPrint( gwIndex, T_SRVAPP, "Send Address Audio dest re-configuration cde to RTP Endpoint \n");       				
	ret = mspSendCommand(GwConfig[gwIndex].AudRtpEp[EP_OUTB].hd,
							mspBuildCommand(MSP_ENDPOINT_RTPFDX, MSP_CMD_RTPFDX_CONFIG),
							&mCONFIG,
							sizeof( msp_ENDPOINT_RTPFDX_CONFIG ) );
	//Verify correct response from mspSendCommand
	if (ret != SUCCESS)
	{
		printf("\n\t ERROR: mspSendCommand returned failure.\n");
		return FAILURE;
	}
	expectedEvent = (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_RTPFDX_CONFIG);
	
    ret = WaitForSpecificEvent( gwIndex, GwConfig[gwIndex].hCtaQueueHd, expectedEvent, &event );
	if ( ret != 0 )
    {
		printf("\n\tERROR: mspSendCommand failed to send valid completion event\n");
		return FAILURE;     
	}   
   
    ret = enableAudioEP( gwIndex, GwConfig[gwIndex].AudRtpEp[EP_OUTB].hd );
    if (ret != SUCCESS)
    {
        return FAILURE;
    }
	return SUCCESS;
}



DWORD switchWithoutDisablingGWAudioDestination(DWORD gwIndex, DWORD destPort, char *destIPAddr)
{
    msp_ENDPOINT_RTPFDX_CONFIG mCONFIG;
	CTA_EVENT event;
	DWORD     expectedEvent, dIP, ret;
   
    mmPrint( gwIndex, T_SRVAPP, "Switch Without Disabling Audio GW to IP <%s> destination port <%d>\n",
             destIPAddr,  destPort); 
   
	memset( &mCONFIG, 0xFF, sizeof ( msp_ENDPOINT_RTPFDX_CONFIG ) );						
	mCONFIG.remoteUDPPort = (WORD)H2NMS_WORD(destPort); 

    dIP = inet_addr(destIPAddr);
    memcpy( &mCONFIG.remoteIPAddr, &dIP, 4);
   
    mmPrint( gwIndex, T_SRVAPP, "Send Address Audio dest re-configuration cde to RTP Endpoint \n");       				
	
    ret = mspSendCommand(GwConfig[gwIndex].AudRtpEp[EP_OUTB].hd,
						 mspBuildCommand(MSP_ENDPOINT_RTPFDX, MSP_CMD_RTPFDX_CONFIG),
						&mCONFIG,
						 sizeof ( msp_ENDPOINT_RTPFDX_CONFIG ) );
	//Verify correct response from mspSendCommand
	if (ret != SUCCESS)
	{
		printf("\n\t ERROR: mspSendCommand returned failure.\n");
		return FAILURE;
	}
	expectedEvent = (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_RTPFDX_CONFIG);
	
    ret = WaitForSpecificEvent( gwIndex, GwConfig[gwIndex].hCtaQueueHd, expectedEvent, &event );
	if ( ret != 0 )
    {
		printf("\n\tERROR: mspSendCommand failed to send valid completion event\n");
		return FAILURE;     
	}   
   
    ret = enableAudioEP( gwIndex, GwConfig[gwIndex].AudRtpEp[EP_OUTB].hd );
    if (ret != SUCCESS)
    {
        return FAILURE;
    }
	return SUCCESS;
}
