/////////////////////////////////////////////////////////////////////////////
// Name:        serport.cpp
// Purpose:
// Author:      Joachim Buermann
// Id:          $Id: serport.cpp,v 1.1.1.1 2004/11/24 10:30:11 jb Exp $
// Copyright:   (c) 2001 Joachim Buermann
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <string.h>
#include "serport.h"

#define SERIALPORT_BUFSIZE 4096

namespace ctb {
#ifdef WINCE
	const TCHAR* COM1 = TEXT("COM1:");
    const TCHAR* COM2 = TEXT("COM2:");
    const TCHAR* COM3 = TEXT("COM3:");
    const TCHAR* COM4 = TEXT("COM4:");
    const TCHAR* COM5 = TEXT("COM5:");
    const TCHAR* COM6 = TEXT("COM6:");
    const TCHAR* COM7 = TEXT("COM7:");
    const TCHAR* COM8 = TEXT("COM8:");
#else
    const char* COM1 = "com1";
    const char* COM2 = "com2";
    const char* COM3 = "com3";
    const char* COM4 = "com4";
    const char* COM5 = "com5";
    const char* COM6 = "com6";
    const char* COM7 = "com7";
    const char* COM8 = "\\\\.\\com8";
#endif

    SerialPort::SerialPort()
    {
	   fd = INVALID_HANDLE_VALUE;
	   m_rtsdtr_state = LinestateNull;
    };

    SerialPort::~SerialPort()
    {
	   Close();
    };

    int SerialPort::CloseDevice()
    {
	   if(fd != INVALID_HANDLE_VALUE) {
		  CloseHandle(fd);
		  fd = INVALID_HANDLE_VALUE;
	   }
	   return 0;
    };

    int SerialPort::ChangeLineState( SerialLineState flags )
    {
	   BOOL ok = FALSE;
	   if(flags & LinestateDtr) {
		  if(m_rtsdtr_state & LinestateDtr) {
			 ok = EscapeCommFunction(fd,CLRDTR);
		  }
		  else {
			 ok = EscapeCommFunction(fd,SETDTR);
		  }
		  m_rtsdtr_state ^= LinestateDtr;
	   }
	   if(flags & LinestateRts) {
		  if(m_rtsdtr_state & LinestateRts) {
			 ok = EscapeCommFunction(fd,CLRRTS);
		  }
		  else {
			 ok = EscapeCommFunction(fd,SETRTS);
		  }
		  m_rtsdtr_state ^= LinestateRts;
	   }
	   if(!ok) return -1;
	   return 0;
    };

    int SerialPort::ClrLineState( SerialLineState flags )
    {
	   BOOL ok = false;
	   if(flags & LinestateDtr) {
		  ok = EscapeCommFunction(fd,CLRDTR);
		  m_rtsdtr_state &= ~LinestateDtr;
	   }
	   if(flags & LinestateRts) {
		  ok = EscapeCommFunction(fd,CLRRTS);
		  m_rtsdtr_state &= ~LinestateRts;
	   }
	   if(!ok) return -1;
	   return 0;
    };

    int SerialPort::GetLineState()
    {
	   BOOL ok = false;
	   DWORD stat;
	   int flags = 0;
	   if(GetCommModemStatus(fd,&stat)) {
		  if(stat & MS_CTS_ON) 
			 flags |= LinestateCts;
		  if(stat & MS_DSR_ON) 
			 flags |= LinestateDsr;
		  if(stat & MS_RING_ON) 
			 flags |= LinestateRing;
		  if(stat & MS_RLSD_ON) 
			 flags |= LinestateDcd;
		  ok = true;
	   }
	   if(!ok) return -1;
	   return flags;
    };

    int SerialPort::Ioctl(int cmd,void* args)
    {
	   COMSTAT comstat;
	   DWORD errors;
	   int result = 0;
	   switch(cmd) {
	   case CTB_RESET:
		  return SendBreak(0);
	   case CTB_SER_GETEINFO:
		  if(ClearCommError(fd,&errors,&comstat)) {
			 // actualize the last events
			 if(errors & CE_BREAK) einfo.brk++;
			 if(errors & CE_FRAME) einfo.frame++;
			 if(errors & CE_OVERRUN) einfo.overrun++;
			 if(errors & CE_RXPARITY) einfo.parity++;
			 *(SerialPort_EINFO*)args = einfo;
			 return 0;
		  }
	   case CTB_SER_GETBRK:
		  if(ClearCommError(fd,&errors,&comstat)) {
			 if(errors & CE_BREAK) result = 1;
			 einfo.brk += result;
			 *(int*)args = result;
			 return 0;
		  }
		  break;
	   case CTB_SER_GETFRM:
		  if(ClearCommError(fd,&errors,&comstat)) {
			 if(errors & CE_FRAME) result = 1;
			 einfo.frame += result;
			 *(int*)args = result;
			 return 0;
		  }
	   case CTB_SER_GETOVR:
		  if(ClearCommError(fd,&errors,&comstat)) {
			 if(errors & CE_OVERRUN) result = 1;
			 einfo.overrun += result;
			 *(int*)args = result;
			 return 0;
		  }
		  break;
	   case CTB_SER_GETPAR:
		  if(ClearCommError(fd,&errors,&comstat)) {
			 if(errors & CE_RXPARITY) result = 1;
			 einfo.parity += result;
			 *(int*)args = result;
			 return 0;
		  }
		  break;
	   case CTB_SER_GETINQUE:
		  if(ClearCommError(fd,&errors,&comstat)) {
			 *(int*)args = (int)comstat.cbInQue;
			 return 0;
		  }
		  break;
	   case CTB_SER_SETPAR:
		  return SetParityBit( *(int*)args == 1 );
	   }
	   // error or unknown command
	   return -1;
    };

