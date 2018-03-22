/*****************************************************************************
*  Filename : clockdemo.c
*
*  Description : Monitors clock status information on AG board(s) specified.
*                With no command line arguments it defaults to 3 boards,
*                0,1 and 2.
*
*  Usage : clockdemo [-bN] where N is the board number whose clock status is
*          to be monitored. Upto a maximum of MAX_NO_OF_BOARDS boards can be
*          monitored at a time. With no cmd line entries,
*          default to -b0, -b1, -b2
*
*          !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*          !!!!!! N0  <NETREF> clock is considered in this demo program !!!!
*          !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*
*          #################################################################
*          To run this demo, one should NOT stop the CFBM service. However,
*          to achieve the best result, use OAM and configure all boards as
*          STANDALONE then run this demo program after all boards boot
*          #################################################################
*
*  Copyright 1999-2002 NMS Communications.
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#if defined (WIN32)
#include <conio.h>
#define kbhit _kbhit
#define getch _getch
#else
#include <sys/time.h>
#define getch getchar
#include <unistd.h>
int kbhit(void);
#endif

#include "ctadef.h"
#include "swidef.h"
#include "oamdef.h"

#include "clockresource.h"

#define VERSION "2.0"

#define MAX_NO_OF_BOARDS 16

#define MAX_STATUS_NAME_SIZE 9

/****************************************************************************************/
#define CLKSYS_STATUS_FALLBACK_OCCURRED     0x01
#define CLKSYS_STATUS_A_FAIL                0x02
#define CLKSYS_STATUS_B_FAIL                0x04
#define CLKSYS_STATUS_INCONSISTENT          0x08
#define CLKSYS_STATUS_REFERENCE_CHANGED     0x10

#define CLKSYS_ACTION_NONE                      0x0
#define CLKSYS_ACTION_RELOAD                    0x1
#define CLKSYS_ACTION_NEW_PRIMARY               0x2
#define CLKSYS_ACTION_NEW_SECONDARY             0x4
#define CLKSYS_ACTION_RELOAD_PRIMARY_FALLBACK   0x8

/******************************************************************************************/
char * RptHeader1     = " Boards   Clock Mode     Current      Mastering      Fallback\n";
char * RptHeader2     = "                      Clock Source  A Clock B Clock  Occurred\n";
char * RptLine        = "-------- ------------ ------------  ------- -------  --------\n";
char * RptLineFormat  = "  %2d:   %-12s %-12s  %-7s %-7s  %-8s\n";
char * RptFooter      = "(Press 'q' then ENTER to Exit.)\n\n";

static int  board_array[MAX_NO_OF_BOARDS];
char serverAddr[64] = {"localhost"};
int _id2index(int board_id, int *index);
int _index2id(int index, int *board_id);

/******************************************************************************************/
static DWORD _initializeData (SWI_CLOCK_ARGS * p_parms,
                              int              board_array[],
                              int            * p_numBoards,
                              char           * file_name);
static void  _printClockStatus (SWI_QUERY_CLOCK_ARGS * data,  int boardNum);
static void  _printClockConfiguration (SWI_CLOCK_ARGS * data, int boardNum);

static void  _getStatusName (DWORD status, char * psz_status);
static void  _usage (char * err_msg, char * prg);
static void  _banner(void);

static void  _cta_init(CTAQUEUEHD  *pctaqueuehd, CTAHD *pctahd);
void static  _cta_Shutdown( CTAHD ctahd );

static int   edebug=0;
static void  _debug(char* s){ if(edebug)printf("%s", s);};

/*****************************************************************************
  banner
*****************************************************************************/
static void _banner(void)
    {
    puts ("\nCLOCKDEMO Version " VERSION " " __DATE__ " " __TIME__);
    }
/*****************************************************************************
  help
*****************************************************************************/
static void _help(void)
    {
    _banner();
    puts ("\n This demo program provides an example of how to manage clocks at the");
    puts ("   system level. The user specifies a list of boards that need to be ");
    puts ("   synchronized. An optional file (recommended) specifies the relative ");
    puts ("   priority of timing references. priority.txt is provided as an example.");
    puts ("   The demo configures the clocks for each board in the list.");
    puts ("   It then monitors the status of the timing     ");
    puts ("   references and the h100/h110 clocks. Upon detecting any status change,");
    puts ("   it reconfigures the boards' clocks to maintain the system synchronization.");
    puts ("\n ");
    }


/*****************************************************************************
  usage
*****************************************************************************/
static void _usage (char * err_msg, char * prg)
    {
    if (err_msg)
        printf ("\n%s\n", err_msg);
    printf ("Usage:\n");
    printf ("\t%s [-@<server> -bN -bN ... -f<File> -d -t<msec>] \n", prg);
    printf ("\t  Where:\n");
    printf( "\t    -@ server -- Server host name or IP address, default = localhost.\n" );
    printf ("\t    -b N      -- a board to be monitored (N is a board number)\n");
    printf ("\t    -f File   -- configure boards using reference priority in File\n");
    printf ("\t                 (see priority.txt for syntax) \n");
    printf ("\t    -d        -- enable debug messages \n");
    printf ("\t    -t msec   -- set clock polling frequency ( default 2000)\n");
    /*** reserve option -r if user stop 'cfbm' and  use this demo to configure the clocks ***/
    //    printf ("\t    -r  -- run this demo to replace 'cfbm'\n");
    printf ("\t    -h        -- functional descriptions of this demo\n");
    printf ("\n\n");
    printf ("Note: all boards listed with '-b' options or listed in 'File' will be monitored\n");

    }

/*****************************************************************************
  _initiData_file
******************************************************************************/
static DWORD _initData_file ( int            board_array[],
                              int            *p_numBoards,
                              char           *file_name)
    {

    /*================================================================================

      The input file contains a list of priority for
      all available timing references. The priority list should be kept in
      a global data base and the status should be kept updated by the _clock_monitor()
      and the OAM events ( eg boards up and down). The data can then be used
      by _choose_new_primary() or _choose_new_secondary() to keep system in sync.

        This module should also establish initial configuration for all baords (h110_parms[]).


            Example of file will look like

              #### Priority      Board#   Source
              0             0        1          ### Board 0 Trunk 1
              1             0        2          ### Board 0 Trunk 2
              3             1        1          ### Board 1 Trunk 1
              4             1        0          ### Board 1 OSC

    ==================================================================================*/

    FILE *cfg;
    char line[256], comment[80], *ch;
    int priority, board, source;
    int fieldsread;
    int line_num, ret;
    int i, nBoard;

    cfg = fopen(file_name, "r");

    if(cfg == NULL)
        {
        printf("Failed to open file %s\n", file_name);
        return -1;
        }

    fseek( cfg, 0L, SEEK_SET );

    nBoard = *p_numBoards;
    line_num=0;

    while( fgets( line, 256, cfg) != NULL)
        {
        line_num++;
        ch = line;
        /* skip the white space */
        while(*ch == ' ' || *ch == 0x09) ch++;
        /* if comment or CR  or end_of_line skip the line */
        if(*ch == '#' || *ch == 0x0d || *ch == 0) continue;
        fieldsread = sscanf(line, "%d %d %d %s\n", &priority, &board, &source, comment);
        if(fieldsread < 3)
            {
            printf("Incorrect format in line %d (Priority  Board  Trunk)\n", line_num);
            continue;
            }
        /* add to monitor list */
        ret = ClkRsc_add_to_list(board, source, priority);
        /* see if board in the interested list */
        if(nBoard)
            {
            for(i=0; i<nBoard; i++)
                if(board_array[i] == board) break;
                /* if already in the list, continue ot next line */
                if(i < nBoard) continue;
            }
        board_array[nBoard] = board;
        nBoard++;
        }

    *p_numBoards = nBoard;


    return SUCCESS;
    }


/*****************************************************************************
  _initiData_oam
*****************************************************************************/
static int  _initData_oam ( int board_array[],
                            int *pn_boards,
                            CTAHD ctahd)
    {
    /*=====================================================================
    Read configurationdata from OAM data base.
    This require use of

      oamOpenObject()
      oamCloseObjec()
      oamGetKeyword()     ... etc

        and the knowledge of what keywords are used to configure the board clock
        Eventually, we establish a list of timing reference priority)


    ======================================================================*/


    return SUCCESS;
    }

/*****************************************************************************
  _printClockStatus
*****************************************************************************/
static void _printClockStatus (SWI_QUERY_CLOCK_ARGS * data,
                               int boardNumber)
    {
    char * p_clockMode = "", *p_fallback = "";
    char   sz_clockSrc [20] = " ";
    char   sz_a_status [MAX_STATUS_NAME_SIZE] = "";
    char   sz_b_status [MAX_STATUS_NAME_SIZE] = "";

    /* no query data available */
    if (data->size == 0) return;

    if (data->clocktype != MVIP95_H100_CLOCKING)
        {
        printf ("  %2d      NON H100 CLOCK TYPE.\n", boardNumber);
        return;
        }

    /*-------------------------------------------*/
    switch (data->ext.h100.clockmode)
        {
        case MVIP95_H100_MASTER_A   :
            if(data->clocksource == MVIP95_SOURCE_H100_B )
                p_clockMode = "SECONDARY";
            else
                p_clockMode = "PRIMARY";
            sprintf(sz_a_status, "YES");
            break;

        case MVIP95_H100_MASTER_B   :
            if(data->clocksource == MVIP95_SOURCE_H100_A )
                p_clockMode = "SECONDARY";
            else
                p_clockMode = "PRIMARY";
            sprintf(sz_b_status, "YES");
            break;
        case MVIP95_H100_SLAVE      :
            p_clockMode = "SLAVE";
            break;
        case MVIP95_H100_STAND_ALONE:
            p_clockMode = "STAND_ALONE";
            break;
        default:
            p_clockMode = "Unrecognized";
        }

    /*-------------------------------------------*/
    switch (data->clocksource)
        {
        case MVIP95_SOURCE_INTERNAL:
            sprintf (sz_clockSrc, "INTERNAL");
            break;

        case MVIP95_SOURCE_NETWORK:
            sprintf (sz_clockSrc, "NETWORK %lu", data->network);
            break;

        case MVIP95_SOURCE_H100_A:
            sprintf (sz_clockSrc, "H100_A");
            break;

        case MVIP95_SOURCE_H100_B:
            sprintf (sz_clockSrc, "H100_B");;
            break;

        case MVIP95_SOURCE_H110_NETREF_1:
            sprintf (sz_clockSrc, "NETREF_1");
            break;

        case MVIP95_SOURCE_H110_NETREF_2:
            sprintf (sz_clockSrc, "NETREF_2");
            break;

        default: /* if don't know the source, clean up the other information */
            sprintf (sz_clockSrc, "unrecognized");
            p_clockMode = "Unrecognized";
            sprintf(sz_a_status, " ");
            sprintf(sz_b_status, " ");

        } /* end of switch(data->clocksource) */



    if (data->clocktype != MVIP95_H100_CLOCKING)
        {
        p_fallback = "XX";
        }
    else
        {
        switch (data->ext.h100.fallbackoccurred)
            {
            case MVIP95_H100_NO_FALLBACK_OCCURRED: p_fallback = "NO";  break;
            case MVIP95_H100_FALLBACK_OCCURRED   : p_fallback = "YES"; break;
            default: p_fallback = "???"; break;
            }
        }

    printf (RptLineFormat, boardNumber, p_clockMode ,
        sz_clockSrc,
        sz_a_status, sz_b_status ,
        p_fallback);
    }

