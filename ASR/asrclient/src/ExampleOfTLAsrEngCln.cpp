#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include "cch.h"
#include "ccy.h"

#include "HighTimer.hpp"
#include "Speech.hpp"
#include "AsrEngCln_c.h"

#define DEFAULT_CAN_NUM 20

#define SPEECH_MANAGER_IP "10.144.7.109"
//#define SPEECH_MANAGER_IP "10.144.131.102"
#define SPEECH_MANAGER_PORT 17016
#define ASR_TYPE "ASR_CET_MIC16k"



static char *gszAsrType=ASR_TYPE;

static char *gszSpeechManagerIp0=SPEECH_MANAGER_IP;
static int  giSpeechManagerPort0=SPEECH_MANAGER_PORT;
static bool gbIsUseSpeechManager0=false;  // 直接連ASR server

static char *gszSpeechManagerIp1=NULL;
static int  giSpeechManagerPort1=SPEECH_MANAGER_PORT;
static bool gbIsUseSpeechManager1=true;

static char *gszSpeechManagerIp2=NULL;
static int  giSpeechManagerPort2=SPEECH_MANAGER_PORT;
static bool gbIsUseSpeechManager2=true;

static short gpsBuf[1000000];
static int giAsr=-1;


static char *gszGrmForTest="Stock";
static char gszBufGrm[70000]="朱國華 | 陳建宏 | davis | 陳保清 ;";
static bool gbIsUseBufGrm=false;

static bool gbIsUsePreloadGrm=true;

enum NGrmAddTypee{
   NGATE_Null,  // 不需add
   NGATE_GrmAtClient,  // 需add, grm file is at client site
   NGATE_GrmAtServer,  // 需add, grm file is at server site
   NGATE_TotalNum
};
static NGrmAddTypee gnIsNeedToAddGrmType=NGATE_Null; // 紀錄要enable 的grm要不add
static char *gpszGrmFileName="GrmFile.grm"; // 紀錄設定的 文法檔

static char gszWaveFileForTest[280]="TestInput.wav";

bool bSetUp0();

///////////////////////////
// 建立一個obj
bool bSetUp0()
{
NTLAsrEngErrorCode nRet;
FILE *fpPcm=NULL,*fpResult=NULL;
CHighTimer Ht1,Ht2;

   //////////////////////////////////
   // 測試create
   if(NTLAEEC_Normal!=(nRet=TLAsrEng_nCreateWithBackup(21,
      gszSpeechManagerIp0,giSpeechManagerPort0,gbIsUseSpeechManager0,
      gszAsrType,
      gszSpeechManagerIp1,giSpeechManagerPort1,gbIsUseSpeechManager1,
      gszSpeechManagerIp2,giSpeechManagerPort2,gbIsUseSpeechManager2
      ))){ // setup ASR environment, call once in an AP
      printf("Main() TLAsrEng_nCreateWithBackup() fail! %s(%d)",gszSpeechManagerIp0,giSpeechManagerPort0);
      return false;
   }

   //////////////////////////////////
   // 測試create後才TLAsrEng_nCreateClientObj
   nRet=TLAsrEng_nCreateClientObj(&giAsr);
   if(NTLAEEC_Normal!=nRet){
      printf("Create AsrClientObj fail after CREATE!(%d)\n",nRet);
      return false;
   }

   //////////////////////////////
   // 啟動sock log
   if(!TLAsrEng_bEnableLog(giAsr,".\\log\\Linux_asr_cln_0.log")){
      printf("TLAsrEng_bEnableLog fail!(%s)\n",TLAsrEng_cpcGetLastError(giAsr));
       return false;
	}
   return true;
}


