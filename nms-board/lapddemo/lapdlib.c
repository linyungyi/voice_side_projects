/*****************************************************************************
*  FILE: lapdlib.c
*
*  DESCRIPTION: a collection of functions which can be used to build Q.931
*				messages
*
* Copyright (c) 1998-2001  NMS Communication.   All rights reserved.
*****************************************************************************/
#include <string.h>

#include "decisdn.h"
#include "lapdlib.h"

extern   char boardType;
/******************************************************************************
    It returns the number of bytes written in the buffer
        for prot_ind use PD_Q931_... defines
******************************************************************************/ 
int InitQ931CC ( char *buff, char prot_ind, CALL_REF cr )
{
  int bytes = 0;

  *( buff + bytes++ ) = prot_ind; /* use PD_Q931_CC or PD_Q931_MNT */  
  *( buff + bytes++ ) = cr.len;
  *( buff + bytes++ ) = cr.value[0]; 
  if (cr.len == 2 )
    *( buff + bytes++ ) = cr.value[1]; 
    
  return bytes;  
}

/******************************************************************************
    Builds a SETUP message
    It returns the number of bytes written in the buffer
******************************************************************************/ 
int BuildSetup ( char * msg_pointer, SETUP_PARM *struct_pointer, CALL_REF cr )
{
  int bytes = 0;

  bytes += InitQ931CC ( (char *) msg_pointer, PD_Q931_CC, cr );
  
  *(msg_pointer + bytes++) = MSG_SETUP;  

  /* IMPORTANT: add the IE's in the proper order  */
  bytes += AddBearerCapability ( msg_pointer + bytes,
                                 struct_pointer->transfer_cap, 
                                 struct_pointer->transfer_rate,
                                 struct_pointer->encoding);
  bytes += AddChannelId ( msg_pointer + bytes,
                          struct_pointer->pref_excl, 
                          struct_pointer->channel_number);
  if ( struct_pointer->calling_numb )
    bytes += AddCalledCalling ( msg_pointer + bytes, 
                              IE_CALLING,
                              struct_pointer->calling_numb );
  if ( struct_pointer->called_numb )
    bytes += AddCalledCalling ( msg_pointer + bytes, 
                              IE_CALLED,
                              struct_pointer->called_numb );
  return bytes;
}


/******************************************************************************
    Builds a SETUP ACK message
    It returns the number of bytes written in the buffer
******************************************************************************/ 
int BuildSetupAck ( char * msg_pointer, SETUP_ACK_PARM *struct_pointer, CALL_REF cr )
{
  int bytes = 0;

  bytes += InitQ931CC ( (char *) msg_pointer, PD_Q931_CC, cr );
  
  *(msg_pointer + bytes++) = MSG_SETUP_ACK;  

  /* IMPORTANT: add the IE's in the proper order  */
  if ( struct_pointer->channel_number )
    bytes += AddChannelId ( msg_pointer + bytes,
                                 PE_EXCL,
                                 struct_pointer->channel_number);
  return bytes;
}


/******************************************************************************
    Builds an ALERTING message
    It returns the number of bytes written in the buffer
******************************************************************************/ 
int BuildAlerting ( char * msg_pointer, ALERTING_PARM *struct_pointer, CALL_REF cr )
{
  int bytes = 0;

  bytes += InitQ931CC ( (char *) msg_pointer, PD_Q931_CC, cr );
  
  *(msg_pointer + bytes++) = MSG_ALERTING;  

  /* IMPORTANT: add the IE's in the proper order  */
  if ( struct_pointer->channel_number )
    bytes += AddChannelId ( msg_pointer + bytes,
                                 PE_EXCL,
                                 struct_pointer->channel_number);
  if ( struct_pointer->progress_desc )
    bytes += AddProgressInd ( msg_pointer + bytes,
                                  struct_pointer->progress_desc);

  return bytes;
}


