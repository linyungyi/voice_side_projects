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
#if defined(WIN32)
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "mega_global.h"
#include "command.h"
#include "gw_types.h"
#include "vm_trace.h"

#include "srv_types.h"
#include "srv_def.h"
#include "mmfi.h"

#define THREEGP_FNAME_EXT   "3gp"

int gExiting = FALSE;

DWORD         disableVideoEP( int nGw, MSPHD hd, DWORD index );
DWORD         enableVideoEP( int nGw, MSPHD hd, DWORD index );
int           getFileFormat( char *fileName );
tVideo_Format convertVideoCodec2Format( DWORD VideoCodec );
void          displayOperation(int nGw, char *operation, char *fileName, int fileFormat);
extern void   get_video_format_string( DWORD format, char *str );
DWORD         closeAllChannels(int nGw);

#ifdef USE_TRC
extern int     getVideoChannelStatus(TRC_HANDLE trchd);
extern DWORD   getFileRes(char *fileName);
#endif
/*********************************************************************************
  cmdPassthruPlayRFC2833()

  Prompt the user for the fields necessary to prepare an RFC2833 Play command
  for the current passthru channel and then send the command via the API
  provided in the H.324M middleware.
**********************************************************************************/
void cmdPassthruPlayRFC2833()
{
   DWORD     gW = 0;
   WORD      nEventID,
             nEventDuration;
   
   printf( "\tA --> Play RFC 2833 Packets \n");
   do
   {
     printf("Please Enter the RFC 2833 Event ID:\n ");
     printf("\tDtmf Digits Events: \n");
     printf("\t  0--9\t\t0--9\n");
     printf("\t  *\t\t10\n");
     printf("\t  #\t\t11\n");
     printf("\t  A--D\t\t12--15\n\n");
     printf("\tANS-type Events: \n");
     printf("\t  ANS\t\t32\n");
     printf("\t  ANS\\\t\t33\n");
     printf("\t  ANSAM\t\t34\n");
     printf("\t  ANSAM\\\t35\n\n");
     printf("\tOthers: \n");
     printf("\t  See RFC 2833 for details.\n");
     printf("\nPlease Enter Event ID: ");
     scanf("%hd",&nEventID);  // Consume trailing newline
     (void)getchar();
  }
  while ((nEventID < 0) || (nEventID > 255));

   printf("Please Enter the RFC 2833 Event duration in ms: ");
   fflush(stdin);
   scanf("%hd",&nEventDuration);
   (void)getchar();  // Consume trailing newline
 
   if (h324PassthruPlayRFC2833(GwConfig[0].MuxEp.hd, nEventID, nEventDuration) != SUCCESS)
   {
      printf("ERROR: h324PassthruPlayRFC2833() failed.\n");     
   }
}

/*********************************************************************************
  cmdPassthruUpdateDTMFMode()

  Prompt the user for the fields necessary to prepare an DTMF mode command
  for the current passthru channel and then send the command via the API
  provided in the H.324M middleware.
**********************************************************************************/
void cmdPassthruUpdateDTMFMode()
{
   int c;
   DWORD gW = 0;
   WORD control, payloadID;

   printf( "\tA --> Set Passthru DTMF Mode \n\n");

   do
   {
     printf("\nPlease enter PayloadID: ");
     scanf("%hd",&payloadID);  // Consume trailing newline
     (void)getchar();
   }
   while((payloadID < 96) || (payloadID > 127));

   do
   {
     printf("\nEnable DTMF shift to sync digit playout? (y/n): ");
     c = tolower(getchar());
     (void)getchar();          // Consume trailing newline
   }
   while(c != 'y' && c != 'n');

   control = (c == 'y') ? DTMF_SHIFT_ENABLED : 0;

   printf("(control: %d (0x%x), payloadID: %d (0x%x))\n", 
          control, control, payloadID, payloadID);

   printf("Using h324PassthruDTMFMode() to send to DEMUX\n");
   if (h324PassthruDTMFMode(GwConfig[0].MuxEp.hd, control, payloadID) != SUCCESS)
   {
      printf("ERROR: h324PassthruDTMFMode() failed.\n");     
   }
}

/*********************************************************************************
  cmdAudPassthru()

  Function to display a menu of all available audio passthru commands.
**********************************************************************************/
void cmdAudPassthru(void)
{
    char selection;
    BOOL display_menu = TRUE;

    // First and last gateways to send commands to
    while(display_menu)
    {
        // TODO Implement Multiple Gateways
        // NOTE LOOP FOR MULTIPLE GATEWAYS NOT IMPLEMENTED HERE
        // Display the options to choose from
        printf("\n\t=====================================================================\n");
        printf("\t      AUDIO PASSTHRU MENU                                            \n");
        printf("\t      ===============================                                \n");
        printf("\tSelect one of the following commands for H245.                       \n");
        printf("\t  a)  Exit Audio Passthru Menu        b)  Help (reprint this menu)   \n");
        printf("\t  c)  Play RFC2833 Event              d)  Update DTMF Mode           \n");
        printf("\t=====================================================================\n");
        printf("\tSelection> ");
        fflush(stdin);
        READ_PARAM("%c", &selection);
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
		cmdPassthruPlayRFC2833();
                break;

            case 'd':  // H245 Round Trip Delay
                display_menu = FALSE;
		cmdPassthruUpdateDTMFMode();
                break;

            default:   // Any other key
                printf("\n\tThe option selected is not valid.\n");
                display_menu = FALSE;
                break;
        }  // End Switch

    } // End While

    return;
}

