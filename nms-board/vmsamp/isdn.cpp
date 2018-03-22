/***********************************************************************
*  File - isdn.cpp
*
*  Description - Functions needed to tart and stop isdn channels
*
*  2005  NMS Communications Corporation.
*
***********************************************************************/

///////////////////
// Include Files //
///////////////////
#include "mega_global.h"
#include "gw_types.h"    // gwParm
#include "vm_trace.h"

char    errortext[120];

///////////////////////////////////////////////////////////////////////////////
//   Procedure : isdnInit
//
//   Description : Function to initialize the ISDN stack for one NAI               
///////////////////////////////////////////////////////////////////////////////
int isdnInit( ISDN_CX *pIsdnCX )
{
    DWORD               ret, i;
    //char                contextname[12];
    CTA_EVENT           event = { 0};
    CTA_SERVICE_DESC    Services[] =        /* for ctaOpenServices */
    {   { {"ADI",  "ADIMGR"}, { 0 }, { 0 }, { 0 } },
        { {"ISDN", "ADIMGR"}, { 0 }, { 0 }, { 0 } },
    };
    unsigned            numservices = sizeof(Services)/sizeof(Services[0]);

    
    // Create a context 
    sprintf( pIsdnCX->szDescriptor, "isdn_%d_%d", pIsdnCX->dBoard, pIsdnCX->dNai );
    if ( ctaCreateContext(  pIsdnCX->QueueHd, 
                            pIsdnCX->dNai+1, 
                            pIsdnCX->szDescriptor, 
                            &pIsdnCX->ctahd ) != SUCCESS )
    {
        printf( "Create isdn context failed, invalid queue\n" );
        return FAILURE;
    }

    // fill in service description and open services 
    for ( i = 0; i < numservices; i++ )
    {  
        Services[i].mvipaddr.board = pIsdnCX->dBoard;
        Services[i].mvipaddr.mode = 0;
    }
    ctaOpenServices( pIsdnCX->ctahd, Services, numservices );

	do
	{
		ret = ctaWaitEvent( pIsdnCX->QueueHd, &event, CTA_WAIT_FOREVER);
		if (ret != SUCCESS) {
			mmPrintErr("isdnInit, ctaWaitEvent returned 0x%x\n", ret);
			return ret;
		}

		if( event.id != CTAEVN_OPEN_SERVICES_DONE ) {
			mmPrintErr("isdnInit, waiting for CTAEVN_OPEN_SERVICES_DONE, received id 0x%x\n",
			       event.id );
		}
	} while (event.id != CTAEVN_OPEN_SERVICES_DONE);	
	
    if ( event.value != CTA_REASON_FINISHED ) {
        mmPrintErr( "Open services failed. 0x%x\n", event.value );
        return FAILURE;
    }

    // start ISDN stack 
    ret = isdnStartProtocol(   pIsdnCX->ctahd, 
			                   ISDN_PROTOCOL_CHANNELIZED,
							   pIsdnCX->dOperator,
							   pIsdnCX->dCountry,
							   pIsdnCX->dNT_TE,
							   pIsdnCX->dNai,
						       NULL );
    if (ret != SUCCESS)
    {
        ctaGetText( pIsdnCX->ctahd, ret, errortext, 40 );
        printf("isdnStartProtocol failure: %s\n",errortext);
        return FAILURE;
    }
	
	do
	{
		ret = ctaWaitEvent( pIsdnCX->QueueHd, &event, CTA_WAIT_FOREVER);
		if (ret != SUCCESS) {
			mmPrintErr("isdnInit, ctaWaitEvent returned 0x%x\n", ret);
			return ret;
		}

		if( event.id != ISDNEVN_START_PROTOCOL ) {
			mmPrintErr("isdnInit, waiting for ISDNEVN_START_PROTOCOL, received id 0x%x\n",
			       event.id );
		}
	} while (event.id != ISDNEVN_START_PROTOCOL);	
	
    if ( event.value != SUCCESS) {
        mmPrintErr("isdnStartProtocol failed to complete successfully on NAI %d\n", pIsdnCX->dNai);
        return FAILURE;
    }

    return SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//   Procedure : isdnInit
//
//   Description : Function to initialize all ISDN stacks for the application
//                 Currently, all values are hard coded.
/////////////////////////////////////////////////////////////////////////////// 
int initAllIsdn( void )
{
    int  i, ret;
	CTAQUEUEHD isdn_qhd;

    ret = ctaCreateQueue(NULL, 0, &isdn_qhd);
	if( ret != SUCCESS ) {
       	printf("\nFailed to create CT Access queue, return code is %x", ret);
        exit( 1 );
   	}      

	printf("ISDN queue created successfully. qhd=0x%x\n", isdn_qhd);

    for(i=0; i< MAX_NAI; i++)
    {
        if( strcmp(gwParm.sIfcType, "E1") == 0  ) {
    		isdnCX[i].dNai  = gwParm.mux_timeslot/30;        
        } else if( strcmp(gwParm.sIfcType, "T1") == 0  ) {
    		isdnCX[i].dNai  = gwParm.mux_timeslot/23;                    
        } else {
            printf("ERROR - invalid interface type ==> %s\n", gwParm.sIfcType );
            exit(1);
        }
        
        isdnCX[i].dBoard    = CmdLineParms.nBoardNum;
        isdnCX[i].dCountry  = gwParm.isdn_country;
        isdnCX[i].dNT_TE    = gwParm.isdn_dNT_TE;
        isdnCX[i].dOperator = gwParm.isdn_operator;
        isdnCX[i].QueueHd   = isdn_qhd;

        ret = isdnInit( &isdnCX[i] );

    }
    return SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//   Procedure : isdnStop
//   Description : Stops  all the ISDN Stacks on a board 
///////////////////////////////////////////////////////////////////////////////
int isdnStop( void )
{
    DWORD ret, i;
    CTA_EVENT event = { 0 };
    int     return_code = SUCCESS;

    for( i=0; i<MAX_NAI; i++ )
    {

	    mmPrintAll("Stopping ISDN Protocol for context %s\n", isdnCX[i].szDescriptor);
        ret = isdnStopProtocol(   isdnCX[i].ctahd );
        if (ret != SUCCESS)
        {
            ctaGetText( isdnCX[i].ctahd, ret, errortext, 40 );
            mmPrintErr("isdnStopProtocol failure: %s\n",errortext);
            return_code = FAILURE;
        }
        else
        {
		
			do
			{
				ret = ctaWaitEvent( isdnCX[i].QueueHd, &event, 3000);
				if (ret != SUCCESS) {
					mmPrintErr("isdnStop, ctaWaitEvent returned 0x%x\n", ret);
					return ret;
				}
		
				if( event.id != ISDNEVN_STOP_PROTOCOL ) {
					mmPrintErr("isdnStop, waiting for ISDNEVN_STOP_PROTOCOL, received id 0x%x\n",
					       event.id );
				}
			} while (event.id != ISDNEVN_STOP_PROTOCOL && event.id != CTAEVN_WAIT_TIMEOUT);	
		
		
            if ( event.value != SUCCESS ) {
                mmPrintErr( "isdnStopProtocol failed to complete successfully for %s\n",
                    isdnCX[i].szDescriptor);
                return_code = FAILURE;
            }  else {
                mmPrintAll("isdnStopProtocol completed successfully for %s\n",
                          	  isdnCX[i].szDescriptor);
            }
        }
    }

    return return_code;
}
