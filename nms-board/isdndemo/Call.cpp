/*****************************************************************************
 *  FILE            : Call.cpp
 *  PROJECT         : isdndemo
 *  DESCRIPTION     : implementation of Call class
 *         
 *   This class implements ISDN ACU call state machine.
 *   Two values identify object of this class: nai and connection ID
 *
 *****************************************************************************
 * Copyright 2001 NMS Communications
 *****************************************************************************/

#include <stdio.h> 
#include <stdarg.h>
#include <string.h>

#include "isdndemo.h"


//-----------------------------------------------------------------------------
//              
//                 Call::Call ()
//
//  Initialize fields of Call object. Initial state is ST_NULL
//-----------------------------------------------------------------------------
Call::Call(NAI *anai, int id)
{
    state       = ST_NULL;
    outbound    = 0;
    nai         = anai;
    channel     = 0;
    conn_id     = id;
}


//-----------------------------------------------------------------------------
//              
//                 Call::isFree ()
//
// Returns "true" if Call is free and may be used for new ISDN call
//-----------------------------------------------------------------------------
bool Call::isFree() const
{
    return state == ST_NULL;
}

//-----------------------------------------------------------------------------
//              
//                 Call::freeResources ()
//
// Invocation of this method indicates that stecified BChannel is no longer
// associated with the Call
//-----------------------------------------------------------------------------
void Call::freeResources(BChannel *ch)
{
    if (channel == ch) channel = 0;
}

//-----------------------------------------------------------------------------
//              
//                 Call::setBChannel ()
//
//  Set BChannel object for the Call based on channel number
//-----------------------------------------------------------------------------
void Call::setBChannel(int ch)
{
    BChannel *chobj = nai->getBChannel(ch);
    
    if (chobj == channel) return;   // The same BChannel object
    if (channel) channel->free();   // Free existing BChannel object

    channel = chobj;
    channel->get(this);
}

//-----------------------------------------------------------------------------
//              
//                 Call::makeCall()
// 
//  Start new ISDN call on the channel with specified number
//
//  Sends ACU_CONN_RQ message
//-----------------------------------------------------------------------------
void Call::makeCall(int b_channel)
{
    //========================================================================= 
    // Set new state and associate itself with BChannel
    //========================================================================= 
    if (state != ST_NULL) error("makeCall() in wrong state\n");

    setBChannel(b_channel);
    
    state           = ST_AWAITING_CONNECT;
    outbound        = true;

    //========================================================================= 
    // Create and send ACU_CONN_RQ
    //========================================================================= 
    char    msg_buffer[ISDN_BUFFER_DATA_LGTH]={0};
    int     length;
    char    uui[128] = "INFORMATION";
    Config *config = nai->d_channel->config;
    void *p_data = msg_buffer;

    // fill in the information 
    Acu_conn_rq_data_chani_nb = 1;

    Acu_conn_rq_data_chani_tab(0)     = b_channel;      // call on this time-slot 
    Acu_conn_rq_data_chani_tab_nai(0) = nai->number;    // and this nai 

    Acu_conn_rq_data_chani_excl = config->exclusive ? ON : OFF;
    Acu_conn_rq_auto_dial = ON;

    // copy in the phone number we are calling
    if( strlen(config->dial_string) > 0 )
    {
        strcpy(Acu_conn_rq_a_ph_num, config->dial_string);
        Acu_conn_rq_ph_num_size = strlen(config->dial_string) + 1;
    }

    // copy in our phone number
    if ( config->calling_string[0] )
    {
        strcpy(Acu_conn_rq_a_calling_nb, config->calling_string);
        Acu_conn_rq_calling_nb_size = strlen(Acu_conn_rq_a_calling_nb) + 1;
    }

    // send user to user information 
    if (config->uui_flag)
    {
      strcpy(Acu_conn_rq_a_uui, uui);
      Acu_conn_rq_uui_size = strlen(uui) + 1;
    }
    else
      Acu_conn_rq_uui_size          = 0;

    // more stuff
    Acu_conn_rq_facility_size       = 0;
    Acu_conn_rq_service             = VOICE_SERVICE;
    Acu_conn_rq_sending_complete    = ON;

     

    Acu_ext_descr_nb		= 0;
    Acu_ext_descr_lgth 		= 0;
    Acu_ext_descr_offset 	= Acu_conn_rq_start_ext_data;
    uchar *p_ext_data       = Acu_ext_descr_first_address;
    
    //
    // Generate  MLPP Precedence Level IE
    //
    if (  config->mlpp_flag )
    {
        acu_ext_precedence_level *p = ( acu_ext_precedence_level * ) p_ext_data;
            
        Acu_ext_descr_nb 	+= 1;
        Acu_ext_descr_lgth	+= sizeof( acu_ext_precedence_level );	
        Acu_ext_lgth	     = sizeof( acu_ext_precedence_level );
        Acu_ext_id           = ACU_EXT_PRECEDENCE_LEVEL;
        p_ext_data		 	+= sizeof( acu_ext_precedence_level );
            
            
            
        p->level        = 3;
        p->lfb          = 2;
        p->change       = 0;
        p->coding_std   = 0;
        p->domain       = 0x123456;
        p->net_id       = 0x0789;
    }
    
    length = Acu_conn_rq_total_size + Acu_ext_descr_lgth;

    nai->d_channel->sendIsdnMessage( 
        nai, ACU_CONN_RQ, conn_id, msg_buffer, length
    );

}

