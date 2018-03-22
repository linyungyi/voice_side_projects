#ifndef _tlttsapi_H_
#define _tlttsapi_H_

#pragma warning  (disable : 4786 4716 4251 4275)

#include "TLTtsErr.h"
#include "TLTtsDef.h"
          
#ifdef tlttsapi_EXPORTS
    #define tlttsapi_API __declspec(dllexport)
#elif defined tlttsapi_LIB
    #define tlttsapi_API
#else
    #define tlttsapi_API __declspec(dllimport)
#endif



class CTtsObj;

// This class is exported from the tlttsapi.dll
class tlttsapi_API CTLTtsApi 
{
    CTtsObj     *m_TtsObj;
    
public:
    
    CTLTtsApi();
    ~CTLTtsApi();
    
    //可填IP , Port 或Multicast Ip  , Port , TTL
    static bool bCreate(const char* cpcSpeechManager,const int ciPort=0,const int ciTTL=1);
    static void vClose();

    bool bGetEngine(const char* pcType);
    
    bool bReleaseEngine();
    
    int  iGetEngineType();

    bool bCheckExist();
    

    //START FILE-SHARING SYNTHESIS
    bool bText2RemoteFile(const char* cpcTextfname,const char* cpcSpchfname,int iTextlimit=125,int iFmt=0);
    
    //GET FILE-SHARING SYNTHESIS RESULT
    bool bGetResult(int iWaitTimeMsec, int* piResult,char* &pcFileNamelist,const char* cpcTextfname,const char* cpcSpchfname);
    
    //SEND TEXT-DATA , GET SPEECH-DATA AND SAVE TO FILE 
    bool bText2File(const char* textdata,const char* spfname,int iTextlimit=125,char* pcMode="tts",int iFmt=0,bool bSync=false);
    
    //Get Text2File Result File Number and List
    int iGetFileNum(const char* &cpcFileNameList);

    //SEND TEXT-DATA , GET SPEECH-DATA to Local BUFFER
    bool bText2Buffer(const char* textdata,int iTextlimit=125,char* pcMode="tts",int iFmt=0,bool bSync=false);
 
    //Get Speech buffer
    //return value  
    // 0       : there is no data (use bIsDone() to check "Is all done?") 
    // -1      : 
    // ulLen : remain data > ulLen            
    // if remain data < ulLen , return sizeof remain data
    long lGetBuffer(char* pcBuffer,unsigned long ulLen);
    
    //check data transfer thread status
    bool bIsDone();
    
    // wait for data transfer thread until all done
    void vWaitForDone();
    
    //SEND TEXT-DATA , GET SPEECH-DATA AND PLAYING 
    bool bText2Sb(const char* textdata,int iTextlimit=125,char* pcMode="tts",bool bSync=false);
    
    //CHECK IS STILL PLAYING?
    bool bIsPlaying();
    
    //STOP THREAD USED FOR SPEECH PLAYING 
    void vStopAudio();

    //play again
    void vPlayAgain();
        
    
    //STOP BOTH FILE-SHARING SYNTHESIS AND SEND TEXT-DATA SYNTHESIS 
    bool bStopTts();
    
    //SET SPEAKERS
    void vSetSpeaker(const char* cpcVoiceName,bool bChiOrEng=0);

    //Get Speaker List
    bool bGetSpeaker(char* &pcSpnamelist,int *piCount,bool bChiOrEng=0);
    

    int iGetState();

    int	 iGetLastError();
    const char*	 cpcGetLastError();

    void vSetAudioSamplingRate(int iSr);

    //////////////////////Corpus Func////////////////////////
    bool bGetLexList(char* &pcSpnamelist,int *piCount);
    bool bSetLexList(const char* pcSpnamelist);
    /////////////////////////////////////////////////////////


    ////////////TLPSLA and CORPUS FUNC////////////////////////
    //ADJ ENG and CHI 0-100
    void vSetParam(const int ciVolume,const int ciTempo,const int ciPitch);
    //PSOLA ONLY ADJ CHINESE ONLY 0.1-9.0
    void vSetParam(float fVolume,float fTempo,float fPitch);
    /////////////////////////////////////////////////////////
    
