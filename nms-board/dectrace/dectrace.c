/*****************************************************************************\
 * 
 *  FILE        : dectrace.c
 *  DESCRIPTION : NMS-ISDN trace decoding utility
 *  
 *****************************************************************************
 *
 *  Copyright 1999-2002 NMS Communications.
 * 
\*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dectrace.h"

#define MAX_CHAR 81    /* max num of char extracted */

#define NON_ISDN    0

/* global variables */
FILE *file_handle;
int log_to_file = 0;
FILE *IsdnlogF = NULL;
int ag_board = 0;
int dec_mask = TRACE_TIME | 
               TRACE_CR | 
               TRACE_MSG | 
               TRACE_IE | 
               TRACE_IE_CONT |
               TRACE_Q921;
int decode_isdn = TRACE_CR | TRACE_MSG | TRACE_IE | TRACE_IE_CONT
                  | TRACE_DL | TRACE_PD | TRACE_BUF | TRACE_Q921;

char decname[128];    /* trace file to be decoded */
long call_ref = -1;
int nai_num = -1;
int board_num = -1;
int group_num = -1;

/*-------------------------- Get Option Function ---------------------------*/
/* Unix-style parsing of command-line options, for systems that lack it. */
#ifndef UNIX
  extern char *optarg;
  extern int   optind;
  extern int   opterr;
  int getopt( int argc, char * const *argv, const char *optstring );
#endif

