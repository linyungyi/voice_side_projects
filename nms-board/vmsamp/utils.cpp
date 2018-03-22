/***********************************************************************
*  File - utils.cpp
*
*  Description - Utility functions needed by H324 interface
*
*  2005  NMS Communications Corporation.
*
***********************************************************************/

///////////////////
// Include Files //
///////////////////
#include "mega_global.h"

#ifdef SOLARIS
    #include <time.h>
#else
    #include "win_stub.h"
#endif


/*****************************************************************************
 *
 * Function   : getopt
 *
 *============================================================================
 *
 * Description: This routine is based on a UNIX standard for parsing options
 *              in command lines, and is provided because some system
 *              libraries do not include it. 'argc' and 'argv' are passed
 *              repeatedly to the function, which returns the next option
 *              letter in 'argv' that matches a letter in 'optstring'.
 *              'optstring' must contain the option letters the command will
 *              recognize; if a letter in 'optstring' is followed by a colon,
 *              the option is expected to have an argument, or group of
 *              arguments, which must be separated from it by white space.
 *              Option arguments cannot be optional. 'optarg' points to the
 *              start of the option-argument on return from getopt.
 *
 *              getopt places in 'optind' the 'argv' index of the next
 *              argument to be processed.  'optind' has an initial value
 *              of 1.
 *
 *              When all options have been processed (i.e., up to the first
 *              non-option argument, or the special option '--'), getopt
 *              returns -1.  All options must precede other operands on the
 *              command line.
 *
 *              Unless 'opterr' is set to 0, getopt prints an error message
 *              on stderr and returns '?' if it encounters an option letter
 *              not included in 'optstring', or no option-argument after an
 *              option that expects one.
 *
 * Parameters : argc      - Count of commandline arguments
 *              argv      - Array of commandline arguments
 *              optstring - Valid commandline options
 *
 * Return     : -1  - Failure
 *              !-1 - Success
 *
 * Notes:     : None
 *
 *****************************************************************************/

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
        || (po=strchr( (char *)optstring, c )) == NULL)
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

}  /*** getopt ***/

/*****************************************************************************
 *
 * Function   : PrintUsage
 *
 *============================================================================
 *
 * Description: Invoked to display commandline usage message when commadline
 *              parameters are entered improperly
 * 
 * Parmeters  : argv - Array of commandline options for setting board level
 *                     parameters
 *           
 * Return     : None
 *
 * Notes      : None
 *
 *****************************************************************************/
void PrintUsage( char *argv[] )
{
    printf("Usage: %s [arguments]\n", argv[0] );
	printf("\t-g<gw.cfg>  - Gateway Configuration File\n");
	printf("\t-s<srv.cfg> - Server Configuration File\n");
	printf("\t-i <TraceMask> - Tracing Bitmask\n");	
}  /*** PrintUsage ***/

#if defined (unix)
/*---------------------------------------------------------------------------
 * kbhit (unixes)
 *--------------------------------------------------------------------------*/
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

