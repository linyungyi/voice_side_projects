/******************************************************************************
  NAME:  evtspi.c

  PURPOSE:

      This file implements the EVT service SPI declared in evtdef.h.

  IMPLEMENTATION:

      For each API function, there is a corresponding SPI function. Each SPI
      function simply:
         1.) marshals API arguments into a message
         2.) marshals the function identifier into a message 
         3.) sends the message to the EVT service (evtProcessCommand)
             via dispSendCommand 

  NOTES:
 
     ABOUT TRACING:  An SPI function does not perform any tracing or error 
                     logging. This functionality is the responsibility of
                     the associated the caller which can either be:
                       - the corresponding EVT API function,  or
                       - a function within another Natural Access Service
                         (remember, other Natural Access services can only invoke
                          Service SPI functions, not API functions!)
      
  Copyright 2001 NMS Communications.  All rights reserved.
******************************************************************************/

#include "string.h"
#include "evtsys.h"

/*----------------------------------------------------------------------------
The following are EVT SPI functions.  Refer to the corresponding EVT API
functions in evtapi.c for the descriptions of all the arguments except the
extra "source" argument to all SPI functions.

The "source" argument indicates the invoker of the SPI function call
which can be CTA_APP_SVCID (if it is called through the API function)
or can be other service id (if it is called by another service).
 ----------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
DWORD NMSAPI evtSpiSetLogFile(CTAHD ctahd, char *logFileName, WORD source)
{
    DWORD               ret; 
    DISP_COMMAND        m; 
    int                 length;

    m.id                = EVTCMD_SET_LOGFILE;
    m.ctahd             = ctahd; 
    /*
    *  If an argument is simply a scalar, set
    *  size field to scalar value and set
    *  corresponding pointer to NULL.
    */
    /*
    *  If an argument is a pointer to a structure,
    *  set size field to sizeof(*argument) and set
    *  corresponding pointer to argument.
    */
    /* A null pointer or a null string means that the standard output file is
    *  used as the event log file.
    */
    if (logFileName == NULL || 0 >= (length = strlen(logFileName)))
    {
        m.size1         = 0;
        m.dataptr1      = NULL;
    }
    else
    {
        /* counting the ending '\0' character */
        m.size1         = length + 1;
        m.dataptr1      = logFileName;
    }
    /*
    *  If size and dataptr are not used, set size
    *  field to 0 and corresponding pointer to NULL.
    */
    m.size2             = 0;
    m.dataptr2          = NULL;

    m.size3             = 0;
    m.dataptr3          = NULL;
    /* NOTE : If size or dataptr is used as out
    *  buffer to store synchronous return, OR in
    *  CTA_VIRTUAL_BUF in size field.
    */
    m.objHd             = 0; 
    m.addr.destination  = EVT_SVCID; 
    m.addr.source       = source; 

    /*
    *   Invoke dispSendCommand with DISP_MESSAGE
    *   constructed above.
    */

    ret = dispSendCommand(&m);

    /*
    *   If synchronous return data is expected
    *   via DISP_COMMAND, copy data into
    *   appropriate argument(s) prior to return.
    *   See tikGetChannelInfo in TIK Service Demo.
    *   Remember to return SUCCESS_RESPONSE from
    *   service side implementation function.
    */

    return ret;
}

/*---------------------------------------------------------------------------*/
DWORD NMSAPI evtSpiAddEventAll(CTAHD ctahd, WORD svcid, WORD source)
{
    DWORD               ret; 
    DISP_COMMAND        m; 

    m.id                = EVTCMD_ADD_EVENTALL;
    m.ctahd             = ctahd; 
    /*
    *  If an argument is simply a scalar, set
    *  size field to scalar value and set
    *  corresponding pointer to NULL.
    */
    m.size1             = svcid;
    m.dataptr1          = NULL;
    /*
    *  If an argument is a pointer to a structure,
    *  set size field to sizeof(*argument) and set
    *  corresponding pointer to argument.
    */
    /*
    *  If size and dataptr are not used, set size
    *  field to 0 and corresponding pointer to NULL.
    */
    m.size2             = 0;
    m.dataptr2          = NULL;

    m.size3             = 0;
    m.dataptr3          = NULL;
    /* NOTE : If size or dataptr is used as out
    *  buffer to store synchronous return, OR in
    *  CTA_VIRTUAL_BUF in size field.
    */
    m.objHd             = 0; 
    m.addr.destination  = EVT_SVCID; 
    m.addr.source       = source; 

    /*
    *   Invoke dispSendCommand with DISP_MESSAGE
    *   constructed above.
    */

    ret = dispSendCommand(&m);

    /*
    *   If synchronous return data is expected
    *   via DISP_COMMAND, copy data into
    *   appropriate argument(s) prior to return.
    *   See tikGetChannelInfo in TIK Service Demo.
    *   Remember to return SUCCESS_RESPONSE from
    *   service side implementation function.
    */

    return ret;
}

