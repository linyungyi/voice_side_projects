#ifndef NMSSNMP_H
#define NMSSNMP_H

#ifndef  _SNMP_CLS
#include <NmsSnmp_pp.h>
#endif

#include <CSnmp.h>

#ifndef   CNMSSTRING_H
#include <CNmsString.h>
#endif

class _SNMPCLASS CNmsSnmp : public Snmp
{
public:
    CNmsSnmp( int& nStatus );
    ~CNmsSnmp();

  void  SetParams( GenAddress ipAddress, 
                   snmp_version version,
                   OctetStr readCommunity, 
                   OctetStr writeCommunity, 
                   int nRetries,
                   int nTimeoutMs,
				   unsigned int port);

  int   SnmpGet( int& n_rValue, const CNmsSnmpOid& oid );
  int   SnmpGet( CNmsString& sz_rValue, const CNmsSnmpOid& oid );
  int   SnmpGet( Vb& vb );
  int   SnmpGet( Vb* pVbList, const int nVbCount );

  int   SnmpSet( int nValue, const CNmsSnmpOid& oid );



  int   SnmpGetNext( Vb& vb );


  int   SnmpGetFirst( Vb& vb );
  int   SnmpGetNextInColumn( Vb& vb, const CNmsSnmpOid TableEntry, const int nColumn );

  virtual CNmsString GetPrintableError( int nStatus );
  LPCSTR  GetPrintableAddress();

protected:
  BOOL  IsOidInATable( CNmsSnmpOid tableOid, CNmsSnmpOid toCheck );



protected:
  struct _SNMPCLASS CNmsSnmpParams
  {
      GenAddress    m_ipAddress;
      snmp_version  m_nVersion;
      OctetStr      m_readCommunity;
      OctetStr      m_writeCommunity;
      int           m_nRetries;
      int           m_nTimeout;
	  unsigned int   m_port;

  } m_SnmpParams;
};


CNmsSnmpOid _SNMPCLASS SnmpMakeFieldOid( const CNmsSnmpOid TableEntry, const int nColumn, const int nIndex );


#endif // ifndef NMSSNMP_PP_H //

