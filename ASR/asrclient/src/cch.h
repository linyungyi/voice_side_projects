#ifndef CCH_H
#define CCH_H

#include <stdlib.h>

#include "CriticalSection.hpp"

#define MAX_MATCH_LEN 100  // match score �ɩү����J���̪�unit��

enum NXFileResult{
   XFILE_Normal,
   XFILE_FileWriteError,
   XFILE_FileReadError,
   XFILE_FileCanNotOpenToWrite,
   XFILE_FileNotFound,
   XFILE_FileTooShort
};

enum NCreateDirResult{
   DIR_CREATED,      // new dir is created
   DIR_EXIST,        // the dir exist already
   DIR_NOT_CREATED   // can not create the dir
};


enum NMatchResultType{
   NMRT_Correct,      // ��unit match���T
   NMRT_Wrong,        // ��unit ���~
   NMRT_Delete,       // ��unit �����F
   NMRT_Insert,       // ��unit ���h�X�Ӫ�
};


#define MAX_DivideException_NUM 1000 // �̦h���������Ӽ�
////////////////////////////////
// �HException���覡�ӰO���{���X�����m
// �o��gpiZeroForDivideException�Ȧb�{���}�lRUN�ɥ���0�A�Q�����@�����N�|����1�A�ϱo���m�O���u���@��
extern volatile int gpiZeroForDivideException[MAX_DivideException_NUM];//�o�ǭȦb�{���}�lRUN�ɥ���0�A�Q�����@�����N�|����1�A�ϱo���m�O���u���@��
extern volatile int giDivideExceptionNo; // ��AP�]�w�A�w��O�O���@�q�{���ҰO����
extern volatile bool gbIsDivideExceptionInited; // �O���O�_���}���Ĥ@��
extern volatile int giTmp; // for tmp use
extern CRITICAL_SECTION gCriticalDivideZero; // �Ψӭ����@���u�঳�@��THREAD����DivideZero

//////////////////////////////
// for cch using in Asr Server Debug
extern volatile int giSerialNoForException_Gui;//���� GUI ���m
extern volatile int giSerialNoForException_GuiEndTh;//���� GuiEndTh ���m
extern volatile int giSerialNoForException_MainTh;//���� MainTh ���m
extern volatile int giSerialNoForException_SpmTh;//���� SpmTh ���m
extern volatile int giSerialNoForException_AsrManagerTh;//���� AsrManagerTh ���m
extern volatile int gpiSerialNoForException_AsrObj[280];//���� AsrObj ���m

extern volatile int gpiSerialNoForException_RcgTh[280]; //���� RcgTh ���m
extern volatile int giSerialNoForException_CdrReporter;//���� CdrReporter ���m
extern volatile int giSerialNoForException_MonitorDemon;//���� MonitorDemon ���m



class CMatchResult{
public:
   CMatchResult(){
      m_iWholeSentence=0;
      m_iCorrectNum=0;
      m_iWrongNum=0;
      m_iDeleteNum=0;
      m_iInsertNum=0;

      m_iTotStdUnitNum=0;
      m_pnMatchResultType=NULL;
      m_piMatchPosInTest=NULL;

      m_piInsertPosInStd=NULL;
      m_piInsertUnitNo=NULL;

   };
   ~CMatchResult(){
      delete [] m_pnMatchResultType;
      delete [] m_piMatchPosInTest;

      delete [] m_piInsertPosInStd;
      delete [] m_piInsertUnitNo;
   };
   //// match score
   int m_iWholeSentence; // 1:���y���� 0:�Ϥ�
   int m_iCorrectNum;        // ���T��unit�ƥ�
   int m_iWrongNum;      // ���~��unit�ƥ�
   int m_iDeleteNum;     // �ֱ���unit�ƥ�
   int m_iInsertNum;     // �h�X�Ӫ�unit�ƥ�

