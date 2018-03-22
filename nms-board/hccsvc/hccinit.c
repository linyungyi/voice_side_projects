/*****************************************************************************
* FILE:        hccinit.c
*
* DESCRIPTION:
*              This file contains the:
*
*                - The exported binding function for hccmgr
*****************************************************************************/

#include "hccsys.h"

STATIC CTASVCMGR_FNS hccServiceMgrFns =
{
    sizeof(CTASVCMGR_FNS),
    hccDefineService,           /* xxxDefineService()                       */
    NULL,                       /* xxxAttachServiceManager() -- use default */
    NULL,                       /* xxxDetachServiceManager() -- use default */
    hccOpenServiceManager,      /* xxxOpenServiceManager()                  */
    hccCloseServiceManager,     /* xxxCloseServiceManager()                 */
    hccOpenService,             /* xxxOpenService()                         */
    hccCloseService,            /* xxxCloseService()                        */
    hccProcessEvent,            /* xxxProcessEvent()                        */
    hccProcessCommand,          /* xxxProcessCommand()                      */
    NULL,                       /* xxxAddRTC()               -- use default */
    NULL,                       /* xxxRemoveRTC()            -- use default */
    hccGetText,                 /* xxxGetText()                             */
    hccFormatMessage,           /* xxxFormatMessage()                       */
    NULL,                       /* xxxSetTraceLevel()                       */
    NULL,                       /* xxxFormatTraceBuffer()                   */
    NULL,                       /* xxxGetFunctionPointer()                  */
};


STATIC      unsigned            hcc_MAJORREV        = HCC_MAJORREV;
STATIC      unsigned            hcc_MINORREV        = HCC_MINORREV;
STATIC      char                hccBuildDate[]      = __DATE__;

STATIC      unsigned            _hccInitialized     = FALSE;

volatile    DWORD*              hccTracePointer;

/*----------------------------------------------------------------------------
 *                      hccInitializeManager()
 * The only function that needs to be exported from the ncc library.
 *--------------------------------------------------------------------------*/
DWORD NMSAPI
    hccInitializeManager ( void )
{
    DWORD ret;
    static CTAINTREV_INFO hccSvrMgrRevInfo = { 0 };

    CTABEGIN( "hccInitializeManager" );

    if ( _hccInitialized )
        return CTAERR_ALREADY_INITIALIZED;

    /* Set the Revision information ... */
    hccSvrMgrRevInfo.size         = sizeof( CTAINTREV_INFO );
    hccSvrMgrRevInfo.majorrev     = hcc_MAJORREV;
    hccSvrMgrRevInfo.minorrev     = hcc_MINORREV;
    hccSvrMgrRevInfo.expapilevel  = NCCAPI_COMPATLEVEL;
    hccSvrMgrRevInfo.expspilevel  = NCCSPI_COMPATLEVEL;
    hccSvrMgrRevInfo.reqdisplevel = DISP_COMPATLEVEL;
    strcpy (hccSvrMgrRevInfo.builddate, hccBuildDate);

    dispGetTracePointer(&hccTracePointer);

    /* Register service manager */
    ret = dispRegisterServiceManager( "HCCMGR",
                                      &hccServiceMgrFns,
                                      &hccSvrMgrRevInfo);

    if (ret != SUCCESS)
    {
        return CTALOGERROR(NULL_CTAHD, ret, NCC_SVCID);
    }

    /*
     * Get the address of the system wide trace pointer for trace checking.
     * If tracing is not enabled then ctaGlobalTracePointer will be set
     * to NULL and no tracing should be done by any service.
     * Note: tracing is not supported in this demo. trace mask is obtained
     *       here anyway.
     */
    dispGetTracePointer(&hccTracePointer);

    _hccInitialized = TRUE;
    return SUCCESS;

}   /* end hccInitializeManager() */

