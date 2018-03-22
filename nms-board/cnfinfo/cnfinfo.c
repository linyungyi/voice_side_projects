/*****************************************************************************
 *  File -  cnfinfo.c
 *
 *  Description - Simple utility allowing to verify that NaturalConference
 *        is installed correctly.It indicates how many resources are
 *        available, how many members can be created depending on the 
 *        modules activated.
 *
 *       You need first to configure your board configuration file in order to allow
 *       some DSPs running NaturalConference. Then load your board with
 *       agmon. Create or Modify the cnf.cfg file (should be placed in
 *       the nms/ctaccess/cfg directory) to reflect your board Configuration File. Then
 *       launch the program cnfinfo to display the configuration
 *
 * 1999-2001 NMS Communications
 *****************************************************************************
 *
 *  Functions defined here are:
 *
 * main-+-Initialize
 *      \-mygetResourceInfo-----+-mygetNbMembers
 *      
 *      *** if no arguments ***
 *         -+-
 *         ** display the resources currently available ***
 *         ** display the boards currently available *** 
 *
 *      *** if arg == -o ***
 *           ** display the boards currently available **
 *
 *      *** if arg == -a ***
 *           -+-DisplayAbbreviation
 *
 *      *** if arg == -r *** 
 *           ** display resources capabilities
 *     
 *
 *****************************************************************************
 */
#define VERSION_NUM  "2.1"
#define VERSION_DATE __DATE__

/*----------------------------- Includes -----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef WIN32
#include <conio.h>
#endif

#include "ctadef.h"
#include "cnfdef.h"
#include "ctademo.h"

/*----------------------------- Application Definitions --------------------*/
/* max number of boards */
#define MAX_NB_BOARDS 10

/* max number of resource */
#define MAX_NB_RESOURCES 512 


/* display informations */
char banner1[] =
"CNFINFO: Natural Conference Utility V%s (%s)" ;

char banner2[] =
"\n\n"
"Board  CNF Resources  CNF Ports";
char banner3[] =
"-----  -------------  ---------";

char banner4[] =
"                                                                             \r";

char banner6[] =
"-------------------------------------------------------------------------";

char banner7[] =
"ResNum  Board  ECHO_CANCELER  DTMF_CLAMPING  TONE_CLAMPING COACHING";
char banner8[] =
"------  -----  -------------  -------------  ------------- --------";

typedef struct
{
    DWORD   board;          /* board number                            */
    char    type[9];        /* board type                              */
    DWORD   boardtype;      /* board type                              */
    int mips;               /* number of Mips available on the board   */
    int num_ivr_slots;      /* number of slots running IVR             */
    unsigned num_cnf_resources;  /* number of conferencing resources           */
    unsigned num_cnf_members;    /* number of members with all capabilities */

} MYBOARDINFO;


typedef struct
{
    DWORD   board;             /* board number associate to the resource */
    unsigned resourcenum;      /* resource number */
    DWORD capabilities;        /* capabilities currently available */
    DWORD max_members[256];         /* max number with all capabilities */
    DWORD max_members_FULL;
    DWORD max_members_NO_EC;   /* max members without Echo */
    DWORD max_members_NO_DTMF; /* max members without Dtmf Clamping */
    DWORD max_members_NO_TONE;
    DWORD max_members_NO_COACHING;

    DWORD max_members_NO_EC_DTMF;
    DWORD max_members_NO_EC_TONE;
    DWORD max_members_NO_EC_COACHING;
    DWORD max_members_NO_DTMF_TONE;
    DWORD max_members_NO_DTMF_COACHING;
    DWORD max_members_NO_TONE_COACHING;

    DWORD max_members_NO_EC_DTMF_TONE;
    DWORD max_members_NO_EC_DTMF_COACHING;
    DWORD max_members_NO_EC_TONE_COACHING;
    DWORD max_members_NO_DTMF_TONE_COACHING;

    DWORD max_members_NO_EC_DTMF_TONE_COACHING;


} MYRESOURCEINFO;


/* Borrowed from demolib.h: */
#if defined(WIN32)
  #define UNREACHED_STATEMENT(statement) statement
#else
  #define UNREACHED_STATEMENT(statement)
#endif

#ifndef ARRAY_DIM
  #define ARRAY_DIM(a) (sizeof(a)/sizeof(*(a)))
#endif


/* ******************* Globals ************************ */
CTAHD           ctahd;      /* Put there to display errors */
CTAQUEUEHD      qhd;        /* Put there to display errors */
static unsigned ReportLevel = DEMO_REPORT_MIN;
unsigned            *resourcelist, nbresource;
unsigned *myresourceList ; // resources list with -r option
unsigned nresources = 0;            // resources list number

/* ******************* forward reference ************************ */
void Help(void);
void DisplayAbbreviation(void);
void GetAndPrintError(CTAHD ctahd, DWORD err, char *funcname);
void Cleanup(CTAQUEUEHD qhd, CTAHD ctahd);
void Initialize(CTAQUEUEHD *pqhd, CTAHD *pctahd);
void mygetResourceInfo(
                        CTAHD ctahd,
                        unsigned maxresources,
                        MYRESOURCEINFO *resourceinfos,
                        unsigned *numresources,
                        unsigned display_mode
                        );
BOOL mygetNbMembers(   unsigned        capabilities, 
                        CNFRESOURCEHD   cnfresourcehd,
                        unsigned        confid,
                        double *        ellapse_time,
                        MYRESOURCEINFO *resourceinfo,
                        unsigned *      nbmember,
                        unsigned        nbresource,
                        unsigned        display_mode,
                        BOOL            go_on);
