#ifndef CNMSSYNCOBJS_H
#define CNMSSYNCOBJS_H

#ifndef   NMSNFCDEFS_H
#include <NmsNfcDefs.h>
#endif

#ifndef   CNMSSTRING_H
#include <CNmsString.h>
#endif


class _NMSNFCCLASS CNmsSyncObject
{
public:
    CNmsSyncObject( const char *pstrName = 0 );
    virtual ~CNmsSyncObject();

    virtual BOOL Lock( TIMEOUT Timeout = INFINITE ) = 0;
    virtual BOOL Unlock() {return FALSE;};  //YM 12/10/02 to fix NA2003-bug25
    virtual void Trace() const = 0;

protected:
    CNmsString  m_strName;

friend class CNmsSingleLock;
};

#define NMS_WAIT_OBJECT_0       ((DWORD   )0x00000000L)
#define NMS_WAIT_FAILED         ((DWORD   )0xFFFFFFFFL)
#define NMS_WAIT_ABANDONED      ((DWORD   )0x00000080L)
#define NMS_WAIT_TIMEOUT        ((DWORD   )0x00000102L)

class _NMSNFCCLASS CNmsMutex : public CNmsSyncObject
{
public:
    CNmsMutex( BOOL bInitiallyOwn = FALSE, LPCSTR lpszName = NULL, BOOL bAttachOnly = FALSE );
  virtual ~CNmsMutex();

  virtual BOOL  Lock( TIMEOUT Timeout = INFINITE );
  virtual BOOL  Unlock();
  operator      TSIMTXHDL() const;

  virtual void  Trace() const;

private:
  void reportFatalError();

private:
    TSIMTXHDL   m_hObject;
    BOOL        m_bUseLightMutex;
};




class _NMSNFCCLASS CNmsEvent : public CNmsSyncObject
{
public:
    CNmsEvent( BOOL bInitiallyOwn = FALSE, BOOL bManualReset = FALSE, LPCSTR lpszName = NULL );
    CNmsEvent( const CNmsEvent& );
    CNmsEvent( TSIEVNHDL hTsiEvent );

  virtual ~CNmsEvent();

  virtual BOOL  Lock( TIMEOUT Timeout = INFINITE );
  virtual BOOL  Unlock();
  operator      TSIEVNHDL() const;

  virtual void  Trace() const;
  CNmsEvent&    operator = ( const CNmsEvent& src );

    BOOL SetEvent();
    BOOL ResetEvent();
    BOOL IsManual();

private:
  void reportFatalError();


protected:
    TSIEVNHDL           m_hObject;
    BOOL                m_bManualReset;
    BOOL                m_bAutoDelete;
    CNmsString          m_szEvName;
 friend DWORD _NMSNFCCLASS SyncWaitForSingleEvent( CNmsEvent& object, TIMEOUT dwMilliseconds );
 friend DWORD _NMSNFCCLASS SyncWaitForMultipleEvents( DWORD nCount, CNmsEvent* events,
                                                    TIMEOUT dwMilliseconds );
 friend DWORD _NMSNFCCLASS SyncWaitForMultipleEventsPtr( DWORD nCount, CNmsEvent** events,
                                                    TIMEOUT dwMilliseconds );
};

class _NMSNFCCLASS CNmsSingleLock
{
public:
    CNmsSingleLock( CNmsSyncObject* pObject, BOOL bInitialLock = FALSE );
  virtual ~CNmsSingleLock();

    BOOL    Lock( TIMEOUT dwTimeOut = INFINITE );
    BOOL    Unlock();
    BOOL    IsLocked() const;

protected:
    CNmsSyncObject* m_pObject;
    BOOL            m_bAcquired;
};




#endif // ifndef CNMSSYNCOBJS_H //

