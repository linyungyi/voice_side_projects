#ifndef   SNMPDEMODEFS_H
#include <SnmpDemoDefs.h>
#endif

#ifndef   NMSNFCUTIL_H
#include <NmsNfcUtil.h>
#endif


LPCSTR SnmpBusTypeToStr( SnmpBusType type )
{
LPSTR ptr = 0;
    switch( type )
    {
    case ISA:
        ptr = "ISA";
        break;
    case PCI:
        ptr = "PCI";
        break;
    }
    NMSASSERT( ptr );
return ptr;
};


LPCSTR SnmpBoardStatusToStr( SnmpBoardStatus status )
{
LPSTR ptr = 0;
    switch( status )
    {
    case BoardStatus_OnLine:
        ptr = "OnLine";
        break;
    case BoardStatus_OnLinePending:
        ptr = "OnLinePending";
        break;
    case BoardStatus_Failed:
        ptr = "Failed";
        break;
    case BoardStatus_OffLine:
        ptr = "OffLine";
        break;
    case BoardStatus_OffLinePending:
        ptr = "OffLinePending";
        break;
    case BoardStatus_Extracted:
        ptr = "Extracted";
        break;
    }
    NMSASSERT( ptr );
return ptr;

};
