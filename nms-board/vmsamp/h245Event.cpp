/***********************************************************************
*  File - h245Event.cpp
*
*  Description - The functions in this file process inbound H324M events
*
*  2005  NMS Communications Corporation.
*
***********************************************************************/

///////////////////
// Include Files //
///////////////////
#include "mega_global.h"
#ifdef SOLARIS
    #include <sys/time.h>
#else
    #include "win_stub.h"
#endif

#include "gw_types.h"
#include "srv_types.h"   
#include "srv_def.h"     /* PlayRecord() */
#include "vm_trace.h"

int   initVideoPath( int nGw, DWORD videoCodec );
DWORD enableVideoChannel( int nGw, MSPHD hd );
DWORD disableVideoChannel( int nGw, MSPHD hd );
DWORD disableAudByChannel( int nGw, MSPHD hd );
DWORD destroyVideoPath( int nGw );

extern tVideo_Format convertVideoCodec2Format( DWORD VideoCodec );
extern int   getFileFormat( char *fileName );
extern void get_video_codec_string( DWORD videoCodec, char *str );
extern DWORD PlayFromPlayList( int nGw, char *fname );

char *LookupH324EventID(unsigned int EventID)
{
	switch(EventID)
	{
		case H324EVN_START_DONE                 : return "H324EVN_START_DONE";
		case H324EVN_LOCAL_TERM_CAPS            : return "H324EVN_LOCAL_TERM_CAPS";
		case H324EVN_CALL_SETUP_DONE            : return "H324EVN_CALL_SETUP_DONE";
		case H324EVN_REMOTE_CAPABILITIES        : return "H324EVN_REMOTE_CAPABILITIES";
		case H324EVN_REMOTE_OLC                 : return "H324EVN_REMOTE_OLC";
      case H324EVN_MEDIA_SETUP_DONE           : return "H324EVN_MEDIA_SETUP_DONE";
		case H324EVN_VIDEO_FAST_UPDATE          : return "H324EVN_VIDEO_FAST_UPDATE";
		case H324EVN_END_SESSION                : return "H324EVN_END_SESSION";
		case H324EVN_USER_INDICATION            : return "H324EVN_USER_INDICATION";
		case H324EVN_ROUND_TRIP_DELAY           : return "H324EVN_ROUND_TRIP_DELAY";
		case H324EVN_ROUND_TRIP_TIMEOUT         : return "H324EVN_ROUND_TRIP_TIMEOUT";
		case H324EVN_STOP_DONE                  : return "H324EVN_STOP_DONE";
		case H324EVN_BI_CHANNEL_DESC            : return "H324EVN_BI_CHANNEL_DESC";
		case H324EVN_H245_INTERNAL_ERROR        : return "H324EVN_H245_INTERNAL_ERROR";
		case H324EVN_CALL_SETUP_FAILED          : return "H324EVN_CALL_SETUP_FAILED";
		case H324EVN_MEDIA_SETUP_FAILED         : return "H324EVN_MEDIA_SETUP_FAILED";
      case H324EVN_VIDEO_CHANNEL_SETUP_FAILED : return "H324EVN_VIDEO_CHANNEL_SETUP_FAILED";
		case H324EVN_CHANNEL_CLOSED             : return "H324EVN_CHANNEL_CLOSED";
		
      case H324EVN_VIDEO_OLC_TIMER_EXPIRED    : return "H324EVN_VIDEO_OLC_TIMER_EXPIRED";
      case H324EVN_END_SESSION_TIMER_EXPIRED  : return "H324EVN_END_SESSION_TIMER_EXPIRED";
      case H324EVN_END_SESSION_DONE           : return "H324EVN_END_SESSION_DONE";	
      
		default                                 : return "UNKNOWN_EVENT";
	}
}

char *LookupH324Reason(unsigned int EventID)
{
	switch(EventID)
	{
		case H324RSN_TERM_CAP_REJECT_UNSPECIFIED                    : return "H324RSN_TERM_CAP_REJECT_UNSPECIFIED";
		case H324RSN_TERM_CAP_REJECT_UNDEFINED_TABLE_ENTRY_USED     : return "H324RSN_TERM_CAP_REJECT_UNDEFINED_TABLE_ENTRY_USED";
		case H324RSN_TERM_CAP_REJECT_DESCRIPTOR_CAPACITY_EXCEEDED   : return "H324RSN_TERM_CAP_REJECT_DESCRIPTOR_CAPACITY_EXCEEDED";
		case H324RSN_TERM_CAP_REJECT_TABLE_ENTRY_CAPACITY_EXCEEDED  : return "H324RSN_TERM_CAP_REJECT_TABLE_ENTRY_CAPACITY_EXCEEDED";
		case H324RSN_TERM_CAP_TIME_OUT                              : return "H324RSN_TERM_CAP_TIME_OUT";
        case H324RSN_TERM_CAP_REMOTE_REJECT                         : return "H324RSN_TERM_CAP_REMOTE_REJECT";
		case H324RSN_TERM_CAP_ERROR_UNSPECIFIED                     : return "H324RSN_TERM_CAP_ERROR_UNSPECIFIED";
		case H324RSN_MASTER_SLAVE_ERROR                             : return "H324RSN_MASTER_SLAVE_ERROR";
		case H324RSN_MUX_TABLE_REJECT_UNSPECIFIED                   : return "H324RSN_MUX_TABLE_REJECT_UNSPECIFIED";
		case H324RSN_MUX_TABLE_REJECT_DESCRIPTOR_TOO_COMPLEX        : return "H324RSN_MUX_TABLE_REJECT_DESCRIPTOR_TOO_COMPLEX";
		case H324RSN_MUX_TABLE_TIME_OUT                             : return "H324RSN_MUX_TABLE_TIME_OUT ";
		case H324RSN_INBOUND_AUDIO_CHANNEL_FAILURE                  : return "H324RSN_INBOUND_AUDIO_CHANNEL_FAILURE";
		case H324RSN_OUTBOUND_AUDIO_CHANNEL_FAILURE                 : return "H324RSN_OUTBOUND_AUDIO_CHANNEL_FAILURE";
		case H324RSN_LCSE_ERROR_INDICATION                          : return "H324RSN_LCSE_ERROR_INDICATION";
		case H324RSN_UNSPECIFIED                                    : return "H324RSN_UNSPECIFIED";
		case H324RSN_AUDIO_IN                                       : return "H324RSN_AUDIO_IN";
        case H324RSN_AUDIO_OUT                                      : return "H324RSN_AUDIO_OUT";
		case H324RSN_VIDEO                                          : return "H324RSN_VIDEO";
		default                                                     : return "UNKNOWN_REASON";
	}
}                            


