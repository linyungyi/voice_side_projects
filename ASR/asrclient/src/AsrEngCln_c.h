//
//	ASR object for Client/Server , C-interface
//
#ifndef _ASRENGCLN_C_HPP_
#define _ASRENGCLN_C_HPP_

#include "SpeechFormatDef.h"
#include "TLAsrEngDef.h"
#include "TLAsrErrMess.h"

/*
#ifdef TLASRCLN_DLL
   #define TLASRCLN_C_API __declspec(dllexport)

#elif defined TLASRCLN_LIB
   #define TLASRCLN_C_API

#else
   #define TLASRCLN_C_API  __declspec(dllimport)
#endif
*/
#define TLASRCLN_C_API

#ifndef MANAGER_PORT
   #define MANAGER_PORT 5019
#endif

enum NTLAsrEngErrorCode
{
	NTLAEEC_Normal,          // ���`
	NTLAEEC_CreateEnvFail,   // �إ����ҥ���
	NTLAEEC_EnvNotCreated,   // �����٨S���إߡA�а���TLAsrEng_nCreate()
   NTLAEEC_CreatClientObjTooMany, // �إߤF�Ӧhclient obj�A�̦hMAX_ASRENG_NUM��
	NTLAEEC_InvalidHanlde,   // �ҵ���handle�èS���������client obj
};
//// obj��error code�аѦ� TLAsrErr.h

/////////////////////////////////////////////////////////////
// �إߦ@������api
/////////////////////////////////////////////////////////////

////////////////////////
// �]�wSpeechManager �� Asr Server��ip��port
// bIsUseSpeechManager�Gtrue �]�wSpeechManager��ip��port�A�n�g��SpeechManager���oAsr Server��ip��port
//                    �Gfalse �����]�wAsr Server��ip��port
// �Ǧ^�ȡGerror code
TLASRCLN_C_API	NTLAsrEngErrorCode TLAsrEng_nCreate(int iMaxObjNum,const char* cszSpmOrServerIp,int iSpmOrServerPort=MANAGER_PORT,bool bIsUseSpeechManager=true);

////////////////////////
// �]�w �h�� SpeechManager �� Asr Server��ip��port, �åB��ASR resource fail�����A�|���_�ˬd�Ʀb�e����ASR resource
//      �O�_���_
// bIsUseSpeechManager�Gtrue �]�wSpeechManager��ip��port�A�n�g��SpeechManager���oAsr Server��ip��port
//                    �Gfalse �����]�wAsr Server��ip��port
// �Ǧ^�ȡGerror code
TLASRCLN_C_API	NTLAsrEngErrorCode    TLAsrEng_nCreateWithBackup(
       int iMaxObjNum,const char* cszSpmOrServerIp,int iSpmOrServerPort=MANAGER_PORT,bool bIsUseSpeechManager=true,
       const char* cszAsrTypeForCheck=NULL,
       const char* cszSpmOrServerIp1=NULL,int iSpmOrServerPort1=0,bool bIsUseSpeechManager1=true,
       const char* cszSpmOrServerIp2=NULL,int iSpmOrServerPort2=0,bool bIsUseSpeechManager2=true);

////////////////////////
// ����Ҧ�resource
TLASRCLN_C_API NTLAsrEngErrorCode    TLAsrEng_nDestroy();

////////////////////////
// �إߤ@��asr engine client obj�b�O���鲣��client�ݹ���
TLASRCLN_C_API NTLAsrEngErrorCode    TLAsrEng_nCreateClientObj(int *piAsrHandle);

////////////////////////
// ���X�@�Ӥw�g�إߪ�asr engine client obj �� pointer
// return the pointer of the OBJ, or NULL while no such OBJ
TLASRCLN_C_API void *    TLAsrEng_pGetClientObjPointer(int iAsrHandle);

////////////////////////
// ����@��asr engine client obj�b�O���鲣��client�ݪ�����
TLASRCLN_C_API NTLAsrEngErrorCode    TLAsrEng_nDestroyClientObj(int iAsrHandle);


/////////////////////////////////////////////////////////////////////////////
//                         Engine �B�zapi
/////////////////////////////////////////////////////////////////////////////

////////////////////////
// ���o�@��asr server resource
// cpcType: Asr Engine �������W��
// pcClientName: client ���W�� �A��Asr server�ѧO��
TLASRCLN_C_API bool    TLAsrEng_bGetEngine(int iAsrHandle,const char* cszEngineType=NULL, const char* pcClientName=NULL);

////////////////////////
// ���asr resource
TLASRCLN_C_API bool    TLAsrEng_bReleaseEngine(int iAsrHandle);

