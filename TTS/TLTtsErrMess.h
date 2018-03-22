#ifndef TLTTS_ERR_MESS_H
#define TLTTS_ERR_MESS_H

const char*  _cppcErrMess[35] = 
{
	"No Err",					// FE_OK					0
	"general err",				// FE_ERR					1
	"create TTS env. err",		//  FE_CREATE_ERR			2
	"speech manager not found",	//  FE_MANAGER_NOT_FOUND	3
	"TTS server timeout",		//  FE_SERVER_TIMEOUT		4
	"version not match",		//  FE_WRONG_VERSION		5
	"invalid handle",			//  FE_INV_HANDLE	6		9
	"thread not stop",			//  FE_THREAD_NOT_STOP	7	30
	"time out",					//  FE_TIME_OUT	8			31
	"start thread err",			//  FE_START_THREAD_ERR	9	32
	"stop thread err",			//  FE_STOP_THREAD_ERR	10	33
	"open file err",			//  FE_OPEN_FILE_ERR	11	40
	"allocate memory err",		//  FE_MEMORY_SIZE_ERR		12
	"parameter err",			//  FE_PARAMETER_ERR	13	42
	"speech format err",		//  FE_SPEECH_FORMAT_ERR	14
	"invalide engine type",		//  FE_INV_ENGINE_TYPE		15		// invalid engine type
	"engine not ready",			//  FE_ENGINE_NOT_READY	16	51
	"no free engine",			//  FE_NO_FREE_ENGINE	17	52
	"not match engine",			//  FE_NO_MATCH_ENGINE	18	53
	"prepare engine err",		//  FE_PREPARE_ENGINE_ERR	19
	"socket err",				//  FE_SOCKET_ERR	20		70
	"socket ack err",			//  FE_ACK_ERR		21		71		// server ACk error
	"",							//  FE_ACTION_ERR			72		// action err
	"protocal sequence err",	//  FE_SEQUENCE_ERR		23		73		// function sequence error
	"Get Ip Port Error",        //  FE_GETIPPORT_ERROR 24
	"Is Already Connected",     //  FE_ISCONNECTED     25
	"GetBuffer Not Start",      //  FE_GETBUFFER_NOTSTART   26
    "GetBuffer Delay",          //  FE_GETBUFFER_DELAY      27
    "GetBuffer Finish",         //  FE_GETBUFFER_FINISH     28
    "GetBuffer STOP",        //  FE_GETBUFFER_STOP       29
};

#endif