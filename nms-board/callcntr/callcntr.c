/*************************************************************************
 *  FILE: callcntr.c
 *
 *  DESCRIPTION:  This program demonstrates the use of the point-to-point
 *                switch API to connect incoming and outgoing calls.
 *
 * Copyright (c) 1999-2002 NMS Communications.  All rights reserved.
 *************************************************************************/
#define VERSION_NUM  "3"
#define VERSION_DATE __DATE__
#include "ctademo.h"
#include "ppxdef.h"

#ifndef MULTITHREAD
#define ENTERCRITSECT()      SUCCESS
#define EXITCRITSECT()       SUCCESS
#else
#define ENTERCRITSECT()      EnterCritSection()
#define EXITCRITSECT()       ExitCritSection()
#endif

/*----------------------------- Application Definitions ---------------*/
#define MAX_THREADS 24
#define OPERATOR     0
#define HOLD_MSG     1
#define LISTENER     2

/* Connection Context States */
#define CTX_IDLE     0
#define CTX_ONHOLD   1
#define CTX_BUSY     2
#define CTX_SEIZED   3
#define CTX_HUNGUP   4

#define CUST_HANGUP -1

typedef struct
{
    CTAQUEUEHD      ctaqueuehd;     /* Natural Access queue handle               */
    CTAHD       hd;             /* Incoming call Natural Access context handle   */
    unsigned    time_slot;      /* timeslot in use by "listener"        */
    unsigned    dsp_stream;     /* DSP Stream (MVIP 95)                 */
    DWORD       brd;            /* AG board number                      */
    DWORD       swi;            /* Switch number                        */
    char       *protocol;       /* TCP to start                         */
    unsigned    id;             /* thread number                        */
    unsigned    conn_id;        /* thread ID at other end of connection */
    DWORD       conn_state;     /* state of the connection context      */
} MYCONTEXT;

/*----------------------------- Application Data -----------------------*/

static MYCONTEXT        MyContext[MAX_THREADS] = {0};
static PPXHD            ppxhd[MAX_THREADS];
static PPX_HANDLE_PARMS ppx_parms = {sizeof(PPX_HANDLE_PARMS), 0x0, NULL };

unsigned   in_board          = 0;
unsigned   out_board         = 1;
unsigned   in_swtch          = 0;
unsigned   out_swtch         = 1;
unsigned   incoming_slot     = 0;
unsigned   outgoing_slot     = 0;
char      *incoming_protocol = "lps0";
char      *outgoing_protocol = "lps0";
char      *hold_protocol     = "nocc";
unsigned   nListeners        = 1;
MYCONTEXT *Operator          = &(MyContext[OPERATOR]);

PPX_MVIP_ADDR listeners[MAX_THREADS], talkers[MAX_THREADS];

/* Prototypes */
int InteractWithCaller(CTAHD ctahd);
void FinalInit(void);


/*--------------------------------------------------------------------------

  OS Independent functions for dealing with critical sections of code

  --------------------------------------------------------------------------*/

#if defined (UNIX)

#ifdef MULTITHREAD
#ifdef SOLARIS
#include <thread.h>
#endif
#include  <synch.h>
#endif
typedef  void *        MTXHDL;      /* Mutex Sem Handle*/

  #ifdef MULTITHREAD
    #if defined (SCO)
      #define MUTEX_T                 rmutex_t
      #define MUTEX_LOCK(mtx)         rmutex_lock(mtx)
      #define MUTEX_UNLOCK(mtx)       rmutex_unlock(mtx)
    #else
      /*
       *  Solaris has no recursive mutices; we have to emulate them.
       */
      typedef struct
      {
        thread_t   tID;          /* Thread ID                           */
        int        useCount;     /* Recursion count                     */
        mutex_t    realMutex;    /* Native (non-recursive) mutex struct */
      } Rmtx;
      #define MUTEX_T                 Rmtx
      #define MUTEX_LOCK(mtx)         mutex_lock(&((mtx)->realMutex))
      #define MUTEX_UNLOCK(mtx)       mutex_unlock(&((mtx)->realMutex))
    #endif /* SOLARIS */
  #endif /* MULTITHREAD */


