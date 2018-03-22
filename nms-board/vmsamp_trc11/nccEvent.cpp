/**********************************************************************
*  File - nccEvent.cpp
*
*  Description - The functions in this file process inbound NCC events 
*
*  2005  NMS Communications Corporation.
*
***********************************************************************/


///////////////////
// Include Files //
///////////////////
#include "mega_global.h"
#include "srv_types.h"   /* cmdStopAudio(), cmdStopVideo() */
#include "srv_def.h"
#include "gw_types.h"    /* gwParms */
#include "vm_trace.h"


char *szStateStr[] = { 
        "IDL",  //		  idle, 
        "SZR",  //        seizure, 
        "INC",  //        incoming, 
        "RCD",  //        receivingDigits
        "ACC",  //        accepting,
        "ANS",  //        answering, 
        "REJ",  //        rejecting, 
        "CON",  //        connected,  
        "DIS",  //        disconnected, 
        "OTB",  //        outboundInit, 
        "PLC",  //        placing, 
        "PRO",  //        proceeding
        "_x_"    //       no_state_change
};
//--------------------------  Functions  -----------------------------

DWORD disableVideoChannel( int nGw, MSPHD hd, DWORD index );
DWORD disableAudByChannel( int nGw, MSPHD hd );
DWORD enableAudByChannel( int nGw, MSPHD hd, DWORD index );
DWORD enableVideoChannel( int nGw, MSPHD hd, DWORD index );
DWORD enableVideoEP( int nGw, MSPHD hd, DWORD index );
DWORD disableVideoEP( int nGw, MSPHD hd, DWORD index );
DWORD disableAudioEP( int nGw, MSPHD hd );

extern tVideo_Format convertVideoCodec2Format( DWORD VideoCodec );