/*****************************************************************************
  _getStatusName
    Not Use -> utility program "SHOWCLKS" provides more status information
*****************************************************************************/
static void _getStatusName (DWORD status, char * psz_status)
    {
    switch (status)
        {
        case MVIP95_CLOCK_STATUS_GOOD   : strcpy (psz_status, "GOOD");   break;
        case MVIP95_CLOCK_STATUS_BAD    : strcpy (psz_status, "BAD");    break;
        case MVIP95_CLOCK_STATUS_UNKNOWN: strcpy (psz_status, "Unknown");break;
        default                         : strcpy (psz_status, "???");
        }
    }

/*****************************************************************************
  _printClockConfiguration
*****************************************************************************/
static void _printClockConfiguration (SWI_CLOCK_ARGS * data,
                                      int boardindex)
    {
    char * p_clockMode = "", *p_fallback = "";
    char   sz_clockSrc [20] = " ";
    char   sz_fallbackSrc [20] = "";
    char   debug_str[80];
    int id;

    /* no query data available */
    if (data->size == 0) return;
    _index2id(boardindex,&id);

    sprintf(debug_str, "\n========Configuring board %d...\n", id);
    _debug (debug_str);

    if (data->clocktype != MVIP95_H100_CLOCKING)
        {
        _debug ("\t\t      NON H100 CLOCK TYPE.\n");
        return;
        }


    /*-------------------------------------------*/
    switch (data->ext.h100.clockmode)
        {
        case MVIP95_H100_MASTER_A   :
            p_clockMode = "MASTER_A";
            break;
        case MVIP95_H100_MASTER_B   :
            p_clockMode = "MASTER_B";
            break;
        case MVIP95_H100_SLAVE      :
            p_clockMode = "SLAVE";
            break;
        case MVIP95_H100_STAND_ALONE:
            p_clockMode = "STAND_ALONE";
            break;
        default:
            p_clockMode = "Unrecognized";
        }

    /*-------------------------------------------*/
    switch (data->clocksource)
        {
        case MVIP95_SOURCE_INTERNAL:
            sprintf (sz_clockSrc, "INTERNAL");
            break;

        case MVIP95_SOURCE_NETWORK:
            sprintf (sz_clockSrc, "NETWORK %lu", data->network);
            break;

        case MVIP95_SOURCE_H100_A:
            sprintf (sz_clockSrc, "H100_A");
            break;

        case MVIP95_SOURCE_H100_B:
            sprintf (sz_clockSrc, "H100_B");;
            break;

        case MVIP95_SOURCE_H110_NETREF_1:
            sprintf (sz_clockSrc, "NETREF_1");
            break;

        case MVIP95_SOURCE_H110_NETREF_2:
            sprintf (sz_clockSrc, "NETREF_2");
            break;

        default: /* if don't know the source, clean up the other information */
            sprintf (sz_clockSrc, "unrecognized");
            p_clockMode = "Unrecognized";

        } /* end of switch(data->clocksource) */

    switch (data->ext.h100.autofallback)
        {
        case MVIP95_H100_ENABLE_AUTO_FB      : p_fallback = "ENABLE";  break;
        case MVIP95_H100_DISABLE_AUTO_FB     : p_fallback = "DISABLE"; break;
        default: p_fallback = "???"; break;
        }

    switch (data->ext.h100.fallbackclocksource)
        {
        case MVIP95_SOURCE_INTERNAL:
            sprintf (sz_fallbackSrc, "INTERNAL");
            break;

        case MVIP95_SOURCE_NETWORK:
            sprintf (sz_fallbackSrc, "NETWORK %lu", data->ext.h100.fallbacknetwork);
            break;

        case MVIP95_SOURCE_H100_A:
            sprintf (sz_fallbackSrc, "H100_A");
            break;

        case MVIP95_SOURCE_H100_B:
            sprintf (sz_fallbackSrc, "H100_B");;
            break;

        case MVIP95_SOURCE_H110_NETREF_1:
            sprintf (sz_fallbackSrc, "NETREF_1");
            break;

        case MVIP95_SOURCE_H110_NETREF_2:
            sprintf (sz_fallbackSrc, "NETREF_2");
            break;

        default: /* if don't know the source */
            sprintf (sz_fallbackSrc, "unrecognized");

        } /* end of switch(data->ext.h100.fallbacksource) */


    sprintf(debug_str, "\t\tclocksource        : %s\n", sz_clockSrc);
    _debug (debug_str);
    sprintf(debug_str, "\t\tclockmode          : %s\n", p_clockMode);
    _debug (debug_str);
    sprintf(debug_str, "\t\tautofallback       : %s\n", p_fallback);
    _debug (debug_str);
    sprintf(debug_str, "\t\tfallbackclocksource: %s\n", sz_fallbackSrc);
    _debug (debug_str);

    }

/*****************************************************************************
  _choose_new_primary()

*****************************************************************************/
static int _choose_new_primary(int clock_status, int secondary_clock,
                               SWI_QUERY_CLOCK_ARGS h110_query_parms[],
                               int num_of_boards,
                               int *pnew_primary,
                               SWI_CLOCK_ARGS *pnew_parms)
    {
    int i, promote_secondary = 0, mode, id;
    int primary_index, primary_source, primary_backup;
    CLKDEMO_TIMING_REFERENCE *select_source;

    switch(secondary_clock)
        {
        case MVIP95_SOURCE_H100_A:
            if( (clock_status & CLKSYS_STATUS_A_FAIL) == 0)
                {
                promote_secondary = 1;
                mode =  MVIP95_H100_MASTER_A;
                }
            break;
        case MVIP95_SOURCE_H100_B:
            if( (clock_status & CLKSYS_STATUS_B_FAIL) == 0)
                {
                promote_secondary = 1;
                mode = MVIP95_H100_MASTER_B;
                }
            break;
        default:
            break;
        }

    if(promote_secondary)
        {
        /* find the current secondary and set the current source as new_primary source.
        Here we lack of available timing reference data base, so set it fallback to OSC */

        for(i=0; i<num_of_boards; i++)
            {
            if(h110_query_parms[i].size == 0 ) continue;
            if(h110_query_parms[i].ext.h100.clockmode == (unsigned)mode) break;
            }
        if(i<num_of_boards)
            {
            /* first find new backup trunk */
            _index2id(i, &id);
            /* embedded trunk number in the upper 16 bits */
            id |= (h110_query_parms[i].network << 16);
            select_source = ClkRsc_get_reference(CLKRULE_SECOND_IN_PRIORITY_SAME_BOARD, id);
            if(select_source == NULL)
                {
                primary_backup = 0;  /* use OSC */
                }
            else
                {
                primary_backup = select_source->Trunk_id;
                }

            pnew_parms->clocktype                  =  MVIP95_H100_CLOCKING;
            pnew_parms->clocksource                =  h110_query_parms[i].clocksource;
            pnew_parms->network                    =  h110_query_parms[i].network;
            pnew_parms->ext.h100.clockmode         =  mode;
            pnew_parms->ext.h100.autofallback      =  MVIP95_H100_ENABLE_AUTO_FB;
            pnew_parms->ext.h100.netrefclockspeed  =  MVIP95_H100_NETREF_8KHZ;
            pnew_parms->ext.h100.fallbackclocksource   =  (primary_backup)? MVIP95_SOURCE_NETWORK: MVIP95_SOURCE_INTERNAL ;
            pnew_parms->ext.h100.fallbacknetwork       =  primary_backup;
            *pnew_primary = i;
            }
        else
            {
            printf(" Some Thing is very wrong here 1\n");
            return 1;
            }
        }
    else
        {
        /* make sure there is at least one board available */
        for(i=0; i<num_of_boards; i++)
            {
            if(h110_query_parms[i].size == 0 ) continue;
            break;
            }
        if(i<num_of_boards)
            {
            /* first find new primary board/trunk */
            select_source = ClkRsc_get_reference(CLKRULE_FIRST_IN_PRIORITY,0);
            if(select_source == NULL)
                {
                primary_index = i;  /* first available board */
                primary_source = 0; /* OSC */

                }
            else
                {
                _id2index(select_source->Board_id, &primary_index);
                primary_source = select_source->Trunk_id;
                }
            /* select the backup trunk */
            _index2id(primary_index, &id);
            /* embedded trunk number in the upper 16 bits */
            id |= (primary_source << 16);

            select_source = ClkRsc_get_reference(CLKRULE_SECOND_IN_PRIORITY_SAME_BOARD, id);
            if(select_source == NULL)
                {
                primary_backup = 0;  /* use OSC */
                }
            else
                {
                primary_backup = select_source->Trunk_id;
                }

            pnew_parms->clocktype                  =  MVIP95_H100_CLOCKING;
            pnew_parms->clocksource                =  (primary_source)?MVIP95_SOURCE_NETWORK:MVIP95_SOURCE_INTERNAL;
            pnew_parms->network                    =  primary_source;
            pnew_parms->ext.h100.clockmode         =  mode;
            pnew_parms->ext.h100.autofallback      =  MVIP95_H100_ENABLE_AUTO_FB;
            pnew_parms->ext.h100.netrefclockspeed  =  MVIP95_H100_NETREF_8KHZ;
            pnew_parms->ext.h100.fallbackclocksource   =  (primary_source)?MVIP95_SOURCE_NETWORK:MVIP95_SOURCE_INTERNAL;
            pnew_parms->ext.h100.fallbacknetwork       =  primary_backup;
            *pnew_primary = primary_index;
            }
        else
            {
            printf(" Some Thing is very wrong here 2\n");
            return 2;
            }


        }

    return SUCCESS;
    }

/*****************************************************************************
  _choose_new_secondary()

*****************************************************************************/
static int _choose_new_secondary ( int primary_index, int primary_clock,
                                   SWI_QUERY_CLOCK_ARGS h110_query_parms[],
                                   int num_of_boards,
                                   int *pnew_secondary,
                                   SWI_CLOCK_ARGS *pnew_parms)
    {
    int i, mode, secondary_index, secondary_source, id;
    CLKDEMO_TIMING_REFERENCE *select_source;


    /* make sure there is at least two boards available */
    for(i=0; i<num_of_boards; i++)
        {
        if(h110_query_parms[i].size == 0 ) continue;
        if(i == primary_index) continue;
        break;
        }
    if(i<num_of_boards)
        {
        switch(primary_clock)
            {
            case MVIP95_SOURCE_H100_A:
                mode =  MVIP95_H100_MASTER_B;
                break;
            case MVIP95_SOURCE_H100_B:
                mode = MVIP95_H100_MASTER_A;
                break;
            default:
                mode = MVIP95_H100_STAND_ALONE;
                break;
            }

        /* select the new secondary */
        _index2id(primary_index, &id);
        select_source = ClkRsc_get_reference(CLKRULE_FIRST_IN_PRIORITY_DIFF_BOARD, id);
        if(select_source == NULL)
            {
            secondary_index = i;
            secondary_source = 0;  /* use OSC */
            }
        else
            {
            _id2index(select_source->Board_id, &secondary_index);
            secondary_source = select_source->Trunk_id;
            }


        pnew_parms->clocktype                  =  MVIP95_H100_CLOCKING;
        pnew_parms->clocksource                =  primary_clock;
        pnew_parms->network                    =  0;
        pnew_parms->ext.h100.clockmode         =  mode;
        pnew_parms->ext.h100.autofallback      =  MVIP95_H100_ENABLE_AUTO_FB;
        pnew_parms->ext.h100.netrefclockspeed  =  MVIP95_H100_NETREF_8KHZ;
        pnew_parms->ext.h100.fallbackclocksource   =  (secondary_source)?MVIP95_SOURCE_NETWORK:MVIP95_SOURCE_INTERNAL;
        pnew_parms->ext.h100.fallbacknetwork       =  secondary_source;
        *pnew_secondary = secondary_index;
        }
    else
        {
        _debug(" Some Thnig may be very wrong here 3\n");
        return 3;
        }

    return SUCCESS;
    }

