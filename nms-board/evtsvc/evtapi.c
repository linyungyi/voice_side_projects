/******************************************************************************
  NAME:  evtapi.c

  PURPOSE:

      This file implements the EVT service API declared in evtdef.h.

  IMPLEMENTATION:

      Each API function simply:
        -  Calls the corresponding SPI function.
        -  Logs errors (if any) via CTAPIERROR.

  Copyright 2001 NMS Communications.  All rights reserved.
******************************************************************************/

#include "evtsys.h"
#include "evtspi.h"


/*----------------------------------------------------------------------------
*  The event data observed by the EVT service is logged to a log file or
*  the standard output file, based on the application's preference.
*
*  The choice of the log file is for the entire application (and all the
*  queues and contexts under it).  In other words, evtSetLogFile() needs to
*  be called at most once per application.
*
*  The standard output file is used to log the events if evtSetLogFile()
*  is not called or is called with a NULL pointer or a null string ""
*  as the log file name.
*
*  EVT Service API Function evtSetLogFile():
*  Designate the log file name in one of the following two ways:
*
*  1. spell out the name of the event log file;
*
*  2. use NULL pointer or NULL string, "", to indicate that the standard
*     output file stdout is used as the event log file.
*
*     The output of the EVT service will commingle with the output of other
*     services.  This could provide a broader context for the events collected.
*
*  Both the source services of the events and EVT service must be opened
*  under the same Natural Access context for EVT to track the specified events.
* ---------------------------------------------------------------------------*/
DWORD NMSAPI evtSetLogFile
(
    CTAHD           ctahd,      /* (I) context handle of event source service */
                                /*     and EVT service */
    char            *logFileName/* (I) event log file name */
                                /*     NULL: using the stdout as the log file */
                                /*     null string "": using stdout as the log file */
)
{
    DWORD           ret;

    /* Needed by CTAAPIERROR. */
    CTABEGIN( "evtSetLogFile" );

    /* Call corresponding SPI function with CTA_APP_SVCID. */
    ret = evtSpiSetLogFile( ctahd, logFileName, CTA_APP_SVCID );
    
    /* Log error on failure. */
    if ( ret != SUCCESS )
    {
       CTAAPIERROR( ctahd, ret );
    }

    return ret;
}

/*-----------------------------------------------------------------------------
    EVT Service API Function evtAddEventAll():
    Add all event codes of a service to the tracking list.
  ---------------------------------------------------------------------------*/
DWORD NMSAPI evtAddEventAll
(
    CTAHD           ctahd,      /* (I) context handle of event source service */
                                /*     and EVT service */
    WORD            svcid       /* (I) event source service ID */
)
{
    DWORD           ret;

    /* Needed by CTAAPIERROR. */
    CTABEGIN( "evtAddEventAll" );

    /* Call corresponding SPI function with CTA_APP_SVCID. */
    ret = evtSpiAddEventAll( ctahd, svcid, CTA_APP_SVCID );
    
    /* Log error on failure. */
    if ( ret != SUCCESS )
    {
       CTAAPIERROR( ctahd, ret );
    }

    return ret;
}

/*-----------------------------------------------------------------------------
    EVT Service API Function evtAddEventValue():
    Add one event code and one associated event value to the tracking list.
    Use the combination of the event code and event value to track the events.

    The event source service ID is implied by the eventCode.
  ---------------------------------------------------------------------------*/
DWORD NMSAPI evtAddEventValue
(
    CTAHD           ctahd,      /* (I) context handle of event source service */
                                /*     and EVT service */
    DWORD           eventCode,  /* (I) event code to be added to the tracking list */
    DWORD           eventValue	/* (I) associated event value */
)
{
    DWORD           ret;

    /* Needed by CTAAPIERROR. */
    CTABEGIN( "evtAddEventValue" );

    /* Call corresponding SPI function with CTA_APP_SVCID. */
    ret = evtSpiAddEventValue( ctahd, eventCode, eventValue, CTA_APP_SVCID );
    
    /* Log error on failure. */
    if ( ret != SUCCESS )
    {
       CTAAPIERROR( ctahd, ret );
    }

    return ret;
}

/*-----------------------------------------------------------------------------
    EVT Service API Function evtAddEvent():
    Add one event code (regardless of the event value) to the tracking list.

    The event source service ID is implied by the eventCode.
  ---------------------------------------------------------------------------*/
DWORD NMSAPI evtAddEvent
(
    CTAHD           ctahd,      /* (I) context handle of event source service */
                                /*     and EVT service */
    DWORD           eventCode   /* (I) event code to be added to the tracking list */
)
{
    DWORD           ret;

    /* Needed by CTAAPIERROR. */
    CTABEGIN( "evtAddEvent" );

    /* Call corresponding SPI function with CTA_APP_SVCID. */
    ret = evtSpiAddEvent( ctahd, eventCode, CTA_APP_SVCID );
    
    /* Log error on failure. */
    if ( ret != SUCCESS )
    {
       CTAAPIERROR( ctahd, ret );
    }

    return ret;
}

