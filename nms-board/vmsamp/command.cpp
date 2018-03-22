/***********************************************************************
*  File - command.cpp
*
*  Description - This file contains functions related to waiting for and 
*                processing user commmands
*
*  2005  NMS Communications Corporation.
*
***********************************************************************/

///////////////////
// Include Files //
///////////////////
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mega_global.h"
#include "command.h"
#include "gw_types.h"
#include "vm_trace.h"

#include "srv_types.h"
#include "srv_def.h"
#include "3gpformat.h"

#define THREEGP_FNAME_EXT   "3gp"

int gExiting = FALSE;
DWORD disableVideoEP( int nGw, MSPHD hd );
DWORD enableVideoEP( int nGw, MSPHD hd );
int   getFileFormat( char *fileName );
tVideo_Format convertVideoCodec2Format( DWORD VideoCodec );
void displayOperation(int nGw, char *operation, char *fileName, int fileFormat);
extern void get_video_format_string( DWORD format, char *str );
DWORD closeAllChannels(int nGw);


/*********************************************************************************
  cmdVidRtpEp

  Function to display a menu of all the mspSendCommands which can be sent to a
  Video RTP endpoint, and then execute these commands.
**********************************************************************************/
void cmdVidRtpEp()
{
    DWORD   command, msp_cmd, cmdSize, cmdWords, i;
    DWORD   k, first_gw, last_gw;
    DWORD   ret, expectedEvent, query;
    DWORD   dwValue;
    WORD    byteCount = 0;
    WORD    value;
    DWORD   subtype = MSP_ENDPOINT_RTPFDX_VIDEO_MPEG4;   
    //DWORD   subtype = MSP_ENDPOINT_RTP_VIDEO_MPEG4;   
    void    *pCmd;
    char    selection;
    CTA_EVENT   event;
    MSP_ACTION  mspAction;
    BOOL display_menu = TRUE;
    BOOL execute_cmd = FALSE;
    msp_ENDPOINT_VIDMP4_ADD_SPECIAL_HDR     add_hdr_cmd;
    msp_ENDPOINT_RTPFDX_MAP                 ep_map_cmd;
    // msp_ENDPOINT_RTPFDX_RTP_PKTSZ_CTRL      pktSzCtrl;
    DWORD   EnableEndpoint  = FALSE;
    DWORD   DisableEndpoint = FALSE;

    // First and last gateways to send commands to
    first_gw = 0;
    last_gw = GwConfig[0].dNumGW - 1;
    while(display_menu)
    {
        // If there are multiple gateways, let the operator choose which on to command
        if(GwConfig[0].dNumGW > 1)
        {
            printf("\nEnter the number of the GW to send a command to "
                   "\n(or enter 99 to send to all gateways) > ");
            fflush(stdin);
            scanf("%hd", &value); 
            if(value != 99)
            {
                first_gw = value;
                last_gw = value;
            }
        }
        //Display the options to choose from
        printf(
        "\n"\
        "\t=====================================================================\n"\
        "\t      VIDEO RTP ENDPOINT COMMAND MENU                                \n"\
        "\t      ===============================                                \n"\
        "\tSelect one of the following commands for the Video RTP endpoint.     \n"\
        "\t  a)  Exit VIDEO RTP Command Menu   b)  Help (reprint this menu)     \n"\
        "\t  e)  mspEnableEndpoint             f)  mspDisableEndpoint           \n"\
        "\t  g)  Set Video Map ID and Vocoder                                   \n"\
        "\t=====================================================================\n");
        printf("\tSelection> ");
        fflush(stdin);
        scanf("%c", &selection);
        switch (selection)
	    {
		    case 'a':  //Quit
		    case 'x':  //Quit
                execute_cmd = FALSE;
                display_menu = FALSE;
                break;

            case 'b':  //Display Menu
                execute_cmd = FALSE;
                display_menu = TRUE;
                break;


            case 'e':  // Enable Endpoint
                EnableEndpoint  = TRUE;
                msp_cmd = 0;
                display_menu = FALSE;
                execute_cmd = TRUE;
                break;

            case 'f':  // Disable Endpoint
                DisableEndpoint  = TRUE;
                msp_cmd = 0;
                display_menu = FALSE;
                execute_cmd = TRUE;
                break;

            case 'g':  // Video Map
                printf("\n\tThe current setting of the Video Map ID = %d,\n\tEnter a new value > ",
                    GwConfig[0].VidRtpEp.Param.EP.RtpRtcp.PayloadMap.payload_id);
                fflush(stdin);
                scanf("%d", &dwValue);
                GwConfig[0].VidRtpEp.Param.EP.RtpRtcp.PayloadMap.payload_id = dwValue;
                ep_map_cmd.payload_id = H2NMS_DWORD(dwValue);

                printf("\n\tThe current setting of the Video Map Vocoder = %d,\n\tEnter a new value > ",
                    GwConfig[0].VidRtpEp.Param.EP.RtpRtcp.PayloadMap.vocoder);
                fflush(stdin);
                scanf("%d", &dwValue);
                GwConfig[0].VidRtpEp.Param.EP.RtpRtcp.PayloadMap.vocoder = dwValue;
                ep_map_cmd.vocoder = H2NMS_DWORD(dwValue);

                msp_cmd = MSP_CMD_RTPFDX_MAP;
                pCmd = &ep_map_cmd;
                cmdSize = sizeof(ep_map_cmd);
                display_menu = FALSE;
                execute_cmd = TRUE;
                break;


            default:   // Any other key
                printf("\n\tThe option selected is not valid.\n");
                display_menu = FALSE;
                break;
        }  // End Switch

        if (execute_cmd)
        {
            command = mspBuildCommand(subtype, msp_cmd);
            //////////////////////////////////////
            /////  mspSendCommand  ///////////////
            //////////////////////////////////////
            for(k=first_gw; k <= last_gw; k++)   //Send commands to each of the endpoints
            {
                //Disable Endpoint for Special Header Changes
                if(DisableEndpoint == TRUE)
                {
                    ret = mspDisableEndpoint(GwConfig[k].VidRtpEp.hd);
                    if (ret != SUCCESS)
                        printf("\n\t ERROR: mspDisableEndpoint returned failure.\n");
                    ret = WaitForSpecificEvent( k, GwConfig[k].hCtaQueueHd, MSPEVN_DISABLE_ENDPOINT_DONE, &event );
		            if ( ret != 0 )
		               printf("\nERROR: mspDisableEndpoint failed to send completion event\n");     
                }

                if(msp_cmd !=0)
                {
                    ret = mspSendCommand(GwConfig[k].VidRtpEp.hd,
                                        command,
                                        pCmd,
                                        cmdSize  );
                    //Verify correct response from mspSendCommand
                    if (ret != SUCCESS)
                        printf("\n\t ERROR: mspSendCommand returned failure.\n");
                    expectedEvent = (MSPEVN_SENDCOMMAND_DONE | msp_cmd);
                    ret = WaitForSpecificEvent( k, GwConfig[k].hCtaQueueHd, expectedEvent, &event );
		            if ( ret != 0 )
		            {
			            printf("\n\tERROR: mspSendCommand failed to send valid completion event\n");     
		            }
                }

                //Enable Endpoint for Special Header Changes
                if(EnableEndpoint == TRUE)
                {
                    ret = mspEnableEndpoint(GwConfig[k].VidRtpEp.hd);
                    if (ret != SUCCESS)
                        printf("\n\t ERROR: mspEnableEndpoint returned failure.\n");
                    ret = WaitForSpecificEvent( k, GwConfig[k].hCtaQueueHd, MSPEVN_ENABLE_ENDPOINT_DONE, &event );
		            if ( ret != 0 )
		               printf("\nERROR: mspEnableEndpoint failed to send completion event\n");     
                }

                //Record the command to file.
                if ( (RecCmdFp !=NULL) && (msp_cmd !=0) )
                {
                    memset(&mspAction, '\0', sizeof(mspAction));
                    mspAction.GWNo      = (WORD)k;
                    mspAction.objType   = MSP_VID_RTP_EP;
                    mspAction.action    = MSP_SEND_CMD;
                    mspAction.command   = command;
                    mspAction.cmdSize   = cmdSize;
                    mspAction.expectedEvent = expectedEvent;
                    memcpy( mspAction.CmdBuff, pCmd, cmdSize );
                
                    if( fwrite(&mspAction, 1, sizeof(mspAction), RecCmdFp) != sizeof(mspAction))
                    {
                        printf("\n\tERROR: Failed to properly write command to file.\n");
                    }
                    fflush(RecCmdFp);
                }
            }


        } // End if execute
    } // End While
    return;
}

/*********************************************************************************
  qryVidRtpEp

  Function to send an mspSendQuery to one or more Video RTP endpoints
**********************************************************************************/
void qryVidRtpEp()
{
    DWORD query, expectedEvent, ret, first_gw, last_gw;
    WORD    i, value;
    CTA_EVENT   event;
    MSP_ACTION  mspAction;

    // First and last gateways to send commands to
    first_gw = 0;
    last_gw = GwConfig[0].dNumGW - 1;
    // If there are multiple gateways, let the operator choose which one to command
    if(GwConfig[0].dNumGW > 1)
    {
        printf("\nEnter the number of the GW to send a query to "
            "(or enter 99 to send to all gateways) > ");
        fflush(stdin);
        scanf("%hd", &value); 
        if(value != 99)
        {
            first_gw = value;
            last_gw = value;
        }
    }

    printf("\nSending an assembly status query to the video RTP EP.");
    query = mspBuildQuery(MSP_ENDPOINT_RTPFDX_VIDEO_MPEG4, 
        MSP_QRY_RTPFDX_VIDEO_MPEG4_ASSY_STATUS);
    for(i=(WORD)first_gw; i <= (WORD)last_gw; i++)   //Send querys to each of the channels
    {
        ret = mspSendQuery( GwConfig[i].VidRtpEp.hd, query );
        if (ret != SUCCESS)
            printf("\n\t ERROR: mspSendQuery failed.\n");
        expectedEvent = (MSPEVN_QUERY_DONE | MSP_QRY_RTPFDX_VIDEO_MPEG4_ASSY_STATUS);
        ret = WaitForSpecificEvent( i, GwConfig[i].hCtaQueueHd, expectedEvent, &event );
		if ( ret != 0 )
		{
			printf("\n\tERROR: mspSendQuery failed to send valid completion event\n");     
		}
    
        //Record the query to file.
        if ( RecCmdFp !=NULL)
        {
            memset(&mspAction, '\0', sizeof(mspAction));
            mspAction.GWNo      = i;
            mspAction.objType   = MSP_ENDPOINT_RTPFDX_VIDEO_MPEG4;
            mspAction.action    = MSP_SEND_QRY;
            mspAction.command   = query;
            mspAction.expectedEvent = expectedEvent;

        
            if( fwrite(&mspAction, 1, sizeof(mspAction), RecCmdFp) != sizeof(mspAction))
            {
                printf("\n\tERROR: Failed to properly write command to file.\n");
            }
            fflush(RecCmdFp);
        }
    }
}


