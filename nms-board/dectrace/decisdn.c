/*****************************************************************************\
 * 
 *  FILE        : decisdn.c
 *  DESCRIPTION : NMS-ISDN trace decoding utility
 *                This file contains a set of functions to decode ISDN messages
 *                
 *****************************************************************************
 *
 * Copyright 1999-2002 NMS Communications.
 * 
\*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "decisdn.h"
#include "dectrace.h"

/* function prototypes */
void PrintString ( char *buffer );
void PrintChar ( char value );
void PrintBits ( char value, int mask );
void PrintNumber ( unsigned char value );
void DecodeProtocolDiscriminator ( unsigned char value );
void DecodeCallReference ( unsigned char *pointer );
void DecodeBC ( unsigned char *pointer ); 
void DecodeChannelId ( unsigned char *pointer );
void DecodeCalledCalling ( unsigned char *pointer );
void DecodeCalledCallingSub ( unsigned char *pointer );
void DecodeCause ( unsigned char *pointer );
void DecodeHLC ( unsigned char *pointer );
void DecodeKeypad ( unsigned char *pointer );
void DecodeLLC ( unsigned char *pointer );
void DecodeUUI ( unsigned char *pointer );
void DecodeDisplay ( unsigned char *pointer );
void DecodeProgressInd ( unsigned char *pointer );
void DecodeNSF ( unsigned char *pointer );
void DecodeCallState ( unsigned char *pointer );
void DecodeShift ( unsigned char *pointer );
void DecodeRestartInd ( unsigned char *pointer );
void DecodeChangeStatus ( unsigned char *pointer ); /* according with 235-900-342 (5ESS) */

void DecodeFacility ( unsigned char *pointer );
void DecodeNotifyInd( unsigned char *pointer );
unsigned char DecodeASN1Component( unsigned char component, unsigned char octet,
                                   unsigned char *pointer, unsigned char size );

void CheckValue (int value1, int value2);

extern int log_to_file;
extern FILE *IsdnlogF;

/* global variables */
unsigned isdn_mask = 0xffff;
unsigned global_call_ref; /* used for DecodeCallState */
unsigned codeset = 0;
unsigned codeset_locked = 0;

/******************************************************************************
                            PrintString
******************************************************************************/
void PrintString ( char *buffer )
{
  printf( "%s", buffer );
  if (log_to_file)
      fprintf (IsdnlogF, "%s", buffer);
}
/******************************************************************************
                            PrintChar
******************************************************************************/
void PrintChar ( char value )
{
    if ((value >= ' ') && (value < 0x7f))
    {
        printf( "= '%c'", value );
        if (log_to_file)
        fprintf (IsdnlogF, "= '%c'", value);
    }
}

/******************************************************************************
                            PrintBits
	mask is an int just to avoid a bunch of warnings
******************************************************************************/
void PrintBits ( char value, int mask )
{
	int i;

	PrintString("\n");
	for (i=7; i>=0; i--)
	{
		if ((mask >> i) & 0x01)
			if ((value >> i) & 0x01)
				PrintString("1");
			else
				PrintString("0");
		else 
			PrintString("-");
	}
}

/******************************************************************************
                            PrintNumber
******************************************************************************/
void PrintNumber ( unsigned char value )
{
  char buff[4];

  sprintf(buff,"%02X ", value);

  PrintString( buff );
}

/******************************************************************************
                            Decode Management Entity Identifier (TEI) for BRI
******************************************************************************/
void DecodeTEIMgt ( unsigned char *pointer, int size )
{
  PrintString( "UI" );

  /* Management Entity Identifier ? */
  if ( ( *pointer != Q921_TEI_MGT ) || ( size < 5 ) )
    return;

  PrintString(" - TEI management : ");

  /* Type */
  switch ( *( pointer + 3 ) )
	{
    Case( 0x01, "Identity Request" );
    Case( 0x02, "Identity Assigned" );
    Case( 0x03, "Identity Denied" );
    Case( 0x04, "Identity Check Request" );
    Case( 0x05, "Identity Check Response" );
    Case( 0x06, "Identity Remove" );
    Case( 0x07, "Identity Verify" );
    Default("UNKNOWN")
	}
  /* Reference indicator */
  PrintString( "\n      Ri = 0x" );
  PrintNumber( *( pointer + 1 ) );
  PrintNumber( *( pointer + 2 ) );

  /* Action indicator */
  PrintString( " Ai = " );
  PrintNumber( *( pointer + 4 ) );
}

/******************************************************************************
                            DecodeQ921Code
******************************************************************************/
void DecodeQ921Code( unsigned char *buffer,/* buffer */
                     int size,               /* buffer size */
                     int mask,             /* mask */
                     unsigned char b_num,  /* board number */
                     unsigned char n_num,  /* NAI number */
                     unsigned char g_num,  /* group number */
                     char direction,       /* INCOMING/OUTGOING message */
                     char *timestring)     /* time stamp */
{
  unsigned char q921_code = buffer[2];
  int i;
  isdn_mask = mask;

  if ( isdn_mask & TRACE_Q921 )
  {
 	PrintString("\n");
	if ( isdn_mask & TRACE_TIME )
	{
		PrintString("\n");
		PrintString(timestring);
	}

    /* frame Info */
    if ( ( q921_code & 1 ) == 0 )    
    {
        if ( isdn_mask & TRACE_BUF )
        {
            PrintString("\n   ");
            for ( i = 0; i < 4; i++)
                PrintNumber( *( buffer + i ) );
        }
    PrintString("\n  Q.921 primitive = ");
        PrintString( "INFO");
        DecodeQ931Message(  &buffer[4], 
                                    size - 4, 
                                    mask, 
                                     b_num, 
                                    n_num,
                                    g_num,
                                    direction, 
                                    timestring);
        return;
    }

    /* frame UI in broadcast mode */
    if ( ( buffer[1] == Q921_BROADCAST ) &&
         ( buffer[2] == Q921_UI        ) &&    
         ( buffer[3] != Q921_TEI_MGT   ) )
    {
        if ( isdn_mask & TRACE_BUF )
        {
            PrintString("\n   ");
            for ( i = 0; i < 3; i++)
                PrintNumber( *( buffer + i ) );
        }
        PrintString("\n  Q.921 primitive = ");
        PrintString( "UI");
        DecodeQ931Message(  &buffer[3], 
                                    size - 3, 
                                    mask, 
                                     b_num, 
                                    n_num,
                                    g_num,
                                    direction, 
                                    timestring);
        return;
    }

    /* other frames */

    if ( isdn_mask & TRACE_BUF )
    {
        PrintString("\n   ");
        for ( i = 0; i < size; i++)
            PrintNumber( *( buffer + i ) );
    }
    PrintString("\n  Q.921 primitive = ");

    switch( q921_code )
    {
      Case(0x01,"RR")
      Case(0x7f,"SABME")
      Case(0x73,"UA")
      Case(0x53,"DISC")
      Case(0x05,"RNR")
      Case(0x1f,"DM")
      Case(0x09,"REJ")
      case Q921_UI: 
            DecodeTEIMgt( &buffer[3], size - 3 );
      break;
	  Default("not decoded")
	}
    PrintString("   board ");
    PrintNumber(b_num);
    PrintString(" nai ");
    PrintNumber(n_num);
    if( g_num != 0xFF )
    {
        PrintString(" group ");
        PrintNumber(g_num);
    }
    switch ( direction )
    {
      Case(INCOMING," <--");
      Case(OUTGOING," -->");
    }
  }
}

/******************************************************************************
                            DecodeAcuMessage
******************************************************************************/
void DecodeAcuMessage(  int mask,             /* mask */
                        unsigned char b_num,  /* board number */
                        unsigned char n_num,  /* NAI number */
                        unsigned char g_num,  /* group number */
                        char direction,       /* INCOMING/OUTGOING message */
                        char code,            /* ACU message code */
                        unsigned char add,    /* connection ID */  /* DLV 23MAY01 */
                        char *timestring)     /* time stamp */
{
    isdn_mask = mask;

	PrintString("\n");
	if ( isdn_mask & TRACE_TIME )
	{
		PrintString("\n");
		PrintString(timestring);
	}
    PrintString("\n  ACU message, primitive  code = ");

    switch( code & 0xFF )
    {
      Case(ACU_CONN_RQ,"ACU_CONN_RQ")
      Case(ACU_CONN_IN,"ACU_CONN_IN")
      Case(ACU_CONN_RS,"ACU_CONN_RS")
      Case(ACU_CONN_CO,"ACU_CONN_CO")
      Case(ACU_CLEAR_RQ,"ACU_CLEAR_RQ")
      Case(ACU_CLEAR_IN,"ACU_CLEAR_IN")
      Case(ACU_CLEAR_RS,"ACU_CLEAR_RS")
      Case(ACU_CLEAR_CO,"ACU_CLEAR_CO")
      Case(ACU_ALERT_RQ,"ACU_ALERT_RQ")
      Case(ACU_ALERT_IN,"ACU_ALERT_IN")
      Case(ACU_PROGRESS_RQ,"ACU_PROGRESS_RQ")
      Case(ACU_PROGRESS_IN,"ACU_PROGRESS_IN")
      Case(ACU_INFO_RQ,"ACU_INFO_RQ")
      Case(ACU_INFO_CO,"ACU_INFO_CO")
      Case(ACU_INIT_RQ,"ACU_INIT_RQ")
      Case(ACU_INIT_CO,"ACU_INIT_CO")
      Case(ACU_SETPARM_RQ,"ACU_SETPARM_RQ")
      Case(ACU_SETPARM_CO,"ACU_SETPARM_CO")
      Case(ACU_USER_INFO_RQ,"ACU_USER_INFO_RQ")
      Case(ACU_USER_INFO_IN,"ACU_USER_INFO_IN")
      Case(ACU_SUSPEND_RQ,"ACU_SUSPEND_RQ")
      Case(ACU_SUSPEND_CO,"ACU_SUSPEND_CO")
      Case(ACU_RESUME_RQ,"ACU_RESUME_RQ")
      Case(ACU_RESUME_CO,"ACU_RESUME_CO")
      Case(ACU_TEST_RQ,"ACU_TEST_RQ")
      Case(ACU_TEST_CO,"ACU_TEST_CO")
      Case(ACU_DIGIT_RQ,"ACU_DIGIT_RQ")
      Case(ACU_DIGIT_IN,"ACU_DIGIT_IN")
      Case(ACU_DIGIT_CO,"ACU_DIGIT_CO")
      Case(ACU_FACILITY_RQ,"ACU_FACILITY_RQ")
      Case(ACU_FACILITY_IN,"ACU_FACILITY_IN")
      Case(ACU_SET_MODE_RQ,"ACU_SET_MODE_RQ")
      Case(ACU_SET_MODE_CO,"ACU_SET_MODE_CO")
      Case(ACU_RS_MODE_RQ,"ACU_RS_MODE_RQ")
      Case(ACU_RS_MODE_CO,"ACU_RS_MODE_CO")
      Case(ACU_INFORMATION_RQ,"ACU_INFORMATION_RQ")
      Case(ACU_INFORMATION_IN,"ACU_INFORMATION_IN")
      Case(ACU_SETUP_REPORT_IN,"ACU_SETUP_REPORT_IN")
      Case(ACU_CALL_PROC_RQ,"ACU_CALL_PROC_RQ")
      Case(ACU_CALL_PROC_IN,"ACU_CALL_PROC_IN")
      Case(ACU_NOTIFY_RQ,"ACU_NOTIFY_RQ")
      Case(ACU_NOTIFY_IN,"ACU_NOTIFY_IN")
      Case(ACU_SETUP_ACK_IN,"ACU_SETUP_ACK_IN")
      Case(ACU_D_CHANNEL_STATUS_RQ,"ACU_D_CHANNEL_STATUS_RQ")
      Case(ACU_D_CHANNEL_STATUS_IN,"ACU_D_CHANNEL_STATUS_IN")
      Case(ACU_SERVICE_RQ,"ACU_SERVICE_RQ")
      Case(ACU_SERVICE_IN,"ACU_SERVICE_IN")
      Case(ACU_SERVICE_CO,"ACU_SERVICE_CO")
      Case(ACU_RESTART_IN,"ACU_RESTART_IN")
      Case(ACU_RESTART_CO,"ACU_RESTART_CO")
      Case(ACU_ERR_IN,"ACU_ERR_IN")
      Case(ACU_TRANSFER_RQ,"ACU_TRANSFER_RQ")
      Case(ACU_TRANSFER_CO,"ACU_TRANSFER_CO")
      Case(ACU_CALLID_IN,"ACU_CALLID_IN")  
      Case(ACU_CALLID_RQ,"ACU_CALLID_RQ")  
      Case(ACU_HOLD_IN,"ACU_HOLD_IN") 
      Case(ACU_HOLD_RS,"ACU_HOLD_RS")  
      Case(ACU_HOLD_CO,"ACU_HOLD_CO") 
      Case(ACU_HOLD_RQ,"ACU_HOLD_RQ")
      Case(ACU_RETRIEVE_IN,"ACU_RETRIEVE_IN")  
      Case(ACU_RETRIEVE_RS,"ACU_RETRIEVE_RS")  
      Case(ACU_RETRIEVE_RQ,"ACU_RETRIEVE_RQ")  
      Case(ACU_RETRIEVE_CO,"ACU_RETRIEVE_CO")  
  
      Default("UNKNOWN")
    }
    PrintString("   board ");
    PrintNumber(b_num);
    PrintString(" nai ");
    PrintNumber(n_num);
    if( g_num != 0xFF )
    {
        PrintString(" group ");
        PrintNumber(g_num);
    }
    PrintString(" id ");
    PrintNumber(add);
    switch ( direction )
    {
      Case(INCOMING," <--");
      Case(OUTGOING," -->");
    }
}

