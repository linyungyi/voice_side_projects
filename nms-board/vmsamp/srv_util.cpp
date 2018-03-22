/**********************************************************************
*  File - srv_util.cpp
*
*  Description - Utility functions needed by Video Messaging Server 
*
*  2005  NMS Communications Corporation.
*
***********************************************************************/

///////////////////
// Include Files //
///////////////////
 #include <ctype.h>  /* isalnum() */

#include "mega_global.h"  /* GW_AUDIO_PORT, GW_VIDEO_PORT, CG_BOARD_IP */
#include "srv_types.h"
#include "srv_def.h"
#include "vm_trace.h"
#include <sys/timeb.h>

#include "vm_stat.h"


// Global variables
char ConfigFileName[80];
char LogFileName[80];
FILE* pLogFile;
SWIHD      hSrvSwitch0;
t_MMStartParms mmParm;
int VmTraceMask = 0x7D;  // No T_H245_DET - 1111101


DWORD  bVerbose = TRUE;
DWORD  bAppLogFileSpecified = 0;

VIDEO_CNXT_INFO VideoCtx[MAX_CONTEXTS];
AUDIO_CNXT_INFO AudioCtx[MAX_CONTEXTS];
AUDIO_CNXT_INFO fiAudioCtx[MAX_CONTEXTS];

static void SetDefaultSrvParms();
static int  ModifySrvParms( char *File );
static void PrintSrvConfiguration();

int initServerConfigFile(char *ConfigFile)
{
	SetDefaultSrvParms();

	if( ConfigFile[0] == '\0' ) {
		mmPrintAll("The Server Config File wasn't specified. Will use default values.\n");
	} else {
		mmPrintAll("Config file specified: %s\n", ConfigFile);			
		ModifySrvParms( ConfigFile );
	}

	// Print updated configuration
	PrintSrvConfiguration();
    
	return 1;	
}


/* ***********************************************************************
   Function: SetDefaultParms
   Description: Set default parameter values
   Input: None
   Output: None
************************************************************************ */
void SetDefaultSrvParms()
{
	mmParm.Operation                  = RECORD;
	mmParm.nPorts                     = 1;
	mmParm.Video_Format               = MPEG4;
	mmParm.Audio_RTP_Format           = AMR_122;
	mmParm.Audio_Record_Format1       = N_AMR;
	strcpy(mmParm.srcIPAddr_video,  CG_BOARD_IP);
	mmParm.srcPort_video              = SRV_VIDEO_PORT;
	strcpy(mmParm.destIPAddr_video, CG_BOARD_IP);
	mmParm.destPort_video             = GW_VIDEO_PORT;
	strcpy(mmParm.srcIPAddr_audio,  CG_BOARD_IP);
	mmParm.srcPort_audio              = SRV_AUDIO_PORT;
	strcpy(mmParm.destIPAddr_audio, CG_BOARD_IP);
	mmParm.destPort_audio             = GW_AUDIO_PORT;
	mmParm.Board                      = CmdLineParms.nBoardNum; // Temporary!
	mmParm.Stream                     = 16;		// IVR/Fusion Channel only
	mmParm.Timeslot                   = 0;		// IVR/Fusion Channel only
	mmParm.enable_Iframe              = 0;		// Disabled
	mmParm.audioStop_InitTimeout      = 0;		// Disabled
	mmParm.audioStop_SilenceTimeout   = 0;	    // Disabled
	mmParm.audioStop_SilenceLevel     = -45;	   
	mmParm.videoStop_InitTimeout      = 0;		// Disabled
	mmParm.videoStop_NosignalTimeout  = 0;	    // Disabled
	mmParm.maxTime                    = 0;		// Disabled
	mmParm.p_videoFile                = NULL;
	mmParm.p_audioFile                = NULL;
	mmParm.native_silence_detect      = 0;        // Disabled
	mmParm.video_buffer_size          = 4000000;
	mmParm.audio_buffer_size          = 4000000;
    mmParm.frameQuota                 = 2;
    
    mmParm.bEnable_Iframe             = TRUE;
	mmParm.bAudioStop_InitTimeout     = TRUE;
	mmParm.bAudioStop_SilenceTimeout  = TRUE;
	mmParm.bAudioStop_SilenceLevel    = FALSE;
	mmParm.bVideoStop_InitTimeout     = TRUE;
	mmParm.bVideoStop_NosignalTimeout = TRUE;
	mmParm.bMaxTime                   = TRUE;     
	mmParm.bPlay2                     = FALSE;     
	mmParm.bAutoRecord                = 1;     	
	mmParm.bAutoPlay                  = 0;     		
	mmParm.bAutoPlayList              = 0;     			
	strcpy( mmParm.autoPlayListFileName, "autoplay.lst" );    			    
    
	strcpy(mmParm.threegpFileName,     "record.3gp");
	
    mmParm.nBoardType                 = 6000;		
}


