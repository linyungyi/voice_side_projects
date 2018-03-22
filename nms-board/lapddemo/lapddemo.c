/*****************************************************************************
*  FILE: lapddemo.c
*
*  DESCRIPTION: example of implementation of a basic Q.931 application over
*               the LAP-D interface
*
* Copyright (c) 1998-2001  NMS Communication.   All rights reserved.
*****************************************************************************/

/* $$Revision "%v" */
#define VERSION_NUM  "2"
#define VERSION_DATE __DATE__

/*---------------------------- Standard Includes ----------------------------*/
#include <stdio.h>
#include "decisdn.h"
#include "lapdlib.h"

#if defined (UNIX)
/* files needed for the included kbhit function, emulating the one
   for OS/2 and NT (conio.h) */
  #include <unistd.h>
  #include <sys/time.h>
  #include <sys/types.h>
  #include <sys/select.h>

  #include <termio.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #define GETCH() getchar()
#elif defined (WIN32)
  #include <conio.h>
  #define GETCH() _getch()
#endif

/*---------------------------- AG Access Includes ---------------------------*/
#include "ctademo.h"

/*---------------------------- ISDN include files ---------------------------*/
#include "isdntype.h"
#include "isdndef.h"
#include "isdnparm.h"
#include "isdndl.h"

/*----------------------------------- Defines ------------------------------*/

#define KEYBOARD_EVENT 0xabcd /* no AG Access events have this code         */

enum msg_index { SETUP_INDEX,
                 SETUP_ACK_INDEX,
                 CALL_PROC_INDEX,
                 ALERTING_INDEX,
                 CONNECT_INDEX,
                 CONNECT_ACK_INDEX,
                 DISCONNECT_INDEX,
                 PROGRESS_INDEX,
                 RELEASE_INDEX,
                 RELEASE_COMP_INDEX,
                 TIMER_T_INDEX,
                 TIMER_t_INDEX
               };

/*----------------------------- Application Definitions --------------------*/
typedef struct
{
    CTAQUEUEHD  ctaqueuehd;       /* Natural Access queue handle                 */
    CTAHD       ctahd;            /* Natural Access context handle               */
} MYCONTEXT;


typedef struct          
{
    char    in_cmd;      /* command to be executed when the 'cause' is received */
    char    out_cmd;     /* command to be executed when the 'cause' is sent */
} COMMAND;

typedef struct			 
{
    char called[32];   /* called number */
    char calling[32];  /* calling number */
    char Bchan;		   /* Bchan */
    char encoding;	   /* A Law vs Mu Law */
    char cause;		   /* in the DISCONNECT msg */
    char pref_excl;     /* Exclusive mode */
} ISDN_PAR;


/*----------------------------- Function Prototypes ------------------------*/
/* isdncta.c */
void PrintHelp ( void );
void IsdnStart ( void );
void IsdnStop ( void );
void ParseArguments ( int argc, char **argv ); 
void MyOpenPort ( void );
DWORD WaitForAnyEvent( CTA_EVENT *eventp );
void MyCTAEventHandler( CTA_EVENT *eventp );
void MyKeybEventHandler( DWORD command );
int ProcessIsdnMsg( CTA_EVENT *eventp);
void PrintOptions( void );
void ClearScreen( void );
void InitMessage (ISDN_MESSAGE *IMsgp);
void EstablishDL( void );
void SendPacket ( char *buf, char code );
void ReadConfigFile( char *namefile);
void ReadParameter ( char *paramp );
void HandleIsdnMessage( char *buff, int size );
void CheckCommandList( char index, char dir );
void ExecuteCommand( char command );
void ShowIsdnMessage ( char *buff, char dir );
void GetSetupDefaults ( SETUP_PARM *setup_p );
#if defined (SCO)
int mykbhit (void);
#endif


/*----------------------------- Global Variables ---------------------------*/

  MYCONTEXT cntx;
  BYTE      errortext[40];
  char	cfg_file[40];			/* configuration file name */
  unsigned  partner_equip;
  int board;					/* board number	        */
  int nai;
  unsigned nfas_group;
  int nfas_group_defined;
  int data_link_established;
  int length_buf;
  COMMAND   command_list [40];
  ISDN_PAR isdn_par;			/* this structure contains user-defined parameters */
  int timer_T = 0;
  int timer_tx = 0;
  CALL_REF call_ref;
  unsigned timer_T_value = 2000;  /* timer value 2 s */   
  unsigned timer_t_value = 1000;  /* timer value 1 s */   
  char verbose = 2;             /* verbose level 
                                   0    show nothing
                                   1    SETUP's only
                                   2    all ISDN messages
                                   3    some details (CR ect)
                                */ 
  /* miscellaneous ISDN info */
  char cause;
  char Bchannel;

  char boardType = IT_OTHER;
