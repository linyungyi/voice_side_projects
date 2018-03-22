#ifndef SNMPCALLBACK_H
#define SNMPCALLBACK_H

#ifndef _PDU_CLS
#include <NmsPdu.h>
#endif

#ifndef _TARGET
#include <NmsTarget.h>
#endif

typedef void (*snmp_callback)( const CNmsSnmpPdu&       pdu,    // pdu passsed in
                               const Target&   target, // target passsed in
                               void * );                // optional callback data


class _SNMPCLASS SnmpCallback
{
public:
    SnmpCallback();
    SnmpCallback( const SnmpCallback& );
    SnmpCallback( const Target* pTarget, // Target to notify (NULL - all targets )
                  const CNmsSnmpOid* oid,         // CNmsSnmpOid to notify (NULL - all oids )
                  snmp_callback, 
                  void * notifycallback_data = 0 );
    ~SnmpCallback();

  SnmpCallback& operator = ( const SnmpCallback& );

public:

    Target*         m_pTarget;
    CNmsSnmpOid*             m_pOidToListen;
    snmp_callback    m_pFunk;
    void *           m_pNotifycallback_data;
};

/*
int operator == ( const Target& target1, const Target& target2 );
*/
#endif // ifndef SNMPCALLBACK //

