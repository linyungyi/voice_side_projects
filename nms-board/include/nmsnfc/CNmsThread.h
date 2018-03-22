#ifndef CNMSTHREAD_H
#define CNMSTHREAD_H
  
#ifndef   NMSNFCUTIL_H
#include <NmsNfcUtil.h>
#endif

#ifndef   CNMSSYNCOBJS_H
#include <CNmsSyncObjs.h>
#endif

#define NMSRUNTIME_CLASS( name )    new name
 
class _NMSNFCCLASS CNmsThread
{ 
public:
	// Do not use directly. Use NmsBeginThread insted of constructor
    CNmsThread();
    virtual ~CNmsThread();
    
    int     GetThreadPriority();
    BOOL    SetThreadPriority( int nPriority );
    BOOL    SuspendThread();
    BOOL    ResumeThread();

    void    EndThread();
	
protected:
    virtual DWORD   Run();
    virtual BOOL    InitInstance();
    virtual void    ExitInstance();
    virtual void    OnIdle(){};
    void            ExitThread( DWORD dwExitCode );

    BOOL    CreateThread(DWORD dwCreateFlags );

public:
    TSITHREADHDL    m_hThread;
    DWORD           m_dwCreateFlags;
    BOOL            m_bAutoDelete;

protected:
    CNmsEvent       m_evExit;
    BOOL            m_bExitThread;

private:
    DWORD   m_dwExitCode;

friend _NMSNFCCLASS CNmsThread* NmsBeginThread( CNmsThread*, int nPriority, 
                                              DWORD dwCreateFlags );
friend THREAD_RET_TYPE NMSAPI _NmsThreadEntry( void* pParam );
};
#endif // ifndef CNMSTHREAD_H //

