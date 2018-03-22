/***********************************************************************
*  File - init.cpp
*
*  Description - Functions needed to initialize the VMSAMP - Video
*                Mail Sample application
*
*  2005  NMS Communications Corporation.
*
***********************************************************************/

///////////////////
// Include Files //
///////////////////
#include "mega_global.h"
#include "gw_types.h"  /* t_GwStartParms */
#include "srv_types.h" /* SRV_AUDIO_PORT, SRV_VIDEO_PORT */
#include "vm_trace.h"

#ifdef USE_TRC
#include "trcapi.h"    /* trcInitialize */
#endif

t_GwStartParms gwParm;
extern CTAHD   sysCTAccessHd;

static int  gw_get_keyword (char **bufp, argval *pval);
static void SetDefaultGwParms();
static void PrintGwConfiguration();
static int  ModifyGwParms( char *FileName );
static int  OpenCtaContext(CTAQUEUEHD QueueHd, CTAHD *CTAccessHd,
					 int timeslot, EP_TYPE ep_type, int port);

int initAudioBypassPath( int nGw );
int initCreateRtpEndpoint(int Port, GW_EP *pRtpEp);
int initCreateChannel(int Port, GW_CHAN *pChan);

/*****************************************************************************
*
* Function   : GetCmdLineParms
*
*============================================================================
*
* Description: Extracts and verifies the format of the commandline
*              parameters and assigns corresponding values to global variables.
* 
* Parmeters  : argc - Commandline argument counter
*              argv - Array of commandline options
*           
* Return     : None
*
* Notes      : None
*
*****************************************************************************/
int initGetCmdLineParms( int argc, char *argv[] ) //, CMDLINE_PARMS** pcmdline_parms
{
    int	 c;
    int  error = FALSE;

	// Set Default Values
	CmdLineParms.bInteractive = TRUE;
    CmdLineParms.nBoardNum = 0;
	CmdLineParms.GwConfigFileName[0]  = '\0';
	CmdLineParms.SrvConfigFileName[0] = '\0';
        
    while( ( c = getopt( argc, argv, "b:g:s:i:?hH" ) ) != -1 )
    {
        // mmPrintAll("c %d is %c\n", i++, (char)c);
        switch( c )
        {
        case '?':
        case 'h':
        case 'H':
            PrintUsage( argv );
            error = TRUE;
            break;
        case 'b':
            sscanf( optarg, "%d", &(CmdLineParms.nBoardNum ) );
            break;
        case 'g':
            strcpy( CmdLineParms.GwConfigFileName, optarg );
            break;          
        case 's':
            strcpy( CmdLineParms.SrvConfigFileName, optarg );
            break;          
        case 'i':
            sscanf( optarg, "%d", &VmTraceMask );
            break;          

        default:
            mmPrintAll( "Invalid option.  Use -? for help\n" );
            error = TRUE;
            break;
        }
    }
    
   
    if( !error )
        return SUCCESS;
    else
        return FAILURE;
    
}  /*** InitGetCmdLineParms ***/

