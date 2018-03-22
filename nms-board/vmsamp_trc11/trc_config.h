#ifndef _TRC_CONFIG_H
#define _TRC_CONFIG_H

#include "trcapi.h"

#ifdef UNIX
#include <netinet/in.h> /* INET_ADDRSTRLEN */
#else
#define INET_ADDRSTRLEN 16
#endif

typedef struct {
	char data[MAX_DECODER_CFG_INFO_SZ];
	int  len;
} tDecoderConfigInfo;

typedef struct {
	tDecoderConfigInfo cfg1;
	tDecoderConfigInfo cfg2;	
} tDecoderCfg;


typedef struct tag_VIDEO_XC
{
    DWORD                   bActive;
    DWORD                   bFVU_Enabled;

    char ipAddr[ INET_ADDRSTRLEN ];    /* VTP IP address */        
    int                     xcChanId;    
    tChannelConfig          xcChanConfig;   
	tDecoderCfg				xcDecoderConfig; 
    DWORD                   state;
} VIDEO_XC;

int trcCallback(tTrcMessage *pMsg, int size);
int initTrcConfig( int nGw );
int initTrc();

#endif // _TRC_CONFIG_H


