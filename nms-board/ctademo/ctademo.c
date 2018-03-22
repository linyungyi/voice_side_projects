/*****************************************************************************
 *  FILE: ctademo.c
 *
 *  DESCRIPTION: a collection of functions used by the demo programs.
 *
 *  Some of these functions (DemoShowError and DemoShowEvent in particular)
 *  are simply utilities.
 *
 *  Most of them, however, represent convenient synchronous wrappers to
 *  Natural Access functions that are widely used in the accompanying demo
 *  programs.  Although using them makes the demo programs more concise
 *  and more readable, the library provides no mechanism for waiting on
 *  multiple drvids -- it implies the use of the
 *  single-call-resource-per-thread model.
 *
 *  The advantage of this is that it tends to make the application code
 *  significantly cleaner and simpler.  Furthermore, where threads are
 *  available, it does not restrict functionality.
 *
 *  Constants defined here are:
 *    DemoVoiceEncodings
 *    DemoNumVoiceEncodings
 *
 *  Functions defined here are:
 *    DemoSetReportLevel
 *    DemoShouldReport
 *    DemoReportComment
 *    DemoReportUnexpectedEvent
 *    DemoShowError
 *    DemoShowEvent
 *    DemoWaitForEvent
 *    DemoWaitForSpecificEvent
 *    DemoSetup
 *    DemoNCCSetup
 *    DemoShutdown
 *    DemoStartProtocol
 *    DemoStartNCCProtocol
 *    DemoStopProtocol
 *    DemoWaitForCall
 *    DemoWaitForNCCCall
 *    DemoAnswerCall
 *    DemoAnswerNCCCall
 *    DemoRejectCall
 *    DemoRejectNCCCall
 *    DemoPlaceCall
 *    DemoPlaceNCCCall
 *    DemoHangUp
 *    DemoHangNCCUp
 *    DemoGetDigits
 *    DemoPromptDigit
 *    DemoPlayMessage
 *    DemoPlayList
 *    DemoPlayText
 *    DemoPlayCurrentDateTime
 *    DemoRecordMessage
 *    DemoDelay
 *    DemoSpawnThread
 *    DemoErrorHandler
 *    DemoErrorHandlerContinue
 *    DemoLoadParFile
 *    DemoLoadParameters
 *    DemoSwiTerminusToMvip90Output
 *    DemoSwiTerminusToMvip90Input
 *    DemoMvip90InputtoMvip95Bus
 *    DemoMvip90InputtoMvip95Stream
 *    DemoMvip90InputtoMvip95Timeslot
 *    DemoMvip90OutputtoMvip95Bus
 *    DemoMvip90OutputtoMvip95Stream
 *    DemoMvip90OutputtoMvip95Timeslot
 *
 * 1996-2001 NMS Communications
 *****************************************************************************/


/****************************** Includes ************************************/
#include <assert.h>
#include "ctademo.h"
#include "ctadef.h"

/*************************** Global Constants *******************************/
const char *DemoVoiceEncodings[] =
{
    /*  0 */ "auto",
    /*  1 */ "NMS-16",
    /*  2 */ "NMS-24",
    /*  3 */ "NMS-32",
    /*  4 */ "NMS-64",
    /*  5 */ "",
    /*  6 */ "",
    /*  7 */ "",
    /*  8 */ "",
    /*  9 */ "",
    /* 10 */ "MULAW",
    /* 11 */ "ALAW",
    /* 12 */ "",
    /* 13 */ "PCM8M16",
    /* 14 */ "OKI-24",
    /* 15 */ "OKI-32",
    /* 16 */ "PCM11M8",
    /* 17 */ "PCM11M16",
    /* 18 */ "",
    /* 19 */ "",
    /* 20 */ "G726",
    /* 21 */ "",
    /* 22 */ "IMA-24",
    /* 23 */ "IMA-32",
    /* 24 */ "GSM",
    /* 25 */ "",
    /* 26 */ "G723-5.3",
    /* 27 */ "G723-6.4",
    /* 28 */ "G729A",
};

const unsigned DemoNumVoiceEncodings = ARRAY_DIM(DemoVoiceEncodings);


/*****************************************************************************
  DemoSetReportLevel
  DemoShouldReport

  DemoSetReportLevel specifies the level of diagnostics to be reported by
  the various ctademo functions.

  DemoShouldReport tests whether or not a given level of diagnostic should
  be reported.

  The supported values (DEMO_REPORT_*) are defined in ctademo.h.
 *****************************************************************************/
static unsigned demo_report_level = DEMO_REPORT_ALL;

void DemoSetReportLevel( unsigned level )
{
    demo_report_level = level;
}

int  DemoShouldReport( unsigned level )
{
    return ( demo_report_level >= level );
}

/* Accessor Functions for name of manager implementing ADI service
 */
static char *demo_adi_manager = "ADIMGR";

char *DemoGetADIManager()
{
    return demo_adi_manager;
}

void DemoSetADIManager( const char *mgr )
{
    demo_adi_manager = (char *)mgr;
}


/* Accessor functions for name of Natural Access configuration file.
 * Note that NULL implies \nms\ctaccess\cfg\cta.cfg IF AND ONLY
 * IF the list of services is NULL.
 */
static char *demo_cfg_file = NULL;

char *DemoGetCfgFile()
{
    return demo_cfg_file;
}

void DemoSetCfgFile( const char *filename )
{
    demo_cfg_file = (char *)filename;
}

/*****************************************************************************
  DemoReportComment
  DemoReportUnexpectedEvent

  Generate diagnostic outputs according to the current report level.
  (See DemoSetReportLevel, above.)
 *****************************************************************************/
