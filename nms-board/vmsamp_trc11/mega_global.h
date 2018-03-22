//*****************************************************************************
//	mega_global.h
//
//	Global typedefs, structures, declarations and #defines for the Media  
//	Gateway Sample Application - megasamp
//
//*****************************************************************************
#include <stdio.h>
#include <string.h>
#if defined (WIN32)
#include <conio.h>
#elif defined (UNIX)
#include <unistd.h>
#include <termio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define GETCH() getchar()
#endif
#include <adidef.h>
#include <nccdef.h>
#include <swidef.h>
#include <ctadef.h>
#include <ctaerr.h>
#include <stdlib.h>
#include "ctype.h"
#include "mspdef.h"
#include "mspcmd.h"
#include "mspquery.h"
#include "mspobj.h"
#include "mspunsol.h"
#include <endian.h>
// ISDN Includes
#include "isdntype.h"
#include "isdndef.h"
#include "isdnparm.h"
#include "isdnval.h"

// Application Includes
#include "h324def.h"

#define MAX_DECODER_CFG_INFO_SZ 128

#ifdef USE_TRC
#include "trc_config.h"
#endif
#include "codecsdef.h"
#include "nmstypes.h"

#if defined(LINUX)
typedef long long hrtime_t;
#endif

#if defined(WIN32)
typedef DWORD hrtime_t;
#define THREADID unsigned
#else
#define THREADID pthread_t
#endif


///////////////////////////////
//////// DEFINES //////////////
///////////////////////////////
#define MAX_GATEWAYS        30     //Largest Number of Channels in a Gateway
#define MAX_TIMESLOTS       48     //Number of Timeslots used by MAX_GATEWAYS
#define MAX_FILENAME        50     //Largest Size of a filename (in characters)
#define NUM_CTA_SERVICES    5
#define FAILURE             0xffff
#define MAX_NAI             1       // 4

//Event type indicators
#define MEGA_KBD_EVENT   0
#define MEGA_CTA_EVENT   1
#define MEGA_EXIT_EVENT  2

//MSP OBJECT TYPES
#define MSP_MUX_EP              1
#define MSP_VID_RTP_EP          2
#define MSP_AUD_RTP_EP          3
#define MSP_AMR_RTP_EP          4
#define MSP_AMR_DS0_EP          5
#define MSP_G711_DS0_EP         6
#define MSP_VID_CHAN            7
#define MSP_AMR_CHAN            8
#define MSP_G711_CHAN           9
#define MSP_AMR_BY_CHAN         10
#define MSP_UNDEFINED           11

//MSP ACTION TYPES
#define MSP_SEND_CMD            1
#define MSP_SEND_QRY            2

// Call Control events (not used, only kept to maintain consistancy with the PBX)
#define PBX_EVENT_PLACE_CALL        0x20002001
#define PBX_EVENT_DISCONNECT        0x20002002
#define PBX_EVENT_PARTNER_CONNECTED 0x20002003

#define CDE_LOCAL_TROMBONE          0x20002004
#define CDE_REMOTE_TROMBONE         0x20002005
#define CDE_CONFIRM_TROMBONE        0x20002006
#define CDE_STOP_TROMBONE           0x20002007
#define CDE_STOP_REMOTE_TROMBONE    0x20002008
#define CDE_STOP_CONFIRM_TROMBONE    0x20002009
#define CDE_REMOTE_FAST_VIDEO_UPDATE    0x2000200A
#define CDE_REMOTE_SKEW_INDICATION    0x2000200B
#define CDE_STOP_VIDEO_RX           0x2000200C
#define CDE_STOP_REMOTE_VIDEO_RX    0x2000200D
#define CDE_REMOTE_VIDEOTEMPORALSPATIALTRADEOFF             0x2000200E

#define PBX_UNDEFINED       0
#define PBX_INBOUND         1
#define PBX_OUTBOUND        2

