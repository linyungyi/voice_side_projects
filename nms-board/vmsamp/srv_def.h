#include <ctadef.h>
#include <ctaerr.h>
#include <swidef.h>
#include <adidef.h>
#include "mspdef.h"
#include "mspcmd.h"

extern SWIHD      hSrvSwitch0;

#define MAX_CONTEXTS	    60
#define NUM_CTA_SERVICES	3

typedef struct 
{
    MSPHD                   hd;
	MSP_ENDPOINT_ADDR       Addr;
    MSP_ENDPOINT_PARAMETER  Param;
} MM_EP;

typedef struct 
{
    MSPHD                  hd;
	MSP_CHANNEL_ADDR       Addr;
    MSP_CHANNEL_PARAMETER  Param;
} MM_CH;

#define SRV_PLAY_LIST_SZ 10

typedef struct {
    int  numFiles;
    int  currFile;    
    char fname[ SRV_PLAY_LIST_SZ ][256];
} SRV_PLAY_LIST;

typedef struct
{
    CTAHD   ctahd;
    char    video_cName[256];
	MM_EP	rtpEp;
	void*	data_buffer;
    DWORD   pr_mode;
    unsigned data_size;
    char    threegp_FileName[512];
    
    BOOL    bPlayList;
    SRV_PLAY_LIST playList;
} VIDEO_CNXT_INFO;

typedef struct
{
    CTAHD   ctahd;
    char    audio_cName[256];
	MM_EP	rtpEp;
	MM_EP	FusionDS0Ep;
	MM_CH	FusionCh;    
	void*	data_buffer;
    DWORD   pr_mode;
    unsigned data_size;
} AUDIO_CNXT_INFO;

extern VIDEO_CNXT_INFO VideoCtx[MAX_CONTEXTS];
extern AUDIO_CNXT_INFO AudioCtx[MAX_CONTEXTS];

int initServer();
int initServerConfigFile(char *fname);

void ShowEvent(CTA_EVENT *eventp);
DWORD NMSSTDCALL ErrorHandler(CTAHD ctahd, DWORD error, char *errtxt, char *func);
int SetupSwitch(int port);
void InitMsppConfiguration();
DWORD PlayRecord( int port, tVideo_Format fileformat );
DWORD SetRecordParms(CTAHD ctahd, ADI_MM_RECORD_PARMS* recParms);
void serverShutdown();
CTAHD FindContext(CTAHD received_chahd, DWORD* Port, DWORD* Type);
