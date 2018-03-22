#ifndef _TLTTSERR_H_
#define _TLTTSERR_H_

//-------------------------------------------
//	ASR client/server function Error
//

#define FUN_ERROR			-1

#define FE_OK				0		// no error

#define FE_ERR				1		// general err
#define FE_CREATE_ERR			2		// create ASR environment error
#define FE_MANAGER_NOT_FOUND		3		// can't connect Manager

#define FE_SERVER_TIMEOUT		4

#define FE_WRONG_VERSION		5
#define FE_INV_HANDLE			6		// invalid handle 


#define FE_THREAD_NOT_STOP		7
#define FE_TIME_OUT			8		// general timeout
#define FE_START_THREAD_ERR		9
#define FE_STOP_THREAD_ERR		10

#define FE_OPEN_FILE_ERR		11
#define FE_MEMORY_SIZE_ERR		12		// memory allocated not enough
#define FE_PARAMETER_ERR		13
#define FE_SPEECH_FORMAT_ERR		14

#define FE_INV_ENGINE_TYPE		15		// invalid engine type
#define FE_ENGINE_NOT_READY		16
#define FE_NO_FREE_ENGINE		17
#define FE_NO_MATCH_ENGINE		18
#define FE_PREPARE_ENGINE_ERR		19

//
//	for client/server ASR
//
#define FE_SOCKET_ERR			20
#define FE_ACK_ERR			21		// server ACk error
//#define FE_ACTION_ERR			22		// action err
#define FE_SEQUENCE_ERR			23		// function sequence error
#define FE_GETIPPORT_ERROR  24
#define FE_ISCONNECTED      25

//
// for lGetBuffer
//
#define FE_GETBUFFER_NOTSTART   26
#define FE_GETBUFFER_DELAY      27
#define FE_GETBUFFER_FINISH     28
#define FE_GETBUFFER_STOP       29
#endif
