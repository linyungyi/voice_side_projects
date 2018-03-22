/*****************************************************************************
  NAME:  tikspi.h
  
  PURPOSE:

      This file contains macros and function prototypes that define the 
      SPI for the TIK sample service.

  Copyright (c) 1997-2002 NMS Communications.  All rights reserved.
 
  ****************************************************************************/

#ifndef TIKSPI_INCLUDED
#define TIKSPI_INCLUDED 

#include "nmstypes.h"
#include "ctadef.h"
#include "tikparm.h"

/* Prevent C++ name mangling. */
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------------------------------------------
  TIK SPI Compatibility Level Id
    - This id is used to determine runtime compatibility between the
      installed TIK service and clients of the TIK service. Clients can
      be another Natural Access Service  (which access TIK functionality
      via the TIK SPI).
    - Clients check this id value via CTAREQSVC_INFO structure passed 
      with dispRegisterService in its service implementation of
      xxxDefineService binding function.
    - SPI compatiblity level value should be incremented when the associated 
      source module changes in a non-backwards compatible way. Refer to the CT 
      Access Service Writer's Manual for more info.
  ---------------------------------------------------------------------------*/
#define TIKSPI_COMPATLEVEL                  1



/*-----------------------------------------------------------------------------
  Associated TIK Service SPI function prototypes.
    - For each API call, there is an associated SPI call which performs
      argument marshalling and invokes dispSendCommand() for processing.
    - Note that the only difference between the API signature and the SPI
      signature is the extra "source" argument.
    - Also note that if another service needed to call a TIK Service function,
      it would call the SPI function - NOT THE API FUNCTION!
  ---------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
  SPI function for tikStartTimer API call.
  ---------------------------------------------------------------------------*/
DWORD NMSAPI
  tikSpiStartTimer( CTAHD            ctahd,
                    TIK_START_PARMS *start,
                    WORD             source );
                            
/*---------------------------------------------------------------------------
  SPI for tikStopTimer() API call.
  -------------------------------------------------------------------------*/
DWORD NMSAPI
  tikSpiStopTimer( CTAHD ctahd,
                   WORD  source );

/*---------------------------------------------------------------------------
  SPI function for tikGetChannelInfo API call.
  --------------------------------------------------------------------------*/
DWORD NMSAPI 
  tikSpiGetChannelInfo( CTAHD             ctahd,
                        TIK_CHANNEL_INFO *info,
                        unsigned          insize,
                        unsigned         *outsize,
                        WORD              source );

#ifdef __cplusplus
}
#endif

#endif /* TIKSPI_INCLUDED */