/*********************************************************************************
  cmdVidChan

  Function to display a menu of all the mspSendCommands which can be sent to a
  Video Channel, and then execute these commands.
**********************************************************************************/
void cmdVidChan()
{
    DWORD   command, msp_cmd, cmdSize;
    DWORD   k, first_gw, last_gw;
    DWORD   ret, expectedEvent;
    WORD    byteCount = 0;
    WORD    value;
    DWORD   dwValue;
    DWORD   subtype = MSP_FILTER_JITTER_VIDEO_MPEG4;   
    void    *pCmd;
    char    selection;
    CTA_EVENT   event;
    MSP_ACTION  mspAction;
    BOOL display_menu = TRUE;
    BOOL execute_cmd = FALSE;
    msp_FILTER_JITTER_VIDMP4_REMOVE_SPECIAL_HDR rm_hdr_cmd;

    // First and last gateways to send commands to
    first_gw = 0;
    last_gw = GwConfig[0].dNumGW - 1;
    while(display_menu)
    {
        // If there are multiple gateways, let the operator choose which on to command
        if(GwConfig[0].dNumGW > 1)
        {
            printf("\nEnter the number of the GW to send a command to "
                   "\n(or enter 99 to send to all gateways) > ");
            fflush(stdin);
            scanf("%hd", &value); 
            if(value != 99)
            {
                first_gw = value;
                last_gw = value;
            }
        }
        //Display the options to choose from
        printf(
        "\n"\
        "\t=====================================================================\n"\
        "\t      VIDEO CHANNEL COMMAND MENU                                     \n"\
        "\t      ===============================                                \n"\
        "\tSelect one of the following commands for the Video RTP endpoint.     \n"\
        "\t  a)  Exit VIDEO CHAN Command Menu  b)  Help (reprint this menu)     \n"\
        "\t  c)  Remove Special Header        \n"\
        "\t=====================================================================\n");
        printf("\tSelection> ");
        fflush(stdin);
        scanf("%c", &selection);
        switch (selection)
	    {
		    case 'a':  //Quit
		    case 'x':  //Quit
                execute_cmd = FALSE;
                display_menu = FALSE;
                break;

            case 'b':  //Display Menu
                execute_cmd = FALSE;
                display_menu = TRUE;
                break;

            case 'c':  // Remove Special Header
                printf("\n\tSize of Header to Remove > ");
                fflush(stdin);
                scanf("%d", &dwValue);
                rm_hdr_cmd.size = H2NMS_DWORD(dwValue);
                msp_cmd = MSP_CMD_VIDMP4_REMOVE_SPECIAL_HDR;
                pCmd = &rm_hdr_cmd;
                cmdSize = sizeof(rm_hdr_cmd);
                display_menu = FALSE;
                execute_cmd = TRUE;
                break;

            default:   // Any other key
                printf("\n\tThe option selected is not valid.\n");
                display_menu = FALSE;
                break;
        }  // End Switch

        if (execute_cmd)
        {
            command = mspBuildCommand(subtype, msp_cmd);
            //////////////////////////////////////
            /////  mspSendCommand  ///////////////
            //////////////////////////////////////
            for(k=first_gw; k <= last_gw; k++)   //Send commands to each of the endpoints
            {
                //Disable Channel for Special Header Changes
                if(msp_cmd == MSP_CMD_VIDMP4_REMOVE_SPECIAL_HDR)
                {
                    ret = mspDisableChannel(GwConfig[k].VidChan.hd);
                    if (ret != SUCCESS)
                        printf("\n\t ERROR: mspDisableChannel returned failure.\n");
                    ret = WaitForSpecificEvent(k, GwConfig[k].hCtaQueueHd, 
								MSPEVN_DISABLE_CHANNEL_DONE, &event );
		            if ( ret != 0 )
		               printf("\nERROR: mspDisableChannel failed to send completion event\n");     
                }

                ret = mspSendCommand(GwConfig[k].VidChan.hd,
                                    command,
                                    pCmd,
                                    cmdSize  );
                //Verify correct response from mspSendCommand
                if (ret != SUCCESS)
                    printf("\n\t ERROR: mspSendCommand returned failure.\n");
                expectedEvent = (MSPEVN_SENDCOMMAND_DONE | msp_cmd);
                ret = WaitForSpecificEvent( k, GwConfig[k].hCtaQueueHd, expectedEvent, &event );
		        if ( ret != 0 )
		        {
			        printf("\n\tERROR: mspSendCommand failed to send valid completion event\n");     
		        }

                //Enable Channel for Special Header Changes
                if(msp_cmd == MSP_CMD_VIDMP4_REMOVE_SPECIAL_HDR)
                {
                    ret = mspEnableChannel(GwConfig[k].VidChan.hd);
                    if (ret != SUCCESS)
                        printf("\n\t ERROR: mspEnableChannel returned failure.\n");
                    ret = WaitForSpecificEvent( k, GwConfig[k].hCtaQueueHd, 
								MSPEVN_ENABLE_CHANNEL_DONE, &event );
		            if ( ret != 0 )
		               printf("\nERROR: mspEnableChannel failed to send completion event\n");     
                }

                //Record the command to file.
                if ( RecCmdFp !=NULL)
                {
                    memset(&mspAction, '\0', sizeof(mspAction));
                    mspAction.GWNo      = (WORD)k;
                    mspAction.objType   = MSP_VID_CHAN;
                    mspAction.action    = MSP_SEND_CMD;
                    mspAction.command   = command;
                    mspAction.cmdSize   = cmdSize;
                    mspAction.expectedEvent = expectedEvent;
                    memcpy( mspAction.CmdBuff, pCmd, cmdSize );
                
                    if( fwrite(&mspAction, 1, sizeof(mspAction), RecCmdFp) != sizeof(mspAction))
                    {
                        printf("\n\tERROR: Failed to properly write command to file.\n");
                    }
                    fflush(RecCmdFp);
                }
            }


        } // End if execute
    } // End While
    return;
}


/*********************************************************************************
  qryVidChan

  Function to send a GET STATUS query to the Video Jitter Filter using 
  mspSendQuerys (There is currently only one query defined for this channel, so
  no menu is presented.)
**********************************************************************************/
void qryVidChan()
{
    DWORD query, expectedEvent, ret, first_gw, last_gw;
    WORD    i, value;
    CTA_EVENT   event;
    MSP_ACTION  mspAction;

    // First and last gateways to send commands to
    first_gw = 0;
    last_gw = GwConfig[0].dNumGW - 1;
    
    // If there are multiple gateways, let the operator choose which on to command
    if(GwConfig[0].dNumGW > 1)
    {
        printf("\nEnter the number of the GW to send a query to "
            "(or enter 99 to send to all gateways) > ");
        fflush(stdin);
        scanf("%hd", &value); 
        if(value != 99)
        {
            first_gw = value;
            last_gw = value;
        }
    }

    printf("\nSending a status query to the video channel.");
    
    for(i=(WORD)first_gw; i <= (WORD)last_gw; i++)   //Send querys to each of the endpoints
    {
        DWORD l_nVideoFilterType;
        // Determine the video filter type for the query
        if(GwConfig[i].VidChan.Addr.channelType  == MGH263VideoChannel)
            l_nVideoFilterType = MSP_FILTER_JITTER_VIDEO_H263;
        else if(GwConfig[i].VidChan.Addr.channelType  == MGVideoChannel)
            l_nVideoFilterType = MSP_FILTER_JITTER_VIDEO_MPEG4;
        else
        {
            printf("\n\t ERROR: Video Channel Type unknown\n");
            return;
        }
        query = mspBuildQuery(l_nVideoFilterType, MSP_QRY_JITTER_VIDEO_GET_STATE);
        ret = mspSendQuery( GwConfig[i].VidChan.hd, query );
        if (ret != SUCCESS)
            printf("\n\t ERROR: mspSendQuery failed.\n");
        expectedEvent = (MSPEVN_QUERY_DONE | MSP_QRY_JITTER_VIDEO_GET_STATE);
        ret = WaitForSpecificEvent( i, GwConfig[i].hCtaQueueHd, expectedEvent, &event );
		if ( ret != 0 )
		{
			printf("\n\tERROR: mspSendQuery failed to send valid completion event\n");     
		}
    
        //Record the query to file.
        if ( RecCmdFp !=NULL)
        {
            memset(&mspAction, '\0', sizeof(mspAction));
            mspAction.GWNo      = i;
            mspAction.objType   = l_nVideoFilterType; // MSP_FILTER_JITTER_VIDEO_H263
            mspAction.action    = MSP_SEND_QRY;
            mspAction.command   = query;
            mspAction.expectedEvent = expectedEvent;

        
            if( fwrite(&mspAction, 1, sizeof(mspAction), RecCmdFp) != sizeof(mspAction))
            {
                printf("\n\tERROR: Failed to properly write command to file.\n");
            }
            fflush(RecCmdFp);
        }
    }


}