/******************************************************************************
    Builds an CALL PROCEEDING message
    It returns the number of bytes written in the buffer
******************************************************************************/ 
int BuildCallProc ( char * msg_pointer, CALL_PROC_PARM *struct_pointer, CALL_REF cr )
{
  int bytes = 0;

  bytes += InitQ931CC ( (char *) msg_pointer, PD_Q931_CC, cr );
  
  *(msg_pointer + bytes++) = MSG_CALL_PROC;  

  /* IMPORTANT: add the IE's in the proper order  */
  if ( struct_pointer->channel_number )
    bytes += AddChannelId ( msg_pointer + bytes,
                                 PE_EXCL,
                                 struct_pointer->channel_number);
  if ( struct_pointer->progress_desc )
    bytes += AddProgressInd ( msg_pointer + bytes,
                                  struct_pointer->progress_desc);
  return bytes;
}


/******************************************************************************
    Builds a PROGRESS message
    It returns the number of bytes written in the buffer
******************************************************************************/ 
int BuildProgress ( char * msg_pointer, PROGRESS_PARM *struct_pointer, CALL_REF cr )
{
  int bytes = 0;

  bytes += InitQ931CC ( (char *) msg_pointer, PD_Q931_CC, cr );
  
  *(msg_pointer + bytes++) = MSG_PROGRESS;  

  /* IMPORTANT: add the IE's in the proper order  */
  if ( struct_pointer->cause )
    bytes += AddCause ( msg_pointer + bytes,
                                 struct_pointer->cause);

    bytes += AddProgressInd ( msg_pointer + bytes,
                                  struct_pointer->progress_desc);

  return bytes;
}


/******************************************************************************
    Builds a DISCONNECT, RELEASE or RELEASE COMPLETE message
    It returns the number of bytes written in the buffer
******************************************************************************/ 
int BuildDisc ( char * msg_pointer, DISC_PARM *struct_pointer, CALL_REF cr )
{
  int bytes = 0;

  bytes += InitQ931CC ( (char *) msg_pointer, PD_Q931_CC, cr );
  
  *(msg_pointer + bytes++) = struct_pointer->msg_type;  

  /* IMPORTANT: add the IE's in the proper order  */
  if ( struct_pointer->cause )
    bytes += AddCause ( msg_pointer + bytes,
                                 struct_pointer->cause);
  return bytes;
}


/******************************************************************************
    Builds a message with no IE's   (CONNECT, CONNECT ACK etc)
    It returns the number of bytes written in the buffer
******************************************************************************/ 
int BuildEmptyMsg ( char * msg_pointer, char msg_type, CALL_REF cr )
{
  int bytes = 0;

  bytes += InitQ931CC ( (char *) msg_pointer, PD_Q931_CC, cr );
  
  *(msg_pointer + bytes++) = msg_type;  

  return bytes;
}

/******************************************************************************
    Builds the Bearer Capability Information Element
        for transfer_cap use ITC_... defines
        for transfer_rate use ITR_... defines
        for code use UIL1_... defines
******************************************************************************/ 
int AddBearerCapability ( char *buff, char transfer_cap, char transfer_rate, char code )
{
  int bytes = 0;
  char *len_p;    

  *( buff + bytes++ ) = IE_BC;
  /* save the pointer to the length of the IE */
  len_p = buff + bytes++;
 
  /* octet 3 */
  *( buff + bytes++ ) = BIT_8 + CS_CCITT + transfer_cap; 
  /* octet 4 */
  *( buff + bytes++ ) = BIT_8 + TM_CIRCUIT + transfer_rate; 
  /* octet 5 */
  *( buff + bytes++ ) = BIT_8 + BIT_6 + code; 

  /* octet 2 */
  *len_p = bytes - 2;   /* length does not include octets 1 and 2 */

  return bytes;
}

/******************************************************************************
    Builds the cause Information Element
        for cause use CAU_... defines
******************************************************************************/ 
int AddCause ( char *buff, char cause )
{
  int bytes = 0;
  char *len_p;    

  *( buff + bytes++ ) = IE_CAUSE;
  /* save the pointer to the length of the IE */
  len_p = buff + bytes++;
 
  /* octet 3 */
  *( buff + bytes++ ) = (char) ( BIT_8 + CS_CCITT + LOC_USR ); 
  /* octet 4 */
  *( buff + bytes++ ) = BIT_8 + cause; 

  /* octet 2 */
  *len_p = bytes - 2;   /* length does not include octets 1 and 2 */

  return bytes;
}