    //////////////////////TTSLINK FUNC///////////////////////////
    void vReset();               
    bool bAppendEmail(const char* szEmail);
    bool bAppendNumeric(const char* cpcNumber);
    bool bAppendDigit(const char* cpcDigit);
    bool bAppendSyl(const char* cpcImf);
    bool bAppendSyl(int* piSyl, int iSylNum);
    bool bAppendPhone(const char* cpcPhone);
    bool bAppendTLname(const char* szStr,const char* pcApName);
    bool bAppendFile(const char* pcFile, const char* pcApName);
    bool bAppendDate(const char* cpcDate);// Month/Day
    bool bAppendBig5(const char* cpcBig5);       // general text
    bool bAppend(const char* szStr);     // general text and english and digit
    
    // get append result
    // cpcFname=NULL(reserved for futrue)
    bool bFetch(char* &pcSndinfo, int *piSndCount,const char* cpcFname=NULL);
    
    //Get Speaker's AP List
    bool bGetAp(char* pcSpname, char* &pcApnamelist,int *piCount);
    //////////////////////////////////////////////////////////////

    //Enable api log
    void vEnableLog();


};


//************************* C interface******************************************//

//-------------------------------------------
tlttsapi_API const char* _stdcall TTS_GetLastError(int iHandle);
tlttsapi_API int _stdcall TTS_GetLastErrorId(int iHandle);
//-------------------------------------------
// Description          : Create TTS Env
// Return type          : tlttsapi_API void
tlttsapi_API void _stdcall 
TTS_Create(const char* pcSpeechManager);

//-------------------------------------------
// Description          :  
// Return type          : tlttsapi_API bool 
// pcType               : Caps
// piHandle(out)        : ttsobj HANDLE pointer
tlttsapi_API bool _stdcall 
TTS_GetEngine(const char* pcType, int* piHandle);


//-------------------------------------------
// Description          : 
// Return type          : tlttsapi_API bool 
// iHandle(in)          : ttsobj HANDLE 
tlttsapi_API bool _stdcall
TTS_ReleaseEngine(int iHandle);

//-------------------------------------------
// Description          : 
// iHandle(in)          : ttsobj HANDLE
// pcSpnamelist(out)    : Speaker Name List
// piCount(out)         : number of speaker
// bChiOrEng(in)        : Chinese=0, English=1
tlttsapi_API bool _stdcall
TTS_GetSpeaker(int iHandle,char* &pcSpnamelist,int *piCount,bool bChiOrEng=false);

//-------------------------------------------
// Description          :    
// iHandle(in)          : ttsobj HANDLE
// pcSpname(in)         : Speaker name
// pcApnamelist(out)    : Speaker's AP name List
// piCount(out)         : number of ap
tlttsapi_API bool _stdcall
TTS_GetAp(int iHandle,char* pcSpname,char* &pcApnamelist,int *piCount);

//-------------------------------------------
// Description          : Set Psola Param
// iHandle(in)          : ttsobj HANDLE    
// fVolume(in)          : fVolume
// fTempo(in)           : fTempo
// fPitch(in)           : fPitch            
tlttsapi_API bool _stdcall
TTS_SetParam(int iHandle,float fVolume,float fTempo,float fPitch);

//-------------------------------------------
// Description          : START FILE-SHARING SYNTHESIS
// iHandle(in)          : ttsobj HANDLE                 
// cpcTextfname(in)     : Text-Data filename
// cpcSpchfname(in)     : Output Speech File Name
// itextlimit(in)       : max number of words for each Segment
// iFmt(in)             : Output Speech Format(6)
tlttsapi_API bool _stdcall 
TTS_StartTtsFile(int iHandle,const char* cpcTextfname,const char* cpcSpchfname,
                 int iTextlimit=125,int iFmt=WAV_LINEAR16);