/* Return a space-delimited string of byte values represented as hexadecimal numbers.
 */
char * makeHexString( char * outBuf, unsigned char * inBuf, unsigned len, unsigned max )
{
    unsigned        idx;
    unsigned char * inByte = inBuf;
    char          * outChar = outBuf;
    unsigned char   nib;

    *outBuf = '\0';

    // Each input byte becomes two output chars plus either a space or the final NUL.
    if ((len * 3) > max)
        return strncat(outBuf, "<too big>", max);

    for (idx = 0; idx < len; idx++)
    {
        nib = ((*inByte >> 4) & 0x0F);
        *outChar++ = (nib <= 9) ? (nib + '0') : ((nib + 'a') - 0xA);
        nib = ((*inByte) & 0x0F);
        *outChar++ = (nib <= 9) ? (nib + '0') : ((nib + 'a') - 0xA);
        *outChar++ = ' ';
        inByte++;
    }

    if (len > 0)
        *(--outChar) = '\0';

    return outBuf;
}


/* Return a space-delimited string of byte values represented as decimal numbers.
 */
char * makeDecString( char * outBuf, unsigned char * inBuf, unsigned len, unsigned max )
{
    char            tmpBuf[4];
    unsigned        idx;

    *outBuf = '\0';

    // Each input byte becomes two output chars plus either a space or the final NUL.
    if ((len * 4) > max)
        return strncat(outBuf, "<too big>", max);

    for (idx = 0; idx < len; idx++)
    {
        if (idx != 0)
            strcat(outBuf, " ");
        sprintf(tmpBuf, "%u", (unsigned)inBuf[idx]);
        strcat(outBuf, tmpBuf);
    }

    return outBuf;
}


char * makeAsciiString( char * outBuf, unsigned char * inBuf, unsigned len, unsigned max )
{
    unsigned        idx;

    *outBuf = '\0';

    for (idx = 0; idx < len; idx++)
    {
        if (idx == max || (inBuf[idx] < ' ' && inBuf[idx] != 0) || inBuf[idx] > '~')
        {
            *outBuf = '\0';
            return outBuf;
        }
        outBuf[idx] = inBuf[idx];
    }

    // Vetted by the "idx == max" test above.
    outBuf[idx] = '\0';

    return outBuf;
}


