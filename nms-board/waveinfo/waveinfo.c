/************************************************************************
 *  File - Waveinfo
 *
 *  Description - Display info about a WAVE file
 *
 *  Copyright (c) 1995-2002 NMS Communications.  All rights reserved.
 ***********************************************************************/
#define VERSION_NUM  "13"
#define VERSION_DATE __DATE__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "ctadef.h"


/* Macros to convert between big and little endian architectures. */

#ifdef SOLARIS_SPARC

#define SWAPDWORD(XDWORD)\
(((XDWORD) << 24) | ((XDWORD) << 8 & 0x00ff0000) |\
 ((XDWORD) >> 24 & 0x000000ff) | ((XDWORD) >> 8 & 0x0000ff00))

#define SWAPBYTES(x) ((((x) & 0x00ff ) << 8) | ((x) >> 8 & 0x00ff))

#endif


void ShowWaveFile (char *filename) ;
int  scanforchunk (char *tag, FILE *filep) ;
char *getErrorText(DWORD errcode ) ;

/*----------------------------- main ------------------------------*/
int main (int argc, char **argv)
{
    DWORD ret ;
    char  fullname[CTA_MAX_PATH];

    CTA_INIT_PARMS initparms = { 0 };

    /* ctaInitialize */
    static CTA_SERVICE_NAME Init_services;  /* dummy */

    /* ctaOpenServices */
    static CTA_SERVICE_DESC Services[] = {
        {
            {"VCE", "vCEMGR"},              /* svcname, svcmgrname */
        }};


    if (argc != 2)
    {
        puts (" Usage: WAVEINFO {filename}");
        exit (1) ;
    }

    initparms.size = sizeof(CTA_INIT_PARMS);
    initparms.ctacompatlevel = CTA_COMPATLEVEL;
    ret = ctaInitialize(&Init_services,
                        0,               /* No service ! */
                        &initparms);
    if (ret != SUCCESS)
    {
        printf( "\007ctaInitialize error %d\n", ret);
        exit(1);
    }

    ret= ctaFindFile(argv[1], "wav", "CTA_DPATH", fullname, sizeof fullname);
    if (ret != SUCCESS)
    {
        printf( "\007Could not find %s. Error %s\n", argv[1], getErrorText(ret) );
        exit(1);
    }

    printf ("\n%s: \n", fullname) ;
    ShowWaveFile (fullname)  ;
    exit (0) ;
    return 0; /* never reached -- just to make the compiler happy */
}



/*--------------------------------------------------------------------------
  ShowWaveFile

  Opens a .WAV file, finds the WAVE header, and displays it.

  This function requires that the file contains a WAVE block and nothing else.
  --------------------------------------------------------------------------- */