/*********************************************************************************
  cmdVidRtpEp

  Function to display a menu of all the mspSendCommands which can be sent to a
  Video RTP endpoint, and then execute these commands.
**********************************************************************************/
void cmdVidRtpEp()
{
    DWORD       command, msp_cmd, cmdSize, i;
    DWORD       k, first_gw, last_gw;
    DWORD       ret, expectedEvent, query, dwValue;
    WORD        byteCount = 0, value;
    DWORD       subtype = MSP_ENDPOINT_RTPFDX_VIDEO_MPEG4;   
    //DWORD     subtype = MSP_ENDPOINT_RTP_VIDEO_MPEG4;   
    void       *pCmd;
    char        selection;
    CTA_EVENT   event;
    MSP_ACTION  mspAction;
    BOOL        display_menu    = TRUE;
    BOOL        execute_cmd     = FALSE;
    DWORD       EnableEndpoint  = FALSE, DisableEndpoint = FALSE;
    
    msp_ENDPOINT_RTPFDX_MAP                 ep_map_cmd;
    msp_ENDPOINT_RTPFDX_RTP_PKTSZ_CTRL      pktSzCtrl;

    // First and last gateways to send commands to
    first_gw = 0;
    last_gw  = GwConfig[0].dNumGW - 1;
    
    while (display_menu)
    {
        // If there are multiple gateways, let the operator choose which on to command
        if (GwConfig[0].dNumGW > 1)
        {
            printf("\nEnter the number of the GW to send a command to "
                   "\n(or enter 99 to send to all gateways) > ");
            fflush(stdin);
            READ_PARAM("%hd", &value); 
            if (value != 99)
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
        "\t  i)  Disc. pending partial frames  j)  Query RTP packet size params \n"\
        "\t  l)  Send Out of band DCI command                                   \n"\
        "\t=====================================================================\n");
        printf("\tSelection> ");
        fflush(stdin);
        READ_PARAM("%c", &selection);
        switch (selection)
	    {
		    case 'a':  //Quit
		    case 'x':  //Quit
                execute_cmd  = FALSE;
                display_menu = FALSE;
                break;

            case 'b':  //Display Menu
                execute_cmd  = FALSE;
                display_menu = TRUE;
                break;


            case 'e':  // Enable Endpoint
                EnableEndpoint  = TRUE;
                msp_cmd         = 0;
                display_menu    = FALSE;
                execute_cmd     = TRUE;
                break;

            case 'f':  // Disable Endpoint
                DisableEndpoint  = TRUE;
                msp_cmd          = 0;
                display_menu     = FALSE;
                execute_cmd      = TRUE;
                break;

            case 'g':  // Video Map
                printf("\n\tThe current setting of the Video Map ID = %d,\n\tEnter a new value > ",
                    GwConfig[0].vidRtpEp[EP_OUTB].Param.EP.RtpRtcp.PayloadMap.payload_id);
                fflush(stdin);
                READ_PARAM("%d", &dwValue);
                GwConfig[0].vidRtpEp[EP_OUTB].Param.EP.RtpRtcp.PayloadMap.payload_id = dwValue;
                ep_map_cmd.payload_id = H2NMS_DWORD(dwValue);

                printf("\n\tThe current setting of the Video Map Vocoder = %d,\n\tEnter a new value > ",
                    GwConfig[0].vidRtpEp[EP_OUTB].Param.EP.RtpRtcp.PayloadMap.vocoder);
                fflush(stdin);
                READ_PARAM("%d", &dwValue);
                GwConfig[0].vidRtpEp[EP_OUTB].Param.EP.RtpRtcp.PayloadMap.vocoder = dwValue;
                ep_map_cmd.vocoder = H2NMS_DWORD(dwValue);

                msp_cmd      = MSP_CMD_RTPFDX_MAP;
                pCmd         = &ep_map_cmd;
                cmdSize      = sizeof(ep_map_cmd);
                display_menu = FALSE;
                execute_cmd  = TRUE;
                break;

            case 'i':  // Discard pending partial video frames
                DisableEndpoint  = TRUE;
                EnableEndpoint  = TRUE;
                msp_cmd = MSP_CMD_RTPFDX_DISCARD_PENDING_PFRAMES;
                subtype = MSP_ENDPOINT_RTPFDX_VIDEO_H263;
                pCmd = NULL;
                cmdSize = 0;
                display_menu = FALSE;
                execute_cmd = TRUE;
                break;

            case 'j':  // Query RTP Packet Size Control Info
                subtype = MSP_ENDPOINT_RTPFDX_VIDEO_H263;
                pCmd = &pktSzCtrl;
                cmdSize = sizeof(pktSzCtrl);

                query = mspBuildQuery(MSP_ENDPOINT_RTPFDX_VIDEO_H263, MSP_QRY_RTPFDX_VIDEO_RTP_PKTSZ_CTRL);

                for(k=first_gw; k <= last_gw; k++)   //Send commands to each of the endpoints
                {
                   ret = mspSendQuery(GwConfig[k].vidRtpEp[EP_OUTB].hd, query);
                   if (ret != SUCCESS)
                      printf("\n\t ERROR: mspSendQuery failed.\n");
                   expectedEvent = (MSPEVN_QUERY_DONE | MSP_QRY_RTPFDX_VIDEO_RTP_PKTSZ_CTRL);
                   ret = WaitForSpecificEvent(k, GwConfig[k].hCtaQueueHd, expectedEvent, &event);
                   if (ret != 0)
                   {
                      printf("\n\tERROR: mspSendQuery failed to send valid completion event\n");     
                   }
                   //                   pktSzCtrl = *(msp_ENDPOINT_RTPFDX_RTP_PKTSZ_CTRL *)event.buffer;
                   pktSzCtrl = *(msp_ENDPOINT_RTPFDX_RTP_PKTSZ_CTRL *)(((char *)event.buffer)+sizeof(DWORD));
   
                   // PRINT the RTP pktsz parameters from the board.
                   pktSzCtrl.pktMaxSz = NMS2H_DWORD(pktSzCtrl.pktMaxSz);
                   pktSzCtrl.aggThreshold = NMS2H_DWORD(pktSzCtrl.aggThreshold);
                   pktSzCtrl.enableAggregation = NMS2H_DWORD(pktSzCtrl.enableAggregation);
                   printf("\n\tQuery RTP Maximum Packet Size Control values:\n");
                   printf("\t\tRTP maximum packet size:   %d bytes\n", pktSzCtrl.pktMaxSz);
                   printf("\t\tRTP aggregation %s:  (flag = %d)\n", pktSzCtrl.enableAggregation ? " ENABLED" : "DISABLED",
                          pktSzCtrl.enableAggregation);
                   if (pktSzCtrl.enableAggregation)
                      printf("\t\tRTP aggregation threshold: %d bytes\n", pktSzCtrl.aggThreshold);
                   printf("\n");
                }

                // Query returns a CTA buffer; release it.
                mspReleaseBuffer(GwConfig[k].hCtaQueueHd, event.buffer);

                // DON'T do the normal post-processing: disable, command, or enable the EP.
                DisableEndpoint  = FALSE;
                EnableEndpoint  = FALSE;
                execute_cmd = FALSE;
                display_menu = FALSE;
                break;

            case 'k':  // Control I-Frame notification
                msp_ENDPOINT_RTPFDX_IFRAME_NOTIFY_CTRL notifyCmd;

                DisableEndpoint = TRUE;
                EnableEndpoint  = TRUE;
                
                msp_cmd = MSP_CMD_RTPFDX_IFRAME_NOTIFY_CTRL;
                subtype = MSP_ENDPOINT_RTPFDX_VIDEO_H263;
                pCmd    = &notifyCmd;
                cmdSize = sizeof(notifyCmd);
                printf("\tControl I-Frame notification: 0=disable, 1=NEXT I-Frame, 2=EACH I-Frame\n\n");
                for ( ; ; )
                {
                    printf("\t\tNotification of I-Frames from H.324M (0-2)> ");
                    while ((selection = getchar()) == '\n')
                    {
                        /* Discard leading newlines */ ;
                    }
                    (void)getchar();  /* Discard newline trailing the character requested */
                    if (selection == '0' || selection == '1' || selection == '2') 
                    {
                       break;
                    }
                    printf("\n\tSelection must be '0' or '1' or '2'  -- try again.\n");
                }
                selection           -= '0';
                notifyCmd.h324Notify = H2NMS_DWORD(selection);
                for ( ; ; )
                {
                    printf("\t\tNotification of I-Frames from RTP/IP (0-2)> ");
                    while ((selection = getchar()) == '\n')
                    {
                        /* Discard leading newlines */ ;
                    }
                    (void)getchar();  /* Discard newline trailing the character requested */
                    if (selection == '0' || selection == '1' || selection == '2') 
                    {
                       break;
                    }
                    printf("\n\tSelection must be '0' or '1' or '2'  -- try again.\n");
                }
                selection         -= '0';
                notifyCmd.ipNotify = H2NMS_DWORD(selection);
                display_menu       = FALSE;
                execute_cmd        = TRUE;
                break;
       
            case 'l':  // Control I-Frame notification
                msp_ENDPOINT_RTPFDX_OUT_OF_BAND_DCI dciCmd;

                DisableEndpoint  = TRUE;
                EnableEndpoint  = TRUE;
                msp_cmd = MSP_CMD_RTPFDX_OUT_OF_BAND_DCI;
                subtype = MSP_ENDPOINT_RTPFDX_VIDEO_MPEG4;

                for(i = 0;i< GwConfig[0].DCI.len;i++)
                {
                    dciCmd.data[i] = GwConfig[0].DCI.data[i];
                }
                dciCmd.len =H2NMS_DWORD(GwConfig[0].DCI.len);
                dciCmd.mode = 1;
                for ( ; ; )
                {
                    printf("\t\tChoose the out of band DCI mode(0-3) ");
                    while ((selection = getchar()) == '\n')
                        /* Discard leading newlines */ ;
                    (void)getchar();  /* Discard newline trailing the character requested */
                    if ( selection == '0' || selection == '1' || selection == '2' || selection == '3')
                    {
                        selection -= '0';
                        dciCmd.mode =  (U8)(selection);
                        break;
                    }
                    printf("\n\tSelection must be '0''1' or '2' or '3'  -- try again.\n");
                }

                pCmd = &dciCmd;
                cmdSize = sizeof(dciCmd);
                printf("\tDCI  notification: \n");
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
            for (k=first_gw; k <= last_gw; k++)   //Send commands to each of the endpoints
            {
                //Disable Endpoint for Special Header Changes
                if (DisableEndpoint == TRUE)
                {
                    ret = mspDisableEndpoint(GwConfig[k].vidRtpEp[EP_OUTB].hd);
                    if (ret != SUCCESS)
                    {
                        printf("\n\t ERROR: mspDisableEndpoint returned failure.\n");
                    }
                    ret = WaitForSpecificEvent( k, GwConfig[k].hCtaQueueHd, MSPEVN_DISABLE_ENDPOINT_DONE, &event );
		            if ( ret != 0 )
                    {
		               printf("\nERROR: mspDisableEndpoint failed to send completion event\n");     
                    }
                }

                if(msp_cmd !=0)
                {
                    ret = mspSendCommand(GwConfig[k].vidRtpEp[EP_OUTB].hd,
                                        command,
                                        pCmd,
                                        cmdSize  );
                    //Verify correct response from mspSendCommand
                    if (ret != SUCCESS)
                    {
                        printf("\n\t ERROR: mspSendCommand returned failure.\n");
                    }
                    expectedEvent = (MSPEVN_SENDCOMMAND_DONE | msp_cmd);
                    ret = WaitForSpecificEvent( k, GwConfig[k].hCtaQueueHd, expectedEvent, &event );
		            if ( ret != 0 )
		            {
			            printf("\n\tERROR: mspSendCommand failed to send valid completion event\n");     
		            }
                }

                //Enable Endpoint for Special Header Changes
                if (EnableEndpoint == TRUE)
                {
                    ret = mspEnableEndpoint(GwConfig[k].vidRtpEp[EP_OUTB].hd);
                    if (ret != SUCCESS)
                    {
                        printf("\n\t ERROR: mspEnableEndpoint returned failure.\n");
                    }
                    ret = WaitForSpecificEvent( k, GwConfig[k].hCtaQueueHd, MSPEVN_ENABLE_ENDPOINT_DONE, &event );
		            if ( ret != 0 )
                    {
		               printf("\nERROR: mspEnableEndpoint failed to send completion event\n");     
                    }
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
                
                    if ( fwrite(&mspAction, 1, sizeof(mspAction), RecCmdFp) != sizeof(mspAction))
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
    DWORD       query,
                expectedEvent,
                ret,
                first_gw,
                last_gw;
    WORD        i,
                value;
    CTA_EVENT   event;
    MSP_ACTION  mspAction;

    // First and last gateways to send commands to
    first_gw = 0;
    last_gw  = GwConfig[0].dNumGW - 1;
    
    // If there are multiple gateways, let the operator choose which one to command
    if (GwConfig[0].dNumGW > 1)
    {
        printf("\nEnter the number of the GW to send a query to "
               "(or enter 99 to send to all gateways) > ");
        fflush(stdin);
        READ_PARAM("%hd", &value); 
        if (value != 99)
        {
            first_gw = value;
            last_gw = value;
        }
    }

    printf("\nSending an assembly status query to the video RTP EP.");
    query = mspBuildQuery(MSP_ENDPOINT_RTPFDX_VIDEO_MPEG4, 
                          MSP_QRY_RTPFDX_VIDEO_MPEG4_ASSY_STATUS);
    for (i=(WORD)first_gw; i <= (WORD)last_gw; i++)   //Send querys to each of the channels
    {
        ret = mspSendQuery( GwConfig[i].vidRtpEp[EP_INB].hd, query );
        if (ret != SUCCESS)
        {
            printf("\n\t ERROR: mspSendQuery failed.\n");
        }
        expectedEvent = (MSPEVN_QUERY_DONE | MSP_QRY_RTPFDX_VIDEO_MPEG4_ASSY_STATUS);
        
        ret = WaitForSpecificEvent( i, GwConfig[i].hCtaQueueHd, expectedEvent, &event );
		if ( ret != 0 )
		{
			printf("\n\tERROR: mspSendQuery failed to send valid completion event\n");     
		}
    
        //Record the query to file.
        if ( RecCmdFp !=NULL)
        {
            memset(&mspAction, '\0', sizeof (mspAction));
            mspAction.GWNo          = i;
            mspAction.objType       = MSP_ENDPOINT_RTPFDX_VIDEO_MPEG4;
            mspAction.action        = MSP_SEND_QRY;
            mspAction.command       = query;
            mspAction.expectedEvent = expectedEvent;

        
            if( fwrite(&mspAction, 1, sizeof (mspAction), RecCmdFp) != sizeof (mspAction))
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
    DWORD       command,
                msp_cmd,
                cmdSize;
    DWORD       k,
                first_gw,
                last_gw;
    DWORD       ret,
                expectedEvent;
    WORD        byteCount = 0;
    WORD        value;
    DWORD       dwValue;
    DWORD       subtype = MSP_FILTER_JITTER_VIDEO_MPEG4;   
    void       *pCmd;
    char        selection;
    CTA_EVENT   event;
    MSP_ACTION  mspAction;
    BOOL        display_menu = TRUE;
    BOOL        execute_cmd  = FALSE;

    msp_FILTER_JITTER_VIDMP4_REMOVE_SPECIAL_HDR rm_hdr_cmd;

    // First and last gateways to send commands to
    first_gw = 0;
    last_gw  = GwConfig[0].dNumGW - 1;

    while(display_menu)
    {
        // If there are multiple gateways, let the operator choose which on to command
        if (GwConfig[0].dNumGW > 1)
        {
            printf("\nEnter the number of the GW to send a command to "
                   "\n(or enter 99 to send to all gateways) > ");
            fflush(stdin);
            READ_PARAM("%hd", &value); 
            if (value != 99)
            {
                first_gw = value;
                last_gw  = value;
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
        READ_PARAM("%c", &selection);
        switch (selection)
	    {
		    case 'a':  //Quit
		    case 'x':  //Quit
                execute_cmd  = FALSE;
                display_menu = FALSE;
                break;

            case 'b':  //Display Menu
                execute_cmd  = FALSE;
                display_menu = TRUE;
                break;

            case 'c':  // Remove Special Header
                printf("\n\tSize of Header to Remove > ");
                fflush(stdin);
                READ_PARAM("%d", &dwValue);
                rm_hdr_cmd.size = H2NMS_DWORD(dwValue);
                msp_cmd         = MSP_CMD_VIDMP4_REMOVE_SPECIAL_HDR;
                pCmd            = &rm_hdr_cmd;
                cmdSize         = sizeof (rm_hdr_cmd);
                display_menu    = FALSE;
                execute_cmd     = TRUE;
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
            for (k=first_gw; k <= last_gw; k++)   //Send commands to each of the endpoints
            {
                //Disable Channel for Special Header Changes
                if (msp_cmd == MSP_CMD_VIDMP4_REMOVE_SPECIAL_HDR)
                {
                    ret = mspDisableChannel(GwConfig[k].vidChannel[EP_OUTB].hd);
                    if (ret != SUCCESS)
                    {
                        printf("\n\t ERROR: mspDisableChannel returned failure.\n");
                    }
                    ret = WaitForSpecificEvent(k, GwConfig[k].hCtaQueueHd, 
								MSPEVN_DISABLE_CHANNEL_DONE, &event );
		            if ( ret != 0 )
                    {
		               printf("\nERROR: mspDisableChannel failed to send completion event\n");     
                    }
                }

                ret = mspSendCommand(GwConfig[k].vidChannel[EP_OUTB].hd,
                                    command,
                                    pCmd,
                                    cmdSize  );
                //Verify correct response from mspSendCommand
                if (ret != SUCCESS)
                {
                    printf("\n\t ERROR: mspSendCommand returned failure.\n");
                }
                expectedEvent = (MSPEVN_SENDCOMMAND_DONE | msp_cmd);
                
                ret = WaitForSpecificEvent( k, GwConfig[k].hCtaQueueHd, expectedEvent, &event );
		        if ( ret != 0 )
		        {
			        printf("\n\tERROR: mspSendCommand failed to send valid completion event\n");     
		        }

                //Enable Channel for Special Header Changes
                if(msp_cmd == MSP_CMD_VIDMP4_REMOVE_SPECIAL_HDR)
                {
                    ret = mspEnableChannel(GwConfig[k].vidChannel[EP_OUTB].hd);
                    if (ret != SUCCESS)
                    {
                        printf("\n\t ERROR: mspEnableChannel returned failure.\n");
                    }
                    
                    ret = WaitForSpecificEvent( k, GwConfig[k].hCtaQueueHd, 
								MSPEVN_ENABLE_CHANNEL_DONE, &event );
		            if ( ret != 0 )
                    {
		               printf("\nERROR: mspEnableChannel failed to send completion event\n");     
                    }
                }

                //Record the command to file.
                if ( RecCmdFp !=NULL)
                {
                    memset(&mspAction, '\0', sizeof (mspAction));
                    mspAction.GWNo          = (WORD)k;
                    mspAction.objType       = MSP_VID_CHAN;
                    mspAction.action        = MSP_SEND_CMD;
                    mspAction.command       = command;
                    mspAction.cmdSize       = cmdSize;
                    mspAction.expectedEvent = expectedEvent;
                    memcpy( mspAction.CmdBuff, pCmd, cmdSize );
                
                    if( fwrite(&mspAction, 1, sizeof (mspAction), RecCmdFp) != sizeof (mspAction))
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
    DWORD       query,
                expectedEvent,
                ret,
                first_gw,
                last_gw;
    WORD        i,
                value;
    CTA_EVENT   event;
    MSP_ACTION  mspAction;

    // First and last gateways to send commands to
    first_gw = 0;
    last_gw  = GwConfig[0].dNumGW - 1;
    
    // If there are multiple gateways, let the operator choose which on to command
    if (GwConfig[0].dNumGW > 1)
    {
        printf("\nEnter the number of the GW to send a query to "
               "(or enter 99 to send to all gateways) > ");
        fflush(stdin);
        READ_PARAM("%hd", &value); 
        if (value != 99)
        {
            first_gw = value;
            last_gw  = value;
        }
    }

    printf("\nSending a status query to the video channel.");
    
    for (i=(WORD)first_gw; i <= (WORD)last_gw; i++)   //Send querys to each of the endpoints
    {
        DWORD l_nVideoFilterType;
        // Determine the video filter type for the query
        if(GwConfig[i].vidChannel[EP_INB].Addr.channelType  == MGH263VideoChannel)
            l_nVideoFilterType = MSP_FILTER_JITTER_VIDEO_H263;
        else if(GwConfig[i].vidChannel[EP_INB].Addr.channelType  == MGVideoChannel)
            l_nVideoFilterType = MSP_FILTER_JITTER_VIDEO_MPEG4;
        else
        {
            printf("\n\t ERROR: Video Channel Type unknown\n");
            return;
        }
        query = mspBuildQuery(l_nVideoFilterType, MSP_QRY_JITTER_VIDEO_GET_STATE);
        ret = mspSendQuery( GwConfig[i].vidChannel[EP_INB].hd, query );
        if (ret != SUCCESS)
        {
            printf("\n\t ERROR: mspSendQuery failed.\n");
        }
        expectedEvent = (MSPEVN_QUERY_DONE | MSP_QRY_JITTER_VIDEO_GET_STATE);
        
        ret = WaitForSpecificEvent( i, GwConfig[i].hCtaQueueHd, expectedEvent, &event );
		if ( ret != 0 )
		{
			printf("\n\tERROR: mspSendQuery failed to send valid completion event\n");     
		}
    
        //Record the query to file.
        if ( RecCmdFp !=NULL)
        {
            memset(&mspAction, '\0', sizeof (mspAction));
            mspAction.GWNo          = i;
            mspAction.objType       = (WORD)l_nVideoFilterType; // MSP_FILTER_JITTER_VIDEO_H263
            mspAction.action        = MSP_SEND_QRY;
            mspAction.command       = query;
            mspAction.expectedEvent = expectedEvent;

        
            if ( fwrite(&mspAction, 1, sizeof (mspAction), RecCmdFp) != sizeof (mspAction))
            {
                printf("\n\tERROR: Failed to properly write command to file.\n");
            }
            fflush(RecCmdFp);
        }
    }


}

/*
 * The user wants to do something to either the EP_OUTB or EP_INB simplex endpoint.
 * Find out which one and return either EP_OUTB or EP_INB.
 */
DWORD whichEpDirection()
{
    if (gwParm.simplexEPs)
    {
        for ( ; ; )
        {
            char buf[4], * cp;
            printf("\n\tSend the cmd to the audio TX or audio RX endpoint? [TX]: ");
            fflush(stdin);
            *buf = '\0';
            cp = fgets(buf, 3, stdin);
            if (*cp != NULL && *buf == '\n')
                break;
            else if (*cp != NULL && toupper(buf[1]) == 'X' || (toupper(buf[0]) == 'T' && toupper(buf[0]) == 'R'))
                return (toupper(buf[0] == 'T')) ? EP_OUTB : EP_INB;

            printf("\n\n\n\tPlease enter either TX or RX -- try again.\n");
        }
    }

    return EP_OUTB; // Default index 0 is TX/RX for full-duplex but TX-only for simplex
}

/*********************************************************************************
  cmdAudRtpEp

  Function to display a menu of all the mspSendCommands which can be sent to a
  AMR Bypass RTP EP, and then execute these commands.
**********************************************************************************/
void cmdAudRtpEp()
{
    DWORD       value,
                command,
                expectedEvent,
                ret,
                first_gw,
                last_gw, index;
    WORD        i;
    CTA_EVENT   event;
    MSP_ACTION  mspAction;
    
    msp_ENDPOINT_RTPFDX_MAP ep_map_cmd;

    // First and last gateways to send commands to
    first_gw = 0;
    last_gw  = GwConfig[0].dNumGW - 1;
    
    // If there are multiple gateways, let the operator choose which on to command
    if (GwConfig[0].dNumGW > 1)
    {
        printf("\nEnter the number of the GW to send a command to "
               "(or enter 99 to send to all gateways) > ");
        fflush(stdin);
        READ_PARAM("%d", &value); 
        if (value != 99)
        {
            first_gw = (DWORD)value;
            last_gw  = (DWORD)value;
        }
    }

    index = 0;
    if (gwParm.simplexEPs)
        index = whichEpDirection();

    printf("\n\tThe current setting of the payloadID = %d,\n\tEnter a new value > ",
        GwConfig[0].AudRtpEp[index].Param.EP.RtpRtcp.PayloadMap.payload_id);
    fflush(stdin);
    READ_PARAM("%d", &value);
    GwConfig[0].AudRtpEp[index].Param.EP.RtpRtcp.PayloadMap.payload_id = value;
    ep_map_cmd.payload_id = H2NMS_DWORD(value);

/*    printf("\n\tThe current setting of the Video Map Vocoder = %d,\n\tEnter a new value > ",
        GwConfig[0].VidRtpEp.Param.EP.RtpRtcp.PayloadMap.vocoder);
    fflush(stdin);
    READ_PARAM("%d", &value);*/
    value = GwConfig[0].AudRtpEp[index].Param.EP.RtpRtcp.PayloadMap.vocoder;
    ep_map_cmd.vocoder = H2NMS_DWORD(value);

    command = mspBuildCommand(MSP_ENDPOINT_RTPFDX, MSP_CMD_RTPFDX_MAP);
    for (i=(WORD)first_gw; i <= (WORD)last_gw; i++)   //Send querys to each of the endpoints
    {
        ret = mspSendCommand(GwConfig[i].AudRtpEp[index].hd,
                            command,
                            &ep_map_cmd,
                             sizeof (ep_map_cmd));
        //Verify correct response from mspSendCommand
        if (ret != SUCCESS)
        {
            printf("\n\t ERROR: mspSendCommand returned failure.\n");
        }
        expectedEvent = (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_RTPFDX_MAP);        
        
        ret = WaitForSpecificEvent( i, GwConfig[i].hCtaQueueHd, expectedEvent, &event );
		if ( ret != 0 )
		{
			printf("\n\tERROR: mspSendCommand failed to send valid completion event\n");     
		}
    
        //Record the query to file.
        if ( RecCmdFp !=NULL)
        {
            memset(&mspAction, '\0', sizeof (mspAction));
            mspAction.GWNo          = i;
            mspAction.objType       = MSP_ENDPOINT_RTPFDX;
            mspAction.action        = MSP_SEND_CMD;
            mspAction.command       = command;
            mspAction.expectedEvent = expectedEvent;
            mspAction.cmdSize       = sizeof (ep_map_cmd);
            memcpy( mspAction.CmdBuff, &ep_map_cmd, sizeof (ep_map_cmd) );


            if ( fwrite(&mspAction, 1, sizeof (mspAction), RecCmdFp) != sizeof (mspAction))
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
    DWORD       value,
                command,
                expectedEvent,
                ret,
                first_gw,
                last_gw, index;
    WORD        i;
    CTA_EVENT   event;
    MSP_ACTION  mspAction;
    
    msp_FILTER_JITTER_CMD  amr_by_chan_cmd;

    // First and last gateways to send commands to
    first_gw = 0;
    last_gw  = GwConfig[0].dNumGW - 1;

    // If there are multiple gateways, let the operator choose which on to command
    if (GwConfig[0].dNumGW > 1)
    {
        printf("\nEnter the number of the GW to send a command to "
               "(or enter 99 to send to all gateways) > ");
        fflush(stdin);
        READ_PARAM("%d", &value); 
        if (value != 99)
        {
            first_gw = (DWORD)value;
            last_gw  = (DWORD)value;
        }
    }

    index = 0;
    if (gwParm.simplexEPs)
        index = whichEpDirection();

    printf("\n\tEnter a new value for the jitter depth > ");
    fflush(stdin);
    READ_PARAM("%d", &value);

    amr_by_chan_cmd.value = H2NMS_DWORD(value);

    command = mspBuildCommand(MSP_FILTER_JITTER, MSP_CMD_JITTER_CHG_DEPTH);
    for (i=(WORD)first_gw; i <= (WORD)last_gw; i++)   //Send querys to each of the endpoints
    {
        ret = mspSendCommand(GwConfig[i].AudByChan[index].hd,
                            command,
                            &amr_by_chan_cmd,
                             sizeof (amr_by_chan_cmd));
        //Verify correct response from mspSendCommand
        if (ret != SUCCESS)
        {
            printf("\n\t ERROR: mspSendCommand returned failure.\n");
        }
        expectedEvent = (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_JITTER_CHG_DEPTH);        
        
        ret = WaitForSpecificEvent( i, GwConfig[i].hCtaQueueHd, expectedEvent, &event );
		if ( ret != 0 )
		{
			printf("\n\tERROR: mspSendQuery failed to send valid completion event\n");     
		}
    
        //Record the query to file.
        if ( RecCmdFp !=NULL)
        {
            memset(&mspAction, '\0', sizeof (mspAction));
            mspAction.GWNo          = i;
            mspAction.objType       = MSP_FILTER_JITTER;
            mspAction.action        = MSP_SEND_CMD;
            mspAction.command       = command;
            mspAction.expectedEvent = expectedEvent;
            mspAction.cmdSize       = sizeof (amr_by_chan_cmd);
            memcpy( mspAction.CmdBuff, &amr_by_chan_cmd, sizeof (amr_by_chan_cmd) );


            if ( fwrite(&mspAction, 1, sizeof (mspAction), RecCmdFp) != sizeof (mspAction))
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
    DWORD     k,
              first_gw,
              last_gw;
    WORD      byteCount = 0;
    WORD      value;
    char      selection;
    BOOL      display_menu = TRUE;
    BOOL      execute_cmd  = FALSE;
    
    PBX_PLACE_CALL_STRUCT pbxPlaceCall;

    // First and last gateways to send commands to
    first_gw = 0;
    last_gw  = GwConfig[0].dNumGW - 1;

    while (display_menu)
    {
        // If there are multiple gateways, let the operator choose which on to command
        if (GwConfig[0].dNumGW > 1)
        {
            printf("\nEnter the number of the GW to send a command to "
                   "\n(or enter 99 to send to all gateways) > ");
            fflush(stdin);
            READ_PARAM("%hd", &value); 
            if (value != 99)
            {
                first_gw = value;
                last_gw  = value;
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
        READ_PARAM("%c", &selection);
        switch (selection)
	    {
		    case 'a':  //Quit
		    case 'x':  //Quit
                execute_cmd  = FALSE;
                display_menu = FALSE;
                break;

            case 'b':  //Display Menu
                execute_cmd  = FALSE;
                display_menu = TRUE;
                break;

            case 'c':  // Place Call
                display_menu = FALSE;
                printf("\n\tNumber to Call > ");
                fflush(stdin);
                READ_PARAM("%s", pbxPlaceCall.calledaddr );
				
	            for (k = first_gw; k <= last_gw; k++)
                {
					autoPlaceCall( k, pbxPlaceCall.calledaddr );
				}							
                break;

            case 'd':  // Disconnect
                display_menu = FALSE;
	            for (k = first_gw; k <= last_gw; k++)
                {
					autoDisconnectCall( k );
				}			
                break;

           case 'e':
               display_menu = FALSE;
               if ( closeAllChannels(0)!= SUCCESS)
               {
                  return;
               }
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
    char selection;
    BOOL display_menu = TRUE;
    DWORD rc, i;

    H324_USER_INPUT_INDICATION l_UII;

    // First and last gateways to send commands to
    while (display_menu)
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
        printf("\t  j)  H245 Signal User Indication                                    \n");
        printf("\t  l)  H245 Close Channel                                             \n");
        printf("\t  m)  Control DEMUX Line Error Reporting                             \n");
        
        printf("\t  n)  H245 Skew Indication                                           \n");
        printf("\t  o)  H245 VendorID Indication                                       \n");
        printf("\t  p)  H245 VideoTemporalSpatialTradeoff Indication                   \n");
        
        printf("\t=====================================================================\n");
        printf("\tSelection> ");
        fflush(stdin);
        READ_PARAM("%c", &selection);
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
                display_menu  = FALSE;
                
                // Fill values in UII structure
				l_UII.length        = 128;
                l_UII.msgType       = H324_USER_INPUT_ALPHANUMERIC;
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
				if ( rc == SUCCESS ) 
                {
					mmPrint( 0, T_H245, "The alphanumeric UII message was sent successfully.\n");
                }
				else
                {
					printf("Failed to send the alphanumeric UII message...\n");					
                }
                break;
            case 'g':  // H245 User Indication - alphanumeric
                display_menu  = FALSE;
                
                // Fill values in UII structure
				l_UII.length        = 512;
                l_UII.msgType       = H324_USER_INPUT_ALPHANUMERIC;
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
				if ( rc == SUCCESS ) 
                {
					mmPrint( 0, T_H245, "The alphanumeric UII message was sent successfully.\n");
                }
				else
                {
					printf("Failed to send the alphanumeric UII message...\n");					
                }
                break;
            case 'h':  // H245 User Indication - nonstandard
                display_menu  = FALSE;
                
                // Fill values in UII structure
				l_UII.length        = 128;
                l_UII.msgType       = H324_USER_INPUT_NONSTANDARD;
                l_UII.szObjectId[0] = 0;
                strcpy( l_UII.szObjectId, "1.5.9.12.234.255.3906.12345.1234567");
                for (i = 0; i<l_UII.length; i++)
                {
                    l_UII.data[i] = (char)(i % 256);
                }
				mmPrint( 0, T_H245, "UII length: %d\n", l_UII.length );
                
                // Send UII
                rc = h324UserIndication( GwConfig[0].MuxEp.hd, &l_UII );
				if ( rc == SUCCESS ) 
                {
					mmPrint( 0, T_H245, "The NonStandard UII message was sent successfully.\n");
                }
				else
                {
					mmPrintErr( 0, T_H245, "Failed to send the NonStandard UII message...\n");					
                }
                break;              
            case 'i':  // H245 User Indication - nonstandard
                display_menu  = FALSE;
                
                // Fill values in UII structure
				l_UII.length        = 512;
                l_UII.msgType       = H324_USER_INPUT_NONSTANDARD;
                l_UII.szObjectId[0] = 0;
                strcpy( l_UII.szObjectId, "1.5.9.12.234.255.3906.12345.1234567");
                
                for (i = 0; i < l_UII.length; i++)
                {
                    l_UII.data[i] = (char)(i % 256);
                }
				mmPrint( 0, T_H245, "UII length: %d\n", l_UII.length );
                
                // Send UII
                rc = h324UserIndication( GwConfig[0].MuxEp.hd, &l_UII );
				if( rc == SUCCESS ) 
                {
					mmPrint( 0, T_H245, "The NonStandard UII message was sent successfully.\n");
                }
				else
                {
					mmPrintErr( 0, T_H245, "Failed to send the NonStandard UII message...\n");					
                }
                break;              
            case 'j':  // H245 User Indication - signal
            {
                H324_USER_INPUT_INDICATION l_UII;
                display_menu  = FALSE;
                
                // Fill values in UII structure
                l_UII.msgType       = H324_USER_INPUT_SIGNAL;
                l_UII.szObjectId[0] = 0;
                l_UII.data[0]       = '#';
                l_UII.length        = 1;

                // Send Signal UII
                rc = h324UserIndication( GwConfig[0].MuxEp.hd, &l_UII );
                if( rc == SUCCESS ) 
                    mmPrint( 0, T_H245, "The Signal UII message was sent successfully.\n");
                else
                    mmPrintErr( 0, T_H245, "Failed to send the Signal UII message...\n");                    
                break;              
            }  
            case 'l':
                display_menu = FALSE;
                printf("\t=====================================================================\n");
                printf("\tSelect one of the following channels to close:                       \n");
                printf("\t  a)  Close Audio In Channel                                         \n");
                printf("\t  b)  Close Audio Out Channel                                        \n");
                printf("\t  c)  Close Bidirectional Video Channel                              \n");
                printf("\t  d)  Close Video In Channel                                         \n");
                printf("\t  e)  Close Video Out Channel                                        \n");				
                printf("\t=====================================================================\n");
                printf("\tSelection> ");
                fflush(stdin);
                READ_PARAM("%c", &selection);
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
				        h324CloseChannel( GwConfig[0].MuxEp.hd, H324RSN_VIDEO_IN );
                        Flow(0, F_APPL, F_H324, "h324CloseChannel(video in)");
                        break;                
                    case 'e':
				        h324CloseChannel( GwConfig[0].MuxEp.hd, H324RSN_VIDEO_OUT );
                        Flow(0, F_APPL, F_H324, "h324CloseChannel(video out)");
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
                READ_PARAM("%c", &selection);
                switch (selection)
	            {
                    WORD value1,
                         value2;
                    case 'a':
                        h324LineErrorReporting( GwConfig[0].MuxEp.hd, 
                                                H324_LINE_STAT_CMD_RESET_STAT,
                                                0,
                                                0);
                        break;
                    case 'b':
                        printf("\n\tEnable/Disable Periodic Statistics (0=Disable, 1=Enable) > ");
                        fflush(stdin);
                        READ_PARAM("%hd", &value1);
                        printf("\n\tSet Periodic Statistics Interval (1-10 seconds X 10) > ");
                        fflush(stdin);
                        READ_PARAM("%hd", &value2);
                        h324LineErrorReporting( GwConfig[0].MuxEp.hd, 
                                                H324_LINE_STAT_CMD_PERIODIC,
                                                value1,
                                                value2);
                        break;
                    case 'c':
                        printf("\n\tEnable/Disable Error Event (0=Disable, 1=Enable) > ");
                        fflush(stdin);
                        READ_PARAM("%hd", &value1);
                        printf("\n\tSet Event Bit Mask (1=Video, 2=Audio, 4=Golay) > ");
                        fflush(stdin);
                        READ_PARAM("%hd", &value2);
                        h324LineErrorReporting( GwConfig[0].MuxEp.hd, 
                                                H324_LINE_STAT_CMD_ERROR_EVENT,
                                                value1,
                                                value2);
                        break;
                }
                break;

            case 'n':
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
                    READ_PARAM("%c", &selection);
                    if (selection == 'a' || selection == 'v')
                    {
                        break;
                    }
                    printf("\n\tSelection must be 'a' or 'v' -- try again.\n");
                }
                printf("\t=====================================================================\n");
                printf("\tSpecify skew interval [0-4095 ms] between channels:                  \n");
                printf("\t=====================================================================\n");
                for (skewInMs=4096; ; )
                {
                    printf("\tInterval> ");
                    fflush(stdin);
                    READ_PARAM("%u", &skewInMs);
                    if (skewInMs <= 4095)
                    {
                        break;
                    }
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
                {
                   mmPrint(0, T_H245, "The SkewIndication message was sent successfully.\n");
                }
                else
                {
                   mmPrintErr("Failed to send the SkewIndication message...\n");
                }
                break;
            case 'o':
                #define VENDORID_TEST_BUF_SZ (sizeof(H324_VENDORID_INDICATION) + 128 + 256 + 256)
                unsigned char l_vidBuf[VENDORID_TEST_BUF_SZ];
                
                H324_VENDORID_INDICATION *l_pVID;
                unsigned char *pByte;
                unsigned       val;
                char           buf[6];
                
                pByte  = &l_pVID->bytes[0];
                l_pVID = (H324_VENDORID_INDICATION *)&l_vidBuf[0];
                
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
                    READ_PARAM("%c", &selection);
                    if (selection == 'o' || selection == 'O' || selection == 'n' || selection == 'N') 
                       break;
                    printf("\n\tSelection must be 'o' or 'n' -- try again.\n");
                }
                
                if (selection == 'o' || selection == 'O')
                {
                    pByte = &l_pVID->bytes[0];
                    
                    for (l_pVID->vendorIDLen = 0; ; )
                    {
                        printf("\tEnter ObjectID field (0..255: [Return] when finished)> ");
                        fflush(stdin);
                        *buf = '\0';
                        if (fgets(buf, 5, stdin) != NULL && *buf != '\n')
                        {
                            sscanf(buf, "%3d", &val);
                            if (val > 255)
                            {
                                printf("\t(Value typed exceeds 255 -- please retype)\n\n");
                                continue;
                            }
                            *pByte++ = (unsigned char)val;
                            l_pVID->vendorIDLen++;
                        }
                        else if (l_pVID->vendorIDLen <= 1)
                        {
                            printf("\t(ObjectID field can be no less than two bytes -- please add another)\n\n");
                        }
                        else
                        {
                            break;
                        }
                    }

                    // This is an Object identifer, not an h221NonStandard identifier.
                    l_pVID->isNonStandard = 0; 
                }
                else // selection == 'n' || selection == 'N'
                {
                    unsigned char  t35CountryCode,
                                   t35Extension;
                    unsigned short manufacturerCode;
                    unsigned       tmp;

                    // Prompt for the NonstandardIdentifier fields.
                    printf("\tEnter t35CountryCode (0..255)> ");
                    fflush(stdin);
                    READ_PARAM("%3d", &tmp);
                    t35CountryCode = (unsigned char)tmp;
                    printf("\tEnter t35Extension (0..255)> ");
                    fflush(stdin);
                    READ_PARAM("%3d", &tmp);
                    t35Extension = (unsigned char)tmp;
                    printf("\tEnter manufacturerCode (0..65535)> ");
                    fflush(stdin);
                    READ_PARAM("%5d", &tmp);
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
                    {
                        break;
                    }
                    sscanf(buf, "%2x", &val);
                    *pByte++ = (unsigned char)(val & 0x00FF);
#ifdef LINUX
                    (void)getchar();  // Discard newline which fflush(stdin) won't discard on Linux
#endif
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
                    {
                        break;
                    }
                    sscanf(buf, "%2x", &val);
                    *pByte++ = (unsigned char)(val & 0x00FF);
#ifdef LINUX
                    (void)getchar();  // Discard newline which fflush(stdin) won't discard on Linux
#endif
                }

                // Issue the assembled VendorID indication to the terminal.
                rc = h324VendorIDIndication(GwConfig[0].MuxEp.hd, l_pVID);
                if (rc == SUCCESS) 
                {
                   mmPrint(0, T_H245, "The VendorID indication was sent successfully.\n");
                }
                else
                {
                   mmPrintErr("Failed to send the VendorID indication msg...\n");
                }
                break;
            case 'p':
                unsigned tradeoff;

                display_menu = FALSE;
                printf("\t=====================================================================\n");
                printf("\tSpecify temporal-spatial tradeoff value for outbound video channel:  \n");
                printf("\t=====================================================================\n");
                for ( ; ; )
                {
                    printf("\tTradeoff value (0..31)> ");
                    fflush(stdin);
                    READ_PARAM("%u", &tradeoff);

                    if (tradeoff < 32) 
                    {
                       break;
                    }
                    printf("\n\tTradoff value out-of-range -- try again.\n");
                }
                
                // Send specified videoTemporalSpatialTradeoff indication to the remote terminal.
                mmPrint(0, T_H245, "Informing terminal videoTemporalSpatialTradeoff value is now %u.\n",
                        tradeoff);
                
                rc = h324VideoTemporalSpatialTradeoffIndication(GwConfig[0].MuxEp.hd, tradeoff);
                if (rc == SUCCESS) 
                {
                   mmPrint(0, T_H245, "The videoTemporalSpatialTradeoff Indication message was sent successfully.\n");
                }
                else
                {
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
    char          selection;
    BOOL          display_menu = TRUE;
	tTrcVtpAll    trcVtpStatus;
    int           i, vtpId;
    DWORD         ret; 	
    MEDIA_GATEWAY_CONFIG *pCfg;

	// Get a pointer to the gateway's configuration
	pCfg = &GwConfig[0];        
	
    while (display_menu)
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
        READ_PARAM("%c", &selection);
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

				ret = trcVTPStatus( &trcVtpStatus ); 				
				if (ret == TRC_SUCCESS)
				{
					printf("%d VTPs defined\n", trcVtpStatus.vtpDefined );
					for(i=0; i<trcVtpStatus.vtpDefined; i++)
					{
						printf("\tVTP ID %d: state [%s]\n", trcVtpStatus.vtp[i].vtpId,
								trcValueName( TRCVALUE_VTPSTATE, trcVtpStatus.vtp[i].state ) );
						printf("\t\tNumber of simplex channels in use by this app: %d\n",
								trcVtpStatus.vtp[i].simplexLocal );
					}
				}
				else
				{
					printf("Error [%s] while requesting VTP status information\n",
							trcValueName( TRCVALUE_RESULT, ret ) );
				}				
                break;

            case 'd': // TRC reset VTP
                display_menu = FALSE;                
                printf("\n\tEnter the VTP ID > ");
                fflush(stdin);
                READ_PARAM("%d", &vtpId);
                
                ret = trcResetVTP( vtpId );
                printf("trcResetVTP ret=%d\n", ret);
                break;
                
            case 'e': // start TRC channel
                display_menu = FALSE;    

                ret = trcCreateVideoChannel( &pCfg->VideoXC.trcChHandle, TRC_CH_SIMPLEX, &pCfg->VideoXC );
                Flow(0, F_APPL, F_TRC, "trcCreateVideoChannel");            
                if( ret != TRC_SUCCESS )
                {
                    mmPrintPortErr( 0, "trcCreateVideoChannel returned failure code %d\n", ret);
                    break;
                }
                else
                {
                    mmPrint( 0, T_GWAPP, "trcCreateVideoChannel allocated the simplex channel.\n");
                }                    
                
                break;
				
            case 'f': // stop TRC channel
               display_menu = FALSE;                
               mmPrint(0, T_GWAPP, "Stop Simplex Video Channel\n");
               ret = trcStopVideoChannel( pCfg->VideoXC.trcChHandle );
               Flow(0, F_APPL, F_TRC, "trcStopVideoChannel");                                        
               
               if( ret != TRC_SUCCESS )
               {
                 mmPrintPortErr(0, "trcStopVideoChannel failed. ret = %d\n", ret);
                 mmPrint(0, T_GWAPP, "Trying to destroy the Video Channel...\n");
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
	DWORD       ret,
                expectedEvent,
                dIP;

	//----------------------------------------
	// SERVER Video
	//----------------------------------------
    disableVideoEP( dGwIndex, VideoCtx[dGwIndex].rtpEp.hd, 0 );
																									
	memset( &mCONFIG, 0xFF, sizeof( msp_ENDPOINT_RTPFDX_CONFIG ) );						
	mCONFIG.remoteUDPPort = H2NMS_WORD(srvDestPort); 
    dIP = inet_addr(srvDestIPAddr);
    memcpy( &mCONFIG.remoteIPAddr, &dIP, 4);
       				
	ret = mspSendCommand(VideoCtx[dGwIndex].rtpEp.hd,
		 			     mspBuildCommand(MSP_ENDPOINT_RTPFDX_VIDEO, MSP_CMD_RTPFDX_CONFIG),
						&mCONFIG,
						 sizeof ( msp_ENDPOINT_RTPFDX_CONFIG ) );

	//Verify correct response from mspSendCommand
	if (ret != SUCCESS)
    {
		printf("\n\t ERROR: mspSendCommand returned failure.\n");
    }
	expectedEvent = (MSPEVN_SENDCOMMAND_DONE | MSP_CMD_RTPFDX_CONFIG);
	
    ret = WaitForSpecificEvent( dGwIndex, GwConfig[dGwIndex].hCtaQueueHd, expectedEvent, &event );
	if ( ret != 0 )
    {
		printf("\n\tERROR: mspSendCommand failed to send valid completion event\n");     
	}

    enableVideoEP( dGwIndex, VideoCtx[dGwIndex].rtpEp.hd, 0 );

/*
	disableVideoChannel( dGwIndex, GwConfig[dGwIndex].VidChan.hd );	
	enableVideoChannel( dGwIndex, GwConfig[dGwIndex].VidChan.hd );										
*/
	return SUCCESS;
}


DWORD configurePlayWithXc( int nGw )
{
	MEDIA_GATEWAY_CONFIG *pCfg;
        	
	// Get a pointer to the gateway's configuration
	pCfg = &GwConfig[nGw];        
	
#ifdef USE_TRC						      
    {

      
        DWORD ret;
        configureVideoRtpEps( nGw, pCfg->VideoXC.trcRes.rxPort[TRC_DIR_SIMPLEX],
                              pCfg->VideoXC.trcRes.ipAddr );

       // dynamic TRC channel
       if(getVideoChannelStatus(pCfg->VideoXC.trcChHandle)==TRC_STATE_ACTIVE){  
           ret = trcIframeVideoChannel( pCfg->VideoXC.trcChHandle, TRC_DIR_SIMPLEX );
           Flow(nGw, F_APPL, F_TRC, "trcIframeVideoChannel");                                        
    
           if(ret != SUCCESS)
           {
               mmPrintPortErr( nGw, "trcIframeVideoChannel (Generate I-Frame) failed. ret = %d\n", ret);
        }     
      }
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
    
    configureVideoRtpEps( nGw, pCfg->vidRtpEp[EP_OUTB].Addr.EP.RtpRtcp.nSrcPort,
                          pCfg->vidRtpEp[EP_OUTB].Addr.EP.RtpRtcp.SrcIpAddress );
    pCfg->bGwVtpServerConfig = FALSE;       
    
    return SUCCESS;    
}

DWORD PlayFromDB( int nGw, char *fileName, tVideo_Format fileFormat )
{ 
	MEDIA_GATEWAY_CONFIG *pCfg;
    	
	pCfg = &GwConfig[nGw]; 

#ifdef USE_TRC 	          
   // resolution support
   DWORD  res;
   res =  getFileRes(fileName);   //Geting the Resolution of the DB file.          

   if(res < 0) {
       mmPrintPortErr(nGw, "the video file %s is not support Resolution %s, error %x \n", \
                     fileName, res);
       return FAILURE;
   }
#endif
                      
    switch ( fileFormat )
    {
        case MPEG4:            
            // If the the negotiated via H245 video codec is different than MPEG4 
            // and the current EPs configuration is not GW-VTP-Server   
#ifdef USE_TRC                  
            if ( pCfg->txVideoCodec != H324_MPEG4_VIDEO ||(res!=TRC_FRAME_RES_QCIF))
#else
            if ( pCfg->txVideoCodec != H324_MPEG4_VIDEO )
#endif            
            {
                if ( !pCfg->bGwVtpServerConfig )
                {
                    // Configure EPs GW-VTP-Server
                    configurePlayWithXc( nGw );
                }
            }
            else
            { // if pCfg->txVideoCodec == H324_MPEG4_VIDEO
                if( pCfg->bGwVtpServerConfig )
                {
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
#ifdef USE_TRC                   
            if ( pCfg->txVideoCodec != H324_H263_VIDEO || (res!=TRC_FRAME_RES_QCIF))
#else
            if ( pCfg->txVideoCodec != H324_H263_VIDEO )
#endif            
            {
                if ( !pCfg->bGwVtpServerConfig )
                {
                    // Configure EPs GW-VTP-Server
                    configurePlayWithXc( nGw );
                }
            }
            else
            { // if pCfg->txVideoCodec == H324_H263_VIDEO
                if ( pCfg->bGwVtpServerConfig )
                {
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
    FILE         *fp;
    int           i;    
    tVideo_Format fileFormat;
        
    fp = fopen( fname, "r" );
    if ( fp == NULL )
    {
    	mmPrintPortErr( nGw, "Failed to open the following playlist: %s\n", fname );       
        return -1;
    }

    i = 0;      
	mmPrint( nGw, T_SRVAPP, "Reading the PlayList: %s\n", fname );    
	while ( fgets(VideoCtx[nGw].playList.fname[i], 256, fp) != NULL )
    {
        VideoCtx[nGw].playList.fname[i][ strlen(VideoCtx[nGw].playList.fname[i])-1 ] = '\0';
        mmPrint( nGw, T_SRVAPP,"\t%d. %s\n", i+1, VideoCtx[nGw].playList.fname[i] );
        i++;
    }
    VideoCtx[nGw].playList.numFiles = i;
    VideoCtx[nGw].playList.currFile = 0;    
    VideoCtx[nGw].bPlayList         = TRUE;
    
    if ( VideoCtx[nGw].playList.numFiles == 0 )
    {
        mmPrint( nGw, T_SRVAPP,"The PlayList is empty.\n");
        return 0;
    }
        
    // Play the first file from the list
    fileFormat = (tVideo_Format)getFileFormat( VideoCtx[nGw].playList.fname[0] );
    if ( fileFormat == -1 )
    {
        mmPrintPortErr( nGw, "Illegal file name: %s\n", VideoCtx[nGw].playList.fname[0] );
    }
    else
    {
        PlayFromDB( nGw, VideoCtx[nGw].playList.fname[0], fileFormat );
    }
        
    fclose( fp );
    
    return SUCCESS;
}

void cmdPlayRecord(void)
{
    char          selection;
    BOOL          display_menu = TRUE;
    char          fileName[256];
       	
    while( display_menu ) {
        printf(
        "\t=====================================================================\n"\
        "\t      PLAY/RECORD COMMAND MENU                                       \n"\
        "\t      ========================                                       \n"\
        "\tSelect one of the following play/record commands.                    \n"\
        "\t  a)  Exit Play/Rec Command Menu      b)  Help (reprint this menu)   \n");
        if (mmParm.Operation == PLAY || mmParm.Operation == RECORD)
        {
            if (mmParm.Audio_RTP_Format != NO_AUDIO)
            { 
                if (AudioCtx[0].pr_mode_p == PR_MODE_ACTIVE1 || AudioCtx[0].pr_mode_p == PR_MODE_ACTIVE2)
                    printf("\t  u)  Stop Playing Audio\n");
                else if (AudioCtx[0].pr_mode_r == PR_MODE_ACTIVE1 || AudioCtx[0].pr_mode_r == PR_MODE_ACTIVE2)
                    printf("\t  u)  Stop Recording Audio\n");
            }
            if (mmParm.Video_Format != NO_VIDEO)
            {
                if (VideoCtx[0].pr_mode_p == PR_MODE_ACTIVE1 || VideoCtx[0].pr_mode_p == PR_MODE_ACTIVE2)
                    printf("\t  v)  Stop Playing Video\n");
                if (VideoCtx[0].pr_mode_r == PR_MODE_ACTIVE1 || VideoCtx[0].pr_mode_r == PR_MODE_ACTIVE2)
                    printf("\t  v)  Stop Recording Video\n");
            }
        }
        else if (mmParm.Operation == SIM_PR)
        {
            if (AudioCtx[0].pr_mode_p == PR_MODE_ACTIVE1 || VideoCtx[0].pr_mode_p == PR_MODE_ACTIVE1)
                printf("\t  m)  Stop Play\n");
            if (AudioCtx[0].pr_mode_r == PR_MODE_ACTIVE1 || VideoCtx[0].pr_mode_r == PR_MODE_ACTIVE1)
                printf("\t  n)  Stop Record\n");
            if (AudioCtx[0].pr_mode_r == PR_MODE_ACTIVE1 &&
                VideoCtx[0].pr_mode_r == PR_MODE_ACTIVE1 &&
                AudioCtx[0].pr_mode_p == PR_MODE_ACTIVE1 &&
                VideoCtx[0].pr_mode_p == PR_MODE_ACTIVE1)
            {
                printf("\t  d)  Stop Simultaneous Play & Record\n");
            }
        }
        if (AudioCtx[0].pr_mode_p != PR_MODE_ACTIVE1 && VideoCtx[0].pr_mode_p != PR_MODE_ACTIVE1 && 
            AudioCtx[0].pr_mode_r != PR_MODE_ACTIVE1 && VideoCtx[0].pr_mode_r != PR_MODE_ACTIVE1)
        {
            printf("\t  s)  Start Play\n");
            printf("\t  r)  Start Record\n");                    
            printf("\t  i)  Start Simultaneous Play/Record\n"); 
            printf("\t  t)  Play file from the database\n");        
            printf("\t  l)  Play files from the PlayList\n");                
        }
        printf("\t=====================================================================\n");
		
        printf("\tSelection> ");
        fflush(stdin);
        READ_PARAM("%c", &selection);
        switch (selection)
	    {
		    case 'a':  //Quit
                display_menu = FALSE;
                break;

            case 'b':  //Display Menu
                display_menu = TRUE;
                break;
            case 'u':  //Stop record audio
                display_menu = FALSE;
                if (mmParm.Audio_RTP_Format != NO_AUDIO)
                {
                    if (mmParm.Operation == RECORD  && (AudioCtx[0].pr_mode_r == PR_MODE_ACTIVE1 || AudioCtx[0].pr_mode_r == PR_MODE_ACTIVE2))
                        cmdStopRecAudio(0);
                    else if (mmParm.Operation == PLAY && (AudioCtx[0].pr_mode_p == PR_MODE_ACTIVE1 || AudioCtx[0].pr_mode_p == PR_MODE_ACTIVE2))
                        cmdStopPlayAudio(0);
                }
                break;
            case 'v':  //Stop record Video
                display_menu = FALSE;
                if (mmParm.Video_Format != NO_VIDEO)
                {
                    if (mmParm.Operation == RECORD && (VideoCtx[0].pr_mode_r == PR_MODE_ACTIVE1 || VideoCtx[0].pr_mode_r == PR_MODE_ACTIVE2))
                        cmdStopRecVideo(0);
                    else if (mmParm.Operation == PLAY && (VideoCtx[0].pr_mode_p == PR_MODE_ACTIVE1 || VideoCtx[0].pr_mode_p == PR_MODE_ACTIVE2))
                        cmdStopPlayVideo(0);            
                }
               break;
            case 'm': //Stop play
                display_menu = FALSE;
                if ( ( (AudioCtx[0].pr_mode_p == PR_MODE_ACTIVE1) ||
                     (VideoCtx[0].pr_mode_p == PR_MODE_ACTIVE1) ) && (mmParm.Operation == SIM_PR))
                {
                    cmdStopPlayVideo(0);
                    cmdStopPlayAudio(0);
                }
               break;
            case 'n': //Stop record
                display_menu = FALSE;
                if ( ( (AudioCtx[0].pr_mode_r == PR_MODE_ACTIVE1) ||
                     (VideoCtx[0].pr_mode_r == PR_MODE_ACTIVE1) ) &&(mmParm.Operation == SIM_PR))
                {
                    cmdStopRecVideo(0);
                }
                break;
            case 's':  //Start Play
                tVideo_Format fileFormat;
               
                if ( GwConfig[0].bGwVtpServerConfig )
                {
                    // Configure EPs Gw-Server 
                    configurePlayWithoutXc( 0 );
                }                 
               
                if ( (AudioCtx[0].pr_mode_p != PR_MODE_ACTIVE1) &&
                     (VideoCtx[0].pr_mode_p != PR_MODE_ACTIVE1) )
                {
                    // Restore the original file name in case some other files were played
                    // from the database
    
                    fileFormat = (tVideo_Format)getFileFormat( VideoCtx[0].threegp_FileName );
                    displayOperation(0, "Start PLAY", VideoCtx[0].threegp_FileName, fileFormat);
            
                    display_menu = FALSE;			
				    mmParm.Operation = PLAY;
				   
			        PlayRecord( 0, fileFormat );
                } 
                break;
            case 'r':  //Start Record
                display_menu = FALSE;			
                if ( (AudioCtx[0].pr_mode_r != PR_MODE_ACTIVE1) &&
                     (VideoCtx[0].pr_mode_r != PR_MODE_ACTIVE1) )
                {
			        mmParm.Operation = RECORD;

                    // Restore the original file name in case some other files were played
                    // from the database
               
                    strcpy(VideoCtx[0].threegp_FileName, mmParm.threegpFileName);					
 		            displayOperation(0, "Start RECORD", VideoCtx[0].threegp_FileName, convertVideoCodec2Format(GwConfig[0].txVideoCodec));
				   
			        PlayRecord( 0, convertVideoCodec2Format(GwConfig[0].txVideoCodec) );
				
			        h324VideoFastUpdate( GwConfig[0].MuxEp.hd );
                } 
                break;
            case 'i':  //Simul Play and Record
                display_menu = FALSE;
                if ( ( (AudioCtx[0].pr_mode_p != PR_MODE_ACTIVE1) &&
                       (VideoCtx[0].pr_mode_p != PR_MODE_ACTIVE1) ) &&
                     ( (AudioCtx[0].pr_mode_r != PR_MODE_ACTIVE1) &&
                       (VideoCtx[0].pr_mode_r != PR_MODE_ACTIVE1) ) )
                {

                    mmParm.Operation = SIM_PR;
                    tVideo_Format fileFormat1;

                    if ( GwConfig[0].bGwVtpServerConfig )
                    {
                        // Configure EPs Gw-Server
                        configurePlayWithoutXc( 0 );
                    }
                    strcpy(VideoCtx[0].threegp_FileName_play, mmParm.threegpFileName_play);
                    fileFormat1 = (tVideo_Format)getFileFormat( VideoCtx[0].threegp_FileName_play );
                    displayOperation(0, "Start PLAY", VideoCtx[0].threegp_FileName_play, fileFormat1);
                    strcpy(VideoCtx[0].threegp_FileName, mmParm.threegpFileName);
                    displayOperation(0, "Start RECORD", VideoCtx[0].threegp_FileName, convertVideoCodec2Format(GwConfig[0].txVideoCodec));

                    PlayRecord( 0, convertVideoCodec2Format(GwConfig[0].txVideoCodec));
                    h324VideoFastUpdate( GwConfig[0].MuxEp.hd );
                }
                break;
            case 'd': // Stop simultaneous play and Record
                display_menu = FALSE;
                if ( ( (AudioCtx[0].pr_mode_r == PR_MODE_ACTIVE1) &&
                       (VideoCtx[0].pr_mode_r == PR_MODE_ACTIVE1) ) &&
                     ( (AudioCtx[0].pr_mode_p == PR_MODE_ACTIVE1) &&
                       (VideoCtx[0].pr_mode_p == PR_MODE_ACTIVE1) ) )
               {
                   cmdStopRecVideo(0);
                   cmdStopPlayVideo(0);
                   cmdStopPlayAudio(0);
               }
              break;
 
            case 't':  //Play file from the database
                display_menu = FALSE;			
                if ( ( (AudioCtx[0].pr_mode_p != PR_MODE_ACTIVE1) &&
                       (VideoCtx[0].pr_mode_p != PR_MODE_ACTIVE1) ) &&
                     ( (AudioCtx[0].pr_mode_r != PR_MODE_ACTIVE1) &&
                       (VideoCtx[0].pr_mode_r != PR_MODE_ACTIVE1) ) )
                { 
                    printf("\tFile name> ");
                    fflush(stdin);
                    READ_PARAM("%s", fileName);
                
                    fileFormat = (tVideo_Format)getFileFormat( fileName );
                    if ( fileFormat == -1 )
                    {
                        printf("\tERROR: Illegal file name: %s\n", fileName);
                    }
                    else
                    {
                        printf("\tFile format: %d\n", fileFormat);               
                        PlayFromDB( 0, fileName, fileFormat );
                    }
                }
                break;                
                
            case 'l':  //Play multiple files from the PlayList
                display_menu = FALSE;			
                if ( ( (AudioCtx[0].pr_mode_p != PR_MODE_ACTIVE1) &&
                       (VideoCtx[0].pr_mode_p != PR_MODE_ACTIVE1) ) &&
                     ( (AudioCtx[0].pr_mode_r != PR_MODE_ACTIVE1) &&
                       (VideoCtx[0].pr_mode_r != PR_MODE_ACTIVE1) ) )
                {
                    printf("\tPlayList file name> ");
                    fflush(stdin);
                    READ_PARAM("%s", fileName);
                
                    PlayFromPlayList( 0, fileName );
                }
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
    DWORD   first_gw,
            last_gw;
    WORD    i,
            value;

    // First and last gateways to List Configuration On
    first_gw = 0;
    last_gw  = GwConfig[0].dNumGW - 1;
    
    // If there are multiple gateways, let the operator choose which on to command
    if (GwConfig[0].dNumGW > 1)
    {
        printf("\nEnter the number of the GW List "
               "(or enter 99 to list all gateways) > ");
        fflush(stdin);
        READ_PARAM("%hd", &value); 
        if (value != 99)
        {
            first_gw = value;
            last_gw  = value;
        }
    }

    for (i = (WORD)first_gw; i <= (WORD)last_gw; i++)   //Send querys to each of the channels
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
                GwConfig[i].vidRtpEp[EP_OUTB].Addr.EP.RtpRtcp.SrcIpAddress, 
                GwConfig[i].vidRtpEp[EP_OUTB].Addr.EP.RtpRtcp.nSrcPort,
                GwConfig[i].vidRtpEp[EP_OUTB].Addr.EP.RtpRtcp.DestIpAddress,
                GwConfig[i].vidRtpEp[EP_OUTB].Addr.EP.RtpRtcp.nDestPort);

        printf("\n    AUDIO RTP    %s:%5d <==> %s:%5d   REMOTE AUDIO (AMR)\n",
                GwConfig[i].AudRtpEp[EP_OUTB].Addr.EP.RtpRtcp.SrcIpAddress, 
                GwConfig[i].AudRtpEp[EP_OUTB].Addr.EP.RtpRtcp.nSrcPort,
                GwConfig[i].AudRtpEp[EP_OUTB].Addr.EP.RtpRtcp.DestIpAddress,
                GwConfig[i].AudRtpEp[EP_OUTB].Addr.EP.RtpRtcp.nDestPort);

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
    DWORD       query,
                expectedEvent,
                ret,
                first_gw,
                last_gw, index;
    WORD        i,
                value;
    CTA_EVENT   event;
    MSP_ACTION  mspAction;

    // First and last gateways to send commands to
    first_gw = 0;
    last_gw  = GwConfig[0].dNumGW - 1;

    // If there are multiple gateways, let the operator choose which on to command
    if (GwConfig[0].dNumGW > 1)
    {
        printf("\nEnter the number of the GW to send a query to "
               "(or enter 99 to send to all gateways) > ");
        fflush(stdin);
        READ_PARAM("%hd", &value); 
        if (value != 99)
        {
            first_gw = value;
            last_gw  = value;
        }
    }

    index = 0;
    if (gwParm.simplexEPs)
        index = whichEpDirection();

    printf("\nSending a status query to the AMR Bypass channel.");
    query = mspBuildQuery(MSP_FILTER_JITTER, 
                          MSP_QRY_JITTER_GET_STATE);
    for (i = (WORD)first_gw; i <= (WORD)last_gw; i++)   //Send querys to each of the channels
    {
        ret = mspSendQuery( GwConfig[i].AudByChan[index].hd, query );
        if (ret != SUCCESS)
        {
            printf("\n\t ERROR: mspSendQuery failed.\n");
        }
        expectedEvent = (MSPEVN_QUERY_DONE | MSP_QRY_JITTER_GET_STATE);
        
        ret = WaitForSpecificEvent( i, GwConfig[i].hCtaQueueHd, expectedEvent, &event );
		if ( ret != 0 )
		{
			printf("\n\tERROR: mspSendQuery failed to send valid completion event\n");     
		}
    
        //Record the query to file.
        if ( RecCmdFp !=NULL)
        {
            memset(&mspAction, '\0', sizeof (mspAction));
            mspAction.GWNo          = i;
            mspAction.objType       = MSP_FILTER_JITTER;
            mspAction.action        = MSP_SEND_QRY;
            mspAction.command       = query;
            mspAction.expectedEvent = expectedEvent;

        
            if ( fwrite(&mspAction, 1, sizeof (mspAction), RecCmdFp) != sizeof (mspAction))
            {
                printf("\n\tERROR: Failed to properly write command to file.\n");
            }
            fflush(RecCmdFp);
        }
    }
}


DWORD cmdFullTromboning( )
{
   
    WORD gwIndex;

    gwIndex = 0;
   
    if (GwConfig[0].dNumGW > 1)
    {
        printf("\nEnter the number of the GW to send a query to ");
        fflush(stdin);
        scanf("%hd", &gwIndex); 
  
    }
 
    sendCDE(gwIndex, CDE_LOCAL_TROMBONE, 0);

    return SUCCESS;
}

DWORD cmdStopTromboning( )
{
   
    WORD gwIndex;

    gwIndex = 0;
   
    mmPrint( gwIndex, T_SRVAPP, "Stop Tromboning\n" );
    
    if (GwConfig[0].dNumGW > 1)
    {
        printf("\nEnter the number of the GW to send a query to ");
        fflush(stdin);
        scanf("%hd", &gwIndex); 
    }
 
    //sendCDE(gwIndex, CDE_STOP_TROMBONE, 0);
    sendCDE(gwIndex, CDE_STOP_VIDEO_RX, 0);
    return SUCCESS;
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
        case 'P':   //Audio Passthru Cmds
            cmdAudPassthru();
            break;
        case 'O':   //Tromboning Command
            cmdFullTromboning();
            break;    
        case 'F':   //Stop Tromboning Command
            cmdStopTromboning();
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
    if (GwConfig[0].vidChannel[0].bActive)
    {
        printf("\t  p)  Audio Passthru Cmd    r)  Video Channel Query\n");
    }
    else
    {
        printf("\t  p)  Audio Passthru Cmd\n");
    }
    printf("\t  s)  AMR Bypass EP Command t)  AMR Bypass EP Query \n");
    
    if (GwConfig[0].AudByChan[0].bActive)
    {
        printf("\t  u)  AMR Bypass Chnl Cmd   v)  AMR Bypass Chnl Query \n");
    }
    
    if (GwConfig[0].CallMode == CALL_MODE_ISDN)
    {
        printf("\t  w)  List Full Config      x)  Call Control \n");
    }
    else
    {
        printf("\t  w)  List Full Configuration \n");
    }

    printf("\t  y)  H245 Commands         z)  Play/Record Commands \n");
    printf("\t  0)  TRC Commands \n");		
    printf("\t  o)  Perform Tromboning    f)  Stop Tromboning \n");
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
    NewEvent.ctahd              = GwConfig[ gw_num ].isdnCall.ctahd;
	
	ctaAllocBuffer( &(NewEvent.buffer), sizeof (PBX_PLACE_CALL_STRUCT) );
    memcpy( NewEvent.buffer, &pbxPlaceCall, sizeof (PBX_PLACE_CALL_STRUCT) );

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

void sendCDE( int gw_num, int cde, int value )
{
    CTA_EVENT NewEvent;  
    //printf("DEBUG sendCDE gw_num <%d>  cde <%d>  value <%d>\n", gw_num, cde, value);

	NewEvent.id       = cde;
	NewEvent.value    = value;
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
THREADRETURN THREADCALLCONV CommandLoop( void *param )
{
	BOOL      break_flag = FALSE;
	int       event_type, *pGwNum, nGw;
	DWORD     ret;
	char      key;
    CTA_EVENT event;

	pGwNum = (int *)param;
	nGw    = *pGwNum;

	if ( gwParm.auto_place_call )
    {
			autoPlaceCall(*pGwNum, "2000" ); //gwParm.dial_number);
	}	

	// Print out the command menu (only once)
	if ( gwParm.nPorts == 1 )
    {
	    PrintCmdMenu();	
    }
	//Main Loop
	while (break_flag == FALSE)
	{
        //Loop until either a keyboard event or CTA event
		for (;;)
		{
            //fflush (stdin);
			memset (&event, 0, sizeof (CTA_EVENT));

            ret = ctaWaitEvent( GwConfig[nGw].hCtaQueueHd, &event, 500 );  //100 ms timeout

			if (ret != SUCCESS)
            {
				mmPrintPortErr( *pGwNum, "ctaWaitEvent failure, ret= 0x%x, event.value= 0x%x, board error (event.size field) %x",
                                 ret, event.value, event.size);
			}
            else
            {
				if (event.id == CTAEVN_WAIT_TIMEOUT)
                {
				    //Didn't get a CTA event
					if ( gwParm.nPorts == 1 )
                    {
					    if (kbhit())
                        {
					    	event_type = MEGA_KBD_EVENT;
					    	break;
					    }
                        else
                        {
					    	continue;
					    }
					}

				}
                else
                { //Got a CTA event
					event_type = MEGA_CTA_EVENT;
					break;
				}
			}
		} //End for

		if (event_type == MEGA_KBD_EVENT)
		{
            while ((key = getchar()) == '\n')
            {
                /* Discard leading newlines */ ;
            }
			key = toupper(key);
#ifdef LINUX
			(void)getchar();    // Flush newline subsequent code is not prepared to deal with.
#else
            fflush(stdin);
#endif
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
			if (ret != SUCCESS)
            {
				mmPrint(nGw, T_GWAPP,"h324SubmitEvent returned 0x%x\n",ret);
            }
			
            if (!consumed)
            {
				break_flag = ProcessCTAEvent(nGw, &event);  
            }
		}
	} //End While
	return NULL;
}

// Get the video file name according to the extension (.3gp )
int getFileFormat( char *fileName )
{
    char *pExt;

    if( fileName == NULL )
    {
        return -1;
    }
    
    pExt = strrchr(fileName, '.');
    if ( pExt == NULL || pExt[1] == '\0' ) 
    {
        return -1;
    }
    
	if ( strcmp(pExt+1,THREEGP_FNAME_EXT) == 0 )
    {
        MMFILE          mmFile;
        FILE_INFO_DESC  fileInfoDesc;
        WORD            streamCount,
                        j;
        WORD            infoSizeLeft;               /* size left in file info buffer */
        DWORD           ret;
        int             blockSize;                  /* size of current block		 */
        WORD            codec = 0;
        BYTE            FileInfo [MAX_FILE_INFO_SIZE];
        BYTE           *pFileInfo;
        
        FILE_INFO_PRESENTATION  *pPresentation;     /* pointer on Presentation block */
        FILE_INFO_STREAM_VIDEO  *pStreamVideo;      /* pointer on Video Stream block	*/
         
        ret = mmOpenFile(fileName, OPEN_MODE_READ, FILE_TYPE_3GP,
                         NULL, VERBOSE_NOTRACES_LEVEL, &mmFile);
        if (ret != SUCCESS)
        {
            mmPrint(0, T_GWAPP,"Failed to Open MM File %s, error %x\n",
                    fileName, ret);
            return FAILURE;
        }

        fileInfoDesc.fileInfoSize = MAX_FILE_INFO_SIZE;
        fileInfoDesc.pFileInfo = &FileInfo [0];

        //Getting the information about the opened MM file
        ret = mmGetFileInfo(&mmFile, &fileInfoDesc);
        if (ret != SUCCESS)
        {
            mmPrint(0, T_GWAPP,"Failed to Get MM File info from file %s, error %x\n",
                    fileName, ret);
            return FAILURE;
        }

		mmCloseFile( &mmFile );
		
        pFileInfo    = fileInfoDesc.pFileInfo;
        infoSizeLeft = fileInfoDesc.fileInfoSize;
		
        pPresentation = (FILE_INFO_PRESENTATION *)pFileInfo;
        blockSize     = pPresentation -> blockHeader.blkSize;
        streamCount   = pPresentation -> streamCount;
        
        pFileInfo    += blockSize;
        infoSizeLeft -= blockSize;
		
        for (j = 0; j < streamCount; j++)
        {
            blockSize    = ((FILE_INFO_BLOCK_HEADER *)pFileInfo) -> blkSize;
            pStreamVideo = (FILE_INFO_STREAM_VIDEO *)pFileInfo;
            
            if (pStreamVideo -> header.streamType == STREAM_TYPE_VIDEO)
            {
                codec = pStreamVideo -> header.codec;
                break;
            }
            
            if (infoSizeLeft > 0)
            {
                pFileInfo    += blockSize;
                infoSizeLeft -= blockSize;
            }
        }
        
        if (codec != 0)
        {
            if ((codec == S_CODEC_H263) &&
                (pStreamVideo -> codec.h263.level == 30) &&
                (pStreamVideo -> codec.h263.profile == 3))
            {
                return H263_PLUS;
            }
            else
            {
                switch ( codec )
                {
                    case S_CODEC_MPEG4:
                        return MPEG4;
                    case S_CODEC_H263:
                        return H263;
                    default:
                        mmPrint(0, T_GWAPP, "Invalid video format!\n");			
                        return -1;
                }
            }
        }
    }     
    return -1;    
}

// Convert video codec to video format
tVideo_Format convertVideoCodec2Format( DWORD VideoCodec )
{

    if (VideoCodec == H324_MPEG4_VIDEO)
    {
        return (MPEG4);
    }
    else if (VideoCodec == H324_H263_VIDEO)
    {
        return (H263);
    }
    else
    {
        return (NO_VIDEO);
    }
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
    DWORD       ret;
   
    Flow(nGw, F_APPL, F_H324, "h324CloseChannel(audio in)"); 
    
    ret = h324CloseChannel( GwConfig[nGw].MuxEp.hd, H324RSN_AUDIO_IN );
    if ( ret != SUCCESS )
    {
        return ret;
    }

    ret = WaitForSpecificEvent( nGw, GwConfig[nGw].hCtaQueueHd, H324EVN_CHANNEL_CLOSED, &event );
    if ( ret != 0 )
    {
        printf("\nERROR: h324CloseChannel Audio IN failed to get remote ACK\n");  
        return ret;
    }
   
    Flow(nGw, F_APPL, F_H324, "h324CloseChannel(audio out)");
    ret = h324CloseChannel( GwConfig[nGw].MuxEp.hd, H324RSN_AUDIO_OUT );
    if ( ret != SUCCESS )
    {
        return ret;
    }
           
    ret = WaitForSpecificEvent( nGw, GwConfig[nGw].hCtaQueueHd, H324EVN_CHANNEL_CLOSED, &event );
    if ( ret != 0 )
    {
        printf("\nERROR: h324CloseChannel Audio OUT failed to get remote ACK\n");  
        return ret;
    }   
                                            
    Flow(nGw, F_APPL, F_H324, "h324CloseChannel(video)");
    
    ret = h324CloseChannel( GwConfig[nGw].MuxEp.hd, H324RSN_VIDEO );
    if ( ret != SUCCESS )
    {
      return ret;
    }
         
    ret = WaitForSpecificEvent( nGw, GwConfig[nGw].hCtaQueueHd, H324EVN_CHANNEL_CLOSED, &event );
    if ( ret != 0 )
    {
        printf("\nERROR: h324CloseChannel Video failed to get remote ACK\n");  
        return ret ;
    }   
   
    Flow(nGw, F_APPL, F_H324, "h324EndSession");
    
    ret = h324EndSession( GwConfig[nGw].MuxEp.hd );
    if( ret != SUCCESS )
    {
        return ret;
    }

    ret = WaitForSpecificEvent( nGw, GwConfig[nGw].hCtaQueueHd, H324EVN_END_SESSION_DONE, &event );
    if ( ret != 0 )
    {
        printf("\nERROR: h324EndSession failed to get remote ACK\n");  
        return ret;
    }       
    return SUCCESS;
}