/*****************************************************************************
  _clock_monitor()
    check if the clock status on board has changed
*****************************************************************************/

static int _clock_monitor(SWI_QUERY_CLOCK_ARGS h110_query_parms[],
                          int num_of_boards, SWIHD swihd_array[] )
    {
    int i, rc, ret=0;
    int valid_report=0, valid_A_failure=0, valid_B_failure=0;
    int valid_A_good=0, valid_B_good=0;
    int any_fallback=0;

    char debug_str[80];

    /* Poll the status of all boards */
    for (i=0; i < num_of_boards; i++)
        {
        if(swihd_array[i] == 0)
            {

            /* If board just goes out, mark it fallback occurred, so that
            dispaly can be updated. Note that _clock_control() should be smart
            enough not to set clock for boards that configuration is not changed */
            if(h110_query_parms[i].size != 0)
                any_fallback = 1;
            h110_query_parms[i].size = 0;
            continue;
            }

            /* If board just come back, mark it fallback occurred, so that
            configuration can be loaded by INCONSISTENT flag ( even if the comeback
            did not cause any clock change )  */
        if( h110_query_parms[i].size == 0)
            {
            any_fallback = 1;
            }

        rc = swiGetBoardClock (swihd_array[i],
            MVIP95_H100_CLOCKING,
            &(h110_query_parms[i]),
            sizeof(h110_query_parms[0]));
        if (rc != SUCCESS)
            {
            /* mark the board's clock status is invalid */
            h110_query_parms[i].size = 0;
            any_fallback = 0;  /* reset, if set previously and read fail */
            }
        }

    /* check if any board has reported fallback or clock failure */

    for(i=0; i<num_of_boards; i++)
        {
        if(h110_query_parms[i].size == 0) continue;
        valid_report++;
        switch (h110_query_parms[i].ext.h100.fallbackoccurred)
            {
            case MVIP95_H100_NO_FALLBACK_OCCURRED: break;
            case MVIP95_H100_FALLBACK_OCCURRED   : any_fallback = 1; break;
            default: break;
            }

        switch (h110_query_parms[i].ext.h100.clockstatus_a)
            {
            case MVIP95_CLOCK_STATUS_GOOD: valid_A_good++;   break;
            case MVIP95_CLOCK_STATUS_BAD : valid_A_failure++; break;
            default: break;
            }

        switch (h110_query_parms[i].ext.h100.clockstatus_b)
            {
            case MVIP95_CLOCK_STATUS_GOOD: valid_B_good++;   break;
            case MVIP95_CLOCK_STATUS_BAD : valid_B_failure++; break;
            default: break;
            }

        }

    /* set the report bits */
    if(any_fallback) ret |= CLKSYS_STATUS_FALLBACK_OCCURRED;
    if(valid_A_failure >= valid_A_good) ret |= CLKSYS_STATUS_A_FAIL;
    if(valid_B_failure >= valid_B_good) ret |= CLKSYS_STATUS_B_FAIL;

    /* if there is a fallback but no report of clock failure, set inconsistent bit */
    if((ret & (CLKSYS_STATUS_FALLBACK_OCCURRED | CLKSYS_STATUS_A_FAIL | CLKSYS_STATUS_B_FAIL)) == CLKSYS_STATUS_FALLBACK_OCCURRED)
        ret |= CLKSYS_STATUS_INCONSISTENT;

    /* update trunk status */
    rc = ClkRsc_update_status(swihd_array);
    if(rc)
        ret |= CLKSYS_STATUS_REFERENCE_CHANGED;


    sprintf(debug_str, "Monitor return 0x%x\n", ret);
    _debug(debug_str);

    return ret;

    }


/*****************************************************************************
  _find_master()
    determine which board is driving the master clock
*****************************************************************************/
static int _find_master(SWI_CLOCK_ARGS h110_parms[], int num_of_boards,
                        int *priboard, int *priclock,
                        int *secboard, int *secclock)
    {
    int i, primary_clock, secondary_clock;
    int primary_board, secondary_board;

    char debug_str[80];


    primary_clock = secondary_clock = MVIP95_SOURCE_DISABLE;
    primary_board = secondary_board = -1;

    /* find the primary and secondary clock */
    for(i=0; i<num_of_boards; i++)
        {
        if((h110_parms[i].ext.h100.clockmode ==  MVIP95_H100_MASTER_A) &&
            (h110_parms[i].clocksource        !=  MVIP95_SOURCE_H100_B)   )
            {
            if(primary_clock != MVIP95_SOURCE_DISABLE)
                printf("ERROR: more than one primary master in the system \n");
            else
                {
                primary_clock   = MVIP95_SOURCE_H100_A;
                primary_board   = i;
                }
            }
        if((h110_parms[i].ext.h100.clockmode ==  MVIP95_H100_MASTER_B) &&
            (h110_parms[i].clocksource        !=  MVIP95_SOURCE_H100_A)   )
            {
            if(primary_clock != MVIP95_SOURCE_DISABLE)
                printf("ERROR: more than one primary master in the system\n");
            else
                {
                primary_clock   = MVIP95_SOURCE_H100_B;
                primary_board   = i;
                }
            }

        if((h110_parms[i].ext.h100.clockmode ==  MVIP95_H100_MASTER_A) &&
            (h110_parms[i].clocksource        ==  MVIP95_SOURCE_H100_B)   )
            {
            if(secondary_clock != MVIP95_SOURCE_DISABLE)
                printf("ERROR: more than one secondary master in the system\n");
            else
                {
                secondary_clock   = MVIP95_SOURCE_H100_A;
                secondary_board   = i;
                }
            }
        if((h110_parms[i].ext.h100.clockmode ==  MVIP95_H100_MASTER_B) &&
            (h110_parms[i].clocksource        ==  MVIP95_SOURCE_H100_A)   )
            {
            if(secondary_clock != MVIP95_SOURCE_DISABLE)
                printf("ERROR: more than one secondary master in the system\n");
            else
                {
                secondary_clock   = MVIP95_SOURCE_H100_B;
                secondary_board   = i;
                }
            }

        }

    *priclock = primary_clock;
    *secclock = secondary_clock;
    *priboard = primary_board;
    *secboard = secondary_board;

    sprintf(debug_str," pclk 0x%x, pbrd %d, sclk 0x%x, sbrd %d\n",
        primary_clock, primary_board,
        secondary_clock, secondary_board);
    _debug(debug_str);

    return 0;
    }


