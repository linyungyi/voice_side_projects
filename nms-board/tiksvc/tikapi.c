/******************************************************************************
  NAME:  tikapi.c

  PURPOSE:

      This file implements the TIK service API declared in tikdef.h.

  IMPLEMENTATION:

      Each API function simply:
        -  Calls the corresponding SPI function.
        -  Logs errors (if any) via CTAPIERROR.

  Copyright (c) 1997-2002 NMS Communications.  All rights reserved.
******************************************************************************/

#include "tiksys.h"
                       


/*-----------------------------------------------------------------------------
  tikStartTimer
    - Tik Service API Function to start timer function 
      on TICK Server.
  ---------------------------------------------------------------------------*/
DWORD NMSAPI tikStartTimer( CTAHD            ctahd, 
                            TIK_START_PARMS *start )
{
    DWORD           ret;

    /* Needed by CTAAPIERROR. */
    CTABEGIN( "tikStartTimer" );

    /* Call corresponding SPI function with CTA_APP_SVCID. */
    ret = tikSpiStartTimer( ctahd, start, CTA_APP_SVCID );
    
    /* Log error on failure */
    if ( ret != SUCCESS )
    {
       CTAAPIERROR( ctahd, ret );
    }

    return ret;
    
}   /* end tikStartTimer() */




/*-----------------------------------------------------------------------------
  tikStopTimer
    - Tik Service API Function to stop timer function 
      on TICK Server.
  ---------------------------------------------------------------------------*/
DWORD NMSAPI tikStopTimer( CTAHD ctahd )
{
    DWORD    ret;

    /* Needed by CTAAPIERROR. */
    CTABEGIN( "tikStopTimer" );

    /* Call corresponding SPI function with CTA_APP_SVCID. */
    ret = tikSpiStopTimer( ctahd, CTA_APP_SVCID );
    
    /* Log error on failure */
    if ( ret != SUCCESS )
    {
       CTAAPIERROR( ctahd, ret );
    }
    
    return ret;
    
}   /* end tikStopTimer() */



/*-----------------------------------------------------------------------------
  tikGetChannelInfo
    - Tik Service API Function to get operational information 
      from TICK Server.
  ---------------------------------------------------------------------------*/
DWORD NMSAPI tikGetChannelInfo( CTAHD             ctahd,
                                TIK_CHANNEL_INFO *info,
                                unsigned          insize,
                                unsigned         *outsize )
{
    DWORD        ret;
    
    /* Needed by CTAAPIERROR. */
    CTABEGIN( "tikGetChannelInfo" );
    
    /* Call corresponding SPI function with CTA_APP_SVCID. */
    ret = tikSpiGetChannelInfo( ctahd, info, insize, outsize, CTA_APP_SVCID );

    /* Log error on failure */
    if ( ret != SUCCESS )
    {
       CTAAPIERROR( ctahd, ret );
    }

    return ret;
    
}   /* end tikGetChannelInfo() */
