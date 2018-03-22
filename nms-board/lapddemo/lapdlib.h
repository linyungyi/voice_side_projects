/*****************************************************************************
*  FILE: lapdlib.h
*
* Copyright (c) 1998-2001  NMS Communication.   All rights reserved.
*****************************************************************************/
typedef struct
{
    char len;
    char value[2];  /* value[0] is the MSB */
} CALL_REF;

typedef struct 
{
  char transfer_cap;        /* ITC_ defines (Bearer Capability) */
  char transfer_rate;       /* ITR_ defines (Bearer Capability) */
  char encoding;            /* UIL1_ defines (Bearer Capability) */
  char pref_excl;           /* PE_ defines (Channel Id) */
  char channel_number;      /* (Channel Id) */
  char calling_numb[32];    /* (Calling Party Number) */ 
  char called_numb[32];     /* (Called Party Number) */ 
}SETUP_PARM; 

typedef struct
{
  char channel_number;      /* (Channel Id) */
  char progress_desc;		/* PRD_ defines (Progress Indicator) */
}CALL_PROC_PARM;

typedef struct
{
  char channel_number;      /* (Channel Id) */
  char progress_desc;		/* PRD_ defines (Progress Indicator) */
}ALERTING_PARM;

typedef struct
{
  char channel_number;      /* (Channel Id) */
}SETUP_ACK_PARM;

typedef struct
{
  char cause;               /* CAU_ define (Cause) */
  char progress_desc;		/* PRD_ defines (Progress Indicator) */
}PROGRESS_PARM;

typedef struct  /* this structure is used for DISCONNECT, RELEASE and RELEASE COMPL */
{
  char msg_type;            /* MSG_DISCONNECT, MSG_RELEASE, MSG_RELEASE_COMP */
  char cause;               /* CAU_ define (Cause) */
}DISC_PARM;


/* function prototypes */
int InitQ931CC ( char *buff, char prot_ind, CALL_REF cr );

int BuildSetup ( char * msg_pointer, SETUP_PARM *struct_pointer, CALL_REF cr );
int BuildSetupAck ( char * msg_pointer, SETUP_ACK_PARM *struct_pointer, CALL_REF cr );
int BuildDisc ( char * msg_pointer, DISC_PARM *struct_pointer, CALL_REF cr );
int BuildAlerting ( char * msg_pointer, ALERTING_PARM *struct_pointer, CALL_REF cr );
int BuildCallProc ( char * msg_pointer, CALL_PROC_PARM *struct_pointer, CALL_REF cr );
int BuildProgress ( char * msg_pointer, PROGRESS_PARM *struct_pointer, CALL_REF cr );
int BuildEmptyMsg ( char * msg_pointer, char msg_type, CALL_REF cr );

int AddBearerCapability ( char *buff, char transfer_cap, char transfer_rate, char code );
int AddCause ( char *buff, char cause );
int AddChannelId ( char *buff, char pref_excl, char channel_num );
int AddCalledCalling ( char *buff, char ie_id, char *number );
int AddProgressInd ( char *buff, char progress_desc );

void ExtractCause ( char *buff, char *causep );
void ExtractBchannel ( char *buff, char *channelp );


/* defines */

#define BIT_1       0x01
#define BIT_2       0x02
#define BIT_3       0x04
#define BIT_4       0x08
#define BIT_5       0x10
#define BIT_6       0x20
#define BIT_7       0x40
#define BIT_8       0x80

