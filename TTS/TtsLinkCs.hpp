//
//	TTS using linking utterance 
//
#ifndef _TTS_LINK_CS
#define _TTS_LINK_CS

#ifdef tlttsapi_EXPORTS
    #define tlttsapi_API __declspec(dllexport)
#elif defined tlttsapi_LIB
    #define tlttsapi_API
#else
    #define tlttsapi_API __declspec(dllimport)
#endif



#pragma warning  (disable : 4786 4716 4251 4275)

#include "ttslink.hpp"
#include "tlttsdef.h"
#include "tlttserr.h"


//	max. AP 
#define MAX_TTS_AP	100

//	common Vox ID
#define TTS_SYLL	0
#define TTS_COMMON	1

class CTLTtsApi;



//------------------------------------------------------------
class  tlttsapi_API CTtsLinkCs : public CTtsLink
{
    
public:
        
    CTtsLinkCs();
    ~CTtsLinkCs();

    //For Corpus CS
    static char s_pcLexdir[255];
    static char s_pcSylldir[255];
    static bool s_bLog;

    //可填IP , Port 或Multicast Ip  , Port , TTL
    static void vSetupCorpus(const char* cpcLexdir,const char* cpcSylldir,const char *cpcIp,int iPort=0,int iTTL=1);
    static void vClose();
    static void vSetEnableLog();

    bool bGetEngine(const char* cpcCaps);
    bool bReleaseEngine();
    bool bCheckExist();
    
    //enable log
    void vEnableLog();
    
    //
    bool bAppendCorpus(const char* szStr);
    
    
    //
    bool bText2Buffer(const char* cpcTextdata,int iTextlimit=20,int iFmt=RAW_LINEAR16,bool bSync=false);
    
    //////////////////////Corpus Func////////////////////////
    bool bGetLexList(char* &pcSpnamelist,int *piCount);
    bool bSetLexList(const char* pcSpnamelist);
    /////////////////////////////////////////////////////////


    //////////////////////TLPSLA FUNC////////////////////////
    //只調中文 範圍 0.1-9.0
    void vSetParam(float fVolume,float fTempo,float fPitch);
    //同時調中、英文 範圍0-100
    void vSetParam(const int ciVolume,const int ciTempo,const int ciPitch);
    /////////////////////////////////////////////////////////
    
    //Get Speech buffer
    //return value  
    // 0       : there is no data (use bIsDone() to check "Is all done?") 
    // -2      : 
    // -1      : 
    // ulLen : remain data > ulLen            
    // if remain data < ulLen , return sizeof remain data
    long lGetBuffer(char* pcBuffer,unsigned long ulLen);
    
    //check data transfer thread status
    //bool bIsDone();
    
    // wait for data transfer thread until all done
    //void vWaitForDone();
    
    //
    bool bStopTtsAndClearBuffer();
    
    const char* cpcGetLastError();
    int         iGetLastError();
    
    CTLTtsApi *m_cstts;
    
};

#endif
