/***********************************************************************
*  File - vmstat.cpp
*
*  Description - Functions needed to retrieve and print Video Mail
*                statistics
*
*  2005  NMS Communications Corporation.
*
***********************************************************************/

///////////////////
// Include Files //
///////////////////
#include "mega_global.h" /* MAX_GATEWAYS */
#include "vm_stat.h"

typedef struct {
	int audio_record;
	int video_record;
	int audio_play;
	int video_play;
	int errors;
	int warnings;
	hrtime_t average_duration;
	int   nb_calls;
} VM_PORT_STAT;

typedef struct {
	int nports;
	VM_PORT_STAT port[ MAX_GATEWAYS ];
} VM_SYS_STAT;

VM_SYS_STAT vm_sys_stat;

void initVmPortStat( int nPorts )
{
	int i;
	
	vm_sys_stat.nports = nPorts;
	for( i=0; i<vm_sys_stat.nports; i++ ) {	
		vm_sys_stat.port[i].audio_record = 0;
		vm_sys_stat.port[i].video_record = 0;
		vm_sys_stat.port[i].audio_play   = 0;
		vm_sys_stat.port[i].video_play   = 0;
		vm_sys_stat.port[i].errors       = 0;
		vm_sys_stat.port[i].warnings     = 0;					
		vm_sys_stat.port[i].average_duration     = 0;
		vm_sys_stat.port[i].nb_calls     = 0;
	} // end for
}

void printVmStat()
{
	int i;
	printf("\nPort| Audio Rec.| Video Rec.|Audio Play |Video Play |Errors  |Duration |calls\n");
	printf("-----------------------------------------------------------------------------\n");
	for( i=0; i<vm_sys_stat.nports; i++ ) {	
		printf(" %-2d | %-9d | %-9d | %-9d | %-9d | %-6d | %-7lld | %-6d \n", i,
			vm_sys_stat.port[i].audio_record, vm_sys_stat.port[i].video_record,
			vm_sys_stat.port[i].audio_play, vm_sys_stat.port[i].video_play,
			vm_sys_stat.port[i].errors, vm_sys_stat.port[i].average_duration, vm_sys_stat.port[i].nb_calls );			
	} // end for	
	printf("----------------------------------------------------------------------------\n\n");
}

void incAudRec( int i )
{
	vm_sys_stat.port[i].audio_record++;
}

void incrVideRec( int i )
{
	vm_sys_stat.port[i].video_record++;
}

void incAudPlay( int i )
{
	vm_sys_stat.port[i].audio_play++;
}

void incVidPlay( int i )
{
	vm_sys_stat.port[i].video_play++;	
}

void incErrors( int i )
{
	vm_sys_stat.port[i].errors++;
}

void updateDuration( int i , hrtime_t duration)
{
	int n;
	hrtime_t average;
	n = vm_sys_stat.port[i].nb_calls;
	average = vm_sys_stat.port[i].average_duration;
	
	if (n == 0)
	   vm_sys_stat.port[i].average_duration = duration;
	else
	   vm_sys_stat.port[i].average_duration = (n*average + duration) / (n+1);
	
	vm_sys_stat.port[i].nb_calls++;
	   
}