/******************************************************************************
                            DecodeDLMessage
******************************************************************************/
void DecodeDLMessage(   int mask,             /* mask */
                        unsigned char b_num,  /* board number */
                        unsigned char n_num,  /* NAI number */
                        unsigned char g_num,  /* group number */
                        char direction,       /* INCOMING/OUTGOING message */
                        char code,            /* DL message code */
                        char *timestring)     /* time stamp */
{
  isdn_mask = mask;

  if ( isdn_mask & TRACE_DL )
  {
    PrintString("\n");
	if ( isdn_mask & TRACE_TIME )
	{
		PrintString("\n");
	    PrintString(timestring);
	}
    switch( code )
    {
      Case(DL_EST_RQ,"\n  data link establish request")
      Case(DL_EST_IN,"\n  data link establish indication: data link is UP")
      Case(DL_EST_CO,"\n  data link establish confirmation: data link is UP")
      Case(DL_REL_RQ,"\n  data link release request")
      Case(DL_REL_IN,"\n  data link release indication: data link is DOWN")
      Case(DL_REL_CO,"\n   data link release confirmation: data link is DOWN")
      Case(DL_COMMIT_RQ,"\n   data link information commitment request")
      Case(DL_COMMIT_CO,"\n   data link information commitment confirmation")
      Default("DL UNKNOWN")
	}
    PrintString("   board ");
    PrintNumber(b_num);
    PrintString(" nai ");
    PrintNumber(n_num);
    if( g_num != 0xFF )
    {
        PrintString(" group ");
        PrintNumber(g_num);
    }
    switch ( direction )
    {
      Case(INCOMING," <--");
      Case(OUTGOING," -->");
    }
  }
}

/******************************************************************************
                            DecodeQ931Message
******************************************************************************/
void DecodeQ931Message( unsigned char *buffer,/* containing the ISDN message */
                        int size,             /* size of buffer */
                        int mask,             /* mask */
                        unsigned char b_num,  /* board number */
                        unsigned char n_num,  /* NAI number */
                        unsigned char g_num,  /* group number */
                        char direction,       /* INCOMING/OUTGOING message */
                        char *timestring)     /* time stamp */
{
  unsigned char *local_p;
  int count = 0;
  int i;
  unsigned char prot_disc;

  isdn_mask = mask;
  if ( isdn_mask & (TRACE_BUF | TRACE_MSG |TRACE_PD | TRACE_CR | TRACE_IE) )
  {
    PrintString("\n");
  }
 
  if ( isdn_mask & TRACE_BUF )
  {
    for ( i = 0; i < size; i++)
	{
		if ( i%16 == 0 )  PrintString("\n   ");
        PrintNumber( *( buffer + i ) );
		if ( i%4 == 3) PrintString("  ");
	}
  }

  local_p = buffer;

  /* the first byte is the protocol discriminator */
  if ( isdn_mask & TRACE_PD )
    DecodeProtocolDiscriminator ( *local_p ); 
  /* save the value of the protocol discriminator */
  prot_disc =  *local_p;

  count++;
  if ( count >= size )
  {
    PrintString("\nERROR: mandatory data missing");
    return;
  }

  /* then we have the call reference */
  local_p = buffer + count;
  if ( isdn_mask & TRACE_CR )
    DecodeCallReference ( local_p ); 
  count+= *local_p + 1;  
  if ( count >= size )
  {
    PrintString("\nERROR: mandatory data missing");
    return;
  }

  /* then the message type */
  local_p = buffer + count;
  if ( isdn_mask & TRACE_MSG )
  {
	PrintBits(*local_p, 0xff);
    PrintString("  message type = ");
    if ( prot_disc == PD_Q931_CC )
    {
      switch ( *local_p )
      {
        Case(MSG_ALERTING,"ALERTING")
        Case(MSG_CALL_PROC,"CALL_PROCEEDING")
        Case(MSG_CONNECT,"CONNECT")
        Case(MSG_CONNECT_ACK,"CONNECT ACKNOWLEDGE")
        Case(MSG_PROGRESS,"PROGRESS")
        Case(MSG_SETUP,"SETUP")
        Case(MSG_SETUP_ACK,"SETUP ACKNOWLEDGE")
        Case(MSG_DISCONNECT,"DISCONNECT")
        Case(MSG_RELEASE,"RELEASE")
        Case(MSG_RELEASE_COMP,"RELEASE COMPLETE")
        Case(MSG_STATUS,"STATUS")
        Case(MSG_STATUS_ENQ,"STATUS ENQUIRY")
        Case(MSG_USER_INFO,"USER INFORMATION")
        Case(MSG_RESTART,"RESTART")
        Case(MSG_RESTART_ACK,"RESTART ACKNOWLEDGE")
        Case(MSG_INFO,"INFORMATION")
        Case(MSG_FACILITY,"FACILITY")
        Case(MSG_NOTIFY,"NOTIFY")
        Case(MSG_HOLD,"HOLD")
        Case(MSG_HOLD_ACK,"HOLD ACKNOWLEDGE")
        Case(MSG_HOLD_REJ,"HOLD REJECT")
        Case(MSG_RETRIEVE,"RETRIEVE")
        Case(MSG_RETRIEVE_ACK,"RETRIEVE ACKNOWLEDGE")
        Case(MSG_RETRIEVE_REJ,"RETRIEVE REJECT")
        Case(MSG_REGISTER,"REGISTER")
		Case(MSG_SEGMENT,"SEGMENT")
		Case(MSG_SUSPEND,"SUSPEND")
		Case(MSG_SUSPEND_ACK,"SUSPEND ACKNOWLEDGE")
		Case(MSG_SUSPEND_REJ,"SUSPEND REJECT")
		Case(MSG_RESUME,"RESUME")
		Case(MSG_RESUME_ACK,"RESUME ACKNOWLEDGE")
		Case(MSG_RESUME_REJ,"RESUME REJECT")
		Case(MSG_CONGESTION_CTRL,"CONGESTION CONTROL")
		Case(MSG_ESCAPE,"ESCAPE FOR NATIONAL USE")
        Default("RESERVED")
      }
    }
    else
    if ((prot_disc == PD_Q931_MNT_NI) ||
        (prot_disc == PD_Q931_MNT))
    {
      switch ( *local_p )
      {
        Case(MSG_SERVICE,"SERVICE")
        Case(MSG_SERVICE_ACK,"SERVICE ACKNOWLEDGE")
        Default("UNKNOWN")
      }
    }

    PrintString("   board ");
    PrintNumber(b_num);
    PrintString(" nai ");
    PrintNumber(n_num);
    if( g_num != 0xFF )
    {
        PrintString(" group ");
        PrintNumber(g_num);
    }
    switch ( direction )
    {
      Case(INCOMING," <--");
      Case(OUTGOING," -->");
    }
  }

  /* here we have the information elements */
  if ( isdn_mask & TRACE_IE )
  {
	codeset = 0;
    while ( ++count < size )
    { 
      local_p = buffer + count;
      PrintBits(*local_p, 0xff);
	  if ( codeset == 0 )
	  {
          PrintString("    IE ");
		  switch ( *local_p )
		  {
		  case IE_BC:
			  PrintString("bearer capability");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeBC( local_p ); 
			  break;
		  case IE_CAUSE:
			  PrintString("cause");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeCause( local_p ); 
			  break;
		  case IE_CHANNEL_ID:
			  PrintString("channel identification");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeChannelId( local_p ); 
			  break;
		  case IE_CALLING:
			  PrintString("calling party number");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeCalledCalling( local_p ); 
			  break;
		  case IE_CALLING_SUB:
			  PrintString("calling party subaddress");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeCalledCallingSub( local_p ); 
			  break;
		  case IE_CALLED:
			  PrintString("called party number");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeCalledCalling( local_p ); 
			  break;
		  case IE_CALLED_SUB:
			  PrintString("called party subaddress");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeCalledCallingSub( local_p ); 
			  break;
		  case IE_CONNECTED:
		  case IE_CONNECTED_QSIG:
			  PrintString("connected party number");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeCalledCalling( local_p ); 
			  break;
		  case IE_HLC:
			  PrintString("high layer compatibility");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeHLC( local_p ); 
			  break;
          case IE_KEYPAD:
              PrintString("keypad facility");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeKeypad( local_p ); 
			  break;
		  case IE_LLC:
			  PrintString("low layer compatibility");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeLLC( local_p ); 
			  break;
		  case IE_UUI:
			  PrintString("user-user information");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeUUI( local_p ); 
			  break;
		  case IE_PROGRESS_IND:
			  PrintString("progress indicator");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeProgressInd( local_p ); 
			  break;
		  case IE_SENDING_COMPL:
			  PrintString("sending complete");
			  break;
		  case IE_CALL_STATE:
			  PrintString("call state");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeCallState( local_p ); 
			  break;
		  case IE_NSF:
			  PrintString("network-specific facilities");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeNSF( local_p ); 
			  break;
          case IE_ORIG_CALLED:
			  PrintString("original called party number");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeCalledCalling( local_p ); 
			  break;
		  case IE_REDIRECTING:
			  PrintString("redirecting number");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeCalledCalling( local_p ); 
			  break;
		  case IE_REDIRECTION:
			  PrintString("redirection number");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeCalledCalling( local_p ); 
			  break;
		  case IE_DISPLAY:
			  PrintString("display");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeDisplay( local_p ); 
			  break;
		  case IE_REST_IND:
			  PrintString("restart indicator");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeRestartInd( local_p ); 
			  break;
		  case IE_CHANGE_STATUS:
			  PrintString("change status");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeChangeStatus( local_p ); 
			  break;
          case IE_FACILITY:
			  PrintString("facility");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeFacility( local_p ); 
              break;
          case IE_NOTIFY_IND:
              PrintString("notification indicator");
			  if ( isdn_mask & TRACE_IE_CONT )
				  DecodeNotifyInd( local_p ); 
              break;
		  /* the following IE are not decoded by this utility */
	      Case(IE_DATE_TIME,"date/time: NOT DECODED")
	      Case(IE_MORE_DATA,"more data: NOT DECODED")
	      Case(IE_SEGMENTED,"segmented message: NOT DECODED")
	      Case(IE_CALL_ID,"call identity: NOT DECODED")
		  Case(IE_INFO_RQ,"information request: NOT DECODED")
		  Case(IE_SIGNAL,"signal: NOT DECODED")
		  Case(IE_SWITCHHOOK,"switchhook: NOT DECODED")
		  Case(IE_FEATURE_ACK,"feature activation: NOT DECODED")
		  Case(IE_FEATURE_IND,"feature indication: NOT DECODED")
		  Case(IE_SERVICE_PROF,"service profile identification: NOT DECODED")
		  Case(IE_ENDPOINT_ID,"endpoint identifier: NOT DECODED")
		  Case(IE_INFO_RATE,"information rate: NOT DECODED")
		  Case(IE_END_TO_END_DELAY,"end to end transit delay: NOT DECODED")
		  Case(IE_TDSI,"transit delay selection and indication: NOT DECODED")
		  Case(IE_PKT_BIN_PAR,"packet layer binary parameters: NOT DECODED")
		  Case(IE_PKT_WIN_SIZE,"packet layer window size: NOT DECODED")
		  Case(IE_PKT_SIZE,"packet size: NOT DECODED")
		  Case(IE_MIN_THR_CLASS,"minimum throughput class: NOT DECODED")
		  Case(IE_TRANSIT_SEL,"transit network selection: NOT DECODED")
		  Case(IE_ESCAPE,"escape for extension: NOT DECODED")
		  default:
			  switch ( *local_p & 0xf0 )
			  {
			  case IE_SHIFT:
				  PrintString("shift");
				  if ( isdn_mask & TRACE_IE_CONT )
					  DecodeShift( local_p ); 
				  break;
			  case IE_CONG_LEVEL:
				  PrintString("congestion level: NOT DECODED");
				  break;
			  case IE_REPEAT_IND:
				  PrintString("repeat indicator: NOT DECODED");
				  break;
			  default:
            	  PrintString("0x");
    	          PrintNumber( *local_p );
				  PrintString("RESERVED");
        		  if ( !( *local_p & 0x80 ) ) /* variable length IE */
		        	{
			          PrintBits(*(local_p + 1), 0xff);
        			  PrintString("        length = 0x");
    	              PrintNumber( *(local_p + 1) );
                      for (i=2; i < *(local_p + 1) + 2; i++)
                      {
    			        PrintBits(*(local_p + i), 0xff);
    			        PrintString("        octet ");
    	                PrintNumber((unsigned char) (i + 1));
    			        PrintString("= 0x");
    	                PrintNumber( *(local_p + i) );
                      }
        			}
				  break;
			  } 
			  break;
		  }
	  }
	  else /* codeset != 0 */
	  {
    	  PrintString("    IE 0x");
    	  PrintNumber( *local_p );
		  switch (codeset)
		  {
		  case 5:
			  PrintString("with codeset for national use, codeset ");
			  break;
		  case 6:
			  PrintString("with network specific codeset, codeset ");
			  break;
		  case 7:
			  PrintString("with user specific codeset, codeset ");
			  break;
		  default:
			  PrintString("with RESERVED codeset, codeset ");
			  break;
		  }
          PrintNumber( (unsigned char) codeset);

		  if (!codeset_locked)
			  codeset = 0;

		  if ( !( *local_p & 0x80 ) ) /* variable length IE */
			{
			  PrintBits(*(local_p + 1), 0xff);
			  PrintString("        length = 0x");
    	      PrintNumber( *(local_p + 1) );
              for (i=2; i < *(local_p + 1) + 2; i++)
              {
    			  PrintBits(*(local_p + i), 0xff);
    			  PrintString("        octet ");
    	          PrintNumber((unsigned char) (i + 1));
    			  PrintString("= 0x");
    	          PrintNumber( *(local_p + i) );
              }
			}
	  }
/* increment the counter for multiple octets IEs */
      if (( *local_p & 0x80 ) == 0)
	    count += *(local_p + 1 ) + 1;
    }
  }

}

