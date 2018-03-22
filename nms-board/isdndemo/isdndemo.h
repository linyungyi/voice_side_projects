/*****************************************************************************
 *  FILE            : isdndemo.h
 *  PROJECT         : isdndemo
 *  DESCRIPTION     : classes and common functions declaration
 *         
 *****************************************************************************
 * Copyright 2001 NMS Communications
 *****************************************************************************/

#ifndef ISDNDEMO_INCLUDED
#define ISDNDEMO_INCLUDED

#include <stdio.h> 

// CTA, VCE and OAM include files 

#include "adidef.h"
#include "ctadef.h"
#include "vcedef.h"
#include "oamdef.h"

// ISDN include files 

#include "isdndef.h"
#include "isdntype.h"
#include "isdnacu.h"
#include "isdnval.h"
#include "isdnparm.h"

#include <stdarg.h>


//============================================================================
//
//                  Global variables
//
//============================================================================
extern bool enableOAM;              // Enable OAM service use 

//============================================================================
//
//                  Program constants
//
//============================================================================
const int MAX_NAIS      = 20;       // Max number of nais in NFAS group
const int ISDNDEMO_WAIT_TIMEOUT  = 10000;   // 10 seconds  

// Program output control:
const int V_CONFIG      = 0x01;     // Program configuration
const int V_CALL        = 0x02;     // Call status messages
const int V_ACU         = 0x04;     // ACU messages
const int V_STAT        = 0x08;     // Statistics
    
//============================================================================
//
//                  Printing functions 
//
//============================================================================
void error (const char *, ...);
void error_cta (unsigned code, const char *, ...);

void debug_printf (int, const char *, ...);
void debug_vprintf(int , const char *format, va_list );


//============================================================================
//
//                  ACU constant to string converting functions 
//
//============================================================================
const char *getACU(int);
const char *getACURC(int);
const char *getACUERR(int);
const char *getCAU(int);


/*****************************************************************************
 
  
 
 *****************************************************************************/
class   NAI;
class   BChannel;
class   DChannel;
class   Board;
class   Timer;
class   Call;

//============================================================================
//
//                  CLASS  Timer 
//
//============================================================================
// This class implements timer based on adiStartTimer() and adiStopTimer() 
// calls. The timer is synchronous.
//
// Reason for this class is the fact that ADI timer is asynchronous.
// You can't stop it just by functionall call of adiStopTimer().
//============================================================================
// Used by classes   : BChannel
//============================================================================
class Timer 
{
    enum { ST_STOP, ST_RUN, ST_STOPPING, ST_STARTING} state;
    CTAHD ctahd;
    unsigned period;

public:
    Timer(CTAHD actahd);

    void start(unsigned aperiod);
    void stop();
    bool event(DWORD value);
};

//============================================================================
//
//                  CLASS  Board 
//
//============================================================================
// This is NMS board (AG, CG series) abstraction from ISDN point of view
// Used to get board specific information.
//============================================================================
// Used by classes   :  BChannel, DChannel, NAI
//============================================================================
class Board 
{
    ADI_BOARD_INFO      board_info; // Internal board information
    HMOBJ               hmobj;      // Object to read OAM keywords
    static CTAHD        ctahd;      // Used to get goard information
    static CTAQUEUEHD   qid;        // Used for creation ctahd

public:
    int number;                     // NMS board number

    static void init();             // Initialize Board class
    Board(int num);                 // Creates board object by number

    enum {PRI_E1, PRI_T1, BRI} type;  // Board type
    //
    // Methods to get board information
    //
    int getMvipStream() const;
    int getMvipSlot  (int trunk, int channel) const;
    int getDChannel() const;
    int getTrunks () const;
    int getMaxChannel() const;
    int getKeyword(const char *, ...) const;

};


//============================================================================
//
//                  CLASS  Context 
//
//============================================================================
// Context object is capable to process Natural Access events.
//============================================================================
// Used by classes   : 
//============================================================================
class Context
{
    static Context **contexts;      // List of all Context objects
    static unsigned count;          // Number of all Context objects
    int index;                      // index of Context

protected:
//  char *title;                    // context text description
    CTAHD ctahd;                    // Natural Access Context
    static CTAQUEUEHD qid;

    Context( ); 
 
    //
    // This functions must be defined in derived classes to
    // provide event processing.
    //
    virtual void processEvent( CTA_EVENT *event ) = 0;
    virtual void start();

public:

//  void sendEvent(DWORD id, DWORD value) const;

    DWORD waitForEvent(DWORD id) const;

    static void eventLoop();

    //
    // This members are used for class configuration. 
    //
    static bool server_mode;
    static void init();

};

//============================================================================
//
//                  STRUCT Config
//
//============================================================================
// This structure contains parameters for creating objects for one D-Channel
//============================================================================
struct  Config
{
    int     nai;                // NAI number to start protocol on
    int     board;              // Board number to start protocol on
    int     group;              // Number of NFAS group or -1

