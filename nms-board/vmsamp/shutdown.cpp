/**********************************************************************
*  File - shutdown.cpp
*
*  Description - H324M interface shutdown related functions
*
*  2005  NMS Communications Corporation.
*
***********************************************************************/

///////////////////
// Include Files //
///////////////////
#include "mega_global.h"
#include "vm_trace.h"

void shutdown()
{

    CTA_EVENT   event;
    DWORD       ret, i;

    for(i=0; i<GwConfig[0].dNumGW; i++)
    {
        mmPrint(i, T_GWAPP, "Shutting Down Gateway %d\n\n",i);

		GwConfig[i].shutdown_in_progress = TRUE;

	    // Stop the H.324 middleware
        if( GwConfig[i].isdnCall.dCallState == CON ) {       
        	mmPrint(i, T_H245,"Stop H.324 middleware.\n");
		    ret = h324Stop(GwConfig[i].MuxEp.hd);
    		if (ret != SUCCESS) {
	    		mmPrintPortErr(i, "h324Stop returned 0x%x.\n", ret);
		    } else {
			    ret = WaitForSpecificEvent ( i, GwConfig[i].hCtaQueueHd, 
    						H324EVN_STOP_DONE, &event);
	        	if (ret != 0)
		        	mmPrintPortErr(i,"H324EVN_STOP_DONE event failure, ret= %x, waitEvent.value=%x\n", ret, event.value);
    		}
        }
                
         //Shut Down Audio Bypass CHANNEL
        if(GwConfig[i].AudRtpEp.bActive && GwConfig[i].MuxEp.bActive)
        {
            //Disable the MSP Channel
			if( GwConfig[i].AudByChan.bEnabled )
			{
            	mmPrint(i, T_MSPP,"Disable the Audio Channel.\n");                        
            	ret = mspDisableChannel(GwConfig[i].AudByChan.hd);
            	if(ret != SUCCESS)
                	mmPrintPortErr(i,"mspDisableChannel did not return SUCCESS\n");
            	else
                	ret = WaitForSpecificEvent ( i, GwConfig[i].hCtaQueueHd, 
								MSPEVN_DISABLE_CHANNEL_DONE, &event);
								
				GwConfig[i].AudByChan.bEnabled = FALSE;
			}

            //Disconnect the MSP Channel
           	mmPrint(i, T_MSPP,"Disconnect the Audio Channel.\n");                                    
            ret = mspDisconnect(GwConfig[i].AudRtpEp.hd,
                                GwConfig[i].AudByChan.hd,
                                GwConfig[i].MuxEp.hd);
            if(ret != SUCCESS)
                mmPrintPortErr(i,"mspDisconnect did not return SUCCESS\n");
            else
                ret = WaitForSpecificEvent ( i, GwConfig[i].hCtaQueueHd, 
							MSPEVN_DISCONNECT_DONE, &event);

            //Destroy the Channel  
           	mmPrint(i, T_MSPP,"Destroy the Audio Channel.\n");                                                
            ret = mspDestroyChannel(GwConfig[i].AudByChan.hd);
            if(ret != SUCCESS)
                mmPrintPortErr(i,"mspDestroyEndpoint did not return SUCCESS\n"); 
            else
                ret = WaitForSpecificEvent (i, GwConfig[i].hCtaQueueHd, 
							MSPEVN_DESTROY_CHANNEL_DONE, &event);
        }

        //If the MUX endpoint is active, shut it down
        if(GwConfig[i].MuxEp.bActive)
        {
            //Destroy the MUX Endpoint  
            mmPrint(i, T_MSPP,"Destroying MUX Endpoint, with MSP Handle %d.\n",GwConfig[i].MuxEp.hd);
            ret = mspDestroyEndpoint(GwConfig[i].MuxEp.hd);
            if(ret != SUCCESS)
                mmPrintErr("mspDestroyEndpoint did not return SUCCESS");
            else
                ret = WaitForSpecificEvent ( i, GwConfig[i].hCtaQueueHd, 
							MSPEVN_DESTROY_ENDPOINT_DONE, &event);
							
			GwConfig[i].MuxEp.bActive = FALSE;
        }

        //If the AMR Bypass RTP endpoint is active, shut it down
        if(GwConfig[i].AudRtpEp.bActive)
        {
           	mmPrint(i, T_MSPP,"Destroy the Audio Endpoint.\n");                                                
            //Destroy the Audio RTP Endpoint  
            ret = mspDestroyEndpoint(GwConfig[i].AudRtpEp.hd);
            if(ret != SUCCESS)
                mmPrintPortErr(i,"mspDestroyEndpoint did not return SUCCESS\n");
            else
                ret = WaitForSpecificEvent ( i, GwConfig[i].hCtaQueueHd, 
							MSPEVN_DESTROY_ENDPOINT_DONE, &event);
        }

    }

    // If the call mode is ISDN, shut down  the ISDN Stacks
    if( GwConfig[0].CallMode == CALL_MODE_ISDN )
    {
        isdnStop( );
    }
}
