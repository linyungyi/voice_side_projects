/*****************************************************************************
*  FILE: isdncta.c
*
*  DESCRIPTION: basic ISDN demo for PRI handling
*      This program start the ISDN stack to manage the D channel of a PRI
*
* Copyright 1997-2001 NMS Communications.
*****************************************************************************/

/* $$Revision "%v" */
#define VERSION_NUM  "5"
#define VERSION_DATE __DATE__

/*---------------------------- Standard Includes ----------------------------*/
#include <stdlib.h>
#include <stdio.h>

#ifndef UNIX
#include <conio.h>
#endif

/*---------------------------- Natural Access Includes ---------------------------*/
#include "ctademo.h"

/*------------------------------ ISDN includes  -----------------------------*/
#include "isdntype.h"
#include "isdndef.h"
#include "isdnparm.h"
#include "isdnval.h"

/*----------------------------- Defines ------------------------------------*/
#if defined (UNIX)
  #define POLL_TIMEOUT 0
  #define POLL_ERROR -1
#endif

/*----------------------------- Application Definitions --------------------*/
typedef struct
{
    CTAQUEUEHD  ctaqueuehd;       /* Natural Access queue handle                 */
    CTAHD       ctahd;            /* Natural Access context handle               */
    SWIHD       swihd;            /* Switching handle                       */
    int         ag_board;         /* board number                           */
    DWORD       nai;              /* Network Access ID                      */
    unsigned    partner_equip;    /* if we are TE our partner is NT         */
  							      /* if we are NT our partner is TE		    */
    unsigned    nfas_group;       /* NFAS group number, needed for          */
                                  /* configurations with duplicate Interface ID */
} MYCONTEXT;

/*----------------------------- Function Prototypes ------------------------*/
void PrintHelp ( void );
void IsdnStart ( MYCONTEXT *cx );
void IsdnStop  ( MYCONTEXT *cx );
void ParseArguments ( int argc, char **argv ); 
void PrintInfo ( void );
void MyMakeConnection ( MYCONTEXT *cx );
int  WaitForKeyboardEvent( CTAQUEUEHD ctaqueuehd, CTA_EVENT *eventp );

/*----------------------------- Global Variables ---------------------------*/
  unsigned  netoperator;		/* the network operator variant to start    */
  unsigned  country;			
  MYCONTEXT *cntx;
  BYTE      errortext[40];
  int		switching_mode;		/* do default switching						*/
  WORD      in_calls_behaviour = 0;
  WORD      out_calls_behaviour = 0;
  WORD      ns_behaviour = 0;
  WORD      acu_behaviour = 0;
  WORD      T309_option = 0;
  WORD      nfas_group_defined = 0;
  BYTE      *services = 0; 

  BYTE      default_services[] = {
	VOICE_SERVICE,
	FAX_SERVICE,
	FAX_4_SERVICE,
	DATA_SERVICE,
	DATA_56KBS_SERVICE,
	DATA_TRANS_SERVICE,
	MODEM_SERVICE,
	AUDIO_7_SERVICE,
	X25_SERVICE,
	X25_PACKET_SERVICE,
	VOICE_GCI_SERVICE,
	VOICE_TRANS_SERVICE,
	V110_SERVICE,
	V120_SERVICE,
	VIDEO_SERVICE,
	TDD_SERVICE,
	0
 };