/*********************************************************************************
  cmdAudRtpEp

  Function to display a menu of all the mspSendCommands which can be sent to a
  AMR Bypass RTP EP, and then execute these commands.
**********************************************************************************/
void cmdAudRtpEp()
{
    DWORD       value, command, expectedEvent, ret, first_gw, last_gw;
    WORD        i;
    CTA_EVENT   event;
    MSP_ACTION  mspAction;
    msp_ENDPOINT_RTPFDX_MAP ep_map_cmd;

    // First and last gateways to send commands to
    first_gw = 0;
    last_gw = GwConfig[0].dNumGW - 1;
    // If there are multiple gateways, let the operator choose which on to command
    if(GwConfig[0].dNumGW > 1)
    {
        printf("\nEnter the number of the GW to send a command to "
            "(or enter 99 to send to all gateways) > ");
        fflush(stdin);
        scanf("%d", &value); 
        if(value != 99)
        {
            first_gw = (DWORD)value;
            last_gw = (DWORD)value;
        }
    }

    printf("\n\tThe current setting of the payloadID = %d,\n\tEnter a new value > ",
        GwConfig[0].AudRtpEp.Param.EP.RtpRtcp.PayloadMap.payload_id);
    fflush(stdin);
    scanf("%d", &value);
    GwConfig[0].AudRtpEp.Param.EP.RtpRtcp.PayloadMap.payload_id = value;
    ep_map_cmd.payload_id = H2NMS_DWORD(value);

/*    printf("\n\tThe current setting of the Video Map Vocoder = %d,\n\tEnter a new value > ",
        GwConfig[0].VidRtpEp.Param.EP.RtpRtcp.PayloadMap.vocoder);
    fflush(stdin);
    scanf("%d", &value);*/
    value = GwConfig[0].AudRtpEp.Param.EP.RtpRtcp.PayloadMap.vocoder;
    ep_map_cmd.vocoder = H2NMS_DWORD(value);

    command = mspBuildCommand(MSP_ENDPOINT_RTPFDX, MSP_CMD_RTPFDX_MAP);
    for(i=(WORD)first_gw; i <= (WORD)last_gw; i++)   //Send querys to each of the endpoints
    {
        ret = mspSendCommand(GwConfig[i].AudRtpEp.hd,
                            command,
                            &ep_map_cmd,
                            sizeof(ep_map_cmd));
        //Verify correct response from mspSendCommand
        if (ret != SUCCESS)
            printf("\n\t ERROR: mspSendCommand returned failure.\n");
        expectedEvent = (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_RTPFDX_MAP);        
        ret = WaitForSpecificEvent( i, GwConfig[i].hCtaQueueHd, expectedEvent, &event );
		if ( ret != 0 )
		{
			printf("\n\tERROR: mspSendCommand failed to send valid completion event\n");     
		}
    
        //Record the query to file.
        if ( RecCmdFp !=NULL)
        {
            memset(&mspAction, '\0', sizeof(mspAction));
            mspAction.GWNo      = i;
            mspAction.objType   = MSP_ENDPOINT_RTPFDX;
            mspAction.action    = MSP_SEND_CMD;
            mspAction.command   = command;
            mspAction.expectedEvent = expectedEvent;
            mspAction.cmdSize   = sizeof(ep_map_cmd);
            memcpy( mspAction.CmdBuff, &ep_map_cmd, sizeof(ep_map_cmd) );


            if( fwrite(&mspAction, 1, sizeof(mspAction), RecCmdFp) != sizeof(mspAction))
            {
                printf("\n\tERROR: Failed to properly write command to file.\n");
            }
            fflush(RecCmdFp);
        }
    }
}


void qryAudRtpEp(){
        printf("\n\tQuery to the AMR RTP endpoint is not currently defined.\n ");
}

/*********************************************************************************
  cmdAmrByChan

  Function to display a menu of all the mspSendCommands which can be sent to a
  AMR Bypass Channel, and then execute these commands.
**********************************************************************************/
void cmdAmrByChan()
{
    DWORD       value, command, expectedEvent, ret, first_gw, last_gw;
    WORD        i;
    CTA_EVENT   event;
    MSP_ACTION  mspAction;
    msp_FILTER_JITTER_CMD  amr_by_chan_cmd;

    // First and last gateways to send commands to
    first_gw = 0;
    last_gw = GwConfig[0].dNumGW - 1;
    // If there are multiple gateways, let the operator choose which on to command
    if(GwConfig[0].dNumGW > 1)
    {
        printf("\nEnter the number of the GW to send a command to "
            "(or enter 99 to send to all gateways) > ");
        fflush(stdin);
        scanf("%d", &value); 
        if(value != 99)
        {
            first_gw = (DWORD)value;
            last_gw = (DWORD)value;
        }
    }

    printf("\n\tEnter a new value for the jitter depth > ");
    fflush(stdin);
    scanf("%d", &value);

    amr_by_chan_cmd.value = H2NMS_DWORD(value);

    command = mspBuildCommand(MSP_FILTER_JITTER, MSP_CMD_JITTER_CHG_DEPTH);
    for(i=(WORD)first_gw; i <= (WORD)last_gw; i++)   //Send querys to each of the endpoints
    {
        ret = mspSendCommand(GwConfig[i].AudByChan.hd,
                            command,
                            &amr_by_chan_cmd,
                            sizeof(amr_by_chan_cmd));
        //Verify correct response from mspSendCommand
        if (ret != SUCCESS)
            printf("\n\t ERROR: mspSendCommand returned failure.\n");
        expectedEvent = (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_JITTER_CHG_DEPTH);        
        ret = WaitForSpecificEvent( i, GwConfig[i].hCtaQueueHd, expectedEvent, &event );
		if ( ret != 0 )
		{
			printf("\n\tERROR: mspSendQuery failed to send valid completion event\n");     
		}
    
        //Record the query to file.
        if ( RecCmdFp !=NULL)
        {
            memset(&mspAction, '\0', sizeof(mspAction));
            mspAction.GWNo      = i;
            mspAction.objType   = MSP_FILTER_JITTER;
            mspAction.action    = MSP_SEND_CMD;
            mspAction.command   = command;
            mspAction.expectedEvent = expectedEvent;
            mspAction.cmdSize   = sizeof(amr_by_chan_cmd);
            memcpy( mspAction.CmdBuff, &amr_by_chan_cmd, sizeof(amr_by_chan_cmd) );


            if( fwrite(&mspAction, 1, sizeof(mspAction), RecCmdFp) != sizeof(mspAction))
            {
                printf("\n\tERROR: Failed to properly write command to file.\n");
            }
            fflush(RecCmdFp);
        }
    }
}

/*********************************************************************************
  cmdIsdnCall

  Function to display a menu of all the call control commands: 
    Place Call
    Disconnect Call
**********************************************************************************/
void cmdIsdnCall( void )
{
    DWORD   k, first_gw, last_gw;
    WORD    byteCount = 0;
    WORD    value;
    char    selection;
    BOOL display_menu = TRUE;
    BOOL execute_cmd = FALSE;
    CTA_EVENT NewEvent;  
    PBX_PLACE_CALL_STRUCT pbxPlaceCall;

    // First and last gateways to send commands to
    first_gw = 0;
    last_gw = GwConfig[0].dNumGW - 1;
    while(display_menu)
    {
        // If there are multiple gateways, let the operator choose which on to command
        if(GwConfig[0].dNumGW > 1)
        {
            printf("\nEnter the number of the GW to send a command to "
                   "\n(or enter 99 to send to all gateways) > ");
            fflush(stdin);
            scanf("%hd", &value); 
            if(value != 99)
            {
                first_gw = value;
                last_gw = value;
            }
        }
        //Display the options to choose from
        printf(
        "\n"\
        "\t=====================================================================\n"\
        "\t      CALL CONTROL COMMAND MENU                                      \n"\
        "\t      ===============================                                \n"\
        "\tSelect one of the following commands for the Video RTP endpoint.     \n"\
        "\t  a)  Exit CALL CONTROL Command Menu  b)  Help (reprint this menu)   \n"\
        "\t  c)  Place Call                      d)  Disconnect Call            \n"\
        "\t  e)  Close all Channel / Send End Session / ISDN Disconnect         \n"\
        "\t=====================================================================\n");        

        printf("\tSelection> ");
        fflush(stdin);
        scanf("%c", &selection);
        switch (selection)
	    {
		    case 'a':  //Quit
		    case 'x':  //Quit
                execute_cmd = FALSE;
                display_menu = FALSE;
                break;

            case 'b':  //Display Menu
                execute_cmd = FALSE;
                display_menu = TRUE;
                break;

            case 'c':  // Place Call
                display_menu = FALSE;
                printf("\n\tNumber to Call > ");
                fflush(stdin);
                scanf("%s", pbxPlaceCall.calledaddr );
				
	            for(k=first_gw; k <= last_gw; k++) {
					autoPlaceCall( k, pbxPlaceCall.calledaddr );
				}							
                break;

            case 'd':  // Disconnect
                display_menu = FALSE;
	            for(k=first_gw; k <= last_gw; k++) {
					autoDisconnectCall( k );
				}			
                break;

           case 'e':
               display_menu = FALSE;
               if ( closeAllChannels(0)!= SUCCESS)
                  return;
               autoDisconnectCall( 0 );
               
               break;        
                        
            default:   // Any other key
                printf("\n\tThe option selected is not valid.\n");
                display_menu = FALSE;
                break;
        }  // End Switch

    } // End While
    return;

}