void displayLine(unsigned numresource);
void displayTitle(unsigned numresource);
void displayAttribut(
                     MYRESOURCEINFO *resourceinfos, 
                     unsigned numresource, 
                     int attr
                     );
/*----------------------------------------------------------------------------
 *                               Help
 *---------------------------------------------------------------------------*/
void Help(void)
{
    printf( banner1, VERSION_NUM, VERSION_DATE );
    printf( "\nUsage: cnfinfo [options]\n\n" );
    printf( "  where options are:\n" );
    printf( "    -h          This help\n" );
    printf( "    -a          Display the abbreviations used by this program\n\n" );
    printf( "    -c          Display only the capabilities for each resource\n" );
    printf( "    -o          Open all the conferencing resources to evaluate\n");
    printf( "                the total number of ports available on each board\n" );
    printf( "    -r n        Open the conferencing resource handle in cnf.cfg\n");
    printf( "                to evaluate the total number of ports available on this\n");
    printf( "                resource. Default = %d\n\n", 0 );
    printf( "    -l number   report level from 0 to 4. Default = %d\n", ReportLevel );
   exit (1);
}

/*----------------------------------------------------------------------------
 *                               Help
 *---------------------------------------------------------------------------*/
void DisplayAbbreviation(void)
{
    printf( "\nAbbreviations used by the program\n" );
    printf( "\nNo E    is used for CNF_NO_ECHO_CANCEL (cnfCreateConference)\n\t\t or NO_ECHO_CANCEL (cnf.cfg file)");
    printf( "\nNo D  is used for CNF_NO_DTMF_CLAMPING (cnfCreateConference)\n\t\t or NO_DTMF_CLAMPING (cnf.cfg file)");
    printf( "\nNo T  is used for CNF_NO_TONE_CLAMPING (cnfCreateConference)\n\t\t or NO_TONE_CLAMPLING (cnf.cfg file)");
    printf( "\nNo Co  is used for CNF_NO_COACHING (cnfCreateConference)\n\t\t or NO_COACHING (cnf.cfg file)");
    printf( "\n\n\t****************************************");
    printf( "\n\t* CNF_RESCAP_ECHO_CANCELER      * 0x%2.2x *", CNF_RESCAP_ECHO_CANCELER); 
    printf( "\n\t* CNF_RESCAP_COACHING           * 0x%2.2x *", CNF_RESCAP_COACHING); 
    printf( "\n\t* CNF_RESCAP_DTMF_CLAMPING      * 0x%2.2x *", CNF_RESCAP_DTMF_CLAMPING);
    printf( "\n\t* CNF_RESCAP_TONE_CLAMPING      * 0x%2.2x *", CNF_RESCAP_TONE_CLAMPING);
    printf( "\n\t* CNF_RESCAP_AUTO_GAIN_CONTROL  * 0x%2.2x *", CNF_RESCAP_AUTO_GAIN_CONTROL);
    printf( "\n\t* CNF_RESCAP_TALKER_PRIVILEGE   * 0x%2.2x *", CNF_RESCAP_TALKER_PRIVILEGE);
    printf( "\n\t* CNF_RESCAP_ACTIVE_TALKER      * 0x%2.2x *", CNF_RESCAP_ACTIVE_TALKER);
    printf( "\n\t****************************************");
    printf( "\n\n" );
    exit (1);
}

/*----------------------------------------------------------------------------
 *                               GetAndPrintError
 *---------------------------------------------------------------------------*/
void GetAndPrintError( CTAHD ctahd, DWORD err, char *funcname )
{
    char  textbuf[80] = "";
    ctaGetText( ctahd, err, textbuf, sizeof( textbuf ) );
    fprintf( stderr, "%s failed: %s (0x%x)\n", funcname, textbuf, err );
    return;
}

/*----------------------------------------------------------------------------
 *                               Cleanup
 *---------------------------------------------------------------------------*/
void Cleanup( CTAQUEUEHD qhd, CTAHD ctahd )
{
    if ( ctahd )
        ctaDestroyContext( ctahd );
    if ( qhd )
        ctaDestroyQueue( qhd );
    free (myresourceList);
    free (resourcelist);
}

/*----------------------------------------------------------------------------
 *                               Initialize
 *---------------------------------------------------------------------------*/
void Initialize( CTAQUEUEHD *pqhd, CTAHD *pctahd )
{
    CTA_SERVICE_NAME service_names[] = { {"cnf", "cnfmgr"} };
    CTA_SERVICE_DESC service_descs[] = { { {"cnf", "cnfmgr"}, {0}, {0}, {0} } };

    /* From now on, the registered error handler will be called.
     * No need to check return values.
     */
    DemoSetReportLevel(ReportLevel);
    ctaSetErrorHandler( DemoErrorHandler, NULL );

    /* Initialize ADI and CNF Service Manager names */
    DemoReportComment("\n");
    DemoInitialize( service_names, ARRAY_DIM( service_names ) );

    /* Open one queue and conferencing service */
    DemoOpenPort( 0, "DEMOCONTEXT", service_descs, ARRAY_DIM( service_descs ), pqhd , pctahd );

}

/*---------------------------------------------------------------------------
 *                               main()
 *--------------------------------------------------------------------------*/
