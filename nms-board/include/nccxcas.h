/*******************************************************************************
*                                                                              *
*                           >>>>>>>>> NOTICE <<<<<<<<                          *
*                            This header must NOT be                           *
*                            used in the creation of                           *
*                            production software.                              *
*                           >>>>>>>>>>>>><<<<<<<<<<<<                          *
*                                                                              *
* This version of the header file is provided to allow the compilation of demo *
* programs which include both ISDN and CAS header files. Since typically one,  *
* but not both, protocol sets are installed, this results in missing headers   *
* required for compilation of some demos programs.                             *
*                                                                              *
* Note that if the same-named file has been installed in the standard          *
* NMS include directory, it will take precedence over this header when         *
* compiling the demo programs.                                                 *
*                                                                              *
*******************************************************************************/

/*****************************************************************************
  NAME:  nccxcas.h
  
  PURPOSE:

      This is a common include file for all CAS protocols for the NCC service.
****************************************************************************/

#ifndef NCCXCAS_INCLUDED
#define NCCXCAS_INCLUDED 

/* Prevent C++ name mangling. */
#ifdef __cplusplus
extern "C"
{
#endif

#include "nccadi.h"
#define    NCC_SVCID    0x1C         
                                     
/* definition of the extended Place Call Argument to be used, if necessary, */
/* with "void * protcallparms" argument to nccPlaceCall()                   */

typedef struct
{
    DWORD           size;
    char            ANIpresentation;
    BYTE            redirectingreason;
    WORD            satellitecircuit;
    char            redirectingaddr[33];
    char            carrierid[33];
    char            pad2[2];
    INT16           usercategory;
    INT16           tollcategory;
    char            datetime[8];
    char            callingname[32];
}   CAS_PLACECALL_EXT;

/* definition of the extended Disconnect Call Argument to be used, if       */
/* necessary, with "void * protcallparms" argument to nccDisconnectCall()   */

typedef struct
{
    DWORD           size;                   /* size of this structure       */
    WORD            cause;                  /* disconnect cause (NCC value) */
    WORD            pad;
}   CAS_DISCONNECT_EXT;

/* definition of the extended Reject Call Argument to be used, if           */
/* necessary, with "void * protcallparms" argument to nccRejectCall()       */

typedef struct
{
    DWORD           size;                   /* size of this structure       */
    WORD            cause;                  /* reject cause (NCC value)     */
    WORD            pad;
}   CAS_REJECT_EXT;


#define NCC_ADI_CAS_PARMID  0x1C011E

typedef struct
{
    DWORD               size;
    CAS_PLACECALL_EXT   placecall_ext;
    CAS_DISCONNECT_EXT  disconnect_ext;
    CAS_REJECT_EXT      reject_ext;
}   NCC_ADI_CAS_PARMS;

extern const CTAPARM_DESC * const _nccParmDescTable[];

/* definition of the CAS Extended Call Status Structure                     */
typedef struct
{
    DWORD size;
    WORD billingrate;           /* information passed with BILLING_SET event    */
    WORD releasecause;          /* additional information about disconnect reason   */
    char datetime [8];          /* date/time from network CID  MMDDhhmm         */
    char carrierid [NCC_MAX_DIGITS+1];      /* contains the carrier ID information  */        
    char redirectingaddr [NCC_MAX_DIGITS+1];/* contains redirecting address information */
    char redirectingreason;     /* contains the reason for redirection          */
    char ANIpresentation;       /* set if the ANI presentation is restricted    */
    char usercategory;          /* contains the user category                   */
    char tollcategory;          /* contains the toll category                   */
    char NAMEpresentation;      /* set if the NAME presentation is restricted   */
    BYTE MWIndicator;           /* 0 = MWI off, FF = MWI on, 1 = no MWI info    */
} NCC_CAS_EXT_CALL_STATUS;

#define MF_SIGNALING            1
#define DECADIC_SIGNALING       2

#define TWO_WAY_TRUNK           0
#define TRUNK_INBOUND           1
#define TRUNK_OUTBOUND          2

#define NO_DIALTONE_ABANDON     0
#define NO_DIALTONE_PROCEED     1

#define DONT_PLAY_CLEARDOWN     0
#define PLAY_CLEARDOWN_DIALTONE 1
#define PLAY_CLEARDOWN_BUSY     2
#define PLAY_CLEARDOWN_FASBUSY  3


#define CAS_REJECT_PLAY_RINGTONE    1
#define CAS_REJECT_PLAY_BUSY        2
#define CAS_REJECT_PLAY_REORDER     3

#define R15_ANI_DIGIT 0x1

#define CALL_REANSWERED 0x400   /* Reanswer (used in MFC-R2) */

#ifdef __cplusplus
}
#endif


#endif /* NCCXCAS_INCLUDED */