////////////////////////
// �ˬd�w���o��asr engine�O�_�٦b�A�Y���b�h���s
// return false:asr engine���b�A�B���s����
TLASRCLN_C_API bool    TLAsrEng_bCheckExist(int iAsrHandle);

////////////////////////
// ����asr resource�A�s Asr Server�Ȯɤ��n���ʦ�Engine
TLASRCLN_C_API bool    TLAsrEng_bLock(int iAsrHandle);

////////////////////////
// �Ѷ}��asr resource������
TLASRCLN_C_API bool    TLAsrEng_bUnLock(int iAsrHandle);

/////////////////////////////////////////////////////////////////////////////
//                       ���{���k �B�zapi
/////////////////////////////////////////////////////////////////////////////
////////////////////////
// �W�[�@�Ӥ��k��server
// pcGrmName:��grm�bserver���N���W��
// nLexLocation: ��grm�����e�q���̨��o�A�аѦ�TLasrdef.h
// cszLexBufOrFileName: �s����grm��buffer�A�άO�ɮצW�١A�YnLexType==LT_SYSTEM�h�������Ѽ�
// iFlagToEng: ��server�B�z��grm���B�~�ѼơA�Ҧp IGNORE_IF_ADDED ���G�Y��grm�w�g�s�b�F�h������return true�A�аѦ�TLasrdef.h
TLASRCLN_C_API bool    TLAsrEng_bAddGrammar(int iAsrHandle,const char* pcGrmName, NLexiconLocation nLexLocation, const char* cszLexBufOrFileName, int iFlagToEng);

////////////////////////
// �P���@�Ӥ��k
// pcGrmName:��grm�bserver���N���W��
// iFlagToEng: ��server�B�z��grm���B�~�ѼơA�]server���P�Ӥ��P�A�Ҧp�i�H�w�q��grm�n����enable�����m�A�ݰѦ�server���W��
TLASRCLN_C_API bool    TLAsrEng_bEnableGrammar(int iAsrHandle,const char* pcGrmName, int iFlagToEng=0);

////////////////////////
// �T���@�Ӥ��k
// pcGrmName:��grm�bserver���N���W��
TLASRCLN_C_API bool    TLAsrEng_bDisableGrammar(int iAsrHandle,const char* pcGrmName);

////////////////////////
// �T���Ҧ����k
TLASRCLN_C_API bool    TLAsrEng_bDisableAllGrammars(int iAsrHandle);

////////////////////////
// �����@�Ӥ��k
// pcGrmName:��grm�bserver���N���W��
TLASRCLN_C_API bool    TLAsrEng_bRemoveGrammar(int iAsrHandle,const char* pcGrmName);

////////////////////////
// �����Ҧ����k
TLASRCLN_C_API bool    TLAsrEng_bRemoveAllGrammars(int iAsrHandle);

////////////////////////
// �����k�������ܰʫ��A�sengine�����w�Ʀn�C�Y�ƥ��S���I�s��api�Aengine�b���{�ɤ]�|�ۦ��I�s�@��
TLASRCLN_C_API bool    TLAsrEng_bPrepareEngine(int iAsrHandle);

/////////////////////////////////////////////////////////////////////////////
//            ��asr engine�Ӱ����(endp)�ҥΪ����{api
/////////////////////////////////////////////////////////////////////////////
////////////////////////
// �]�w�Ұ�rcg���A���W�LiMsec�@���A�y�̨S�����ܴN�⥢��
TLASRCLN_C_API bool    TLAsrEng_bSetMaxWaitTimeToSpk(int iAsrHandle,int iMsec);

////////////////////////
// �]�w�Ұ�rcg���A���y�̻��ܪ��׶W�LiMsec�@���A�N���W�����ç������{�A
TLASRCLN_C_API bool    TLAsrEng_bSetMaxSpeechDuration(int iAsrHandle,int iMsec);

////////////////////////////
// �]�w�������{��ani dnis���������Ƶ�server for CDR�A�s�u���\���]�w�@���Y�i�A�]�i�Honline�ܧ�
TLASRCLN_C_API bool    TLAsrEng_bSetCallInfo(int iAsrHandle,const char *szCallInfo);

////////////////////////
// �}�l�����rcg���{�A
// iRcgType ���w��asr srver�Τ��P�ժ�confidence threshold �� off_sil,
//        0: for only rcg �����p, 1: for fullduplex play and rcg �����p
TLASRCLN_C_API bool    TLAsrEng_bStartRcg(int iAsrHandle,NRcgType nRcgType=NRT_OnlyRcg);

