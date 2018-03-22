/*****************************************************************************
 *  FILE            : DChannel.cpp
 *  PROJECT         : isdndemo
 *  DESCRIPTION     : implementation of DChannel class
 *         
 *   DChannel receives ISDN message. I passes this message to apropriate
 *   bCobtext basing on NAI and  B-channel number or connection ID. 
 *    
 *
 *****************************************************************************
 * Copyright 2001 NMS Communications
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "isdndemo.h"


//-----------------------------------------------------------------------------
//              
//                 DChannel::DChannel ()
//
//   1. Open services on Natural Access context (created by Context constructor)
//   2. Start ISDN protocol
//-----------------------------------------------------------------------------
DChannel::DChannel( Config *cfg )
{
    static int anumber = 0;
    int i;
    DWORD ret;  
    //=========================================================================
    //
    // Object fields initialization
    //
    //=========================================================================
    config          = cfg;
    group           = config->group;
    board           = new Board( config->board );
    number          = anumber++;

    open_ports = open_outbound_ports =0;

    for( i=0; i < MAX_NAIS; i++ ) nai[i] = 0;

    //=========================================================================
    //
    // Open log file here
    //
    //=========================================================================
    logfile = 0;
    for (i=0; i<4; i++) stat[i] = stat_old[i] = 0;
    if (config->log) 
    {
        char name[256];
        
        if (group != -1) 
            sprintf(name, "isdndemo_b%dg%d.log",board->number, group);
        else
            sprintf(name, "isdndemo_b%da%d.log",board->number, config->nai);

        logfile = fopen(name,"w");
        if (!logfile) 
            error("can't open log file \"%s\":%s",name, strerror(errno));

    } 

    //=========================================================================
    //
    // Open services
    //
    //=========================================================================
    CTA_SERVICE_DESC    svc_desc[]=
    {
        { {"ADI" ,  "ADIMGR" }, {0}, {0} },
        { {"ISDN" , "ADIMGR" }, {0}, {0} },
    };
    
    for (i=0; i < 2; i++)
    {
        svc_desc[i].mvipaddr.board    = board->number;
        svc_desc[i].mvipaddr.mode     = 0;
    }   

    ctaOpenServices( ctahd, svc_desc, 2 );

    ret = waitForEvent( CTAEVN_OPEN_SERVICES_DONE );
    if (ret!= CTA_REASON_FINISHED) error_cta( ret, "ctaOpenService()");
    

    //========================================================================  
    //
    // Create Board object for D-channel board
    //
    //========================================================================  

    logprintf(V_CONFIG, "Board number .... %d\n", board->number);
    const char *type;
    switch (board->type)
    {
    case Board::PRI_T1 : type = "PRI T1"; break;
    case Board::PRI_E1 : type = "PRI E1"; break;
    case Board::BRI    : type = "BRI";    break;
    default:             type = "Unknown";
    }
    logprintf(V_CONFIG, "Board type ...... %s\n", type);
    

    int nai_D;

    //========================================================================  
    // Read NFAS group information
    //========================================================================  
    if ( group != -1 ) 
    {
        int trunks = board->getTrunks();

        nai_D = -1; // NAI for NFAS obtained from configuration
        
        for ( trunk=0; trunk < trunks; trunk++ ) 
        if (group == board->getKeyword("NetworkInterface.T1E1[%d].ISDN.NFASGroup",trunk))
        {
            int anai[MAX_NAIS];
            int aboard[MAX_NAIS];
            int atrunk[MAX_NAIS];
            int i, nai_count;
            int backup = board->getKeyword("NetworkInterface.T1E1[%d].ISDN.D_Channel_Backup_Trunk",trunk);


            logprintf(V_CONFIG,"NFAS group ...... %d\n", group);

            //
            // Read NFAS configuration 
            //

			nai_count = board->getKeyword("NetworkInterface.T1E1[%d].ISDN.NFAS_Member.Count",trunk);

            for ( i=0; i< nai_count; i++) 
            {   
                anai[i]   = board->getKeyword("NetworkInterface.T1E1[%d].ISDN.NFAS_Member[%d].NAI", trunk, i);
                aboard[i] = board->getKeyword("NetworkInterface.T1E1[%d].ISDN.NFAS_Member[%d].Board", trunk,i);
                atrunk[i] = board->getKeyword("NetworkInterface.T1E1[%d].ISDN.NFAS_Member[%d].Trunk", trunk,i);

                if (anai[i] == -1 || aboard[i] == -1 || atrunk[i] == -1) break; //this is not valid record  

                // Look if it's a D-channel NAI
                if (board->number == aboard[i] && trunk == atrunk[i]) nai_D = anai[i];
            }
            
            nai_count = i;

            //
            // Create NFAS contextes 
            //
            for (i=0; i<nai_count; i++) 
            {
                bool isdn  = false ;
                char *note ="";

                if (aboard[i] == board->number)
                {
                    if (atrunk[i] == trunk)  { isdn = true; note = "D-channel";}
                    if (atrunk[i] == backup) { isdn = true; note = "backup D-channel";}
                }
                
                logprintf(V_CONFIG,"Group member .... %d: nai =%3d, board =%3d, trunk =%2d  %s\n",
                    i, anai[i], aboard[i], atrunk[i], note);

                nai[ anai[i] ] = new NAI(anai[i], this, aboard[i], atrunk[i], isdn );

            }

            startProtocol(nai_D);

            break; 

        } // for and if 

        if (trunk == trunks) 
            error("group %d not found on board %d\n", group, board);

    } //if

    //========================================================================  
    // Single trunk configuration
    //========================================================================  
    else
    {
        trunk = nai_D = config->nai;
        nai[ nai_D ] = new NAI(nai_D, this, board->number, nai_D, true );
        startProtocol(nai_D);

        logprintf(V_CONFIG,"NAI ............. %d\n", nai_D);
    }   

    logprintf(V_CONFIG,"Inbound  ports .. %d\n", open_ports - open_outbound_ports);
    logprintf(V_CONFIG,"Outbound ports .. %d\n", open_outbound_ports);
    logprintf(V_CONFIG,"Intercall delay . %d ms\n", config->intercall);
    logprintf(V_CONFIG,"Out call length . %d ms\n", config->hold_time);
    logprintf(V_CONFIG,"\n");


    time(&starttime);
    adiStartTimer(ctahd, config->logtime*1000, 1);     


}


extern struct {  int protocol;  int country; char *name; } PROTOCOLS[];

//--------------------------------------------------------------------------
//
//             DChannel::startProtocol()
//
//--------------------------------------------------------------------------
void DChannel::startProtocol(int anai)
{

    int i;
    int country;
    //========================================================================
    //
    // Get protocol information
    //
    //========================================================================  
    if (config->protocol == 0)
        switch (board->type) 
        {
        case Board::PRI_T1: config->protocol = ISDN_OPERATOR_ATT_4ESS; break;
        case Board::PRI_E1: config->protocol = ISDN_OPERATOR_ETSI; break;
        case Board::BRI   : config->protocol = ISDN_OPERATOR_ETSI; break;
        }

    for(i=0;PROTOCOLS[i].protocol && PROTOCOLS[i].protocol!=config->protocol; i++);
    
    if (PROTOCOLS[i].protocol)
    {
        country = PROTOCOLS[i].country;
        logprintf(V_CONFIG,"Protocol ........ %s\n", PROTOCOLS[i].name );
        logprintf(V_CONFIG,"Side ............ %s\n", config->network?"NT":"TE");
    }
    else 
        error("Unknown protocol number %d", config->protocol);

    //=========================================================================
    //
    // Start ISDN protocol
    //
    //=========================================================================

    ISDN_PROTOCOL_PARMS_Q931CC cc_parms={0};

    // if we are a network, our partner is the terminal 
    DWORD partner = (config->network) ? EQUIPMENT_TE : EQUIPMENT_NT;

    if (config->Q931)         cc_parms.acu_behaviour |= ACU_SEND_Q931_BUFFER;
    if (config->Transparent)  cc_parms.ns_behaviour  |= NS_IE_RELAY_BEHAVIOUR;
    if (group >=0)            cc_parms.nfas_group    = group;

    cc_parms.acu_behaviour |= ACU_SEND_D_CHANNEL_STATUS_CHANGE;
    cc_parms.ns_behaviour  |= NS_ACCEPT_UNKNOWN_FAC_IE;

    cc_parms.size = sizeof(ISDN_PROTOCOL_PARMS_Q931CC);
    cc_parms.rate = ISDN_RATE_64K;
    cc_parms.services_list[0] = VOICE_SERVICE;
    cc_parms.services_list[1] = FAX_SERVICE;
    cc_parms.services_list[2] = FAX_4_SERVICE;
    cc_parms.services_list[3] = DATA_SERVICE;
    cc_parms.services_list[4] = DATA_GCI_SERVICE;
    cc_parms.services_list[5] = DATA_56KBS_SERVICE;
    cc_parms.services_list[6] = DATA_TRANS_SERVICE;
    cc_parms.services_list[7] = MODEM_SERVICE;
    cc_parms.services_list[8] = AUDIO_7_SERVICE;
    cc_parms.services_list[9] = X25_SERVICE;
    cc_parms.services_list[10] = X25_PACKET_SERVICE;
    cc_parms.services_list[11] = VOICE_SERVICE;
    cc_parms.services_list[12] = VOICE_GCI_SERVICE;
    cc_parms.services_list[13] = VOICE_TRANS_SERVICE;
    cc_parms.services_list[14] = V110_SERVICE;
    cc_parms.services_list[15] = V120_SERVICE;
    cc_parms.services_list[16] = VIDEO_SERVICE;
    cc_parms.services_list[17] = TDD_SERVICE;
    cc_parms.services_list[18] = NO_SERVICE;
    
    cc_parms.aoc_s_presubscribed = ON;
    cc_parms.aoc_d_presubscribed = ON;
    cc_parms.aoc_e_presubscribed = ON;
    
    isdnStartProtocol(ctahd, ISDN_PROTOCOL_Q931CC, config->protocol, 
                      country, partner, anai, &cc_parms);
    DWORD ret = waitForEvent( ISDNEVN_START_PROTOCOL );
    if (ret!= SUCCESS) 
        error_cta(ret, "isdnStartProtocol(): board %d nai %d", board->number, anai);
}


//-----------------------------------------------------------------------------
//              
//                 DChannel::getNewPortMode()
//
//-----------------------------------------------------------------------------
bool DChannel::getNewPortMode()
{
       bool outbound =false;
       

       if (open_ports >= config->first_outbound_channel && 
           open_outbound_ports < config->number_outbound_ports)
       {        
            outbound = true;
            open_outbound_ports++;
       }
       open_ports++;

       return outbound;
}



//-----------------------------------------------------------------------------
//              
//                 DChannel::processEvent ()
//
//  This function process following events:
//
//    ISDNEVN_ERROR
//    ISDNEVN_SEND_MESSAGE
//    ISDNEVN_RCV_MESSAGE       ->  ISDNEVN_RCV_MESSAGE (to BChannel)
//
//-----------------------------------------------------------------------------
void DChannel::processEvent( CTA_EVENT *event )
{
  ISDN_PACKET *isdnpkt;
  void *p_data;

  switch (event->id)
  {

    //==========================================================================
    // Message  : ADIEVN_TIMER_DONE
    // Action   : print statistics
    //==========================================================================
    case ADIEVN_TIMER_DONE:
        {
            char *names[] = {
                "Inbound calls",
                "Outbound calls",
                "Failures detected",
                "Glares detected"
            };
            
            time_t t;
            int i;
            time(&t);
            //-----------------------------------------------------------------
            // Print channel information and time
            //-----------------------------------------------------------------
            logprintf(V_STAT,"------------------------------------------------------------------------------\n");
            debug_printf(V_STAT,  "%20s : %d\n",   "D-channel #", number);
            logprintf(V_STAT,"%20s : %s",   "Current time",ctime(&t));

            logprintf(V_STAT,"%20s : %-6d +%d\n\n", "Test time",
                     (int) difftime(t,starttime), config->logtime);

            //-----------------------------------------------------------------
            // Print general calls and failures statistics
            //-----------------------------------------------------------------
            for(i=0; i<4; i++)
            {
                logprintf(V_STAT,"%20s : %-6d +%d\n", 
                    names[i],
                    stat[i],
                    stat[i]-stat_old[i]
                );

                stat_old[i] = stat[i];
            }
            logprintf(V_STAT,"\n");

            //-----------------------------------------------------------------
            // Print NAI based statistics
            //-----------------------------------------------------------------

            for(i=0;i<MAX_NAIS;i++) if (nai[i])
            {
                logprintf(V_STAT,"              NAI %2d : ",i);
                int ch = nai[i]->board->getMaxChannel();
                for(int j=1; j<= ch; j++) 
                {   
                    BChannel *b = nai[i]->getBChannel(j);
                    char icons[]="-iob";
                    char c = b ? icons[b->getStatistics() & 3] : '*';
                    logprintf(V_STAT,"%c",c);
                    if ( j%5 == 0 )  logprintf(V_STAT,"  ");
                }
                logprintf(V_STAT,"\n");
            }

            debug_printf(V_STAT,"------------------------------------------------------------------------------\n");
            adiStartTimer(ctahd, config->logtime*1000, 1);

        }
        break;


    //==========================================================================
    // Message  : ISDNEVN_ERROR
    // Action   : print error message
    //==========================================================================
    case ISDNEVN_ERROR:
      //printf(MTYPE_ERR,"ISDNEVN_ERROR (0x%08x)\n", event->id);
      break;

    //==========================================================================
    // Message  : ISDNEVN_SEND_MESSAGE
    // Action   : check return status
    //==========================================================================
    case ISDNEVN_SEND_MESSAGE:
      if (event->value != SUCCESS)
        error_cta(event->value,"ISDNEVN_SEND_MESSAGE");
      break;

    //==========================================================================
    // Message  : ISDNEVN_RCV_MESSAGE
    // Action   : process message by coresponding b-channl
    //==========================================================================
    case ISDNEVN_RCV_MESSAGE:
      if( event->value != SUCCESS )
      {
        error_cta(event->value ,"ISDNEVN_RCV_MESSAGE");
        break;
      }

      isdnpkt = (ISDN_PACKET *)event->buffer;
      p_data  = isdnpkt->data;

      // process the information basing on the sender, 
      switch (isdnpkt->message.from_ent)
      {
        case ENT_CC:            /* call control */
            {
                Call     *call  = 0;
                const NAI *anai = nai[ isdnpkt->message.nai];

                //------------------------------------------------------------
                // Find who will process message:
                //------------------------------------------------------------
                switch (isdnpkt->message.code)
                {

                //************************************************************
                // Process event not associated with any call
                //************************************************************
                case ACU_RESTART_IN:
                case ACU_D_CHANNEL_STATUS_IN:
                case ACU_SERVICE_IN:
                    break;

                //************************************************************
                // Find B-channel for Call associated events
                //************************************************************

                default:         // Look by connection ID
                    if ( anai )
                        call = anai->getCall(isdnpkt->message.add.conn_id);
                    break;
                };

                printIsdnMessage( &(isdnpkt->message), &(isdnpkt->data), false );
            
                if ( call ) call->processIsdnMessage(isdnpkt);


            }


            break;

        case ENT_MPH:           /* physical layer management */
        case ENT_SM:            /* system management */
        default:    
            break;
      }

      isdnReleaseBuffer(ctahd, event->buffer);
      break;

    //==========================================================================
    // Message  : all others
    // Action   : ignore
    //==========================================================================
    default:
//    selfPrint(MTYPE_CIN,"Unknown event(0x%08x), value = 0x%08x\n",    
 //            event->id, event->value);

      break;
  }

  return;
}

//-----------------------------------------------------------------------------
//              
//          DChannel::sendIsdnMessage
//
//  Prepares and sends ISDN message.
//-----------------------------------------------------------------------------
int DChannel::sendIsdnMessage(const NAI *anai, char code, int conn_id, void *msg_buffer, int length)
{
    ISDN_MESSAGE imsg   = {0};
    static int   userid  = 0;

    imsg.from_ent    = ENT_APPLI;
    imsg.to_ent      = ENT_CC;
    imsg.to_sapi     = ACU_SAPI;
    imsg.code        = code;
    imsg.nai         = (BYTE) anai->number;
    imsg.add.conn_id = conn_id;
    imsg.data_size   = length;
    imsg.userid      = userid++;

    if (group >=0) imsg.nfas_group  = (WORD) group; 
    
    printIsdnMessage( &imsg, msg_buffer, true );

    return isdnSendMessage(ctahd, &imsg, msg_buffer, length);
}


//-----------------------------------------------------------------------------
//              
//          DChannel::printIsdnMessage
//
//-----------------------------------------------------------------------------
void DChannel::printIsdnMessage( ISDN_MESSAGE *imsg, void *p_data, bool out )
{
    void printExtParameters( void * p_data );
    void print931Buffer( void * p_data, unsigned size  );
    

    debug_printf(V_ACU, "(D%-2d %02d:%02d) %s %s ", 
            number,
            imsg->nai,
            (int)imsg->add.conn_id,
            out ? "<--" : "-->",
            getACU( imsg->code )
    );

    switch ( imsg->code )
    {
    case ACU_RESTART_IN:
        debug_printf(V_ACU, "(nai=%2d, ch=%2d)\n",
            Acu_restart_int_id,
            Acu_restart_b_chan
        );
        break;

    case ACU_D_CHANNEL_STATUS_IN:
        debug_printf(V_ACU, "(Status = %s)\n",
            Acu_d_channel_state ? "UP" : "DOWN"
        );
        break;

    case ACU_SERVICE_IN:
        debug_printf(V_ACU, "( %s, nai=%2d, ch=%2d, %s )\n",
            Acu_service_pref==I_PREF_INTERFACE ? "interface" : "b-channel",
            Acu_service_int_id, Acu_service_b_chan,
            Acu_service_status==I_B_CHAN_IN_SERVICE ? "IS" :
            Acu_service_status==I_B_CHAN_OUT_OF_SERVICE ? "OOS" :"?"  
        );
        break;

    case ACU_SETUP_REPORT_IN:
    case ACU_CONN_IN: 
        debug_printf(V_ACU, "( ch=%d, '%s' -> '%s' )\n",
            Acu_conn_in_data_chani,
            (Acu_conn_in_calling_nb_size>0) ? Acu_conn_in_a_calling_nb : "",
            (Acu_conn_in_called_nb_size>0)  ? Acu_conn_in_a_called_nb  : ""
        );
        printExtParameters ( p_data );
        print931Buffer ( Acu_conn_in_a_q931, Acu_conn_in_q931_size );

        break;

    case ACU_CONN_RQ: 
        debug_printf(V_ACU, "( ch=%d, '%s' -> '%s' )\n",
            Acu_conn_rq_data_chani,
            (Acu_conn_rq_calling_nb_size>0) ? Acu_conn_rq_a_calling_nb : "",
            (Acu_conn_rq_called_nb_size>0)  ? Acu_conn_rq_a_called_nb  : ""
        );
        printExtParameters ( p_data );
        print931Buffer ( Acu_conn_rq_a_tsp_ie_list, Acu_conn_rq_tsp_ie_list_size );
        
        break;

    case ACU_ALERT_IN: 
        debug_printf(V_ACU, "( ch=%d )\n",
            Acu_alert_in_data_chani
        );
        printExtParameters ( p_data );
        print931Buffer ( Acu_alert_in_a_q931, Acu_alert_in_q931_size );
        break;

    case ACU_CALL_PROC_IN: 
        debug_printf(V_ACU, "( ch=%d )\n",
            Acu_call_proc_in_data_chani
        );
        printExtParameters ( p_data );
        print931Buffer ( Acu_call_proc_in_a_q931, Acu_call_proc_in_q931_size );
        break;


    case ACU_CLEAR_RQ:
        debug_printf(V_ACU, "( %d-%s )\n",
            Acu_clear_rq_cause,
            getCAU( Acu_clear_rq_cause )
        );
        break;

    case ACU_CLEAR_IN:
        debug_printf(V_ACU, "( %s , %d-%s )\n",
            getACURC( Acu_clear_in_ret_code ),
            Acu_clear_in_network_cause,
            getCAU( Acu_clear_in_network_cause )
        );
        printExtParameters ( p_data );
        print931Buffer ( Acu_clear_in_a_q931, Acu_clear_in_q931_size );        
        break;

    case ACU_CLEAR_CO:
        debug_printf(V_ACU, "( %s , %d-%s )\n",
            getACURC(Acu_clear_co_ret_code),
            Acu_clear_co_network_cause,
            getCAU(Acu_clear_co_network_cause)
        );
        printExtParameters ( p_data );
        print931Buffer ( Acu_clear_co_a_q931, Acu_clear_co_q931_size );
        break;

    case ACU_ERR_IN:
        debug_printf(V_ACU, "%s ('%c')\n", 
        getACUERR( Acu_err_in_cause ), Acu_err_in_cause );
        break;

    case ACU_FACILITY_IN:
        debug_printf(V_ACU, "\n");
        print931Buffer ( Acu_facility_a_q931, Acu_facility_q931_size );
        break;

    default:
        debug_printf(V_ACU, "\n");
        break;

    }
}