/*----------------------------------------------------------------------------
                            main
----------------------------------------------------------------------------*/
int main ( int argc, char **argv )
{
  unsigned ret,i;
  CTA_EVENT eventp;
  char contextname[6];

  CTA_INIT_PARMS initparms = { 0 };	    /* for ctaInitialize */
  CTA_SERVICE_NAME servicelist[] =      /* for ctaInitialize */
  {   { "ADI",  "ADIMGR" },
      { "ISDN", "ADIMGR" },
	  { "SWI",  "SWIMGR" }
  };
  unsigned servicenumber = sizeof(servicelist)/sizeof(servicelist[0]);
  CTA_SERVICE_DESC services[] =        /* for ctaOpenServices */
  {   { {"ADI",  "ADIMGR"}, { 0 }, { 0 }, { 0 } },
      { {"ISDN", "ADIMGR"}, { 0 }, { 0 }, { 0 } },
      { {"SWI",  "SWIMGR"}, { 0 }, { 0 }, { 0 } }
  };
  unsigned numservices = sizeof(services)/sizeof(services[0]);


  printf ("ISDN demon for Natural Access V.%s (%s)\n", VERSION_NUM, VERSION_DATE);

  /* do some default initialization */
  netoperator = ISDN_OPERATOR_ATT_5E10;
  country = 0;
  switching_mode =  0;				/* default = EXCLUSIVE */

  /* allocate and initialize the context */
  cntx = (MYCONTEXT *)malloc (sizeof(MYCONTEXT));
  cntx->ctaqueuehd = NULL_CTAQUEUEHD;   /* initialize to safe value */
  cntx->ctahd = NULL_CTAHD;             /* initialize to safe value */
  cntx->partner_equip = EQUIPMENT_NT;  
  cntx->ag_board = 0;  
  cntx->nai = 0;
  cntx->nfas_group = 0; 


  ParseArguments( argc, argv );

  /* 
   * Natural Access initialization:
   * Register an error handler to display errors returned from API functions;
   * no need to check return values 
   */

  ctaSetErrorHandler( DemoErrorHandler, NULL );

  /* Initialize size of init parms structure */
  initparms.size = sizeof(CTA_INIT_PARMS);
  
  DemoReportComment( "Initializing and opening the Natural Access context..." );

  /* ctaInitialize */
  ret = ctaInitialize( servicelist, servicenumber, &initparms );
  if (ret != SUCCESS)
  {  
     printf("\tctaInitialize failure");
	 exit( 1 );
  }

  /* open services */
  /* fill in some stuff for services */
  for ( i = 0; i < numservices; i++ )
  {  
    services[i].mvipaddr.board = cntx->ag_board;
    services[i].mvipaddr.mode = 0;
  }
  /* Create the name of the context */
  strcpy( contextname, "isdn");

  /* use the function in ctademo to open and initialize the context              */
  DemoOpenPort( (DWORD) cntx->ag_board,	/* use the number of the board as userid */ 
	            contextname, 
				services,
                numservices, 
				&(cntx->ctaqueuehd), 
				&(cntx->ctahd) );

  /* if mode = switching make the MVIP default connections */
  if ( switching_mode )
    MyMakeConnection ( cntx );

  /* start ISDN stack */
  IsdnStart( cntx );

  PrintInfo();					/* inform the user of the started variant    */
      

  printf("\n\tEnter 'q' to exit\n"); 

  while (1)
    if ( WaitForKeyboardEvent( cntx->ctaqueuehd, &eventp ) == 'q' )
    {
      IsdnStop( cntx );
      exit ( 0 );
    }

  return 0;
}

