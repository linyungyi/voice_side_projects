#ifndef _TLASREngDef_H_
#define _TLASREngDef_H_

#define CONNECT_TIME_OUT_MSEC	(5*1000)	// 5 sec., 1st time connect to server


#define _DEBUG

#ifdef _DEBUG
#define TLASRENG_SOCKET_TIME_OUT_MSEC		(5000*1000)	// 5000 sec.
//#define TLASRENG_SOCKET_TIME_OUT_MSEC		(2*1000)	// 2 sec.
#else
#define TLASRENG_SOCKET_TIME_OUT_MSEC		(5*1000)	// 5 sec.
#endif

enum NLexiconLocation
{
   LT_SYSTEM,			//0	fixed grammar, _digits, or server pre-loaded grammar
      LT_FILE,			//1   �Nlocal file����lexicon�ǵ�servser
      LT_BUFFER,		//2   �Nlocal buffer����lexicon�ǵ�servser
      LT_FILE_SERVER	//3	file in server site, for client/server architecture
};

////////////////////////
// �w�q���{���i���覡
enum NRcgType
{
   NRT_OnlyRcg,         // �HSyncBuffer()���覡�Asrv�@endp�Aclient�ݶȧ@���u����
   NRT_FullDuplexRcg,   // �HSyncBuffer()���覡�Asrv�@endp�Aclient�ݶi�������u����
   NRT_FrameSyncRcg,    // �Hinit, reset, sync, end���覡�Asrv���@endp
   NRT_TotalNum
};


//------------------------------------------------------------
//	define ASR Protocal
//
//	0 means OK for every ACK
//
#define AP_CHECK_VERSION		0
#define AP_CHECK_VERSION_ACK	100

#define AP_DISCONNECT			1		// don't need ACK

#define AP_ADD_GRAMMAR_NAME			11
#define AP_ADD_GRAMMAR_NAME_ACK		111

#define AP_ADD_GRAMMAR_DATA			12
#define AP_ADD_GRAMMAR_DATA_ACK		112

#define AP_ENABLE_GRAMMAR		13
#define AP_ENABLE_GRAMMAR_ACK	113

#define AP_DISABLE_GRAMMAR		14
#define AP_DISABLE_GRAMMAR_ACK	114

#define AP_REMOVE_GRAMMAR		15
#define AP_REMOVE_GRAMMAR_ACK	115

#define AP_SET_CAND_NUM			16
#define AP_SET_CAND_NUM_ACK		116

#define AP_REMOVE_ALL_ENGINES		17
#define AP_REMOVE_ALL_ENGINES_ACK	117

#define AP_REGISTER_GRAMMAR		18
#define AP_REGISTER_GRAMMAR_ACK	118

#define AP_SET_SPCH_FORMAT		19
#define AP_SET_SPCH_FORMAT_ACK	119

#define AP_DISABLE_ALL_GRAMMARS	20
#define AP_DISABLE_ALL_GRAMMARS_ACK	120

#define AP_REMOVE_ALL_GRAMMARS	21
#define AP_REMOVE_ALL_GRAMMARS_ACK	121

#define AP_GET_SPCH_DATA		22
#define AP_GET_SPCH_DATA_ACK	122

#define AP_SEND_SPCH_DATA		23
#define AP_SEND_SPCH_DATA_ACK	123

#define AP_SET_WAIT_TIME		30		// set max-wait time for ASR
#define AP_SET_WAIT_TIME_ACK	130

#define AP_START_RCG			31
#define AP_START_RCG_ACK		131

#define AP_STOP_RCG				32
#define AP_STOP_RCG_ACK			132

#define	AP_SYNC_BUFFER			33
#define	AP_SYNC_BUFFER_ACK		133

#define	AP_GET_RESULT			34
#define	AP_GET_RESULT_ACK		134

#define	AP_GET_CAND_NUM			35
#define	AP_GET_CAND_NUM_ACK		135

#define AP_GET_CANDIDATE		36
#define AP_GET_CANDIDATE_ACK	136

#define AP_SPEECH_END			37
#define AP_SPEECH_END_ACK		137

#define AP_PREPARE_ENGINE		38
#define AP_PREPARE_ENGINE_ACK	138

#define AP_STATE_RESET			39
#define AP_STATE_RESET_ACK		139			// state reset to ready

#define AP_CHECK				40				// ap to server to check if server alive
#define AP_CHECK_ACK			140			// check if ASR lives

#define AP_GET_RESIDUAL_SMP		41
#define AP_GET_RESIDUAL_SMP_ACK	141			// get how many pcm data residue

#define AP_LOCK					42
#define AP_LOCK_ACK				142			// get how many pcm data residue

#define AP_UNLOCK				43

#define AP_SET_ASR_PARA   44
#define AP_SET_ASR_PARA_ACK  144

#define AP_LOAD_HMM_MODEL					45				// �Nmodel load���O�����A�õ��@�ӥN��
#define AP_LOAD_HMM_MODEL_ACK				145			// Name:***,StateTable:****,HhmModel:*****,

#define AP_REMOVE_HMM_MODEL					46				// �N���w�N����model�q�O���餤����
#define AP_REMOVE_HMM_MODEL_ACK				146			// Name:***,

#define AP_GET_WAV_PATH   					47				// client ���^�������{��wav file path
#define AP_GET_WAV_PATH_ACK				147			// Path:***,

#define AP_SET_CALL_INFO   				48				// client �ӳ]�w�������{��ani dnis����������
#define AP_SET_CALL_INFO_ACK				148			// Ani:***,Dnis:***,



//	for RcgProcess protocal
#define AP_INIT			51
#define AP_INIT_ACK		151
#define AP_RESET		52
#define AP_RESET_ACK	152
#define AP_SYNC			53
#define AP_SYNC_ACK		153
#define AP_END			54
#define AP_END_ACK		154
#define AP_STOP			55
#define AP_STOP_ACK		155


/*
//----------------------------------------------------------
//	server-to-client msg
#define SC_ERR					201
#define SC_PROC_INIT_ERR		202
#define SC_PROC_RESET_ERR		203
#define SC_PROC_SYNC_ERR		204
#define SC_PROC_END_ERR			205
#define SC_PROC_STOP_ERR		206
*/

#define AP_FAIL_ACK			999


//	Flags for adding-grammar
#define MULTI_KEYWORDS	0x0001	// allow multi-keywords. "�x�_...�x�_"
#define IGNORE_IF_ADDED	0x0002	// ignore this fx. if lexion with same name had loaded
#define SYSTEM_PRELOAD	0x1000	// the grammar is preload mode
#define SYSTEM_PRELOAD_ACTIVE	0x2000	// the grammar is preload mode and as active

// Flags for enable-grammar
// data put in the lower to bytes
#define MAX_DIGIT_LENGTH 0x00010000	// max. digit length for known-length digit recognition

// result for bGetResult()
#define AR_NOT_START	0
#define AR_RUN			1		// still in RCG
#define AR_DONE		2		// OK
#define AR_TIMEOUT	3		// ASR timeout
#define AR_SPK_EARLY	4		// speak too early
#define AR_FAIL		5		// ASR fail�A�Ҧp������ѡA�ο��J���y���L�k�������{
#define AR_NoAvailableGrammar 6 //�S���������k�Q�Ұ�
#define AR_SpeechSaturated	7		// �y���Ӥj�n�w�g���M���F
#define AR_Stop	8		   // �Quser break �άOMIC fail�Ӥ����F

#endif