/* ***********************************************************************
   Function: ModifyParms
   Description: Modify parameter values according to the config file
   Input: None
   Output: returns FAILURE if can not open config file, SUCCESS otherwise
************************************************************************ */
int ModifySrvParms( char *FileName )
{
	FILE* pConfigFile;
    char buf [256];
    char *p = buf;
	argval value;
	int keyword;

	if ((pConfigFile = fopen(FileName, "r")) == NULL)
	{
		mmPrintAll("\nCan not open config file %s. Will use default parameters...\n\n", FileName);
		return FAILURE;
	}

	mmPrintAll("Reading parameters from the configuration file:\n");
	for(;;)
	{	 /* Read a line from config file; break on EOF or error. */
        if (fgets (buf, sizeof buf, pConfigFile) == NULL)
            break;

        /* Strip comments, quotes, and newline. */
        StripComments ("#;", (char *)buf);

		/* First token is keyword; some may be followed by a value or range */
		p = buf;
        keyword = get_keyword (&p, &value);

		switch(keyword)
		{
		case -1:
			continue;
		case 0:
			continue;
		case OPERATION:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.Operation = (tOpType)(value.numval);
			mmPrintAll("Operation = %d\n", mmParm.Operation);
			break;
		case N_PORTS:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.nPorts = value.numval;
			mmPrintAll("nPorts = %d\n", mmParm.nPorts);
			break;
		case VIDEO_FORMAT:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.Video_Format = (tVideo_Format)(value.numval);
			mmPrintAll("Video Format = %d\n", mmParm.Video_Format);
			break;
		case AUDIO_RTP_FORMAT:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.Audio_RTP_Format = (tAudio_RTP_Format)(value.numval);
			mmPrintAll("Audio RTP Format = %d\n", mmParm.Audio_RTP_Format);
			break;
		case AUDIO_RECORD_FORMAT1:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.Audio_Record_Format1 = (tAudio_Format)(value.numval);
			mmPrintAll("Audio Record Format = %d\n", mmParm.Audio_Record_Format1);
			break;
		case SRC_IPADDR_VIDEO:
			if( getargs(p,&value, STRINGVALUE) != SUCCESS )
                continue;
			strcpy(mmParm.srcIPAddr_video, value.strval);
			mmPrintAll("Source IP Address for Video = %s\n", mmParm.srcIPAddr_video);
			break;
		case SRC_PORT_VIDEO:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.srcPort_video = value.numval;
			mmPrintAll("Source Port for Video = %d\n", mmParm.srcPort_video);
			break;
		case DEST_IPADDR_VIDEO:
			if( getargs(p,&value, STRINGVALUE) != SUCCESS )
                continue;
			strcpy(mmParm.destIPAddr_video, value.strval);
			mmPrintAll("Destination IP Address for Video = %s\n", mmParm.destIPAddr_video);
			break;
		case DEST_PORT_VIDEO:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.destPort_video = value.numval;
			mmPrintAll("Destination Port for Video = %d\n", mmParm.destPort_video);
			break;
		case SRC_IPADDR_AUDIO:
			if( getargs(p,&value, STRINGVALUE) != SUCCESS )
                continue;
			strcpy(mmParm.srcIPAddr_audio, value.strval);
			mmPrintAll("Source IP Address for Audio = %s\n", mmParm.srcIPAddr_audio);
			break;
		case SRC_PORT_AUDIO:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.srcPort_audio = value.numval;
			mmPrintAll("Source Port for Audio = %d\n", mmParm.srcPort_audio);
			break;
		case DEST_IPADDR_AUDIO:
			if( getargs(p,&value, STRINGVALUE) != SUCCESS )
                continue;
			strcpy(mmParm.destIPAddr_audio, value.strval);
			mmPrintAll("Destination IP Address for Audio = %s\n", mmParm.destIPAddr_audio);
			break;
		case DEST_PORT_AUDIO:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.destPort_audio = value.numval;
			mmPrintAll("Destination Port for Audio = %d\n", mmParm.destPort_audio);
			break;
		case TIMESLOT:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.Timeslot = value.numval;
			mmPrintAll("Timeslot = %d\n", mmParm.Timeslot);
			break;
		case ENABLE_IFRAME:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.enable_Iframe = value.numval;
			mmPrintAll("Enable Iframe = %d\n", mmParm.enable_Iframe);
            mmParm.bEnable_Iframe             = TRUE;
			break;
		case AUDIO_STOP_INIT_TIMEOUT:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.audioStop_InitTimeout = value.numval;
			mmPrintAll("Audio Stop Init Timeout = %d\n", mmParm.audioStop_InitTimeout);
            mmParm.bAudioStop_InitTimeout     = TRUE;
			break;
		case AUDIO_STOP_SILENCE_TIMEOUT:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.audioStop_SilenceTimeout = value.numval;
			mmPrintAll("Audio Stop Silence Timeout = %d\n", mmParm.audioStop_SilenceTimeout);
            mmParm.bAudioStop_SilenceTimeout  = TRUE;
			break;
		case AUDIO_STOP_SILENCE_LEVEL:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.audioStop_SilenceLevel = value.numval;
			mmPrintAll("Audio Stop Silence Level = %d\n", mmParm.audioStop_SilenceLevel);
            mmParm.bAudioStop_SilenceLevel    = TRUE;
			break;
		case VIDEO_STOP_INIT_TIMEOUT:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.videoStop_InitTimeout = value.numval;
			mmPrintAll("Video Stop Init Timeout = %d\n", mmParm.videoStop_InitTimeout);
            mmParm.bVideoStop_InitTimeout     = TRUE;
			break;
		case VIDEO_STOP_NOSIGNAL_TIMEOUT:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.videoStop_NosignalTimeout = value.numval;
			mmPrintAll("Video Stop No-Signal Timeout = %d\n", mmParm.videoStop_NosignalTimeout);
            mmParm.bVideoStop_NosignalTimeout = TRUE;
			break;
		case MAXTIME:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.maxTime = value.numval;
			mmPrintAll("Max Play/Record Time = %d\n", mmParm.maxTime);
            mmParm.bMaxTime                   = TRUE;
			break;
		case NATIVE_SILENCE_DETECT:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.native_silence_detect = value.numval;
			mmPrintAll("Native Audio Silence Detect = %d\n", mmParm.native_silence_detect);
			break;
		case VIDEO_BUFFER_SIZE:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.video_buffer_size = value.numval;
			mmPrintAll("Video Buffer Size = %d\n", mmParm.video_buffer_size);
			break;
		case AUDIO_BUFFER_SIZE:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.audio_buffer_size = value.numval;
			mmPrintAll("Audio Buffer Size = %d\n", mmParm.audio_buffer_size);
			break;
		case FRAME_QUOTA:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.frameQuota = value.numval;
			mmPrintAll("Frame Quota = %d\n", mmParm.frameQuota);
			break;
		case AUTO_RECORD:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.bAutoRecord = value.numval;
			mmPrintAll("AutoRecord = %d\n", mmParm.bAutoRecord);
			break;
		case AUTO_PLAY:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.bAutoPlay = value.numval;
			mmPrintAll("AutoPlay = %d\n", mmParm.bAutoPlay);
			break;
		case AUTO_PLAY_LIST:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			mmParm.bAutoPlayList = value.numval;
			mmPrintAll("AutoPlayList = %d\n", mmParm.bAutoPlayList);
			break;
		case AUTO_PLAY_LIST_FILE_NAME:
			if( getargs(p,&value, STRINGVALUE) != SUCCESS )
                continue;
			strcpy(mmParm.autoPlayListFileName, value.strval);
			mmPrintAll("Auto Play List File Name = %s\n", mmParm.autoPlayListFileName);
			break;			            
		case THREEGP_FILE_NAME:
			if( getargs(p,&value, STRINGVALUE) != SUCCESS )
                continue;
			strcpy(mmParm.threegpFileName, value.strval);
			mmPrintAll("3GP File Name = %s\n", mmParm.threegpFileName);
			break;							
		case BOARD_TYPE:
			if( getargs(p,&value, NUMVALUE) != SUCCESS )
                continue;
			if((value.numval != 6000) && (value.numval != 6500))
			{
				printf("Invalid nBoardType option (only 6000 and 6500 are valid)\n");
				continue;
			}
			mmParm.nBoardType = value.numval;
			printf("nBoardType = %d\n", mmParm.nBoardType);
			break;            
		default:
			continue;
		}
	}

	fclose(pConfigFile);

	return SUCCESS;
}