////////////////////////////////////
// test StartRcg+SyncBuffer的rcg，由SRV作endp
int iTestSyncBufferRcgSrvEndp_Wav0(int iAsrHandle,const char *szWavFile,int iRcgType)
{
FILE *fpResult=NULL;
int iLen;
CHighTimer Ht1,Ht2;
CSpeech Speech;

CJstring tmp,str;
CJstring strInfo,strFea,strFileName;
CJstring strPath,strPersonDir;
static int iNo=0;
bool bIsFirst=true;
int iOfs;
bool bRet;
bool bIsAsrBusyFlag; // asr
int iAsrResult; // for


///////////////////////////////////////////////////////////////////////////////////////////
//  *3                              do offline rcg  eng                             //
///////////////////////////////////////////////////////////////////////////////////////////
   ////////////////////////////
   // ani dnisserver for CDRonline
   if(!TLAsrEng_bSetCallInfo(iAsrHandle,"Ani:1111,Dnis:3838,")){
      printf("TLAsrEng_bSetCallInfo fail!(%s)\n",TLAsrEng_cpcGetLastError(iAsrHandle));
      return false;
   }


   if(!Speech.bLoadWavFile(szWavFile)){
      printf("Can not open %s!\n",szWavFile);
      return false;
   }

   iLen=Speech.iGetSmpNum();
   Ht1.vGetCurCount();
   if(iLen>0){
      ////////////////////
      // A
      // 1. formatTLAsrEng_bSetSpeechFormat()
      if(!TLAsrEng_bSetSpeechFormat(iAsrHandle,NSFD_LINER16)){ // NSFD_LINER16
         printf("TLAsrEng_bSetSpeechFormat fail!(%s)\n",TLAsrEng_cpcGetLastError(iAsrHandle));
         return false;
      }

      ///////////////////////////////////////
      //// 4. do rcg
      // a. TLAsrEng_bStartRcg(iAsrHandle);
      // b. 20msecsilTLAsrEng_bSyncBuffer()25(=0.5 sec)
      // c. TLAsrEng_bSyncBuffer() speech dataresult
      // d. TLAsrEng_bSyncBuffer() 20msecsil75(=1.5 sec)
      // d. do can and wait for finished

//TryAgain:

      ///// a.
      switch(iRcgType){
      case 0:
      default:
         bRet=TLAsrEng_bStartRcg(iAsrHandle,NRT_OnlyRcg);
         break;
      case 1:
         bRet=TLAsrEng_bStartRcg(iAsrHandle,NRT_FullDuplexRcg);
         break;
      }
      if(!bRet){
         printf("Asr Error:%s\n",TLAsrEng_cpcGetLastError(iAsrHandle));
         return 12;
      }

      //////
      int giTotSmp;// smp
      short *psPcm;// speechpcm buffer
      int iCandNum;
      const char *szRcgResult;


      giTotSmp=Speech.iGetSmpNum();
      psPcm=Speech.m_psPcmBuffer;

      int ii;

      // SPEECH
      iOfs=0;
      bIsAsrBusyFlag=true;
      while(giTotSmp>0 && bIsAsrBusyFlag){
         if(giTotSmp>=1000){
            iLen=1000;
         } else {
            iLen=giTotSmp;
         }
         bRet=TLAsrEng_bSyncBuffer(iAsrHandle,psPcm+iOfs,iLen*sizeof(short),&iAsrResult);
         if(!bRet){
            printf("TLAsrEng_bSyncBuffer fail at Speech(%d smp)!(%s)\n",iOfs,TLAsrEng_cpcGetLastError(iAsrHandle));
            return 15;
         }

         switch(iAsrResult){
         case AR_RUN:
            break; // continue
         case AR_NOT_START:
            printf("AR_NOT_START\n");
            bIsAsrBusyFlag=false;
            break;
         case AR_DONE:
            printf("AR_DONE\n");
            bIsAsrBusyFlag=false;
            break;
         case AR_TIMEOUT:
            printf("AR_TIMEOUT\n");
            bIsAsrBusyFlag=false;
            break;
         case AR_SPK_EARLY:
            printf("AR_SPK_EARLY\n");
            bIsAsrBusyFlag=false;
            break;
         case AR_FAIL:
            printf("AR_FAIL\n");
            bIsAsrBusyFlag=false;
            break;
         case AR_NoAvailableGrammar:
            printf("AR_NoAvailableGrammar\n");
            bIsAsrBusyFlag=false;
            break;
         default:
            printf("ASR result is %d\n",iAsrResult);
            bIsAsrBusyFlag=false;
            break;
         }

         iOfs+=iLen;
         giTotSmp-=iLen;
      }

      // 1.5 sec sil
      iOfs=Speech.iGetSmpNum()-160;
      short psTmp[160];
      memset(psTmp,0,160*sizeof(short));
      for(ii=0;ii<75 && bIsAsrBusyFlag;ii++){
         bRet=TLAsrEng_bSyncBuffer(iAsrHandle,psTmp,160*sizeof(short),&iAsrResult);// 0.020*25=0.5 sec
         if(!bRet){
            printf("TLAsrEng_bSyncBuffer fail at tail sil(%d*0.02Sec)!(%s)\n",ii,TLAsrEng_cpcGetLastError(iAsrHandle));
            return 16;
         }

         switch(iAsrResult){
         case AR_RUN:
            break; // continue
         case AR_NOT_START:
            printf("AR_NOT_START\n");
            bIsAsrBusyFlag=false;
            break;
         case AR_DONE:
            printf("AR_DONE\n");
            bIsAsrBusyFlag=false;
            break;
         case AR_TIMEOUT:
            printf("AR_TIMEOUT\n");
            bIsAsrBusyFlag=false;
            break;
         case AR_SPK_EARLY:
            printf("AR_SPK_EARLY\n");
            bIsAsrBusyFlag=false;
            break;
         case AR_FAIL:
            printf("AR_FAIL\n");
            bIsAsrBusyFlag=false;
            break;
         case AR_NoAvailableGrammar:
            printf("AR_NoAvailableGrammar\n");
            bIsAsrBusyFlag=false;
            break;
         default:
            printf("ASR result is %d\n",iAsrResult);
            bIsAsrBusyFlag=false;
            break;
         }
      }

      TLAsrEng_bSpeechEnd(iAsrHandle);

      //
      CHighTimer WaitHt1,WaitHt2;
      double dTimeOut,dTimeUsed;
      WaitHt1.vGetCurCount();
      dTimeOut=10;
      while(bIsAsrBusyFlag){
         WaitHt2.vGetCurCount();
         dTimeUsed=WaitHt2.dTakeTimeSecFrom(WaitHt1);
         if(dTimeUsed> dTimeOut){
            printf("TLAsrEng_bGetResult wait for AR_DONE too long!(%ld)\n",dTimeUsed);
            dTimeOut+=1;
            //return 17;
         }
         bRet=TLAsrEng_bGetResult(iAsrHandle,500,&iAsrResult);// wait 0.5 sec
         if(!bRet){
            printf("TLAsrEng_bGetResult fail !(%s)\n",TLAsrEng_cpcGetLastError(iAsrHandle));
            return 18;
         }

         switch(iAsrResult){
         case AR_RUN:
            break; // continue
         case AR_NOT_START:
            printf("AR_NOT_START\n");
            printf("AR_NOT_START\n");
            bIsAsrBusyFlag=false;
            break;
         case AR_DONE:
            printf("AR_DONE\n");
            printf("AR_DONE\n");
            bIsAsrBusyFlag=false;
            break;
         case AR_TIMEOUT:
            printf("AR_TIMEOUT\n");
            printf("AR_TIMEOUT\n");
            bIsAsrBusyFlag=false;
            break;
         case AR_SPK_EARLY:
            printf("AR_SPK_EARLY\n");
            printf("AR_SPK_EARLY\n");
            bIsAsrBusyFlag=false;
            break;
         case AR_FAIL:
            printf("AR_FAIL\n");
            printf("AR_FAIL\n");
            bIsAsrBusyFlag=false;
            break;
         case AR_NoAvailableGrammar:
            printf("AR_NoAvailableGrammar\n");
            printf("AR_NoAvailableGrammar\n");
            bIsAsrBusyFlag=false;
            break;
         default:
            printf("ASR result is %d\n",iAsrResult);
            printf("ASR result is %d\n",iAsrResult);
            bIsAsrBusyFlag=false;
            break;
         }
      }


      if(iAsrResult!=AR_DONE){
         printf("iAsrResult!=AR_DONE\n");
         return 19;
      }


      ////////////////////////////
      //
      // 1. iWaitTimeMsec
      if(!TLAsrEng_bGetCanNum(iAsrHandle,&iCandNum,15000)){
         printf("TLAsrEng_bGetCanNum fail!(%s)\n",TLAsrEng_cpcGetLastError(iAsrHandle));
         return false;
      }
      Ht2.vGetCurCount();
      printf("======================================================================");
      printf("\nOffline AP endp(%s) : (%lf Sec)\n",szWavFile,Ht2.dTakeTimeSecFrom(Ht1));
//      fpResult=fopen("rcg_Result.txt","at+");
      if(fpResult!=NULL){
         fprintf(fpResult,"\n\noffline AP_Endp  CanNum:%d\n",iCandNum);
      }
      printf("offline AP_Endp CanNum:%d\n",iCandNum);
      for(ii=0;ii<iCandNum;ii++){
         ////////////////////////////
         // iCandNum
         // 1.CKeyValstringCKeyVal
         // 2.CKeyVal"k0:v0;k1:v1,""KeyPhr-0:,SpkPhr-0:,Value-0:.,Confd-0:90,"
         // 3.
         //   Lex:
         //   PhrNum:PhrNum(int)
         //   Confd: 0-100       (int)
         //   Prob:      (float)
         //   Frame: Frame
         //
         //       KeyPhr-N:N          ( char[])
         //       SpkPhr-N:N    ( char[])
         //       Value-N: N      ( char[])
         //       Confd-N: N0-100       (int)
         if(!TLAsrEng_bGetCandidate(iAsrHandle,ii,&szRcgResult)){
            printf("TLAsrEng_bGetCandidate fail!(%s)\n",TLAsrEng_cpcGetLastError(iAsrHandle));
            return false;
         }

         if(fpResult!=NULL){
            fprintf(fpResult,"(%d) =====>\n(%s)\n\n",ii,szRcgResult);
         }
         printf("(%d) =====>\n(%s)\n\n",ii,szRcgResult);
      }
      if(fpResult!=NULL){
         fclose(fpResult);
      }
   }
   return true;
}


