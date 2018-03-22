/*****************************************************************************
 *  FILE            : NAI.cpp
 *  PROJECT         : isdndemo
 *  DESCRIPTION     : implementation of NAI class
 *
 *  NAI object has no harware resources directely associated with it. It is
 *  glue between DChannel (receiving ISDN messages) and BChannel (Timer and
 *  voice plaing)
 *
 *  NAI has no state machine.
 *         
 *****************************************************************************
 * Copyright 2001 NMS Communications
 *****************************************************************************/

#include "isdndemo.h"

//----------------------------------------------------------------------------
//
//                  NAI::NAI()
//
//  Creates NAI object and all related Call and BChannel objects
//----------------------------------------------------------------------------
NAI::NAI(int anumber, DChannel *Dch, int brd, int atrunk, bool isdn)
{
    int i;
    int ch_count;

    //========================================================================
    // Initialize members
    //========================================================================
    trunk       = atrunk;
    d_channel   = Dch;
    number      = anumber;
    board       = new Board(brd);

    //========================================================================
    // Create Calls
    //========================================================================
    for (i=0; i< ACU_MX_CALLS; i++) calls[i] = new Call(this, i);

    //========================================================================
    // Create BChanneles  
    //========================================================================
    ch_count    = board->getMaxChannel();
    b_channel   = new BChannel *[ch_count];

    int d = isdn ? board->getDChannel() : 0; // index of D-channel

    for (int ch=1; ch <= ch_count; ch++) 
        if (ch != d) 
            b_channel[ch-1] = new BChannel( this, ch, d_channel->getNewPortMode());
        else
            b_channel[ch-1] = 0;
}

//----------------------------------------------------------------------------
//
//                  NAI::getFreeCall()
//
//  Returns the first free Call with the least connection ID.
//----------------------------------------------------------------------------
Call *NAI::getFreeCall() const
{
    for( int i=0; i<ACU_MX_CALLS; i++)
        if (calls[i]->isFree()) return calls[i];

    error("Cannot find free connection id for NAI %d\n",number);
    return 0;
}

//----------------------------------------------------------------------------
//
//                  NAI::getBChannel()
//
//  Returns B-channel object by its number. 0 if not found or incorrect.
//----------------------------------------------------------------------------
BChannel * NAI::getBChannel(int ch) const
{
    if ( ch> 0 && ch <= board->getMaxChannel() ) return b_channel[ch-1];

    error("Bad B-channel %d\n",ch);
    return 0;
}

//----------------------------------------------------------------------------
//
//                 NAI::getCall()
//
// Returns Call by its connection ID.
//----------------------------------------------------------------------------
Call *NAI::getCall(int conn_id) const
{
    if (conn_id>=0 && conn_id <ACU_MX_CALLS) return calls[conn_id];
    return 0;
}