/*---------------------------------------------------------------------------*/
DWORD NMSAPI evtSpiAddEventValue(CTAHD ctahd, DWORD eventCode, DWORD eventValue, WORD source)
{
    DWORD               ret; 
    DISP_COMMAND        m; 

    m.id                = EVTCMD_ADD_EVENTVALUE;
    m.ctahd             = ctahd; 
    /*
    *  If an argument is simply a scalar, set
    *  size field to scalar value and set
    *  corresponding pointer to NULL.
    */
    m.size1             = eventCode;
    m.dataptr1          = NULL;

    m.size2             = eventValue;
    m.dataptr2          = NULL;
    /*
    *  If an argument is a pointer to a structure,
    *  set size field to sizeof(*argument) and set
    *  corresponding pointer to argument.
    */
    /*
    *  If size and dataptr are not used, set size
    *  field to 0 and corresponding pointer to NULL.
    */
    m.size3             = 0;
    m.dataptr3          = NULL;
    /* NOTE : If size or dataptr is used as out
    *  buffer to store synchronous return, OR in
    *  CTA_VIRTUAL_BUF in size field.
    */
    m.objHd             = 0; 
    m.addr.destination  = EVT_SVCID; 
    m.addr.source       = source; 

    /*
    *   Invoke dispSendCommand with DISP_MESSAGE
    *   constructed above.
    */

    ret = dispSendCommand(&m);

    /*
    *   If synchronous return data is expected
    *   via DISP_COMMAND, copy data into
    *   appropriate argument(s) prior to return.
    *   See tikGetChannelInfo in TIK Service Demo.
    *   Remember to return SUCCESS_RESPONSE from
    *   service side implementation function.
    */

    return ret;
}

/*---------------------------------------------------------------------------*/
DWORD NMSAPI evtSpiAddEvent(CTAHD ctahd, DWORD eventCode, WORD source)
{
    DWORD               ret; 
    DISP_COMMAND        m; 

    m.id                = EVTCMD_ADD_EVENT;
    m.ctahd             = ctahd; 
    /*
    *  If an argument is simply a scalar, set
    *  size field to scalar value and set
    *  corresponding pointer to NULL.
    */
    m.size1             = eventCode;
    m.dataptr1          = NULL;
    /*
    *  If an argument is a pointer to a structure,
    *  set size field to sizeof(*argument) and set
    *  corresponding pointer to argument.
    */
    /*
    *  If size and dataptr are not used, set size
    *  field to 0 and corresponding pointer to NULL.
    */
    m.size2             = 0;
    m.dataptr2          = NULL;

    m.size3             = 0;
    m.dataptr3          = NULL;
    /* NOTE : If size or dataptr is used as out
    *  buffer to store synchronous return, OR in
    *  CTA_VIRTUAL_BUF in size field.
    */
    m.objHd             = 0; 
    m.addr.destination  = EVT_SVCID; 
    m.addr.source       = source; 

    /*
    *   Invoke dispSendCommand with DISP_MESSAGE
    *   constructed above.
    */

    ret = dispSendCommand(&m);

    /*
    *   If synchronous return data is expected
    *   via DISP_COMMAND, copy data into
    *   appropriate argument(s) prior to return.
    *   See tikGetChannelInfo in TIK Service Demo.
    *   Remember to return SUCCESS_RESPONSE from
    *   service side implementation function.
    */

    return ret;
}

