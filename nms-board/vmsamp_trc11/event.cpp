/**********************************************************************
*  File - event.cpp
*
*  Description - This file contains functions related to event processing
*
*  2005  NMS Communications Corporation.
*
***********************************************************************/

///////////////////
// Include Files //
///////////////////
#include <time.h>

#if defined(WIN32)
#include "windows.h"
//#define gethrtime timeGetTime
#elif defined(SOLARIS)
#include <sys/time.h>  /* gethrtime */
#endif

#include "mega_global.h"
#include "gw_types.h"
#include "srv_types.h"
#include "srv_def.h"
#include "adiEvent.h"
#include "vm_trace.h"

#include "vm_stat.h"
void ShowDemuxPeriodicStats(int nGw, msp_ENDPOINT_DEMUX_PERIODIC_STATS *stat);

#if defined(WIN32)
U32 msDiff(hrtime_t beg, hrtime_t end) {
   hrtime_t interval =  (end >= beg) ?
                       (end  - beg) : (~(beg-end) + 1);
   return interval;
}
U32 msTimeStamp(void) { return (U32)timeGetTime(); }
#else
hrtime_t msDiff(hrtime_t beg, hrtime_t end) { return (end - beg); }
hrtime_t msTimeStamp(void)
#if defined(SOLARIS)
   { return (gethrtime() / 1000000); }
#else // Linux POSIX complianet
   {
      timespec tp;
      int i =  clock_gettime(CLOCK_REALTIME, &tp);
      return (tp.tv_sec * 1000 + tp.tv_nsec / 1000000);
   }
#endif // SOLARIS
#endif // WIN32


//************************* WaitForSpecificEvent *************************//
//  WaitForSpecificEvent
//
//  This function waits for a specific event, but allows up unsolicited
//  events to be processed while waiting.  Waiting ends when 10 timeouts
//  (70ms each) have occured, and desired event is not found.
//
//  SUCCESS is returned if the desired event is found, and it's return value
//  is CTA_REASON_FINISHED, otherwise FAILURE is returned
//**************************************************************************
DWORD WaitForSpecificEvent( int nGw, CTAQUEUEHD queue_hd, DWORD desired_event, CTA_EVENT *eventp )
{
	DWORD ret,timeout_count;
	BOOL  timed_out   = FALSE;
	BOOL  event_found = FALSE;

    memset (eventp, 0, sizeof (CTA_EVENT));

    //Try 10 times before giving up on Specific event
    for(timeout_count=0; (timeout_count<20) && (!event_found);)
    {
        ret = ctaWaitEvent( queue_hd, eventp, 240 );  // 70ms second timeout
        if ( ret != SUCCESS )
        {
            mmPrintPortErr(nGw, "ctaWaitEvent error, returned %x\n", ret);
            return FAILURE;
        }
	
	    if (eventp->id != CTAEVN_WAIT_TIMEOUT)
        {
			BOOL consumed;
			ret = h324SubmitEvent(eventp, &consumed);
			if(ret != SUCCESS)
				mmPrint(nGw, T_GWAPP,"h324SubmitEvent returned 0x%x\n",ret);
       		if(consumed)
				continue;
			
			//Process all non-consumed, non-time-out events	
            ProcessCTAEvent(nGw, eventp);
            if(eventp->id == desired_event)
                event_found = TRUE;            
        }
        else //Timed out
        {
            timeout_count++;
        }
    }  //  END for

    if(!event_found)
    {
        mmPrintPortErr(nGw, "Specific event %08x not found\n",desired_event);
        return FAILURE;
    }
    if ( event_found && 
		( (eventp->value != CTA_REASON_FINISHED) && (eventp->value != SUCCESS) ) )
    {
	        mmPrintPortErr(nGw, "Specific event %08x found, but with value = %08x, not CTA_REASON_FINISHED\n",
    	        desired_event, eventp->value);
        	return FAILURE;
    }
    return SUCCESS;
}


