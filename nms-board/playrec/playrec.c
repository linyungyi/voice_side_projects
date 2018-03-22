/****************************************************************************
 *
 *  playrec.c
 *
 *  Example code demonstrating play and record using:
 *      - asynchronous buffer submission
 *      - callbacks
 *
 *  The demo is written to show, in source code form, how to do
 *  play and record with the two methods.  The code does not include
 *  any call control.  To execute this demo, and actually operate it,
 *  you must do one of the following to the demo:
 *    - add call-control (refer to inadi, outadi, etc.),
 *    - answer an incoming loop-start line with the adiAssertSignal function,
 *    - connect a phone directly to an operator workstation or DID hybrid,
 *    - use switching to make a connection.
 *
 * Copyright (c) 1999-2002 NMS Communications.  All rights reserved.
 ****************************************************************************/

/*----------------------------- Includes -----------------------------------*/
#include "ctademo.h"

/*----------------------------- Application Definitions --------------------*/
#if defined (UNIX)
  #define TEMP_FILE "/tmp/temp.vce"
#else
  #define TEMP_FILE "temp.vce"
#endif


typedef struct
{
    CTAQUEUEHD  ctaqueuehd;       /* Natural Access queue handle                 */
    CTAHD       ctahd;            /* Natural Access context handle               */
    unsigned    lowlevelflag;     /* flag to get low-level events           */
} MYCONTEXT;

static MYCONTEXT MyContext =
{
    NULL_CTAQUEUEHD, /* Natural Access queue handle   */
    NULL_CTAHD,      /* Natural Access context handle */
    0                /* lowlevelflag             */
};

MYCONTEXT *cx = &MyContext;


/*----------------------------- Application Data ---------------------------*/
FILE         *voice_fp;                /* FILE for voice data               */
void         *voice_buffer1;           /* malloc'd buffer for voice data    */
void         *voice_buffer2;           /* malloc'd buffer for voice data    */

unsigned   maxrecordtime = 20*1000; /* in ms */
unsigned   buffersize    = 0;
unsigned   ag_board      = 0;
unsigned   mvip_stream   = 0;
unsigned   mvip_slot     = 0;


/***************************
 *
 *  Forward References
 *
 ***************************/

int ExamplePlayAsync( char *filename, unsigned encoding );
int ExampleRecordAsync( char *filename, unsigned encoding,
                        unsigned maxrecordtime );

int ExamplePlayWithCallback( char *filename, unsigned encoding );
int ExampleRecordWithCallback( char *filename, unsigned encoding,
                               unsigned maxrecordtime );

int NMSSTDCALL ExamplePlayCallback( void *accarg, void *buffer,
                                    unsigned size, unsigned *rsize );
int NMSSTDCALL ExampleRecordCallback( void *accarg, void *buffer,
                                      unsigned size );

void CmdLineOptions( int argc, char *argv[] );


/*******************************************************
 *
 *  ExampleInstructions
 *
 *******************************************************/
void ExampleInstructions( void )
{
    puts( "The 'playrec' program demonstrates the recording and playback" );
    puts( "of speech using both:" );
    puts( "  - asynchronous buffer submission" );
    puts( "  - callbacks" );

    puts( "The demo is written to show, in source code form, how to do");
    puts( "play and record with the two methods.  The code does not include");
    puts( "any call control.  To execute this demo, and actually operate it,");
    puts( "you must do one of the following to the demo:");
    puts( "  - add call-control (refer to inadi, outadi, etc.),");
    puts( "  - answer an incoming loop-start line with the adiAssertSignal function,");
    puts( "  - connect a phone directly to an operator workstation or DID hybrid,");
    puts( "  - use switching to make a connection.");
    puts( "" );
    puts( "See the source code for further information. If you have done one" );
    puts( "of these and want to continue, hit enter. Otherwise hit CTL-C." );
    getchar();
}


/*******************************************************
 *
 *  main
 *
 *******************************************************/