/*----------------------------------------------------------------------------
							PrintHelp

	Prints the list of values for the command line options
----------------------------------------------------------------------------*/
void PrintHelp ( void )
{
    printf( "Usage: isdncta [opts]\n" );
    printf( "where opts are:\n" );
    printf( "-b n        AG board number (Terminal Equipment side)\n");
    printf( "-B n        AG board number (Network Terminator side)\n");
    printf( "-a n        Network Access Identifier\n");
    printf( "-g n        NFAS group number (if NAI values are duplicate)\n");
    printf( "-I n        in_calls_behaviour in hex (optional)\n");
    printf( "    CC_TRANSPARENT_OVERLAP_RCV = 0x%04x\n",CC_TRANSPARENT_OVERLAP_RCV);
    printf( "    CC_SET_CHAN_ID             = 0x%04x\n",CC_SET_CHAN_ID);
    printf( "-O n        out_calls_behaviour in hex (optional)\n");
    printf( "    CC_USER_SENDING_COMPLETE   = 0x%04x\n",CC_USER_SENDING_COMPLETE);
    printf( "    CC_USE_MU_LAW              = 0x%04x\n",CC_USE_MU_LAW);
    printf( "    CC_USE_A_LAW               = 0x%04x\n",CC_USE_A_LAW);
    printf( "-N n        ns_behaviour in hex (optional)\n");
    printf( "    NS_SEND_USER_CONNECT_ACK   = 0x%04x (ETSI only)\n",NS_SEND_USER_CONNECT_ACK);
    printf( "    NS_EXPLICIT_INTERFACE_ID   = 0x%04x\n",NS_EXPLICIT_INTERFACE_ID);
    printf( "    NS_PRESERVE_EXT_BIT_IN_CHAN_ID = 0x%04x (DMS only)\n",NS_PRESERVE_EXT_BIT_IN_CHAN_ID);
    printf( "    NS_ACCEPT_UNKNOWN_FAC_IE   = 0x%04x\n", NS_ACCEPT_UNKNOWN_FAC_IE );
    printf( "    NS_IE_RELAY_BEHAVIOUR      = 0x%04x\n", NS_IE_RELAY_BEHAVIOUR    );
    printf( "    NS_PBX_XY                  = 0x%04x (DPNSS only)\n", NS_PBX_XY );
    printf( "-A n        acu_behaviour in hex (optional)\n");
    printf( "    ACU_SEND_Q931_BUFFER       = 0x%04x\n", ACU_SEND_Q931_BUFFER );
    printf( "    ACU_SEND_UNKNOWN_FACILITY  = 0x%04x\n", ACU_SEND_UNKNOWN_FACILITY );
	
    printf( "-T          t309 option for D channel backup configurations\n");
    printf( "-s          switching mode:  use this option to create\n"
		"                  the MVIP default connections if EnableMvip = YES\n");
    printf( "-o n        Network operator variant\n"  
            "              France Telecom VN6             = %d\n"
            "              Northern Telecom DMS100        = %d\n"
            "              Northern Telecom DMS250        = %d\n"
            "              Nippon Telegraph and Telephone = %d\n"
            "              EuroISDN                       = %d\n"
            "              Australian Telecom 1           = %d\n"
            "              ECMA - QSIG                    = %d\n"
            "              Hong Kong Telecom              = %d\n"
            "              National ISDN 2                = %d\n"
            "              AT&T 5ESS10                    = %d   (default)\n"
            "              AT&T 4ESS                      = %d\n"
            "              Korean Operator                = %d\n"
            "              Taiwanese Operator             = %d\n"
            "              ANSI T1.607                    = %d\n"
            "              DPNSS                          = %d\n",
		    ISDN_OPERATOR_FT_VN6,
		    ISDN_OPERATOR_NT_DMS,
		    ISDN_OPERATOR_NT_DMS250,
		    ISDN_OPERATOR_NTT,
		    ISDN_OPERATOR_ETSI,
		    ISDN_OPERATOR_AUSTEL_1,
			ISDN_OPERATOR_ECMA_QSIG,
		    ISDN_OPERATOR_HK_TEL,
		    ISDN_OPERATOR_NI2,
		    ISDN_OPERATOR_ATT_5E10,
			ISDN_OPERATOR_ATT_4ESS,
			ISDN_OPERATOR_KOREAN_OP,
            ISDN_OPERATOR_TAIWAN_OP,
            ISDN_OPERATOR_ANSI_T1_607,
            ISDN_OPERATOR_DPNSS
				
	);
    printf( "-c country  Supported countries are\n"
			"              USA                            = %d\n"
		    "              Belgium                        = %d\n"
		    "              France                         = %d\n"
			"              Great Britain                  = %d\n"
			"              Sweden                         = %d\n"
			"              Germany                        = %d\n"
			"              Australia                      = %d\n"
			"              Japan                          = %d\n"
			"              Korea                          = %d\n"
			"              Taiwan                         = %d\n"
			"              China                          = %d\n"
			"              Singapore                      = %d\n"
			"              Hong Kong                      = %d\n"
			"              Europe                         = %d\n",
			COUNTRY_USA,
			COUNTRY_BEL,
			COUNTRY_FRA,
			COUNTRY_GBR,
			COUNTRY_SWE,
			COUNTRY_GER,
			COUNTRY_AUS,
			COUNTRY_JPN,
			COUNTRY_KOREA,
            COUNTRY_TAIWAN,
			COUNTRY_CHINA,
			COUNTRY_SINGAPORE,
			COUNTRY_HONG_KONG,
			COUNTRY_EUR);
	printf ( "-S service_list   accept only specified services \n");
	printf ( "-S #services_list accept all services but specified\n");
#define printService(x) printf("              %c - " #x "\n",x)

	printService(VOICE_SERVICE);
	printService(FAX_SERVICE);
	printService(FAX_4_SERVICE);
	printService(DATA_SERVICE);
	printService(DATA_56KBS_SERVICE);
	printService(DATA_TRANS_SERVICE);
	printService(MODEM_SERVICE);
	printService(AUDIO_7_SERVICE);
	printService(X25_SERVICE);
	printService(X25_PACKET_SERVICE);
	printService(VOICE_GCI_SERVICE);
	printService(VOICE_TRANS_SERVICE);
	printService(V110_SERVICE);
	printService(V120_SERVICE);
	printService(VIDEO_SERVICE);
	printService(TDD_SERVICE);

    printf( "-h or -?    Display this help screen\n");
}

