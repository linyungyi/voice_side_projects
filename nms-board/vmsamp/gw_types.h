#ifndef _GW_TYPES_INCLUDE
#define _GW_TYPES_INCLUDE

#define GW_DIAL_NUM_SZ 20

typedef struct {
	// IP addresses and UDP ports
	char		   srcIPAddr_video[20];
	DWORD		   srcPort_video;
	char		   destIPAddr_video[20];
	DWORD	       destPort_video;
	char		   srcIPAddr_audio[20];
	DWORD		   srcPort_audio;
	char		   destIPAddr_audio[20];
	DWORD		   destPort_audio;
	
	// ISDN parameters	
	unsigned int   isdn_country;
	unsigned int   isdn_operator;	
	DWORD          isdn_dNT_TE;
	char		   sIfcType[10];        
	
	// Endpoints configuration parameters
	tVideo_Format	      videoFormat;
	tVideo_Format	      videoAlternateFormat;
	tAudio_Format         audioFormat;	
	
	unsigned short     gsmamr_mode;
	unsigned short     gsmamr_maxBitRate;
	DWORD		       mux_timeslot;
	unsigned int	   h263_maxBitRate;	
	
	// Load test parameters
	int			   auto_place_call;    // Place the call when initialization completed
	int			   auto_hangup;        // Disconnect the call having received PLAY_DONE or RECORD_DONE
	unsigned int   between_calls_time; // The time between the calls (millisec)
	char	       dial_number[ GW_DIAL_NUM_SZ ]; // Number to dial automatically
	int            nPorts;  // Number of ports
	int			   nCalls;  // Number of calls for the load test
	
	// H245 
	int            sendUniDirAudioOLC; // Send unidirectional Audio OLC (On/Off)
	int            sendVideoOLC;
    int            rejectVideoOLC;
    int            specifyVOVOL; // Specify VO/VOL header in Video OLC
    int			   ackUniDirAudioOLC; // Acknowledge Audio OLC as Unidirectional/Bidirectional
    
    // TRC
    int            trcTraceMask;        // TRC trace mask for the console
    int            trcFileTraceMask;    // TRC trace mask for the file
    char           sTrcConfigFile[100]; // TRC configuration file name
    char           sTrcLogFile[100];      // TRC log file name         
    
    // H324    
    char           sH324LogFile[100];   // H324 log file name     

   // XC parameters
    int            useVideoXc;    
} t_GwStartParms;

extern t_GwStartParms gwParm;

#define GSMAMR_MAXBITRATE     101
#define GSAMR_MODE            102
#define GW_ISDN_COUNTRY       103
#define GW_ISDN_OPERATOR      104
#define GW_SENDUNIDIRAUDOLC   105
#define GW_MUX_TIMESLOT       106
#define GW_ISDN_NT_TE         107
#define GW_AUTO_PLACE_CALL    108
#define GW_AUTO_HANG_UP       109
#define GW_SEND_VIDEO_OLC     110
#define GW_BETWEEN_CALLS_TIME 111
#define GW_DIAL_NUMBER        112
#define GW_NUM_OF_PORTS       113
#define GW_NUM_OF_CALLS       114
#define GW_ACKUNIDIRAUDOLC    115
#define GW_VIDEO_FORMAT       116
#define GW_AUDIO_FORMAT       117
#define GW_H263_MAXBITRATE    118
#define GW_USE_VIDEO_XC       119
#define TRC_CONFIG_FILE       120
#define GW_TRC_TRACE_MASK     121
#define GW_REJECT_VIDEO_OLC   122
#define GW_VIDEO_ALTERNATE_FORMAT   123
#define GW_SPECIFY_VO_VOL       124
#define GW_HANG_UP_ON_UUI       125
#define GW_IFC_TYPE             126
#define GW_H324_LOG_FILE        127
#define GW_TRC_FILE_TRACE_MASK  128
#define TRC_LOG_FILE            129

#endif