#elif defined (WIN32)
typedef  void*         MTXHDL;      /* Mutex Sem Handle */
#endif

#if defined (UNIX)

static  MUTEX_T  init_mutex = { 0 };

void EnterCritSection( void )
{
    MUTEX_LOCK( &init_mutex );
}

void ExitCritSection( void )
{
    MUTEX_UNLOCK( &init_mutex );
}


#elif defined (WIN32)

static BOOL             CriticalSectionInitialized = FALSE;
static CRITICAL_SECTION CritSection = { 0 };

void EnterCritSection ( void )
{
    if ( CriticalSectionInitialized == FALSE )
    {
        InitializeCriticalSection( &CritSection );
    }

    EnterCriticalSection (&CritSection);
}


void ExitCritSection ( void )
{
    LeaveCriticalSection ( &CritSection );
}

#endif

/* ------------------------------------------------------------------------- */

void PrintHelp( void )
{
    printf( "Usage: callcntr [opts]\n" );
    printf( "  where opts are:\n\n" );
    printf( "  -A adi_mgr       Natural Access manager to use for ADI service.         "
            "Default = %s\n", DemoGetADIManager() );
    printf( "  -b n             OAM board #: inbound ('customer') calls   "
            "Default = %d\n", in_board );
    printf( "  -B n             OAM board #: outbound ('operator') call   "
            "Default = %d\n", out_board );
    printf( "  -x n             PPX switch number for inbound calls.        "
            "Default = %d\n", in_swtch );
    printf( "  -X n             PPX switch number for outbound call.        "
            "Default = %d\n", out_swtch );
    printf( "  -s n,n,...,n     Incoming ('customer') DSP timeslots         "
            "Default = %d\n", incoming_slot );
    printf( "  -S n             Outgoing ('operator') DSP timeslot          "
            "Default = %d,%d\n", 1,0 );
    printf( "  -i protocol      Incoming ('customer') Protocol to run.      "
            "Default = %s\n", incoming_protocol );
    printf( "  -o protocol      Outgoing ('operator') Protocol to run.      "
            "Default = %s\n", outgoing_protocol );
    printf( "  -F filename      Natural Access Configuration File name.          "
            "Default = None\n" );

}


/* -------------------------------------------------------------------------
  Process command line options.
  ------------------------------------------------------------------------- */
void ProcessArgs( int argc, char **argv )
{
    int         c;
    unsigned    p[8], i;
    MYCONTEXT  *cx = &MyContext[LISTENER];

    while( ( c = getopt( argc, argv, "A:B:b:F:X:x:S:s:i:o:Hh?" ) ) != -1 )
    {
        switch( c )
        {
            case 'A':
                DemoSetADIManager( optarg );
                break;
            case 'b':
                sscanf( optarg, "%d", &in_board );
                break;
            case 'B':
                sscanf( optarg, "%d", &out_board );
                break;
            case 'F':
                DemoSetCfgFile( optarg );
                break;
            case 'x':
                sscanf( optarg, "%d", &in_swtch );
                break;
            case 'X':
                sscanf( optarg, "%d", &out_swtch );
                break;
            case 'S':
                sscanf( optarg, "%d", &outgoing_slot );
                break;
            case 's':
                nListeners = sscanf( optarg, "%d,%d,%d,%d,%d,%d,%d,%d",
                                     &p[0],&p[1],&p[2],&p[3],&p[4],&p[5],&p[6],&p[7]);
                if ( nListeners < 1 )
                {
                    PrintHelp();
                    exit( 1 );
                }
                else
                {
                    incoming_slot = p[0];
                    for ( i = 0 ; i < nListeners ; i++, cx++)
                    {
                        cx->time_slot = p[i] ;
                    }
                }
                break;
            case 'i':
                incoming_protocol = optarg;
                break;
            case 'o':
                outgoing_protocol = optarg;
                break;
            case 'h':
            case 'H':
            case '?':
            default:
                PrintHelp();
                exit( 1 );
                break;
        }
    }
    if (optind < argc)
    {
        printf( "Invalid option.  Use -h for help.\n" );
        exit( 1 );
    }
}

/*-----------------------------------------------------------------------
  FinalInit()

  Store global settings in each context.
  -----------------------------------------------------------------------*/