int main( int argc, char *argv[] )
{
    DWORD e = SUCCESS;


    CmdLineOptions( argc, argv );

/*    ExampleInstructions();*/

    DemoSetup( ag_board, mvip_stream, mvip_slot,
                  &cx->ctaqueuehd,
                  &cx->ctahd);

    ctaSetErrorHandler( DemoErrorHandler, NULL );

    DemoSetReportLevel( DEMO_REPORT_COMMENTS );


    DemoStartProtocol( cx->ctahd, "nocc", NULL, NULL );

    /*
     *  Play and record using asynchronous buffer submissions.
     */

    puts( "Prompting with asynchronous play..." );
    ExamplePlayAsync( "async.vce", ADI_ENCODE_NMS_24 );

    puts( "Recording asynchronously..." );
    ExampleRecordAsync( TEMP_FILE, ADI_ENCODE_NMS_24, maxrecordtime );

    puts( "Playing back asynchronously..." );
    ExamplePlayAsync( TEMP_FILE, ADI_ENCODE_NMS_24 );

    /*
     *  Play and record using callbacks for data transfer.
     */

    puts( "Prompting with callback play..." );
    ExamplePlayWithCallback( "callback.vce", ADI_ENCODE_NMS_24 );

    puts( "Recording with callback..." );
    ExampleRecordWithCallback( TEMP_FILE, ADI_ENCODE_NMS_24, maxrecordtime );

    puts( "Playing back with callback..." );
    ExamplePlayWithCallback( TEMP_FILE, ADI_ENCODE_NMS_24 );

    return 0;
}


/*****************************************************
 *
 *  ExamplePlayAsync - Demonstrates the ADI play
 *        async function.
 *
 *****************************************************/

int ExamplePlayAsync( char *filename, unsigned encoding )
{
    unsigned  flags;
    unsigned  rsize;
    CTA_EVENT cta_event;

    adiFlushDigitQueue( cx->ctahd );

    if ( buffersize == 0 )
    {
        /*  buffersize == 0 => use default.
         *  Find the AG buffer size to determine the
         *  optimal buffer to malloc.
         */

        adiGetEncodingInfo( cx->ctahd, ADI_ENCODE_NMS_24, NULL, NULL,
                           &buffersize );
    }

    if ( ( voice_buffer1 = malloc( buffersize ) ) == NULL )
    {
        printf( "failed malloc of %d bytes\n", buffersize );
        exit( -1 );
    }

    if ( ( voice_fp = fopen( filename, "rb" ) ) == NULL )
    {
        printf ("%s failed open\n", filename);
        perror("fopen");
        exit( -1 );
    }

    rsize = fread( voice_buffer1, 1, buffersize, voice_fp );

    if ( ferror( voice_fp ) )
    {
        perror("fread");
        exit( -1 );
    }

    /*
     *  If this is the only buffer (feof() != 0), then
     *  indicate so in flags.
     */
    flags = feof( voice_fp ) ? ADI_PLAY_LAST_BUFFER : 0;

    /*
     *  Start playing and submit 1st buffer.  The event
     *  handler will read and submit any subsequent
     *  buffers.
     */

    adiPlayAsync( cx->ctahd, encoding, voice_buffer1, rsize, flags, NULL );

    do
    {
        DemoWaitForEvent( cx->ctahd, &cta_event );

        if( cta_event.id == ADIEVN_PLAY_BUFFER_REQ )
        {
            /*
             *  Read next voice buffer and submit it.
             *  This is a trivial example of
             *  SubmitPlayBuffer.  If you had a disk or
             *  network server process, it might write to
             *  a FIFO which would be read here.
             */

            rsize = fread( voice_buffer1, 1, buffersize, voice_fp );

            flags = feof( voice_fp ) ? ADI_PLAY_LAST_BUFFER : 0;

            adiSubmitPlayBuffer( cx->ctahd, voice_buffer1, rsize, flags );
        }
    } while( cta_event.id != ADIEVN_PLAY_DONE );

    free( voice_buffer1 );
    fclose( voice_fp );

    voice_buffer1 = voice_fp = NULL;

    return SUCCESS;
}


/*****************************************************
 *
 *  ExampleRecordAsync - Demonstrates the ADI record
 *        async function.
 *
 *****************************************************/

