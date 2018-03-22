#ifndef _TRC_CONFIG_H
#define _TRC_CONFIG_H

#include "trcapi.h"

typedef struct {
	char data[MAX_DECODER_CFG_INFO_SZ];
	int  len;
} tDecoderConfigInfo;

typedef struct {
	tDecoderConfigInfo cfg1;
	tDecoderConfigInfo cfg2;	
} tDecoderCfg;

/* TRC channel state */
#define XCSTATE_NONE              200
#define XCSTATE_STOPPED           201
#define XCSTATE_STARTING          202
#define XCSTATE_STARTED           203
#define XCSTATE_STOPPING          204
#define XCSTATE_DESTROYING        206
#define XCSTATE_CREATING          207
#define XCSTATE_CREATED           208

typedef struct tag_VIDEO_XC
{
    DWORD                   bActive;
    DWORD                   bFVU_Enabled;

	TRC_HANDLE				trcChHandle;
	DWORD					vtpId;
	tTrcRes					trcRes;        /* assigned transcoder resource addressing information */
	tTrcChConfig			trcChConfig;   /* channel configuration information */

	tDecoderCfg				xcDecoderConfig; 
    DWORD                   state;
} VIDEO_XC;

U32 trcCallback(tTrcMessage *pMsg, S32 size);
int initTrcConfig( int nGw );
int initTrc();
void change_xc_state(int nGw, int new_state);

#endif // _TRC_CONFIG_H


