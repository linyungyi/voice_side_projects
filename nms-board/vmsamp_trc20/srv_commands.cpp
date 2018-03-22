/**********************************************************************
*  File - srv_commands.cpp
*
*  Description - Video Messaging Server - the function to stop
*                audio/video play/record
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

void cmdStopVideo( int port )
{
    DWORD ret;

    mmPrint(port, T_SRVAPP, "Stopping Video Play/Record\n");

    if ((mmParm.Operation == RECORD) || (mmParm.Operation == SIM_PR)) {
        ret = adiStopRecording(VideoCtx[ port ].ctahd);
        if (ret != SUCCESS) {
                mmPrintPortErr(port, "nFailed to stop Recording for port %d, Err=%x\n", port, ret);
                exit(1);
        }
		// Use this flag while processing ADIEVN_RECORD_BUFFER_FULL
		VideoCtx[port].bStopRecordCalled = true;		
		// Set the coressponding flag for audio since this channel will be stopped automatically
		AudioCtx[port].bStopRecordCalled = true;						
    }
	
    if( ((mmParm.Operation == PLAY) || (mmParm.Operation == SIM_PR)) &&
		 (VideoCtx[port].pr_mode_p == PR_MODE_ACTIVE1) )
    {
        ret = adiStopPlaying(VideoCtx[ port ].ctahd);
        if (ret != SUCCESS) {
                mmPrintPortErr(port, "Failed to stop Playing for port %d, Err= %x\n", port, ret);
                exit(1);
        }
    }
		
    // Use this flag while processing ADIEVN_PLAY_BUFFER_REQ
    VideoCtx[port].bStopPlayCalled = true;				

    return;
}

void cmdStopAudio( int port )
{
    DWORD ret;
    CTAHD ctahd;

    mmPrint(port, T_SRVAPP, "Stopping Audio Play/Record\n");

    ctahd = AudioCtx[ port ].ctahd;
    if ((mmParm.Operation == RECORD) || (mmParm.Operation == SIM_PR)) {
        ret = adiStopRecording(ctahd);
        if (ret != SUCCESS) {
                mmPrintPortErr(port, "Failed to stop Recording for port %d, Err=%x\n", port, ret);
                exit(1);
        }
		// Use this flag while processing ADIEVN_RECORD_BUFFER_FULL
		AudioCtx[port].bStopRecordCalled = true;				
		// Set the coressponding flag for video since this channel will be stopped automatically
		VideoCtx[port].bStopRecordCalled = true;								
    }

    if( ((mmParm.Operation == PLAY) || (mmParm.Operation == SIM_PR)) &&
		 (AudioCtx[port].pr_mode_p == PR_MODE_ACTIVE1) )
	{
        ret = adiStopPlaying(ctahd);
        if (ret != SUCCESS) {
                mmPrintPortErr(port, "Failed to stop Playing for port %d, Err=%x\n", port, ret);
                exit(1);
        }
    }
		
    // Use this flag while processing ADIEVN_PLAY_BUFFER_REQ
    AudioCtx[port].bStopPlayCalled = true;					

    return;
}

void cmdStopPlayAudio( int port)
{
    DWORD ret;
    mmPrint(port, T_SRVAPP, "Stopping Audio Playing for port %d \n", port);

    ret = adiStopPlaying(AudioCtx[ port ].ctahd);
    if (ret != SUCCESS) {
            mmPrintPortErr(port, "Failed to stop audio Playing for port %d, Err=%x\n", port, ret);
            exit(1);
    }
		
    // Use this flag while processing ADIEVN_PLAY_BUFFER_REQ
    AudioCtx[port].bStopPlayCalled = true;	
    
    return;

}

void cmdStopPlayVideo( int port)
{
    DWORD ret;
    mmPrint(port, T_SRVAPP, "Stopping Video Playing for port %d \n", port);

    ret = adiStopPlaying(VideoCtx[ port ].ctahd);
    if (ret !=SUCCESS) {
        mmPrintPortErr(port, "Failed to stop Video Playing for the port %d, Err=%x\n", port, ret);
        exit(1);
    }
	    
    // Use this flag while processing ADIEVN_PLAY_BUFFER_REQ
    VideoCtx[port].bStopPlayCalled = true;		
	    
    return;

}
void cmdStopRecAudio( int port)
{
    DWORD ret;
    mmPrint(port, T_SRVAPP, "Stopping Audio recording for port %d \n", port);

    ret = adiStopRecording(AudioCtx[ port ].ctahd);
    if (ret != SUCCESS) {
            mmPrintPortErr(port, "Failed to stop audio Recording for port %d, Err=%x\n", port, ret);
            exit(1);
    }

	// Use this flag while processing ADIEVN_RECORD_BUFFER_FULL
	AudioCtx[port].bStopRecordCalled = true;
	// Set the coressponding flag for video since this channel will be stopped automatically
	VideoCtx[port].bStopRecordCalled = true;				
				
    return;

}

void cmdStopRecVideo( int port)
{
    DWORD ret;
    mmPrint(port, T_SRVAPP, "Stopping Video recording for port %d \n", port);

    ret = adiStopRecording(VideoCtx[ port ].ctahd);
    if (ret !=SUCCESS) {
        mmPrintPortErr(port, "Failed to stop Video recording for the port %d, Err=%x\n", port, ret);
        exit(1);
    }

	// Use this flag while processing ADIEVN_RECORD_BUFFER_FULL
	VideoCtx[port].bStopRecordCalled = true;		
	// Set the coressponding flag for audio since this channel will be stopped automatically
	AudioCtx[port].bStopRecordCalled = true;				

    return;

}

