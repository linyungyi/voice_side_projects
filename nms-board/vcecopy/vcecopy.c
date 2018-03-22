/***************************************************************************
*  File - vcecopy.c
*
*  Description - Voice file copy/convert utility
*
*  Copyright (c) 1996-2002 NMS Communications.  All rights reserved.
***************************************************************************/

#define REVISION_NUM  "4"
#define VERSION_NUM   "1"
#define REVISION_DATE __DATE__

char banner[] = "Natural Access Voice Copy/Conversion Utility V1.%s (%s)\n\n" ;

#include <stdio.h>   /* Other includes follow the help display */
/*----------------------------------------------------------------------------
                            PrintHelp()
----------------------------------------------------------------------------*/
void PrintHelp( void )
{

    puts(
"Usage: vcecopy {source file} {destination file} [options]\n"
"  where options are:\n"
"\n"
"  Encoding options:\n"
"    -e <encoding>    Specifies the encoding of the source file; this is needed\n"
"                     only if the source is a flat (\".VCE\") file.\n"
"                     -e? displays valid choices\n"
"\n"
"  VOX file creation options\n"
"    -x               Deletes the destination file if it exists.  The default\n"
"                     is to replace or insert message(s) from the source to the\n"
"                     destination file.\n"
"\n"
"    -n <indices>     Specifies number of indices to allocate in a new VOX file.\n"
"                     Default is the larger of 250 or 2x the number of messages.\n"
"\n"
"  Conversion options\n"
"    -g <gain>        Specifies gain in dB.  The default is 0 (no gain).\n"
"                     The valid range is -24 to +24\n"
"\n"
"    -s <energy>      Specifies to Shave silence from the beginning and end of a\n"
"                     message containing an NMS encoding.  <energy> is a relative\n"
"                     threshhold. Default is 25.\n"
"\n"
"    -c <encoding>    Specifies the encoding of the destination file when it\n"
"                     is to be converted from the source.\n"
"                     -c? displays valid choices\n"
"\n"
"                     Note: you can alternately specify a Wave PCM encoding as\n"
"                     <rate><M|S><bits>. For example, 22M16 for a coding of\n"
"                     22000 (nominal) samples/sec, Mono, 16 bits per sample.\n"
"                     <rate> can be 8, 11, 22, or 44.\n"
"\n"
"  Message number options\n"
"   -m [<src msg>][,<dest msg>]\n"
"                     Specifies source and destination message numbers. The\n"
"                     default is to copy all messages in the source file.\n"
"                     If only a source message is specified, the destination\n"
"                     message number is the same as the source message number.\n"
"                     Source and destination message numbers are ignored if the\n"
"                     respective file type is non-VOX.\n"
"\n"
"  Message text options\n"
"   -t N[ONE]         Do not copy message text\n"
"   -t O[NLY]         Copy ONLY message text\n"
"   -t F[ILE]         Source file is a text file containing lines of the form\n"
"                     <message number>:<text>\n"
"\n"
"  File options:\n"
"    -i <input file type  (VOX | WAV | VCE)>\n"
"    -o <output file type (VOX | WAV | VCE)>\n"
"\n"
"                     By default, the file type is inferred from the\n"
"                     file name.  Any besides .VOX or .WAV is considered\n"
"                     \"VCE\" (flat).");
    return;
}


/*----------------------------------------------------------------------------
                            Preprocess
----------------------------------------------------------------------------*/
#include "ctademo.h"
#include <string.h>
#include <stdlib.h>
#ifndef UNIX
#include <io.h>
#endif /* UNIX */

#define  EGYDFLT     25L         /* if changed also change help text above  */

/*----------------------------------------------------------------------------
                            Globals
----------------------------------------------------------------------------*/
int        Gain          = 0;
unsigned   Encoding      = 0;
unsigned   DstEncoding   = 0;
unsigned   InFiletype    = 0 ;
unsigned   OutFiletype   = 0 ;
unsigned   Ovrwrt        = 0;
int        EgyMax        = 0;
unsigned   IdxCnt        = 0;
unsigned   SrcMsg        = VCE_ALL_MESSAGES;         /* source message */
unsigned   DstMsg        = VCE_ALL_MESSAGES;         /* destination message */
enum {TEXT_DFLT, TEXT_ONLY, TEXT_NONE, TEXT_FILE} TextOpt = TEXT_DFLT;
VCE_WAVE_INFO Waveinfo   = {sizeof(VCE_WAVE_INFO), 0};

char      *Infile        = NULL;
char      *Outfile       = NULL;
CTAHD      Ctahd         = 0 ;
CTAQUEUEHD Ctaqhd        = 0 ;
VCEHD      Vhin          = 0 ;
VCEHD      Vhout         = 0 ;
unsigned   Highmsg       = VCE_UNDEFINED_MESSAGE ;
unsigned   Framesize     = 0;
unsigned   Frametime     = 0;
int        Newfile       = 0;

char       Infullname  [CTA_MAX_PATH] ;
char       Outfullname [CTA_MAX_PATH] ;

char      *(*Msgtxt)[] = { NULL };       /* Ptr to allocated array of ptrs */

