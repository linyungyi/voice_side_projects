#ifndef _VM_STAT_INCLUDE
#define _VM_STAT_INCLUDE

void initVmPortStat( int nPorts );
void printVmStat();
void incAudRec( int i );
void incrVideRec( int i );
void incAudPlay( int i );
void incVidPlay( int i );
void incErrors( int i );

void updateDuration( int i , hrtime_t duration);
#endif
