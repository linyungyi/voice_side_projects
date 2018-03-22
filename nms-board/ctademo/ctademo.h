/*****************************************************************************
*  FILE: ctademo.h
*
*  DESCRIPTION: a common include file for the demo programs, useful
*      even for those that don't use ctademo functions.
*
* Copyright (c) 1996-2002 NMS Communications.  All rights reserved.
*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------- System Includes ----------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#if defined (UNIX)
  #include <fcntl.h>
  #include <unistd.h>
  #include <poll.h>
  #ifdef MAKEOBJ
      #include <termio.h>
  #endif
  #include <sys/time.h>
#elif defined (WIN32)
  #include <windows.h>
#endif


/*----------------------------- Natural Access Includes -------------------------*/
#include "ctadef.h"
#include "adidef.h"
#include "vcedef.h"
#include "swidef.h"
#include "nccdef.h"
/*----------------------------- Definitions --------------------------------*/
/* Recognized levels of reporting by ctademo functions.
 */
#define DEMO_REPORT_MIN         0 /* Errors only */
#define DEMO_REPORT_UNEXPECTED  1 /* Errors + unexpected events */
#define DEMO_REPORT_COMMENTS    2 /* Errors + unexpected events + comments */
#define DEMO_REPORT_ALL         3 /* Errors + all events + comments */

/* Some return codes.  SUCCESS is already defined in "ctadef.h". */
#define FAILURE             (-1)
#define DISCONNECT          (-2)
#define FILEOPENERROR       (-3)
#define PARMERROR           (-4)

/* Some generally useful macros. */
#ifndef MIN
  #define MIN(a,b) (((a)<(b)) ? (a):(b))
#endif
#ifndef MAX
  #define MAX(a,b) (((a)>(b)) ? (a):(b))
#endif

#ifndef TRUE
  #define TRUE  (1)
#endif
#ifndef FALSE
  #define FALSE (0)
#endif

#ifndef ARRAY_DIM
  #define ARRAY_DIM(a) (sizeof(a)/sizeof(*(a)))
#endif

/* A wrapper for non-functional statements (typically, unreached returns)
 * that some compilers insist on and others complain about.
 * The behavior should really be determined by the compiler, not the OS.
 */
#if defined(WIN32)
  #define UNREACHED_STATEMENT( statement ) statement
#else
  #define UNREACHED_STATEMENT( statement )
#endif

/* Some support for OS-independent thread manipulation.
 */
#if defined (WIN32)
  #include <process.h>
  #define THREAD_RET_TYPE             unsigned
  #define THREAD_CALLING_CONVENTION   __stdcall
  #define THREAD_RETURN               return 0
#elif defined (UNIX)
  #include <thread.h>
  #define THREAD_RET_TYPE             void *
  #define THREAD_CALLING_CONVENTION
  #define THREAD_RETURN               return NULL
#endif


/*-------------------------- Global Constants ------------------------------*/
extern const char    *DemoVoiceEncodings[];
extern const unsigned DemoNumVoiceEncodings;


/*-------------------------- Message Numbers -------------------------------*/
#define PROMPT_ZERO      0
#define PROMPT_ONE       1
#define PROMPT_TWO       2
#define PROMPT_THREE     3
#define PROMPT_FOUR      4
#define PROMPT_FIVE      5
#define PROMPT_SIX       6
#define PROMPT_SEVEN     7
#define PROMPT_EIGHT     8
#define PROMPT_NINE      9
#define PROMPT_CALLED   10 /* "The digits dialed were..."                   */
#define PROMPT_CALLING  11 /* "The calling party's number is..."            */
#define PROMPT_WELCOME  12 /* "Thank you for calling."                      */
#define PROMPT_OPTION   13 /* "Press 1 to play, 2 to record, 3 to hangup."  */
#define PROMPT_NOTHING  14 /* "There is nothing to play."                   */
#define PROMPT_INTRODUC 15 /* "You are being called by the demo."           */
#define PROMPT_GETEXT   16 /* "Please enter the extension of the party...   */
#define PROMPT_WILLTRY  17 /* "I'll try that extension now.                 */
#define PROMPT_GOODBYE  18 /* "Goodbye.                                     */