/*****************************************************************************
  _clock_control()
    determine if clock reconfiguration is required
*****************************************************************************/
static int _clock_control(int clock_status,
                          SWI_CLOCK_ARGS h110_parms[],
                          SWI_QUERY_CLOCK_ARGS h110_query_parms[],
                          int num_of_boards)
    {
    int ret,  rc, i, primary_clock, secondary_clock, new_primary_clock;
    int primary_index, secondary_index, new_primary_index, new_secondary_index;
    SWI_CLOCK_ARGS new_primary_parms, new_secondary_parms;
    int id, id2, primary_backup;
    CLKDEMO_TIMING_REFERENCE *select_source, *backup_source, *current_source=NULL;

    new_primary_parms.size = new_secondary_parms.size = sizeof(SWI_CLOCK_ARGS);

    for(i=0; i<num_of_boards; i++)
        {
        /* skip the board that can't communicate */
        if(h110_query_parms[i].size == 0)
            h110_parms[i].size = 0;
        else
            h110_parms[i].size = sizeof(SWI_CLOCK_ARGS);
        }

    _find_master(h110_parms, num_of_boards, &primary_index, &primary_clock, &secondary_index, &secondary_clock);

    /**********************************************************************************
    * When there are multiple status changes, the priority to deal with the change
    * is important. 1. primary clock fail, 2. secondary clock fail 3. inconsistant report
    * If there is reference status change, recheck the adjustments made by above
    ************************************************************************************/
    if(((clock_status & CLKSYS_STATUS_A_FAIL) && (primary_clock == MVIP95_SOURCE_H100_A) ) ||
        ((clock_status & CLKSYS_STATUS_B_FAIL) && (primary_clock == MVIP95_SOURCE_H100_B) )  )
        {
        /* !! primary clock fail !!*/
        /* select new primary and/or secondary and set h110_parms[] accordingly */

        rc = _choose_new_primary( clock_status, secondary_clock,
                                  h110_query_parms, num_of_boards,
                                  &new_primary_index, &new_primary_parms);
        if(rc != SUCCESS)
            {
            printf(" Fail to locate new Primary Master, Keep current status !!!\n");
            return CLKSYS_ACTION_NONE;
            }

        new_primary_clock = (new_primary_parms.ext.h100.clockmode == MVIP95_H100_MASTER_A)? MVIP95_SOURCE_H100_A: MVIP95_SOURCE_H100_B;

        rc = _choose_new_secondary( new_primary_index, new_primary_clock,
                                    h110_query_parms, num_of_boards,
                                    &new_secondary_index, &new_secondary_parms);
        if(rc != SUCCESS)
            {
            _debug(" Can't locate new Secondary Master, Run without it !!!\n");
            }


        for(i=0; i<num_of_boards; i++)
            {
            /* if the board can't communicate, set it to slave. This is because if the
            board was master, a new master will be selected. Later if we
            try to find who is the master of the system, the old board should not confuse
            the _find_master() module */
            if(h110_query_parms[i].size == 0)
                {
                h110_parms[i].clocktype                    =  MVIP95_H100_CLOCKING;
                h110_parms[i].clocksource                  =  new_primary_clock;
                h110_parms[i].network                      =  0;
                h110_parms[i].ext.h100.clockmode           =  MVIP95_H100_SLAVE;
                h110_parms[i].ext.h100.autofallback        =  MVIP95_H100_ENABLE_AUTO_FB;
                h110_parms[i].ext.h100.netrefclockspeed    =  MVIP95_H100_NETREF_8KHZ;
                h110_parms[i].ext.h100.fallbackclocksource =  (new_primary_clock == MVIP95_SOURCE_H100_B)? MVIP95_H100_MASTER_A: MVIP95_H100_MASTER_B;
                h110_parms[i].ext.h100.fallbacknetwork     =  0;
                continue;
                }

            if(i == new_primary_index)
                {
                h110_parms[i]                    =  new_primary_parms;
                }
            /* if successfully choose a new secondary */
            else if(rc == SUCCESS && i == new_secondary_index)
                {
                h110_parms[i]                = new_secondary_parms;
                }
            /* the rest should be slave */
            else
                {
                h110_parms[i].clocktype                    =  MVIP95_H100_CLOCKING;
                h110_parms[i].clocksource                  =  new_primary_clock;
                h110_parms[i].network                      =  0;
                h110_parms[i].ext.h100.clockmode           =  MVIP95_H100_SLAVE;
                h110_parms[i].ext.h100.autofallback        =  MVIP95_H100_ENABLE_AUTO_FB;
                h110_parms[i].ext.h100.netrefclockspeed    =  MVIP95_H100_NETREF_8KHZ;
                h110_parms[i].ext.h100.fallbackclocksource =  (new_primary_clock == MVIP95_SOURCE_H100_B)? MVIP95_SOURCE_H100_A: MVIP95_SOURCE_H100_B;
                h110_parms[i].ext.h100.fallbacknetwork     =  0;
                }

            }
        ret = CLKSYS_ACTION_NEW_PRIMARY;
        }
    else if(((clock_status & CLKSYS_STATUS_A_FAIL) && (secondary_clock == MVIP95_SOURCE_H100_A) ) ||
        ((clock_status & CLKSYS_STATUS_B_FAIL) && (secondary_clock == MVIP95_SOURCE_H100_B) )    )
        {
        /* !!secondary_clock fail!! */
        /* select new secondary and set h110_parms[] accordingly */
        rc = _choose_new_secondary( primary_index,    primary_clock,
                                    h110_query_parms, num_of_boards,
                                    &new_secondary_index, &new_secondary_parms);
        if(rc != SUCCESS)
            {
            _debug(" Can't locate new Secondary Master, Run without it !!!\n");
            }
        else
            {
            /* set old secondary to slave */
            h110_parms[secondary_index].clocktype                  =  MVIP95_H100_CLOCKING;
            h110_parms[secondary_index].clocksource                =  primary_clock;
            h110_parms[secondary_index].network                    =  0;
            h110_parms[secondary_index].ext.h100.clockmode         =  MVIP95_H100_SLAVE;
            h110_parms[secondary_index].ext.h100.autofallback      =  MVIP95_H100_ENABLE_AUTO_FB;
            h110_parms[secondary_index].ext.h100.netrefclockspeed  =  MVIP95_H100_NETREF_8KHZ;
            h110_parms[secondary_index].ext.h100.fallbackclocksource   =  (primary_clock == MVIP95_SOURCE_H100_B)? MVIP95_SOURCE_H100_A: MVIP95_SOURCE_H100_B;
            h110_parms[secondary_index].ext.h100.fallbacknetwork       =  0;

            /* set new secondary */
            h110_parms[new_secondary_index] = new_secondary_parms;
            }
        ret =  CLKSYS_ACTION_NEW_SECONDARY;
        }
    else if(clock_status & CLKSYS_STATUS_INCONSISTENT)
        {
        for(i=0; i<num_of_boards; i++)
            {
            /* if the board can't communicate, set it to slave. This is because if the
            board was master, a new master will be selected. Later if we
            try to find who is the master of the system, the old board should not confuse
            the _find_master() module */
            if(h110_query_parms[i].size == 0)
                {
                h110_parms[i].clocktype                    =  MVIP95_H100_CLOCKING;
                h110_parms[i].clocksource                  =  primary_clock;
                h110_parms[i].network                      =  0;
                h110_parms[i].ext.h100.clockmode           =  MVIP95_H100_SLAVE;
                h110_parms[i].ext.h100.autofallback        =  MVIP95_H100_ENABLE_AUTO_FB;
                h110_parms[i].ext.h100.netrefclockspeed    =  MVIP95_H100_NETREF_8KHZ;
                h110_parms[i].ext.h100.fallbackclocksource =  secondary_clock;
                h110_parms[i].ext.h100.fallbacknetwork     =  0;
                continue;
                }

            /* reset the board which has fallback */
            if(h110_query_parms[i].ext.h100.fallbackoccurred == MVIP95_H100_FALLBACK_OCCURRED)
                {
                if(i== primary_index)
                    {
                    /* use the current source as primary source and select
                    a new backup source. */
                    _index2id(i, &id);
                    id |= (h110_query_parms[i].network << 16);

                    /* find out which trunk backup the first source */
                    select_source = ClkRsc_get_reference(CLKRULE_SECOND_IN_PRIORITY_SAME_BOARD, id);
                    if(select_source == NULL)
                        {
                        primary_backup = 0;
                        }
                    else
                        {
                        primary_backup = select_source->Trunk_id;
                        }

                    h110_parms[i].clocktype                    =  MVIP95_H100_CLOCKING;
                    h110_parms[i].clocksource                  =  h110_query_parms[i].clocksource;
                    h110_parms[i].network                      =  h110_query_parms[i].network;
                    h110_parms[i].ext.h100.clockmode           =  (primary_clock == MVIP95_SOURCE_H100_B)? MVIP95_H100_MASTER_B: MVIP95_H100_MASTER_A;
                    h110_parms[i].ext.h100.autofallback        =  MVIP95_H100_ENABLE_AUTO_FB;
                    h110_parms[i].ext.h100.netrefclockspeed    =  MVIP95_H100_NETREF_8KHZ;
                    h110_parms[i].ext.h100.fallbackclocksource =  (primary_backup)?MVIP95_SOURCE_NETWORK :MVIP95_SOURCE_INTERNAL;
                    h110_parms[i].ext.h100.fallbacknetwork     =  primary_backup;
                    }
                else if(i== secondary_index)
                    {
                    /* In this case, there may be a false fallback. Just restore it */
                    h110_parms[i].clocktype                    =  MVIP95_H100_CLOCKING;
                    h110_parms[i].clocksource                  =  primary_clock;
                    h110_parms[i].network                      =  0;
                    h110_parms[i].ext.h100.clockmode           =  (primary_clock == MVIP95_SOURCE_H100_B)? MVIP95_H100_MASTER_A: MVIP95_H100_MASTER_B;
                    h110_parms[i].ext.h100.autofallback        =  MVIP95_H100_ENABLE_AUTO_FB;
                    h110_parms[i].ext.h100.netrefclockspeed    =  MVIP95_H100_NETREF_8KHZ;
                    h110_parms[i].ext.h100.fallbackclocksource =  h110_query_parms[i].clocksource;
                    h110_parms[i].ext.h100.fallbacknetwork     =  h110_query_parms[i].network;
                    }
                else
                    {
                    /* In ths case, there may be a false fallback. Just restore it */
                    h110_parms[i].clocktype                    =  MVIP95_H100_CLOCKING;
                    h110_parms[i].clocksource                  =  primary_clock;
                    h110_parms[i].network                      =  0;
                    h110_parms[i].ext.h100.clockmode           =  MVIP95_H100_SLAVE;
                    h110_parms[i].ext.h100.autofallback        =  MVIP95_H100_ENABLE_AUTO_FB;
                    h110_parms[i].ext.h100.netrefclockspeed    =  MVIP95_H100_NETREF_8KHZ;
                    h110_parms[i].ext.h100.fallbackclocksource =  secondary_clock;
                    h110_parms[i].ext.h100.fallbacknetwork     =  0;
                    }
                }
            }
        ret =  CLKSYS_ACTION_RELOAD;
        }
    else
        ret =  CLKSYS_ACTION_NONE;

    if(clock_status & CLKSYS_STATUS_REFERENCE_CHANGED)
        {
        /* H100 clocks are OK, no board report fallback, but there is at least one new comer. */
        /* Need to check if the current configuration use the highest priority reference */

        /* find out which trunk got the first priority */
        select_source = ClkRsc_get_reference(CLKRULE_FIRST_IN_PRIORITY, 0);
        if(select_source == NULL)
            {
            /* no reference is available -> no-op here */
            return CLKSYS_ACTION_NONE;
            }
        rc = _index2id(primary_index, &id);
        rc = ClkRsc_get_info(id, h110_parms[primary_index].network, &current_source);

        if(rc != 0 || current_source->Priority > select_source->Priority ||
            current_source->Status == MVIP95_TIMING_REF_STATUS_BAD )
            {
            /* primary is NOT using the highest priority trunk  */
            /* select new primary and secondary */
            /* set old primary to slave */
            h110_parms[primary_index].clocktype                    =  MVIP95_H100_CLOCKING;
            h110_parms[primary_index].clocksource                  =  primary_clock;
            h110_parms[primary_index].network                      =  0;
            h110_parms[primary_index].ext.h100.clockmode           =  MVIP95_H100_SLAVE;
            h110_parms[primary_index].ext.h100.autofallback        =  MVIP95_H100_ENABLE_AUTO_FB;
            h110_parms[primary_index].ext.h100.netrefclockspeed    =  MVIP95_H100_NETREF_8KHZ;
            h110_parms[primary_index].ext.h100.fallbackclocksource =  (primary_clock == MVIP95_SOURCE_H100_B)? MVIP95_SOURCE_H100_A: MVIP95_SOURCE_H100_B;
            h110_parms[primary_index].ext.h100.fallbacknetwork     =  0;

            rc = _id2index(select_source->Board_id, &primary_index);
            /* set new primary */
            h110_parms[primary_index].clocktype                    =  MVIP95_H100_CLOCKING;
            h110_parms[primary_index].clocksource                  =  (select_source->Trunk_id)?MVIP95_SOURCE_NETWORK :MVIP95_SOURCE_INTERNAL;
            h110_parms[primary_index].network                      =  select_source->Trunk_id;
            h110_parms[primary_index].ext.h100.clockmode           =  (primary_clock == MVIP95_SOURCE_H100_B)? MVIP95_H100_MASTER_B: MVIP95_H100_MASTER_A;;
            h110_parms[primary_index].ext.h100.autofallback        =  MVIP95_H100_ENABLE_AUTO_FB;
            h110_parms[primary_index].ext.h100.netrefclockspeed    =  MVIP95_H100_NETREF_8KHZ;

            /* find new backup source */
            backup_source = ClkRsc_get_reference(CLKRULE_SECOND_IN_PRIORITY_SAME_BOARD,
                ( select_source->Board_id | (select_source->Trunk_id << 16))   );
            if(backup_source != NULL)
                {
                h110_parms[primary_index].ext.h100.fallbackclocksource   =  (backup_source->Trunk_id)?MVIP95_SOURCE_NETWORK :MVIP95_SOURCE_INTERNAL;
                h110_parms[primary_index].ext.h100.fallbacknetwork       =  backup_source->Trunk_id;
                }
            else
                {      /* Use OSC */
                h110_parms[primary_index].ext.h100.fallbackclocksource   =  MVIP95_SOURCE_INTERNAL;
                h110_parms[primary_index].ext.h100.fallbacknetwork       =  0;
                }


            /* find new secondary */
            select_source = ClkRsc_get_reference(CLKRULE_FIRST_IN_PRIORITY_DIFF_BOARD,select_source->Board_id);
            if(select_source != NULL)
                {
                if(secondary_index != primary_index)
                    {
                    /* set old secondary to slave, only if it is not the
                    new primary */
                    h110_parms[secondary_index].clocktype                 =  MVIP95_H100_CLOCKING;
                    h110_parms[secondary_index].clocksource               =  primary_clock;
                    h110_parms[secondary_index].network                   =  0;
                    h110_parms[secondary_index].ext.h100.clockmode            =  MVIP95_H100_SLAVE;
                    h110_parms[secondary_index].ext.h100.autofallback     =  MVIP95_H100_ENABLE_AUTO_FB;
                    h110_parms[secondary_index].ext.h100.netrefclockspeed =  MVIP95_H100_NETREF_8KHZ;
                    h110_parms[secondary_index].ext.h100.fallbackclocksource  =  (primary_clock == MVIP95_SOURCE_H100_B)? MVIP95_SOURCE_H100_A: MVIP95_SOURCE_H100_B;
                    h110_parms[secondary_index].ext.h100.fallbacknetwork      =  0;
                    }
                rc = _id2index(select_source->Board_id, &secondary_index);
                /* set new secondary */
                h110_parms[secondary_index].clocktype                  =  MVIP95_H100_CLOCKING;
                h110_parms[secondary_index].clocksource                =  primary_clock;
                h110_parms[secondary_index].network                    =  0;
                h110_parms[secondary_index].ext.h100.clockmode         =  (primary_clock == MVIP95_SOURCE_H100_B)? MVIP95_H100_MASTER_A: MVIP95_H100_MASTER_B;
                h110_parms[secondary_index].ext.h100.autofallback      =  MVIP95_H100_ENABLE_AUTO_FB;
                h110_parms[secondary_index].ext.h100.netrefclockspeed  =  MVIP95_H100_NETREF_8KHZ;
                h110_parms[secondary_index].ext.h100.fallbackclocksource   =  (select_source->Trunk_id)?MVIP95_SOURCE_NETWORK :MVIP95_SOURCE_INTERNAL;
                h110_parms[secondary_index].ext.h100.fallbacknetwork       =  select_source->Trunk_id;
                }
            return CLKSYS_ACTION_NEW_PRIMARY;
            }

        /* if both primary and secondary backup reference change their status,
           update the secondary backup first so that in worse case when primary
           clock is down we still have a valid secondary clock */

        /* find out if the secondary backup using available high priority reference */
        /*  note: secondary is always using primary clock as first source */

        select_source = ClkRsc_get_reference(CLKRULE_FIRST_IN_PRIORITY_DIFF_BOARD,id);
        if(select_source != NULL)
            {  /* possible new backup reference, check it out */
            _index2id(secondary_index, &id2);
            rc = ClkRsc_get_info(id2, h110_parms[secondary_index].ext.h100.fallbacknetwork, &current_source);

            if(rc != 0 || current_source->Priority > select_source->Priority    ||
                current_source->Status == MVIP95_TIMING_REF_STATUS_BAD )
                {
                /* secondary backup is NOT using the highest available trunk  */

                /* set old secondary to slave */
                h110_parms[secondary_index].clocktype                  =  MVIP95_H100_CLOCKING;
                h110_parms[secondary_index].clocksource                =  primary_clock;
                h110_parms[secondary_index].network                    =  0;
                h110_parms[secondary_index].ext.h100.clockmode         =  MVIP95_H100_SLAVE;
                h110_parms[secondary_index].ext.h100.autofallback      =  MVIP95_H100_ENABLE_AUTO_FB;
                h110_parms[secondary_index].ext.h100.netrefclockspeed  =  MVIP95_H100_NETREF_8KHZ;
                h110_parms[secondary_index].ext.h100.fallbackclocksource   =  (primary_clock == MVIP95_SOURCE_H100_B)? MVIP95_SOURCE_H100_A: MVIP95_SOURCE_H100_B;
                h110_parms[secondary_index].ext.h100.fallbacknetwork       =  0;

                rc = _id2index(select_source->Board_id, &secondary_index);
                /* set new secondary */
                h110_parms[secondary_index].clocktype                  =  MVIP95_H100_CLOCKING;
                h110_parms[secondary_index].clocksource                =  primary_clock;
                h110_parms[secondary_index].network                    =  0;
                h110_parms[secondary_index].ext.h100.clockmode         =  (primary_clock == MVIP95_SOURCE_H100_B)? MVIP95_H100_MASTER_A: MVIP95_H100_MASTER_B;
                h110_parms[secondary_index].ext.h100.autofallback      =  MVIP95_H100_ENABLE_AUTO_FB;
                h110_parms[secondary_index].ext.h100.netrefclockspeed  =  MVIP95_H100_NETREF_8KHZ;
                h110_parms[secondary_index].ext.h100.fallbackclocksource   =  (select_source->Trunk_id)?MVIP95_SOURCE_NETWORK :MVIP95_SOURCE_INTERNAL;
                h110_parms[secondary_index].ext.h100.fallbacknetwork       =  select_source->Trunk_id;
                return CLKSYS_ACTION_NEW_SECONDARY;
                }

            }

        /* find out if the primary backup using available high priority reference */
        select_source = ClkRsc_get_reference(CLKRULE_SECOND_IN_PRIORITY_SAME_BOARD,
                                             ( id | (h110_query_parms[primary_index].network << 16))   );
        if(select_source != NULL)
            {  /* possible new backup reference, check it out */

            rc = ClkRsc_get_info(id, h110_parms[primary_index].ext.h100.fallbacknetwork, &current_source);

            if(rc != 0 || current_source->Priority > select_source->Priority  ||
                current_source->Status == MVIP95_TIMING_REF_STATUS_BAD)
                {
                /* primary backup is NOT using the highest available trunk  */
                /* change the backup source */
                h110_parms[primary_index].ext.h100.fallbackclocksource =  (select_source->Trunk_id)?MVIP95_SOURCE_NETWORK :MVIP95_SOURCE_INTERNAL;
                h110_parms[primary_index].ext.h100.fallbacknetwork     =  select_source->Trunk_id;

                return CLKSYS_ACTION_RELOAD_PRIMARY_FALLBACK;
                }

            }

        } /* end of reference change */

    return ret;

    }



