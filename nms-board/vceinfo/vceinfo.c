/***************************************************************************
*  File - vceinfo.c
*
*  Description - utility to display information about a voice file, including
*       the size of each message.
*
* 1996-2001  NMS Communications.
***************************************************************************/
#define REVISION_NUM  "5"
#define REVISION_DATE __DATE__
#define VERSION_NUM   "1"

/*----------------------------- Includes -----------------------------------*/
#include <stdio.h>
#include "ctademo.h"
#include "ctadef.h"
#include "vcedef.h"

/*----------------------------- Forward References ------------------------*/
void  RunDemo( unsigned filecount, char **filename );
void  ShowFile( CTAHD ctahd, char *filename );
void  Setup( CTAQUEUEHD *ctaqhd, CTAHD *ctahd );

/*----------------------------------------------------------------------------
                            options
----------------------------------------------------------------------------*/
unsigned Encoding      = 0;
unsigned Filetype      = 0;
unsigned NoMessageInfo = 0;

/*----------------------------------------------------------------------------
                            PrintHelp()
----------------------------------------------------------------------------*/
void PrintHelp( void )
{
    printf( "\nUsage: vceinfo [opts] {filename ...}\n" );
    printf( "  where opts are:\n" );
    printf( "  -e n        Encoding, Default = %d %s\n", Encoding,
            (Encoding<DemoNumVoiceEncodings)?DemoVoiceEncodings[Encoding]:"" );
    printf( "  -e ?        Display encoding choices\n");
    printf( "  -f type     File type: VOX, WAV or VCE. Default=infer from "
            "filename\n");
    printf( "  -n          No message information\n");
    puts( "" );
    return ;
}