void FinalInit()
{
    MYCONTEXT *cx;
    unsigned   i;

    cx = &MyContext[OPERATOR];
    cx->time_slot = outgoing_slot;
    cx->swi = out_swtch;
    cx->brd = out_board;
    cx->protocol = outgoing_protocol;
    cx->id = OPERATOR;
    cx->conn_state = CTX_IDLE;

    cx = &MyContext[HOLD_MSG];
    cx->time_slot = outgoing_slot + 1;
    cx->swi = out_swtch;
    cx->brd = out_board;
    cx->protocol = hold_protocol;
    cx->id = HOLD_MSG;
    cx->conn_state = CTX_IDLE;

    for ( i = 0, cx = &MyContext[LISTENER] ; i < nListeners ; i++, cx++ )
    {
        cx->swi = in_swtch;
        cx->brd = in_board;
        cx->protocol = incoming_protocol;
        cx->id = i + LISTENER;
        cx->conn_state = CTX_IDLE;
    }

    return ;
}
/*----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
  GetState

  Thread safe routine to read a port's state
  -----------------------------------------------------------------------*/
DWORD GetState( unsigned port_id )
{
    DWORD state;

    ENTERCRITSECT();
    state = MyContext[ port_id ].conn_state;
    EXITCRITSECT();

    return( state );
}

/*-----------------------------------------------------------------------
  GetState

  Thread safe routine to set a port's state
  -----------------------------------------------------------------------*/
void SetState( unsigned port_id, DWORD state )
{
    ENTERCRITSECT();
    MyContext[ port_id ].conn_state = state;
    EXITCRITSECT();
}

/*-----------------------------------------------------------------------
  TestAndSetState

  Thread safe test-and-set operation on a port's state; used to
  seize control of the operator port.
  -----------------------------------------------------------------------*/
int TestAndSetState( unsigned port_id, DWORD test_state, DWORD new_state )
{
    int   ret = FALSE;

    ENTERCRITSECT();
    if (MyContext[ port_id ].conn_state == test_state)
    {
        MyContext[ port_id ].conn_state = new_state;
        ret = TRUE;
    }
    EXITCRITSECT();

    return ret;
}


/*-----------------------------------------------------------------------
  GetExtension

  Play prompt, and get digits from user.
  -----------------------------------------------------------------------*/
int GetExtension( CTAHD ctahd, int count, char *digits )
{
    int ret;
    int i;
    int maxtries = 2;

    printf( "\tGetting extension...\n");
    for( i = 0; i < maxtries; i++ )
    {
        if( DemoPlayMessage( ctahd, "ctademo", PROMPT_GETEXT,
                             ADI_ENCODE_NMS_24, NULL ) !=
            SUCCESS )
            return FAILURE;

        printf( "\tCollecting digits to dial...\n" );
        if( ( ret = DemoGetDigits( ctahd, count, digits ) ) == SUCCESS )
            return SUCCESS;

        if( ret == DISCONNECT )
            return ret;
    }

    if( DemoPlayMessage( ctahd, "ctademo", PROMPT_GOODBYE,
                         ADI_ENCODE_NMS_24, NULL ) ==
        DISCONNECT )
        return DISCONNECT;

    return -1;
}

/*-----------------------------------------------------------------------
  InteractWithCaller

  Greet caller.
  -----------------------------------------------------------------------*/
int InteractWithCaller(CTAHD ctahd)
{
    int ret;

    /* Play message to the caller. */
    adiFlushDigitQueue( ctahd );
    ret = DemoPlayMessage(ctahd, "ctademo", PROMPT_WELCOME,
                          ADI_ENCODE_NMS_24, NULL);
    if (ret != SUCCESS )
    {
        return ret;
    }

    /* Tell the caller that we're going to try making the call.
     * "I'll try that extension now."
     */
    ret = DemoPlayMessage(ctahd, "ctademo", PROMPT_WILLTRY,
                          ADI_ENCODE_NMS_24, NULL );
    if (ret != SUCCESS )
    {
        return ret;
    }

    return SUCCESS;

}

/*-----------------------------------------------------------------------
  SetupInOutConx

  Create or destroy a QUAD connection on the local voice and signalling
  streams.
  -----------------------------------------------------------------------*/