   int m_iTotStdUnitNum; // ���J�зǪ�unit�ƥ�
   ////////////////////////
   // 1.����������G�u����correct and wrong delete
   NMatchResultType *m_pnMatchResultType; //�����۹����зǦr�ꪺmatch���G
   int *m_piMatchPosInTest;  // �����зǦr��match��test�r�ꪺ���m�Awhen correct and wrong
                             // ����, when delete,
   ////////////////////////
   // 2.����������G�u����insert�A���h�֭�insert�N�O�h�֭�
   int *m_piInsertPosInStd;  // ����insert unit���e������std�����m
   int *m_piInsertUnitNo;    // ����insert�i�Ӫ�unit �s��

};

///////////////////////////
// get the file size of the file handle
off_t filelength(int iFileHandle);

int getchCch();
int kbhit(void);

bool gbCatPath(char *szDes,int iMaxLen,const char *szDriver,const char *szFileName);


////////////////////////////////////
// 1. szDateTime's len must be 27 bytes
// 2. �H���yyyy�@���~���G"Date:yyyymmdd,Time:hhmmss,"
void gvGetDateTimeWithKeyValStyle(char *szDateTime);// szDateTime's len must be 27 bytes

////////////////////////////////////
// 1. �H�褸yyyy�@���~���G
//    bToInsertSeparater=true  "yyyy/mm/dd" 11 bytes
//                       false "yyyymmdd"     9 bytes
// 2. input: iDateTimeLen is  szDateTime's len
bool gbGetYearDate(char *szDate,int iDateTimeLen,bool bToInsertSeparater);

////////////////////////////////////
// 1. �H�褸yyyy�@���~���G
//    bToInsertSeparater=true  "yyyy/mm/dd hh:mm:ss" 20 bytes
//                       false "yyyymmdd_hhmmss"     16 bytes
// 2. input: iDateTimeLen is  szDateTime's len
bool gbGetYearDateTime(char *szDateTime,int iDateTimeLen,bool bToInsertSeparater);

////////////////////////////////////
// 1. �H�褸yyyy�@���~���A�Φbrbt��cdr�G
//    "20050408_10:23:43.104"
//    "yyyymmdd_hh:mm:ss.mmm" 22 bytes
// 2. input: iDateTimeLen is  szDateTime's len
bool gbGetRbtYearDateTime(char *szDateTime,int iDateTimeLen);

////////////////////////////////////
// 1. �H�褸yyyy�@���~���G
//    "yyyymmddhhmmss"     15 bytes
// 2. input: iDateTimeLen is  szDateTime's len
bool gbGetYearDateTime(char *szDateTime,int iDateTimeLen);

////////////////////////////////////
// 1. bToInsertSeparater=true  "mm/dd hh:mm:ss" 15 bytes
//                       false "mmdd_hhmmss"     12 bytes
// 2. input: iDateTimeLen is  szDateTime's len
bool gbGetDateTime(char *szDateTime,int iDateTimeLen,bool bToInsertSeparater);

////////////////////////////////////
// 1. bToInsertSeparater=true  "mm/dd hh:mm:ss:mmm" 19 bytes
//                       false "mmdd_hhmmssmmm"     15 bytes
// 2. input: iDateTimeLen is  szDateTime's len
bool gbGetDateTime2Msec(char *szDateTime,int iDateTimeLen,bool bToInsertSeparater);

////////////////////////////////////
// 1. bToInsertSeparater=true  "yyyy/mm/dd hh:mm:ss:mmm" 24 bytes
//                       false "yyyymmdd_hhmmssmmm"     19 bytes
// 2. input: iDateTimeLen is  szDateTime's len
bool gbGetYearDateTime2Msec(char *szDateTime,int iDateTimeLen,bool bToInsertSeparater);

////////////////////////////////////
// 1. bToInsertSeparater=true  "mm/dd" 6 bytes
//                       false "mmdd"  5 bytes
// 2. input: iDateTimeLen is  szDateTime's len
bool gbGetDate(char *szDateTime,int iDateTimeLen,bool bToInsertSeparater);

////////////////////////////////////
// 1. bToInsertSeparater=true  "hh:mm:ss" 9 bytes
//                       false "hhmmss"   7 bytes
// 2. input: iDateTimeLen is  szDateTime's len
bool gbGetTime(char *szDateTime,int iDateTimeLen,bool bToInsertSeparater);

