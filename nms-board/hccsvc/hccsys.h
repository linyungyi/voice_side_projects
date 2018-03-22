/*****************************************************************************
* FILE:        hccsys.h
*
* DESCRIPTION:
*              This file contains the:
*
*                - macro, definition, and function protoctypes
*****************************************************************************/

#ifndef hccSysH
#define hccSysH

/*---------------------------------------------------------------------
 *    Includes
 *-------------------------------------------------------------------*/
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>             /* for various str functions    */
#include <stdlib.h>             /* for exit(),malloc(),free()   */
#include <ctype.h>              /* for tolower(), toupper()     */
#include <fcntl.h>              /* for open()                   */
#include <stdio.h>

#include "nmstypes.h"
#include "ctadef.h"
#include "ctaerr.h"
#include "ctadisp.h"

#include "nccdef.h"
#include "nccsys.h"
#include "nccstart.h"

#include "adidef.h"
#include "adispi.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define HCC_MAJORREV            1
#define HCC_MINORREV            0

//state id for HostTcp states
#define HOSTTCP_STATE_NULL                          (0x0)
#define HOSTTCP_STATE_UNINITIALIZED                 (0x1)
#define HOSTTCP_STATE_IDLE                          (0x2)
#define HOSTTCP_STATE_SEIZURE                       (0x3)
#define HOSTTCP_STATE_RECEIVE_ADDR                  (0x4)
#define HOSTTCP_STATE_WAITAPP_REJACCEPT             (0x5)
#define HOSTTCP_STATE_ACCEPT                        (0x6)
#define HOSTTCP_STATE_REJECT                        (0x7)
#define HOSTTCP_STATE_ANSWER                        (0x8)
#define HOSTTCP_STATE_WAITPARTY_LEAVELINE           (0x9)
#define HOSTTCP_STATE_SEIZING                       (0xA)
#define HOSTTCP_STATE_SEIZING_ACKED1                (0xB)
#define HOSTTCP_STATE_SEIZING_ACKED2                (0xC)
#define HOSTTCP_STATE_SEND_ADDR                     (0xD)
#define HOSTTCP_STATE_DISCONNECTED                  (0xE)

#define HCC_WNK_CAPABILITY  \
          NCC_CAP_ACCEPT_CALL \
        | NCC_CAP_MEDIA_IN_SETUP \

#define HCC_CALL_NOT_BLOCKED   0x0      //incoming and outgoing calls are allowed
#define HCC_CALL_BLOCKING      0x1      //call blocking command is issued, but line
                                        // is active and NCCEVN_CALL_BLOCKED hasn't been
                                        //generated yet
#define HCC_CALL_BLOCKED       0x2      //all calls are blocked, no incoming/outbound call

#ifndef NCCSPI_COMPATLEVEL
#define NCCSPI_COMPATLEVEL                          1
#endif



DWORD NMSAPI
    hccDefineService (
        char            *svcname );

DWORD NMSAPI
    hccOpenServiceManager (
        CTAHD           ctahd,
        void            *queuecontext,
        void            **mgrcontext );

DWORD NMSAPI
    hccCloseServiceManager (
        CTAHD           ctahd,
        void            *mgrcontext );

DWORD NMSAPI
    hccOpenService (
        CTAHD           ctahd,
        void            *mgrcontext,
        char            *svcname,
        unsigned        svcid,
        CTA_MVIP_ADDR   *mvipaddr,
        CTA_SERVICE_ARGS *svccargs );

DWORD NMSAPI
    hccCloseService (
        CTAHD       ctahd,
        void        *mgrcontext,
        char        *svcname,
        unsigned    svcid );

DWORD NMSAPI
    hccProcessEvent (
        CTAHD           ctahd,
        void            *mgrcontext,
        DISP_EVENT      *ctaevt);

DWORD NMSAPI
    hccProcessCommand (
        CTAHD           ctahd,
        void            *mgrcontext,
        DISP_COMMAND    *ctacmd );

const char* NMSAPI
    hccGetText ( unsigned code );

DWORD NMSAPI
    hccFormatMessage (
        DISP_MESSAGE*   pmsg,
        char*           s,
        unsigned        size,
        char*           indent );

char *
    _hccGetValueText(
        DISP_EVENT*     pevent,
        char*           buffer );

void
    _hccFormatEvent(
        DISP_EVENT *pevent,
        char *printbuf,            /* dispatcher buffer */
        unsigned pbsize,           /* buf size          */
        char *indent);             /* indent            */

char *
    _hccGetSizeText(
        DISP_EVENT*     pevent );

void
    _hccFormatCommand(
        DISP_EVENT *commandp,
        char *printbuf,
        unsigned pbsize,
        char *indent );

void
    _hccFormatError(
        DISP_EVENT *commandp,
        char *printbuf,
        unsigned pbsize,
        char *indent );

#ifdef __cplusplus
}
#endif

#endif //HCCSYS_INCLUDED