int ExampleRecordAsync( char *filename, unsigned encoding,
                        unsigned maxrecordtime )
{
    CTA_EVENT cta_event;

    adiFlushDigitQueue( cx->ctahd );

    if( buffersize == 0 )
    {
        /*
         *  buffersize == 0 => use default.
         *  Find the AG buffer size to determine the optimal
         *  buffer to malloc.
         */

        adiGetEncodingInfo( cx->ctahd, ADI_ENCODE_NMS_24, NULL, NULL, &buffersize );
    }

    /*
     *  Malloc 2 buffers.  Recording implements a
     *  double-buffering scheme.
     */

    if ( ( voice_buffer1 = malloc( buffersize ) ) == NULL )
    {
        printf( "failed malloc of %d bytes\n", buffersize );
        exit( -1 );
    }

    if ( ( voice_buffer2 = malloc( buffersize ) ) == NULL )
    {
        printf( "failed malloc of %d bytes\n", buffersize );
        exit( -1 );
    }

    if ( ( voice_fp = fopen( filename, "wb" ) ) == NULL )
    {
        printf( "%s not found\n", filename );
        perror( "fopen" );
        exit( -1 );
    }

    /*
     *  Start recording and submit 1st buffer.  The event
     *  handler will write and submit any subsequent
     *  buffers.
     */

    adiRecordAsync( cx->ctahd, encoding, maxrecordtime, voice_buffer1,
                    buffersize, NULL );

    do
    {
        DemoWaitForEvent( cx->ctahd, &cta_event );

        switch ( cta_event.id )
        {
            case ADIEVN_RECORD_BUFFER_FULL:
                /*
                 *  Write filled voice buffer to disk and submit
                 *  new empty one.  This is also a trivial example
                 *  of asynchronous play/record (see comment
                 *  above).
                 */

                fwrite( cta_event.buffer, 1, cta_event.size, voice_fp );

                if ( cta_event.value & ADI_RECORD_BUFFER_REQ )
                {
                    adiSubmitRecordBuffer( cx->ctahd, cta_event.buffer,
                                           cta_event.size );
                }
                break;


            case ADIEVN_RECORD_STARTED:
                /*
                 *  When ADI initiates recording on the AG
                 *  board, it generates a "record started"
                 *  event.  Use this event as stimulus to
                 *  submit the second buffer of the
                 *  double-buffering scheme.
                 */

                if ( cta_event.value & ADI_RECORD_BUFFER_REQ )
                {
                    adiSubmitRecordBuffer( cx->ctahd, voice_buffer2,
                                           buffersize );
                }
                break;
        }
    } while ( cta_event.id != ADIEVN_RECORD_DONE );

    free( voice_buffer1 );
    free( voice_buffer2 );
    fclose( voice_fp );

    voice_buffer1 = voice_buffer2 = voice_fp = NULL;

    return SUCCESS;
}


/******************************************************
 *
 *  ExamplePlayWithCallback - Demonstrates the ADI play
 *        function callbacks.
 *
 ******************************************************/

int ExamplePlayWithCallback( char *filename, unsigned encoding )
{
    CTA_EVENT cta_event;

    voice_fp = fopen( filename, "rb" );
    if( voice_fp == NULL )
    {
        printf( "%s not found\n", filename );
        perror( "fopen" );
        exit( -1 );
    }

    adiFlushDigitQueue( cx->ctahd );

    /*
     *  Start playing in callback mode passing the address
     *  of our callback.  The File Pointer is used as the
     *  callback argument.
     */

    adiStartPlaying( cx->ctahd, encoding, ExamplePlayCallback,
                     (void *)voice_fp, NULL );

    DemoWaitForSpecificEvent( cx->ctahd, ADIEVN_PLAY_DONE, &cta_event );

    fclose( voice_fp );
    voice_fp = NULL;

    return SUCCESS;
}


/*****************************************************
 *
 *  ExamplePlayCallback - Invoked by ADI whenever more
 *       voice data is needed for current play function.
 *
 *****************************************************/

int NMSSTDCALL ExamplePlayCallback( void *accarg, void *buffer,
                                       unsigned size, unsigned *rsize )
{
    FILE *fp   = accarg;

    printf( "\tExamplePlayCallback() called.\n" );

    *rsize = fread( buffer, 1, size, fp );

    if ( ferror( fp ) )
        return (-1);

    return feof( fp ) ? ADI_PLAY_LAST_BUFFER : SUCCESS;
}