void main ( int argc, char **argv )
{
  int i,c;
  /* ISDN Message variables */
  char ent1, ent2, dir, code;
  char timestring[10];
  unsigned char board_number,nai_number, group_number;
  unsigned char isdnbuffer[256];    /* ISDN buffer */
  int numbytes;                     /* Length of ISDN buffer data*/    
  unsigned add;                     /* address */

  strcpy( decname, "agerror.log");

  while( ( c = getopt( argc, argv, "d:f:a:c:b:g:Hh?" ) ) != -1 )
  {
    switch( c )
    {
      case 'f': /* source file: default agerror.log */
        sscanf( optarg, "%s", decname );
        break;
      case 'd': /* decoding mask */
        sscanf( optarg, "%x", &dec_mask  );
        break;
      case 'c': /* decode only one call reference */
        sscanf( optarg, "%x", &call_ref  );
        break;
      case 'a': /* decode only one nai */
        sscanf( optarg, "%x", &nai_num  );
        break;
      case 'g': /* decode only one NFAS group */
        sscanf( optarg, "%x", &group_num  );
        break; 
      case 'b': /* decode only signals to/from one board */
        sscanf( optarg, "%d", &board_num  );
        break;
      default:
        PrintUsage();
        break;
    }
  }

  if ( strcmp( decname, "-" ) == 0 )    /* stdout */
  {
    file_handle = fdopen(0, "r");
    if ( file_handle == NULL )
    {
      printf("Error opening stdout\n");
      exit ( -1 );
    }
  }
  else                                    /* regular file */
  {
      /* open the file for reading */
      file_handle = fopen( decname, "r");
      if ( file_handle == NULL )
      {
        printf("Error opening the file %s\n", decname);
        exit ( -1 );
      }
  }
  while ( !feof(file_handle) )
  {
        int board,nai,group; /* Temporary storage vars */
        char *str;
        
        /************** START: read message at this point**********/ 
        
        /*----- Step 1. Find "OAM:" pattern -------*/
        for( str = "OMN:"; *str; ++str ) {
        c = getc(file_handle);
            if ((char)c != *str || c==EOF) break;
        }
        if (*str) continue; /* No pattern match */
        
        /*----- Step 2. Read message information---*/

        if ( fgets(isdnbuffer,255,file_handle) == NULL ) continue;

        /* OMNITEL buffer format for ISDN 1.4x and above  with code < 128 */
        if ( sscanf(isdnbuffer,
            "%x %10s dir=%*c %c=>%c, code=%c nai=%2x group=%2x "
            "sapi=%*2x add=%2x id=%*2x %6x",
            &board,
            timestring,
            /*&dir,*/
            &ent1,
            &ent2,
            &code,
            &nai,
            &group,
            &add,
            &numbytes) == 9) {} 
        /* OMNITEL buffer format for ISDN 1.4x and above  with code >= 128 */
        else if ( sscanf(isdnbuffer,
            "%x %10s dir=%*c %c=>%c, code=0x%2x nai=%2x group=%2x "
            "sapi=%*2x add=%2x id=%*2x %6x",
            &board,
            timestring,
            /*&dir,*/
            &ent1,
            &ent2,
            &c, /* Use 'int c' instead of 'uchar code' */ 
            &nai,
            &group,
            &add,
            &numbytes) == 9) { code = c & 0xFF ; } 
         /* OMNITEL buffer format for ISDN 1.3x and before */
         else if ( sscanf(isdnbuffer,
            "%x %10s dir=%*c %c=>%c, code=%c nai=%2x "
            "sapi=%*2x add=%2x inf=%*4x,%*x %6x",
            &board,
            timestring,
            /*&dir,*/
            &ent1,
            &ent2,
            &code,
            &nai,
            &add,
            &numbytes) == 8) { group = nai; }
         /* Invalid OMNITEL buffer format */
         else { continue;}

        
        /* Conversion from int to unsigned char */
        nai_number   = nai;  
        group_number = group;
        board_number = board;


        /*----- Step 3. Skip trash line before buffer ---*/
        if ( numbytes != 0 )
        {
            for(i=0; i==0 ;) 
            {
                fscanf(file_handle," %1s",isdnbuffer); 
                if (isxdigit(isdnbuffer[0]))
                {
                    ungetc(isdnbuffer[0],file_handle);
                    i = 1;
                }
                else if (fgets(isdnbuffer,255,file_handle) == NULL) i=2;
            }
            if (i != 1) continue;

            if ( numbytes > sizeof(isdnbuffer) ) continue;    /* message too big */ 
        }
        /*----- Step 4. Read buffer --------------------*/
        for(i=0;i<numbytes;i++) {
            if (fscanf(file_handle,"%2x",&c) != 1) break;
            isdnbuffer[i] = 0xff & c;
        }
        if (i != numbytes) continue; /* not all numbers was read */

        /************** END: message is read at this point**********/ 

        /* look for Q.921 and Q.931 messages */
        if ( decode_isdn &&
              ( ( ent2 == ENT_DL_D && ent1 == ENT_PH_D ) || 
                ( ent1 == ENT_DL_D && ent2 == ENT_PH_D ) ) )
        {
          if ( ent1 == ENT_PH_D )   /* direction */
              dir = INCOMING;
          else
              dir = OUTGOING;
          if (code == 'c' || code == 'C') /* PH_DA_IN and PH_DA_RQ */
          {
            /* do we look for a particular call reference and/or a
               particular nai and/or particular board ? */
            if ( ( ( call_ref == -1 ) || CheckCallRef( call_ref, isdnbuffer ) ) 
                && ( ( board_num == -1 ) || (board_num == board_number ) )
                && ( ( nai_num == -1 ) || (nai_num == nai_number ) )
                && ( ( group_num == -1 ) || (group_num == group_number ) ))
            {
                    DecodeQ921Code( isdnbuffer, 
                                    numbytes, 
                                    dec_mask, 
                                    board_number, 
                                    nai_number,
                                    group_number,
                                    dir, 
                                    timestring );
            }
          } /* end PH_DA_IN and PH_DA_RQ */
          else 
              DecodePH( dec_mask,
                        board_number,
                        nai_number,
                        group_number,
                        dir,
                        code,
                        timestring);

        } /* end look for Q.921 and Q.931 messages */
        else
        /* look for DL/NS messages */
        if ( decode_isdn && 
              ( ( ent2 == ENT_DL_D && ent1 == ENT_NS ) || 
                ( ent1 == ENT_DL_D && ent2 == ENT_NS ) )  )
        {
            switch (code)
            {
                case DL_DA_RQ:
                case DL_DA_IN:
                case DL_U_DA_RQ:
                case DL_U_DA_IN:
                    break;
                default:
                    if ( ent1 == ENT_DL_D )   /* direction */
                        dir = INCOMING;
                    else
                      dir = OUTGOING;
                    /* do we look for a particular nai and/or a
                       particular board ? */
                    if ( ( ( board_num == -1 ) || (board_num == board_number ) )
                       && ( ( nai_num == -1 ) || (nai_num == nai_number ) )
                       && ( ( group_num == -1 ) || (group_num == group_number ) ))
                    {
                          DecodeDLMessage( dec_mask,
                                           board_number, 
                                           nai_number, 
                                           group_number,
                                           dir, 
                                           code,
                                           timestring); 
                    }

                    break;
            }
        } /* end look for DL/NS messages */
        else
        /* look for ACU messages */
        if ( (dec_mask & TRACE_ACU) && (
             ( ( ent2 == ENT_CC &&
               ( ent1 == ENT_APPLI || ent1 == '+' ) ) || 
             ( ent1 == ENT_CC &&
               ( ent2 == ENT_APPLI || ent2 == '+' ) ) ) ) )
        {
          if ( ent1 == ENT_CC )   /* direction */
            dir = INCOMING;
          else
            dir = OUTGOING;
         /* do we look for a particular board ? */
          if ( ( ( board_num == -1 ) || (board_num == board_number ) )
               && ( ( nai_num == -1 ) || (nai_num == nai_number ) )
               && ( ( group_num == -1 ) || (group_num == group_number ) ))
          {
              DecodeAcuMessage( dec_mask,
                                board_number, 
                                nai_number, 
                                group_number,
                                dir, 
                                code,
                                add,
                                timestring); 
          }
        }  /* end look for ACU messages */
    
  } /* end while */
  
  /* close the file */
  fclose( file_handle );
}