/*----------------------------------------------------------------------------
                            Forward References
----------------------------------------------------------------------------*/
void ParseOptions (int argc, char *argv[]) ;
unsigned GetEncoding (char *optstring, BOOL dest);
void ShowEncoding (unsigned encoding) ;
DWORD NMSSTDCALL ErrorHandler( CTAHD crhd, DWORD errorcode,
                                               char *errortext, char *func ) ;
void Setup (void) ;
int OpenFiles (void);
int ReadMessageText( FILE *text_fp, unsigned *highmsg);
int DoConvert(void);
void CopyAndTrimSilence (unsigned srcmsg, unsigned msgsize, unsigned dstmsg);
int copyframes (unsigned dstmsg, unsigned dstfrm,
                unsigned srcmsg, unsigned srcfrm, unsigned nframes);
int getegy( unsigned frm );
void Cleanup (void);


/*----------------------------------------------------------------------------
                             main
----------------------------------------------------------------------------*/
int main (int argc, char *argv[])
{
    int exitcode;

    printf( banner, REVISION_NUM, REVISION_DATE );
    ParseOptions (argc, argv);

    Setup () ;

    if ((exitcode=OpenFiles ()) == 0)
    {
        /* ShowEncoding(DstEncoding); */
        exitcode = DoConvert() ;
    }
    Cleanup();
    exit (exitcode) ;
}

/*----------------------------------------------------------------------------
                            parseoptions

Note: getopt() is not used.  Options can be mixed with file names.
----------------------------------------------------------------------------*/
void ParseOptions (int argc, char *argv[])
{
    int i, j;
    char *pc;
    char opt;
    char *optstring ;
    char junk[256];
    int SrcMsgSpecified = 0;
    int DstMsgSpecified = 0;

    i = 1 ;
    while ( i < argc )      /* Parse options */
    {
        pc = argv[i++];

        if ( *pc == '-'
#ifndef UNIX
             || *pc == '/'
#endif
           )
        {
            opt = *++pc ;

             /* First, test for options that take no argument */
            if (toupper(opt) ==  'X')         /* Overwrite any existing file */
            {
                if (*(pc+1) != '\0')
                    goto badarg ;
                 Ovrwrt = 1;
                 continue;
            }
            if (*(pc+1) != '\0')
                optstring = pc+1 ;
            else if (i < argc
#ifndef UNIX
                     && argv[i][0] != '/'
#endif
                     && argv[i][0] != '-')
                optstring = argv[i++];
            else
                optstring = "" ;

            switch( toupper(opt) )
            {
                case 'C':                       /* Destination Encoding */
                    DstEncoding = GetEncoding (optstring, TRUE /*dest*/) ;
                    if (DstEncoding == 0 && Waveinfo.format == 0)
                        goto badarg ;
                    break;

                case 'E':                       /* Encoding */
                    Encoding = GetEncoding (optstring, FALSE /*not dest*/) ;
                    if (Encoding == 0)
                        goto badarg ;
                    break;

                case 'G':                       /* Gain */
                    if (optstring[0] == '\0')
                        goto badarg ;
                    Gain = atoi( optstring );
                    break;

                case 'I':                        /* Input file type */
                    InFiletype = DemoGetFileType (optstring);
                    if (InFiletype == 0)
                        goto badarg ;
                    break;

                case 'O':                        /* Input file type */
                    OutFiletype = DemoGetFileType (optstring);
                    if (OutFiletype == 0)
                        goto badarg ;
                    break;

                case 'S':                       /* Shave silence */
                    /* if optstring is NULL, then atoi will return 0
                       (and next line will ensure the the default
                       value is used) */
                    EgyMax = atoi( optstring );
                    if ( EgyMax <= 1 )  EgyMax = EGYDFLT;
                    break;

                case 'N':                       /* Number of indices to alloc */
                    IdxCnt = atoi( optstring );
                    if (IdxCnt == 0)
                        goto badarg ;
                    break;

                case 'M':                       /* Explicit message number */
                    /* check for leading comma to copy all indexes into one */
                    if (strchr( optstring, (int)',' ) == optstring)
                    {
                        if (sscanf( optstring, ",%d%s", &DstMsg, junk ) != 1)
                            goto badarg ;
                        DstMsgSpecified = 1;
                    }
                    else
                    {
                        j = sscanf( optstring,
                                    "%d,%d%s", &SrcMsg, &DstMsg, junk );
                        if (j < 1 || j > 2)
                            goto badarg ;
                        if (j >= 1)
                            SrcMsgSpecified = 1;
                        if (j >= 2)
                            DstMsgSpecified = 1;
                    }
                    break;

                case 'T':                        /* Message text option */
                    /* Some unixes do not have stricmp(). */
                    if (strcmp (optstring, "NONE") == 0
                     || strcmp (optstring, "N") == 0
                     || strcmp (optstring, "none") == 0
                     || strcmp (optstring, "n") == 0)
                        TextOpt = TEXT_NONE;
                    else if (strcmp (optstring, "ONLY") == 0
                     || strcmp (optstring, "O") == 0
                     || strcmp (optstring, "only") == 0
                     || strcmp (optstring, "o") == 0)
                        TextOpt = TEXT_ONLY;
                    else if (strcmp (optstring, "FILE") == 0
                     || strcmp (optstring, "F") == 0
                     || strcmp (optstring, "file") == 0
                     || strcmp (optstring, "f") == 0)
                        TextOpt = TEXT_FILE;
                    else
                        goto badarg ;
                    break;

                case '?':
                case 'H':
                    PrintHelp ();
                    exit (0);

                default:
                    printf( "Option %c not recognized.\n", opt );
                    goto recommendhelp ;
            }
        }
        else if (Infile == NULL)
            Infile = pc ;
        else if (Outfile == NULL)
            Outfile = pc ;
        else
        {
            printf( "Argument %s not recognized.\n", pc );
            goto recommendhelp ;
        }
    }
    if (Infile == NULL)
    {
        puts ("filenames missing");
        goto recommendhelp ;
    }
    if (Outfile == NULL)
    {
        puts ("2 filenames required");
        goto recommendhelp ;
    }

    if (TextOpt == TEXT_FILE && InFiletype != 0)
    {
        printf ("Input file type may not be specified "
                "when input file is a message text file\n");
        exit (1);
    }
    if ((TextOpt == TEXT_FILE || TextOpt == TEXT_ONLY)
         && (EgyMax != 0 || Gain != 0))
    {
        puts ("Gain or trim options are invalid "
              "when copying only message text\n");
        exit(1);
    }
    if (TextOpt == TEXT_FILE && DstEncoding != 0 && Encoding != 0)
    {
        puts ("Only one encoding may be specified "
              "when input file is a message text file\n");
        exit(1);
    }

    /* Determine file types, append extensions if none provided */
    if (TextOpt != TEXT_FILE)
    {
        /* Voice file */
        DemoDetermineFileType (Infile, 0, Infullname, &InFiletype) ;
    }
    else
    {
        /* Message text file */
        strcpy (Infullname, Infile);
        if (strrchr (Infile, '.') == NULL)
        {
            strcat (Infullname, ".txt");
        }
    }
    DemoDetermineFileType (Outfile, 1, Outfullname, &OutFiletype) ;
    printf("Source file     : %s\n", Infullname);
    printf("Destination file: %s\n", Outfullname);

    if (TextOpt == TEXT_FILE && OutFiletype != VCE_FILETYPE_VOX)
    {
        printf("Message text options are only valid for .VOX files\n");
        exit (1);
    }
    if (TextOpt != TEXT_FILE
         && InFiletype == VCE_FILETYPE_FLAT && Encoding == 0)
    {
        printf("  You must specify the encoding of the input file\n");
        exit (1);
    }
    if (Waveinfo.format != 0 && OutFiletype != VCE_FILETYPE_WAVE)
    {
        puts ("Wave encoding can be specified only for Wave files");
        exit (1);
    }
    if (IdxCnt != 0 && OutFiletype != VCE_FILETYPE_VOX)
    {
        puts ("File indexes can be specified only for VOX files");
        exit (1);
    }

    /* for vox-file copying, DstMsg == SrcMsg by default */
    if (OutFiletype == VCE_FILETYPE_VOX)
    {
        if (! DstMsgSpecified )
            if (SrcMsgSpecified)
                DstMsg = SrcMsg;
            else if (TextOpt != TEXT_FILE && InFiletype != VCE_FILETYPE_VOX)
                DstMsg = 0;
            /* else default to all-all */
    }
    /* destination message number is ignored for non-vox files */
    else
    {
        if (DstMsgSpecified && DstMsg != 0)
            printf("  Destination message number ignored\n");
        DstMsg = 0;
    }

    /* source message number must be 0 for non-vox files */
    if (TextOpt != TEXT_FILE && InFiletype != VCE_FILETYPE_VOX)
    {
        if (SrcMsgSpecified && SrcMsg != 0
         && (DstMsgSpecified || OutFiletype != VCE_FILETYPE_VOX))
            printf("  Source message number ignored\n");
        SrcMsg = 0;
    }
    return ;

badarg:
    printf("Missing or invalid argument to -%c option\n", opt);
recommendhelp:
    printf("Type %s -? or -h for help\n", argv[0]);
    exit (1);
}


