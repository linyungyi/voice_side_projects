/*****************************************************************************
 *  FILE            : BChannel.cpp
 *  PROJECT         : isdndemo
 *  DESCRIPTION     : implementation of BChannel class
 *         
 *
 *****************************************************************************
 * Copyright 2001 NMS Communications
 *****************************************************************************/

#include <stdio.h> 
#include <stdarg.h>
#include <string.h>

#include "isdndemo.h"


char const *VOICE_FILE  = "welcome.vce";


//-----------------------------------------------------------------------------
//              
//                 BChannel::BChannel ()
//
//   1. Open services on Natural Access context (created by Context constructor)
//   2. Start "nocc" protocol
//-----------------------------------------------------------------------------
BChannel::BChannel( NAI *anai, int channel, bool aoutbound_flag):
    timer( ctahd ) 
{
    DWORD ret;

    //=========================================================================
    //
    // Object fields initialization
    //
    //=========================================================================
    config      = anai->d_channel->config;
    nai         = anai;
    number      = channel; 
    call        = 0;
    state       = ST_FREE;
    playing     = false;
    outbound_flag = aoutbound_flag;
    vcehd       = 0;
    calls_in    = 0;
    calls_out   = 0;
    statistics  = 0;

    //=========================================================================
    //
    // Open services
    //
    //=========================================================================
    CTA_SERVICE_DESC    svc_desc[2]=
    {
        { {"ADI" , "ADIMGR" }, {0}, {0} },
        { {"VCE" , "VCEMGR" }, {0}, {0} },
    };
    
    for (int i=0; i<2; i++)
    {
        svc_desc[i].mvipaddr.board    = nai->board->number;
        svc_desc[i].mvipaddr.stream   = nai->board->getMvipStream();
        svc_desc[i].mvipaddr.timeslot = nai->board->getMvipSlot(nai->trunk, number);
        svc_desc[i].mvipaddr.mode     = config->voice ? ADI_VOICE_DUPLEX : 0;
    }   

    ctaOpenServices( ctahd, svc_desc, 2 );
    ret = waitForEvent(CTAEVN_OPEN_SERVICES_DONE);
    if (ret!= CTA_REASON_FINISHED) error_cta( ret,"ctaOpenService()");

    //=========================================================================
    //
    // Start "nocc" protocol
    //
    //=========================================================================
    if (config->voice)
    {

        ADI_START_PARMS stparms;
        ctaGetParms(ctahd, ADI_START_PARMID, &stparms, sizeof(stparms));
        stparms.callctl.mediamask = 0; // Some boards has problems with resoures

        adiStartProtocol(ctahd, "nocc", NULL, &stparms);
        ret = waitForEvent( ADIEVN_STARTPROTOCOL_DONE );

        if (ret != CTA_REASON_FINISHED) 
            error_cta(ret,"adiStartProtocol(): board %d trunk %d B-channel %d",
                nai->board->number,
                nai->trunk, number
        );
    }

}

//-----------------------------------------------------------------------------
//              
//                 BChannel::start ()
//
//  This method starts placing outbound calls if it is required by outbound_flag
//-----------------------------------------------------------------------------
void BChannel::start()
{
    if (outbound_flag)
    {
        Call *c;
        c = nai->getFreeCall();
        if (c) c->makeCall(number);

    }

}
//-----------------------------------------------------------------------------
//              
//                 BChannel::get ()
//
// Invocation of this method indicates association of this BChannel with Call.
//-----------------------------------------------------------------------------
void BChannel::get(Call *acall)
{
    if (call) 
    {
        printInfo("Glare detected\n");
        nai->d_channel->statistics(DChannel::GLARE);
        call->freeResources(this);
    }

    call  = acall;
    state = ST_USED;

    timer.stop();
}

//-----------------------------------------------------------------------------
//              
//                 BChannel::free ()
//
// Invocation of this method indicates that BChannel is no longer associated
// with any Call.
//-----------------------------------------------------------------------------
void BChannel::free()
{
    call  = 0;
    state = ST_FREE;

    if ( outbound_flag ) timer.start( config->intercall );

}

