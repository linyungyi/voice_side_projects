/******************************************************************************
  NAME:  tikspi.c

  PURPOSE:

      This file implements the TIK service SPI declared in tikdef.h.

  IMPLEMENTATION:

      For each API function, there is a corresponding SPI function. Each SPI
      function simply:
         1.) marshals API arguments into a message
         2.) marshals the function identifier into a message 
         3.) sends the message to the TIK service (tikProcessCommand)
             via dispSendCommand 

  NOTES:
 
     ABOUT TRACING:  An SPI function does not perform any tracing or error 
                     logging. This functionality is the responsibility of
                     the associated the caller which can either be:
                       - the corresponding TIK API function,  or
                       - a function within another Natural Access Service
                         (remember, other Natural Access services can only invoke
                          TIK SPI functions, not TIK API functions!)
      

  Copyright (c) 1997-2002 NMS Communications.  All rights reserved.
******************************************************************************/

#include "tiksys.h"


/*---------------------------------------------------------------------------*/
DWORD NMSAPI tikSpiStartTimer( CTAHD            ctahd,
                               TIK_START_PARMS *start,
                               WORD             source )
{
    void           *dataptr1;
    DWORD           size1;
    DISP_COMMAND    m;
    
    if ( start != NULL )
    {
       /* Use application supplied TIK parameters. */
       size1         = start->size;
       dataptr1      = (void *)start;
    }
    else
    {
       /* Use TIK parameter default values stored in Natural Access. */
       size1         = 0;
       dataptr1      = NULL;
    }

    /* Marshal function id, function arguments, destination service, and 
     * source identifier into message buffer. */
    m.id               = TIKCMD_START;
    m.ctahd            = ctahd;
    m.size1            = size1;
    m.dataptr1         = (dataptr1);
    m.size2            = 0;
    m.dataptr2         = NULL;
    m.size3            = 0;
    m.dataptr3         = NULL;
    m.objHd            = 0;
    m.addr.destination = TIK_SVCID;
    m.addr.source      = source;

    /* Send command to TIK service via Natural Access dispatcher. */
    return dispSendCommand(&m);
}


/*---------------------------------------------------------------------------*/
DWORD NMSAPI tikSpiStopTimer( CTAHD ctahd, WORD source )
{
    DISP_COMMAND m;

    /* Marshal function id, function arguments, destination service, and 
     * source identifier into message buffer. */
    m.id               = TIKCMD_STOP;
    m.ctahd            = ctahd;
    m.size1            = 0;
    m.dataptr1         = NULL;
    m.size2            = 0;
    m.dataptr2         = NULL;
    m.size3            = 0;
    m.dataptr3         = NULL;
    m.objHd            = 0;
    m.addr.destination = TIK_SVCID;
    m.addr.source      = source;

    /*
    *  Send command to TIK service via Natural Access dispatcher. */
    return dispSendCommand(&m);
}


/*---------------------------------------------------------------------------*/
DWORD NMSAPI tikSpiGetChannelInfo( CTAHD             ctahd, 
                                   TIK_CHANNEL_INFO *info,
                                   unsigned          insize,
                                   unsigned         *outsize,
                                   WORD              source )
{
    DISP_COMMAND m;
    DWORD        ret;

    if ( outsize == NULL )
    {
       return CTAERR_BAD_ARGUMENT;
    }

    /* Marshal function id, function arguments, destination service, and 
     * source identifier into message buffer. 
     * 
     * Note that by ORing CTA_VIRTUAL_BUF into the size1 field, we are
     * informing the dispatcher that return data is expected in the 
     * dataptr1 field. */
    m.id               = TIKCMD_GET_CHANNEL_INFO;
    m.ctahd            = ctahd;
    m.size1            = (DWORD)(insize | CTA_VIRTUAL_BUF);
    m.dataptr1         = (void *)(info);
    m.size2            = 0;
    m.dataptr2         = NULL;
    m.size3            = 0;
    m.dataptr3         = NULL;
    m.objHd            = 0;
    m.addr.destination = TIK_SVCID;
    m.addr.source      = source;

    /* Send command to TIK service via Natural Access dispatcher. */
    ret = dispSendCommand(&m);

    if ( ret == SUCCESS )
    {
       /*
       *  Copy returned size of info in outsize.
       */
       *outsize = (unsigned)m.size3;
    }

    return ret;
}