/*---------------------------------------------------------------------------
 * get_keyword - Finds the first alpha-numeric token and looks it up in
 *   a table.  Verifies equals or colon separator after the keyword.  Some
 *   keywords may have an optional value or range before the separator.
 *
 *--------------------------------------------------------------------------*/
int get_keyword (char **bufp, argval *pval)
{

    static struct {
        char *keytxt;
        int   keyid;
    } table [] = {
        {"Operation",					OPERATION               },
		{"nPorts",						N_PORTS                 },
		{"Video_Format",				VIDEO_FORMAT            },
        {"Audio_RTP_Format",			AUDIO_RTP_FORMAT        },
		{"Audio_Record_Format1",		AUDIO_RECORD_FORMAT1    },
		{"srcIPAddr_video",				SRC_IPADDR_VIDEO        },
		{"srcPort_video",				SRC_PORT_VIDEO          },
		{"destIPAddr_video",			DEST_IPADDR_VIDEO       },
		{"destPort_video",				DEST_PORT_VIDEO         },
		{"srcIPAddr_audio",				SRC_IPADDR_AUDIO        },
		{"srcPort_audio",				SRC_PORT_AUDIO          },
		{"destIPAddr_audio",			DEST_IPADDR_AUDIO       },
		{"destPort_audio",				DEST_PORT_AUDIO         },
		{"Stream",						STREAM                  },
		{"Timeslot",					TIMESLOT                },
		{"enable_Iframe",				ENABLE_IFRAME           },
		{"audioStop_InitTimeout",		AUDIO_STOP_INIT_TIMEOUT },
		{"audioStop_SilenceTimeout",	AUDIO_STOP_SILENCE_TIMEOUT },
		{"audioStop_SilenceLevel",		AUDIO_STOP_SILENCE_LEVEL},
		{"videoStop_InitTimeout",		VIDEO_STOP_INIT_TIMEOUT },
		{"videoStop_NosignalTimeout",	VIDEO_STOP_NOSIGNAL_TIMEOUT },
		{"Maxtime",						MAXTIME                 },
		{"native_silence_detect",		NATIVE_SILENCE_DETECT   },
		{"video_buffer_size",		    VIDEO_BUFFER_SIZE       },
		{"audio_buffer_size",		    AUDIO_BUFFER_SIZE       },
		{"frame_quota",		            FRAME_QUOTA             },
		{"AutoRecord",  				AUTO_RECORD             },		
		{"AutoPlay",    				AUTO_PLAY               },				
		{"3gpFileName",			    	THREEGP_FILE_NAME       },	
		{"nBoardType",  				BOARD_TYPE              },				        				
		{"AutoPlayList",   				AUTO_PLAY_LIST          },				
		{"ListFileName",		    	AUTO_PLAY_LIST_FILE_NAME },	        
    };
    int i;
    char *p = *bufp;
    char *ptoken;
    int size = 0;
    int keyid = -1;
    char keywordtxt[80];

    /* Skip leading spaces; done if eol or if rest is comment */
    SKIPSPACE(p);
    if (*p == '\0')
        return 0;
    ptoken = p;
    /* Skip to end of keyword */
    while (isalnum(*p) || *p == '_')
    {
        ++p;
        ++size;
    }
    if (size == 0)
        return -1;
    /* Scan table to validate keyword */
    strncpy (keywordtxt, ptoken, size);
    keywordtxt[size] = '\0';
    for (i=0; i<sizeof table/sizeof table[0]; i++)
        if (strcmp(keywordtxt, table[i].keytxt) == 0)
        {
            keyid = table[i].keyid;
            break;
        }

	*bufp = p;
    return keyid;
} /* get_keyword */


