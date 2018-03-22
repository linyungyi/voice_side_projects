#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_
#pragma warning(disable:4786)

//////////////////////////////////////////////////////

#ifdef WIN32
	#include <windows.h>
//#ifndef BASETYPES			// to avoid wintype.h or windef.h
#else
	//typedef unsigned long DWORD;
	//typedef unsigned short WORD;
	//typedef unsigned int UINT;

	#define MAX_PATH	260			// max. file path length
	#define true 1
	#define false 0
//	#define NULL 0
	#define INFINITE	0xFFFFFFFF
#endif		// endif of BASETYPES

//////////////////////////////////////////////////////

#ifdef _WINDOWS
	#define	vMsg2(s1,s2) ::MessageBox(NULL,s2,s1,MB_OK)
	#define	vMsg(s) ::MessageBox(NULL,s,"TLASR",MB_OK)
#else
	#include <stdio.h>
	#define vMsg2(s1,s2) fprintf(stderr,"%s: %s\n",s1,s2)
	#define vMsg(s) fprintf(stderr,"%s\n",s)
#endif

//////////////////////////////////////////////////////
#ifdef _UNIX_
typedef int bool;
#endif

#define MAX_UINT 0x7FFFFFFF

typedef	void EX_ASR; // exception type for CAsrError

//	2-dimension
struct CTwoDimFloat
{
	float m_fX;
	float m_fY;
};

//-------------------------------------------------------
//	Device Status
enum NDeviceStatus
{
	DEV_NotReady,
	DEV_Ready,
	DEV_InUsed,
	DEV_Stopping
};



#endif