/*-------------------------- Accessor Functions -------------------------------*/
char *DemoGetADIManager( void );
void  DemoSetADIManager( const char *mgr );
char *DemoGetCfgFile( void );
void  DemoSetCfgFile( const char *filename );

void DemoInitialize(
    CTA_SERVICE_NAME  servicelist[],
    unsigned          numservices );

void DemoOpenPort(
  unsigned          userid,        /* for ctaCreateContext                  */
  char             *contextname,   /* for ctaCreateContext                  */
  CTA_SERVICE_DESC  services[],    /* for ctaOpenServices                   */
  unsigned          numservices,   /* number of services                    */
  CTAQUEUEHD       *ctaqueuehd,    /* returned Natural Access queue handle       */
  CTAHD            *ctahd );       /* returned Natural Access context handle     */

void DemoOpenAnotherPort(
  unsigned          userid,        /* for ctaCreateContext                  */
  char             *contextname,   /* for ctaCreateContext                  */
  CTA_SERVICE_DESC  services[],    /* for ctaOpenServices                   */
  unsigned          numservices,   /* number of services                    */
  CTAQUEUEHD        ctaqueuehd,    /* Natural Access queue handle                */
  CTAHD            *ctahd );       /* returned Natural Access context handle     */

void  DemoSetup(                   /* Init and OpenPort w/ basic services   */
  unsigned          ag_board,      /* AG board number                       */
  unsigned          mvip_stream,   /* MVIP stream                           */
  unsigned          mvip_slot,     /* MVIP timeslot                         */
  CTAQUEUEHD       *ctaqueuehd,    /* returned Natural Access queue handle       */
  CTAHD            *ctahd );       /* returned Natural Access context handle     */

void  DemoCSSetup(                 /* Init and OpenPort w/ basic services   */
  unsigned          ag_board,      /* AG board number                       */
  unsigned          mvip_stream,   /* MVIP stream                           */
  unsigned          mvip_slot,     /* MVIP timeslot                         */
  CTAQUEUEHD       *ctaqueuehd,    /* returned Natural Access queue handle       */
  CTAHD            *ctahd,         /* returned Natural Access context handle     */
  char             *cntx_name );   /* Natural Access context name                */

void  DemoNCCSetup(                /* Init and OpenPort w/ basic services   */
  unsigned          ag_board,      /* AG board number                       */
  unsigned          mvip_stream,   /* MVIP stream                           */
  unsigned          mvip_slot,     /* MVIP timeslot                         */
  CTAQUEUEHD       *ctaqueuehd,    /* returned Natural Access queue handle       */
  CTAHD            *ctahd,         /* returned Natural Access context handle     */
  char             *cntx_name );   /* Natural Access context name                */


void  DemoShutdown(
  CTAHD             ctahd );       /* context handle                        */


/*-------------------------- Event Functions -------------------------------*/
void  DemoWaitForEvent( CTAHD ctahd, CTA_EVENT *eventp );
void  DemoWaitForSpecificEvent( CTAHD ctahd, DWORD desired_event,
                                CTA_EVENT *eventp );
void  DemoShowEvent( CTA_EVENT *eventp );


/*-------------------------- Protocol/Call Control Functions ---------------*/
void  DemoStartProtocol(
  CTAHD             ctahd,         /* context handle                        */
  char             *protocol,      /* ADI protocol (must be avail on board) */
  WORD             *protparmsp,    /* optional protocol-specific parms      */
  ADI_START_PARMS  *stparmsp );    /* optional parms for adiStartProtocol   */

void DemoStartNCCProtocol(
    CTAHD            ctahd,
    char            *protocol,       /* TCP to run (must be avail on board)   */
    void*            protparms,       /* protocol-specific parameters to pass  */
    void*            mgrparms,        /* manager start parameters              */
    NCC_START_PARMS *stparmsp );      /* optional parms for nccStartProtocol   */


void  DemoStopProtocol(
  CTAHD             ctahd );       /* context handle                        */

void  DemoWaitForCall(
  CTAHD             ctahd,         /* context handle                        */
  char             *calledaddr,    /* DID/DNIS information                  */
  char             *callingaddr ); /* ANI/CID information                   */