//-------------------------------------------
// Description          : GET FILE-SHARING SYNTHESIS RESULT
// iHandle(in)          : ttsobj HANDLE
// iWaitTime(in)        : wait for mSec
// cpcTextfname(in)     : Text-Data filename
// cpcSpchfname(in)     : Output Speech File Name
// piResult(out)        : Current Synthesis Result(NOT_START,FAIL,DONE,already finish amount)           
tlttsapi_API  bool _stdcall
TTS_GetResult(int iHandle, int iWaitTime, int* piResult,char* &pcFileNamelist,const char* cpcTextfname,
              const char* cpcSpchfname);

//-------------------------------------------
// Description          : SEND TEXT-DATA , GET SPEECH-DATA AND SAVE TO FILE 
// iHandle(in)          : ttsobj HANDLE 
// cpcTextdata(in)      : Text-Data Pointer
// cpcSpchfname(in)     : Output Speech File Name
// iTextlimit(in)       : max number of words for each Segment
// pcMode(in)           : TTSLINK tag(tts,syll,phone,number,tlname,big5,email,date)
// iFmt(in)             : Output Speech Format
// bSync(in)            : Sync or Async
tlttsapi_API  bool _stdcall
TTS_StartTtsFileEx(int iHandle,const char* cpcTextdata,const char* cpcSpchfname,
                   int iTextlimit=125,char* pcMode="tts",int iFmt=WAV_LINEAR16,
                   bool bSync=false);

//-------------------------------------------
// Description          : SEND TEXT-DATA , GET SPEECH-DATA BUFFER (PCM) 
// iHandle(in)          : ttsobj HANDLE         
// cpcTextdata(in)      : Text-Data Pointer
// iTextlimit(in)       : max number of words for each Segment
// pcMode(in)           : TTSLINK tag(tts,syll,phone,number,tlname,big5,email,date)
// iFmt(in)             : Output Speech Format
// bSync(in)            : Sync or Async
tlttsapi_API  bool _stdcall
TTS_StartTtsBuffer(int iHandle,const char* cpcTextdata,int iTextlimit=125,
                   char* pcMode="tts",int iFmt=0,bool bSync=false);

//----------------------------------------------
// Description          : Copy buffer, after using TTS_StartTtsBuffer
//                                        you must alloc buffer yourself
// Return value         : -1   , there is no data (use bIsDone() to check "Is all done?") 
//                                        ulLen , remain data > ulLen(your request)            
//                                        if remain data < ulLen , return sizeof remain data
// iHandle(in)          : ttsobj HANDLE 
// pcBuffer(out)        : buffer pointer
// ulLen(in)            : buffer Length
tlttsapi_API  long _stdcall TTS_GetBuffer(int iHandle,char* pcBuffer,unsigned long ulLen);

//-------------------------------------------
// Description          : CHECK Data trainsfer thread IS STILL RUNNING? 
// iHandle(in)          : ttsobj HANDLE 
tlttsapi_API  bool _stdcall TTS_IsDone(int iHandle);

//-------------------------------------------
// Description          : WAIT Data trainsfer thread UTIL finish 
// iHandle(in)          : ttsobj HANDLE 
tlttsapi_API  bool _stdcall TTS_WaitForDone(int iHandle);

//-------------------------------------------
// Description          : SEND TEXT-DATA , GET SPEECH-DATA AND PLAYING 
// iHandle(in)          : ttsobj HANDLE         
// cpcTextData(in)      : Text-Data Pointer
// iTextlimit(in)       : max number of words for each Segment
// pcMode(in)           : TTSLINK tag(tts,syll,phone,number,tlname,big5,email,date)
// bSync(in)            : Sync or Async
tlttsapi_API  bool _stdcall
TTS_StartTtsAudio(int iHandle,const char* cpcTextData,int iTextlimit=125,
                  char* pcMode="tts",bool bSync=false);

//-------------------------------------------
// Description          : STOP all SYNTHESIS 
// iHandle(in)          : ttsobj HANDLE 
tlttsapi_API  bool _stdcall TTS_StopTts(int iHandle);

