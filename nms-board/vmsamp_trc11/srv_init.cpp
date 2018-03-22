/**********************************************************************
*  File - srv_init.cpp
*
*  Description - Video Messaging Server initialization functions
*
*  2005  NMS Communications Corporation.
*
***********************************************************************/

///////////////////
// Include Files //
///////////////////
#include <malloc.h>

#include "mega_global.h"
#include "srv_types.h"
#include "srv_def.h"
#include "vm_trace.h"

int ServiceCount;
CTA_SERVICE_DESC ServDesc[NUM_CTA_SERVICES]; /* for ctaOpenServices */
char* ServDesc_Close[NUM_CTA_SERVICES];

static void SelectEncoders();
static int srvOpenPort(CTAQUEUEHD QueueHd, CTAHD *CTAccessHd, 
					   tTimeslot_Mode Mode, int port);

int initServer() 
{
    DWORD                  i,
                           ret;
    CTA_EVENT              event;
    msp_FILTER_ENCODE_CMD *pFilterEncodeCmd = 0;
    
    pFilterEncodeCmd = (msp_FILTER_ENCODE_CMD*)malloc(sizeof(msp_FILTER_ENCODE_CMD));
    pFilterEncodeCmd->value = 0;
    // Initialize  audioMode, ADIaudEncoder, FusChanType, FusEncodeRate
    SelectEncoders();
    
	ctaSetErrorHandler( NULL, NULL );

    switch (mmParm.nBoardType)
    {
        case 6000:
            mmParm.base_fusion_timeslot = BASE_FUSION_TIMESLOT_CG6000;
    	    mmParm.base_ivr_timeslot    = BASE_IVR_TIMESLOT_CG6000;
            mmParm.Stream               = 16;
            break;
            
        case 6500:
            mmParm.base_fusion_timeslot = BASE_FUSION_TIMESLOT_CG6500;
    	    mmParm.base_ivr_timeslot    = BASE_IVR_TIMESLOT_CG6500;
            mmParm.Stream               = 64;            
            break;

        case 6565:
            mmParm.base_fusion_timeslot = 210;
    	    mmParm.base_ivr_timeslot    = 120;
            mmParm.Stream               = 64;
            break;
           
        default:
       	    mmPrintAll("Invalid board type ==> %dn",mmParm.nBoardType);
	        return FAILURE;
    }
    
    	
	InitMsppConfiguration();
	
	for (i=0; i<mmParm.nPorts; i++)
	{
		///////////////////////////////////////////////////////////////
		// Create Native Video Channel: ADI Port and Video RTP Endpoint
		///////////////////////////////////////////////////////////////
		if(mmParm.Video_Format != NO_VIDEO)
		{
			mmPrint(i, T_CTA, "Create Native Video context...\n");
			ret = srvOpenPort(GwConfig[i].hCtaQueueHd, 
                    &VideoCtx[i].ctahd, NO_TS_MODE, i);			

	        if (ret != SUCCESS)
            {
		        mmPrintPortErr(i, "srvOpenPort failed, ret=%d.\n", ret);
		        return FAILURE;
	        }

            ret = mspCreateEndpoint (VideoCtx[i].ctahd, 
									&VideoCtx[i].rtpEp.Addr, 
									&VideoCtx[i].rtpEp.Param, 
									&VideoCtx[i].rtpEp.hd);
	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("\nmspCreateEndpoint returned %d.", ret);
		        return FAILURE;
	        }

            ret = WaitForSpecificEvent(i, GwConfig[i].hCtaQueueHd, 
						MSPEVN_CREATE_ENDPOINT_DONE, &event);
	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("\nMSPEVN_CREATE_ENDPOINT_DONE event failure, ret= %x,"
                        "\nevent.value=%x, board error (event.size field) %x\n\n",
			            ret, event.value, event.size);
		        return FAILURE;
	        }
			mmPrint(i, T_MSPP, "Native Video RTP Endpoint is created, epHd=%x...\n", VideoCtx[i].rtpEp.hd);
        } // End If Video 
		/////////////////////////////////////////////////////////////////////////////////////////
		// Create Audio Channel With Silence Detection: ADI Port and Audio RTP Endpoint
		/////////////////////////////////////////////////////////////////////////////////////////
		if ((mmParm.audioMode1 == NATIVE_SD) ||
            (mmParm.audioMode1 == FUS_IVR))
        {
            // open a Port
			mmPrint(i, T_CTA, "Create Audio context...\n");
			ret = srvOpenPort(GwConfig[i].hCtaQueueHd, 
                    &AudioCtx[i].ctahd, TS_MODE, i);
	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("\nsrvOpenPort failed, ret=%d.", ret);
		        return FAILURE;
	        }
			// Create Audio RTP Endpoint
            ret = mspCreateEndpoint (AudioCtx[i].ctahd, 
									&AudioCtx[i].rtpEp.Addr, 
									&AudioCtx[i].rtpEp.Param, 
									&AudioCtx[i].rtpEp.hd);
	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("\nmspCreateEndpoint returned %d.", ret);
		        return FAILURE;
	        }
					
            ret = WaitForSpecificEvent (i, GwConfig[i].hCtaQueueHd, 
						MSPEVN_CREATE_ENDPOINT_DONE, &event);
	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("\nMSPEVN_CREATE_ENDPOINT_DONE event failure, ret= %x,"
                        "\nevent.value=%x, board error (event.size field) %x\n\n",
			            ret, event.value, event.size);
		        return FAILURE;
	        } 
			mmPrint(i, T_MSPP, "Native Audio RTP Endpoint is created, epHd=%x...\n", AudioCtx[i].rtpEp.hd);
            // Create a DSO Endpoint
            ret = mspCreateEndpoint (AudioCtx[i].ctahd, 
									&AudioCtx[i].FusionDS0Ep.Addr, 
									&AudioCtx[i].FusionDS0Ep.Param, 
									&AudioCtx[i].FusionDS0Ep.hd);
	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("\nmspCreateEndpoint returned %d.", ret);
		        return FAILURE;
	        }
			
            ret = WaitForSpecificEvent(i, GwConfig[i].hCtaQueueHd, 
						MSPEVN_CREATE_ENDPOINT_DONE, &event);
	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("\nMSPEVN_CREATE_ENDPOINT_DONE event failure, ret= %x,"
                        "\nevent.value=%x, board error (event.size field) %x\n\n",
			            ret, event.value, event.size);
		        return FAILURE;
	        } 
			mmPrint(i, T_MSPP, "Native Audio DS0 Endpoint is created...\n");
			// Create a Fusion Channel
			ret = mspCreateChannel (AudioCtx[i].ctahd,
								   &AudioCtx[i].FusionCh.Addr,
								   &AudioCtx[i].FusionCh.Param, 
								   &AudioCtx[i].FusionCh.hd);
	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("\nmspCreateChannel returned %d.", ret);
		        return FAILURE;
	        }
					
			ret = WaitForSpecificEvent(i, GwConfig[i].hCtaQueueHd, MSPEVN_CREATE_CHANNEL_DONE, &event);
	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("\nMSPEVN_CREATE_CHANNEL_DONE event failure, ret= %x,"
                        "\nevent.value=%x, board error (event.size field) %x\n\n",
			            ret, event.value, event.size);
		        return FAILURE;
	        }
			mmPrint(i, T_MSPP, "Fusion Audio Channel is created...\n");

			// Connect Audio RTP EP and Fusion DS0 EP using Fusion Channel
		    ret = mspConnect(AudioCtx[i].rtpEp.hd, AudioCtx[i].FusionCh.hd, AudioCtx[i].FusionDS0Ep.hd);
	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("\nError: mspConnect returned %d.", ret);
		        return FAILURE;
	        }
		    ret = WaitForSpecificEvent(i, GwConfig[i].hCtaQueueHd, MSPEVN_CONNECT_DONE, &event);
	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("\nMSPEVN_CONNECT_DONE event failure, ret= %x,"
                        "\nevent.value=%x, board error (event.size field) %x\n\n",
			            ret, event.value, event.size);
		        return FAILURE;
	        }
            mmPrint(i, T_MSPP, "Connect the channel to the Audio RTP and Fusion DS0 endpoints done...\n");

        } // END If NATIVE AUDIO With silence detection or Fusion IVR
        
		/////////////////////////////////////////////////////////////////////////////////////////
		// Create Audio Channel Native with no silence Detection:
		/////////////////////////////////////////////////////////////////////////////////////////
        else if (mmParm.audioMode1 == NATIVE_NO_SD)
		{
			mmPrint (i, T_CTA, "Create Native Audio context...\n");
			srvOpenPort (GwConfig[i].hCtaQueueHd, 
                        &AudioCtx[i].ctahd, NO_TS_MODE, i);

	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("\nsrvOpenPort failed, ret=%d.", ret);
		        return FAILURE;
	        }

            ret = mspCreateEndpoint (AudioCtx[i].ctahd, 
									&AudioCtx[i].rtpEp.Addr, 
									&AudioCtx[i].rtpEp.Param, 
									&AudioCtx[i].rtpEp.hd);
	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("\nmspCreateEndpoint returned %d.", ret);
		        return FAILURE;
	        }

            ret = WaitForSpecificEvent(i, GwConfig[i].hCtaQueueHd, 
                                       MSPEVN_CREATE_ENDPOINT_DONE, &event);
	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("\nMSPEVN_CREATE_ENDPOINT_DONE event failure, ret= %x,"
                        "\nevent.value=%x, board error (event.size field) %x\n\n",
			            ret, event.value, event.size);
		        return FAILURE;
	        }

			mmPrint(i, T_MSPP, "Native Audio RTP Endpoint is created, epHd=%x...\n", AudioCtx[i].rtpEp.hd);
        } // native audio
	}     // for nPorts
	
   	if ((mmParm.audioMode1 == NATIVE_SD) ||
        (mmParm.audioMode1 == FUS_IVR))
    {
	    // Setup Switch and make switching connections
	   	if ((ret = SetupSwitch(0)) != SUCCESS)
        {
		    mmPrintAll("\nFailed to setup switch\n");
		    return FAILURE;
	   	}      
	    mmPrint(i, T_CTA, "Switch is ready...\n"); 
   	}
	
	return SUCCESS;
}

