
#ifndef NCCSYS_INCLUDED
#define NCCSYS_INCLUDED


#include "nmstypes.h"
#include "ctadisp.h"
#include "nccdef.h"


#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------------------------------------------
  NCC Command Codes.
    - These codes are constructed using the NCC_SVCID, the Natural Access
      CTA_CODE_COMMAND typecode, and a sequence number. The formula is
          COMMANDCODE = ((NCC_SVCID<<16) | (CTA_CODE_COMMAND | SEQUENCE))
    - Command codes are used with xxxProcessCommand binding function
      to invoke server side implementation function corresponding to
      application invoked API.
    - The command numbers MUST be sequential. New commands should be added
      to the end of this table in order.
  ---------------------------------------------------------------------------*/
#define NCCCMD_ACCEPT_CALL              0x001C3000
#define NCCCMD_ANSWER_CALL              0x001C3001
#define NCCCMD_AUTOMATIC_TRANSFER       0x001C3002
#define NCCCMD_BLOCK_CALLS              0x001C3003
#define NCCCMD_DISCONNECT_CALL          0x001C3004
#define NCCCMD_GET_CALL_STATUS          0x001C3005
#define NCCCMD_GET_EXTENDED_CALL_STATUS 0x001C3006
#define NCCCMD_GET_LINE_STATUS          0x001C3007
#define NCCCMD_HOLD_CALL                0x001C3008
#define NCCCMD_PLACE_CALL               0x001C3009
#define NCCCMD_QUERY_CAPABILITY         0x001C300A
#define NCCCMD_REJECT_CALL              0x001C300B
#define NCCCMD_RELEASE_CALL             0x001C300C
#define NCCCMD_RETRIEVE_CALL            0x001C300D
#define NCCCMD_SEND_CALL_MESSAGE        0x001C300E
#define NCCCMD_SEND_DIGITS              0x001C300F
#define NCCCMD_SEND_LINE_MESSAGE        0x001C3010
#define NCCCMD_SET_BILLING              0x001C3011
#define NCCCMD_START_PROTOCOL           0x001C3012
#define NCCCMD_STOP_PROTOCOL            0x001C3013
#define NCCCMD_TRANSFER_CALL            0x001C3014
#define NCCCMD_UNBLOCK_CALLS            0x001C3015
#define NCCCMD_TCT_TRANSFER_CALL        0x001C3016
#define NCCCMD_GET_CALLID               0x001C3017
#define NCCCMD_HOLD_RESPONSE            0x001C3018
#define NCCCMD_REGISTER_USER            0x001C3019
#define NCCCMD_ACKNOWLEDGE_CALL         0x001C301A
#define NCCCMD_SEND_PRACK               0x001C301B
#define NCCCMD_SEND_PRACK_RESPONSE      0x001C301C
#define NCCCMD_SUBSCRIBE                0x001C301D

/* Number of NCC commands. Update if more commands added. */
#define NCCCMD_QTY                      ((0xfff & NCCCMD_SUBSCRIBE)+1)


/* The following capability bits indicate the protocol supports (will send up) the
   corresponding event */
#define NCC_CAP_GET_BILLING              (0x10000000) /* NCCEVN_BILLING_INDICATION */
#define NCC_CAP_OVERLAPPED_RECEIVING     (0x20000000) /* NCCEVN_RECEIVED_DIGIT     */
#define NCC_CAP_STATUS_UPDATE             (0x1000000) /* NCCEVN_STATUS_UPDATE      */
#define NCC_CAP_CAPABILITY_UPDATE         (0x2000000) /* NCCEVN_CAPABILITY_UPDATE  */

/* The following capability indications signify support of the following feature:  */
/* - no call control commands are supported                                        */
#define NCC_CAP_NO_CALL_CONTROL            (0x100000)


/*-----------------------------------------------------------------------------
  nccGetText() Macros.

    - These macros convert NCC Service command, event, error, reason & value
      codes to their corresponding ASCII macro identifier.

    - WARNING!!! If a new event or value code is added has a string length
      that is greater than any event or value defined previously, be sure
      to update the #defines for NCC_MAX_EVENT_STRLEN and NCC_MAX_VALUE_STRLEN.
  ---------------------------------------------------------------------------*/

#define TEXTCASE(e) case e: return #e