/******************************************************************************
                                PrintUsage
******************************************************************************/
void PrintUsage ( void )
{
  printf("\nUSAGE: dectrace [-f source_file] [-d trace_mask] [-b board]");
  printf("\n                [-a nai] [-g group][-c call_ref]\n");
  printf("\n  source_file = file to be decoded (default agerror.log)");
  printf("\n  board = decode only the messages from/to this board (default all)");
  printf("\n  nai = decode only the messages on this nai (default all)");
  printf("\n  group = decode only the messages on this group (default all)");
  printf("\n  call_ref = decode only the messages with this call reference (default all)");
  printf("\n             Note: use hex notation (i.e. -c 0a00)");
  printf("\n  trace_mask = decoding mask (for Q.931 decoding)");
  printf("\n               (default TRACE_TIME | TRACE_MSG | TRACE_CR | TRACE_IE | ");
  printf("\n                TRACE_IE_CONT | ISDN_MSG | TRACE_Q921)");
  printf("\n    %04x =   TRACE_BUF      =  prints the whole hex buffer",TRACE_BUF); 
  printf("\n    %04x =   TRACE_PD       =  decodes the protocol discriminator",TRACE_PD); 
  printf("\n    %04x =   TRACE_CR       =  decodes the call reference",TRACE_CR); 
  printf("\n    %04x =   TRACE_MSG      =  decodes the message type",TRACE_MSG); 
  printf("\n    %04x =   TRACE_IE       =  decodes the information element id",TRACE_IE); 
  printf("\n    %04x =   TRACE_IE_CONT  =  decodes the data contents of the IE",TRACE_IE_CONT); 
  printf("\n    %04x =   TRACE_Q921     =  decodes the Q.921 primitives (SABME, RR...)",TRACE_Q921); 
  printf("\n    %04x =   TRACE_DL       =  decodes the commands/events at the DL interface",TRACE_DL); 
  printf("\n    %04x =   TRACE_TIME     =  prints a time stamp before the messages",TRACE_TIME);  
  printf("\n    %04x =   TRACE_PH       =  decodes the PH primitives",TRACE_PH); 
  printf("\n    %04x =   TRACE_ACU      =  decodes the ACU messages",TRACE_ACU); 

  exit( 0 );
}
/******************************************************************************
                                MyAtoi
   converts an ASCII char ( in hex format ) to a number
******************************************************************************/
char MyAtoi ( char value )
{
    if ( ( value >= '0' ) && ( value <= '9' ) ) /* digit */
      return ( value - '0' );

    if ( ( value >= 'a' ) && ( value <= 'f' ) ) /* lower case */
      return ( value - 'a' + 10 );
    
    if ( ( value >= 'A' ) && ( value <= 'F' ) ) /* upper case */
      return ( value - 'A' + 10 );

    return -1;   /* it's not a number */
}