void DemoWaitForNCCCall(
  CTAHD ctahd,                     /*context handle                         */
  NCC_CALLHD *callhd,               /*call handle                            */
  char *calledaddr,                /*number to be dialed                    */
  char *callingaddr);             /*number dialing from                    */

int   DemoAnswerCall(
  CTAHD             ctahd,         /* context handle                        */
  unsigned          numrings );    /* number of rings before answering      */

int   DemoAnswerNCCCall(
  CTAHD             ctahd,         /* context handle                        */
  NCC_CALLHD        callhd,        /* call handle                           */
  unsigned          numrings,
  void*             answerparms );    /* number of rings before answering      */



void  DemoRejectCall(
  CTAHD             ctahd,         /* context handle                        */
  unsigned          method );      /* method of rejecting (ADI_REJ_xxx)     */

void  DemoRejectNCCCall(
  CTAHD             ctahd,         /* context handle                        */
  NCC_CALLHD        callhd,        /* call handle                           */
  unsigned          method,        /* method of rejecting (ADI_REJ_xxx)     */
  void*             params );      /* rejectcall parameters                 */

int   DemoPlaceCall(
  CTAHD             ctahd,         /* context handle                        */
  char             *digits,        /* dial string                           */
  ADI_PLACECALL_PARMS *placeparmsp );/* place call parameters               */

int   DemoPlaceNCCCall(
  CTAHD             ctahd,         /* context handle                        */
  char             *digits,        /* dial string                           */
  char             *callingaddr,   /* address placing the call              */
  void*             mgrparms,      /* manager-specific parameters           */
  void*             protparms,     /* protocol-specific parameters          */
  NCC_CALLHD        *callhd );      /* place call parameters               */

int   DemoHangUp(
  CTAHD             ctahd );        /* context handle                       */

int   DemoHangNCCUp(
  CTAHD             ctahd,
  NCC_CALLHD        callhd,          /* context handle                       */
  void*             hangparms,
  void*             disparms);   /* hangup parameters                     */


/*-------------------------- Voice Files -----------------------------------*/
unsigned DemoGetFileType(
  char             *extension );   /* a voice filename extension            */

void DemoDetermineFileType(
  char             *filename,      /* input name, with or without extension */
  unsigned          forcelocal,    /* if TRUE and no path in filename, then */
                                   /*  prepend "./" to fullname.            */
  char             *fullname,      /* output (expanded) filename            */
  unsigned         *filetype );    /* input/output (0=>infer from extension)*/


/*-------------------------- Play and Record -------------------------------*/
int   DemoPlayMessage(
  CTAHD             ctahd,         /* context handle                        */
  char             *filename,      /* name of voice file                    */
  unsigned          message,       /* the message number to play            */
  unsigned          encoding,      /* the encoding scheme of the file       */
  VCE_PLAY_PARMS   *playparmsp );  /* optional parms for vcePlayMessage     */

int   DemoPlayList(
  CTAHD             ctahd,         /* context handle                        */
  char             *filename,      /* name of voice file                    */
  unsigned          message[],     /* array of message numbers to play      */
  unsigned          count,         /* number of messages to play            */
  unsigned          encoding,      /* the encoding scheme of the file       */
  VCE_PLAY_PARMS   *playparmsp );  /* optional parms for vcePlayList        */

int   DemoRecordMessage(
  CTAHD             ctahd,         /* context handle                        */
  char             *filename,      /* name of voice file                    */
  unsigned          message,       /* the message number to record          */
  unsigned          encoding,      /* the encoding scheme of the file       */
  VCE_RECORD_PARMS *recparmsp );   /* optional parms for vceRecordMessage   */

int   DemoPlayText(
  CTAHD             ctahd,         /* context handle                        */
  char             *textstring,    /* name of voice file                    */
  VCE_PLAY_PARMS   *playparmsp );  /* optional parms for vceList            */

int   DemoPlayCurrentDateTime(
  CTAHD             ctahd,         /* context handle                        */
  unsigned          datetime,      /* format to speak                       */
  VCE_PLAY_PARMS   *playparmsp );  /* optional parms for vceList            */