/******************************************************************************
                            DecodeProtocolDiscriminator
******************************************************************************/
void DecodeProtocolDiscriminator ( unsigned char value )
{
  PrintString("\n          protocol discriminator = ");
  switch ( value )
  {
    Case(PD_Q931_MNT,"Q.931 Maintenance")
    Case(PD_Q931_MNT_NI,"National ISDN Maintenance")
    Case(PD_Q931_CC,"Q.931 Call Control")
    Default("UNKNOWN")
  }
}

/******************************************************************************
                            DecodeCallReference
	pointer points to the call reference length
******************************************************************************/
void DecodeCallReference ( unsigned char *pointer )
{
  unsigned char tmp=0; /* used to check if call ref is 0 or not */
  int i, flag = 0;
 
  PrintString("\n          call reference = ");

  if ( pointer[0] == 0 ) /* Dummy Call Reference */
  {
      PrintString("<dummy>");
  }
  else 
  {
      tmp  = pointer[1] & ~0x80; /* get rid of the first bit of the call reference (flag) */
      flag = (pointer[1] & 0x80) ? 1 : 0;

      PrintNumber( tmp ) ;
  
      for (i = 2; i <= pointer[0]; i++)
      {
         PrintNumber( pointer[i] );
         tmp |= pointer[i]; 
      }
  }
  
  global_call_ref = (tmp != 0) ? 0 : 1;
  
  if (flag) PrintString("    flag = 1");
  else      PrintString("    flag = 0");
}

/******************************************************************************
                            DecodeBC
    decodes the bearer capability IE
******************************************************************************/
void DecodeBC ( unsigned char *pointer )
{
  unsigned char size, count;
  unsigned char * local_p;
  char ext_flag;

  size = *( pointer + 1 ) + 1; /* 1 is for the size itself */
  count = 1;
  
  /* length */
  local_p = pointer + count;
  PrintBits(*local_p, 0xff);
  PrintString("        length = 0x");
  PrintNumber( (unsigned char ) *local_p );

  if ( ++count > size )
    return;
  /* octet 3 */
  local_p = pointer + count;
  PrintString("\n              octet 3");
  PrintBits(*local_p, 0x80);
  PrintString("        extension bit");
  CheckValue (*local_p & 0x80, 0x80);
  PrintBits(*local_p, 0x60);
  PrintString("        coding standard = ");
  switch ( *local_p & 0x60 )
  {
    Case(CS_CCITT,"CCITT")
    Case(CS_RES,"reserved for other standards")
    Case(CS_NTL,"national")
    Case(CS_DEF,"defined for the network on the other side")
  }
  PrintBits(*local_p, 0x1f);
  PrintString("        information transfer capability = ");
  switch ( *local_p & 0x1f )
  {
    Case(ITC_SPEECH,"speech")
    Case(ITC_UDI,"unrestricted digital info")
    Case(ITC_RDI,"restricted digital info")
    Case(ITC_31K,"3.1 Khz audio")
    Case(ITC_7K,"7 Khz audio")
    Case(ITC_VIDEO,"video")
    Default("RESERVED")
  }

  if ( ++count > size )
    return;
  /* octet 4 */
  local_p = pointer + count;
  PrintString("\n              octet 4");
  PrintBits(*local_p, 0x80);
  PrintString("        extension bit");
  ext_flag = *local_p & 0x80;
  PrintBits(*local_p, 0x60);
  PrintString("        transfer mode = ");
  switch ( *local_p & 0x60 )
  {
    Case(TM_CIRCUIT,"circuit mode")
    Case(TM_PKT,"packet mode")
    Default("RESERVED")
  }
  PrintBits(*local_p, 0x1f);
  PrintString("        information transfer rate = ");
  switch ( *local_p & 0x1f )
  {
    Case(ITR_PKT,"code for packet mode")
    Case(ITR_64K,"64 Kbit/s")
    Case(ITR_2X64K,"2*64 Kbit/s")
    Case(ITR_384K,"384 Kbit/s")
    Case(ITR_1536K,"1536 Kbit/s")
    Case(ITR_1920K,"1920 Kbit/s")
    Default("RESERVED")
  }

  if ( !ext_flag )
  {
    if ( ++count > size )
      return;
    /* octet 4a */
    local_p = pointer + count;
    ext_flag = *local_p & 0x80;
    PrintBits(*local_p, 0xff);
    PrintString("              octet 4a NOT DECODED");
    ext_flag = *local_p & 0x80;
  } 

  if ( !ext_flag )
  {
    if ( ++count > size )
      return;
    /* octet 4b */
    local_p = pointer + count;
    PrintBits(*local_p, 0xff);
    PrintString("              octet 4b NOT DECODED");
  } 

  if ( ++count > size )
    return;
  /* octet 5 */
  local_p = pointer + count;
  PrintString("\n              octet 5");
  PrintBits(*local_p, 0x80);
  PrintString("        extension bit");
  ext_flag = *local_p & 0x80;
  PrintBits(*local_p, 0x60);
  PrintString("        layer 1 id = ");
  switch ( *local_p & 0x60)
  {
    Case(0x20,"layer 1 id")
    Default("RESERVED")
  }
  PrintBits(*local_p, 0x1f);
  PrintString("        user info layer 1 = ");
  switch ( *local_p & 0x1f )
  {
    Case(UIL1_CCITT,"CCITT V.110/X.30")
    Case(UIL1_MULAW,"mu law")
    Case(UIL1_ALAW,"A law")
    Case(UIL1_G721,"G.721")
    Case(UIL1_G722,"G.722")
    Case(UIL1_H261,"H.261")
    Case(UIL1_NOCCITT,"non CCITT")
    Case(UIL1_V120,"CCITT V.120")
    Case(UIL1_X31,"CCITT X.31")
    Default("RESERVED")
  }

  if ( !ext_flag )
  {
    if ( ++count > size )
      return;
    /* octet 5a */
    local_p = pointer + count;
    PrintBits(*local_p, 0xff);
    PrintString("              octet 5a NOT DECODED");
    ext_flag = *local_p & 0x80;
  } 

  if ( !ext_flag )
  {
    if ( ++count > size )
      return;
    /* octet 5b */
    local_p = pointer + count;
    PrintBits(*local_p, 0xff);
    PrintString("              octet 5b NOT DECODED");
    ext_flag = *local_p & 0x80;
  } 

  if ( !ext_flag )
  {
    if ( ++count > size )
      return;
    /* octet 5c */
    local_p = pointer + count;
    PrintBits(*local_p, 0xff);
    PrintString("              octet 5c NOT DECODED");
    ext_flag = *local_p & 0x80;
  } 

  if ( !ext_flag )
  {
    if ( ++count > size )
      return;
    /* octet 5d */
    local_p = pointer + count;
    PrintBits(*local_p, 0xff);
    PrintString("              octet 5d NOT DECODED");
  } 

  if ( ++count > size )
    return;
  /* octet 6 */
  local_p = pointer + count;
  PrintBits(*local_p, 0xff);
  PrintString("              octet 6 NOT DECODED");

  if ( ++count > size )
    return;
  /* octet 7 */
  local_p = pointer + count;
  PrintBits(*local_p, 0xff);
  PrintString("              octet 7 NOT DECODED");
}

/******************************************************************************
                            DecodeChannelId
    decodes the channel identification IE
******************************************************************************/
void DecodeChannelId ( unsigned char *pointer )
{
  unsigned char size, count;
  unsigned char * local_p;
  char ext_flag, i_id, i_type;

  size = *( pointer + 1 ) + 1; /* 1 is for the size itself */
  count = 1;

  /* length */
  local_p = pointer + count;
  PrintBits(*local_p, 0xff);
  PrintString("        length = 0x");
  PrintNumber( (unsigned char ) *local_p );

  if ( ++count > size )
    return;
  /* octet 3 */
  local_p = pointer + count;
  PrintString("\n              octet 3");
  PrintBits(*local_p, 0x80);
  PrintString("        extension bit");
  CheckValue (*local_p & 0x80, 0x80);
  PrintBits(*local_p, 0x40);
  PrintString("        interface identifier = ");
  switch ( *local_p & 0x40 )
  {
    Case(IID_IMPL,"implicitly identified")
    Case(IID_EXPL,"explicitly identified")
  }
  i_id = ( *local_p & 0x40 );
  PrintBits(*local_p, 0x20);
  PrintString("        interface type = ");
  switch ( *local_p & 0x20 )
  {
    Case(IT_BASIC,"basic interface")
    Case(IT_OTHER,"PRI")
  }
  i_type = ( *local_p & 0x20 );
  PrintBits(*local_p, 0x10);
  PrintString("        spare bits = ");
  switch ( *local_p & 0x10 )
  {
    Case(0,"spare bits")
    Default("RESERVED")
  } 
  PrintBits(*local_p, 0x08);
  PrintString("        pref/excl = ");
  switch ( *local_p & 0x08 )
  {
    Case(PE_PREF,"preferred")
    Case(PE_EXCL,"exclusive")
  }
  PrintBits(*local_p, 0x04);
  PrintString("        D-channel ind = ");
  switch ( *local_p & 0x04 )
  {
    Case(DCI_NOT,"not")
    Case(DCI_YES,"yes")
  }
  PrintBits(*local_p, 0x03);
  PrintString("        info channel selection = ");
  switch ( *local_p & 0x03 )
  {
    Case(ICS_NO,"no channel")
    Case(ICS_B1,"B1 channel")
    Case(ICS_B2,"B2 channel")
    Case(ICS_ANY,"any channel")
  }

  if (i_id != IID_IMPL )
  {
    /* octet 3.1 (this octet may be repeated) */
	do
	{
      if ( ++count > size )
        return;
      local_p = pointer + count;
      ext_flag = *local_p & 0x80;
      PrintString("\n              octet 3.1");
      PrintBits(*local_p, 0x80);
      PrintString("        extension bit");
      PrintBits(*local_p, 0x7f);
      PrintString("        interface id = ");
      PrintNumber( (unsigned char ) (*local_p & 0x7f) );
	}while ( !ext_flag );
  }

  if (i_type != IT_BASIC )
  {
	  if ( ++count > size )
		  return;
	  /* octet 3.2 */
	  local_p = pointer + count;
	  PrintString("\n              octet 3.2");
      PrintBits(*local_p, 0x80);
      PrintString("        extension bit");
	  CheckValue (*local_p & 0x80, 0x80);
      PrintBits(*local_p, 0x60);
	  PrintString("        coding standard = ");
	  switch ( *local_p & 0x60 )
	  {
		  Case(CS_CCITT,"CCITT")
		  Case(CS_RES,"reserved for other standards")
		  Case(CS_NTL,"national")
		  Case(CS_DEF,"defined for the network on the other side")
	  }
      PrintBits(*local_p, 0x10);
	  PrintString("        number/map = ");
	  switch ( *local_p & 0x10 )
	  {
		  Case(NM_NUMB,"number")
			  Case(NM_MAP,"map")
	  }
      PrintBits(*local_p, 0x0f);
	  PrintString("        channel type = ");
	  switch ( *local_p & 0x0f )
	  {
		  Case(CHT_B,"B channel")
		  Case(CHT_H0,"H0 channel")
		  Case(CHT_H11,"H11 channel")
		  Case(CHT_H12,"H12 channel")
		  Default("RESERVED")
	  }
	  
     /* octet 3.3 (this octet may be repeated) */
      do
	  {
		if ( ++count > size )
			return;
		local_p = pointer + count;
        ext_flag = *local_p & 0x80;
        PrintString("\n              octet 3.3");
		PrintBits(*local_p, 0x80);
		PrintString("        extension bit");
        PrintBits(*local_p, 0x7f);
	    PrintString("        channel number = 0x");
	    PrintNumber( (unsigned char ) (*local_p & 0x7f) );
	  }while ( !ext_flag );
  }

}