DWORD SetupInOutConx( MYCONTEXT *cx, int make_conn, DWORD mode)
{
    DWORD         ret ;

    listeners[cx->id].switch_number = cx->swi ;
    listeners[cx->id].bus = MVIP95_LOCAL_BUS;
    listeners[cx->id].stream =  1 + cx->dsp_stream;    /* DSP input */
    listeners[cx->id].timeslot = cx->time_slot;

    talkers[cx->id].switch_number = cx->swi;
    talkers[cx->id].bus = MVIP95_LOCAL_BUS;
    talkers[cx->id].stream = 0;                /* line output */
    talkers[cx->id].timeslot = cx->time_slot;

    if( make_conn )
        ret = ppxConnect( cx->hd, &talkers[cx->id], &listeners[cx->id],
                          mode  );
    else
        ret = ppxDisconnect( cx->hd, &talkers[cx->id], &listeners[cx->id],
                             mode  );

    if (ret != SUCCESS)
    {
        printf("ppxConnect failed\n");
        exit(-1);
    }

    return SUCCESS;
}

/*-----------------------------------------------------------------------
  VoiceConx()

  Makes or breaks a DUPLEX connections between the voice lines of the
  Operator's port ("talker") and the Customer's port ("listener").
  -----------------------------------------------------------------------*/
DWORD VoiceConx( CTAHD ctahd, MYCONTEXT *cust, int make_conn )
{
    DWORD         ret;

    listeners[cust->id].stream = 1;                    /* line input */
    listeners[cust->id].timeslot = cust->time_slot;
    listeners[cust->id].switch_number = cust->swi ;
    listeners[cust->id].bus = MVIP95_LOCAL_BUS;

    if ( GetState( cust->id ) == CTX_ONHOLD )
    {
        /*
         * If the caller had been put on hold, need to take them
         * off of hold before making the voice connection to the
         * Operator.
         */

        ppxRemoveListeners( ppxhd[HOLD_MSG], &listeners[cust->id], 1 );
    }

    talkers[OPERATOR].switch_number = Operator->swi ;
    talkers[OPERATOR].bus = MVIP95_LOCAL_BUS;
    talkers[OPERATOR].stream = 0;                    /* line output */
    talkers[OPERATOR].timeslot = Operator->time_slot;

    if ( make_conn )
    {
        ret = ppxConnect( ctahd, &(talkers[OPERATOR]),
                          &(listeners[cust->id]), PPX_DUPLEX);
        if (ret != SUCCESS)
        {
            printf("ppxConnect failed\n");
            exit(-1);
        }
        Operator->conn_id = cust->id;
    }
    else
    {
        /* Purposely turn off error handler. The voice connection may
         *  have already been disconnected by the other end. If this
         *  function returns an error, that's OK in this situation.
         */
        ctaSetErrorHandler( NULL, NULL );
        ppxDisconnect( ctahd, &(talkers[OPERATOR]),
                       &(listeners[cust->id]), PPX_DUPLEX);
        /* Reinstate error handler. */
        ctaSetErrorHandler( DemoErrorHandler, NULL );
    }
    return SUCCESS;
}

/*-----------------------------------------------------------------------
  WhileTheyWait()

  Create a connection block which is used to manage the voice output
  to callers that have been placed on hold.  This connecton block
  allows easy management of a single talker, multiple listener situation.
  -----------------------------------------------------------------------*/

void WhileTheyWait( MYCONTEXT *cx )
{
    DWORD e;

    DemoStartProtocol( cx->hd, cx->protocol, NULL, NULL );

    /* Allocate a connection block  */

    e = ppxCreateConnection( cx->hd,"", &ppx_parms,
                             &(ppxhd[cx->id]) );
    if (e != SUCCESS)
    {
        printf("Unable to create a connection block; returns: %d\n",e);
        exit(-1);
    }

    /*
     * Set this port as the talker on this "Please hold...will try
     * that extension now" connection
     */
    talkers[cx->id].switch_number = cx->swi;
    talkers[cx->id].bus = MVIP95_LOCAL_BUS;
    talkers[cx->id].stream = cx->dsp_stream;
    talkers[cx->id].timeslot = cx->time_slot;

    e = ppxSetTalker( ppxhd[cx->id], &talkers[cx->id] );
    if( e != SUCCESS )
    {
        printf("Could not set talker\n");
        exit(-1);
    }

    for(;;)
    {
        /* Continuously play message */

        adiFlushDigitQueue( cx->hd );
        e = DemoPlayMessage( cx->hd, "ctademo",  PROMPT_WILLTRY,
                             ADI_ENCODE_NMS_24, NULL );
        if ( e!= SUCCESS )
        {
            printf("Could not play hold message\n");
            exit(-1);
        }

        DemoSleep( 2 );
    }
}
/*-----------------------------------------------------------------------
  KeepOnHold()

  Adds this caller as a listener to the hold message connection block.
  -----------------------------------------------------------------------*/