int main (int argc, char *argv[])
{
    unsigned        numboards, numresources, i, j, k;
    MYBOARDINFO     boardinfos[MAX_NB_BOARDS];
    MYRESOURCEINFO  *resourceinfos;
    unsigned mymaxresources = 0;
    unsigned        display_mode=0;     /* Mode used to display 
                                         *  resource configuration 
                                         */
    unsigned myresource = -1;
    BOOL found = FALSE;                                        
    int c;
    char buf[200] = "";
    /* 
     * Process command-line arguments.
     */
     
    while( ( c = getopt( argc, argv, "cr:ol:CR:OL:aAhH?" ) ) != -1 )
    {
        switch( toupper( c ) )
        {
            case 'C':
                display_mode=1;
                break;    
            case 'R':
            {
                if(nresources == 0)
                {
                    myresourceList = malloc( 2 * sizeof(unsigned) );
                    if( myresourceList == NULL)
                    {
                        fprintf( stderr, "\nImpossible to allocate memory for myresourceList...");
                        return (1);
                    }
                }
                else
                   if( (myresourceList = realloc( myresourceList,(( nresources +1)* (sizeof( unsigned )) ))) == NULL )
                   { 
                       fprintf( stderr, "\nImpossible to reallocate memory for myresourceList...");
                       return (1);
                   }
                sscanf( optarg, "%u", &myresourceList[nresources]);
                nresources++;
            }
                break;
            case 'A':
                DisplayAbbreviation();
                break;
            case 'O':
                display_mode=2;
                break;
            case 'L':
                sscanf( optarg, "%d", &ReportLevel );
                break;
            case 'H':
            case '?':
            default:
                Help();
                exit(0);
        }
    }
    if (optind < argc)
    {
        printf( "Invalid option.  Use -h for help.\n" );
        exit(1);
    }
    fprintf(stderr,  banner1, VERSION_NUM, VERSION_DATE );
    fprintf(stderr, "\n\nAnalysing the configuration     ");
    /* 
     * CT Access initialization
     */
    Initialize( &qhd, &ctahd );
    /* 
     * Retrieve information concerning the NaturalConference ressources
     * This routine fullfill the array of MYRESOURCEINFO structure. 
     * Numresources is used to retrieve
     * the number of resources available and initialized in the 
     * presourceinfo structure.
     *
     * if Display_mode==0 then we open the conferences 
     * using with several non used capabilities
     * and it could take a long time (12 ms for each member created...)
     * if Display_mode==1 then only retrieve information 
     * concerning the resource (used with cnfinfo -r)
     * if Display_mode==2 then only retrieve the number of members with 
     * full capabilities and we don't close
     * the created conference (used with cnfinfo -o).
     */
    
    
    /* 
     * Memory Allocation of the resourcelist & resourceinfo arrayS
     */
    if(nresources == 0)
    {    
        cnfGetResourceList(ctahd, 0, NULL, &mymaxresources);
    }
    else
    {
        mymaxresources = nresources;
        mymaxresources = nresources;
    }
    resourcelist = malloc( (mymaxresources) * sizeof(unsigned) );
    if(resourcelist == NULL) {  fprintf( stderr, "\nImpossible to allocate memory..."); return (1); }
    resourceinfos = malloc( (mymaxresources) * sizeof(MYRESOURCEINFO) );
    mygetResourceInfo(ctahd, 
                      mymaxresources,
                      resourceinfos, 
                      &numresources, 
                      display_mode);

    fprintf(stderr, "\r");
    puts ( banner4 );

    if(numresources==0)
    {
        fprintf(stderr, "\nNo valid resources found. This problem can occur for several reasons:");
        fprintf(stderr, "\n - The cnf.cfg file must be created in the nms\\ctaccess\\cfg directory");
        fprintf(stderr, "\n - The ""DSP"" or ""Board"" information are invalid in the cnf.cfg");
        fprintf(stderr, "\n - The board configuration file and the cnf.cfg are not consistent");
        fprintf(stderr, "\n - NaturalConference is not compatible with your CTAccess version\n\n");
        exit (0);
    }

    switch(display_mode)
    {
    case 0:
        /* 
         * Display the resources currently available
         */
        displayTitle(numresources);
        printf ("\n                    |");
        displayAttribut(resourceinfos, numresources,0);
        displayLine(numresources);
        printf ("\n|Features           |");
        displayAttribut(resourceinfos, numresources, 1);
        printf ("\n|Full               |");
        displayAttribut(resourceinfos, numresources,2);
        printf ("\n|Wo (E)             |", CNF_NO_ECHO_CANCEL);
        displayAttribut(resourceinfos, numresources,3);
        printf ("\n|Wo (D)             |", CNF_NO_DTMF_CLAMPING);
        displayAttribut(resourceinfos, numresources,4);
        printf ("\n|Wo (T)             |", CNF_NO_TONE_CLAMPING);
        displayAttribut(resourceinfos, numresources,5);
        printf ("\n|Wo (Co)            |", CNF_NO_COACHING);
        displayAttribut(resourceinfos, numresources,6);
        printf ("\n|Wo (E & D)         |", CNF_NO_ECHO_CANCEL | CNF_NO_DTMF_CLAMPING);
        displayAttribut(resourceinfos, numresources,7);
        printf ("\n|Wo (E & T)         |", CNF_NO_ECHO_CANCEL | CNF_NO_TONE_CLAMPING);
        displayAttribut(resourceinfos, numresources,8);
        printf ("\n|Wo (E & Co)        |", CNF_NO_ECHO_CANCEL | CNF_NO_COACHING);
        displayAttribut(resourceinfos, numresources,9);
        printf ("\n|Wo (D & T)         |", CNF_NO_DTMF_CLAMPING | CNF_NO_TONE_CLAMPING);
        displayAttribut(resourceinfos, numresources,10);
        printf ("\n|Wo (D & Co)        |", CNF_NO_DTMF_CLAMPING | CNF_NO_COACHING);
        displayAttribut(resourceinfos, numresources,11);
        printf ("\n|Wo (T & Co)        |", CNF_NO_TONE_CLAMPING | CNF_NO_COACHING);
        displayAttribut(resourceinfos, numresources,12);
        printf ("\n|Wo (E & D & T)     |", CNF_NO_ECHO_CANCEL | CNF_NO_DTMF_CLAMPING | CNF_NO_TONE_CLAMPING);
        displayAttribut(resourceinfos, numresources,13);
        printf ("\n|Wo (E & D & Co)    |", CNF_NO_ECHO_CANCEL | CNF_NO_DTMF_CLAMPING | CNF_NO_COACHING);
        displayAttribut(resourceinfos, numresources,14);
        printf ("\n|Wo (E & T & Co)    |", CNF_NO_ECHO_CANCEL | CNF_NO_TONE_CLAMPING | CNF_NO_COACHING);
        displayAttribut(resourceinfos, numresources,15);
        printf ("\n|Wo (D & T & Co)    |", CNF_NO_DTMF_CLAMPING | CNF_NO_TONE_CLAMPING | CNF_NO_COACHING);
        displayAttribut(resourceinfos, numresources,16);
        printf ("\n|Wo (E & D & T & Co)|", CNF_NO_ECHO_CANCEL | CNF_NO_DTMF_CLAMPING | CNF_NO_TONE_CLAMPING | CNF_NO_COACHING);
        displayAttribut(resourceinfos, numresources,17);

        numboards = 0;
        for( i = 0; i < numresources; i++) {    
            BOOL find = FALSE;   
            for( k = 0; k < numboards; k++)
            {
                if ((boardinfos[k].board == resourceinfos[i].board)) 
                {
                    find = TRUE;
                    boardinfos[k].num_cnf_resources ++;
                    boardinfos[k].num_cnf_members += resourceinfos[i].max_members_FULL;
                    k = numboards;
                }
            }
            if (find == FALSE) {
                boardinfos[numboards].board = resourceinfos[i].board;
                boardinfos[numboards].num_cnf_resources = 1;
                boardinfos[numboards].num_cnf_members = resourceinfos[i].max_members_FULL; ;
                numboards++;
            }        
        }
        displayLine(numresources);
        printf("\n");
        puts ( banner6 );
        fprintf(stderr, "\n(Type \"cnfinfo -a\" to have details on the abbreviations used in this table)");
        /* 
         * Display the boards currently available
         */
        
        // ***************Display invalid resources list**********************
        k = 0;
        for(i = 0;i < nbresource; i++)
        {
            for(j = 0;j < numresources; j++)
            {
                if(resourcelist[i]==resourceinfos[j].resourcenum)
                {
                    found = TRUE;
                }
            }
            if (found==FALSE)
            {
                if (k == 0)
                {
                    printf("\n\n\nThe following resources are invalid:\n-----------------------------------\n");
                    k++;
                }
                printf("%d\t",resourcelist[i]);
            }
            found = FALSE;
        }
        puts ( banner2 );
        puts ( banner3 );
        for(i = 0;i < numboards; i++)
        {
            printf ( " %3d        %3u         %3u\n", 
                            boardinfos[i].board, 
                            boardinfos[i].num_cnf_resources,
                            boardinfos[i].num_cnf_members );
        }
        puts ( banner4 );
        break;  /* End of case display_mode==0 */

    case 1:
        puts ( banner7 );
        puts ( banner8 );
        for(i=0;i<numresources; i++)
        { 
            printf ( " %4d   %3d         %3s           %3s            %3s         %3s\n", 
                resourceinfos[i].resourcenum,
                resourceinfos[i].board, 
                ( (resourceinfos[i].capabilities & CNF_RESCAP_ECHO_CANCELER) > 0 ? "yes" : "no"),
                ( (resourceinfos[i].capabilities & CNF_RESCAP_DTMF_CLAMPING) > 0 ? "yes" : "no"),
                ( (resourceinfos[i].capabilities & CNF_RESCAP_TONE_CLAMPING) > 0 ? "yes" : "no"),
                ( (resourceinfos[i].capabilities & CNF_RESCAP_COACHING) > 0 ? "yes" : "no")
                );
        }

        break;  /* End of case display_mode==1 */

    case 2:
        /* 
         * Retrieve information concerning the boards
         */
        numboards = 0;        
        for( i = 0; i < numresources; i++) {
            BOOL find = FALSE;   
            for( k = 0; k < numboards; k++)
            {
                if ((boardinfos[k].board == resourceinfos[i].board)) 
                {
                    find = TRUE;
                    boardinfos[k].board = resourceinfos[i].board;
                    boardinfos[k].num_cnf_resources ++;
                    boardinfos[k].num_cnf_members += resourceinfos[i].max_members_FULL; 
                    k = numboards;
                }
            }
            if (find == FALSE) {
                boardinfos[numboards].board = resourceinfos[i].board;
                boardinfos[numboards].num_cnf_resources = 1;
                boardinfos[numboards].num_cnf_members = resourceinfos[i].max_members_FULL;
                numboards++;
            }
        }
        /* 
         * Display the boards currently available
         */
        fprintf(stderr, "\nThe following table displays the maximum number of ports available");
        if(nresources > 0)
        {
            fprintf(stderr, "\nfor resources:");
            for (k = 0; k < nresources; k++)
                printf(" %d",myresourceList[k]);
        }
        else
            fprintf(stderr, "\non each board using full capabilities for each resource.");
        fprintf(stderr, "\nLimitations in the total number of Conference ports are due to the HMIC chip.");
        
        // ***************Display failed RESOURCES list**********************
        k = 0;
        for(i = 0;i < nbresource; i++)
        {
            for(j = 0;j < numresources; j++)
            {
                if(resourcelist[i]==resourceinfos[j].resourcenum)
                {
                    found = TRUE;
                }
            }
            if (found==FALSE)
            {
                if (k == 0)
                {
                    printf("\n\n\nThe following resources are invalid:\n-----------------------------------\n");
                    k++;
                }
                printf("%d\t",resourcelist[i]);
            }
            found = FALSE;
        }
        puts ( banner2 );
        puts ( banner3 );
        
        for(i = 0;i < numboards; i++)
        {
            printf ( " %3d        %3u         %3u\n", 
                            boardinfos[i].board, 
                            boardinfos[i].num_cnf_resources,
                            boardinfos[i].num_cnf_members );
        }
        puts ( banner4 );


        /* Used for Debug Only when having all the resources opened (Test HMIC limitation) */
        /* fprintf(stderr, "Press a key to leave the program");
           _getch();
        */
        break;  /* End of case display_mode==2 */

    default:
        break;

    }
    free(resourceinfos);

    Cleanup(qhd,ctahd); 
    return 0;
}