/*---------------------------------------------------------------------------
 * getargs - get the arguments that are expected for a keyword.  The second
 *    argument is a bitmask that specifies the allowable data formats.
 *
 * Returns found type if successful, 0 if not.
 *--------------------------------------------------------------------------*/
int getargs (char *p, argval *value, int type)
{
    char *ptoken;
    char left[100];
	int size = 0;

    left[0] = '\0';

    /* Skip leading spaces */
    SKIPSPACE(p);

    /* get target string: (all chars up to eol or comment) */
    if (*p == '\0')
        ptoken = NULL;
    else
        ptoken = p;

	if (*p == '=')
		++p;
	else {
		mmPrintAll("Missing argument in the configuration file\n");
        return 0;
    }

	SKIPSPACE(p);
	ptoken = p;
    while (isalnum(*p) || *p == '-' || *p == '_' || *p == '.')
    {
        ++p;
        ++size;
    }
    strncpy (left, ptoken, size);
    left[size] = '\0';

	if(type == NUMVALUE)
	{
		if (sscanf (left, "%d", &value->numval) == 1)
  	         return SUCCESS;
		return 0;
	}
	if(type == HEXVALUE)
	{
		if (sscanf (left, "%x", &value->numval) == 1)
  	         return SUCCESS;
		return 0;
	}    
	else // STRINGVALUE
	{
		if (sscanf (left, "%s", &value->strval) == 1)
  	         return SUCCESS;
		return 0;
	}
} /* getargs */


