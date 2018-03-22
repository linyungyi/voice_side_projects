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
#include "mmfi.h"
#include "gw_types.h"

extern void get_video_format_string( DWORD format, char *str );
extern void get_audio_format_string( tAudio_Format format, char *str );

static DWORD getADIvidEncoder( tVideo_Format fileFormat );
static WORD getNewVideoCodecDef(int port);
static WORD getNewAudioCodecDef(int port);


//Valli --Porting ALS Dyn TRc chanl setup
extern DWORD configurePlayWithoutXc( int nGw );
extern DWORD configurePlayWithXc( int nGw );
extern tVideo_Format convertVideoCodec2Format( DWORD VideoCodec );

#ifdef USE_TRC
// dynamic VTP channel
int     getVideoChannelStatus(TRC_HANDLE trchd);
DWORD   stopVideoChannel(int dGwIndex);
DWORD   startVideoChannel(int dGwIndex);
char	getNewVideoType(WORD codec);

// resolution support
DWORD   getFileRes(char *fileName);
void	get_video_res_string(DWORD FrameRes, char *str);
char   getFrameResType(WORD wWidth, WORD wHeight);
char   getFrameRes(FILE_INFO_DESC *pFileInfoDesc); 
DWORD   get3gpFileRes(char *fileName, MMFILE *pMMFile, FILE_INFO_DESC *pFileInfoDesc);

DWORD   configVidChan(int port, char fileVidType, char fileFrameRes);

#ifndef TRC_FRAME_RES_UNKNOWN
#define TRC_FRAME_RES_UNKNOWN  (-1)
#endif

#endif