/******************************************************************************
                                MyExit
******************************************************************************/
void MyExit ( void )
{
  /* close the file */
  fclose( file_handle );
  exit ( 0 );
}


/******************************************************************************
                                CheckCallRef
  The following function checks whether the call reference in the buffer
  excluding the first bit (flag) corresponde to value. 
  In this case it returns 1 otherwise it returns 0.
  The call reference information element starts after the layer 1 header and 
  the protocol discriminator at buff[5]:
  buff[5] = call ref length
  buff[6] ( and optionally buff [7] ) = call ref value

******************************************************************************/
int CheckCallRef ( long value, unsigned char *buff )
{
    long value2;

    if ( *(buff + 5) == 1)  /* 1 byte call reference */
      value2 = *( buff + 6 ) & 0x7f;
    else                    /* 2 bytes call reference */
      value2 = (*( buff + 6 ) & 0x7f) * 0x100 + *( buff + 7 ) ;

    if ( value == value2 ) 
      return 1;
    else
      return 0;
}

/*****************************************************************************
                                getopt

 This routine is based on a UNIX standard for parsing options in command
 lines, and is provided because some system libraries do not include it.
 'argc' and 'argv' are passed repeatedly to the function, which returns
 the next option letter in 'argv' that matches a letter in 'optstring'.

 'optstring' must contain the option letters the command will recognize;
 if a letter in 'optstring' is followed by a colon, the option is expected
 to have an argument, or group of arguments, which must be separated from
 it by white space.  Option arguments cannot be optional.

 'optarg' points to the start of the option-argument on return from getopt.

 getopt places in 'optind' the 'argv' index of the next argument to be
 processed.  'optind' has an initial value of 1.

 When all options have been processed (i.e., up to the first non-option
 argument, or the special option '--'), getopt returns -1.  All options
 must precede other operands on the command line.

 Unless 'opterr' is set to 0, getopt prints an error message on stderr
 and returns '?' if it encounters an option letter not included in
 'optstring', or no option-argument after an option that expects one.
*****************************************************************************/
#ifndef UNIX
char *optarg = NULL;
int optind = 1;
int opterr = 1;

int getopt( int argc, char * const *argv, const char *optstring )
{
    static int   multi = 0;  /* remember that we are in the middle of a group */
    static char *pi = NULL;  /* .. and where we are in the group */

    char *po;
    int c;

    if (optind >= argc)
        return -1;

    if (!multi)
    {
        pi = argv[optind];
        if (*pi != '-' && *pi != '/')
            return -1;
        else
        {
            c = *(++pi);

            /* test for null option - usually means use stdin
             * but this is treated as an arg, not an option
             */
            if (c == '\0')
                return -1;
            else if (c == '-')
            {
                /*/ '--' terminates options */
                ++ optind;
                return -1;
            }
        }

    }
    else
        c = *pi;

    ++pi;

    if( ! isalnum(c)
        || (po=strchr( optstring, c )) == NULL)
    {
        if (opterr && c != '?')
            fprintf (stderr, "%s: illegal option -- %c\n", argv[0], c);
        c = '?';
    }
    else if (*(po+1) == ':')
    {
        /* opt-arg required.
         * Note that the standard requires white space, but current
         * practice has it optional.
         */
        if (*pi != '\0')
        {
            optarg = pi;
            multi = 0;
            ++ optind;
        }
        else
        {
            multi = 0;
            if (++optind >= argc)
            {
                if (opterr)
                    fprintf (stderr,
                        "%s: option requires an argument -- %c\n", argv[0],c);
                return '?';
            }
            optarg = argv[optind++];
        }
        return c;
    }

    if (*pi == '\0')
    {
        multi = 0;
        ++ optind;
    }
    else
    {
        multi = 1;
    }
    return c;
}
#endif