int ProcessH324Event( CTA_EVENT *pevent, DWORD dGwIndex )
{
	char buffer[120];
	MEDIA_GATEWAY_CONFIG *pCfg;
	DWORD ret;		
    H324_LCD *l_pLCD;
   	int   vtpId;
	H324_START_PARAMS StartParams;
		
#ifdef USE_TRC	
    tTxCoderParam txCoderParam;     
#endif    

	 // NMS TCoder MPEG4 Header	
    BYTE mpeg4_cfg_nms[] = { 
    0x00, 0x00, 0x01, 0xb0, 0x08, 0x00, 0x00, 0x01, 
    0xb5, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,                
    0x01, 0x20, 0x00, 0x88, 0x40, 0x07, 0xa8, 0x2c, 
    0x20, 0x90, 0xa2, 0x1f 
    };
    int mpeg4_cfg_len_nms = 28;

/*    
    // DNA MPEG4 Header
    BYTE mpeg4_cfg_nms[] = { 
    0x00, 0x00, 0x01, 0xb0, 0x08, 0x00, 0x00, 0x01, 
    0xb5, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,                
    0x01, 0x20, 0x00, 0x84, 0x5d, 0x4c, 0x28, 0x2c, 
    0x20, 0x90, 0xa2, 0x1f 
    };
    int mpeg4_cfg_len_nms = 28;      
*/                                              
	//Print all H.324 events
	h324FormatEvent("ProcessH324Event: ", pevent, buffer, sizeof(buffer));
	mmPrint(dGwIndex, T_H245, buffer);

	// Get a pointer to the gateway's configuration
	pCfg = &GwConfig[dGwIndex];
	
    switch( pevent->id )
    {

	case H324EVN_START_DONE:
	    Flow(dGwIndex, F_H324, F_APPL, "H324EVN_START_DONE");
        mmPrint(dGwIndex, T_H245, "Calling h324SetupCall\n");
	    ret = h324SetupCall(pCfg->MuxEp.hd);
	    Flow(dGwIndex, F_APPL, F_H324, "h324SetupCall");
	    pCfg->nTriesVideoOLC = 0; // DRTDRT reset counter video OLC attempts
		if(ret != SUCCESS)
		{
			mmPrint(dGwIndex, T_H245, "Error:h324SetupCall return 0x%08x\n",ret);
			return ret;
		}
        else
		// DRT DUMMY Location for GetTermCaps and SetTermCaps, move elseware later
		{
            WORD i;
            

		
			ret = h324GetTermCaps( pCfg->MuxEp.hd, H324_LOCAL_TERMINAL, &pCfg->localTermCaps);
		
			Flow(dGwIndex, F_APPL, F_H324, "h324GetTermCaps");
            mmPrint(dGwIndex, T_H245, "Call to Local h324GetTermCaps returned %d capabilities\n", 
                pCfg->localTermCaps.wCapCount );
            // Example of setting detailed capabilities here
            for(i=0;i<pCfg->localTermCaps.wCapCount;i++)
            {
                switch(pCfg->localTermCaps.capTable[i].choice)
                {
                case H324_H263_VIDEO:
                    mmPrint(dGwIndex, T_H245, "Local Capability [%d] = %d (H263)\n", i, pCfg->localTermCaps.capTable[i].choice );
                    pCfg->localTermCaps.capTable[i].bTransmit = false;				
					pCfg->localTermCaps.capTable[i].u.h263.mode.bit_rate	       = gwParm.h263_maxBitRate;
                    pCfg->localTermCaps.capTable[i].u.h263.mode.unrestricted_vector= true;
                    pCfg->localTermCaps.capTable[i].u.h263.mode.arithmetic_coding  = true;
                     
                     pCfg->localTermCaps.capTable[i].u.h263.capability.pb = FALSE;
                     pCfg->localTermCaps.capTable[i].u.h263.capability.temporal_spatial_tradeoff = FALSE;

                     pCfg->localTermCaps.capTable[i].u.h263.mode.resolution     = h263_qcif_resolution_chosen;
					
					pCfg->localTermCaps.capTable[i].u.h263.capability.max_bit_rate = gwParm.h263_maxBitRate;														
					pCfg->localTermCaps.capTable[i].u.h263.capability.bit_mask = h263_qcif_mpi_present;
                    pCfg->localTermCaps.capTable[i].u.h263.capability.qcif_mpi = 2;
                     
                     
									
                     break;
                     
               case H324_MPEG4_VIDEO:
                     mmPrint(dGwIndex, T_H245, "Local Capability [%d] = %d (MPEG4)\n", i, pCfg->localTermCaps.capTable[i].choice );
                     pCfg->localTermCaps.capTable[i].bTransmit = false;
                    pCfg->localTermCaps.capTable[i].u.mpeg4.mode.bit_rate           = 43000;
                    pCfg->localTermCaps.capTable[i].u.mpeg4.capability.max_bit_rate = 43000;					
                    pCfg->localTermCaps.capTable[i].u.mpeg4.mode.unrestricted_vector= true;

                     pCfg->localTermCaps.capTable[i].u.mpeg4.capability.bit_mask = 0;
                    
                     break;				
                    	
                case H324_G723_AUDIO:
                     mmPrint(dGwIndex, T_H245, "Local Capability [%d] = %d (G723)\n", i, pCfg->localTermCaps.capTable[i].choice );
                     pCfg->localTermCaps.capTable[i].bTransmit = false;
                     break;
                     
               case H324_AMR_AUDIO:
                     mmPrint(dGwIndex, T_H245, "Local Capability [%d] = %d (AMR)\n", i, pCfg->localTermCaps.capTable[i].choice );
                     pCfg->localTermCaps.capTable[i].bTransmit = false;
					pCfg->localTermCaps.capTable[i].u.gsmamr.mode.choice = gwParm.gsmamr_mode;
                    break;					
                }
            }

			ret = h324SetTermCaps( pCfg->MuxEp.hd, &pCfg->localTermCaps);
			mmPrint(dGwIndex, T_H245, "Calling h324SetTermCaps\n");
			Flow(dGwIndex, F_APPL, F_H324, "h324SetTermCaps");

		}
		break;

	case H324EVN_CALL_SETUP_DONE:
        H324_MEDIA_CAPABILITY *pMP4Cap;
        
        Flow(dGwIndex, F_H324, F_APPL, "H324EVN_CALL_SETUP_DONE");
        mmPrint(dGwIndex, T_H245, "Calling h324SetupMedia\n");
	    ret = h324SetupMedia(pCfg->MuxEp.hd);
	    Flow(dGwIndex, F_APPL, F_H324, "h324SetupMedia");
		if(ret != SUCCESS) {
			mmPrint(dGwIndex, T_H245, "Error:h324SetupMedia returned 0x%08x\n",ret);
			return ret;
		}

		// Audio Channel
		ret = h324DefineMediaChannel( pCfg->MuxEp.hd, &pCfg->localTermCaps.capTable[0], 0 ); //out channel
		mmPrint(dGwIndex, T_H245, "Calling h324DefineMediaChannel for Out Audio OLC with Audio codec %d\n", pCfg->localTermCaps.capTable[0].choice);
		Flow(dGwIndex, F_APPL, F_H324, "h324DefineMediaChannel");
		if(ret != SUCCESS) {
			mmPrint(dGwIndex, T_H245, "Error: h324DefineMediaChannel for audio returned 0x%08x\n",ret);
			return ret;
		}

		if( gwParm.sendVideoOLC ) {
			// Video Channel			
			H324_TERM_CAPS			remoteTermCaps;
			DWORD                preferedVideoCodec = -1;
			DWORD                alternativeVideoCodec = -1;
			WORD                 i;

		   ret = h324GetTermCaps( pCfg->MuxEp.hd, H324_REMOTE_TERMINAL, &remoteTermCaps);
		
			Flow(dGwIndex, F_APPL, F_H324, "h324GetTermCaps remote");
         mmPrint(dGwIndex, T_H245, "Call to Remote h324GetTermCaps returned %d capabilities\n", 
                remoteTermCaps.wCapCount );
			
			
			// Check if our preferred or alternative codec correspond to one of the remote
			for(i=0;i<remoteTermCaps.wCapCount;i++)
         {
            if ( pCfg->localTermCaps.capTable[1].choice == remoteTermCaps.capTable[i].choice )
               preferedVideoCodec = pCfg->localTermCaps.capTable[1].choice;
            if ( pCfg->localTermCaps.capTable[2].choice == remoteTermCaps.capTable[i].choice )
               alternativeVideoCodec = pCfg->localTermCaps.capTable[2].choice;                           
			}
			
			int index_preferred = 1;
			// if only our alternative codec matches, it should be the preferred one
			if ((preferedVideoCodec == -1) && (alternativeVideoCodec != -1))
			{
			   preferedVideoCodec = alternativeVideoCodec;
			   index_preferred = 2;
			}
			
			if (preferedVideoCodec != -1)
			{
   			mmPrint(dGwIndex, T_H245, "Calling h324DefineMediaChannel for Video OLC with Video codec %d\n", preferedVideoCodec);
   			
   			if ((preferedVideoCodec == H324_MPEG4_VIDEO)&& gwParm.specifyVOVOL )
   			{
               // Modify Video Channel Header to look like requested config
               BYTE    *l_pHdrCfg = mpeg4_cfg_nms;
               int     l_nHdrCfgLen = mpeg4_cfg_len_nms;
               
               // Modify config to look like requested config
               pMP4Cap = &pCfg->localTermCaps.capTable[index_preferred];
               pMP4Cap->u.mpeg4.capability.bit_mask                  |= mp4video_decoder_config_info_present;
               pMP4Cap->u.mpeg4.capability.decoder_config_info_len   = l_nHdrCfgLen;
               pMP4Cap->u.mpeg4.capability.decoder_config_info       = new unsigned char[l_nHdrCfgLen];
               memcpy( pMP4Cap->u.mpeg4.capability.decoder_config_info,
                     l_pHdrCfg,
                     l_nHdrCfgLen   );			
   			}
   			
   			ret = h324DefineMediaChannel( pCfg->MuxEp.hd, &pCfg->localTermCaps.capTable[index_preferred], // out channel
   										  &pCfg->localTermCaps.capTable[index_preferred] );               // in channel
   			Flow(dGwIndex, F_APPL, F_H324, "h324DefineMediaChannel");
   			if(ret != SUCCESS) {
   				mmPrint(dGwIndex, T_H245, "Error: h324DefineMediaChannel for video returned 0x%08x\n",ret);
   				return ret;
   			}
   		}	
		}
		
		break;

	case H324EVN_MEDIA_SETUP_DONE:
        Flow(dGwIndex, F_H324, F_APPL, "H324EVN_MEDIA_SETUP_DONE"); 
		mmPrint(dGwIndex, T_H245, "H324EVN_MEDIA_SETUP_DONE received.\n");

#ifdef USE_TRC
        if( gwParm.useVideoXc ) {
            // Allocates a transcoder resource 
            ret = trcCreateVideoChannel( TRC_CH_SIMPLEX, &vtpId,
                                     &GwConfig[dGwIndex].VideoXC.xcChanId,
                                     &txCoderParam );
            Flow(dGwIndex, F_APPL, F_TRC, "trcCreateVideoChannel");            
            if( ret != TRC_SUCCESS ) {
                mmPrintPortErr( dGwIndex, "trcCreateVideoChannel returned failure code %d\n", ret);
                return FAILURE;
            } else {
                mmPrint( dGwIndex, T_GWAPP, "trcCreateVideoChannel allocated the simplex channel.\n");
                mmPrint( dGwIndex, T_GWAPP, "New channel ID=%d. TxCoder params. vtpId=%d, IP: %s, port1=%d\n",
                           pCfg->VideoXC.xcChanId, vtpId, txCoderParam.ipAddr,
                           txCoderParam.RxPort1 );
                pCfg->VideoXC.xcChanConfig.params1.RxPort = txCoderParam.RxPort1;
                strcpy( pCfg->VideoXC.ipAddr, txCoderParam.ipAddr );
            }    
    
            // Setup the transcoding channel's parameters according to the negotiated video codec
            switch( pCfg->txVideoCodec ) {
                case H324_MPEG4_VIDEO:
                    mmPrint( dGwIndex, T_GWAPP, "Starting H263 --> MPEG4 transcoding channel.\n");                                                            
                    pCfg->VideoXC.xcChanConfig.params1.VidType  = 2; // H263
                    pCfg->VideoXC.xcChanConfig.params2.VidType      = 0; // MPEG4
                    pCfg->VideoXC.xcChanConfig.params1.outPayloadID = 100;   
                    break;
                case H324_H263_VIDEO:
                    mmPrint( dGwIndex, T_GWAPP, "Starting MPEG4 --> H263 transcoding channel.\n");                                        
                    pCfg->VideoXC.xcChanConfig.params1.VidType  = 0; // MPEG4  
                    pCfg->VideoXC.xcChanConfig.params2.VidType      = 2; // H263 
                    pCfg->VideoXC.xcChanConfig.params1.outPayloadID = 34;                   
                    break;
            }
            
            // Starts a media channel on the allocated VTP
            if( pCfg->VideoXC.bActive ) {       
                mmPrint(dGwIndex, T_GWAPP, "Start Simplex Video Channel. Rx1=%d, Dest1=%d,  DestIP1=%s\n",
                        pCfg->VideoXC.xcChanConfig.params1.RxPort,
                        pCfg->VideoXC.xcChanConfig.params1.DestPort,
                        pCfg->VideoXC.xcChanConfig.params1.DestIP);            
    	        ret = trcStartVideoChannel( pCfg->VideoXC.xcChanId,
                                            &pCfg->VideoXC.xcChanConfig );
                Flow(dGwIndex, F_APPL, F_TRC, "trcStartVideoChannel");                            
  	            if( ret != TRC_SUCCESS ) {
                    mmPrintPortErr( dGwIndex, "trcStartVideoChannel failed to start the channel. rc = %d\n", ret);
                    return FAILURE;
                }        
            } 
        } // end if gwParm.useVideoXc
#endif // USE_TRC			      
        
		ret = initVideoPath( dGwIndex, pCfg->txVideoCodec );
		if( ret != SUCCESS )
			return ret;	
        
        // Enable the Video Channel 
	    enableVideoChannel( dGwIndex, pCfg->VidChan.hd );        
        
		if( mmParm.bAutoRecord == 1 ) {
			
			mmParm.Operation = RECORD;
			PlayRecord( dGwIndex, convertVideoCodec2Format(GwConfig[dGwIndex].txVideoCodec) );
		}
				
		if( mmParm.bAutoPlay == 1 ) {
			tVideo_Format fileFormat;
			mmParm.Operation = PLAY;	
			                           
	        fileFormat = (tVideo_Format)getFileFormat( VideoCtx[dGwIndex].threegp_FileName );	
            	
			PlayRecord( dGwIndex, fileFormat );
		} else if( mmParm.bAutoPlayList == 1 ) {
            PlayFromPlayList( dGwIndex, mmParm.autoPlayListFileName );
        }
        
	    break;

	case H324EVN_REMOTE_CAPABILITIES:
        {
            H324_TERM_CAPS* pTermCaps = (H324_TERM_CAPS*)(pevent->buffer);
            WORD i;
            char string[40];
            char line[80];

            Flow(dGwIndex, F_H324, F_APPL, "H324EVN_REMOTE_CAPABILITIES");
            //mmPrint(dGwIndex, T_H245, "H324EVN_REMOTE_CAPABILITIES received. \n");
            if(pTermCaps)
            {
                line[0]='\0';
                for(i=0; i<pTermCaps->wCapCount; i++)
                {
					sprintf(string, " %d ",pTermCaps->capTable[i].choice);
					strcat(line, string);
                }
                mmPrint(dGwIndex, T_H245, "Remote terminal offered %d capabilities of types %s\n",
                    pTermCaps->wCapCount, line );
            }
            else
                mmPrint(dGwIndex, T_H245, "ERROR: H324EVN_REMOTE_CAPABILITIES had a NULL Buffer. \n");
            // Process Remote capabilities here, to determine the type of Channels to open.
        }
		break;
    case H324EVN_REMOTE_OLC:
        
        l_pLCD = (H324_LCD*)(pevent->buffer);

        Flow(dGwIndex, F_H324, F_APPL, "H324EVN_REMOTE_OLC");
        //mmPrint(dGwIndex, T_H245, "H324EVN_REMOTE_OLC received. \n");
        if (l_pLCD->bReceive)
         mmPrint(dGwIndex, T_H245, "rxChoice %d  rxChannel %d\n",l_pLCD->rxChoice, l_pLCD->rxChannel);

        if (l_pLCD->bTransmit)
         mmPrint(dGwIndex, T_H245, "txChoice %d  txChannel %d\n",l_pLCD->txChoice, l_pLCD->txChannel);
        
        // Always accept audio
        if(l_pLCD->rxChoice == H324_AMR_AUDIO)
        {
            mmPrint(dGwIndex, T_H245, "Accept Remote Audio OLC with codec %d. rx channel %d \n", l_pLCD->rxChoice, l_pLCD->rxChannel);
            h324OLCResponse( pCfg->MuxEp.hd, H324_ACCEPT, l_pLCD->rxChannel );
            Flow(dGwIndex, F_APPL, F_H324, "h324OLCResponse");

        }
        // DRTDRT Accept video OLC after 1 rejection
        else if((l_pLCD->rxChoice == H324_MPEG4_VIDEO)||(l_pLCD->rxChoice == H324_H263_VIDEO))
        {
   		   if( gwParm.rejectVideoOLC ) {


                  if(pCfg->nTriesVideoOLC < 1) // gwParm.nRejectVideoOLC)   reject the first video OLC
                  {
                      mmPrint(dGwIndex, T_H245, "Reject Remote Video OLC with codec %d. rx channel %d \n", l_pLCD->rxChoice, l_pLCD->rxChannel);
                      h324OLCResponse( pCfg->MuxEp.hd, H324_REJECT, l_pLCD->rxChannel );
                      Flow(dGwIndex, F_APPL, F_H324, "h324OLCResponse");
                  }
                  else
                  {
                      mmPrint(dGwIndex, T_H245, "Accept Remote Video OLC with codec %d after first rejection. rx channel %d \n", l_pLCD->rxChoice, l_pLCD->rxChannel);
                      h324OLCResponse( pCfg->MuxEp.hd, H324_ACCEPT, l_pLCD->rxChannel );
                      Flow(dGwIndex, F_APPL, F_H324, "h324OLCResponse");
                  }
                  pCfg->nTriesVideoOLC++;                
                           
            }
            else
            {
                mmPrint(dGwIndex, T_H245, "Accept Remote Video OLC with codec %d. rx channel %d \n", l_pLCD->rxChoice, l_pLCD->rxChannel);
                h324OLCResponse( pCfg->MuxEp.hd, H324_ACCEPT, l_pLCD->rxChannel );
                Flow(dGwIndex, F_APPL, F_H324, "h324OLCResponse");
            }
        
       
        }
        break;  


    case H324EVN_END_SESSION:
		Flow(dGwIndex, F_H324, F_APPL, "H324EVN_END_SESSION");
		//mmPrint(dGwIndex, T_H245, "  Event: H324EVN_END_SESSION received\n");
        break;

	case H324EVN_STOP_DONE: 
		Flow(dGwIndex, F_H324, F_APPL, "H324EVN_STOP_DONE");
        mmPrint(dGwIndex, T_H245, "H324EVN_STOP_DONE received\n");
        CTA_EVENT waitEvent;        

       	mmPrint(dGwIndex, T_H245,"Call h324Delete.\n");                    
	    ret = h324Delete(pCfg->MuxEp.hd);
        Flow(dGwIndex, F_APPL, F_H324, "h324Delete");        
		if(ret != SUCCESS)
		{
			mmPrintPortErr(dGwIndex, "h324Delete returned 0x%08x\n",ret);
			return ret;
		}

        // Disable the Audio Bypass Channel 
   	    disableAudByChannel( dGwIndex, pCfg->AudByChan.hd );

        // Disable the Video Channel 
        disableVideoChannel( dGwIndex, pCfg->VidChan.hd );	            
        
        // Destroy the Video Path: MSPP EP+Channel
        ret = destroyVideoPath( dGwIndex );

#ifdef USE_TRC                       
        // Stop and destroy the transcoding channel
        if( gwParm.useVideoXc ) {        
            ret = trcStopVideoChannel( pCfg->VideoXC.xcChanId );
            Flow(dGwIndex, F_APPL, F_TRC, "trcStopVideoChannel");                                        
            if( ret != TRC_SUCCESS ) {
              mmPrintPortErr(dGwIndex, "trcStopVideoChannel failed. ret = %d\n", ret);
              mmPrint(dGwIndex, T_GWAPP, "Trying to destroy the Video Channel ID=0x%x...\n",
                      pCfg->VideoXC.xcChanId );
              ret = trcDestroyVideoChannel( pCfg->VideoXC.xcChanId );
              Flow(dGwIndex, F_APPL, F_TRC, "trcDestroyVideoChannel");                                          
              if( ret != TRC_SUCCESS )
                mmPrintPortErr(dGwIndex, "trcDestroyVideoChannel failed. rc = %d\n", ret);                
              else 
                  mmPrint(dGwIndex, T_GWAPP, "Video Channel ID=0x%x destroyed.\n", pCfg->VideoXC.xcChanId);                    

              pCfg->VideoXC.xcChanId = -1;
            }
        }
#endif // USE_TRC
		
        mmPrint(dGwIndex, T_H245, "Releasing ISDN Call\n");
        nccReleaseCall ( GwConfig[dGwIndex].isdnCall.callhd, NULL );
        Flow(dGwIndex, F_APPL, F_NCC, "nccReleaseCall");
        
		break;

    case H324EVN_VIDEO_FAST_UPDATE:
		Flow(dGwIndex, F_H324, F_APPL, "H324EVN_VIDEO_FAST_UPDATE");
		mmPrint(dGwIndex, T_H245, "Event: H324EVN_VIDEO_FAST_UPDATE received\n");	
        
#ifdef USE_TRC                       		
        if( pCfg->bGwVtpServerConfig && gwParm.useVideoXc && pCfg->isdnCall.dCallState == CON ) {                
            ret = trcConfigVideoChannel( pCfg->VideoXC.xcChanId,
                                         TRCCMD_GENERATE_IFRAME, 1, NULL, 0 );
            Flow(dGwIndex, F_APPL, F_TRC, "trcConfigVideoChannel");                                        
            if(ret != SUCCESS) {
                mmPrintPortErr( dGwIndex, "trcConfigVideoChannel (Generate I-Frame) failed. rc = %d\n", ret);
            }                                   
        }
#endif // USE_TRC        
		break;

	case H324EVN_BI_CHANNEL_DESC:
      char video_str[20];

    
      Flow(dGwIndex, F_H324, F_APPL, "H324EVN_BI_CHANNEL_DESC");
      mmPrint(dGwIndex, T_H245, "Event: H324EVN_BI_CHANNEL_DESC received\n");			
        
      // Retrieve the negotiated tx video codec
      l_pLCD = (H324_LCD *)(pevent->buffer);
      pCfg->txVideoCodec = l_pLCD->txChoice;
      get_video_codec_string( pCfg->txVideoCodec, video_str );
		mmPrint(dGwIndex, T_H245, "Tx video codec from H324EVN_BI_CHANNEL_DESC event: %d (%s)\n",
                pCfg->txVideoCodec, video_str);			         


      if (l_pLCD->bReceive)
      {
         mmPrint(dGwIndex, T_H245, "rxChoice %d  rxChannel %d\n",l_pLCD->rxChoice, l_pLCD->rxChannel);
         
         if(l_pLCD->rxChoice == H324_MPEG4_VIDEO)
         {
            int i;
            if ((l_pLCD->rxU.mpeg4.capability.decoder_config_info != NULL ) && 
                (l_pLCD->rxU.mpeg4.capability.decoder_config_info_len != 0 ) )  
            {
            	printf("l_pLCD->rxU.mpeg4.capability.decoder_config_info_len = %d bytes:\n",
            		l_pLCD->rxU.mpeg4.capability.decoder_config_info_len);
            	for(i=0;i<l_pLCD->rxU.mpeg4.capability.decoder_config_info_len;i++)
            	{
            		printf("%02x ",l_pLCD->rxU.mpeg4.capability.decoder_config_info[i]);
            		if((i%16) == 15 ) printf("\n");
            	}
            	if((i%16) != 0 ) printf("\n");         
            }
      	
         }
        
        }
        if (l_pLCD->bTransmit)
            mmPrint(dGwIndex, T_H245, "txChoice %d  txChannel %d\n",l_pLCD->txChoice, l_pLCD->txChannel);
      


		break;

	case H324EVN_ROUND_TRIP_DELAY:
		Flow(dGwIndex, F_H324, F_APPL, "H324EVN_ROUND_TRIP_DELAY");
		mmPrint(dGwIndex, T_H245, "Event: H324EVN_ROUND_TRIP_DELAY received\n");			
		break;

	case H324EVN_USER_INDICATION:
    {              
		Flow(dGwIndex, F_H324, F_APPL, "H324EVN_USER_INDICATION");
        
		if( pevent->buffer != NULL )	
        {
            H324_USER_INPUT_INDICATION *l_pUII = (H324_USER_INPUT_INDICATION *)pevent->buffer;	
            switch(l_pUII->msgType)
            {
                case H324_USER_INPUT_ALPHANUMERIC:
                {
                    mmPrint(dGwIndex, T_H245, "Event H324EVN_USER_INDICATION received of type ALPHANUMERIC\n");
                    mmPrint(dGwIndex, T_H245, "  Data Length: %d,      Data Value:\n", l_pUII->length);
                    mmPrintBinary(dGwIndex, T_H245, (BYTE*)l_pUII->data, l_pUII->length);
                    break;
                }
                case H324_USER_INPUT_NONSTANDARD:
                {
                    mmPrint(dGwIndex, T_H245, "Event H324EVN_USER_INDICATION received of type NONSTANDARD\n");
                    mmPrint(dGwIndex, T_H245, "  ObjectID: \"%s\"\n", l_pUII->szObjectId);
                    mmPrint(dGwIndex, T_H245, "  Data Length: %d,      Data Value:\n", l_pUII->length);
                    mmPrintBinary(dGwIndex, T_H245, (BYTE*)l_pUII->data, l_pUII->length);
                    break;
                }
                default:
			        mmPrint(dGwIndex, T_H245ERR, "ERROR: Event H324EVN_USER_INDICATION received, but has unknown type\n");
                    break;
            }
        }		
		else
			mmPrint(dGwIndex, T_H245ERR, "ERROR: Event H324EVN_USER_INDICATION received with no data buffer.\n");
				
		break;
    }     

	case H324EVN_H245_INTERNAL_ERROR:
	case H324EVN_MEDIA_SETUP_FAILED:        
        mmPrintPortErr(dGwIndex, "Event %s received with reason code %s\n",
                LookupH324EventID(pevent->id), LookupH324Reason(pevent->size) );
        Flow(dGwIndex, F_H324, F_APPL, LookupH324EventID(pevent->id));
        
        if( pCfg->isdnCall.dCallState == CON ) {
    		ret = h324EndSession( pCfg->MuxEp.hd );               
            Flow(dGwIndex, F_APPL, F_H324, "h324EndSession");        
		    if(ret != SUCCESS) {
			    mmPrintPortErr(dGwIndex, "h324EndSession returned 0x%08x\n",ret);
    		}

            autoDisconnectCall( dGwIndex );										            
        }
        break;    
    
	case H324EVN_CALL_SETUP_FAILED:
        mmPrintPortErr(dGwIndex, "Event %s received with reason code %s\n",
                LookupH324EventID(pevent->id), LookupH324Reason(pevent->size) );
        Flow(dGwIndex, F_H324, F_APPL, LookupH324EventID(pevent->id));
        
		ret = h324EndSession( pCfg->MuxEp.hd );               
        Flow(dGwIndex, F_APPL, F_H324, "h324EndSession");        
		if(ret != SUCCESS) {
			mmPrintPortErr(dGwIndex, "h324EndSession returned 0x%08x\n",ret);
		}

        if( gwParm.auto_hangup )         
            autoDisconnectCall( dGwIndex );										
                
        break;        
        
	case H324EVN_CHANNEL_CLOSED:
        mmPrint(dGwIndex, T_H245, "H.245 channel closed (channel = %s).\n", LookupH324Reason(pevent->size) );
        Flow(dGwIndex, F_H324, F_APPL, LookupH324EventID(pevent->id));
        break;

	case H324EVN_VIDEO_CHANNEL_SETUP_FAILED:

        mmPrintPortErr(dGwIndex, "Event %s received with reason code \n\t\t%s\n",
                LookupH324EventID(pevent->id), LookupH324Reason(pevent->size) );

        Flow(dGwIndex, F_H324, F_APPL, LookupH324EventID(pevent->id));
        break;

	case H324EVN_VIDEO_OLC_TIMER_EXPIRED :
	case H324EVN_END_SESSION_TIMER_EXPIRED :
	case H324EVN_END_SESSION_DONE :
        Flow(dGwIndex, F_H324, F_APPL, LookupH324EventID(pevent->id));
        break;
    case H324EVN_H223_SKEW_INDICATION :
        
        if (!pevent->buffer)
           mmPrint(dGwIndex, T_H245, "H.223 Skew Indication w/NULL buffer ptr!\n");
        else 
        {
           H324_H223_SKEW_INDICATION * pInd = (H324_H223_SKEW_INDICATION *)pevent->buffer;
           mmPrint(dGwIndex, T_H245, "H.223 SkewIndication rcvd: channel #%u (%s,LCN1) skewed %u ms. behind channel #%u (%s,LCN2).\n",
                   pInd->logicalChannelNumber1, (pInd->skewType == videoLate) ? "video" : "audio", pInd->skewInMs, 
                   pInd->logicalChannelNumber2, (pInd->skewType == videoLate) ? "audio" : "video");
        }
        break;
                       
    case H324EVN_VENDORID_INDICATION :
        
        if (!pevent->buffer)
           mmPrint(dGwIndex, T_H245, "H.245 VendorID Indication rcv'd w/NULL buffer ptr!\n");
        else 
        {
           H324_VENDORID_INDICATION * pInd = (H324_VENDORID_INDICATION *)pevent->buffer;
           char buf[385], buf2[385];

           mmPrint(dGwIndex, T_H245, "H.245 VendorID Indication received:\n");
           if (pInd->isNonStandard)
           {
               mmPrint(dGwIndex, T_H245, "\tH.221 NonStandard vendorID:\n");
               mmPrint(dGwIndex, T_H245, "\t\tt35CountryCode: %d\n", (unsigned)pInd->bytes[0]);
               mmPrint(dGwIndex, T_H245, "\t\tt35Extension: %d\n",(unsigned)pInd->bytes[1]);
               mmPrint(dGwIndex, T_H245, "\t\tmanufacturerCode: %d\n",
                       ((((unsigned)pInd->bytes[2] << 8) & 0x0000FF00) | ((unsigned)pInd->bytes[3] & 0x000000FF)));
           }
           else
               mmPrint(dGwIndex, T_H245, "\tvendorID: %s (%s)\n",
               makeDecString(buf, &pInd->bytes[0], pInd->vendorIDLen, sizeof(buf)),
                       makeHexString(buf2, &pInd->bytes[0], pInd->vendorIDLen, sizeof(buf2)));
           mmPrint(dGwIndex, T_H245, "\tproductNumber: %s (%s)\n",
                   makeAsciiString(buf, &pInd->bytes[pInd->vendorIDLen], pInd->productNumberLen, sizeof(buf)),
                   makeHexString(buf2, &pInd->bytes[pInd->vendorIDLen], pInd->productNumberLen, sizeof(buf2)));
           mmPrint(dGwIndex, T_H245, "\tversionNumber: %s (%s)\n", 
                   makeAsciiString(buf, &pInd->bytes[pInd->vendorIDLen+pInd->productNumberLen], pInd->versionNumberLen, sizeof(buf)),
                   makeHexString(buf2, &pInd->bytes[pInd->vendorIDLen+pInd->productNumberLen], pInd->versionNumberLen, sizeof(buf2)));
        }
        break;
                       
    case H324EVN_VIDEOTEMPORALSPATIALTRADEOFF_INDICATION :
        
        if (!pevent->buffer)
      mmPrint(dGwIndex, T_H245, "H.245 Misc. VideoTemporalSpatialTradeoff Indication w/NULL buffer ptr!\n");
        else 
        {
           H324_VIDEOTEMPORALSPATIALTRADEOFF_INDICATION * pInd = (H324_VIDEOTEMPORALSPATIALTRADEOFF_INDICATION *)pevent->buffer;
           mmPrint(dGwIndex, T_H245, "VideoTemporalSpatialTradeoff Indication rcvd: channel #%u, tradeoff: %u.\n",
                   pInd->logicalChannelNumber, pInd->tradeoff);
        }
        break;

   default:
        break;
    }
    return SUCCESS;
}

