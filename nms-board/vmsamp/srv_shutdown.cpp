/**********************************************************************
*  File - srv_shutdown.cpp
*
*  Description - Video Messaging Server shutdown related functions
*
*  2005  NMS Communications Corporation.
*
***********************************************************************/

///////////////////
// Include Files //
///////////////////
#include "mega_global.h"
#include "srv_types.h"
#include "srv_def.h"
#include "vm_trace.h"

extern int ServiceCount;
extern CTA_SERVICE_DESC ServDesc[NUM_CTA_SERVICES]; /* for ctaOpenServices */
extern char* ServDesc_Close[NUM_CTA_SERVICES];
extern FILE* pLogFile;
extern DWORD  bAppLogFileSpecified;

static DWORD TearDownFusionChannel(CTAQUEUEHD QueueHd, AUDIO_CNXT_INFO Ctx, int port);
static int ClosePort(int port, CTAQUEUEHD QueueHd, CTAHD *CTAccessHd);

void serverShutdown()
{
	DWORD i, ret;
	CTA_EVENT event;

	for (i=0; i<mmParm.nPorts; i++)
	{
		/////////////////////////////////////////////////////////////////////////////////////////
		// Destroy Native Video Channel: ADI Port and Video RTP Endpoint
		/////////////////////////////////////////////////////////////////////////////////////////
		if(mmParm.Video_Format != NO_VIDEO)
		{
			ret = mspDestroyEndpoint(VideoCtx[i].rtpEp.hd);
	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("\nmspDestroyEndpoint returned %d.", ret);
		        exit(1);
	        }

            ret = WaitForSpecificEvent(i, GwConfig[i].hCtaQueueHd, MSPEVN_DESTROY_ENDPOINT_DONE, &event);
	        if (ret != 0)
	        {
		        mmPrintAll ("\nMSPEVN_DESTROY_ENDPOINT_DONE event failure, ret= %x,"
                        "\nevent.value=%x, board error (event.size field) %x\n\n",
			            ret, event.value, event.size);
		        exit(1);
	        }
			mmPrint(i, T_MSPP, "Native Video RTP Endpoint is destroyed...\n");

			mmPrint(i, T_CTA, "Destroy Native Video context...\n");
			ret = ClosePort(i, GwConfig[i].hCtaQueueHd, &VideoCtx[i].ctahd);
	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("\nClosePort failed, ret=%d.", ret);
		        exit(1);
	        }
        } // native video

		/////////////////////////////////////////////////////////////////////////////////////////
		// Destroy Native Audio Channel with no silence Detection:
		/////////////////////////////////////////////////////////////////////////////////////////
		if( mmParm.audioMode1 == NATIVE_NO_SD )
		{
			ret = mspDestroyEndpoint(AudioCtx[i].rtpEp.hd);
	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("\nmspDestroyEndpoint returned %d.", ret);
		        exit(1);
	        }

            ret = WaitForSpecificEvent(i, GwConfig[i].hCtaQueueHd, MSPEVN_DESTROY_ENDPOINT_DONE, &event);
	        if (ret != 0)
	        {
		        mmPrintAll ("\nMSPEVN_DESTROY_ENDPOINT_DONE event failure, ret= %x,"
                        "\nevent.value=%x, board error (event.size field) %x\n\n",
			            ret, event.value, event.size);
		        exit(1);
	        }
			mmPrint(i, T_MSPP, "Native Audio RTP Endpoint is destroyed...\n");

			mmPrint(i, T_CTA, "Destroy Native Audio context...\n");
			ret = ClosePort(i, GwConfig[i].hCtaQueueHd, &AudioCtx[i].ctahd);
	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("ClosePort failed, ret=%d.", ret);
		        exit(1);
	        }
        } // native audio

		/////////////////////////////////////////////////////////////////////////////////////////
		// Destroy Native Audio Channel With Silence Detection (or fusion IVR): 
		/////////////////////////////////////////////////////////////////////////////////////////
		else if( ( mmParm.audioMode1 == NATIVE_SD ) || ( mmParm.audioMode1 == FUS_IVR ) ) {
			if(ret != SUCCESS)
				mmPrintAll("\n\tERROR:  swiCloseSwitch did not return SUCCESS");

			ret = TearDownFusionChannel(GwConfig[i].hCtaQueueHd, AudioCtx[i], i);
	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("\nTearing down of the Fusion channel failed, ret=%d.", ret);
		        exit(1);
	        }

			mmPrint(i, T_CTA, "Destroy Native Audio context...\n");
			ret = ClosePort(i, GwConfig[i].hCtaQueueHd, &AudioCtx[i].ctahd);
	        if (ret != SUCCESS)
	        {
		        mmPrintAll ("ClosePort failed, ret=%d.\n", ret);
		        exit(1);
	        }
		}

	    if ((ret = ctaDestroyQueue(GwConfig[i].hCtaQueueHd)) != SUCCESS) {
        	mmPrintAll("\nFailed to destroy CT Access queue, return code is %x", ret);
	        exit(1);
    	}      
	    mmPrint(i, T_CTA, "ctaDestroyQueue, port %d <-- SUCCESS\n", i);
        
	} // for all ports
    
	if( ( mmParm.audioMode1 == NATIVE_SD ) || ( mmParm.audioMode1 == FUS_IVR ) ) {
        ret = swiCloseSwitch( hSrvSwitch0 );
    }
    

	if (bAppLogFileSpecified)
		fclose(pLogFile);

	return;
}