/*-----------------------------------------------------------------------------
    EVT Service API Function evtRemoveEventAll():
    Remove all event codes of a service from the tracking list.

    Each evtRemoveEventAll() call must have a prior matching evtAddEventAll()
    call with identical arguments.
  ---------------------------------------------------------------------------*/
DWORD NMSAPI evtRemoveEventAll
(
    CTAHD           ctahd,      /* (I) context handle of event source service */
                                /*     and EVT service */
    WORD            svcid       /* (I) event source service ID */
)
{
    DWORD           ret;

    /* Needed by CTAAPIERROR. */
    CTABEGIN( "evtRemoveEventAll" );

    /* Call corresponding SPI function with CTA_APP_SVCID. */
    ret = evtSpiRemoveEventAll( ctahd, svcid, CTA_APP_SVCID );
    
    /* Log error on failure. */
    if ( ret != SUCCESS )
    {
       CTAAPIERROR( ctahd, ret );
    }

    return ret;
}

/*-----------------------------------------------------------------------------
    EVT Service API Function evtRemoveEventValue():
    Remove one event code and its associated event value from the tracking list.

    The event source service ID is implied by the eventCode.

    Each evtRemoveEventValue() call must have a prior matching evtAddEventValue()
    call with identical arguments.
  ---------------------------------------------------------------------------*/
DWORD NMSAPI evtRemoveEventValue
(
    CTAHD           ctahd,      /* (I) context handle of event source service */
                                /*     and EVT service */
    DWORD           eventCode,  /* (I) event code to be removed from the tracking list */
    DWORD           eventValue	/* (I) associated event value */
)
{
    DWORD           ret;

    /* Needed by CTAAPIERROR. */
    CTABEGIN( "evtRemoveEventValue" );

    /* Call corresponding SPI function with CTA_APP_SVCID. */
    ret = evtSpiRemoveEventValue( ctahd, eventCode, eventValue, CTA_APP_SVCID );
    
    /* Log error on failure. */
    if ( ret != SUCCESS )
    {
       CTAAPIERROR( ctahd, ret );
    }

    return ret;
}

/*-----------------------------------------------------------------------------
    EVT Service API Function evtRemoveEvent():
    Remove one event code (regardless of the event value) from the tracking list.

    The event source service ID is implied by the eventCode.

    Each evtRemoveEvent() call must have a prior matching evtAddEvent()
    call with identical arguments.
  ---------------------------------------------------------------------------*/
DWORD NMSAPI evtRemoveEvent
(
    CTAHD           ctahd,      /* (I) context handle of event source service */
                                /*     and EVT service */
    DWORD           eventCode   /* (I) event code to be removed from the tracking list */
)
{
    DWORD           ret;

    /* Needed by CTAAPIERROR. */
    CTABEGIN( "evtRemoveEvent" );

    /* Call corresponding SPI function with CTA_APP_SVCID. */
    ret = evtSpiRemoveEvent( ctahd, eventCode, CTA_APP_SVCID );
    
    /* Log error on failure. */
    if ( ret != SUCCESS )
    {
       CTAAPIERROR( ctahd, ret );
    }

    return ret;
}

/*-----------------------------------------------------------------------------
    EVT Service API Function evtLogStart():
    Start logging events to the log file.

    EVT application can use pairs of evtLogStart() and evtLogStop()
    to bracket software segments of interest for event tracking purpose.

    Within one context, there should not be nested pairs of evtLogStart() and
    evtLogStop().  Two consecutive evtLogStart() calls will not change the fact
    that the event logging is on.  Two consecutive evtLogStop() calls will not
    change the fact that the event logging is off.
  ---------------------------------------------------------------------------*/
DWORD NMSAPI evtLogStart
(
    CTAHD           ctahd       /* (I) context handle of event source service */
                                /*     and EVT service */
)
{
    DWORD           ret;

    /* Needed by CTAAPIERROR. */
    CTABEGIN( "evtLogStart" );

    /* Call corresponding SPI function with CTA_APP_SVCID. */
    ret = evtSpiLogStart( ctahd, CTA_APP_SVCID );
    
    /* Log error on failure. */
    if ( ret != SUCCESS )
    {
       CTAAPIERROR( ctahd, ret );
    }

    return ret;
}

/*-----------------------------------------------------------------------------
    EVT Service API Function evtLogStop():
    Stop logging events to the log file.
  ---------------------------------------------------------------------------*/
DWORD NMSAPI evtLogStop
(
    CTAHD           ctahd       /* (I) context handle of event source service */
                                /*     and EVT service */
)
{
    DWORD           ret;

    /* Needed by CTAAPIERROR. */
    CTABEGIN( "evtLogStop" );

    /* Call corresponding SPI function with CTA_APP_SVCID. */
    ret = evtSpiLogStop( ctahd, CTA_APP_SVCID );
    
    /* Log error on failure. */
    if ( ret != SUCCESS )
    {
       CTAAPIERROR( ctahd, ret );
    }

    return ret;
}
