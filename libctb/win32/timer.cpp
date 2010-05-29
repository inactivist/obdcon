/////////////////////////////////////////////////////////////////////////////
// Name:        win32/timer.cpp
// Purpose:
// Author:      Joachim Buermann
// Id:          $Id: timer.cpp,v 1.2 2004/11/30 12:39:17 jb Exp $
// Copyright:   (c) 2001 Joachim Buermann
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
# ifndef DWORD_PTR 
#  define DWORD_PTR DWORD*
# endif
#endif

#include "timer.h"

namespace ctb {

    Timer::Timer(unsigned int msecs)
    {
		timeout_in_ms = msecs;
		start_time = GetTickCount();
    }

	bool Timer::timeout()
	{
		return (GetTickCount() - start_time >= timeout_in_ms);
	}

    void sleepms(unsigned int ms)
    {
	   Sleep(ms);
    };

} // namespace ctb