/*****************************************************************************
  _clock_reconfigure
    set the configuration with h110_parms that may be updated by __clock_control
*****************************************************************************/
static int _clock_reconfigure(int action,
                              SWI_CLOCK_ARGS h110_parms[],
                              SWI_QUERY_CLOCK_ARGS h110_query_parms[],
                              SWIHD swihd_array[], int num_of_boards)
    {
    int i, rc, id;
    int primary_clock, secondary_clock;
    int primary_board, secondary_board;
    SWI_CLOCK_ARGS standalone_parms, slave_parms;

    char debug_str[80];

    standalone_parms.size                            =  sizeof(SWI_CLOCK_ARGS);
    standalone_parms.clocktype                       =  MVIP95_H100_CLOCKING;
    standalone_parms.clocksource                     =  MVIP95_SOURCE_INTERNAL;
    standalone_parms.network                         =  0;
    standalone_parms.ext.h100.clockmode              =  MVIP95_H100_STAND_ALONE;
    standalone_parms.ext.h100.autofallback           =  MVIP95_H100_ENABLE_AUTO_FB;
    standalone_parms.ext.h100.netrefclockspeed       =  MVIP95_H100_NETREF_8KHZ;
    standalone_parms.ext.h100.fallbackclocksource    =  MVIP95_SOURCE_INTERNAL;
    standalone_parms.ext.h100.fallbacknetwork        =  0;

    switch(action)
        {
        case CLKSYS_ACTION_RELOAD:
            _debug(" ACTION_RELOAD\n");
            for(i=0; i<num_of_boards; i++)
                {
                /* Ideally, it should compare the current configuration and the intend one.
                If they are different, then reconfigure the board. Since we can't get a
                complete current configuration ( eg the fallback source), here we check
                fallbackoccurred and the primary timing reference for action */
                if(swihd_array[i] != 0 && h110_parms[i].size != 0)
                    {
                    if(h110_query_parms[i].ext.h100.fallbackoccurred == MVIP95_H100_FALLBACK_OCCURRED        ||
                        h110_query_parms[i].clocksource               != h110_parms[i].clocksource            ||
                        h110_query_parms[i].ext.h100.clockmode        != h110_parms[i].ext.h100.clockmode     ||
                        h110_query_parms[i].ext.h100.autofallback     != h110_parms[i].ext.h100.autofallback  )
                        {
                        rc = swiConfigBoardClock (swihd_array[i], &(h110_parms[i]));
                        if (rc != 0)
                            {
                            _index2id(i, &id);
                            printf ("\n ERROR 0x%x: Reconfigure H110 Clock (%u) \n",
                                rc, id);
                            swiConfigBoardClock (swihd_array[i], &(standalone_parms));
                            }

                        }
                    } /* fi swihd_array[i] != 0 */
                }
            break;
        case CLKSYS_ACTION_NEW_PRIMARY:
            _debug(" ACTION_NEW_PRIMARY\n");

            /* find out who is the new master */
            _find_master(h110_parms, num_of_boards, &primary_board, &primary_clock, &secondary_board, &secondary_clock);

            /* find out if the primary or secondary clock has been driven by other boards */
            for(i=0; i<num_of_boards; i++)
                {   /* skip the board that can't read the status */
                if(h110_query_parms[i].size == 0) continue;

                if((h110_query_parms[i].ext.h100.clockmode ==  MVIP95_H100_MASTER_A) &&
                    (h110_query_parms[i].clocksource        !=  MVIP95_SOURCE_H100_A)     )
                    {
                    /* if driven by other board, STOP that board from driving */
                    if(primary_clock == MVIP95_SOURCE_H100_A && i != primary_board)
                        {
                        rc = swiConfigBoardClock (swihd_array[i], &(standalone_parms));
                        if (rc != 0)
                            {
                            _index2id(i, &id);
                            printf ("\n ERROR 0x%x: Reconfigure StandAlone PA (%u) \n",
                                rc, id);
                            }
                        sprintf(debug_str,"Set board %d StandAlone -  Not drive PA\n");
                        _debug(debug_str);

                        }
                    }
                if((h110_query_parms[i].ext.h100.clockmode ==  MVIP95_H100_MASTER_B) &&
                    (h110_query_parms[i].clocksource        !=  MVIP95_SOURCE_H100_B)     )
                    {
                    /* if driven by other board, STOP that board from driving */
                    if(primary_clock == MVIP95_SOURCE_H100_B && i != primary_board)
                        {
                        rc = swiConfigBoardClock (swihd_array[i], &(standalone_parms));
                        if (rc != 0)
                            {
                            _index2id(i, &id);
                            printf ("\n ERROR 0x%x: Reconfigure StandAlone PB (%u) \n",
                                rc, id);
                            }
                        sprintf(debug_str,"Set board %d StandAlone -  Not drive PB\n");
                        _debug(debug_str);

                        }
                    }

                if((h110_query_parms[i].ext.h100.clockmode ==  MVIP95_H100_MASTER_A) &&
                    (h110_query_parms[i].clocksource        ==  MVIP95_SOURCE_H100_B)     )
                    {
                    /* if driven by other board, STOP that board from driving */
                    if(secondary_clock == MVIP95_SOURCE_H100_A && i != secondary_board)
                        {
                        rc = swiConfigBoardClock (swihd_array[i], &(standalone_parms));
                        if (rc != 0)
                            {
                            _index2id(i, &id);
                            printf ("\n ERROR 0x%x: Reconfigure StandAlone SA (%u) \n",
                                rc, id);
                            }
                        sprintf(debug_str,"Set board %d StandAlone -  Not drive SA\n");
                        _debug(debug_str);

                        }
                    }
                if((h110_query_parms[i].ext.h100.clockmode ==  MVIP95_H100_MASTER_B) &&
                    (h110_query_parms[i].clocksource        ==  MVIP95_SOURCE_H100_A)     )
                    {
                    /* if driven by other board, STOP that board from driving */
                    if(secondary_clock == MVIP95_SOURCE_H100_B && i != secondary_board)
                        {
                        rc = swiConfigBoardClock (swihd_array[i], &(standalone_parms));
                        if (rc != 0)
                            {
                            _index2id(i, &id);
                            printf ("\n ERROR 0x%x: Reconfigure StandAlone SB (%u) \n",
                                rc, id);
                            }
                        sprintf(debug_str,"Set board %d StandAlone -  Not drive SB\n");
                        _debug(debug_str);

                        }
                    }
                }


            /* reconfigure the primary master */
            rc = swiConfigBoardClock (swihd_array[primary_board], &(h110_parms[primary_board]));
            if (rc != 0)
                {
                _index2id(i, &id);
                printf ("\n ERROR 0x%x: Reconfigure Primary fail (%u) \n",
                    rc, id);
                swiConfigBoardClock (swihd_array[primary_board], &(standalone_parms));

                }
            /* reconfigure the secondary master */
            rc = swiConfigBoardClock (swihd_array[secondary_board], &(h110_parms[secondary_board]));
            if (rc != 0)
                {
                _index2id(i,&id);
                printf ("\n ERROR 0x%x: Reconfigure Secondary fail (%u) \n",
                    rc, id);
                swiConfigBoardClock (swihd_array[secondary_board], &(standalone_parms));

                }

            /* reconfigure all slaves */
            for(i=0; i<num_of_boards; i++)
                {
                if(i == primary_board || i == secondary_board) continue;
                if(swihd_array[i] != 0 && h110_parms[i].size != 0)
                    {
                    rc = swiConfigBoardClock (swihd_array[i], &(h110_parms[i]));
                    if (rc != 0)
                        {
                        _index2id(i, &id);
                        printf ("\n ERROR 0x%x: Reconfigure Slave fail (%u) \n",
                            rc, id);
                        swiConfigBoardClock (swihd_array[i], &(standalone_parms));

                        }

                    }
                }
            break;
        case CLKSYS_ACTION_NEW_SECONDARY:
            _debug(" ACTION_NEW_SECONDARY\n");
            /* set configuration for the new secondary master. Since the new secondary
            may not be the same board set all slaves too */
            standalone_parms.size                          =  sizeof(SWI_CLOCK_ARGS);
            standalone_parms.clocktype                     =  MVIP95_H100_CLOCKING;
            standalone_parms.clocksource                   =  MVIP95_SOURCE_INTERNAL;
            standalone_parms.network                       =  0;
            standalone_parms.ext.h100.clockmode                =  MVIP95_H100_STAND_ALONE;
            standalone_parms.ext.h100.autofallback         =  MVIP95_H100_ENABLE_AUTO_FB;
            standalone_parms.ext.h100.netrefclockspeed     =  MVIP95_H100_NETREF_8KHZ;
            standalone_parms.ext.h100.fallbackclocksource  =  MVIP95_SOURCE_INTERNAL;
            standalone_parms.ext.h100.fallbacknetwork      =  0;


            /* find out who is the new master */
            _find_master(h110_parms, num_of_boards, &primary_board, &primary_clock, &secondary_board, &secondary_clock);

            slave_parms.size                           =  sizeof(SWI_CLOCK_ARGS);
            slave_parms.clocktype                      =  MVIP95_H100_CLOCKING;
            slave_parms.clocksource                    =  primary_clock;
            slave_parms.network                        =  0;
            slave_parms.ext.h100.clockmode             =  MVIP95_H100_SLAVE;
            slave_parms.ext.h100.autofallback          =  MVIP95_H100_ENABLE_AUTO_FB;
            slave_parms.ext.h100.netrefclockspeed      =  MVIP95_H100_NETREF_8KHZ;
            slave_parms.ext.h100.fallbackclocksource   =  secondary_clock;
            slave_parms.ext.h100.fallbacknetwork       =  0;

            /* find out if secondary clock has been driven by other boards */
            for(i=0; i<num_of_boards; i++)
                {   /* skip the board that can't read the status */
                if(h110_query_parms[i].size == 0) continue;

                if((h110_query_parms[i].ext.h100.clockmode ==  MVIP95_H100_MASTER_A) &&
                   (h110_query_parms[i].clocksource        ==  MVIP95_SOURCE_H100_B)     )
                    {
                    /* if driven by other board, STOP that board from driving */
                    if(secondary_clock == MVIP95_SOURCE_H100_A && i != secondary_board)
                        {
                        rc = swiConfigBoardClock (swihd_array[i], &(standalone_parms));
                        if (rc != 0)
                            {
                            _index2id(i, &id);
                            printf ("\n ERROR 0x%x: Reconfigure StandAlone SA (%u) \n", rc, id);
                           }
                        sprintf(debug_str,"Set board %d StandAlone -  Not drive SA\n");
                        _debug(debug_str);
                        break;
                        }
                    }
                if((h110_query_parms[i].ext.h100.clockmode ==  MVIP95_H100_MASTER_B) &&
                   (h110_query_parms[i].clocksource        ==  MVIP95_SOURCE_H100_A)     )
                    {
                    /* if driven by other board, STOP that board from driving */
                    if(secondary_clock == MVIP95_SOURCE_H100_B && i != secondary_board)
                        {
                        rc = swiConfigBoardClock (swihd_array[i], &(standalone_parms));
                        if (rc != 0)
                            {
                            _index2id(i, &id);
                            printf ("\n ERROR 0x%x: Reconfigure StandAlone SB (%u) \n", rc, id);
                            }
                        sprintf(debug_str,"Set board %d StandAlone -  Not drive SB\n");
                        _debug(debug_str);

                        }
                    }
                }


            /* reconfigure the secondary master */
            rc = swiConfigBoardClock (swihd_array[secondary_board], &(h110_parms[secondary_board]));
            if (rc != 0)
                {
                _index2id(i, &id);
                printf ("\n ERROR 0x%x: Reconfigure Secondary fail (%u) \n", rc, id);
                swiConfigBoardClock (swihd_array[secondary_board], &(standalone_parms));
                }

            /* reconfigure all slaves */
            for(i=0; i<num_of_boards; i++)
                {
                if(i == primary_board || i == secondary_board) continue;
                if(swihd_array[i] != 0 && h110_parms[i].size != 0)
                    {
                    rc = swiConfigBoardClock (swihd_array[i], &(h110_parms[i]));
                    if (rc != 0)
                        {
                        _index2id(i, &id);
                        printf ("\n ERROR 0x%x: Reconfigure Slave fail (%u) \n", rc, id);
                        swiConfigBoardClock (swihd_array[i], &(standalone_parms));

                        }

                    }
                }

            break;
        case CLKSYS_ACTION_RELOAD_PRIMARY_FALLBACK:
            _debug(" ACTION_RELOAD_PRIMARY\n");
            /* in this case, only the fallback source got change,
               first find out who is the master */
            _find_master(h110_parms, num_of_boards, &primary_board, &primary_clock, &secondary_board, &secondary_clock);
            rc = swiConfigBoardClock (swihd_array[primary_board], &(h110_parms[primary_board]));
            if (rc != 0)
                {
                _index2id(primary_board, &id);
                printf ("\n ERROR 0x%x: Reload Primary fail (%u) \n", rc, id);
                swiConfigBoardClock (swihd_array[primary_board], &(standalone_parms));
                }
            break;
        case CLKSYS_ACTION_NONE:
            break;
        default:
            break;
    }

    /* verify the results before return */
    /* Poll the status of all boards */
    for (i=0; i < num_of_boards; i++)
        {
        if(swihd_array[i] == 0)
            {
            h110_query_parms[i].size = 0;
            continue;
            }
        rc = swiGetBoardClock (swihd_array[i],
                               MVIP95_H100_CLOCKING,
                               &(h110_query_parms[i]),
                               sizeof(h110_query_parms[0]));
        if (rc != SUCCESS)
            {
            /* mark the board's clock status is invalid */
            h110_query_parms[i].size = 0;
            }
        }

    return 0;
    }