/*---------------------------------------------------------------------------
 *                               mygetNbMembers()
 *
 * This function returns the number of members that is supported on a
 * given resource when using non capabilities. All the members are created.
 * If display_mode=0 then we close the conference after calling this function
 * otherwise we remain it open.
 *--------------------------------------------------------------------------*/
BOOL mygetNbMembers(   unsigned        capabilities, 
                        CNFRESOURCEHD   cnfresourcehd,
                        unsigned        confid,
                        double *        ellapse_time,
                        MYRESOURCEINFO *resourceinfo,
                        unsigned *      nbmember,
                        unsigned        nbresource,
                        unsigned        display_mode,
                        BOOL go_on)
{
    DWORD           memberid;
    DWORD           ret = SUCCESS;
    BOOL            endc = TRUE;
    char buf [100];
    //*nbmember = 0; 
    /*
     * Here we need to change the resource handler.
     * We want to continue if the current resource isn't available.
     * The current error handler exits on all errors
     */
    ctaSetErrorHandler(NULL, NULL);
    if (go_on == TRUE)
    {
        sprintf(buf,"\t Capabilities: 0x%04x",capabilities);
        DemoReportComment (buf);
        while( (ret = cnfJoinConference (cnfresourcehd, confid, NULL, &memberid)) == SUCCESS)
        {
            (*nbmember) += 1;
            endc = FALSE; 
        }
        /* When using display_mode, we could reach the HMIC limitation
         * Thus we don't handle the error CNFERR_BOARD_ERROR in this case */
        if (display_mode == 2)
        {
            if( (ret != CNFERR_NOT_ENOUGH_RESOURCE_SPACE) && (ret != CTAERR_BOARD_ERROR))
            {
              GetAndPrintError( ctahd, ret, "\ncnfJoinConference" );
              Cleanup( qhd, ctahd );
              exit(-1);
            }
            ret = TRUE;
        }
        else
        {
            if(ret != CNFERR_NOT_ENOUGH_RESOURCE_SPACE) 
            {
              GetAndPrintError( ctahd, ret, "\ncnfJoinConference" );
              Cleanup( qhd, ctahd );
              exit(-1);
            }
            
        }
        //resourceinfo->max_members[capabilities & 0xFF] = *nbmember;
        sprintf(buf,"\t Max Member: %3d",*nbmember);
        DemoReportComment (buf);
    } 
    /*
     * Restore the previous error handler that exits on all errors
     */

    ctaSetErrorHandler(DemoErrorHandler, NULL);
    return endc;
}