/*----------------------------------------------------------------------------
                            GetEncoding
----------------------------------------------------------------------------*/
unsigned GetEncoding (char *optstring, BOOL dest)
{
    unsigned rate ;
    char     mode ;
    unsigned bits ;
    unsigned encoding = 0;

    if (*optstring != '?')
    {
        if (sscanf( optstring, "%d%c%d", &rate, &mode, &bits) == 3)
        {
            if (mode == 'm' || mode == 'M')
                Waveinfo.nchannels = 1;
            else if (mode == 's' || mode == 'S')
                Waveinfo.nchannels = 2;
            else
            {
                printf("Invalid mode %c. M or S expected\n", mode);
                goto d_opterror ;
            }
            if (bits == 8 || bits == 16)
                Waveinfo.bitspersample = bits ;
            else
            {
                printf("Invalid samplesize %d. 8 or 16 expected\n",
                             bits);
                goto d_opterror ;
            }

            switch (rate)
            {
                case 8:   Waveinfo.samplespersec =  8000; break ;
                case 11:  Waveinfo.samplespersec = 11025; break ;
                case 22:  Waveinfo.samplespersec = 22050; break ;
                case 44:  Waveinfo.samplespersec = 44100; break ;
                default:
                    printf("Invalid rate %d. "
                           "Rate must be 8,11,22 or 44\n", rate);
                    goto d_opterror ;
            }
            Waveinfo.blocksize = Waveinfo.nchannels *
                                           Waveinfo.bitspersample/8;
            Waveinfo.datarate = Waveinfo.samplespersec *
                                                Waveinfo.blocksize;
            Waveinfo.format         = 1 ;   /* PCM */
            return 0;
        }
        else
        {
            sscanf( optstring, "%d", &encoding );
            return encoding ;
        }
    }
    else // -e? or -c?   -- list valid encoding values and their names
    {
        unsigned i ;
        for (i=1; i<DemoNumVoiceEncodings; i++)
        {
            if (*DemoVoiceEncodings[i] != '\0')
            {
                /* indicate whether conversion is supported for this encoding */
                BOOL convertible = FALSE;
                switch (i)
                {
                    case VCE_ENCODE_NMS_16  :  /*  1 */
                    case VCE_ENCODE_NMS_24  :  /*  2 */
                    case VCE_ENCODE_NMS_32  :  /*  3 */
                    case VCE_ENCODE_NMS_64  :  /*  4 */
                    case VCE_ENCODE_MULAW   :  /* 10 */
                    case VCE_ENCODE_ALAW    :  /* 11 */
                    case VCE_ENCODE_PCM8M16 :  /* 13 */
                    case VCE_ENCODE_OKI_32  :  /* 15 */
                    case VCE_ENCODE_PCM11M8 :  /* 16 */
                    case VCE_ENCODE_PCM11M16:  /* 17 */
                    case VCE_ENCODE_G726    :  /* 20 */
                    case VCE_ENCODE_IMA_32  :  /* 23 */
                        convertible = TRUE;
                        break;
                    default:
                        break;
                }
                if (dest)
                {
                    // -c?    show only convertible encodings
                    if (convertible)
                    {
                        printf( " %2d %-8s\n", i, DemoVoiceEncodings[i]);
                    }
                }
                else
                {
                    // -e?   indicate non-convertible encodings with an asterisk
                    printf( " %2d %-8s %c\n", i, DemoVoiceEncodings[i],
                                                       convertible ? ' ' : '*');
                }
            }
        }
        if (! dest)
        {
            printf("\n (* = can copy to same encoding only)\n");
        }
        exit(0) ;
    }

 d_opterror:
    puts("Invalid encoding");
    return 0;
}

