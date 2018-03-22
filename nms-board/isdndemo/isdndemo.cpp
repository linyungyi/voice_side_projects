/*****************************************************************************
 *  FILE            : isdndemo.cpp
 *  PROJECT         : isdndemo
 *  DESCRIPTION     : System configuration and initialization
 *
 *         
 *****************************************************************************
 * Copyright 2001 NMS Communications
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "isdndemo.h"
#include "ctademo.h"

#include "decisdn.h"

//---------------------------------------------------------------------------
//
//           Local function declaration
//
//---------------------------------------------------------------------------
void readConfig(int argc, char **argv);
void printHelp();
DWORD NMSSTDCALL cta_error_handler ( CTAHD, DWORD, char *,char *);

//----------------------------------------------------------------------------
//
//           Local variables which hold program configuration
//
//----------------------------------------------------------------------------
bool    exit_on_error;               // Exit program on Natural Access errors
int     verbose = V_CONFIG | V_CALL; // Program verbosity 
char    OPTIONS[]= "mXC:b:O:o:p:d:a:nD:VtsSQR:lL:I:g:T:v:0PhH?";

bool    enableOAM = true;            // Use OAM service 
//--------------------------------------------------------------------------
//
//  List of protocols supported by application
//
//----------------------------------------------------------------------------
struct {  int protocol; int country; char *name; } PROTOCOLS[]= 
{
        { ISDN_OPERATOR_ECMA_QSIG,  COUNTRY_EUR,    "QSIG" },
        { ISDN_OPERATOR_HK_TEL,     COUNTRY_HONG_KONG, "Honk Kong ISDN" },  
        { ISDN_OPERATOR_TAIWAN_OP,  COUNTRY_TAIWAN, "Taiwanese Operator"},
        { ISDN_OPERATOR_KOREAN_OP,  COUNTRY_KOREA,  "Korean Operator" },
        { ISDN_OPERATOR_NTT,        COUNTRY_JPN,    "Nippon Telegraph and Telephone (NTT)" },
        { ISDN_OPERATOR_AUSTEL_1,   COUNTRY_AUS,    "Australian ISDN" },
        { ISDN_OPERATOR_ETSI,       COUNTRY_EUR,    "EuroISDN" },
        { ISDN_OPERATOR_FT_VN6,     COUNTRY_EUR,    "France Telecom VN6" },
        { ISDN_OPERATOR_NT_DMS,     COUNTRY_USA,    "Northern Telecom DMS100" },
        { ISDN_OPERATOR_NT_DMS250,  COUNTRY_USA,    "Northern Telecom DMS250" },
        { ISDN_OPERATOR_NI2,        COUNTRY_USA,    "National ISDN 2" },
        { ISDN_OPERATOR_ATT_4ESS,   COUNTRY_USA,    "AT&T 4ESS" },
        { ISDN_OPERATOR_ATT_5E10,   COUNTRY_USA,    "AT&T 5E10" },
        { ISDN_OPERATOR_ANSI_T1_607,COUNTRY_USA,    "ANSI T1.607" },
        { ISDN_OPERATOR_DPNSS,      COUNTRY_EUR,    "DPNSS" },
        { 0, 0, 0 }
};



//--------------------------------------------------------------------------
//
//             main()
//
//   1. Initialize CTAccess
//   2. Initialize classes
//   3. Read configuration
//--------------------------------------------------------------------------
int main(int argc, char *argv[])
{		
    exit_on_error = true;

    debug_printf(V_CONFIG, "NMS-ISDN Messaging API demo program\n\n");

	//=========================================================================
    // Process global program options 
    //=========================================================================
	int c;
    while( ( c = getopt( argc, argv, OPTIONS ) ) != -1 )
    switch( c )
    {
	case 'h':
	case 'H':
	case '?': printHelp();       break;
	case '0': enableOAM = false; break;
    case 'v':
        if (sscanf(optarg,"%x",&verbose) != 1)
            error("Bad verbosity: %s\n",optarg);
        break;			
	}


    //=========================================================================
    // Initialize Natural Access
    //=========================================================================
    CTA_SERVICE_NAME servicelist[] =
    { 
        { "ADI",  "ADIMGR" },
        { "ISDN", "ADIMGR" },
        { "VCE",  "VCEMGR" },
        { "OAM",  "OAMMGR" }
    };
    CTA_INIT_PARMS initparms = { 0 };
    initparms.size       = sizeof(CTA_INIT_PARMS);
    initparms.ctaflags   = CTA_MODE_LIBRARY;
	if (enableOAM) initparms.ctaflags |= CTA_MODE_SERVER;
    
    ctaSetErrorHandler(cta_error_handler, NULL);
    ctaInitialize( servicelist, enableOAM ? 4 : 3, &initparms );
    
    Context::init();
    Board::init();

    //=========================================================================
    // Read program configuration and create contextes
    //=========================================================================
    readConfig (argc, argv);

    //=========================================================================
    // Start placing and receiving calls
    //=========================================================================
    exit_on_error = false;
    debug_printf(V_CONFIG,"\nStart placing and receiving calls ...\n\n");

    Context::eventLoop();
    

    return 0;
}


//---------------------------------------------------------------------------
//
//          Config::Config() 
//  
//  Set default configuration values
//---------------------------------------------------------------------------
Config::Config()
{
  nai                       = 0;
  group                     = -1;
  board                     = 0;
  network                   = false;
  protocol                  = 0;

  Q931                      = false;
  Transparent               = false;
  uui_flag                  = false; 
  mlpp_flag                 = false; 
  strcpy( dial_string, "1234567");
  calling_string[0]         = '\0'; // No calling string
  log                       = false;
  logtime                   = 60; // 1 minute 

  intercall                 = 1000;                     
  hold_time                 = 15000;              
  voice                     = false;
  exclusive                 = true;
  first_outbound_channel    = 0;
  number_outbound_ports     = 0;

  clear_cause               = 0;
}


//---------------------------------------------------------------------------
//
//          Config::copy() 
//  
//  Copy values from one configuration to another
//---------------------------------------------------------------------------
Config *Config::copy()
{
    Config *c = new Config();
    c->protocol = protocol;
    return c;
}

//--------------------------------------------------------------------------
//
//             get_number()
//
//   Convert string to integer and exit program on error. 
//   ( Used by readConfig() )        
//--------------------------------------------------------------------------
int get_number(const char *str, const char *message)
{
    int ret;
    if (sscanf(str,"%d",&ret) != 1 || ret <0)
        error("%s : wrong value %s, expected number\n",message,str);
    return ret;
}


//---------------------------------------------------------------------------
//
//         readConfig() 
//  
//  Process command line parameters
//---------------------------------------------------------------------------
void readConfig(int argc, char **argv)
{
  int c;
  Config *cfg = new Config();
  extern int optind;
  
  optind = 1; // Reset options counter after global options processing	
  
  while( ( c = getopt( argc, argv, OPTIONS ) ) != -1 )
    switch( c )
    {
      case 'Q': 
          cfg->Q931 = true;
          break;

      case 'n': 
          cfg->network = true;
          break;

      case 't': 
          cfg->Transparent = true;
          break;

      case 'V': 
          cfg->voice = true;
          break;

      case 'P': 
          cfg->exclusive = false;
          break;

      case 'S': 
          Context::server_mode = true;     
          break;
  
      case 'l': 
          cfg->log = true;                
          break;

      case 'm': 
          cfg->mlpp_flag = true;                
          break;

      case 'b':                
        cfg->board = get_number(optarg, "board number");  
        break;

      case 'C':
        cfg->clear_cause = get_number(optarg, "call clear cause");  
        break;

      case 'g':
		if(!enableOAM) error("NFAS is not supported without OAM service\n");
        cfg->group = get_number(optarg, "NFAS group number");  
        break;

      case 'L':
        cfg->logtime = get_number(optarg, "logging interval");  
        break;

      case 'O':
        cfg->first_outbound_channel = 
                     get_number(optarg, "First outbound channel");  
        break;

      case 'a':
        cfg->nai = get_number(optarg, "nai number");            
        break;

      case 'o':
        cfg->number_outbound_ports = 
                   get_number(optarg, "number of outbound ports");
        break;

      case 'I':
        cfg->intercall = get_number(optarg,"intercall delay");
        break;

      case 'p':                          
        cfg->protocol = get_number(optarg, "protocol");  
        break;

      case 'T':
        cfg->hold_time = get_number (optarg, "call length");
        break;

      case 'd':
        sscanf(optarg, "%32s", cfg->dial_string);
        break;

      case 'D':
        sscanf(optarg, "%32s", cfg->calling_string);
        break;

      case 'X':
        new DChannel(cfg );
        cfg = cfg->copy();
        break;
            
      default:
		error("Unrecognizable option '%c'\n", c);
		
      case '0': 
      case 'v':
		// Global options
        break;
    }  

    new DChannel( cfg );
}

//--------------------------------------------------------------------------
//
//             printHelp()
//
//   Prints program help and exit program.
//--------------------------------------------------------------------------
void printHelp()
{
    printf(
            "\nISDN specific keys:\n\n" 
            "  -b <board number>                default    = 0\n"
            "  -a <NAI>                         default    = 0\n"
            "  -g <NFAS group>                  default    = no NFAS\n"
            "  -n NT side                       default    = TE side\n"
            "  -p <protocol>                    default    = AT&T 4ESS for T1\n"
            "                                   default    = EuroISDN  for E1\n"
    );

    for(int i=0; PROTOCOLS[i].protocol; i++)
       printf("     %2d - %s\n", PROTOCOLS[i].protocol, PROTOCOLS[i].name);

    printf(
            "\n"
            "Call placing and receiving keys:\n\n"
            "  -o <number of out-bound ports>   default    = 0\n"
            "  -O <first out-bound channel>     default    = 0\n"
            "  -d <string to dial>              default    = 1234567\n"
            "  -D <calling number>              default    = no\n"
            "  -T <Out call length in msec>     default    = 15000\n"
            "  -I <Intercall time in msec>      default    = 1000 msec\n"
            "  -C <call clear cause value>      default    = 0\n"
            "  -t Send transparently a codeset 7 IE in connection response\n"
            "  -m Send MLPP Precedence Level in SETUP\n"
            "  -Q Receive Q.931 buffers\n"
            "  -V Play voice file\n"
            "  -P Place calls in preferred mode default    = exclusive\n"
            "\n"
            "Program operation keys:\n\n"
            "  -X This key is used to start more then one D-channel. All keys after\n" 
            "     this one are for next D-channel\n"
            "  -l Log statistics to a file      default    = NO\n"
            "  -L <logging interval in sec>     default    = 60\n"
            "  -S Set SERVER mode operatoion    default    = LIBRARY\n"
			"  -0 Disable use of OAM service    default    = enable\n"
            "  -v <hex verbosity level >        default    = 0x03\n"
            "     0x01  - program configuration\n"
            "     0x02  - calls status messages\n"
            "     0x04  - ACU messages\n"
            "     0x08  - calls statistics\n"
    );
    exit(1);		
}

//--------------------------------------------------------------------------
//
//             error()
//
//   Prints error message and exit program. Message has prinf format.
//--------------------------------------------------------------------------
void error(const char *format, ... )
{
    va_list args;
    va_start( args, format );
    fprintf(stderr,"error: ");
    vfprintf(stderr,format,args);
    va_end( args );
    fprintf(stderr,"\n\n");
    exit(1);
};




//--------------------------------------------------------------------------
//
//             error_cta()
//
//   Prints error message and exit program. Message has prinf format.
//--------------------------------------------------------------------------
void error_cta(unsigned code,const char *format, ... )
{
    char text[256];
    va_list args;

    va_start( args, format );

    fprintf(stderr,"error: ");
    vfprintf(stderr,format,args);

    ctaGetText(NULL_CTAHD, code,  text, sizeof(text));
    fprintf(stderr,": %s (0x%08x)\n", text, code);

    va_end( args );
    if (exit_on_error) exit(1);
};

//--------------------------------------------------------------------------
//
//             cta_error_handler()
//
//   Print Natural Access error message and exit (if necessary)
//--------------------------------------------------------------------------
DWORD NMSSTDCALL 
  cta_error_handler ( CTAHD ctahd, DWORD err_no, char *errtxt,char *func )
{
    if (exit_on_error)
        error("%s(): %s (0x%08x)\n",func, errtxt, err_no);
    else
        printf("%s(): %s (0x%08x)\n",func, errtxt, err_no);
    return 0;
}

//--------------------------------------------------------------------------
//
//             debug_printf(), debug_vprintf()
//
// Print message to the screen acording to verbose mode
//--------------------------------------------------------------------------
void debug_printf(int mode, const char *format, ... )
{
    va_list args;
    va_start( args, format );
    debug_vprintf(mode, format, args);
    va_end( args );
};

void debug_vprintf(int mode, const char *format, va_list args )
{
    if ( (mode & verbose) != mode ) return;
    vprintf(format,args);
};



//--------------------------------------------------------------------------
//
//  These are methods for ACY constant conversion to strings
//
//--------------------------------------------------------------------------
#define CASE(x) case x : return  #x

const char *getACU(int acu)
 {
    switch ( acu )
    {
    CASE( ACU_CONN_RQ );
    CASE( ACU_CONN_IN );
    CASE( ACU_CONN_RS );
    CASE( ACU_CONN_CO );
    CASE( ACU_CLEAR_RQ );
    CASE( ACU_CLEAR_IN );
    CASE( ACU_CLEAR_RS );
    CASE( ACU_CLEAR_CO );
    CASE( ACU_ALERT_RQ );
    CASE( ACU_ALERT_IN );
    CASE( ACU_PROGRESS_RQ );
    CASE( ACU_PROGRESS_IN );
    CASE( ACU_INFO_RQ );
    CASE( ACU_INFO_CO );
    CASE( ACU_INIT_RQ );
    CASE( ACU_INIT_CO );
    CASE( ACU_SETPARM_RQ );
    CASE( ACU_SETPARM_CO );
    CASE( ACU_USER_INFO_RQ );
    CASE( ACU_USER_INFO_IN );
    CASE( ACU_SUSPEND_RQ );
    CASE( ACU_SUSPEND_CO );
    CASE( ACU_RESUME_RQ );
    CASE( ACU_RESUME_CO );
    CASE( ACU_TEST_RQ );
    CASE( ACU_TEST_CO );
    CASE( ACU_DIGIT_RQ );
    CASE( ACU_DIGIT_IN );
    CASE( ACU_DIGIT_CO );
    CASE( ACU_FACILITY_RQ );
    CASE( ACU_FACILITY_IN );
    CASE( ACU_SET_MODE_RQ );
    CASE( ACU_SET_MODE_CO );
    CASE( ACU_RS_MODE_RQ );
    CASE( ACU_RS_MODE_CO );
    CASE( ACU_INFORMATION_RQ );
    CASE( ACU_INFORMATION_IN );
    CASE( ACU_SETUP_REPORT_IN );
    CASE( ACU_CALL_PROC_RQ );
    CASE( ACU_CALL_PROC_IN );
    CASE( ACU_NOTIFY_RQ );
    CASE( ACU_NOTIFY_IN );
    CASE( ACU_SETUP_ACK_IN );
    CASE( ACU_D_CHANNEL_STATUS_RQ );
    CASE( ACU_D_CHANNEL_STATUS_IN );
    CASE( ACU_ERR_IN );
    CASE( ACU_SERVICE_RQ );
    CASE( ACU_SERVICE_IN );
    CASE( ACU_SERVICE_CO );
    CASE( ACU_RESTART_IN );
    CASE( ACU_RESTART_CO );
    default: return "UNKNOWN";
    }
}

const char *getACURC(int acurc) { 
    switch( acurc )
    {
    CASE( ACURC_BUSY );
    CASE( ACURC_NOPROCEED );
    CASE( ACURC_NOANSWER );
    CASE( ACURC_NOAUTOANSWER );
    CASE( ACURC_CONGESTED );
    CASE( ACURC_INCOMING );
    CASE( ACURC_NOLINE );
    CASE( ACURC_ERRNUM );
    CASE( ACURC_INHNUM );
    CASE( ACURC_2MNUM );
    CASE( ACURC_HUNGUP );
    CASE( ACURC_NETWORK_ERROR );
    CASE( ACURC_TIMEOUT );
    CASE( ACURC_BAD_SERVICE );
    CASE( ACURC_INTERNAL );
    default: return "UNKNOWN";
    }
};


const char *getACUERR(int acuerr) 
{
    switch ( acuerr )
    {
    CASE( ACUER_PRIMITIVE_CODE );
    CASE( ACUER_PARAM_VAL );
    CASE( ACUER_MANDATORY_PARAM_MISSING );
    CASE( ACUER_PARAM_TYPE );
    CASE( ACUER_PARAM_LGTH );
    CASE( ACUER_UNEXPECTED_PRIMITIVE );
    CASE( ACUER_PRIMITIVE_NOT_IMPLEMENTED );
    CASE( ACUER_NO_TIMER_AVAILABLE );
    CASE( ACUER_CONGESTION );
    default: return "UNKNOWN";
    };
}


const char *getCAU(int cau) 
{ 
    switch ( cau )
    {
    CASE( CAU_UNALL );
    CASE( CAU_NOR_STN );
    CASE( CAU_NOR_D );
    CASE( CAU_CH_UNACC );
    CASE( CAU_AWARD );
    CASE( CAU_NORMAL_CC );
    CASE( CAU_BUSY );
    CASE( CAU_NO_USER_RES );
    CASE( CAU_NO_ANSW );
    CASE( CAU_REJ );
    CASE( CAU_NUM_CHANGED );
    CASE( CAU_NON_SEL );
    CASE( CAU_DEST_OOF );
    CASE( CAU_INV );
    CASE( CAU_FAC_REJ );
    CASE( CAU_RES_TO_SE );
    CASE( CAU_NORMAL_UNSPEC );
    CASE( CAU_NO_CIRC_CHAN );
    CASE( CAU_NET_OOF );
    CASE( CAU_TEMP_FAIL );
    CASE( CAU_CONG );
    CASE( CAU_ACC_INF_DISC );
    CASE( CAU_NOT_AVAIL );
    CASE( CAU_RES_UNAVAIL );
    CASE( CAU_QOF_UNAVAIL );
    CASE( CAU_FAC_NOT_SUB );
    CASE( CAU_BC_NOT_AUT );
    CASE( CAU_BC_NOT_AVAIL );
    CASE( CAU_SERV_NOTAVAIL );
    CASE( CAU_BC_NOT_IMP );
    CASE( CAU_CHT_NOT_IMP );
    CASE( CAU_FAC_NOT_IMP );
    CASE( CAU_RESTR_BC );
    CASE( CAU_SERV_NOT_IMP );
    CASE( CAU_INV_CRV );
    CASE( CAU_CH_NOT_EX );
    CASE( CAU_CALL_ID_NOT_EX );
    CASE( CAU_CALL_ID_IN_USE );
    CASE( CAU_NO_CALL_SUSP );
    CASE( CAU_CALL_CLEARED );
    CASE( CAU_INCOMP_DEST );
    CASE( CAU_INV_TN );
    CASE( CAU_INV_MSG );
    CASE( CAU_MAND_IE_MISSING );
    CASE( CAU_MSGT_NOT_EX );
    CASE( CAU_MSG_NOT_COMP );
    CASE( CAU_IE_NOT_EX );
    CASE( CAU_INV_IE_CONTENTS );
    CASE( CAU_MSG_NOT_COMP_CS );
    CASE( CAU_RECOVERY );
    CASE( CAU_PROTO_ERR );
    CASE( CAU_INTERW );
    CASE( CAU_PREEMPTION );
    CASE( CAU_PREEMPTION_CRR );
    CASE( CAU_PREC_CALL_BLK );
    CASE( CAU_BC_INCOMP_SERV );
    CASE( CAU_OUT_CALLS_BARRED );
    CASE( CAU_SERV_VIOLATED );
    CASE( CAU_IN_CALLS_BARRED );
    CASE( CAU_DEST_ADD_MISSING );
    CASE( CAU_RESTART );
    CASE( CAU_TIMER_300 );
    CASE( CAU_TIMER_302 );
    CASE( CAU_TIMER_303 );
    CASE( CAU_TIMER_304 );
    CASE( CAU_TIMER_308 );
    CASE( CAU_TIMER_309 );
    CASE( CAU_TIMER_310 );
    CASE( CAU_TIMER_316 );
    CASE( CAU_TIMER_317 );
    CASE( CAU_TIMER_318 );
    CASE( CAU_TIMER_319 );
    CASE( CAU_TIMER_399 );
    default: return "UNKNOWN";
    };
}