DWORD KeepOnHold( MYCONTEXT *cx )
{
    DWORD e;
    unsigned port_id;
    CTA_EVENT       event;

    port_id = cx->id;

    if ( GetState ( port_id ) != CTX_ONHOLD )
    {
        /* Need to add as a listener to the hold message
         * connection block.
         */
        SetupInOutConx( cx, FALSE, PPX_DUPLEX );

        SetState( port_id, CTX_ONHOLD );

        listeners[port_id].switch_number = cx->swi;
        listeners[port_id].bus = MVIP95_LOCAL_BUS;
        listeners[port_id].stream = 1;                 /* line input */
        listeners[port_id].timeslot = cx->time_slot;

        e = ppxAddListeners( ppxhd[HOLD_MSG],
                             &listeners[port_id], 1 );
        if (e != SUCCESS)
        {
            return e;
        }
    }

    /* Wait awhile before retrying the Operator */
    e = ctaWaitEvent( cx->ctaqueuehd, &event, 5000 );

    if ( e != SUCCESS )
    {
        return(e);
    }
    else
    {
        switch ( event.id )
        {
            case ADIEVN_CALL_DISCONNECTED:

                /* Caller hungup before connecting to the operator.*/
                return(CUST_HANGUP);

            default:
                break;
        }
    }
    return(SUCCESS);
}


/*-----------------------------------------------------------------------
  TakeOffHold()

  Removes this caller from the hold message connection block.
  -----------------------------------------------------------------------*/
void TakeOffHold( MYCONTEXT *cx )
{
    DWORD e;

    /*
     * Customer didn't wait to get connected; remove connection to
     * the DSP's outgoing message.
     */

    listeners[cx->id].stream = 1;     /* Line input */
    e = ppxRemoveListeners( ppxhd[HOLD_MSG], &listeners[cx->id], 1 );
    if (e != SUCCESS)
    {
        printf("\007ppxRemoveListeners returned %d\n", e);
        return ;
    }

    return ;
}
/*-----------------------------------------------------------------------
  RunDemo()

  Open a ctahd, start protocol, and loop answering incoming calls.
  -----------------------------------------------------------------------*/
THREAD_RET_TYPE THREAD_CALLING_CONVENTION RunDemo( void *arg )