/*----------------------------------------------------------------------------
                            main
----------------------------------------------------------------------------*/
int main ( int argc, char **argv )
{
  DWORD ret;
  CTA_EVENT event;

  CTA_INIT_PARMS initparms = { 0 };	    /* for ctaInitialize */
  CTA_SERVICE_NAME servicelist[] =      /* for ctaInitialize */
  {   { "ADI",  "ADIMGR" },
      { "ISDN", "ADIMGR" }
  };
  unsigned servicenumber = sizeof(servicelist)/sizeof(servicelist[0]);


  printf ("raw LAPD demo program for Natural Access V.%s (%s)\n", VERSION_NUM, VERSION_DATE);

  /* do some default initialization */
  board = 0;
  nai = 0;
  nfas_group = 0;
  partner_equip = EQUIPMENT_NT;
  data_link_established = FALSE ;
  strcpy(cfg_file,"lapddemo.cfg");

  ParseArguments( argc, argv );

  call_ref.len = 2;
  call_ref.value[0] = 0x12;
  call_ref.value[1] = 0x34;
 
  memset (&isdn_par, 0, sizeof(isdn_par));

  ReadConfigFile(cfg_file);

  
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
  MyOpenPort();

    if (IT_BASIC == boardType)
        call_ref.len = 1;        
    else
        call_ref.len = 2;        
  /* start ISDN stack */
  IsdnStart();

  EstablishDL();   /* try to establish the data link */


  /* enter the main loop */
  while (1)
  {
     

     WaitForAnyEvent( &event );

     switch (event.id)
     {
       case KEYBOARD_EVENT:
           MyKeybEventHandler( event.value );
         break;
	   default:	/* CT event */
	     if (event.id != 0);
           MyCTAEventHandler( &event );
		 break;
	 }		 
  }


  return SUCCESS;
}

