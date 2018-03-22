#ifndef NMSNFCDEFS_H
#define NMSNFCDEFS_H

#ifndef   NMSNFCTYPES_H
#include <NmsNfcTypes.h>
#endif

#if defined (WIN32)
    #ifdef NMSNFC_EXPORTS
        #define _NMSNFCCLASS    __declspec( dllexport )
    #else
        #define _NMSNFCCLASS    __declspec( dllimport )
    #endif
#else
    #define _NMSNFCCLASS
#endif

#define NFCAPI  _NMSNFCCLASS

#endif // ifndef NMSNFC //