#define DATETIME_DAY         1
#define DATETIME_DATE        2
#define DATETIME_FULL_DATE   3
#define DATETIME_DAY_DATE    4
#define DATETIME_TIME        5
#define DATETIME_DATE_TIME   6
#define DATETIME_COMPLETE    7


/*-------------------------- Delay -----------------------------------------*/
int DemoDelay(
  CTAHD             ctahd,         /* Natural Access context handle              */
  unsigned          ms );          /* millisecond delay                     */


/*-------------------------- Sleep -----------------------------------------*/
void DemoSleep( int time );


/*-------------------------- Spawn Thread ----------------------------------*/
int DemoSpawnThread  (THREAD_RET_TYPE (THREAD_CALLING_CONVENTION *func)(void *),
                    void *arg );


/*-------------------------- Digit Collection ------------------------------*/
int   DemoGetDigits(
  CTAHD             ctahd,         /* context handle                        */
  int               count,         /* number of digits to get               */
  char             *digits );      /* returned digits                       */

int   DemoPromptDigit(
  CTAHD             ctahd,         /* context handle                        */
  char             *promptfile,    /* name of voice file                    */
  unsigned          message,       /* the message number to play            */
  unsigned          encoding,      /* the encoding scheme of the file       */
  VCE_PLAY_PARMS   *playparmsp,    /* optional parms for vcePlayMessage     */
  char             *digit,         /* returned digit                        */
  int               maxtries );    /* max num of times to prompt for digit  */


/*-------------------------- Parameter Functions ---------------------------*/
int  DemoLoadParameters(
  CTAHD             ctahd,         /* context to set parameters for         */
  char             *protocol,      /* international protocol to load        */
  char             *product );     /* name of NMS product (default "adi")   */

int  DemoLoadParFile(
  CTAHD             ctahd,         /* context to set parameters for         */
  char             *filename );    /* name of the parameter file to load    */


/*-------------------------- Reporting Functions ---------------------------*/
void  DemoSetReportLevel( unsigned level );
int   DemoShouldReport( unsigned level );

void  DemoReportComment( char *s );
void  DemoReportUnexpectedEvent( CTA_EVENT *eventp );

void  DemoShowError( char *preface, DWORD errcode );

DWORD NMSSTDCALL DemoErrorHandler( CTAHD ctahd, DWORD errorcode,
                                   char *errortext, char *func );

DWORD NMSSTDCALL DemoErrorHandlerContinue( CTAHD ctahd, DWORD errorcode,
                                   char *errortext, char *func );

/*-------------------------- MVIP-95/MVIP-90 Converstion Functions ---------*/
void DemoSwiTerminusToMvip90Output(SWI_TERMINUS t, SWI_TERMINUS *r);
void DemoSwiTerminusToMvip90Input(SWI_TERMINUS t, SWI_TERMINUS *r);
DWORD DemoMvip90InputtoMvip95Bus(SWI_TERMINUS t);
DWORD DemoMvip90InputtoMvip95Stream(SWI_TERMINUS t);
DWORD DemoMvip90InputtoMvip95Timeslot(SWI_TERMINUS t);
DWORD DemoMvip90OutputtoMvip95Bus(SWI_TERMINUS t);
DWORD DemoMvip90OutputtoMvip95Stream(SWI_TERMINUS t);
DWORD DemoMvip90OutputtoMvip95Timeslot(SWI_TERMINUS t);


/*-------------------------- Get Option Function ---------------------------*/
/* Unix-style parsing of command-line options, for systems that lack it. */
#ifndef UNIX
  extern char *optarg;
  extern int   optind;
  extern int   opterr;
  int getopt( int argc, char * const *argv, const char *optstring );
#endif


#if !defined (WIN32)
  int stricmp( const char *s1, const char *s2 ); /* Case-insensitive strcmp() */
#endif


#if defined (UNIX)
  int kbhit (void);                     /*  NT style kbhit */
#elif defined (WIN32)
  #define kbhit _kbhit
#endif
void DemoSetupKbdIO( int init );
void RestoreSetupKbdIO( void );

#ifdef __cplusplus
}
#endif