/*----------------------------------------------------------------------------
                                main
----------------------------------------------------------------------------*/
int main( int argc, char **argv )
{
    int c;
    char     **filename;        /* Ptr to list of filenames in argv */
    unsigned   filecount;

    printf( "Natural Access Voice File Info Utility V %s.%s (%s)\n",
            VERSION_NUM, REVISION_NUM, REVISION_DATE );

    while( ( c = getopt( argc, argv, "ne:f:Hh?" ) ) != -1 )
    {
        switch( c )
        {
            case 'n':
                NoMessageInfo = 1;
                break;

            case 'f':
                Filetype = DemoGetFileType (optarg);
                break;

            case 'e':
                if ( *optarg != '?' )
                    sscanf( optarg, "%d", &Encoding );
                else
                {
                    unsigned i ;
                    for (i=1; i<DemoNumVoiceEncodings; i++)
                        if (*DemoVoiceEncodings[i] != '\0')
                            printf( " %d %s\n", i, DemoVoiceEncodings[i] );
                    exit( 0 );
                }
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
    if ( optind == argc )
    {
        printf( "\nFilename required!\n" );
        PrintHelp();
        exit( 1 );
    }
    filename  = &argv[optind];     /* pointer to array of filenames */
    filecount = argc-optind;

    RunDemo( filecount, filename );
    return 0;
}


/*-------------------------------------------------------------------------
                            ErrorHandler
-------------------------------------------------------------------------*/
DWORD NMSSTDCALL ErrorHandler( CTAHD crhd, DWORD errorcode,
                               char *errortext, char *func )
{
    if ( errorcode != VCEERR_INVALID_MESSAGE
     &&  errorcode != CTAERR_FILE_NOT_FOUND
     &&  errorcode != CTAERR_BAD_ARGUMENT
     && ! (errorcode == VCEERR_WRONG_FILE_TYPE
            && strcmp(func, "vceReadMessageText") == 0))
    {
        printf( "\007Error in %s: %s (%#x)\n", func, errortext, errorcode );
        exit( errorcode );
    }
    return errorcode;
}


/*-------------------------------------------------------------------------
                            RunDemo
-------------------------------------------------------------------------*/
void RunDemo( unsigned filecount, char **filename )
{
    unsigned   i;
    CTAHD      ctahd;
    CTAQUEUEHD ctaqhd;

     /* Turn off display of events and function comments */
    DemoSetReportLevel( DEMO_REPORT_UNEXPECTED );

    /* Register an error handler to display errors returned from API functions;
     * no need to check return values in the demo.
     */
    ctaSetErrorHandler( ErrorHandler, NULL );

    /* Open a port, which maps to a telephone line interface.
     * On this port, we'll accept calls and interact with the caller.
     */
    Setup( &ctaqhd, &ctahd );

    /* Show all files specified */
    for ( i = 0; i < filecount; i++ )
        ShowFile( ctahd, filename[i] );

    /* All done - synchronously close services, destroy context, destroy queue*/
    ctaDestroyQueue( ctaqhd );
    exit( 0 );
}


/*-------------------------------------------------------------------------
                            ShowFile

Show file information.
-------------------------------------------------------------------------*/
void ShowFile( CTAHD ctahd, char *filename )
{
    VCEHD    vh ;
    unsigned message, highmessage;
    unsigned msgsize;
    unsigned ret;
    char     fullname[CTA_MAX_PATH];
    VCE_OPEN_INFO info;
    char    *pc;
    unsigned EncodingKnown = 1;
    char     textbuf[49];
    unsigned textsize;


    DemoDetermineFileType (filename, 0, fullname, &Filetype) ;

    printf( "\nFile: '%s'\n", fullname );

    /* If filetype is "flat" and encoding not specified, default to NMS24 */
    if ( Filetype == VCE_FILETYPE_FLAT && Encoding == 0 )
    {
         Encoding = VCE_ENCODE_NMS_24 ;
         EncodingKnown = 0;
    }

    if ( ( ret = vceOpenFile( ctahd, fullname, Filetype,
                              VCE_PLAY_ONLY, Encoding, &vh ) ) != SUCCESS )
    {
        /* Assume file-not-found. Note that other errors will be caught by
         * the error handler.
         */
        puts( "\t*** Unable to open file!" );
        return;
    }

    vceGetOpenInfo( vh, &info, sizeof info );

    if( EncodingKnown )
        switch( info.filetype )
        {
            case VCE_FILETYPE_VOX:  pc = "NMS Vox"; break;
            case VCE_FILETYPE_WAVE: pc = "MS Wave"; break;
            case VCE_FILETYPE_FLAT: pc = "Flat";    break;
            default:                pc = "Unnamed"; break;
        }
    else
        pc = "Unspecified flat file encoding, assuming NMS-24";

    printf( "Type: %s\n", pc );

    switch( info.encoding )
    {
        case VCE_ENCODE_NMS_16:   pc = "NMS 16 Kbps";           break;
        case VCE_ENCODE_NMS_24:   pc = "NMS 24 Kbps";           break;
        case VCE_ENCODE_NMS_32:   pc = "NMS 32 Kbps";           break;
        case VCE_ENCODE_NMS_64:   pc = "NMS 64";                break;
        case VCE_ENCODE_MULAW:    pc = "Mu-law";                break;
        case VCE_ENCODE_ALAW:     pc = "A-law";                 break;
        case VCE_ENCODE_PCM8M16:  pc = "PCM 8Khz Mono 16 bit";  break;
        case VCE_ENCODE_OKI_24:   pc = "OKI 24 Kbps";           break;
        case VCE_ENCODE_OKI_32:   pc = "OKI 32 Kbps";           break;
        case VCE_ENCODE_PCM11M8:  pc = "PCM 11Khz Mono 8 bit";  break;
        case VCE_ENCODE_PCM11M16: pc = "PCM 11Khz Mono 16 bit"; break;
        case VCE_ENCODE_G726:     pc = "G.726 ADPCM 32 Kbps";   break;
        case VCE_ENCODE_IMA_24:   pc = "IMA ADPCM 24 Kbps";     break;
        case VCE_ENCODE_IMA_32:   pc = "IMA ADPCM 32 Kbps";     break;
        case VCE_ENCODE_GSM:      pc = "MS GSM";                break;
        case VCE_ENCODE_G723_5:   pc = "G723 5.3 Kbps";         break;
        case VCE_ENCODE_G723_6:   pc = "G723 6.4 Kbps";         break;
        case VCE_ENCODE_G729A:    pc = "G729A";                 break;
        case VCE_ENCODE_AMR_475:  pc = "AMR 4.75 Kbps";         break;
        case VCE_ENCODE_AMR_515:  pc = "AMR 5.15 Kbps";         break;
        case VCE_ENCODE_AMR_59:   pc = "AMR 5,9 Kbps";          break;
        case VCE_ENCODE_AMR_67:   pc = "AMR 6.7 Kbps";          break;
        case VCE_ENCODE_AMR_74:   pc = "AMR 7.4 Kbps";          break;
        case VCE_ENCODE_AMR_795:  pc = "AMR 7.95 Kbps";         break;
        case VCE_ENCODE_AMR_102:  pc = "AMR 10.2 Kbps";         break;
        case VCE_ENCODE_AMR_122:  pc = "AMR 12.2 Kbps";         break;
        case VCE_ENCODE_EDTX_MULAW:     pc = "EDTX Mu-law";        break;
        case VCE_ENCODE_EDTX_ALAW:      pc = "EDTX A-law";         break;
        case VCE_ENCODE_EDTX_G726:      pc = "EDTX G,726 32Kbps";  break;
        case VCE_ENCODE_EDTX_G729A:     pc = "EDTX G729A";         break;
        case VCE_ENCODE_EDTX_G723:      pc = "EDTX G723";          break;
        case VCE_ENCODE_EDTX_G723_5:    pc = "EDTX G723 5.3 Kbps"; break;
        case VCE_ENCODE_EDTX_G723_6:    pc = "EDTX G723 6.4 Kbps"; break;
        case VCE_ENCODE_EDTX_AMRNB:     pc = "EDTX AMR";           break;
        case VCE_ENCODE_EDTX_AMRNB_475: pc = "EDTX AMR 4.75 Kbps"; break;
        case VCE_ENCODE_EDTX_AMRNB_515: pc = "EDTX AMR 5.15 Kbps"; break;
        case VCE_ENCODE_EDTX_AMRNB_59:  pc = "EDTX AMR 5,9 Kbps";  break;
        case VCE_ENCODE_EDTX_AMRNB_67:  pc = "EDTX AMR 6.7 Kbps";  break;
        case VCE_ENCODE_EDTX_AMRNB_74:  pc = "EDTX AMR 7.4 Kbps";  break;
        case VCE_ENCODE_EDTX_AMRNB_795: pc = "EDTX AMR 7.95 Kbps"; break;
        case VCE_ENCODE_EDTX_AMRNB_102: pc = "EDTX AMR 10.2 Kbps"; break;
        case VCE_ENCODE_EDTX_AMRNB_122: pc = "EDTX AMR 12.2 Kbps"; break;
        default:                  pc = "Unknown";               break;
    }
    printf( "Encoding: %s\n", pc );

    vceGetHighMessageNumber( vh, &highmessage );

    if ( highmessage == VCE_UNDEFINED_MESSAGE )
        printf( "Empty voice file!" );
    else
    {
        printf( "High Msg: %d\n", highmessage );

        if( !NoMessageInfo )
        {
            /* Show each message message in the file */
            for ( message = 0; message <= highmessage; message++ )
            {
                if ( vceGetMessageSize( vh, message, &msgsize ) != SUCCESS )
                    msgsize = 0;

                if (vceReadMessageText (vh, message, textbuf, sizeof textbuf-1,
                                                  &textsize) != SUCCESS )
                    textsize = 0 ;

                if (msgsize == 0 && textsize == 0)
                    continue;

                printf( "\tMsg %3d is %5d ms", message, msgsize );

                if (textsize != 0)
                {
                     /* print the message "text" only if it's printable.
                      * Add a terminating nul if necessary.
                      */
                    unsigned char c;
                    unsigned      i;
                    for (i=0; i<textsize; i++)
                    {
                        c = (unsigned char)textbuf[i];
                        if (isprint(c) || c=='\t')
                            continue;
                        else
                            break;    /* non-print or nul */
                    }
                    if (i == textsize)
                        textbuf[i] = '\0';
                    if (i > 0 && textbuf[i] == '\0')
                        printf("   \"%s\"", textbuf);
                }

                printf("\n");
            }
        }
    }
    vceClose( vh );
    return;
}

/*-------------------------------------------------------------------------
                            Setup

Initializes Natural Access for use with the VCE service only,
and then creates a queue and context.
-------------------------------------------------------------------------*/
void Setup( CTAQUEUEHD *ctaqueuehd, CTAHD *ctahd )
{
    CTA_SERVICE_NAME servicelist[] =     /* for ctaInitialize */
    {   { "VCE", "VCEMGR" }
    };
    CTA_SERVICE_DESC services[] =        /* for ctaOpenServices */
    {   { {"VCE", "VCEMGR"}, { 0 }, { 0 }, { 0 } }
    };

    DemoInitialize( servicelist, ARRAY_DIM(servicelist));

    DemoOpenPort( 0, "DEMOCONTEXT", services, ARRAY_DIM(services),
                  ctaqueuehd, ctahd );
}