////////////////////////
// �s�򪺱N�y�ưe�Jengine
TLASRCLN_C_API bool    TLAsrEng_bSyncBuffer(int iAsrHandle,void* pvPcm, int iByte,int* piAsrResult);		// warning !!! , iByte !!!

////////////////////////
// �S���y�ƤF�A���W�����ç������{
TLASRCLN_C_API bool    TLAsrEng_bSpeechEnd(int iAsrHandle);

////////////////////////
// AP �n�D�L���󰱤����{
TLASRCLN_C_API bool    TLAsrEng_bStopRcg(int iAsrHandle);

////////////////////////
// ���o�d�bengine���|���i�������sample��
TLASRCLN_C_API bool    TLAsrEng_bGetResidualSmp(int iAsrHandle,int* piSmpNum);

////////////////////////
// ���^engine�����쪺�y��
// �y����format��bSetSpeechFormat(NSpeechFormatDef nSpeechFormat)�]�w
TLASRCLN_C_API bool    TLAsrEng_bGetSpeechData(int iAsrHandle,void* pvPcm, int* piByte, int iMaxByte);

////////////////////////
// �ˬd�ثe�����{���p�A
// 1. �i�H�b�C��bSyncBuffer()���N�ˬd�@��
// 2. ��bSpeechEnd()�ᤣ�_�ˬd�������{����
TLASRCLN_C_API bool    TLAsrEng_bGetResult(int iAsrHandle,int iWaitTimeMsec, int* piResult);

/////////////////////////////////////////////////////////////////////////////
//            AP �ۤv�����(endp)�ҥΪ����{api�A�w�q�bCFrameProcess
/////////////////////////////////////////////////////////////////////////////
///////////////////////////
//  �}�l���{�e���_�l�ʧ@�A�q�`bInit()��bReset()���O�@���{�e���_�l�ʧ@�A
//  �C�����s���{(�Ҧp����{���{��������쪺�y��������)���|�I�sbReset()�A
//  ��bInit()�u�b�ǳƭn�}�l���{�e�@�@���Ӥw�C�ҥHbReset()�t�׭n�U�ֶV�n�A
//  ��ɥi�����ݭn���_�l�ʧ@���bbInit()���C
TLASRCLN_C_API bool    TLAsrEng_bInit(int iAsrHandle);	// call once when entering ENDP loop
TLASRCLN_C_API bool    TLAsrEng_bReset(int iAsrHandle);	// call at each time speech start/restart

////////////////////////////
// �B�z�y���e�����I����
TLASRCLN_C_API bool    TLAsrEng_bHeadSilence(int iAsrHandle,short *psData, int iSmp);

////////////////////////////
// �B�z��쪺�y��
TLASRCLN_C_API bool    TLAsrEng_bSync(int iAsrHandle,short *psData, int iSmp);	// frame-sync.

////////////////////////////
// �B�z���b�y���᭱���I����
TLASRCLN_C_API bool    TLAsrEng_bTailSilence(int iAsrHandle,short *psData, int iSmp);

////////////////////////////
// �w�g�S���y���F�A���W�������{���G
TLASRCLN_C_API bool    TLAsrEng_bEnd(int iAsrHandle);	// normal end

////////////////////////////
// �L���󵲧����{�A�]�S�����{���G
TLASRCLN_C_API bool    TLAsrEng_bStop(int iAsrHandle);	// abnormal stop

/////////////////////////////////////////////////////////////////////////////
//                       ���{���G �B�zapi
/////////////////////////////////////////////////////////////////////////////
////////////////////////////
// �]�w�n�Ǧ^�e�X�W�����{���G
// 1. score�����ۦP�����@�ӡA�Ҧp�P�����C
// 2. �]���@�n�Ǧ^iCanNum�Ӥ��Pscore�����{���G�A�ҥHbGetCanNum()�i���o����iCanNum�٦h�Ӫ����{���G
TLASRCLN_C_API bool    TLAsrEng_bSetCanNum(int iAsrHandle,int iCanNum);

////////////////////////////
// ���^�������{�@���h�֭ӿ��{���G���ƥ�
// 1. iWaitTimeMsec���ݿ��{�������ɶ�
TLASRCLN_C_API bool    TLAsrEng_bGetCanNum(int iAsrHandle,int *piCandNum, int iWaitTimeMsec=5000);

////////////////////////////
// ���^�������{��쪺���}�l�����m�Ϊ���
// 1.�����bbGetCanNum()���\���~�వ
TLASRCLN_C_API bool    TLAsrEng_bGetEndpStartSmpAndLen(int iAsrHandle,int *piStratSmp,int *piSmpNum);