/******************************************************************************
                            DecodeKeypad
    decodes the Keypad Facility IEs
******************************************************************************/
void DecodeKeypad ( unsigned char *pointer )
{
  unsigned char size, count;
  unsigned char * local_p;
  char number[2];

  size = *( pointer + 1 ) + 1; /* 1 is for the size itself */
  count = 1;

  /* length */
  local_p = pointer + count;
  PrintBits(*local_p, 0xff);
  PrintString("        length = 0x");
  PrintNumber( (unsigned char ) *local_p );
  
  if ( ++count > size )
    return;

  /* octets 3 etc */
  PrintString("\n                number = "); /* no bits printed */
  while (count <= size)
  {
    local_p = pointer + count++;
    number[0] = *local_p;
    number[1] = 0;
    PrintString(number);
  }
}

/******************************************************************************
                            DecodeCalledCalling
    decodes the Called, Calling and Redirecting Party Number IEs
******************************************************************************/
void DecodeCalledCalling ( unsigned char *pointer )
{
  unsigned char size, count;
  unsigned char * local_p;
  char ext_flag;
  char number[2];

  size = *( pointer + 1 ) + 1; /* 1 is for the size itself */
  count = 1;

  /* length */
  local_p = pointer + count;
  PrintBits(*local_p, 0xff);
  PrintString("        length = 0x");
  PrintNumber( (unsigned char ) *local_p );
  
  if ( ++count > size )
    return;
  /* octet 3 */
  local_p = pointer + count;
  ext_flag = *local_p & 0x80;
  PrintString("\n              octet 3");
  PrintBits(*local_p, 0x80);
  PrintString("        extension bit");
  PrintBits(*local_p, 0x70);
  PrintString("        type of number = ");
  switch ( *local_p & 0x70  )
  {
    Case(TON_UNKNOWN,"unknown")
    Case(TON_INTL,"international")
    Case(TON_NTL,"national")
    Case(TON_NET,"network specific")
    Case(TON_SUB,"subscriber")
    Case(TON_ABB,"abbreviated")
    Case(TON_RES,"reserved for extension")
    Default("RESERVED")
  }
  PrintBits(*local_p, 0x0f);
  PrintString("        numbering plan = ");
  switch ( *local_p & 0x0f )
  {
    Case(NP_UNKNOWN,"unknown")
    Case(NP_ISDN,"ISDN")
    Case(NP_DATA,"data")
    Case(NP_TELEX,"telex")
    Case(NP_NTL,"national")
    Case(NP_PRIVATE,"private")
    Case(NP_RES,"reserved for extension")
    Default("RESERVED")
  }

  if ( !ext_flag )  /* the ext_flag should be 0 only for the 
	                   calling party number */
  {
    if ( ++count > size )
      return;
    /* octet 3a */
    local_p = pointer + count;
    ext_flag = *local_p & 0x80;
    PrintString("\n              octet 3a");
    PrintBits(*local_p, 0x80);
    PrintString("        extension bit");
    PrintBits(*local_p, 0x60);
    PrintString("        presentation ind = ");
    switch ( *local_p & 0x60 )
    {
      Case(PI_ALLOWED,"allowed")
      Case(PI_RESTRICTED,"restricted")
      Case(PI_NOT_AVAIL,"number not available")
      Default("RESERVED")
    } 
    PrintBits(*local_p, 0x1c);
    PrintString("        spare bits = ");
    switch ( *local_p & 0x1c )
    {
      Case(0,"spare bits")
      Default("RESERVED")
    } 
    PrintBits(*local_p, 0x03);
    PrintString("        screening ind = ");
    switch ( *local_p & 0x03  )
    {
      Case(SI_UP_NS,"user provided, not screened")
      Case(SI_UP_VP,"user provided, verified and passed")
      Case(SI_UP_VF,"user provided, verified and failed")
      Case(SI_NP,"network provided")
    } 
  
  }
  
  if ( !ext_flag )  /* the ext_flag should be 0 only for the 
	                   redirecting party number or
                       original called party number */
  {
    if ( ++count > size )
      return;
    /* octet 3b */
    local_p = pointer + count;
    ext_flag = *local_p & 0x80;
    PrintString("\n              octet 3b");
    PrintBits(*local_p, 0x80);
    PrintString("        extension bit");
    PrintBits(*local_p, 0x70);
    PrintString("        spare bits = ");
    switch ( *local_p & 0x70 )
    {
      Case(0,"spare bits")
      Default("RESERVED")
    } 
	PrintBits(*local_p, 0x0f);
    PrintString("        reason for redirection = ");
    switch ( *local_p & 0x0f )
    {
      Case(RR_UNKNOWN,"unknown")
      Case(RR_CFB,"call forwarding busy")
      Case(RR_CFNR,"call forwarding no reply")
      Case(RR_CFNETBUSY,"call forwarding network busy")
      Case(RR_TRANSFER,"call transfer")
      Case(RR_PICKUP,"call pickup")
      Case(RR_DTEOO,"called DTE out of order")
      Case(RR_CFBYDTE,"call forwarding by the called DTE")
      Case(RR_COVOFFNET,"call coverage off network")
      Case(RR_SENDALL,"send all calls")
      Case(RR_STATIONHUNT,"station hunt")
      Case(RR_CFU,"call forwarding unconditional")
      Default("RESERVED")
    } 
  }
  if ( !ext_flag )  /* the ext_flag should be 0 only for the 
	                   original called party number */
  {
    if ( ++count > size )
      return;
    /* octet 3c */
    local_p = pointer + count;
    ext_flag = *local_p & 0x80;
    PrintString("\n              octet 3c");
    PrintBits(*local_p, 0x80);
    PrintString("        extension bit");
    CheckValue (*local_p & 0x80, 0x80);
    PrintBits(*local_p, 0x40);
    PrintString("        CFNR indicator = ");
    switch ( *local_p & 0x40 )
    {
      Case(0x40,"TRUE")
      Default("FALSE")
    } 
    PrintBits(*local_p, 0x38);
    PrintString("        spare bits = ");
    switch ( *local_p & 0x38 )
    {
      Case(0,"spare bits")
      Default("RESERVED")
    } 
	PrintBits(*local_p, 0x07);
    PrintString("        redirection counter = ");
    PrintNumber( (unsigned char ) (*local_p & 0x07) );
  }

  /* octets 4 etc */
  PrintString("\n              octet 4 etc");
  PrintString("\n                number = "); /* no bits printed */
  while ( ++count <= size)
  {
    local_p = pointer + count;
    number[0] = *local_p;
    number[1] = 0;
    PrintString(number);
  }
}

/******************************************************************************
                            DecodeCalledCallingSub
    decodes the Called/ing Party Subaddress IEs
******************************************************************************/
void DecodeCalledCallingSub ( unsigned char *pointer )
{
  unsigned char size, count;
  unsigned char * local_p;
  char sub_type;
  int first_byte = 1;
  char number[2];

  size = *( pointer + 1 ) + 1; /* 1 is for the size itself */
  count = 1;

  /* length */
  local_p = pointer + count;
  PrintBits(*local_p, 0xff);
  PrintString("        length = 0x");
  PrintNumber( (unsigned char ) *local_p );
  
  if ( ++count > size )
    return;
  /* octet 3 */
  local_p = pointer + count;
  PrintString("\n              octet 3");
  PrintBits(*local_p, 0x80);
  PrintString("        extension bit");
  CheckValue (*local_p & 0x80, 0x80);
  PrintBits(*local_p, 0x70);
  PrintString("        type of subaddress = ");
  switch ( *local_p & 0x70  )
  {
    Case(TOS_NSAP,"NSAP")
    Case(TOS_USER,"user specified")
    Default("RESERVED")
  }
  sub_type = *local_p & 0x70;
  PrintBits(*local_p, 0x08);
  PrintString("        odd/even indicator = ");
  switch ( *local_p & 0x08 )
  {
    Case(OE_EVEN,"even number of address signals")
    Case(OE_ODD,"odd number of address signals")
  }
  PrintBits(*local_p, 0x07);
  PrintString("        spare bits = ");
  switch ( *local_p & 0x07 )
  {
    Case(0,"spare bits")
    Default("RESERVED")
  } 

  /* octets 4 etc */
  PrintString("\n              octet 4 etc");
  PrintString("\n                number = ");
  while ( ++count <= size)
  {
    local_p = pointer + count;
    if ( sub_type == TOS_NSAP )
      if ( first_byte )
      {
        first_byte = 0;
        if ( *local_p != 0x50 )
        {
          PrintString("CODE UNKNOWN");
          return;
        } 
      }
      else
      {
        number[0] = *local_p; /* IA5 characters */
        number[1] = 0;
        PrintString(number);
      }
    else
    {
      number[0] = *local_p - 0x30; /* BCD */
      number[1] = 0;
      PrintString(number);
    }
  }
}

