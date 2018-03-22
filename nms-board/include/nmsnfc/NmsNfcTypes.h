#ifndef NMSNFCTYPES_H
#define NMSNFCTYPES_H

#ifndef NMSTYPES_INCLUDED
#include <nmstypes.h>
#endif

#ifndef TSIDEF_INCLUDED
#include <tsidef.h>
#endif
/*
#ifndef CTADISP_INCLUDED
#include <ctadisp.h>
#endif
*/
#ifdef __cplusplus
extern "C" {
#endif

#ifndef UINT
typedef unsigned int    UINT;
#endif

#ifndef UNIX
  #ifndef BOOL_DECLARED
    #define  BOOL_DEF
    typedef int BOOL;
  #endif  
#endif

#ifndef LONG
typedef long            LONG;
#endif

#ifndef LPCSTR
typedef const char *    LPCSTR;
#endif

#ifndef LPSTR
typedef char *          LPSTR;
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    1
#endif

#ifndef NULL
#define NULL    0
#endif

#ifndef INFINITE
#define INFINITE    0xFFFFFFFF  // Infinite timeout
#endif

//#ifndef min
#define nms_max(a,b)    (((a) > (b)) ? (a) : (b))
#define nms_min(a,b)    (((a) < (b)) ? (a) : (b))
//#endif

#ifdef __cplusplus
}
#endif
#endif // ifndef NMSNFCTYPES_H //