void cmdH245( void )
{
    char    selection;
    BOOL display_menu = TRUE;
	bool rc;

    // First and last gateways to send commands to
    while(display_menu)
    {

        // TODO Implement Multiple Gateways
        // NOTE LOOP FOR MULTIPLE GATEWAYS NOT IMPLEMENTED HERE
        //Display the options to choose from
        printf("\n\t=====================================================================\n");
        printf("\t      H245 COMMAND MENU                                              \n");
        printf("\t      ===============================                                \n");
        printf("\tSelect one of the following commands for H245.                       \n");
        printf("\t  a)  Exit H245 Command Menu          b)  Help (reprint this menu)   \n");
        printf("\t  c)  H245 Fast Video Update          d)  H245 Round Trip Delay      \n");
        printf("\t  e)  H245 End Session                                               \n");
        printf("\t  f)  H245 Alphanumeric User Indication (128 bytes)                  \n");
        printf("\t  g)  H245 Alphanumeric User Indication (512 bytes)                  \n");
        printf("\t  h)  H245 Non-Standard User Indication (128 bytes)                  \n");
        printf("\t  i)  H245 Non-Standard User Indication (512 bytes)                  \n");	
        printf("\t  l)  H245 Close Channel                                             \n");
        printf("\t  m)  Control DEMUX Line Error Reporting                             \n");
        
        printf("\t  n)  H245 Skew Indication                                           \n");
        printf("\t  o)  H245 VendorID Indication                                       \n");
        printf("\t  p)  H245 VideoTemporalSpatialTradeoff Indication                   \n");
        
        printf("\t=====================================================================\n");
        printf("\tSelection> ");
        fflush(stdin);
        scanf("%c", &selection);
        switch (selection)
	    {
		    case 'a':  //Quit
		    case 'x':  //Quit
                display_menu = FALSE;
                break;

            case 'b':  //Display Menu
                display_menu = TRUE;
                break;

            case 'c':  // H245 Fast Video Update
                display_menu = FALSE;
				h324VideoFastUpdate( GwConfig[0].MuxEp.hd );
                break;

            case 'd':  // H245 Round Trip Delay
                display_menu = FALSE;
				h324RoundTripDelay( GwConfig[0].MuxEp.hd );				
                break;

            case 'e':  // H245 End Session
                display_menu = FALSE;
				h324EndSession( GwConfig[0].MuxEp.hd );
                break;

            case 'f':  // H245 User Indication - alphanumeric
            {
                H324_USER_INPUT_INDICATION l_UII;
                display_menu  = FALSE;
                
                // Fill values in UII structure
				l_UII.length = 128;
                l_UII.msgType = H324_USER_INPUT_ALPHANUMERIC;
                l_UII.szObjectId[0] = 0;
				strncpy( l_UII.data, "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
                             "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
                             "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
                             "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
                             "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
                             "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
                             "0123456789ABCDEF0123456789ABCDEF", l_UII.length);
				mmPrint( 0, T_H245, "UII length: %d\n", l_UII.length );
                // Send UII
				rc = h324UserIndication( GwConfig[0].MuxEp.hd, &l_UII );
				if( rc == SUCCESS ) 
					mmPrint( 0, T_H245, "The alphanumeric UII message was sent successfully.\n");
				else
					printf("Failed to send the alphanumeric UII message...\n");					
                break;
            }
            case 'g':  // H245 User Indication - alphanumeric
            {
                H324_USER_INPUT_INDICATION l_UII;
                display_menu  = FALSE;
                
                // Fill values in UII structure
				l_UII.length = 512;
                l_UII.msgType = H324_USER_INPUT_ALPHANUMERIC;
                l_UII.szObjectId[0] = 0;
				strncpy( l_UII.data, "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
                             "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
                             "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
                             "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
                             "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
                             "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
                             "0123456789ABCDEF0123456789ABCDEF", l_UII.length );
				mmPrint( 0, T_H245, "UII length: %d\n", l_UII.length );
                // Send UII
				rc = h324UserIndication( GwConfig[0].MuxEp.hd, &l_UII );
				if( rc == SUCCESS ) 
					mmPrint( 0, T_H245, "The alphanumeric UII message was sent successfully.\n");
				else
					printf("Failed to send the alphanumeric UII message...\n");					
                break;
            }            
            case 'h':  // H245 User Indication - nonstandard
            {
                H324_USER_INPUT_INDICATION l_UII;
                DWORD i;
                display_menu  = FALSE;
                
                // Fill values in UII structure
				l_UII.length = 128;
                l_UII.msgType = H324_USER_INPUT_NONSTANDARD;
                l_UII.szObjectId[0] = 0;
                strcpy( l_UII.szObjectId, "1.5.9.12.234.255.3906.12345.1234567");
                for(i=0; i<l_UII.length; i++)
                    l_UII.data[i]=i%256;
				mmPrint( 0, T_H245, "UII length: %d\n", l_UII.length );
                // Send UII
                rc = h324UserIndication( GwConfig[0].MuxEp.hd, &l_UII );
				if( rc == SUCCESS ) 
					mmPrint( 0, T_H245, "The NonStandard UII message was sent successfully.\n");
				else
					mmPrintErr( 0, T_H245, "Failed to send the NonStandard UII message...\n");					
                break;              
            }  
            case 'i':  // H245 User Indication - nonstandard
            {
                H324_USER_INPUT_INDICATION l_UII;
                DWORD i;
                display_menu  = FALSE;
                
                // Fill values in UII structure
				l_UII.length = 512;
                l_UII.msgType = H324_USER_INPUT_NONSTANDARD;
                l_UII.szObjectId[0] = 0;
                strcpy( l_UII.szObjectId, "1.5.9.12.234.255.3906.12345.1234567");
                for(i=0; i<l_UII.length; i++)
                    l_UII.data[i]=i%256;
				mmPrint( 0, T_H245, "UII length: %d\n", l_UII.length );
                // Send UII
                rc = h324UserIndication( GwConfig[0].MuxEp.hd, &l_UII );
				if( rc == SUCCESS ) 
					mmPrint( 0, T_H245, "The NonStandard UII message was sent successfully.\n");
				else
					mmPrintErr( 0, T_H245, "Failed to send the NonStandard UII message...\n");					
                break;              
            }  

            case 'l':
                display_menu = FALSE;
                printf("\t=====================================================================\n");
                printf("\tSelect one of the following channels to close:                       \n");
                printf("\t  a)  Close Audio In Channel                                         \n");
                printf("\t  b)  Close Audio Out Channel                                        \n");
                printf("\t  c)  Close Video Channel                                            \n");
                printf("\t  d)  Re-Open Video Channel                                          \n");
                printf("\t=====================================================================\n");
                printf("\tSelection> ");
                fflush(stdin);
                scanf("%c", &selection);
                switch (selection)
	            {
                    case 'a':
				        h324CloseChannel( GwConfig[0].MuxEp.hd, H324RSN_AUDIO_IN );
                        Flow(0, F_APPL, F_H324, "h324CloseChannel(audio in)");
                        break;
                    case 'b':
				        h324CloseChannel( GwConfig[0].MuxEp.hd, H324RSN_AUDIO_OUT );
                        Flow(0, F_APPL, F_H324, "h324CloseChannel(audio out)");
                        break;
                    case 'c':
				        h324CloseChannel( GwConfig[0].MuxEp.hd, H324RSN_VIDEO );
                        Flow(0, F_APPL, F_H324, "h324CloseChannel(video)");
                        break;                
                    case 'd':
                     if ((GwConfig[0].localTermCaps.capTable[1].choice == H324_MPEG4_VIDEO)&& gwParm.specifyVOVOL )
                     {
                        // Modify Video Channel Header to look like requested config
                        /*
                        	 // NMS TCoder MPEG4 Header	
                         BYTE mpeg4_cfg_nms[] = { 
                         0x00, 0x00, 0x01, 0xb0, 0x08, 0x00, 0x00, 0x01, 
                         0xb5, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,                
                         0x01, 0x20, 0x00, 0x88, 0x40, 0x07, 0xa8, 0x2c, 
                         0x20, 0x90, 0xa2, 0x1f 
                         };
                         int mpeg4_cfg_len_nms = 28;
                         */
                       	 // DNA MPEG4 Header	
                         BYTE mpeg4_cfg_nms[] = { 
                         0x00, 0x00, 0x01, 0xb0, 0x08, 0x00, 0x00, 0x01, 
                         0xb5, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,                
                         0x01, 0x20, 0x00, 0x84, 0x5d, 0x4c, 0x28, 0x2c, 
                         0x20, 0x90, 0xa2, 0x1f 
                         };
                         int mpeg4_cfg_len_nms = 28;                         
                         
                         
                        BYTE    *l_pHdrCfg = mpeg4_cfg_nms;
                        int     l_nHdrCfgLen = mpeg4_cfg_len_nms;
                        H324_MEDIA_CAPABILITY *pMP4Cap;
                                                
                        // Modify config to look like requested config
                        pMP4Cap = &GwConfig[0].localTermCaps.capTable[1];
                        pMP4Cap->u.mpeg4.capability.bit_mask                  |= mp4video_decoder_config_info_present;
                        pMP4Cap->u.mpeg4.capability.decoder_config_info_len   = l_nHdrCfgLen;
                        pMP4Cap->u.mpeg4.capability.decoder_config_info       = new unsigned char[l_nHdrCfgLen];
                        memcpy( pMP4Cap->u.mpeg4.capability.decoder_config_info,
                              l_pHdrCfg,
                              l_nHdrCfgLen   );			
                     }
                    
                    h324DefineMediaChannel( GwConfig[0].MuxEp.hd, &GwConfig[0].localTermCaps.capTable[1], // out channel
										  &GwConfig[0].localTermCaps.capTable[1] );               // in channel
				        
                        Flow(0, F_APPL, F_H324, "h324OpenChannel(video)");
                        break;
                }
                break;
            case 'm':
                display_menu = FALSE;
                printf("\t=====================================================================\n");
                printf("\tSelect one of the following Error report types:                      \n");
                printf("\t  a)  Reset Statistics                                               \n");
                printf("\t  b)  Periodic Statistics                                            \n");
                printf("\t  c)  Error Event                                                    \n");
                printf("\t=====================================================================\n");
                printf("\tSelection> ");
                fflush(stdin);
                scanf("%c", &selection);
                switch (selection)
	            {
                    WORD value1, value2;
                    case 'a':
                        h324LineErrorReporting( GwConfig[0].MuxEp.hd, 
                                H324_LINE_STAT_CMD_RESET_STAT,
                                0,
                                0);
                        break;
                    case 'b':

                        printf("\n\tEnable/Disable Periodic Statistics (0=Disable, 1=Enable) > ");
                        fflush(stdin);
                        scanf("%hd", &value1);
                        printf("\n\tSet Periodic Statistics Interval (1-10 seconds X 10) > ");
                        fflush(stdin);
                        scanf("%hd", &value2);
                        h324LineErrorReporting( GwConfig[0].MuxEp.hd, 
                                H324_LINE_STAT_CMD_PERIODIC,
                                value1,
                                value2);
                        break;
                    case 'c':
                        printf("\n\tEnable/Disable Error Event (0=Disable, 1=Enable) > ");
                        fflush(stdin);
                        scanf("%hd", &value1);
                        printf("\n\tSet Event Bit Mask (1=Video, 2=Audio, 4=Golay) > ");
                        fflush(stdin);
                        scanf("%hd", &value2);
                        h324LineErrorReporting( GwConfig[0].MuxEp.hd, 
                                H324_LINE_STAT_CMD_ERROR_EVENT,
                                value1,
                                value2);
                        break;
                }
                break;

            case 'n':
                {
                unsigned skewInMs;

                display_menu = FALSE;
                printf("\t=====================================================================\n");
                printf("\tSelect the type of channel skew to indicate:                         \n");
                printf("\t  a)  Audio lags video                                               \n");
                printf("\t  v)  Video lags audio                                               \n");
                printf("\t=====================================================================\n");
                for ( ; ; )
                {
                    printf("\tSelection> ");
                    fflush(stdin);
                    scanf("%c", &selection);
                    if (selection == 'a' || selection == 'v') break;
                    printf("\n\tSelection must be 'a' or 'v' -- try again.\n");
                }
                printf("\t=====================================================================\n");
                printf("\tSpecify skew interval [0-4095 ms] between channels:                  \n");
                printf("\t=====================================================================\n");
                for (skewInMs=4096; ; )
                {
                    printf("\tInterval> ");
                    fflush(stdin);
                    scanf("%u", &skewInMs);
                    if (skewInMs <= 4095) break;
                    printf("\n\tSkew value %u ms. exceeds 4095 ms. maximum -- try again.\n", skewInMs);
                }
                
                // Send specified skew interval indication to the remote terminal.
                // (No need to prompt for the LCNs, they are implicit in the MSP handle.)
                mmPrint(0, T_H245, "Informing terminal that %s channel is skewed %u ms. behind %s channel.\n",
                        (selection == 'a') ? "audio" : "video", skewInMs,
                        (selection == 'a') ? "video" : "audio");
                rc = h324_h223SkewIndication(GwConfig[0].MuxEp.hd, skewInMs, 
                        (selection == 'a') ? audioLate : videoLate);
                if (rc == SUCCESS) 
                   mmPrint(0, T_H245, "The SkewIndication message was sent successfully.\n");
                else
                   mmPrintErr("Failed to send the SkewIndication message...\n");
                }
                break;

            case 'o':
            {
                #define VENDORID_TEST_BUF_SZ (sizeof(H324_VENDORID_INDICATION) + 128 + 256 + 256)
                unsigned char l_vidBuf[VENDORID_TEST_BUF_SZ];
                H324_VENDORID_INDICATION * l_pVID = (H324_VENDORID_INDICATION *)&l_vidBuf[0];
                unsigned char * pByte = &l_pVID->bytes[0];
                unsigned       val, val2;
                char           buf[4];

                display_menu = FALSE;
                printf("\t=====================================================================\n");
                printf("\t  vendorID field:                                                    \n");
                printf("\t                                                                     \n");
                printf("\t    (O)bject Identifier or                                           \n");
                printf("\t    (N)onstandard Identifier?                                        \n");
                printf("\t                                                                     \n");
                printf("\t=====================================================================\n");
                memset(&l_vidBuf, 0, VENDORID_TEST_BUF_SZ);
                for ( ; ; )
                {
                    printf("\tSelection> ");
                    fflush(stdin);
                    scanf("%c", &selection);
                    if (selection == 'o' || selection == 'O' || selection == 'n' || selection == 'N') 
                       break;
                    printf("\n\tSelection must be 'o' or 'n' -- try again.\n");
                }
                if (selection == 'o' || selection == 'O')
                {
                    pByte = &l_pVID->bytes[0];
                    
                    for (l_pVID->vendorIDLen = 0; ; l_pVID->vendorIDLen++)
                    {
                        printf("\tEnter ObjectID field (0..255: [Return] when finished)> ");
                        fflush(stdin);
                        *buf = '\0';
                        if (fgets(buf, 4, stdin) == NULL || *buf == '\n')
                            break;
                        sscanf(buf, "%3d", &val);
                        *pByte++ = (unsigned char)val;
                    }

                    // This is an Object identifer, not an h221NonStandard identifier.
                    l_pVID->isNonStandard = 0; 
                }
                else // selection == 'n' || selection == 'N'
                {
                    unsigned char t35CountryCode, t35Extension;
                    unsigned short manufacturerCode;
                    unsigned tmp;

                    // Prompt for the NonstandardIdentifier fields.
                    printf("\tEnter t35CountryCode (0..255)> ");
                    fflush(stdin);
                    scanf("%3d", &tmp);
                    t35CountryCode = (unsigned char)tmp;
                    printf("\tEnter t35Extension (0..255)> ");
                    fflush(stdin);
                    scanf("%3d", &tmp);
                    t35Extension = (unsigned char)tmp;
                    printf("\tEnter manufacturerCode (0..65535)> ");
                    fflush(stdin);
                    scanf("%5d", &tmp);
                    manufacturerCode = (unsigned short)tmp;

                    // Write the four bytes of NonstandardIdentifer to the
                    // beginning of the vendorID buffer and set 1st field's length.
                    *pByte++ = t35CountryCode;
                    *pByte++ = t35Extension;
                    *pByte++ = (unsigned char)(manufacturerCode >> 8);
                    *pByte++ = (unsigned char)(manufacturerCode & 0x00FF);
                    l_pVID->vendorIDLen = 4;

                    // This is an h221NonStandard identifier, not an Object identifer.
                    l_pVID->isNonStandard = 1;
                }

                printf("\t=====================================================================\n");
                printf("\t  productNumber field:                                               \n");
                printf("\t                                                                     \n");
                printf("\t    The VendorID productNumber field is optional and is specified    \n");
                printf("\t    in hexadecimal octets.  Press [Return] after entering each       \n");
                printf("\t    octet.  Press [Return] twice when the last octet has been        \n");
                printf("\t    entered.  No more than 256 octets may be entered in this field.  \n");
                printf("\t                                                                     \n");
                printf("\t=====================================================================\n");
                for (l_pVID->productNumberLen = 0; l_pVID->productNumberLen < 256; l_pVID->productNumberLen++)
                {
                    printf("\tEnter productNumber octet (0..FF: [Return] when finished)> ");
                    fflush(stdin);
                    *buf = '\0';
                    if (fgets(buf, 3, stdin) == NULL || *buf == '\n')
                        break;
                    sscanf(buf, "%2x", &val);
                    *pByte++ = (unsigned char)(val & 0x00FF);
                }

                printf("\t=====================================================================\n");
                printf("\t  versionNumber field:                                               \n");
                printf("\t                                                                     \n");
                printf("\t    The VendorID versionNumber field is optional and is specified    \n");
                printf("\t    in hexadecimal octets.  Press [Return] after entering each       \n");
                printf("\t    octet.  Press [Return] twice when the last octet has been        \n");
                printf("\t    entered.  No more than 256 octets may be entered in this field.  \n");
                printf("\t                                                                     \n");
                printf("\t=====================================================================\n");
                for (l_pVID->versionNumberLen = 0; l_pVID->versionNumberLen < 256; l_pVID->versionNumberLen++)
                {
                    printf("\tEnter versionNumber octet (0..FF: [Return] when finished)> ");
                    fflush(stdin);
                    *buf = '\0';
                    if (fgets(buf, 3, stdin) == NULL || *buf == '\n')
                        break;
                    sscanf(buf, "%2x", &val);
                    *pByte++ = (unsigned char)(val & 0x00FF);
                }

                // Issue the assembled VendorID indication to the terminal.
                rc = h324VendorIDIndication(GwConfig[0].MuxEp.hd, l_pVID);
                if (rc == SUCCESS) 
                   mmPrint(0, T_H245, "The VendorID indication was sent successfully.\n");
                else
                   mmPrintErr("Failed to send the VendorID indication msg...\n");

            }
            break;

            case 'p':
            {
                unsigned tradeoff;

                display_menu = FALSE;
                printf("\t=====================================================================\n");
                printf("\tSpecify temporal-spatial tradeoff value for outbound video channel:  \n");
                printf("\t=====================================================================\n");
                for ( ; ; )
                {
                    printf("\tTradeoff value (0..31)> ");
                    fflush(stdin);
                    scanf("%u", &tradeoff);

                    if (tradeoff < 32) 
                       break;
                    printf("\n\tTradoff value out-of-range -- try again.\n");
                }
                
                // Send specified videoTemporalSpatialTradeoff indication to the remote terminal.
                mmPrint(0, T_H245, "Informing terminal videoTemporalSpatialTradeoff value is now %u.\n",
                        tradeoff);
                rc = h324VideoTemporalSpatialTradeoffIndication(GwConfig[0].MuxEp.hd, tradeoff);
                if (rc == SUCCESS) 
                   mmPrint(0, T_H245, "The videoTemporalSpatialTradeoff Indication message was sent successfully.\n");
                else
                   mmPrintErr("Failed to send the videoTemporalSpatialTradeoff Indication message...\n");
            }
            break;

            default:   // Any other key
                printf("\n\tThe option selected is not valid.\n");
                display_menu = FALSE;
                break;
        }  // End Switch

    } // End While
    return;

}