/*-------------------------------------------------------------------------
                            ShowEncoding
-------------------------------------------------------------------------*/
void ShowEncoding (unsigned encoding)
{
    const char *text ;
    if (encoding < DemoNumVoiceEncodings)
        text = DemoVoiceEncodings[encoding];
    else if (encoding >= 256)
        text = "special" ;
    else
        text = "" ;
    printf( "\tEncoding   = %d (%s)\n", encoding, text );
    return ;
}

/*-------------------------------------------------------------------------
                            Setup
-------------------------------------------------------------------------*/
void Setup (void)
{
    CTA_EVENT event ;
    CTA_INIT_PARMS initparms = { 0 };
    unsigned       ret ;

     /* ctaInitialize */
    static CTA_SERVICE_NAME Init_services[] = {{"VCE", "VCEMGR"}};

     /* ctaOpenServices */
    static CTA_SERVICE_DESC Services[] = {{{"VCE", "vCEMGR"}}};

     /* Turn off display of events and function comments */
    DemoSetReportLevel(DEMO_REPORT_UNEXPECTED);

    /* Register an error handler to display errors returned from API functions;
     * no need to check return values in the demo.
     */
    ctaSetErrorHandler( ErrorHandler, NULL );

    initparms.size = sizeof(CTA_INIT_PARMS);
    initparms.ctacompatlevel = CTA_COMPATLEVEL;
    ret = ctaInitialize(Init_services, ARRAY_DIM(Init_services), &initparms);
    if( ret != SUCCESS )
    {
        printf("ERROR: ctaInitialize returned 0x%08x (see ctaerr.h)\n", ret);
        exit (1);
    }

    /* Open the Natural Access application queue with all managers */
    ctaCreateQueue(NULL, 0, &Ctaqhd);

    /* Create a call resource for handling the incoming call */
    ctaCreateContext(Ctaqhd, 0, NULL, &Ctahd);

    /* Open the service managers and associated services */
    ctaOpenServices(Ctahd, Services, ARRAY_DIM(Services));

    /* Wait for the service manager and services to be opened asynchronously */
    DemoWaitForSpecificEvent(Ctahd, CTAEVN_OPEN_SERVICES_DONE, &event);
    if (event.value != CTA_REASON_FINISHED)
    {
        DemoShowError( "Opening services failed", event.value );
        exit( 1 );
    }
    return;
} // Setup

/*-------------------------------------------------------------------------
                            ErrorHandler
-------------------------------------------------------------------------*/
DWORD NMSSTDCALL ErrorHandler( CTAHD crhd, DWORD errorcode,
                                               char *errortext, char *func )
{
    if (errorcode != CTAERR_FILE_NOT_FOUND
     && errorcode != CTAERR_FILE_EXISTS)
    {
        printf( "\007Error in %s: %s (%#x)\n", func, errortext, errorcode );
        Cleanup();
        if (Newfile)
        {
            remove (Outfullname);
        }
        exit( errorcode );
    }
    return errorcode;
}


