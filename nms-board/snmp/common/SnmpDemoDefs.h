#ifndef SNMPDEMODEFS_H
#define SNMPDEMODEFS_H

#ifndef   NMSNFCDEFS_H
#include <NmsNfcDefs.h>
#endif

// SNMP Space

// SNMP

#define SZ_OID_SYSDESC     "1.3.6.1.2.1.1.1.0"
#define SZ_OID_SYSUPTIME   "1.3.6.1.2.1.1.3.0"
#define SZ_OID_SYSCONTACT  "1.3.6.1.2.1.1.4.0"
#define SZ_OID_SYSNAME     "1.3.6.1.2.1.1.5.0" // Computer name
#define SZ_OID_SYSLOCATION "1.3.6.1.2.1.1.6.0"


#define SZ_OID_CHASSIS_BOARD_TRAP_ENABLE "1.3.6.1.4.1.2628.2.2.4.2.0"


// Tables entry
#define SZ_OID_BUS_SEGMENT_TABLE_ENTRY  "1.3.6.1.4.1.2628.2.2.2.4.1"
    #define  BUS_SEGMENT_INDEX           1
    #define  BUS_SEGMENT_TYPE            2
    #define  BUS_SEGMENT_SLOTS_OCCUPIED  4

#define SZ_OID_BOARD_ACCESS_TABLE_ENTRY "1.3.6.1.4.1.2628.2.2.3.1.1"
    #define BOARD_ACCESS_BOARD_INDEX     3

#define SZ_OID_BOARD_TABLE_ENTRY         "1.3.6.1.4.1.2628.2.2.4.3.1"
    #define BOARD_BUS_SEGMENT_TYPE       2
    #define BOARD_BUS_SEGMENT_NUMBER     3
    #define BOARD_SLOT_NUMBER            4
    #define BOARD_MODEL_TEXT             6
    #define BOARD_FAMILY_ID              7
    #define BOARD_FAMILY_NUMBER          8
    #define BOARD_STATUS                 10
    #define BOARD_COMMAND                11
    #define BOARD_TRAP_ENABLE            17


// Nms Chassis Trap entry
#define SZ_OID_NMSCHASSIS_TRAP_ENTRY    "1.3.6.1.4.1.2628.2.2.6"




#define SZ_OID_DSX1_CONFIG_ENTRY        "1.3.6.1.2.1.10.18.6.1"
    #define DSX1_LINE_INDEX                   1
    #define DSX1_CIRCUIT_IDENTIFIER           8
    #define DSX1_LINE_STATUS                  10
    #define DSX1_LINE_STATUSLASTCHANGE        16
    #define DSX1_LINE_STATUSCHANGETRAPENABLE  17

// Dsx1 Trap entry
#define SZ_OID_DS1_TRAPS_ENTRY          "1.3.6.1.2.1.10.18.15"


typedef enum _SnmpBusType
{
    ISA = 1,
    PCI
} SnmpBusType;

typedef enum _SnmpBoardStatus
{
    BoardStatus_OnLine = 1,
    BoardStatus_OnLinePending,
    BoardStatus_Failed,
    BoardStatus_OffLine,
    BoardStatus_OffLinePending,
    BoardStatus_Extracted
} SnmpBoardStatus;

typedef enum _SnmpBoardCommand
{
    BoardCommand_On = 1,
    BoardCommand_Off

} SnmpBoardCommand;

typedef enum _SnmpTrapEnable
{
    TrapOn = 1,
    TrapOff
} SnmpTrapEnable;

LPCSTR SnmpBusTypeToStr( SnmpBusType type );
LPCSTR SnmpBoardStatusToStr( SnmpBoardStatus status );


#endif // ifndef SNMPDEMODEFS_H //

