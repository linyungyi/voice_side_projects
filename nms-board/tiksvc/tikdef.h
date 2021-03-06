/*****************************************************************************
  NAME:  tikdef.h
  
  PURPOSE:

      This file contains macros and function prototypes that define the API
      for the TIK sample service.

      This API allows a Natural Access application to ACT as a "tick client".
      A tick client requests a specific number of "tick" strings to be
      delivered from the "Tick Server" at a specified frequency. 
      
      The TIK sample service, however, operates on behalf of the Natural Access
      application as the "real" tick client. Once "tick" strings are received
      by the TIK Service, a Natural Access event is generated and dispatched to
      the Natural Access client. 


  Copyright (c) 1997-2002 NMS Communications.  All rights reserved.
  ****************************************************************************/

#ifndef TIKDEF_INCLUDED
#define TIKDEF_INCLUDED 


#include "nmstypes.h"
#include "ctadef.h"
#include "tikparm.h"

/* Prevent C++ name mangling. */
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------------------------------------------
  TIK Service ID.  
    - These id's are allocated by NMS Developer Support. 
    - Do not reuse an existing Service ID as processing conflicts will occur!
    - Shown here for clarity, really defined in the tikparm.h file that is 
      generated from the tikparm.pf file via pf2src.
  ---------------------------------------------------------------------------*/
/*#define TIK_SVCID                         0x40*/



/*-----------------------------------------------------------------------------
  TIK Version Ids  
    - These id's are used to identify the revision level of this service. 
  ---------------------------------------------------------------------------*/
#define TIK_MAJORREV                        1
#define TIK_MINORREV                        0



/*-----------------------------------------------------------------------------
  TIK API Compatibility Level Id
    - This id is used to determine runtime compatibility between the
      installed TIK service and clients of the TIK service. Clients can
      be a Natural Access application which accesses TIK functionality
      via the TIK API.
    - Clients(Natural Access application) #include this file into their code and, 
      therefore, insert "hard coded" values for each TIK compat level
      into their client code.
    - The API_COMPATLEVEL is used by Natural Access application developers who
      want to check the "hard coded" value in their application against the
      "hard coded" value in the installed TIK service (as determined
      via ctaGetServiceVersion() will be available in CT Access Version 2.1).
    - API compatibility level value should be incremented when the associated
      source module changes in a non-backwards compatible way. Refer to the 
      Natural Access Service Writer's Manual for more info.
  ---------------------------------------------------------------------------*/
#define TIKAPI_COMPATLEVEL                  1

  
/*-----------------------------------------------------------------------------
  TIK Event Codes.
    - These codes are constructed using the TIK_SVCID, the Natural Access
      CTA_CODE_EVENT id, and a 1-based sequence number. The formula is
        EVENTCODE = ((TIK_SVCID<<16) | (CTA_CODE_EVENT | SEQUENCE))
    - If an event declaration is a done event 0x100 must also be ORed into
      event code.
        EVENTCODE = ((TIK_SVCID<<16) | (CTA_CODE_EVENT | 0x100 | SEQUENCE)
    - The actual event codes are "hardcoded" for source browsing purposes.
    - Note that the TIK Service also generates Natural Access events as well; 
      therefore this is not an exhaustive list of all, TIK generated, 
      event codes.
  ---------------------------------------------------------------------------*/

/* Event generated by TIK Service and sent back to client for all
 * tick strings
 */
#define TIKEVN_TIMER_TICK                 0x402001

/* Event generated by TIK Service and sent back to client to indicate
 * completion of all tick strings having been sent to the client. 
 * NOTE : EventCode = ((TIK_SVCID<<16) | (CTA_CODE_EVENT | 0X100 | SEQUENCE)
 */
#define TIKEVN_TIMER_DONE                 0x402101



/*-----------------------------------------------------------------------------
  TIK Reason Codes. 
    - These codes are constructed using the TIK_SVCID, the Natural Access
      CTA_CODE_REASON id, and a 1-based sequence number. The formula is
        REASONCODE = ((TIK_SVCID<<16) | (CTA_CODE_REASON | SEQUENCE))
    - The actual reason codes are "hardcoded" for source browsing purposes.
    - Note that for the TIKEVN_TIMER_DONE event, generic Natural Access codes
      will be reused; they are:
        - CTAEVN_REASON_STOPPED: indicates that tikStopTimer() (as requested
          by client) was successful.
        - CTAEVN_REASON_DONE:    indicates that the Tick Server has 
          successfully generated all requested tick strings at the requested
          frequency.
  ---------------------------------------------------------------------------*/

