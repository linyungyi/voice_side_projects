#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <termio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "nmstypes.h"
#include "mspdef.h"
#include "endian.h"

typedef enum { PLAY=0, RECORD } tOpType;

typedef enum { NONE=0, NATIVE_NO_SD, NATIVE_SD, FUS_IVR } tAudioMode;
typedef enum { NO_TS_MODE=0, TS_MODE } tTimeslot_Mode;
typedef struct {
    // Parameters set up initially
	tVideo_Format	        Video_Format;
	tAudio_RTP_Format	    Audio_RTP_Format;
	tAudio_Format			Audio_Record_Format1;
    tAudioMode  audioMode1;
    DWORD       ADIvidEncoder;
    DWORD       ADIaudEncoder1;
    DWORD       FusChanType;
    DWORD       FusEncodeRate;
    DWORD       FusFilterType;
	tOpType		Operation;
	DWORD		nPorts;
	char		srcIPAddr_video[20];
	DWORD		srcPort_video;
	char		destIPAddr_video[20];
	DWORD		destPort_video;
	char		srcIPAddr_audio[20];
	DWORD		srcPort_audio;
	char		destIPAddr_audio[20];
	DWORD		destPort_audio;
    DWORD       Board;
	DWORD		Stream;
	DWORD		Timeslot;
	DWORD		enable_Iframe;
	DWORD		audioStop_InitTimeout;
	DWORD		audioStop_SilenceTimeout;
	int 		audioStop_SilenceLevel;
	DWORD		videoStop_InitTimeout;
	DWORD		videoStop_NosignalTimeout;
	DWORD		maxTime;
	FILE*		p_videoFile;
	FILE*		p_audioFile;
	DWORD		native_silence_detect;
	DWORD		video_buffer_size;
	DWORD		audio_buffer_size;
    DWORD       frameQuota;
    // BOOLEANS to record if parameters were reset in config file
	DWORD		bEnable_Iframe;
	DWORD		bAudioStop_InitTimeout;
	DWORD		bAudioStop_SilenceTimeout;
	DWORD 		bAudioStop_SilenceLevel;
	DWORD		bVideoStop_InitTimeout;
	DWORD		bVideoStop_NosignalTimeout;
	DWORD		bMaxTime;
    DWORD       bPlay2;
	DWORD		bAutoRecord;
	DWORD		bAutoPlay;	
    
    // Load test related parameters
    DWORD       bAutoPlayList;
	char		autoPlayListFileName[80];    
    
	char		threegpFileName[80];
	int         nBoardType;      // 6000 for CG6000, 6500 for CG6500C    
    DWORD       base_fusion_timeslot;
    DWORD       base_ivr_timeslot;    
} t_MMStartParms;

extern t_MMStartParms mmParm;


#define SKIPSPACE(p) while (*p == '\t' || *p == ' ') ++p;
#define NUMVALUE	1
#define HEXVALUE	2
#define STRINGVALUE	3
#define FAILURE             0xffff

//Event type indicators
#define MMPR_KBD_EVENT   0
#define MMPR_CTA_EVENT   1

//Resource Definition Timeslot mapping
#define BASE_IVR_TIMESLOT_CG6000     48
#define BASE_FUSION_TIMESLOT_CG6000  80
#define BASE_IVR_TIMESLOT_CG6500     120
#define BASE_FUSION_TIMESLOT_CG6500  180


typedef struct t_argval {      /* Struct returned from getargs() */
    int keyindex;          /* keyword (index into passed list) */
    int numval;            /* value */
	char strval[80];       /* string value*/
} argval;

// Dummy values for Configuration keywords
#define OPERATION					1
#define N_PORTS						2
#define VIDEO_FORMAT				3
#define AUDIO_RTP_FORMAT			4
#define AUDIO_RECORD_FORMAT1		5
#define SRC_IPADDR_VIDEO			6
#define SRC_PORT_VIDEO				7
#define DEST_IPADDR_VIDEO			8
#define DEST_PORT_VIDEO				9
#define SRC_IPADDR_AUDIO			10
#define SRC_PORT_AUDIO				11
#define DEST_IPADDR_AUDIO			12
#define DEST_PORT_AUDIO				13
#define STREAM						14
#define TIMESLOT					15
#define ENABLE_IFRAME				16
#define AUDIO_STOP_INIT_TIMEOUT		17
#define AUDIO_STOP_SILENCE_TIMEOUT	18
#define AUDIO_STOP_SILENCE_LEVEL	19
#define VIDEO_STOP_INIT_TIMEOUT		20
#define VIDEO_STOP_NOSIGNAL_TIMEOUT	21
#define MAXTIME						22
#define NATIVE_SILENCE_DETECT		25
#define VIDEO_BUFFER_SIZE   		26
#define AUDIO_BUFFER_SIZE   		27
#define FRAME_QUOTA                 28
#define AUTO_RECORD     			32
#define AUTO_PLAY        			33
#define THREEGP_FILE_NAME			34
#define STORE_3GP       			35
#define BOARD_TYPE     			    37
#define AUTO_PLAY_LIST 			    38
#define AUTO_PLAY_LIST_FILE_NAME    39

#define PR_MODE_INIT                 0
#define PR_MODE_ACTIVE1              1
#define PR_MODE_DONE1                2
#define PR_MODE_ACTIVE2              3
#define PR_MODE_DONE2                4

#define SRV_AUDIO_PORT               3000
#define SRV_VIDEO_PORT               4000

MSP_CHANNEL_TYPE ConvertChannelType(tAudio_Format HostFormat);
int  get_keyword (char **bufp, argval *pval);
int  getargs (char *p, argval *value, int type);
void StripComments(char *comment_chars, char *text);

void ProcessCommand(char key);
void PrintCmdMenu();
void cmdStopVideo( int port );
void cmdStopAudio( int port );
int kbhit(void);
void mmSetupKbdIO( int init );
