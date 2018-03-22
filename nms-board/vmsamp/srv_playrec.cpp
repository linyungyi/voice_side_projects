/**********************************************************************
*  File - srv_playrec.cpp
*
*  Description - Video Messaging Server - a wrapper for play/record
*                functions
*
*  2005  NMS Communications Corporation.
*
***********************************************************************/

///////////////////
// Include Files //
///////////////////
#include <stdlib.h>  /* malloc() */

#include "mega_global.h"
#include "srv_types.h"
#include "srv_def.h"
#include "vm_trace.h"
#include "3gpformat.h"

extern void get_video_format_string( DWORD format, char *str );
extern void get_audio_format_string(  tAudio_Format format, char *str );

static DWORD getADIvidEncoder( tVideo_Format fileFormat );

/////////////////////////////////////////////////////////////////////////////////
//  PlayRecord()
//  Main Function to start playing or recording
/////////////////////////////////////////////////////////////////////////////////
DWORD PlayRecord( int port, tVideo_Format fileformat )
{
	ADI_MM_RECORD_PARMS recParms;
	DWORD       i, ret, wrSize;
	unsigned    Encoding;
	DWORD audioEP_filterhd, videoEP_filterhd;
	CTA_EVENT   event;
	MSPHD       epHd;
	tAudioMode  audioMode;
	DWORD       ADIaudEncoder;
	msp_FILTER_ENCODE_CMD FilterEncodeCmd;
	msp_FILTER_ENCODE_CMD *pFilterEncodeCmd = 0;
    NMS_FORMAT_TO_3GP_DESC fdesc;
    NMS_AUDIO_DESC audiodesc;
    NMS_VIDEO_DESC videodesc;
    char video_buf_format[20];
    char audio_buf_format[20];
	 FORMAT_3GP_TO_NMS_DESC	format_desc;


	if( (VideoCtx[port].pr_mode == PR_MODE_ACTIVE1) || (AudioCtx[port].pr_mode == PR_MODE_ACTIVE1) ) {
		mmPrintPortErr(port, "Failed to start Play or Rec Operation - ALready active\n");
		return FAILURE;
	}
      
	mmParm.ADIvidEncoder = getADIvidEncoder( fileformat );
	if( mmParm.ADIvidEncoder < 0 ) {
		mmPrintPortErr(port, "File Format <0x%x> not supported for Play/Rec operation\n", fileformat);
		return FAILURE;
	}

    pFilterEncodeCmd = &FilterEncodeCmd;
    pFilterEncodeCmd->value=0;
    i = port; 
	 
    if(mmParm.Video_Format != NO_VIDEO) {
	    // Get Video EP Filter Handle
	    ret = mspGetFilterHandle(VideoCtx[i].rtpEp.hd, MSP_ENDPOINT_RTPFDX_VIDEO, &videoEP_filterhd);
	    if (ret != SUCCESS) {
		    mmPrintPortErr(port, "Failed to get Video EP Filter Handle, %x\n", ret);
		    return FAILURE;
	    }

	    // Allocate buffers to read or write video  
	    if ((VideoCtx[i].data_buffer = (void *)malloc(mmParm.video_buffer_size)) == NULL) {
		    mmPrintPortErr(i,"Video buffer:failed malloc of %d bytes for port %d\n", mmParm.video_buffer_size, i);
		    exit( -1 );
	    }
    }  // END If there is a video channel

    if(mmParm.audioMode1 != NONE) {
	    // Get Audio EP Filter Handle
	    epHd = AudioCtx[i].rtpEp.hd;
	    ret = mspGetFilterHandle(epHd, MSP_ENDPOINT_RTPFDX, &audioEP_filterhd);
	    if (ret != SUCCESS) {
		    mmPrintPortErr(i, "Failed to get Audio EP Filter Handle, %x\n", ret);
		    return FAILURE;
           }
    }  // END If there is an Audio Channel

    // Allocate buffers to read or write audio 
    if ((AudioCtx[i].data_buffer = (void *)malloc(mmParm.audio_buffer_size)) == NULL) {
		mmPrintPortErr(i,"Native Audio Buffer:failed malloc of %d bytes for port %d\n", 
        		             mmParm.audio_buffer_size, i);
		exit( -1 );
    }

    mmPrint(i, T_MSPP, "Video EP Filter Handle: %x, Audio EP Filter Handle: %x\n", videoEP_filterhd, audioEP_filterhd);
          
	VideoCtx[i].pr_mode = PR_MODE_ACTIVE1;
    AudioCtx[i].pr_mode = PR_MODE_ACTIVE1;
    ADIaudEncoder = mmParm.ADIaudEncoder1;
    audioMode = mmParm.audioMode1;

        ////////////////////////////////
        // RECORD
        ////////////////////////////////
		if (mmParm.Operation == RECORD) {
            if( ( mmParm.audioMode1 == NATIVE_SD ) || ( mmParm.audioMode1 == FUS_IVR ) ) {
                // Enable  Channel
                ret = mspEnableChannel(AudioCtx[i].FusionCh.hd);
	            if (ret != SUCCESS)
	            {
		            mmPrintAll ("\nmspEnableChannel returned %d.", ret);
		            return FAILURE;
	            }
		        ret = WaitForSpecificEvent(i, GwConfig[i].hCtaQueueHd, 
						MSPEVN_ENABLE_CHANNEL_DONE, &event);
	            if (ret != SUCCESS)
	            {
		            mmPrintAll ("\nMSPEVN_ENABLE_CHANNEL_DONE event failure, ret= %x, event.value=%x,"
                            "\nboard error (event.size field) %x\n\n",
			                ret, event.value, event.size);
		            return FAILURE;
	            }
                mmPrint(i, T_MSPP, "Enable channel done...\n");                
            }
            
			// Do Recording of Video first
			if(mmParm.Video_Format != NO_VIDEO) {
				if ((ret = SetRecordParms(VideoCtx[i].ctahd, &recParms)) != SUCCESS) {
					mmPrintPortErr(i, "Failed to setup Record Parameters\n");
					exit(1);
				}
           
				ret = adiRecordMMToMemory(VideoCtx[i].ctahd, 
										  mmParm.ADIvidEncoder,                 // Encoding
										  VideoCtx[i].data_buffer, 
										  mmParm.video_buffer_size,
										  videoEP_filterhd,
										  audioEP_filterhd,
										  &recParms);
				if (ret != SUCCESS) {
					mmPrintPortErr(i, "Failed to Record to memory, %x\n", ret);
					exit(1);
				}
			}

			// Recording of the Audio 
			if( audioMode != NONE) {               
				if ((ret = SetRecordParms(AudioCtx[i].ctahd, &recParms)) != SUCCESS) {
					mmPrintPortErr(i, "Failed to setup Record Parameters\n");
					exit(1);
				}
               
				ret = adiRecordMMToMemory(AudioCtx[i].ctahd, 
										  ADIaudEncoder, 
										  AudioCtx[i].data_buffer, 
										  mmParm.audio_buffer_size,
										  videoEP_filterhd,
										  audioEP_filterhd,
										  &recParms);
            }  // END IF Record Audio
		}  // END IF Record
            
        ////////////////////////////////
        // PLAY
        ////////////////////////////////
		else {
   			ret = get3GPInfo( VideoCtx[i].threegp_FileName,             
							   &audiodesc, &videodesc, NULL, 0 );            
			if (ret != FORMAT_SUCCESS) {
	            mmPrintPortErr(i, "Failed to Get 3GP info from file %s, error %x\n",
							   VideoCtx[i].threegp_FileName, ret);
	            return FAILURE;
   			}

            get_audio_format_string( audiodesc.codec, audio_buf_format );
            get_video_format_string( videodesc.codec, video_buf_format );
            
            mmPrint(i, T_SRVAPP, "3GP data to be read from the file %s\n", VideoCtx[i].threegp_FileName);
            mmPrint(i, T_SRVAPP, "%d bytes of Estimated Audio (codec %s)  \n", audiodesc.bufSize, audio_buf_format);
            mmPrint(i, T_SRVAPP, "%d bytes of Estimated Video (codec %s)  \n", videodesc.bufSize, video_buf_format);

            if (audiodesc.codec != mmParm.Audio_Record_Format1) {
	            mmPrintPortErr(i, "Audio codec from 3GP Audio track  <%d> different from Configured one <%d>\n", 
	                                 audiodesc.codec, mmParm.Audio_Record_Format1);
	            exit(-1);
			}
   	      
			if ( mmParm.audio_buffer_size < audiodesc.bufSize) {
	            mmPrintPortErr(i, "Audio Buffer size <%d> can't contain 3Gp Audio track <%d>\n", mmParm.audio_buffer_size, audiodesc.bufSize);
	            exit(-1);
			}

			if ( mmParm.video_buffer_size < videodesc.bufSize) {
	            mmPrintPortErr(i, "Video Buffer size <%d> can't contain 3Gp Video track <%d>\n", mmParm.video_buffer_size, videodesc.bufSize);
	            exit(-1);
			} 
  	      
			// set RTP payload types to default (providing a NULL pointer format3GPtoNMS does the same)
			format_desc.amrPayloadType   = DEFAULT_PAYLOAD_TYPE;
			format_desc.mpeg4PayloadType = DEFAULT_PAYLOAD_TYPE;
			format_desc.h263PayloadType  = DEFAULT_PAYLOAD_TYPE;

            ret = format3GPtoNMS ( VideoCtx[i].threegp_FileName,             
								   AudioCtx[i].data_buffer , 
								   VideoCtx[i].data_buffer,       
									&audiodesc, 
									&videodesc, 
									&format_desc,					// control of the conversion
  									0 );                               
			if (ret != FORMAT_SUCCESS) {
				mmPrintPortErr(i, "Failed to format from file %s to NMS , error %x\n", VideoCtx[i].threegp_FileName, ret );
	            exit(-1);
			}		                
            
            mmPrint(i, T_SRVAPP, "Play Video (%d bytes)  \n", videodesc.bufSize);  	      
			ret = adiPlayMMFromMemory(VideoCtx[i].ctahd, 
									  mmParm.ADIvidEncoder, 
									  VideoCtx[i].data_buffer, 
									  videodesc.bufSize, // beh 9/21/02 replace BUFFERSIZE with actual size wrSize
									  videoEP_filterhd,
									  NULL);
			if (ret != SUCCESS) {
   				mmPrintPortErr(i, "Failed to Play Video from memory, %x\n", ret);
   				exit(1);
   			}
   				
   			mmPrint(i, T_SRVAPP, "Play Audio (%d bytes)  \n", audiodesc.bufSize);	
   			ret = adiPlayMMFromMemory(AudioCtx[i].ctahd, 
   										  ADIaudEncoder, 
   										  AudioCtx[i].data_buffer, 
   										  audiodesc.bufSize, // beh 9/21/02 replace BUFFERSIZE with actual size wrSize
   										  audioEP_filterhd,
   										  NULL);
   			if (ret != SUCCESS)
   			{
   				mmPrintPortErr(i, "Failed to Play Audio from memory, %x\n", ret);
   				exit(1);
   			}
   	   
		} // End If play

	return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//  SetRecordParms()
//  Function to Get the Current CTA Parameters, an reset only those That were
//  Changed in the config file.
/////////////////////////////////////////////////////////////////////////////////
DWORD SetRecordParms(CTAHD ctahd, ADI_MM_RECORD_PARMS* recParms)
{
	DWORD ret;

	ret = ctaGetParms(ctahd, ADI_RECORD_PARMID, recParms, sizeof(ADI_MM_RECORD_PARMS));
	if (ret != SUCCESS)
	{
		mmPrintAll ("\nFailed getting Record Parameters %d.", ret);
		return FAILURE;
	}

	if (mmParm.bEnable_Iframe == TRUE)
	    recParms->startonvideoIframe = mmParm.enable_Iframe;

	if (mmParm.bAudioStop_InitTimeout == TRUE)
	    recParms->novoicetime = mmParm.audioStop_InitTimeout;

	if (mmParm.bAudioStop_SilenceTimeout == TRUE)
	    recParms->silencetime = mmParm.audioStop_SilenceTimeout;

	if (mmParm.bAudioStop_SilenceLevel == TRUE)
	    recParms->silenceampl = mmParm.audioStop_SilenceLevel;

	if (mmParm.bVideoStop_InitTimeout == TRUE)
	    recParms->novideotime = mmParm.videoStop_InitTimeout;

	if (mmParm.bVideoStop_NosignalTimeout == TRUE)
	    recParms->videotimeout = mmParm.videoStop_NosignalTimeout;

	if (mmParm.bMaxTime == TRUE)
	    recParms->maxtime = mmParm.maxTime;
    
	return SUCCESS;
}

DWORD getADIvidEncoder( tVideo_Format fileFormat )
{
	switch (fileFormat) {
   		case MPEG4:
			return ADI_ENCODE_NATIVE_MPEG4;
   		case H263:
            return ADI_ENCODE_NATIVE_H_263;        
   
        default:
			return -1;
	}     

	return -1;
}