    int SerialPort::IsOpen()
    {
	   return (fd != INVALID_HANDLE_VALUE);
    };

    int SerialPort::OpenDevice(const TCHAR* devname, void* dcs)
    {
	   // if dcs isn't NULL, type cast
	   if(dcs) m_dcs = *(SerialPort_DCS*)dcs;

	   fd = CreateFile(devname,	// device name
				    GENERIC_READ | GENERIC_WRITE,	// O_RDWR
				    0,		// not shared
				    NULL,	// default value for object security ?!?
				    OPEN_EXISTING, // file (device) exists
					0,
				    NULL); // no more handle flags
	
	   if(fd == INVALID_HANDLE_VALUE) {
		  return -1;
	   }
	   // save the device name
	   strncpy(m_devname,(char*)devname,sizeof(m_devname));
	   // we write an eos to avoid a buffer overflow
	   m_devname[sizeof(m_devname)-1] = '\0';

	   // device control block
	   DCB dcb;
	   memset(&dcb,0,sizeof(dcb));
	   dcb.DCBlength = sizeof(dcb);
	   dcb.BaudRate = m_dcs.baud;
	   dcb.fBinary = 1;

	   m_rtsdtr_state = LinestateNull;

	   // Specifies whether the CTS (clear-to-send) signal is monitored 
	   // for output flow control. If this member is TRUE and CTS is turned
	   // off, output is suspended until CTS is sent again.
	   dcb.fOutxCtsFlow = m_dcs.rtscts;

	   // Specifies the DTR (data-terminal-ready) flow control. 
	   // This member can be one of the following values:
	   // DTR_CONTROL_DISABLE   Disables the DTR line when the device is 
	   //                       opened and leaves it disabled. 
	   // DTR_CONTROL_ENABLE    Enables the DTR line when the device is 
	   //                       opened and leaves it on. 
	   // DTR_CONTROL_HANDSHAKE Enables DTR handshaking. If handshaking is 
	   //                       enabled, it is an error for the application
	   //                       to adjust the line by using the 
	   //                       EscapeCommFunction function.  
	   dcb.fDtrControl = DTR_CONTROL_DISABLE;
	   m_rtsdtr_state |= LinestateDtr;
	   // Specifies the RTS flow control. If this value is zero, the
	   // default is RTS_CONTROL_HANDSHAKE. This member can be one of
	   // the following values:
	   // RTS_CONTROL_DISABLE   Disables the RTS line when device is
	   //                       opened and leaves it disabled.
	   // RTS_CONTROL_ENABLED   Enables the RTS line when device is
	   //                       opened and leaves it on.
	   // RTS_CONTROL_HANDSHAKE Enables RTS handshaking. The driver
	   //                       raises the RTS line when the
	   //                       "type-ahead" (input)buffer is less than
	   //                       one-half full and lowers the RTS line
	   //                       when the buffer is more than three-quarters
	   //                       full. If handshaking is enabled, it is an
	   //                       error for the application to adjust the
	   //                       line by using the EscapeCommFunction function.
	   // RTS_CONTROL_TOGGLE    Specifies that the RTS line will be high if 
	   //                       bytes are available for transmission. After
	   //                       all buffered bytes have been send, the RTS
	   //                       line will be low.
	   if(m_dcs.rtscts) dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
	   else {
		  dcb.fRtsControl = RTS_CONTROL_DISABLE;
		  m_rtsdtr_state |= LinestateRts;
	   }
	   // Specifies the XON/XOFF flow control.
	   // If fOutX is true (the default is false), transmission stops when the
	   // XOFF character is received and starts again, when the XON character
	   // is received.
	   dcb.fOutX = m_dcs.xonxoff;
	   // If fInX is true (default is false), the XOFF character is sent when
	   // the input buffer comes within XoffLim bytes of being full, and the
	   // XON character is sent, when the input buffer comes within XonLim
	   // bytes of being empty.
	   dcb.fInX = m_dcs.xonxoff;
	   // default character for XOFF is 0x13 (hex 13)
	   dcb.XoffChar = 0x13;
	   // default character for XON is 0x11 (hex 11)
	   dcb.XonChar = 0x11;
	   // set the minimum number of bytes allowed in the input buffer before
	   // the XON character is sent (3/4 of full size)
	   dcb.XonLim = (SERIALPORT_BUFSIZE >> 2) * 3;
	   // set the maximum number of free bytes in the input buffer, before the
	   // XOFF character is sent (3/4 of full size)
	   dcb.XoffLim = (SERIALPORT_BUFSIZE >> 2) * 3;

	   // parity
	   switch( m_dcs.parity ) {

	   case PARITY_ODD: dcb.Parity = ODDPARITY; break;
	   case PARITY_EVEN: dcb.Parity = EVENPARITY; break;
	   case PARITY_MARK: dcb.Parity = MARKPARITY; break;
	   case PARITY_SPACE: dcb.Parity = SPACEPARITY; break;
	   default: dcb.Parity = NOPARITY;

	   }
	   // stopbits
	   if(m_dcs.stopbits == 2) dcb.StopBits = TWOSTOPBITS;
	   else dcb.StopBits = ONESTOPBIT;
	   // wordlen, valid values are 5,6,7,8
	   dcb.ByteSize = m_dcs.wordlen;

	   if(!SetCommState(fd,&dcb))
		  return -2;

	   /* THIS IS OBSOLETE!!!
	   // event should be triggered, if there are some received data
	   if(!SetCommMask(fd,EV_RXCHAR))
	   return -4;
	   */

	   COMMTIMEOUTS cto = {MAXDWORD,0,0,0,0};
	   if(!SetCommTimeouts(fd,&cto))
		  return -5;

	   // for a better performance with win95/98 I increased the internal
	   // buffer to SERIALPORT_BUFSIZE (normal size is 1024, but this can 
	   // be a little bit to small, if you use a higher baudrate like 115200)
	   if(!SetupComm(fd,SERIALPORT_BUFSIZE,SERIALPORT_BUFSIZE))
		  return -6;

	   // clear the internal error struct
	   memset(&einfo,0,sizeof(einfo));
	   return 0;
    };

