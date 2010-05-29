#ifndef LIBCTB_WINDOWS_TIMER_H_INCLUDED_
#define LIBCTB_WINDOWS_TIMER_H_INCLUDED_

/////////////////////////////////////////////////////////////////////////////
// Name:        win32/timer.h
// Purpose:
// Author:      Joachim Buermann
// Copyright:   (c) 2010 Joachim Buermann
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include <windows.h>

namespace ctb {

    class Timer
    {
    protected:
	   unsigned int timeout_in_ms;
       unsigned int start_time;
    public:
	   Timer(unsigned int msec);
	   bool timeout();
    };

/*!
  \fn
  A plattform independent function, to go to sleep for the given
  time interval.
  \param ms time interval in milli seconds
*/
    void sleepms(unsigned int ms);
	
} // namespace ctb

#endif