////////////////////////////////////
// 1. bToInsertSeparater=true  "hh:mm:ss:mmm" 13 bytes
//                       false "hhmmssmmm"   10 bytes
// 2. input: iDateTimeLen is  szDateTime's len
bool gbGetTime2Msec(char *szDateTime,int iDateTimeLen,bool bToInsertSeparater);


//////////////////////////////////////////  XFile functions /////////////////////////////
//////////////////////////////////
// 1.�� pcInput[]�HpcMask[]�i��XOR�A�é��bpcOutPut[]���C
void gvXorBuf(const char *pcInput,char *pcOutput,int iInputLen,const char *pcMask,int iMaskLen);

//////////////////////////////////
// 1. szData must be '\0' ended
// 2. iDataLen do not inlcude the '\0' bytes
// 3. the offset between iStartPos must >= (iDataLen+1)
NXFileResult gnWriteXData(char *szFileName,char *szData,int iStartPos,int iDataLen);

//////////////////////////////////
// 1. szData must be '\0' ended
// 2. iDataLen do not inlcude the '\0' bytes
// 3. the offset between iStartPos must >= (iDataLen+1)
NXFileResult gnReadXData(char *szFileName,char *szData,int iStartPos,int iDataLen);


/////////////////////////////////
// 1. create dir from root to leafes, one by one
NCreateDirResult gnCreateDeepDir(const char *szPathEx);

////////////////////////////// tel no filter //////////////////////////
/////////////////////////
// 1.filter tel no to des buf
// return Num of tel no got
int giFilterTelNo(const char *szSrc,char *szDes);


////////////////////////////////
// find the szPattern from szBuf, and record start pos and end pos
bool  gbFindString(const char *szBuf,char *szPattern,int *piStartPos,int *piEndPos);

////////////////////////////////
// find the pos of first digit
bool  gbFindFirstDigit(const char *szBuf,int *piPos);

//////////////////////////
// ���Xarray�������ȳ̤j�����m
int giFindAbsMax(short *psBuf,int iLen);
int giFindAbsMax(int *piBuf,int iLen);
int giFindAbsMax(float *pfBuf,int iLen);
int giFindAbsMax(double *pdBuf,int iLen);

//////////////////////////////
// �b iFrom to iEnd���ۥX�̤j��
int giFindMax(double *pdBuf,int iFrom,int iEnd);


/***********************
* 1. alloc memory buf.
* 2. copy string to the buf.
*   return: the buf pointer.
*           NULL when szMsg==NULL
*/
char *gszAllocString(const char *szMsg);

//***************************************************************************
// �l�椸����
// Project Name: �@���`��function by cch
// Author: ���ا�
// Date: 91/01/23
// �\�໡��: iOfs�ѥثe���m���e�����Ĥ@�Ӥ��Ospace,0x0d,0x0a,0x09���a���C�i�H�OiOfs�ثe���m�C
// �P���L�l�椸���I�s���Y�P�I�s����:
// �ܼ�:
//    return new ofs
//           �Y�I��'\0'�Χ䤣���A�hreturn -1
// Revision:
//***************************************************************************
int giLastNotSpaceLnTab(const char *szBuf,int iOfs);

/////////////////////////////
// �\��:iOfs�ѥثe���m���Ჾ���Ĥ@�Ӥ��Ospace,0x0d,0x0a,0x09���a���C�i�H�OiOfs�ثe���m�C
// return new ofs
//        �Y�I��'\0'�A�hreturn -1
int giNextNotSpaceLnTab(const char *szBuf,int iOfs);

/////////////////////////////
// �\��:iOfs�ѥثe���m���Ჾ���Ĥ@�ӬOspace,0x0d,0x0a,0x09���a���C�i�H�OiOfs�ثe���m�C
// return new ofs
//        �Y�I��'\0'�A�hreturn -1
int giNextSpaceLnTab(const char *szBuf,int iOfs);