/******************************************************************************
                            DecodeCause
    decodes the Cause IE
******************************************************************************/
void DecodeCause ( unsigned char *pointer )
{
  unsigned char size, count;
  unsigned char * local_p;
  char ext_flag;

  size = *( pointer + 1 ) + 1; /* 1 is for the size itself */
  count = 1;

  /* length */
  local_p = pointer + count;
  PrintBits(*local_p, 0xff);
  PrintString("        length = 0x");
  PrintNumber( (unsigned char ) *local_p );
  
  if ( ++count > size )
    return;
  /* octet 3 */
  local_p = pointer + count;
  ext_flag = *local_p & 0x80;
  PrintString("\n              octet 3");
  PrintBits(*local_p, 0x80);
  PrintString("        extension bit");
  PrintBits(*local_p, 0x60);
  PrintString("        coding standard = ");
  switch ( *local_p & 0x60 )
  {
    Case(CS_CCITT,"CCITT")
    Case(CS_RES,"reserved for other standards")
    Case(CS_NTL,"national")
    Case(CS_DEF,"defined for the network on the other side")
  }
  PrintBits(*local_p, 0x10);
  PrintString("        spare bits = ");
  switch ( *local_p & 0x10 )
  {
    Case(0,"spare bits")
    Default("RESERVED")
  } 
  PrintBits(*local_p, 0x0f);
  PrintString("        location = ");
  switch ( *local_p & 0x0f )
  {
    Case(LOC_USR,"user")
    Case(LOC_PRI_LOC,"private network serving local user")
    Case(LOC_PUB_LOC,"public network serving local user")
    Case(LOC_TN,"transit network")
    Case(LOC_PUB_REM,"public network serving remote user")
    Case(LOC_PRI_REM,"private network serving remote user")
    Case(LOC_INTL,"international network")
    Case(LOC_NBIP,"network beyond interworking point")
    Default("RESERVED")
  }

  if ( !ext_flag ) 
  {
    if ( ++count > size )
      return;
    /* octet 3a */
    local_p = pointer + count;
    PrintString("\n              octet 3a");
    PrintBits(*local_p, 0x80);
    PrintString("        extension bit");
    CheckValue (*local_p & 0x80, 0x80);
    PrintBits(*local_p, 0x7f);
    PrintString("        reccomendation = ");
    switch ( *local_p & 0x7f )
    {
      Case(REC_Q931,"Q.931")
      Case(REC_X21,"X.21")
      Case(REC_X25,"X.25")
      Default("RESERVED")
    }
  }

  if ( ++count > size )
    return;
  /* octet 4 */
  local_p = pointer + count;
  PrintString("\n              octet 4");
  PrintBits(*local_p, 0x80);
  PrintString("        extension bit");
  CheckValue (*local_p & 0x80, 0x80);
  PrintBits(*local_p, 0x7f);
  PrintString("        cause = ");
  switch ( *local_p & 0x7f )
  {
    /* these ones should be the most likely to happen */
    Case(CAU_NORMAL_CC,"normal call clearing")
    Case(CAU_BUSY,"user busy")
    Case(CAU_NO_USER_RES,"no user responding")
    Case(CAU_NO_ANSW,"no answer from user")
    Case(CAU_REJ,"call rejected")
    Case(CAU_NORMAL_UNSPEC,"normal, unspecified")
    Case(CAU_NO_CIRC_CHAN,"no circuit/channel available")
    Case(CAU_CONG,"switching equipment congestion")
    Case(CAU_NOT_AVAIL,"requested circuit/channel not available")
    Case(CAU_INV_CRV,"invalid call reference value")
    Case(CAU_MAND_IE_MISSING,"mandatory information element is missing")
    Case(CAU_INV_IE_CONTENTS,"invalid information element contents")
    Case(CAU_MSG_NOT_COMP_CS,"message not compatible with call state")
    Case(CAU_RECOVERY,"recovery on timer expiry")
    Case(CAU_PROTO_ERR,"protocol error")
    /* these ones should be more rare */
    Case(CAU_UNALL,"unallocated number")
    Case(CAU_NOR_STN,"no route to transit network")
    Case(CAU_NOR_D,"no route to destination")
    Case(CAU_CH_UNACC,"channel unnacceptable")
    Case(CAU_AWARD,"call awarded and delivered in an est. ch.")
    Case(CAU_NUM_CHANGED,"number changed")
    Case(CAU_NON_SEL,"non-selected user clearing")
    Case(CAU_DEST_OOF,"destination out of order")
    Case(CAU_INV,"invalid number format")
    Case(CAU_FAC_REJ,"facility rejected")
    Case(CAU_RES_TO_SE,"response to STATUS ENQUIRY")
    Case(CAU_NET_OOF,"network out of order")
    Case(CAU_TEMP_FAIL,"temporary failure")
    Case(CAU_ACC_INF_DISC,"access information discarded")
    Case(CAU_RES_UNAVAIL,"resource unavailable")
    Case(CAU_QOF_UNAVAIL,"quality of service unavailable")
    Case(CAU_FAC_NOT_SUB,"requested facility not subscribed")
    Case(CAU_BC_NOT_AUT,"bearer capability not authorized")
    Case(CAU_BC_NOT_AVAIL,"bearer capability not presently available")
    Case(CAU_SERV_NOTAVAIL,"service or option not available")
    Case(CAU_BC_NOT_IMP,"bearer capability not implemented")
    Case(CAU_CHT_NOT_IMP,"channel type not implemented")
    Case(CAU_FAC_NOT_IMP,"requested facility non implemented")
    Case(CAU_RESTR_BC,"only restricted digital bearer cap. is available")
    Case(CAU_SERV_NOT_IMP,"service or option not implemented")
    Case(CAU_CH_NOT_EX,"identified channel does not exist")
    Case(CAU_CALL_ID_NOT_EX,"call identity does not exist")
    Case(CAU_CALL_ID_IN_USE,"call identity in use")
    Case(CAU_NO_CALL_SUSP,"no call suspended")
    Case(CAU_CALL_CLEARED,"the call has been cleared")
    Case(CAU_INCOMP_DEST,"incompatible destination")
    Case(CAU_INV_TN,"invalid transit network selection")
    Case(CAU_INV_MSG,"invalid message")
    Case(CAU_MSGT_NOT_EX,"message type not existent/implemented")
    Case(CAU_MSG_NOT_COMP,"msg not compatible with call state or msg type not existent")
    Case(CAU_IE_NOT_EX,"information element not existent/implemented")
    Case(CAU_INTERW,"interworking")
  /* the following cause values are defined in the 4ESS, 5ESS and DMS specs
  	 the ETS and NI2 specs do not add any value to the Q.931 specs */
	Case(CAU_PREEMPTION,"preemption (5ESS)")
	Case(CAU_PREEMPTION_CRR,"preemption, circuit reserved for reuse (5ESS)")
	Case(CAU_PREC_CALL_BLK,"precedence call blocked (5ESS)")
	Case(CAU_BC_INCOMP_SERV,"bearer capability incompatible with service request (5ESS)")
	Case(CAU_OUT_CALLS_BARRED,"outgoing calls barred (4ESS,5ESS)")
	Case(CAU_SERV_VIOLATED,"service operation violated (5ESS)")
	Case(CAU_IN_CALLS_BARRED,"incoming calls barred (DMS,4ESS,5ESS)")
	Case(CAU_DEST_ADD_MISSING,"destination address missing and direct call not subscribed (DMS)")
    Default("RESERVED")
  }

  /* octet 5 ... */
  if ( count >= size )
    return;
  PrintString("\n              octet 5");
  PrintString("\n                diagnostic(s) = ");
  while ( ++count <= size)
  {
    local_p = pointer + count;
    PrintNumber( *local_p);
  }
}

/******************************************************************************
                            DecodeHLC
    decodes the High Layer Compatibility IE
******************************************************************************/
void DecodeHLC ( unsigned char *pointer )
{
  unsigned char size, count;
  unsigned char * local_p;
  char ext_flag;

  size = *( pointer + 1 ) + 1; /* 1 is for the size itself */
  count = 1;

  /* length */
  local_p = pointer + count;
  PrintBits(*local_p, 0xff);
  PrintString("        length = 0x");
  PrintNumber( (unsigned char ) *local_p );
  
  if ( ++count > size )
    return;
  /* octet 3 */
  local_p = pointer + count;
  PrintString("\n              octet 3");
  PrintBits(*local_p, 0x80);
  PrintString("        extension bit");
  CheckValue (*local_p & 0x80, 0x80);
  PrintBits(*local_p, 0x60);
  PrintString("        coding standard = ");
  switch ( *local_p & 0x60 )
  {
    Case(CS_CCITT,"CCITT")
    Case(CS_RES,"reserved for other standards")
    Case(CS_NTL,"national")
    Case(CS_DEF,"defined for the network on the other side")
  }
  PrintBits(*local_p, 0x1c);
  PrintString("        interpretation = ");
  switch ( *local_p & 0x1c )
  {
    Case(INT_FIRST,"first HLC to be used in the call");
    Default("RESERVED")
  }
  PrintBits(*local_p, 0x03);
  PrintString("        presentation method = ");
  switch ( *local_p & 0x03 )
  {
    Case(PM_HLPP,"high layer protocol profile");
    Default("RESERVED")
  }
  
  if ( ++count > size )
    return;
  /* octet 4 */
  local_p = pointer + count;
  ext_flag = *local_p & 0x80;
  PrintString("\n              octet 4");
  PrintBits(*local_p, 0x80);
  PrintString("        extension bit");
  PrintBits(*local_p, 0x7f);
  PrintString("        high layer characteristics id = ");
  switch ( *local_p & 0x7f )
  {
    Case(HLC_TEL,"telephony")
    Case(HLC_FAX4,"fax group 4")
    Case(HLC_DAPG4,"doc. app. prof. group 4 ")
    Case(HLC_DAPMM,"doc. app. prof. mixed-mode")
    Case(HLC_DAPPF,"doc. app. prof. processable form")
    Case(HLC_TELETEXT,"teletext")
    Case(HLC_DAPV,"doc. app. prof. videotext")
    Case(HLC_TELEX,"telex")
    Case(HLC_MHS,"message handling system")
    Case(HLC_OSI,"OSI application")
    Case(HLC_RES_MAINT,"reserved for maintenance")
    Case(HLC_RES_MNG,"reserved for management")
    Default("RESERVED")
  }

  if ( !ext_flag ) 
  {
    if ( ++count > size )
      return;
    /* octet 4a */
    local_p = pointer + count;
    PrintString("\n                      octet 4a NOT DECODED");
  }
}

/******************************************************************************
                            DecodeLLC
    decodes the Low Layer Compatibility IE
******************************************************************************/
void DecodeLLC ( unsigned char *pointer )
{
  unsigned char size, count;
  unsigned char * local_p;
  char ext_flag;

  size = *( pointer + 1 ) + 1; /* 1 is for the size itself */
  count = 1;

  /* length */
  local_p = pointer + count;
  PrintBits(*local_p, 0xff);
  PrintString("        length = 0x");
  PrintNumber( (unsigned char ) *local_p );
  
  if ( ++count > size )
    return;
  /* octet 3 */
  local_p = pointer + count;
  ext_flag = *local_p & 0x80;
  PrintString("\n              octet 3");
  PrintBits(*local_p, 0x80);
  PrintString("        extension bit");
  PrintBits(*local_p, 0x60);
  PrintString("        coding standard = ");
  switch ( *local_p & 0x60 )
  {
    Case(CS_CCITT,"CCITT")
    Case(CS_RES,"reserved for other standards")
    Case(CS_NTL,"national")
    Case(CS_DEF,"defined for the network on the other side")
  }
  PrintBits(*local_p, 0x1f);
  PrintString("        information transfer capability = ");
  switch ( *local_p & 0x1f )
  {
    Case(ITC_SPEECH,"speech")
    Case(ITC_UDI,"unrestricted digital info")
    Case(ITC_RDI,"restricted digital info")
    Case(ITC_31K,"3.1 Khz audio")
    Case(ITC_7K,"7 Khz audio")
    Case(ITC_VIDEO,"video")
    Default("RESERVED")
  }

  if ( !ext_flag ) 
  {
    if ( ++count > size )
      return;
    /* octet 3a */
    local_p = pointer + count;
    PrintString("\n              octet 3a");
    PrintBits(*local_p, 0x80);
    PrintString("        extension bit");
    CheckValue (*local_p & 0x80, 0x80);
    PrintBits(*local_p, 0x40);
    PrintString("        negotiation indicator = ");
    switch ( *local_p & 0x40 )
    {
      Case(NI_NEG_NOT_POSS,"out-band negotiation not possible")
      Case(NI_NEG_POSS,"out-band negotiation possible")
      Default("RESERVED")
    }
    PrintBits(*local_p, 0x3f);
    PrintString("        spare bits = ");
    switch ( *local_p & 0x3f )
	{
      Case(0,"spare bits")
      Default("RESERVED")
	} 
  }

  if ( ++count > size )
    return;
  /* octet 4 */
  local_p = pointer + count;
  ext_flag = *local_p & 0x80;
  PrintString("\n              octet 4");
  PrintBits(*local_p, 0x80);
  PrintString("        extension bit");
  PrintBits(*local_p, 0x60);
  PrintString("        transfer mode = ");
  switch ( *local_p & 0x60 )
  {
    Case(TM_CIRCUIT,"circuit mode")
    Case(TM_PKT,"packet mode")
    Default("RESERVED")
  }
  PrintBits(*local_p, 0x1f);
  PrintString("        information transfer rate = ");
  switch ( *local_p & 0x1f )
  {
    Case(ITR_PKT,"code for packet mode")
    Case(ITR_64K,"64 Kbit/s")
    Case(ITR_2X64K,"2*64 Kbit/s")
    Case(ITR_384K,"384 Kbit/s")
    Case(ITR_1536K,"1536 Kbit/s")
    Case(ITR_1920K,"1920 Kbit/s")
    Default("RESERVED")
  }

  if ( !ext_flag )
  {
    if ( ++count > size )
      return;
    /* octet 4a */
    local_p = pointer + count;
    ext_flag = *local_p & 0x80;
    PrintBits(*local_p, 0xff);
    PrintString("              octet 4a NOT DECODED");
  } 

  if ( !ext_flag )
  {
    if ( ++count > size )
      return;
    /* octet 4b */
    local_p = pointer + count;
    ext_flag = *local_p & 0x80;
    PrintBits(*local_p, 0xff);
    PrintString("              octet 4b NOT DECODED");
  } 

  if ( ++count > size )
    return;
  /* octet 5 */
  local_p = pointer + count;
  ext_flag = *local_p & 0x80;
  PrintBits(*local_p, 0xff);
  PrintString("              octet 5 NOT DECODED");

  if ( !ext_flag )
  {
    if ( ++count > size )
      return;
    /* octet 5a */
    local_p = pointer + count;
    ext_flag = *local_p & 0x80;
    PrintBits(*local_p, 0xff);
    PrintString("              octet 5a NOT DECODED");
  } 

  if ( !ext_flag )
  {
    if ( ++count > size )
      return;
    /* octet 5b */
    local_p = pointer + count;
    ext_flag = *local_p & 0x80;
    PrintBits(*local_p, 0xff);
    PrintString("              octet 5b NOT DECODED");
  } 

  if ( !ext_flag )
  {
    if ( ++count > size )
      return;
    /* octet 5c */
    local_p = pointer + count;
    ext_flag = *local_p & 0x80;
    PrintBits(*local_p, 0xff);
    PrintString("              octet 5c NOT DECODED");
  } 

  if ( !ext_flag )
  {
    if ( ++count > size )
      return;
    /* octet 5d */
    local_p = pointer + count;
    ext_flag = *local_p & 0x80;
    PrintBits(*local_p, 0xff);
    PrintString("              octet 5d NOT DECODED");
  } 

  if ( ++count > size )
    return;
  /* octet 6 */
  local_p = pointer + count;
  ext_flag = *local_p & 0x80;
  PrintBits(*local_p, 0xff);
  PrintString("              octet 6 NOT DECODED");

  if ( !ext_flag )
  {
    if ( ++count > size )
      return;
    /* octet 6a */
    local_p = pointer + count;
    ext_flag = *local_p & 0x80;
    PrintBits(*local_p, 0xff);
    PrintString("              octet 6a NOT DECODED");
  } 

  if ( ++count > size )
    return;
  /* octet 7 */
  local_p = pointer + count;
  ext_flag = *local_p & 0x80;
  PrintBits(*local_p, 0xff);
  PrintString("              octet 7 NOT DECODED");

  if ( !ext_flag )
  {
    if ( ++count > size )
      return;
    /* octet 7a */
    local_p = pointer + count;
    ext_flag = *local_p & 0x80;
    PrintBits(*local_p, 0xff);
    PrintString("              octet 7a NOT DECODED");
  } 


}