//-----------------------------------------------------------------------------
//              
//                 Call::hangUp()
//
//  Initiate hang-up by sending ACU_CLEAR_RQ
//-----------------------------------------------------------------------------
void Call::hangUp()
{
    //========================================================================= 
    // Set new state and inform BChannel
    //========================================================================= 
    if (state != ST_ACTIVE)  error("hangUpCall() in wrong state\n");

    state = ST_AWAITING_CLEARANCE;

    if (channel) channel->stopCall();

    //========================================================================= 
    // Create and send ACU_CLEAR_RQ
    //========================================================================= 
    char msg_buffer[ISDN_BUFFER_DATA_LGTH]= {0};

    void *p_data = msg_buffer;

    Acu_clear_rq_priority   = ACU_PHIGH;
    Acu_clear_rq_cause      = nai->d_channel->config->clear_cause;

    nai->d_channel->sendIsdnMessage( 
        nai, ACU_CLEAR_RQ, conn_id, msg_buffer, Acu_clear_rq_total_size
    );

}

//-----------------------------------------------------------------------------
//              
//                 BChannel::unexpIsdnMessage()
//
// Report unexpected ISDN message
//-----------------------------------------------------------------------------
void Call::unexpIsdnMessage(char msg)
{
    const char *st = "UNKNOWN";
    switch (state) 
    {
    case ST_NULL:               st = "ST_NULL";               break;
    case ST_ACTIVE:             st = "ST_ACTIVE";             break;
    case ST_AWAITING_CONNECT:   st = "ST_AWAITING_CONNECT";   break; 
    case ST_AWAITING_CLEARANCE: st = "ST_AWAITING_CLEARANCE"; break;
    }

    printf("Unexpected %s in %s state\n", getACU(msg), st);

   //========================================================================= 
    // Create and send ACU_CLEAR_RQ
    //========================================================================= 
    char msg_buffer[ISDN_BUFFER_DATA_LGTH]= {0};

    void *p_data = msg_buffer;

    Acu_clear_rq_priority   = ACU_PHIGH;
    Acu_clear_rq_cause      = 0;         // 0 => normal clearing

    nai->d_channel->sendIsdnMessage( 
        nai, ACU_CLEAR_RQ, conn_id, msg_buffer, Acu_clear_rq_total_size
    );    
}


//-----------------------------------------------------------------------------
//              
//                 Call::processIsdnMessage()
//
//  Process ISDN following ACU messages:
//  - ACU_CONN_IN 
//  - ACU_CONN_CO 
//  - ACU_CLEAR_IN 
//  - ACU_CLEAR_CO 
//  - ACU_CALL_PROC_IN
//  - ACU_ALERT_IN
//-----------------------------------------------------------------------------