/*----------------------------------------------------------------------------
							ParseArguments 

	Parses and checks the command line options
----------------------------------------------------------------------------*/
void ParseArguments ( int argc, char **argv )
{
  int c;

  while( ( c = getopt( argc, argv, "b:a:g:i:v:f:nHh?" ) ) != -1 )
  {
    switch( c )
    {
      case 'b':
        sscanf( optarg, "%d", &board );
        break;
      case 'a':
        sscanf( optarg, "%d", &nai );
        break;
      case 'g':
        sscanf( optarg, "%u", &nfas_group );
        nfas_group_defined = 1;
        break;
      case 'v':
        sscanf( optarg, "%d", &verbose );
        break;
      case 'n':
        partner_equip = EQUIPMENT_TE;  
        break;
      case 'f':
        sscanf( optarg, "%s", cfg_file );
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
 
}

/*----------------------------------------------------------------------------
							PrintHelp

	Prints the list of values for the command line options
----------------------------------------------------------------------------*/
void PrintHelp ( void )
{
    printf( "Usage: lapddemo [opts]\n" );
    printf( "where opts are:\n" );
    printf( "-b n        AG board number \n");
    printf( "-n	         NT side (default = TE side)\n");
    printf( "-a n        Network Access Identifier \n");
    printf( "-g n        NFAS group number (if duplicate NAI values configured)\n");
	printf( "-v level    Verbosity level of messages printed on screen, where:\n"
            "            level = 0 --> Show no messages\n"
            "            level = 1 --> Show SETUP messages only\n"
            "            level = 2 --> Show all ISDN messages (default)\n" );
	printf( "-f filename Name of configuration file to be read\n"
			"            (default configuration file = lapddemo.cfg)\n");
    printf( "-h or -?    Display this help screen\n");
}

/*----------------------------------------------------------------------------
							IsdnStop

	Stops the ISDN stack
----------------------------------------------------------------------------*/
void IsdnStop ( void )
{
  DWORD adi_ret;
  CTA_EVENT event;

  DemoReportComment( "Stopping ISDN stack..." );

  adi_ret = isdnStopProtocol( cntx.ctahd );
  
  if (adi_ret != SUCCESS)
  {
	ctaGetText( cntx.ctahd, adi_ret, (char *)errortext,40);
    printf("isdnStopProtocol failure %s\n",(char *)errortext);
    exit(1);
  }
  
  DemoWaitForSpecificEvent( cntx.ctahd, ISDNEVN_STOP_PROTOCOL, &event );

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

	Initializes Natural Access and starts the ISDN stack
----------------------------------------------------------------------------*/
void IsdnStart ( void )
{
  DWORD adi_ret;
  CTA_EVENT event = { 0 };
  ISDN_PROTOCOL_PARMS_LAPD dl_parms;

  if( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
  {
      if( nfas_group_defined )
      {
          if (partner_equip == EQUIPMENT_TE)
              printf( "\tStarting ISDN stack on board %d nai %d NFAS group %d (NT side)...\n", board, nai, nfas_group );
          else
              printf( "\tStarting ISDN stack on board %d nai %d NFAS group %d (TE side)...\n", board, nai, nfas_group );
      }
  }
  /* start the ISDN stack */
  if( nfas_group_defined )
  {
      memset(&dl_parms, 0, sizeof(ISDN_PROTOCOL_PARMS_LAPD));
      dl_parms.size = sizeof(ISDN_PROTOCOL_PARMS_LAPD);
      dl_parms.nfas_group = nfas_group;
      adi_ret = isdnStartProtocol( cntx.ctahd, 
	                           ISDN_PROTOCOL_LAPD,
							   0,
                               0, 
							   partner_equip,
                               nai,
                               (void *)&dl_parms );
  }
  else
    adi_ret = isdnStartProtocol( cntx.ctahd, 
	                           ISDN_PROTOCOL_LAPD,
							   0,
                               0, 
							   partner_equip,
                               nai,
                               NULL );
  if (adi_ret != SUCCESS)
  {
	ctaGetText( cntx.ctahd, adi_ret, (char *)errortext,40);
    printf("isdnStartProtocol failure: %s\n",(char *)errortext);
    exit(1);
  }

  DemoWaitForSpecificEvent( cntx.ctahd, ISDNEVN_START_PROTOCOL, &event );

  if ( event.value != SUCCESS)
  {
      printf( "\tExiting...\n");
	  exit( 1 );
  }

}

/*-----------------------------------------------------------------------------
                            MyOpenPort()

   The following function uses the function DemoOpenPort (defined in ctademo.c) 
   to open and initialize the services        
-----------------------------------------------------------------------------*/
void MyOpenPort ( void )
{
  char contextname[6];
  unsigned i;
  CTA_SERVICE_DESC services[] =        /* for ctaOpenServices */
  {   { {"ADI",  "ADIMGR"}, { 0 }, { 0 }, { 0 } },
      { {"ISDN", "ADIMGR"}, { 0 }, { 0 }, { 0 } }
  };
  unsigned numservices = sizeof(services)/sizeof(services[0]);

  /* Fill board information         */
  for ( i = 0; i < numservices; i++ )
    services[i].mvipaddr.board    = board;

  /* Create the name of the context */
  strcpy( contextname, "isdn");

  /* use the function in ctademo to open and initialize the context              */
  DemoOpenPort( (DWORD) board,	/* use the number of the board as userid */ 
	            contextname, 
				services,
                numservices, 
				&(cntx.ctaqueuehd), 
				&(cntx.ctahd) );
{
    DWORD ret;
    char  textbuf[80];
    ADI_BOARD_INFO boardinfo ;
    
    ret = adiGetBoardInfo( cntx.ctahd, board, sizeof boardinfo, &boardinfo );

    if ( ret == CTAERR_INVALID_BOARD ) return;
    if ( ret == CTAERR_DRIVER_OPEN_FAILED) 
    {
        // temp? Until all boards are in the board # mapping database, must
        // assume all boards not defined are AG boards, although AG driver
        // may not even be active
        return;
    }

    // Board is not invalid, but is indicated as "not found".
    // If this happens, it means board exists but is not started, and
    // system can't give us proper info on it to display.  So skip it.
    if ( ret == CTAERR_NOT_FOUND )
    {
        return;
    }

    if ( ret != SUCCESS )
    {
        ctaGetText( cntx.ctahd, ret, textbuf, sizeof textbuf );
        fprintf( stderr, "\n%3d   adiGetBoardInfo() failed: %s (0x%x)\n",
                 board, textbuf, ret );
        return ;
    }
    if (ADI_BOARDTYPE_AG2000BRI ==  boardinfo.boardtype )   
    {
        boardType = IT_BASIC;
    }
    else
    {
        boardType = IT_OTHER;
    }
}
}

/*-----------------------------------------------------------------------------
                            WaitForAnyEvent()

   The following function waits for either an AG event or for a
   keyboard event.  
-----------------------------------------------------------------------------*/
DWORD WaitForAnyEvent( CTA_EVENT *eventp )
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
      ctaret = ctaWaitEvent( cntx.ctaqueuehd, eventp, 100); /* wait for 100 ms */
      if (ctaret != SUCCESS)
      {
          printf( "\n\tCTAERR_SVR_COMM 0x%x\n", ctaret);
          exit( 1 );
      }

      if (eventp->id == CTAEVN_WAIT_TIMEOUT)
      {

#if defined (WIN32)
        if(kbhit())                 /* check if a key has been pressed */
        {
          eventp->value = GETCH();
          eventp->id = KEYBOARD_EVENT;   /* indicate that a key is ready */
          return SUCCESS;
        }
#elif defined (SCO)
        if(mykbhit())                 /* check if a key has been pressed */
        {
          eventp->value = GETCH();
          eventp->id = KEYBOARD_EVENT;   /* indicate that a key is ready */
          return SUCCESS;
        }

#elif defined (UNIX)
#define POLL_ERROR -1
        if ( poll( fds, 2, 200 ) == POLL_ERROR )
        {
          if ( errno == EINTR ) continue ;
          perror("poll()") ;
          exit(-1) ;
        }
          if ( fds[0].revents & POLLIN )      /* would poll forever          */
          {
            eventp->value = GETCH();
            eventp->id = KEYBOARD_EVENT;      /* indicate that a key is ready */
            return SUCCESS;
          }
#endif  /* SCO, UNIX, WIN32 */
      }
      else return SUCCESS;
    }

}

/*-----------------------------------------------------------------------------
                            MyKeybEventHandler()

   The following function handle the command from keyboard
-----------------------------------------------------------------------------*/
void MyKeybEventHandler ( DWORD command )
{
  char * msg_pointer; 
  char * struct_pointer; 

  switch(command)
  {
    case 'q':	/* quit */
      IsdnStop();
      DemoReportComment( "Exiting..." );
	  exit (0);
	  break;
	case 'e':	/* establish data link */
      EstablishDL();
	  break;
	case 'S':	/* SETUP */
      struct_pointer = malloc( sizeof(SETUP_PARM)  );
      msg_pointer = malloc ( MAX_ISDN_BUFFER_SIZE ); 
      GetSetupDefaults ( ( SETUP_PARM *) struct_pointer );
	  if ( isdn_par.calling )
	    strcpy( (( SETUP_PARM *) struct_pointer)->calling_numb, isdn_par.calling ); 
	  if ( isdn_par.called )
	    strcpy( (( SETUP_PARM *) struct_pointer)->called_numb, isdn_par.called ); 
	  if ( isdn_par.Bchan )
	    (( SETUP_PARM *) struct_pointer)->channel_number = isdn_par.Bchan; 
	  if ( isdn_par.encoding )
	    (( SETUP_PARM *) struct_pointer)->encoding = isdn_par.encoding; 
        (( SETUP_PARM *) struct_pointer)->pref_excl = isdn_par.pref_excl;
      length_buf = BuildSetup ( msg_pointer, ( SETUP_PARM *) struct_pointer, call_ref );
      SendPacket( msg_pointer, DL_DA_RQ);
      free (struct_pointer);
      free (msg_pointer);
	  break;
	case 'h':	/* help */
	case 'H':
	case '?':
	default:
	  ClearScreen();
	  PrintOptions();
	  break;
  }

  return;
}

/*----------------------------------------------------------------------------
                            MyCTAEventHandler()

  CT Event handler routine.
----------------------------------------------------------------------------*/
void MyCTAEventHandler ( CTA_EVENT *eventp )
{

  switch( eventp->id )
  {
    case ISDNEVN_SEND_MESSAGE:
      if( eventp->value != SUCCESS) 
      {
        DemoShowError( "IsdnSendMessage", eventp->value );
	    exit( 1 );
	  }
      break;
    case ISDNEVN_RCV_MESSAGE:
	  ProcessIsdnMsg(eventp);
	  break;
    case ADIEVN_TIMER_DONE:
      if (eventp->value == CTA_REASON_FINISHED) 
        if ( timer_T )
        {
          CheckCommandList ( TIMER_T_INDEX, 'i' );
          timer_T = 0;
        }
        else
        {
          CheckCommandList ( TIMER_t_INDEX, 'i' );
          timer_tx = 0;
        }
	  break;
    default:
      DemoReportUnexpectedEvent( eventp );
	  break;
  }
}

/*----------------------------------------------------------------------------
                            ProcessIsdnMsg()

  ISDN message handler routine
----------------------------------------------------------------------------*/
int ProcessIsdnMsg ( CTA_EVENT *eventp)
{
  ISDN_PACKET *isdnpkt;
  ISDN_MESSAGE *isdnmsg;
  DWORD adi_ret;
  char isdnmessage, sender;

  /* Parse ISDN messages */

  /* pick up the ISDN_PACKET structure */

  isdnpkt = (ISDN_PACKET *)eventp->buffer;
  isdnmsg = (ISDN_MESSAGE *)&isdnpkt->message;

  /* pick up some information */

  isdnmessage = isdnmsg->code;
  sender = isdnmsg->from_ent;

  /* Based on the sender, process the information */
  switch (sender)
  {
    case ENT_MPH:           /* physical layer management */
      printf("Physical layer management\n");
      break;

    case ENT_DL_D:            /* this is the only case that should happen */
	  switch (isdnmessage)
	  {
	    case DL_EST_IN:	/* received SABME message, the stack automatically sends a UA message */	    		  
		  DemoReportComment ("received DL_EST_IN: Data Link has been established");
		  data_link_established = TRUE;
		  break;
	    case DL_EST_CO:	/* received UA message after our SABME */		  
		  DemoReportComment ("received DL_EST_CO: Data Link has been established");
		  data_link_established = TRUE;
		  break;
	    case DL_U_DA_IN:		  /* received a packed of data					 */
	    case DL_DA_IN:		 
          HandleIsdnMessage( (char *)isdnpkt->data, isdnpkt->message.data_size );
		  break;
		case DL_REL_IN:
		  DemoReportComment ("received DL_REL_IN: Data Link is down");
          if ( data_link_established )
          {
		    data_link_established = FALSE ;
            EstablishDL();                      /* try to re-establish the data link */
          }
		  break;		  
		case DL_REL_CO:
		  break;		  
		default:
		  DemoReportUnexpectedEvent( eventp );
	      break;
	  }
	  break;

    case ENT_SM:            /* system management */
      printf("System management\n");
      break;

    default:
      printf("isdndemo: Sender %c   Message %c\n", sender, isdnmessage);
      break;
  }

  /* Need to free buffer here */
  /* this is important -- do not forget to do it */

  adi_ret = isdnReleaseBuffer(cntx.ctahd, eventp->buffer);

  if (adi_ret != SUCCESS)
  {
	ctaGetText( cntx.ctahd, adi_ret, (char *)errortext,40);
    printf("isdnReleaseBuffer failure %s\n",(char *)errortext);
    exit( 1 );
  }
  
  return SUCCESS;
}

/*----------------------------------------------------------------------------
							SendPacket()

  The following function sends an arbitrary buffer on the trunk
----------------------------------------------------------------------------*/
void SendPacket ( char *buf, char code )
{
  ISDN_MESSAGE IMsg;
  DWORD adi_ret;

  if ( !data_link_established )
  {
    DemoReportComment( "Data link layer has not been established" );
	return;
  }

  InitMessage( &IMsg );

  IMsg.code = code;	


  IMsg.data_size = length_buf;

  adi_ret = isdnSendMessage(cntx.ctahd, &IMsg, (void *)buf, length_buf);

  if ( adi_ret != SUCCESS )
  {
	ctaGetText( cntx.ctahd, adi_ret, (char *)errortext,40);
    printf("isdnStopProtocol failure %s\n",(char *)errortext);
    exit(1);
  }

  ShowIsdnMessage(buf, 'o');

}

/*----------------------------------------------------------------------------
							PrintOptions 
	Prints the run time options
----------------------------------------------------------------------------*/
void PrintOptions ( void )
{
  printf ("LAP-D demo program for Natural Access V.%s (%s)\n\n", 
	        VERSION_NUM, VERSION_DATE);
  printf("e   - establish data link\n");
  printf("S   - send SETUP\n");
  printf("q   - quit\n");
  printf("hH? - display this list\n\n");

}

/*----------------------------------------------------------------------------
							InitMessage()
	fill the header of an IsdnMessage to be sent to DL entity
----------------------------------------------------------------------------*/
void InitMessage (ISDN_MESSAGE *IMsgp)
{
  memset((void *)IMsgp, 0, sizeof(ISDN_MESSAGE));

  IMsgp->from_ent = ENT_APPLI;			/* from */
  IMsgp->to_ent = ENT_DL_D;				/* to	*/
  IMsgp->nai = nai;
  IMsgp->nfas_group = nfas_group;
  IMsgp->to_sapi = DL_SAPI_SIG;
  IMsgp->add.tei = 1;					/* always 1 for LAP-D */
}

/*-----------------------------------------------------------------------------
                            EstablishDL()

   The following function sends a SABME message on the trunk in order to 
   establish the data link layer
-----------------------------------------------------------------------------*/
void EstablishDL( void )
{
  ISDN_MESSAGE IMsg;

  InitMessage( &IMsg );

  IMsg.code = DL_EST_RQ;

  isdnSendMessage(cntx.ctahd, &IMsg, NULL, IMsg.data_size);

  return;
}

/*-----------------------------------------------------------------------------
                            mykbhit()

   The following function implements mykbhit() for SCO
-----------------------------------------------------------------------------*/
#if defined SCO
int mykbhit()
{
   fd_set rdflg;
   struct timeval timev;

   /*
    *  Execute select() for READ on STDIN without timeout (poll keyboard).
    */

   FD_ZERO(&rdflg);
   FD_SET(STDIN_FILENO, &rdflg);

   timev.tv_sec=0;
   timev.tv_usec=0;

   select(1, &rdflg, NULL, NULL, &timev);

   return( FD_ISSET( STDIN_FILENO, &rdflg ) );

}
#endif



/*----------------------------------------------------------------------------
                            ClearScreen()

----------------------------------------------------------------------------*/
void ClearScreen( void )
{
#if defined (WIN32)
    system( "cls" );
#elif defined (UNIX)
    system( "clear" );
#endif
}

/*----------------------------------------------------------------------------
                            ReadConfigFile()
    reads the configuration file
----------------------------------------------------------------------------*/
void ReadConfigFile( char *namefile)
{

  FILE *file_handle;
  char command_line[256];
  int index,line_length,scanned,dir;
  
  /* open the file for reading */
  file_handle = fopen( namefile, "r");
  if ( file_handle == NULL )
  {
    printf("Error opening the file %s\n", namefile);
    exit ( -1 );
  }

  /* read a line from the file */
  while ( fgets( command_line, 255, file_handle ) != NULL )
  {
    scanned = 0;
    line_length = strlen(command_line);
 
    /* the first argument is the cause or a parameter*/
    if ( (int)(scanned += strspn( command_line + scanned, " \t")) >= line_length )
      continue;
    switch ( *( command_line + scanned ))
    {
	  case '!':		/* this is a parameter */
		  ReadParameter( command_line + scanned + 1 );
        continue;
	  case 'S':     /* SETUP  */
        index = SETUP_INDEX;
        break;
	  case 's':     /* SETUP ACK */
        index = SETUP_ACK_INDEX;
        break;
      case 'K':     /* CALL PROCEEDING  */
        index = CALL_PROC_INDEX;
        break;
      case 'A':     /* ALERTING  */
        index = ALERTING_INDEX;
        break;
      case 'C':     /* CONNECT  */
        index = CONNECT_INDEX;
        break;
      case 'c':     /* CONNECT ACKNOWLEDGE  */
        index = CONNECT_ACK_INDEX;
        break;
      case 'D':     /* DISCONNECT  */
        index = DISCONNECT_INDEX;
        break;
      case 'P':     /* PROGRESS  */
        index = PROGRESS_INDEX;
        break;
      case 'R':     /* RELEASE  */
        index = RELEASE_INDEX;
        break;
      case 'r':     /* RELEASE COMP */
        index = RELEASE_COMP_INDEX;
        break;
      case 'T':     /* TIMER T */
        index = TIMER_T_INDEX;
        break;
      case 't':     /* TIMER t */
        index = TIMER_t_INDEX;
        break;
      case '#':
      default:
        continue;
    }

    /* the second argument is the direction */
    scanned++;
    if ( (int)(scanned += strspn( command_line + scanned, " \t")) >= line_length )
      continue;
    switch ( *( command_line + scanned ))
    {
      case 'i':     /* in */
      case 'o':     /* out */
        dir = *(command_line + scanned );
        break;
      case '#':
      default:
        continue;
    }

    /* the third argument is the command */
    scanned++;
    if ( (int)(scanned += strspn( command_line + scanned, " \t")) >= line_length )
      continue;
    switch ( *( command_line + scanned ) )
    {
      case 'S':     /* send SETUP */
      case 's':     /* send SETUP ACK */
      case 'K':     /* send CALL PROCEEDING */
      case 'P':     /* send PROGRESS */
      case 'A':     /* send ALERTING */
      case 'c':     /* send CONNECT ACK */
      case 'C':     /* send CONNECT */
      case 'D':     /* send DISCONNECT */
      case 'R':     /* send RELEASE */
      case 'r':     /* send RELEASE COMPLETE */
      case 'T':     /* start TIMER T*/
      case 't':     /* start TIMER t*/
        if ( dir == 'o' )
          command_list[index].out_cmd = *( command_line + scanned );
        else
          command_list[index].in_cmd = *( command_line + scanned );
        break;
      case '#':
      default:
        continue;
    }

  }

}

/*----------------------------------------------------------------------------
                            ReadParameter()
    reads a parameter from the configuration file
	paramp points to the parameter id
	
	NOTE: comments (#) are NOT allowed between the parameter id and the value.
	Futhermore, if the parameter id is present, the value must be present
----------------------------------------------------------------------------*/
void ReadParameter ( char *paramp )
{
  int bytes,bytes2;
  char param_id = *paramp;
  char string[13];
  
  /* skip the blanks */
  bytes = 1;
  bytes += strspn( paramp + bytes, " \t");
  /* count the non-blank char */
  bytes2  = strcspn( paramp + bytes, " \t\n");

  switch ( param_id ) 
  {
    case 'A':	/* ANI */
	  strncpy ( isdn_par.calling, paramp + bytes, bytes2 );
	  break;
    case 'D':	/* DNIS */
  	  strncpy ( isdn_par.called, paramp + bytes, bytes2 );
	  break;
    case 'B':	/* B channel */
	  isdn_par.Bchan = atoi( paramp + bytes );
	  break;
    case 'E':	/* encoding (A Law vs Mu Law)*/
	  isdn_par.encoding = atoi( paramp + bytes );
	  break;
    case 'C':	/* Cause */
  	  strncpy ( string, paramp + bytes, bytes2 );
	  isdn_par.cause = atoi( paramp + bytes );
	  break;
    case 'X':	/* X exclusive mode */
	  if (atoi( paramp + bytes )) 
	    isdn_par.pref_excl = PE_EXCL;
	  else 
	    isdn_par.pref_excl = PE_PREF;
	  break;
    case 'T':	/* timer T */
	  timer_T_value = atoi( paramp + bytes );
	  break;
    case 't':	/* timer t */
	  timer_t_value = atoi( paramp + bytes );
	  break;
  }
}

/*----------------------------------------------------------------------------
                            HandleIsdnMessage()
    decodes info from an incoming Q931 message and check the command list for 
    appropriate actions
----------------------------------------------------------------------------*/
void HandleIsdnMessage( char *buff, int size )
{
  int count = 0;
  char *pointer;
  char index = -1;
  char ie_id;
  char pd;
  char msg_type;

  pointer = buff;
  
  /* the first byte is the protocol discriminator */
  pd = *(pointer + count++);
  
  /* then the call reference length and value */
  /* change the first bit to reverse direction */
  call_ref.len = *(pointer + count++);
  switch ( call_ref.len )
  {
    case 1:
      call_ref.value[0] = *(pointer + count++) ^ 0x80;
      break;
    case 2:
      call_ref.value[0] = *(pointer + count++) ^ 0x80;
      call_ref.value[1] = *(pointer + count++);
      break;
    default:
      printf("\nInvalid call reference length ");
      exit ( -1 );
      break;
  } 

    
  /* then the message type */
  msg_type = *(pointer + count++);

  /* other info can be gathered here */
  while ( count < size )
  {
    switch ( ie_id = *(pointer + count) )
    {
      case IE_CAUSE:
        ExtractCause( pointer + count, &cause );
        break;
      case IE_CHANNEL_ID:
        ExtractBchannel( pointer + count, &Bchannel );
        break;
      default:
        break;
    }
    
    if ( ie_id & 0x80 )     /* single octet ie */
      count++;
    else                    /* variable length ie */
      count += *(pointer + count + 1) + 2;

  }
  
  if ( pd == PD_Q931_CC )
  {
   switch ( msg_type )
   {
    case MSG_ALERTING:
      index = ALERTING_INDEX;
      break;
    case MSG_CALL_PROC:
      index = CALL_PROC_INDEX;
      break;
    case MSG_SETUP:
      index = SETUP_INDEX;
      break;
    case MSG_SETUP_ACK:
      index = SETUP_ACK_INDEX;
      break;
    case MSG_CONNECT:
      index = CONNECT_INDEX;
      break;
    case MSG_CONNECT_ACK:
      index = CONNECT_ACK_INDEX;
      break;
    case MSG_DISCONNECT:
      index = DISCONNECT_INDEX;
      break;
    case MSG_PROGRESS:
      index = PROGRESS_INDEX;
      break;
    case MSG_RELEASE:
      index = RELEASE_INDEX;
      break;
    case MSG_RELEASE_COMP:
      index = RELEASE_COMP_INDEX;
      break;
    default:
      break;
   }
  }

  ShowIsdnMessage(buff, 'i');

  CheckCommandList ( index, 'i' );

}

/*----------------------------------------------------------------------------
                            CheckCommandList()
----------------------------------------------------------------------------*/
void CheckCommandList( char index, char dir )
{
  switch ( dir )
  {
    case 'i':
        ExecuteCommand (command_list[index].in_cmd);
      break;
    case 'o':
        ExecuteCommand (command_list[index].out_cmd);
      break;
  }
}

/*----------------------------------------------------------------------------
                            ExecuteCommand()
----------------------------------------------------------------------------*/
void ExecuteCommand( char command )
{
  char * msg_pointer; 
  void * struct_pointer; 

  switch ( command )
  {
    case 'S':        /* send SETUP */
      struct_pointer = malloc( sizeof(SETUP_PARM)  );
      msg_pointer = malloc ( MAX_ISDN_BUFFER_SIZE ); 
      GetSetupDefaults ( ( SETUP_PARM *) struct_pointer );
	  if ( isdn_par.calling )
	    strcpy( (( SETUP_PARM *) struct_pointer)->calling_numb, isdn_par.calling ); 
	  if ( isdn_par.called )
	    strcpy( (( SETUP_PARM *) struct_pointer)->called_numb, isdn_par.called ); 
	  if ( isdn_par.Bchan )
	    (( SETUP_PARM *) struct_pointer)->channel_number = isdn_par.Bchan; 
        (( SETUP_PARM *) struct_pointer)->pref_excl = isdn_par.pref_excl;
      length_buf = BuildSetup ( msg_pointer, ( SETUP_PARM *) struct_pointer, call_ref );
      SendPacket( msg_pointer, DL_DA_RQ);
      free (struct_pointer);
      free (msg_pointer);
      /* check if something else has to be done */
      CheckCommandList( SETUP_INDEX, 'o' );
      break;
    case 's':        /* send SETUP ACK */
      struct_pointer = malloc( sizeof(SETUP_ACK_PARM)  );
      msg_pointer = malloc ( MAX_ISDN_BUFFER_SIZE );
      (( SETUP_ACK_PARM *)struct_pointer)->channel_number = Bchannel;	/* this was extracted from the SETUP */ 
      length_buf = BuildSetupAck ( msg_pointer, ( SETUP_ACK_PARM *) struct_pointer, call_ref );
      SendPacket( msg_pointer, DL_DA_RQ);
      free (struct_pointer);
      free (msg_pointer);
      /* check if something else has to be done */
      CheckCommandList( SETUP_ACK_INDEX, 'o' );
      break;
    case 'K':        /* send CALL PROCEEDING */
      struct_pointer = malloc( sizeof(CALL_PROC_PARM)  );
      msg_pointer = malloc ( MAX_ISDN_BUFFER_SIZE );
      (( CALL_PROC_PARM *)struct_pointer)->channel_number = Bchannel;	/* this was extracted from the SETUP */ 
      length_buf = BuildCallProc ( msg_pointer, ( CALL_PROC_PARM *) struct_pointer, call_ref );
      SendPacket( msg_pointer, DL_DA_RQ);
      free (struct_pointer);
      free (msg_pointer);
      /* check if something else has to be done */
      CheckCommandList( CALL_PROC_INDEX, 'o' );
      break;
    case 'A':        /* send ALERTING */
      struct_pointer = malloc( sizeof(ALERTING_PARM)  );
      msg_pointer = malloc ( MAX_ISDN_BUFFER_SIZE );
      (( ALERTING_PARM *)struct_pointer)->channel_number = Bchannel;	/* this was extracted from the SETUP */ 
      length_buf = BuildAlerting ( msg_pointer, ( ALERTING_PARM *) struct_pointer, call_ref );
      SendPacket( msg_pointer, DL_DA_RQ);
      free (struct_pointer);
      free (msg_pointer);
      /* check if something else has to be done */
      CheckCommandList( ALERTING_INDEX, 'o' );
      break;
    case 'C':        /* send CONNECT */
      msg_pointer = malloc ( MAX_ISDN_BUFFER_SIZE ); 
      length_buf = BuildEmptyMsg ( msg_pointer, MSG_CONNECT, call_ref );
      SendPacket( msg_pointer, DL_DA_RQ);
      free (msg_pointer);
      /* check if something else has to be done */
      CheckCommandList( CONNECT_INDEX, 'o' );
      break;
    case 'c':        /* send CONNECT ACK */
      msg_pointer = malloc ( MAX_ISDN_BUFFER_SIZE ); 
      length_buf = BuildEmptyMsg ( msg_pointer, MSG_CONNECT_ACK, call_ref );
      SendPacket( msg_pointer, DL_DA_RQ);
      free (msg_pointer);
      /* check if something else has to be done */
      CheckCommandList( CONNECT_ACK_INDEX, 'o' );
      break;
    case 'T':        /* start timer */
      if ( timer_T == 0 && timer_tx == 0 ) 
      {
        timer_T = 1;
        adiStartTimer( cntx.ctahd, timer_T_value, 1 ); 
        /* check if something else has to be done */
        CheckCommandList( TIMER_T_INDEX, 'o' );
      }
      break;
    case 't':        /* start timer */
      if ( timer_T == 0 && timer_tx == 0 ) 
      {
        timer_tx = 1;
        adiStartTimer( cntx.ctahd, timer_t_value, 1 ); 
        /* check if something else has to be done */
        CheckCommandList( TIMER_t_INDEX, 'o' );
      }
      break;
    case 'D':        /* send DISCONNECT */
      struct_pointer = malloc( sizeof(DISC_PARM)  );
      msg_pointer = malloc ( MAX_ISDN_BUFFER_SIZE );
      (( DISC_PARM *)struct_pointer)->msg_type = MSG_DISCONNECT; 
	  if ( isdn_par.cause )
        (( DISC_PARM *)struct_pointer)->cause = isdn_par.cause; 
	  else
        (( DISC_PARM *)struct_pointer)->cause = CAU_NORMAL_CC; 
      length_buf = BuildDisc ( msg_pointer, ( DISC_PARM *) struct_pointer, call_ref );
      SendPacket( msg_pointer, DL_DA_RQ);
      free (struct_pointer);
      free (msg_pointer);
      /* check if something else has to be done */
      CheckCommandList( DISCONNECT_INDEX, 'o' );
      break;
    case 'P':        /* send PROGRESS */
      struct_pointer = malloc( sizeof(PROGRESS_PARM)  );
      msg_pointer = malloc ( MAX_ISDN_BUFFER_SIZE );
      (( PROGRESS_PARM *)struct_pointer)->progress_desc = PRD_IN_BAND; 
      length_buf = BuildProgress ( msg_pointer, ( PROGRESS_PARM *) struct_pointer, call_ref );
      SendPacket( msg_pointer, DL_DA_RQ);
      free (struct_pointer);
      free (msg_pointer);
      /* check if something else has to be done */
      CheckCommandList( PROGRESS_INDEX, 'o' );
      break;
    case 'R':        /* send RELEASE */
      struct_pointer = malloc( sizeof(DISC_PARM)  );
      msg_pointer = malloc ( MAX_ISDN_BUFFER_SIZE );
      (( DISC_PARM *)struct_pointer)->msg_type = MSG_RELEASE; 
      (( DISC_PARM *)struct_pointer)->cause = 0; 
      length_buf = BuildDisc ( msg_pointer, ( DISC_PARM *) struct_pointer, call_ref );
      SendPacket( msg_pointer, DL_DA_RQ);
      free (struct_pointer);
      free (msg_pointer);
      /* check if something else has to be done */
      CheckCommandList( RELEASE_INDEX, 'o' );
      break;
    case 'r':        /* send RELEASE COMPLETE */
      struct_pointer = malloc( sizeof(DISC_PARM)  );
      msg_pointer = malloc ( MAX_ISDN_BUFFER_SIZE );
      (( DISC_PARM *)struct_pointer)->msg_type = MSG_RELEASE_COMP; 
      (( DISC_PARM *)struct_pointer)->cause = 0; 
      length_buf = BuildDisc ( msg_pointer, ( DISC_PARM *) struct_pointer, call_ref );
      SendPacket( msg_pointer, DL_DA_RQ);
      free (struct_pointer);
      free (msg_pointer);
      /* check if something else has to be done */
      CheckCommandList( RELEASE_COMP_INDEX, 'o' );
      break;

    default:
      break;
  }

}


/*----------------------------------------------------------------------------
                            ShowIsdnMessage()
  according with the verbose level shows the messages
  buff is the pointer to the Q.931 buffer
  dir is 'i' or 'o'
----------------------------------------------------------------------------*/
void ShowIsdnMessage ( char *buff, char dir )
{
  char msg_type,crl;
  static unsigned calls = 0;    /* counts the number of SETUP */

  if ( verbose == 0 )
    return;
  
  /* extract the message type (4th or 5th byte depending on the call ref length) */
  crl = *(buff + 1);
  msg_type = *(buff + crl + 2);
  
  if (dir == 'i')
    printf("\n received    ");
  else
    printf("\n transmitted ");

  if ( msg_type == MSG_SETUP )
  {
    if (dir == 'i')
      printf("SETUP n. %d", ++calls);
    else
      printf("SETUP n. %d", ++calls);
  }

  if ( verbose == 1 )
    return;

 
 switch ( msg_type )
  {
    case MSG_ALERTING:
      printf("ALERTING");
      break;
    case MSG_SETUP_ACK:
      printf("SETUP_ACK");
      break;
    case MSG_CALL_PROC:
      printf("CALL PROCEEDING");
      break;
    case MSG_CONNECT:
      printf("CONNECT");
      break;
    case MSG_CONNECT_ACK:
      printf("CONNECT_ACK");
      break;
    case MSG_DISCONNECT:
      printf("DISCONNECT");
      break;
    case MSG_PROGRESS:
      printf("PROGRESS");
      break;
    case MSG_STATUS:
      printf("STATUS");
      break;
    case MSG_RELEASE:
      printf("RELEASE");
      break;
    case MSG_RELEASE_COMP:
      printf("RELEASE_COMP");
      break;
  }

  if ( verbose == 3 )
  {
     /* print some stuff */
  }  

  fflush(stdout);
}

/******************************************************************************
        GetSetupDefaults
******************************************************************************/ 
void GetSetupDefaults ( SETUP_PARM *setup_p )
{
  setup_p->transfer_cap = ITC_SPEECH;  
  setup_p->transfer_rate = ITR_64K; 
  setup_p->encoding = UIL1_MULAW;     
  setup_p->pref_excl = PE_EXCL;     
  setup_p->channel_number = 1;     
  strcpy( setup_p->calling_numb, "1234");    
  strcpy( setup_p->called_numb, "67890");     

}

