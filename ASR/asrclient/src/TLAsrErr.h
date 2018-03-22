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
#define FE_INV_HANDLE			9		// invalid handle 可能handle超過範圍或沒有對應任何 asr client obj
#define FE_SERVER_CONNECT_ERR	10		// connect to ASR/TTS server error
#define FE_SERVER_DATA_FORMAT_ERROR	11  // server 傳回來的 資料字串格式錯誤
#define FE_SET_CAND_NUM_ERR		12
#define FE_SERVER_CAND_FORMAT_ERROR	13  // server 傳回來的 cand 字串格式錯誤
#define FE_SYNC_BUFFER_FULL	14    //syncBuffer. server 的緩衝buffer爆掉了
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
#define FE_SERVER_RESPONSE_TIMEOUT   77  //Server在設定的時間內沒有回應

#define FE_WAVE_FILE_NOT_SAVED   80  //Server的設定並沒有儲存wave file
#define FE_WANTED_DATA_NOT_READY 81  //所要的資料還沒好或根本不會產生

#define FE_SUBGRAMMAR_CAN_NOT_ENABLE	82 // 名字如___***的subnet GRAMMAR不可自行enable
#define FE_NO_RESULT_READY    	83 // 還沒有辨認或沒有完成辨認

// for all
#define FE_NOT_IMPLEMENTED      200

////////////////////////////////////
//  以下定義 Asr Server ApMoniter的error code
#define ASRAEM_DoFirstSetup_Failed  100 // CMainTh::bDoFirstSetup() fail!
#define ASRAEM_EnglishWordNotInDict 101 // 有英文單字在字典中查不到


////////////////////////////////////
//  以下定義 GrmCHecker load 文法時的error code
#define GRM_NO_NAME_FIELD       1000 // simple word 有一行沒有name的欄位
#define GRM_ALIAS_LOST          1001 // 該有alias的地方，沒有alias
#define GRM_NOT_IN_ENGLISH_DICT 1002 // 此英文單字在字典中查不到
#define GRM_NOT_IN_STD_TABLE    1003 // 此中文字在TL std字表中找不到std code
#define GRM_NOT_IN_POIN_TABLE   1004 // 此中文字在破音字表中找不到
#define GRM_FILE_NOT_FOUND      1005 // 文法檔案無法開啟
#define GRM_SUBNET_NOT_FOUND    1006 // 無法在lexnet list中找到相對應的subnet
#define GRM_NOT_IN_HMM          1007 // 此unit name在 MultiTxHMM中找不到



#endif

//const char* cpcGetAsrErrMess(int iErr);