//-----------------------------------------------------------------------------
//              
//                 BChannel::startCall()
//
//  Called when call enter connected state
//-----------------------------------------------------------------------------
void BChannel::startCall()
{
    state = ST_CALL;

    if ( config->voice  &&
           vceOpenFile(ctahd, VOICE_FILE, VCE_FILETYPE_FLAT, VCE_PLAY_ONLY,
                ADI_ENCODE_NMS_24, &vcehd) == SUCCESS &&
           vcePlayMessage( vcehd, 0, NULL ) == SUCCESS
    ) playing = true;

    if (call->outbound)
    {     
        statistics |= 0x02;
        timer.start( config->hold_time );
        printInfo("Connect outbound call %d\n", ++calls_out);
        nai->d_channel->statistics(DChannel::OUTBOUND);
    }
    else
    {     
        statistics |= 0x01;
        printInfo("Connect inbound call %d\n", ++calls_in);
        nai->d_channel->statistics(DChannel::INBOUND);
    }

}

//-----------------------------------------------------------------------------
//              
//                 BChannel::stopCall()
//
//  Called when call leave connected state
//-----------------------------------------------------------------------------
void BChannel::stopCall()
{
    if ( playing ) vceStop( ctahd );

    timer.stop();

    printInfo("Disconnect call\n");

}


//-----------------------------------------------------------------------------
//              
//                 BChannel::processEvent ()
//
//  Process following events:
//
//     VCEEVN_PLAY_DONE
//     ADIEVN_BOARD_ERROR
//     ADIEVN_TIMER_DONE   
//  
//-----------------------------------------------------------------------------
void BChannel::processEvent( CTA_EVENT *event )
{
  switch (event->id)
  {
    //==========================================================================
    // Message  : VCEEVN_PLAY_DONE
    // Action   : do some clearance
    //==========================================================================
    case VCEEVN_PLAY_DONE:
        playing = false;
        vceClose( vcehd );
        vcehd = 0;
        break;

    //==========================================================================
    // Message  : ADIEVN_BOARD_ERROR
    // Action   : inform error
    //==========================================================================
    case ADIEVN_BOARD_ERROR:
      error_cta(event->value, "ADIEVN_BOARD_ERROR");
      break;

    //==========================================================================
    // Message  : ADIEVN_TIMER_DONE
    // Action   : pass event value to Timer object
    //==========================================================================
    case ADIEVN_TIMER_DONE:
      if ( timer.event(event->value) )
        switch ( state )
        {
        case ST_CALL:   // Hang up existing call
            call->hangUp(); 
            break;      
        case ST_FREE:   // make new call
            {
                Call *c = nai->getFreeCall();
                c->makeCall(number);
            }
        }

      break;


    //==========================================================================
    // Message  : others
    // Action   : do nothing
    //==========================================================================
    default:
      printInfo("Unexpected Natural Access event 0x%08x value 0x%08x\n",
              event->id, event->value);

      break;
  }

  return;
}


//-----------------------------------------------------------------------------
//              
//                 BChannel::printInfo
//
//  Print formatted string + BChannel coordinates
//-----------------------------------------------------------------------------
void BChannel::printInfo(const char *format, ...)
{
    debug_printf(V_CALL, "(D%-2d %02d:%02d) Channel %2d : ",
        nai->d_channel->number,
        nai->number,
        call ? call->conn_id : -1,
        number
    );

    va_list args;
    va_start( args, format );
    debug_vprintf(V_CALL, format,args);
    va_end( args );
}


//-----------------------------------------------------------------------------
//              
//                 BChannel::getStatistics()
//
//  Returns statistics value and clear it.
//-----------------------------------------------------------------------------
int BChannel::getStatistics()
{
    int ret = statistics;
    statistics = 0;
    return ret;
}