/*----------------------------------------------------------------------------
							ParseArguments 

	Parses and checks the command line options
----------------------------------------------------------------------------*/
void ParseArguments ( int argc, char **argv )
{
  int c,i,temp;

  while( ( c = getopt( argc, argv, "a:b:B:g:o:c:I:O:N:A:sTS:Hh?" ) ) != -1 )
  {
    switch( c )
    {      
      case 'a':
        sscanf( optarg, "%d", &i );
        cntx->nai = i; 
        break;
      case 'b':
        sscanf( optarg, "%d", &i );
        cntx->partner_equip = EQUIPMENT_NT;  
        cntx->ag_board = i;  
        break;
      case 'B':
        sscanf( optarg, "%d", &i );
        cntx->partner_equip = EQUIPMENT_TE;  
        cntx->ag_board = i;  
        break;
      case 'g':
        sscanf( optarg, "%d", &i );
        cntx->nfas_group = i; 
        nfas_group_defined = 1;
        break;
      case 's':
		switching_mode = 1;
        break;
      case 'I':
		 /* sscanf reads an int, but I need a WORD here */
		 sscanf( optarg, "%x", &temp);
		 in_calls_behaviour = (WORD) temp;
        break;
      case 'O':
		 /* sscanf reads an int, but I need a WORD here */
		 sscanf( optarg, "%x", &temp);
		 out_calls_behaviour = (WORD) temp;
        break;
      case 'N':
		 /* sscanf reads an int, but I need a WORD here */
		 sscanf( optarg, "%x", &temp);
		 ns_behaviour = (WORD) temp;
        break;
      case 'A':
		 /* sscanf reads an int, but I need a WORD here */
		 sscanf( optarg, "%x", &temp);
		 acu_behaviour = (WORD) temp;
         break;
      case 'T':
		T309_option = 1;
        break;    

      case 'S':
		services = (BYTE *)optarg;
        break;    

      case 'o':
        sscanf( optarg, "%d", &netoperator );
		switch (netoperator)
        {
		  case ISDN_OPERATOR_FT_VN6:
		  case ISDN_OPERATOR_NT_DMS:
		  case ISDN_OPERATOR_NT_DMS250:
		  case ISDN_OPERATOR_NTT:
		  case ISDN_OPERATOR_ETSI:
		  case ISDN_OPERATOR_AUSTEL_1:
		  case ISDN_OPERATOR_HK_TEL:
		  case ISDN_OPERATOR_NI2:
		  case ISDN_OPERATOR_ATT_4ESS:
		  case ISDN_OPERATOR_ECMA_QSIG:
		  case ISDN_OPERATOR_ATT_5E10:
		  case ISDN_OPERATOR_KOREAN_OP:
          case ISDN_OPERATOR_TAIWAN_OP:
          case ISDN_OPERATOR_ANSI_T1_607:
          case ISDN_OPERATOR_DPNSS:
            break;
          default:
            printf("? Unrecognized Network Operator %d\n", netoperator);
			PrintHelp();
			exit(1);
      	  break;
		}
        break;
      case 'c':
        sscanf( optarg, "%d", &country );
        break;
      case 'h':
      case 'H':
      case '?':
        PrintHelp();
        exit( 1 );
        break;
      default:
        printf("? Unrecognized option -%c %s\n", c, optarg );
        exit( 1 );
        break;
    }
  }
  if ( !country )	/* the user didn't choose any coutries. Pick a default */
    switch ( netoperator )	  
    {
	  case ISDN_OPERATOR_FT_VN6:
		country=COUNTRY_FRA;
		break;
	  case ISDN_OPERATOR_NT_DMS:
	  case ISDN_OPERATOR_NT_DMS250:
	  case ISDN_OPERATOR_NI2:
	  case ISDN_OPERATOR_ATT_5E10:
      case ISDN_OPERATOR_ATT_4ESS:
      case ISDN_OPERATOR_ANSI_T1_607:
		country=COUNTRY_USA;
		break;
	  case ISDN_OPERATOR_NTT:
		country=COUNTRY_JPN;
		break;
	  case ISDN_OPERATOR_ETSI:
	  case ISDN_OPERATOR_ECMA_QSIG: /* for QSIG country has no meaning */
	  case ISDN_OPERATOR_DPNSS:
		country=COUNTRY_EUR;
		break;
	  case ISDN_OPERATOR_AUSTEL_1:
		country=COUNTRY_AUS;
		break;
	  case ISDN_OPERATOR_HK_TEL:
		country=COUNTRY_HONG_KONG;
		break;
	  case ISDN_OPERATOR_KOREAN_OP:
		country=COUNTRY_KOREA;
        break;
      case ISDN_OPERATOR_TAIWAN_OP:
        country=COUNTRY_TAIWAN;
      default: 
        break;
    }

}