#define NCC_COMMANDS() \
        TEXTCASE(NCCCMD_ACCEPT_CALL); \
        TEXTCASE(NCCCMD_ANSWER_CALL); \
        TEXTCASE(NCCCMD_AUTOMATIC_TRANSFER); \
        TEXTCASE(NCCCMD_BLOCK_CALLS); \
        TEXTCASE(NCCCMD_DISCONNECT_CALL); \
        TEXTCASE(NCCCMD_GET_CALL_STATUS); \
        TEXTCASE(NCCCMD_GET_EXTENDED_CALL_STATUS); \
        TEXTCASE(NCCCMD_GET_LINE_STATUS); \
        TEXTCASE(NCCCMD_HOLD_CALL); \
        TEXTCASE(NCCCMD_PLACE_CALL); \
        TEXTCASE(NCCCMD_QUERY_CAPABILITY); \
        TEXTCASE(NCCCMD_REJECT_CALL); \
        TEXTCASE(NCCCMD_RELEASE_CALL); \
        TEXTCASE(NCCCMD_RETRIEVE_CALL); \
        TEXTCASE(NCCCMD_SEND_CALL_MESSAGE); \
        TEXTCASE(NCCCMD_SEND_DIGITS); \
        TEXTCASE(NCCCMD_SEND_LINE_MESSAGE); \
        TEXTCASE(NCCCMD_SET_BILLING); \
        TEXTCASE(NCCCMD_START_PROTOCOL); \
        TEXTCASE(NCCCMD_STOP_PROTOCOL); \
        TEXTCASE(NCCCMD_TRANSFER_CALL); \
        TEXTCASE(NCCCMD_UNBLOCK_CALLS); \
        TEXTCASE(NCCCMD_TCT_TRANSFER_CALL); \
        TEXTCASE(NCCCMD_GET_CALLID); \
        TEXTCASE(NCCCMD_HOLD_RESPONSE); \
        TEXTCASE(NCCCMD_REGISTER_USER); \
        TEXTCASE(NCCCMD_ACKNOWLEDGE_CALL); \
        TEXTCASE(NCCCMD_SEND_PRACK); \
        TEXTCASE(NCCCMD_SEND_PRACK_RESPONSE); \
        TEXTCASE(NCCCMD_SUBSCRIBE)

#define NCC_MAX_EVENT_STRLEN 34

#define NCC_EVENTS() \
        TEXTCASE(NCCEVN_START_PROTOCOL_DONE); \
        TEXTCASE(NCCEVN_STOP_PROTOCOL_DONE); \
        TEXTCASE(NCCEVN_PROTOCOL_ERROR); \
        TEXTCASE(NCCEVN_ACCEPTING_CALL); \
        TEXTCASE(NCCEVN_ANSWERING_CALL); \
        TEXTCASE(NCCEVN_BILLING_INDICATION); \
        TEXTCASE(NCCEVN_BILLING_SET); \
        TEXTCASE(NCCEVN_CALLS_BLOCKED); \
        TEXTCASE(NCCEVN_CALLS_UNBLOCKED); \
        TEXTCASE(NCCEVN_CALL_CONNECTED); \
        TEXTCASE(NCCEVN_CALL_DISCONNECTED); \
        TEXTCASE(NCCEVN_CALL_HELD); \
        TEXTCASE(NCCEVN_PROTOCOL_EVENT); \
        TEXTCASE(NCCEVN_CALL_PROCEEDING); \
        TEXTCASE(NCCEVN_CALL_STATUS_UPDATE); \
        TEXTCASE(NCCEVN_HOLD_REJECTED); \
        TEXTCASE(NCCEVN_INCOMING_CALL); \
        TEXTCASE(NCCEVN_LINE_IN_SERVICE); \
        TEXTCASE(NCCEVN_LINE_OUT_OF_SERVICE); \
        TEXTCASE(NCCEVN_REMOTE_ALERTING); \
        TEXTCASE(NCCEVN_REMOTE_ANSWERED); \
        TEXTCASE(NCCEVN_PLACING_CALL); \
        TEXTCASE(NCCEVN_REJECTING_CALL); \
        TEXTCASE(NCCEVN_CALL_RELEASED); \
        TEXTCASE(NCCEVN_CALL_RETRIEVED); \
        TEXTCASE(NCCEVN_RETRIEVE_REJECTED); \
        TEXTCASE(NCCEVN_SEIZURE_DETECTED); \
        TEXTCASE(NCCEVN_CAPABILITY_UPDATE); \
        TEXTCASE(NCCEVN_EXTENDED_CALL_STATUS_UPDATE); \
        TEXTCASE(NCCEVN_RECEIVED_DIGIT); \
        TEXTCASE(NCCEVN_BLOCK_FAILED); \
        TEXTCASE(NCCEVN_UNBLOCK_FAILED); \
        TEXTCASE(NCCEVN_CALLID_AVAILABLE); \
        TEXTCASE(NCCEVN_HOLD_INDICATION); \
        TEXTCASE(NCCEVN_REGISTER_USER); \
        TEXTCASE(NCCEVN_PRACK_INDICATION); \
        TEXTCASE(NCCEVN_PRACK_CONFIRMATION); \
        TEXTCASE(NCCEVN_SUBSCRIBE); \
        TEXTCASE(NCCEVN_NOTIFY)