/******************************************************
 *
 *  ExampleRecordWithCallback - Demonstrates the ADI
 *        record using callbacks for data transfer.
 *
 ******************************************************/

int ExampleRecordWithCallback( char *filename, unsigned encoding,
                               unsigned maxrecordtime )
{
    CTA_EVENT cta_event;

    if ( (voice_fp = fopen( filename, "wb" )) == NULL )
    {
        printf( "%s failed open\n", filename );
        perror( "fopen" );
        exit( -1 );
    }

    adiFlushDigitQueue( cx->ctahd );

    /*
     *  Start recording in callback mode passing the
     *  address of our callback.  The File Pointer
     *  is used as the callback argument.
     */

    adiStartRecording( cx->ctahd, encoding, maxrecordtime,
                       ExampleRecordCallback, (void *)voice_fp, NULL );

    DemoWaitForSpecificEvent( cx->ctahd, ADIEVN_RECORD_DONE, &cta_event );

    fclose( voice_fp );

    return SUCCESS;
}


/*******************************************************
 *
 *  ExampleRecordCallback - Invoked by ADI whenever a
 *       record voice buffer is full for the current
 *       record function.
 *
 *******************************************************/

int NMSSTDCALL ExampleRecordCallback( void *accarg, void *buffer,
                                         unsigned size )
{
    DemoReportComment( "ExampleRecordCallback() called." );

    fwrite( buffer, 1, size, (FILE*) accarg );
    return ferror( (FILE*) accarg ) ? -1 : 0;
}


/*******************************************************
 *
 *  CmdLineOptions
 *
 *******************************************************/

void CmdLineOptions(int argc, char **argv)
{
    int  c;

    while( ( c = getopt( argc, argv, "A:F:b:s:qw:d:p:z:r:t:Hh?" ) ) != -1 )
    {
        switch( c )
        {
                unsigned   value1, value2;

            case 'A':
                DemoSetADIManager( optarg );
                break;

            case 'F':
                DemoSetCfgFile( optarg );
                break;

            case 'b':
                ag_board = (unsigned) atoi(optarg);
                printf("AG/QX board=%d\n", ag_board);
                break;

            case 's':
                switch (sscanf( optarg, "%d:%d", &value1, &value2 ))
                {
                    case 2:
                        mvip_stream = value1;
                        mvip_slot   = value2;
                        break;
                    case 1:
                        mvip_slot   = value1;
                        break;
                    default:
                        goto prog_help;
                }
                printf( "MVIP stream=%d, slot=%d\n", mvip_stream, mvip_slot );
                break;

            case 'z':
                buffersize = (unsigned) atoi(optarg);
                printf( "buffer size=%d\n", buffersize );
                break;

            case 'r':
                maxrecordtime = (unsigned) atoi(optarg)*1000;
                printf( "MAX record time=%d (sec)\n", maxrecordtime/1000 );
                break;

            case 'h':
            case 'H':
            case '?':
                goto prog_help;

            default:
                goto prog_help;
        }
    }
    if (optind < argc)
        goto prog_help;

    return;

prog_help:
    printf( "\nUsage: "
            "%s [-A adi_mgr] [-b board] [-s n:m] [-z bytes] [-r seconds] [-F filename]\n",
            argv[0]);

    printf( "  -A adi_mgr  Natural Access ADI manager to use.            "
            "Default = %s\n", DemoGetADIManager() );

    printf( "  -b board    OAM board with DSP.                "
            "Default=%d\n", ag_board );

    printf( "  -s [n:]m    DSP [stream and] timeslot.         "
            "Default=%d:%d\n", mvip_stream, mvip_slot );

    printf( "  -z bytes    play/record buffer size.           "
            "Default=port dependent\n" );

    printf( "  -r seconds  Max record time.                   "
            "Default=%d\n", maxrecordtime/1000 );

    printf( "  -F filename Natural Access Configuration File name. "
            "Default = None\n" );


    printf("\n");
    exit( 1 );
}
