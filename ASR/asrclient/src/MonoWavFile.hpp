#ifndef _MonoWavFile_HPP_
#define _MonoWavFile_HPP_

#include <stdio.h>
#include "AsrError.hpp"

#ifndef WaveFormatForMonoWav
#define WaveFormatForMonoWav

//-------------------------------------------------------------------
//	Microsoft definition, in mmreg.h
//
#define  WAVE_FORMAT_UNKNOWN    0x0000  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_PCM		  1
#define  WAVE_FORMAT_ADPCM      0x0002  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_IEEE_FLOAT 0x0003  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_ALAW       0x0006  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_MULAW      0x0007  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_OKI_ADPCM  0x0010  /*  OKI  */
#define  WAVE_FORMAT_DVI_ADPCM  0x0011  /*  Intel Corporation  */
#define  WAVE_FORMAT_IMA_ADPCM  (WAVE_FORMAT_DVI_ADPCM) /*  Intel Corporation  */

//	TL defintion
enum NWaveFormat {
   NWF_Signed16=1,     // 16 bit pcm
   NWF_Unsigned8,    // 8 bit pcm
   NWF_Mulaw8,        // 8 bit MuLaw
   NWF_Alaw8,        // 8 bit ALaw
   NWF_Float32        // 32 bit ieee float
};




enum NMonoWavFileErrorCode
{	
	NMWFEC_Normal,			// O.K.
	NMWFEC_FileNotFound, // 無法開檔
	NMWFEC_FormatError, // wav data的格式不對
	NMWFEC_NotSupportWavFormat, // wav data的格式尚未支援
};

#endif


class CMonoWavFile
{
public:
	
   CMonoWavFile(){
      m_iSampleRateRead=8000;     // opened file sample rate
      m_iSmpNumInFile=0;         // opened file中共有多少smp
      m_iSmpNumReadOut=0;
      m_nFormatRead = NWF_Signed16; // opened file is which format

      m_fpRead=NULL;
      m_iOfsDataStart=0;
      m_szAttributeRead=NULL;

      m_fpWrite=NULL;
      m_iByteNumWrote=0;
      m_iSmpNumWrote=0;
      m_iRiffSizePos=0;
      m_iDataSizePos=0;
      m_nFormatWrite=NWF_Signed16;
   };
   ~CMonoWavFile(){
      if(m_fpRead!=NULL)vCloseRead();
      if(m_fpWrite!=NULL)bCloseWrite();
   };

   virtual bool	bIsReadFileOpened(){
      if(m_fpRead!=NULL)
         return true;
      else 
         return false;
   };
	virtual EX_ASR	eOpenToRead(const char *szFileName,int *piSr,NWaveFormat *pnFormat,int *piChannelNum=NULL);
   virtual int iReadSmp(void *pvBuf,int iSmpNum,NWaveFormat nFormat=NWF_Signed16);
   virtual const char *szGetAttribute(){ return m_szAttributeRead;};
   virtual bool bRewindRead();
   virtual void vCloseRead(){
      if(m_fpRead!=NULL){
         fclose(m_fpRead);
         m_fpRead=NULL;
      }
      delete m_szAttributeRead;
      m_szAttributeRead=NULL;
   };

   virtual bool	bIsWriteFileOpened(){
      if(m_fpWrite!=NULL)
         return true;
      else 
         return false;
   };
   virtual EX_ASR	eOpenToWrite(const char *szFileName,int iSr,NWaveFormat nFormat=NWF_Signed16,int iChannelNum=1);
   virtual int iPutSmp(const void *pvBuf,int iSmpNum,NWaveFormat nFormat=NWF_Signed16);
   virtual bool bCloseWrite(const char *szAttribute=NULL);

   //// for read
   int	m_iSampleRateRead;     // opened file sample rate
   int   m_iSmpNumInFile;       // opened file中共有多少smp
   int   m_iChannelNum;         // opened file 是mono(1) stereo(2)

   //// for write
   int   m_iSmpNumWrote;      // 紀錄目前寫入幾個smp

protected:
   //// for read
   FILE *m_fpRead;
   int   m_iOfsDataStart;    // ofs of data start
   int   m_iSmpNumReadOut;      // 已經讀走多少了
   char *m_szAttributeRead;  // the attribute got from file
   NWaveFormat m_nFormatRead; // opened file is which format

   //// for write
   FILE *m_fpWrite;
   int   m_iByteNumWrote;      // close 時要寫入 nbytes的 file ofs
   int   m_iRiffSizePos;   // 紀錄要寫入riff size的file pos
   int   m_iDataSizePos;   // 紀錄要寫入riff size的file pos
   NWaveFormat m_nFormatWrite; // opened file is which format
};

#endif
