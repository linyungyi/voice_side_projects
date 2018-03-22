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
#include "mmfi.h"


extern DWORD PlayFromDB( int nGw, char *fileName, tVideo_Format fileFormat );
extern int   getFileFormat( char *fileName );
extern tVideo_Format convertVideoCodec2Format( DWORD VideoCodec );
extern void get_video_format_string( DWORD format, char *str );
extern void get_audio_format_string(  tAudio_Format format, char *str );

int videoSubmitBufferWriteBuffer( int Port, void **ppBufferSubmit, unsigned  bufSizeSubmit,
								  void **ppBufWrite, unsigned bufSizeWrite );
int audioSubmitBufferWriteBuffer( int Port, void **ppBufferSubmit, unsigned  bufSizeSubmit,
								  void **ppBufWrite, unsigned bufSizeWrite );
int videoSubmitPlayBufferReadBuffer( int Port, void *pBufferSubmit, unsigned  bufSizeSubmit,
									 void *pBufRead, unsigned bufSizeRead );
int audioSubmitPlayBufferReadBuffer( int Port, void *pBufferSubmit, unsigned  bufSizeSubmit,
									 void *pBufRead, unsigned bufSizeRead );

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


void FormatTo3GP(DWORD Port, DWORD type)
{                          
    DWORD ret;
   
    mmPrint(Port, T_SRVAPP, "Writing data to the 3GP file %s\n",  VideoCtx[Port].threegp_FileName);

    switch (type)
    {
        case 0: //Video
            mmPrint(Port, T_SRVAPP, "Video Data to be written size %u\n",
                    VideoCtx[Port].data_size_r);
            ret = mmWriteStream(&VideoCtx[Port].mmStreamRec,
                         (BYTE *)VideoCtx[Port].data_buffer_r,
                                 VideoCtx[Port].data_size_r,
                                 NULL,
                                &VideoCtx[Port].byteCountRec,
                                 NULL,
                                 NULL);
            
            if (ret != SUCCESS)
            {
                mmPrintPortErr(Port, "Failed to Write Video stream, %x\n", ret);
            }
            mmPrint(Port, T_SRVAPP, "%u of video Data Written to the 3GP file %s\n",
                    VideoCtx[Port].byteCountRec ,VideoCtx[Port].threegp_FileName);
            break;
        case 1: //Audio
            mmPrint(Port, T_SRVAPP, "Audio Data to be written size %u\n",
                    AudioCtx[Port].data_size_r);
            ret = mmWriteStream(&AudioCtx[Port].mmStreamRec,
                         (BYTE *)AudioCtx[Port].data_buffer_r,
                                 AudioCtx[Port].data_size_r,
                                 NULL,
                                &AudioCtx[Port].byteCountRec,
                                 NULL,
                                 NULL);
            
            if (ret != SUCCESS)
            {
                mmPrintPortErr(Port, "Failed to Write Audio stream, %x\n", ret);
            }
            mmPrint(Port, T_SRVAPP, "%u of audio Data Written to the 3GP file %s\n", 
                    AudioCtx[Port].byteCountRec ,VideoCtx[Port].threegp_FileName);
            break;
        default:
            mmPrintPortErr(Port, "Event for unknown type, %x\n", type);
            break;
    }
}
  