/*---------------------------------------------------------------------------*/
DWORD NMSAPI evtSpiRemoveEventAll(CTAHD ctahd, WORD svcid, WORD source)
{
    DWORD               ret; 
    DISP_COMMAND        m; 

    m.id                = EVTCMD_REMOVE_EVENTALL;
    m.ctahd             = ctahd; 
    /*
    *  If an argument is simply a scalar, set
    *  size field to scalar value and set
    *  corresponding pointer to NULL.
    */
    m.size1             = svcid;
    m.dataptr1          = NULL;
    /*
    *  If an argument is a pointer to a structure,
    *  set size field to sizeof(*argument) and set
    *  corresponding pointer to argument.
    */
    /*
    *  If size and dataptr are not used, set size
    *  field to 0 and corresponding pointer to NULL.
    */
    m.size2             = 0;
    m.dataptr2          = NULL;

    m.size3             = 0;
    m.dataptr3          = NULL;
    /* NOTE : If size or dataptr is used as out
    *  buffer to store synchronous return, OR in
    *  CTA_VIRTUAL_BUF in size field.
    */
    m.objHd             = 0; 
    m.addr.destination  = EVT_SVCID; 
    m.addr.source       = source; 

    /*
    *   Invoke dispSendCommand with DISP_MESSAGE
    *   constructed above.
    */

    ret = dispSendCommand(&m);

    /*
    *   If synchronous return data is expected
    *   via DISP_COMMAND, copy data into
    *   appropriate argument(s) prior to return.
    *   See tikGetChannelInfo in TIK Service Demo.
    *   Remember to return SUCCESS_RESPONSE from
    *   service side implementation function.
    */

    return ret;
}

/*---------------------------------------------------------------------------*/
DWORD NMSAPI evtSpiRemoveEventValue(CTAHD ctahd, DWORD eventCode, DWORD eventValue, WORD source)
{
    DWORD               ret; 
    DISP_COMMAND        m; 

    m.id                = EVTCMD_REMOVE_EVENTVALUE;
    m.ctahd             = ctahd; 
    /*
    *  If an argument is simply a scalar, set
    *  size field to scalar value and set
    *  corresponding pointer to NULL.
    */
    m.size1             = eventCode;
    m.dataptr1          = NULL;

    m.size2             = eventValue;
    m.dataptr2          = NULL;
    /*
    *  If an argument is a pointer to a structure,
    *  set size field to sizeof(*argument) and set
    *  corresponding pointer to argument.
    */
    /*
    *  If size and dataptr are not used, set size
    *  field to 0 and corresponding pointer to NULL.
    */
    m.size3             = 0;
    m.dataptr3          = NULL;
    /* NOTE : If size or dataptr is used as out
    *  buffer to store synchronous return, OR in
    *  CTA_VIRTUAL_BUF in size field.
    */
    m.objHd             = 0; 
    m.addr.destination  = EVT_SVCID; 
    m.addr.source       = source; 

    /*
    *   Invoke dispSendCommand with DISP_MESSAGE
    *   constructed above.
    */

    ret = dispSendCommand(&m);

    /*
    *   If synchronous return data is expected
    *   via DISP_COMMAND, copy data into
    *   appropriate argument(s) prior to return.
    *   See tikGetChannelInfo in TIK Service Demo.
    *   Remember to return SUCCESS_RESPONSE from
    *   service side implementation function.
    */

    return ret;
}

