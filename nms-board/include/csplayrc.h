/*****************************************************************************
*  File -  csplayrc.h
*
*  Description - Header file for Client-Server functionality of incta
*
*
* Copyright (c) 1999-2002 NMS Communications.  All rights reserved.
*****************************************************************************/

/*---------------- Application Generated Events ----------------------------*/
#define     APPEVN_PLAY_MESSAGE     CTA_USER_EVENT(0x1) /* Play Message     */
#define     APPEVN_RECORD_MESSAGE   CTA_USER_EVENT(0x2) /* Record Message   */
#define     APPEVN_PLAY_DONE        CTA_USER_EVENT(0x3) /* Play Done        */
#define     APPEVN_RECORD_DONE      CTA_USER_EVENT(0x4) /* Record Done      */
#define     APPEVN_PLAY_FAILURE     CTA_USER_EVENT(0x5) /* Play Failure     */
#define     APPEVN_RECORD_FAILURE   CTA_USER_EVENT(0x6) /* Record Failure   */
#define     APPEVN_SHUT_DOWN        CTA_USER_EVENT(0x7) /* Shut Down        */
#define     APPEVN_CONTEXT_NAME     CTA_USER_EVENT(0x8) /* Context Name     */

/*---------------- Reasons in Application Generated Events -----------------*/
#define     APP_REASON_FINISHED                 0x0001  /* File not known   */
#define     APP_REASON_DISCONNECT               0x0002  /* File not known   */
#define     APP_REASON_UNKNOWN_FILE             0x0003  /* File not known   */
#define     APP_REASON_FUNCTION_FAILED          0x0004  /* File not known   */
#define     APP_REASON_UNKNOWN_REASON           0x0005  /* File not known   */

/*----------------------- File Types ---------------------------------------*/
#define     CS_DEMO_FILE                        1
#define     CS_TEMP_FILE                        2
#define     CS_CONTEXT_FILE                     3

#define CX_FILE "_context_name_"