/*****************************************************************************
* Function   : initGwMsppConfig
*============================================================================
* Description: Provides an initial set of parameters and addresses for a
*              gateway test setup. Defaults are stored in a 
*              MEDIA_GATEWAY_CONFIG structure called GwConfig[0]
* 
* Parmeters  : None     
* Return     : None
* Notes      : None
*
*****************************************************************************/
int initGwMsppConfig()
{
    int i;
    
	GwConfig[0].dNumGW = gwParm.nPorts;
	for( i=0; i<gwParm.nPorts; i++ ) {

		GwConfig[i].callNum       = 0;
        
		// Configure the MUX Endpoint
		GwConfig[i].MuxEp.bActive                       = TRUE;
   		GwConfig[i].MuxEp.bQueueEnabled                 = FALSE;
    	GwConfig[i].MuxEp.Addr.size                     = sizeof(GwConfig[i].MuxEp.Addr);
	    GwConfig[i].MuxEp.Addr.nBoard                   = CmdLineParms.nBoardNum;
    	GwConfig[i].MuxEp.Addr.eEpType                  = MSP_ENDPOINT_MUX;
	    GwConfig[i].MuxEp.Addr.EP.Mux.nTimeslot         = gwParm.mux_timeslot+i;
    	GwConfig[i].MuxEp.Param.size                    = sizeof(GwConfig[i].MuxEp.Param);
	    GwConfig[i].MuxEp.Param.eParmType               = MSP_ENDPOINT_MUX;
	    GwConfig[i].MuxEp.bReady                        = TRUE;
		
		// Video EPs/Channels parameters which are the same for all video formats		
	    GwConfig[i].VidRtpEp.bActive = TRUE;
	    GwConfig[i].VidRtpEp.bEnabled                   = TRUE;		
		GwConfig[i].VidRtpEp.Addr.size                  = sizeof(GwConfig[i].VidRtpEp.Addr);
	    GwConfig[i].VidRtpEp.Addr.nBoard                = CmdLineParms.nBoardNum;
	    strcpy(GwConfig[i].VidRtpEp.Addr.EP.RtpRtcp.DestIpAddress, gwParm.destIPAddr_video);
		strcpy(GwConfig[i].VidRtpEp.Addr.EP.RtpRtcp.SrcIpAddress,  gwParm.srcIPAddr_video);
	    GwConfig[i].VidRtpEp.Addr.EP.RtpRtcp.nDestPort  = gwParm.destPort_video + 2*(WORD)i;
    	GwConfig[i].VidRtpEp.Addr.EP.RtpRtcp.nSrcPort   = gwParm.srcPort_video + 2*(WORD)i;
	    GwConfig[i].VidRtpEp.Param.size                 = sizeof(GwConfig[i].VidRtpEp.Param);
	    GwConfig[i].VidRtpEp.Param.EP.RtpRtcp.TypeOfService = 0;
    	GwConfig[i].VidRtpEp.Param.EP.RtpRtcp.startRtcp = 0;
	    GwConfig[i].VidRtpEp.Param.EP.RtpRtcp.RtpTsFreq = 90000;
    	GwConfig[i].VidRtpEp.Param.EP.RtpRtcp.Session_bw= 64000;
  		GwConfig[i].VidRtpEp.Param.EP.RtpRtcp.frameQuota= 2;
	    GwConfig[i].VidRtpEp.Param.EP.RtpRtcp.linkEvents= 0;

	    GwConfig[i].VidChan.bActive = TRUE;
	    GwConfig[i].VidChan.bEnabled                    = FALSE;
		GwConfig[i].VidChan.Addr.size		            = sizeof(GwConfig[i].VidChan.Addr);
		GwConfig[i].VidChan.Addr.nBoard		            = CmdLineParms.nBoardNum;
	    // Currently, only NULL parameters are sent
		GwConfig[i].VidChan.Param.size		            = sizeof(GwConfig[i].VidChan.Param);
	    GwConfig[i].VidChan.Param.ChannelParms.VideoParms.JitterParms.adapt_enabled = H2NMS_WORD(0);
    	GwConfig[i].VidChan.Param.ChannelParms.VideoParms.JitterParms.depth         = H2NMS_WORD(2);
	    GwConfig[i].VidChan.Param.ChannelParms.VideoParms.JitterParms.size          =
    	    sizeof(GwConfig[i].VidChan.Param.ChannelParms.VideoParms.JitterParms);


		// Configure the Audio Bypass RTP Endpoint and Channel according to the audio format
		switch( gwParm.audioFormat ) {
			case N_AMR:
				// Endpoint 
				GwConfig[i].AudRtpEp.Addr.eEpType    = MSP_ENDPOINT_RTPFDX;		
			    GwConfig[i].AudRtpEp.Param.eParmType = MSP_ENDPOINT_RTPFDX; 
			    GwConfig[i].AudRtpEp.Param.EP.RtpRtcp.PayloadMap.payload_id = MSP_CONST_VOCODER_AMR3267;
    			GwConfig[i].AudRtpEp.Param.EP.RtpRtcp.PayloadMap.vocoder    = MSP_CONST_VOCODER_AMR3267;									
				
				// Channel
				GwConfig[i].AudByChan.Addr.channelType  = MGAMRBypassChannel;
				GwConfig[i].AudByChan.Param.channelType = MGAMRBypassChannel; 	
				break;
			case N_G723:
				// Endpoint 			
				GwConfig[i].AudRtpEp.Addr.eEpType    = MSP_ENDPOINT_RTPFDX;		
			    GwConfig[i].AudRtpEp.Param.eParmType = MSP_ENDPOINT_RTPFDX; 
			    GwConfig[i].AudRtpEp.Param.EP.RtpRtcp.PayloadMap.payload_id = 0xffffffff;
    			GwConfig[i].AudRtpEp.Param.EP.RtpRtcp.PayloadMap.vocoder    = 0xffffffff;
				
				// Channel
				GwConfig[i].AudByChan.Addr.channelType  = MGG723BypassChannel;
				GwConfig[i].AudByChan.Param.channelType = MGG723BypassChannel; 					
				break;
			default:
				mmPrintErr("Invalid audio format!\n");			
				break;
		}		
		
		// Audio EPs/Channels parameters which are the same for all audio formats
	    GwConfig[i].AudRtpEp.bActive                      = TRUE;
		GwConfig[i].AudRtpEp.Addr.size                    = sizeof(GwConfig[i].AudRtpEp.Addr);
	    GwConfig[i].AudRtpEp.Addr.nBoard                  = CmdLineParms.nBoardNum;	     
	    strcpy(GwConfig[i].AudRtpEp.Addr.EP.RtpRtcp.DestIpAddress, gwParm.destIPAddr_audio);
	    GwConfig[i].AudRtpEp.Addr.EP.RtpRtcp.nDestPort  = gwParm.destPort_audio + 2*(WORD)i;
		strcpy(GwConfig[i].AudRtpEp.Addr.EP.RtpRtcp.SrcIpAddress, gwParm.srcIPAddr_audio);
	    GwConfig[i].AudRtpEp.Addr.EP.RtpRtcp.nSrcPort     = gwParm.srcPort_audio + 2*(WORD)i;
	    GwConfig[i].AudRtpEp.Param.size                   = sizeof(GwConfig[i].AudRtpEp.Param);
	    GwConfig[i].AudRtpEp.Param.EP.RtpRtcp.RtpTsFreq   = 8000;
	    GwConfig[i].AudRtpEp.Param.EP.RtpRtcp.Session_bw  = 4750;
	    GwConfig[i].AudRtpEp.Param.EP.RtpRtcp.frameQuota  = 2;
	    GwConfig[i].AudRtpEp.Param.EP.RtpRtcp.linkEvents  = 0;

	    GwConfig[i].AudByChan.bActive               = TRUE;
	    GwConfig[i].AudByChan.bEnabled              = FALSE;
		GwConfig[i].AudByChan.Addr.size		        = sizeof(GwConfig[i].AudByChan.Addr);
		GwConfig[i].AudByChan.Addr.nBoard	        = CmdLineParms.nBoardNum;
		GwConfig[i].AudByChan.Param.size		    = sizeof(GwConfig[i].AudByChan.Param);
    	GwConfig[i].AudByChan.Param.ChannelParms.VoiceParms.size                         = 
        sizeof(GwConfig[i].AudByChan.Param.ChannelParms.VoiceParms);
	    GwConfig[i].AudByChan.Param.ChannelParms.VoiceParms.JitterParms.size             =
        	sizeof(GwConfig[i].AudByChan.Param.ChannelParms.VoiceParms.JitterParms);
    	GwConfig[i].AudByChan.Param.ChannelParms.VoiceParms.JitterParms.depth            = H2NMS_WORD(2);
	    GwConfig[i].AudByChan.Param.ChannelParms.VoiceParms.JitterParms.adapt_enabled    = H2NMS_WORD(0);

    	// Configure the call control state 
	    GwConfig[i].CallMode                            = 1;
    
    	GwConfig[i].isdnCall.dCXIndex                   = i;
	    GwConfig[i].isdnCall.callhd                     = NULL_NCC_CALLHD;
	   	GwConfig[i].isdnCall.dCallState                 = IDL;
	    GwConfig[i].isdnCall.dNewState                  = IDL;
	    GwConfig[i].isdnCall.sCallStat.calledaddr[i]    = '\0';
	    sprintf(GwConfig[i].isdnCall.szDescriptor, "Call_%02d", GwConfig[i].isdnCall.dCXIndex);

	    // Configure H323 Library
	    GwConfig[i].h323Lib.bActive                     = FALSE;

        GwConfig[i].txVideoCodec = H324_MPEG4_VIDEO;
        GwConfig[i].bGwVtpServerConfig = FALSE;

#ifdef USE_TRC                        		
        // Configure the Video Transcoder
        GwConfig[i].VideoXC.bActive  = TRUE;
        
        GwConfig[i].VideoXC.bFVU_Enabled                = TRUE;
        sprintf( GwConfig[i].VideoXC.ipAddr,            gwParm.destIPAddr_video );
        GwConfig[i].VideoXC.xcChanId = -1;
//        GwConfig[i].VideoXC.state    = XCSTATE_NONE;
        
        tChannelConfig *pChConfig = &GwConfig[i].VideoXC.xcChanConfig;           
        memset( pChConfig, 0, sizeof (tChannelConfig)); 
               
        pChConfig->params1.VidType  = 2;  // H263
        pChConfig->params2.VidType  = 0;  // MPEG4
        pChConfig->params1.RxPort   = 0;  // Will be defined after trcCreateVideoChannel
        pChConfig->params1.DestPort = GwConfig[i].VidRtpEp.Addr.EP.RtpRtcp.nSrcPort; 
        strcpy(pChConfig->params1.DestIP, GwConfig[i].VidRtpEp.Addr.EP.RtpRtcp.SrcIpAddress);
        pChConfig->params1.outDataRate       = 43; // kbps
        pChConfig->params1.outFrameRate      = 9;  // frame/sec
        pChConfig->params1.outPayloadID      = 100;
        pChConfig->params1.jitterCfg.mode    = 3;                                                                                                        
        pChConfig->params1.jitterCfg.latency = 0;         
#endif // USE_TRC
        
        GwConfig[i].G711Chan.bActive     = FALSE;
		GwConfig[i].shutdown_in_progress = FALSE;
        GwConfig[i].video_path_initialized = FALSE;
        GwConfig[i].audio_path_initialized = FALSE;        
                
	}
	
    return SUCCESS;

}  /*** initDefaultConfig ***/