////////////////////////////
// ���^�������{��iCandNo�ӿ��{���G
// 1.���{���G�HCKeyVal���榡string�Ǧ^�A�i�H��CKeyVal�ӸѶ}���o���e�C
// 2.CKeyVal���榡��"k0:v0;k1:v1,"�A�Ҧp"KeyPhr-0:���@,SpkPhr-0:���@,Value-0:�q��.���@,Confd-0:90,"
// 3.�C�@�ӿ��{���G���e�p�U�G
//   Lex:   �����{���G�Ҧb�����k
//   PhrNum:�����{���G�@���{�즳PhrNum�ӵ�(int)
//   Confd: �����{���G���i�H��0-100       (int)
//   Prob:  �����{���G�����{�o�쪺���v    (float)
//   Frame: �����{���G�@��Frame�ӻy��
//    �C�@�ӿ��{�쪺�������e�p�U
//       KeyPhr-N:��N�ӵ����N�����r          (�r�� char[])
//       SpkPhr-N:��N�ӵ����y�̩Ұ᪺���r    (�r�� char[])
//       Value-N: ��N�ӵ��ұa���y�N���e      (�r�� char[])
//       Confd-N: ��N�ӵ����i�H��0-100       (int)
TLASRCLN_C_API bool    TLAsrEng_bGetCandidate(int iAsrHandle,int iCandNo, char const** ppcCand);

////////////////////////////
// ���^�������{�Ҧs�U�Ӫ�wav file path,�����bbGetCanNum()���\���~�i�H�I�s
TLASRCLN_C_API bool   TLAsrEng_bGetWavPath(int iAsrHandle,char **pszWavPath);

////////////////////////
//	���^��iCand�W�����{���G�A���bbuffer���A�é��Jm_kvCurCand��
TLASRCLN_C_API bool    TLAsrEng_bGetCandidateToKv(int iAsrHandle,int iCandNo);

////////////////////////
//	�N�bm_kvCurCand�������{���G��key���X��
TLASRCLN_C_API bool    TLAsrEng_bGetCandidateDataFromKv(int iAsrHandle,const char *szKey,char *szVal,int iMaxValLen,int *piValLenGot);

/////////////////////////////////////////////////////////////////////////////
//                       ���L api
/////////////////////////////////////////////////////////////////////////////
////////////////////
// �]�w���X�J�y����format�Afunction �pbSyncBuffer()�BbGetSpeechData()�B
//   bHeadSilence()�BbTailSilence()�BbSync()���ھڳo��SpeechFormat�ӨM�wdata���j�p�榡
TLASRCLN_C_API bool    TLAsrEng_bSetSpeechFormat(int iAsrHandle,NSpeechFormatDef nSpeechFormat);

////////////////////
// �����J�y����������A���ӥh�������{
// 1. �y����format��TLAsrEng_bSetSpeechFormat()�]�w
// 2. bIsDoEndp:1 ASR engine �|���@�����,�A�N��쪺���i�����{
//              0 ASR engine �����N���J�����i�����{
TLASRCLN_C_API bool    TLAsrEng_bOffLineRcg(int iAsrHandle,void* pvPcm, int iByte,bool bIsDoEndp);


////////////////////
// �禡���楢�ѮɡA���o�̫��@���o�ͪ������~�X�A���{���ݪ�
TLASRCLN_C_API int    TLAsrEng_iGetLastError(int iAsrHandle);

////////////////////
// �禡���楢�ѮɡA���o�̫��@���o�ͪ������~�T���X�A���{�����ݪ�
TLASRCLN_C_API const char*	  TLAsrEng_cpcGetLastError(int iAsrHandle);

////////////////////
// 1. �禡���楢�ѮɡA���o�̫��@���o�ͪ������~�T���X�A���{�����ݪ�
// 2. �\���MTLAsrEng_cpcGetLastError�@�ˡA���]�L�k�ǥ��T��BSTR�r���^�h��vb,�G���Ħ�api���oerror msg
TLASRCLN_C_API bool  TLAsrEng_bGetLastErrorStringVb(int iAsrHandle,char *szErrorMsg,int iMaxErrorMsgLen,int *piErrorMsgLenGot);

//////////////////////////////////
// �Ұʹ�sock data�@log
TLASRCLN_C_API bool    TLAsrEng_bEnableLog(int iAsrHandle,const char *szLogFileName);
//////////////////////////////////
// ������sock data�@log
TLASRCLN_C_API bool    TLAsrEng_bDisableLog(int iAsrHandle);

#endif
