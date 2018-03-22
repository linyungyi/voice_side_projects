#ifndef _JTSRING_HPP_
#define _JTSRING_HPP_

#include "DynBuffer.hpp"
#include <string.h>

class CJstring : public CDynBuffer
{
public:
	CJstring() {m_pcBuffer[0] = 0; 	m_iPnt=0;};
	CJstring(const char* pc);

	CJstring& operator=(const char* pc);		// add jk 200/5/1
	CJstring& operator=(const CJstring& src);
	char*	pcStrCpy(const char *pc);
	char*	pcStrCat(const char *pc);
	char*	pcStrCat(const CJstring& Str);
	void	vReset() {m_pcBuffer[0]=0;};
	int		iLen() {return strlen(m_pcBuffer);};
	bool	bIsEmpty() { return (m_pcBuffer[0] == 0) ? true : false;};

	//	retrieve string
	void	vResetPnt() {m_iPnt = 0;};
	char*	pcGetString(char* pcOut, char cDelimiter=' ');
protected:
	int		m_iPnt;
};

#endif
