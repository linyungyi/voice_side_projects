#ifndef NMSSNMPEXCEPTION_H
#define NMSSNMPEXCEPTION_H

#ifndef   NMSSNMPDEFS_H
#include <NmsSnmpDefs.h>
#endif

#ifndef   CNMSEXCEPTION_H
#include <CNmsException.h>
#endif

class _SNMPCLASS  CNmsSnmpException : public CNmsException
{
public:
    CNmsSnmpException( int nStatus, LPCSTR szErrorLine = 0 );
   ~CNmsSnmpException();

  void ReportError( LPCSTR szErrorText = 0 );
  static int  Throw( int nStatus,  LPSTR szFile, UINT nLine );

public:
    int         m_nStatus;
    CNmsString  m_szErrorLine;
};

#define THROW_SNMP_EXCEPTION( nStatus ) CNmsSnmpException::Throw( nStatus, __FILE__, __LINE__ )


#endif // ifndef NMSSNMPEXCEPTION_H //