/*----------------------------------------------------------------------------
							IsdnStop

	Stops the ISDN stack
----------------------------------------------------------------------------*/
void IsdnStop ( MYCONTEXT *cx )
{
  DWORD adi_ret;
  CTA_EVENT event;

  DemoReportComment( "Stopping ISDN stack..." );
  DemoReportComment( "...waiting for RESTART ACK from the remote end..." );

  adi_ret = isdnStopProtocol( cx->ctahd );
  
  if (adi_ret != SUCCESS)
  {
	ctaGetText( cx->ctahd, adi_ret, (char *) errortext,40);
    printf("isdnStopProtocol failure %s\n",errortext);
    exit(1);
  }
  
  DemoWaitForSpecificEvent( cx->ctahd, ISDNEVN_STOP_PROTOCOL, &event );

  if (event.value != SUCCESS)
  {
     DemoShowError( "Error in stopping ISDN stack", event.value );
     exit(1);
   }
   else
     DemoReportComment( "ISDN stack stopped" );
}

/*----------------------------------------------------------------------------
							IsdnStart

	Initializes Natural Access and starts the chosen ISDN variant
----------------------------------------------------------------------------*/
void IsdnStart ( MYCONTEXT *cx )
{
  DWORD adi_ret;
  CTA_EVENT event = { 0 };
  ISDN_PROTOCOL_PARMS_CHANNELIZED cc_parms;

  
  if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
      if( nfas_group_defined )
      {
          if (cx->partner_equip == EQUIPMENT_TE)
              printf( "\tStarting ISDN stack on board %d nai %d NFAS group %d NT side...\n", cx->ag_board, cx->nai, cx->nfas_group );
          else
              printf( "\tStarting ISDN stack on board %d nai %d NFAS group %d TE side...\n", cx->ag_board, cx->nai, cx->nfas_group );
      }
      else
      {
          if (cx->partner_equip == EQUIPMENT_TE)
              printf( "\tStarting ISDN stack on board %d nai %d NT side...\n", cx->ag_board, cx->nai );
          else
              printf( "\tStarting ISDN stack on board %d nai %d TE side...\n", cx->ag_board, cx->nai );
      }

  /* start the ISDN stack */
  if  (T309_option || in_calls_behaviour || out_calls_behaviour || 
       ns_behaviour || acu_behaviour || nfas_group_defined || services)
  {
      memset(&cc_parms, 0, sizeof(ISDN_PROTOCOL_PARMS_CHANNELIZED));

      cc_parms.size = sizeof(ISDN_PROTOCOL_PARMS_CHANNELIZED);
	  cc_parms.in_calls_behaviour = in_calls_behaviour;
	  cc_parms.out_calls_behaviour = out_calls_behaviour;
	  cc_parms.ns_behaviour = ns_behaviour;
      cc_parms.acu_behaviour = acu_behaviour;
	  cc_parms.t309 = T309_option;
      cc_parms.nfas_group = cx->nfas_group;

	  if ( services ) 
	  {
		  BYTE *p = cc_parms.services_list;
		  int i;
          if ( services[0] == '#' )
		  {
             
             services++;
             for ( i =0; i< default_services[i] ; i++ )
                if ( !strchr(services, default_services[i]) )
				   *p++ = default_services[i];
		  }
		  else 
		  {
			 for ( i =0; i< CC_MX_SERVICES && services[i] ; i++ ) 
				*p++ = services[i]; 
		  }
          *p = NO_SERVICE;

		  printf("\tEnabled services: "); 
		  for(i=0; cc_parms.services_list+i < p ; i++) printf("%c ", cc_parms.services_list[i]);
		  printf("\n");
	  }

      adi_ret = isdnStartProtocol( cx->ctahd, 
			                       ISDN_PROTOCOL_CHANNELIZED,
								   netoperator,
					               country, 
								   cx->partner_equip,
							       cx->nai,                                   
								   &cc_parms );
  }
  else
  {
	  adi_ret = isdnStartProtocol( cx->ctahd, 
			                       ISDN_PROTOCOL_CHANNELIZED,
								   netoperator,
					               country, 
								   cx->partner_equip,
							       cx->nai,
								   NULL );
  }

  if (adi_ret != SUCCESS)
  {
	ctaGetText( cx->ctahd, adi_ret, (char *) errortext,40);
    printf("isdnStartProtocol failure: %s\n",errortext);
    exit(1);
  }

  DemoWaitForSpecificEvent( cx->ctahd, ISDNEVN_START_PROTOCOL, &event );

  if ( event.value != SUCCESS)
  {
      printf( "\tExiting...\n");
	  exit( 1 );
  }

}

