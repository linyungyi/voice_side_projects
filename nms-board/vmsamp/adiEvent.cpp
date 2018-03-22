/**********************************************************************
*  File - adiEvent.cpp
*
*  Description - The functions in this file processes ADI specific
*                events
*
*  2005  NMS Communications Corporation.
*
***********************************************************************/

#include "mega_global.h"
#include "gw_types.h"
#include "srv_types.h"
#include "srv_def.h"
#include "adiEvent.h"
#include "vm_trace.h"
#include "vm_stat.h"
#include "3gpformat.h"


extern DWORD PlayFromDB( int nGw, char *fileName, tVideo_Format fileFormat );
extern int   getFileFormat( char *fileName );
extern tVideo_Format convertVideoCodec2Format( DWORD VideoCodec );
extern void get_video_format_string( DWORD format, char *str );
extern void get_audio_format_string(  tAudio_Format format, char *str );

CTAHD FindContext(CTAHD received_ctahd, DWORD* Port, DWORD* Type)
{
	DWORD i;

	for(i=0; i<mmParm.nPorts; i++)
	{   // Look through video contexts first
		if (VideoCtx[i].ctahd == received_ctahd)
		{
			*Port = i;
			*Type = 0; // Video
			return (VideoCtx[i].ctahd);
		}
        // Look through audio contexts
		if (AudioCtx[i].ctahd == received_ctahd)
		{
			*Port = i;
			*Type = 1; // Native Audio
			return (AudioCtx[i].ctahd);
		}
	}
	return 0;
}


void FormatTo3GP(DWORD Port)
{      					
   NMS_FORMAT_TO_3GP_DESC formatdesc;
   NMS_AUDIO_DESC audiodesc;
   NMS_VIDEO_DESC videodesc;
   DWORD retFormat;
   char video_buf_format[20];
   char audio_buf_format[20];
   
   mmPrint(Port, T_SRVAPP, "Writing data to the 3GP file %s\n",  VideoCtx[Port].threegp_FileName);
      
   audiodesc.codec = mmParm.Audio_Record_Format1;
   audiodesc.bufSize = AudioCtx[Port].data_size;
   get_audio_format_string( audiodesc.codec, audio_buf_format );

   videodesc.codec = convertVideoCodec2Format(GwConfig[Port].txVideoCodec);
   videodesc.bufSize = VideoCtx[Port].data_size;
   get_video_format_string( videodesc.codec, video_buf_format );
   
   formatdesc.maxSize = 0;
   formatdesc.maxTime = 0;
   formatdesc.formatMask = FORMAT_NO_HEADER_CHECK;         

   mmPrint(Port, T_SRVAPP, "%d bytes of Audio (codec %s)\n", audiodesc.bufSize,  audio_buf_format);
   mmPrint(Port, T_SRVAPP, "%d bytes of Video (codec %s)\n", videodesc.bufSize,  video_buf_format);
   
                    
   retFormat = formatNMSto3GP (  AudioCtx[Port].data_buffer ,            
                           VideoCtx[Port].data_buffer,            
                           &audiodesc,         
   					         &videodesc,          
   					         VideoCtx[Port].threegp_FileName,
   					         &formatdesc,
   					         //NULL,                                
     					         0 );            

  if (retFormat != FORMAT_SUCCESS)
  {
     mmPrintPortErr(Port, "Failed to format into file %s to 3GP , error %x\n", VideoCtx[Port].threegp_FileName, retFormat );
     mmPrintPortErr(Port, "Failed to format into file Audio Size <%d>,Video Size <%d>\n", audiodesc.bufSize, videodesc.bufSize );
  }		 
    
}
  