void cmdTrc( void )
{
#ifdef USE_TRC
    char    selection;
    BOOL display_menu = TRUE;
	bool rc;
	tTrcStatus trcStatus;
    int i, j, vtpId;
    DWORD ret;
  	MEDIA_GATEWAY_CONFIG *pCfg;
  	CTA_EVENT   event;
  	tTxCoderParam txCoderParam;

	// Get a pointer to the gateway's configuration
	pCfg = &GwConfig[0];        
	
    while(display_menu)
    {
        //Display the options to choose from
        printf("\n\t===================================================================\n");
        printf("\t      TRC COMMAND MENU                                               \n");
        printf("\t      ===============================                                \n");
        printf("\tSelect one of the following commands for TRC.                        \n");
        printf("\t  a)  Exit TRC Command Menu           b)  Help (reprint this menu)   \n");
        printf("\t  c)  TRC Get Status                  d)  Reset VTP                  \n");
        printf("\t  e)  TRC start Channel               f)  TRC stop Channel           \n");
        printf("\t=====================================================================\n");
        printf("\tSelection> ");
        fflush(stdin);
        scanf("%c", &selection);
        switch (selection)
	    {
		    case 'a':  //Quit
		    case 'q':  //Quit
		    case 'x':  //Quit
                display_menu = FALSE;
                break;

            case 'b':  //Display Menu
                display_menu = TRUE;
                break;

            case 'c':  // TRC Get Status
                display_menu = FALSE;
				trcGetStatus( &trcStatus );
				printf("\nNum of transcoders: %d\n", trcStatus.nNumTranscoderPlatforms );
				for( i=0; i<trcStatus.nNumTranscoderPlatforms; i++ ) {
					printf("\tVTP ID %d: status=%d, maxNumSimplex=%d, numActiveSimplex=%d, numActiveFdx=%d, IP=%s\n",
						i, trcStatus.trInfo[i].status, trcStatus.trInfo[i].maxNumSimplexChnls,
						trcStatus.trInfo[i].numActiveSimplexChnls, trcStatus.trInfo[i].numActiveFdxChnls,
						trcStatus.trInfo[i].ipAddr );
                    for( j=0; j<trcStatus.trInfo[i].numActiveSimplexChnls+trcStatus.trInfo[i].numActiveFdxChnls; j++ ) {
                        printf("\t\tChannel ID=0x%x, Status=%d\n",
                                 trcStatus.trInfo[i].channel[j].id,
                                 trcStatus.trInfo[i].channel[j].status );
                    }
				}                
                break;
            case 'd': // TRC reset VTP
                display_menu = FALSE;                
                printf("\n\tEnter the VTP ID > ");
                fflush(stdin);
                scanf("%d", &vtpId);
                
                ret = trcResetVTP( vtpId );
                printf("trcResetVTP ret=%d\n", ret);
                break;
                
            case 'e': // start TRC channel
                display_menu = FALSE;    
                  ret = trcCreateVideoChannel( TRC_CH_SIMPLEX, &vtpId,
                                           &pCfg->VideoXC.xcChanId,
                                           &txCoderParam );
                  Flow(0, F_APPL, F_TRC, "trcCreateVideoChannel");            
                  if( ret != TRC_SUCCESS ) {
                      mmPrintPortErr( 0, "trcCreateVideoChannel returned failure code %d\n", ret);
                      break;
                  } else {
                      mmPrint( 0, T_GWAPP, "trcCreateVideoChannel allocated the simplex channel.\n");
                      mmPrint( 0, T_GWAPP, "New channel ID=%d. TxCoder params. vtpId=%d, IP: %s, port1=%d\n",
                                 pCfg->VideoXC.xcChanId, vtpId, txCoderParam.ipAddr,
                                 txCoderParam.RxPort1 );
                      pCfg->VideoXC.xcChanConfig.params1.RxPort = txCoderParam.RxPort1;
                      strcpy( pCfg->VideoXC.ipAddr, txCoderParam.ipAddr );
                  }                    
                
                mmPrint(0, T_GWAPP, "Start Simplex Video Channel <%d>. Rx1=%d, Dest1=%d,  DestIP1=%s\n",
                        pCfg->VideoXC.xcChanId,
                        pCfg->VideoXC.xcChanConfig.params1.RxPort,
                        pCfg->VideoXC.xcChanConfig.params1.DestPort,
                        pCfg->VideoXC.xcChanConfig.params1.DestIP);            
               ret = trcStartVideoChannel( pCfg->VideoXC.xcChanId,
                                            &pCfg->VideoXC.xcChanConfig );
               Flow(0, F_APPL, F_TRC, "trcStartVideoChannel");                            
  	            if( ret != TRC_SUCCESS ) {
                    mmPrintPortErr( 0, "trcStartVideoChannel failed to start the channel. rc = %d\n", ret);
                }                                    
                break;
				
            case 'f': // stop TRC channel
                display_menu = FALSE;                
                mmPrint(0, T_GWAPP, "Stop Simplex Video Channel <%d>. \n",pCfg->VideoXC.xcChanId);
               ret = trcStopVideoChannel( pCfg->VideoXC.xcChanId );
               Flow(0, F_APPL, F_TRC, "trcStopVideoChannel");                                        
               if( ret != TRC_SUCCESS ) {
                 mmPrintPortErr(0, "trcStopVideoChannel failed. ret = %d\n", ret);
                 mmPrint(0, T_GWAPP, "Trying to destroy the Video Channel ID=0x%x...\n",
                         pCfg->VideoXC.xcChanId );
               }

			   
/*               
               ret = WaitForSpecificEvent( 0, pCfg->hCtaQueueHd, TRCEVN_STOP_CHANNEL_DONE, &event );
		         if ( ret != 0 )
		            printf("\nERROR: trcStopVideoChannel failed to send completion event\n");     
*/                
               break;
                   
            default:   // Any other key
                printf("\n\tThe option selected is not valid.\n");
                display_menu = FALSE;
                break;
        }  // End Switch

    } // End While
#else
	printf("the option selected is not valid (TRC is not supported)\n");
#endif	
    return;

}

