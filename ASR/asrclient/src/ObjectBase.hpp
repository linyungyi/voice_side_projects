#ifndef _OBJECT_BASE_H
#define _OBJECT_BASE_H

#include <string.h>
#include "TypeDef.h"


void gvShowErrorOld(int iErrCode, char *pcModule, char *pcErr);

class	CObjectBase
{
public:
	CObjectBase() {m_iErrCode = 0, m_pcErr[0] = 0, m_pcModule[0] = 0;};
	virtual void vShowError();
	void vSetError(int iErr,  const char *pcM,  const char *pcE=NULL);
	void vCopyError(CObjectBase *);

	int		m_iErrCode;
	char	m_pcErr[80];
	char	m_pcModule[40];
};

#endif