    bool    Q931;               // ACU_SEND_Q931_BUFFER
    bool    Transparent;        // NS_IE_RELAY_BEHAVIOUR
    bool    network;            // true - NT side,  false - TE side            
    int     protocol;           // NMS Operator
    bool    voice;              // play voice
    bool    uui_flag;           // Send User-to-User information
    bool    mlpp_flag;          // Send MLPP Precedence Level in SETUP
    bool    exclusive;          // Set outbound call mode to "Exclusive" vs. "Preferred"  
    char    dial_string[32];    // String to dial
    char    calling_string[32]; // Calling party number
    int     intercall;          // Delay between calls
    int     hold_time;          // Length of the call
    int     first_outbound_channel;
    int     number_outbound_ports;
    uchar   clear_cause;        // Context call clearing cause
    
    bool    log;                // Logging enabled
    int     logtime;            // Loging time is sec

    Config();
    Config *copy();

};

//============================================================================
//
//                  CLASS  BChannel 
//
//============================================================================
// This kind of Contextes is used to access voice playing and timer functions.
//============================================================================
// Used by classes   : CALL, NAI
//============================================================================
class BChannel : public Context
{
    //
    // Context class implementation
    //
    virtual void processEvent( CTA_EVENT *event );
    virtual void start();

    //
    // Utility functions
    //
    void printInfo(const char *format, ...);

    //
    // This members hold Context configuration
    //
    int         number;         // B-channel number
    const NAI   *nai;           // NAI for Context
    Config      *config;        // configuration parameters
    bool        outbound_flag;  // do outbound calls (behavior flag)
    VCEHD       vcehd;          // voice handler

    //
    // This members hold object state indication
    //
    Timer       timer;          // ADI timer
    Call        *call;          // associated Call

    //
    // Some context statistics
    //
    int calls_in;               // number of inbound calls for channel
    int calls_out;              // number of inbound calls for channel
    int statistics;             

    //
    // Internal object state
    //
    enum { ST_FREE, ST_USED, ST_CALL } state;
    bool    playing;            // indicates playing 


public:

    BChannel( NAI *anai, int achannel, bool flag);

    inline int getNumber() { return number;} 

    void get(Call *acall);      // Call enter connected state
    void free();                // Call exit from connected state
    void startCall();
    void stopCall();

    int getStatistics();

};

//============================================================================
//
//                  CLASS  Call 
//
//============================================================================
//============================================================================
// Base classes      : no
// Children classes  : no
// Used by classes   : BChannel, NAI
//============================================================================
class Call
{
    const NAI   *nai;           // NAI associated with the call

    //
    // Implementation of ISDN state machine
    //
    enum { 
        ST_NULL,
        ST_ACTIVE,
        ST_AWAITING_CONNECT,
        ST_AWAITING_CLEARANCE
    } state;                    // AG-ISDN Messaging API states

    void unexpIsdnMessage(char msg);

public:
    int         conn_id;        // call connection ID (unique for NAI)
    bool        outbound;       // the call is outbound 
    BChannel    *channel;       // B-channel associated with the call

    Call(NAI *anai, int id);

    bool isFree() const;
    void freeResources(BChannel *);
    void setBChannel(int);  

    //
    // Methods affecting call state
    //
    void processIsdnMessage( ISDN_PACKET * );
    void hangUp();              // hang up existing call
    void makeCall(int ch);      // make new call

};


//============================================================================
//
//                  CLASS  DChannel 
//
//============================================================================
// This kind of Contextes implements distributer.
// It received ISDN messages and transfer them to apropriate BChannel
//
// Each object of this class has associated NAI objects (one for each
// nai supported by D-channel).
//============================================================================
// Base classes      : Context
// Children classes  : no
// Used by classes   : NAI, Call
//============================================================================
class DChannel : public Context
{
    int     group;                  // NFAS group number
    int     trunk;
    const Board *board;
    const NAI       *nai[MAX_NAIS];         // List of NAIs for D-channel
    
    int     open_ports;
    int     open_outbound_ports;

    virtual void processEvent( CTA_EVENT *event );
    void printIsdnMessage( ISDN_MESSAGE *, void *, bool);

    void logprintf (int, const char *, ...);

    FILE *logfile;
    int stat[4], stat_old[4];
    time_t starttime;
public:
    Config *config;

    int number;

    DChannel( Config *cfg );

    int sendIsdnMessage(const NAI *anai, char code, int conn_id, void *msg_buffer,
                        int length);

    bool getNewPortMode();
    void startProtocol(int anai);

    enum {
        INBOUND,        // inbound call
        OUTBOUND,       // outbound call
        FAIL,           // outbound call failed 
        GLARE           // glare condition detected
    };

    void statistics(int);
};

//============================================================================
//
//                  CLASS  NAI 
//
//============================================================================
//
// This class present NAI abstaraction. It is associated with specific trunk
// of specific board. It has BChannel objects for each B-channel of this trunk.
// It also have Call objects associated with nai number
//  
//============================================================================
// Used by classes   : DChannel, BChannel, Call
//============================================================================
class NAI
{
    Call        *calls[ACU_MX_CALLS];
    BChannel    **b_channel;    // List of associated BChannels 


public:
    int         number;         // nai number
    int         trunk;          // trunk number
    const Board *board;         // board
    DChannel    *d_channel;     // DChannel object for D-channel


    NAI(int number, DChannel *Dch, int brd, int atrunk, bool isdn);

    BChannel    *getBChannel(int ch) const;
    Call        *getCall(int conn_id) const;
    Call        *getFreeCall() const;
};

#endif //ISDNDEMO_INCLUDED