/************************ Video Channel Destruction ****************************/

int destroyVideoPort(int port, CTAQUEUEHD QueueHd, CTAHD *CTAccessHd)
{
	CTA_EVENT event;
	DWORD ret;
    char *ServDesc_Close[NUM_CTA_SERVICES];    

    // MSPP service 
    ServDesc_Close[0] = "MSP";
             
	if((ret = ctaCloseServices(*CTAccessHd, ServDesc_Close, 1) != SUCCESS))
	{
		mmPrintPortErr(port,"Failed to close services for Context %d..Return code is 0x%x.\n", CTAccessHd, ret);
		return ret;
	}
	ret = WaitForSpecificEvent(port, QueueHd, CTAEVN_CLOSE_SERVICES_DONE, &event );
	if (ret != SUCCESS)
	{
		mmPrintPortErr(port, "Close services failed for Context %d...event.value=0x%x\n", CTAccessHd, event.value);     
		return ret;
	}
	mmPrint(port, T_CTA, "CT Access services closed for Context %x\n", CTAccessHd);

	if ((ret = ctaDestroyContext(*CTAccessHd)) != SUCCESS)
	{
		mmPrintAll("\nFailed to destroy CT Access context\n");
		return ret;
	}
	ret = WaitForSpecificEvent(port, QueueHd, CTAEVN_DESTROY_CONTEXT_DONE, &event );
	if (ret != SUCCESS)
	{
		mmPrintPortErr(port, "Destroy Context failed for Context %d...event.value=0x%x\n", CTAccessHd, event.value);     
		return ret;
	}	
	
	mmPrint(port, T_CTA, "CT Access context destroyed, CtaHd = %x\n", CTAccessHd);

	return SUCCESS;
}