/*---------------------------------------------------------------------------
 *                               getCnfMembers()
 *
 * This function looks for all the maximum cnf members
 *
 *--------------------------------------------------------------------------*/
void getCnfMembers(     CNFRESOURCEHD           cnfresourcehd,
                        CNF_CONFERENCE_PARMS *  cnfparms,
                        MYRESOURCEINFO*         resourceinfo,
                        double *                ellapse_time,
                        unsigned                nbresource,
                        unsigned *              nbmember,
                        unsigned                display_mode)
{
    DWORD                   confids[100];
    CNF_CONFERENCE_INFO     cnfinfo;
    int i = 0;
    int j = 0;
    char buf [200];
    BOOL endc = FALSE;
    /* 
    * Calculate the pourcentage. if display_mode=0 then we create
    * 16 conferences for each resource. otherwise we create only 1 conference
    */
    if (display_mode == 0)
        *(ellapse_time) += 100.0/(nbresource*16.0);
    else
        *(ellapse_time) += 100.0/(nbresource);
    if (ReportLevel <= DEMO_REPORT_MIN)
        fprintf(stderr,"\b\b\b\b%3d%%", (int) *ellapse_time);
    while (!endc){
        cnfCreateConference (cnfresourcehd, cnfparms, &confids[i]);
        sprintf(buf,"->cnfCreateConference:0x%08x", confids[i]);
        DemoReportComment(buf);
        cnfGetConferenceInfo (cnfresourcehd, confids[i], &cnfinfo, sizeof(cnfinfo));
        endc = mygetNbMembers(  cnfinfo.capabilities, 
                                cnfresourcehd,
                                confids[i],
                                ellapse_time,
                                resourceinfo,
                                nbmember,
                                nbresource,
                                display_mode,
                                !endc);
        sprintf(buf,"nbmber:%d",*nbmember);
        DemoReportComment(buf);
        i++;
    }
    if (display_mode != 2) {
        for (j = 0; j < i;j++) {
            sprintf(buf,"->cnfCloseConference:0x%08x", confids[j]);
            DemoReportComment(buf);
            cnfCloseConference (cnfresourcehd, confids[j]);
        };
    }
}

