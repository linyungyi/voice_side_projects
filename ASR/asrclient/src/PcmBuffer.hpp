#ifndef _PCM_BUFFER_H_
#define _PCM_BUFFER_H_

#include <assert.h>
#include "TypeDef.h"
#include "ObjectBase.hpp"

class CPcmBuffer: public CObjectBase
{
public:
enum NPcmBufErr
{
	PCMBUFERR_FileOpenError,
	PCMBUFERR_FileReadError,
	PCMBUFERR_FileWriteError,
	PCMBUFERR_InputFileTooBig,
	PCMBUFERR_InputBufTooLong,
	PCMBUFERR_BufFull,
	PCMBUFERR_MemNotEnough
};

	CPcmBuffer();  // must use bInit() to alloc buf
	CPcmBuffer(int iSmp);
	CPcmBuffer(const CPcmBuffer &pPcm,bool bOptimizeSize=true);
	CPcmBuffer& operator=(const CPcmBuffer& src);
	~CPcmBuffer();

	EX_ASR	eSetup(int iSmp);

	virtual bool bSaveFile(const char *szFile); // save simple pcm16 file
	virtual bool bLoadFile(const char *szFile,bool bExpandBuf=true); // load simple pcm16 file
	virtual void vReset() {m_iSampleNum = 0; m_iGetPos=0;};

	void	vFillZeroToWholeBuf();
	bool	bAppendData(short *psData, int iSmp);
	bool	bInit(int iSamples);
	bool	bLoadPcm16File(const char *szFile,bool bExpandBuf=true);
	bool	bLoadUpcmFile(const char *szFile,bool bExpandBuf=true);
	bool	bLoadAlawFile(const char *szFile,bool bExpandBuf=true);
	bool	bLoadPcmBuf(const short *psPcmIn,int iLen,bool bExpandBuf=true);
	bool	bLoadUpcmBuf(const unsigned char *upcUpcmIn,int iLen,bool bExpandBuf=true);
	bool	bLoadAlawBuf(const unsigned char *upcAlaw,int iLen,bool bExpandBuf=true);
	bool	bSavePcm16File(const char *szFile);
	bool	bSaveUpcmFile(const char *szFile);
	bool	bSaveAlawFile(const char *szFile);

	int		iGetData(short *psData, int iSmp);	// return Num of sample got
	int		iGetSmpNum() {return m_iSampleNum;};
	int		iGetMaxSmpNum() {return m_iMaxSmpNum;};

	unsigned long	dwGetSmpNum() {return (unsigned long)m_iSampleNum;};	// old version
	unsigned long	dwGetMaxSmpNum() {return (unsigned long)m_iMaxSmpNum;};	// old version

	void	vSetSmpNum(int iSmp) {assert(iSmp>=0); m_iSampleNum = iSmp;}; // for skip tail
	void	vCalcAveAmp();
	void	vCalcMax();
	int		iGetMaxAmp() {vCalcMax(); return m_iMaxAmp;};		// add 2002/10/31
	void	vRewind() {m_iGetPos = 0;};

	float	fGetAmp() {return m_fAmp;};
	float	fGetAve() {return m_fAve;};

	short*	m_psPcmBuffer;			/* PCM buffer */
	int		m_iSampleNum;			/* number of sample */
	int		m_iMax;	// max.
	int		m_iMin;	// min.
	int		m_iMaxAmp;
	float	m_fAve;	// average
	float	m_fAmp;   // amplitude

protected:
    int     m_iMaxSmpNum;
    int     m_iGetPos;			   /* the position will be taken */

private:
	void	vNull();
	void	vClearBuffer();

};

#endif