/*----------------------------------------------------------------------------
								PrintInfo

	Inform the user of the started protocol
----------------------------------------------------------------------------*/
void PrintInfo ( void )
{
  if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
  {
	 printf("\n\t   Network Operator = ");
	 switch (netoperator)
     {
		case ISDN_OPERATOR_FT_VN6:
		  printf ("France Telecom VN6");
          break;
		case ISDN_OPERATOR_NT_DMS:
		  printf ("Northern Telecom DMS100");
          break;
		case ISDN_OPERATOR_NT_DMS250:
		  printf ("Northern Telecom DMS250");
          break;
		case ISDN_OPERATOR_NTT:
		  printf ("Nippon Telegraph and Telephone INS-1500");
          break;
		case ISDN_OPERATOR_ETSI:
		  printf ("EuroISDN");
          break;
		case ISDN_OPERATOR_KOREAN_OP:
		  printf ("Korean Operator");
          break;
        case ISDN_OPERATOR_TAIWAN_OP:
          printf ("Taiwanese Operator");
          break;
		case ISDN_OPERATOR_AUSTEL_1:
		  printf ("Australian Telecom 1");
          break;
		case ISDN_OPERATOR_HK_TEL:
		  printf ("Hong Kong Telecom");
          break;
		case ISDN_OPERATOR_NI2:
		  printf ("National ISDN 2");
          break;
		case ISDN_OPERATOR_ATT_5E10:
		  printf ("AT&T 5ESS10");
          break;
	    case ISDN_OPERATOR_ATT_4ESS:
		  printf ("AT&T 4ESS");
          break;
		case ISDN_OPERATOR_ECMA_QSIG:
		  printf ("QSIG");
          break;
		case ISDN_OPERATOR_ANSI_T1_607:
		  printf ("ANSI T1.607");
          break;
		case ISDN_OPERATOR_DPNSS:
		  printf ("DPNSS");
          break;
		default:
          break;
	 }  	  

	 printf("\n\t   Country          = ");
	 switch (country)
     {
	    case COUNTRY_USA:
		  printf("USA");
		  break;
		case COUNTRY_BEL:
		  printf("Belgium");
		  break;
		case COUNTRY_CHINA:
		  printf("China");
		  break;
		case COUNTRY_KOREA:
		  printf("Korea");
		  break;
        case COUNTRY_TAIWAN:
          printf("Taiwan");
          break;
		case COUNTRY_FRA:
		  printf("France");
		  break;
		case COUNTRY_GBR:
		  printf("Great Britain");
		  break;
		case COUNTRY_SWE:
		  printf("Sweden");
		  break;
		case COUNTRY_GER:
		  printf("Germany");
		  break;
		case COUNTRY_AUS:
		  printf("Australia");
		  break;
		case COUNTRY_JPN:
		  printf("Japan");
		  break;
		case COUNTRY_HONG_KONG:
		  printf("Hong Kong");
		  break;
		case COUNTRY_SINGAPORE:
		  printf("Singapore");
		  break;
		case COUNTRY_EUR:
		  printf("Europe");
		  break;
        default:
		  break;
	 }

  }

}

