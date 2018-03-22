/***************************************************************************
*  File - BOARDINF.C  -  Displays info about booted NMS boards.
*
*  Copyright (c) 1995-2002 NMS Communications.  All rights reserved.
***************************************************************************/
#define VERSION_NUM  "13"
#define VERSION_DATE __DATE__


char banner1[] =
"Natural Access Board Information Demo V.%s (%s)\n\n" ;
char banner2[] =
"                                                                     MVIP-95\n"
"Board  Addr   Type        MIPS    Free memory    Ports   DSP Slots   streams";
char banner3[] =
"-----  ----   ----        ----    -----------    -----   ---------   -------";
char banner4[] =
"                                                         ";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ctadef.h"
#include "adidef.h"

/* Borrowed from demolib.h: */
#if defined(WIN32)
  #define UNREACHED_STATEMENT( statement ) statement
#else
  #define UNREACHED_STATEMENT( statement )
#endif

#ifndef ARRAY_DIM
  #define ARRAY_DIM(a) (sizeof(a)/sizeof(*(a)))
#endif


/* ******************* globals ************************ */
char          *AdiMgr = "adimgr";


/* ******************* forward reference ************************ */
void Help();
void GetAndPrintError( CTAHD ctahd, DWORD err, char *funcname );
void CleanupAndExit( CTAQUEUEHD qhd, CTAHD ctahd );
void Initialize( CTAQUEUEHD *pqhd, CTAHD *pctahd );


/*----------------------------------------------------------------------------
 *                               Help
 *---------------------------------------------------------------------------*/
void Help()
    {
        printf( "Usage: boardinf [options]\n" );
        printf( "  where options are:\n" );
        printf( "    -A adi_mgr  Natural Access manager to use for ADI service.        "
                "Default = %s\n", AdiMgr );
        printf( "    -h          This help.\n\n" );
        exit (1);
    }

/*----------------------------------------------------------------------------
 *                               GetAndPrintError
 *---------------------------------------------------------------------------*/
void GetAndPrintError( CTAHD ctahd, DWORD err, char *funcname )
{
    char  textbuf[80] = "";
    ctaGetText( ctahd, err, textbuf, sizeof( textbuf ) );
    fprintf( stderr, "%s failed: %s (0x%x)\n", funcname, textbuf, err );
    return;
}

/*----------------------------------------------------------------------------
 *                               CleanupAndExit
 *---------------------------------------------------------------------------*/
void CleanupAndExit( CTAQUEUEHD qhd, CTAHD ctahd )
{
    if ( ctahd )
        ctaDestroyContext( ctahd );
    if ( qhd )
        ctaDestroyQueue( qhd );
    exit( -1 );
}

/*----------------------------------------------------------------------------
 *                               Initialize
 *---------------------------------------------------------------------------*/
void Initialize( CTAQUEUEHD *pqhd, CTAHD *pctahd )
{
    CTA_EVENT evt;
    DWORD ret;

    CTA_SERVICE_NAME service_names[] = { {"adi", ""} };
    CTA_SERVICE_DESC service_descs[] = { { {"adi", ""}, {0}, {0}, {0} } };

    CTA_INIT_PARMS   initparm   = {0};

    service_names[0].svcmgrname = AdiMgr;
    service_descs[0].name.svcmgrname = AdiMgr;
    service_descs[0].mvipaddr.board = ADI_AG_DRIVER_ONLY;

    initparm.size = sizeof( initparm );
    initparm.traceflags = 0;
    initparm.parmflags = CTA_PARM_MGMT_SHARED_IF_PRESENT;
    initparm.filename = NULL;
    initparm.ctacompatlevel = CTA_COMPATLEVEL;
    initparm.reserved = 0;

    ret = ctaInitialize( service_names, ARRAY_DIM(service_names), &initparm );
    if( ret != SUCCESS )
    {
        GetAndPrintError( NULL_CTAHD, ret, "ctaInitialize()" );
        exit(-1);
    }

    ret = ctaCreateQueue( NULL, 0, pqhd );
    if (ret != SUCCESS)
    {
        if ( ret == CTAERR_DRIVER_OPEN_FAILED )
        {
            fprintf( stderr, "Driver open failed.  Have you started agmon?" );
            exit(-1);
        }
        GetAndPrintError( NULL_CTAHD, ret, "ctaCreateQueue()" );
        exit( -1 );
    }

    ret = ctaCreateContext( *pqhd, 0, "brdinfo", pctahd );
    if (ret != SUCCESS)
    {
        GetAndPrintError( NULL_CTAHD, ret, "ctaCreateContext()" );
        CleanupAndExit( *pqhd, *pctahd );
    }

    ret = ctaOpenServices( *pctahd, service_descs, ARRAY_DIM(service_descs) );
    if ( ret != SUCCESS )
    {
        GetAndPrintError( *pctahd, ret, "ctaOpenServices()" );
        CleanupAndExit( *pqhd, *pctahd );
    }

    do
    {
        ctaWaitEvent( *pqhd, &evt, CTA_WAIT_FOREVER );
    } while ( evt.id != CTAEVN_OPEN_SERVICES_DONE );

    if ( evt.value != CTA_REASON_FINISHED )
    {
        GetAndPrintError( *pctahd, ret, "Open services" );
        CleanupAndExit( *pqhd, *pctahd );
    }

    return;
}