DWORD destroyVideoPath( int nGw )
{
	MEDIA_GATEWAY_CONFIG *pCfg;
	DWORD ret;		   
    CTA_EVENT waitEvent;        
        
	pCfg = &GwConfig[nGw];
    if( pCfg->video_path_initialized == FALSE )           
        return SUCCESS;
            
    //Disconnect the Video Channel
   	mmPrint(nGw, T_MSPP,"Disconnect the Video Channel.\n");    
    ret = mspDisconnect(pCfg->VidRtpEp.hd, pCfg->VidChan.hd, pCfg->MuxEp.hd);
    if(ret != SUCCESS)
        mmPrintPortErr(nGw,"mspDisconnect did not return SUCCESS\n");
    else
        ret = WaitForSpecificEvent (nGw, pCfg->hCtaQueueHd,
                                    MSPEVN_DISCONNECT_DONE, &waitEvent);

    //Destroy the Video Channel  
    mmPrint(nGw, T_MSPP,"Destroy the Video Channel.\n");        
    ret = mspDestroyChannel(pCfg->VidChan.hd);
    if(ret != SUCCESS)
        mmPrintPortErr(nGw,"mspDestroyEndpoint did not return SUCCESS\n"); 
    else
        ret = WaitForSpecificEvent (nGw, pCfg->hCtaQueueHd, 
	            					MSPEVN_DESTROY_CHANNEL_DONE, &waitEvent);

    ret = destroyVideoPort( nGw, pCfg->hCtaQueueHd, &pCfg->VidChan.ctahd );          
    if( ret != SUCCESS ) {
        mmPrintPortErr(nGw,"destroyVideoPort did not return SUCCESS\n");         
    }
    
    //Destroy the Video RTP Endpoint  
  	mmPrint(nGw, T_MSPP,"Destroy the Video Endpoint.\n");            
    ret = mspDestroyEndpoint(pCfg->VidRtpEp.hd);
        if(ret != SUCCESS)
            mmPrintPortErr(nGw,"mspDestroyEndpoint did not return SUCCESS\n");
        else
            ret = WaitForSpecificEvent ( nGw, pCfg->hCtaQueueHd, 
                						MSPEVN_DESTROY_ENDPOINT_DONE, &waitEvent);
        							
   	pCfg->VidRtpEp.bEnabled = FALSE;        

    ret = destroyVideoPort( nGw, pCfg->hCtaQueueHd, &pCfg->VidRtpEp.ctahd );          
    if( ret != SUCCESS ) {
        mmPrintPortErr(nGw,"destroyVideoPort did not return SUCCESS\n");         
    }
    
    pCfg->video_path_initialized = FALSE;    
    
    return ret;    
}