{
    const unsigned   port_id = (unsigned)arg;
    MYCONTEXT       *cx = &MyContext[port_id];
    char             called_digits[20];
    char             calling_digits[20];
    CTA_EVENT        event;
    ADI_CONTEXT_INFO info;
    DWORD            ctaret;
    int              bConnected = FALSE;

    /* for ctaOpenServices */
    CTA_SERVICE_DESC services[] =
    {   { {"ADI", ""      }, { 0 }, { 0 }, { 0 } },
        { {"VCE", "VCEMGR"}, { 0 }, { 0 }, { 0 } },
        { {"PPX", "PPXMGR"}, { 0 }, { 0 }, { 0 } }
    };

    /* Get Correct ADI Service Manager */
    services[0].name.svcmgrname = DemoGetADIManager();

    /*
     * Fill in ADI service MVIP address information for incoming
     * connection
     */
    services[0].mvipaddr.board    = cx->brd;
    services[0].mvipaddr.stream   = 0;
    services[0].mvipaddr.timeslot = cx->time_slot;
    services[0].mvipaddr.mode     = ADI_FULL_DUPLEX;

    DemoOpenPort( port_id, "PPXCONTEXT", services, ARRAY_DIM(services),
                  &(cx->ctaqueuehd), &(cx->hd) );

    /* Get the DSP's Mvip-95 stream number */
    adiGetContextInfo( cx->hd, &info, sizeof info);
    cx->dsp_stream = info.stream95;

    if( port_id == HOLD_MSG )
    {
        /*
         * Setup DSP to play message when the operator is
         * unavailable for caller. We will add and remove
         * listeners to the DSP's associated connection block.
         */

        WhileTheyWait( cx ); /* This never returns */
    }

    /*
     * Set up  QUAD (VOICE + SIGNAL) connections on both the incoming and
     * outgoing ports, connection the line interfaces to the
     * local DSP on the board.
     */
    SetupInOutConx( cx, TRUE, PPX_QUAD) ;


    /* Loop forever, receiving calls and processing them.
     * If any function returns a failure or disconnect,
     * simply hang up and wait for the next call.
     */

    for(;;)
    {
        if( port_id == OPERATOR )
        {
            /*
             * This operator thread waits here until customer calls in
             * and a voice connection is made. The customer's thread will
             * cause the operator's "conn_state" to transition to CTX_BUSY
             */

            DWORD state;
            while( (state = GetState( OPERATOR )) == CTX_IDLE
                   || state == CTX_SEIZED )
            {
                DemoSleep( 1 ); /* wait until needed */
            }

            if (state == CTX_HUNGUP)
                goto hangup;

            /* otherwise proceed to wait for hangup  event */

        }
        else /* Customer line */
        {
            DemoStartProtocol( cx->hd, cx->protocol, NULL, NULL );

            /* Wait for the "customer" to call in. */
            DemoWaitForCall( cx->hd, called_digits, calling_digits );

            if ( DemoAnswerCall( cx->hd, 1 /* #rings */ ) != SUCCESS )
                goto hangup;

            if ( InteractWithCaller( cx->hd ) != SUCCESS )
                goto hangup;

            do
            {
                /* Stay in loop until hangup or connect to operator */
                if (TestAndSetState( OPERATOR, CTX_IDLE, CTX_SEIZED ))
                {
                    /*
                     * Initiate a protocol on the Operator resource so we
                     * can accept calls.
                     */
                    DemoStartProtocol( Operator->hd, Operator->protocol,
                                       NULL, NULL );

                    /* Place second call on the line. */
                    if ( DemoPlaceCall( Operator->hd, called_digits, NULL )
                         != SUCCESS )
                    {
                        SetState( OPERATOR, CTX_HUNGUP );
                        goto hangup;
                    }

                    /*
                     * Make voice connection to operator. This will
                     * set both the customer and the operator to
                     * the CTX_BUSY state.
                     */

                    if ( GetState ( cx->id ) != CTX_ONHOLD )
                    {
                        /* Disconnect caller from DSP */
                        SetupInOutConx( cx, FALSE, PPX_DUPLEX );
                    }
                    /* Disconnect operator line from its DSP */
                    SetupInOutConx( Operator, FALSE, PPX_DUPLEX );

                    /* Connect the caller and operator lines together */
                    VoiceConx(  Operator->hd, cx, TRUE );
                    SetState( cx->id, CTX_BUSY );
                    SetState( OPERATOR, CTX_BUSY );
                    bConnected = TRUE;
                }
                else
                {
                    /*
                     * Put or keep this caller on Hold until the
                     * Operator becomes available.
                     */

                    if( KeepOnHold( cx ) != SUCCESS )
                        goto hangup ;
                }
            } while ( GetState ( cx->id ) != CTX_BUSY );
        }


        /* Wait until call is complete, i.e., someone hangs up */
        for (;;)
        {
            ctaret = ctaWaitEvent( cx->ctaqueuehd, &event,
                                   CTA_WAIT_FOREVER);
            if (ctaret != SUCCESS)
            {
                printf( "\007ctaWaitEvent returned %x\n", ctaret);
                goto hangup;
            }
            switch ( event.id )
            {
                case ADIEVN_CALL_DISCONNECTED:

                    /* Disconnect voice path */
                    VoiceConx(  Operator->hd, cx, FALSE );
                    goto hangup;

                default:

                    break;
            }
        }

        /**********
         * HANGUP *
         *********/
hangup:

        DemoHangUp( cx->hd );
        DemoStopProtocol( cx->hd );

        if  (port_id != OPERATOR)
        {
            if (GetState ( port_id ) != CTX_ONHOLD)
            {
                if (bConnected)
                {
                    /* Re-Connect line to DSP */
                    SetupInOutConx( cx, TRUE, PPX_DUPLEX );

                    /* Re-connect operator line from its DSP */
                    SetupInOutConx( Operator, TRUE, PPX_DUPLEX );

                    bConnected = FALSE;

                    SetState( port_id, CTX_IDLE );
                }
            }
            else /* on hold */
            {
                TakeOffHold( cx ) ;

                /* Re-Connect line to DSP */
                SetupInOutConx( cx, TRUE, PPX_DUPLEX );

                SetState( port_id, CTX_IDLE );
            }
        }
        else  /* operator */
        {
            SetState ( OPERATOR, CTX_HUNGUP );
            /* Disconnect voice path */
            VoiceConx(  Operator->hd, &MyContext[ Operator->conn_id ], FALSE );

            /* Wait for other side to hangup          */
            /* before resetting this port connection. */
            while ( GetState ( Operator->conn_id ) == CTX_BUSY )
            {
                DemoSleep ( 1 );
            }

            SetState ( OPERATOR, CTX_IDLE );
        }
    }
    UNREACHED_STATEMENT( ppxCloseConnection( ppxhd[HOLD_MSG] ); )
    UNREACHED_STATEMENT( ctaDestroyQueue( cx->ctaqueuehd ); )
    UNREACHED_STATEMENT( THREAD_RETURN; )
}


