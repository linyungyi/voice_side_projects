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

    mmPrint(port, T_SRVAPP, "Stopping Video Record\n");

	if (mmParm.Operation == RECORD) {
		ret = adiStopRecording(VideoCtx[ port ].ctahd);
		if (ret != SUCCESS) {
			mmPrintPortErr(port, "nFailed to stop Recording for port %d, Err=%x\n", port, ret);
			exit(1);
		}
	} else {
		ret = adiStopPlaying(VideoCtx[ port ].ctahd);
		if (ret != SUCCESS) {
			mmPrintPortErr(port, "Failed to stop Playing for port %d, Err= %x\n", port, ret);
			exit(1);
		}
	}

	return;
}

void cmdStopAudio( int port )
{
	DWORD i, ret;
	CTAHD ctahd;

    mmPrint(port, T_SRVAPP, "Stopping Audio Record\n");
	
	ctahd = AudioCtx[ port ].ctahd;
	if (mmParm.Operation == RECORD) {
		ret = adiStopRecording(ctahd);
		if (ret != SUCCESS) {
			mmPrintPortErr(port, "Failed to stop Recording for port %d, Err=%x\n", port, ret);
			exit(1);
		}
	} else { // PLAY
		ret = adiStopPlaying(ctahd);
		if (ret != SUCCESS) {
			mmPrintPortErr(port, "Failed to stop Playing for port %d, Err=%x\n", port, ret);
			exit(1);
		}
	}

	return;
}