#define CALL_MODE_NOCC      0
#define CALL_MODE_ISDN      1

// MEDIA TYPES (AS USED BY THE DEMUX)
#define MEDIA_TYPE_DATA         0
#define MEDIA_TYPE_AUDIO        1
#define MEDIA_TYPE_VIDEO        2
#define MEDIA_TYPE_UNDEFINED    3

#define CG_BOARD_IP    "10.118.7.103"
#define GW_AUDIO_PORT  1000
#define GW_VIDEO_PORT  2000

// Trombone states
#define MESSAGING_STATE   0
#define TROMBONING_STATE  1
#define CONNECTING_STATE  2

//3GP
#define MAX_FILE_INFO_SIZE 5000

/////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MACROS //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
#define SWAP_ALL_BYTE(x) ((((x) & 0x00ff ) << 8) | ((x) >> 8 & 0x00ff))

#ifdef UNIX
#define THREADID       pthread_t
#define THREADRETURN   void *
#define THREADCALLCONV
#else
#define THREADID       unsigned
#define THREADRETURN   unsigned
#define THREADCALLCONV __stdcall
#endif

//////////////////////////////////////////////////////////////////////////////////
//////// TEMPORARY STRUCTURES UNTIL REAL STRUCTURE IS WRITTEN   //////////////////
//////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

///////////////////////////////
///// Enumerated Types ////////
///////////////////////////////
typedef enum { 
    IDL  = 0,  //   idle=0, 
    SZR,  //        seizure, 
    INC,  //        incoming, 
    RCD,  //        receivingDigits
    ACC,  //        accepting,
    ANS,  //        answering, 
    REJ,  //        rejecting, 
    CON,  //        connected,  
    DIS,  //        disconnected, 
    OTB,  //        outboundInit, 
    PLC,  //        placing, 
    PRO,  //        proceeding
    _x_    //       no_state_change
             } tLineState;

///////////////////////////////
//////// STRUCTURES ///////////
///////////////////////////////

// The MSP_ACTION structure contains the contents of a single MSPP command 
// or query.
typedef struct tag_MSP_ACTION
{
    WORD                    GWNo;           //Which gateway in the array
    WORD                    objType;        //Which type of GW or EP is being acted on
    WORD                    action;         //Command or Query
    DWORD                   command;        //Specific command or query sent
    DWORD                   cmdSize;        //Size of the data in the command buffer
    DWORD                   expectedEvent;  //Event to wait for after command
    char                    CmdBuff[258];   //Command data sent
} MSP_ACTION;

// The GW_EP structure contains all of the information required to define
// an MSPP endpoint in the gateway, including; address, parameters, handles
// and whether this EP is an active part of current test.
typedef struct tag_GW_EP
{
    BOOL                    bActive;
	BOOL                    bQueueEnabled;	
    CTAHD                   ctahd;      // CTAccess Context Handle
    MSPHD                   hd;
	MSP_ENDPOINT_ADDR       Addr;
    MSP_ENDPOINT_PARAMETER  Param;
	BOOL					bReady;
	BOOL					bEnabled;	
} GW_EP;

// The GW_CHAN structure contains all of the information required to define
// an MSPP channel in the gateway, including; address, parameters, handles
// and whether this channel is an active part of the current test.
typedef struct tag_GW_CHAN
{
    BOOL                    bActive;
	BOOL					bEnabled;
    CTAHD                   ctahd;      // CTAccess Context Handle
    MSPHD                   hd;
	MSP_CHANNEL_ADDR        Addr;
    MSP_CHANNEL_PARAMETER   Param;
} GW_CHAN;

typedef struct tag_GW_CALL
{
    int             dCXIndex;           // Index if this context in context array
    char            szDescriptor[14];   // Context descriptor
    tLineState      dCallState;		
    tLineState      dNewState;		
    NCC_CALL_STATUS sCallStat;          // NCC Call status info
    NCC_CALLHD      callhd;
    CTAHD           ctahd; 
    DWORD           dDirection;
} GW_CALL;