void Call::processIsdnMessage(  ISDN_PACKET *isdnpkt )
{
    char msg_buffer[ISDN_BUFFER_DATA_LGTH]={0}; // For new messages
    void *p_data = isdnpkt->data;
    Config *config = nai->d_channel->config;
  
    switch( isdnpkt->message.code )
    {
    //========================================================================= 
    // Message          : ACU_CALL_PROC_IN
    // Supported state  : not specified 
    // Next state       : the same
    // Send message     : no
    //========================================================================= 
    case ACU_CALL_PROC_IN:
        
        if ( Acu_call_proc_in_data_chani_nb != 0 ) setBChannel(Acu_call_proc_in_data_chani);
        break;

    //========================================================================= 
    // Message          : ACU_ALERT_IN
    // Supported state  : not specified 
    // Next state       : the same
    // Send message     : no
    //========================================================================= 
    case ACU_ALERT_IN:

        if ( Acu_alert_in_data_chani_nb != 0 ) setBChannel(Acu_alert_in_data_chani);
        break;

    //========================================================================= 
    // Message          : ACU_CONN_IN
    // Supported state  : ST_NULL 
    // Next state       : ST_AWAITING_CONNECT
    // Send message     : ACU_CONN_RS
    //========================================================================= 
    case ACU_CONN_IN:

        if (state != ST_NULL)
        {
            unexpIsdnMessage(ACU_CONN_IN);
            return;
        }

        if ( Acu_conn_in_data_chani_nb != 0 ) setBChannel(Acu_conn_in_data_chani);

        outbound = false;
        state    = ST_AWAITING_CONNECT;

        //=====================================================================
        // Send ACU_CONN_RQ
        //===================================================================== 
        p_data = msg_buffer;

        //fill in some stuff 
        Acu_conn_rs_priority      = ACU_PLOW;             // priority
        Acu_conn_rs_service       = VOICE_SERVICE;        // type of service 
        Acu_conn_rs_data_chani    = channel->getNumber(); // B-channel number
        Acu_conn_rs_data_chani_nb = 1;                    // only one B-channel 

        // EXAMPLE OF SENDING A TRANSPARENT IE
        if (config->Transparent)
        {
            unsigned char buf[10] =
            {
                0x9f, // non-locking shif to codeset 7 
                0x01, // user defined IE 
                0x07, // length of the IE 
                0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37
            };
            Acu_conn_rs_tsp_ie_list_size = sizeof(buf);
            memcpy(Acu_conn_rs_a_tsp_ie_list, buf, sizeof(buf));
        }


        nai->d_channel->sendIsdnMessage( 
            nai, ACU_CONN_RS, conn_id, msg_buffer, Acu_conn_rs_total_size 
        );

        break;

    //========================================================================= 
    // Message          : ACU_CONN_CO
    // Supported state  : ST_AWAITING_CONNECT
    // Next state       : ST_ACTIVE
    // Send message     : no
    //========================================================================= 
    case ACU_CONN_CO:

        switch( state )
        {
        //*********************************************************************
        case ST_AWAITING_CONNECT:

            state = ST_ACTIVE;
            
            if ( Acu_conn_co_data_chani_nb != 0 ) setBChannel(Acu_conn_co_data_chani);

            if (channel) channel->startCall();
            break;

        //*********************************************************************
        default:
            unexpIsdnMessage(ACU_CONN_CO);
        }

        break;

    //========================================================================= 
    // Message          : ACU_CLEAR_IN
    // Supported state  : ST_ACTIVE, ST_AWAITING_CLEARANCE, ST_AWAITING_CONNECT
    // Next state       : ST_AWAITING_CLEARANCE
    // Send message     : ACU_CLEAR_RS
    //========================================================================= 
    case ACU_CLEAR_IN:

        switch ( state )
        {
        //*********************************************************************
        case ST_AWAITING_CLEARANCE:
//          Clear collision detected
            break;

        //*********************************************************************
        case ST_ACTIVE:
            if (channel) channel->stopCall();

        //*********************************************************************
        case ST_AWAITING_CONNECT:

            state = ST_AWAITING_CLEARANCE;

            p_data =  msg_buffer;
            Acu_clear_rs_priority = ACU_PHIGH;

            nai->d_channel->sendIsdnMessage( 
                nai, ACU_CLEAR_RS, conn_id, msg_buffer, Acu_clear_rs_total_size);
            break;

        //*********************************************************************
        default:
            unexpIsdnMessage(ACU_CLEAR_IN);
        }
        break;

    //========================================================================= 
    // Message          : ACU_CLEAR_CO
    // Supported state  : All but ST_NULL
    // Next state       : ST_NULL
    // Send message     : no
    //========================================================================= 
    case ACU_CLEAR_CO:

        switch( state )
        {
        //*********************************************************************
        case ST_NULL:
            unexpIsdnMessage(ACU_CLEAR_CO);
            break;
        

        //*********************************************************************
        case ST_ACTIVE:
            if (channel) channel->stopCall();
            break;

        //*********************************************************************
        case ST_AWAITING_CONNECT:
            nai->d_channel->statistics(DChannel::FAIL);
            break;
        }

        if (channel) 
        {
            channel->free();
            channel = 0;
        }

        outbound = false;
        state  = ST_NULL;

        break;
    } //switch
}