/*****************************************************************************
  main
*****************************************************************************/
int main (int argc, char *argv[])
    {
    CTA_SERVICE_NAME servicelist[] =     /* for ctaInitialize */
        {
            { "SWI", "SWIMGR" },
            { "OAM", "OAMMGR" }

        };
    CTA_SERVICE_DESC serviceOpen [] =     /* for ctaOpenServices */
        {
            { {"SWI", "SWIMGR"}, { 0 }, { 0 }, { 0 } },
            { {"OAM", "OAMMGR"}, { 0 }, { 0 }, { 0 } }
        };
    int                   i, j, c;
    DWORD                 rc;
    int                   action, clock_status;
    int                   monitor_only = 1;
    int                   host_cfbm_on = 1;
    int                   num_of_boards;
    DWORD                 nBoardNumber;
    SWIHD                 swihd_array[MAX_NO_OF_BOARDS];
    CTAQUEUEHD            ctaqueuehd;
    CTAHD                 ctahd;
    CTA_EVENT             cta_event;
    int                   bCtaOwnsBuf;
    OAM_MSG              *pOamMsg;
    SWI_CLOCK_ARGS        h110_parms [MAX_NO_OF_BOARDS], standalone_parms;
    SWI_QUERY_CLOCK_ARGS  h110_query_parms [MAX_NO_OF_BOARDS] ;
    char                  sz_config_file[80];
    char                  debug_str[80];
    CLKDEMO_TIMING_REFERENCE *select_source;
    int primary_board, primary_source, primary_backup, secondary_board, secondary_source;
    int primary_index, secondary_index;
    int timeout = 2000;


    /*----------------------------------------------------------*/

    /* get user options */
    num_of_boards = 0;
    if (argc > 1)
        {
        for (i=1, num_of_boards=0; i<argc; i++)
            {
            if (*argv[i] == '-' && *(argv[i]+1) == '@')
                {
                if (*(argv[i]+2) == 0) /* space in the cmd line */
                    {
                    if (++i < argc)
                        strcpy(serverAddr, argv[i]);
                    else
                        {
                        printf ("ERROR: -@ must follow with a server address\n");
                        _usage (NULL, argv[0]);
                        return -2;
                        }

                    }
                else
                    strcpy(serverAddr, argv[i]+2);
                }
            else if (*argv[i] == '-' && *(argv[i]+1) == 'b')
                {
                if (num_of_boards >= MAX_NO_OF_BOARDS)
                    {
                    printf ("ERROR: Maximum number of the board is %d\n",
                        MAX_NO_OF_BOARDS);
                    _usage (NULL, argv[0]);
                    return -2;
                    }

                if (*(argv[i]+2) == 0) /* space in the cmd line */
                    {
                    if (++i < argc)
                        board_array[num_of_boards] = atoi(argv[i]);
                    else
                        continue;
                    }
                else
                    board_array[num_of_boards] = atoi((argv[i]+2));
                num_of_boards ++;
                }
            else if (*argv[i] == '-' && *(argv[i]+1) == 'f')
                {
                if (*(argv[i]+2) == 0) /* space in the cmd line */
                    {
                    if (++i < argc)
                        strcpy(sz_config_file, argv[i]);
                    else
                        {
                        printf ("ERROR: -f must follow with a file name\n");
                        _usage (NULL, argv[0]);
                        return -2;

                        }

                    }
                else
                    strcpy(sz_config_file, argv[i]+2);

                /* user provides initial configuration data */
                monitor_only = 0;
                }
            else if (*argv[i] == '-' && *(argv[i]+1) == 'd')
                {
                /* turn on debug message */
                edebug = 1;
                }
            else if (*argv[i] == '-' && *(argv[i]+1) == 'r')
                {
                /* user has turn off host 'cfbm' */
                host_cfbm_on = 0;
                }
            else if (*argv[i] == '-' && *(argv[i]+1) == 't')
                {
                /* change the timeout value */
                if (*(argv[i]+2) == 0) /* space in the cmd line */
                    {
                    if (++i < argc)
                        timeout = atoi(argv[i]);
                    else
                        continue;
                    }
                else
                    timeout = atoi((argv[i]+2));

                }
            else if (*argv[i] == '-' && *(argv[i]+1) == 'h')
                {
                /* print out the functional descriptions */
                _help();
                _usage (NULL, argv[0]);
                goto shutdown ;
                }
            else
                {
                _usage ("Invalid Usage", argv[0]);
                return -2;
                }
            }
        }


        /* error checking for user input */
        for (i=0; i < num_of_boards ; i++)
            {
            for (j=i+1; j < num_of_boards; j++)
                {
                if (board_array [i] == board_array [j])
                    {
                    printf ("ERROR: Board %d used more than once\n",
                        board_array [i]);
                    _usage (NULL, argv[0]);
                    return -2;
                    }
                }
            }


        /* initialize Natural Access services for this process */
        _cta_init(&ctaqueuehd, &ctahd);

        /*---------- read user assigned clock configuration ---------------*/
        if(monitor_only == 0)
            {
            if (_initData_file ( board_array, &num_of_boards, sz_config_file) != SUCCESS)
                {
                printf ("\nData initialization failed\n");
                exit (1);
                }
            }

        /*----- Another way of getting configurations information is from the OAM data based ---*/
        else if(host_cfbm_on == 0)
            {
            _initData_oam ( board_array, &num_of_boards, ctahd);

            }


        /****** This demo works only if there are more than 2 boards ********/
        if (num_of_boards < 2)
            {
            printf ("ERROR: you must specify at least 2 boards\n");
            _usage (NULL, argv[0]);
            goto shutdown ;

            }



        /* Open switch for each board */
        for (i=0; i < num_of_boards; i++)
            {
            rc = swiOpenSwitch(ctahd, "AGSW", board_array[i], 0, &(swihd_array[i]));
            if (rc != SUCCESS)
                {
                printf ("swiOpenSwitch on AGSW_%d failed (%d, 0x%x).\n",
                    board_array[i], rc, rc);
                swihd_array[i] = 0;
                }
            }


        /*--------------------------------------------------------------------------------------*/
        /*----- if the configuration data is coming from OAM data base,                      ---*/
        /*----- we assume that CFBM has already set the board clock according to the         ---*/
        /*----- user configuration. If the CFBM running on the host was stop, ie             ---*/
        /*----- the OAM did not set the board clock, or                                      ---*/
        /*----- if user specify configuration file then configure the board clock            ---*/

        if (monitor_only == 0 || host_cfbm_on == 0)  /* 0 -> Not TRUE */
            {

            ClkRsc_update_status(swihd_array);


            /* find out which board/trunk get first priority */
            select_source = ClkRsc_get_reference(CLKRULE_FIRST_IN_PRIORITY, 0);
            if(select_source == NULL)
                {
                printf(" Can't figure out which source has the highest priority\n");
                printf(" Use OSC on first board as Primary \n");
                primary_board = board_array[0];
                primary_source = 0;

                }
            else
                {
                primary_board = select_source->Board_id;
                primary_source = select_source->Trunk_id;
                }

            /* find out which trunk backup the first source */
            select_source = ClkRsc_get_reference(CLKRULE_SECOND_IN_PRIORITY_SAME_BOARD, ((primary_source << 16) | primary_board));
            if(select_source == NULL)
                {
                printf(" Can't figure out which source can backup the highest priority source\n");
                printf(" Use OSC as Primary fallback source \n");
                primary_backup = 0;

                }
            else
                {
                primary_backup = select_source->Trunk_id;
                }

            /* find out which board/trunk can be Secondary */
            select_source = ClkRsc_get_reference(CLKRULE_FIRST_IN_PRIORITY_DIFF_BOARD, primary_board);
            if(select_source == NULL)
                {
                for(i=0; i<num_of_boards; i++)
                    if(board_array[i] != primary_board) break;

                    printf(" Can't figure out which source can be the Secondary\n");
                    printf(" Use OSC on first board different than Primary as Scondary \n");
                    secondary_board = board_array[i];
                    secondary_source = 0;
                }
            else
                {
                secondary_board = select_source->Board_id;
                secondary_source = select_source->Trunk_id;
                }

            /* set up the configuration, we choose the H100_A as primary clock */

            for(i=0; i<num_of_boards; i++)
                {
                if(board_array[i] == primary_board)
                    {
                    h110_parms[i].size                        =  sizeof(SWI_CLOCK_ARGS);
                    h110_parms[i].clocktype                   =  MVIP95_H100_CLOCKING;
                    h110_parms[i].clocksource                 =  (primary_source)? MVIP95_SOURCE_NETWORK: MVIP95_SOURCE_INTERNAL ;
                    h110_parms[i].network                     =  primary_source;
                    h110_parms[i].ext.h100.clockmode          =  MVIP95_H100_MASTER_A;
                    h110_parms[i].ext.h100.autofallback       =  MVIP95_H100_ENABLE_AUTO_FB;
                    h110_parms[i].ext.h100.netrefclockspeed   =  MVIP95_H100_NETREF_8KHZ;
                    h110_parms[i].ext.h100.fallbackclocksource    =  (primary_backup)? MVIP95_SOURCE_NETWORK: MVIP95_SOURCE_INTERNAL ;
                    h110_parms[i].ext.h100.fallbacknetwork        =  primary_backup;
                    primary_index = i;
                    }
                else if (board_array[i] == secondary_board)
                    {
                    h110_parms[i].size                         = sizeof(SWI_CLOCK_ARGS);
                    h110_parms[i].clocktype                   =  MVIP95_H100_CLOCKING;
                    h110_parms[i].clocksource                 =  MVIP95_SOURCE_H100;
                    h110_parms[i].network                     =  0;
                    h110_parms[i].ext.h100.clockmode          =  MVIP95_H100_MASTER_B;
                    h110_parms[i].ext.h100.autofallback       =  MVIP95_H100_ENABLE_AUTO_FB;
                    h110_parms[i].ext.h100.netrefclockspeed   =  MVIP95_H100_NETREF_8KHZ;
                    h110_parms[i].ext.h100.fallbackclocksource    =  (secondary_source)? MVIP95_SOURCE_NETWORK: MVIP95_SOURCE_INTERNAL ;
                    h110_parms[i].ext.h100.fallbacknetwork        =  secondary_source;;
                    secondary_index = i;
                    }
                else
                    {
                    h110_parms[i].size                         = sizeof(SWI_CLOCK_ARGS);
                    h110_parms[i].clocktype                   =  MVIP95_H100_CLOCKING;
                    h110_parms[i].clocksource                 =  MVIP95_SOURCE_H100;
                    h110_parms[i].network                     =  0;
                    h110_parms[i].ext.h100.clockmode          =  MVIP95_H100_SLAVE;
                    h110_parms[i].ext.h100.autofallback       =  MVIP95_H100_ENABLE_AUTO_FB;
                    h110_parms[i].ext.h100.netrefclockspeed   =  MVIP95_H100_NETREF_8KHZ;
                    h110_parms[i].ext.h100.fallbackclocksource    =  MVIP95_SOURCE_H100_B;
                    h110_parms[i].ext.h100.fallbacknetwork        =  0;
                    }
                _printClockConfiguration (&(h110_parms[i]), i);

                }
            /* configure all as standalone */
            standalone_parms.size                          =  sizeof(SWI_CLOCK_ARGS);
            standalone_parms.clocktype                     =  MVIP95_H100_CLOCKING;
            standalone_parms.clocksource                   =  MVIP95_SOURCE_INTERNAL;
            standalone_parms.network                       =  0;
            standalone_parms.ext.h100.clockmode                =  MVIP95_H100_STAND_ALONE;
            standalone_parms.ext.h100.autofallback         =  MVIP95_H100_ENABLE_AUTO_FB;
            standalone_parms.ext.h100.netrefclockspeed     =  MVIP95_H100_NETREF_8KHZ;
            standalone_parms.ext.h100.fallbackclocksource  =  MVIP95_SOURCE_INTERNAL;
            standalone_parms.ext.h100.fallbacknetwork      =  0;


            for (i=0; i < num_of_boards; i++)
                {
                if(swihd_array[i] == 0 )
                    continue;
                rc = swiConfigBoardClock (swihd_array[i], &(standalone_parms));
                if (rc != 0)
                    {
                    printf ("\n ERROR 0x%x: H110 Clock Configuration STANDALONE, Board %u \n",
                        rc, board_array[i]);
                    goto shutdown ;

                    }
                }

            /* configure the primary */
            rc = swiConfigBoardClock (swihd_array[primary_index], &(h110_parms[primary_index]));
            if (rc != 0)
                {
                printf ("\n ERROR 0x%x: H110 Clock Configuration, Board %u \n",
                    rc, board_array[primary_index]);
                goto shutdown ;

                }
            /* configure the secondary */
            rc = swiConfigBoardClock (swihd_array[secondary_index], &(h110_parms[secondary_index]));
            if (rc != 0)
                {
                printf ("\n ERROR 0x%x: H110 Clock Configuration, Board %u \n",
                    rc, board_array[secondary_index]);
                goto shutdown ;

                }
            /* configure all slaves */
            for (i=0; i < num_of_boards; i++)
                {
                if(swihd_array[i] == 0 || i==primary_index || i == secondary_index)
                    continue;
                rc = swiConfigBoardClock (swihd_array[i], &(h110_parms[i]));
                if (rc != 0)
                    {
                    printf ("\n ERROR 0x%x: H110 Clock Configuration, Board %u \n",
                        rc, board_array[i]);
                    goto shutdown ;

                    }
                }
    }
    else /* read the current status as initial configuration */
        {
        for (i=0; i < num_of_boards; i++)
            {
            if(swihd_array[i] == 0) continue;
            /* not all configuration data can be retrieved from the swiGetBoardClock */
            /* set default to UNKNOWN */
            h110_parms[i].size                           =  sizeof(SWI_CLOCK_ARGS);
            h110_parms[i].clocktype                      =  MVIP95_H100_CLOCKING;
            h110_parms[i].clocksource                    =  MVIP95_SOURCE_DISABLE;
            h110_parms[i].network                        =  0;
            h110_parms[i].ext.h100.clockmode             =  MVIP95_H100_STAND_ALONE;
            h110_parms[i].ext.h100.autofallback          =  MVIP95_H100_DISABLE_AUTO_FB;
            h110_parms[i].ext.h100.netrefclockspeed      =  MVIP95_H100_NETREF_8KHZ;
            h110_parms[i].ext.h100.fallbackclocksource   =  MVIP95_SOURCE_DISABLE;
            h110_parms[i].ext.h100.fallbacknetwork       =  0;
            rc = swiGetBoardClock (swihd_array[i],
                MVIP95_H100_CLOCKING,
                &(h110_query_parms[i]),
                sizeof(h110_query_parms[0]));
            if (rc == SUCCESS)
                { /* clock source is valid if there is no fallback */
                if(h110_query_parms[i].ext.h100.fallbackoccurred == MVIP95_H100_NO_FALLBACK_OCCURRED)
                    {
                    h110_parms[i].clocktype            = h110_query_parms[i].clocktype;
                    h110_parms[i].clocksource          = h110_query_parms[i].clocksource;
                    h110_parms[i].network              = h110_query_parms[i].network;
                    h110_parms[i].ext.h100.clockmode   = h110_query_parms[i].ext.h100.clockmode;
                    }
                }
            }

        } /* end of if-else */


    /* register for OAM events */
    rc = oamAlertRegister( ctahd );
    if (rc != SUCCESS)
        {
        printf ("oamAlertRegister failed (%d, 0x%x).\n",
            rc, rc);
        goto shutdown ;

        }

    /* print out the initial status */
    _banner();

    printf ("%s%s%s", RptHeader1, RptHeader2,RptLine);

    for (i=0; i < num_of_boards; i++)
        {
        if(swihd_array[i] == 0) continue;
        rc = swiGetBoardClock (swihd_array[i],
            MVIP95_H100_CLOCKING,
            &(h110_query_parms[i]),
            sizeof(h110_query_parms[0]));

        _printClockStatus (&(h110_query_parms[i]), board_array[i]);
        }
    printf ("%s%s", RptLine, RptFooter);


    /* get Natural Access event with timeout, only interest in HSWEVN_XXXX */
    /* if timeout, do a poll to get the latest clock status */

    while (1)
        {
        ctaWaitEvent( ctaqueuehd, &cta_event, timeout);
        // Check if buffer is owned by Natural Access and must be freed by us below.
        bCtaOwnsBuf = cta_event.buffer && (cta_event.size & CTA_INTERNAL_BUFFER);

        switch (cta_event.id)
            {
            case OAMEVN_STARTBOARD_DONE:
            case OAMEVN_STOPBOARD_DONE:
                /* if OAM did not return SUCCESS on the event, skip the message */
                if(cta_event.value != OAM_REASON_FINISHED) continue;

                /* decode oam message */
                pOamMsg = (OAM_MSG *)cta_event.buffer;

                if(cta_event.id == OAMEVN_STARTBOARD_DONE )    // a board has just been hotswapped in
                    {
                    oamBoardGetNumber( ctahd, ((char*)pOamMsg + pOamMsg->dwOfsSzName), &nBoardNumber);
                    sprintf(debug_str,"received BOARD_STARTED message (%d)\n", (int)nBoardNumber);
                    _debug(debug_str);
                    for(i=0; i<num_of_boards; i++)
                        {
                        if(board_array[i] == (int)nBoardNumber)
                            {
                            break;
                            }
                        if(i< num_of_boards)
                            {
                            rc = swiOpenSwitch(ctahd, "AGSW", board_array[i], 0, &(swihd_array[i]));
                            if (rc != SUCCESS)
                                {
                                printf ("swiOpenSwitch on AGSW_%d failed (%d, 0x%x).\n",
                                    board_array[i], rc, rc);
                                swihd_array[i] = 0;
                                }

                            }
                        }
                    }
                else
                    {               /* a board has been hotswapped out */
                    oamBoardGetNumber( ctahd, ((char*)pOamMsg + pOamMsg->dwOfsSzName), &nBoardNumber);
                    sprintf(debug_str,"received BOARD_STOPPED message (%d)\n", (int)nBoardNumber);
                    _debug(debug_str);
                    for(i=0; i<num_of_boards; i++)
                        {
                        if(board_array[i] == (int)nBoardNumber)
                            {
                            break;
                            }
                        if(i< num_of_boards)
                            {
                            swiCloseSwitch(swihd_array[i]);
                            swihd_array[i] = 0;
                            }
                        }
                    }

                break;
            case CTAEVN_WAIT_TIMEOUT:  /* no event, so just poll the clock status */
                break;
            default:
                if (bCtaOwnsBuf)
                    {
                    ctaFreeBuffer( cta_event.buffer );      // our reponsibility to free
                    }
                goto keyscan;
            }
        if (bCtaOwnsBuf)
            {
            ctaFreeBuffer( cta_event.buffer );      // our reponsibility to free
            }

        /* based only on the valid data, determine if any clock has failed */

        /* update h100 clock status */
        clock_status = _clock_monitor(h110_query_parms, num_of_boards, swihd_array);


        action = _clock_control(clock_status, h110_parms, h110_query_parms, num_of_boards);
        if(action != CLKSYS_ACTION_NONE)
            {
            /* take actions here */

            _clock_reconfigure(action, h110_parms, h110_query_parms, swihd_array, num_of_boards);

            _banner();

            /* update the screen */
            printf ("%s%s%s", RptHeader1, RptHeader2, RptLine);

            for (i=0; i < num_of_boards; i++)
                {
                if(swihd_array[i] == 0) continue;
                _printClockStatus (&(h110_query_parms[i]), board_array[i]);
                }
            printf ("%s%s", RptLine, RptFooter);

            for (i=0; i < num_of_boards; i++)
                {
                _printClockConfiguration (&(h110_parms[i]), i);
                }

            }

keyscan:        /* scan for 'q' to exit */
        while (kbhit())
            {
            c=getch();
            if (c == 'q' || c == 'Q')
                goto shutdown ;
            }

        }
shutdown:
    printf ("Shutting down...\n");
    select_source = ClkRsc_get_reference(CLKRULE_FIRST_IN_LIST, 0);
    while(select_source != NULL)
        {
        select_source = ClkRsc_remove_from_list(select_source->Board_id, select_source->Trunk_id);
        }
    /* DemoShutdown is a module in ../ctademo directory */
    _cta_Shutdown (ctahd);
    printf ("Done.\n");

    return 0;
    }


