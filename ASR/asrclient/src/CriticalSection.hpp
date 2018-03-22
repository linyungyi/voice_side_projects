// Criticalsection Warp class , auto unlock in ~()
// by owenyang
#ifndef __CRITICALSECTION_H__
#define __CRITICALSECTION_H__

#include <pthread.h>


//////////////////////////////
// for windows compatible
#define CRITICAL_SECTION pthread_mutex_t

#define InitializeCriticalSection(pCriticalSection) pthread_mutex_init(pCriticalSection,NULL)
#define DeleteCriticalSection(pCriticalSection) pthread_mutex_destroy(pCriticalSection)

#define EnterCriticalSection(pCriticalSection) pthread_mutex_lock(pCriticalSection)
#define LeaveCriticalSection(pCriticalSection) pthread_mutex_unlock(pCriticalSection)


class  CCriticalsection
{
public:
	CCriticalsection() 	{pthread_mutex_init(&m_CritSect,NULL);};
	~CCriticalsection()	{pthread_mutex_destroy(&m_CritSect);};

	void Lock() 		{pthread_mutex_lock(&m_CritSect);};
	void Unlock()		{pthread_mutex_unlock(&m_CritSect);};

protected:

	pthread_mutex_t 	m_CritSect;
};

class CLock
{
public:
	CLock(CCriticalsection* pCritsect){m_pCritSect=pCritsect;m_pCritSect->Lock();};
	~CLock(){m_pCritSect->Unlock();};

	void Lock(){m_pCritSect->Lock();};
	void Release(){m_pCritSect->Unlock();};

protected:
	CCriticalsection* 	m_pCritSect;


};
#endif //__CRITICALSECTION_H__