/******************************************************************************
    Builds the Channel Identification Information Element
        for pref_excl use PE_... defines
******************************************************************************/ 
int AddChannelId ( char *buff, char pref_excl, char channel_num )
{
  int bytes = 0;
  char *len_p;    

  *( buff + bytes++ ) = IE_CHANNEL_ID;
  /* save the pointer to the length of the IE */
  len_p = buff + bytes++;

  /* octet 3 */
  *( buff + bytes++ ) = BIT_8 + IID_IMPL + boardType + pref_excl + DCI_NOT + ICS_B1; 
  /* octet 3.2 */
  *( buff + bytes++ ) = (char) (BIT_8 + CS_CCITT + NM_NUMB + CHT_B); 
  /* octet 3.3 */
  *( buff + bytes++ ) = channel_num; 

  /* octet 2 */
  *len_p = bytes - 2;   /* length does not include octets 1 and 2 */

  return bytes;
}

/******************************************************************************
    Builds the Called/ing Party Number Information Element
        for ie_id use IE_CALLING or IE_CALLED
******************************************************************************/ 
int AddCalledCalling ( char *buff, char ie_id, char *number )
{
  int bytes = 0;
  char *len_p;    

  *( buff + bytes++ ) = ie_id;
  /* save the pointer to the length of the IE */
  len_p = buff + bytes++;

  if ( ie_id == IE_CALLED )
  {
    /* octet 3 */
    *( buff + bytes++ ) = (char) (BIT_8 + TON_UNKNOWN + NP_ISDN);
  }
  else
  {
    /* octet 3 */
    *( buff + bytes++ ) = (char) (TON_UNKNOWN + NP_ISDN);
    /* octet 3a */
    *( buff + bytes++ ) = (char) (BIT_8 + PI_ALLOWED + SI_UP_NS);
  }
  
  /* octets 4... */
  strcpy( (buff + bytes), number );    
  bytes += strlen (number);

  /* octet 2 */
  *len_p = bytes - 2;   /* length does not include octets 1 and 2 */

  return bytes;
}

/******************************************************************************
    Builds the Progress Indicator Information Element
        for progress_desc use PRD_... defines
******************************************************************************/ 
int AddProgressInd ( char *buff, char progress_desc )
{
  int bytes = 0;
  char *len_p;    

  *( buff + bytes++ ) = IE_PROGRESS_IND;
  /* save the pointer to the length of the IE */
  len_p = buff + bytes++;

  /* octet 3 */
  *( buff + bytes++ ) = (char) BIT_8 + CS_CCITT + LOC_USR;

  /* octet 4 */
  *( buff + bytes++ ) = BIT_8 + progress_desc;

  /* octet 2 */
  *len_p = bytes - 2;   /* length does not include octets 1 and 2 */

  return bytes;
}
  
/******************************************************************************
    Extract the cause value from the Cause Information Element
      buff points to the IE id (IE_CAUSE)
      causep is a pointer to the returned cause value
******************************************************************************/ 
void ExtractCause ( char *buff, char *causep )
{
  if ( *( buff + 2) & 0x80 )    /* octet 3a is not present */
    *causep = *( buff + 3);
  else
    *causep = *( buff + 4);
}

/******************************************************************************
    Extract the B Channel from the Channel Id Information Element
      buff points to the IE id (IE_CHAN_ID)
      channelp is a pointer to the returned channel value (in most of the 
	  variants the channel value is actually (*channelp & 0x7f) )
******************************************************************************/ 
void ExtractBchannel ( char *buff, char *channelp )
{
  /* we assume interface implicity identified ( octet 3.1 is missing) */
	
  *channelp = *( buff + 4);
}