/********************************************************************************
  _cta_init

        Initialize CTAccess and create a Natural Access handle and Natural Access Queue.
********************************************************************************/
static void _cta_init(CTAQUEUEHD  *pctaqueuehd, CTAHD *pctahd)
    {
    DWORD dwErr;
    // Initialize Natural Access process.
    CTA_SERVICE_NAME servicenames[] = { { "OAM", "OAMMGR" },{ "SWI", "SWIMGR" }  };
    unsigned const nNumServices = sizeof(servicenames) / sizeof(servicenames[0]);
    CTA_INIT_PARMS initparms = {0};
    char *szServices[] = { "OAMMGR", "SWIMGR" };
    char *szDescriptor = "Clock Demo";
    CTA_EVENT cta_event;
    CTA_SERVICE_DESC servicedescs[] =     /* for ctaOpenServices */
        {
            { {"SWI", "SWIMGR"}, { 0 }, { 0 }, { 0 } },
            { {"OAM", "OAMMGR"}, { 0 }, { 0 }, { 0 } }
        };

    memset( &initparms, 0, sizeof(CTA_INIT_PARMS) );
    initparms.size = sizeof(CTA_INIT_PARMS);
    dwErr = ctaInitialize( servicenames, nNumServices, &initparms );

    if (dwErr != SUCCESS ) // no server (initially)
        {
        printf( "Can't initialize Natural Access, 0x%x.\n", dwErr );
        exit(1);
        }

    // Create an application event queue.
    dwErr = ctaCreateQueue( szServices, nNumServices, pctaqueuehd );
    if (dwErr != SUCCESS)
        {
        printf( "Can't create queue, 0x%x.\n", dwErr );
        exit(1);
        }

    // set default server, it could be user specified server or "localhost"(default)
    dwErr = ctaSetDefaultServer(serverAddr);
    if (dwErr != SUCCESS)
        {
        printf("Can't set default server, 0x%x. \n", dwErr );
        exit(1);
        }

    // Create a Natural Access context.
    dwErr = ctaCreateContext( *pctaqueuehd, 0, szDescriptor, pctahd );
    if (dwErr != SUCCESS)
        {
        printf( "Can't create context, 0x%x.\n", dwErr );
        exit(1);
        }

    // Open services on the Natural Access context.


    dwErr = ctaOpenServices( *pctahd, servicedescs, nNumServices );
    if (dwErr != SUCCESS)
        {
        printf( "Can't open services, 0x%x.\n", dwErr );
        exit( 1 );
        }

    // Loop till "open services" is done, displaying any error messages.
    for (;;)
        {
        // Retrieve an event from the event queue.
        dwErr = ctaWaitEvent( *pctaqueuehd, &cta_event, CTA_WAIT_FOREVER );
        if (dwErr != SUCCESS)
            {
            printf( "Error while waiting for services to open, 0x%x.\n", dwErr );
            exit( 1 ); // don't retry, will most likely fail forever
            }
        if (cta_event.id == CTAEVN_OPEN_SERVICES_DONE)
            break;
        printf( "Waiting for services to open, got message 0x%x.\n",
            cta_event.id );
        }
    }