/*-----------------------------------------------------------------------------
                            MyMakeConnection()

   The following function opens a switching device and make the default 
   connections for ISDN which are the following (using MVIP 95 model)
   for  T1 or E1 boards 
         0:0..22(29) -> 5:0..22(29)   
         4:0..22(29) -> 1:0..22(29) 
         2:0 -> 9:0
         3:0 -> 8:0
   for Dual T1 boards 
         2:0 -> 21:0  20:0 -> 3:0
         6:0 -> 23:0  22:0 -> 7:0
         0:0..22 -> 17:0..22   16:0..22  ->  1:0..22 
         4:0..22 -> 17:24..46  16:24..46 ->  5:0..22 
   for Dual E1 boards 
         2:0 -> 21:0  20:0 -> 3:0
         6:0 -> 23:0  22:0 -> 7:0
         0:0..29 -> 17:0..29   16:0..29   ->  1:0..29 
         4:0..29 -> 17:30..59  16:30..59  ->  5:0..29 
   for Quad T1 boards 
         2:0 -> 21:0  20:0 -> 3:0
         6:0 -> 23:0  22:0 -> 7:0
        10:0 -> 25:0  24:0 -> 11:0
        14:0 -> 27:0  26:0 -> 15:0
         0:0..22 -> 17:0..22   16:0..22  ->  1:0..22 
         4:0..22 -> 17:24..46  16:24..46 ->  5:0..22 
         8:0..22 -> 17:48..70  16:48..70 ->  9:0..22 
        12:0..22 -> 17:72..94  16:72..94 -> 13:0..22 
   for Quad E1 boards 
         2:0 -> 21:0  20:0 -> 3:0
         6:0 -> 23:0  22:0 -> 7:0
        10:0 -> 25:0  24:0 -> 11:0
        14:0 -> 27:0  26:0 -> 15:0
         0:0..29 -> 17:0..29    16:0..29   ->  1:0..29 
         4:0..29 -> 17:30..59   16:30..59  ->  5:0..29 
         8:0..29 -> 17:60..89   16:60..89  ->  9:0..29 
        12:0..29 -> 17:90..119  16:90..119 -> 13:0..29 
-----------------------------------------------------------------------------*/
void MyMakeConnection ( MYCONTEXT *cx )
{
  SWI_TERMINUS *outputs, *inputs;
  DWORD adi_ret;
  int i,j,k;
  int num_channels ;			/* number of B channels                 */
  int num_trunks;           
  int skip_ts = 0;              /* one timeslot every trunk is skipped
                                   in the connections for T1 trunks     */
  int skip_st = 0;              /* in the dual boards the DSP resources
                                   are still on streams 16 and 17 
                                   (like Quad)                          */
  ADI_BOARD_INFO boardinfo;     /* ADI structure for board info         */
  int num_connections, conx;


  /* load boad info to figure out the number of B channels */
  adi_ret = adiGetBoardInfo( cx->ctahd, cx->ag_board, sizeof(ADI_BOARD_INFO),
                            &boardinfo);
  if (adi_ret != SUCCESS)
  {
	ctaGetText( cx->ctahd, adi_ret, (char *) errortext,40);
    printf("adiGetBoardInfo failure: %s\n",errortext);
    exit( 1 );
  }

  switch (boardinfo.trunktype)
  {
	case ADI_TRUNKTYPE_T1:			/* T1 board */            
	   num_channels = 23;
	   break;
	case ADI_TRUNKTYPE_E1:			/* E1 board */
	   num_channels = 30;
	   break;
	case ADI_TRUNKTYPE_BRI:			/* BRI board */
	   num_channels = 8;
	   break;
	default:
      printf( "Illegal board for this demo, board%d\n", cx->ag_board );
      break; 
  }

  num_trunks = boardinfo.numtrunks;

  if( num_trunks == 2 )				/* Dual T1 or E1 board */
	  skip_st = 8; 
  if ( boardinfo.boardtype == ADI_BOARDTYPE_AG4000_1T ||
       boardinfo.boardtype == ADI_BOARDTYPE_AG4000_1E )
      skip_st = 12; /* in the AG 4000 with 1 trunk the DSP resources 
                       are on stream 16 and 17 */

  if( boardinfo.trunktype == ADI_TRUNKTYPE_T1 && num_trunks != 1 )	
  {
	  /* T1 Dual or Quad board */
	  skip_ts = 1;
  }  
  
  /* open the switching device */
  adi_ret = swiOpenSwitch(cx->ctahd, "AGSW", cx->ag_board, 0, &(cx->swihd));

  if (adi_ret != SUCCESS)
  {
    ctaGetText( cx->ctahd, adi_ret, (char *) errortext,40);
    printf("swiOpenSwitch failure: %s\n",errortext);
    exit( 1 );
  }
  
  inputs  = (SWI_TERMINUS *)malloc( sizeof(SWI_TERMINUS)*(num_channels + 1) * num_trunks * 2 ); 
  outputs = (SWI_TERMINUS *)malloc( sizeof(SWI_TERMINUS)*(num_channels + 1) * num_trunks * 2 ); 
  
  /* create the list of connections */
      if( boardinfo.trunktype != ADI_TRUNKTYPE_BRI  )	
      {
          for ( k = 0; k < num_trunks; k++ )             
          {
              j = k * ( num_channels + 1 ) * 2; 
              for ( i = 0; i < num_channels; i++, j+=2 ) /* the B channels */
              {
                 inputs[j].stream = k * 4;
                 inputs[j].timeslot = i;
                 inputs[j].bus = MVIP95_LOCAL_BUS;
                 outputs[j].stream = num_trunks * 4 + 1 + skip_st;
                 outputs[j].timeslot = i + k * (num_channels + skip_ts);
                 outputs[j].bus = MVIP95_LOCAL_BUS;
                 inputs[j+1].stream = num_trunks * 4 + skip_st;
                 inputs[j+1].timeslot = i + k * (num_channels + skip_ts);
                 inputs[j+1].bus = MVIP95_LOCAL_BUS;
                 outputs[j+1].stream = 1 + k * 4;
                 outputs[j+1].timeslot = i;
                 outputs[j+1].bus = MVIP95_LOCAL_BUS;
              }

              inputs[j].stream = 2 + k * 4;		 /* the D channel */
              inputs[j].timeslot = 0;
              inputs[j].bus = MVIP95_LOCAL_BUS;
              outputs[j].stream = num_trunks * 4 + 5 + k * 2 + skip_st;
              outputs[j].timeslot = 0;
              outputs[j].bus = MVIP95_LOCAL_BUS;
              inputs[j+1].stream =  num_trunks * 4 + 4 + k * 2 + skip_st;
              inputs[j+1].timeslot = 0;
              inputs[j+1].bus = MVIP95_LOCAL_BUS;
              outputs[j+1].stream = 3 + k * 4;
              outputs[j+1].timeslot = 0;
              outputs[j+1].bus = MVIP95_LOCAL_BUS;
          }
      }
      else /* for AG2000 BRI board */
      {
          for ( i = 0; i < num_channels; i++) /* the B channels */
          {
             inputs[i].stream = 0;
             inputs[i].timeslot = i;
             inputs[i].bus = MVIP95_LOCAL_BUS;
             outputs[i].stream = 5;
             outputs[i].timeslot = i;
             outputs[i].bus = MVIP95_LOCAL_BUS;
             inputs[i+num_channels].stream = 4;
             inputs[i+num_channels].timeslot = i;
             inputs[i+num_channels].bus = MVIP95_LOCAL_BUS;
             outputs[i+num_channels].stream = 1;
             outputs[i+num_channels].timeslot = i;
             outputs[i+num_channels].bus = MVIP95_LOCAL_BUS;
          }
      }
  /* make the connections */
  /* because of a limitation of the MVIP 95 driver on Solaris we cannot make
     more than 32 connections at a time. */
#define MAX_CONNECTIONS 32
      if( boardinfo.trunktype != ADI_TRUNKTYPE_BRI  )	
          num_connections = (num_channels + 1) * 2 * num_trunks;
      else /* for AG2000 BRI board */
          num_connections = num_channels * 2 ;
  i = 0;

  while (num_connections > 0)
  {
	conx = ( ( num_connections > MAX_CONNECTIONS ) ? MAX_CONNECTIONS : num_connections);

	adi_ret = swiMakeConnection(cx->swihd, &inputs[i * MAX_CONNECTIONS], 
		                        &outputs[i * MAX_CONNECTIONS], conx );
	if (adi_ret != SUCCESS)
	{
		ctaGetText( cx->ctahd, adi_ret, (char *) errortext,40);
	    printf("swiMakeConnection failure: %s\n",errortext);
	    exit( 1 );
	}
	i++;
	num_connections -= conx;
  }

  /* free memory */
  free(inputs);
  free(outputs);
}