/////////////////////////////
// �\��:iOfs�ѥثe���m���Ჾ����iNum�ӬOszPattern���a���C�i�H�OiOfs�ثe���m�C
// return new ofs which is the begin position of the pattern
//        �Y�I��'\0'�A�hreturn -1
int giNextPattern(const char *szBuf,int iOfs,const char *szPattern,int iNum=1);

/////////////////////////////
// �\��:
// 1. skip space,0x0d,0x0a,0x09 in front of szSrc
// 2. copy the char before next space,0x0d,0x0a,0x09 from src to des
// 3. return new ofs at next first space,0x0d,0x0a,0x09
//        �Y�S���o����char�A�hreturn -1
int giGetNextString(const char *szSrc,int iSrcOfs,char *szDes,int iMaxDesLen);

/////////////////////////////
// �\��:�h���Y���space,0x0d,0x0a,0x09
void gvSkipHeadTailSpaceLnTab(const char *szInBuf,char *szOutBuf);

////////////////////////////
// �j���ʪ�����pc
bool gbShutDownPc();

////////////////////////////
// �j���ʪ�reboot pc
bool gbRebootPc();


/////////////////////////////
// �Narray��'A' to 'Z' ��char�ର 'a' to 'z'
void gvSzToLow(const char *szSrc,char *szDes,int iLen=-1);
/////////////////////////////
// �Narray��'a' to 'z' ��char�ର 'A' to 'Z'
void gvSzToUp(const char *szSrc,char *szDes,int iLen=-1);

/////////////////////////////
// �NszSrc�����Τj�p�g�^���r��Ϊ��ԧB�Ʀr���ର�b�Ϊ��^���r��
void gvWholeSizeAlphaDigitToHelfSize(const char *szSrc,char *szDes);

/////////////////////////////
// �NszSrc�����ΡB�b�Ϊ����I�Ÿ����ର�b�Ϊ�space
void gvAnyPunctuationToHelfSizeSpace(const char *szSrc,char *szDes);

/////////////////////////////
// �NszSrc���Y���space�h���B������space�u�d�@��
void gvSkipHeadTailSpace_KeepOneSpace(const char *szSrc,char *szDes);

bool gbMatchScore(int *piStdUnitNo, int iStdUnitNum, int *piTestUnitNo, int iTestUnitNo,CMatchResult *pMatchResult);

/*
///////////////////////////
// 1. �i����iPid=getpid();���Xprocess id�A�I�s��function
// 2. szExeFileName�|���J�������|
// 3. szDateTime����24 bytes �H�W(2004/01/09 10:11:35:168)
bool gGetProcessFileNameAndDateTime(int iPid,char *szExeFileName,int iExeFileNameMaxLen,char *szDateTime,int iDateTimeMaxLen);

///////////////////////////
// 1. �i����iPid=getpid();���Xprocess id�A�I�s��function
bool gGetProcessMemoryUsage(int iPid,unsigned int *puiCurMem,unsigned int *puiPeakMem);

///////////////////////////
// 1. �����@�ӧ����W�ߪ��{��
// 2. �Psystem() or spawn()���P�I�Osystem() or spawn()���ͥX�Ӫ�PROCESS�|�O�@��CHILD PROCESS�A�u�nCHILD PROCESS
//    �S�����AMAIN PROCESS�����F�A�hMAIN PROCESS�Ҧ���SOCKET PORT�٬O���|���
bool gbRunProcess(const char * cszFilename,const char * cszWorkpath,const char * cszPara);
*/


//////////////////////////////
// ���o���wFILE���̫������ɶ����ɮפj�p
//    "2005/04/08 10:23:43.717"
//    "yyyy/mm/dd hh:mm:ss.mmm" 24 bytes
//
bool bGetFileDateTimeSize(const char *cszFilePath,char *szDateTime,int iDateTimeLen,unsigned long *pulFileSizeLow,unsigned long *pulFileSizeHigh);

//////////////////////////////
// ���o���wFILE���̫������ɶ�
//    "2005/04/08 10:23:43.717"
//    "yyyy/mm/dd hh:mm:ss.mmm" 24 bytes
//
bool bGetFileDateTime(const char *cszFilePath,char *szDateTime,int iDateTimeLen);