///////////////////////////
//// 收尾
bool bTearDown0()
{
NTLAsrEngErrorCode nRet;
   nRet=TLAsrEng_nDestroyClientObj(giAsr);
   if(NTLAEEC_Normal!=nRet){
      printf("TLAsrEng_nDestroyClientObj(%d) fail!(%d)\n",giAsr,nRet);
      return false;
   }
   nRet=TLAsrEng_nDestroy();
   if(NTLAEEC_Normal!=nRet){
      printf("TLAsrEng_nDestroy() fail!(%d)\n",nRet);
      return false;
   }
   return true;
}


////////////////////////////////////////
// iTestNo: 哪一種測試
// iTestTimes: 在 setup 及 teardown 中的測試做幾次
int iTest0(int iTestTimes)
{
int iFailNum;
int iRcgNo;
   iFailNum=0;

   if(!bSetUp0() ){
      iFailNum++;
      goto End;
   } // 測試offline RCG:SRV do ENDP

   for(iRcgNo=0;iRcgNo<iTestTimes;iRcgNo++){
      if(!TLAsrEng_bGetEngine(giAsr,gszAsrType,"cch_client")){
         printf("TLAsrEng_bGetEngine fail!(%s)\n",TLAsrEng_cpcGetLastError(giAsr));
         goto NextTurn;
      }


      if(!TLAsrEng_bSetCanNum(giAsr,5)){
         printf("TLAsrEng_bSetCanNum fail!(%s) (%s)(%d)\n",TLAsrEng_cpcGetLastError(giAsr),__FILE__, __LINE__);
         goto NextTurn;
      }
      ////////////////////////
      // 設定啟動rcg後，當語者沒有說話時間超過iMsec毫秒，就馬上結束並完成辨認，
      if(!TLAsrEng_bSetMaxWaitTimeToSpk(giAsr,50000)){
         printf("TLAsrEng_bSetMaxWaitTimeToSpk fail!(%s) (%s)(%d)\n",TLAsrEng_cpcGetLastError(giAsr),__FILE__, __LINE__);
         goto NextTurn;
      }

      ////////////////////////
      // 設定啟動rcg後，當語者說話長度超過iMsec毫秒，就馬上結束並完成辨認，
      if(!TLAsrEng_bSetMaxSpeechDuration(giAsr,15000)){
         printf("TLAsrEng_bSetMaxSpeechDuration fail!(%s) (%s)(%d)\n",TLAsrEng_cpcGetLastError(giAsr),__FILE__, __LINE__);
         goto NextTurn;
      }

      if(gbIsUseBufGrm){
         if(!TLAsrEng_bAddGrammar(giAsr,gszGrmForTest, LT_BUFFER,gszBufGrm, 0)){
            printf("TLAsrEng_bAddGrammar fail!(%s) (%s)(%d)\n",TLAsrEng_cpcGetLastError(giAsr),__FILE__, __LINE__);
            goto NextTurn;
         }
      }

      //// add grammar
      switch(gnIsNeedToAddGrmType){
      case NGATE_GrmAtClient:
         if(!TLAsrEng_bAddGrammar(giAsr,gszGrmForTest, LT_FILE,gpszGrmFileName, 0)){
            printf("TLAsrEng_bAddGrammar fail!(%s) (%s)(%d)\n",TLAsrEng_cpcGetLastError(giAsr),__FILE__, __LINE__);
            goto NextTurn;
         }
         break;
      case NGATE_GrmAtServer:
         if(!TLAsrEng_bAddGrammar(giAsr,gszGrmForTest, LT_FILE_SERVER,gpszGrmFileName, 0)){
            printf("TLAsrEng_bAddGrammar fail!(%s) (%s)(%d)\n",TLAsrEng_cpcGetLastError(giAsr),__FILE__, __LINE__);
            goto NextTurn;
         }
         break;
      default:
         break;
      }

      if(gnIsNeedToAddGrmType!=NGATE_Null || gbIsUseBufGrm || gbIsUsePreloadGrm){
         if(!TLAsrEng_bEnableGrammar(giAsr,gszGrmForTest)){
            printf("TLAsrEng_bEnableGrammar fail!(%s) (%s)(%d)\n",TLAsrEng_cpcGetLastError(giAsr),__FILE__, __LINE__);
            goto NextTurn;
         }
      }

      if(!iTestSyncBufferRcgSrvEndp_Wav0(giAsr,gszWaveFileForTest,0) ){
         iFailNum++;
      } // 測試offline RCG:SRV no ENDP

      if(!TLAsrEng_bDisableAllGrammars(giAsr)){
         printf("TLAsrEng_bDisableGrammar fail!(%s) (%s)(%d)\n",TLAsrEng_cpcGetLastError(giAsr),__FILE__, __LINE__);
         goto NextTurn;
      }
      if(!TLAsrEng_bRemoveAllGrammars(giAsr)){
         printf("TLAsrEng_bRemoveAllGrammars fail!(%s) (%s)(%d)\n",TLAsrEng_cpcGetLastError(giAsr),__FILE__, __LINE__);
         goto NextTurn;
      }

NextTurn:
      if(!TLAsrEng_bReleaseEngine(giAsr)){
         printf("TLAsrEng_bReleaseEngine(%d) fail!(%s)\n",giAsr,TLAsrEng_cpcGetLastError(giAsr));
      }


      // break;
      printf("Finishing %d 次測試。\n",iRcgNo);
      printf("press any key to do next or q/Q to quit!!\n");
      char c;
      c=getchCch();
      switch(c){
      case 'q':
      case 'Q':
         goto End;
      }
   }

End:
   if(!bTearDown0() ){ iFailNum++;} // 收尾
   if(iFailNum==0){
      printf("\n\n=======> 測試 全部OK!!\n");
   } else {
      printf("\n\n=======> 測試  失敗!!\n");
   }

   return 1;
}