//-------------------------------------------
// Description          : CHECK IS STILL PLAYING?
// iHandle(in)          : ttsobj HANDLE 
tlttsapi_API  bool _stdcall TTS_IsPlaying(int iHandle);

//-------------------------------------------
// Description          : STOP THREAD USED FOR SPEECH PLAYING 
// iHandle(in)          : ttsobj HANDLE 
tlttsapi_API  bool _stdcall TTS_StopAudio(int iHandle);

//-------------------------------------------
// Description          : change speaker  
// iHandle(in)          : ttsobj HANDLE 
// cpcSpname(in)        : SPEAKER NAME
// bChiOrEng(in)        : Chinese=0, English=1
tlttsapi_API  bool _stdcall TTS_vSetSpeaker(int iHandle,const char* cpcSpname,bool bChiOrEng=false);

//----------------------------------------------
// Description          : TTSLINK Reset 
// iHandle(in)          : ttsobj HANDLE 
tlttsapi_API  bool _stdcall TTS_Reset(int iHandle);

//----------------------------------------------
// Description          : TTSLINK append , use fetch to get result
// iHandle(in)          : ttsobj HANDLE 
// cpcTextData(in)      : Text-Data pointer
// pcMode(in)           : TTSLINK tag(tts,syll,phone,numeric,digit,tlname,big5,email,date)
//
// note: 1. TTS_Append(iHandle,"ㄐㄧㄤ ㄐㄧㄤ","syll"); -->for IMF
//          TTS_Append(iHandle,"2 278 278","syll");     -->for piSyl
//       2. TTS_Append(iHandle,"一 二","big5"); --> only append "一" 
//          because space , use "tts" tag if sentence has space
// example: 1. TTS_Append(iHandle,"梁所長 所長室","tlname tl104"); 
//          2. TTS_Append(iHandle,"emailreadername","file portal"); 
//          3. TTS_Append(iHandle,"/tlttsapiserver/spch/901205/jkc/common/bye.vox","file");-->fullpath 
tlttsapi_API  bool _stdcall
TTS_Append(int iHandle,const char* cpcTextData,char* pcMode="tts");

//----------------------------------------------
// Description          : Get TTSLINK append result
// iHandle(in)          : ttsobj HANDLE 
// pcSndinfo(in)        : Sound File Name Info (ex. 13 /tts/5002.wav 13 /tts/5003/wav)需自行配記憶體
// piSndCount(out)      : Sound File Count
// cpcFname(in)         : Output Speech File name(Reservsed for future)
tlttsapi_API  bool _stdcall
TTS_Fetch(int iHandle,char* &pcSndinfo, int* piSndCount,const char* cpcFname=NULL); 


//----------------------------------------------
tlttsapi_API  bool _stdcall TTS_CheckExist(int iHandle);
tlttsapi_API  void _stdcall TTS_Close();
tlttsapi_API  int  _stdcall TTS_GetEngineType(int iHandle);
tlttsapi_API  bool _stdcall TTS_GetLexList(int iHandle,char* &pcSpnamelist,int *piCount);
tlttsapi_API  bool _stdcall TTS_SetLexList(int iHandle,const char* pcSpnamelist);
tlttsapi_API  int _stdcall  TTS_GetState(int iHandle);
tlttsapi_API  void _stdcall TTS_EnableLog(int iHandle);
//************************* C interface******************************************//
//For Simple Interface Use begin here

//-------------------------------------------
// Description          : SEND TEXT-DATA , GET SPEECH-DATA AND SAVE TO FILE 
// iHandle(in)          : ttsobj HANDLE 
// cpcTextdata(in)      : Text-Data Pointer
// cpcSpchfname(in)     : Output Speech File Name
// iTextlimit(in)       : max number of words for each Segment
// pcMode(in)           : TTSLINK tag(tts,syll,phone,number,tlname,big5,email,date)
// iFmt(in)             : Output Speech Format
// bSync(in)            : Sync or Async
tlttsapi_API  bool _stdcall TTS_Tts2File(int iHandle,const char* cpcTextdata,const char* cpcSpchfname,int iFmt=WAV_LINEAR16,bool bSync=false);





#endif