#ifdef USE_TRC
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
#endif // USE_TRC

/*****************************************************************************
* Function   : initGateway
*============================================================================
* Description: Start the gateway by creating the Que, creating the CTA Context
* initialize the cta services, and then creating the endpoints and channels.
* 
* Parmeters  : None        
* Return     : None
* Notes      : None
*
*****************************************************************************/
int initGateway()
{
    DWORD   i, ret;
    WORD    startCP = 0;        // NCC Parm
    DWORD   mediamask = 0;      // NCC Parm
    
    CTA_INIT_PARMS		initparms = { 0 };    
    CTA_EVENT           event;
    CTA_SERVICE_NAME Services[] =    /* for ctaInitialize */
    {
		{ "ADI", "ADIMGR" },
        { "NCC", "ADIMGR" },
        { "MSP", "MSPMGR" },
        { "SWI", "SWIMGR" },
        { "ISDN", "ADIMGR" }
    };

   ret = h324Initialize(gwParm.sH324LogFile);
   if ( ret != SUCCESS)
   {
      mmPrintErr("h324Initialize . Return code is %x\n", ret);
      exit(1);
   }   

#ifdef USE_TRC    
    if( gwParm.useVideoXc ) {
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
    }
#endif
           
    initGwMsppConfig();

	/*** Initialize size of init parms structure ***/
    initparms.size = sizeof( CTA_INIT_PARMS );
    
    /*Check if daemon is running by calling ctaInitialize with
    * parmflags set to use system global default parameters and
    * traceflags set to enable tracing. If an error is returned
    * try again without system parameters and tracing.*/
    
    initparms.traceflags        = CTA_TRACE_ENABLE;
    initparms.filename          = NULL;
    initparms.ctacompatlevel    = CTA_COMPATLEVEL;

	ctaSetErrorHandler( NULL, NULL );
    //////////////////////////////
    //////// ctaInitialize ///////
    //////////////////////////////
    if (( ret = ctaInitialize( Services, NUM_CTA_SERVICES, &initparms )) != SUCCESS )
    {
	   /*
        * Clear parameter flags and use process global default
        * parameters
        */
        initparms.parmflags = 0;
                
        /*** Clear trace flags ***/
        initparms.traceflags = 0;
        
        if (( ret = ctaInitialize(Services,
            sizeof(Services)/sizeof(Services[0]),
            &initparms )) != SUCCESS )
        {
            mmPrintAll("\nFailed to Initialize CT Access, return code is %x",ret);
            exit(1);
        }
    }
    ctaSetErrorHandler( ErrorHandler, NULL );

   
    // Set Some NCC Parameters
    if( SUCCESS != (ret=ctaSetParmByName (  NULL_CTAHD, 
                                            "NCC.X.ADI_ISDN.START_EXT.startCP",
                                            &startCP,
                                            sizeof( startCP ) ) ) )
    {
        mmPrintAll("\nctaSetParmByName returned %x for NCC.X.ADI_ISDN.START_EXT.startCP", ret);
    }
    if( SUCCESS != (ret=ctaSetParmByName (  NULL_CTAHD, 
                                            "NCC.X.ADI_START.mediamask",
                                            &mediamask,
                                            sizeof( mediamask ) ) ) )
    {
        mmPrintAll("\nctaSetParmByName returned %x for NCC.X.ADI_START.mediamask", ret);
    }

    {
        unsigned char iebuf[50]={
            0x04,                   // BC ID
            0x02,                   // BC length
            0x88, 0x90,             // BC data
            
            0x7C,                   // LLC ID
            0x03,                   // LLC length
            0x88, 0x90, 0xA6,       // LLC data
            0x00                    // Ending 0
        };
        if( SUCCESS != (ret=ctaSetParmByName(  NULL_CTAHD,
                                               "NCC.X.ADI_ISDN.PLACECALL_EXT.ie_list",
                                               iebuf,
                                               sizeof( iebuf ) ) ) )
        {
            mmPrintAll("\nctaSetParmByName returned 0x%x for NCC.X.ADI_ISDN.PLACECALL_EXT.ie_list", ret);
        }
    }
      
    // If call mode is ISDN, initialize the ISDN stacks
    if( (GwConfig[0].CallMode == CALL_MODE_ISDN) && (GwConfig[0].MuxEp.bActive) )
    {
        ret = initAllIsdn();
    }

	/* Create CTA Contexts and EndPoints and Channels and Connections... */
	for (i=0; i<gwParm.nPorts ; i++)
	{
	    if (( ret = ctaCreateQueue(NULL, 0, &(GwConfig[i].hCtaQueueHd)) ) != SUCCESS ) {
        	mmPrintErr("Failed to create CT Access queue, return code is %x", ret);
	        exit( 1 );
    	}      
	    mmPrint(i, T_CTA, "CT Access queue for events created\n");
	
	
        mmPrint(i, T_GWAPP, "Creating Gateway %d\n",i);

		// Create the MUX DS0 Endpoint
        if(GwConfig[i].MuxEp.bActive)
        {
            mmPrint( i, T_MSPP, "Creating the MUX context and DS0 endpoint.\n");
		    OpenCtaContext( GwConfig[i].hCtaQueueHd, &GwConfig[i].MuxEp.ctahd, 
                      GwConfig[i].MuxEp.Addr.EP.Mux.nTimeslot, MUX, i);
            ret = mspCreateEndpoint(GwConfig[i].MuxEp.ctahd,
                                    &GwConfig[i].MuxEp.Addr,
                                    &GwConfig[i].MuxEp.Param,
                                    &GwConfig[i].MuxEp.hd);
	        if (ret != SUCCESS)
	        {
		        mmPrintPortErr(i, "mspCreateEndpoint returned %d.\n", ret);
		        return FAILURE;
	        }
            ret = WaitForSpecificEvent ( i, GwConfig[i].hCtaQueueHd, 
						MSPEVN_CREATE_ENDPOINT_DONE, &event);
	        if (ret != 0) {
		        mmPrintPortErr(i, "MSPEVN_CREATE_ENDPOINT_DONE event failure, ret= %x\n",ret);
		        return FAILURE;
	        }

            // If call mode is ISDN, start NCC
            if( GwConfig[i].CallMode == CALL_MODE_ISDN )
            {
                NCC_START_PARMS startparms = { 0 };
                char    errortext[120];

                ctaGetParms( GwConfig[i].MuxEp.ctahd, NCC_START_PARMID,
                             &startparms, sizeof startparms );

                startparms.eventmask = 0xffff; // low-level events 

                GwConfig[i].isdnCall.ctahd = GwConfig[i].MuxEp.ctahd;

                if( (ret = nccStartProtocol( GwConfig[i].MuxEp.ctahd, "isd0",
                                        &startparms, NULL, NULL )) != SUCCESS)
		        {
			        ctaGetText( GwConfig[i].MuxEp.ctahd, ret, errortext, 40 );
			        printf("nccStartProtocol failure: %s\n",errortext);
			        return FAILURE;
		        }
            }
        }
		
		// Init audio path
		ret = initAudioBypassPath( i );
		if( ret != SUCCESS )
			return ret;	
				
    }  //End For each port

    return SUCCESS;
} //End initGateway()