void ShowWaveFile (char *filename)
{

    /* Wave header for PCM and non-PCM formats (adapted from MMREG.H) */
    typedef struct
    {
        WORD    wFormatTag;        /* format type */
        WORD    nChannels;         /* number of channels (i.e. mono, stereo...) */
        DWORD   nSamplesPerSec;    /* sample rate */
        DWORD   nAvgBytesPerSec;   /* for buffer estimation */
        WORD    nBlockAlign;       /* block size of data */
        WORD    wBitsPerSample;

        // the last 2 fields are not present in PCM format
        WORD    cbSize;
        BYTE    extra[80];         /* Possible extra data */
    } _PCM_WAVEFORMAT;

    /* Beginninng of any WAVE file */
    typedef struct
    {
        DWORD      riff ;             /* Must contain 'RIFF' */
        DWORD      riffsize ;         /* size of rest of RIFF chunk (entire file?) */
        DWORD      wave;              /* Must contain 'WAVE' */
    } _WAVEFILEHEADER ;

    FILE            *filep ;
    _WAVEFILEHEADER  filehdr ;
    _PCM_WAVEFORMAT  waveinfo = {0};
    int              fmtsize ;
    size_t           fmtwords ;
    int              bytes ;
    int              time ;

    filep = fopen (filename, "rb");
    if (filep == NULL)
    {
        perror ("Could not open file");
        return  ;
    }

    /* Expect the following structure at the beginning of the file
       Position         contents
       --------         --------
       0-3              'RIFF'
       4-7              size of the data that follows
       8-11             'WAVE'

       followed by a sequence of "chunks" containing:
       n-n+3             4 char chunk id
       n+4 - n+7         size of data
       n+8 - n+8+size    data

       A 'fmt' chunk is manadatory

       0-3             'fmt '
       4-7              size of the format block (16)
       8-23             wave header - see typedef above
       24 ..            option data for non-PCM formats

       A subsequent 'data' chunk is also mandatory

       0-3              'data'
       4-7               size of the data that follows
       8-size+8          data
     */

    if (fread (&filehdr, 1, sizeof filehdr, filep) != sizeof filehdr
        || strncmp ((char *)&filehdr.riff, "RIFF", 4) != 0
        || strncmp ((char *)&filehdr.wave, "WAVE", 4) != 0 )
    {
        printf("%s is not a simple WAVE file\n", filename) ;
        return ;
    }

    /* Scan for 'fmt' chunk */
    if ((fmtsize=scanforchunk ("fmt ", filep)) == -1)
    {
        printf("Unable to find 'fmt' chunk in %s\n", filename) ;
        return ;
    }

    /* Read the 'fmt' chunk (wave info block).  Read an even number of bytes
     * to ensure file is positioned at the next chunk.
     */
    fmtwords= (size_t)((fmtsize+1)/2) ;
    if (fmtsize > sizeof waveinfo
        ||  fread (&waveinfo, 2, fmtwords, filep) != fmtwords)
    {
        printf("Bad 'fmt' chunk in %s\n", filename) ;
        return ;
    }

#ifdef SOLARIS_SPARC
    /* the RIFF file formats are all little endian (Intel byte ordering format) */
    /* so we need to flip WORDS and DWORDS that have been read from the file */

    waveinfo.wFormatTag      = SWAPBYTES(waveinfo.wFormatTag);
    waveinfo.nChannels       = SWAPBYTES(waveinfo.nChannels);
    waveinfo.nSamplesPerSec  = SWAPDWORD(waveinfo.nSamplesPerSec);
    waveinfo.nAvgBytesPerSec = SWAPDWORD(waveinfo.nAvgBytesPerSec);
    waveinfo.nBlockAlign     = SWAPBYTES(waveinfo.nBlockAlign);
    waveinfo.wBitsPerSample  = SWAPBYTES(waveinfo.wBitsPerSample);
    waveinfo.cbSize          = SWAPBYTES(waveinfo.cbSize);
#endif

    /* Scan for 'data' chunk */
    if ((bytes=scanforchunk ("data", filep)) == -1)
    {
        printf("Unable to find 'data' chunk in %s\n", filename) ;
        return ;
    }

    if (waveinfo.nChannels != 1 && waveinfo.nChannels != 2 )
    {
        printf("Number of channels, %d, is invalid in %s\n",
               waveinfo.nChannels, filename) ;
        return ;
    }

    /* Compute time based on sample rate and bits per sample.  Use
     * nAvgBytesPerSec only if one of the those is 0, because nAvgBytesPerSec
     *  is not always correct.
     */

    if (waveinfo.wBitsPerSample !=0 && waveinfo.nSamplesPerSec != 0)
    {
        if (bytes < INT_MAX/100)
        {
            time  =  (bytes * 100)
                        /
                (waveinfo.nSamplesPerSec * waveinfo.wBitsPerSample
                  * waveinfo.nChannels / 8);
        }
        else
        {
            time  =  bytes /
                      ( (waveinfo.nSamplesPerSec * waveinfo.wBitsPerSample
                         * waveinfo.nChannels / 8) / 100 );
        }

    }
    else if (waveinfo.nAvgBytesPerSec != 0)
    {
        if (bytes < INT_MAX/100)
        {
            time =  (bytes * 100) / waveinfo.nAvgBytesPerSec;
        }
        else
        {
            time =  (bytes / (waveinfo.nAvgBytesPerSec / 100));
        }

    }
    else
    {
        time = 0;
    }

    if (waveinfo.wFormatTag == 1)
        printf ("   Format: PCM\n");
    else
        printf ("   Format: 0x%x\n", waveinfo.wFormatTag);
    printf ("   %s\n", waveinfo.nChannels == 1 ? "mono" : "stereo");
    printf ("   %d samples/second\n",  waveinfo.nSamplesPerSec );
    printf ("   %d bytes/second\n",    waveinfo.nAvgBytesPerSec );
    printf ("   %d byte(s)/block \n",  waveinfo.nBlockAlign);
    printf ("   %d bits/sample\n",     waveinfo.wBitsPerSample);
    printf ("   %d bytes, = %d.%02d seconds\n", bytes, time/100, time%100);

    if (waveinfo.wFormatTag != 1  && waveinfo.cbSize != 0)
    {
        int size = waveinfo.cbSize;
        BYTE *p = waveinfo.extra;
        printf ("   Additional data (hex):");
        while (size > 1)
        {
            printf(" %04x", *(WORD*)p);
            p += 2;
            size -= 2;
        }
        if (size)
        {
            printf(" %02x", *p);
        }
        printf("\n");
    }

    fclose (filep) ;
    return ;
}


/***********************************************************************
 * scanforchunk - returns chunk size or -1
 *
 * If successful, file position is at beginning of chunk data
 * On input, current file pos is assumed to be at the start of next chunk
 ***********************************************************************/
int scanforchunk (char *tag, FILE *filep)
{
    /* Chunk header */
    struct {
        DWORD   id ;          /* 4-char chunk id */
        DWORD   size ;        /* chunk size */
    } chunkhdr ;

    for (;;)
    {
        if (fread (&chunkhdr, 1, sizeof chunkhdr, filep) != sizeof chunkhdr)
            return -1 ;

#ifdef SOLARIS_SPARC
        chunkhdr.size = SWAPDWORD(chunkhdr.size);
#endif

        if (strncmp ((char *)&chunkhdr.id, tag, 4) == 0)
            break ;
        else
        {
            long size = ((chunkhdr.size +1) & ~1) ;   /* round up to even */
            if (fseek(filep, size, SEEK_CUR ) !=0)
                return -1 ;
        }
    }
    return  (int)chunkhdr.size ;
}


/*----------------------------- getErrorText ------------------------------*/
char *getErrorText(DWORD errcode )
{
    static char text[40];

    ctaGetText( NULL_CTAHD, errcode, text, sizeof(text) );
    return text ;
}

