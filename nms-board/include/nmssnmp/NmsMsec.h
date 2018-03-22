/* -*-C++-*- */
/*
  Copyright (c) 1996
  Hewlett-Packard Company

  ATTENTION: USE OF THIS SOFTWARE IS SUBJECT TO THE FOLLOWING TERMS.
  Permission to use, copy, modify, distribute and/or sell this software
  and/or its documentation is hereby granted without fee. User agrees
  to display the above copyright notice and this license notice in all
  copies of the software and any documentation of the software. User
  agrees to assume all liability for the use of the software; Hewlett-Packard
  makes no representations about the suitability of this software for any
  purpose. It is provided "AS-IS without warranty of any kind,either express
  or implied. User hereby grants a royalty-free license to any and all
  derivatives based upon this software code base.
*/
#ifndef _MSEC
#define _MSEC

//----[ includes ]----------------------------------------------------- 
extern "C" 
{
#if defined (UNIX)
    #include <sys/types.h>          // NOTE:  due to 10.10 bug, order is important
                                    //   in that all routines must include types.h
                                    //   and time.h in same order otherwise you
                                    //   will get conflicting definitions of 
                                    //   "fd_set" resulting in link time errors.
    #include <sys/time.h>
    #include <sys/param.h>
#elif defined (WIN32)
    #ifndef _WINSOCKAPI_
    #include <winsock.h>
    #endif
#endif
}
#include <nmstypes.h>

//----[ defines ]------------------------------------------------------
#define MSECOUTBUF 20

//----[ msec class ]--------------------------------------------------- 
class msec {
public:
  msec(void);
  msec(const msec &in_msec);

  friend int operator==(const msec &t1, const msec &t2);
  friend int operator!=(const msec &t1, const msec &t2);
  friend int operator<=(const msec &t1, const msec &t2);
  friend int operator>=(const msec &t1, const msec &t2);
  friend int operator<(const msec &t1, const msec &t2);
  friend int operator>(const msec &t1, const msec &t2);
  msec &operator-=(const INT32 millisec);
  msec &operator-=(const timeval &t1);
  msec &operator+=(const INT32 millisec);
  msec &operator+=(const timeval &t1);
  msec &operator=(const msec &t1);
  msec &operator=(const timeval &t1);
  operator DWORD() const;
  void refresh();
  void SetInfinite();
  int IsInfinite() const;
  void GetDelta(const msec &future, timeval &timeout) const;
  void GetDeltaFromNow(timeval &timeout) const;
  char *get_printable();
  
private:
  timeval m_time;
  char m_output_buffer[MSECOUTBUF];
};

#endif // _MSEC

