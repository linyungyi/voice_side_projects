/*****************************************************************************
 *  FILE            : Context.cpp
 *  PROJECT         : isdndemo
 *  DESCRIPTION     : implementation of Context class
 *
 *   This is base class for all Contextes in the program. It implements
 *   singlethreaded Natural Access event processing 
 *
 *  NOTES: 
 *    "server_mode"  must be initialized before the first call of 
 *     any Context methods.
 *
 *
 *****************************************************************************
 * Copyright 2001 NMS Communications
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "isdndemo.h"
#include "ctademo.h"


//---------------------------------------------------------------------------
// Class Context static variables
//---------------------------------------------------------------------------
bool        Context::server_mode   = false;
Context     **Context::contexts    = new Context*[256];
unsigned    Context::count         = 0;

CTAQUEUEHD Context::qid;

//---------------------------------------------------------------------------
//
//          Context::init() 
// 
// Create common queue for all Contextes. Must be called before creation of
// any instance of Context
//---------------------------------------------------------------------------
void Context::init() 
{
    char *mgr_list[] = { "ADIMGR", "VCEMGR" };
    int mgr_count=2;
    ctaCreateQueue( mgr_list, mgr_count, &qid );
}

//-----------------------------------------------------------------------------
//              
//           Context::start()
//
// This virtual method is called just before entering event processing loop.
//-----------------------------------------------------------------------------
void Context::start() {}


//---------------------------------------------------------------------------
//
//          Context::Context() 
//
//  This constructor creates Natural Access context  for context and 
//  registers new context in contextes table
//---------------------------------------------------------------------------
Context::Context() 
{
    contexts[count] = this;
    index = count++;
    
    char name[256];

    if (server_mode)
        sprintf(name,"localhost/context%d", index); 
    else 
        sprintf(name,"context%d", index); 

    
    ctaCreateContext(qid, index, name, &ctahd);
}

//---------------------------------------------------------------------------
//
//          Context::eventLoop() 
//
//  This function reads events and process them with processEvent().
//---------------------------------------------------------------------------
void Context::eventLoop()
{
    for(unsigned i=0; i<count; i++) contexts[i]->start();

    CTA_EVENT event = {0};
    for(;;)
    {
        ctaWaitEvent ( qid, &event, CTA_WAIT_FOREVER );
        if (event.userid < count)   
            contexts[event.userid]->processEvent ( &event );
    }
}


//---------------------------------------------------------------------------
//
//          Context::sendEvent() 
//  
//  Sends Natural Access event (build by id and value) to the Context
//---------------------------------------------------------------------------
/*
void Context::sendEvent(DWORD id, DWORD value) const
{
    CTA_EVENT event = {0};

    event.id        = id;
    event.userid    = index;
    event.ctahd     = ctahd;
    event.value     = value;

}
*/

//--------------------------------------------------------------------------
//
//             Context::waitForEvent()
//
//   Wait for specific events. If timeout, exit. Returns value of event. 
//--------------------------------------------------------------------------
DWORD Context::waitForEvent(DWORD id) const
{
    CTA_EVENT cta_event = {0};
    for (;;)
    {  
        ctaWaitEvent(qid,&cta_event, ISDNDEMO_WAIT_TIMEOUT);
        if (cta_event.id == id) return cta_event.value;
        if (cta_event.id == CTAEVN_WAIT_TIMEOUT)
            error("Timeout");
    }   
    return 0;
}