/*---------------------------------------------------------------------------
 * StripComments - eliminates part of string starting with one of the 
 *    specified comment characters.
 * Returns found type if successful, 0 if not.
 *--------------------------------------------------------------------------*/
void StripComments(char *comment_chars, char *text)
{
    int quoted = 0;
    char *ps;

    for (ps = text; *ps != '\0'; ps++)
    {
        if (*ps == '"')
        {
            quoted = !quoted;
            continue;
        }
        if ((!quoted) && (strchr (comment_chars, *ps) != NULL))
        {
            *ps = '\n';
            ps++;
            *ps = '\0';
            break;
        }
    }
}


/* ***********************************************************************
   Function: mmPrint
   Description: Prints string on the screen if bVerbose = TRUE
                Prints string to the Log file if bAppLogFileSpecified = TRUE
				Otherwise does nothing
   Input: String to be printed
   Output: None
************************************************************************ */
void mmPrint(int Port, int Type, char *Format, ... )
{
	va_list va;

	if ((bVerbose == FALSE) && (bAppLogFileSpecified == 0))
		return;
		
	if( Type & VmTraceMask ) {
		va_start( va, Format);
		if (bVerbose == TRUE) {
			printf("%-3d : ", Port);
			vprintf(Format,va);
		}
/*		
		if (bAppLogFileSpecified) {
			printf("%3d : ", Port);			
			vfprintf(pLogFile, Format, va);
		}
*/		
	}
}

/* ***********************************************************************
   Function: mmPrintAll
   Description: Prints string on the screen independant of bVerbose setting
                Prints string to the Log file if bAppLogFileSpecified = TRUE
                
                Intended for printing errors and other things which should
                Always be displayed.
   Input: String to be printed
   Output: None
************************************************************************ */
void mmPrintAll(char *Format, ... )
{
	va_list va;
    
	va_start( va, Format);
	vprintf(Format, va);
/*	
	if (bAppLogFileSpecified)
		vfprintf(pLogFile, Format, va);
*/		
}