DWORD configureVideoRtpEps( int dGwIndex, WORD srvDestPort, char *srvDestIPAddr )
{
	msp_ENDPOINT_RTPFDX_CONFIG mCONFIG;
	CTA_EVENT   event;
	DWORD   ret, expectedEvent, dIP;

	//----------------------------------------
	// SERVER Video
	//----------------------------------------
    disableVideoEP( dGwIndex, VideoCtx[dGwIndex].rtpEp.hd );
																									
	memset( &mCONFIG, 0xFF, sizeof( msp_ENDPOINT_RTPFDX_CONFIG ) );						
	mCONFIG.remoteUDPPort = H2NMS_WORD(srvDestPort); 
    dIP = inet_addr(srvDestIPAddr);
    memcpy( &mCONFIG.remoteIPAddr, &dIP, 4);
       				
	ret = mspSendCommand(VideoCtx[dGwIndex].rtpEp.hd,
							mspBuildCommand(MSP_ENDPOINT_RTPFDX_VIDEO, MSP_CMD_RTPFDX_CONFIG),
							&mCONFIG,
							sizeof( msp_ENDPOINT_RTPFDX_CONFIG ) );
	//Verify correct response from mspSendCommand
	if (ret != SUCCESS)
		printf("\n\t ERROR: mspSendCommand returned failure.\n");
	expectedEvent = (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_RTPFDX_CONFIG);
	ret = WaitForSpecificEvent( dGwIndex, GwConfig[dGwIndex].hCtaQueueHd, expectedEvent, &event );
	if ( ret != 0 ) {
		printf("\n\tERROR: mspSendCommand failed to send valid completion event\n");     
	}

    enableVideoEP( dGwIndex, VideoCtx[dGwIndex].rtpEp.hd );

/*
	disableVideoChannel( dGwIndex, GwConfig[dGwIndex].VidChan.hd );	
	enableVideoChannel( dGwIndex, GwConfig[dGwIndex].VidChan.hd );										
*/
	return SUCCESS;
}


DWORD configurePlayWithXc( int nGw )
{
	MEDIA_GATEWAY_CONFIG *pCfg;
    DWORD ret;
        	
	// Get a pointer to the gateway's configuration
	pCfg = &GwConfig[nGw];        
	
#ifdef USE_TRC						      
    configureVideoRtpEps( nGw, pCfg->VideoXC.xcChanConfig.params1.RxPort,
                          pCfg->VideoXC.ipAddr );

    ret = trcConfigVideoChannel( pCfg->VideoXC.xcChanId,
                                 TRCCMD_GENERATE_IFRAME, 1, NULL, 0 );
    Flow(nGw, F_APPL, F_TRC, "trcConfigVideoChannel");                                        
    if(ret != SUCCESS) {
        mmPrintPortErr( nGw, "trcConfigVideoChannel (Generate I-Frame) failed. ret = %d\n", ret);
    }                                   
#endif // USE_TRC
       
    pCfg->bGwVtpServerConfig = TRUE;   
    
    return SUCCESS;
}

DWORD configurePlayWithoutXc( int nGw )
{
	MEDIA_GATEWAY_CONFIG *pCfg;
    	
	// Get a pointer to the gateway's configuration
	pCfg = &GwConfig[nGw];        
    
    configureVideoRtpEps( nGw, pCfg->VidRtpEp.Addr.EP.RtpRtcp.nSrcPort,
                          pCfg->VidRtpEp.Addr.EP.RtpRtcp.SrcIpAddress );
    pCfg->bGwVtpServerConfig = FALSE;       
    
    return SUCCESS;    
}

DWORD PlayFromDB( int nGw, char *fileName, tVideo_Format fileFormat )
{ 
	MEDIA_GATEWAY_CONFIG *pCfg;
    	
	pCfg = &GwConfig[nGw];           
              
    switch( fileFormat ) {
        case MPEG4:            
            // If the the negotiated via H245 video codec is different than MPEG4 
            // and the current EPs configuration is not GW-VTP-Server          
            if( pCfg->txVideoCodec != H324_MPEG4_VIDEO ) {
                if( !pCfg->bGwVtpServerConfig ) {
                    // Configure EPs GW-VTP-Server
                    configurePlayWithXc( nGw );
                }
            } else { // if pCfg->txVideoCodec == H324_MPEG4_VIDEO
                if( pCfg->bGwVtpServerConfig ) {
                    // Configure EPs Gw-Server 
                    configurePlayWithoutXc( nGw );
                }                
            }
         
            strcpy(VideoCtx[nGw].threegp_FileName, fileName); 
			mmParm.Operation = PLAY;
            
			displayOperation(nGw,"Start PLAY", fileName, fileFormat);
    		PlayRecord( nGw, fileFormat );
            
            break;
            
        case H263: 
            // If the the negotiated via H245 video codec is different than H263 
            // and the current EPs configuration is not GW-VTP-Server          
            if( pCfg->txVideoCodec != H324_H263_VIDEO ) {
                if( !pCfg->bGwVtpServerConfig ) {
                    // Configure EPs GW-VTP-Server
                    configurePlayWithXc( nGw );
                }
            } else { // if pCfg->txVideoCodec == H324_H263_VIDEO
                if( pCfg->bGwVtpServerConfig ) {
                    // Configure EPs Gw-Server 
                    configurePlayWithoutXc( nGw );
                }                
            }
            
            strcpy(VideoCtx[nGw].threegp_FileName, fileName); 
			   
			mmParm.Operation = PLAY;
            
			displayOperation(nGw,"Start PLAY", fileName, fileFormat);           
    		PlayRecord( nGw, fileFormat );
    		
            break;
        default:
            printf("PlayFromDB ERROR: Invalid file format ==> %d\n", fileFormat );
            break;
    }
        
    return SUCCESS;
}

DWORD PlayFromPlayList( int nGw, char *fname )
{
    FILE *fp;
    int i;    
    tVideo_Format fileFormat;
        
    fp = fopen( fname, "r" );
    if( fp == NULL ) {
    	mmPrintPortErr( nGw, "Failed to open the following playlist: %s\n", fname );       
        return -1;
    }

    i = 0;      
	mmPrint( nGw, T_SRVAPP, "Reading the PlayList: %s\n", fname );    
	while( fgets(VideoCtx[nGw].playList.fname[i], 256, fp) != NULL ) {
        VideoCtx[nGw].playList.fname[i][ strlen(VideoCtx[nGw].playList.fname[i])-1 ] = '\0';
        mmPrint( nGw, T_SRVAPP,"\t%d. %s\n", i+1, VideoCtx[nGw].playList.fname[i] );
        i++;
    }
    VideoCtx[nGw].playList.numFiles = i;
    VideoCtx[nGw].playList.currFile = 0;    
    VideoCtx[nGw].bPlayList = TRUE;
    
    if( VideoCtx[nGw].playList.numFiles == 0 ) {
        mmPrint( nGw, T_SRVAPP,"The PlayList is empty.\n");
        return 0;
    }
        
    // Play the first file from the list
    fileFormat = (tVideo_Format)getFileFormat( VideoCtx[nGw].playList.fname[0] );
    if( fileFormat == -1 ) {
        mmPrintPortErr( nGw, "Illegal file name: %s\n", VideoCtx[nGw].playList.fname[0] );
    } else {
        PlayFromDB( nGw, VideoCtx[nGw].playList.fname[0], fileFormat );
    }
        
    fclose( fp );
    
    return SUCCESS;
}