// For ISDN Contexts only
typedef struct tag_ISDN_CX
{
    CTAHD           ctahd;              // CTA context handle
    CTAQUEUEHD      QueueHd;            // CTA Queue
    char            szDescriptor[14];   // Context descriptor
    DWORD           dBoard;
	DWORD           dNai;            
    DWORD           dOperator;          
    DWORD           dCountry;         
	DWORD			dNT_TE;
} ISDN_CX ;

typedef struct tag_H323_LIB
{
    DWORD                   bActive;    // Do we run the H323 Library, or not?
    // Add here whatever is needed to interface with the H323 LIB
} H323_LIB;

typedef struct tag_DEMUX_STATS
{
    DWORD num_videoCRCerrors;
    DWORD num_audioCRCerrors;
} DEMUX_STATS; 

typedef struct {
    unsigned char data[MAX_DECODER_CFG_INFO_SZ];
    DWORD  len;
} tDCInfo;

//  The Media Gateway Config structure contains all parameter and address 
//  Information needed to define a media gateway.  This includes all parameters 
//  for each of the endpoints, and for the media channels 
typedef struct tag_MEDIA_GATEWAY_CONFIG
{
    DWORD                   dNumGW;        // Number of Gateways in the test
    CTAHD                   ctahd;         // CTAccess Context Handle
    DWORD                   CallMode;      // Place call, answer call or no call control
    DWORD                   CallState;  
    GW_EP                   MuxEp;         // MUX Endpoint

    // +JD: FDX/Simplex co-existence
    GW_EP                   vidRtpEp[2];   // Index 0 is FDX or simplex inbound; index 1 simplex outbound

    GW_EP                   AudRtpEp[2];   // Audio Bypass Channel AMR/G.723 (depends upon configuration)

    GW_CHAN                 vidChannel[2]; // Video Channel MPEG4/H263/H263+ (depends upon configuration)

    GW_CHAN                 AudByChan[2];  // AMR Bypass Channel AMR/G.723 (depends upon configuration)

    GW_CHAN                 G711Chan[2];
    // -JD: FDX/Simplex co-existence

#ifdef USE_TRC    
    VIDEO_XC                VideoXC;        // Video Transcoder
    BOOL                    bVideoChanRestart; 
/* dynamic VTP channel
   Boolean flag used to indicate the TRC video to be restart. 
     TRUE, video TRC channel need to be stoped and started. 
     FALSE, video TRC channel does not to be stopped/restarted.   
   used in trcEvent.cpp and srv_playrec.cpp 
*/
#endif
	
    DWORD                   txVideoCodec;   // TX Video codec negotiated with the remote terminal
    BOOL                    bGwVtpServerConfig; // Eps configuration GW-VTP-Server (FALSE by default)

    tDCInfo                 DCI;
            
    GW_CALL                 isdnCall;
    H323_LIB                h323Lib;
	CTAQUEUEHD              hCtaQueueHd;	
	DWORD					callNum;
	H324_TERM_CAPS			localTermCaps;
	
    WORD                    h324State;

    DWORD                   nTriesVideoOLC; // DRTDRT Temporary Location for this

    DEMUX_STATS             DemuxStats;	
    
	BOOL					shutdown_in_progress; // To ignore NCC_DISCONNECT in case of shutdown
    
    BOOL                    video_path_initialized[2];
    BOOL                    audio_path_initialized[2];
    
	hrtime_t          start;
	
	WORD              TromboneState;
	DWORD             trombonePartner;
} MEDIA_GATEWAY_CONFIG;