/******************************************************************************
                            DecodeUUI
    decodes the User-User  IE
******************************************************************************/
void DecodeUUI ( unsigned char *pointer )
{
  unsigned char size, count;
  unsigned char * local_p;
  char number[2];

  size = *( pointer + 1 ) + 1; /* 1 is for the size itself */
  count = 1;

  /* length */
  local_p = pointer + count;
  PrintBits(*local_p, 0xff);
  PrintString("        length = 0x");
  PrintNumber( (unsigned char ) *local_p );
  
  if ( ++count > size )
    return;
  /* octet 3 */
  local_p = pointer + count;
  PrintString("\n              octet 3");
  PrintBits(*local_p, 0xff);
  PrintString("        protocol discriminator = ");
  switch ( *local_p )
  {
    Case(PD_UUS,"user-user specific")
    Case(PD_OSI,"OSI")
    Case(PD_X244,"X.244")
    Case(PD_SM,"system management")
    Case(PD_IA5,"IA5")
    Case(PD_V120,"V.120")
    Case(PD_Q931,"Q.931 user-network message")
    Default("unknown or RESERVED")
  }

  if ( count >= size )
    return;  
  /* octets 4 etc */
  PrintString("\n              octet 4 etc");
  PrintString("\n                user information = ");
  while ( ++count <= size)
  {
    local_p = pointer + count;
    number[0] = *local_p;
    number[1] = 0;
    PrintString(number);
  }
}

/******************************************************************************
                            DecodeDisplay
    decodes the Display  IE
******************************************************************************/
void DecodeDisplay ( unsigned char *pointer )
{
  unsigned char size, count;
  unsigned char * local_p;
  char number[2];

  size = *( pointer + 1 ) + 1; /* 1 is for the size itself */
  count = 1;

  /* length */
  local_p = pointer + count;
  PrintBits(*local_p, 0xff);
  PrintString("        length = 0x");
  PrintNumber( (unsigned char ) *local_p );
  
  if ( ++count > size )
    return;
  /* octet 3 etc */
  PrintString("\n              octet 3 etc");
  PrintString("\n                display information = ");
  while ( count <= size)
  {
    local_p = pointer + count++;
    if (*local_p & 0x80)
    {
        PrintString(" tag=");
        PrintNumber( (unsigned char ) *local_p );
    }
    else
    {
        number[0] = *local_p;
        number[1] = 0;
        PrintString(number);
    }
  }
}

/******************************************************************************
                            DecodeProgressInd
    decodes the Progress Indicator  IE
******************************************************************************/
void DecodeProgressInd ( unsigned char *pointer )
{
  unsigned char size, count;
  unsigned char * local_p;

  size = *( pointer + 1 ) + 1; /* 1 is for the size itself */
  count = 1;
  
  /* length */
  local_p = pointer + count;
  PrintBits(*local_p, 0xff);
  PrintString("        length = 0x");
  PrintNumber( (unsigned char ) *local_p );

  if ( ++count > size )
    return;
  /* octet 3 */
  local_p = pointer + count;
  PrintString("\n              octet 3");
  PrintBits(*local_p, 0x80);
  PrintString("        extension bit");
  CheckValue (*local_p & 0x80, 0x80);
  PrintBits(*local_p, 0x60);
  PrintString("        coding standard = ");
  switch ( *local_p & 0x60 )
  {
    Case(CS_CCITT,"CCITT")
    Case(CS_RES,"reserved for other standards")
    Case(CS_NTL,"national")
    Case(CS_DEF,"defined for the network on the other side")
  }
  PrintBits(*local_p, 0x10);
  PrintString("        spare bits = ");
  switch ( *local_p & 0x10 )
  {
    Case(0,"spare bits")
    Default("RESERVED")
  } 
  PrintBits(*local_p, 0x0f);
  PrintString("        location = ");
  switch ( *local_p & 0x0f )
  {
    Case(LOC_USR,"user")
    Case(LOC_PRI_LOC,"private network serving local user")
    Case(LOC_PUB_LOC,"public network serving local user")
    Case(LOC_TN,"transit network")
    Case(LOC_PUB_REM,"public network serving remote user")
    Case(LOC_PRI_REM,"private network serving remote user")
    Case(LOC_INTL,"international network")
    Case(LOC_NBIP,"network beyond interworking point")
    Default("RESERVED")
  }

  if ( ++count > size )
    return;
  /* octet 4 */
  local_p = pointer + count;
  PrintString("\n              octet 4");
  PrintBits(*local_p, 0x80);
  PrintString("        extension bit");
  CheckValue (*local_p & 0x80, 0x80);
  PrintBits(*local_p, 0x7f);
  PrintString("        progress description = ");
  switch ( *local_p & 0x7f )
  {
    Case(PRD_NOT_ISDN,"call is not end-to-end ISDN")
    Case(PRD_DEST_NOT_ISDN,"destination address is non-ISDN")
    Case(PRD_ORIG_NOT_ISDN,"origination address is non-ISDN")
    Case(PRD_RET_ISDN,"call has returned to ISDN")
    Case(PRD_IN_BAND,"in-band info is now available")
    Default("RESERVED")
  }
}

/******************************************************************************
                            DecodeNSF
    decodes the Network Specific Facilities IE
******************************************************************************/
void DecodeNSF ( unsigned char *pointer )
{
  unsigned char size, count, count2, net_id_len;
  unsigned char * local_p;
  char number[2];
  
  size = *( pointer + 1 ) + 1; /* 1 is for the size itself */
  count = 1;
  
  /* length */
  local_p = pointer + count;
  PrintBits(*local_p, 0xff);
  PrintString("        length = 0x");
  PrintNumber( (unsigned char ) *local_p );

  if ( ++count > size )
    return;
  /* octet 3 */
  local_p = pointer + count;
  PrintString("\n              octet 3");
  PrintBits(*local_p, 0xff);
  PrintString("        length of network id = ");
  PrintNumber( *local_p );
  net_id_len = *local_p;

  if ( *local_p > 0 )
  {
    if ( ++count > size )
      return;
    /* octet 3.1*/
    local_p = pointer + count;
    PrintString("\n              octet 3.1");
    PrintBits(*local_p, 0x80);
    PrintString("        extension bit");
    CheckValue (*local_p & 0x80, 0x80);
    PrintBits(*local_p, 0x70);
    PrintString("        type of network identification = ");
    switch ( *local_p & 0x70 )
    {
      Case(TOI_USER,"user specified")
      Case(TOI_NTL,"national")
      Case(TOI_INTL,"international")
      Default("RESERVED")  
    }
    PrintBits(*local_p, 0x0f);
    PrintString("        network identification plan= ");
    switch ( *local_p & 0x0f )
    {
      Case(NIP_UNKNOWN,"unknown")
      Case(NIP_CIC,"carrier identification code")
      Case(NIP_DNIC,"data network identification code")
      Default("RESERVED")  
    }

    /* octet 3.2 etc */
    PrintString("\n              octet 3.2 etc");
    PrintString("\n               network identification = ");
    count2 = 1;
    while ( ++count2 <= net_id_len)
    {
      if ( ++count > size )
        return;
      local_p = pointer + count;
      number[0] = *local_p;
      number[1] = 0;
      PrintString(number);
    }
  }

  /* octets 4 etc */
  PrintString("\n              octet 4 etc");
  PrintBits(*local_p, 0xff);
  PrintString("        nsf specification = ");
  while ( ++count <= size)
  {
    local_p = pointer + count;
    PrintNumber(*local_p);
  }
}

/******************************************************************************
                            DecodeCallState
    decodes the Call State IE
******************************************************************************/
void DecodeCallState ( unsigned char *pointer )
{
  unsigned char size, count;
  unsigned char * local_p;

  size = *( pointer + 1 ) + 1; /* 1 is for the size itself */
  count = 1;
  
  /* length */
  local_p = pointer + count;
  PrintBits(*local_p, 0xff);
  PrintString("        length = 0x");
  PrintNumber( (unsigned char ) *local_p );

  if ( ++count > size )
    return;
  /* octet 3 */
  local_p = pointer + count;
  PrintString("\n              octet 3");
  PrintBits(*local_p, 0xc0);
  PrintString("        coding standard = ");
  /* NOTE this time coding standard is in bits 8 and 7 */
  switch ( (char)((*local_p & 0xc0)) >> 1 )
  {
    Case(CS_CCITT,"CCITT")
    Case(CS_RES,"reserved for other standards")
    Case(CS_NTL,"national")
    Case(CS_DEF,"defined for the network on the other side")
  }
  PrintBits(*local_p, 0x3f);
  if ( global_call_ref )
  {
    PrintString("        global interface state value = ");
    switch ( *local_p & 0x3f )
	{
      Case(GLOB_REST0,"null")
      Case(GLOB_REST1,"restart request")
      Case(GLOB_REST2,"restart")
      Default("RESERVED")
	}
  }
  else
  {
    PrintString("        call state value = ");
    switch ( *local_p & 0x3f )
	{
      Case(CST_NULL,"null")
      Case(CST_CALL_INI,"call initiated")
      Case(CST_OVL_SND,"overlap sending")
      Case(CST_OUT_PROC,"outgoing call proceeding")
      Case(CST_CALL_DEL,"call delivered")
      Case(CST_CALL_PRES,"call present")
      Case(CST_CALL_RCV,"call received")
      Case(CST_CONN_RQ,"connect request")
      Case(CST_IN_CALL_PROC,"incoming call proceeding")
      Case(CST_ACTIVE,"active")
      Case(CST_DISC_RQ,"disconnect request")
      Case(CST_DISC_IN,"disconnect indication")
      Case(CST_SUSP_RQ,"suspend request")
      Case(CST_RES_RQ,"resume request")
      Case(CST_REL_RQ,"release request")
      Case(CST_CALL_ABORT,"call abort")
      Case(CST_OVL_RCV,"overlap receiving")
      Default("RESERVED")
	}
  }
}

