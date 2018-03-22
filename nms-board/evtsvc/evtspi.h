/*****************************************************************************
  NAME:  evtspi.h
  
  PURPOSE:

      This file contains macros and function prototypes that define the 
      SPI for the EVT sample service.

      Your service will be specified by the prefix 'evt' throughout all
      related template service source files.

  Copyright 2001 NMS Communications.  All rights reserved.
  ****************************************************************************/

#ifndef EVTSPI_INCLUDED
#define EVTSPI_INCLUDED 


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
  EVT SPI Compatibility Level Id
    - This id is used to determine runtime compatibility between the
      installed EVT service and clients of the EVT service. Clients can
      be another Natural Access Service (which access EVT functionality
      via the EVT SPI).
    - Clients check this id value via CTAREQSVC_INFO structure
      passed with dispRegisterService in its service implementation
      of evtDefineService binding function.
    - SPI compatiblity level value should be incremented when the associated 
      source module changes in a non-backwards compatible way. Refer to the CT 
      Access Service Writer's Manual for more info.
  ---------------------------------------------------------------------------*/
#define EVTSPI_COMPATLEVEL                  1



/*-----------------------------------------------------------------------------
  Associated EVT Service SPI function prototypes.
    - For each API call, there is an associated SPI call which performs
      argument marshalling and invokes dispSendCommand() for processing.
    - Note that the only difference between the API signature and the SPI
      signature is the extra "source" argument.
    - Also note that if another service needed to call a this Service function,
      it would call the SPI function - NOT THE API FUNCTION!
  ---------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
    ctahd:    Handle for Natural Access context to start.
    arg1:     Some DWORD argument.  May be modified to any type of pass
              argument.
    arg2:     Some structure argument.  May be modified to any type of pass
              argument.
    source:   Service ID of calling service.  Either Application or Another
              Natural Access compliant service.          
              

  Returns SUCCESS on successful execution, otherwise returns a Natural Access
  or Service specific error code on failure.
  ---------------------------------------------------------------------------*/

/* The following are SPI functions: */

/* Designate the log file name */
DWORD NMSAPI evtSpiSetLogFile(CTAHD ctahd, char *logFileName, WORD source);


/* Add all event codes of a service to the tracking list. */
DWORD NMSAPI evtSpiAddEventAll(CTAHD ctahd, WORD svcid, WORD source);

/* Add one event code and one associated event value to the tracking list. */
DWORD NMSAPI evtSpiAddEventValue(CTAHD ctahd, DWORD eventCode, DWORD eventValue, WORD source);

/* Add one event code (regardless of the event value) to the tracking list. */
DWORD NMSAPI evtSpiAddEvent(CTAHD ctahd, DWORD eventCode, WORD source);


/* Remove all event codes of a service from the tracking list. */
DWORD NMSAPI evtSpiRemoveEventAll(CTAHD ctahd, WORD svcid, WORD source);

/* Remove one event code and its associated event value from the tracking list. */
DWORD NMSAPI evtSpiRemoveEventValue(CTAHD ctahd, DWORD eventCode, DWORD eventValue, WORD source);

/* Remove one event code (regardless of the event value) from the tracking list. */
DWORD NMSAPI evtSpiRemoveEvent(CTAHD ctahd, DWORD eventCode, WORD source);


/* Start logging events to the log file. */
DWORD NMSAPI evtSpiLogStart(CTAHD ctahd, WORD source);

/* Stop logging events to the log file. */
DWORD NMSAPI evtSpiLogStop(CTAHD ctahd, WORD source);


#ifdef __cplusplus
}
#endif


#endif /* EVTSPI_INCLUDED */