int OpenCtaContext(CTAQUEUEHD QueueHd, 
             CTAHD *CTAccessHd, 
             int timeslot, 
             EP_TYPE ep_type,
			 int port)
{

	int ret, ServiceCount;
	CTA_SERVICE_DESC ServDesc[NUM_CTA_SERVICES]; /* for ctaOpenServices */
	CTA_EVENT event;

    /////////////////////////////////
    //////// ctaCreateContext ///////
    /////////////////////////////////
	if ( (ret = ctaCreateContext( QueueHd, 0, "Media GW App", CTAccessHd )) != SUCCESS )
	{
		/*** This routine should not recieve an invalid queue handle ***/
		mmPrintPortErr(port, "Failed to create CT Access context, invalid queue handle %x.\n",
            QueueHd);
		exit( 1 );
	}
	mmPrint(port, T_CTA, "CT Access context created, CtaHd = 0x%x timeslot = %d\n",
        *CTAccessHd, timeslot );

    /*** Open the services on the contexts ***/

    //Must open the ADI service for each DS0 endpoint that will be created

    ServiceCount = 0;

	if( (ep_type == MUX) || (ep_type == DS0) )
	{
		// ADI service
		ServDesc[ServiceCount].name.svcname      = "ADI";
		ServDesc[ServiceCount].name.svcmgrname   = "ADIMGR";
		ServDesc[ServiceCount].mvipaddr.board    = CmdLineParms.nBoardNum;
		ServDesc[ServiceCount].mvipaddr.stream   = 0;
		ServDesc[ServiceCount].mvipaddr.timeslot = timeslot;
		ServDesc[ServiceCount].mvipaddr.mode     = ADI_VOICE_DUPLEX;
		ServiceCount++;
	}
	if( ep_type == MUX )
	{
		// NCC service
		ServDesc[ServiceCount].name.svcname      = "NCC";
		ServDesc[ServiceCount].name.svcmgrname   = "ADIMGR";
		ServDesc[ServiceCount].mvipaddr.board    = CmdLineParms.nBoardNum;
		ServDesc[ServiceCount].mvipaddr.stream   = 0;
		ServDesc[ServiceCount].mvipaddr.timeslot = timeslot;
		ServDesc[ServiceCount].mvipaddr.mode     = ADI_VOICE_DUPLEX;
		ServiceCount++;
	}

    // MSPP service 
    ServDesc[ServiceCount].name.svcname      = "MSP";
    ServDesc[ServiceCount].name.svcmgrname   = "MSPMGR";
    ServDesc[ServiceCount].mvipaddr.board    = 0;
    ServiceCount++;

    ////////////////////////////////
    //////// ctaOpenServices ///////
    ////////////////////////////////
	if((ret = ctaOpenServices( *CTAccessHd, ServDesc, ServiceCount)
        != SUCCESS ))
	{
		mmPrintPortErr(port, "Failed to open services for timeslot %d..Return code is 0x%x.\n",
            timeslot, ret);
		exit( 1 );
	}

	/*** Wait for services to be opened asynchronously ***/
	ret = WaitForSpecificEvent( port, QueueHd, CTAEVN_OPEN_SERVICES_DONE, &event );
	if ( ret != 0 )
	{
		mmPrintPortErr(port, "Open services failed for Context %d...event.value=0x%x\n",
            timeslot, event.value);     
		exit( 1 );
	}
	mmPrint(port, T_CTA, "CT Access services opened for Context 0x%x\n", *CTAccessHd);

	return ret;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Synchronus wrappers for MSPP functions
//////////////////////////////////////////////////////////////////////////////////////////////

int initCreateEpAndWait(int Port,
						CTAHD ctahd,
                        MSP_ENDPOINT_ADDR* addr,
                        MSP_ENDPOINT_PARAMETER* parm,
                        MSPHD* ephd)
{
    CTA_EVENT event;
    DWORD ret = mspCreateEndpoint(ctahd, addr, parm, ephd);
	if (ret != SUCCESS)
	{
		printf ("mspCreateEndpoint returned 0x%x.\n", ret);
		return FAILURE;
	}
    mmPrint( Port, T_MSPP, "mspCreateEndpoint returned handle 0x%x\n", *ephd);
    ret = WaitForSpecificEvent (Port, GwConfig[Port].hCtaQueueHd, 
				MSPEVN_CREATE_ENDPOINT_DONE, &event);
	if (ret != 0) {
		mmPrintPortErr(Port, "MSPEVN_CREATE_ENDPOINT_DONE event failure, ret= 0x%x\n", ret);
		return FAILURE;
	}
	return SUCCESS;
}

int initCreateChannelAndWait(int Port,
							 CTAHD ctahd,
							 MSP_CHANNEL_ADDR* addr,
							 MSP_CHANNEL_PARAMETER* parm,
							 MSPHD* chnhd)
{
	CTA_EVENT event;
    DWORD ret = mspCreateChannel( ctahd, addr, parm, chnhd);
	if (ret != SUCCESS)
	{
		mmPrintPortErr( Port, "mspCreateChannel returned 0x%x.\n", ret);
		return FAILURE;
	}

    mmPrint( Port, T_MSPP, "mspCreateChannel returned handle 0x0%x\n", *chnhd);
	ret = WaitForSpecificEvent (Port, GwConfig[Port].hCtaQueueHd, 
				MSPEVN_CREATE_CHANNEL_DONE, &event);
	if (ret != 0) {
		mmPrintPortErr(Port, "MSPEVN_CREATE_CHANNEL_DONE event failure, ret= 0x%x\n", ret);
		return FAILURE;
	}
	return SUCCESS;
}

int initConnectAndWait(int Port,
					   MSPHD ephd1, 
					   MSPHD chnhd, 
					   MSPHD ephd2)

{
	CTA_EVENT event;
    DWORD ret = mspConnect(ephd1, chnhd, ephd2);
	if (ret != SUCCESS)
	{
		printf ("mspConnect returned 0x%x.\n", ret);
		return FAILURE;
	}
	ret = WaitForSpecificEvent (Port, GwConfig[Port].hCtaQueueHd, MSPEVN_CONNECT_DONE, &event);
	if (ret != 0)
	{
		mmPrintPortErr(Port, "MSPEVN_CONNECT_DONE event failure, ret= %x\n", ret);
		return FAILURE;
	}
	return SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Functions to create various MSPP endpoints and Channels
//////////////////////////////////////////////////////////////////////////////////////////////

int initCreateRtpEndpoint(int Port, GW_EP *pRtpEp)
{
	OpenCtaContext( GwConfig[Port].hCtaQueueHd,
		            &pRtpEp->ctahd,
                    -1, /* no timeslot used for RTP */
					RTP, Port);
    return initCreateEpAndWait( Port, pRtpEp->ctahd, &pRtpEp->Addr,
                            &pRtpEp->Param, &pRtpEp->hd );
}

int initCreateChannel(int Port, GW_CHAN *pChan)
{
	OpenCtaContext( GwConfig[Port].hCtaQueueHd, &pChan->ctahd,
					-1, /* no timeslot used for channels */
					NA, Port);
	return initCreateChannelAndWait(Port, pChan->ctahd,
					        		      &pChan->Addr,
										  NULL,//&pChan->Param,
										  &pChan->hd);
}

//////////////////////////////////////////////////////////////////////////////////////////////

int initGatewayConfigFile(char *ConfigFile)
{
	SetDefaultGwParms();

	if( ConfigFile[0] == '\0' ) {
		printf("The Gateway Config File wasn't specified. Will use default values.\n");
	} else {
		printf("Gateway Config file specified: %s\n", ConfigFile);	
		ModifyGwParms( ConfigFile );
	}

	// Print updated gateway configuration
	PrintGwConfiguration();
    
	return 1;	
}


/* ***********************************************************************
   Function: SetDefaultGwParms
   Description: Set default Gateway's parameter values
   Input: None
   Output: None
************************************************************************ */
void SetDefaultGwParms()
{
	strcpy(gwParm.srcIPAddr_video,  CG_BOARD_IP);
	gwParm.srcPort_video              = GW_VIDEO_PORT;
	strcpy(gwParm.destIPAddr_video, CG_BOARD_IP);
	gwParm.destPort_video             = SRV_VIDEO_PORT;
	strcpy(gwParm.srcIPAddr_audio,  CG_BOARD_IP);
	gwParm.srcPort_audio              = GW_AUDIO_PORT;
	strcpy(gwParm.destIPAddr_audio, CG_BOARD_IP);
	gwParm.destPort_audio             = SRV_AUDIO_PORT;

	gwParm.videoFormat	      = MPEG4;
	gwParm.videoAlternateFormat = NO_VIDEO;
	gwParm.audioFormat	      = N_AMR;	
	
	gwParm.gsmamr_mode        = gsmamr_MR122;
	gwParm.gsmamr_maxBitRate  = 12200;
	gwParm.h263_maxBitRate    = 43000; //32000; //46000;
    	
	// ISDN
	gwParm.isdn_country       = COUNTRY_JPN;       // 81	
	gwParm.isdn_operator      = ISDN_OPERATOR_NTT; // 9	
	gwParm.isdn_dNT_TE        = EQUIPMENT_NT;	
	
	// H245
	gwParm.sendUniDirAudioOLC = 1;	
	gwParm.sendVideoOLC	      = 1;
	gwParm.rejectVideoOLC	      = 0;		
	gwParm.specifyVOVOL	      = 0;

	gwParm.ackUniDirAudioOLC  = 1;
	
	// Load test related
	gwParm.nPorts             = 1;		
	gwParm.auto_place_call    = 0;	
	gwParm.auto_hangup        = 0;		
	gwParm.mux_timeslot       = 0;	
	gwParm.nCalls             = -1;	// Run forever
	gwParm.between_calls_time = 3000;  // in milliseconds
	strcpy( gwParm.dial_number, "2000" );
    
    // TRC
    gwParm.useVideoXc         = 1; // Enable
    gwParm.trcTraceMask       = 0x20;    
    gwParm.trcFileTraceMask   = 0x20;       
    strcpy( gwParm.sTrcConfigFile, "trc.cfg" );
    strcpy( gwParm.sTrcLogFile, "" );
    
   
   
    strcpy( gwParm.sIfcType, "E1" );    
   
    // H324
    strcpy( gwParm.sH324LogFile, "h324.log" );
}


/* ***********************************************************************
   Function: ModifyGwParms
   Description: Modify parameter values according to the config file
   Input: None
   Output: returns FAILURE if can not open config file, SUCCESS otherwise
************************************************************************ */
int ModifyGwParms( char *FileName )
{
	FILE* pConfigFile;
    char buf [256];
    char *p = buf;
	argval value;
	int keyword;

	if ((pConfigFile = fopen(FileName, "r")) == NULL)
	{
		printf("\nCan not open config file %s. Will use default parameters...\n\n", FileName);
		return FAILURE;
	}

	printf("Reading parameters from the configuration file:\n");
	for(;;)
	{	 /* Read a line from config file; break on EOF or error. */
        if (fgets (buf, sizeof buf, pConfigFile) == NULL)
            break;

        /* Strip comments, quotes, and newline. */
        StripComments ("#;", (char *)buf);

		/* First token is keyword; some may be followed by a value or range */
		p = buf;
        keyword = gw_get_keyword (&p, &value);

		switch(keyword)
		{
		case -1:
			continue;
		case 0:
			continue;
		case SRC_IPADDR_VIDEO:
			if( getargs(p,&value, STRINGVALUE) != SUCCESS )
                continue;
			strcpy(gwParm.srcIPAddr_video, value.strval);
			printf("Source IP Address for Video = %s\n", gwParm.srcIPAddr_video);
			break;
		case SRC_PORT_VIDEO:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.srcPort_video = value.numval;
			printf("Source Port for Video = %d\n", gwParm.srcPort_video);
			break;
		case DEST_IPADDR_VIDEO:
			if( getargs(p,&value, STRINGVALUE) != SUCCESS )
                continue;
			strcpy(gwParm.destIPAddr_video, value.strval);
			printf("Destination IP Address for Video = %s\n", gwParm.destIPAddr_video);
			break;
		case DEST_PORT_VIDEO:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.destPort_video = value.numval;
			printf("Destination Port for Video = %d\n", gwParm.destPort_video);
			break;
		case SRC_IPADDR_AUDIO:
			if( getargs(p,&value, STRINGVALUE) != SUCCESS )
                continue;
			strcpy(gwParm.srcIPAddr_audio, value.strval);
			printf("Source IP Address for Audio = %s\n", gwParm.srcIPAddr_audio);
			break;
		case SRC_PORT_AUDIO:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.srcPort_audio = value.numval;
			printf("Source Port for Audio = %d\n", gwParm.srcPort_audio);
			break;
		case DEST_IPADDR_AUDIO:
			if( getargs(p,&value, STRINGVALUE) != SUCCESS )
                continue;
			strcpy(gwParm.destIPAddr_audio, value.strval);
			printf("Destination IP Address for Audio = %s\n", gwParm.destIPAddr_audio);
			break;
		case DEST_PORT_AUDIO:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.destPort_audio = value.numval;
			printf("Destination Port for Audio = %d\n", gwParm.destPort_audio);
			break;
		case GW_VIDEO_FORMAT:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.videoFormat = (tVideo_Format)value.numval;
			printf("Video Format = %d\n", gwParm.videoFormat);
			break;		
		case GW_VIDEO_ALTERNATE_FORMAT:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.videoAlternateFormat = (tVideo_Format)value.numval;
			printf("Video AlternateFormat = %d\n", gwParm.videoAlternateFormat);
			break;	
		case GW_AUDIO_FORMAT:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.audioFormat = (tAudio_Format)value.numval;
			printf("Audio Format = %d\n", gwParm.audioFormat);
			break;					
		case GSMAMR_MAXBITRATE:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.gsmamr_maxBitRate = value.numval;
			printf("AMR maxBitRate = %d\n", gwParm.gsmamr_maxBitRate);
			break;
		case GSAMR_MODE:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.gsmamr_mode = value.numval;
			printf("AMR mode = %d\n", gwParm.gsmamr_mode);
			break;
		case GW_ISDN_COUNTRY:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.isdn_country = value.numval;
			printf("ISDN Country = %d\n", gwParm.isdn_country);
			break;
		case GW_ISDN_OPERATOR:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.isdn_operator = value.numval;
			printf("ISDN Operator = %d\n", gwParm.isdn_operator);
			break;
		case GW_SENDUNIDIRAUDOLC:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.sendUniDirAudioOLC = value.numval;
			printf("Send Unidir. Audio OLC = %d\n", gwParm.sendUniDirAudioOLC);
			break;
		case GW_SEND_VIDEO_OLC:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.sendVideoOLC = value.numval;
			printf("Send Video OLC = %d\n", gwParm.sendVideoOLC);
			break;			
		case GW_REJECT_VIDEO_OLC:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.rejectVideoOLC = value.numval;
			printf("Reject Video OLC = %d\n", gwParm.rejectVideoOLC);
			break;						
		case GW_SPECIFY_VO_VOL:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.specifyVOVOL = value.numval;
			printf("Specify VOVOL header = %d\n", gwParm.specifyVOVOL);
			break;											
		case GW_ACKUNIDIRAUDOLC:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.ackUniDirAudioOLC = value.numval;
			printf("ACK Unidir. Audio OLC = %d\n", gwParm.ackUniDirAudioOLC);
			break;			
		case GW_MUX_TIMESLOT:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.mux_timeslot = value.numval;
			printf("MUX Timeslot = %d\n", gwParm.mux_timeslot);
			break;
		case GW_ISDN_NT_TE:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.isdn_dNT_TE = value.numval;
			printf("NT_TE = %d\n", gwParm.isdn_dNT_TE);
			break;
		case GW_AUTO_PLACE_CALL:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.auto_place_call = value.numval;
			printf("AutoPlaceCall = %d\n", gwParm.auto_place_call);
			break;
		case GW_AUTO_HANG_UP:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.auto_hangup = value.numval;
			printf("AutoHangup = %d\n", gwParm.auto_hangup);
			break;
		case GW_BETWEEN_CALLS_TIME:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.between_calls_time = value.numval;
			printf("BetweenCallsTime = %d\n", gwParm.between_calls_time);
			break;
		case GW_DIAL_NUMBER:
			if( getargs(p,&value, STRINGVALUE) != SUCCESS )
                continue;
			strcpy(gwParm.dial_number, value.strval);
			printf("DialNumber = %s\n", gwParm.dial_number);
			break;
		case GW_NUM_OF_PORTS:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.nPorts = value.numval;
			printf("nPorts = %d\n", gwParm.nPorts);
			break;
		case GW_NUM_OF_CALLS:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.nCalls = value.numval;
			printf("nCalls = %d\n", gwParm.nCalls);
			break;
		case GW_H263_MAXBITRATE:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.h263_maxBitRate = value.numval;
			printf("H263 maxBitRate = %d\n", gwParm.h263_maxBitRate);
			break;
		case GW_USE_VIDEO_XC:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			gwParm.useVideoXc = value.numval;
			printf("Use Video Transcoding = %d\n", gwParm.useVideoXc);
			break;
        case TRC_CONFIG_FILE:
			if( getargs(p,&value, STRINGVALUE) != SUCCESS )
                continue;
			strcpy(gwParm.sTrcConfigFile, value.strval);
			printf("TRC config file = %s\n", gwParm.sTrcConfigFile);
            break;                  
        case TRC_LOG_FILE:
			if( getargs(p,&value, STRINGVALUE) != SUCCESS )
                continue;
			strcpy(gwParm.sTrcLogFile, value.strval);
			printf("TRC log file = %s\n", gwParm.sTrcLogFile);
            break;                          
        case GW_TRC_TRACE_MASK:
			if( getargs(p,&value, HEXVALUE) != SUCCESS )
                continue;
			gwParm.trcTraceMask = value.numval;
			printf("trcTraceMask = 0x%x\n", gwParm.trcTraceMask);            
            break;
        case GW_TRC_FILE_TRACE_MASK:
			if( getargs(p,&value, HEXVALUE) != SUCCESS )
                continue;
			gwParm.trcFileTraceMask = value.numval;
			printf("trcFileTraceMask = 0x%x\n", gwParm.trcFileTraceMask);            
            break;            
        case GW_IFC_TYPE:
			if( getargs(p,&value, STRINGVALUE) != SUCCESS )
                continue;
			strcpy(gwParm.sIfcType, value.strval);
			printf("Interface type = %s\n", gwParm.sIfcType);            
            break;
        case GW_H324_LOG_FILE:
			if( getargs(p,&value, STRINGVALUE) != SUCCESS )
                continue;
			strcpy(gwParm.sH324LogFile, value.strval);
			printf("H324 Log File file = %s\n", gwParm.sH324LogFile);
            break;    
						
		default:
			continue;
		}
	}

	fclose(pConfigFile);

	return SUCCESS;
}


/*---------------------------------------------------------------------------
 * get_keyword - Finds the first alpha-numeric token and looks it up in
 *   a table.  Verifies equals or colon separator after the keyword.  Some
 *   keywords may have an optional value or range before the separator.
 *
 *--------------------------------------------------------------------------*/
static int gw_get_keyword (char **bufp, argval *pval)
{

    static struct {
        char *keytxt;
        int   keyid;
    } table [] = {
		{"srcIPAddr_video",				SRC_IPADDR_VIDEO        },
		{"srcPort_video",				SRC_PORT_VIDEO          },
		{"destIPAddr_video",			DEST_IPADDR_VIDEO       },
		{"destPort_video",				DEST_PORT_VIDEO         },
		{"srcIPAddr_audio",				SRC_IPADDR_AUDIO        },
		{"srcPort_audio",				SRC_PORT_AUDIO          },
		{"destIPAddr_audio",			DEST_IPADDR_AUDIO       },
		{"destPort_audio",				DEST_PORT_AUDIO         },
		{"Video_Format",				GW_VIDEO_FORMAT         },		
        {"Video_AlternateFormat",	   	GW_VIDEO_ALTERNATE_FORMAT },		
		{"Audio_Format",				GW_AUDIO_FORMAT         },		
		{"gsmamr_maxBitRate",			GSMAMR_MAXBITRATE       },
		{"gsmamr_mode",					GSAMR_MODE              },			
		{"isdn_country",				GW_ISDN_COUNTRY         },				
		{"isdn_operator",				GW_ISDN_OPERATOR        },				
		{"isdn_dNT_TE", 				GW_ISDN_NT_TE           },						
		{"sendUniDirAudioOLC",			GW_SENDUNIDIRAUDOLC     },
		{"sendVideoOLC",    			GW_SEND_VIDEO_OLC       },
		{"rejectVideoOLC",    			GW_REJECT_VIDEO_OLC     },
		{"ackUniDirAudioOLC",			GW_ACKUNIDIRAUDOLC      },				
		{"specifyVoVol",			    GW_SPECIFY_VO_VOL       },				
		{"mux_timeslot",			    GW_MUX_TIMESLOT         },																								
		{"AutoPlaceCall",			    GW_AUTO_PLACE_CALL      },	
		{"AutoHangup",			        GW_AUTO_HANG_UP         },
		{"HangupOnUUI",			        GW_HANG_UP_ON_UUI       },		
		{"BetweenCallsTime",	        GW_BETWEEN_CALLS_TIME   },
		{"DialNumber",			        GW_DIAL_NUMBER          },
		{"nPorts",  			        GW_NUM_OF_PORTS         },
		{"nCalls",  			        GW_NUM_OF_CALLS         },
		{"h263_maxBitRate",				GW_H263_MAXBITRATE      },
		{"useVideoXc",		    		GW_USE_VIDEO_XC         },
		{"trcConfigFile",		        TRC_CONFIG_FILE         },    
		{"trcLogFile",		        TRC_LOG_FILE         },    		            
		{"trcTraceMask",		        GW_TRC_TRACE_MASK       },        
		{"trcFileTraceMask",	        GW_TRC_FILE_TRACE_MASK  },        
		{"interfaceType",		        GW_IFC_TYPE             },
		{"h324LogFile",		            GW_H324_LOG_FILE        },       
   
		         
    };
    int i;
    char *p = *bufp;
    char *ptoken;
    int size = 0;
    int keyid = -1;
    char keywordtxt[80];

    /* Skip leading spaces; done if eol or if rest is comment */
    SKIPSPACE(p);
    if (*p == '\0')
        return 0;
    ptoken = p;
    /* Skip to end of keyword */
    while (isalnum(*p) || *p == '_')
    {
        ++p;
        ++size;
    }
    if (size == 0)
        return -1;
    /* Scan table to validate keyword */
    strncpy (keywordtxt, ptoken, size);
    keywordtxt[size] = '\0';
    for (i=0; i<sizeof table/sizeof table[0]; i++)
        if (strcmp(keywordtxt, table[i].keytxt) == 0)
        {
            keyid = table[i].keyid;
            break;
        }

	*bufp = p;
    return keyid;
} /* get_keyword */

void get_video_format_string( DWORD format, char *str )
{
	switch(format) {
		case NO_VIDEO:
			strcpy( str, "NO_VIDEO" );
			break;
		case MPEG4:
			strcpy( str, "MPEG4" );
			break;
		case H263:
			strcpy( str, "H263" );
			break;
		case H263_PLUS:
			strcpy( str, "H263_PLUS" );
			break;
		default:
			strcpy( str, "UNKNOWN" );
			break;			
	}
}

void get_audio_format_string(  tAudio_Format format, char *str )
{
	switch(format) {
		case N_AMR:
			strcpy( str, "AMR" );
			break;
		case N_G723:
			strcpy( str, "G.723.1" );		
			break;
		default:
			strcpy( str, "UNKNOWN" );		
			break;
	}
}

/* ***********************************************************************
   Function: PrintGwConfiguration
   Description: Prints an updated gateway configuration on the screen
   Input: None
   Output: None
************************************************************************ */
void PrintGwConfiguration()
{
	char video_str[20];
	char audio_str[20];

	printf("******************************************\n");
	printf("  Configuration Parameters for the Gateway:\n");
	printf("Number of ports                    : %d\n", gwParm.nPorts);	
	printf("Source IP Address for Video        : %s\n", gwParm.srcIPAddr_video);
	printf("Source Port for Video              : %d\n", gwParm.srcPort_video);
	printf("Destination IP Address for Video   : %s\n", gwParm.destIPAddr_video);
	printf("Destination Port for Video         : %d\n", gwParm.destPort_video);
	printf("Source IP Address for Audio        : %s\n", gwParm.srcIPAddr_audio);
	printf("Source Port for Audio              : %d\n", gwParm.srcPort_audio);
	printf("Destination IP Address for Audio   : %s\n", gwParm.destIPAddr_audio);
	printf("Destination Port for Audio         : %d\n", gwParm.destPort_audio);
	
	get_video_format_string( gwParm.videoFormat, video_str );
	printf("Video format                       : %s\n", video_str);

	get_video_format_string( gwParm.videoAlternateFormat, video_str );
	printf("Video Alternateformat              : %s\n", video_str);

	get_audio_format_string( gwParm.audioFormat, audio_str );
	printf("Audio format                       : %s\n", audio_str);
	
	printf("AMR maxBitRate                     : %d\n", gwParm.gsmamr_maxBitRate);
	printf("H263 maxBitRate                    : %d\n", gwParm.h263_maxBitRate);	
	printf("AMR mode                           : %d\n", gwParm.gsmamr_mode);	
	printf("ISDN Country                       : %d\n", gwParm.isdn_country);	
	printf("ISDN Operator                      : %d\n", gwParm.isdn_operator);	
	printf("ISDN Equipment                     : %d\n", gwParm.isdn_dNT_TE);		
	printf("Send Unidir. Audio OLC             : %d\n", gwParm.sendUniDirAudioOLC);
	printf("Send Video OLC                     : %d\n", gwParm.sendVideoOLC);
	printf("Reject Video OLC                   : %d\n", gwParm.rejectVideoOLC);		
	printf("Specify VO/VOL header              : %d\n", gwParm.specifyVOVOL);		
	printf("Ack Unidir. Audio OLC              : %d\n", gwParm.ackUniDirAudioOLC);		
	printf("MUX Timeslot                       : %d\n", gwParm.mux_timeslot);									
	printf("AutoPlaceCall                      : %d\n", gwParm.auto_place_call);
	printf("AutoHangup                         : %d\n", gwParm.auto_hangup);
	printf("BetweenCallsTime                   : %d\n", gwParm.between_calls_time);
	printf("DialNumber                         : %s\n", gwParm.dial_number);	
	printf("Number of calls                    : %d\n", gwParm.nCalls);		
	printf("Number of calls                    : %d\n", gwParm.nCalls);		    
	printf("Use Video Transcoding              : %d\n", gwParm.useVideoXc);		        
    printf("TRC Config File                    : %s\n", gwParm.sTrcConfigFile);	    
    printf("TRC log File                       : %s\n", gwParm.sTrcLogFile);	        
    printf("trcTraceMask                       : 0x%x\n", gwParm.trcTraceMask);    
    printf("trcFileTraceMask                   : 0x%x\n", gwParm.trcFileTraceMask);        
    printf("Interface type                     : %s\n", gwParm.sIfcType);	   
    printf("H324 LOG File                      : %s\n", gwParm.sH324LogFile);	   
	         
	printf("******************************************\n");	
}

void get_video_codec_string( DWORD videoCodec, char *str )
{
    switch( videoCodec ) {
        case H324_MPEG4_VIDEO:
			strcpy( str, "MPEG4" );            
            break;
        case H324_H263_VIDEO:
			strcpy( str, "H263" );            
            break;
        case H324_H261_VIDEO:
			strcpy( str, "H261" );            
            break;            
		default:
			strcpy( str, "UNKNOWN" );
			break;			            
    }    
}

int initVideoPath( int nGw, DWORD videoCodec )
{
	int ret = SUCCESS;
	char video_str[20];

    get_video_codec_string( videoCodec, video_str );

	//Configure the Video RTP Endpoint and Channel according to the video format
	switch( videoCodec ) {
		case H324_MPEG4_VIDEO:
	    	GwConfig[nGw].VidRtpEp.Addr.eEpType      = MSP_ENDPOINT_RTPFDX_VIDEO_MPEG4;			
	    	GwConfig[nGw].VidRtpEp.Param.eParmType   = MSP_ENDPOINT_RTPFDX_VIDEO_MPEG4;
		    GwConfig[nGw].VidRtpEp.Param.EP.RtpRtcp.PayloadMap.payload_id = 100;	
    		GwConfig[nGw].VidRtpEp.Param.EP.RtpRtcp.PayloadMap.vocoder    = 100;	
				
			GwConfig[nGw].VidChan.Addr.channelType   = MGVideoChannel;			
			GwConfig[nGw].VidChan.Param.channelType  = MGVideoChannel;												
			break;
		case H324_H263_VIDEO:
	    	GwConfig[nGw].VidRtpEp.Addr.eEpType      = MSP_ENDPOINT_RTPFDX_VIDEO_H263;			
	    	GwConfig[nGw].VidRtpEp.Param.eParmType   = MSP_ENDPOINT_RTPFDX_VIDEO_H263;				
		    GwConfig[nGw].VidRtpEp.Param.EP.RtpRtcp.PayloadMap.payload_id = 34;	
    		GwConfig[nGw].VidRtpEp.Param.EP.RtpRtcp.PayloadMap.vocoder    = 34;	
				
			GwConfig[nGw].VidChan.Addr.channelType   = MGH263VideoChannel;			
			GwConfig[nGw].VidChan.Param.channelType  = MGH263VideoChannel;											
			break;
		default:
			mmPrintPortErr(nGw,"Invalid video format!\n");
			return FAILURE;
	}
    
    
	// Create the Video RTP EndPoint
    if(GwConfig[nGw].VidRtpEp.bActive) {
	    mmPrint( nGw, T_MSPP, "Creating the %s Video RTP context and endpoint\n", video_str);	
		if(initCreateRtpEndpoint(nGw, &GwConfig[nGw].VidRtpEp) != SUCCESS)
			return FAILURE;			
	}

    GwConfig[nGw].VidRtpEp.bEnabled = TRUE;        
        
	// Create the Video Channel
    if(GwConfig[nGw].VidChan.bActive) {
	    mmPrint( nGw, T_MSPP, "Create the %s Video channel context and channel.\n", video_str);
		if(initCreateChannel(nGw, &GwConfig[nGw].VidChan) != SUCCESS)
			return FAILURE;
	}
	
	// Connect the Video RTP EndPoint and MUX EndPoint using the Video Channel
    if( GwConfig[nGw].MuxEp.bActive && 
    	GwConfig[nGw].VidChan.bActive && 
        GwConfig[nGw].VidRtpEp.bActive)	{
	    mmPrint( nGw, T_MSPP, "Connecting the %s Video channel to the %s Video RTP and MUX endpoints.\n", 
				video_str, video_str);		
		ret = initConnectAndWait(nGw, GwConfig[nGw].VidRtpEp.hd,
								GwConfig[nGw].VidChan.hd, 
								GwConfig[nGw].MuxEp.hd);
	}	
		    
    GwConfig[nGw].video_path_initialized = TRUE;    
    
	return ret;
}

int initAudioBypassPath( int nGw )
{
	int ret = SUCCESS;
	char audio_str[20];

	get_audio_format_string( gwParm.audioFormat, audio_str );

	// Create the Audio Bypass Rtp EP 
    if(GwConfig[nGw].AudRtpEp.bActive) {
	    mmPrint( nGw, T_MSPP, "Creating the %s RTP context and endpoint\n", audio_str);	
		if(initCreateRtpEndpoint(nGw, &GwConfig[nGw].AudRtpEp) != SUCCESS)
			return FAILURE;			
	}

	// Create the Audio Bypass Channel 
    if(GwConfig[nGw].AudByChan.bActive) {
	    mmPrint( nGw, T_MSPP, "Create the %s Bypass channel context and channel.\n", audio_str);
		if(initCreateChannel(nGw, &GwConfig[nGw].AudByChan) != SUCCESS)
			return FAILURE;
	}
	
	// Connect the Audio Bypass RTP EndPoint and MUX EndPoint using the Video Channel
    if( GwConfig[nGw].MuxEp.bActive && 
    	GwConfig[nGw].AudByChan.bActive && 
        GwConfig[nGw].AudRtpEp.bActive)	{
	    mmPrint( nGw, T_MSPP, "Connecting the %s Bypass channel to the %s Bypass RTP and MUX endpoints.\n",
			audio_str, audio_str); 
		ret = initConnectAndWait(nGw, GwConfig[nGw].AudRtpEp.hd,
								GwConfig[nGw].AudByChan.hd, 
								GwConfig[nGw].MuxEp.hd);
	}	
    
    GwConfig[nGw].audio_path_initialized = TRUE;

	return ret;
}


