/*************************************************************************
* libobd - The OBD-II access library
* Distributed under MPL 1.1
*
* Copyright (c) 2010 Stanley Huang <stanleyhuangyc@gmail.com>
* All rights reserved.
**************************************************************************/

#ifndef _LIBOBD_H
#define _LIBOBD_H

#include <string>
#include "ctb.h"

#define QUERY_INTERVAL 200
#define QUERY_INTERVAL_MIN 50
#define QUERY_INTERVAL_MAX 500
#define QUERY_INTERVAL_STEP 25
#define ADAPT_PERIOD  30000 /* ms */

typedef struct {
	DWORD time;
	int value;
} PID_DATA;

typedef struct {
	int pid;
	int dataBytes;
	int priority;
	const char* name;
	int active;
	PID_DATA data;
} PID_INFO;

#define PID_RPM 0x010C
#define PID_SPEED 0x010D
#define PID_THROTTLE 0x0111 
#define PID_LOAD 0x0104
#define PID_COOLANT_TEMP 0x0105
#define PID_FUEL_SHORT_TERM 0x0106
#define PID_FUEL_LONG_TERM 0x0107
#define PID_INTAKE_TEMP 0x010F

#define INVALID_PID_DATA 0x80000000

class COBD;

class COBD
{
public:
	COBD():connected(false),running(true),lastTick(0),updateInterval(QUERY_INTERVAL),updateFlags(PID_RPM | PID_SPEED) {}
	~COBD() { Uninit(); }
	int GetSensorData(int id);
	char* SendCommand(std::string cmd, char* lookfor = 0, bool readall = false);
	DWORD Update();
	bool Init(const TCHAR* devname, int baudrate, const char* protocol);
	void Uninit();
	void Wait(int interval);
	static PID_INFO* GetPidInfo(int pid);
	static PID_INFO* GetPidInfo(const char* name);
	bool connected;
	DWORD updateFlags;
	bool running;
private:
	bool RetrieveSensor(int pid, PID_DATA& data);
	DWORD lastTick;
	DWORD startTime;
	int updateInterval;
	ctb::IOBase* device;
};

#ifdef WINCE
#define MSG(x) MessageBox(0, TEXT(x), 0, MB_OK);
#else
#define MSG(x) fprintf(stderr, "%s\n", x);
#endif

#endif
