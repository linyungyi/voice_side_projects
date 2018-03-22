/******************************************************************************\
* File:    CNmsException.h
* Autor:   Dmitry Krasnonosenkikh
* Project: CTA
*
* Desc:    Base exception class
*               CNmsException
*
* Revision History:
*          11/30/98     - DK first implementation.
*
*
*     (c)  1998-2002 NMS Communications.  All rights reserved.
\******************************************************************************/
#ifndef _CNMSEXCEPTION_H_
#define _CNMSEXCEPTION_H_

#ifndef   NMSNFCDEFS_H
#include <NmsNfcDefs.h>
#endif

#ifndef   CNMSSTRING_H
#include <CNmsString.h>
#endif

class _NMSNFCCLASS CNmsException 
{
 // constructors and destructors 
 public: 
    CNmsException( DWORD dwErrCode, LPCSTR pszErrDesc = 0 );
    virtual ~CNmsException();

 // public methods
 public: 
    DWORD       ErrCode()   { return m_dwErrCode;  };
    LPCSTR      ErrText()   { return (LPCSTR)m_sErrDesc; };

    static DWORD  RiseNmsNfcException( DWORD dwRes,  LPSTR szFile, UINT nLine );
 // protected members
 protected:
    CNmsString          m_sErrDesc;
    DWORD               m_dwErrCode;
};

#define RISE_NFC_EXCEPTION( dwRes ) \
    CNmsException::RiseNmsNfcException( dwRes, __FILE__, __LINE__ )

#endif //_CNMSEXCEPTION_H_