//////////////////////////////
// ���o���wFILE���̫������ɶ�
//    "2005/04/08"
//    "yyyy/mm/dd" 11 bytes   bToInsertSeparater==true
//    "yyyymmdd" 9 bytes                          false
bool bGetFileDate(const char *cszFilePath,char *szDate,int iDateTimeLen,bool bToInsertSeparater);

//////////////////////////////
// ���o���wFILE���̫������ɶ�
//    "10:23:43"
//    "hh:mm:ss" 11 bytes   bToInsertSeparater==true
//    "hhmmss" 7 bytes
bool bGetFileTime(const char *cszFilePath,char *szTime,int iDateTimeLen,bool bToInsertSeparater);

//////////////////////////////
// ���o���wFILE���̫������ɶ�
//    "10:23:43.777"
//    "hh:mm:ss.777" 13 bytes   bToInsertSeparater==true
//    "hhmmss777" 10 bytes
bool bGetFileTime2Msec(const char *cszFilePath,char *szTime,int iDateTimeLen,bool bToInsertSeparater);

/////////////////////////////////////////////////////////////////////////
// cch global error log
// 1. �Τ@�Nmsg�������@��szLogFileName
// 2. �ϥ�criticalsection���Ҧ�thread���i�H����
// 3. szMsg�����e�����n�������ۤv�O�֡B�ɶ�
/////////////////////////////////////////////////////////////////////////
bool gbGlobalErrorLog(const char *szMsg);

////////////////////////////////////
// �}��log file
bool gbOpenGlobalErrorLog(const char *szLogFileName);

////////////////////////////////////
// ����log file
void gvCloseGlobalErrorLog();

/*
//////////////////////////////////////////////////////////////////////////
// API for Exception �B�z��

////////////////////////////////////
// ���ѱNSEHun signed int �ର _EXCEPTION_POINTERS obj
void gSe_translator(unsigned int e, _EXCEPTION_POINTERS* p);

////////////////////////////////////
// Exception Filter function
// �����ˬd _EXCEPTION_POINTERS ���e�æs��
unsigned int guiExceptionFilter(_EXCEPTION_POINTERS *pExceptionPointers,const char *szNameForException,volatile int *piSerialNoForException=NULL);

*/
//*************************************************************
// Auto-Delete or new �B�z��
//*************************************************************
//////////////////////////
// malloc() or realloc() �X�Ӫ�pointer�i�H�Φ�CLASS��auto free
class CAutoFreePointer{
public:
   void **m_ppvPt;
   CAutoFreePointer(){
      m_ppvPt=NULL;
   };
   ~CAutoFreePointer(){
      if(m_ppvPt!=NULL && *m_ppvPt!=NULL){
         free((char*)*m_ppvPt);
         *m_ppvPt=NULL;
      }
   };

   void vSetPointerToFree(void **ppvPt){
      m_ppvPt=ppvPt;
   }
};

//////////////////////////
// new �X�Ӫ�pointer�i�H�Φ�CLASS��auto delete
template<class CType>
class CAutoDeletePointer{
public:
   CType **m_ppvPt;
   CAutoDeletePointer(){
      m_ppvPt=NULL;
   };
   ~CAutoDeletePointer(){
      if(m_ppvPt!=NULL && *m_ppvPt!=NULL){
         delete [] (*m_ppvPt);
      }
   };

   void vSetPointerToFree(CType **ppvPt){
      m_ppvPt=(CType **)ppvPt;
   }
};

/*
///////////////////////////////////////
// �N UTF8 �ର Big5
// 1. Big5 �������r�O 2 bytes
// 2. Utf8 �������r�O 3 bytes
// 3. �õL�k�ˬd szSrc �O�_�u�� Utf8
// 4. if szDes's size is not enough then return false.
bool bUtf8ToBig5(const char *szSrc,char *szDes,int iDesSizeByteNum);

///////////////////////////////////////
// �N Big5 �ର Utf8
// 1. Big5 �������r�O 2 bytes
// 2. Utf8 �������r�O 3 bytes,�ҥH szDes �̫O�I�ݶ}�X szSrc �� 1.5 ���j
// 3. �õL�k�ˬd szSrc �O�_�u�� Big5
// 4. if szDes's size is not enough then return false.
bool bBig5ToUtf8(const char *szSrc,char *szDes,int iDesSizeByteNum);
*/

