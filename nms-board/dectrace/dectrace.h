/*****************************************************************************\
 * 
 *  FILE        : dectrace.h
 *  DESCRIPTION : NMS-ISDN trace decoding utility
 *  
 *****************************************************************************
 *
 *  Copyright 1999-2002 NMS Communications.
 * 
\*****************************************************************************/

#include "ctadef.h"
#include "isdntype.h"
#include "isdndef.h"
#include "isdnparm.h"
#include "isdndl.h"
#include "decisdn.h"

#if defined (UNIX)
  #include <unistd.h>
#endif

/******************************************************************************
   miscellaneous
******************************************************************************/
  /* mask */
  #define TRACE_BUF     0x8000  /* prints the whole hex buffer */
  #define TRACE_PD      0x2000  /* decodes the protocol discriminator */
  #define TRACE_CR      0x1000  /* decodes the call reference */
  #define TRACE_MSG     0x0800  /* decodes the message type */
  #define TRACE_IE      0x0400  /* decodes the information element id */
  #define TRACE_IE_CONT 0x0200  /* decodes the IE contents */
  #define TRACE_Q921    0x0080  /* decodes the Q.921 primitives (SABME, RR...) */
  #define TRACE_DL      0x0040  /* decodes the commands/events at the DL interface (DL_EST_IN...)*/
  #define TRACE_TIME    0x0008  /* prints a time stamp before the messages */ 
  #define TRACE_PH      0x0002  /* decodes PH messages */ 
  #define TRACE_ACU     0x0001  /* decodes ACU messages */ 

  /* direction */
  #define INCOMING  0
  #define OUTGOING  1
  
  /* utilities */                                 
  #define Case(b1,b2) case b1: PrintString(b2); break;
  #define Default(b1) default: PrintString(b1); break;

  /* function prototypes */
void DecodeQ931Message( unsigned char *buffer,/* containing the ISDN message */
                        int size,             /* size of buffer */
                        int mask,             /* mask */
                        unsigned char b_num,  /* board number */
                        unsigned char n_num,  /* NAI number */
                        unsigned char g_num,  /* group number */
                        char direction,       /* INCOMING/OUTGOING message */
                        char *timestring);    /* time stamp */
void DecodeQ921Code( unsigned char *buffer,	  
					 int size,				  /* buffer size */
                     int mask,             /* mask */
                     unsigned char b_num,  /* board number */
                     unsigned char n_num,  /* NAI number */
                     unsigned char g_num,  /* group number */
                     char direction,       /* INCOMING/OUTGOING message */
                     char *timestring);     /* time stamp */
void DecodeAcuMessage(  int mask,             /* mask */
                        unsigned char b_num,  /* board number */
                        unsigned char n_num,  /* NAI number */
                        unsigned char g_num,  /* group number */
                        char direction,       /* INCOMING/OUTGOING message */
                        char code,            /* ACU message code */
                        unsigned char add,    /* address */
                        char *timestring);    /* time stamp */
void DecodeDLMessage(   int mask,             /* mask */
                        unsigned char b_num,  /* board number */
                        unsigned char n_num,  /* NAI number */
                        unsigned char g_num,  /* group number */
                        char direction,       /* INCOMING/OUTGOING message */
                        char code,            /* DL message code */
                        char *timestring);    /* time stamp */
void DecodePH( int mask,				/* trace mask */
			   unsigned char b_num,		/* board number */
               unsigned char n_num,		/* nai number */
               unsigned char g_num,     /* group number */
               char direction,		    /* INCOMING/OUTGOING message */
               char code,               /* PH message code */
               char *timestring);       /* time stamp */
 
void MyExit ( void );
int CheckString ( char *string );
char MyAtoi ( char value );
int PrepareString ( char *string, char *buffer );
void PrintUsage ( void );
void ExtractTimeStamp ( char *source, char *dest );
unsigned char ExtractNai ( char *string );
unsigned char ExtractGroup ( char *string );
int CheckCallRef ( long value, unsigned char *buff );
#ifndef UNIX
int getopt( int argc, char * const *argv, const char *optstring );
#endif