#define NCC_REASONS() \
        TEXTCASE(NCCREASON_OUT_OF_RESOURCES); \
        TEXTCASE(NCCREASON_WRONG_CC_METHOD)

#define NCC_ERRORS() \
        TEXTCASE(NCCERR_NOT_CAPABLE); \
        TEXTCASE(NCCERR_INVALID_CALL_STATE); \
        TEXTCASE(NCCERR_ADDRESS_BLOCKED); \
        TEXTCASE(NCCERR_NO_CALLID)

#define NCC_MAX_VALUE_STRLEN 34

#define NCC_VALUES() \
        TEXTCASE(NCC_ACCEPT_PLAY_RING); \
        TEXTCASE(NCC_ACCEPT_PLAY_SILENT); \
        TEXTCASE(NCC_ACCEPT_USER_AUDIO); \
        TEXTCASE(NCC_ACCEPT_USER_AUDIO_INTO_CONNECT); \
        TEXTCASE(NCC_BILLINGSET_DEFAULT); \
        TEXTCASE(NCC_BILLINGSET_FREE); \
        TEXTCASE(NCC_BILLINGSET_CHARGE_ALTERNATE); \
        TEXTCASE(NCC_BILLINGSET_SEND_PULSE); \
        TEXTCASE(NCC_BLOCK_TIMEOUT); \
        TEXTCASE(NCC_BLOCK_REJECTALL); \
        TEXTCASE(NCC_BLOCK_OUT_OF_SERVICE); \
        TEXTCASE(NCC_CON_ANSWERED); \
        TEXTCASE(NCC_CON_CED); \
        TEXTCASE(NCC_CON_DIALTONE_DETECTED); \
        TEXTCASE(NCC_CON_PROCEEDING); \
        TEXTCASE(NCC_CON_RING_BEGIN); \
        TEXTCASE(NCC_CON_RING_QUIT); \
        TEXTCASE(NCC_CON_SIGNAL); \
        TEXTCASE(NCC_CON_SIT_DETECTED); \
        TEXTCASE(NCC_CON_TIMEOUT); \
        TEXTCASE(NCC_CON_VOICE_BEGIN); \
        TEXTCASE(NCC_CON_VOICE_END); \
        TEXTCASE(NCC_CON_VOICE_EXTENDED); \
        TEXTCASE(NCC_CON_VOICE_LONG); \
        TEXTCASE(NCC_CON_VOICE_MEDIUM); \
        TEXTCASE(NCC_CON_TDD_DETECTED);	\
        TEXTCASE(NCC_DIS_BUSY); \
        TEXTCASE(NCC_DIS_CED); \
        TEXTCASE(NCC_DIS_CLEARDOWN_TONE); \
        TEXTCASE(NCC_DIS_REORDER); \
        TEXTCASE(NCC_DIS_DIAL_FAILURE); \
        TEXTCASE(NCC_DIS_DIALTONE); \
        TEXTCASE(NCC_DIS_GLARE); \
        TEXTCASE(NCC_DIS_HOST_TIMEOUT); \
        TEXTCASE(NCC_DIS_INCOMING_FAULT); \
        TEXTCASE(NCC_DIS_NO_ACKNOWLEDGEMENT); \
        TEXTCASE(NCC_DIS_NO_CS_RESOURCE); \
        TEXTCASE(NCC_DIS_NO_DIALTONE); \
        TEXTCASE(NCC_DIS_NO_LOOP_CURRENT); \
        TEXTCASE(NCC_DIS_REJECT_REQUESTED); \
        TEXTCASE(NCC_DIS_REMOTE_ABANDONED); \
        TEXTCASE(NCC_DIS_REMOTE_NOANSWER); \
        TEXTCASE(NCC_DIS_RING_BEGIN); \
        TEXTCASE(NCC_DIS_RING_QUIT); \
        TEXTCASE(NCC_DIS_SIGNAL); \
        TEXTCASE(NCC_DIS_UNASSIGNED_NUMBER); \
        TEXTCASE(NCC_DIS_SIGNAL_UNKNOWN); \
        TEXTCASE(NCC_DIS_SIT_DETECTED); \
        TEXTCASE(NCC_DIS_TIMEOUT); \
        TEXTCASE(NCC_DIS_TRANSFER); \
        TEXTCASE(NCC_DIS_VOICE_BEGIN); \
        TEXTCASE(NCC_DIS_VOICE_END); \
        TEXTCASE(NCC_DIS_VOICE_EXTENDED); \
        TEXTCASE(NCC_DIS_VOICE_LONG); \
        TEXTCASE(NCC_DIS_VOICE_MEDIUM); \
        TEXTCASE(NCC_DIS_PROTOCOL_ERROR); \
        TEXTCASE(NCC_DIS_CONGESTION); \
        TEXTCASE(NCC_DIS_REJECTED); \
        TEXTCASE(NCC_DIS_PREEMPTED_REUSE); \
        TEXTCASE(NCC_DIS_PREEMPTED_IDLE); \
        TEXTCASE(NCC_RELEASED_GLARE); \
        TEXTCASE(NCC_RELEASED_ERROR); \
        TEXTCASE(NCC_CALL_STATUS_CALLINGADDR); \
        TEXTCASE(NCC_OUT_OF_SERVICE_DIGIT_TIMEOUT); \
        TEXTCASE(NCC_OUT_OF_SERVICE_NO_LOOP_CURRENT); \
        TEXTCASE(NCC_OUT_OF_SERVICE_PERM_SIGNAL); \
        TEXTCASE(NCC_OUT_OF_SERVICE_REMOTE_BLOCK); \
        TEXTCASE(NCC_OUT_OF_SERVICE_WINK_STUCK); \
        TEXTCASE(NCC_OUT_OF_SERVICE_NO_DIGITS); \
        TEXTCASE(NCC_OUT_OF_SERVICE_LINE_FAULT); \
        TEXTCASE(NCC_PROTERR_BAD_CALLERID); \
        TEXTCASE(NCC_PROTERR_CAPABILITY_ERROR); \
        TEXTCASE(NCC_PROTERR_DIGIT_TIMEOUT); \
        TEXTCASE(NCC_PROTERR_EXTRA_DIGITS); \
        TEXTCASE(NCC_PROTERR_INVALID_DIGIT); \
        TEXTCASE(NCC_PROTERR_NO_CS_RESOURCE); \
        TEXTCASE(NCC_PROTERR_PREMATURE_ANSWER); \
        TEXTCASE(NCC_PROTERR_TIMEOUT); \
        TEXTCASE(NCC_PROTERR_COMMAND_OUT_OF_SEQUENCE); \
        TEXTCASE(NCC_PROTERR_EVENT_OUT_OF_SEQUENCE); \
        TEXTCASE(NCC_PROTERR_FALSE_SEIZURE); \
        TEXTCASE(NCC_PROTERR_TCT_FAILED); \
        TEXTCASE(NCC_REJECT_HOST_TIMEOUT); \
        TEXTCASE(NCC_REJECT_PLAY_BUSY); \
        TEXTCASE(NCC_REJECT_PLAY_REORDER); \
        TEXTCASE(NCC_REJECT_PLAY_RINGTONE); \
        TEXTCASE(NCC_REJECT_USER_AUDIO); \
        TEXTCASE(NCC_ANSWER_MODEM); \
        TEXTCASE(NCC_ANSWER_SIGNAL); \
        TEXTCASE(NCC_ANSWER_VOICE); \
        TEXTCASE(NCC_UNBLOCK_TIMEOUT); \
        TEXTCASE(NCC_RETRIEVED_AUTO_TRANSFER_FAILED); \
        TEXTCASE(NCC_EXTENDED_CALL_STATUS_CATEGORY);



#ifdef __cplusplus
}
#endif


#endif /* NCCSYS_INCLUDED */
