#ifndef NMSSNMPTRAP_H
#define NMSSNMPTRAP_H

#ifndef  _SNMP_CLS
#include <NmsSnmp_pp.h>
#endif

class CSnmpTrapThread;

class _SNMPCLASS CNmsSnmpTrap : public SnmpTrap
{
public:
    CNmsSnmpTrap( int& nStatus );
  virtual ~CNmsSnmpTrap();

  virtual int EnableTraps( BOOL bEnable );


protected:
    CSnmpTrapThread* pTrapThread;
};

#endif // ifndef NMSSNMPTRAP_H //