/******************************************************************************
                            DecodeShift
    decodes the Shift IE
******************************************************************************/
void DecodeShift ( unsigned char *pointer )
{
  PrintBits(*pointer, 0xf0);
  PrintString("        info element id = shift");
  PrintBits(*pointer, 0x08);
  PrintString("        locking shift = ");
  if ( *pointer & 0x08 )
  {
    PrintString("no");
    codeset_locked = 0;
  }
  else
  {
    PrintString("yes");
    codeset_locked = 1;
  }
  PrintBits(*pointer, 0x07);
  PrintString("        new codeset = ");
  PrintNumber( (unsigned char) (*pointer & 0x07) );
  codeset = *pointer & 0x07;
}

/******************************************************************************
                            DecodeRestartInd
    decodes the Restart Indicator IE
******************************************************************************/
void DecodeRestartInd ( unsigned char *pointer )
{
  unsigned char size, count;
  unsigned char * local_p;

  size = *( pointer + 1 ) + 1; /* 1 is for the size itself */
  count = 1;
  
  /* length */
  local_p = pointer + count;
  PrintBits(*local_p, 0xff);
  PrintString("        length = 0x");
  PrintNumber( (unsigned char ) *local_p );

  if ( ++count > size )
    return;
  /* octet 3 */
  local_p = pointer + count;
  PrintString("\n              octet 3");
  PrintBits(*local_p, 0x80);
  PrintString("        extension bit");
  CheckValue (*local_p & 0x80, 0x80);
  PrintBits(*local_p, 0x78);
  PrintString("        spare bits = ");
  switch ( *local_p & 0x78 )
  {
    Case(0,"spare bits")
    Default("RESERVED")
  } 
  PrintBits(*local_p, 0x07);
  PrintString("        class = ");
  switch ( *local_p & 0x07 )
  {
    Case(CLA_CHAN,"indicated channels (in channel id IE )")
    Case(CLA_INT,"single interface")
    Case(CLA_ALL,"all interfaces")
    Default("RESERVED")  
  }
}

/******************************************************************************
                            DecodeChangeStatus
    decodes the change status IE according with 235-900-342 (5ESS) 
******************************************************************************/
void DecodeChangeStatus ( unsigned char *pointer )
{
  unsigned char size, count;
  unsigned char * local_p;

  size = *( pointer + 1 ) + 1; /* 1 is for the size itself */
  count = 1;
  
  /* length */
  local_p = pointer + count;
  PrintBits(*local_p, 0xff);
  PrintString("        length = 0x");
  PrintNumber( (unsigned char ) *local_p );

  if ( ++count > size )
    return;
  /* octet 3 */
  local_p = pointer + count;
  PrintString("\n              octet 3");
  PrintBits(*local_p, 0x80);
  PrintString("        extension bit");
  CheckValue (*local_p & 0x80, 0x80);
  PrintBits(*local_p, 0x40);
  PrintString("        preference = ");
  switch ( *local_p & 0x40 )
  {
    Case(PREF_INTERFACE,"all interface")
    Case(PREF_CHANNEL,"channel")
  }
  PrintBits(*local_p, 0x38);
  PrintString("        spare bits = ");
  switch ( *local_p & 0x38 )
  {
    Case(0,"spare bits")
    Default("RESERVED")
  } 
  PrintBits(*local_p, 0x07);
  PrintString("        new status = ");
  switch ( *local_p & 0x07 )
  {
    Case(NEW_STAT_IS,"in service")
    Case(NEW_STAT_MNT,"maintenance")
    Case(NEW_STAT_OOS,"out of service")
    Default("RESERVED")  
  }

}

/******************************************************************************
                            DecodeFacility
    decodes the facility IE
        decodes the invoke component
******************************************************************************/
void DecodeFacility ( unsigned char *pointer )
{
  unsigned char size, count;
  char * local_p;
  char comp_flag = '0';
  unsigned char comp_length = 0;

  size = *( pointer + 1 ) + 1; /* 1 is for the size itself */
  count = 1;
  
  /* length */
  local_p = pointer + count;
  PrintBits(*local_p, 0xff);
  PrintString("        length = 0x");
  PrintNumber( (unsigned char ) *local_p ); 

  if ( ++count > size )
    return;
  /* octet 3 */
  local_p = pointer + count;
  PrintString("\n              octet 3");

  PrintBits(*local_p, 0x80);
  PrintString("        extension bit");
  CheckValue (*local_p & 0x80, 0x80);
  PrintBits(*local_p, 0x60);
  PrintString("        spare bits = ");
  switch ( *local_p & 0x60 )
  {
    Case(0,"spare bits")
    Default("RESERVED")
  } 
  PrintBits(*local_p, 0x1f);
  PrintString("        protocol profile = ");
  switch ( *local_p & 0x1f )
  {
    Case(PP_SSA,"supplementary service applications")
    Case(PP_NE,"networking extensions")
    Default("RESERVED")
  } 

  if ( (*local_p & 0x80) == 0 )
  {
      if ( ++count > size ) return;
      PrintString("\n              octet 3a");
      PrintBits(*local_p, 0xFF);
      PrintString("        OCTET IS NOT DECODED");
  }

  if ( ++count > size ) return;
  /* octet 4 */
  local_p = pointer + count;
  PrintString("\n              octet 4");
  PrintBits(*local_p, 0xc0);
  PrintString("        class = ");
  switch ( *local_p & 0xc0 )
  {
    Case(CLA_F_CS,"context-specific")
    Default("RESERVED")
  }
  PrintBits(*local_p, 0x20);
  PrintString("        form = ");
  switch ( *local_p & 0x20 )
  {
    Case(FRM_CONST,"constructor")
    Default("RESERVED")
  }

  PrintBits(*local_p, 0x1f);
  PrintString("        component tag = ");
  switch ( *local_p & 0x1f )
  {
    Case(CT_ROSE_INV,"invoke")
    Case(CT_ROSE_RETRES,"return result")
    Case(CT_ROSE_RETERR,"return error")
    Case(CT_ROSE_REJ,"reject")
    Case(CT_FACILITY_NFE,"network facility extensions")
    Default("RESERVED")
  }
  comp_flag = *local_p & 0x1f ;

  if ( ++count > size )
    return;
  /* octet 5 */
  local_p = pointer + count;
  PrintString("\n              octet 5");
  PrintBits(*local_p, 0x80);
  PrintString("        length format = ");
  switch ( *local_p & 0x80 )
  {
    Case(0,"short form, 1 octet")
    Default("long form")
  }
  PrintBits(*local_p, 0x7f);
  PrintString("        length of components = ");
  PrintNumber ((unsigned char ) (*local_p & 0x7f));
  comp_length = *local_p & 0x7f;

  if ( ++count > size )
    return;
  /* octet 6 */
  local_p = pointer + count;
  count++;
  PrintString("\n              octet 6");

  switch ( comp_flag )
  {
  case CT_ROSE_INV:
      /* decode invoke component */
      PrintString("\n                invoke component");
      count = DecodeASN1Component( CT_ROSE_INV, count, local_p, comp_length );
      break;
  case CT_ROSE_RETRES:
      /* decode return result component */
      PrintString("\n                return result component");
      count = DecodeASN1Component( CT_ROSE_RETRES, count, local_p, comp_length );
      break;
  case CT_ROSE_RETERR:
      /* decode return error component */
      PrintString("\n                return error component");
      count = DecodeASN1Component( CT_ROSE_RETERR, count, local_p, comp_length );
      break;
  case CT_ROSE_REJ:
      /* decode reject component */
      PrintString("\n                reject component");
      count = DecodeASN1Component( CT_ROSE_REJ, count, local_p, comp_length );
      break;
  case CT_FACILITY_NFE:
      /* decode network facility extension component */
      PrintString("\n                NFE component");
      count--;
      break;
  default:
      /* unknown component */
      PrintString("                  unknown component");
      count--;
      break;
  }
                                /* print any unprocessed octets in the facility ie */
  while (count <= size)
  {
      local_p = pointer + count;
      PrintBits(*local_p, 0xff);
      PrintString("      octet ");
      PrintNumber((unsigned char) (count + 1));
      PrintString("= 0x");
      PrintNumber( *local_p );
      PrintChar(*local_p);
      count++;
  }

}

void DecodeNotifyInd( unsigned char *pointer )
{
  unsigned char size, count;
  char * local_p;
  unsigned char comp_length = 0;

  size = *( pointer + 1 ) + 1; /* 1 is for the size itself */
  count = 1;
  
  /* length */
  local_p = pointer + count;
  PrintBits(*local_p, 0xff);
  PrintString("        length = 0x");
  PrintNumber( (unsigned char ) *local_p ); 
  comp_length = *local_p & 0xff;

  if ( ++count > size )
    return;
  /* octet 3 */
  local_p = pointer + count;
  PrintString("\n              octet 3");

  PrintBits(*local_p, 0x80);
  PrintString("        extension bit");

  PrintBits(*local_p, 0x7f);
  PrintString("        notification description = ");
  switch ( *local_p & 0x7f )
  {
    Case(USER_SUSPENDED,"user suspended")
    Case(USER_RESUMED,"user resumed")
    Case(CALL_FORWARDED,"call forwarded")
    Case(BEARER_SERVICE_CHANGE,"bearer service change")

    Case(EXTENDED_DESCRIPTION,"discriminator for extension to ASN.1 encoded component")
    Case(EXT_ISO_ASN1_NOTIF_DATA,"discriminator for extension to ASN.1 encoded component for ISO")

    Default("RESERVED")
  }

  if ( ++count > size )
    return;

  /* All "Notification Descriptions" but "ASN.1"s have only 3 octets. If there is more than
  3 octets the "Description" is either EXTENDED_DESCRIPTION or EXT_ISO_ASN1_NOTIF_DATA */

  PrintString("\n              octet 4");
  /* octet 4.X */
  local_p = pointer + count;
  count++;
  comp_length -= 1;
  count = DecodeASN1Component( CT_ROSE_RETRES, count, local_p, comp_length );

}