////////////////////////////////////////////////////////////////////////////////////
// Function to process an incoming line sezure
////////////////////////////////////////////////////////////////////////////////////
int Sez( CTA_EVENT *event, GW_CALL *pCtx )
{
    pCtx->callhd = event->objHd;
	mmPrint(pCtx->dCXIndex, T_CC, "Call Handle is %08x for CTAHD %08x\n",
			 pCtx->ctahd, pCtx->callhd);
    pCtx->dDirection = PBX_INBOUND;
    return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////
// Function to process an incoming call event
/////////////////////////////////////////////////////////////////////////////////
///
int Inc( CTA_EVENT *event, GW_CALL *pCtx )
{
    DWORD ret;
    
    // Retrieve information on the call
    ret = nccGetCallStatus ( pCtx->callhd, &pCtx->sCallStat, sizeof(pCtx->sCallStat) );

	mmPrint(pCtx->dCXIndex, T_CC, "Incoming call to %s\n", pCtx->sCallStat.calledaddr);

    // Change from PBX - Answer immediately, without going to Accepting state
    //nccAcceptCall( pCtx->callhd, NCC_ACCEPT_PLAY_RING, NULL ); // Accept call, play ring
    if(SUCCESS != nccAnswerCall ( pCtx->callhd, 0 /*zero ring*/, NULL) )
    {
        return FAILURE;
    }

    Flow(pCtx->dCXIndex, F_APPL, F_NCC, "nccAnswerCall");    
    return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////
// Function to process a NCCEVN_CALL_DISCONNECTED event
////////////////////////////////////////////////////////////////////////////////////
int Dis( CTA_EVENT *event, GW_CALL *pCtx )
{ 
    DWORD ret;
	MEDIA_GATEWAY_CONFIG *pCfg;
    	
	// Get a pointer to the gateway's configuration
	pCfg = &GwConfig[pCtx->dCXIndex];    

   // tell the remote to stop tromboning, if needed
   if (  pCfg->TromboneState == TROMBONING_STATE)
   {  
      // must disable Endpoint because Destroying them does not guarantee that an other Endpoint 
      // on the board can get the same destination   
      disableVideoEP( pCtx->dCXIndex, GwConfig[pCtx->dCXIndex].vidRtpEp[0].hd, 0 );
      if (gwParm.simplexEPs)
         disableVideoEP( pCtx->dCXIndex, GwConfig[pCtx->dCXIndex].vidRtpEp[1].hd, 1 );      
      disableAudioEP( pCtx->dCXIndex, GwConfig[pCtx->dCXIndex].AudRtpEp[0].hd );
      if (gwParm.simplexEPs)
          disableAudioEP( pCtx->dCXIndex, GwConfig[pCtx->dCXIndex].AudRtpEp[1].hd );
   }

    
	//Stop playing/recording when disconnected
    if( (mmParm.Audio_RTP_Format != NO_AUDIO) && 
		((AudioCtx[pCtx->dCXIndex].pr_mode_p == PR_MODE_ACTIVE1) ||
		 (AudioCtx[pCtx->dCXIndex].pr_mode_r == PR_MODE_ACTIVE1)) )
        	cmdStopAudio( pCtx->dCXIndex );				

    if( (mmParm.Video_Format != NO_VIDEO) &&
		((VideoCtx[pCtx->dCXIndex].pr_mode_p == PR_MODE_ACTIVE1) ||
		 (VideoCtx[pCtx->dCXIndex].pr_mode_r == PR_MODE_ACTIVE1)) )
	        cmdStopVideo( pCtx->dCXIndex );	

	if( pCfg->shutdown_in_progress )
		return SUCCESS;
          
    // Stop the H.324 middleware
    mmPrint(pCtx->dCXIndex, T_H245,"Stop H.324 middleware.\n");
	ret = h324Stop(pCfg->MuxEp.hd);
    Flow(pCtx->dCXIndex, F_APPL, F_H324, "h324Stop");    
    
	mmPrint(pCtx->dCXIndex, T_CC,"%s Call Disconnected\n", pCtx->szDescriptor);
          
    // Release the call
/*    
    nccReleaseCall ( pCtx->callhd, NULL );
    Flow(pCtx->dCXIndex, F_APPL, F_NCC, "nccReleaseCall");
*/    
    return SUCCESS; 
}



////////////////////////////////////////////////////////////////////////////////////
// Function to process NCCEVN_ACCEPTING_CALL call event
////////////////////////////////////////////////////////////////////////////////////
int Acc( CTA_EVENT *event, GW_CALL *pCtx )
{ 
    // Change from PBX - if we get to this state, Answer immediately, 
    if(SUCCESS != nccAnswerCall ( pCtx->callhd, 0 , NULL) ) // one ring
    {
        return FAILURE;
    }
    
    Flow(pCtx->dCXIndex, F_APPL, F_NCC, "nccAnswerCall");    
    return SUCCESS; 
}

////////////////////////////////////////////////////////////////////////////////////
// Invalid Place Call - Function implemented when a PBX_EVENT_PLACE_CALL comes in
// and the line is not currently idle. 
////////////////////////////////////////////////////////////////////////////////////
int Ipc( CTA_EVENT *event, GW_CALL *pCtx )
{ 
    // Do not place the call (generate error message, and do nothing)
    mmPrint(pCtx->dCXIndex, T_CC, "Warning: Call can not be placed on an active line\n");
    return SUCCESS; 
}


////////////////////////////////////////////////////////////////////////////////////
// Otb - Outbound call initiate
// Function used when a line in the idle state receives PBX_EVENT_PLACE_CALL 
////////////////////////////////////////////////////////////////////////////////////
int Otb( CTA_EVENT *event, GW_CALL *pCtx )
{ 
    int     ret;
    PBX_PLACE_CALL_STRUCT *pPbxPCStruct;

    // If the place call event has no buffer, the call fails
    if(event->buffer == NULL)
    {
        return FAILURE;
    }

    // If the line is not Idle, return failure
    if( pCtx->dCallState != IDL)
    {
        return FAILURE;
    }

    // Copy the event buffer values into the context structure
    pPbxPCStruct = (PBX_PLACE_CALL_STRUCT*)event->buffer;
    pCtx->dDirection = PBX_OUTBOUND;

    memcpy( pCtx->sCallStat.calledaddr,  pPbxPCStruct->calledaddr,  sizeof(pCtx->sCallStat.calledaddr) );
    memcpy( pCtx->sCallStat.callingaddr, pPbxPCStruct->callingaddr, sizeof(pCtx->sCallStat.callingaddr));
    memcpy( pCtx->sCallStat.callingname, pPbxPCStruct->callingname, sizeof(pCtx->sCallStat.callingname));

    // Place the call 
    ret = nccPlaceCall ( pCtx->ctahd, 
                         pCtx->sCallStat.calledaddr, 
                         pCtx->sCallStat.callingaddr, 
                         NULL, 
                         NULL, 
                         &pCtx->callhd );
    if( ret != SUCCESS )
    {
        return FAILURE;
    }

    Flow(pCtx->dCXIndex, F_APPL, F_NCC, "nccPlaceCall");
        
    return SUCCESS; 
}


////////////////////////////////////////////////////////////////////////////////////
// PRj - Partner Reject 
// Function to process PBX_EVENT_DISCONNECT call event, while in Accepting state
// (Parterner disconnect forces a reject)
////////////////////////////////////////////////////////////////////////////////////
int PRj( CTA_EVENT *event, GW_CALL *pCtx )
{ 
    DWORD ret;
    if(SUCCESS != (ret=nccRejectCall ( pCtx->callhd, NCC_REJECT_PLAY_REORDER, NULL ) ) )
    {
        return FAILURE;
    }   
    Flow(pCtx->dCXIndex, F_APPL, F_NCC, "nccRejectCall");    
    return SUCCESS; 
} 

////////////////////////////////////////////////////////////////////////////////////
// PDc - Partner Disconnect
// Function to process PBX_EVENT_DISCONNECT call event, while in Connected state
// (Parterner disconnect forces a disconnect)
////////////////////////////////////////////////////////////////////////////////////
int PDc( CTA_EVENT *event, GW_CALL *pCtx )
{
    DWORD ret;
    
    if(SUCCESS != (ret=nccDisconnectCall  ( pCtx->callhd, NULL ) ) )
    {
        return FAILURE;
    }
    Flow(pCtx->dCXIndex, F_APPL, F_NCC, "nccDisconnectCall");    
    return SUCCESS; 
} 

////////////////////////////////////////////////////////////////////////////////////
// PDi - Partner Disconnect Invalid
// Function to process PBX_EVENT_DISCONNECT call event, while the call is not 
// connected - Issue a warning, and do nothing
////////////////////////////////////////////////////////////////////////////////////
int PDi( CTA_EVENT *event, GW_CALL *pCtx )
{
    mmPrint(pCtx->dCXIndex, T_CC, "Warning: Attempt to disconnect a disconnected line was ignored\n");
    return SUCCESS; 
} 

////////////////////////////////////////////////////////////////////////////////////
// Con - Call Connected
// Function to process NCCEVN_CALL_CONNECTED call event
////////////////////////////////////////////////////////////////////////////////////
int Con( CTA_EVENT *event, GW_CALL *pCtx )
{ 
    H324_START_PARAMS params;
	MEDIA_GATEWAY_CONFIG *pCfg;
    	
	// Get a pointer to the gateway's configuration
	pCfg = &GwConfig[pCtx->dCXIndex];    
    
    params.size         = sizeof(params);
    params.terminalType = 201;
    params.iCapCount    = 2;
	params.bAutoChannelSetup = TRUE;
	
	switch( gwParm.audioFormat ) {
		case N_AMR:	
			params.iCap[0] = H324_AMR_AUDIO;		
			break;			
		case N_G723:
			params.iCap[0] = H324_G723_AUDIO;
			break;
		default:
			mmPrintPortErr(pCtx->dCXIndex,"Invalid audio format!\n");
			return FAILURE;			
	}

	switch( gwParm.videoFormat ) {
		case MPEG4:	
			params.iCap[1] = H324_MPEG4_VIDEO;		
			break;			
		case H263:
			params.iCap[1] = H324_H263_VIDEO;
			break;
        case H263_PLUS: /* JJF mod */
            params.iCap[1] = H324_H263_VIDEO;
			break;                
		default:
			mmPrintPortErr(pCtx->dCXIndex,"Invalid video format!\n");
			return FAILURE;			
	}

	switch( gwParm.videoAlternateFormat ) {
	   case NO_VIDEO:	
			break;
		case MPEG4:	
			params.iCap[2] = H324_MPEG4_VIDEO;
			params.iCapCount++;		
			break;			
		case H263:
			params.iCap[2] = H324_H263_VIDEO;
			params.iCapCount++;
			break;
        case H263_PLUS: /* JJF mod */
            params.iCap[2] = H324_H263_VIDEO;
            params.iCapCount++;
			break;                
		default:
			mmPrintPortErr(pCtx->dCXIndex,"Invalid alternate video format!\n");
			return FAILURE;			
	}

	mmPrint(pCtx->dCXIndex, T_CC,"%s Call Connected\n", pCtx->szDescriptor);

    // Enable the Audio Bypass Channel 
    enableAudByChannel( pCtx->dCXIndex, GwConfig[pCtx->dCXIndex].AudByChan[0].hd, 0 );
    if (gwParm.simplexEPs)
        enableAudByChannel( pCtx->dCXIndex, GwConfig[pCtx->dCXIndex].AudByChan[1].hd, 1 );
   
	// Start up the H.324 middleware
   	GwConfig[pCtx->dCXIndex].TromboneState = CONNECTING_STATE;
   	h324Start(pCtx->ctahd, pCfg->MuxEp.hd, &params);
    Flow(pCtx->dCXIndex, F_APPL, F_H324, "h324Start");
               
    return SUCCESS; 
}

////////////////////////////////////////////////////////////////////////////////////
// Function to Re - Initialize a context after a call is disconnected
////////////////////////////////////////////////////////////////////////////////////
void pbxInitializeCtx(GW_CALL *pCtx)
{
    pCtx->callhd                    = NULL_NCC_CALLHD;
    pCtx->dDirection                = PBX_UNDEFINED;
    pCtx->dNewState                 = IDL;
    pCtx->sCallStat.callingname[0]  = '\0';
    pCtx->sCallStat.calledaddr[0]   = '\0';
    pCtx->sCallStat.calledaddr[0]   = '\0';
    return;
}

////////////////////////////////////////////////////////////////////////////////////
// Function to process NCCEVN_CALL_RELEASED call event
////////////////////////////////////////////////////////////////////////////////////
int Idl( CTA_EVENT *event, GW_CALL *pCtx )
{ 
    pbxInitializeCtx(pCtx);
    return SUCCESS; 
}  

void drt_debug(void)
{
	return;
}

////////////////////////////////////////////////////////////////////////////////////
// Null function does nothing
////////////////////////////////////////////////////////////////////////////////////
int _n_( CTA_EVENT *event, GW_CALL *pCtx ){ return SUCCESS; }

//Dummy Functions replace with real ones if needed
int Ans( CTA_EVENT *event, GW_CALL *pCtx ){ return SUCCESS; }  // on NCCEVN_ANSWERING_CALL  no action needed
int Pro( CTA_EVENT *event, GW_CALL *pCtx ){ return SUCCESS; }  // on NCCEVN_CALL_PROCEEDING no action needed
int Rej( CTA_EVENT *event, GW_CALL *pCtx ){ return SUCCESS; }  // on NCCEVN_REJECTING_CALL  no action needed
int Rcd( CTA_EVENT *event, GW_CALL *pCtx ){ return SUCCESS; }  // on NCCEVN_RECEIVED_DIGIT  no action needed
int Plc( CTA_EVENT *event, GW_CALL *pCtx ){ return SUCCESS; }  // on NCCEVN_PLACING_CALL    no action needed


// Print out the NCC capabilities of a line
void pbxListCapabilities( GW_CALL *pCtx )
{
#define PRINT_CAP( capmask, cap ) if(capmask & cap){ mmPrint(pCtx->dCXIndex, T_CC, "  "#cap"\n"); }

    NCC_PROT_CAP protcap;
    if( SUCCESS != nccQueryCapability( pCtx->ctahd, &protcap, sizeof( protcap) ) )
        return;
    mmPrint(pCtx->dCXIndex, T_CC, "Capabilities of %s:\n", pCtx->szDescriptor);

    PRINT_CAP( protcap.capabilitymask, NCC_CAP_ACCEPT_CALL )
    PRINT_CAP( protcap.capabilitymask, NCC_CAP_SET_BILLING )
    PRINT_CAP( protcap.capabilitymask, NCC_CAP_OVERLAPPED_SENDING )
    PRINT_CAP( protcap.capabilitymask, NCC_CAP_HOLD_CALL )
    PRINT_CAP( protcap.capabilitymask, NCC_CAP_SUPERVISED_TRANSFER )
    PRINT_CAP( protcap.capabilitymask, NCC_CAP_AUTOMATIC_TRANSFER )
    PRINT_CAP( protcap.capabilitymask, NCC_CAP_EXTENDED_CALL_STATUS )
    PRINT_CAP( protcap.capabilitymask, NCC_CAP_SEND_CALL_MESSAGE )
    PRINT_CAP( protcap.capabilitymask, NCC_CAP_SEND_LINE_MESSAGE )
    PRINT_CAP( protcap.capabilitymask, NCC_CAP_HOLD_IN_ANY_STATE )
    PRINT_CAP( protcap.capabilitymask, NCC_CAP_DISCONNECT_IN_ANY_STATE )
    PRINT_CAP( protcap.capabilitymask, NCC_CAP_MEDIA_IN_SETUP )
    PRINT_CAP( protcap.capabilitymask, NCC_CAP_CALLER_ID )  

    return;
}


////////////////////////////////////////////////////////////////////////////////////
// ProcessNCCEvent
//
// This is the main function used for processing all incoming events on a line
// For all events related to the call state, there is a state machine which 
// determines what state to change to, and what action to take for any state/event
// pair.
////////////////////////////////////////////////////////////////////////////////////
int ProcessNCCEvent( CTA_EVENT *event, GW_CALL *pCtx )
{
    DWORD           i;
    int             ret;
    static DWORD    callEvents[] = {
                        NCCEVN_ACCEPTING_CALL,
                        NCCEVN_ANSWERING_CALL,
                        NCCEVN_CALL_CONNECTED,
                        NCCEVN_CALL_DISCONNECTED,
                        NCCEVN_CALL_PROCEEDING,
                        NCCEVN_INCOMING_CALL,
                        NCCEVN_PLACING_CALL,
                        NCCEVN_REJECTING_CALL,
                        NCCEVN_CALL_RELEASED,
                        NCCEVN_SEIZURE_DETECTED,
                        NCCEVN_RECEIVED_DIGIT,
                        PBX_EVENT_PLACE_CALL,
                        PBX_EVENT_DISCONNECT,
                        PBX_EVENT_PARTNER_CONNECTED   };

#define CALL_EVENT_COUNT 14
#define CALL_STATE_COUNT 12

    // This table represents the state changes of the state transition diagram
    // for a call -- _x_ means "no state change"
    static tLineState stateTransArray[CALL_EVENT_COUNT][CALL_STATE_COUNT] = {
    /*IDL  SZR  INC  RCD  ACC  ANS  REJ  CON  DIS  OTB  PLC  PRO  _x_  */
    { _x_, _x_, ACC, ACC, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_ }, //    NCCEVN_ACCEPTING_CALL,
    { _x_, _x_, ANS, ANS, ANS, _x_, _x_, _x_, _x_, _x_, _x_, _x_ }, //    NCCEVN_ANSWERING_CALL,
    { _x_, _x_, _x_, _x_, _x_, CON, _x_, _x_, _x_, _x_, CON, CON }, //    NCCEVN_CALL_CONNECTED,
    { DIS, DIS, DIS, DIS, DIS, DIS, DIS, DIS, DIS, DIS, DIS, DIS }, //    NCCEVN_CALL_DISCONNECTED,
    { _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, PRO, _x_ }, //    NCCEVN_CALL_PROCEEDING,
    { _x_, INC, _x_, INC, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_ }, //    NCCEVN_INCOMING_CALL,
    { _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, PLC, _x_, _x_ }, //    NCCEVN_PLACING_CALL,
    { _x_, _x_, REJ, REJ, REJ, _x_, _x_, _x_, _x_, _x_, _x_, _x_ }, //    NCCEVN_REJECTING_CALL,
    { _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, IDL, IDL, _x_, _x_ }, //    NCCEVN_CALL_RELEASED,
    { SZR, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_ }, //    NCCEVN_SEIZURE_DETECTED,
    { _x_, RCD, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_ }, //    NCCEVN_RECEIVED_DIGIT,
    { OTB, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_ }, //    PBX_EVENT_PLACE_CALL,
    { _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_ }, //    PBX_EVENT_DISCONNECT,
    { _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_, _x_ } };//   PBX_EVENT_PARTNER_CONNECTED 

    // This table represents the actions which must be taken for each event
    // The function _n_ does nothing
    static int (*pEventFunction[CALL_EVENT_COUNT][CALL_STATE_COUNT])( CTA_EVENT *event, GW_CALL *pCtx ) = {
    /*IDL  SZR  INC  RCD  ACC  ANS  REJ  CON  DIS  OTB  PLC  PRO  _x_  */
    {&_n_,&_n_,&Acc,&Acc,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_ }, //    NCCEVN_ACCEPTING_CALL,
    {&_n_,&_n_,&_n_,&Ans,&Ans,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_ }, //    NCCEVN_ANSWERING_CALL,
    {&_n_,&_n_,&_n_,&_n_,&_n_,&Con,&_n_,&_n_,&_n_,&_n_,&Con,&Con }, //    NCCEVN_CALL_CONNECTED,
    {&Dis,&Dis,&Dis,&Dis,&Dis,&Dis,&Dis,&Dis,&Dis,&Dis,&Dis,&Dis }, //    NCCEVN_CALL_DISCONNECTED,
    {&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&Pro,&_n_ }, //    NCCEVN_CALL_PROCEEDING,
    {&_n_,&Inc,&_n_,&Inc,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_ }, //    NCCEVN_INCOMING_CALL,
    {&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&Plc,&_n_,&_n_ }, //    NCCEVN_PLACING_CALL,
    {&_n_,&_n_,&Rej,&Rej,&Rej,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_ }, //    NCCEVN_REJECTING_CALL,
    {&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&Idl,&Idl,&_n_,&_n_ }, //    NCCEVN_CALL_RELEASED,
    {&Sez,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_ }, //    NCCEVN_SEIZURE_DETECTED,
    {&_n_,&Rcd,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_ }, //    NCCEVN_RECEIVED_DIGIT,
    {&Otb,&Ipc,&Ipc,&Ipc,&Ipc,&Ipc,&Ipc,&Ipc,&Ipc,&Ipc,&Ipc,&Ipc }, //    PBX_EVENT_PLACE_CALL,
    {&PDi,&PDc,&PDc,&PDc,&PRj,&PDc,&_n_,&PDc,&_n_,&PDc,&PDc,&PDc }, //    PBX_EVENT_DISCONNECT,
    {&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_,&_n_ } };//   PBX_EVENT_PARTNER_CONNECTED */



    // Switch on event to display the event
	if( event->id != ADIEVN_RECORD_BUFFER_FULL )
	{
		mmPrint(pCtx->dCXIndex, T_CC, "EVENT received on  cx: %08x cx_name: %s, state %s\n", 
		    pCtx->ctahd, pCtx->szDescriptor, 
			szStateStr[pCtx->dCallState]);
	}
    switch( event->id )
    {
        case NCCEVN_ACCEPTING_CALL:
            Flow(pCtx->dCXIndex, F_NCC, F_APPL, "NCCEVN_ACCEPTING_CALL");
            mmPrint(pCtx->dCXIndex, T_CC, "Event:NCCEVN_ACCEPTING_CALL\n");
            break;
        case NCCEVN_ANSWERING_CALL:
            Flow(pCtx->dCXIndex, F_NCC, F_APPL, "NCCEVN_ANSWERING_CALL");
            mmPrint(pCtx->dCXIndex, T_CC, "Event:NCCEVN_ACCEPTING_CALL\n");
            break;
        case NCCEVN_CALL_CONNECTED:
            Flow(pCtx->dCXIndex, F_NCC, F_APPL, "NCCEVN_CALL_CONNECTED");
            mmPrint(pCtx->dCXIndex, T_CC, "Event:NCCEVN_ACCEPTING_CALL\n");
            break;
        case NCCEVN_CALL_DISCONNECTED:
            Flow(pCtx->dCXIndex, F_NCC, F_APPL, "NCCEVN_CALL_DISCONNECTED");
            mmPrint(pCtx->dCXIndex, T_CC, "Event:NCCEVN_CALL_DISCONNECTED\n");
            break;
        case NCCEVN_CALL_PROCEEDING:
            Flow(pCtx->dCXIndex, F_NCC, F_APPL, "NCCEVN_CALL_PROCEEDING");
            mmPrint(pCtx->dCXIndex, T_CC, "Event:NCCEVN_CALL_PROCEEDING\n");
            break;
         case NCCEVN_INCOMING_CALL:
            Flow(pCtx->dCXIndex, F_NCC, F_APPL, "NCCEVN_INCOMING_CALL");
            mmPrint(pCtx->dCXIndex, T_CC, "Event:NCCEVN_INCOMING_CALL\n");
            break;
         case NCCEVN_PLACING_CALL:
            Flow(pCtx->dCXIndex, F_NCC, F_APPL, "NCCEVN_PLACING_CALL");
            mmPrint(pCtx->dCXIndex, T_CC, "Event:NCCEVN_PLACING_CALL\n");
            break;
        case NCCEVN_REJECTING_CALL:
            Flow(pCtx->dCXIndex, F_NCC, F_APPL, "NCCEVN_REJECTING_CALL");
            mmPrint(pCtx->dCXIndex, T_CC, "Event:NCCEVN_REJECTING_CALL\n");
            break;
        case NCCEVN_CALL_RELEASED:
            Flow(pCtx->dCXIndex, F_NCC, F_APPL, "NCCEVN_CALL_RELEASED");
            mmPrint(pCtx->dCXIndex, T_CC, "Event:NCCEVN_CALL_RELEASED\n");
            break;
        case NCCEVN_SEIZURE_DETECTED:
            Flow(pCtx->dCXIndex, F_NCC, F_APPL, "NCCEVN_SEIZURE_DETECTED");
            mmPrint(pCtx->dCXIndex, T_CC, "Event:NCCEVN_SEIZURE_DETECTED\n");
            break;
                    
        case NCCEVN_CAPABILITY_UPDATE:
			mmPrint(pCtx->dCXIndex, T_CC, "Event: NCCEVN_CAPABILITY_UPDATE sent to %s \n", pCtx->szDescriptor);
            pbxListCapabilities( pCtx );
			break;
        case PBX_EVENT_PLACE_CALL:
			mmPrint(pCtx->dCXIndex, T_CC, "Event: PBX_EVENT_PLACE_CALL sent to %s\n", pCtx->szDescriptor);
            break;
        case PBX_EVENT_DISCONNECT:
			mmPrint(pCtx->dCXIndex, T_CC, "Event: PBX_EVENT_DISCONNECT sent to %s \n", pCtx->szDescriptor);
            break;
        case PBX_EVENT_PARTNER_CONNECTED:
			mmPrint(pCtx->dCXIndex, T_CC, "Event: PBX_EVENT_PARTNER_CONNECTED sent to %s\n", pCtx->szDescriptor);
            break;

        default:
            break;
    }

    // Find out if the event was a call event, and if so, find the event index, i
    for(i=0; i<CALL_EVENT_COUNT; i++)
    {
        if(event->id == callEvents[i])
            break;
    }
    
    // If it was a call event...
    if(i<CALL_EVENT_COUNT)
    {

        // If there was no state change, new state equals old state
        pCtx->dNewState = stateTransArray[i][pCtx->dCallState];
        if(pCtx->dNewState == _x_)
        {
             pCtx->dNewState = pCtx->dCallState;
        }

        // Now perform the required function for this state and event
        ret = (*pEventFunction[i][pCtx->dCallState])( event, pCtx);

        // Set the call state to the new call state
		if(pCtx->dCallState != pCtx->dNewState)
			mmPrint(pCtx->dCXIndex, T_CC, "%s State Transition %s ==>> %s\n",  
			  pCtx->szDescriptor, szStateStr[pCtx->dCallState], szStateStr[pCtx->dNewState]);
        pCtx->dCallState = pCtx->dNewState;
    }



    return SUCCESS;
}

/**************************** MSPP functions *************************/

DWORD disableVideoChannel( int nGw, MSPHD hd, DWORD index )
{
	DWORD ret;
	CTA_EVENT waitEvent;

	mmPrint(nGw, T_MSPP,"Disable the Video channel.\n");
	if( !GwConfig[nGw].vidChannel[index].bEnabled ) {
		mmPrint( nGw, T_MSPP, "Warning: Video Channel was already disabled. Do nothing.\n");
		return SUCCESS;
	}
	
    ret = mspDisableChannel(hd);
	if (ret != SUCCESS) {
	    mmPrintPortErr(nGw, "mspDisableChannel returned %d.\n", ret);
		return ret;
    } else {
		ret = WaitForSpecificEvent ( nGw, GwConfig[nGw].hCtaQueueHd, 
					MSPEVN_DISABLE_CHANNEL_DONE, &waitEvent);
    	if (ret != 0) {
	    	mmPrintPortErr( nGw, "MSPEVN_DISABLE_CHANNEL_DONE event failure, ret= %x, waitEvent.value=%x\n",
				        	ret, waitEvent.value);
			return FAILURE;
	   	} else
			GwConfig[nGw].vidChannel[index].bEnabled = FALSE;
	}
	
	return SUCCESS;
}

DWORD disableAudByChannel( int nGw, MSPHD hd, DWORD index )
{
	DWORD ret;
	CTA_EVENT waitEvent;

	mmPrint(nGw, T_MSPP,"Disable the Audio Bypass Channel.\n");
	if( !GwConfig[nGw].AudByChan[index].bEnabled ) {
		mmPrint( nGw, T_MSPP, "Warning: Audio Bypass Channel was already disabled. Do nothing.\n");
		return SUCCESS;
	}	
	
    ret = mspDisableChannel(hd);
	if (ret != SUCCESS) {
	    mmPrintPortErr(nGw, "nmspDisableChannel returned %d.\n", ret);
		return ret;
    } else {
		ret = WaitForSpecificEvent ( nGw, GwConfig[nGw].hCtaQueueHd, 
					MSPEVN_DISABLE_CHANNEL_DONE, &waitEvent);
    	if (ret != 0) {
	    	mmPrintPortErr( nGw, "MSPEVN_DISABLE_CHANNEL_DONE event failure, ret= %x, waitEvent.value=%x\n",
			        	ret, waitEvent.value );
			return FAILURE;
    	} else
			GwConfig[nGw].AudByChan[index].bEnabled = FALSE;
	}

	return SUCCESS;
}


DWORD enableAudByChannel( int nGw, MSPHD hd, DWORD index )
{
	DWORD ret;
	CTA_EVENT waitEvent;

	mmPrint(nGw, T_MSPP,"Enable the Audio Bypass Channel.\n");
	if( GwConfig[nGw].AudByChan[index].bEnabled ) {
		mmPrint( nGw, T_MSPP, "Warning: Audio Bypass Channel was already enabled. Do nothing.\n");
		return SUCCESS;
	}

    ret = mspEnableChannel(hd);
    if (ret != SUCCESS) {
	    mmPrintPortErr( nGw, "mspEnableChannel returned %d.\n", ret);
		return ret;
    } else {
		ret = WaitForSpecificEvent ( nGw, GwConfig[nGw].hCtaQueueHd, 
					MSPEVN_ENABLE_CHANNEL_DONE, &waitEvent);
    	if (ret != 0) {
	    	mmPrintPortErr(nGw, "MSPEVN_ENABLE_CHANNEL_DONE event failure, ret= %x, waitEvent.value=%x,"
                   	" board error (waitEvent.size field) %x\n\n",
		        	ret, waitEvent.value, waitEvent.size);
			return ret;
    	} else
			GwConfig[nGw].AudByChan[index].bEnabled = TRUE;
	}
	
	return SUCCESS; 	
}

DWORD enableVideoChannel( int nGw, MSPHD hd, DWORD index )
{
	DWORD ret;
	CTA_EVENT waitEvent;
	
    mmPrint(nGw,T_MSPP,"Enable the Video channel.\n");
	if( GwConfig[nGw].vidChannel[index].bEnabled ) {
		mmPrint( nGw, T_MSPP, "Warning: Video Channel was already enabled. Do nothing.\n");
		return SUCCESS;
	}
	
    ret = mspEnableChannel(hd);
    if (ret != SUCCESS) {
		    mmPrintPortErr( nGw, "mspEnableChannel returned %d.\n", ret);
    } else {
		ret = WaitForSpecificEvent ( nGw, GwConfig[nGw].hCtaQueueHd, 
					MSPEVN_ENABLE_CHANNEL_DONE, &waitEvent);
    	if (ret != 0) {
	    	mmPrintPortErr(nGw, "MSPEVN_ENABLE_CHANNEL_DONE event failure, ret= %x, waitEvent.value=%x,"
                   	" board error (waitEvent.size field) %x\n\n",
		        	ret, waitEvent.value, waitEvent.size);
			return ret;
    	} else
			GwConfig[nGw].vidChannel[index].bEnabled = TRUE;
	}
	
	return SUCCESS;
}

DWORD enableVideoEP( int nGw, MSPHD hd, DWORD index )
{
	DWORD ret;
	CTA_EVENT waitEvent;
	
    mmPrint(nGw,T_MSPP,"Enable the Video EP.\n");
	if( GwConfig[nGw].vidRtpEp[index].bEnabled ) {
		mmPrint( nGw, T_MSPP, "Warning: Video endpoint was already enabled. Do nothing.\n");
		return SUCCESS;
	}
	
    ret = mspEnableEndpoint(hd);
    if (ret != SUCCESS) {
		    mmPrintPortErr( nGw, "mspEnableEndpoint returned %d.\n", ret);
    } else {
		ret = WaitForSpecificEvent ( nGw, GwConfig[nGw].hCtaQueueHd, 
					MSPEVN_ENABLE_ENDPOINT_DONE, &waitEvent);
    	if (ret != 0) {
	    	mmPrintPortErr(nGw, "MSPEVN_ENABLE_ENDPOINT_DONE event failure, ret= %x, waitEvent.value=%x,"
                   	" board error (waitEvent.size field) %x\n\n",
		        	ret, waitEvent.value, waitEvent.size);
			return ret;
    	} else
			GwConfig[nGw].vidRtpEp[index].bEnabled = TRUE;
	}
	
	return SUCCESS;
}

DWORD disableVideoEP( int nGw, MSPHD hd, DWORD index )
{
	DWORD ret;
	CTA_EVENT waitEvent;

	mmPrint(nGw, T_MSPP,"Disable the Video endpoint.\n");
	if( !GwConfig[nGw].vidRtpEp[index].bEnabled ) {
		mmPrint( nGw, T_MSPP, "Warning: Video endpoint was already disabled. Do nothing.\n");
		return SUCCESS;
	}
	
    ret = mspDisableEndpoint(hd);
	if (ret != SUCCESS) {
	    mmPrintPortErr(nGw, "mspDisableEndpoint returned %d.\n", ret);
		return ret;
    } else {
		ret = WaitForSpecificEvent ( nGw, GwConfig[nGw].hCtaQueueHd, 
					MSPEVN_DISABLE_ENDPOINT_DONE, &waitEvent);
    	if (ret != 0) {
	    	mmPrintPortErr( nGw, "MSPEVN_DISABLE_ENDPOINT_DONE event failure, ret= %x, waitEvent.value=%x\n",
				        	ret, waitEvent.value);
			return FAILURE;
	   	} else
			GwConfig[nGw].vidRtpEp[index].bEnabled = FALSE;
	}
	
	return SUCCESS;
}


DWORD enableAudioEP( int nGw, MSPHD hd )
{
	DWORD ret;
	CTA_EVENT waitEvent;
	
    mmPrint(nGw,T_MSPP,"Enable the Audio EP.\n");
/*    
	if( GwConfig[nGw].VidRtpEp.bEnabled ) {
		mmPrint( nGw, T_MSPP, "Warning: Video endpoint was already enabled. Do nothing.\n");
		return SUCCESS;
	}
*/	
    ret = mspEnableEndpoint(hd);
    if (ret != SUCCESS) {
		    mmPrintPortErr( nGw, "mspEnableEndpoint returned %d.\n", ret);
    } else {
		ret = WaitForSpecificEvent ( nGw, GwConfig[nGw].hCtaQueueHd, 
					MSPEVN_ENABLE_ENDPOINT_DONE, &waitEvent);
    	if (ret != 0) {
	    	mmPrintPortErr(nGw, "MSPEVN_ENABLE_ENDPOINT_DONE event failure, ret= %x, waitEvent.value=%x,"
                   	" board error (waitEvent.size field) %x\n\n",
		        	ret, waitEvent.value, waitEvent.size);
			return ret;
    	} 
//    	else
//			GwConfig[nGw].VidRtpEp.bEnabled = TRUE;
	}
	
	return SUCCESS;
}

DWORD disableAudioEP( int nGw, MSPHD hd )
{
	DWORD ret;
	CTA_EVENT waitEvent;

	mmPrint(nGw, T_MSPP,"Disable the Audio endpoint.\n");
/*	
	if( !GwConfig[nGw].VidRtpEp.bEnabled ) {
		mmPrint( nGw, T_MSPP, "Warning: Video endpoint was already disabled. Do nothing.\n");
		return SUCCESS;
	}
*/
	
    ret = mspDisableEndpoint(hd);
	if (ret != SUCCESS) {
	    mmPrintPortErr(nGw, "mspDisableEndpoint returned %d.\n", ret);
		return ret;
    } else {
		ret = WaitForSpecificEvent ( nGw, GwConfig[nGw].hCtaQueueHd, 
					MSPEVN_DISABLE_ENDPOINT_DONE, &waitEvent);
    	if (ret != 0) {
	    	mmPrintPortErr( nGw, "MSPEVN_DISABLE_ENDPOINT_DONE event failure, ret= %x, waitEvent.value=%x\n",
				        	ret, waitEvent.value);
			return FAILURE;
	   } 
//	   else
//			GwConfig[nGw].VidRtpEp.bEnabled = FALSE;
	}
	
	return SUCCESS;
}



 

																													