void cmdPlayRecord(void)
{
    char    selection;
    BOOL display_menu = TRUE;
    tVideo_Format fileFormat;
    char fileName[256];
       	
	while( display_menu ) {
        printf(
        "\t=====================================================================\n"\
        "\t      PLAY/RECORD COMMAND MENU                                       \n"\
        "\t      ========================                                       \n"\
        "\tSelect one of the following play/record commands.                    \n"\
        "\t  a)  Exit Play/Rec Command Menu      b)  Help (reprint this menu)   \n");
	    if( ( mmParm.Audio_RTP_Format != NO_AUDIO) && 
     	 ( (AudioCtx[0].pr_mode == PR_MODE_ACTIVE1) || (AudioCtx[0].pr_mode == PR_MODE_ACTIVE2) ) )
       		 printf("\t  u)  Stop Audio                                          \n");
	    if( ( mmParm.Video_Format != NO_VIDEO) && 
    	  ( (VideoCtx[0].pr_mode == PR_MODE_ACTIVE1) || (VideoCtx[0].pr_mode == PR_MODE_ACTIVE2) ) )
        	printf("\t  v)  Stop Video                                          \n");
       	printf("\t  s)  Start Play                                          \n");
	    if( (AudioCtx[0].pr_mode != PR_MODE_ACTIVE1) && (AudioCtx[0].pr_mode != PR_MODE_ACTIVE2) )				
	       	printf("\t  r)  Start Record                                        \n");					
       	printf("\t  t)  Play file from the database                                   \n");        
       	printf("\t  l)  Play files from the PlayList                                  \n");                
        printf("\t=====================================================================\n");
		
        printf("\tSelection> ");
        fflush(stdin);
        scanf("%c", &selection);
        switch (selection)
	    {
		    case 'a':  //Quit
                display_menu = FALSE;
                break;

            case 'b':  //Display Menu
                display_menu = TRUE;
                break;

            case 'u':  //Stop audio
                display_menu = FALSE;
			    if( (mmParm.Audio_RTP_Format != NO_AUDIO) && 
			      ( (AudioCtx[0].pr_mode == PR_MODE_ACTIVE1) || (AudioCtx[0].pr_mode == PR_MODE_ACTIVE2) ) )
			        	cmdStopAudio(0);				
                break;

            case 'v':  //Stop Video
                display_menu = FALSE;			
			    if( (mmParm.Video_Format != NO_VIDEO) && 
				      ( (VideoCtx[0].pr_mode == PR_MODE_ACTIVE1) || (VideoCtx[0].pr_mode == PR_MODE_ACTIVE2) ) )
				        cmdStopVideo(0);			
                break;

            case 's':  //Start Play
               tVideo_Format fileFormat;
               
               if( GwConfig[0].bGwVtpServerConfig ) {
                   // Configure EPs Gw-Server 
                   configurePlayWithoutXc( 0 );
               }                

               // Restore the original file name in case some other files were played
               // from the database
    
               fileFormat = (tVideo_Format)getFileFormat( VideoCtx[0].threegp_FileName );
               displayOperation(0, "Start PLAY", VideoCtx[0].threegp_FileName, fileFormat);
            
               display_menu = FALSE;			
				   mmParm.Operation = PLAY;
				   
			   PlayRecord( 0, fileFormat );
               break;

            case 'r':  //Start Record
               display_menu = FALSE;			
			   mmParm.Operation = RECORD;

               // Restore the original file name in case some other files were played
               // from the database

               strcpy(VideoCtx[0].threegp_FileName, mmParm.threegpFileName);					
 		       displayOperation(0, "Start RECORD", VideoCtx[0].threegp_FileName, convertVideoCodec2Format(GwConfig[0].txVideoCodec));
				   
			   PlayRecord( 0, convertVideoCodec2Format(GwConfig[0].txVideoCodec) );
				
			   h324VideoFastUpdate( GwConfig[0].MuxEp.hd );
               break;

            case 't':  //Play file from the database
                display_menu = FALSE;			
                printf("\tFile name> ");
                fflush(stdin);
                scanf("%s", fileName);
                
                fileFormat = (tVideo_Format)getFileFormat( fileName );
                if( fileFormat == -1 ) {
                    printf("\tERROR: Illegal file name: %s\n", fileName);
                } else {
                    printf("\tFile format: %d\n", fileFormat);               
                    PlayFromDB( 0, fileName, fileFormat );
                }
                break;                
                
            case 'l':  //Play multiple files from the PlayList
                display_menu = FALSE;			
                printf("\tPlayList file name> ");
                fflush(stdin);
                scanf("%s", fileName);
                
                PlayFromPlayList( 0, fileName );
                break;
                
            default:   // Any other key
                printf("\n\tThe option selected is not valid.\n");
                display_menu = FALSE;
                break;
        }  // End Switch
	}  // End While
}            

/*********************************************************************************
  void listConfig()

  Function to list the current configuration of a 
**********************************************************************************/            
void listConfig()
{
    DWORD   first_gw, last_gw;
    WORD    i, value;

    // First and last gateways to List Configuration On
    first_gw = 0;
    last_gw = GwConfig[0].dNumGW - 1;
    // If there are multiple gateways, let the operator choose which on to command
    if(GwConfig[0].dNumGW > 1)
    {
        printf("\nEnter the number of the GW List "
            "(or enter 99 to list all gateways) > ");
        fflush(stdin);
        scanf("%hd", &value); 
        if(value != 99)
        {
            first_gw = value;
            last_gw = value;
        }
    }
    for(i=(WORD)first_gw; i <= (WORD)last_gw; i++)   //Send querys to each of the channels
    {
        printf("\nGW %2d", i);
        printf("\n    MUX OUT      %2d:%3d  ===>  %2d:%3d     TRUNK", 
                16, GwConfig[i].MuxEp.Addr.EP.Mux.nTimeslot, 
                1+4*(GwConfig[i].MuxEp.Addr.EP.Mux.nTimeslot/24),
                GwConfig[i].MuxEp.Addr.EP.Mux.nTimeslot%24);
        printf("\n    MUX IN       %2d:%3d  <===  %2d:%3d     TRUNK", 
                17, GwConfig[i].MuxEp.Addr.EP.Mux.nTimeslot, 
                4*(GwConfig[i].MuxEp.Addr.EP.Mux.nTimeslot/24),
                GwConfig[i].MuxEp.Addr.EP.Mux.nTimeslot%24);
        printf("\n");
		
        printf("\n    VIDEO RTP    %s:%5d <==> %s:%5d   REMOTE VIDEO",
                GwConfig[i].VidRtpEp.Addr.EP.RtpRtcp.SrcIpAddress, 
                GwConfig[i].VidRtpEp.Addr.EP.RtpRtcp.nSrcPort,
                GwConfig[i].VidRtpEp.Addr.EP.RtpRtcp.DestIpAddress,
                GwConfig[i].VidRtpEp.Addr.EP.RtpRtcp.nDestPort);

        printf("\n    AUDIO RTP    %s:%5d <==> %s:%5d   REMOTE AUDIO (AMR)\n",
                GwConfig[i].AudRtpEp.Addr.EP.RtpRtcp.SrcIpAddress, 
                GwConfig[i].AudRtpEp.Addr.EP.RtpRtcp.nSrcPort,
                GwConfig[i].AudRtpEp.Addr.EP.RtpRtcp.DestIpAddress,
                GwConfig[i].AudRtpEp.Addr.EP.RtpRtcp.nDestPort);

    }// End For each gateway   
/*      Output should look like this:
GW 0
    MUX OUT      16:0   ===>  1:0     TRUNK 
    MUX IN       17:0   <===  2:0     TRUNK
    VIDEO RTP    10.1.8.85:16388 <==> 10.1.8.234:16388   REMOTE VIDEO
    AUDIO RTP    10.1.8.85:16388 <==> 10.1.8.234:16384   REMOTE AUDIO (G711)
*/
} // End listConfig()



/*********************************************************************************
  qryAmrByChan

  Function to display a menu of all the mspSendQuerys which can be sent to a
  AMR Bypass Channel, and then execute these queries.
**********************************************************************************/
void qryAmrByChan()
{
    DWORD query, expectedEvent, ret, first_gw, last_gw;
    WORD    i, value;
    CTA_EVENT   event;
    MSP_ACTION  mspAction;

    // First and last gateways to send commands to
    first_gw = 0;
    last_gw = GwConfig[0].dNumGW - 1;
    // If there are multiple gateways, let the operator choose which on to command
    if(GwConfig[0].dNumGW > 1)
    {
        printf("\nEnter the number of the GW to send a query to "
            "(or enter 99 to send to all gateways) > ");
        fflush(stdin);
        scanf("%hd", &value); 
        if(value != 99)
        {
            first_gw = value;
            last_gw = value;
        }
    }

    printf("\nSending a status query to the AMR Bypass channel.");
    query = mspBuildQuery(MSP_FILTER_JITTER, 
        MSP_QRY_JITTER_GET_STATE);
    for(i=(WORD)first_gw; i <= (WORD)last_gw; i++)   //Send querys to each of the channels
    {
        ret = mspSendQuery( GwConfig[i].AudByChan.hd, query );
        if (ret != SUCCESS)
            printf("\n\t ERROR: mspSendQuery failed.\n");
        expectedEvent = (MSPEVN_QUERY_DONE | MSP_QRY_JITTER_GET_STATE);
        ret = WaitForSpecificEvent( i, GwConfig[i].hCtaQueueHd, expectedEvent, &event );
		if ( ret != 0 )
		{
			printf("\n\tERROR: mspSendQuery failed to send valid completion event\n");     
		}
    
        //Record the query to file.
        if ( RecCmdFp !=NULL)
        {
            memset(&mspAction, '\0', sizeof(mspAction));
            mspAction.GWNo      = i;
            mspAction.objType   = MSP_FILTER_JITTER;
            mspAction.action    = MSP_SEND_QRY;
            mspAction.command   = query;
            mspAction.expectedEvent = expectedEvent;

        
            if( fwrite(&mspAction, 1, sizeof(mspAction), RecCmdFp) != sizeof(mspAction))
            {
                printf("\n\tERROR: Failed to properly write command to file.\n");
            }
            fflush(RecCmdFp);
        }
    }
}



