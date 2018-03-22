/*****************************************************************************
 *  FILE            : Board.cpp
 *  PROJECT         : isdndemo
 *  DESCRIPTION     : implementation of Board class
 *         
 *  Board class use its own Natural Access context to get board information. This 
 *  context is created by Board::init() method. This context is always 
 *  created in Library mode.
 *
 *****************************************************************************
 * Copyright 2001 NMS Communications
 *****************************************************************************/
 
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "isdndemo.h"

//-----------------------------------------------------------------------------
// Static members of Board class
//-----------------------------------------------------------------------------
CTAHD       Board::ctahd;
CTAQUEUEHD  Board::qid;

//-----------------------------------------------------------------------------
//              
//                 Board::Board ()
//
//  Construct Board object for specified board number.
//-----------------------------------------------------------------------------
Board::Board(int bnumber)
{

    number  = bnumber;
    hmobj   = 0;

    adiGetBoardInfo(ctahd, number, sizeof(ADI_BOARD_INFO),&board_info);
    switch( board_info.trunktype )
    {
       case ADI_TRUNKTYPE_T1:   type = PRI_T1; break;  
       case ADI_TRUNKTYPE_E1:   type = PRI_E1; break;
       case ADI_TRUNKTYPE_BRI:  type = BRI; break;
       default : 
           error ("Unsupported board type for this demo\n");
    }
}

//-------------------------------------------------------------------------------
//              
//                 Board::init()
//
//  Perform initialization of Board class. Create utility Natural Access context.
//  ctaInitialize() must be called befor this method. 
//-------------------------------------------------------------------------------
void Board::init()
{
  CTA_EVENT             cta_event = {0};
  CTA_SERVICE_DESC      svc_desc[]={
      { {"ADI" , "ADIMGR"},{0},{0},{0} },
      { {"OAM" , "OAMMGR"},{0},{0},{0} },
  };
        
  char *mgr_list[]  =  {"ADIMGR","OAMMGR"};
  
  for(int i=0;i<2;i++)
  {
    svc_desc[i].mvipaddr.board = ADI_AG_DRIVER_ONLY;
  }
  
  if (enableOAM)
  {
      ctaCreateQueue( mgr_list, 2, &qid );
      ctaCreateContext( qid, 0, "//localhost/board", &ctahd );
      ctaOpenServices( ctahd, svc_desc, 2 );
  }
  else
  {
      ctaCreateQueue( mgr_list, 1, &qid );
      ctaCreateContext( qid, 0, "board", &ctahd );
      ctaOpenServices( ctahd, svc_desc, 1 );
  }

  for (;;)
  {
      ctaWaitEvent(qid,&cta_event, ISDNDEMO_WAIT_TIMEOUT);
      if (cta_event.id == CTAEVN_OPEN_SERVICES_DONE) break;;
      if (cta_event.id == CTAEVN_WAIT_TIMEOUT)
          error("reading board configuration: wait timeout");
  } 

  if ( cta_event.value != CTA_REASON_FINISHED )
    error("configuration: services could not be opened\n");


}


//-----------------------------------------------------------------------------
//              
//                 Board::getTrunks ()
//
//  Returns number of trunks for the board
//-----------------------------------------------------------------------------
int Board::getTrunks () const
{
    return board_info.numtrunks;
}

//-----------------------------------------------------------------------------
//              
//                 Board::getMaxChannel ()
//
//  Returns maximal number of channels for the board trunk
//  Channals are numered from 1 to getMaxChannel()
//-----------------------------------------------------------------------------
int Board::getMaxChannel () const
{
    int ret = 0;

    switch ( type )
    {
    case PRI_T1 : ret = 24; break;
    case PRI_E1 : ret = 31; break;
    case BRI    : ret = 2;  break;
    }

    return ret;
}

//-----------------------------------------------------------------------------
//              
//                 Board::getMvipStream ()
//
//  Returns MVIP stream for the board DSP resources
//-----------------------------------------------------------------------------
int Board::getMvipStream () const 
{
    if (type == BRI) return 0;
    switch( board_info.numtrunks )
    {
       case 1  : return 0;  // AG1 ISA  BOARD ( not supported by OAM  )
       case 2  :
       case 4  : return 16;
       case 16 : return 64;    
    }

    return -1;
}

//-----------------------------------------------------------------------------
//              
//                 Board::getMvipSlot ()
//
//  Returns MVIP stream for the board DSP resources basing on trunk number 
//  and ISDN channel (1 .. ) 
//-----------------------------------------------------------------------------
int Board::getMvipSlot(int trunk, int channel) const
{
    int ret = 0;

    switch ( type )
    {
    case PRI_E1: 
        // 16 is incorrect value for E1
        if (channel == 16) error("Board::getMvipSlot(): E1 b-channel 16\n");
        if (channel >= 16) channel--;
        ret = 30*trunk + (channel-1);
        break;

    case PRI_T1:        
        ret = 24*trunk + (channel-1);
        break;

    case BRI:
        ret = 2*trunk + (channel-1);
        break;
    }

    return ret;
}


//-----------------------------------------------------------------------------
//              
//                 Board::getBChannel ()
//
//  Returns D-channel channel number. 0 means no D-channel
//-----------------------------------------------------------------------------

int Board::getDChannel() const
{
    int ret = 0;

    switch ( type )
    {
    case PRI_T1 : ret = 24; break;
    case PRI_E1 : ret = 16; break;
    case BRI    : ret = 0;  break;
    }
    
    return ret;
}

//-----------------------------------------------------------------------------
//              
//                 Board::getKeyword()
//
// Reads OAM keyword for the board and returns it numeric value. It uses 
// printf like format to form keyword name.
//-----------------------------------------------------------------------------
int Board::getKeyword( const char *format, ...) const
{
    char key_name[256];
    char key_value[256];
    va_list args;

    if (hmobj == 0)
    {
        oamBoardLookupByNumber(ctahd, number, key_name, sizeof(key_name));
        oamOpenObject(ctahd, key_name, const_cast<HMOBJ *>(&hmobj), OAM_READONLY);
    }

    va_start( args, format );
    vsprintf( key_name, format, args ); 
    oamGetKeyword( hmobj, key_name, key_value, sizeof( key_value ) );
    
    va_end( args );
    
    return atoi( key_value );
}