/* ***********************************************************************
   Function: mmPrintAErr
   Description: Prints string on the screen independant of bVerbose setting
                Prints string to the Log file if bAppLogFileSpecified = TRUE
				Prints "ERROR" the message.
                
                Intended for printing errors and other things which should
                Always be displayed.
   Input: String to be printed
   Output: None
************************************************************************ */
void mmPrintErr(char *Format, ... )
{
	va_list va;
    
	va_start( va, Format);
	printf("ERROR: ");
	vprintf(Format, va);
/*	
	if (bAppLogFileSpecified)
		vfprintf(pLogFile, Format, va);
*/		
}


void mmPrintPortErr(int Port, char *Format, ... )
{
	va_list va;
    
	va_start( va, Format);
	printf("%-3d : ERROR: ", Port);
	vprintf(Format, va);
	incErrors(Port);
/*	
	if (bAppLogFileSpecified)
		vfprintf(pLogFile, Format, va);
*/		
}

// Prints out the contents of a binary buffer
void mmPrintBinary(int Port, int Type, BYTE *buf, int size)
{
	int i=0;
	char str[80];
	char line[100];
    char ascii[20];
	str[0]='\0';
	line[0]='\0';
    ascii[0]='\0';

	sprintf(line, "%04x: ",i);
	for(i=0;i<size;i++)
			{
				if((i%16==0)&&(i!=0))
				{
					mmPrint(Port, Type, "%-59s  |%-16s|\n", line, ascii);
					line[0]='\0';
                    ascii[0]='\0';
					sprintf(str, "%04x: ",i);
					strcat(line, str);
				}
				if(i%8==0)
				{
					sprintf(str, "  ");
					strcat(line, str);
				}
				sprintf(str, " %02x", *(buf+i) &0x00ff   );
				strcat(line, str);
                // Is it a printable ascii char
                if( (*(buf+i)>31) && (*(buf+i)<127))
                    str[0] = *(buf+i);
                else
                    str[0] = 32;
                str[1] = 0;
                strcat(ascii, str);
			}
			mmPrint(Port, Type, "%-59s  |%-16s|\n", line, ascii);
}

/* ***********************************************************************
   Function: PrintConfiguration
   Description: Prints an updated configuration on the screen
   Input: None
   Output: None
************************************************************************ */
void PrintSrvConfiguration()
{
	char ch;

	mmPrintAll("******************************************\n");
	mmPrintAll("  Configuration Parameters for the Server:\n");
	mmPrintAll("Operation (0=Play, 1=Record)       : %d\n", mmParm.Operation);
	mmPrintAll("Number of Ports to be ran          : %d\n", mmParm.nPorts);
	mmPrintAll("Video Format                       : %d\n", mmParm.Video_Format);
	mmPrintAll("Audio RTP Format                   : %d\n", mmParm.Audio_RTP_Format);
	mmPrintAll("Audio Record Format                : %d\n", mmParm.Audio_Record_Format1);
	mmPrintAll("Source IP Address for Video        : %s\n", mmParm.srcIPAddr_video);
	mmPrintAll("Source Port for Video              : %d\n", mmParm.srcPort_video);
	mmPrintAll("Destination IP Address for Video   : %s\n", mmParm.destIPAddr_video);
	mmPrintAll("Destination Port for Video         : %d\n", mmParm.destPort_video);
	mmPrintAll("Source IP Address for Audio        : %s\n", mmParm.srcIPAddr_audio);
	mmPrintAll("Source Port for Audio              : %d\n", mmParm.srcPort_audio);
	mmPrintAll("Destination IP Address for Audio   : %s\n", mmParm.destIPAddr_audio);
	mmPrintAll("Destination Port for Audio         : %d\n", mmParm.destPort_audio);
	mmPrintAll("Timeslot                           : %d\n", mmParm.Timeslot);
	mmPrintAll("I-frame Enabled                    : %d\n", mmParm.enable_Iframe);
	mmPrintAll("Audio Stop Initial Timeout         : %d\n", mmParm.audioStop_InitTimeout);
	mmPrintAll("Audio Stop Silence Timeout         : %d\n", mmParm.audioStop_SilenceTimeout);
	mmPrintAll("Audio Stop Silence Level           : %d\n", mmParm.audioStop_SilenceLevel);
	mmPrintAll("Video Stop initial Timeout         : %d\n", mmParm.videoStop_InitTimeout);
	mmPrintAll("Video Stop No-Signal Timeout       : %d\n", mmParm.videoStop_NosignalTimeout);
	mmPrintAll("Maxtime                            : %d\n", mmParm.maxTime);
	mmPrintAll("Native Silence Detect              : %d\n", mmParm.native_silence_detect);
	mmPrintAll("Video Buffer Size                  : %d\n", mmParm.video_buffer_size);
	mmPrintAll("Audio Buffer Size                  : %d\n", mmParm.audio_buffer_size);
	mmPrintAll("Frame Quota                        : %d\n", mmParm.frameQuota);
	mmPrintAll("AutoRecord                         : %d\n", mmParm.bAutoRecord);
	mmPrintAll("AutoPlay                           : %d\n", mmParm.bAutoPlay);	
	mmPrintAll("AutoPlayList                       : %d\n", mmParm.bAutoPlayList);	
	mmPrintAll("Auto Play List File Name           : %s\n", mmParm.autoPlayListFileName);        
	mmPrintAll("3GP File Name                      : %s\n", mmParm.threegpFileName);
	mmPrintAll("Board type                         : %d\n", mmParm.nBoardType);	    
	mmPrintAll("******************************************\n");	
}



