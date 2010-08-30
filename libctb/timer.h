#ifndef LIBCTB_TIMER_H_INCLUDED_
#define LIBCTB_TIMER_H_INCLUDED_

/////////////////////////////////////////////////////////////////////////////
// Name:        timer.h
// Purpose:     simple wrapper file
// Author:      Joachim Buermann
// Copyright:   (c) 2010 Joachim Buermann
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#if defined (WIN32) || defined (_WIN32_WCE)
# include "win32/timer.h"
#else
# include "linux/timer.h"
#endif

#endif

