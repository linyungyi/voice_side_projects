/*****************************************************************************
 *  FILE            : Timer.cpp
 *  PROJECT         : isdndemo
 *  DESCRIPTION     : implementation of Timer class
 *  
 *   From the user point of view Timer has 2 states:
 *
 *     STOPPED   - entered by stop() or event()
 *     RUNNING   - entered by start()
 *
 *   event() method process value of ADIEVN_TIMER_DONE event and returns true
 *   if Timer expired.
 *
 *   Internally Timer has a state machine with 4 states:
 *
 *     ST_STOP      - STOPPED
 *     ST_STOPPING  - STOPPED but waiting for ADI timer stop confirmation
 *     ST_RUN       - RUNNING
 *     ST_STARTING  - RUNNING but waiting for ADI timer stop confirmation
 *
 *****************************************************************************
 * Copyright 2001 NMS Communications
 *****************************************************************************/
#include "isdndemo.h"

#include <stdio.h>
//-----------------------------------------------------------------------------
//              
//                 Timer::Timer ()
//
// Creates timer and puts it into ST_STOP state.
//-----------------------------------------------------------------------------
Timer::Timer(CTAHD actahd) : state(ST_STOP), ctahd(actahd)
{
}


//-----------------------------------------------------------------------------
//              
//                 Timer::start ()
//
// Starts timer for given period. May be called several times. 
// If timer is already RUNNING, its stopped and started again
//-----------------------------------------------------------------------------
void Timer::start( unsigned aperiod )
{
    switch(state)
    {
    case ST_STOP:
        if ( adiStartTimer(ctahd, aperiod, 1) != SUCCESS ) 
            error("Cannot start timer\n");
        
        state = ST_RUN;
        break;

    case ST_RUN:
        if ( adiStopTimer(ctahd) != SUCCESS ) break;

    case ST_STOPPING:
    case ST_STARTING:
        period = aperiod;
        state  = ST_STARTING;
        break;  
    }
}

//-----------------------------------------------------------------------------
//              
//                 Timer::stop ()
//
// Stop running timer
//-----------------------------------------------------------------------------
void Timer::stop()
{
    switch(state)
    {
    case ST_STOP:
        break;

    case ST_RUN:
        if ( adiStopTimer(ctahd) != SUCCESS )
            error("Cannot stop timer\n");

    case ST_STOPPING:
    case ST_STARTING:
        state  = ST_STOPPING;
        break;
    }
}

//-----------------------------------------------------------------------------
//              
//                 Timer::event ()
//
// Process timer event. Event is value field of ADIEVN_TIMER_DONE.
// 
// Returns "true" if timer expired, 
//-----------------------------------------------------------------------------
bool Timer::event(DWORD value)
{
    switch(state)
    {
    case ST_STOP:
        error("Enexpected Timer Event\n");
        break;
        
    case ST_RUN:
        state = ST_STOP;
        return ( value == CTA_REASON_FINISHED );

    case ST_STOPPING:
        state = ST_STOP;
        break;

    case ST_STARTING:
        if ( adiStartTimer(ctahd, period, 1) != SUCCESS )
            error("Cannot start timer\n");

        state  = ST_RUN;
        break;      
    }
    return false;
}
