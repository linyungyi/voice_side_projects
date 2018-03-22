#ifndef _VM_TRACE
#define _VM_TRACE

// Tracing types
#if !defined(T_H245)
#define T_H245      0x000001
#endif
#define T_H245_DET  0x000002
#define T_MSPP      0x000004
#define T_GWAPP     0x000008
#define T_SRVAPP    0x000010
#define T_CC        0x000020
#define T_CTA       0x000040
#define T_H323      0x000080
#define T_XC        0x000100
#define T_FLOW      0x000200
#define T_STAT      0x000400
#define T_LOAD      0x000800  // Very minimal printing for load tests
#define T_QRYRSP    0x001000  // Print the events corresponding to user queries

extern int VmTraceMask;

// Tracing
void mmPrint(int Port, int Type, char *Format, ... );
void mmPrintAll(char *Format, ... );
void mmPrintErr(char *Format, ... );
void mmPrintPortErr(int Port, char *Format, ... );
void mmPrintBinary(int Port, int Type, BYTE *buf, int size);


/////////////// System Flow Diagram /////////////////

// types
typedef struct {
    int     nObjID;
    char szObjName[6];
} t_FlowObj;

// Defines
#define F_H324      1
#define F_H323      2
#define F_APPL      3
#define F_TRC       4
#define F_H245      5
#define F_FOMA      6
#define F_NM        7
#define F_MSPP      8
#define F_NCC       9

#define FLOW_MAX_OBJ    10

// functions
bool FlowInit( int nScreenWidth, int nCount, t_FlowObj ObjArray[FLOW_MAX_OBJ], char* sgFileName);
void Flow( int nChan, int from, int to, char* szEvnName );
#endif // _VM_TRACE