void ProcessADIEvent( CTA_EVENT *pevent )
{
	CTAHD  prCtahd;
    DWORD  total_missing_packets=0;
    DWORD  total_bytes_recorded=0;
	DWORD  Port, Type, wrSize, ret;
	MEDIA_GATEWAY_CONFIG *pCfg;

	prCtahd = FindContext(pevent->ctahd, &Port, &Type);	
	pCfg = &GwConfig[Port];
	
    switch (pevent->id) {
		case ADIEVN_RECORD_DONE:
			mmPrint(Port, T_SRVAPP, "ADIEVN_RECORD_DONE is received...\n");
       		 /* beh - sign bit of size is set if a buffer is delivered in event, must mask off sign bit for actual size*/
	        if (pevent->size & CTA_INTERNAL_BUFFER )
    	         pevent->size &= ~ CTA_INTERNAL_BUFFER;
	        total_bytes_recorded = pevent->size;
	        
			if(prCtahd != 0) {
				bool last_rec_done = false;			
				switch (Type) {		
					case 0: // Video
						mmPrint(Port, T_SRVAPP, "Recording was performed for Video Port# %d\n", Port);

        	            if(VideoCtx[Port].pr_mode == PR_MODE_ACTIVE1) {
                	        VideoCtx[Port].pr_mode = PR_MODE_DONE1;
                    	} 
	               
						VideoCtx[Port].data_size = total_bytes_recorded;
						if(AudioCtx[Port].pr_mode == PR_MODE_ACTIVE1)
							mmPrint(Port, T_SRVAPP, "Video completed, but Not Audio - Wait to write into 3GP file\n");
						else {
		                    FormatTo3GP(Port);     
   	                 
  					        mmPrint(Port, T_SRVAPP, "Store to 3GP done\n");
  					        free(VideoCtx[Port].data_buffer);
  					        free(AudioCtx[Port].data_buffer);        
						}    
											
						if( AudioCtx[Port].pr_mode != PR_MODE_ACTIVE1 )
							last_rec_done = true;						
							
						if (pevent->buffer)
	                    	total_missing_packets = *((DWORD *)pevent->buffer);
	   		            mmPrint(Port, T_SRVAPP, "Context: %x, Reason: %x, Number of bytes recorded: %d, Number of missing RTP packets: %d\n", 
				                   pevent->ctahd, pevent->value, total_bytes_recorded, total_missing_packets);
						incrVideRec( Port ); 
        	            break;
					
					case 1: // Audio
						mmPrint(Port, T_SRVAPP, "Recording was performed for Audio Port# %d\n", Port);

                        if(AudioCtx[Port].pr_mode == PR_MODE_ACTIVE1)
                        {
                            AudioCtx[Port].pr_mode = PR_MODE_DONE1;
                        }
                  
						AudioCtx[Port].data_size = total_bytes_recorded;

						if(VideoCtx[Port].pr_mode == PR_MODE_ACTIVE1)
							mmPrint(Port, T_SRVAPP, "Audio completed, but Not Video - Wait to write into 3GP file\n");
						else {
	                        FormatTo3GP(Port); 				         
                        
  					        mmPrint(Port, T_SRVAPP, "Store to 3GP done\n");
  					        free(AudioCtx[Port].data_buffer);
  					        free(VideoCtx[Port].data_buffer);       
						} 
									
						if( VideoCtx[Port].pr_mode != PR_MODE_ACTIVE1 )
							last_rec_done = true;
																			
                        if (pevent->buffer)
                            total_missing_packets = *((DWORD *)pevent->buffer);

   			            mmPrint(Port, T_SRVAPP, "Context: %x, Reason: %x, Number of bytes recorded: %d, Number of missing RTP packets: %d\n", 
			                   pevent->ctahd, pevent->value, total_bytes_recorded, total_missing_packets);
			                    			                   
						if( mmParm.audioMode1 == NATIVE_SD ) {
			   	                CTA_EVENT           disable_event;
                                // Disable  Channel
                                ret = mspDisableChannel(AudioCtx[Port].FusionCh.hd);
	                            if (ret != SUCCESS)
	                            {
		                            mmPrintPortErr(Port, "mspDisableChannel returned %d.\n", ret);
		                            return;
	                            }
		                        ret = WaitForSpecificEvent(Port, pCfg->hCtaQueueHd, 
											MSPEVN_DISABLE_CHANNEL_DONE, &disable_event);
	                            if (ret != SUCCESS)
	                            {
		                            mmPrintPortErr(Port, "MSPEVN_DISABLE_CHANNEL_DONE event failure, ret= %x, event.value=%x",
                                             ret, disable_event.value);
		                            return;
	                            }
                                mmPrint(Port, T_SRVAPP, "Disable channel done...\n");
						}
						incAudRec( Port );
 						break;
					default:
						mmPrintAll("EVENT FOR UNKNOWN CONTEXT\n");
						return;
				}              
				
				if( gwParm.auto_hangup && last_rec_done ) {
					autoDisconnectCall( Port );								
				}				
				 
			}
			else {
				mmPrintAll("EVENT FOR UNKNOWN CONTEXT\n");
				return;
			}
			break;
		
	case ADIEVN_PLAY_DONE:
			mmPrint(Port, T_SRVAPP, "ADIEVN_PLAY_DONE is received...\n");

			mmPrint(Port, T_SRVAPP, "Context: %x, Reason: %x, Number of bytes played: %d\n", pevent->ctahd, pevent->value,
                    pevent->size&0x7FFFFFFF); /* ctaccess uses sign bit of size field, mask off */
			if(prCtahd != 0)
			{
				bool last_play_done = false;				
                CTA_EVENT  disable_event;
                DWORD      ret;
				
				switch (Type) {
					case 0: // Video
                        if(VideoCtx[Port].pr_mode == PR_MODE_ACTIVE1) {
                            VideoCtx[Port].pr_mode = PR_MODE_DONE1;
                        }
 
						mmPrint(Port, T_SRVAPP, "Playing was performed for Video Port# %d\n", Port);
						mmPrint(Port, T_SRVAPP, "%d bytes played from the file\n", pevent->size);
						
						if( AudioCtx[Port].pr_mode != PR_MODE_ACTIVE1 )
							last_play_done = true;
						free(VideoCtx[Port].data_buffer);
						incVidPlay( Port );
						break;
					case 1: // Audio
                        if(AudioCtx[Port].pr_mode == PR_MODE_ACTIVE1)
                        {
                            if(  mmParm.audioMode1 == FUS_IVR  )
		                    {
                                // Enable  Channel
                                ret = mspDisableChannel(AudioCtx[Port].FusionCh.hd);
	                            if (ret != SUCCESS)
	                            {
		                            mmPrintAll ("\nmspDisableChannel returned %d.", ret);
		                            return;
	                            }
		                        ret = WaitForSpecificEvent(Port, pCfg->hCtaQueueHd,
											MSPEVN_DISABLE_CHANNEL_DONE, &disable_event);
	                            if (ret != SUCCESS)
	                            {
		                            mmPrintAll ("\nMSPEVN_DISABLE_CHANNEL_DONE event failure, ret= %x, event.value=%x,"
                                            "\nboard error (event.size field) %x\n\n",
			                                ret, disable_event.value, disable_event.size);
		                            return;
	                            }
                                mmPrint(Port, T_MSPP, "Disable channel done...\n");
                            }
                            AudioCtx[Port].pr_mode = PR_MODE_DONE1;
                        }
						mmPrint(Port, T_SRVAPP, "Playing was performed for Audio Port# %d\n", Port);
						mmPrint(Port, T_SRVAPP, "%d bytes played from the file\n", pevent->size);
						
						if( VideoCtx[Port].pr_mode != PR_MODE_ACTIVE1 )
							last_play_done = true;

						free(AudioCtx[Port].data_buffer);
						incAudPlay( Port );
						break;			
						
					default:
						mmPrintAll("EVENT FOR UNKNOWN CONTEXT\n");
						return;
				}
				
				if( gwParm.auto_hangup && last_play_done && !mmParm.bAutoPlayList ) {
					autoDisconnectCall( Port );								
				}
				
               
                if( VideoCtx[Port].bPlayList && last_play_done) {
                    tVideo_Format fileFormat;
                    int idx;
                    
                    VideoCtx[Port].playList.currFile++;
                    idx = VideoCtx[Port].playList.currFile;
                    
                    if( idx < VideoCtx[Port].playList.numFiles ) {
                        // Play th next file from the PlayList
                        fileFormat = (tVideo_Format)getFileFormat( VideoCtx[Port].playList.fname[idx] );
                        if( fileFormat == -1 ) {
                            mmPrintPortErr( Port, "Illegal file name: %s\n", VideoCtx[Port].playList.fname[idx] );
                        } else {                     
                            PlayFromDB( Port, VideoCtx[Port].playList.fname[idx],
                                        fileFormat );
                        }
                    } else {
    				   mmPrint(Port, T_SRVAPP, "PlayList completed.\n" );       
                       VideoCtx[Port].bPlayList = FALSE;
                       VideoCtx[Port].playList.currFile = 0;     
                       VideoCtx[Port].playList.numFiles = 0;    
                       
                        if( gwParm.auto_hangup && mmParm.bAutoPlayList ) {
        					autoDisconnectCall( Port );								
		        		}
                                                     
                    }
                    
                } // end if bPlayList
			}
			else {
				mmPrintAll("EVENT FOR UNKNOWN CONTEXT\n");
				return;
			}		
				
		break;
		
	default:
		break;
	
	} // end switch
	
	return;
}