/*****************************************************************************
  _cta_Shutdown

        Synchronously closes a context and its queue.
*****************************************************************************/
void static _cta_Shutdown( CTAHD ctahd )
    {
    CTA_EVENT event;
    CTAQUEUEHD ctaqueuehd;
    DWORD ctaret;

    ctaGetQueueHandle( ctahd, &ctaqueuehd );

    ctaDestroyContext( ctahd );    /* this will close services automatically */

    /* Wait for CTAEVN_DESTROY_CONTEXT_DONE */
    for (;;)
        {
        ctaret = ctaWaitEvent( ctaqueuehd, &event, CTA_WAIT_FOREVER);
        if (ctaret != SUCCESS)
            {
            printf( "\007ctaWaitEvent returned %x\n", ctaret);
            exit( 1 );
            }


        if (ctahd == event.ctahd && event.id == CTAEVN_DESTROY_CONTEXT_DONE)
            {
            break;
            }

        printf( "Waiting for Shut down received unexpected event 0x%x\n", event.id);

        }

    if(event.buffer && (event.size & CTA_INTERNAL_BUFFER))
        ctaFreeBuffer( event.buffer );

    if(event.value != CTA_REASON_FINISHED )
        {
        printf( "Destroying the Natural Access context failed %d\n", event.value );
        exit( 1 );
        }

    ctaDestroyQueue( ctaqueuehd );
    }


/*****************************************************************************
  id2index

        convert logical board id into an array index that used by some data array

          board_array[] is a global data
*****************************************************************************/
int _id2index(int board_id, int *index)
    {
    int i;

    for(i=0; i<MAX_NO_OF_BOARDS; i++)
        {
        if(board_array[i] == board_id) break;
        }

    *index = i;

    if(i== MAX_NO_OF_BOARDS)
        return -1;
    else
        return 0;
    }

/*****************************************************************************
  index2id

      convert an array index into logical board number

      board_array[] is a global data
*****************************************************************************/

int _index2id(int index, int *board_id)
    {
    if(index < MAX_NO_OF_BOARDS)
        {
        *board_id = board_array[index];
        return 0;
        }
    else
        {
        *board_id = -1;
        return -1;
        }
    }

/***************************************************************************
  kbhit (unixes)
****************************************************************************/
#if defined (unix)
int kbhit (void )
    {
    fd_set rdflg;
    struct timeval timev;
    FD_ZERO(&rdflg );
    FD_SET(STDIN_FILENO, &rdflg );
    timev.tv_sec = 0;
    timev.tv_usec = 0;
    select( 1, &rdflg, NULL, NULL, &timev );
    return(FD_ISSET( STDIN_FILENO, &rdflg));
    }
#endif