//-----------------------------------------------------------------------------
//   Print ACU Transparent IE or Q.931 buffer     
//-----------------------------------------------------------------------------
void print931Buffer( void * p_data, unsigned size  )
{
    if ( size > 0 )
    {
        uchar *data = static_cast<uchar *>(p_data);

        debug_printf(V_ACU,"\t<Q.931 IE Buffer>:");
        for ( unsigned i = 0; i < size; i++ )
        {
            if ( ( i & 0x0F ) == 0 )     debug_printf(V_ACU,"\n\t\t");
            debug_printf(V_ACU,"%02X ", data[i]);
            if ( ( i & 0x03 ) == 0x03 )  debug_printf(V_ACU,"   ");            
        } 
        debug_printf(V_ACU,"\n\n");
    }
}



//-----------------------------------------------------------------------------
//   Print ACU extended parameters     
//-----------------------------------------------------------------------------
void printExtParameters( void * p_data )
{
    uchar *p_ext_data = Acu_ext_descr_first_address;
    
    for ( int i = 0; i < Acu_ext_descr_nb; i++ )
    {
        // Process according to parameter type
        switch ( Acu_ext_id )
        {
        //--------------------------------------------------------------------------------
        //          Network  Specific  Facility
        //--------------------------------------------------------------------------------
        case ACU_EXT_SPF_FAC_IE:
            {
                // Verify parameter structure integrity:
                acu_ext_spf_fac_ie *p = ( acu_ext_spf_fac_ie * ) p_ext_data;

                debug_printf(V_ACU, "\t<EXT>: NSF: NI type=0x%x, NI='%.*s', "
                    "act=%d, srv=%d, cod=%d, param=0x%02X\n", 
                    p->net_id_type,
                    p->net_id_lgth,
                    p->net_id,
                    p->action,
                    p->serv_feature,
                    p->facility_coding,
                    p->param_fld
                );
            }
            break;
        //--------------------------------------------------------------------------------
        //          MLPP Precedence Level
        //--------------------------------------------------------------------------------
        case ACU_EXT_PRECEDENCE_LEVEL:
            {
                acu_ext_precedence_level *p = ( acu_ext_precedence_level * ) p_ext_data;
                
                debug_printf(V_ACU, "\t<EXT>: PRECEDENCE: level=%d, lfb=%d, chg=%d "
                    "NI='%X%X%X%X', domain=0x%06X\n",
                    p->level,
                    p->lfb,
                    p->change,
                    ( p->net_id >> 12 ) & 0x0F,
                    ( p->net_id >> 8  ) & 0x0F,
                    ( p->net_id >> 4  ) & 0x0F,
                    ( p->net_id >> 0  ) & 0x0F,
                    p->domain
                );
            }
            break;
            
        //--------------------------------------------------------------------------------
        //          Supplementary Services
        //--------------------------------------------------------------------------------
        case ACU_EXT_SERVICES:
            switch ( Acu_ext_ss_op_id )
            {
            case ACU_OP_ID_AOC_INFORM:
                {
                    acu_ss_aoc_inform_invoke *p = ( acu_ss_aoc_inform_invoke * ) p_ext_data;
                    
                    const char * name  = 0;
                    
                    switch ( p->aoc_type )
                    {
                    case ACU_SS_AOC_TYPE_AOC_S_INFORM:
                        debug_printf(V_ACU, "\t<EXT>: SERVICE: AOC-S\n");
                        break;
                        
                    case ACU_SS_AOC_TYPE_AOC_D_INFORM:
                        name = "AOC-D";
                        
                    case ACU_SS_AOC_TYPE_AOC_E_INFORM:
                        if ( ! name ) name = "AOC-E";
                        
                        debug_printf(V_ACU, "\t<EXT>: SERVICE: %s:\n", name );
                        {
                            #define CASE(x) case x: str = #x; break
                            const char *str;
                            
                            
                            tAcuSSAocDInform &info = p->aoc_data.aoc_d;
                            if ( info.currency_amount.invoke )
                            {
                                debug_printf(V_ACU, "\t\tcurrency amount = %d\n", info.currency_amount.value);
                            }
                            
                            if ( info.currency_id_size.invoke )
                            {
                                debug_printf(V_ACU, "\t\tcurrency ID     = '%.*s'\n", info.currency_id_size.value, info.currency_id );
                            }
                            
                            if ( info.multiplier.invoke )
                            {
                                debug_printf(V_ACU, "\t\tmultiplier      = %d\n", info.multiplier.value);
                            }
                            
                            if ( info.billing_id.invoke )
                            {
                                switch ( info.billing_id.value )
                                {
                                CASE(ACU_SS_AOC_BILLING_ID_NORMAL);
                                CASE(ACU_SS_AOC_BILLING_ID_CREDIT_CARD);
                                CASE(ACU_SS_AOC_BILLING_ID_REVERSE);
                                CASE(ACU_SS_AOC_BILLING_ID_CFU);
                                CASE(ACU_SS_AOC_BILLING_ID_CFB);
                                CASE(ACU_SS_AOC_BILLING_ID_CFNR);
                                CASE(ACU_SS_AOC_BILLING_ID_CD);
                                CASE(ACU_SS_AOC_BILLING_ID_TRF);
                                default:    
                                    str = "<not valid>";                                  
                                }
                                debug_printf(V_ACU, "\t\tbilling ID      = %s\n", str);
                            }
                            if ( info.charge_association.invoke )
                            {
                                debug_printf(V_ACU, "\t\tassociation     = PRESENT\n");
                            }

                            switch(info.type_of_charging)
                            {
                            CASE(ACU_SS_AOC_TYPE_OF_CHARGE_SUBTOTAL);
                            CASE(ACU_SS_AOC_TYPE_OF_CHARGE_TOTAL);
                            CASE(ACU_SS_AOC_TYPE_OF_CHARGE_NOT_AVAILABLE);
                            default:    
                                str = "<not valid>";                                  
                            }
                            
                            debug_printf(V_ACU, "\t\ttype of charge  = %s\n", str);

                            switch(info.recorded_charges)
                            {
                            CASE(ACU_SS_AOC_RECORDED_CHARGES_CHARGING_UNITS);
                            CASE(ACU_SS_AOC_RECORDED_CHARGES_CURRENCY_UNITS);
                            CASE(ACU_SS_AOC_RECORDED_CHARGES_FREE_OF_CHARGE);
                            default:    
                                str = "<not valid>";                                  
                            }
                            
                            debug_printf(V_ACU, "\t\trecorded charge = %s\n", str);
                            debug_printf(V_ACU, "\t\tcompleted       = %s\n", (info.completed?"YES":"NO"));
                            
                            if ( info.recorded_units.invoke )
                            {
                                unsigned num = info.recorded_units.num_records;
                                
                                if ( num > MAX_RECORDED_UNIT_RECORDS ) num = MAX_RECORDED_UNIT_RECORDS;

                                for (unsigned i = 0; i < num; i++ )
                                    debug_printf(V_ACU, "\t\trec. unit %d    = %d, type =%d\n", 
                                        i+1, 
                                        info.recorded_units.num_of_charging_units[i],
                                        info.recorded_units.type_of_charging_units[i]
                                    );
                            }
                        }
                        break;
                        
                    default:
                        debug_printf(V_ACU, "\t<EXT>: SERVICE: AOC-?\n");
                        break;
                    }
                      
                }
                break;
                
            default:
                debug_printf(V_ACU, "\t<EXT>: SERVICE: unknown op_id=0x%04X\n", Acu_ext_ss_op_id );
                break;
                
            }
            break;
                 

        //--------------------------------------------------------------------------------
        //          Not Supported (by demo) Extended Parameters
        //--------------------------------------------------------------------------------
        default:
            debug_printf(V_ACU, "\t<EXT>: UNKNOWN: id=0x%04X\n", Acu_ext_id );
            break;
        }
    
        p_ext_data += Acu_ext_lgth; // Move to the next parameter
    } // for

}

//--------------------------------------------------------------------------
//
//             DChannel::logprintf()
//
//   Prints to the log file.
//--------------------------------------------------------------------------
void DChannel::logprintf(int  mode, const char *format, ... )
{
    va_list args;
    va_start( args, format );

    if (logfile)
    {
        vfprintf(logfile,format,args);
        fflush(logfile);
    }
    debug_vprintf(mode, format,args);

    va_end( args );
};


//--------------------------------------------------------------------------
//
//             DChannel::logprintf()
//
//--------------------------------------------------------------------------
void DChannel::statistics(int s)
{
    stat[s]++;
}