////////////////////////////////////////////////////////////////////////////////
//                            FLOW                                            
//  Utilities for printing the system call flow
////////////////////////////////////////////////////////////////////////////////
#define     FLOW_BOX_WIDTH  9
#define     FLOW_MAX_LINE   200

bool        gFlowInit = false;
t_FlowObj   gObjArray[FLOW_MAX_OBJ];
int         gCount;
int         gScreenWidth;
int         gObjCenter[FLOW_MAX_OBJ];   // Location of each of the object centers

char        gSzCleanLine[FLOW_MAX_LINE];
char*       gSzSpaceLine =
        "                                                                                                                                                                                                        ";
char*       gSzDashLine =
        "--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";
FILE       *gFlowFp;
    

/* ***********************************************************************
   Function: timeString()
   Description: returns a formatted time string
************************************************************************ */
char* timeString()
{
    static struct timeb the_time;
    static char TimeString[50] = "";
    static char TimeStringMs[50] = "";
	static time_t lasttime;
    struct tm *time_ptr;

	ftime(&the_time);
	if(lasttime != the_time.time)
	{
		lasttime = the_time.time;
		time_ptr = localtime(&the_time.time);
		strftime(TimeString, sizeof(TimeString), "%H:%M:%S",
				 time_ptr);
        sprintf(TimeStringMs, "%s.%03d", TimeString, the_time.millitm);
	}    
    return TimeStringMs;
}