// The CMD LINE PARM structure contains values representing the contents 
// of the command line entries.
typedef struct tag_CMD_LINE_PARAM
{
    BOOL                    bCMDFile;
    BOOL                    bInteractive;
    DWORD                   nBoardNum;
    char                    GwConfigFileName[MAX_FILENAME];
    char                    SrvConfigFileName[MAX_FILENAME];	
    char                    CMDFileName[MAX_FILENAME];
} CMD_LINE_PARAMS;

typedef struct 
{
    int  dCXIndex;                      // Context Index of the inbound side
    char calledaddr [NCC_MAX_DIGITS+1]; /* Called number address */
    char callingaddr[NCC_MAX_DIGITS+1]; /* Calling number address */
    char callingname[NCC_MAX_CALLING_NAME]; /* Calling name info */
} PBX_PLACE_CALL_STRUCT;



typedef enum ep_type { MUX, DS0, RTP, NA } EP_TYPE;


////////////////////////////
// Declare GLOBAL VARIABLES
////////////////////////////
// Declare container for the command line parameters
extern CMD_LINE_PARAMS      CmdLineParms;
// Declare the container for all the media gateway information   
extern MEDIA_GATEWAY_CONFIG GwConfig[];
extern ISDN_CX              isdnCX[];
extern char                 *optarg;
extern int                  optind;
extern int                  opterr;
extern FILE                 *RecCmdFp;         // File Pointer to file of recorded commands
extern FILE                 *PlayCmdFp;         // File Pointer to file of recorded commands
extern FILE                 *RecH245Fp;         // File Pointer to file for recording H245

////////////////////////////
// Declare Common Functions
////////////////////////////
extern int  initGetCmdLineParms( int argc, char *argv[] );

extern int  initGateway();

extern int  initGatewayConfigFile( char *FileName );

extern DWORD WaitForSpecificEvent( int nGw, CTAQUEUEHD queue_hd, DWORD desired_event, CTA_EVENT *eventp );

extern DWORD CheckForUnsolicitedEvents( CTAHD CtaQueueHd, CTA_EVENT *eventp );

extern BOOL ProcessCTAEvent(int nGw, CTA_EVENT *pevent);

extern int  ProcessNCCEvent( CTA_EVENT *event, GW_CALL *pCtx );

extern void ReleaseMSPEvent( CTA_EVENT *pevent );

extern DWORD NMSSTDCALL ErrorHandler( CTAHD ctahd, DWORD error, char *errtxt, char *func );

extern int  getopt( int argc, char * const *argv, const char *optstring );

extern void PrintUsage( char *argv[] );

extern THREADRETURN THREADCALLCONV CommandLoop( void *param );

extern void PrintCmdMenu();

extern void shutdown();

extern int initAllIsdn( void );

extern int isdnStop( void );

extern int ProcessH324Event( CTA_EVENT *pevent, DWORD dGwIndex );

extern int ProcessTrcEvent( CTA_EVENT *pevent, DWORD dGwIndex );

extern int ProcessTromboneEvent( CTA_EVENT *pevent, DWORD dGwIndex );

extern void sendCDE( int gw_num, int cde, int value );

extern void autoDisconnectCall( int gw_num );

extern void autoPlaceCall( int gw_num, char *dial_number );

#ifdef LINUX
	extern void read_parameter(char *format, ... );
	#define READ_PARAM(f,p) read_parameter(f,p)
#else
	#define READ_PARAM(f,p) scanf(f,p)
#endif

#if defined (UNIX)
  int kbhit (void);                     /*  NT style kbhit */
#elif defined (WIN32)
  #define kbhit _kbhit
#endif


//Valli - dynamic Trc Channel Setup, modifications from VMAPP by ALS
extern DWORD CheckForXcoding(DWORD dGwIndex, tVideo_Format fileFormat);

// Full-duplex/Simplex RTP endpoint and channel support
// The outbound EP is index 0 whether FDX or simplex
// The inbound EP is index 0 if FDX, but index 1 if simplex
#define EP_OUTB (0)
#define EP_INB  ((gwParm.simplexEPs) ? 1 : 0)