int SetupSwitch( int port )
{
    CTA_EVENT event;
	CTAHD hSwi;
	DWORD ret;
	CTA_SERVICE_DESC SWIService[] = { { {"SWI", "SWIMGR"}, {0}, {0}, {0} },};
	SWI_TERMINUS input, output;
	int res,i;
        
    // Create a switching context
	ret = ctaCreateContext(GwConfig[port].hCtaQueueHd, 0, "hSwi", &hSwi);
	if(ret  != SUCCESS)
	{
	    mmPrintAll("Failed to create CT Access switch context, invalid queue handle %x.\n",GwConfig[port].hCtaQueueHd);
	    return FAILURE;
	}	    
	mmPrint(port, T_CTA, "ctaCreateContext(SWI) <-- SUCCESS\n");

	/*** Open SWI service ***/
	ret = ctaOpenServices(hSwi, SWIService, 1); 
	if(ret  != SUCCESS)
	{
	    mmPrintAll("Failed to open SWI service..Return code is 0x%x.\n",ret);
	    return FAILURE;
	}
	WaitForSpecificEvent(port, GwConfig[port].hCtaQueueHd, CTAEVN_OPEN_SERVICES_DONE, &event );
	if ( event.value != CTA_REASON_FINISHED )
	{
	    mmPrintAll("Open SWI service failed...event.value=0x%x\n",event.value);
	    return FAILURE;
	}
	mmPrint(port, T_CTA, "SWI Service has been opened...\n");
	    
	/*** Open and reset the switchblock for the CG boards ***/
	ret = swiOpenSwitch( hSwi, "cg6ksw", mmParm.Board, 0x0, &hSrvSwitch0);
	if(ret != SUCCESS)
	{
	    mmPrintAll("Failed to open switchblock on CG6000 ..Return code is 0x%x.\n",ret);
	    return FAILURE;
	}
   
    mmPrint(port, T_CTA, "hSrvSwitch0 = %x\n\n",hSrvSwitch0);
	for(i=0; i < (int)mmParm.nPorts; i++)
	{
		// Connect Fusion DS0 EP and IVR Context using SWI
		//Connect Fusion to IVR
		input.bus       = MVIP95_LOCAL_BUS;
		output.bus      = MVIP95_LOCAL_BUS;
        
        switch( mmParm.nBoardType ) {
            case 6000:
        		input.stream    = 16;
	        	output.stream   = 17;
                break;
            
            case 6500:
            case 6565:
        		input.stream    = 64;
	        	output.stream   = 65;
                break;
            
            default:
        	    mmPrintAll("Invalid board type ==> %dn",mmParm.nBoardType);
	            return FAILURE;
        }
        
        input.timeslot  = i + mmParm.base_fusion_timeslot;
	    output.timeslot = i + mmParm.base_ivr_timeslot;
        
		mmPrint(i, T_CTA, "Switch together Fusion EP %02d:%02d and ADI Context %02d:%02d\n",
			     input.stream, input.timeslot, output.stream, output.timeslot);
		res = swiMakeConnection(hSrvSwitch0, &input, &output, 1); 
		if ( res != SUCCESS )
		{
			mmPrintAll("Failed to make switch connection..Return code is 0x%x.\n", res);
			return FAILURE;
		}
        
   		//Connect IVR to Fusion
        input.timeslot  = i + mmParm.base_ivr_timeslot;
	    output.timeslot = i + mmParm.base_fusion_timeslot;
                
		mmPrint(i, T_CTA, "Switch together ADI Context %02d:%02d and Fusion EP %02d:%02d\n",
			    input.stream, input.timeslot, output.stream, output.timeslot);
		res = swiMakeConnection(hSrvSwitch0, &input, &output, 1); 
		if ( res != SUCCESS )
		{
			mmPrintAll("Failed to make switch connection..Return code is 0x%x.\n", res);
			return FAILURE;
		}
	}
	return SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////////
//  srvOpenPort()
//  Function to create each port.  Includes the following steps:
//		Create a new CTA context
//		Define the services associated with that context
//		Open the CTA services
/////////////////////////////////////////////////////////////////////////////////
int srvOpenPort(CTAQUEUEHD QueueHd, CTAHD *CTAccessHd, 
				tTimeslot_Mode Mode, int port)
{
	int ret = SUCCESS;
	CTA_EVENT event;
	
	/////////////////////////////////
	//////// ctaCreateContext ///////
	/////////////////////////////////
	if ((ret = ctaCreateContext(QueueHd, 0, "Multi-Media App", CTAccessHd)) != SUCCESS) {
		/*** This routine should not recieve an invalid queue handle ***/
		mmPrintPortErr(port, "Failed to create CT Access context, invalid queue handle %x.\n", QueueHd);
		return ret;
	}
	mmPrint(port, T_CTA, "CT Access context created, CtaHd = %x\n", CTAccessHd );

    /*** Open the services on the contexts ***/
	ServiceCount = 0;
	// ADI service
	ServDesc[ServiceCount].name.svcname      = "ADI";
	ServDesc[ServiceCount].name.svcmgrname   = "ADIMGR";
	ServDesc[ServiceCount].mvipaddr.board    = mmParm.Board;
	ServDesc[ServiceCount].mvipaddr.stream   = mmParm.Stream;
	ServDesc[ServiceCount].mvipaddr.timeslot = port+mmParm.base_ivr_timeslot;
	if (Mode == NO_TS_MODE)
		ServDesc[ServiceCount].mvipaddr.mode = 0;	// mode=0 in case of Native channel
	else	
		ServDesc[ServiceCount].mvipaddr.mode = ADI_VOICE_DUPLEX;

	ServDesc_Close[ServiceCount] = "ADI";
	ServiceCount++;

	// MSPP service 
	ServDesc[ServiceCount].name.svcname      = "MSP";
	ServDesc[ServiceCount].name.svcmgrname   = "MSPMGR";
	ServDesc[ServiceCount].mvipaddr.board    = mmParm.Board;
	ServDesc[ServiceCount].mvipaddr.stream   = mmParm.Stream;
	ServDesc[ServiceCount].mvipaddr.timeslot = port+mmParm.base_fusion_timeslot;
	if (Mode == NO_TS_MODE)
		ServDesc[ServiceCount].mvipaddr.mode = 0;	// mode=0 in case of Native channel
	else	
		ServDesc[ServiceCount].mvipaddr.mode = ADI_VOICE_DUPLEX;
	ServDesc_Close[ServiceCount] = "MSP";
	ServiceCount++;

	////////////////////////////////
	//////// ctaOpenServices ///////
	////////////////////////////////
	if((ret = ctaOpenServices( *CTAccessHd, ServDesc, ServiceCount) != SUCCESS ))
	{
		mmPrintPortErr(port, "Failed to open services... Return code is 0x%x.\n", ret);
		return ret;
	}

	/*** Wait for services to be opened asynchronously ***/
	ret = WaitForSpecificEvent(port, QueueHd, CTAEVN_OPEN_SERVICES_DONE, &event );
	if ( ret != SUCCESS)
	{
		mmPrintPortErr(port,"Open services failed...event.value=0x%x\n", event.value);     
		return ret;
	}
	mmPrint(port, T_CTA, "CT Access services opened for Context %x\n", CTAccessHd);

	//////////////////////////////////
	//  If Timeslot mode, set up NOCC
	//////////////////////////////////
	if (Mode == TS_MODE)
	{
		if((ret = adiStartProtocol( *CTAccessHd, "nocc", NULL, NULL) != SUCCESS ))
		{
			mmPrintAll("\nFailed to start NOCC protocol for Context %x..Return code is 0x%x.\n", CTAccessHd, ret);
			return ret;
		}
		// Wait for protocol to be opened 
		ret = WaitForSpecificEvent(port, QueueHd, ADIEVN_STARTPROTOCOL_DONE, &event );
		if ( ret != SUCCESS)
		{
			mmPrintAll("\nFailed to start NOCC protocol for Context %x...event.value=0x%x\n", CTAccessHd, event.value);     
			return ret;
		}
		mmPrint(port, T_CTA, "NOCC Protocol Started for Context %x\n", CTAccessHd);
	}
	return ret;
}

DWORD AudRecFormatToADI ( tAudio_Format format )
{   switch( format ) {
        case N_G723:     return ADI_ENCODE_NATIVE_G_723_1;
        case N_AMR:      return ADI_ENCODE_NATIVE_AMRNB;
        case F_G711U:    return ADI_ENCODE_MULAW;
        case F_G711A:    return ADI_ENCODE_ALAW;
        case F_G723_5:   return ADI_ENCODE_G723_5;
        case F_G723_6:   return ADI_ENCODE_G723_6;
        case F_GSM_FR:   return ADI_ENCODE_GSM;
        case F_AMR_475:  return ADI_ENCODE_AMR_475;
        case F_AMR_515:  return ADI_ENCODE_AMR_515;
        case F_AMR_590:  return ADI_ENCODE_AMR_59;
        case F_AMR_670:  return ADI_ENCODE_AMR_67;
        case F_AMR_740:  return ADI_ENCODE_AMR_74;
        case F_AMR_795:  return ADI_ENCODE_AMR_795;
        case F_AMR_102:  return ADI_ENCODE_AMR_102;
        case F_AMR_122:  return ADI_ENCODE_AMR_122;
        default:         return ADI_ENCODE_G723_5; }
}


/////////////////////////////////////////////////////////////////////////////////
//  void SelectEncoders();
//  Function to initialize the following parameters based on Config File Entries
//      mmParm.audioModeX           mmParm.FusEncodeRate    mmParm.FusChanType
//      mmParm.ADIaudEncoder        mmParm.ADIvidEncoder    
/////////////////////////////////////////////////////////////////////////////////
void SelectEncoders()
{
    // Set the Audio Mode
    if( mmParm.Audio_RTP_Format == NO_AUDIO)
        mmParm.audioMode1 = NONE;
    else if( (mmParm.Audio_Record_Format1 == N_G723) ||
             (mmParm.Audio_Record_Format1 == N_AMR ) )
    {
        if(mmParm.native_silence_detect == TRUE)
            mmParm.audioMode1 = NATIVE_SD;
        else
            mmParm.audioMode1 = NATIVE_NO_SD;
    }
    else  // Fusion IVR
        mmParm.audioMode1 = FUS_IVR;
    
    // Set The ADI Encoder Type
    mmParm.ADIaudEncoder1 = AudRecFormatToADI( mmParm.Audio_Record_Format1 );
            
    // Set the ADI Video Encoder Type
	switch (mmParm.Video_Format) 
	{
		case MPEG4:
            mmParm.ADIvidEncoder = ADI_ENCODE_NATIVE_MPEG4;
            break;
		case H263:
            mmParm.ADIvidEncoder = ADI_ENCODE_NATIVE_H_263;
            break;
		case H263_PLUS:
            mmParm.ADIvidEncoder = ADI_ENCODE_NATIVE_H_263P;
            break;
        default:
            mmParm.ADIvidEncoder = ADI_ENCODE_NATIVE_H_263;
    }        
    // Set the Fusion Channel Type, and the Fusion Encoder Rate
	switch (mmParm.Audio_RTP_Format) 
	{
		case G723_5:
            mmParm.FusEncodeRate = 1;
			mmParm.FusChanType = G723DecodeSimplex;
            mmParm.FusFilterType = MSP_FILTER_G723_ENCODER;
            break;
		case G723_6:
            mmParm.FusEncodeRate = 0;
			mmParm.FusChanType = G723DecodeSimplex;
            mmParm.FusFilterType = MSP_FILTER_G723_ENCODER;
            break;
		case G711:
            mmParm.FusEncodeRate = 0;
			mmParm.FusChanType = G711DecodeSimplex;
            mmParm.FusFilterType = MSP_FILTER_G711_ENCODER;
            break;
		case AMR_475:
            mmParm.FusEncodeRate = 7;
			mmParm.FusChanType = AMRDecodeSimplex;
            mmParm.FusFilterType = MSP_FILTER_AMR_ENCODER;
            break;
		case AMR_515:
            mmParm.FusEncodeRate = 6;
			mmParm.FusChanType = AMRDecodeSimplex;
            mmParm.FusFilterType = MSP_FILTER_AMR_ENCODER;
            break;
		case AMR_590:
            mmParm.FusEncodeRate = 5;
			mmParm.FusChanType = AMRDecodeSimplex;
            mmParm.FusFilterType = MSP_FILTER_AMR_ENCODER;
            break;
		case AMR_670:
            mmParm.FusEncodeRate = 4;
			mmParm.FusChanType = AMRDecodeSimplex;
            mmParm.FusFilterType = MSP_FILTER_AMR_ENCODER;
            break;
		case AMR_740:
            mmParm.FusEncodeRate = 3;
			mmParm.FusChanType = AMRDecodeSimplex;
            mmParm.FusFilterType = MSP_FILTER_AMR_ENCODER;
            break;
		case AMR_795:
            mmParm.FusEncodeRate = 2;
			mmParm.FusChanType = AMRDecodeSimplex;
            mmParm.FusFilterType = MSP_FILTER_AMR_ENCODER;
            break;
		case AMR_102:
            mmParm.FusEncodeRate = 1;
			mmParm.FusChanType = AMRDecodeSimplex;
            mmParm.FusFilterType = MSP_FILTER_AMR_ENCODER;
            break;
		case AMR_122:
            mmParm.FusEncodeRate = 0;
			mmParm.FusChanType = AMRDecodeSimplex;
            mmParm.FusFilterType = MSP_FILTER_AMR_ENCODER;
            break;
		case NO_AUDIO:
		default:
            mmParm.FusEncodeRate = 0;
			mmParm.FusChanType = NOT_DEFINED;
            mmParm.FusFilterType = MSP_FILTER_G723_ENCODER;
	}
    
    return;
}


/////////////////////////////////////////////////////////////////////////////////
//  InitMsppConfiguration()
//  Function to initialize the values in each of the Context Data Structures
/////////////////////////////////////////////////////////////////////////////////
void InitMsppConfiguration()
{
	BYTE i;

	for (i=0; i<mmParm.nPorts; i++)
	{
        VideoCtx[i].pr_mode_p = PR_MODE_INIT;
        VideoCtx[i].pr_mode_r = PR_MODE_INIT;
        
        VideoCtx[i].playList.numFiles = 0;
        VideoCtx[i].playList.currFile = 0;        
        VideoCtx[i].bPlayList         = FALSE;
            
		if(mmParm.Video_Format != NO_VIDEO)
		{
			// Configure Native Video RTP Endpoint
            VideoCtx[i].rtpEp.hd                            = 0;
			VideoCtx[i].rtpEp.Addr.size 					= sizeof(VideoCtx[i].rtpEp.Addr);
			VideoCtx[i].rtpEp.Addr.nBoard 					= mmParm.Board;
			VideoCtx[i].rtpEp.Addr.eEpType 					= MSP_ENDPOINT_RTPFDX_VIDEO;
			strcpy(VideoCtx[i].rtpEp.Addr.EP.RtpRtcp.SrcIpAddress, mmParm.srcIPAddr_video);
			VideoCtx[i].rtpEp.Addr.EP.RtpRtcp.nSrcPort 		= (WORD)mmParm.srcPort_video + i*2;
			strcpy(VideoCtx[i].rtpEp.Addr.EP.RtpRtcp.DestIpAddress, mmParm.destIPAddr_video);
			VideoCtx[i].rtpEp.Addr.EP.RtpRtcp.nDestPort 	= (WORD)mmParm.destPort_video + i*2;
			VideoCtx[i].rtpEp.Param.size  					= sizeof(VideoCtx[i].rtpEp.Param);
			VideoCtx[i].rtpEp.Param.eParmType 				= MSP_ENDPOINT_RTPFDX_VIDEO;
			VideoCtx[i].rtpEp.Param.EP.RtpRtcp.TypeOfService= 0;
			VideoCtx[i].rtpEp.Param.EP.RtpRtcp.startRtcp  	= 0;
			VideoCtx[i].rtpEp.Param.EP.RtpRtcp.RtpTsFreq  	= 90000;
			VideoCtx[i].rtpEp.Param.EP.RtpRtcp.Session_bw 	= 64000;
			VideoCtx[i].rtpEp.Param.EP.RtpRtcp.frameQuota 	= mmParm.frameQuota;
			VideoCtx[i].rtpEp.Param.EP.RtpRtcp.linkEvents 	= 0;
			VideoCtx[i].rtpEp.Param.EP.RtpRtcp.PayloadMap.payload_id = 96;
			VideoCtx[i].rtpEp.Param.EP.RtpRtcp.PayloadMap.vocoder    = 100;
			if (mmParm.nPorts > 1) {
	            sprintf(VideoCtx[i].threegp_FileName_play, "load_%02d_%s", i, mmParm.threegpFileName_play);
                    sprintf(VideoCtx[i].threegp_FileName, "load_%02d_%s", i, mmParm.threegpFileName);
			} else {
	            strcpy(VideoCtx[i].threegp_FileName_play, mmParm.threegpFileName_play);            
                    strcpy(VideoCtx[i].threegp_FileName, mmParm.threegpFileName);
		    }   
		}

        AudioCtx[i].pr_mode_p = PR_MODE_INIT;
        AudioCtx[i].pr_mode_r = PR_MODE_INIT;
        if( mmParm.audioMode1 != NONE )		
            {
                MSP_VOICE_CHANNEL_PARMS *pVoiceParm;
				// Configure Native Audio RTP Endpoint
			
			// Configure the Audio RTP Endpoint and Channel according to the audio format
			switch( mmParm.Audio_Record_Format1 ) {
				case N_AMR:
					AudioCtx[i].rtpEp.Addr.eEpType    = MSP_ENDPOINT_RTPFDX;		
				    AudioCtx[i].rtpEp.Param.eParmType = MSP_ENDPOINT_RTPFDX; 
				    AudioCtx[i].rtpEp.Param.EP.RtpRtcp.PayloadMap.payload_id = MSP_CONST_VOCODER_AMR3267;
    				AudioCtx[i].rtpEp.Param.EP.RtpRtcp.PayloadMap.vocoder    = MSP_CONST_VOCODER_AMR3267;									
					break;
				case N_G723:
					AudioCtx[i].rtpEp.Addr.eEpType    = MSP_ENDPOINT_RTPFDX;		
				    AudioCtx[i].rtpEp.Param.eParmType = MSP_ENDPOINT_RTPFDX; 
			   		AudioCtx[i].rtpEp.Param.EP.RtpRtcp.PayloadMap.payload_id = 0xffffffff;
		    		AudioCtx[i].rtpEp.Param.EP.RtpRtcp.PayloadMap.vocoder    = 0xffffffff;
					break;
				default:
					mmPrintPortErr(i, "Invalid audio format!\n");			
					break;
			} // end switch
								
            AudioCtx[i].rtpEp.hd                            = 0;
			AudioCtx[i].rtpEp.Addr.size 					= sizeof(AudioCtx[i].rtpEp.Addr);
			AudioCtx[i].rtpEp.Addr.nBoard 					= mmParm.Board;
			strcpy(AudioCtx[i].rtpEp.Addr.EP.RtpRtcp.SrcIpAddress, mmParm.srcIPAddr_audio);
			AudioCtx[i].rtpEp.Addr.EP.RtpRtcp.nSrcPort 		= (WORD)mmParm.srcPort_audio + i*2;
			strcpy(AudioCtx[i].rtpEp.Addr.EP.RtpRtcp.DestIpAddress, mmParm.destIPAddr_audio);
			AudioCtx[i].rtpEp.Addr.EP.RtpRtcp.nDestPort 	= (WORD)mmParm.destPort_audio + i*2;
			AudioCtx[i].rtpEp.Param.size  					= sizeof(AudioCtx[i].rtpEp.Param);
			AudioCtx[i].rtpEp.Param.EP.RtpRtcp.TypeOfService= 0;
			AudioCtx[i].rtpEp.Param.EP.RtpRtcp.startRtcp  	= 0;
			AudioCtx[i].rtpEp.Param.EP.RtpRtcp.RtpTsFreq  	= 8000;
			AudioCtx[i].rtpEp.Param.EP.RtpRtcp.Session_bw 	= 64000;
			AudioCtx[i].rtpEp.Param.EP.RtpRtcp.frameQuota 	= mmParm.frameQuota;
			AudioCtx[i].rtpEp.Param.EP.RtpRtcp.linkEvents 	= 0;

			// Configure Fusion DS0 Endpoint
			AudioCtx[i].FusionDS0Ep.hd                        = 0;
            AudioCtx[i].FusionDS0Ep.Addr.size                 = sizeof(AudioCtx[i].FusionDS0Ep.Addr);
			AudioCtx[i].FusionDS0Ep.Addr.nBoard               = mmParm.Board;
			AudioCtx[i].FusionDS0Ep.Addr.eEpType              = MSP_ENDPOINT_DS0;
			AudioCtx[i].FusionDS0Ep.Addr.EP.DS0.nTimeslot     = i + mmParm.base_fusion_timeslot;
			AudioCtx[i].FusionDS0Ep.Param.size                = sizeof(AudioCtx[i].FusionDS0Ep.Param);
			AudioCtx[i].FusionDS0Ep.Param.eParmType           = MSP_ENDPOINT_DS0;
			AudioCtx[i].FusionDS0Ep.Param.EP.DS0.media        = MSP_VOICE;
			AudioCtx[i].FusionDS0Ep.Param.EP.DS0.size         = sizeof(AudioCtx[i].FusionDS0Ep.Param.EP.DS0);
			// Configure Fusion Audio Channel
			AudioCtx[i].FusionCh.hd       		              = 0;	
			AudioCtx[i].FusionCh.Addr.size                    = sizeof(MSP_CHANNEL_ADDR);
			AudioCtx[i].FusionCh.Addr.nBoard                  = mmParm.Board;
			AudioCtx[i].FusionCh.Addr.channelType             = (MSP_CHANNEL_TYPE)(mmParm.FusChanType); 
            AudioCtx[i].FusionCh.Addr.FilterAttribs           = 0;

			AudioCtx[i].FusionCh.Param.size = sizeof(MSP_CHANNEL_PARAMETER);
            AudioCtx[i].FusionCh.Param.channelType            = (MSP_CHANNEL_TYPE)(mmParm.FusChanType); 
    
            pVoiceParm = &AudioCtx[i].FusionCh.Param.ChannelParms.VoiceParms;
            pVoiceParm->size                                  = sizeof(MSP_VOICE_CHANNEL_PARMS);            
            pVoiceParm->JitterParms.size                      = sizeof(msp_FILTER_JITTER_PARMS);
            pVoiceParm->JitterParms.depth                     =  0xffffffff;
            pVoiceParm->JitterParms.adapt_enabled             =  0xffffffff;
            
			pVoiceParm->EncoderParms.size                     = sizeof(msp_FILTER_ENCODER_PARMS);
			pVoiceParm->EncoderParms.Mode                     = 0xffff;
			pVoiceParm->EncoderParms.Gain                     = 0xffff;
			pVoiceParm->EncoderParms.VadControl               = 0xffff;
			pVoiceParm->EncoderParms.NotchControl             = 0xffff;
			pVoiceParm->EncoderParms.IPFormat                 = H2NMS_WORD(0x0001); // RFC 3267 
            pVoiceParm->EncoderParms.Rate                     = (WORD)H2NMS_WORD(mmParm.FusEncodeRate);
			pVoiceParm->EncoderParms.PayloadID                = H2NMS_WORD(98); 
			pVoiceParm->EncoderParms.DtmfMode                 = 0xffff;
            
			pVoiceParm->DecoderParms.size                     = sizeof(msp_FILTER_DECODER_PARMS);
			pVoiceParm->DecoderParms.Mode                     = 0xffff;     
			pVoiceParm->DecoderParms.Gain                     = 0xffff;
			pVoiceParm->DecoderParms.IPFormat                 = 0xffff;
			pVoiceParm->DecoderParms.PayloadID                = 0xffff;
			pVoiceParm->DecoderParms.DtmfMode                 = 0xffff;
            

		}
	}
}

