#ifndef _SPEECH_HPP_
#define _SPEECH_HPP_

#include <stdio.h>
#include "PcmBuffer.hpp"
#include "MonoWavFile.hpp"

/////////////////////////
// �w�q save�ɡA����DATA���覡
enum NDataExtractMode{
   NSM_All,    // ���׭YDATA�O mono or stereo �����s
   NSM_Left,   // mono�����s, �YDATA�Ostereo�u�s���n�D
   NSM_Right,  // mono�����s, �YDATA�Ostereo�u�s�S�n�D
   NSM_Mono2Stereo,  // �N����mono�s��stereo �����k�n�D�A�YDATA�O stere �h�Ӧs
   NSM_TotalNum
};
//------------------------------
//	Speech file type, remove by jk 02/10/15 to CSeqSpchFile
//
//#define SPCH_FILE_FORMAT_WAVE	1		// .wav
//#define SPCH_FILE_FORMAT_RAW16	3		// .pcm
//#define SPCH_FILE_FORMAT_ULAW	2		// .vox

class CSpeech : public CPcmBuffer
{
public:

	CSpeech();
	CSpeech(int iSmp);
	CSpeech(int iSr, int iSec);
	EX_ASR eSetup(int iSr, int iSec);
	CSpeech(const CSpeech &pP,bool bOptimizeSize);

	~CSpeech();

	// smart file access, save/load to/from *.wav or *.pcm(all other)
	virtual bool	bSaveFile(const char *szFileMainName);
	virtual bool	bLoadFile(const char *szFileMainName,bool bExpandBuf=true);


	bool	bInit(int iSmp);
	bool	bLoadWavFile(const char *szFileMainName,int *piSampleRate=NULL,bool bExpandBuf=true,NWaveFormat *pnFormat=NULL,int *piChannelNum=NULL,bool bIsCombineToMono=false);
	bool	bLoadUnixPcmFile(const char *szFileMainName,bool bExpandBuf=true); // default file ext is *.bin
	bool	bLoadPcmFile(const char *szFileMainName,bool bExpandBuf=true){
            return CPcmBuffer::bLoadPcm16File(szFileMainName,bExpandBuf);
         };// default file ext is *.pcm
	bool	bSavePcmFile(const char *szFileMainName){
            return CPcmBuffer::bSavePcm16File(szFileMainName);
         }; // short bin file without header
	bool	bSaveUnixPcmFile(const char *szFileMainName); // short bin file with UNIX format without header
	bool	bSaveWavFile(const char *szFileMainName,int iSR=0,NWaveFormat nFormat=NWF_Signed16,int iChannelNum=0,NDataExtractMode nDataExtractMode=NSM_All);//save *.wav file: default:16 bits PCM with class's samplerate
	bool	bSaveWavFile(const char *szFileMainName,const char *szAttribute,int iSR=0,NWaveFormat nFormat=NWF_Signed16,int iChannelNum=0,NDataExtractMode nDataExtractMode=NSM_All);//save *.wav file: default:16 bits PCM with class's samplerate

	bool	bLoadOkiAdpcmFile(const char *szFileName,bool bExpandBuf=true);
	bool	bSaveOkiAdpcmFile(const char *szFileName);

	void	vSetSmpRate(int iSr){m_iSampleRate=iSr;};
	void	vSetMark(int iStart,int iEnd) {m_iMarkIni = iStart; m_iMarkEnd = iEnd;};
	void	vResetMark() {m_iMarkIni = 0; m_iMarkEnd = m_iSampleNum;};

   const char* cszGetAttribute();
   void  vSetAttribute(const char* szAttribute);

	int		m_iMarkIni;
	int		m_iMarkEnd;
	int		iGetSmpRate() {return m_iSampleRate;};
	unsigned long	dwGetSmpRate() {return (unsigned long)m_iSampleRate;};	// old version
	float	fGetMaxSpeechSec() {return (float)m_iMaxSmpNum/m_iSampleRate;};
   int   m_iChannelNum;         // opened file �Omono(1) stereo(2)

protected:
   char *m_szAttribute;  //����attribute���e
   int		m_iSampleRate;		/* sampling rate */

private:
	void	vNull();
};


//
bool	gbPlayFile();

#endif