void ProcessADIEvent( CTA_EVENT *pevent )
{
    CTAHD                   prCtahd;
    DWORD                   total_missing_packets = 0;
    DWORD                   total_bytes_recorded  = 0;
    DWORD                   Port, Type, ret;
    MEDIA_GATEWAY_CONFIG   *pCfg;

    prCtahd = FindContext(pevent->ctahd, &Port, &Type);    
    pCfg    = &GwConfig[Port];
    
    switch (pevent->id)
    {
        case ADIEVN_RECORD_DONE:
            mmPrint(Port, T_SRVAPP, "ADIEVN_RECORD_DONE is received...\n");
            
            /* beh - sign bit of size is set if a buffer is delivered in event, must mask off sign bit for actual size*/
            if (pevent->size & CTA_INTERNAL_BUFFER )
            {
                 pevent->size &= ~ CTA_INTERNAL_BUFFER;
            }
            total_bytes_recorded = pevent->size;
            
            if (prCtahd != 0)
            {
                bool last_rec_done = false;            
                
                switch (Type)
                {        
                    case 0: // Video
                        mmPrint(Port, T_SRVAPP, "Recording was performed for Video Port# %d\n", Port);

                        if (VideoCtx[Port].pr_mode_r == PR_MODE_ACTIVE1)
                        {
                            VideoCtx[Port].pr_mode_r = PR_MODE_DONE1;
                        } 

                        VideoCtx[Port].data_size_r = total_bytes_recorded;
                        
                        if (!mmParm.bPartialBuffers)
                        {
                            FormatTo3GP(Port, Type);
                        }

                        mmPrint(Port, T_SRVAPP, "Store to 3GP done\n");
                           
                        if (VideoCtx[Port].data_buffer_r != NULL)
                        {
                            free(VideoCtx[Port].data_buffer_r);
                            VideoCtx[Port].data_buffer_r = NULL;
                        }
                            
                        if (VideoCtx[Port].addDataBufferRec1 != NULL)
                        {
                            free(VideoCtx[Port].addDataBufferRec1);
                            VideoCtx[Port].addDataBufferRec1 = NULL;
                        }
                            
                        if (VideoCtx[Port].addDataBufferRec2 != NULL)
                        {
                            free(VideoCtx[Port].addDataBufferRec2);
                            VideoCtx[Port].addDataBufferRec2 = NULL;
                        }

                        VideoCtx[Port].bRecDone = true;
                                            
                        if (AudioCtx[Port].pr_mode_r != PR_MODE_ACTIVE1)
                        {
                            last_rec_done = true;
                        }

                        if (pevent->buffer)
                        {
                            total_missing_packets = *((DWORD *)pevent->buffer);
                        }
                        
                        mmPrint(Port, T_SRVAPP, "Context: %x, Reason: %x, Number of bytes recorded: %d, Number of missing RTP packets: %d\n", 
                                pevent->ctahd, pevent->value, total_bytes_recorded, total_missing_packets);
                        
                        incrVideRec(Port); 
                        break;
                    
                    case 1: // Audio
                        mmPrint(Port, T_SRVAPP, "Recording was performed for Audio Port# %d\n", Port);

                        if (AudioCtx[Port].pr_mode_r == PR_MODE_ACTIVE1)
                        {
                            AudioCtx[Port].pr_mode_r = PR_MODE_DONE1;
                        }
                  
                        AudioCtx[Port].data_size_r = total_bytes_recorded;

                        if (!mmParm.bPartialBuffers)
                        {
                            FormatTo3GP(Port, Type);
                        }
                            
                        mmPrint(Port, T_SRVAPP, "Store to 3GP done\n");
                        if (AudioCtx[Port].data_buffer_r != NULL)
                        {
                            free(AudioCtx[Port].data_buffer_r);
                            AudioCtx[Port].data_buffer_r = NULL;
                        }
                            
                        if (AudioCtx[Port].addDataBufferRec1 != NULL)
                        {
                            free(AudioCtx[Port].addDataBufferRec1);
                            AudioCtx[Port].addDataBufferRec1 = NULL;
                        }
                            
                        if (AudioCtx[Port].addDataBufferRec2 != NULL)
                        {
                            free(AudioCtx[Port].addDataBufferRec2);
                            AudioCtx[Port].addDataBufferRec2 = NULL;
                        }

                        AudioCtx[Port].bRecDone = true;
                                    
                        if (VideoCtx[Port].pr_mode_r != PR_MODE_ACTIVE1)
                        {
                            last_rec_done = true;
                        }
                                                                            
                        if (pevent->buffer)
                        {
                            total_missing_packets = *((DWORD *)pevent->buffer);
                        }

                        mmPrint(Port, T_SRVAPP, "Context: %x, Reason: %x, Number of bytes recorded: %d, Number of missing RTP packets: %d\n", 
                                pevent->ctahd, pevent->value, total_bytes_recorded, total_missing_packets);
                                                               
                        if (mmParm.audioMode1 == NATIVE_SD)
                        {
                            if (pCfg->shutdown_in_progress == FALSE)
                            {
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
                        }
                        incAudRec( Port );
                         break;
                    default:
                        mmPrintAll("EVENT FOR UNKNOWN CONTEXT\n");
                        return;
                }              
                
                if ((VideoCtx[Port].bRecDone == true) && (AudioCtx[Port].bRecDone == true))
                {
                    ret = mmCloseFile(&VideoCtx[Port].mmFileRec);
                    if (ret != SUCCESS)
                    {
                        mmPrintPortErr(Port, "Failed to Close the MM file, %x\n", ret);
                        exit(-1);
                    }
                    VideoCtx[Port].bRecDone = false;
                    AudioCtx[Port].bRecDone = false;
                }
                
                if (gwParm.auto_hangup && last_rec_done)
                {
                    autoDisconnectCall( Port );                                
                }                
                 
            }
            else
            {
                mmPrintAll("EVENT FOR UNKNOWN CONTEXT\n");
                return;
            }
            break;
        
        case ADIEVN_PLAY_DONE:
            mmPrint(Port, T_SRVAPP, "ADIEVN_PLAY_DONE is received...\n");

            mmPrint(Port, T_SRVAPP, "Context: %x, Reason: %x, Number of bytes played: %d\n", pevent->ctahd, pevent->value,
                    pevent->size&0x7FFFFFFF); /* ctaccess uses sign bit of size field, mask off */
            if (prCtahd != 0)
            {
                bool       last_play_done = false;                
                CTA_EVENT  disable_event;
                DWORD      ret;
                
                switch (Type)
                {
                    case 0: // Video
                        if(VideoCtx[Port].pr_mode_p == PR_MODE_ACTIVE1)
                        {
                            VideoCtx[Port].pr_mode_p = PR_MODE_DONE1;
                        }
 
                        mmPrint(Port, T_SRVAPP, "Playing was performed for Video Port# %d\n", Port);
                        mmPrint(Port, T_SRVAPP, "%d bytes played from the file\n", pevent->size);
                        
                        if (AudioCtx[Port].pr_mode_p != PR_MODE_ACTIVE1)
                        {
                            last_play_done = true;
                        }
                        
                        if (VideoCtx[Port].data_buffer_p != NULL)
                        {
                            free(VideoCtx[Port].data_buffer_p);
                            VideoCtx[Port].data_buffer_p = NULL;
                        }
                        
                        if (VideoCtx[Port].addDataBufferPlay != NULL)
                        {
                            free(VideoCtx[Port].addDataBufferPlay);
                            VideoCtx[Port].addDataBufferPlay = NULL;
                        }

                        VideoCtx[Port].bPlayDone = true;

                        incVidPlay( Port );
                        break;
                    case 1: // Audio
                        if (AudioCtx[Port].pr_mode_p == PR_MODE_ACTIVE1)
                        {
                            if (mmParm.audioMode1 == FUS_IVR)
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
                            
                            if(AudioCtx[Port].pr_mode_p == PR_MODE_ACTIVE1)
                            {
                                AudioCtx[Port].pr_mode_p = PR_MODE_DONE1;
                            }
                        }
                        AudioCtx[Port].data_size_p += pevent->size;
                        
                        mmPrint(Port, T_SRVAPP, "Playing was performed for Audio Port# %d\n", Port);
                        mmPrint(Port, T_SRVAPP, "%d bytes played from the file\n", pevent->size);
                        
                        if (VideoCtx[Port].pr_mode_p != PR_MODE_ACTIVE1)
                        {
                            last_play_done = true;
                        }

                        if (AudioCtx[Port].data_buffer_p != NULL)
                        {
                            free(AudioCtx[Port].data_buffer_p);
                            AudioCtx[Port].data_buffer_p = NULL;
                        }
                        
                        if (AudioCtx[Port].addDataBufferPlay != NULL)
                        {
                            free(AudioCtx[Port].addDataBufferPlay);
                            AudioCtx[Port].addDataBufferPlay = NULL;
                        }

                        AudioCtx[Port].bPlayDone = true;
                        incAudPlay( Port );
                        break;            
                        
                    default:
                        mmPrintAll("EVENT FOR UNKNOWN CONTEXT\n");
                        return;
                }
                
                if ((VideoCtx[Port].bPlayDone == true) && (AudioCtx[Port].bPlayDone == true))
                {
                    ret = mmCloseFile(&VideoCtx[Port].mmFilePlay);
                    if (ret != SUCCESS)
                    {
                        mmPrintPortErr(Port, "Failed to Close the MM file, %x\n", ret);
                        exit(-1);
                    }
                    VideoCtx[Port].bPlayDone = false;
                    AudioCtx[Port].bPlayDone = false;
                }

                if (gwParm.auto_hangup && last_play_done && !mmParm.bAutoPlayList)
                {
                    autoDisconnectCall( Port );                                
                }
                
               
                if (VideoCtx[Port].bPlayList && last_play_done)
                {
                    tVideo_Format fileFormat;
                    int           idx;
                    
                    VideoCtx[Port].playList.currFile++;
                    idx = VideoCtx[Port].playList.currFile;
                    
                    if (idx < VideoCtx[Port].playList.numFiles)
                    {
                        // Play th next file from the PlayList
                        fileFormat = (tVideo_Format)getFileFormat( VideoCtx[Port].playList.fname[idx] );
                        if (fileFormat == -1)
                        {
                            mmPrintPortErr( Port, "Illegal file name: %s\n",
                                            VideoCtx[Port].playList.fname[idx] );
                        }
                        else
                        {                     
                            PlayFromDB( Port, VideoCtx[Port].playList.fname[idx],
                                        fileFormat );
                        }
                    }
                    else
                    {
                       mmPrint(Port, T_SRVAPP, "PlayList completed.\n" );       
                       VideoCtx[Port].bPlayList         = FALSE;
                       VideoCtx[Port].playList.currFile = 0;     
                       VideoCtx[Port].playList.numFiles = 0;    
                       
                       if (gwParm.auto_hangup && mmParm.bAutoPlayList)
                       {
                           autoDisconnectCall( Port );                                
                       }
                                                     
                    }
                } // end if bPlayList
            }
            else
            {
                mmPrintAll("EVENT FOR UNKNOWN CONTEXT\n");
                return;
            }        
            break;

        case ADIEVN_PLAY_BUFFER_REQ:
            
            mmPrint(Port, T_SRVAPP, "ADIEVN_PLAY_BUFFER_REQ is received... Type=%d\n", Type);
            if (prCtahd != 0)
            {
                switch (Type)
                {
                    case 0: // Video
                        if (VideoCtx[Port].bUseAddPlayBuffer == true)
                        {
							// Submit a next buffer for playing and continue reading a stream
							// from the 3GP file
							videoSubmitPlayBufferReadBuffer(  Port, VideoCtx[Port].addDataBufferPlay,
															 VideoCtx[Port].byteCountPlay,
															 VideoCtx[Port].data_buffer_p,
															 VideoCtx[Port].bufSizePlay );	
															 						
							VideoCtx[Port].bUseAddPlayBuffer = false;
                        }
                        else if (VideoCtx[Port].bUseAddPlayBuffer == false)
                        {						
							// Submit a next buffer for playing and continue reading a stream
							// from the 3GP file						
							videoSubmitPlayBufferReadBuffer(  Port, VideoCtx[Port].data_buffer_p,
															 VideoCtx[Port].byteCountPlay,
															 VideoCtx[Port].addDataBufferPlay,
															 VideoCtx[Port].bufSizePlay );	
							VideoCtx[Port].bUseAddPlayBuffer = true;
                        }
                        else
                        {
                            mmPrintPortErr(Port, "No buffer freed!\n");
                            return;
                        }
                        
                        break;
                    
                    case 1: // Audio
                        if (AudioCtx[Port].bUseAddPlayBuffer == true)
                        {
							// Submit a next buffer for playing and continue reading a stream
							// from the 3GP file						
							audioSubmitPlayBufferReadBuffer(  Port, AudioCtx[Port].addDataBufferPlay,
															 AudioCtx[Port].byteCountPlay,
															 AudioCtx[Port].data_buffer_p,
															 AudioCtx[Port].bufSizePlay );	

							AudioCtx[Port].bUseAddPlayBuffer = false;                            
                        }
                        else if (AudioCtx[Port].bUseAddPlayBuffer == false)
                        {
							// Submit a next buffer for playing and continue reading a stream
							// from the 3GP file						
							audioSubmitPlayBufferReadBuffer(  Port, AudioCtx[Port].data_buffer_p,
															 AudioCtx[Port].byteCountPlay,
															 AudioCtx[Port].addDataBufferPlay,
															 AudioCtx[Port].bufSizePlay );	
													
							AudioCtx[Port].bUseAddPlayBuffer = true;
                        }
                        else
                        {
                            mmPrintPortErr(Port, "No buffer freed!\n");
                            return;
                        }
                        break;            
                        
                    default:
                        mmPrintAll("EVENT FOR UNKNOWN CONTEXT\n");
                        return;
                }
            }
            else
            {
                mmPrintAll("EVENT FOR UNKNOWN CONTEXT\n");
                return;
            }        
            break;
			
        case ADIEVN_RECORD_BUFFER_FULL:
            mmPrint(Port, T_SRVAPP, "ADIEVN_RECORD_BUFFER_FULL is received...\n");
            if (prCtahd != 0)
            {
                switch (Type)
                {
                    case 0: // Video
                        mmPrint(Port, T_SRVAPP, "Video size : %u\n", pevent->size);
						
                        if (VideoCtx[Port].data_buffer_r == NULL)
                        {
                            // Submit data_buffer_r for recording, write to 3GP addDataBufferRec1
                            videoSubmitBufferWriteBuffer( Port, &VideoCtx[Port].data_buffer_r,
                                                          VideoCtx[Port].bufSizeRec,
                                                          &VideoCtx[Port].addDataBufferRec1,
                                                          pevent->size );								
                        }
                        else if (VideoCtx[Port].addDataBufferRec1 == NULL)
                        {
                            // Submit addDataBufferRec1 for recording, write to 3GP addDataBufferRec2
                            videoSubmitBufferWriteBuffer( Port, &VideoCtx[Port].addDataBufferRec1,
                                                          VideoCtx[Port].bufSizeRec,
                                                          &VideoCtx[Port].addDataBufferRec2,
                                                          pevent->size );								                            
                        }
                        else if (VideoCtx[Port].addDataBufferRec2 == NULL)
                        {
                            // Submit addDataBufferRec2 for recording, write to 3GP data_buffer_r
                            videoSubmitBufferWriteBuffer( Port, &VideoCtx[Port].addDataBufferRec2,
                                                          VideoCtx[Port].bufSizeRec,
                                                          &VideoCtx[Port].data_buffer_r,
                                                          pevent->size );								                            
                        }
                        else
                        {
                            mmPrintPortErr(Port, "No video buffer available!\n");
                            return;
                        }
							
                        VideoCtx[Port].data_size_r += pevent->size;							
						
						// This buffer will be freed by the application in videoSubmitBufferWriteBuffer
						// By setting it to NULL we prevent the double-freeing of this buffer by ctaFreeBuffer
						pevent->buffer = NULL;
						
                        break;
                    
                    case 1: // Audio
                        mmPrint(Port, T_SRVAPP, "Audio size : %u\n", pevent->size);
						
                        if (AudioCtx[Port].data_buffer_r == NULL)
                        {
                            // Submit data_buffer_r for recording, write to 3GP addDataBufferRec1
                            audioSubmitBufferWriteBuffer( Port, &AudioCtx[Port].data_buffer_r,
                                                          AudioCtx[Port].bufSizeRec,
                                                          &AudioCtx[Port].addDataBufferRec1,
                                                          pevent->size );								
                        }
                        else if (AudioCtx[Port].addDataBufferRec1 == NULL)
                        {
                            // Submit addDataBufferRec1 for recording, write to 3GP addDataBufferRec2
                            audioSubmitBufferWriteBuffer( Port, &AudioCtx[Port].addDataBufferRec1,
                                                          AudioCtx[Port].bufSizeRec,
                                                          &AudioCtx[Port].addDataBufferRec2,
                                                          pevent->size );								
                        }
                        else if (AudioCtx[Port].addDataBufferRec2 == NULL)
                        {
                            // Submit addDataBufferRec2 for recording, write to 3GP data_buffer_r
                            audioSubmitBufferWriteBuffer( Port, &AudioCtx[Port].addDataBufferRec2,
                                                          AudioCtx[Port].bufSizeRec,
                                                          &AudioCtx[Port].data_buffer_r,
                                                          pevent->size );								
                        }
                        else
                        {
                            mmPrintPortErr(Port, "No audio buffer available!\n");
                            return;
                        }
							
                        AudioCtx[Port].data_size_r += pevent->size;							
						
						// This buffer will be freed by the application in videoSubmitBufferWriteBuffer
						// By setting it to NULL we prevent the double-freeing of this buffer by ctaFreeBuffer
						pevent->buffer = NULL;

                        break;            
                        
                    default:
                        mmPrintAll("EVENT FOR UNKNOWN CONTEXT\n");
                        return;
                }
            }
            else
            {
                mmPrintAll("EVENT FOR UNKNOWN CONTEXT\n");
                return;
            }        
            break;
        
        case ADIEVN_RECORD_STARTED:
            mmPrint(Port, T_SRVAPP, "ADIEVN_RECORD_STARTED is received...\n");
            if (prCtahd != 0)
            {
                if ((pevent->value == ADI_RECORD_BUFFER_REQ) &&
                    (mmParm.bPartialBuffers == TRUE))
                {
                    switch (Type)
                    {
                        case 0: // Video
                            //Allocating memory for the video buffer
                            if ((VideoCtx[Port].addDataBufferRec1 = (void *)malloc(VideoCtx[Port].bufSizeRec)) == NULL)
                            {
                                mmPrintPortErr(Port,"Native Video record Buffer:failed malloc of %d bytes for port %d\n", 
                                               VideoCtx[Port].bufSizeRec, Port);
                                exit(-1);
                            }

                            ret = adiSubmitRecordBuffer(VideoCtx[Port].ctahd, 
                                                        VideoCtx[Port].addDataBufferRec1, 
                                                        VideoCtx[Port].bufSizeRec);
                            
                            if (ret != SUCCESS)
                            {
                                mmPrintPortErr(Port, "Failed to submit Video Buffer, %x\n", ret);
                                exit(-1);
                            }
                            
                            break;

                        case 1: // Audio
                            //Allocating memory for the audio buffer
                            if ((AudioCtx[Port].addDataBufferRec1 = (void *)malloc(AudioCtx[Port].bufSizeRec)) == NULL)
                            {
                                mmPrintPortErr(Port,"Native Audio record Buffer:failed malloc of %d bytes for port %d\n", 
                                               AudioCtx[Port].bufSizeRec, Port);
                                exit(-1);
                            }
                            
                            ret = adiSubmitRecordBuffer(AudioCtx[Port].ctahd, 
                                                        AudioCtx[Port].addDataBufferRec1, 
                                                        AudioCtx[Port].bufSizeRec);
                            if (ret != SUCCESS)
                            {
                                mmPrintPortErr(Port, "Failed to submit Audio Buffer, %x\n", ret);
                                exit(-1);
                            }
                            break;            
                            
                        default:
                            mmPrintAll("EVENT FOR UNKNOWN CONTEXT\n");
                            return;
                    }
                }
            }
            else
            {
                mmPrintAll("EVENT FOR UNKNOWN CONTEXT\n");
                return;
            }
            VideoCtx[Port].data_size_r = 0;
            AudioCtx[Port].data_size_r = 0;
            break;
        
        default:
            mmPrint(Port, T_SRVAPP, "Context: %x, Reason: %x\n", pevent->ctahd, pevent->value);
            break;
    
    } // end switch
    
    if (VideoCtx[Port].playuii == 1)
    {
        if (mmParm.Operation == SIM_PR)
        {
            if((AudioCtx[Port].pr_mode_p != PR_MODE_ACTIVE1) &&
               (VideoCtx[Port].pr_mode_p != PR_MODE_ACTIVE1) &&
               (AudioCtx[Port].pr_mode_r != PR_MODE_ACTIVE1) &&
               (VideoCtx[Port].pr_mode_r != PR_MODE_ACTIVE1))
            {
                tVideo_Format fileFormat;
                
                VideoCtx[Port].playuii = 0;
                fileFormat             = (tVideo_Format)getFileFormat( VideoCtx[Port].threegp_FileName );
                
                PlayFromDB( Port, VideoCtx[Port].threegp_FileName, fileFormat );
            }
        }
        if(mmParm.Operation == RECORD)
        {
            if((AudioCtx[Port].pr_mode_r != PR_MODE_ACTIVE1) &&
               (VideoCtx[Port].pr_mode_r != PR_MODE_ACTIVE1))
            {
             
                tVideo_Format fileFormat;
                
                VideoCtx[Port].playuii = 0;
                fileFormat             = (tVideo_Format)getFileFormat( VideoCtx[Port].threegp_FileName );
                
                PlayFromDB( Port, VideoCtx[Port].threegp_FileName, fileFormat );
            }
        }
    }

    return;
}

int videoSubmitBufferWriteBuffer( int Port, void **ppBufferSubmit, unsigned  bufSizeSubmit, void **ppBufWrite, unsigned bufSizeWrite )
{
	DWORD ret;


	*ppBufferSubmit = (void *)malloc( bufSizeSubmit );
	if( *ppBufferSubmit == NULL )
	{
		mmPrintPortErr(Port,"videoSubmitBufferWriteBuffer: malloc failed!\n");
		exit(-1);	
	}

    if( VideoCtx[Port].bStopRecordCalled == true )
	{
		mmPrint(Port, T_SRVAPP, "Video recording has been stopped. Don't submit next video buffer\n");											
	}
	else
	{
		ret = adiSubmitRecordBuffer(VideoCtx[Port].ctahd, *ppBufferSubmit, bufSizeSubmit );                              
		if (ret != SUCCESS)
		{
			mmPrintPortErr(Port, "Failed to submit Video Buffer %x\n", ret);
	        exit(-1);
		}
    }
	              
	ret = mmWriteStream( &VideoCtx[Port].mmStreamRec, (BYTE *)*ppBufWrite, bufSizeWrite, 
                         NULL, &VideoCtx[Port].byteCountRec, NULL,  NULL);                                
    if (ret != SUCCESS)
    {
		mmPrintPortErr(Port, "Failed to Write Video stream, %d\n", ret);
	    free( *ppBufWrite );
		*ppBufWrite = NULL;		
		return -1;
    }

    mmPrint(Port, T_SRVAPP, "%u of video Data Written to the 3GP file %s\n",
            VideoCtx[Port].byteCountRec ,VideoCtx[Port].threegp_FileName);

    free( *ppBufWrite );
	*ppBufWrite = NULL;
	
	return 0;
}

int audioSubmitBufferWriteBuffer( int Port, void **ppBufferSubmit, unsigned  bufSizeSubmit, void **ppBufWrite, unsigned bufSizeWrite )
{
	DWORD ret;

	*ppBufferSubmit = (void *)malloc( bufSizeSubmit );
	if( *ppBufferSubmit == NULL )
	{
		mmPrintPortErr(Port,"audioSubmitBufferWriteBuffer: malloc failed!\n");
		exit(-1);	
	}

	if( AudioCtx[Port].bStopRecordCalled == true )
	{
		mmPrint(Port, T_SRVAPP, "Audio recording has been stopped. Don't submit next audio buffer\n");											
	}
	else
	{
		ret = adiSubmitRecordBuffer(AudioCtx[Port].ctahd, *ppBufferSubmit, bufSizeSubmit );                              
		if (ret != SUCCESS)
		{
			mmPrintPortErr(Port, "Failed to submit Audio Buffer %x\n", ret);
    	    exit(-1);
		}
	}             
				   
	ret = mmWriteStream( &AudioCtx[Port].mmStreamRec, (BYTE *)*ppBufWrite, bufSizeWrite, 
                         NULL, &AudioCtx[Port].byteCountRec, NULL,  NULL);                                
    if (ret != SUCCESS)
    {
		mmPrintPortErr(Port, "Failed to Write Audio stream, %x\n", ret);
	    free( *ppBufWrite );
		*ppBufWrite = NULL;		
		return -1;
    }

    mmPrint(Port, T_SRVAPP, "%u of audio Data Written to the 3GP file %s\n",
            AudioCtx[Port].byteCountRec, VideoCtx[Port].threegp_FileName);

    free( *ppBufWrite );
	*ppBufWrite = NULL;
	
	return 0;
}

int videoSubmitPlayBufferReadBuffer( int Port, void *pBufferSubmit, unsigned  bufSizeSubmit,
									 void *pBufRead, unsigned bufSizeRead )
{
	DWORD ret;

	if( VideoCtx[Port].bStopPlayCalled == true )
	{
		mmPrint(Port, T_SRVAPP, "Video playing has been stopped. Don't submit/read next video buffer\n");											
		return 0;
	}

	if (VideoCtx[Port].bLastBuffer == true)
    {
    	ret = adiSubmitPlayBuffer(VideoCtx[Port].ctahd,
                                  pBufferSubmit, bufSizeSubmit,
                                  ADI_PLAY_LAST_BUFFER);
		if (ret != SUCCESS)
        {
        	mmPrintPortErr(Port, "Failed to submit last Video Buffer, %x\n", ret);
            exit(-1);
        }
        VideoCtx[Port].data_size_p += bufSizeSubmit;
        VideoCtx[Port].bLastBuffer   = false;
	}
    else
	{
        ret = adiSubmitPlayBuffer(VideoCtx[Port].ctahd,
    	                          pBufferSubmit, bufSizeSubmit,
                                  NULL);
        if (ret != SUCCESS)
        {
	        mmPrintPortErr(Port, "Failed to submit Video Buffer, %x size, %d\n", ret, bufSizeSubmit);
            exit(-1);
        }

        VideoCtx[Port].data_size_p += bufSizeSubmit;

        ret = mmReadStream(&VideoCtx[Port].mmStreamPlay, 
                           SAMPLE_COUNT_MAX,
		                   (BYTE *)pBufRead, bufSizeRead,
                           &VideoCtx[Port].byteCountPlay);
        if (ret == MMERR_END_OF_STREAM)
        {
	        mmPrint(Port, T_SRVAPP, "End of Video Stream reached...\n");
            VideoCtx[Port].bLastBuffer   = true;
        }
        else if (ret != SUCCESS)
        {
	        mmPrintPortErr(Port, "Failed to Read Video stream, %x\n", ret);
            return -1;
        }
	}

	return 0;
}

int audioSubmitPlayBufferReadBuffer( int Port, void *pBufferSubmit, unsigned  bufSizeSubmit,
									 void *pBufRead, unsigned bufSizeRead )
{
	DWORD ret;

	if( AudioCtx[Port].bStopPlayCalled == true )
	{
		mmPrint(Port, T_SRVAPP, "Audio playing has been stopped. Don't submit/read next audio buffer\n");											
		return 0;
	}

	if (AudioCtx[Port].bLastBuffer == true)
    {
    	ret = adiSubmitPlayBuffer(AudioCtx[Port].ctahd,
                                  pBufferSubmit, bufSizeSubmit,
                                  ADI_PLAY_LAST_BUFFER);
		if (ret != SUCCESS)
        {
        	mmPrintPortErr(Port, "Failed to submit last Audio Buffer, %x\n", ret);
            exit(-1);
        }
        AudioCtx[Port].data_size_p += bufSizeSubmit;
        AudioCtx[Port].bLastBuffer   = false;
	}
    else
	{
        ret = adiSubmitPlayBuffer(AudioCtx[Port].ctahd,
    	                          pBufferSubmit, bufSizeSubmit,
                                  NULL);
        if (ret != SUCCESS)
        {
	        mmPrintPortErr(Port, "Failed to submit Audio Buffer, %x size, %d\n", ret, bufSizeSubmit);
            exit(-1);
        }

        AudioCtx[Port].data_size_p += bufSizeSubmit;

        ret = mmReadStream(&AudioCtx[Port].mmStreamPlay, 
                           SAMPLE_COUNT_MAX,
		                   (BYTE *)pBufRead, bufSizeRead,
                           &AudioCtx[Port].byteCountPlay);
        if (ret == MMERR_END_OF_STREAM)
        {
	        mmPrint(Port, T_SRVAPP, "End of Audio Stream reached...\n");
            AudioCtx[Port].bLastBuffer   = true;
        }
        else if (ret != SUCCESS)
        {
	        mmPrintPortErr(Port, "Failed to Read Audio stream, %x\n", ret);
            return -1;
        }
	}

	return 0;
}