/*---------------------------------------------------------------------------
 *                               main()
 *--------------------------------------------------------------------------*/
int main ( int argc, char *argv[] )
{
    CTAQUEUEHD     qhd;
    CTAHD          ctahd;
    DWORD          board;
    unsigned       p_count = 0;

    /* Process command-line arguments.
     */
    while (--argc > 0)
    {
        if (**++argv == '/' || **argv == '-')
        {
            switch (*++*argv)
            {
            case 'A' :
                if (*++*argv != '\0')
                    AdiMgr = *argv;               /*  "-Amgr" */
                else if (--argc > 0)
                    AdiMgr = *++argv;             /*  "-A mgr" */
                else
                    Help();
                break;

            default:
                Help();
            }
        }
    }

    Initialize( &qhd, &ctahd );

    printf( banner1, VERSION_NUM, VERSION_DATE );
    puts ( banner2 );
    puts ( banner3 );


    for ( board=0; board<32; board++ )
    {
        unsigned       i;
        DWORD          ret;
        char           textbuf[80];

        /* board info variables. */
        ADI_BOARD_INFO boardinfo ;
        unsigned       numslots ;
        DWORD          slot1, slot2 ;
        unsigned       prevslot ;
        char          *type ;

        unsigned       pci;
        unsigned       m95stream  = 0;
        unsigned       position;
        char           outbuf[20];
        unsigned       nchar;

        ADI_TIMESLOT32 slotlist [512];  /* Returned array of timeslots */

        ret = adiGetBoardInfo( ctahd, board, sizeof boardinfo, &boardinfo );

        if ( ret == CTAERR_INVALID_BOARD )
        {
            continue;
        }

        if ( ret == CTAERR_DRIVER_OPEN_FAILED)
        {
            // temp? Until all boards are in the board # mapping database, must
            // assume all boards not defined are AG boards, although AG driver
            // may not even be active
            continue;
        }

        // Board is not invalid, but is indicated as "not found".
        // If this happens, it means board exists but is not started, and
        // system can't give us proper info on it to display.  So skip it.
        if ( ret == CTAERR_NOT_FOUND )
        {
            continue;
        }

        if ( ret != SUCCESS )
        {
           ctaGetText( ctahd, ret, textbuf, sizeof textbuf );
           fprintf( stderr, "\n%3d   adiGetBoardInfo() failed: %s (0x%x)\n",
                     board, textbuf, ret );
           continue ;
        }

        pci    = 0;
        switch ( boardinfo.boardtype )
        {
            case ADI_BOARDTYPE_QUADT1_CONNECT:
            case ADI_BOARDTYPE_AGQUADT1: type = "AG-QT";  m95stream = 16; break;
            case ADI_BOARDTYPE_QUADE1_CONNECT:
            case ADI_BOARDTYPE_AGQUADE1: type = "AG-QE";  m95stream = 16; break;
            case ADI_BOARDTYPE_AGDUALT1: type = "AG-DT";  m95stream = 16; break;
            case ADI_BOARDTYPE_AGDUALE1: type = "AG-DE";  m95stream = 16; break;
            case ADI_BOARDTYPE_CPCI_QUADT1: type = "AG-CPCIQT";  m95stream = 16;
                 break;
            case ADI_BOARDTYPE_CPCI_QUADE1: type = "AG-CPCIQE";  m95stream = 16;
                 break;
            case ADI_BOARDTYPE_AG2000  : type = "AG-2000";m95stream = 4; break;
            case ADI_BOARDTYPE_AG2000C : type = "AG-2000C";m95stream = 4; break;

            case ADI_BOARDTYPE_AG2000BRI: type = "AG-2000BRI"; m95stream = 4;
                break;

            case ADI_BOARDTYPE_AG4000_4T: type = "AG4000-4T";m95stream = 16;
                break;
            case ADI_BOARDTYPE_AG4000_4E: type = "AG4000-4E";m95stream = 16;
                break;
            case ADI_BOARDTYPE_AG4000_2T: type = "AG4000-2T";m95stream = 16;
                break;
            case ADI_BOARDTYPE_AG4000_2E: type = "AG4000-2E";m95stream = 16;
                break;
            case ADI_BOARDTYPE_AG4000_1T: type = "AG4000-1T";m95stream = 16;
                break;
            case ADI_BOARDTYPE_AG4000_1E: type = "AG4000-1E";m95stream = 16;
                break;
            case ADI_BOARDTYPE_AG4000C_4T: type = "AG4000C-4T";m95stream = 16;
                break;
            case ADI_BOARDTYPE_AG4000C_4E: type = "AG4000C-4E";m95stream = 16;
                break;
            case ADI_BOARDTYPE_AG4000C_2T: type = "AG4000C-2T";m95stream = 16;
                break;
            case ADI_BOARDTYPE_AG4000C_2E: type = "AG4000C-2E";m95stream = 16;
                break;
            case ADI_BOARDTYPE_QX2000:     type = "QX-2000" ;m95stream = 4;
                break;

            case ADI_BOARDTYPE_CG6000C_QUAD:
                            type = "CG6000C"   ; m95stream = 16; pci = 1;
                            break;
            case ADI_BOARDTYPE_CG6000:
                            type = "CG6000"    ; m95stream = 16; pci = 1;
                            break;
            case ADI_BOARDTYPE_CG6100C:
                            type = "CG6100C"   ; m95stream = 64; pci = 1;
                            break;
            case ADI_BOARDTYPE_CG6500C:
                            type = "CG6500C"   ; m95stream = 64; pci = 1;
                            break;
            case ADI_BOARDTYPE_CG6060:
                            type = "CG6060"   ; m95stream = 16; pci = 1;
                            break;
            case ADI_BOARDTYPE_CG6060C:
                            type = "CG6060C"   ; m95stream = 16; pci = 1;
                            break;
            case ADI_BOARDTYPE_CG6565:
                            type = "CG6565"   ; m95stream = 64; pci = 1;
                            break;
            case ADI_BOARDTYPE_CG6565C:
                            type = "CG6565C"   ; m95stream = 64; pci = 1;
                            break;
            case ADI_BOARDTYPE_CGHOST:
                            type = "CGHOST";
                            break;

            default:
            case ADI_BOARDTYPE_UNKNOWN : type = "Unknown"; break;
        }
        puts ( "" ) ;

        /*
         *  Read the configuration for the board. Passing 0 in the 'mode'
         *  (third) argument means get the DSP slots corresponding to actual
         *  line interfaces if applicable.
         */
        if ( adiGetBoardSlots32 ( ctahd, board, 0, ARRAY_DIM(slotlist),
                                  slotlist, &numslots ) != SUCCESS )
            continue ;

        if (boardinfo.ioaddr == 0)
        {
            /* No base I/O address; presumably a PCI board. */
            printf ( " %2d    %4s   %-11s %4d    %11d    %5d   ",
                     board, "- ",             type, boardinfo.totalmips,
                     boardinfo.freemem, numslots );
        }
        else if (pci)
        {
            /* Base I/O address is actually PCI bus and slot. */
            printf ( " %2d   %2d,%2d   %-11s %4d    %11d    %5d   ",
                     board, boardinfo.ioaddr >> 8, boardinfo.ioaddr & 0xff,
                     type, boardinfo.totalmips,
                     boardinfo.freemem, numslots );
        }
        else
        {
            printf ( " %2d    %4x   %-11s %4d    %11d   %5d   ",
                     board, boardinfo.ioaddr, type, boardinfo.totalmips,
                     boardinfo.freemem, numslots );
        }

        p_count += numslots ;

        if ( numslots == 0 )
        {
            continue ;
        }

        position = 0;
        nchar    = 0;
        i        = 0;
        while (i < numslots )
        {
            slot1  = slotlist[i].slot;
            prevslot = slot1;

            /* Identify continuous range of slots */
            while ( ++i < numslots && slotlist[i].slot == prevslot+1 )
            {
                prevslot++;
            }

            slot2 = slotlist[i-1].slot;

            nchar = 0;
            nchar = sprintf (outbuf+nchar, "%d", slot1 ) ;
            if ( slot2 != slot1 )
                nchar += sprintf (outbuf+nchar, "..%d", slot2 ) ;

            if (position+nchar > 10)
            {
                /* No more room for slots on this line,
                 * so print stream info + newline + indent for next line.
                 */
                printf ("%*c%d-%d\n%s", 12-position, ' ', m95stream,
                                                    m95stream+3, banner4);
                position = 0;
            }
            else if (position != 0)
                position += printf(",", outbuf);

            position += printf("%s", outbuf);
        }
        if (m95stream != 0)
        {
            printf ("%*c%d-%d", 12-position, ' ', m95stream, m95stream+3);
        }
        printf("\n");
    }

    printf( "\nTotal port count=%d\n", p_count ) ;

    exit( 0 );
    UNREACHED_STATEMENT( return 0; )
}
