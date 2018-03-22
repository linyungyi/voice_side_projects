/*****************************************************************************
  NAME:  evtdef.h
  
  PURPOSE:

      This file contains macros and function prototypes that define the API
      for the Event Tracking (EVT) service.

      The prefix 'evt' will be used throughout all related source files.

  Copyright 2001 NMS Communications.  All rights reserved.
  ****************************************************************************/

#ifndef EVTDEF_INCLUDED
#define EVTDEF_INCLUDED 


/* Standard Natural Access include files. */
#include "nmstypes.h"
#include "ctadef.h"
/* #include "evtparm.h" no need for service parameters in EVT service */


/* Prevent C++ name mangling. */
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------------------------------------------
  EVT Service ID.  
    - These id's are allocated by NMS Developer Support. 
    - Do not reuse an existing Service ID as processing conflicts will occur!
    - It is defined here in the absence of evtparm.h file.
  ---------------------------------------------------------------------------*/
#define EVT_SVCID               0x004E


/*-----------------------------------------------------------------------------
  EVT Version Ids  
    - These id's are used to identify the revision level of this service. 
  ---------------------------------------------------------------------------*/
#define EVT_MAJORREV            1
#define EVT_MINORREV            0



/*-----------------------------------------------------------------------------
  EVT API Compatibility Level Id
    - This id is used to determine runtime compatibility between the
      installed EVT service and clients of the EVT service. Clients can
      be a Natural Access application which accesses EVT functionality
      via the EVT API.
    - Clients(Natural Access application) #include this file into their code and, 


      therefore, insert "hard coded" values for each EVT compat level
      into their client code.
    - The API_COMPATLEVEL is used by Natural Access application developers who
      want to check the "hard coded" value in their application against the
      "hard coded" value in the installed EVT service (as determined
      via ctaGetServiceVersion() will be available in CT Access Version 2.1).
    - API compatibility level value should be incremented when the associated
      source module changes in a non-backwards compatible way. Refer to the 
      Natural Access Service Writer's Manual for more info.
  ---------------------------------------------------------------------------*/
#define EVTAPI_COMPATLEVEL      1



/*-----------------------------------------------------------------------------
  EVT Event Codes.
    - These codes are constructed using the EVT_SVCID, the Natural Access
      CTA_CODE_EVENT typecode, and a sequence number. The formula is
        EVENTCODE = ((EVT_SVCID<<16) | (CTA_CODE_EVENT | SEQUENCE))
    - If an event declaration is a done event 0x100 must also be ORed into
      event code.
        EVENTCODE = ((TIK_SVCID<<16) | (CTA_CODE_EVENT | 0x100 | SEQUENCE)
    - Note that the EVT Service also generates Natural Access events as well; 
      therefore this is not an exhaustive list of all, EVT generated, 
      event codes.
    - EVT Event Codes here are not used and these are place holders for now.  
  ---------------------------------------------------------------------------*/
/* EVT event one. */
#define EVTEVN_ONE              0x004E2001
                                /* ((EVT_SVCID<<16)|(CTA_CODE_EVENT|0x01)) */

/* EVT event two. */
#define EVTEVN_TWO              0x004E2002
                                /* ((EVT_SVCID<<16)|(CTA_CODE_EVENT|0X02)) */

/* EVT done event one. */
#define EVTEVN_ONE_DONE         0x004E2101
                        /* ((EVT_SVCID<<16)|(0x100)|(CTA_CODE_EVENT|0X01)) */

/* EVT done event two. */
#define EVTEVN_TWO_DONE         0x004E2102
                        /* ((EVT_SVCID<<16)|(0x100)|(CTA_CODE_EVENT|0X02)) */

/*-----------------------------------------------------------------------------
  EVT Reason Codes. 
    - EVT reason codes for events defined above.  These values are to be 
      utilized in the "value" member of the event structure, so that an 
      application can determine why an event occurred.
    - These codes are constructed using the EVT_SVCID, the Natural Access
      CTA_CODE_REASON typecode, and a sequence number. The formula is
        REASONCODE = ((EVT_SVCID<<16) | (CTA_CODE_REASON | SEQUENCE))
    - EVT Reason Codes here are not used and these are place holders for now.  
  ---------------------------------------------------------------------------*/
/* Event occurred because of reason one. */
#define EVTREASON_ONE             0x004E1001
                                  /* ((EVT_SVCID<<16)|(CTA_CODE_REASON|0x1)) */

/* Event occurred because of reason two. */
#define EVTREASON_TWO             0x004E1002
                                  /* ((EVT_SVCID<<16)|(CTA_CODE_REASON|0x2)) */


/*-----------------------------------------------------------------------------
  EVT Error codes. 
    -   Error codes are the return values for API calls for error events
        peculiar to this service.  You may wish to check ctaerr.h to see if a 
        generic Natural Access error code is good enough to suite your debugging
        needs.
    - These codes are constructed using the EVT_SVCID, the Natural Access
      CTA_CODE_ERROR typecode, and a sequence number. The formula is
         ERRORCODE = ((EVT_SVCID<<16) | (CTA_CODE_ERROR | SEQUENCE))  
  ---------------------------------------------------------------------------*/
/* Owner Conflict Error */
#define EVTERR_OWNER_CONFLICT     0x004E0001
                                  /* ((EVT_SVCID<<16)|(CTA_CODE_ERROR|0X3)) */
/**/
/*-----------------------------------------------------------------------------
  EVT Service API function prototypes.
  ---------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Notes on EVT Service API Functions:

1. ctahd is the first argument of all EVT Service API functions.
   It designates an NA Access Core Context under which both the event source
   service (an RTC provider) and the event tracking service (an RTC consumer)
   are opened.

2. All EVT Service API Functions return SUCCESS on successful execution,
   otherwise return a Natural Access or Service specific error code on failure.

3. Each evtAddEvent????() call may or may not have a subsequent
   evtRemoveEvent????() call.

   Each evtRemoveEvent????() call must have a prior evtAddEvent????() call
   and the matching pair must have identical arguments.
------------------------------------------------------------------------------*/

/* The following are exported API functions: */

/* Designate the log file name */
DWORD NMSAPI evtSetLogFile(CTAHD ctahd, char *logFileName);


/* Add all event codes of a service to the tracking list. */
DWORD NMSAPI evtAddEventAll(CTAHD ctahd, WORD svcid);

/* Add one event code and one associated event value to the tracking list. */
DWORD NMSAPI evtAddEventValue(CTAHD ctahd, DWORD eventCode, DWORD eventValue);

/* Add one event code (regardless of the event value) to the tracking list. */
DWORD NMSAPI evtAddEvent(CTAHD ctahd, DWORD eventCode);


/* Remove all event codes of a service from the tracking list. */
DWORD NMSAPI evtRemoveEventAll(CTAHD ctahd, WORD svcid);

/* Remove one event code and its associated event value from the tracking list. */
DWORD NMSAPI evtRemoveEventValue(CTAHD ctahd, DWORD eventCode, DWORD eventValue);

/* Remove one event code (regardless of the event value) from the tracking list. */
DWORD NMSAPI evtRemoveEvent(CTAHD ctahd, DWORD eventCode);


/* Start logging events to the log file. */
DWORD NMSAPI evtLogStart(CTAHD ctahd);

/* Stop logging events to the log file. */
DWORD NMSAPI evtLogStop(CTAHD ctahd);

#ifdef __cplusplus
}
#endif


#endif /* EVTDEF_INCLUDED */