/////////////////////////////////////////////////////////////////////////////////
//  PlayRecord()
//  Main Function to start playing or recording
/////////////////////////////////////////////////////////////////////////////////
DWORD PlayRecord( int port, tVideo_Format fileformat )
{
    ADI_MM_RECORD_PARMS     recParms;
    DWORD                   i,
                            ret;
    unsigned                /*Encoding,*/
                            tempVideoByteCount,
                            tempAudioByteCount;
    unsigned                flags;
    DWORD                   audioEP_filterhd,
                            videoEP_filterhd;
    CTA_EVENT               event;
    MSPHD                   epHd;
    tAudioMode              audioMode;
    DWORD                   ADIaudEncoder;
    tVideo_Format fileformat_r;
    fileformat_r = convertVideoCodec2Format(GwConfig[port].txVideoCodec);
    DWORD ADIvidEncoder;
    msp_FILTER_ENCODE_CMD   FilterEncodeCmd;
    msp_FILTER_ENCODE_CMD  *pFilterEncodeCmd = 0;
    char                    threegp_FileName_p[80];
    char                    threegp_FileName_r[80];

    WORD                    streamCount,
                            j,
                            infoSizeLeft;       /* size left in file info buffer */
    FILE_INFO_PRESENTATION *pPresentation;      /* pointer on Presentation block */
    FILE_INFO_STREAM_AUDIO *pStreamHeader;      /* pointer on Stream Header      */
    int                     blockSize;          /* size of current block         */
    BYTE                    FileInfo [MAX_FILE_INFO_SIZE];
    BYTE                   *pFileInfo;

    ADIvidEncoder = getADIvidEncoder( fileformat_r ); 
   
    if (((VideoCtx[port].pr_mode_r == PR_MODE_ACTIVE1) ||
         (AudioCtx[port].pr_mode_r == PR_MODE_ACTIVE1)) &&
        ((mmParm.Operation == RECORD) || (mmParm.Operation == SIM_PR)))
    {
        mmPrintPortErr (port, " Record Operation - Aready active\n");
        return FAILURE;
    }

    if (((VideoCtx[port].pr_mode_p == PR_MODE_ACTIVE1) || 
         (AudioCtx[port].pr_mode_p == PR_MODE_ACTIVE1)) &&
        ((mmParm.Operation == PLAY) || (mmParm.Operation == SIM_PR)))
    {
        mmPrintPortErr (port, " Play Operation - Aready active\n");
        return FAILURE;
    }

    mmParm.ADIvidEncoder = getADIvidEncoder(fileformat);
    if(mmParm.ADIvidEncoder < 0)
    {
        mmPrintPortErr (port, "File Format <0x%x> not supported for Play/Rec operation\n", fileformat);
        return FAILURE;
    }

    pFilterEncodeCmd        = &FilterEncodeCmd;
    pFilterEncodeCmd->value = 0;
    i                       = port; 
     
    if (mmParm.Video_Format != NO_VIDEO)
    {
        // Get Video EP Filter Handle
        ret = mspGetFilterHandle (VideoCtx[i].rtpEp.hd, MSP_ENDPOINT_RTPFDX_VIDEO, &videoEP_filterhd);
        if (ret != SUCCESS)
        {
            mmPrintPortErr (port, "Failed to get Video EP Filter Handle, %x\n", ret);
            return FAILURE;
        }
            
    }  // END If there is a video channel

    if (mmParm.audioMode1 != NONE)
    {
        // Get Audio EP Filter Handle
        epHd = AudioCtx[i].rtpEp.hd;
        ret  = mspGetFilterHandle (epHd, MSP_ENDPOINT_RTPFDX, &audioEP_filterhd);
        
        if (ret != SUCCESS)
        {
            mmPrintPortErr(i, "Failed to get Audio EP Filter Handle, %x\n", ret);
            return FAILURE;
        }
    }  // END If there is an Audio Channel

    if (mmParm.Operation == RECORD)
    { 
        VideoCtx[i].pr_mode_r = PR_MODE_ACTIVE1;
        AudioCtx[i].pr_mode_r = PR_MODE_ACTIVE1;
        strcpy (threegp_FileName_r, VideoCtx[i].threegp_FileName);
		AudioCtx[i].bRecDone    = false;
		VideoCtx[i].bRecDone    = false;		
		AudioCtx[i].bStopRecordCalled  = false;
		VideoCtx[i].bStopRecordCalled  = false;			
    }
    if (mmParm.Operation == PLAY)
    {
        VideoCtx[i].pr_mode_p = PR_MODE_ACTIVE1;
        AudioCtx[i].pr_mode_p = PR_MODE_ACTIVE1;
        strcpy (threegp_FileName_p, VideoCtx[i].threegp_FileName);
		AudioCtx[i].bPlayDone   = false;
		AudioCtx[i].bLastBuffer = false;			
		VideoCtx[i].bPlayDone   = false;
		VideoCtx[i].bLastBuffer = false;											
		AudioCtx[i].bUseAddPlayBuffer = false;			
		VideoCtx[i].bUseAddPlayBuffer = false;						
		AudioCtx[i].bStopPlayCalled   = false;
		VideoCtx[i].bStopPlayCalled   = false;						
    }
    if (mmParm.Operation == SIM_PR)
    {
        VideoCtx[i].pr_mode_p = PR_MODE_ACTIVE1;
        AudioCtx[i].pr_mode_p = PR_MODE_ACTIVE1;
        VideoCtx[i].pr_mode_r = PR_MODE_ACTIVE1;
        AudioCtx[i].pr_mode_r = PR_MODE_ACTIVE1;
        strcpy (threegp_FileName_p, VideoCtx[i].threegp_FileName_play);
        strcpy (threegp_FileName_r, VideoCtx[i].threegp_FileName);
		AudioCtx[i].bRecDone    = false;
		AudioCtx[i].bPlayDone   = false;
		AudioCtx[i].bLastBuffer = false;			
		VideoCtx[i].bRecDone    = false;
		VideoCtx[i].bPlayDone   = false;
		VideoCtx[i].bLastBuffer = false;											
		AudioCtx[i].bStopRecordCalled  = false;
		VideoCtx[i].bStopRecordCalled  = false;						
		AudioCtx[i].bUseAddPlayBuffer = false;			
		VideoCtx[i].bUseAddPlayBuffer = false;									
		AudioCtx[i].bStopPlayCalled   = false;
		VideoCtx[i].bStopPlayCalled   = false;												
    }
    ADIaudEncoder = mmParm.ADIaudEncoder1;
    audioMode     = mmParm.audioMode1;

    if (mmParm.bPartialBuffers)
    {
        VideoCtx[i].dataFormatDescPlay.flags |= FORMAT_FLAG_MEM_CHUNKS;
        VideoCtx[i].dataFormatDescRec.flags |= FORMAT_FLAG_MEM_CHUNKS;        
        AudioCtx[i].dataFormatDescPlay.flags |= FORMAT_FLAG_MEM_CHUNKS;
        AudioCtx[i].dataFormatDescRec.flags |= FORMAT_FLAG_MEM_CHUNKS;        
    }
    
    ////////////////////////////////
    // RECORD
    ////////////////////////////////
    if ((mmParm.Operation == RECORD) || (mmParm.Operation == SIM_PR))
    { 
        //Opening the file to play the MM from
        ret = mmOpenFile (threegp_FileName_r,
                          OPEN_MODE_WRITE,
                          FILE_TYPE_3GP,
                          NULL,
                          VERBOSE_NOTRACES_LEVEL,
                         &VideoCtx[i].mmFileRec);
        mmPrint (i, T_SRVAPP, "3GP file created: %s\n",
                 threegp_FileName_r);

        if (ret != SUCCESS)
        {
            mmPrintPortErr (i, "Failed to Open MM File %s, error %x\n",
                            threegp_FileName_r, ret);
            return FAILURE;
        }

        if ((mmParm.audioMode1 == NATIVE_SD) || (mmParm.audioMode1 == FUS_IVR))
        {
            // Enable  Channel
            ret = mspEnableChannel (AudioCtx[i].FusionCh.hd);
            if (ret != SUCCESS)
            {
                mmPrintAll ("\nmspEnableChannel returned %d.", ret);
                return FAILURE;
            }
            ret = WaitForSpecificEvent (i, GwConfig[i].hCtaQueueHd, 
                                        MSPEVN_ENABLE_CHANNEL_DONE, &event);
            if (ret != SUCCESS)
            {
                mmPrintAll ("\nMSPEVN_ENABLE_CHANNEL_DONE event failure, ret= %x, event.value=%x,"
                            "\nboard error (event.size field) %x\n\n",
                            ret, event.value, event.size);
                return FAILURE;
            }
            mmPrint (i, T_MSPP, "Enable channel done...\n");                
        }
                     
        // Do Recording of Video first
        if (mmParm.Video_Format != NO_VIDEO)
        {
            mmPrint (i, T_SRVAPP, "Recording video for file: %s\n",
                     threegp_FileName_r);
            if ((ret = SetRecordParms (VideoCtx[i].ctahd, &recParms)) != SUCCESS)
            {
                mmPrintPortErr (i, "Failed to setup Record Parameters\n");
                exit(1);
            }
           
            VideoCtx[i].codecRec = getNewVideoCodecDef(i);
            if (VideoCtx[i].codecRec == S_CODEC_MPEG4)
            {
                VideoCtx[i].dataFormatDescRec.videoLevel   = 0;
                VideoCtx[i].dataFormatDescRec.videoProfile = 0;                
            }
   	        else if (VideoCtx[i].codecRec == S_CODEC_H263)
       	    {
           	    VideoCtx[i].dataFormatDescRec.videoLevel   = 10;
               	VideoCtx[i].dataFormatDescRec.videoProfile = 0;
            }
            
            //Opening the video stream from the MM file 
            ret = mmOpenStream(&VideoCtx[i].mmFileRec,
                                NULL,
                                STREAM_TYPE_VIDEO,
                                VideoCtx[i].codecRec,
                               &VideoCtx[i].dataFormatDescRec,
                               &VideoCtx[i].mmStreamRec,
                                NULL);

            if (ret != SUCCESS)
            {
                mmPrintPortErr (i, "Failed to Open MM Video Stream, %x\n", ret);
                exit(-1);
            }

            if (mmParm.bPartialBuffers)
            {
                VideoCtx[i].bufSizeRec = MAX_BUFFER_SIZE;
            }
            else
            {
                VideoCtx[i].bufSizeRec = mmParm.video_buffer_size;
            }

            //Allocating memory for the video buffer
            if ((VideoCtx[i].data_buffer_r = (void *)malloc(VideoCtx[i].bufSizeRec)) == NULL)
            {
                mmPrintPortErr (i,"Native Video record Buffer:failed malloc of %d bytes for port %d\n", 
                                VideoCtx[i].bufSizeRec, i);
                exit(-1);
            }

            if(mmParm.bPartialBuffers)
            {
                ret = adiRecordMMAsync (VideoCtx[i].ctahd,
                                        mmParm.ADIvidEncoder,
                                        VideoCtx[i].data_buffer_r,
                                        VideoCtx[i].bufSizeRec,
                                        videoEP_filterhd,
                                        audioEP_filterhd,
                                       &recParms);
            }
            else
            {
				if(mmParm.Operation !=SIM_PR)
                {
					ret = adiRecordMMToMemory (VideoCtx[i].ctahd, 
                                           mmParm.ADIvidEncoder,
                                           VideoCtx[i].data_buffer_r, 
                                           VideoCtx[i].bufSizeRec,
                                           videoEP_filterhd,
                                           audioEP_filterhd,
										   &recParms);
                }
                else
                {
                    ret = adiRecordMMToMemory(VideoCtx[i].ctahd,
                                              ADIvidEncoder,                 // Encoding
                                              VideoCtx[i].data_buffer_r,
                                              mmParm.video_buffer_size,
                                              videoEP_filterhd,
                                              audioEP_filterhd,
                                             &recParms);
                }      
			}
            
            if (ret != SUCCESS)
            {
                mmPrintPortErr (i, "Failed to Record Video to memory, %x\n", ret);
                exit(-1);
            }

        }

        // Recording of the Audio 
        if (audioMode != NONE)
        {
            if ((ret = SetRecordParms(AudioCtx[i].ctahd, &recParms)) != SUCCESS)
            {
                mmPrintPortErr (i, "Failed to setup Record Parameters\n");
                exit(1);
            }
              
            AudioCtx[i].codecRec = getNewAudioCodecDef(i);

            //Opening the video stream from the MM file 
            ret = mmOpenStream (&VideoCtx[i].mmFileRec,
                                 NULL,
                                 STREAM_TYPE_AUDIO,
                                 AudioCtx[i].codecRec,
                                &AudioCtx[i].dataFormatDescRec,
                                 &AudioCtx[i].mmStreamRec,
                                 NULL);

            if (ret != SUCCESS) {
                mmPrintPortErr (i, "Failed to Open MM Audio Stream, %x\n", ret);
                exit(-1);
            }
    
            if (mmParm.bPartialBuffers)
            {
                AudioCtx[i].bufSizeRec = MAX_BUFFER_SIZE;
            }
            else
            {
                AudioCtx[i].bufSizeRec = mmParm.audio_buffer_size;
            }
            
            //Allocating memory for the audio buffer
            if ((AudioCtx[i].data_buffer_r = (void *)malloc(AudioCtx[i].bufSizeRec)) == NULL)
            {
                mmPrintPortErr (i,"Native Audio record Buffer:failed malloc of %d bytes for port %d\n",
                                AudioCtx[i].bufSizeRec, i);
                exit(-1);
            }

            if(mmParm.bPartialBuffers)
            {
                ret = adiRecordMMAsync (AudioCtx[i].ctahd,
                                        ADIaudEncoder,
                                        AudioCtx[i].data_buffer_r,
                                        AudioCtx[i].bufSizeRec,
                                        videoEP_filterhd,
                                        audioEP_filterhd,
                                       &recParms);
            }
            else
            {
                ret = adiRecordMMToMemory (AudioCtx[i].ctahd, 
                                           ADIaudEncoder, 
                                           AudioCtx[i].data_buffer_r, 
                                           AudioCtx[i].bufSizeRec,
                                           videoEP_filterhd,
                                           audioEP_filterhd,
                                          &recParms);
            }
            
            if (ret != SUCCESS)
            {
                mmPrintPortErr (i, "Failed to Record Audio to memory, %x\n", ret);
                exit(-1);
            }

        }  // END IF Record Audio
    }  // END IF Record
            
    ////////////////////////////////
    // PLAY
    ////////////////////////////////
    if ((mmParm.Operation == PLAY) || (mmParm.Operation == SIM_PR))
    {
#ifdef USE_TRC      
        // resolution support
        char video_buf_res[20];     //string used to output video resolution to log a
        char fileFrameRes; // video file resolution type same as trcapi.h(CIFPATCH)
#endif
              
        VideoCtx[i].data_size_p = 0;
        AudioCtx[i].data_size_p = 0;

        //Opening the file to play the MM from
        ret = mmOpenFile (threegp_FileName_p,
                          OPEN_MODE_READ,
                          FILE_TYPE_3GP,
                          NULL,
                          VERBOSE_NOTRACES_LEVEL,
                         &VideoCtx[i].mmFilePlay);

        if (ret != SUCCESS)
        {
            mmPrintPortErr (i, "Failed to Open MM File %s, error %x\n",
                            threegp_FileName_p, ret);
            return FAILURE;
        }
        mmPrint (i, T_SRVAPP, "3GP file opened: %s\n",
                 threegp_FileName_p);

        VideoCtx[i].fileInfoDesc.fileInfoSize = MAX_FILE_INFO_SIZE;
        VideoCtx[i].fileInfoDesc.pFileInfo    = &FileInfo [0];

        //Getting the information about the opened MM file
        ret = mmGetFileInfo (&VideoCtx[i].mmFilePlay, &VideoCtx[i].fileInfoDesc);
        if (ret != SUCCESS)
        {
            mmPrintPortErr (i, "Failed to Get MM File info from file %s, error %x\n",
                            threegp_FileName_p, ret);
            return FAILURE;
        }
        
        pFileInfo     = VideoCtx[i].fileInfoDesc.pFileInfo;
        infoSizeLeft  = VideoCtx[i].fileInfoDesc.fileInfoSize;
        
        pPresentation = (FILE_INFO_PRESENTATION *)pFileInfo;
        blockSize     = pPresentation -> blockHeader.blkSize;
        streamCount   = pPresentation -> streamCount;
        
        pFileInfo    += blockSize;
        infoSizeLeft -= blockSize;
        
        for (j = 0; j < streamCount; j++)
        {
            blockSize     = ((FILE_INFO_BLOCK_HEADER *)pFileInfo) -> blkSize;
            pStreamHeader = (FILE_INFO_STREAM_AUDIO *)pFileInfo;
            
            if (pStreamHeader -> header.streamType == STREAM_TYPE_AUDIO)
            {
                AudioCtx[i].codecPlay    = pStreamHeader -> header.codec;
                AudioCtx[i].streamIdPlay = pStreamHeader -> header.streamID;
            }
            else if (pStreamHeader-> header.streamType == STREAM_TYPE_VIDEO)
            {
                VideoCtx[i].codecPlay    = pStreamHeader -> header.codec;
                VideoCtx[i].streamIdPlay = pStreamHeader -> header.streamID;            
            }
            
            if (infoSizeLeft > 0)
            {
                pFileInfo    += blockSize;
                infoSizeLeft -= blockSize;
            }
        }
        
        mmPrint (i, T_SRVAPP, "3GP data to be read from the file %s\n",
                 threegp_FileName_p);

#ifdef USE_TRC         
        // resolution support          
         fileFrameRes=getFrameRes(&VideoCtx[i].fileInfoDesc);
         get_video_res_string( fileFrameRes, video_buf_res);
            
        // dynamic VTP channel
        char  fileVidType=getNewVideoType(VideoCtx[i].codecPlay);
        ret=configVidChan(port, fileVidType, fileFrameRes);

         if(ret != SUCCESS)
         {
             mmPrintPortErr(i, "Failed to reconfigure TRC Video Channel\n");
             exit(1);
         }  
#endif
            
        if (VideoCtx[i].streamIdPlay != 0)
        {
            // If H.263 codec and GW configured for RFC 2190: format for RFC 2190.
            if( VideoCtx[i].codecPlay == S_CODEC_H263 && gwParm.h263encapRfc == 2190)
                VideoCtx[i].dataFormatDescPlay.flags |= FORMAT_FLAG_H263_2190;
		
            //Opening the video stream from the MM file 
            ret = mmOpenStream (&VideoCtx[i].mmFilePlay,
                                 VideoCtx[i].streamIdPlay,
                                 STREAM_TYPE_VIDEO,
                                 VideoCtx[i].codecPlay,
                                &VideoCtx[i].dataFormatDescPlay,
                                &VideoCtx[i].mmStreamPlay,
                                &VideoCtx[i].dataFormatInfo);

            if (ret != SUCCESS)
            {
                mmPrintPortErr (i, "Failed to Open MM Video Stream, %x\n", ret);
                exit(-1);
            }
            mmPrint (i, T_SRVAPP, "Video track opened from file: %s\n",
                     threegp_FileName_p);

            if (mmParm.bPartialBuffers)
            {
                VideoCtx[i].bufSizePlay = MAX_BUFFER_SIZE;

                //Allocating memory for the first video buffer
                if ((VideoCtx[i].data_buffer_p = (void *)malloc(VideoCtx[i].bufSizePlay)) == NULL)
                {
                    mmPrintPortErr (i,"Native Video record Partial Buffer 1:failed malloc of %d bytes for port %d\n", 
                                    VideoCtx[i].bufSizePlay, i);
                    exit(-1);
                }
            
                //Allocating memory for the second video buffer
                if ((VideoCtx[i].addDataBufferPlay = (void *)malloc(VideoCtx[i].bufSizePlay)) == NULL)
                {
                    mmPrintPortErr (i,"Native Video record Partial Buffer 2:failed malloc of %d bytes for port %d\n", 
                                    VideoCtx[i].bufSizePlay, i);
                    exit(-1);
                }
            }
            else
            {
                VideoCtx[i].bufSizePlay = VideoCtx[i].dataFormatInfo.estTotalStreamSize;

                //Allocating memory for the video buffer
                if ((VideoCtx[i].data_buffer_p = (void *)malloc(VideoCtx[i].bufSizePlay)) == NULL)
                {
                    mmPrintPortErr (i,"Native Video record Buffer:failed malloc of %d bytes for port %d\n", 
                                    VideoCtx[i].bufSizePlay, i);
                    exit(-1);
                }
            }
            mmPrint (i, T_SRVAPP, "Buffer created of size: %d\n",
                     VideoCtx[i].bufSizePlay);

            //Reading the video stream filling up the video buffer
            ret = mmReadStream(&VideoCtx[i].mmStreamPlay,
                                SAMPLE_COUNT_MAX,
                        (BYTE *)VideoCtx[i].data_buffer_p,
                                VideoCtx[i].bufSizePlay, 
                               &VideoCtx[i].byteCountPlay);

            if (ret == MMERR_END_OF_STREAM)
            {
	             mmPrint(i, T_SRVAPP, "End of Video Stream reached...\n");
                 VideoCtx[i].bLastBuffer   = true;
            }
            else if(ret != SUCCESS)
            {
                mmPrintPortErr (i, "Failed to Read Video stream, %x\n", ret);
                exit(-1);
            }

            mmPrint (i, T_SRVAPP, "Video stream read from file: %s\n",
                     threegp_FileName_p);
            
        }

        //Opening the audio stream from the MM file
        ret = mmOpenStream (&VideoCtx[i].mmFilePlay,
                             AudioCtx[i].streamIdPlay,
                             STREAM_TYPE_AUDIO,
                             AudioCtx[i].codecPlay,
                            &AudioCtx[i].dataFormatDescPlay,
                            &AudioCtx[i].mmStreamPlay,
                            &AudioCtx[i].dataFormatInfo);
                            
        if (ret != SUCCESS)
        {
            mmPrintPortErr (i, "Failed to Open MM Audio Stream, %x\n", ret);
            exit(-1);
        }
            
        if (mmParm.bPartialBuffers)
        {
            AudioCtx[i].bufSizePlay = MAX_BUFFER_SIZE;

            //Allocating memory for the first audio buffer
            if ((AudioCtx[i].data_buffer_p = (void *)malloc(AudioCtx[i].bufSizePlay)) == NULL)
            {
                mmPrintPortErr (i,"Native Audio record Partial Buffer 1:failed malloc of %d bytes for port %d\n",
                                AudioCtx[i].bufSizePlay, i);
                exit(-1);
            }

            //Allocating memory for the second audio buffer
            if ((AudioCtx[i].addDataBufferPlay = (void *)malloc(AudioCtx[i].bufSizePlay)) == NULL)
            {
                mmPrintPortErr (i,"Native Audio record Partial Buffer 2:failed malloc of %d bytes for port %d\n", 
                                AudioCtx[i].bufSizePlay, i);
                exit(-1);
            }

        }
        else
        {
            AudioCtx[i].bufSizePlay = AudioCtx[i].dataFormatInfo.estTotalStreamSize;
            
            //Allocating memory for the audio buffer
            if ((AudioCtx[i].data_buffer_p = (void *)malloc(AudioCtx[i].bufSizePlay)) == NULL)
            {
                mmPrintPortErr (i,"Native Audio record Buffer:failed malloc of %d bytes for port %d\n",
                                AudioCtx[i].bufSizePlay, i);
                exit(-1);
            }
        }

        //Reading the audio stream filling up the audio buffer
        ret = mmReadStream (&AudioCtx[i].mmStreamPlay,
                             SAMPLE_COUNT_MAX, 
                     (BYTE *)AudioCtx[i].data_buffer_p,
                             AudioCtx[i].bufSizePlay, 
                            &AudioCtx[i].byteCountPlay);

        if (ret == MMERR_END_OF_STREAM)
        {
	        mmPrint(i, T_SRVAPP, "End of Audio Stream reached...\n");
            AudioCtx[i].bLastBuffer   = true;
        }
		else if(ret != SUCCESS)
        {
            mmPrintPortErr (i, "Failed to Read Audio stream, %x\n", ret);
            exit(-1);
        }

        //Starting to play the stream(s)
        if (mmParm.bPartialBuffers)
        {
            if (VideoCtx[i].streamIdPlay != 0)
            {
                //Starting to play the MM file video
                mmPrint (i, T_SRVAPP, "Play Video (%d bytes)  \n",
                         VideoCtx[i].byteCountPlay); 
                
                if( VideoCtx[i].bLastBuffer )
	                flags = ADI_PLAY_LAST_BUFFER;
                else
	                flags = 0;

                ret = adiPlayMMAsync (VideoCtx[i].ctahd,
                                      mmParm.ADIvidEncoder,
                                      VideoCtx[i].data_buffer_p,
                                      VideoCtx[i].byteCountPlay,
                                      flags,
                                      videoEP_filterhd,
                                      NULL);
                                      
                if (ret != SUCCESS)
                {
                    mmPrintPortErr (i, "Failed to Play Video from memory, %x\n", ret);
                    exit(-1);
                }
                
                VideoCtx[i].data_size_p += VideoCtx[i].byteCountPlay;


    
                // Fill in the second video buffer if haven't reached the end of the stream 
                if ( !VideoCtx[i].bLastBuffer )
                {
                    //Reading the video stream filling up the video buffer
                    ret = mmReadStream (&VideoCtx[i].mmStreamPlay,
                                        SAMPLE_COUNT_MAX,
                                        (BYTE *)VideoCtx[i].addDataBufferPlay,
                                        VideoCtx[i].bufSizePlay, 
                                        &tempVideoByteCount);
                    if (ret == MMERR_END_OF_STREAM)
                    {
                        mmPrint(i, T_SRVAPP, "End of Video Stream reached...\n");
                        VideoCtx[i].bLastBuffer   = true;
                    }
                    else if(ret != SUCCESS)
                    {
                        mmPrintPortErr (i, "Failed to Read Video stream, %x\n", ret);
	                    exit(-1);
                    }            
                    VideoCtx[i].bUseAddPlayBuffer = true;												                            
                    VideoCtx[i].byteCountPlay = tempVideoByteCount;						
                }                
		            
            }

            //Starting to play the MM file audio
            mmPrint (i, T_SRVAPP, "Play Audio (%d bytes)  \n",
                     AudioCtx[i].byteCountPlay); 
            
            if( AudioCtx[i].bLastBuffer )
	            flags = ADI_PLAY_LAST_BUFFER;
            else
	            flags = 0;

            ret = adiPlayMMAsync (AudioCtx[i].ctahd,
                                  ADIaudEncoder,
                                  AudioCtx[i].data_buffer_p,
                                  AudioCtx[i].byteCountPlay,
                                  flags,
                                  audioEP_filterhd,
                                  NULL);
                                  
            if (ret != SUCCESS)
            {
                mmPrintPortErr (i, "Failed to Play Audio from memory, %x\n", ret);
                exit(-1);
            }

            AudioCtx[i].data_size_p += AudioCtx[i].byteCountPlay;

            // Fill in the second buffer if haven't reached the end of the stream 
            if ( !AudioCtx[i].bLastBuffer )
            {
                //Reading the audio stream filling up the second audio buffer
                ret = mmReadStream (&AudioCtx[i].mmStreamPlay,
                                    SAMPLE_COUNT_MAX, 
                                    (BYTE *)AudioCtx[i].addDataBufferPlay,
                                    AudioCtx[i].bufSizePlay, 
                                    &tempAudioByteCount);
                if (ret == MMERR_END_OF_STREAM)
                {
                    mmPrint(i, T_SRVAPP, "End of Audio Stream reached...\n");
                    AudioCtx[i].bLastBuffer   = true;
                }
                else if(ret != SUCCESS)
                {
                    mmPrintPortErr (i, "Failed to Read Audio stream, %x\n", ret);
                    exit(-1);
                }	                                
                AudioCtx[i].bUseAddPlayBuffer = true;						
                AudioCtx[i].byteCountPlay = tempAudioByteCount;					
            }							
        }
        else
        {
            if (VideoCtx[i].streamIdPlay != 0)
            {
                //Starting to play the MM file video
                mmPrint (i, T_SRVAPP, "Play Video (%d bytes)  \n",
                         VideoCtx[i].byteCountPlay);            
                ret = adiPlayMMFromMemory (VideoCtx[i].ctahd, 
                                           mmParm.ADIvidEncoder, 
                                           VideoCtx[i].data_buffer_p, 
                                           VideoCtx[i].byteCountPlay,
                                           videoEP_filterhd,
                                           NULL);
                                           
                if (ret != SUCCESS)
                {
                    mmPrintPortErr (i, "Failed to Play Video from memory, %x\n", ret);
                    exit(1);
                }
                
                VideoCtx[i].data_size_p += VideoCtx[i].byteCountPlay;
            }

            //Starting to play the MM file audio
            mmPrint (i, T_SRVAPP, "Play Audio (%d bytes)  \n",
                     AudioCtx[i].byteCountPlay);    
            ret = adiPlayMMFromMemory (AudioCtx[i].ctahd, 
                                       ADIaudEncoder, 
                                       AudioCtx[i].data_buffer_p, 
                                       AudioCtx[i].byteCountPlay,
                                       audioEP_filterhd,
                                       NULL);

            if (ret != SUCCESS)
            {
                mmPrintPortErr (i, "Failed to Play Audio from memory, %x\n", ret);
                exit(1);
            }

            AudioCtx[i].data_size_p += AudioCtx[i].byteCountPlay;
        }
    } // End If play

    return SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////////
//  getNewAudioCodecDef()
//  Function to make a bridge between the old audio codecs definition and the 
//  new ones used with the new library.
/////////////////////////////////////////////////////////////////////////////////
WORD getNewAudioCodecDef(int port)
{
	switch (mmParm.Audio_Record_Format1)
    {
		case N_AMR:
			return S_CODEC_AMR;
		case N_G723:
			return S_CODEC_G723_1;
		default:
			mmPrintPortErr(port, "Invalid audio format!\n");			
			return S_CODEC_UNKNWN;
	}
}


/////////////////////////////////////////////////////////////////////////////////
//  getNewVideoCodecDef()
//  Function to make a bridge between the 324M negotiated codec definition and the 
//  definitions used by the mmfi library.
/////////////////////////////////////////////////////////////////////////////////
WORD getNewVideoCodecDef(int port)
{
	switch (GwConfig[port].txVideoCodec)
    {
		case H324_MPEG4_VIDEO:
			return S_CODEC_MPEG4;
		case H324_H263_VIDEO:
			return S_CODEC_H263;
        default:
			mmPrintPortErr(port, "Invalid video format!\n");			
			return S_CODEC_UNKNWN;
    }        
}


/////////////////////////////////////////////////////////////////////////////////
//  SetRecordParms()
//  Function to Get the Current CTA Parameters, an reset only those That were
//  Changed in the config file.
/////////////////////////////////////////////////////////////////////////////////
DWORD SetRecordParms(CTAHD ctahd, ADI_MM_RECORD_PARMS* recParms)
{
    DWORD ret;

    ret = ctaGetParms (ctahd, ADI_RECORD_PARMID, recParms, sizeof (ADI_MM_RECORD_PARMS));
    if (ret != SUCCESS)
    {
        mmPrintAll ("\nFailed getting Record Parameters %d.", ret);
        return FAILURE;
    }

    if (mmParm.bEnable_Iframe == TRUE)
    {
        recParms->startonvideoIframe = mmParm.enable_Iframe;
    }

    if (mmParm.bAudioStop_InitTimeout == TRUE)
    {
        recParms->novoicetime = mmParm.audioStop_InitTimeout;
    }

    if (mmParm.bAudioStop_SilenceTimeout == TRUE)
    {
        recParms->silencetime = mmParm.audioStop_SilenceTimeout;
    }

    if (mmParm.bAudioStop_SilenceLevel == TRUE)
    {
        recParms->silenceampl = mmParm.audioStop_SilenceLevel;
    }

    if (mmParm.bVideoStop_InitTimeout == TRUE)
    {
        recParms->novideotime = mmParm.videoStop_InitTimeout;
    }

    if (mmParm.bVideoStop_NosignalTimeout == TRUE)
    {
        recParms->videotimeout = mmParm.videoStop_NosignalTimeout;
    }

    if (mmParm.bMaxTime == TRUE)
    {
        recParms->maxtime = mmParm.maxTime;
    }
    
    return SUCCESS;
}

DWORD getADIvidEncoder(tVideo_Format fileFormat)
{
    switch (fileFormat)
    {
        case MPEG4:
            return ADI_ENCODE_NATIVE_MPEG4;
			
        case H263:
            if (gwParm.h263encapRfc == 2190)
                return ADI_ENCODE_NATIVE_H_263;
			else
	            return ADI_ENCODE_NATIVE_H_263P;        
				
        default:
            return -1;
    }     

    return -1;
}

#ifdef USE_TRC 
	         
DWORD CheckForXcoding(DWORD dGwIndex, tVideo_Format fileFormat)
{
 DWORD ret = SUCCESS;
 MEDIA_GATEWAY_CONFIG *pCfg;
 DWORD  res;

 res =  getFileRes(VideoCtx[dGwIndex].threegp_FileName_play);
 
        pCfg = &GwConfig[dGwIndex];
                      //  tVideo_Format fileFormat;
       switch( fileFormat ) {
        case MPEG4:
            if( (pCfg->txVideoCodec != H324_MPEG4_VIDEO  ) || (res!=TRC_FRAME_RES_QCIF) ) {
                   //  mmPrint( dGwIndex, T_GWAPP, "Starting MPEG4 - H263 transcoding.\n");
                if( !pCfg->bGwVtpServerConfig ) {
                    if(gwParm.useVideoXc)
                     {
                     mmPrint( dGwIndex, T_GWAPP, "Starting MPEG4 - H263 transcoding.\n");
                    // Configure EPs GW-VTP-Server
                    configurePlayWithXc( dGwIndex );
                     }else{
                     mmPrint(dGwIndex, T_GWAPP, "useVideoXc not specified in the configuration file!\n"); 
                     }
                }
            } else { // if pCfg->txVideoCodec == H324_MPEG4_VIDEO
                if( pCfg->bGwVtpServerConfig ) {
                    // Configure EPs Gw-Server
                    configurePlayWithoutXc( dGwIndex );
                }

            }
               // PlayRecord( dGwIndex, 1, fileFormat );

            break;
        case H263:
            if( (pCfg->txVideoCodec != H324_H263_VIDEO ) ||(res!=TRC_FRAME_RES_QCIF) ){
                  if(gwParm.useVideoXc)
                     {
      //          mmPrint( dGwIndex, T_GWAPP, "Starting H263 - MPEG4 transcoding.\n");
                if( !pCfg->bGwVtpServerConfig ) {
                   mmPrint( dGwIndex, T_GWAPP, "Starting H263 - MPEG4 transcoding.\n");
                    // Configure EPs GW-VTP-Server
                    configurePlayWithXc( dGwIndex );
                     }else{
                     mmPrint(dGwIndex, T_GWAPP, "useVideoXc not specified in the configuration file!\n");
                     }
                }
            } else { // if pCfg->txVideoCodec == H324_H263_VIDEO
                if( pCfg->bGwVtpServerConfig ) {
                    // Configure EPs Gw-Server
                    configurePlayWithoutXc( dGwIndex );
                }
            }
                //PlayRecord( dGwIndex, 1, fileFormat );
             break;
      default:
            printf("ERROR: Invalid file format ==> %d\n", fileFormat );
            break;
    }
    return ret;
}
#endif

#ifdef USE_TRC
// dynamic VTP channel
int   getVideoChannelStatus(TRC_HANDLE trchd) 
{

	DWORD ret;	
	tTrcChInfo trcChInfo;
	        
   ret = trcInfoVideoChannel( trchd, &trcChInfo );
             
   if( ret != SUCCESS )
   {
           mmPrintPortErr( 0, "trcInfoVideoChannel failed. rc = 0x%x\n, [%s]",
   				    ret, trcValueName(TRCVALUE_RESULT,ret) );  				
   }        
        
   return (trcChInfo.summary.status.state);
} 

DWORD  startVideoChannel(int dGwIndex)
{
        MEDIA_GATEWAY_CONFIG   *pCfg;
        extern t_GwStartParms  gwParm;
        CTA_EVENT              event;
        DWORD                  ret;

        pCfg=&GwConfig[dGwIndex];

			// Start the video channel
			ret = trcStartVideoChannel( pCfg->VideoXC.trcChHandle,
									  &pCfg->VideoXC.trcChConfig );
			Flow(dGwIndex, F_APPL, F_TRC, "trcStartVideoChannel");
			
			if( ret != TRC_SUCCESS )
			{
				mmPrintPortErr( dGwIndex, "trcStartVideoChannel failed. rc = 0x%x, [%s]\n",
								 ret, trcValueName(TRCVALUE_RESULT,ret) );  
			}
			else
			{
	            change_xc_state( dGwIndex, XCSTATE_STARTING );
			}					
               

        if( ret != TRC_SUCCESS ) {
                mmPrintPortErr( dGwIndex, "trcStartVideoChannel failed to start the channel. rc = %d\n", ret);
                           return FAILURE;
        } 
        //block until  

        ret=WaitForSpecificEvent( dGwIndex, 
                                  pCfg->hCtaQueueHd,
                                  TRCEVN_START_CHANNEL_DONE,
                                 &event);

        if(ret!= SUCCESS) {
                mmPrintPortErr( dGwIndex, "trcStartVideoChannel failed to start the channel or timeout. rc = %d\n", ret);
                return FAILURE;
        }

        return ret;
}


DWORD  stopVideoChannel(int dGwIndex)
{
        MEDIA_GATEWAY_CONFIG   *pCfg;
        extern t_GwStartParms  gwParm;
        CTA_EVENT              event;
        DWORD                  ret;

        pCfg=&GwConfig[dGwIndex];



                      
        ret = trcStopVideoChannel( pCfg->VideoXC.trcChHandle);

        Flow(dGwIndex, F_APPL, F_TRC, "trcStopVideoChannel");                            

        if( ret != TRC_SUCCESS ) {
                mmPrintPortErr( dGwIndex, "trcStopVideoChannel failed to stop the channel. rc = %d\n", ret);
                return FAILURE;
        } 
        //block until  

        ret=WaitForSpecificEvent( dGwIndex, 
                                  pCfg->hCtaQueueHd,
                                  TRCEVN_STOP_CHANNEL_DONE,
                                 &event);

        if(ret!= SUCCESS) {
                mmPrintPortErr( dGwIndex, "trcStopVideoChannel failed to stop the channel or timeout. rc = %d\n", ret);
                return FAILURE;
        }

        return ret;

} 

DWORD   configVidChan(int port, char fileVidType, char fileFrameRes){
        
	MEDIA_GATEWAY_CONFIG  *pCfg;
        extern  t_GwStartParms gwParm; 
        int  chanStatus=0;
        DWORD       ret;


        pCfg=&GwConfig[port] ; 
  
        if(pCfg->VideoXC.bActive==TRUE&&gwParm.useVideoXc==TRUE) { 
                chanStatus=getVideoChannelStatus(pCfg->VideoXC.trcChHandle);
                pCfg->bVideoChanRestart=0;                                                       
                if (pCfg->bGwVtpServerConfig==TRUE){
                   	if (  (pCfg->VideoXC.trcChConfig.endpointA.frameRes == fileFrameRes) &&
                   	      (pCfg->VideoXC.trcChConfig.endpointA.vidType == (unsigned long)fileVidType) &&
                         	(chanStatus==TRC_STATE_ACTIVE) ){
                              	  //Do not restart the Channel
                                  pCfg->bVideoChanRestart=0; 

                         }else { // Need start/Restart the Video Channel

                                  mmPrint(port, T_GWAPP, "Need Stop/Restart Simplex Video Channel. \n");
                    
                                   //reset the video transcoder channel leg one's parameters. 
                                   pCfg->VideoXC.trcChConfig.endpointA.vidType = fileVidType;
                                   
                                   if (pCfg->VideoXC.trcChConfig.endpointA.vidType == TRC_VIDTYPE_H263)
                                   {
                                       pCfg->VideoXC.trcChConfig.endpointA.packetizeMode = TRC_PACKETIZE_2190;
                                       pCfg->VideoXC.trcChConfig.endpointA.profile = TRC_PROFILE_BASELINE;
	                                    pCfg->VideoXC.trcChConfig.endpointA.level = TRC_H263_LEVEL_10;
	                                }
                                   if (pCfg->VideoXC.trcChConfig.endpointA.vidType == TRC_VIDTYPE_MPEG4)
                                   {
                                       pCfg->VideoXC.trcChConfig.endpointA.packetizeMode = TRC_PACKETIZE_3016;
                                       pCfg->VideoXC.trcChConfig.endpointA.profile = TRC_PROFILE_SIMPLE;
	                                    pCfg->VideoXC.trcChConfig.endpointA.level = TRC_MPEG4_LEVEL_0;
                                   }
                                   
                                   pCfg->VideoXC.trcChConfig.endpointA.frameRes= fileFrameRes;

                                   if(chanStatus==TRC_STATE_ACTIVE) {// video channel is active,need restart it;  
                                          // video channel  to be stoped
                                           pCfg->bVideoChanRestart=1;
                                           ret=stopVideoChannel(port); //block mode to stop videochannel
                                           if (ret != TRC_SUCCESS) {
                                                      mmPrintPortErr( port, "trcStopVideoChannel failed to stop the channel. rc = %d\n", ret);
                                                      return FAILURE;
                                            }
                                    } //end of if ChanStatus == TRC_STATE_ACTIVE ;

                                    ret = startVideoChannel(port); //Block mode to start Video Channel. 

                                    if (ret != TRC_SUCCESS ) {
                                            mmPrintPortErr( port, "trcStartVideoChannel(failed to start the channel. rc =%d\n", ret);
                                            return FAILURE;
                                    }
                                    ret = trcIframeVideoChannel( pCfg->VideoXC.trcChHandle, TRC_DIR_SIMPLEX );   
                                    
                                    Flow(port, F_APPL, F_TRC, "trcConfigVideoChannel");

                                    if( ret != SUCCESS) {
                                            mmPrintPortErr( port, "trcConfigVideoChannel failed to Configure the channel. rc = %d\n", ret);
                                    } 
                         } //end of restart if;
              }  //end of if  bGwVtpServerConfig==TRUE)
      } //end of if pCfg->VideoXC.bAcvtive . 
      
      return SUCCESS;
}

char	getNewVideoType(WORD codec)
{
       char ret=0;

       switch (codec) {
          case S_CODEC_H263:  //H263.
               ret=TRC_VIDTYPE_H263; 
               break;        
          case S_CODEC_MPEG4:  //MPEG4 
               ret=TRC_VIDTYPE_MPEG4; 
               break;
          default:
               ret=0 ;  //NO_VIDEO
               break;
       }
       return ret;
}

char   getFrameResType(WORD wWidth, WORD wHeight){

	const   WORD    QCIF_Width=176;
	const   WORD    QCIF_Height=144;
	
	const   WORD    CIF_Width=352;
	const   WORD    CIF_Height=288;

	const	WORD	SUBQCIF_Width=128;
	const   WORD	SUBQCIF_Heigh=96;
		
	const	WORD	Error=33;  // allow the cif or qcif resoultion has small variance. 
	char 	result =  TRC_FRAME_RES_UNKNOWN; 
        	
	if( wWidth<=(QCIF_Width+Error) && wWidth >= (QCIF_Width-Error) ){
		//QCIF File;
		result=TRC_FRAME_RES_QCIF;  // defined in trcapi.h
	}

	if ( wWidth<=(CIF_Width+Error) && wWidth >= (CIF_Width-Error) ){
		//CIF File; 	
                result=TRC_FRAME_RES_CIF;  // defined in trcapi.h
        }
        if ( wWidth<=(SUBQCIF_Width+Error) && wWidth >= (SUBQCIF_Width-Error) ){

                //SUBQCIF File;
                result=TRC_FRAME_RES_SUBQCIF;  // defined in trcapi.h
        }
 
        return result;

}	

DWORD getFileRes(char *filename)
{
   DWORD ret;
        
   MMFILE          mmFile;
   FILE_INFO_DESC  fileInfoDesc;
   BYTE            FileInfo [MAX_FILE_INFO_SIZE];
   
   ret = mmOpenFile(filename, OPEN_MODE_READ, FILE_TYPE_3GP,
                   NULL, VERBOSE_NOTRACES_LEVEL, &mmFile);
   if (ret != SUCCESS)
   {
      mmPrint(0, T_GWAPP,"Failed to Open MM File %s, error %x\n",
              filename, ret);
      return FAILURE;
   }
   
   fileInfoDesc.fileInfoSize = MAX_FILE_INFO_SIZE;
   fileInfoDesc.pFileInfo = &FileInfo [0];
   
   ret = get3gpFileRes(filename, &mmFile, &fileInfoDesc);
   
   if (ret < 0)
   {
      return FAILURE;
   }
   return ret;
        
}

char  getFrameRes(FILE_INFO_DESC *pFileInfoDesc)
{

    WORD                    streamCount,
                            j,
                            infoSizeLeft;       /* size left in file info buffer */
    FILE_INFO_PRESENTATION *pPresentation;      /* pointer on Presentation block */
    FILE_INFO_STREAM_AUDIO *pStreamHeader;      /* pointer on Stream Header      */
	 FILE_INFO_STREAM_VIDEO *pStreamVideo;		/* pointer on Video Stream block */
    int                     blockSize;          /* size of current block         */
    BYTE                   *pFileInfo;
    WORD                    width;
	 WORD			            height; 
	 char                   result;
	
    pFileInfo     = pFileInfoDesc -> pFileInfo;
    infoSizeLeft  = pFileInfoDesc -> fileInfoSize;
        
    pPresentation = (FILE_INFO_PRESENTATION *)pFileInfo;
    blockSize     = pPresentation -> blockHeader.blkSize;
    streamCount   = pPresentation -> streamCount;
        
    pFileInfo    += blockSize;
    infoSizeLeft -= blockSize;

    for (j = 0; j < streamCount; j++)
    {
        blockSize     = ((FILE_INFO_BLOCK_HEADER *)pFileInfo) -> blkSize;
        pStreamHeader = (FILE_INFO_STREAM_AUDIO *)pFileInfo;
            
        if (pStreamHeader-> header.streamType == STREAM_TYPE_VIDEO)
        {
        	pStreamVideo = (FILE_INFO_STREAM_VIDEO *)pFileInfo;
            width  = pStreamVideo -> format.width;
            height = pStreamVideo -> format.height;
            break;
        }
            
        if (infoSizeLeft > 0)
        {
            pFileInfo    += blockSize;
            infoSizeLeft -= blockSize;
        }
    }
	
	 result = getFrameResType(width,height);

    return result;
}	// end of PrintFileInfo

void get_video_res_string(DWORD FrameRes, char *str){

	switch(FrameRes) {
                case TRC_FRAME_RES_QCIF: strcpy(str, "QCIF");
                        break;
                case TRC_FRAME_RES_CIF: strcpy(str, "CIF");
                        break;

                default: 
                        strcpy(str, "UNKNOWN");
                        break;
        }
 
        return;
}

/***********************************************************************************
 *   return: TRC_FRAME_RES_UNKNOWN : -1 unknow FRAME_RESOLUTION or Error
 *           TRC_FRAME_RES_QCIF    : 1 
 *           TRC_FRAME_RES_CIF     : 2
        
 * 
 * 
 ***********************************************************************************/ 

DWORD   get3gpFileRes(char *fileName, MMFILE *pMMFile, FILE_INFO_DESC *pFileInfoDesc)
{
    DWORD                   ret;
        
    //Getting the information about the opened MM file
    ret = mmGetFileInfo (pMMFile, pFileInfoDesc);
                 
    if (ret != SUCCESS)
    {
        printf("Failed to get the 3GP infomation from the file: %s  err code is %x\n", 
                fileName,ret);
        return TRC_FRAME_RES_UNKNOWN ;
    }
    
    ret= getFrameRes(pFileInfoDesc);

    if (ret == TRC_FRAME_RES_UNKNOWN)
    {
        printf("The Resolution of the 3GP file %s is UNKNOW FORMAT. \n", fileName);      
        return ret;
    }

    
    return ret; 
}	

#endif