    int SerialPort::Read(char* buf,size_t len)
    {
	   DWORD read;
	   int m = m_fifo->items();
	   while(len) {
		  if(m_fifo->get(buf) == 1) {
			 len--;
			 buf++;
		  }
		  else {
			 break;
		  }
	   }
	   if(!ReadFile(fd,buf,len,&read,0)) {
		  // if we use a asynchrone reading, ReadFile gives always
		  // FALSE
		  // ERROR_IO_PENDING means ok, other values show an error
		 return -1;
	   }
	   else {
		  // ok, we have read all wanted bytes
		  return (int)read + m;
	   }
    };

    int SerialPort::SendBreak(int duration)
    {
	   if(duration <= 0) duration = 1;
	   if(!SetCommBreak(fd)) return -1;
	   // win32 Sleep parameter is ms
	   Sleep(duration * 250);
	   if(!ClearCommBreak(fd)) return -1;
	   // no error
	   return 0;
    };

/*
  FIXME! : We need some additional code to check the success of the
  baudrate modulation (non-standard rates depend on the used
  UART chipset).		 
*/
    int SerialPort::SetBaudrate( int baudrate )
    {
	   DCB dcb;

	   // get the current dcb...
	   if(!GetCommState(fd,&dcb)) {
		  return -1;
	   }
	   dcb.BaudRate = baudrate;
	   // and write it back
	   if(!SetCommState(fd,&dcb)) {
		  return -1;
	   }
	   m_dcs.baud = baudrate;
	   return 0;
    }

    int SerialPort::SetLineState( SerialLineState flags )
    {
	   BOOL ok = false;
	   if(flags & LinestateDtr) {
		  ok = EscapeCommFunction(fd,SETDTR);
		  // save the new state
		  m_rtsdtr_state |= LinestateDtr;
	   }
	   if(flags & LinestateRts) {
		  ok = EscapeCommFunction(fd,SETRTS);
		  // save the new state
		  m_rtsdtr_state |= LinestateRts;
	   }
	   if(!ok) return -1;
	   return 0;
    };

    int SerialPort::SetParityBit( bool parity )
    {
	   DCB dcb;

	   if( ! GetCommState( fd, &dcb ) ) {

		  return -1;

	   }	   

	   parity ? dcb.Parity = MARKPARITY : dcb.Parity = SPACEPARITY;

	   if( ! SetCommState( fd, &dcb ) ) {

		  return -2;

	   }
	   return 0;
    }

    int SerialPort::Write(char* buf,size_t len)
    {
	   DWORD write;
	   if(!WriteFile(fd,buf,len,&write,0)) {
			 return -1;
		}
		  else {
			 // VERY IMPORTANT to flush the data out of the internal
			 // buffer
			 FlushFileBuffers(fd);
			 // first you must call GetOverlappedResult, then you
			 // get the REALLY transmitted count of bytes
			return (int)write;
	   }
    };

} // namespace ctb