/*---------------------------------------------------------------------------
 * main
 *--------------------------------------------------------------------------*/
int main( int argc, char **argv )
{
    MYCONTEXT *cx;
    unsigned   i;

    ProcessArgs( argc, argv );

    printf( "Natural Access Point-to-Point Switching Demo V.%s (%s)\n",
            VERSION_NUM, VERSION_DATE );

    printf( "\tBoard for incoming  call (customers)    = %d\n", in_board );
    printf( "\tBoard for outgoing call (operator)      = %d\n", out_board );
    printf( "\tPPX switch number for incoming call     = %d\n", in_swtch );
    printf( "\tPPX switch number for outgoing call     = %d\n", out_swtch );
    printf( "\tOutgoing (Operator) TimeSlot            = %d\n",
            outgoing_slot);
    printf( "\tNumber of incoming lines (Customers)    = %d\n",
            nListeners );

    cx = &(MyContext[LISTENER]);
    for ( i = 0 ; i < nListeners ; i++, cx++)
    {
        printf("\tIncoming (Customer) Timeslot            = %d\n",
               cx->time_slot) ;
    }
    printf( "\tIncoming (Customer) Protocol            = %s\n",
            incoming_protocol );
    printf( "\tOutgoing (Operator) Protocol            = %s\n",
            outgoing_protocol );

    printf( "\n" );


    /* Register an error handler to display errors returned from
     * API functions; no need to check return values in the demo.
     */
    ctaSetErrorHandler( DemoErrorHandler, NULL );

    /* initialize Natural Access services for this process */
    {
        CTA_SERVICE_NAME servicelist[] =     /* for ctaInitialize */
        {   { "ADI", ""       },
            { "VCE", "VCEMGR" },
            { "PPX", "PPXMGR" }
        };

        /* Initialize ADI Service Manager name */
        servicelist[0].svcmgrname = DemoGetADIManager();

        DemoInitialize( servicelist, ARRAY_DIM(servicelist) );
    }

    /*
     * Demonstate the use of a Connection block with the
     * addition and removal of listening ports.
     */

    FinalInit() ;

    for ( i=0; i< (nListeners + 2) ; i++ )
    {
        if (DemoSpawnThread( RunDemo, (void *)(i) ) == FAILURE)
        {
            printf("Could not create thread %d\n", i);
            exit(-1);
        }
    }

    /* Sleep forever. */
    for(;;)
        DemoSleep( 1000 );

    UNREACHED_STATEMENT( return 0; )
}