/*********************************************************************************
  ProcessCommand

  Function process the top level command or query request, and call a submenu.
**********************************************************************************/
void ProcessCommand(char key)
{
    	
	switch (key)
	{
        case 'B':  //Print Help
            PrintCmdMenu();
            break;
        case 'K':   //Video RTP EP Command
            cmdVidRtpEp();
            break;
        case 'L':   //Video RTP EP Query
            qryVidRtpEp();
            break;
        case 'Q':   //Video Channel Command
            cmdVidChan();
            break;
        case 'R':   //Video Channel Query
            qryVidChan();
            break;
        case 'S':   //AMR Bypass RTP EP Command
            cmdAudRtpEp();
            break;
        case 'T':   //AMR Bypass RTP EP Query
            qryAudRtpEp();
            break;
        case 'U':   //AMR Bypass Channel Command 
            cmdAmrByChan();
            break;
        case 'V':   //AMR Bypass Channel Query
            qryAmrByChan();
            break;
        case 'W':   //List the current configuration of a gateway
            listConfig();
            break;
        case 'X':   //List the current configuration of a gateway
            cmdIsdnCall();
            break;
        case 'Z':   //Play/Record commands
            cmdPlayRecord();
            break;
        case 'Y':   //H245 Commands
            cmdH245();
            break;
        case '0':   //TRC Commands
            cmdTrc();
            break;
            
        default:
            printf("\n\n\tCommand Entered is not currently supported.\n\n");
            break;
    } //End Switch
    return;
} //End ProcessCommand



/*********************************************************************************
  PrintCmdMenu

  Prints the available menu of commands, depending on what type of endpoints and 
  channels are currently active.
**********************************************************************************/
void PrintCmdMenu()
{
    printf(COMMAND_MENU);

    printf("\t  k)  Video RTP EP Command  l)  Video RTP EP Query  \n");
    if(GwConfig[0].VidChan.bActive)
        printf("\t  r)  Video Channel Query \n");
    printf("\t  s)  AMR Bypass EP Command t)  AMR Bypass EP Query \n");
    if(GwConfig[0].AudByChan.bActive)
        printf("\t  u)  AMR Bypass Chnl Cmd   v)  AMR Bypass Chnl Query \n");
    if(GwConfig[0].CallMode == CALL_MODE_ISDN)
        printf("\t  w)  List Full Config      x)  Call Control \n");
    else
        printf("\t  w)  List Full Configuration \n");

    printf("\t  y)  H245 Commands         z)  Play/Record Commands \n");
    printf("\t  0)  TRC Commands \n");		
    printf("\t=====================================================================\n\n");

    return;
}

void autoPlaceCall( int gw_num, char *dial_number )
{
    CTA_EVENT NewEvent;  
    PBX_PLACE_CALL_STRUCT pbxPlaceCall;

	strcpy( pbxPlaceCall.calledaddr, dial_number );
    pbxPlaceCall.callingaddr[0] = '\0';
    pbxPlaceCall.callingname[0] = '\0';
    NewEvent.id                 = PBX_EVENT_PLACE_CALL;
    NewEvent.value              =  gw_num; //SUCCESS;
    NewEvent.size               = sizeof(PBX_PLACE_CALL_STRUCT);
    NewEvent.ctahd = GwConfig[ gw_num ].isdnCall.ctahd;
	
	ctaAllocBuffer( &(NewEvent.buffer), sizeof(PBX_PLACE_CALL_STRUCT) );
    memcpy( NewEvent.buffer, &pbxPlaceCall, sizeof(PBX_PLACE_CALL_STRUCT) );

    ctaQueueEvent(&NewEvent);
}

void autoDisconnectCall( int gw_num )
{
    CTA_EVENT NewEvent;  

	NewEvent.id       = PBX_EVENT_DISCONNECT;
	NewEvent.value    = SUCCESS;
	NewEvent.buffer   = NULL;
	NewEvent.size     = 0;
    NewEvent.ctahd    = GwConfig[ gw_num ].isdnCall.ctahd;
	
    ctaQueueEvent(&NewEvent);	
}

/*********************************************************************************
  CommandLoop

  This is the main loop of the Medai Gateway Sample App.  Once initialized, the
  program spends most of it's time in this loop either waiting for events, or 
  processing user commands.
**********************************************************************************/
void *CommandLoop( void *param )
{
	BOOL  break_flag = FALSE;
	int   event_type, i, *pGwNum, nGw;
	DWORD ret;
	char  key;
    CTA_EVENT event;

	pGwNum = (int *)param;
	nGw = *pGwNum;

	if( gwParm.auto_place_call ) {
			autoPlaceCall(*pGwNum, "2000" ); //gwParm.dial_number);
	}	

	// Print out the command menu (only once)
	if( gwParm.nPorts == 1 )
	    PrintCmdMenu();	
			
	//Main Loop
	while (break_flag == FALSE)
	{
		//Loop until either a keyboard event or CTA event
		for (;;)
		{
            //fflush (stdin);
			memset (&event, 0, sizeof (CTA_EVENT));

            ret = ctaWaitEvent( GwConfig[nGw].hCtaQueueHd, &event, 500 );  //100 ms timeout

			if (ret != SUCCESS) {
				mmPrintPortErr( *pGwNum, "ctaWaitEvent failure, ret= 0x%x, event.value= 0x%x, board error (event.size field) %x",
					 ret, event.value, event.size);
			} else {
				if (event.id == CTAEVN_WAIT_TIMEOUT) {

				    //Didn't get a CTA event
					if( gwParm.nPorts == 1 ) {
					    if (kbhit()) {
					    	event_type = MEGA_KBD_EVENT;
					    	break;
					    } else {
					    	continue;
					    }
					}

				} else { //Got a CTA event
					event_type = MEGA_CTA_EVENT;
					break;
				}
			}
		} //End for

		if (event_type == MEGA_KBD_EVENT)
		{
			key = getchar();
			key = toupper(key);
            fflush(stdin);
			if (key == 'A')
			{
				break_flag = TRUE;
			}
			else
			{
				ProcessCommand(key);
                //Print out the command menu
                PrintCmdMenu();
			}
		}
		else  //A CTA Event
		{
			BOOL consumed;
			ret = h324SubmitEvent(&event, &consumed);
			if(ret != SUCCESS)
				mmPrint(nGw, T_GWAPP,"h324SubmitEvent returned 0x%x\n",ret);
			if(!consumed)
				break_flag = ProcessCTAEvent(nGw, &event);  
		}

	} //End While

	return NULL;
}

// Get the video file name according to the extension (.3gp )
int getFileFormat( char *fileName )
{
    int fileFormat;    
    char *pExt;

    if( fileName == NULL ) 
        return -1;
    
    pExt = strrchr(fileName, '.');
    if( pExt == NULL || pExt[1] == '\0' ) 
        return -1;       
    
	if( strcmp(pExt+1,THREEGP_FNAME_EXT) == 0 ) {
         NMS_FORMAT_TO_3GP_DESC fdesc;
         NMS_AUDIO_DESC audiodesc;
         NMS_VIDEO_DESC videodesc;
         DWORD ret;
         
         ret = get3GPInfo     (  fileName,             
                     &audiodesc,  
   			         &videodesc, 
                     NULL,
   			         0 );
            
         if (ret != FORMAT_SUCCESS)
         {
            mmPrint(0, T_GWAPP,"Failed to Get 3GP info from file %s, error %x\n",  fileName, ret);
            return -1;
         }
   
        return (videodesc.codec);
    }     
	return -1;    
}

// Convert video codec to video format
tVideo_Format convertVideoCodec2Format( DWORD VideoCodec )
{

   if (VideoCodec == H324_MPEG4_VIDEO)
      return (MPEG4);
   else if (VideoCodec == H324_H263_VIDEO)
      return (H263);
   else
      return (NO_VIDEO);
      
   
}


void displayOperation(int nGw, char *operation, char *fileName, int fileFormat)
{
   char buf_format[20];
   
   get_video_format_string( fileFormat, buf_format );
	mmPrint(nGw, T_H245, "%s. Name=%s, Format=%s\n\n", operation, fileName, buf_format);
}			

DWORD closeAllChannels(int nGw)
{   
   CTA_EVENT   event;
   DWORD   ret;
   
   Flow(nGw, F_APPL, F_H324, "h324CloseChannel(audio in)"); 
   ret = h324CloseChannel( GwConfig[nGw].MuxEp.hd, H324RSN_AUDIO_IN );
   if( ret != SUCCESS )    
      return ret;

   ret = WaitForSpecificEvent( nGw, GwConfig[nGw].hCtaQueueHd, H324EVN_CHANNEL_CLOSED, &event );
   if ( ret != 0 )
   {
      printf("\nERROR: h324CloseChannel Audio IN failed to get remote ACK\n");  
      return ret;
   }
   
   Flow(nGw, F_APPL, F_H324, "h324CloseChannel(audio out)");
   ret = h324CloseChannel( GwConfig[nGw].MuxEp.hd, H324RSN_AUDIO_OUT );
   if( ret != SUCCESS )    
      return ret;
           
   ret = WaitForSpecificEvent( nGw, GwConfig[nGw].hCtaQueueHd, H324EVN_CHANNEL_CLOSED, &event );
   if ( ret != 0 )
   {
      printf("\nERROR: h324CloseChannel Audio OUT failed to get remote ACK\n");  
      return ret;
   }   
                                            
   Flow(nGw, F_APPL, F_H324, "h324CloseChannel(video)");
   ret = h324CloseChannel( GwConfig[nGw].MuxEp.hd, H324RSN_VIDEO );
   if( ret != SUCCESS )    
      return ret;
         
   ret = WaitForSpecificEvent( nGw, GwConfig[nGw].hCtaQueueHd, H324EVN_CHANNEL_CLOSED, &event );
   if ( ret != 0 )
   {
      printf("\nERROR: h324CloseChannel Video failed to get remote ACK\n");  
      return ret ;
   }   
   
   Flow(nGw, F_APPL, F_H324, "h324EndSession");
   ret = h324EndSession( GwConfig[nGw].MuxEp.hd );
   if( ret != SUCCESS )    
      return ret;

   ret = WaitForSpecificEvent( nGw, GwConfig[nGw].hCtaQueueHd, H324EVN_END_SESSION_DONE, &event );
   if ( ret != 0 )
   {
      printf("\nERROR: h324EndSession failed to get remote ACK\n");  
      return ret;
   }       
   
}
