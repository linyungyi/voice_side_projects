#include <ctadef.h>
#include <ctaerr.h>
#include <swidef.h>
#include <adidef.h>
#include "mspdef.h"
#include "mspcmd.h"

//New 3GP library
#include "mmfi.h"

extern SWIHD      hSrvSwitch0;

#define MAX_CONTEXTS        60

#if !defined(NUM_CTA_SERVICES)
#define NUM_CTA_SERVICES    3
#endif

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

#define MAX_BUFFER_SIZE 16400

typedef struct
{
    CTAHD               ctahd;
    char                video_cName[256];
    MM_EP               rtpEp;
    void*               data_buffer_p;
    void*               data_buffer_r;
    DWORD               playuii;
    DWORD               pr_mode_p;
    DWORD               pr_mode_r;
    unsigned            data_size_p;
    unsigned            data_size_r;
    char                threegp_FileName_play[512];
    char                threegp_FileName[512]; 
    BOOL                bPlayList;
    SRV_PLAY_LIST       playList;

//  MMFI_VERS_INFO      verInfo;
//  WORD                verInfoSize = sizeof (MMFI_VERS_INFO);
    MMFILE              mmFilePlay;
    MMFILE              mmFileRec;
    FILE_INFO_DESC      fileInfoDesc;
    DATA_FORMAT_DESC    dataFormatDescPlay;
    DATA_FORMAT_DESC    dataFormatDescRec;
    DATA_FORMAT_INFO    dataFormatInfo;    
    MMSTREAM            mmStreamPlay;
    MMSTREAM            mmStreamRec;
    DWORD               streamIdPlay;
    DWORD               streamIdRec;
    WORD                codecPlay;
    WORD                codecRec;
    
    void*               addDataBufferPlay;
    void*               addDataBufferRec1;
    void*               addDataBufferRec2;

    unsigned            bufSizePlay;
    unsigned            bufSizeRec;
    unsigned            byteCountPlay;
    unsigned            byteCountRec;
	
	bool				bRecDone;
	bool 				bPlayDone;	 
	bool 				bLastBuffer;	
	bool				bStopRecordCalled;
	bool				bStopPlayCalled;	
	bool				bUseAddPlayBuffer;
} VIDEO_CNXT_INFO;

typedef struct
{
    CTAHD               ctahd;
    char                audio_cName[256];
    MM_EP               rtpEp;
    MM_EP               FusionDS0Ep;
    MM_CH               FusionCh;    
    void*               data_buffer_p;
    void*               data_buffer_r;
    DWORD               pr_mode_p;
    DWORD               pr_mode_r;
    unsigned            data_size_p;
    unsigned            data_size_r;

    DATA_FORMAT_DESC    dataFormatDescPlay;
    DATA_FORMAT_DESC    dataFormatDescRec;  
    DATA_FORMAT_INFO    dataFormatInfo;    
    MMSTREAM            mmStreamPlay;
    MMSTREAM            mmStreamRec;
    DWORD               streamIdPlay;
    DWORD               streamIdRec;
    WORD                codecPlay;
    WORD                codecRec;
    unsigned            bufSizePlay;
    unsigned            bufSizeRec;

    void*               addDataBufferPlay;
    void*               addDataBufferRec1;
    void*               addDataBufferRec2;

    unsigned            byteCountPlay;
    unsigned            byteCountRec;
	
	bool				bRecDone;
	bool 				bPlayDone;	 
	bool 				bLastBuffer;	
	
	bool				bStopRecordCalled;		
	bool				bStopPlayCalled;		
	bool				bUseAddPlayBuffer;	
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