/*-------------------------------------------------------------------------
                            OpenFiles
-------------------------------------------------------------------------*/
int OpenFiles (void)
{
    VCE_OPEN_INFO  info ;
    VCE_OPEN_INFO  outinfo ;
    void          *pcreate ;
    unsigned       ret ;
    VCE_CREATE_VOX voxcreate;
    unsigned       msgsize ;

    /* Open input file */
    if (TextOpt != TEXT_FILE)
    {
        if (vceOpenFile (Ctahd, Infullname, InFiletype, VCE_PLAY_ONLY, Encoding,
                   &Vhin) != SUCCESS)
        {
             /* Assume file-not-found. Note that other errors will be caught by
              * the error handler.
              */
            printf ("    *** Unable to open input file %s", Infullname);
            return 1;
        }

        vceGetHighMessageNumber (Vhin, &Highmsg);
        if (SrcMsg == VCE_ALL_MESSAGES && Highmsg == VCE_UNDEFINED_MESSAGE)
        {
            printf("    *** Input file is empty - output not opened\n");
            return 1 ;
        }

        if (SrcMsg != VCE_ALL_MESSAGES
            && vceGetMessageSize(Vhin, SrcMsg, &msgsize) == SUCCESS
            && msgsize == 0)
        {
            printf("    *** Input message is empty - output not opened\n");
            return 1 ;
        }

        /* If source encoding not specified, then it's gotten from the file */
        if (Encoding == 0)
        {
            vceGetOpenInfo (Vhin, &info, sizeof info);
            Encoding = info.encoding ;
        }

        if (Encoding > VCE_ENCODE_NMS_64 && EgyMax != 0)
        {
            printf( "Cannot shave silence in non-NMS encoding.\n");
            return 1;
        }

        /* If trimming silence, need frame size */
        if (EgyMax != 0)
            vceGetEncodingInfo  (Ctahd, Encoding, &Framesize, &Frametime);
    }
    else /* input is message text file */
    {
        char fullpathname [CTA_MAX_PATH] ;
        FILE *text_fp;

        if( ctaFindFile( Infullname, "txt", "CTA_DPATH",
                                 fullpathname, CTA_MAX_PATH) != SUCCESS )
        {
            printf("     *** Message Text file %s not found\n", Infullname);
            return 1;
        }

        text_fp = fopen( fullpathname, "r" );
        if( text_fp == NULL )
        {
            printf ("    *** Unable to open input file %s", fullpathname);
            return 1;
        }

        if (ReadMessageText(text_fp, &Highmsg) != 0)
        {
            fclose (text_fp);
            return 1;
        }

        fclose (text_fp);
        if (Highmsg == VCE_UNDEFINED_MESSAGE)
        {
            printf("    *** Input file is empty - output not opened\n");
            return 1 ;
        }
    }

    /*********************************************/
    /* Prepare to open or create the output file.*/
    /*********************************************/

    /* If file exists and delete option not specified and file type VOX,
       then open, vs create */

    /* Note that the error handler will abort the process if the file is
     * read-only or the path is invalid.
     */

    if (OutFiletype == VCE_FILETYPE_VOX
        && ! Ovrwrt
        && vceOpenFile ( Ctahd, Outfullname, OutFiletype, VCE_PLAY_RECORD,
                   DstEncoding, &Vhout) == SUCCESS)
    {
          /* If dest encoding was not specified, then get from the file */
        if (DstEncoding == 0)
        {
            vceGetOpenInfo (Vhout, &outinfo, sizeof outinfo);
            DstEncoding = outinfo.encoding ;
        }
          /* If source is a text file, force source encoding to equal dest */
        if (TextOpt == TEXT_FILE)
            if (Encoding == 0)
                Encoding = DstEncoding ;
            else if (Encoding != DstEncoding)
            {
                printf(" Destination file does not have specified encoding\n");
                return 1 ;
            }

        if (EgyMax  && (DstEncoding != Encoding || Gain != 0))
        {
            printf(" Conversion + silence trim not currently supported\n");
            printf(" Perform these operations separately\n");
            return 1 ;
        }
    }
    else  /* Not VOX, or VOX file does not exist, or delete specified */
    {
          /* If Destination encoding not specified, use input type */
        if (DstEncoding == 0 && Waveinfo.format == 0)
        {
            if (Encoding == 0)
            {
                 /* This can only occur if input is a text file */
                printf("You must specify an encoding\n");
                return 1 ;
            }
            DstEncoding = Encoding ;
        }

        if (EgyMax  && (DstEncoding != Encoding || Gain != 0))
        {
            printf(" Conversion + silence trim not currently supported\n");
            printf(" Perform these operations separately\n");
            return 1 ;
        }

        if (remove( Outfullname ) == -1 && errno != ENOENT)
        {
            printf ("Unable to delete %s: %s\n", Outfullname, strerror (errno));
            return 1;
        }

         /* If destination encoding was specified as a Wave format, pass
          * Wave info structure to vcecreateFile */
        if (Waveinfo.format != 0)
            pcreate = &Waveinfo ;

         /* If destination file is a VOX file, pass index count, if specified,
          * or if copying all messages
          */
        else if (OutFiletype == VCE_FILETYPE_VOX)
        {
            if (IdxCnt != 0)
            {
                voxcreate.maxindex = IdxCnt ;
                voxcreate.size     = sizeof voxcreate;
                pcreate = &voxcreate ;
            }
            else if (DstMsg == VCE_ALL_MESSAGES
                  && SrcMsg == VCE_ALL_MESSAGES
                  && TextOpt != TEXT_ONLY )
            {
                 /* count messages in source */
                unsigned i ;
                unsigned msgcount = 0 ;

                if (TextOpt != TEXT_FILE)
                {
                    if (Highmsg != VCE_UNDEFINED_MESSAGE)
                        for (i=0; i<=Highmsg; i++)
                        {
                            if (vceGetMessageSize(Vhin, i, &msgsize)
                                    == SUCCESS && msgsize != 0)
                                ++msgcount ;
                        }
                }
                else  /* Message text file */
                {
                    for (i=0; i<=Highmsg; i++)
                        if ((*Msgtxt)[i] != NULL)
                            ++msgcount;
                }

                if (msgcount*2 > VCEVOX_DFLTIDX)
                {
                    voxcreate.maxindex = 2 * msgcount;
                    voxcreate.size     = sizeof voxcreate;
                    pcreate = &voxcreate ;
                }
                else
                    pcreate = NULL ;
            }
            else
                pcreate = NULL ;
        }
        else
            pcreate = NULL ;

        if ((ret=vceCreateFile (Ctahd, Outfullname, OutFiletype, DstEncoding,
                        pcreate, &Vhout)) != SUCCESS)
        {
            switch (ret)
            {
                case CTAERR_FILE_EXISTS :
                    printf ("    *** Output file %s already exists."
                            "  Use -x option to overwrite it\n", Outfile);
                    break;
                default:
                    printf ("    *** Unable to create output file %s", Outfile);
                    break;
            }
            return 1 ;
        }
        else
            Newfile = 1;
    }
    return 0 ;
}

