/***********************************************************************
*  File - NmsNfcUtil.h
*
*  Description - NMSTRACE, NMSASSERT, NMSVERIFY, NMSASSERT_VALID
*
*  Author - 
*
*  Copyright (c) 199?-2004  NMS Communications.  All rights reserved.
***********************************************************************/

#ifndef NMSNFCUTIL_H
#define NMSNFCUTIL_H

#include "tsidef.h" //added by chuck

#ifndef   NMSNFCDEFS_H
#include "NmsNfcDefs.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void NMSAPI __NmsAssert(const char *info, const char *file, unsigned line);
void NMSAPI __NmsTrace( LPCSTR lpszFormat, ... );

#if defined (DEBUG) || defined (_DEBUG)
    #define NMSTRACE        ::__NmsTrace
    #define NMSASSERT(A) \
	    A ? void(0) : ::__NmsAssert(#A,__FILE__, __LINE__ )
    #define NMSVERIFY(f)    NMSASSERT(f)
#else
    #define NMSTRACE        1 ? (void)0 : ::__NmsTrace
    #define NMSASSERT(A)    (A)
    #define NMSVERIFY(f)    (f)
#endif

#define NMSASSERT_VALID(A) A ? void(0) : ::__NmsAssert(#A,__FILE__, __LINE__ )

#ifdef __cplusplus
}
#endif

#endif // ifndef NMSNFCUTIL_H //





