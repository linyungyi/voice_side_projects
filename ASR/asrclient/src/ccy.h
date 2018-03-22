#ifndef __CCY_H__
#define __CCY_H__


#define DWORD unsigned long
#define UINT  unsigned int
#define INFINITE 0xFFFFFFFF

#define BYTE    unsigned char
#define USHORT  unsigned short
#define ULONG   unsigned long
#define UCHAR   unsigned char



void Sleep(DWORD dwMillseconds);

//void gvSwap2(void *pV);
//void gvSwap4(void *pV);
bool bNeedByteSwap();

#endif //__CCY_H__
