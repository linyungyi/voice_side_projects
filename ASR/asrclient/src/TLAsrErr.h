#ifndef _TLASRERR_H_
#define _TLASRERR_H_

//-------------------------------------------
//	ASR client/server function Error
//

#define FUN_ERROR				-1

#define FE_OK					0		// no error

//	ASR object error
#define FE_ERR					1		// general err
#define FE_CREATE_ERR			2		// create ASR environment error
#define FE_NO_GRAMMAR			3		// pre-load grammar err
#define FE_MANAGER_NOT_FOUND	4		// can't connect Manager
#define FE_NOT_CREATE			5		// not create ASR environment
#define FE_SERVER_TIMEOUT		6
#define FE_NO_ASR_OBj			7
#define FE_WRONG_VERSION		8
#define FE_INV_HANDLE			9		// invalid handle �i��handle�W�L�d��ΨS���������� asr client obj
#define FE_SERVER_CONNECT_ERR	10		// connect to ASR/TTS server error
#define FE_SERVER_DATA_FORMAT_ERROR	11  // server �Ǧ^�Ӫ� ��Ʀr��榡���~
#define FE_SET_CAND_NUM_ERR		12
#define FE_SERVER_CAND_FORMAT_ERROR	13  // server �Ǧ^�Ӫ� cand �r��榡���~
#define FE_SYNC_BUFFER_FULL	14    //syncBuffer. server ���w��buffer�z���F
#define FE_SYNC_RCG_ERR			15
#define FE_STOP_RCG_ERR			16
#define FE_ASR_MODE_ERR			17		// fx. mode err (endp vs. frame-process mode) 
#define FE_RCG_RUNNING			18
#define FE_RCG_NOT_RUN			19

// grammar
#define FE_NO_SYSTEM_GRAMMAR	20
#define FE_ADD_GRAMMER_ERR		21
#define FE_ENABLE_GRAMMAR_ERR	22
#define FE_COMPILE_GRAMMER_ERR	23
#define FE_GRAMMAR_NAME_NOT_FOUND	24
#define FE_DISABLE_GRAMMAR_ERR	25
#define FE_GRAMMAR_TYPE_ERR		26
#define FE_PRELOAD_GRAMMAR_ERR	27
#define FE_GRAMMAR_HAS_ADDED	28		// grammar has added
#define FE_GRAMMAR_TOO_LARGE	29

#define FE_THREAD_NOT_STOP		30
#define FE_TIME_OUT				31		// general timeout
#define FE_START_THREAD_ERR		32
#define FE_STOP_THREAD_ERR		33

#define FE_GRAMMAR_OVERFLOW		37		// exceed the maximun grammar number

#define FE_OPEN_FILE_ERR		40
#define FE_MEMORY_SIZE_ERR		41		// memory allocated not enough
#define FE_PARAMETER_ERR		42
#define FE_SPEECH_FORMAT_ERR	43

//	get eneige
#define FE_INV_ENGINE_TYPE		50		// invalid engine type
#define FE_ENGINE_NOT_READY	51
#define FE_NO_FREE_ENGINE		52
#define FE_NO_MATCH_ENGINE		53
#define FE_PREPARE_ENGINE_ERR	54
#define FE_GRAMMAR_NOT_SET		55
#define FE_ENGINE_ALREADY_GOT	56
#define FE_ENGINE_SETUP_ERR		57

//	error in frame-propcess mode
#define FE_FUN_INIT_ERR			60
#define FE_FUN_RESET_ERR		61
#define FE_FUN_SYNC_ERR			62
#define FE_FUN_END_ERR			63
#define FE_FUN_STOP_ERR			64

#define FE_DATAPIPE_ERR			65
#define FE_ENDP_ERR				66
//
//	for client/server ASR
//
#define FE_SOCKET_ERR			70
#define FE_ACK_ERR				71		// server ACk error
#define FE_SOCKET_TIMEOUT		72		// 
#define FE_SEQUENCE_ERR			73		// function sequence error
#define FE_SOCKET_PROTOCAL_ERR	74
#define FE_CHANGE_PORT          75      //
#define FE_NO_FREEPORT          76
#define FE_SERVER_RESPONSE_TIMEOUT   77  //Server�b�]�w���ɶ����S���^��

#define FE_WAVE_FILE_NOT_SAVED   80  //Server���]�w�èS���x�swave file
#define FE_WANTED_DATA_NOT_READY 81  //�ҭn������٨S�n�ήڥ����|����

#define FE_SUBGRAMMAR_CAN_NOT_ENABLE	82 // �W�r�p___***��subnet GRAMMAR���i�ۦ�enable
#define FE_NO_RESULT_READY    	83 // �٨S����{�ΨS��������{

// for all
#define FE_NOT_IMPLEMENTED      200

////////////////////////////////////
//  �H�U�w�q Asr Server ApMoniter��error code
#define ASRAEM_DoFirstSetup_Failed  100 // CMainTh::bDoFirstSetup() fail!
#define ASRAEM_EnglishWordNotInDict 101 // ���^���r�b�r�夤�d����


////////////////////////////////////
//  �H�U�w�q GrmCHecker load ��k�ɪ�error code
#define GRM_NO_NAME_FIELD       1000 // simple word ���@��S��name�����
#define GRM_ALIAS_LOST          1001 // �Ӧ�alias���a��A�S��alias
#define GRM_NOT_IN_ENGLISH_DICT 1002 // ���^���r�b�r�夤�d����
#define GRM_NOT_IN_STD_TABLE    1003 // ������r�bTL std�r���䤣��std code
#define GRM_NOT_IN_POIN_TABLE   1004 // ������r�b�}���r���䤣��
#define GRM_FILE_NOT_FOUND      1005 // ��k�ɮ׵L�k�}��
#define GRM_SUBNET_NOT_FOUND    1006 // �L�k�blexnet list�����۹�����subnet
#define GRM_NOT_IN_HMM          1007 // ��unit name�b MultiTxHMM���䤣��



#endif

//const char* cpcGetAsrErrMess(int iErr);