/*-------------------------------------------------------------------------
                            ReadMessageText

  Read a text file into memory.
-------------------------------------------------------------------------*/
int ReadMessageText( FILE *text_fp, unsigned *highmsg)
{
    Msgtxt = malloc ((VCEVOX_MAX_MESSAGE+1) * sizeof(char *));
    if (Msgtxt == NULL)
    {
        printf ("    *** Memory allocation failed ***");
        return 1;
    }
    memset (Msgtxt, 0, (VCEVOX_MAX_MESSAGE+1) * sizeof(char *));

    while( !feof( text_fp ) )
    {
        char buff[256];
        unsigned msg ;

        fgets( (char *)buff, sizeof buff, text_fp );  /* read a line */

        if( feof( text_fp ) )
            break;

        if( isdigit( *buff ) )
        {
            char *pc;

            msg = atoi( buff );
            if( msg > VCEVOX_MAX_MESSAGE )
                continue;

            pc = strchr( buff, ':' );
            if( pc != NULL )
            {
                char *ptmp;

                ++pc;
                for( ptmp=pc ; *ptmp != 0; ptmp++ )
                    if( iscntrl( *ptmp) )
                    {
                        *ptmp = '\0';
                        break;
                    }
                ptmp = malloc (strlen(pc)+1);
                if (ptmp == NULL)
                {
                    printf ("    *** Memory allocation failed ***");
                    return 1;
                }
                strcpy( ptmp, pc );
                (*Msgtxt)[msg] = ptmp;

                if( *highmsg == VCE_UNDEFINED_MESSAGE || msg > *highmsg )
                    *highmsg = msg;
            }
        }
    }
    return 0;
}