///////////////////////////////////////
// �N &gt;/&nbsp;/&lt;/&quot;/&amp; �ର-->(>)/( )/(<)/(")/(&)
// 1. if szDes's size is not enough then return false.
bool bHtmlToAscii(const char *szSrc,char *szDes,int iDesSizeByteNum);

///////////////////////////////////////
// �N  (>)/( )/(<)/(")/(&) �ର--> &gt;/&nbsp;/&lt;/&quot;/&amp;
// 1. �]��html��bytes �Ƹ��h�A�̫O�u�n 5 ��
// 2. if szDes's size is not enough then return false.
bool bAsciiToHtml(const char *szSrc,char *szDes,int iDesSizeByteNum);


//////////////////////////////////////////////
//      AutoDelete �ϥνd��
//////////////////////////////////////////////
////////////////////////////////////////////////////////////////
//  // malloc use CAutoFreePointer
//  void vTest1()
//  {
//  CTest *pcTmp1;
//  CAutoFreePointer AutoFreePointer;
//     AutoFreePointer.vSetPointerToFree((void **)&pcTmp1);
//     pcTmp1=(CTest *)malloc(100); // malloc or realloc �X�Ӫ� �n�� CAutoFreePointer��free
//     //pcTmp1=new CTest[100];    // �Ynew�X�Ӫ��A�� CAutoFreePointer��free �|�����D
//  }
//
//  /////////////////////////
//  // new use CAutoDeletePointer
//  void vTest2()
//  {
//  CTest *pTmp1;
//  CAutoDeletePointer<CTest> AutoDeletePointer1;
//     AutoDeletePointer1.vSetPointerToFree(&pTmp1);
//     //pTmp1=(CTest *)malloc(100*sizeof(CTest)); // �Ymalloc or realloc�X�Ӫ��A�� CAutoDeletePointer �� delete �|�����D
//     pTmp1=new CTest[100];                       // new �X�Ӫ� �n�� CAutoDeletePointer �� delete
//  }
//
//  /////////////////////////
//  // new use CAutoDeletePointer
//  void vTest3()
//  {
//  CTest (*pTmp2)[10];
//  CAutoDeletePointer<CTest [10]> AutoDeletePointer2;
//     AutoDeletePointer2.vSetPointerToFree(&pTmp2);
//     pTmp2=new CTest[100][10];
//  }
/////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// �� pfData �s�� bin ��
void gvSaveFloatFile(char *szFileName,float *pfData,int iNum);
//////////////////////////////////////////////////////////////////////////
// �� fData �ƻs iNum �ӦA�s�� bin ��
void  gvSaveFloatFileRepeat(char *szFileName,float fData,int iNum);

//////////////////////////////////////////////////////////////////////////
// �� pdData �s�� bin ��
void gvSaveDoubleFile(char *szFileName,double *pdData,int iNum);
//////////////////////////////////////////////////////////////////////////
// �� dData �ƻs iNum �ӦA�s�� bin ��
void  gvSaveDoubleFileRepeat(char *szFileName,double dData,int iNum);

//////////////////////////////////////////////////////////////////////////
// �� psData �s�� bin ��
void gvSaveShortFile(char *szFileName,short *psData,int iNum);
//////////////////////////////////////////////////////////////////////////
// �� sData �ƻs iNum �ӦA�s�� bin ��
void  gvSaveShortFileRepeat(char *szFileName,short sData,int iNum);

//////////////////////////////////////////////////////////////////////////
// �� pbData[]*fVal �s�� float32 bin ��
void gvSaveBoolFile(char *szFileName,bool *pbData,int iNum,float fVal);
//////////////////////////////////////////////////////////////////////////
// �� bData*fVal �ƻs iNum �ӦA�s�� float32 bin ��
void  gvSaveBoolFileRepeat(char *szFileName,bool bData,int iNum,float fVal);



#endif