/*---------------------------------------------------------------------------
 *                               mygetResourceInfo()
 *
 * This function looks for all the resource numbers available and fullfill
 * the array of MYRESOURCEINFO structure
 *--------------------------------------------------------------------------*/
void mygetResourceInfo( CTAHD ctahd, 
                        unsigned maxresources,
                        MYRESOURCEINFO *resourceinfos, 
                        unsigned *numresources, 
                        unsigned display_mode)
{
    unsigned            resource;
    int  i=0, j;
    DWORD               ret = SUCCESS;
    CNFRESOURCEHD       cnfresourcehd;
    CNF_RESOURCE_INFO   resourceinfo;
    double              ellapse_time = 0.0;/* Time in pourcentage 
                                            * used when analysing 
                                            * the configuration */
    char                textbuf[80] = "";
    char buf [200];
    CTA_EVENT           evt;
    CNF_CONFERENCE_PARMS    cnfparms;
    /* 
     * Definition of the CNF_CONFERENCE_PARMS structure
     */
    cnfparms.size = sizeof(CNF_CONFERENCE_PARMS);
    cnfparms.allocated_members = 0;
    cnfparms.user_value = 0;


    /* 
     * Retrieve resources list 
     */    
    if(nresources > 0)
        nbresource = nresources;
    else
        cnfGetResourceList(ctahd, maxresources, resourcelist, &nbresource);
    /* 
     * For each resource available, we fullfill the MYRESOURCEINFO structure 
     */
    
    for ( resource = 0,*numresources = 0; resource < nbresource; resource++ )
    {
        ctaSetErrorHandler(NULL, NULL);
        if(nresources > 0)
            resourcelist[resource] = myresourceList[resource];
        ret=cnfOpenResource( ctahd, resourcelist[resource], &cnfresourcehd );
        if ( ret != SUCCESS )
        {
            ctaGetText( ctahd, ret, textbuf, sizeof( textbuf ) );
            fprintf(stderr, "\rResource #%-4d is invalid: (%s)              \nAnalysing the configuration     ", resourcelist[resource], textbuf);
            ellapse_time += 100.0/nbresource;
            continue;
        }
        /*
         * Get the CNFEVN_OPEN_RESOURCE_DONE event
         */
        else{
            DemoWaitForSpecificEvent( ctahd, CNFEVN_OPEN_RESOURCE_DONE, &evt);
            sprintf(buf,"->cnfOpenResource %u:0x%08x", resourcelist[resource], cnfresourcehd);
            DemoReportComment(buf);
            cnfGetResourceInfo( cnfresourcehd, &resourceinfo, sizeof(resourceinfo) );
            //cnfGetResourceInfo( cnfresourcehd, &resourceinfo, 500 );    
            resourceinfos[*numresources].resourcenum = resourcelist[resource];
            resourceinfos[*numresources].board = resourceinfo.board;
            resourceinfos[*numresources].capabilities = resourceinfo.capabilities;
            for (j = 0; j < 0xFF; j++)
            {
                resourceinfos[*numresources].max_members[j] = 0;
            }
            resourceinfos[*numresources].max_members_FULL = 0;
            resourceinfos[*numresources].max_members_NO_EC = 0;
            resourceinfos[*numresources].max_members_NO_DTMF = 0;
            resourceinfos[*numresources].max_members_NO_TONE = 0;
            resourceinfos[*numresources].max_members_NO_COACHING = 0;
            resourceinfos[*numresources].max_members_NO_EC_DTMF = 0;
            resourceinfos[*numresources].max_members_NO_EC_TONE = 0;
            resourceinfos[*numresources].max_members_NO_EC_COACHING = 0;
            resourceinfos[*numresources].max_members_NO_DTMF_TONE = 0;
            resourceinfos[*numresources].max_members_NO_DTMF_COACHING = 0;
            resourceinfos[*numresources].max_members_NO_TONE_COACHING = 0;
            resourceinfos[*numresources].max_members_NO_EC_DTMF_TONE = 0;
            resourceinfos[*numresources].max_members_NO_EC_DTMF_COACHING = 0;
            resourceinfos[*numresources].max_members_NO_EC_TONE_COACHING = 0;
            resourceinfos[*numresources].max_members_NO_DTMF_TONE_COACHING = 0;
            resourceinfos[*numresources].max_members_NO_EC_DTMF_TONE_COACHING = 0;

            if (display_mode == 0)
            {
                cnfparms.flags = 0;
                DemoReportComment("getMember :Full");
                getCnfMembers (
                                cnfresourcehd, 
                                &cnfparms, 
                                &(resourceinfos[*numresources]), 
                                &ellapse_time, 
                                nbresource,
                                (unsigned*)&(resourceinfos[*numresources].max_members_FULL),
                                display_mode);

                cnfparms.flags = CNF_NO_ECHO_CANCEL;
                DemoReportComment("getMember :No EC");
                getCnfMembers (
                                cnfresourcehd, 
                                &cnfparms, 
                                &(resourceinfos[*numresources]), 
                                &ellapse_time, 
                                nbresource,
                                (unsigned*)&(resourceinfos[*numresources].max_members_NO_EC),
                                display_mode);

                cnfparms.flags = CNF_NO_DTMF_CLAMPING;
                DemoReportComment("getMember :No DTMF");
                getCnfMembers (
                                cnfresourcehd, 
                                &cnfparms, 
                                &(resourceinfos[*numresources]), 
                                &ellapse_time, 
                                nbresource,
                                (unsigned*)&(resourceinfos[*numresources].max_members_NO_DTMF),
                                display_mode);
                cnfparms.flags = CNF_NO_TONE_CLAMPING;
                DemoReportComment("getMember :No Tone");
                getCnfMembers (
                                cnfresourcehd, 
                                &cnfparms, 
                                &(resourceinfos[*numresources]), 
                                &ellapse_time, 
                                nbresource,
                                (unsigned*)&(resourceinfos[*numresources].max_members_NO_TONE),
                                display_mode);
                cnfparms.flags = CNF_NO_COACHING;
                DemoReportComment("getMember :No Coaching");
                getCnfMembers (
                                cnfresourcehd, 
                                &cnfparms, 
                                &(resourceinfos[*numresources]), 
                                &ellapse_time, 
                                nbresource,
                                (unsigned*)&(resourceinfos[*numresources].max_members_NO_COACHING),
                                display_mode);
                cnfparms.flags = CNF_NO_ECHO_CANCEL | CNF_NO_DTMF_CLAMPING;
                DemoReportComment("getMember :No EC, No DTMF");
                getCnfMembers (
                                cnfresourcehd, 
                                &cnfparms, 
                                &(resourceinfos[*numresources]), 
                                &ellapse_time, 
                                nbresource,
                                (unsigned*)&(resourceinfos[*numresources].max_members_NO_EC_DTMF),
                                display_mode);
                cnfparms.flags = CNF_NO_ECHO_CANCEL | CNF_NO_TONE_CLAMPING;
                DemoReportComment("getMember :No EC, No Tone");
                getCnfMembers (
                                cnfresourcehd, 
                                &cnfparms, 
                                &(resourceinfos[*numresources]), 
                                &ellapse_time, 
                                nbresource,
                                (unsigned*)&(resourceinfos[*numresources].max_members_NO_EC_TONE),
                                display_mode);
                cnfparms.flags = CNF_NO_ECHO_CANCEL | CNF_NO_COACHING;
                DemoReportComment("getMember :No EC, No Coaching");
                getCnfMembers (
                                cnfresourcehd, 
                                &cnfparms, 
                                &(resourceinfos[*numresources]), 
                                &ellapse_time, 
                                nbresource,
                                (unsigned*)&(resourceinfos[*numresources].max_members_NO_EC_COACHING),
                                display_mode);
                cnfparms.flags = CNF_NO_DTMF_CLAMPING | CNF_NO_TONE_CLAMPING;
                DemoReportComment("getMember :No DTMF, No Tone");
                getCnfMembers (
                                cnfresourcehd, 
                                &cnfparms, 
                                &(resourceinfos[*numresources]), 
                                &ellapse_time, 
                                nbresource,
                                (unsigned*)&(resourceinfos[*numresources].max_members_NO_DTMF_TONE),
                                display_mode);
                cnfparms.flags = CNF_NO_DTMF_CLAMPING | CNF_NO_COACHING;
                DemoReportComment("getMember :No DTMF, No Coaching");
                getCnfMembers (
                                cnfresourcehd, 
                                &cnfparms, 
                                &(resourceinfos[*numresources]), 
                                &ellapse_time, 
                                nbresource,
                                (unsigned*)&(resourceinfos[*numresources].max_members_NO_DTMF_COACHING),
                                display_mode);
                cnfparms.flags = CNF_NO_TONE_CLAMPING | CNF_NO_COACHING;
                DemoReportComment("getMember :No Tone, No Coaching");
                getCnfMembers (
                                cnfresourcehd, 
                                &cnfparms, 
                                &(resourceinfos[*numresources]), 
                                &ellapse_time, 
                                nbresource,
                                (unsigned*)&(resourceinfos[*numresources].max_members_NO_TONE_COACHING),
                                display_mode);
                cnfparms.flags = CNF_NO_ECHO_CANCEL | CNF_NO_DTMF_CLAMPING  | CNF_NO_TONE_CLAMPING;
                DemoReportComment("getMember :No EC, No DTMF, No TONE");
                getCnfMembers (
                                cnfresourcehd, 
                                &cnfparms, 
                                &(resourceinfos[*numresources]), 
                                &ellapse_time, 
                                nbresource,
                                (unsigned*)&(resourceinfos[*numresources].max_members_NO_EC_DTMF_TONE),
                                display_mode);
                cnfparms.flags = CNF_NO_ECHO_CANCEL | CNF_NO_DTMF_CLAMPING  | CNF_NO_COACHING;
                DemoReportComment("getMember :No EC, No DTMF, No Coaching");
                getCnfMembers (
                                cnfresourcehd, 
                                &cnfparms, 
                                &(resourceinfos[*numresources]), 
                                &ellapse_time, 
                                nbresource,
                                (unsigned*)&(resourceinfos[*numresources].max_members_NO_EC_DTMF_COACHING),
                                display_mode);
                cnfparms.flags = CNF_NO_ECHO_CANCEL | CNF_NO_TONE_CLAMPING  | CNF_NO_COACHING;
                DemoReportComment("getMember :No EC, No Tone, No Coaching");
                getCnfMembers (
                                cnfresourcehd, 
                                &cnfparms, 
                                &(resourceinfos[*numresources]), 
                                &ellapse_time, 
                                nbresource,
                                (unsigned*)&(resourceinfos[*numresources].max_members_NO_EC_TONE_COACHING),
                                display_mode);
                cnfparms.flags = CNF_NO_DTMF_CLAMPING | CNF_NO_TONE_CLAMPING  | CNF_NO_COACHING;
                DemoReportComment("getMember :No DTMF, No Tone, No Coaching");
                getCnfMembers (
                                cnfresourcehd, 
                                &cnfparms, 
                                &(resourceinfos[*numresources]), 
                                &ellapse_time, 
                                nbresource,
                                (unsigned*)&(resourceinfos[*numresources].max_members_NO_DTMF_TONE_COACHING),
                                display_mode);
                cnfparms.flags = CNF_NO_ECHO_CANCEL | CNF_NO_DTMF_CLAMPING | CNF_NO_TONE_CLAMPING  | CNF_NO_COACHING;
                DemoReportComment("getMember :No EC, No DTMF, No Tone, No Coaching");
                getCnfMembers (
                                cnfresourcehd, 
                                &cnfparms, 
                                &(resourceinfos[*numresources]), 
                                &ellapse_time, 
                                nbresource,
                                (unsigned*)&(resourceinfos[*numresources].max_members_NO_EC_DTMF_TONE_COACHING),
                                display_mode);

                cnfCloseResource( cnfresourcehd );
                DemoWaitForSpecificEvent( ctahd, CNFEVN_CLOSE_RESOURCE_DONE, &evt);
                sprintf(buf,"->cnfCloseResource:0x%08x",cnfresourcehd);
                DemoReportComment(buf);
            }
            else if (display_mode == 2)
            {
               /*
                * We determine the number of members with all the capabilities
                */
                cnfparms.flags = 0;
                getCnfMembers (
                                cnfresourcehd, 
                                &cnfparms, 
                                &(resourceinfos[*numresources]), 
                                &ellapse_time, 
                                nbresource,
                                (unsigned*)&(resourceinfos[*numresources].max_members_FULL),
                                display_mode
                                );
            }
            (*numresources)++;
        }
    } /* End of for ( resource=0; resource<nbresource; resource++ ) */       
    //free (resourcelist);
}