/*-------------------------------------------------------------------------
                            DoConvert

 Copy or convert all messages
-------------------------------------------------------------------------*/
int DoConvert(void)
{
    unsigned msg;
    unsigned msgsize;
    char    *display;

    /* If copying all messages from VOX to existing VOX, do each individually.
     * This preserves messages in the dest that have 0 size in the source.
     * Also need to copy individually if trimming.
     */
    if ( InFiletype == VCE_FILETYPE_VOX && OutFiletype == VCE_FILETYPE_VOX
      && SrcMsg == VCE_ALL_MESSAGES && DstMsg == VCE_ALL_MESSAGES
      && TextOpt != TEXT_ONLY
      && (!Newfile || EgyMax != 0))
    {
        for( msg = 0; msg <= Highmsg; msg++ )
        {
            vceSetCurrentMessage (Vhin, msg) ;
            vceGetCurrentSize (Ctahd, &msgsize) ;
            if ( msgsize == 0)
                    continue;

            printf( "Message %03d ", msg );

            /* if trimming, do all the extra work */
            if ( EgyMax != 0)
            {
                CopyAndTrimSilence (msg, msgsize, msg);
                display = "";
            }
            /* Convert */
            else if (DstEncoding != Encoding || Gain != 0)
            {
                vceConvertMessage (Vhin, msg, Vhout, msg, Gain) ;
                display = "Converted.";
            }
            else
            {
                /* Straight copy */
                vceCopyMessage (Vhin, msg, Vhout, msg) ;
                display = "Copied.";
            }

            /* Copy text, too, unless opted not to */
            if (TextOpt != TEXT_NONE)
                vceCopyMessageText (Vhin, msg, Vhout, msg) ;

            puts (display);
        }
    }
    else if (TextOpt == TEXT_FILE) /* message text from text file */
    {
        for( msg = 0; msg <= Highmsg; msg++ )
        {
            if ((*Msgtxt)[msg] != NULL)
            {
                vceWriteMessageText (Vhout, msg, (*Msgtxt)[msg],
                                     strlen((*Msgtxt)[msg])+1);
                printf( "Message %03d Copied\n", msg );
            }
        }
    }
    else
    {
        /* No loop necessary - either only one message or let vce library
         * handle ALL_MESSAGES as one.
         */

        if (TextOpt != TEXT_FILE)
        {
            /* if trimming, do all the extra work */
            if ( EgyMax != 0 )
            {
                vceSetCurrentMessage (Vhin, SrcMsg) ;
                vceGetCurrentSize (Ctahd, &msgsize) ;
                CopyAndTrimSilence (SrcMsg, msgsize, DstMsg) ;
                display = "";
            }
            else if (TextOpt == TEXT_ONLY)
                display = "Copied.";
            else if  (DstEncoding != Encoding || Gain != 0)
            {
                /* conversion */
                vceConvertMessage (Vhin, SrcMsg, Vhout, DstMsg, Gain) ;
                display = "Converted.";
            }
            else
            {
                /* Straight copy */
                vceCopyMessage (Vhin, SrcMsg, Vhout, DstMsg);
                display = "Copied.";
            }

            /* If VOX-VOX, copy text, too, unless opted not to */
            /* Screen out the case of all messages to one */
            if ( InFiletype == VCE_FILETYPE_VOX
              && OutFiletype == VCE_FILETYPE_VOX
              && TextOpt != TEXT_NONE
              && (SrcMsg != VCE_ALL_MESSAGES || DstMsg == VCE_ALL_MESSAGES))
                vceCopyMessageText (Vhin, SrcMsg, Vhout, DstMsg) ;

            if ( EgyMax == 0 )
            {
                if ( InFiletype == VCE_FILETYPE_VOX
                  && SrcMsg == VCE_ALL_MESSAGES)
                    printf ("All messages ");
                else
                    printf ("Message ");
            }
            puts (display);
        }
        else  /* message text from text file */
        {
            if ((*Msgtxt)[SrcMsg] != NULL)
                vceWriteMessageText (Vhout, SrcMsg, (*Msgtxt)[SrcMsg],
                                    strlen((*Msgtxt)[SrcMsg])+1);
            else
                vceWriteMessageText (Vhout, SrcMsg, NULL, 0);
            puts ("Message text copied.") ;
        }
    }

    return 0;
}


/*-------------------------------------------------------------------------
                            CopyAndTrimSilence
-------------------------------------------------------------------------*/
#define FRMAVERANGE 20L  /* number for frames to to average at a time */
#define MINOVERFRMS 10L  /* Minimum # of frms in range which must be over
                            to qualify as non silence threshold */
struct {
    unsigned frm;
    int      egy;
    } EgyCache[FRMAVERANGE];
static int      ECLast = 0;

/*-------------------------------------------------------------------------*/
void CopyAndTrimSilence (unsigned srcmsg, unsigned msgsize, unsigned dstmsg)
{
    unsigned i ;
    unsigned maxfrm;
    unsigned begfrm, lastfrm, endfrm ;
    int      egy;
    unsigned frmsover, f;

    /* mark cache as empty  */
    for( i=0; i < FRMAVERANGE; i++ )
        EgyCache[i].frm = (unsigned)-1;
    ECLast = 0;

    maxfrm = msgsize/Frametime ;

    begfrm  = 0;
    if (maxfrm > FRMAVERANGE)
    {

        /* shave beginning of message */

        /*
          Find first frame where the energy is above the threshold
          AND the next 'FRMAVERANGE'-1 frames have more
          than 'MINOVERFRMS' frames over the threshold energy.
          Once this position is found, back up 'FRMAVERANGE' frames
          to be sure at least a few of the lower energy frames are
          included to ensure a clean startup of the message when
          played back.
        */

        lastfrm = maxfrm - FRMAVERANGE;
        begfrm  = 0;

        do
        {
            egy = getegy( begfrm );
            if ( egy >= EgyMax )
            {
                frmsover = 1;
                for ( f = 1; f < FRMAVERANGE; f++ )
                    if ( getegy(begfrm+f) >= EgyMax )
                        frmsover++;
            }
            begfrm++;                       /* go to next frame */

        } while( (egy      < EgyMax       ||
                      frmsover < MINOVERFRMS) &&
                      begfrm   < lastfrm );

         /* back up a few frames  */
        if ( begfrm < FRMAVERANGE )
            begfrm = 0;
        else
            begfrm -= FRMAVERANGE;
    }

    endfrm = maxfrm-FRMAVERANGE;
    if ((endfrm-1) > begfrm)
    {

        /* Shave end of message (Same algorithm as above in reverse) */

        /*
          Scan backwards to find the last frame where the energy is
          above the threshold AND the previous 'FRMAVERANGE'-1
          frames have more than 'MINOVERFRMS' frames over the
          threshold energy.  Once this position is found, go forward
          'FRMAVERANGE' frames to be sure at least a few of the lower
          energy frames are included to ensure a clean ending of the
          message when played back.
        */

        lastfrm = MAX (FRMAVERANGE-1, begfrm) ;

        do
        {
            endfrm--;

            egy = getegy( endfrm );
            if ( egy >= EgyMax )
            {
                frmsover = 1;
                for ( f = 1; f < FRMAVERANGE; f++ )
                    if ( getegy(endfrm-f) >= EgyMax )
                        frmsover++;
            }

        } while( (egy      < EgyMax       ||
                      frmsover < MINOVERFRMS) &&
                      endfrm   > lastfrm );

        endfrm += FRMAVERANGE;
        if ( endfrm >= maxfrm )
            endfrm = maxfrm-1;
    }

    /* Delete old dest msg (if any) */
    vceEraseMessage(Vhout, dstmsg );

    /* Copy frames from source message to dest */
    if (copyframes (dstmsg, 0, srcmsg, begfrm, endfrm-begfrm+1) == 0)
        printf( "Beg:%03ld/End:%03ld frames trimmed and %4ld frames copied.",
                  begfrm, maxfrm-endfrm-1, endfrm-begfrm+1 );
    return ;
}