int ClosePort(int port, CTAQUEUEHD QueueHd, CTAHD *CTAccessHd)
{
	CTA_EVENT event;
	DWORD ret;

	if((ret = ctaCloseServices(*CTAccessHd, ServDesc_Close, ServiceCount) != SUCCESS))
	{
		mmPrintPortErr(port,"Failed to close services for Context %d..Return code is 0x%x.\n", CTAccessHd, ret);
		return ret;
	}
	ret = WaitForSpecificEvent(port, QueueHd, CTAEVN_CLOSE_SERVICES_DONE, &event );
	if (ret != SUCCESS)
	{
		mmPrintPortErr(port, "Close services failed for Context %d...event.value=0x%x\n", CTAccessHd, event.value);     
		return ret;
	}
	mmPrint(port, T_CTA, "CT Access services closed for Context %x\n", CTAccessHd);

	if ((ret = ctaDestroyContext(*CTAccessHd)) != SUCCESS)
	{
		mmPrintAll("\nFailed to destroy CT Access context\n");
		return ret;
	}
	mmPrint(port, T_CTA, "CT Access context destroyed, CtaHd = %x\n", CTAccessHd);

	return SUCCESS;
}

DWORD TearDownFusionChannel( CTAQUEUEHD QueueHd, AUDIO_CNXT_INFO Ctx, int port )
{
    CTA_EVENT event;
	DWORD ret;
			
	// Disconnect Audio RTP EP and Fusion DS0 EP using Fusion Channel
	ret = mspDisconnect(Ctx.rtpEp.hd, Ctx.FusionCh.hd, Ctx.FusionDS0Ep.hd);
	if (ret != SUCCESS)
	{
		mmPrintAll ("mspDisconnect returned %d.", ret);
		return FAILURE;
	}
	ret = WaitForSpecificEvent(port, QueueHd, MSPEVN_DISCONNECT_DONE, &event);
	if (ret != SUCCESS)
	{
		mmPrintPortErr(port, "MSPEVN_DISCONNECT_DONE event failure, ret= %x\n", ret);
        return FAILURE;
	}
    mmPrint(port, T_MSPP, "Disconnect the Fusion channel to the Audio RTP and Fusion DS0 endpoints done...\n");

	// Destroy Fusion Channel
	ret = mspDestroyChannel(Ctx.FusionCh.hd);
	if (ret != SUCCESS)
	{
		mmPrintPortErr(port, "mspDestroyChannel returned %d.\n", ret);
		return FAILURE;
	}
	ret = WaitForSpecificEvent(port, QueueHd, MSPEVN_DESTROY_CHANNEL_DONE, &event);
	if (ret != SUCCESS)
	{
		mmPrintAll ("\nMSPEVN_DESTROY_CHANNEL_DONE event failure, ret= %x,"
                "\nevent.value=%x, board error (event.size field) %x\n\n", ret, event.value, event.size);
		return FAILURE;
	}
	mmPrint(port, T_MSPP, "Fusion Channel is destroyed...\n");

	// Destroy Fusion DS0 Endpoint
    ret = mspDestroyEndpoint(Ctx.FusionDS0Ep.hd);
	if (ret != SUCCESS)
	{
		mmPrintAll ("\nmspDestroyEndpoint returned %d.", ret);
		return FAILURE;
	}
    ret = WaitForSpecificEvent(port, QueueHd, MSPEVN_DESTROY_ENDPOINT_DONE, &event);
	if (ret != SUCCESS)
	{
		mmPrintAll ("\nMSPEVN_DESTROY_ENDPOINT_DONE event failure, ret= %x,"
                "\nevent.value=%x, board error (event.size field) %x\n\n", ret, event.value, event.size);
		return FAILURE;
	} 
	mmPrint(port, T_MSPP, "Fusion DS0 Endpoint is destroyed...\n");


	// Destroy RTP0 Endpoint
	ret = mspDestroyEndpoint(Ctx.rtpEp.hd);
	if (ret != SUCCESS)
	{
		mmPrintAll ("mspDestroyEndpoint returned %d.", ret);
		return FAILURE;
	}
    ret = WaitForSpecificEvent(port, QueueHd, MSPEVN_DESTROY_ENDPOINT_DONE, &event);
	if (ret != 0)
	{
		mmPrintAll ("\nMSPEVN_DESTROY_ENDPOINT_DONE event failure, ret= %x,"
                "\nevent.value=%x, board error (event.size field) %x\n\n",
			    ret, event.value, event.size);
		return FAILURE;
	}
	mmPrint(port, T_MSPP, "Fusion Audio RTP Endpoint is destroyed...\n");

    
	return SUCCESS;
}