void  DemoReportComment( char *s )
{
    if ( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
        printf( "\t%s\n", s );
}

void  DemoReportUnexpectedEvent( CTA_EVENT *eventp )
{
    if ( DemoShouldReport( DEMO_REPORT_UNEXPECTED ) )
    {
        /* If DemoShouldReport( DEMO_REPORT_ALL ), then the event is
         * assumed to have been displayed already.
         */
        if ( ! DemoShouldReport( DEMO_REPORT_ALL ) )
            DemoShowEvent( eventp );

        if ( !CTA_IS_USER_EVENT( eventp->id ) )
            printf( "\t\t\t  --unexpected event.\n" );
        else
            printf( "\t\t\t  --user generated event.\n" );

    }
}


/*****************************************************************************
  _demoGetText (internal function)

 Returns text for the corresponding error return code from the demo header file.
 *****************************************************************************/
static const char *_demoGetText( DWORD errorcode)
{
    switch (errorcode)
    {
        case (DWORD) FAILURE        : return "FAILURE";
        case (DWORD) DISCONNECT     : return "DISCONNECT";
        case (DWORD) FILEOPENERROR  : return "FILEOPENERROR";
        case (DWORD) PARMERROR      : return "PARMERROR";
        default                     : return NULL;
    }
}


/*****************************************************************************
  DemoShowError

  Print a message for a Natural Access or demo error.
 *****************************************************************************/
void DemoShowError( char *preface, DWORD errcode )
{
    char text[40];
    const char *demotext;

    if ((demotext = _demoGetText(errcode)) != NULL)
    {
        strncpy( text, demotext, sizeof(text));
        text[sizeof(text)-1] = '\0';
    }
    else
    {
        ctaGetText( NULL_CTAHD, errcode, text, sizeof(text) );
    }
    printf( "\t%s: %s\n", preface, text );
    return;
}


/*****************************************************************************
  DemoFreeCTABuf

  In Natural Access Client/Server mode, ensure proper release of Natural Access
  owned buffers.
 *****************************************************************************/
void DemoFreeCTABuff( CTA_EVENT *pEvent )
{
    if ( pEvent->buffer && (pEvent->size & CTA_INTERNAL_BUFFER) )

        ctaFreeBuffer( pEvent->buffer );
}

/*****************************************************************************
  DemoShowEvent

  Print information about a Natural Access event.
 *****************************************************************************/
void DemoShowEvent( CTA_EVENT *eventp )
{
#define FORMAT_BUFFER_SIZE 1000 /* max size ever needed */
    char format_buffer[FORMAT_BUFFER_SIZE];
    char *lineprefix = "\t";  /* default demo indent */


    format_buffer[0] = '\0';

    if ( CTA_IS_USER_EVENT( eventp->id ) )
    {
        /* Application specific event */
        sprintf( format_buffer, "%sEvent: Application 0x%08X,  value: 0x%08x\n",
                 lineprefix, eventp->id, eventp->value );
    }
    else
    {
        ctaFormatEvent(lineprefix, eventp, format_buffer, FORMAT_BUFFER_SIZE);
    }

    printf( "%s", format_buffer );

    return;
}


/*****************************************************************************
  DemoWaitForEvent

 *****************************************************************************/
void DemoWaitForEvent( CTAHD ctahd, CTA_EVENT *eventp )
{
    CTAQUEUEHD ctaqueuehd;
    DWORD ctaret;

    ctaGetQueueHandle(ctahd, &ctaqueuehd);

    ctaret = ctaWaitEvent( ctaqueuehd, eventp, CTA_WAIT_FOREVER);

    if (ctaret != SUCCESS)
    {
        printf( "\007ctaWaitEvent returned %x\n", ctaret);
        exit( 1 );
    }

    if ( DemoShouldReport( DEMO_REPORT_ALL ) )
        DemoShowEvent( eventp );
    return;
}


/*****************************************************************************
  DemoWaitForSpecificEvent

  This function loops, calling DemoWaitForEvent and discarding the result
  until the desired event arrives.  That event is then returned in the
  provided struct.  Displaying of events is controlled by DemoSetReportLevel().
 *****************************************************************************/
void DemoWaitForSpecificEvent( CTAHD ctahd,
                               DWORD desired_event, CTA_EVENT *eventp )
{
    for (;;)
    {
        DemoWaitForEvent (ctahd, eventp);

        if ( (ctahd == eventp->ctahd) && (eventp->id == desired_event) )
            break;

        DemoReportUnexpectedEvent( eventp );
    }

    return;
}


/*****************************************************************************
  DemoInitialize

  Initialize Natural Access with a list of services to be used later.
 *****************************************************************************/
void DemoInitialize(
                    CTA_SERVICE_NAME servicelist[],
                    unsigned         numservices )
{
    DWORD ret;
    CTA_INIT_PARMS initparms = {0};
    CTA_ERROR_HANDLER hdlr;

    /* Initialize size of init parms structure */
    initparms.size = sizeof(CTA_INIT_PARMS);
    initparms.ctacompatlevel = CTA_COMPATLEVEL;

    /* If daemon running then initialize tracing
     * and system global default parameters.
     */
    initparms.traceflags = CTA_TRACE_ENABLE;
    initparms.parmflags  = CTA_PARM_MGMT_SHARED;

    /* Get name of Natural Access configuration file */
    initparms.filename = DemoGetCfgFile();

    DemoReportComment( "Initializing and opening the Natural Access context..." );

    /* Set error handler to NULL and remember old handler */
    ctaSetErrorHandler( NULL, &hdlr );

    if ( ( ret = ctaInitialize( NULL, 0, &initparms ) ) != SUCCESS )
    {
        if (ret == CTAERR_WRONG_COMPATLEVEL)
        {
            printf(
    "Compatibility levels of application and Natural Access Libraries differ.\n"
    "Please recompile with current include files (CTA_COMPATLEVEL %d).\n",
                   CTA_COMPATLEVEL);
            exit (1);
        }
        else
        {
            printf("Unable to invoke ctaInitialize(). Error = 0x%x "
                   "(ref ctaerr.h)\n",ret);
            exit (1);
        }
    }
    else
    {
        ctaSetErrorHandler( hdlr, NULL );  /* restore error handler */

        DemoReportComment("Using system global default parms. Trace enabled." );
    }
}


/*****************************************************************************
  DemoOpenPort

  Synchronously opens a port on a device.
 *****************************************************************************/
void DemoOpenPort(
         unsigned         userid,        /* for ctaCreateContext              */
         char            *contextname,   /* for ctaCreateContext              */
         CTA_SERVICE_DESC services[],    /* for ctaOpenServices               */
         unsigned         numservices,   /* number of services                */
         CTAQUEUEHD      *ctaqueuehd,    /* returned Natural Access queue handle   */
         CTAHD           *ctahd)         /* returned Natural Access context handle */
{
    /* Open the Natural Access application queue, attaching all defined
     * service managers.
     */
    ctaCreateQueue( NULL, 0, ctaqueuehd );

    /* Create a context and opens services on new queue */
    DemoOpenAnotherPort(
                        userid,
                        contextname,
                        services,
                        numservices,
                        *ctaqueuehd,
                        ctahd );
}


/*****************************************************************************
  DemoOpenAnotherPort

  Synchronously opens another port on an existing device (queue).
 *****************************************************************************/
void DemoOpenAnotherPort(
        unsigned          userid,        /* for ctaCreateContext              */
        char             *contextname,   /* for ctaCreateContext              */
        CTA_SERVICE_DESC  services[],    /* for ctaOpenServices               */
        unsigned          numservices,   /* number of services                */
        CTAQUEUEHD        ctaqueuehd,    /* Natural Access queue handle            */
        CTAHD            *ctahd )        /* returned Natural Access context handle */
{
    CTA_EVENT event = { 0};

    /* Create a Natural Access context */
    if ( ctaCreateContext( ctaqueuehd, userid, contextname, ctahd ) != SUCCESS )
    {
        /* This routine should NOT receive an invalid queue handle */
        DemoReportComment( "Create context failed, invalid queue" );
        exit( 1 );
    }

    /* Open services */
    ctaOpenServices( *ctahd, services, numservices );

    /* Wait for services to be opened asynchronously */
    DemoWaitForSpecificEvent( *ctahd,
                              CTAEVN_OPEN_SERVICES_DONE, &event );
    DemoFreeCTABuff( &event );

    if ( event.value != CTA_REASON_FINISHED )
    {
        DemoShowError( "Open services failed", event.value );
        exit( 1 );
    }
}

/*****************************************************************************
  DemoSetup

  This function is maintained now for Backward Compatibility
  Please use DemoCSSetup to use Client-Server functionality better

 *****************************************************************************/
void DemoSetup(
          unsigned   ag_board,           /* AG board number                   */
          unsigned   mvip_stream,        /* MVIP stream                       */
          unsigned   mvip_slot,          /* MVIP timeslot                     */
          CTAQUEUEHD *ctaqueuehd,        /* returned Natural Access queue handle   */
          CTAHD      *ctahd)             /* returned Natural Access context handle */
{

    DemoCSSetup( ag_board, mvip_stream, mvip_slot, ctaqueuehd, ctahd,
                 "DEMOCONTEXT" );

}



/*****************************************************************************
  DemoCSSetup

  Synchronously opens a port on a device with the basic Natural Access services.
  Note that, since it always gives a userid of 0 to DemoOpenPort, it is good
  for only one port per AG device. Use DemoInitialize and DemoOpenPort
  individually to create contexts with different userid and contextname,
  and/or to add other services.

  Since this procedure calls DemoInitialize, this routine should only
  be called ONCE for the entire process.  Multithreaded programs should
  use DemoInitialize and DemoOpenPort separately.
 *****************************************************************************/
void DemoCSSetup(
          unsigned   ag_board,           /* AG board number                   */
          unsigned   mvip_stream,        /* MVIP stream                       */
          unsigned   mvip_slot,          /* MVIP timeslot                     */
          CTAQUEUEHD *ctaqueuehd,        /* returned Natural Access queue handle   */
          CTAHD      *ctahd,             /* returned Natural Access context handle */
          char       *cntx_name)         /* returned Natural Access context name   */
{
    CTA_SERVICE_NAME servicelist[] =     /* for ctaInitialize */
    {   { "ADI", ""},
        { "SWI", "SWIMGR"},
        { "VCE", "VCEMGR"}
    };
    CTA_SERVICE_DESC services[] =    /* for ctaOpenServices */
    {   { {"ADI", ""},       {0}, {0}, {0} },
        { {"SWI", "SWIMGR"}, {0}, {0}, {0} },
        { {"VCE", "VCEMGR"}, {0}, {0}, {0} }
    };

    /* Get Correct ADI Service Manager */
    services[0].name.svcmgrname = demo_adi_manager;
    servicelist[0].svcmgrname   = demo_adi_manager;

    /* Fill in ADI service (index 0) MVIP address information */
    services[0].mvipaddr.board    = ag_board;
    services[0].mvipaddr.stream   = mvip_stream;
    services[0].mvipaddr.timeslot = mvip_slot;
    services[0].mvipaddr.mode     = ADI_FULL_DUPLEX;

    DemoInitialize( servicelist, ARRAY_DIM(servicelist) );

    DemoOpenPort( 0, "DEMOCONTEXT", services, ARRAY_DIM(services),
                  ctaqueuehd, ctahd );
}


/*NCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCC*/


/*****************************************************************************
  DemoNCCSetup

  Synchronously opens a port on a device with the basic Natural Access services.
  Note that, since it always gives a userid of 0 to DemoOpenPort, it is good
  for only one port per AG device. Use DemoInitialize and DemoOpenPort
  individually to create contexts with different userid and contextname,
  and/or to add other services.

  Since this procedure calls DemoInitialize, this routine should only
  be called ONCE for the entire process.  Multithreaded programs should
  use DemoInitialize and DemoOpenPort separately.
 *****************************************************************************/
void DemoNCCSetup(
          unsigned   ag_board,           /* AG board number                   */
          unsigned   mvip_stream,        /* MVIP stream                       */
          unsigned   mvip_slot,          /* MVIP timeslot                     */
          CTAQUEUEHD *ctaqueuehd,        /* returned Natural Access queue handle   */
          CTAHD      *ctahd,             /* returned Natural Access context handle */
          char       *cntx_name)         /* returned Natural Access context name   */
{
    /***********************************************************
     *                   W A R N I N G  !!
     *                   =================
     * If the order of the services is changed, then you must
     * ensure that the assignment of demo_adi_manager is done
     * with the correct array offset to "ADI".
     */
    CTA_SERVICE_NAME servicelist[] =     /* for ctaInitialize */
    {
        { "NCC", "ADIMGR"},
        { "ADI", ""},
        { "SWI", "SWIMGR"},
        { "VCE", "VCEMGR"}

    };
    CTA_SERVICE_DESC services[] =    /* for ctaOpenServices */
    {
        { {"NCC", "ADIMGR"}, {0}, {0}, {0} },
        { {"ADI", ""},       {0}, {0}, {0} },
        { {"SWI", "SWIMGR"}, {0}, {0}, {0} },
        { {"VCE", "VCEMGR"}, {0}, {0}, {0} }
    };

    /* Get Correct ADI Service Manager -- See WARNING above */
    services[1].name.svcmgrname = demo_adi_manager;
    servicelist[1].svcmgrname   = demo_adi_manager;

    /* Fill in NCC service (index 0) MVIP address information */
    services[0].mvipaddr.board    = ag_board;
    services[0].mvipaddr.stream   = mvip_stream;
    services[0].mvipaddr.timeslot = mvip_slot;
    services[0].mvipaddr.mode     = ADI_FULL_DUPLEX;

    /* Fill in ADI service (index 1) MVIP address information */
    services[1].mvipaddr.board    = ag_board;
    services[1].mvipaddr.stream   = mvip_stream;
    services[1].mvipaddr.timeslot = mvip_slot;
    services[1].mvipaddr.mode     = ADI_FULL_DUPLEX;

    DemoInitialize( servicelist, ARRAY_DIM(servicelist) );

    DemoOpenPort( 0, cntx_name, services, ARRAY_DIM(services),
                  ctaqueuehd, ctahd );
}


/*NCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCC*/

/*****************************************************************************
  DemoShutdown

  Synchronously closes a context and its queue.
 *****************************************************************************/
void DemoShutdown( CTAHD ctahd )
{
    CTA_EVENT event;
    CTAQUEUEHD ctaqueuehd;

    ctaGetQueueHandle( ctahd, &ctaqueuehd );

    ctaDestroyContext( ctahd );    /* this will close services automatically */

    /* Wait for CTAEVN_DESTROY_CONTEXT_DONE */
    for (;;)
    {
        DWORD ctaret = ctaWaitEvent( ctaqueuehd, &event, CTA_WAIT_FOREVER);
        if (ctaret != SUCCESS)
        {
            printf( "\007ctaWaitEvent returned %x\n", ctaret);
            exit( 1 );
        }

        if ( DemoShouldReport( DEMO_REPORT_ALL ) )
        {
            DemoShowEvent( &event );
        }

        if (ctahd == event.ctahd && event.id == CTAEVN_DESTROY_CONTEXT_DONE)
        {
            break;
        }
        DemoReportUnexpectedEvent( &event );
    }

    DemoFreeCTABuff( &event );

    if ( event.value != CTA_REASON_FINISHED )
    {
        DemoShowError( "Destroying the Natural Access context failed", event.value );
        exit( 1 );
    }

    ctaDestroyQueue( ctaqueuehd );
    DemoReportComment( "Destroying the Natural Access context and queue...");
}


/*****************************************************************************
  DemoStartProtocol

  Synchronously starts up a protocol on a device.
 *****************************************************************************/
void DemoStartProtocol(
        CTAHD            ctahd,
        char            *protocol,   /* TCP to run (must be avail on board)   */
        WORD            *protparmsp, /* optional protocol-specific parms      */
        ADI_START_PARMS *stparmsp )  /* optional parms for adiStartProtocol   */
{
    CTA_EVENT event;
    DWORD     ret;

    ret = adiStartProtocol( ctahd, protocol, protparmsp, stparmsp );

    DemoWaitForSpecificEvent( ctahd, ADIEVN_STARTPROTOCOL_DONE, &event );
    DemoFreeCTABuff( &event );

    if ( event.value != CTA_REASON_FINISHED )
    {
        DemoShowError( "Start protocol failed", event.value );
        printf( "\tCheck the board configuration for available protocols.\n" );
        exit( 1 );
    }
}


/*NCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCC*/

/*****************************************************************************
  DemoStartNCCProtocol

  Synchronously starts up a protocol on a device.
 *****************************************************************************/
void DemoStartNCCProtocol(
        CTAHD            ctahd,
        char*            protocol,   /* TCP to run (must be avail on board)   */
        void*            protparms,  /* protocol-specific parameters to pass  */
        void*            mgrparms,   /* manager start parameters              */
        NCC_START_PARMS *stparmsp )  /* optional parms for nccStartProtocol   */
{
    CTA_EVENT event;
    DWORD     ret;

    ret = nccStartProtocol (ctahd, protocol, stparmsp, mgrparms, protparms);

    /* If the application is linked with ADI.LIB (libadi.so) instead of
     *  ADIAPI.LIB (libadimgr.so), then ADIERR_INVALID_QUEUEID will be returned;
     *  the error handler (which was registered with Natural Access) will not be
     *  called.
     */

    if ( ret != CTAERR_BAD_ARGUMENT )
    {
        DemoWaitForSpecificEvent( ctahd, NCCEVN_START_PROTOCOL_DONE, &event );
        DemoFreeCTABuff( &event );

        if ( event.value != CTA_REASON_FINISHED )
        {
            DemoShowError( "Start protocol failed", event.value );
            printf( "\tCheck board configuration for available protocols.\n" );
            exit( 1 );
        }
    }
    else
    {
        DemoShowError( "Start protocol failed", ret );
        printf( "\tCheck board configuration for available protocols.\n" );
        exit( 1 );
    }
}

/*NCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCC*/




/*****************************************************************************
  DemoStopProtocol

  Synchronously stop a protocol on a device.
 *****************************************************************************/
void DemoStopProtocol( CTAHD ctahd )
{
    CTA_EVENT event;

    adiStopProtocol( ctahd );

    DemoWaitForSpecificEvent( ctahd, ADIEVN_STOPPROTOCOL_DONE, &event );
    DemoFreeCTABuff( &event );

    if ( event.value != CTA_REASON_FINISHED )
    {
        DemoShowError( "Stop protocol failed", event.value );
        exit( 1 );
    }
}


/*****************************************************************************
  DemoWaitForCall

  Wait for an incoming call on the specified port, optionally returning the
  number called.  Assumes one port/dev.

Note: this may not detect an invalid ctahd if port.dev is valid.
 *****************************************************************************/
void DemoWaitForCall( CTAHD ctahd, char *calledaddr, char *callingaddr )
{

    DemoReportComment( "--------\n\tWaiting for incoming call..." );

    for (;;)
    {
        CTA_EVENT event;

        DemoWaitForEvent( ctahd, &event );

        switch ( event.id )
        {
            case ADIEVN_INCOMING_CALL:
                DemoReportComment( "Incoming Call..." );
                {
                    /* Get any address information for the application
                     * to play with.
                     */
                    ADI_CALL_STATUS status;
                    adiGetCallStatus( ctahd, &status, sizeof status  );
                    if ( calledaddr != NULL )
                        strcpy( calledaddr, status.calledaddr );
                    if ( callingaddr != NULL )
                        strcpy( callingaddr, status.callingaddr );
                }

                DemoFreeCTABuff( &event );
                return;

                /* Swallow these low-level events. */
            case ADIEVN_SEIZURE_DETECTED:
            case ADIEVN_INCOMING_DIGIT:
            case ADIEVN_PROTOCOL_ERROR :     /* e.g. abandoned after seizure */
            case ADIEVN_OUT_OF_SERVICE:      /* e.g, permanent signal        */
            case ADIEVN_IN_SERVICE:          /* back in service after OOS    */
                break;

            default:
                DemoReportUnexpectedEvent( &event );
                break;
        }

        /* Ensure proper release of Natural Access buffers in Client/Server mode */
        DemoFreeCTABuff( &event );

    }
}
/*NCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCC*/

/*****************************************************************************
  DemoWaitForNCCCall

  Wait for an incoming call on the specified port, optionally returning the
  number called.  Assumes one port/dev.

Note: this may not detect an invalid ctahd if port.dev is valid.
 *****************************************************************************/
void DemoWaitForNCCCall(
                        CTAHD       ctahd,
                        NCC_CALLHD *callhd,
                        char       *calledaddr,
                        char       *callingaddr )
{

    DemoReportComment( "--------\n\tWaiting for incoming call..." );
    for (;;)
    {
        CTA_EVENT event;
        DemoWaitForEvent( ctahd, &event );

        switch ( event.id )
        {
            case NCCEVN_INCOMING_CALL:
                DemoReportComment( "Incoming Call..." );

                /* Get any address information for the application
                 * to play with.
                 */
                {
                    NCC_CALL_STATUS status;
                    nccGetCallStatus( *callhd, &status, sizeof(status)  );
                    if ( calledaddr != NULL )
                        strcpy( calledaddr, status.calledaddr );
                    if ( callingaddr != NULL )
                        strcpy( callingaddr, status.callingaddr );
                }
                DemoFreeCTABuff( &event );
                return;

                /* Swallow these low-level events. */
            case NCCEVN_SEIZURE_DETECTED:
                *callhd = event.objHd;
                DemoReportComment("Seizure Detected...");
                break;
            case NCCEVN_RECEIVED_DIGIT:
            case NCCEVN_PROTOCOL_ERROR :     /* e.g. abandoned after seizure */
            case NCCEVN_LINE_OUT_OF_SERVICE:      /* e.g, permanent signal */
            case NCCEVN_LINE_IN_SERVICE:          /* back in service after OOS */
                break;
            case NCCEVN_CALL_DISCONNECTED:
                nccReleaseCall(*callhd, NULL);

                /* Free up Natural Access buffer if necessary before reusing
                 * structure
                 */
                DemoFreeCTABuff( &event );
                DemoWaitForSpecificEvent(ctahd, NCCEVN_CALL_RELEASED, &event);
                break;

            default:
                DemoReportUnexpectedEvent( &event );
                break;
        }

        /* Ensure proper release of Natural Access buffers in Client/Server mode */
        DemoFreeCTABuff( &event );

    }
}

/*NCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCC*/



/*****************************************************************************
  DemoAnswerCall

 *****************************************************************************/
int DemoAnswerCall( CTAHD ctahd, unsigned numrings )
{
    CTA_EVENT event;

    DemoReportComment( "Answering call..." );
    adiAnswerCall( ctahd, numrings );

    for (;;)
    {
        DemoWaitForEvent( ctahd, &event );

        switch ( event.id )
        {
            case ADIEVN_ANSWERING_CALL :
                /* expect CALL_CONNECTED after rings are complete */
                break;

            case ADIEVN_CALL_CONNECTED:
                DemoReportComment( "Call connected." );
                DemoFreeCTABuff( &event );
                return SUCCESS;

            case ADIEVN_CALL_DISCONNECTED:
                DemoReportComment( "Call abandoned." );
                DemoFreeCTABuff( &event );
                return DISCONNECT;

            case ADIEVN_REJECTING_CALL:
                DemoReportComment( "Board timed out waiting for answer." );
                /* Now wait for disconnect!  */
                break;

            default:
                DemoReportUnexpectedEvent( &event );
                break;
        }

        /* Ensure proper release of Natural Access buffers in Client/Server mode */
        DemoFreeCTABuff( &event );

    }

    UNREACHED_STATEMENT( return FAILURE; )
}
/*NCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCC*/
/*****************************************************************************
  DemoAnswerNCCCall

 *****************************************************************************/
int DemoAnswerNCCCall(
                      CTAHD      ctahd,
                      NCC_CALLHD callhd,
                      unsigned   numrings,
                      void      *answerparms )
{
    CTA_EVENT event;

    DemoReportComment( "Answering call..." );
    nccAnswerCall( callhd, numrings, answerparms );

    for (;;)
    {
        DemoWaitForEvent( ctahd, &event );

        switch ( event.id )
        {
            case NCCEVN_ANSWERING_CALL :
                /* expect CALL_CONNECTED after rings are complete */
                break;

            case NCCEVN_CALL_CONNECTED:
                DemoReportComment( "Call connected." );
                DemoFreeCTABuff( &event );
                return SUCCESS;

            case NCCEVN_CALL_DISCONNECTED:
                DemoReportComment( "Call abandoned." );
                DemoFreeCTABuff( &event );
                return DISCONNECT;

            case NCCEVN_REJECTING_CALL:
                DemoReportComment( "Board timed out waiting for answer." );
                /* Now wait for disconnect!  */
                break;

            default:
                DemoReportUnexpectedEvent( &event );
                break;
        }

        /* Ensure proper release of  buffers in C/S mode */
        DemoFreeCTABuff( &event );

    }

    UNREACHED_STATEMENT( return FAILURE; )
}
/*NCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCC*/
/*****************************************************************************
  DemoRejectCall

  Reject the call by playing busy or fast busy.
  Returns when the caller hangs up.
 *****************************************************************************/
void DemoRejectCall( CTAHD ctahd, unsigned method )
{
    CTA_EVENT event;

    DemoReportComment( "Rejecting call..." );

    /* This function doesn't support custom audio. */
    assert( method != ADI_REJ_USER_AUDIO );

    adiRejectCall( ctahd, method );

    for (;;)
    {
        DemoWaitForEvent(ctahd, &event);

        switch ( event.id )
        {
            case ADIEVN_REJECTING_CALL:
                break;

            case ADIEVN_CALL_DISCONNECTED:
                DemoReportComment( "Caller hung up." );
                DemoFreeCTABuff( &event );
                return;

            default:
                DemoReportUnexpectedEvent( &event );
                break;
        }

        /* Ensure proper release of Natural Access buffers in Client/Server mode */
        DemoFreeCTABuff( &event );

    }
}

/*NCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCC*/
/*****************************************************************************
  DemoRejectNCCCall

  Reject the call by playing busy or fast busy.
  Returns when the caller hangs up.
 *****************************************************************************/
void DemoRejectNCCCall(
                       CTAHD       ctahd,
                       NCC_CALLHD  callhd,
                       unsigned    method,
                       void       *params )
{
    CTA_EVENT event;

    DemoReportComment( "Rejecting call..." );
    /* This function doesn't support custom audio. */
    assert( method != NCC_REJECT_USER_AUDIO );

    nccRejectCall( callhd, method, params );

    for (;;)
    {
        DemoWaitForEvent(ctahd, &event);

        switch ( event.id )
        {
            case NCCEVN_REJECTING_CALL:
                break;

            case NCCEVN_CALL_DISCONNECTED:
                DemoReportComment( "Caller hung up." );
                DemoFreeCTABuff( &event );
                return;

            default:
                DemoReportUnexpectedEvent( &event );
                break;
        }

        /* Ensure proper release of Natural Access buffers in Client/Server mode */
        DemoFreeCTABuff( &event );
    }
}

/*NCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCC*/




/*****************************************************************************
  DemoPlaceCall

  There are fancier ways to handle "glare resolution" (arrival of an inbound
  call while trying to place an outbound call); here, we just reject the call.
 *****************************************************************************/
int DemoPlaceCall( CTAHD ctahd, char *digits, ADI_PLACECALL_PARMS *parmsp )
{
    CTA_EVENT event;

    if ( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
        printf( "--------\n\tPlacing a call to '%s'...\n", digits );
    adiPlaceCall( ctahd, digits, parmsp );

    for (;;)
    {
        DemoWaitForEvent( ctahd, &event );

        switch ( event.id )
        {
            case ADIEVN_SEIZURE_DETECTED:    /* "low-level" events */
            case ADIEVN_CALL_PROCEEDING:
            case ADIEVN_REMOTE_ALERTING:
            case ADIEVN_REMOTE_ANSWERED:
            case ADIEVN_PROTOCOL_ERROR:      /* false seizure case */
                break;

            case ADIEVN_PLACING_CALL:        /* Glare resolved */
                break;

            case ADIEVN_INCOMING_CALL:
                DemoReportComment( "--------\n\tRefusing inbound call..." );
                DemoRejectCall( ctahd, ADI_REJ_PLAY_REORDER );
                DemoFreeCTABuff( &event );
                return DISCONNECT;

            case ADIEVN_CALL_CONNECTED:
                DemoReportComment( "Connected." );
                DemoFreeCTABuff( &event );
                return SUCCESS;

            case ADIEVN_CALL_DISCONNECTED:
                DemoReportComment( "Call failed." );
                DemoFreeCTABuff( &event );
                return DISCONNECT;

            case ADIEVN_OUT_OF_SERVICE:
                DemoReportComment( "Call failed - line out of service.\n"
                                   "\t (waiting for in-service)\n" );
                break;

            case ADIEVN_IN_SERVICE:          /* back in service after O.O.S. */
                return FAILURE;

            default:
                DemoReportUnexpectedEvent( &event );
                break;
        }

        /* Ensure proper release of Natural Access buffers in Client/Server mode */
        DemoFreeCTABuff( &event );
    }
    UNREACHED_STATEMENT( return FAILURE; )
}
/*NCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCC*/



/*****************************************************************************
  DemoPlaceCall

  There are fancier ways to handle "glare resolution" (arrival of an inbound
  call while trying to place an outbound call); here, we just reject the call.
 *****************************************************************************/
int DemoPlaceNCCCall(
                     CTAHD       ctahd,
                     char       *digits,
                     char       *callingaddr,
                     void       *mgrparms,
                     void       *protparms,
                     NCC_CALLHD *callhd)
{
    CTA_EVENT event;
    DWORD     ret;

    if ( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
        printf( "--------\n\tPlacing a call to '%s'...\n", digits );

    ret = nccPlaceCall( ctahd,
                        digits,
                        callingaddr,
                        mgrparms,
                        protparms,
                        callhd );

    if (ret != SUCCESS)
    {
        DemoReportComment("PlaceCall failed...");
        return DISCONNECT;
    }

    while (1)
    {
        DemoWaitForEvent( ctahd, &event );

        switch ( event.id )
        {
            case NCCEVN_SEIZURE_DETECTED:
            case NCCEVN_CALL_PROCEEDING:
            case NCCEVN_REMOTE_ALERTING:
            case NCCEVN_REMOTE_ANSWERED:
            case NCCEVN_PROTOCOL_ERROR:      /* false seizure case */
                break;

            case NCCEVN_PLACING_CALL:        /* Glare resolved */
                break;

            case NCCEVN_INCOMING_CALL:
                DemoReportComment( "--------\n\tRefusing inbound call..." );
                DemoRejectNCCCall( ctahd, *callhd, NCC_REJECT_PLAY_REORDER,
                                   NULL );
                DemoFreeCTABuff( &event );
                return DISCONNECT;

            case NCCEVN_CALL_CONNECTED:
                DemoReportComment( "Connected." );
                DemoFreeCTABuff( &event );
                return SUCCESS;

            case NCCEVN_CALL_DISCONNECTED:
                DemoReportComment( "Call failed." );
                DemoFreeCTABuff( &event );
                return DISCONNECT;

            case NCCEVN_LINE_OUT_OF_SERVICE:
                DemoReportComment( "Call failed - line out of service.\n"
                                   "\t (waiting for in-service)\n" );
                break;

            case NCCEVN_LINE_IN_SERVICE:      /* back in service after O.O.S. */
                DemoFreeCTABuff( &event );
                return FAILURE;

            default:
                DemoReportUnexpectedEvent( &event );
                break;
        }

        /* Ensure proper release of Natural Access buffers in Client/Server mode */
        DemoFreeCTABuff( &event );
    }
    UNREACHED_STATEMENT( return FAILURE; )
}
/*NCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCC*/





/*****************************************************************************
  DemoHangUp

  Hang up the call, waiting for the Call Released event.
 *****************************************************************************/
int DemoHangUp( CTAHD ctahd )
{
    CTA_EVENT event;

    DemoReportComment( "Hanging up..." );

    adiReleaseCall( ctahd );

    for (;;)
    {
        DemoWaitForEvent( ctahd, &event );

        switch ( event.id )
        {
            case ADIEVN_COLLECTION_DONE:
            case VCEEVN_PLAY_DONE:
            case VCEEVN_RECORD_DONE:
            case ADIEVN_DIGIT_END:
                break;   /* swallow */

            case ADIEVN_CALL_DISCONNECTED:
                DemoReportComment( "Other party hung up." );
                break;

            case ADIEVN_CALL_RELEASED:
                DemoReportComment( "Call done." );
                return DISCONNECT;

            default:
                DemoReportUnexpectedEvent( &event );
                break;
        }

        /* Ensure proper release of Natural Access buffers in Client/Server mode */
        DemoFreeCTABuff( &event );
    }

    UNREACHED_STATEMENT( return FAILURE; )
}
/*NCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCC*/
/*****************************************************************************
  DemoHangNCCUp

  Hang up the call, waiting for the Call Released event.
 *****************************************************************************/
int DemoHangNCCUp(
                  CTAHD       ctahd,
                  NCC_CALLHD  callhd,
                  void       *hangparms,
                  void       *disparms )
{
    CTA_EVENT event;
    unsigned isdiscon = 0;
    NCC_CALL_STATUS   callstatus = {0};

    nccGetCallStatus(callhd, &callstatus, sizeof(callstatus));

    if (callstatus.held != 0 &&
        callstatus.state != NCC_CALLSTATE_DISCONNECTED)
    {
        DemoReportComment( "Retrieving call..." );
        nccRetrieveCall( callhd, NULL );
        for (;;)
        {
            DemoWaitForEvent( ctahd, &event );

            if ( event.id == NCCEVN_CALL_RETRIEVED )
            {
                DemoReportComment( "Call retrieved." );
                break;
            }
            else if ( event.id == NCCEVN_CALL_DISCONNECTED )
            {
                /* Call was disconnected before it was retrieved. */
                DemoReportComment( "Call disconnected." );
                break;
            }

            /* Ensure proper release of Natural Access buffers in Client/Server mode */
            DemoFreeCTABuff( &event );
        }

        /* Refresh call status information. */
        nccGetCallStatus(callhd, &callstatus, sizeof(callstatus));
    }

    if (callstatus.state != NCC_CALLSTATE_DISCONNECTED)
    {
        DemoReportComment( "Hanging up..." );

        nccDisconnectCall( callhd, disparms );
        do
        {
            DemoWaitForEvent(ctahd, &event);
            switch ( event.id )
            {
                case ADIEVN_DIGIT_END:
                    break;
                case NCCEVN_CALL_DISCONNECTED:
                    break;
            }
            /* Ensure proper release of Natural Access buffers in Client/Server mode */
            DemoFreeCTABuff( &event );

        }while (event.id != NCCEVN_CALL_DISCONNECTED);

        /* Refresh call status information. */
        nccGetCallStatus(callhd, &callstatus, sizeof(callstatus));
    }

    if (callstatus.state == NCC_CALLSTATE_DISCONNECTED)
    {
        DemoReportComment( "Releasing...");

        nccReleaseCall( callhd, hangparms );
        for (;;)
        {
            DemoWaitForEvent( ctahd, &event );

            switch ( event.id )
            {
                case ADIEVN_COLLECTION_DONE:
                case VCEEVN_PLAY_DONE:
                case VCEEVN_RECORD_DONE:
                case ADIEVN_DIGIT_END:
                    break;   /* swallow */

                case NCCEVN_CALL_DISCONNECTED:
                    DemoReportComment( "Other party hung up." );
                    break;

                case NCCEVN_CALL_RELEASED:
                    DemoReportComment( "Call done." );
                    return DISCONNECT;

                default:
                    DemoReportUnexpectedEvent( &event );
                    break;
            }

            /* Ensure proper release of Natural Access buffers in Client/Server mode */
            DemoFreeCTABuff( &event );
        }
    }

    UNREACHED_STATEMENT( return FAILURE; )
}
/*NCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCCNCC*/

/*****************************************************************************
  DemoGetDigits

  Collect specified number of DTMFs, synchronously.
 *****************************************************************************/
int DemoGetDigits( CTAHD ctahd, int count, char *digits )
{
    CTA_EVENT         event;
    ADI_COLLECT_PARMS parms;
    int               ret;

    ctaGetParms( ctahd, ADI_COLLECT_PARMID, &parms, sizeof( parms ) );
    parms.firsttimeout = 3000;  /* 3 seconds to wait for first digit */
    parms.intertimeout = 3000;  /* 3 seconds between digits */

    memset( digits, 0, count+1 );   /* +1 for null-terminator */
    adiCollectDigits( ctahd, digits, count, &parms );

    for (;;)
    {
        DemoWaitForEvent(ctahd, &event);

        switch ( event.id )
        {
            case ADIEVN_COLLECTION_DONE:
                if ( event.value == CTA_REASON_RELEASED )
                    ret = DISCONNECT;
                else if ( CTA_IS_ERROR( event.value ) )
                    ret = FAILURE;
                else if ( strlen( digits ) == 0 )
                    ret = FAILURE;
                else
                    ret = SUCCESS;

       /* Ensure proper release of Natural Access buffers in Client/Server mode */
                DemoFreeCTABuff( &event );
                return ret;

            case ADIEVN_CALL_DISCONNECTED:
                DemoReportComment( "Caller hung up." );
                break;

            case ADIEVN_DIGIT_BEGIN:
            case ADIEVN_DIGIT_END:
                break;

            default:
                DemoReportUnexpectedEvent( &event );
                break;
        }

        /* Ensure proper release of Natural Access buffers in Client/Server mode */
        DemoFreeCTABuff( &event );
    }

    UNREACHED_STATEMENT( return FAILURE; )
}


/*****************************************************************************
  DemoPromptDigit

  Play a voice prompt and get a single digit back from the user.
 *****************************************************************************/
int DemoPromptDigit( CTAHD ctahd, char *promptfile, unsigned message,
                     unsigned encoding, VCE_PLAY_PARMS *parmsp,
                     char *digit, int maxtries )
{
    int i;
    for ( i = 0; i < maxtries; i++ )
    {
        int ret;

        if ( DemoPlayMessage( ctahd, promptfile, message,
                              encoding, parmsp ) != SUCCESS )
            return FAILURE;

        ret = DemoGetDigits( ctahd, 1, digit );
        if ( ret==SUCCESS || ret==DISCONNECT )
            return ret;
    }

    return FAILURE;
}


/*****************************************************************************
  DemoPlayMessage

  Synchronously play a voice file message, swallowing events up to and
  including VCEEVN_PLAY_DONE.  (All events are displayed, however.)
 *****************************************************************************/
int DemoPlayMessage(
         CTAHD            ctahd,        /* Natural Access context handle           */
         char            *filename,     /* name of voice file                 */
         unsigned         message,      /* the message number to play         */
         unsigned         encoding,     /* the encoding scheme of the file    */
         VCE_PLAY_PARMS  *parmsp )      /* optional parms for adiStartPlaying */
{
    if ( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
        printf( "\tPlaying file '%s', msg #%d...\n", filename, message );

    return DemoPlayList( ctahd, filename, &message, 1, encoding, parmsp );
}


/*****************************************************************************
  DemoPlayList

  Synchronously play a list of voice file messages, swallowing events up to
  and including VCEEVN_PLAY_DONE.  (All events are displayed, however.)
 *****************************************************************************/
int DemoPlayList(
         CTAHD            ctahd,        /* Natural Access context handle           */
         char            *filename,     /* name of voice file                 */
         unsigned         message[],    /* array of message numbers to play   */
         unsigned         count,        /* number of messages to play         */
         unsigned         encoding,     /* the encoding scheme of the file    */
         VCE_PLAY_PARMS  *parmsp )      /* optional parms for adiStartPlaying */
{
    VCEHD            vh;
    CTA_EVENT        event;
    int              ret;

    if ( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
        printf( "\tPlaying %d messages from '%s'...\n", count, filename );

    if ( vceOpenFile( ctahd, filename, VCE_FILETYPE_VOX, VCE_PLAY_ONLY,
                      encoding, &vh )  != SUCCESS )
        return FAILURE;

    if ( vcePlayList( vh, message, count, parmsp ) != SUCCESS )
    {
        vceClose( vh );
        return FAILURE;
    }

    for (;;)
    {
        DemoWaitForEvent( ctahd, &event );

        switch ( event.id )
        {
            case VCEEVN_PLAY_DONE:
                vceClose( vh );

                if ( event.value == CTA_REASON_RELEASED )
                    ret =  DISCONNECT;
                else if ( CTA_IS_ERROR(event.value) )
                {
                    DemoShowError( "DemoPlayFile failed", event.value );
                    ret = FAILURE;
                }
                else
                    ret = SUCCESS;

       /* Ensure proper release of Natural Access buffers in Client/Server mode */
                DemoFreeCTABuff( &event );
                return ret;

            case NCCEVN_CALL_DISCONNECTED:
            case ADIEVN_CALL_DISCONNECTED:
                DemoReportComment( "Other party hung up." );
                break;

            case ADIEVN_DIGIT_BEGIN:
            case ADIEVN_DIGIT_END:
                break;

            default:
                DemoReportUnexpectedEvent( &event );
                break;
        }

        /* Ensure proper release of Natural Access buffers in Client/Server mode */
        DemoFreeCTABuff( &event );
    }

    UNREACHED_STATEMENT( return FAILURE; )
}


/*****************************************************************************
  DemoPlayText

  Synchronously play a list of voice file messages, generated using the
  text prompt conversion functions, swallowing events up to
  and including VCEEVN_PLAY_DONE.  (All events are displayed, however.)
 *****************************************************************************/
int   DemoPlayText(
                   CTAHD           ctahd,       /* context handle             */
                   char           *textstring,  /* name of voice file         */
                   VCE_PLAY_PARMS *playparmsp ) /* optional parms for vceList */
{
#define MAXENTRIES 100
    unsigned msglst[MAXENTRIES];
    unsigned msgcnt = 0;
    VCEPROMPTHD prompthandle;

    if ( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
        printf( "\tPlaying text string: '%s'...\n", textstring );

    if (  vceLoadPromptRules( ctahd, "AMERICAN", &prompthandle ) != SUCCESS )
        return FAILURE;

    if ( vceBuildPromptList( prompthandle, 0, textstring,
                             msglst, MAXENTRIES, &msgcnt ) != SUCCESS )
    {
        vceUnloadPromptRules( prompthandle );
        return FAILURE;
    }

    vceUnloadPromptRules( prompthandle );
    return DemoPlayList( ctahd, "AMERICAN", msglst, msgcnt, 0, playparmsp );
}


/*****************************************************************************
  DemoPlayCurrentDateTime

  Synchronously play the current date and time using the
  text prompt conversion functions, swallowing events up to
  and including VCEEVN_PLAY_DONE.  (All events are displayed, however.)
 *****************************************************************************/
int   DemoPlayCurrentDateTime(
                 CTAHD             ctahd,       /* context handle             */
                 unsigned          datetime,    /* format to speak            */
                 VCE_PLAY_PARMS   *playparmsp ) /* optional parms for vceList */
{
    char texttime[80];
    time_t today = time( &today );
    struct tm *temptime = localtime( &today );

    if ( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
        printf( "\tPlaying current date/time, format=%d...\n", datetime );

#define _FMT(x)  strftime( texttime, sizeof texttime, x, temptime )
    switch ( datetime )
    {
        case DATETIME_DAY:       _FMT( "%a" );                    break;
        case DATETIME_DATE:      _FMT( "%m/%d" );                 break;
        case DATETIME_FULL_DATE: _FMT( "%m/%d/%y" );              break;
        case DATETIME_DAY_DATE:  _FMT( "%a %b %d" );              break;
        case DATETIME_TIME:      _FMT( "%H:%M" );                 break;
        case DATETIME_DATE_TIME: _FMT( "%m/%d at %H:%M" );        break;
        default:
        case DATETIME_COMPLETE:  _FMT( "Mon %m/%d/%y at %H:%M" ); break;
    }
#undef _FMT

    return DemoPlayText( ctahd, texttime, playparmsp );
}


/*****************************************************************************
  DemoRecordMessage

  Synchronously record a voice file, swallowing events up to and including
  ADI_EVN_RECORD_DONE.  (All events are displayed, however.)
 *****************************************************************************/
int DemoRecordMessage(
         CTAHD             ctahd,     /* Natural Access context handle             */
         char             *filename,  /* name of voice file                   */
         unsigned          message,   /* the message number to play           */
         unsigned          encoding,  /* the encoding scheme of the file      */
         VCE_RECORD_PARMS *parmsp )   /* optional parms for adiStartRecording */
{
    VCEHD            vh;
    CTA_EVENT        event;
    unsigned         maxtime = 0;  /* no cap on length of recording */
    int              ret = SUCCESS;

    if ( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
        printf( "\tRecording File '%s'...\n", filename );

    if ( ( vceOpenFile( ctahd, filename, VCE_FILETYPE_VOX, VCE_PLAY_RECORD,
                        encoding, &vh ) != SUCCESS )
         && ( vceCreateFile( ctahd, filename, VCE_FILETYPE_VOX,
                             encoding, NULL, &vh ) != SUCCESS ) )
        return FAILURE;

    if ( vceRecordMessage( vh, message, 0, parmsp ) != SUCCESS )
    {
        vceClose( vh );
        return FAILURE;
    }

    for (;;)
    {
        DemoWaitForEvent( ctahd, &event );

        switch ( event.id )
        {
            case VCEEVN_RECORD_DONE:
                vceClose( vh );
                if ( event.value == CTA_REASON_RELEASED )
                    ret = DISCONNECT;

                else if ( CTA_IS_ERROR(event.value) )
                {
                    DemoShowError( "Record failed", event.value );
                    ret = FAILURE;
                }
                else
                    ret = SUCCESS;

       /* Ensure proper release of Natural Access buffers in Client/Server mode */
                DemoFreeCTABuff( &event );
                return ret;

            case ADIEVN_CALL_DISCONNECTED:
                DemoReportComment( "Caller hung up." );
                break;

            case ADIEVN_DIGIT_BEGIN:
            case ADIEVN_DIGIT_END:
                break;

            default:
                DemoReportUnexpectedEvent( &event );
                break;
        }

        /* Ensure proper release of Natural Access buffers in Client/Server mode */
        DemoFreeCTABuff( &event );
    }

    UNREACHED_STATEMENT( return FAILURE; )
}


/*****************************************************************************
  DemoDelay

 *****************************************************************************/
int DemoDelay(
              CTAHD     ctahd,       /* Natural Access context handle              */
              unsigned  ms )         /* millisecond delay                     */
{
    CTA_EVENT        event;
    unsigned         ret = SUCCESS;

    if ( DemoShouldReport( DEMO_REPORT_COMMENTS ) )
        printf( "\tDelaying %d ms...\n", ms );

    if ( adiStartTimer( ctahd, ms, 1 ) != SUCCESS )
    {
        return FAILURE;
    }

    for (;;)
    {
        DemoWaitForEvent( ctahd, &event );

        switch ( event.id )
        {
            case ADIEVN_TIMER_DONE:
                if ( CTA_IS_ERROR(event.value) )
                {
                    DemoShowError( "Record failed", event.value );
                    ret = FAILURE;
                }

       /* Ensure proper release of Natural Access buffers in Client/Server mode */
                DemoFreeCTABuff( &event );
                return ret;

            case ADIEVN_CALL_DISCONNECTED:
                DemoReportComment( "Caller hung up." );
                ret = DISCONNECT;
                break;

            default:
                DemoReportUnexpectedEvent( &event );
                break;
        }

        /* Ensure proper release of Natural Access buffers in Client/Server mode */
        DemoFreeCTABuff( &event );

    }

    UNREACHED_STATEMENT( return FAILURE; )
}


/*****************************************************************************
  DemoSleep

  An OS-independent sleep function.

  time    time in seconds to sleep
*****************************************************************************/
void DemoSleep( int time )
{
#if defined (WIN32)
    Sleep( time*1000 );

#elif defined (UNIX)
    sleep( time );
#endif
    return;
}


/*****************************************************************************
  DemoSpawnThread

  A simple OS-independent thread-creation function.
  Takes a function and one argument; returns SUCCESS or FAILURE
 *****************************************************************************/
int DemoSpawnThread( THREAD_RET_TYPE (THREAD_CALLING_CONVENTION *func)(void *),
                 void *arg )
{
#if defined (WIN32)
    unsigned thraddr;
    if ( _beginthreadex( NULL, 0, func, arg, 0, &thraddr ) == 0 )
        return (FAILURE);
    return (SUCCESS);
#elif defined (UNIX)
    thread_t tid;
    if ( thr_create( NULL, 0, func, arg, THR_DETACHED, &tid ) == 0)
         return (SUCCESS);
    return (FAILURE);
#endif
}


/*****************************************************************************
  DemoErrorHandler

  An error handler to be registered with ctaSetErrorHandler(), which will
  cause it to be invoked whenever a Natural Access function is about to return
  an error.
  This one simply prints an error message and then terminates the application.
 *****************************************************************************/
DWORD NMSSTDCALL DemoErrorHandler( CTAHD ctahd, DWORD errorcode,
                                   char *errortext, char *func )
{
    if ( errorcode == CTAERR_FILE_NOT_FOUND )
        return errorcode;                    /* needs to be handled in app */

    if (ctahd == NULL_CTAHD)
        printf( "\007Error in %s [ctahd=null]: %s (%#x)\n",
                func, errortext, errorcode );
    else
        printf( "\007Error in %s [ctahd=%#x]: %s (%#x)\n",
                func, ctahd, errortext, errorcode );

    /* NOTE that if if you don't end the process within this routing,
       you MUST return the error code that caused this routine to be called.
       Refer to the commented line of code below. */
    exit( errorcode );

    /* If the error handler returns, the return code is passed back to
     * the API function, which will return the error code to the caller.
     * See the function below
     */
    /* return errorcode; */
}

/*****************************************************************************
  DemoErrorHandlerContinue

  An error handler to be registered with ctaSetErrorHandler(), which will
  cause it to be invoked whenever a Natural Access function is about to return
  an error.
  This one simply prints an error message and returns the error code the the
  application.
 *****************************************************************************/
DWORD NMSSTDCALL DemoErrorHandlerContinue ( CTAHD ctahd, DWORD errorcode,
                                            char *errortext, char *func )
{
    if ( errorcode == CTAERR_FILE_NOT_FOUND )
        return errorcode;                    /* needs to be handled in app */

    if (ctahd == NULL_CTAHD)
        printf( "\007Error in %s [ctahd=null]: %s (%#x)\n",
                func, errortext, errorcode );
    else
        printf( "\007Error in %s [ctahd=%#x]: %s (%#x)\n",
                func, ctahd, errortext, errorcode );

    return errorcode;
}


/*****************************************************************************
  DemoLoadParFile

  Reads a parameter file and calls ctaSetParmByName to set a new default
  value for each parameter specified in the file.
 *****************************************************************************/

/* Used in DemoLoadParFile and DemoLoadParameters: */
static char *mystrlwr( char *string );

/* Used in DemoLoadParFile: */
static void strip_parfile_comments( char *str );
static void decode_parm( char *literal, unsigned format,
                         BYTE *b, unsigned *psize );

int  DemoLoadParFile( CTAHD ctahd, char *filename )
{
    FILE         *fp;
    DWORD         ctaret;
    CTA_PARM_INFO parminfo;
    int           line, i;
    unsigned      nsize;
    char          temp [ 256 ];
    char          name [ 60 ];
    char          value[ 60 ];
    char          tmpstr[ 80 ] = "Loading parm file ";

    if ( filename == NULL )
        return FAILURE;

    strcat( tmpstr, filename );
    strcat( tmpstr, "..." );

    DemoReportComment( tmpstr );

    if ( ( fp = fopen( filename, "r" ) ) == NULL )
    {
        printf( "DemoLoadParFile couldn't open file %s: %s\n",
                filename, strerror(errno) );
        return FILEOPENERROR;
    }

    line = 0;

    /*
     * find [ctapar] section in .ini file
     */
    while ( !feof( fp ) )
    {
        fgets( (char *)temp, sizeof temp, fp );  /* read a line */

        if ( feof( fp ) )
        {
            printf ( "DemoLoadParFile: %s: No [ctapar] section.\n", filename );
            return PARMERROR;
        }

        line++;

        if ( strncmp( mystrlwr( temp ), "[ctapar]", 8)  == 0 )
            break;
    }

    /*
     * process all parameter changes
     */
    while ( !feof( fp ) )
    {
        fgets( (char *)temp, sizeof temp, fp );   /* read a line */
        if ( feof( fp ) )
            break;
        line++;

        if ( temp[0] == '[' )             /* on new [] section, all done */
            break;

        strip_parfile_comments( temp );

        i = sscanf( (char *)temp, " %s = %s %s", name, value, tmpstr );

        if ( ( i == 0 ) || ( i == -1 ) )  /* empty line */
            continue;

        if ( i != 2 )                     /* not exactly 2 arguments */
        {
            printf( "DemoLoadParFile: %s (line %d): Syntax error: not exactly"
                    " 2 arguments\n", filename, line );

            fclose( fp );
            return PARMERROR;
        }

        /* Get info for this field: */
        ctaret = ctaGetParmInfo ( 0, name, 0, &parminfo );
        if ( ctaret != SUCCESS )
        {
            printf( "DemoLoadParFile: %s (line %d): Error: invalid name '%s'\n",
                    filename, line, name );

            fclose( fp );
            return PARMERROR;
        }

        decode_parm( value, (unsigned)parminfo.format, (BYTE*)temp, &nsize );

        /* Parameter file must have service name prepended to the
           parameter name followed by a '.' ??? */

        ctaret = ctaSetParmByName( ctahd, name, temp, nsize );
        if ( ctaret != SUCCESS )
        {
            sprintf( "DemoLoadParFile: %s (line %d): Error setting"
                     " parameter '%s'\n", filename, line, name );

            fclose( fp );
            return PARMERROR;
        }
    }

    fclose( fp );
    return SUCCESS;
}


/*****************************************************************************
  DemoLoadParameters

  Loads a country-specific parameter file specified by a two three-letter
  abbreviations, one for product YYY (MFC, EAM, etc.) and country XXX
  (THA, CHN, etc.). If NULL or "" is given for the country, the file "YYY.par"
  is loaded, where YYY represents the product string.
  Loading "YYY.par" configures the parameters for the most recent country
  installed on the system.  Country-specific parameter files, "YYYXXX.par",
  are found either locally or in an OS-dependent parameter directory.

  The first argument specifies the context for which the parameters will be
  changed.  Specify NULL_CTAHD for a process-wide change; however, if this
  is the case, the application should call ctaRefreshParms() before calling
  adiStartProtocol(), to activate the process-wide parameters for the
  current context.

 *****************************************************************************/
#ifdef UNIX
#define COUNTRYPARDIR "/opt/nms/ctaccess/cfg/"
#else
#define COUNTRYPARDIR "\\nms\\ctaccess\\cfg\\"
#endif

int  DemoLoadParameters( CTAHD ctahd, char *protocol, char *product )
{
    char filename[128];
    char buffer [128];
    FILE *fp;
    int  printf_on_errors;

    /* protocol name is mandatory */
    if ( ( protocol == NULL ) || ( protocol[0] == '\0' ) )
        return FAILURE;

    /* check whether a real string was passed in */
    if ( ( product == NULL ) || ( product[0] == '\0'  ) )
    {
        /* if no product specification is given, use "adi" */
        printf_on_errors = FALSE;
        strcpy ( filename, "adi" );
    }
    else
    {
        /* a product specification is given */
        printf_on_errors = TRUE;
        strcpy ( filename, mystrlwr(product) );
    }
    strcat ( filename, mystrlwr(protocol) );
    strcat ( filename, ".par" );

    /* create the non-local file name by appending the local file name
     * to a fixed path (where the international installation places the
     * yyy.par and yyyxxx.par files).  Note that for Windows,
     * this function assumes that the executable is run from the drive on
     * which Natural Access was installed.
     */

    if ( ( fp = fopen( filename, "r" ) ) !=  NULL )
    {
        /* found .par file in local directory */
        fclose( fp );
        DemoLoadParFile( ctahd, filename );
        return SUCCESS;
    }

    else
    {
        sprintf( buffer, "%s%s", COUNTRYPARDIR, filename );
        strcpy (filename, buffer);
        if ( ( fp = fopen( filename, "r" ) ) !=  NULL )
        {
            /* found .par file  in country-specific .par directory */
            fclose( fp );
            DemoLoadParFile( ctahd, filename );
            return SUCCESS;
        }

        else
        {
            /* did not find .par file */
            if ( printf_on_errors )
                printf( "DemoLoadCountryPar: couldn't open file %s: %s\n",
                        filename, strerror(errno) );
            return FILEOPENERROR;
        }
    }
}

/*****************************************************************************
  strip_parfile_comments

  Removes all '#' delineated comments from str.  If the '#' occurs within
  a quoted string (double-quotes), it is ignored.  Used in DemoLoadParFile.
 *****************************************************************************/
static void strip_parfile_comments( char *str )
{
    char *p_quote, *p_pound, *ps;

    for ( ps = str;; ps++ )
    {
        p_pound = strchr( ps, '#' );
        if ( p_pound == NULL ) return;  /* If there are no '#'s, we're done. */

        p_quote = strchr( ps, '"' );   /* Check for any double-quotes. */
        if ( p_quote == NULL )          /* If there are no double-quotes, cut */
        {
            /* off the string at the '#' & return */
            *p_pound = '\0';
            return;
        }

        if ( p_pound < p_quote )  /* If the '#' is before the first quote,   */
        {
            /* cut off the string at the '#' & return. */
            *p_pound = '\0';
            return;
        }
        /* otherwise, the '#' is in a quoted */
        /* string, so advance to the next    */
        ps = strchr( p_quote+1, '"' );  /* double-quote and go on            */
        if ( ps == NULL ) return;
    }
}


/*****************************************************************************
  decode_parm

  Used in DemoLoadParFile.
 *****************************************************************************/
static void decode_parm( char *literal, unsigned format,
                         BYTE *b, unsigned *psize )
{
    WORD wvalue;
    DWORD dvalue;
    INT16 jvalue;
    unsigned len;

    switch ( format )
    {
        case CTA_FMT_WORD:
            wvalue = (WORD) strtoul( literal, NULL, 0 );
            memcpy( b, &wvalue, sizeof( WORD ) );
            *psize = sizeof( WORD );
            break;

        case CTA_FMT_INT16:
            jvalue = (INT16) strtol( literal, NULL, 0 );
            memcpy( b, &jvalue, sizeof( INT16 ) );
            *psize = sizeof( INT16 );
            break;

        case CTA_FMT_INT32:    /* Unsigned is used for INT32 to allow         */
            /* entering DSP 'backdoor' values (0x8000xxxx) */
        case CTA_FMT_DWORD:
            dvalue = (DWORD) strtoul( literal, NULL, 0 );
            memcpy( b, &dvalue, sizeof( DWORD ) );
            *psize = sizeof( DWORD );
            break;

        case CTA_FMT_STRING:
            len = strlen( literal );
            if ( *literal == '"' )         /* remove quotes if necessary */
            {
                literal++;
                len -= 2;
            }
            memcpy( (char *) b, literal, len );
            *(b+len) = '\0';             /* note: might be overwriting */
            *psize = len+1;              /*  unwanted ending '"'       */
            break;

        case CTA_FMT_BYTE:
            *b = (BYTE) strtoul( literal, NULL, 0 );
            *psize = sizeof( BYTE );
            break;

        case CTA_FMT_INT8:
            *b = (BYTE) strtol( literal, NULL, 0 );
            *psize = sizeof( char );
            break;

        default:
            break;
    }
    return;
}


/*****************************************************************************
  mystrlwr

  Changes a string to all lowercase (in place) and returns a pointer to the
  resulting string.  Provided because some system libraries do not include it.
  Used in DemoLoadParFile and DemoLoadCountryPar.
 *****************************************************************************/
static char *mystrlwr( char *string )
{
    char *str_p = string;
    while ( ( *str_p = tolower( *str_p ) ) != '\0' )
        str_p++;
    return string;
}

#if !defined (WIN32)

/*---------------------------------------------------------------------------
 * Case-insensitive strcmp() for non-NT systems.
 *--------------------------------------------------------------------------*/

int stricmp( const char *s1, const char *s2 )
{
    for ( ; ; s1++, s2++ )
    {
        char c = toupper(*s1), d = toupper(*s2);

        if (c != d)
            return c - d;
        else if (c == '\0')
            break;
    }
    return 0;
}
#endif

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

    /* YM010405 support -@ */
    if ( ! (isalnum(c) || (c == '@') )
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


/*****************************************************************************
  DemoGetFileType

  Returns the voice encoding corresponding to the given file extension,
  or 0 if unidentifiable.
 *****************************************************************************/
unsigned DemoGetFileType( char *extension )
{
    unsigned filetype;

    /* Some unixes do not have stricmp(). */
    if (strcmp (extension, "wav") == 0
        || strcmp (extension, "WAV") == 0
        || strcmp (extension, "wave") == 0
        || strcmp (extension, "WAVE") == 0
        || strcmp (extension, "2") == 0)
        filetype = VCE_FILETYPE_WAVE;
    else if (strcmp (extension, "vox") == 0
             || strcmp (extension, "VOX") == 0
             || strcmp (extension, "1") == 0)
        filetype = VCE_FILETYPE_VOX;
    else if (strcmp (extension, "vce") == 0
             || strcmp (extension, "VCE") == 0
             || strcmp (extension, "flat") == 0
             || strcmp (extension, "FLAT") == 0
             || strcmp (extension, "3") == 0)
        filetype = VCE_FILETYPE_FLAT;
    else
        filetype = 0;

    return filetype;
}

#if defined (WIN32)
#define DEMO_PATH_SLASH '\\'
#else
#define DEMO_PATH_SLASH '/'
#endif

/*****************************************************************************
  DemoDetermineFileType

  Expands a file name and infers file type if not specified.
  If there is no extension in the file name and the file type is 0, then
  ".vox" is appended and file type is set to VCE_FILETYPE_VOX.
 *****************************************************************************/
void DemoDetermineFileType(
            char      *filename,    /* input name, with or without extension  */
            unsigned   forcelocal,  /* if TRUE and no path in filename, then  */
                                    /*  prepend "./" to fullname.             */
            char      *fullname,    /* output (expanded) filename             */
            unsigned  *filetype )   /* input/output (0=>infer from extension) */
{
    char *pext;

    fullname[0] = '\0';
    if (strpbrk (filename, "/\\:") == NULL)
        /* Filename includes no path or drive specification. */
        if (forcelocal)
        {
            fullname[0] = '.';
            fullname[1] = DEMO_PATH_SLASH;
            fullname[2] = '\0';
        }
    strcat (fullname, filename);

    /* Find the provided extension, if any. */
    /* Look for last '.' and verify that it is not followed by a later slash */
    pext = strrchr (filename, '.');
    if (pext != NULL && strpbrk (pext, "/\\:") != NULL)
        pext = NULL;

    /* If file type is 0, infer from filename extension.
     * A specified filetype implies a default extension.
     * In absence of both filetype and extension, assume VOX.
     */
    if (*filetype == 0)
    {
        if (pext == NULL)
            *filetype = VCE_FILETYPE_VOX;
        else
            if ((*filetype = DemoGetFileType (pext+1)) == 0)
                /* Map unrecognized extenstions to FLAT. */
                *filetype = VCE_FILETYPE_FLAT;
    }

    /* If no extension in filename, append default. */
    if (pext == NULL)
        switch (*filetype)
        {
            case VCE_FILETYPE_VOX  :
                strcat (fullname, ".vox");
                break;

            case VCE_FILETYPE_WAVE :
                strcat (fullname, ".wav");
                break;

            case VCE_FILETYPE_FLAT :
                strcat (fullname, ".vce");
                break;

            default:
                break;
        }
    return;
}

/*--------------------- MVIP-95/MVIP-90 Conversion Functions -----------*/
#define even(A) ((A%2) == 0)

void DemoSwiTerminusToMvip90Output(SWI_TERMINUS t, SWI_TERMINUS *r)
{
    if (t.bus == MVIP95_MVIP_BUS)
    {
        if (even(t.stream))
        {
            r->bus = MVIP95_MVIP_BUS;
            r->stream = (t.stream/2)+8;
            r->timeslot = t.timeslot;
        }
        else
        {
            r->bus = MVIP95_MVIP_BUS;
            r->stream = (t.stream-1)/2;
            r->timeslot = t.timeslot;
        }
    }
    else if (t.bus == MVIP95_LOCAL_BUS)
    {
        if (even(t.stream))
        {
            r->bus = MVIP95_MVIP_BUS;
            r->stream = 0xFFFFFFFF;
            r->timeslot = t.timeslot;
        }
        else
        {
            r->bus = MVIP95_MVIP_BUS;
            r->stream = ((t.stream-1)/2)+16;
            r->timeslot = t.timeslot;
        }
    }
    else if (t.bus == MVIP95_MC1_BUS)
    {
        if (even(t.stream))
        {
            r->bus = MVIP95_MC1_BUS;
            r->stream = 0xFFFFFFFF;
            r->timeslot = t.timeslot;
        }
        else
        {
            r->bus = MVIP95_MC1_BUS;
            r->stream = ((t.stream-1)/2)+16;
            r->timeslot = t.timeslot;
        }
    }
}

void DemoSwiTerminusToMvip90Input(SWI_TERMINUS t, SWI_TERMINUS *r)
{
    if (t.bus == MVIP95_MVIP_BUS)
    {
        if (even(t.stream))
        {
            r->bus = MVIP95_MVIP_BUS;
            r->stream = t.stream/2;
            r->timeslot = t.timeslot;
        }
        else
        {
            r->bus = MVIP95_MVIP_BUS;
            r->stream = ((t.stream-1)/2)+8;
            r->timeslot = t.timeslot;
        }
    }
    else if (t.bus == MVIP95_LOCAL_BUS)
    {
        if (even(t.stream))
        {
            r->bus = MVIP95_LOCAL_BUS;
            r->stream = (t.stream/2)+16;
            r->timeslot = t.timeslot;
        }
        else
        {
            r->bus = MVIP95_LOCAL_BUS;
            r->stream = 0xFFFFFFFF;
            r->timeslot = t.timeslot;
        }
    }
    else if (t.bus == MVIP95_MC1_BUS)
    {
        if (even(t.stream))
        {
            r->bus = MVIP95_MC1_BUS;
            r->stream = (t.stream/2)+16;
            r->timeslot = t.timeslot;
        }
        else
        {
            r->bus = MVIP95_MC1_BUS;
            r->stream = 0xFFFFFFFF;
            r->timeslot = t.timeslot;
        }
    }
}

DWORD DemoMvip90InputtoMvip95Bus(SWI_TERMINUS t)
{
    DWORD  ret;

    if ((t.stream >= 0) && (t.stream <= 15))
    {
        ret = MVIP95_MVIP_BUS;
    }
    else if (t.stream >= 16)
    {
        ret = MVIP95_LOCAL_BUS;
    }

    return(ret);
}

DWORD DemoMvip90InputtoMvip95Stream(SWI_TERMINUS t)
{
    DWORD ret;

    if ((t.stream >= 0) && (t.stream <= 7))
    {
        ret = t.stream*2;
    }
    else if ((t.stream >= 8) && (t.stream <= 15))
    {
        ret = ((t.stream-8)*2)+1;
    }
    else if (t.stream >= 16)
    {
        ret = (t.stream-16)*2;
    }

    return(ret);
}

DWORD DemoMvip90InputtoMvip95Timeslot(SWI_TERMINUS t)
{
    return t.timeslot;
}

DWORD DemoMvip90OutputtoMvip95Bus(SWI_TERMINUS t)
{
    DWORD ret;

    if ((t.stream >= 0) && (t.stream <= 15))
    {
        ret = MVIP95_MVIP_BUS;
    }
    else if (t.stream >= 16)
    {
        ret = MVIP95_LOCAL_BUS;
    }

    return(ret);
}

DWORD DemoMvip90OutputtoMvip95Stream(SWI_TERMINUS t)
{
    DWORD ret;

    if ((t.stream >= 0) && (t.stream <= 7))
    {
        ret = (t.stream*2)+1;
    }
    else if ((t.stream >= 8) && (t.stream <= 15))
    {
        ret = ((t.stream-8)*2);
    }
    else if (t.stream >= 16)
    {
        ret = ((t.stream-16)*2)+1;
    }

    return(ret);
}

DWORD DemoMvip90OutputtoMvip95Timeslot(SWI_TERMINUS t)
{
    return t.timeslot;
}


/*----------------------------------------------------------------------------
  DemoSetupKbdIO()

  Sets up stuff for unbuffered keyboard I/O.
  init = 1  Initialized for unbuffered keyboard I/O
  init = 0  Restore original settings for tty
  ----------------------------------------------------------------------------*/
void DemoSetupKbdIO( int init )
{
#if defined (UNIX)
    static struct termio tio, old_tio;
    static int initialized = 0;
    static int fd;

    if ( init == 0 )
    {
        if ( initialized == 0 )
            return;

        ioctl( fd, TCSETAW, &old_tio );  /* Restore original settings */
        initialized = 0;
        return;
    }

    /* Set up UNIX for unbuffered I/O */
    setbuf( stdout, NULL );
    setbuf( stdin, NULL );

    fd = open( "/dev/tty", O_RDWR );
    ioctl( fd, TCGETA, &old_tio );      /* Remember original setting */
    ioctl( fd, TCGETA, &tio );
    tio.c_lflag    &= ~ICANON;
    tio.c_cc[VMIN]  = 1;
    tio.c_cc[VTIME] = 0;
    ioctl( fd, TCSETA, &tio );

    initialized = 1;

#elif defined (WIN32) || defined (__NETWARE_386__)
    if ( init != 0 )
    {
        setbuf( stdout, NULL );        /* use unbuffered i/o */
        setbuf( stdin, NULL );
    }
#endif
}

void RestoreSetupKbdIO( void )
{
    DemoSetupKbdIO( 0 );
}