/* Reason Code for TIKEVN_TIMER_DONE - 
 * indicates that an unexpected error has occurred at the TICK Server. 
 */
#define TIK_REASON_UNKNOWN_SERVER_REASON  0x401001

/* Reason Code for CTAEVN_DISP_OPEN_SERVICE_DONE -
 * indicates a failure in initializing communication with the TIK Server
 */
#define TIK_REASON_INVALID_CHANNEL        0x401002

/* Reason Code for CTAEVN_DISP_OPEN_SERVICE_DONE -
 * indicates that the requested channel is occupied. Try another channel. 
 */
#define TIK_REASON_CHANNEL_ACTIVE         0x401003



/*-----------------------------------------------------------------------------
  TIK Error codes. 
    - These codes are constructed using the TIK_SVCID, the Natural Access
      CTA_CODE_ERROR id, and a 1-based sequence number. The formula is
         ERRORCODE = ((TIK_SVCID<<16) | (CTA_CODE_ERROR | SEQUENCE))  
    - The actual error codes are "hardcoded" for source browsing purposes.
  ---------------------------------------------------------------------------*/

/* Error communicating with server. */
#define TIKERR_COMM_FAILURE               0x400001

/* Server connection is not been estiblished. */
#define TIKERR_CHANNEL_NOT_OPENED         0x400002

/* Owner conflict for one owner per channel for the life of Natural Access 
 * context */
#define TIKERR_OWNER_CONFLICT             0x400003

/* Unknown server response. */
#define TIKERR_UNKNOWN_SERVER_RESPONSE    0x400004

/* Can not establish connection to server. */
#define TIKERR_CAN_NOT_CONNECT            0x400005


/**/
/*-----------------------------------------------------------------------------
  Exported API Data Structures
    - Note that the TIK "parameter" structure (TIK_START_PARMS) is not defined
      here. Rather, this structure appears in tikparm.h which is
      automatically generated by pf2src.
  ---------------------------------------------------------------------------*/
#define TIKSVR_MSG_SIZE                   (64)

/* Status of open service. */
typedef struct
{
    DWORD size;                         /* For compatibility.          */
    DWORD calls;                        /* Successful API calls made.  */
    DWORD ticked;                       /* Total number of Tick strings
                                           received since the TIK
                                           service was opened          */
    DWORD requested;                    /* Total number of Tick strings
                                           requested since the TIK 
                                           service was opened          */
    DWORD remaining;                    /* Ticks remaining to be 
                                           received for this instance 
                                           of tikStartTimer().         */
    DWORD ms;                           /* Frequency of ticks.         */
    char  tick_string[TIKSVR_MSG_SIZE]; /* String sent by tick server. */
} TIK_CHANNEL_INFO;


/**/
/*-----------------------------------------------------------------------------
  TIK Service API function prototypes.
  ---------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
  Request Tick strings to be generated (i.e., start the ticker).
  Operation of this API call is asynchronous.

    ctahd:    Handle for Natural Access context to start.
    start:    Timer start parameter structure (see tikparm.h) includes:
              * a count field specifying the number of tick strings
              * a frequency field specifying how often to send tick strings.
  ---------------------------------------------------------------------------*/
DWORD NMSAPI
  tikStartTimer( CTAHD            ctahd,
                 TIK_START_PARMS *start );


/*-----------------------------------------------------------------------------
  Request termination of Tick strings (i.e., stops a running ticker).
  Operation of this API call is asynchronous.

    ctahd:    Handle for context with running ticker.
  ---------------------------------------------------------------------------*/
DWORD NMSAPI
  tikStopTimer( CTAHD ctahd );


/*-----------------------------------------------------------------------------
  Gets the status of a ticking service.
  Operation of this API call is synchronous.
  
    ctahd:    Handle for context with running ticker.
    info:     Defined TIK_CHANNEL_INFO structure(above).
    size:     Size (TIK_CHANNEL_INFO) structure.
  ---------------------------------------------------------------------------*/
DWORD NMSAPI
  tikGetChannelInfo( CTAHD             ctahd, 
                     TIK_CHANNEL_INFO *info,
                     unsigned          insize,
                     unsigned         *outsize );

#ifdef __cplusplus
}
#endif


#endif /* TIKDEF_INCLUDED */