//************************* ProcessCTAEvent *************************//
BOOL ProcessCTAEvent(int nGw, CTA_EVENT *pevent)
{  
    char    event_val[100];
	BOOL break_flag = FALSE;

    event_val[0] = '0';
 
#define EVENT_V(chr, ev) case (ev):sprintf(chr, #ev); break;
#define EVENT_I(chr, ei) case (ei):mmPrint(nGw, T_CTA, #ei" %s\n", chr);break;
#define MSP_EVENT_I(hd, chr, ei) case (ei): \
    mmPrint(nGw, T_MSPP, #ei" %s. Handle %08x\n", chr, hd); \
    break;

    // Decode the event value field
    switch (pevent->value)
    {
    EVENT_V( event_val, CTA_REASON_FINISHED)
    EVENT_V( event_val, CTA_REASON_STOPPED) 
    EVENT_V( event_val, CTA_REASON_TIMEOUT) 
    EVENT_V( event_val, CTA_REASON_DIGIT) 
    EVENT_V( event_val, CTA_REASON_NO_VOICE) 
    EVENT_V( event_val, CTA_REASON_VOICE_END) 
    EVENT_V( event_val, CTA_REASON_RELEASED) 
    EVENT_V( event_val, CTA_REASON_RECOGNITION) 
    EVENT_V( event_val, CTA_REASON_NATIVE_COMPANION_RECORD_STOPPED)

    EVENT_V( event_val, CTAERR_RESERVED) 
    EVENT_V( event_val, CTAERR_FATAL) 
    EVENT_V( event_val, CTAERR_BOARD_ERROR) 
    EVENT_V( event_val, CTAERR_INVALID_CTAQUEUEHD) 
    EVENT_V( event_val, CTAERR_INVALID_CTAHD) 
    EVENT_V( event_val, CTAERR_OUT_OF_MEMORY) 
    EVENT_V( event_val, CTAERR_BAD_ARGUMENT) 
    EVENT_V( event_val, CTAERR_OUT_OF_RESOURCES) 
    EVENT_V( event_val, CTAERR_NOT_IMPLEMENTED) 
    EVENT_V( event_val, CTAERR_NOT_FOUND) 
    EVENT_V( event_val, CTAERR_BAD_SIZE) 
    EVENT_V( event_val, CTAERR_INVALID_STATE) 
    EVENT_V( event_val, CTAERR_FUNCTION_NOT_AVAIL) 
    EVENT_V( event_val, CTAERR_FUNCTION_NOT_ACTIVE) 
    EVENT_V( event_val, CTAERR_FUNCTION_ACTIVE) 
    EVENT_V( event_val, CTAERR_SHAREMEM_ACCESS) 
    EVENT_V( event_val, CTAERR_INCOMPATIBLE_REVISION) 
    EVENT_V( event_val, CTAERR_RESOURCE_CONFLICT) 
    EVENT_V( event_val, CTAERR_INVALID_SEQUENCE) 
    EVENT_V( event_val, CTAERR_DRIVER_OPEN_FAILED) 
    EVENT_V( event_val, CTAERR_DRIVER_VERSION) 
    EVENT_V( event_val, CTAERR_DRIVER_RECEIVE_FAILED) 
    EVENT_V( event_val, CTAERR_DRIVER_SEND_FAILED) 
    EVENT_V( event_val, CTAERR_DRIVER_ERROR) 
    EVENT_V( event_val, CTAERR_TOO_MANY_OPEN_FILES) 
    EVENT_V( event_val, CTAERR_INVALID_BOARD) 
    EVENT_V( event_val, CTAERR_OUTPUT_ACTIVE) 
    EVENT_V( event_val, CTAERR_CREATE_MUTEX_FAILED) 
    EVENT_V( event_val, CTAERR_LOCK_TIMEOUT) 
    EVENT_V( event_val, CTAERR_ALREADY_INITIALIZED) 
    EVENT_V( event_val, CTAERR_NOT_INITIALIZED) 
    EVENT_V( event_val, CTAERR_INVALID_HANDLE) 
    EVENT_V( event_val, CTAERR_PATH_NOT_FOUND) 
    EVENT_V( event_val, CTAERR_FILE_NOT_FOUND) 
    EVENT_V( event_val, CTAERR_FILE_ACCESS_DENIED) 
    EVENT_V( event_val, CTAERR_FILE_EXISTS) 
    EVENT_V( event_val, CTAERR_FILE_OPEN_FAILED) 
    EVENT_V( event_val, CTAERR_FILE_CREATE_FAILED) 
    EVENT_V( event_val, CTAERR_FILE_READ_FAILED) 
    EVENT_V( event_val, CTAERR_FILE_WRITE_FAILED) 
    EVENT_V( event_val, CTAERR_DISK_FULL) 
    EVENT_V( event_val, CTAERR_CREATE_EVENT_FAILED) 
    EVENT_V( event_val, CTAERR_EVENT_TIMEOUT) 
    EVENT_V( event_val, CTAERR_OS_INTERNAL_ERROR) 
    EVENT_V( event_val, CTAERR_INTERNAL_ERROR) 
    EVENT_V( event_val, CTAERR_DUPLICATE_WAITOBJ) 
    EVENT_V( event_val, CTAERR_NO_LICENSE) 
    EVENT_V( event_val, CTAERR_LICENSE_EXPIRED) 
    EVENT_V( event_val, CTAERR_INVALID_SYNTAX) 
    EVENT_V( event_val, CTAERR_INVALID_KEYWORD) 
    EVENT_V( event_val, CTAERR_SECTION_NOT_FOUND) 
    EVENT_V( event_val, CTAERR_WRONG_COMPATLEVEL) 
    EVENT_V( event_val, CTAERR_PARAMETER_ERROR) 

    EVENT_V( event_val, MSPERR_INVALID_CONNECTION) 
    EVENT_V( event_val, MSPERR_INTERNAL_HANDLE) 
    EVENT_V( event_val, MSPERR_CANNOT_OPEN_SERVICE_COMPONENT) 
    EVENT_V( event_val, MSPERR_INVALID_HANDLE) 
    EVENT_V( event_val, MSPERR_CHANNEL_NOT_DEFINED) 
    EVENT_V( event_val, MSPERR_CHANNEL_NOT_CONNECTED) 
    EVENT_V( event_val, MSPERR_ENDPOINT_NOT_ALLOCATED) 
    EVENT_V( event_val, MSPERR_DRIVER_COMMAND_FAILED) 
    EVENT_V( event_val, MSPERR_UNKNOWN_FILTER_OBJECT) 
    EVENT_V( event_val, MSPERR_SENDCOMMAND_FAILED) 
    EVENT_V( event_val, MSPERR_SENDQUERY_FAILED) 
    EVENT_V( event_val, MSPERR_ENDPOINT_BUSY) 
    EVENT_V( event_val, MSPERR_FILTER_BUSY) 
    EVENT_V( event_val, MSPERR_ENDPOINT_IS_ENABLED) 
    EVENT_V( event_val, MSPERR_ENDPOINT_IS_DISABLED) 
    EVENT_V( event_val, MSPERR_CHANNEL_IS_ENABLED) 
    EVENT_V( event_val, MSPERR_CHANNEL_IS_DISABLED) 
    EVENT_V( event_val, MSPRSN_FAILED_TO_ALLOCATE)
    EVENT_V( event_val, MSPRSN_FAILED_TO_DEALLOCATE)
    EVENT_V( event_val, MSPRSN_FAILED_TO_START)
    EVENT_V( event_val, MSPRSN_FAILED_TO_STOP)
    EVENT_V( event_val, MSPRSN_FAILED_TO_INTRACONNECT)
    EVENT_V( event_val, MSPRSN_FAILED_TO_INTRADISCONNECT)
    EVENT_V( event_val, MSPRSN_FAILED_TO_INTERCONNECT)
    EVENT_V( event_val, MSPRSN_FAILED_TO_INTERDISCONNECT)

    default:sprintf(event_val, "UNKNOWN_VALUE");break;
    } //End Switch on value

    //Decode the Event ID field, then parse the buffer
	switch (pevent->id)
    {
    EVENT_I(event_val, CTAEVN_NULL_EVENT)
    EVENT_I(event_val, CTAEVN_WAIT_TIMEOUT)
    EVENT_I(event_val, CTAEVN_UPDATE_WAITOBJS)
    EVENT_I(event_val, CTAEVN_DESTROY_CONTEXT_DONE)
    EVENT_I(event_val, CTAEVN_OPEN_SERVICES_DONE)
    EVENT_I(event_val, CTAEVN_CLOSE_SERVICES_DONE)

    MSP_EVENT_I(pevent->objHd, event_val, MSPEVN_CREATE_ENDPOINT_DONE)
    MSP_EVENT_I(pevent->objHd, event_val, MSPEVN_DESTROY_ENDPOINT_DONE)
    MSP_EVENT_I(pevent->objHd, event_val, MSPEVN_ENABLE_ENDPOINT_DONE)
    MSP_EVENT_I(pevent->objHd, event_val, MSPEVN_DISABLE_ENDPOINT_DONE)
    MSP_EVENT_I(pevent->objHd, event_val, MSPEVN_CREATE_CHANNEL_DONE)
    MSP_EVENT_I(pevent->objHd, event_val, MSPEVN_DESTROY_CHANNEL_DONE)
    MSP_EVENT_I(pevent->objHd, event_val, MSPEVN_ENABLE_CHANNEL_DONE)
    MSP_EVENT_I(pevent->objHd, event_val, MSPEVN_DISABLE_CHANNEL_DONE)
    MSP_EVENT_I(pevent->objHd, event_val, MSPEVN_CONNECT_DONE)
    MSP_EVENT_I(pevent->objHd, event_val, MSPEVN_DISCONNECT_DONE)

    //PROCCESS UNSOLICITED EVENTS
	//
	// These events can all be ignored. If they show up, it means that the H.324 API isn't active 
	// for that channel
    case (MSPEVN_DPF_DEBUG_REPORT):
        mmPrint(nGw, T_MSPP, "MSPEVN_DPF_DEBUG_REPORT %s. Handle %08x\n",  event_val, pevent->objHd);
	    break;
    case (MSPEVN_MUX_PMSYNC_STARTED):
		mmPrint(nGw, T_MSPP, "MSPEVN_MUX_PMSYNC_STARTED %s. Handle %08x\n",event_val, pevent->objHd);
		break;
    case (MSPEVN_MUX_PMSYNC_STOPPED):
		mmPrint(nGw, T_MSPP, "MSPEVN_MUX_PMSYNC_STOPPED %s Handle %08x\n", event_val, pevent->objHd);
    	break;
    case (MSPEVN_MUX_GET_H245_MSG):
        mmPrint(nGw, T_MSPP, "MSPEVN_MUX_GET_H245_MSG %s. Handle %08x\n",  event_val, pevent->objHd);
		break;

    //  This case handles both MUX EP MSP_QRY_GET_CAPABILITY_SETTING and
    //  video rtp ep MSP_QRY_JITTER_VIDEO_GET_STATE done (since they
    //  have the same Event ID)
    case (MSPEVN_QUERY_DONE | MSP_QRY_JITTER_VIDEO_GET_STATE):

        if(GwConfig[nGw].MuxEp.hd == pevent->objHd)
            mmPrint(nGw, T_MSPP, "MSPEVN_QUERY_DONE MSP_QRY_GET_CAPABILITY_SETTING %s. Handle %08x\n",
				 event_val, pevent->objHd);

        // For a Video Channel, process a JITTER_VIDEO_MPEG4_GET_STATUS QUERY
        else if(GwConfig[nGw].vidChannel[EP_INB].hd == pevent->objHd)
        {
            mmPrint(nGw, T_MSPP, "MSPEVN_QUERY_DONE MSP_QRY_JITTER_VIDEO_MPEG4_GET_STATUS %s. Handle %08x\n", 
				event_val, pevent->objHd);
            if((pevent->value == CTA_REASON_FINISHED) && ( pevent->buffer != NULL ))
            {
                printf("\n  pktsReceived = %8d,   pktsAccepted = %8d,  "\
                       "\n  pktsRejected = %8d,   lates        = %8d,   pktsLost     = %8d,"
                       "\n  pktsCurrent  = %8d,   overflows    = %8d,   underflows   = %8d,"
                       "\n  duplicates   = %8d,   reorders     = %8d",
                NMS2H_DWORD(((msp_FILTER_JITTER_VIDEO_MPEG4_STATE*)(pevent->buffer))->pktsReceived  ),
                NMS2H_DWORD(((msp_FILTER_JITTER_VIDEO_MPEG4_STATE*)(pevent->buffer))->pktsAccepted  ),
                NMS2H_DWORD(((msp_FILTER_JITTER_VIDEO_MPEG4_STATE*)(pevent->buffer))->pktsRejected  ),
                NMS2H_DWORD(((msp_FILTER_JITTER_VIDEO_MPEG4_STATE*)(pevent->buffer))->lates         ),
                NMS2H_DWORD(((msp_FILTER_JITTER_VIDEO_MPEG4_STATE*)(pevent->buffer))->pktsLost      ),
                NMS2H_DWORD(((msp_FILTER_JITTER_VIDEO_MPEG4_STATE*)(pevent->buffer))->pktsCurrent   ),
                NMS2H_DWORD(((msp_FILTER_JITTER_VIDEO_MPEG4_STATE*)(pevent->buffer))->overflows     ),
                NMS2H_DWORD(((msp_FILTER_JITTER_VIDEO_MPEG4_STATE*)(pevent->buffer))->underflows    ),
                NMS2H_DWORD(((msp_FILTER_JITTER_VIDEO_MPEG4_STATE*)(pevent->buffer))->duplicates    ),
                NMS2H_DWORD(((msp_FILTER_JITTER_VIDEO_MPEG4_STATE*)(pevent->buffer))->reorders      ));

            }
        }
        // For a AMR Bypass Channel, process a msp_FILTER_JITTER_STATE QUERY
        else if(GwConfig[nGw].AudByChan[EP_INB].hd == pevent->objHd)
        {
            printf("\n  MSPEVN_QUERY_DONE MSP_QRY_JITTER_GET_STATE %s", event_val);
            printf("\n  Event Handle %08x", pevent->objHd);
            if((pevent->value == CTA_REASON_FINISHED) && ( pevent->buffer != NULL ))
            {
                printf("\nfilter_id     = %08x, rx            = %8d, rx_accept      = %8d,"
                       "\ntx            = %8d, tx_valid       = %8d, requested_depth= %8d,"
                       "\nactual_depth  = %8d, remote_frm_dur = %8d, underflows     = %8d,"
                       "\noverflows     = %8d, lost_pkts      = %8d, duplicate_frms = %8d,"
                       "\nreordered_frms= %8d, rejected_frms  = %8d, dtmf_frames    = %8d,"
                       "\ncombined_frms = %8d, adapt_enabled  = %8d, discarded_frms = %8d,"
                       "\naverage_depth = %8d",
                NMS2H_DWORD(((msp_FILTER_JITTER_STATE*)(pevent->buffer))->filter_id             ),
                NMS2H_DWORD(((msp_FILTER_JITTER_STATE*)(pevent->buffer))->rx                    ),
                NMS2H_DWORD(((msp_FILTER_JITTER_STATE*)(pevent->buffer))->rx_accept             ),
                NMS2H_DWORD(((msp_FILTER_JITTER_STATE*)(pevent->buffer))->tx                    ),
                NMS2H_DWORD(((msp_FILTER_JITTER_STATE*)(pevent->buffer))->tx_valid              ),
                NMS2H_DWORD(((msp_FILTER_JITTER_STATE*)(pevent->buffer))->requested_depth       ),
                NMS2H_DWORD(((msp_FILTER_JITTER_STATE*)(pevent->buffer))->actual_depth          ),
                NMS2H_DWORD(((msp_FILTER_JITTER_STATE*)(pevent->buffer))->remote_frame_duration ),
                NMS2H_DWORD(((msp_FILTER_JITTER_STATE*)(pevent->buffer))->underflows            ),
                NMS2H_DWORD(((msp_FILTER_JITTER_STATE*)(pevent->buffer))->overflows             ),
                NMS2H_DWORD(((msp_FILTER_JITTER_STATE*)(pevent->buffer))->lost_pkts             ),
                NMS2H_DWORD(((msp_FILTER_JITTER_STATE*)(pevent->buffer))->duplicate_frames      ),
                NMS2H_DWORD(((msp_FILTER_JITTER_STATE*)(pevent->buffer))->reordered_frames      ),
                NMS2H_DWORD(((msp_FILTER_JITTER_STATE*)(pevent->buffer))->rejected_frames       ),
                NMS2H_DWORD(((msp_FILTER_JITTER_STATE*)(pevent->buffer))->dtmf_frames           ),
                NMS2H_DWORD(((msp_FILTER_JITTER_STATE*)(pevent->buffer))->combined_frames       ),
                NMS2H_DWORD(((msp_FILTER_JITTER_STATE*)(pevent->buffer))->adapt_enabled         ),
                NMS2H_DWORD(((msp_FILTER_JITTER_STATE*)(pevent->buffer))->discarded_frames      ),
                NMS2H_DWORD(((msp_FILTER_JITTER_STATE*)(pevent->buffer))->average_depth         ));
            }
        }



        break;
    case (MSPEVN_QUERY_DONE | MSP_QRY_GET_LCN_INFO):
        if(GwConfig[nGw].MuxEp.hd == pevent->objHd)
            printf("\n  MSPEVN_QUERY_DONE MSP_QRY_GET_LCN_INFO %s", event_val);
        else if(GwConfig[nGw].vidRtpEp[EP_OUTB].hd == pevent->objHd || GwConfig[nGw].vidRtpEp[EP_INB].hd == pevent->objHd)
        {
            msp_VIDMP4_RTP_ASSEMBLY_STATUS *pVRtpStat;
            pVRtpStat = (msp_VIDMP4_RTP_ASSEMBLY_STATUS*)(pevent->buffer);

            printf("\n  MSPEVN_QUERY_DONE MSP_QRY_RTPFDX_VIDEO_MPEG4_ASSY_STATUS %s", 
                event_val);
            if((pevent->value == CTA_REASON_FINISHED) && ( pevent->buffer != NULL ))
            {
                printf("\n  totInvalidPkts  = %5d, numConsecCfg     = %5d, numCfgThenVp     = %5d,"
                       "\n  numCfgOnly      = %5d, numCfgFragThenCfg= %5d, numCfgFragThenVop= %5d,"
                       "\n  numCfgFragThenVp= %5d, numCfgFragOnly   = %5d, numInitialFrag   = %5d,"
                       "\n  startPMNotCFGErr= %5d, numVOLSearchErr  = %5d, numVOPSearchErr  = %5d,"
                       "\n  TxFrameType.I   = %5d, TxFrameType.P    = %5d, TxFrameType.B    = %5d,"
                       "\n  TxFrameType.Spr = %5d",
                    NMS2H_DWORD(pVRtpStat->totInvalidPkts    ),
                    NMS2H_DWORD(pVRtpStat->numConsecCfg      ),
                    NMS2H_DWORD(pVRtpStat->numCfgThenVp      ),
                    NMS2H_DWORD(pVRtpStat->numCfgOnly        ),
                    NMS2H_DWORD(pVRtpStat->numCfgFragThenCfg ),
                    NMS2H_DWORD(pVRtpStat->numCfgFragThenVop ),
                    NMS2H_DWORD(pVRtpStat->numCfgFragThenVp  ),
                    NMS2H_DWORD(pVRtpStat->numCfgFragOnly    ),
                    NMS2H_DWORD(pVRtpStat->numInitialFrag    ),
                    NMS2H_DWORD(pVRtpStat->startPMNotCFGError),
                    NMS2H_DWORD(pVRtpStat->numVOLSearchErrors),
                    NMS2H_DWORD(pVRtpStat->numVOPSearchErrors),
                    NMS2H_DWORD(pVRtpStat->txFrameTypes.numI ),
                    NMS2H_DWORD(pVRtpStat->txFrameTypes.numP ),
                    NMS2H_DWORD(pVRtpStat->txFrameTypes.numB ),
                    NMS2H_DWORD(pVRtpStat->txFrameTypes.numSprite) );
            }
        }
        break;
    case (MSPEVN_QUERY_DONE | MSP_QRY_GET_SN_MODE):
        printf("\n  MSPEVN_QUERY_DONE MSP_QRY_GET_SN_MODE %s", event_val);
        break;
    case (MSPEVN_QUERY_DONE | MSP_QRY_GET_AL_MODE):
        printf("\n  MSPEVN_QUERY_DONE MSP_QRY_GET_AL_MODE %s", event_val);
        break;
    case (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_MUX_CONFIG):
        if(GwConfig[nGw].MuxEp.hd == pevent->objHd)
            mmPrint(nGw, T_MSPP, "MSPEVN_SENDCOMMAND_DONE  MSP_CMD_MUX_CONFIG %s. Handle %08x\n",
				 event_val, pevent->objHd);
        else if(GwConfig[nGw].AudByChan[EP_OUTB].hd == pevent->objHd || GwConfig[nGw].AudByChan[EP_INB].hd == pevent->objHd)
        {
            printf("\n  MSPEVN_SENDCOMMAND_DONE  MSP_CMD_AMR_SET_AUDIO_RATE %s", event_val);
            printf("\n  Event Handle %08x", pevent->objHd);
        }
        else if(GwConfig[nGw].AudByChan[EP_OUTB].hd == pevent->objHd || GwConfig[nGw].AudByChan[EP_INB].hd == pevent->objHd)
        {
            mmPrint(nGw, T_MSPP, "MSPEVN_SENDCOMMAND_DONE  MSP_CMD_JITTER_CHG_DEPTH %s\n", event_val);
		  }
        else if(GwConfig[nGw].vidRtpEp[EP_OUTB].hd == pevent->objHd || GwConfig[nGw].vidRtpEp[EP_INB].hd == pevent->objHd)
        {
            mmPrint(nGw, T_MSPP, "MSPEVN_SENDCOMMAND_DONE  MSP_CMD_RTPFDX_CONFIG %s\n", event_val);
		}
        break;
    case (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_MUX_SEND_MC_INFO):
        if(GwConfig[nGw].MuxEp.hd == pevent->objHd)
            mmPrint(nGw, T_MSPP, "MSPEVN_SENDCOMMAND_DONE  MSP_CMD_MUX_SEND_MC_INFO %s\n", event_val);
        break;
    case (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_MUX_SEND_LCN_INFO):
        mmPrint(nGw, T_MSPP, "MSPEVN_SENDCOMMAND_DONE  MSP_CMD_MUX_SEND_LCN_INFO %s\n", event_val);
        break;
    // Case used for MSP_CMD_RTPFDX_MAP and MSP_CMD_MUX_SEND_CAPABILITY_SETTING
    case (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_MUX_SEND_CAPABILITY_SETTING):
        if(GwConfig[nGw].MuxEp.hd == pevent->objHd)
            mmPrint(nGw, T_MSPP, "MSPEVN_SENDCOMMAND_DONE  MSP_CMD_MUX_SEND_CAPABILITY_SETTING %s\n", event_val);
        else if (GwConfig[nGw].AudRtpEp[EP_OUTB].hd == pevent->objHd || GwConfig[nGw].AudRtpEp[EP_INB].hd == pevent->objHd ||
                 GwConfig[nGw].vidRtpEp[EP_OUTB].hd == pevent->objHd || GwConfig[nGw].vidRtpEp[EP_INB].hd == pevent->objHd)
        {
            printf("\n  MSPEVN_SENDCOMMAND_DONE  MSP_CMD_RTPFDX_MAP %s", event_val);
        }
        break;
    case (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_MUX_SEND_H245_MSG):
        mmPrint(nGw, T_MSPP, "MSPEVN_SENDCOMMAND_DONE  MSP_CMD_MUX_SEND_H245_MSG %s. Handle %08x\n",
			 event_val, pevent->objHd);
        break;
    case (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_MUX_SET_SN_MODE):
        mmPrint(nGw, T_MSPP, "MSPEVN_SENDCOMMAND_DONE  MSP_CMD_MUX_SET_SN_MODE %s\n", event_val);
        break;
    case (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_MUX_SET_AL_MODE):
        mmPrint(nGw, T_MSPP, "MSPEVN_SENDCOMMAND_DONE  MSP_CMD_MUX_SET_AL_MODE %s\n", event_val);
        break;
    case (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_MUX_START_DATA_DUMP):
        if(GwConfig[nGw].vidRtpEp[EP_OUTB].hd == pevent->objHd || GwConfig[nGw].vidRtpEp[EP_INB].hd == pevent->objHd)
            mmPrint(nGw, T_MSPP, "MSPEVN_SENDCOMMAND_DONE  MSP_CMD_RTPFDX_VIDEO_RTP_PKTSZ_CTRL %s\n", event_val);
        else
        mmPrint(nGw, T_MSPP, "MSPEVN_SENDCOMMAND_DONE  MSP_CMD_MUX_START_DATA_DUMP %s\n", event_val);
        break;
    case (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_MUX_STOP_DATA_DUMP):
        if(GwConfig[nGw].vidRtpEp[EP_OUTB].hd == pevent->objHd || GwConfig[nGw].vidRtpEp[EP_INB].hd == pevent->objHd)
            mmPrint(nGw, T_MSPP, "MSPEVN_SENDCOMMAND_DONE  MSP_CMD_RTPFDX_DISCARD_PENDING_PFRAMES %s\n", event_val);
        else    
            mmPrint(nGw, T_MSPP, "MSPEVN_SENDCOMMAND_DONE  MSP_CMD_MUX_STOP_DATA_DUMP %s\n", event_val);
        break;
    case (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_RTPFDX_IFRAME_NOTIFY_CTRL):
        if(GwConfig[nGw].vidRtpEp[EP_OUTB].hd == pevent->objHd || GwConfig[nGw].vidRtpEp[EP_INB].hd == pevent->objHd)
            mmPrint(nGw, T_MSPP, "MSPEVN_SENDCOMMAND_DONE  MSP_CMD_RTPFDX_IFRAME_NOTIFY_CTRL\n");
        break;    
    case (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_RTPFDX_OUT_OF_BAND_DCI):
        if(GwConfig[nGw].vidRtpEp[EP_OUTB].hd == pevent->objHd || GwConfig[nGw].vidRtpEp[EP_INB].hd == pevent->objHd)
            mmPrint(nGw, T_MSPP, "MSPEVN_SENDCOMMAND_DONE  MSP_CMD_RTPFDX_OUT_OF_BAND_DCI\n");
        break;
    case (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_RTPFDX_RTTS_CTRL):
        if(GwConfig[nGw].vidRtpEp[EP_OUTB].hd == pevent->objHd || GwConfig[nGw].vidRtpEp[EP_INB].hd == pevent->objHd)
            mmPrint(nGw, T_MSPP, "MSPEVN_SENDCOMMAND_DONE  MSP_CMD_RTPFDX_RTTS_CTRL\n");
        break;
    case (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_RTPFDX_STOP_VIDEO_RX):
        if(GwConfig[nGw].vidRtpEp[EP_OUTB].hd == pevent->objHd || GwConfig[nGw].vidRtpEp[EP_INB].hd == pevent->objHd)
            mmPrint(nGw, T_MSPP, "MSPEVN_SENDCOMMAND_DONE  MSP_CMD_RTPFDX_STOP_VIDEO_RX\n");
        break;        
    case (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_MUX_SET_MC_TABLE_ENTRY):
        if(GwConfig[nGw].MuxEp.hd == pevent->objHd)
            mmPrint(nGw, T_MSPP, "MSPEVN_SENDCOMMAND_DONE  MSP_CMD_MUX_SET_MC_TABLE_ENTRY %s\n", event_val);
        break;
    case (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_MUX_GET_MC_TABLE_ENTRY):
        if(GwConfig[nGw].MuxEp.hd == pevent->objHd)
			printf("\n  MSPEVN_SENDCOMMAND_DONE  MSP_CMD_MUX_GET_MC_TABLE_ENTRY %s", event_val);
        break;
    case (MSPEVN_DEMUX_PERIODIC_STATS):
        if((GwConfig[nGw].MuxEp.hd == pevent->objHd) && (pevent->buffer != NULL) && (pevent->size > 0))
            ShowDemuxPeriodicStats(nGw, (msp_ENDPOINT_DEMUX_PERIODIC_STATS *)pevent->buffer);
        break;
    case (MSPEVN_DEMUX_CRC_ERR_REPORT):
        if(GwConfig[nGw].MuxEp.hd == pevent->objHd)
        {
            msp_ENDPOINT_DEMUX_CRC_ERROR_REPORTS *err = (msp_ENDPOINT_DEMUX_CRC_ERROR_REPORTS*)pevent->buffer;
            switch( NMS2H_DWORD(err->error_check_type) )
            {
                case 1:  // Video CRC Error
                    mmPrint(nGw, T_STAT, "VIDEO CRC ERROR Received at Demux, Error Count = %d\n",
                            NMS2H_DWORD(err->total_num_errors)  );
                    break;
                case 2:  // Audio CRC Error
                    mmPrint(nGw, T_STAT, "AUDIO CRC ERROR Received at Demux, Error Count = %d\n",
                            NMS2H_DWORD(err->total_num_errors) );
                    break;
                case 4:  // Golay
                    mmPrint(nGw, T_STAT, "HEADER GOLAY CODING ERROR Received at Demux, Error Count = %d\n",
                            NMS2H_DWORD(err->total_num_errors) );
                    break;
            }
        }
        break;        
        
        
        
        
    case (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_VIDMP4_ADD_SPECIAL_HDR):
            printf("\n  MSPEVN_SENDCOMMAND_DONE  MSP_CMD_VIDMP4_ADD_SPECIAL_HDR %s", event_val);
        break;
    case (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_VIDMP4_REMOVE_SPECIAL_HDR):
            printf("\n  MSPEVN_SENDCOMMAND_DONE  MSP_CMD_VIDMP4_REMOVE_SPECIAL_HDR %s", event_val);
        break;
    case (MSPEVN_RTCP_REPORT):
        printf("\n  MSPEVN_RTCP_REPORT %s", event_val);
        break;

    case MSPEVN_VIDEO_IFRAME_H324:
        if(GwConfig[nGw].vidRtpEp[EP_OUTB].hd == pevent->objHd || GwConfig[nGw].vidRtpEp[EP_INB].hd == pevent->objHd)
            mmPrint(nGw, T_MSPP, "Unsolicited Event MSPEVN_VIDEO_IFRAME_H324 %s\n", event_val);
        break;    
    case MSPEVN_VIDEO_IFRAME_RTPIP:
        if(GwConfig[nGw].vidRtpEp[EP_OUTB].hd == pevent->objHd || GwConfig[nGw].vidRtpEp[EP_INB].hd == pevent->objHd)
            mmPrint(nGw, T_MSPP, "Unsolicited Event MSPEVN_VIDEO_IFRAME_RTPIP %s\n", event_val);
        break;    

    case MSPEVN_RFC2833_REPORT:
        if(GwConfig[nGw].AudRtpEp[EP_OUTB].hd == pevent->objHd || GwConfig[nGw].AudRtpEp[EP_INB].hd == pevent->objHd)
          mmPrint(nGw, T_MSPP, "Unsolicited Event MSPEVN_RFC2833_REPORT: id %d vol %d, duration %d: %s\n",
                  ((DISASM_DTMF_EVENT_STRUCT *)pevent->buffer)->EvtID,
                  ((DISASM_DTMF_EVENT_STRUCT *)pevent->buffer)->EvtVol & 0x7F,
                  ((DISASM_DTMF_EVENT_STRUCT *)pevent->buffer)->EvtDuration,
                  (((DISASM_DTMF_EVENT_STRUCT *)pevent->buffer)->EvtVol & 0x80) ? "END" : "BEGIN");
        break;    

    case (MSPEVN_VIDEO_RX_STOPPED):
        if(GwConfig[nGw].vidRtpEp[EP_OUTB].hd == pevent->objHd || GwConfig[nGw].vidRtpEp[EP_INB].hd == pevent->objHd)
            mmPrint(nGw, T_MSPP, "Unsolicited Event  MSPEVN_VIDEO_RX_STOPPED %s\n", event_val);
        break;    

    // If it is an NCC Event, determine which gateway, and process the event
    case NCCEVN_START_PROTOCOL_DONE:
    case NCCEVN_ACCEPTING_CALL:
    case NCCEVN_ANSWERING_CALL:
    case NCCEVN_CALL_PROCEEDING:
    case NCCEVN_INCOMING_CALL:
    case NCCEVN_PLACING_CALL:
    case NCCEVN_REJECTING_CALL:
    case NCCEVN_CALL_RELEASED:
    case NCCEVN_SEIZURE_DETECTED:
    case NCCEVN_RECEIVED_DIGIT:
    case PBX_EVENT_DISCONNECT:
    case PBX_EVENT_PLACE_CALL:			
    case PBX_EVENT_PARTNER_CONNECTED: 
    case NCCEVN_REMOTE_ANSWERED:  
	case NCCEVN_CALL_DISCONNECTED:
	case NCCEVN_CALL_CONNECTED:	
	case NCCEVN_REMOTE_ALERTING:
	case NCCEVN_EXTENDED_CALL_STATUS_UPDATE:
	    ProcessNCCEvent( pevent, &(GwConfig[nGw].isdnCall ));
		
		switch( pevent->id ) {
			case NCCEVN_CALL_CONNECTED:
				GwConfig[nGw].callNum++; 
                GwConfig[nGw].start = msTimeStamp();
				break;

			case NCCEVN_CALL_RELEASED:
      			// If this is a load test and the maximum number of ports was reached
           		if( (gwParm.nPorts > 1) && (GwConfig[nGw].callNum == (DWORD)gwParm.nCalls) ) {
   	        		mmPrint(nGw, T_GWAPP, "Maximum number of calls reached.\n");
   		        	break_flag = TRUE;
       			} else {
        			if( gwParm.auto_place_call && !gwParm.useVideoXc ) {
                	    // Starts the timer to place the next call
   	   	        		if( adiStartTimer( pevent->ctahd, gwParm.between_calls_time, 1 ) != SUCCESS ) 
   	    	   	    		mmPrintPortErr(nGw, "Failed to start the ADI timer.\n");
                        
   		    	        	mmPrint( nGw, T_GWAPP, "adiStartTimer started.\n");
       		       	}				

                    updateDuration(nGw, msDiff(GwConfig[nGw].start, msTimeStamp()) );
                }          
			    break;				
                
			default:
				break;			
		} //end internal switch
        break;

	case ADIEVN_TIMER_DONE:
		mmPrint(nGw, T_SRVAPP, "ADIEVN_TIMER_DONE is received.\n");			
		if( GwConfig[nGw].callNum < (DWORD)gwParm.nCalls ) {
			autoPlaceCall(nGw, gwParm.dial_number);
		} 			
		break;    
				
	case ADIEVN_RECORD_DONE:
	case ADIEVN_PLAY_DONE:
    case ADIEVN_PLAY_BUFFER_REQ:
    case ADIEVN_RECORD_BUFFER_FULL:
    case ADIEVN_RECORD_STARTED:
		ProcessADIEvent( pevent );		
		break;    
	
	case H324EVN_START_DONE :
	case H324EVN_LOCAL_TERM_CAPS :
	case H324EVN_REMOTE_CAPABILITIES :
	case H324EVN_MEDIA_SETUP_DONE :
	case H324EVN_VIDEO_FAST_UPDATE :
	case H324EVN_END_SESSION :
	case H324EVN_USER_INDICATION :
	case H324EVN_ROUND_TRIP_DELAY :
	case H324EVN_STOP_DONE :
	case H324EVN_H245_INTERNAL_ERROR :
	case H324EVN_CALL_SETUP_FAILED :
	case H324EVN_MEDIA_SETUP_FAILED :
	case H324EVN_VIDEO_CHANNEL_SETUP_FAILED  :
	case H324EVN_CHANNEL_CLOSED :
	case H324EVN_VIDEO_OLC_TIMER_EXPIRED :
	case H324EVN_END_SESSION_TIMER_EXPIRED :
	case H324EVN_END_SESSION_DONE :
	case H324EVN_H223_SKEW_INDICATION :
	case H324EVN_VENDORID_INDICATION :
	case H324EVN_VIDEOTEMPORALSPATIALTRADEOFF_INDICATION :
	case H324EVN_PASSTHRU_PLAY_RFC2833_DONE:
	case H324EVN_PASSTHRU_DTMF_MODE_DONE:
	case H324EVN_LCD:				
        ProcessH324Event( pevent, nGw );
		break;

#ifdef USE_TRC
    case TRCEVN_START_CHANNEL_DONE:   
    case TRCEVN_STOP_CHANNEL_DONE:   
    case TRCEVN_CONFIG_CHANNEL_DONE:   
    case TRCEVN_CHANNEL_FAILURE:  
        ProcessTrcEvent( pevent, nGw );
        break;        
        
#endif        

   case CDE_LOCAL_TROMBONE:
   case CDE_REMOTE_TROMBONE:
   case CDE_CONFIRM_TROMBONE:
   case CDE_STOP_TROMBONE:
   case CDE_STOP_REMOTE_TROMBONE:
   case CDE_STOP_CONFIRM_TROMBONE:
   case CDE_REMOTE_FAST_VIDEO_UPDATE:
   case CDE_REMOTE_VIDEOTEMPORALSPATIALTRADEOFF:
   case CDE_REMOTE_SKEW_INDICATION:
   case CDE_STOP_VIDEO_RX:
   case CDE_STOP_REMOTE_VIDEO_RX:
      ProcessTromboneEvent( pevent, nGw );
      break;
      		
    default:            
        break;
    } //End Switch On Event ID
	

    // Always release the mspEventBuffer after processing an event
    if( pevent->id      >=  MSPEVN_BASE   && 
        pevent->id      <=  MSPEVN_UNSOL_MAX)
    {
        ReleaseMSPEvent( pevent );
    }
    else
    {
        // Free any event buffer here
        if(pevent->buffer != NULL)
        {
            ctaFreeBuffer ( pevent->buffer );
        }
    }

	return break_flag;
} //End ProcessCTAEvent


////////////////////////////////////////////////////////////////////////////////////
//  ReleaseMSPEvent
//
//  Function used to release the Buffer for all MSP events
//  Always checks to see if it is a valid MSP Event with a buffer before releasing 
//  the buffer.
////////////////////////////////////////////////////////////////////////////////////
void ReleaseMSPEvent( CTA_EVENT *pevent )
{
	DWORD ret;
	
	if( pevent->id      >=  MSPEVN_BASE       && 
        pevent->id      <=  MSPEVN_UNSOL_MAX  &&      
        pevent->size    !=  0                 && 
        pevent->buffer  !=  NULL	)
	{
		ret = mspReleaseBuffer( pevent->objHd, pevent->buffer); 
		if ( ret != SUCCESS) {
			mmPrintErr( "Mspp release buffer failed! mspReleaseBuffer returned: %d (0x%x)\n", ret, ret );
		}
	}
}


/*---------------------------------------------------------------------------
 * ErrorHandler
 *
 *  Called by CTAccess before it returns an error from any function.
 *--------------------------------------------------------------------------*/
DWORD NMSSTDCALL ErrorHandler( CTAHD ctahd, DWORD error, char *errtxt,
                                 char *func )
{
    /* print out the error and return */
    mmPrintErr( " !! ErrorHandler: Call to: %s, caused: %s, on ctahd=%#x !!\n", func , errtxt, ctahd);
    return error;
}

char *num_2_stars(int i)
{
    switch( i )
    {
    case  0:  return "           ";
    case  1:  return "*          ";
    case  2:  return "**         ";
    case  3:  return "***        ";
    case  4:  return "****       ";
    case  5:  return "*****      ";
    case  6:  return "******     ";
    case  7:  return "*******    ";
    case  8:  return "********   ";
    case  9:  return "*********  ";
    case 10:  return "********** ";
    case 11:  return "***********";
    default:  return "xxxxxxxxxxx";
    }
}


void ShowDemuxPeriodicStats(int nGw, msp_ENDPOINT_DEMUX_PERIODIC_STATS *stat)
{

    DWORD viderr = NMS2H_DWORD( stat->num_videoCRCerrors);
    int vidok    = NMS2H_DWORD( stat->num_videoCRCsuccesses);
    DWORD auderr = NMS2H_DWORD( stat->num_audioCRCerrors);
    int audok    = NMS2H_DWORD( stat->num_audioCRCsuccesses);

    if((GwConfig[nGw].DemuxStats.num_audioCRCerrors == auderr) &&
       (GwConfig[nGw].DemuxStats.num_videoCRCerrors == viderr))
        return;

    mmPrint(nGw, T_STAT, "Video(Err/OK)=%03d/%05d (%1.4f%%) %s, Audio(Err/OK)=%03d/%05d (%1.4f%%) %s\n",
            viderr, vidok, 100.0*viderr/(viderr+vidok),
            num_2_stars(viderr - GwConfig[nGw].DemuxStats.num_videoCRCerrors),
            auderr, audok, 100.0*auderr/(auderr+audok),
            num_2_stars(auderr - GwConfig[nGw].DemuxStats.num_audioCRCerrors) );
            
    GwConfig[nGw].DemuxStats.num_audioCRCerrors = auderr;
    GwConfig[nGw].DemuxStats.num_videoCRCerrors = viderr;
}

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
    
#ifdef USE_TRC	
    switch( pEvent->id ) {
        case TRCEVN_VTP_FAILURE:
        {
            char strReason[100];
            int reason;
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
        }
        case TRCEVN_VTP_RECOVERED:
            mmPrintAll("systemMonitor: TRCEVN_VTP_RECOVERED event received. VTP ID=%d\n", 
                    pEvent->value );                        
            break;
        default:
            break;
    }
#endif // USE_TRC

    if(pEvent->buffer != NULL) {
        ctaFreeBuffer( pEvent->buffer );
    }        
    
    return SUCCESS;    
}