/* ***********************************************************************
   Function: FlowInit
   Description: Initializes the way call flows will be printed to the screen
   Input: Screen width, Array of IDs and Names
   Output: None
************************************************************************ */
bool FlowInit( int nScreenWidth, int nCount, t_FlowObj ObjArray[FLOW_MAX_OBJ], char* szFileName)
{
    int i;
    char topLine[FLOW_MAX_LINE];
    char line[FLOW_MAX_LINE];
    int nSpace;                      // Space between each of the object boxes
    
    gCount = nCount;
    gScreenWidth = nScreenWidth;
    
    // if there is not enough space to print the call flow boxes on the screen
    if(nScreenWidth < FLOW_BOX_WIDTH*nCount + nCount -1)
        return false;
    if(nScreenWidth > FLOW_MAX_LINE)
        return false;
    
    	if ((gFlowFp = fopen(szFileName, "w")) == NULL)
	{
		return false;
	}
    // Figure out the amount of space between boxes
    nSpace = (nScreenWidth - FLOW_BOX_WIDTH*nCount)/(nCount-1);
    memcpy(gObjArray, ObjArray, FLOW_MAX_OBJ*sizeof(t_FlowObj));
    
    // Find the center point of each object
    strncpy(gSzCleanLine, gSzSpaceLine, nScreenWidth);
    gObjCenter[0] = FLOW_BOX_WIDTH/2;
    gSzCleanLine[gObjCenter[0]] = '|';
    for(i=1; i<nCount; i++)
    {
        gObjCenter[i] = gObjCenter[i-1] + FLOW_BOX_WIDTH + nSpace;
        gSzCleanLine[gObjCenter[i]] = '|';
    }
    
    // Draw the Header
    topLine[0]=0;
    strncat( topLine, gSzDashLine, FLOW_BOX_WIDTH);
    for(i=1; i<nCount; i++)
    {
        strncat( topLine, gSzSpaceLine, nSpace);
        strncat( topLine, gSzDashLine, FLOW_BOX_WIDTH);
    }
    fprintf(gFlowFp, "            :%08x: %s\n",0, topLine);
    
    line[0]=0;
    sprintf( line, "| %5s |", gObjArray[0].szObjName);
    for(i=1; i<nCount; i++)
    {
        strncat( line, gSzSpaceLine, nSpace);
        strncat( line, "| ", 2);
        strncat( line, gObjArray[i].szObjName, 5);
        strncat( line, " |", 2);
    }
    fprintf(gFlowFp, "            :%08x: %s\n",0, line);
    fprintf(gFlowFp, "            :%08x: %s\n",0, topLine);
    fprintf(gFlowFp, "            :%08x: %s\n",0, gSzCleanLine);
      
    gFlowInit = true;
    return true;
}

/* ***********************************************************************
   Function: Flow
   Description: Prints a flow arrow from the "from" object to the "to"
                object, and labels it with the event name.
   Output: None
************************************************************************ */
void Flow( int nChan, int from, int to, char* szEvnName )
{
    int fromInd = -1, toInd = -1;         // From and to index
    int stLen;                  // Length of the string
    int labelStart;
    char line[FLOW_MAX_LINE];   // Scratch line
    char l_szEvnName[FLOW_MAX_LINE];
    int i;
    int step = 1;               // Arrow Direction (1 = l2r, -1 = r2l)
    
    if(!gFlowInit)
        return;
    if(from == to)
        return;
    
    for( i=0; i<gCount; i++ )
    {
        if(from == gObjArray[i].nObjID)
        {
            fromInd = i;
            break;
        }
    }
    if(fromInd == -1)
        return;
    for( i=0; i<gCount; i++ )
    {
        if(to == gObjArray[i].nObjID)
        {
            toInd = i;
            break;
        }
    }
    if(toInd == -1)
        return;
    // Write the label line
    strncpy( line, gSzCleanLine, gScreenWidth+1 );
    
    //Truncate long labels
    stLen = strlen(szEvnName);
    if(stLen > gScreenWidth)
        stLen = gScreenWidth;
    strncpy ( l_szEvnName, szEvnName, gScreenWidth );

    // Locate the label, correct to prevent overhang, and print
    labelStart = (gObjCenter[fromInd] + gObjCenter[toInd] - stLen)/2;
    if(labelStart < 0)
        labelStart = 0;
    if(labelStart + stLen > gScreenWidth)
        labelStart = gScreenWidth - stLen;
    memcpy(&line[labelStart], l_szEvnName, stLen);
    fprintf(gFlowFp, "            :%08x: %s\n",nChan, gSzCleanLine);
    fprintf(gFlowFp, "            :%08x: %s\n",nChan, line);
    
    // draw the arrow
    if(gObjCenter[fromInd] > gObjCenter[toInd])
        step = -1;
    strncpy( line, gSzCleanLine, gScreenWidth+1 );
    for(i = gObjCenter[fromInd]+step; i != gObjCenter[toInd]- step; i+=step)
        line[i] = '-';
    if(step == -1)
        line[i] = '<';
    else
        line[i] = '>';
    fprintf(gFlowFp, "%s:%08x: %s\n", timeString(),nChan, line);
           
    
    return;
}