/*---------------------------------------------------------------------------
 *                               displayLine(unsigned number)
 *--------------------------------------------------------------------------*/
void displayLine(unsigned numresource)
{
    int i ;
    printf ("\n|-------------------|");
    for (i = 0; i < (int)numresource; i++)
    {
        printf("--------|");
    }
}
/*---------------------------------------------------------------------------
 *                               displayTitle(unsigned number)
 *--------------------------------------------------------------------------*/
void displayTitle(unsigned numresource)
{
    int i ;
    printf ("\n                    |");
    for (i = 0; i < (int)numresource; i++)
    {
        printf("  Rs/Bd |");
    }
}

/*---------------------------------------------------------------------------
 *                               displayAttribut
 *--------------------------------------------------------------------------*/
void displayAttribut(MYRESOURCEINFO *resourceinfos, unsigned numresource, int attr)
{
    int i;
    for (i = 0; i < (int)numresource; i++)
    {
        if (attr == 0) printf (" %3d/%-2d |",resourceinfos[i].resourcenum, resourceinfos[i].board);
        if (attr == 1) printf (" 0x%04x |",resourceinfos[i].capabilities);
        if (attr == 2) printf ("   %2d   |",resourceinfos[i].max_members_FULL);

        if (attr == 3) printf ("   %2d   |",resourceinfos[i].max_members_NO_EC);
        if (attr == 4) printf ("   %2d   |",resourceinfos[i].max_members_NO_DTMF);
        if (attr == 5) printf ("   %2d   |",resourceinfos[i].max_members_NO_TONE);
        if (attr == 6) printf ("   %2d   |",resourceinfos[i].max_members_NO_COACHING);

        if (attr == 7) printf ("   %2d   |",resourceinfos[i].max_members_NO_EC_DTMF);
        if (attr == 8) printf ("   %2d   |",resourceinfos[i].max_members_NO_EC_TONE);
        if (attr == 9) printf ("   %2d   |",resourceinfos[i].max_members_NO_EC_COACHING);
        if (attr == 10) printf ("   %2d   |",resourceinfos[i].max_members_NO_DTMF_TONE);
        if (attr == 11) printf ("   %2d   |",resourceinfos[i].max_members_NO_DTMF_COACHING);
        if (attr == 12) printf ("   %2d   |",resourceinfos[i].max_members_NO_TONE_COACHING);

        if (attr == 13) printf ("   %2d   |",resourceinfos[i].max_members_NO_EC_DTMF_TONE);
        if (attr == 14) printf ("   %2d   |",resourceinfos[i].max_members_NO_EC_DTMF_COACHING);
        if (attr == 15) printf ("   %2d   |",resourceinfos[i].max_members_NO_EC_TONE_COACHING);
        if (attr == 16) printf ("   %2d   |",resourceinfos[i].max_members_NO_DTMF_TONE_COACHING);

        if (attr == 17) printf ("   %2d   |",resourceinfos[i].max_members_NO_EC_DTMF_TONE_COACHING);
    }
}