unsigned char DecodeASN1Component( unsigned char component, unsigned char octet, 
                                   unsigned char *pointer, unsigned char size )
{
#define invoke_id_tag           0
#define opvalue_id_tag_int      1
#define opvalue_id_octet_string 2
#define opvalue_id_tag_obj_id   3
#define sequence_id_tag         4
#define set_id_tag              5
#define service_id_tag          6
#define preferred_id_tag        7
#define errvalue_id_tag_int     8
#define errvalue_id_tag_obj_id  9
#define linked_id_tag           10
#define general_problem_id_tag  11
#define invoke_problem_id_tag   12
#define retres_problem_id_tag   13
#define reterr_problem_id_tag   14
#define null_id_tag             15
#define unknown_id_tag          16

char *id_tags [] =
{
/*invoke_id_tag           */        "invoke identifier",
/*opvalue_id_tag_int      */        "integer, operation value",
/*opvalue_id_octet_string */        "octet string, operation value",
/*opvalue_id_tag_obj_id   */        "object identifier, operation value",
/*sequence_id_tag         */        "sequence",
/*set_id_tag              */        "set",
/*service_id_tag          */        "service",
/*preferred_id_tag        */        "preferred",
/*errvalue_id_tag_int     */        "integer, error value",
/*errvalue_id_tag_obj_id  */        "object identifier, error value",
/*linked_id_tag           */        "linked identifier data element",
/*general_problem_id_tag  */        "general problem",
/*invoke_problem_id_tag   */        "invoke problem",
/*retres_problem_id_tag   */        "return result problem",
/*reterr_problem_id_tag   */        "return error problem",
/*null_id_tag             */        "null identifier",
/*unknown_id_tag          */        "unknown tag"
};

  unsigned char count, long_length;
  char * local_p;
  int curr_id_tag;
  char buffer[256] = {0};
  char buff[50] = {0};
  int i, length, temp_value;


  curr_id_tag = unknown_id_tag;
  length = 0;

  for ( count = 0; count < size; count ++ )
  {
      if ( count )
      {
          sprintf( buffer, "\n              octet %d.%d", octet, count);
          PrintString( buffer );
      }
      local_p = pointer + count;
      PrintBits(*local_p, 0xc0);
      PrintString("        class = ");
      switch ( *local_p & 0xc0 )
      {
        Case(CLA_F_UN,"universal")
        Case(CLA_F_CS,"context-specific")
        Default("RESERVED")
      }

      PrintBits(*local_p, 0x20);
      PrintString("        form = ");
      switch ( *local_p & 0x20 )
      {
        Case(FRM_PRIMITIVE,"primitive")
        Case(FRM_CONST,"constructor")
        Default("RESERVED")
      }

      /* check tag id component */
      PrintBits(*local_p, 0x1f);
      PrintString("        tag = ");

      if ( curr_id_tag != unknown_id_tag )
      {
          if ( component == CT_ROSE_INV || component == CT_ROSE_RETRES )
          { 
              if ( (*local_p &0x1f) == ID_TAG_OP_VAL_INT )
                curr_id_tag = opvalue_id_tag_int;
              else if ( (*local_p &0x1f) == ID_TAG_OP_OCTET_STRING )
                curr_id_tag = opvalue_id_octet_string;
              else if ( (*local_p &0x1f) == ID_TAG_OP_VAL_OBJ_ID)
                curr_id_tag = opvalue_id_tag_obj_id;
              else
                curr_id_tag = unknown_id_tag;
          }
          else if ( component == CT_ROSE_RETERR )
          { 
              if ( (*local_p &0x1f) == ID_TAG_ERROR_VAL_INT )
                curr_id_tag = errvalue_id_tag_int;
              else if ( (*local_p &0x1f) == ID_TAG_ERROR_VAL_OBJ_ID )
                curr_id_tag = errvalue_id_tag_obj_id;
              else
                curr_id_tag = unknown_id_tag;
          }
          else
            curr_id_tag = unknown_id_tag;
      }
      else if ( curr_id_tag == service_id_tag )
      {  /* octet 6.8.3 */
          if ( (component == CT_ROSE_INV) && ((*local_p &0x1f) == ID_TAG_PREF ) )
            curr_id_tag = preferred_id_tag;
          else
            curr_id_tag = unknown_id_tag;
      }

      if ( curr_id_tag == unknown_id_tag )
      {
          if ( component == CT_ROSE_REJ )
          {
              if ( (*local_p &0xff) == ID_TAG_RETERR_RROBLEM )          //0x83  
                  curr_id_tag = reterr_problem_id_tag;
              else if ( (*local_p &0xff) == ID_TAG_RETRES_RROBLEM )     //0x82
                  curr_id_tag = retres_problem_id_tag;
              else if ( (*local_p &0xff) == ID_TAG_INVOKE_RROBLEM )     //0x81
                  curr_id_tag = invoke_problem_id_tag;
              else if ( (*local_p &0xff) == ID_TAG_GENERAL_RROBLEM )    //0x80
                  curr_id_tag = general_problem_id_tag;
              else
                  curr_id_tag = unknown_id_tag;
          }

          /* tag is not identified */
          if ( curr_id_tag == unknown_id_tag )
          {
              if ( (*local_p &0xff) == ID_TAG_LINKED )          // 0x80
                  curr_id_tag = linked_id_tag;
              else if ( (*local_p &0x1f) == ID_TAG_INVOKE )     // 0x02
                  curr_id_tag = invoke_id_tag;
              else if ((*local_p &0x1f) == ID_TAG_NULL)         // 0x05
                  curr_id_tag = null_id_tag;
              else if ((*local_p &0x1f) == ID_TAG_SERVICE)      // 0x06
                  curr_id_tag = service_id_tag;
              else if ((*local_p &0x1f) == ID_TAG_SEQUENCE )    // 0x10
                  curr_id_tag = sequence_id_tag;
              else if ((*local_p &0x1f) == ID_TAG_SET )         // 0x11
                  curr_id_tag = set_id_tag;
              else
                  curr_id_tag = unknown_id_tag;
          }
      }

      sprintf( buffer, "%s", id_tags[curr_id_tag]);
      PrintString( buffer );

      /* get component's length */
      if ( ++count > size ) return (octet + count); /* end of component */
      local_p = pointer + count;
      sprintf( buffer, "\n              octet %d.%d", octet, count);
      PrintString( buffer );
      PrintBits(*local_p, 0x80);
      PrintString("        length format = ");
      switch ( *local_p & 0x80 )
      {
        Case(0,"short form, 1 octet")
        Default("long form")
      } 
      PrintBits(*local_p, 0x7f);
      if ( *local_p & 0x80 )
      { /* length of the conmponent > 127 octet => long form of the length is used */
            long_length = (*local_p & 0x7f) +1;  /* number of octets keeping the length */
            length = 0;
            for ( i = 0; i < long_length && count < size ; i++)
            {
                count++;
                local_p = pointer + count;
                PrintBits(*local_p, 0xff);

                length <<= 8;
                length |= (*local_p & 0xff);

            }
      }
      else
      { /* "short" length form */
          length = *local_p & 0x7f; /* length of the current tag */
      }
      sprintf( buffer, "        length of %s = %x", id_tags[curr_id_tag], length );
      PrintString( buffer );

      if ( curr_id_tag == sequence_id_tag || curr_id_tag == set_id_tag ) continue;

      if ( length == 0 ) continue;

      if ( ++count > size ) return (octet + count); /* end of component */
      local_p = pointer + count;
      if ( length > 1 )
        sprintf( buffer, "\n              octet %d.%d-%d", octet, count, count+length -1);
      else
        sprintf( buffer, "\n              octet %d.%d", octet, count);
      PrintString( buffer );


      if ( length > 1 ) 
      {
          sprintf( buffer, "\n                value of %s:", id_tags[curr_id_tag] );
          PrintString( buffer );
      }
      else
      {
          PrintBits(*local_p, 0xff);
          sprintf( buffer, "        value of %s = ", id_tags[curr_id_tag] );
          PrintString( buffer );
      }
      switch ( curr_id_tag )
      {
      case opvalue_id_tag_int:
      case opvalue_id_octet_string:
          if ( length > 1 )
          {
              for (i = 0; i < length && count < size; i++ )
              {

                  PrintBits(*local_p, 0xff);
                  sprintf( buffer, "            value = %x", (*local_p & 0xff));
                  PrintString( buffer );
                  count++;
                  local_p = pointer + count;
              }
              count--;
          }
          else
              PrintNumber( (unsigned char ) (*local_p & 0xff)); 
          break;
      case opvalue_id_tag_obj_id:
      case errvalue_id_tag_obj_id:
          if ( length > 1 )
          {
              char subid, subid_flag;

              subid = 1;
              subid_flag = 0;
              temp_value = 0;
              for (i = 0; i < length && count < size; i++)
              { /* decode object identifier */

                  PrintBits(*local_p, 0xff);

                  temp_value |= ((*local_p &0x7f));

                  if ( subid == 1 ) 
                      sprintf ( buff, "(%d %d)", temp_value / 40, temp_value % 40 );
                  else if ( !(*local_p & 0x80) )
                      sprintf( buff, "(%d)", temp_value );
                  else
                      sprintf( buff, "" );

                  sprintf( buffer, "            subidentifier %d%s= %x %s", 
                    subid, (subid_flag)?" cont'd ":" ", (*local_p & 0xff), buff );
                  
                  PrintString( buffer );

                  /* subidentifier is finished */
                  if ( !(*local_p & 0x80) )  subid++;
                  if ( *local_p & 0x80 ) subid_flag = 1;
                  else subid_flag = 0;
              
                  /* the last octet in the current subidentifier */
                  if ( !(*local_p & 0x80) ) temp_value = 0;
                  /* 8th bit is used to specify the long format */
                  else temp_value <<= 7;    

                  count++;
                  local_p = pointer + count;
              }
              count--;
          }
          else
              PrintNumber( (unsigned char ) (*local_p & 0xff)); 
          break;
      case service_id_tag:
          if ( length > 1 )
          {
              for (i = 0; i < length && count < size; i++)
              { /* decode object identifier */
                  if ( length > 1 ) PrintBits(*local_p, 0xff);
                  sprintf( buffer, "            value = %x", *local_p & 0xff );
                  PrintString( buffer );
                  count++;
                  local_p = pointer + count;
              }
              count --;
          }
          else
          {
              switch ( *local_p & 0xff)
              {
                  Case(0x01, "service 1" );
                  Case(0x02, "service 2" );
                  Case(0x03, "service 3" );
                  Default("RESERVED");
              }
          }
          break;
      case preferred_id_tag:
          switch ( *local_p & 0xff)
          {
              Case(0x00, "false (service is required)" );
              Case(0x01, "true (service is preferred)" );
              Default("RESERVED");
          }
          break;
      case errvalue_id_tag_int:
          switch ( component )
          {
          case CT_ROSE_INV:
              switch ( *local_p & 0xff)
              {
                  Case(0x01, "not supported" );
                  Default("RESERVED");
              }
              break;
          case CT_ROSE_RETERR:
          case CT_ROSE_REJ:
              PrintNumber( (unsigned char ) (*local_p & 0xff)); 
              break;
          }
          break;
      case general_problem_id_tag:
          switch ( *local_p & 0xff)
          {
              Case(0x00, "unrecognized component" );
              Case(0x01, "mistyped component" );
              Case(0x02, "badly structured component" );
              Default("RESERVED");
          }
          break;
      case invoke_problem_id_tag:
          switch ( *local_p & 0xff)
          {
              Case(0x00, "duplicate invocation" );
              Case(0x01, "unrecognized operation" );
              Case(0x02, "mistyped argument" );
              Case(0x03, "resource limitation" );
              Case(0x04, "initiator releasing" );
              Case(0x05, "unrecognized linked identifier" );
              Case(0x06, "linked response unexpected" );
              Case(0x07, "unexpected child operation" );
              Default("RESERVED");
          }
          break;
      case retres_problem_id_tag:
          switch ( *local_p & 0xff)
          {
              Case(0x00, "unrecognized invocation" );
              Case(0x01, "result response unexpected" );
              Case(0x02, "mistyped result" );
              Default("RESERVED");
          }
          break;
      case reterr_problem_id_tag:
          switch ( *local_p & 0xff)
          {
              Case(0x00, "unrecognized invocation" );
              Case(0x01, "error response unexpected" );
              Case(0x02, "unrecognized error" );
              Case(0x03, "unexpected error" );
              Case(0x04, "mistyped parameter" );
              Default("RESERVED");
          }
          break;
      case unknown_id_tag:  /* print the value of unknown_id_tag to follow the decode*/
      case null_id_tag:
      default:
          if ( length > 1 )
          {
              for (i = 0; i < length && count < size; i++ )
              { /* decode object identifier */

                  PrintBits(*local_p, 0xff);
                  sprintf( buffer, "            value = %x", (*local_p & 0xff));
                  PrintString( buffer );
                  count++;
                  local_p = pointer + count;
              }
              count--;
          }
          else
              PrintNumber( (unsigned char ) (*local_p & 0xff)); 
          break;
      }
  }
  return (octet + count);
}

/******************************************************************************
                            Decode Physical layer primitives
******************************************************************************/
void DecodePH( int mask,                /* trace mask */
               unsigned char b_num,        /* board number */
               unsigned char n_num,        /* nai number */
               unsigned char g_num,     /* group number */
               char direction,            /* INCOMING/OUTGOING message */
               char code,               /* DL message code */
               char *timestring)        /* time stamp */
{
    if ( !( mask & TRACE_PH ) ) return;

    PrintString("\n");
    if ( mask & TRACE_TIME )
    {
        PrintString("\n");
        PrintString(timestring);
    }

    PrintString( "\n  PH primitive = " );
    switch( code )
    {
        Case('A', "Activation Request");
        Case('a', "Activation Indication");
        Case('B', "Desactivation Request");
        Case('b', "Desactivation Indication");
        Default("PH UNKNOWN");
    }
    PrintString("   board ");
    PrintNumber(b_num);
    PrintString(" nai ");
    PrintNumber(n_num);
    switch ( direction )
    {
      Case(INCOMING," <--");
      Case(OUTGOING," -->");
    }

}

/******************************************************************************
                            CheckValue
    prints a warning if value1 != value2
******************************************************************************/
void CheckValue (int value1, int value2)
{
	if (value1 != value2)
		PrintString("\n        bit checking = RESERVED");
}