/*-----------------------------------------------------------------------------
                            WaitForKeyboardEvent()

   The following function waits for a keyboard event.  
   It returns the key.
-----------------------------------------------------------------------------*/
int WaitForKeyboardEvent( CTAQUEUEHD ctaqueuehd, CTA_EVENT *eventp )
{
    DWORD ctaret ;

#ifndef SCO
#if defined (UNIX)
    struct pollfd fds[2];
    /* get the handle and set up the polling structure */
    fds[0].fd     = STDIN_FILENO ;
    fds[0].events = POLLIN;
#endif
#endif


    while (1)
    {
      ctaret = ctaWaitEvent( ctaqueuehd, eventp, 100); /* wait for 100 ms */
      if (ctaret != SUCCESS)
      {
          printf( "\n\t CTAERR_SVR_COMM 0x%x\n", ctaret);
          exit( 1 );
      }

      if (eventp->id == CTAEVN_WAIT_TIMEOUT)
      {

#if defined (SCO)
        if (kbhit())
          return getc(stdin);
#elif defined (UNIX)
        if ( poll( fds, 2, 200 ) == POLL_ERROR )
        {
          if ( errno == EINTR ) 
            return 0;
          perror("poll()") ;
          exit(-1) ;
        }
        if ( fds[0].revents & POLLIN )      /* would poll forever          */
          return getc(stdin);
#elif defined (WIN32)
        if(kbhit())                 /* check if a key has been pressed */
          return _getch();
#endif  /* UNIX, WIN32 */
      }
      else /* no CT events should be received at this point */
	  {
		  printf("Unexpected CT event: 0x%x\n", eventp->id );
	      return -1;
      }
    }

}

