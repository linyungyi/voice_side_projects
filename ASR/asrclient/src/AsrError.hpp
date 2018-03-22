#ifndef _ASRERROR_H_
#define _ASRERROR_H_

#include <string.h>
#include "TypeDef.h"
#include "jstring.hpp"

typedef	void EX_ASR;		// exception type for CAsrError

void vgShowError(int iErrCode, char *pcModule, char *pcErr, char *pcOut=NULL);

#define MX_MODULE_CHAR_LEN	40

typedef void (vErrMsgCallBackFunc)(int iErrCode,const char *szErrMsg,int iPara);


//--------------------------------------------------------
class CAsrError
{
public:
   static vErrMsgCallBackFunc *s_pCallBackFunc;
   static int s_iCallBackPara;
   static void vSetErrorMsgCallBack(vErrMsgCallBackFunc *pCallBackFunc,int iPara){
      s_pCallBackFunc=pCallBackFunc;
      s_iCallBackPara=iPara;
   };

	CAsrError() {m_iErrCode=0;};
	CAsrError(int iErrCode, const char *pcM="", const char *pcE="",const char *pcO="");
	CAsrError(const CAsrError& src);
	void vCopyError(CAsrError& Err);
	void vCopyError(CAsrError* pErr);
	void vShowError();
	void vShowErrorCallBack();
	virtual const char *pcShowError();
	virtual void vSetError(int iErrCode, const char *pcM="", const char *pcE="",const char *pcO="");
	virtual int  iGetErrCode() {return m_iErrCode;};

	int			m_iErrCode;
	CJstring	m_Module;
	CJstring	m_Msg;
	CJstring	m_Err;
	CJstring	m_Obj;
};


#endif