/*---------------------------------------------------------------------------*/
DWORD NMSAPI evtSpiRemoveEvent(CTAHD ctahd, DWORD eventCode, WORD source)
{
    DWORD               ret; 
    DISP_COMMAND        m; 

    m.id                = EVTCMD_REMOVE_EVENT;
    m.ctahd             = ctahd; 
    /*
    *  If an argument is simply a scalar, set
    *  size field to scalar value and set
    *  corresponding pointer to NULL.
    */
    m.size1             = eventCode;
    m.dataptr1          = NULL;
    /*
    *  If an argument is a pointer to a structure,
    *  set size field to sizeof(*argument) and set
    *  corresponding pointer to argument.
    */
    /*
    *  If size and dataptr are not used, set size
    *  field to 0 and corresponding pointer to NULL.
    */
    m.size2             = 0;
    m.dataptr2          = NULL;

    m.size3             = 0;
    m.dataptr3          = NULL;
    /* NOTE : If size or dataptr is used as out
    *  buffer to store synchronous return, OR in
    *  CTA_VIRTUAL_BUF in size field.
    */
    m.objHd             = 0; 
    m.addr.destination  = EVT_SVCID; 
    m.addr.source       = source; 

    /*
    *   Invoke dispSendCommand with DISP_MESSAGE
    *   constructed above.
    */

    ret = dispSendCommand(&m);

    /*
    *   If synchronous return data is expected
    *   via DISP_COMMAND, copy data into
    *   appropriate argument(s) prior to return.
    *   See tikGetChannelInfo in TIK Service Demo.
    *   Remember to return SUCCESS_RESPONSE from
    *   service side implementation function.
    */

    return ret;
}

/*---------------------------------------------------------------------------*/
DWORD NMSAPI evtSpiLogStart(CTAHD ctahd, WORD source)
{
    DWORD               ret; 
    DISP_COMMAND        m; 

    m.id                = EVTCMD_LOGSTART;
    m.ctahd             = ctahd; 
    /*
    *  If an argument is simply a scalar, set
    *  size field to scalar value and set
    *  corresponding pointer to NULL.
    */
    /*
    *  If an argument is a pointer to a structure,
    *  set size field to sizeof(*argument) and set
    *  corresponding pointer to argument.
    */
    /*
    *  If size and dataptr are not used, set size
    *  field to 0 and corresponding pointer to NULL.
    */
    m.size1             = 0;
    m.dataptr1          = NULL;

    m.size2             = 0;
    m.dataptr2          = NULL;

    m.size3             = 0;
    m.dataptr3          = NULL;
    /* NOTE : If size or dataptr is used as out
    *  buffer to store synchronous return, OR in
    *  CTA_VIRTUAL_BUF in size field.
    */
    m.objHd             = 0; 
    m.addr.destination  = EVT_SVCID; 
    m.addr.source       = source; 

    /*
    *   Invoke dispSendCommand with DISP_MESSAGE
    *   constructed above.
    */

    ret = dispSendCommand(&m);

    /*
    *   If synchronous return data is expected
    *   via DISP_COMMAND, copy data into
    *   appropriate argument(s) prior to return.
    *   See tikGetChannelInfo in TIK Service Demo.
    *   Remember to return SUCCESS_RESPONSE from
    *   service side implementation function.
    */

    return ret;
}

/*---------------------------------------------------------------------------*/
DWORD NMSAPI evtSpiLogStop(CTAHD ctahd, WORD source)
{
    DWORD               ret; 
    DISP_COMMAND        m; 

    m.id                = EVTCMD_LOGSTOP;
    m.ctahd             = ctahd; 
    /*
    *  If an argument is simply a scalar, set
    *  size field to scalar value and set
    *  corresponding pointer to NULL.
    */
    /*
    *  If an argument is a pointer to a structure,
    *  set size field to sizeof(*argument) and set
    *  corresponding pointer to argument.
    */
    /*
    *  If size and dataptr are not used, set size
    *  field to 0 and corresponding pointer to NULL.
    */
    m.size1             = 0;
    m.dataptr1          = NULL;

    m.size2             = 0;
    m.dataptr2          = NULL;

    m.size3             = 0;
    m.dataptr3          = NULL;
    /* NOTE : If size or dataptr is used as out
    *  buffer to store synchronous return, OR in
    *  CTA_VIRTUAL_BUF in size field.
    */
    m.objHd             = 0; 
    m.addr.destination  = EVT_SVCID; 
    m.addr.source       = source; 

    /*
    *   Invoke dispSendCommand with DISP_MESSAGE
    *   constructed above.
    */

    ret = dispSendCommand(&m);

    /*
    *   If synchronous return data is expected
    *   via DISP_COMMAND, copy data into
    *   appropriate argument(s) prior to return.
    *   See tikGetChannelInfo in TIK Service Demo.
    *   Remember to return SUCCESS_RESPONSE from
    *   service side implementation function.
    */

    return ret;
}

