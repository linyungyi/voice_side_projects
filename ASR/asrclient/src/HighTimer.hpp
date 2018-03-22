#ifndef __HIGHTIMER_HPP__
#define __HIGHTIMER_HPP__

#include <sys/time.h>

class CHighTimer
{
public:
    
    void 	vGetCurCount();
    double	dTakeTimeSecFrom(CHighTimer& ht);
    
protected:
    timeval  m_tCounter;
    //timezone m_tz;
};

#endif //__HIGHTIMER_HPP__