/*-------------------------------------------------------------------------
                            copyframes
-------------------------------------------------------------------------*/
int copyframes (unsigned dstmsg, unsigned dstfrm,
                unsigned srcmsg, unsigned srcfrm, unsigned nframes)
{
     /* Copy frames from source message to dest */

    BYTE          *framebuf;
    unsigned       bufsize ;
    unsigned       bufframes;

    /* Allocate a buffer for copying */
    bufframes = 4096/Framesize ;
    bufsize =  bufframes * Framesize ;
    if ((framebuf = malloc(bufsize)) == NULL)
    {
        printf( "Out of memory.\n");
        Cleanup();
        if (Newfile)
        {
            remove (Outfullname);
        }
        exit(1);
    }

    while ( nframes != 0 )
    {
        unsigned actual ;
        unsigned bytesread;
        unsigned byteswritten;
        unsigned frms ;

        frms = MIN (nframes, bufframes);

         /* Set context to source position */
        vceSetCurrentMessage (Vhin, srcmsg) ;
        vceSetPosition ( Ctahd, srcfrm*Frametime, VCE_SEEK_SET, &actual);
        if (actual != srcfrm*Frametime)
        {
            printf("Internal copy error - source position\n");
            goto error;
        }

         /* Read a block from the source */
        vceRead (Ctahd, framebuf, frms*Framesize, &bytesread) ;
        if (bytesread == 0)
        {
            printf("Internal copy error - read source\n");
            goto error;
        }

         /* Set context to dest position */
        vceSetCurrentMessage (Vhout, dstmsg) ;
        vceSetPosition ( Ctahd, dstfrm*Frametime, VCE_SEEK_SET, &actual);
        if (actual != dstfrm*Frametime)
        {
            printf("Internal copy error - destination position\n");
            goto error;
        }

         /* Write the block to the dest */
        vceWrite (Ctahd, framebuf, bytesread, VCE_INSERT, &byteswritten);
        if (byteswritten != bytesread)
        {
            printf("Internal copy error - read destination\n");
            goto error;
        }

        nframes -= frms ;
        srcfrm += frms ;
        dstfrm += frms ;
    }
    free (framebuf) ;
    return 0;
error:
    free (framebuf) ;
    return -1;
}

/*-------------------------------------------------------------------------
                            getegy
-------------------------------------------------------------------------*/
int getegy( unsigned frm )
{
    int      egy;
    unsigned i;
    BYTE     frame[162];
    unsigned bytesread ;
    unsigned actual ;

    /*
       A cache of frame energies is maintained so shave algorithm only really
       goes to disk once for each frame's energy, even though it uses it
       multiple times.
    */

    for( i=0; i< FRMAVERANGE; i++ )        /* 1st see if in cache */
    {
        if ( frm == EgyCache[i].frm )  return( EgyCache[i].egy );
    }

    if (vceSetPosition ( Ctahd, frm*Frametime, VCE_SEEK_SET, &actual) != SUCCESS
         || actual != frm*Frametime)
        return 0;
    if (vceRead (Ctahd, (BYTE *)frame, Framesize, &bytesread) != SUCCESS
         || bytesread != Framesize)
        return 0;
    egy = (int)(frame[Framesize-2] & 0x7f) ;

    EgyCache[ECLast].egy = egy;
    EgyCache[ECLast].frm = frm;
    if ( ++ECLast >= FRMAVERANGE )
        ECLast = 0;

    return( egy );
}


/*-------------------------------------------------------------------------
                            Cleanup
-------------------------------------------------------------------------*/
void Cleanup (void)
{
    CTA_EVENT event ;

    if (Vhin != 0)
    {
        vceClose (Vhin) ;
    }
    if (Vhout != 0)
    {
        vceClose (Vhout) ;
    }
    if (Msgtxt != NULL)
    {
        unsigned i;
        for (i=0;i<=VCEVOX_MAX_MESSAGE;i++)
            free ((*Msgtxt)[i]);
        free (Msgtxt);
    }

    if (Ctahd != 0)
    {
        ctaDestroyContext(Ctahd);

        DemoWaitForSpecificEvent(Ctahd,
                             CTAEVN_DESTROY_CONTEXT_DONE,
                             &event);
        ctaDestroyQueue(Ctaqhd);
    }
    return ;
}

