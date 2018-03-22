#ifndef _DYNBUFFER_HPP_
#define _DYNBUFFER_HPP_

#define MIN_DYN_SIZE 64

class CDynBuffer
{
public:
	CDynBuffer(int iInitSize=MIN_DYN_SIZE);
	~CDynBuffer();
	void* pvReSize(int iSize);
	void* pvReSizeCopy(int iSize);
	void* pvCopy(const void* pvSrc, int iSize);

	char*	m_pcBuffer;
	int		m_iByte;
};

#endif
