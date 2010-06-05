/*************************************************************************
* libobd - The OBD-II access library
* Distributed under MPL 1.1
*
* Copyright (c) 2010 Stanley Huang <stanleyhuangyc@gmail.com>
* All rights reserved.
**************************************************************************/

#include <stdio.h>
#include <iostream>
#include "libobd.h"

using namespace ctb;
using namespace std;

static PID_INFO pids[] = {
	{0x0103, 2, 3, "Fuel system status"},
	{0x0104, 1, 1, "Calculated engine load value"},		// % A*100/255
	{0x0105, 1, 3, "Engine coolant temperature"},			// буC 	A-40
	{0x0106, 1, 2, "Short term fuel trim #1"},		// -100 (Rich) 	99.22 (Lean) 	 % 	(A-128) * 100/128
	{0x0107, 1, 2, "Long term fuel trim #1"},		// -100 (Rich) 	99.22 (Lean) 	 % 	(A-128) * 100/128
	{0x0108, 1, 2, "Short term fuel trim #2"},		// -100 (Rich) 	99.22 (Lean) 	 % 	(A-128) * 100/128
	{0x0109, 1, 2, "Long term fuel trim #2"},		// -100 (Rich) 	99.22 (Lean) 	 % 	(A-128) * 100/128
	{0x010A, 1, 2, "Fuel pressure"},						// kPa (gauge) 	A*3
	{0x010B, 1, 2, "Intake manifold absolute pressure"},	// kPa (absolute)	A
	{0x010C, 2, 1, "Engine RPM"},							// ((A*256)+B)/4
	{0x010D, 1, 1, "Vehicle speed"},						// km/h 	A
	{0x010E, 1, 2, "Ignition Timing advance"},				// бу relative to #1 cylinder 	A/2 - 64
	{0x010F, 1, 3, "Intake air temperature"}, 				// буC 	A-40
	{0x0110, 2, 2, "MAF air flow rate"},					// g/s 	((A*256)+B) / 100
	{0x0111, 1, 1, "Throttle position"},					// % 	A*100/255
	{0x0121, 2, 2, "Distance traveled with malfunction indicator lamp (MIL) on"},		// km 	(A*256)+B
	{0x0122, 2, 3, "Fuel Rail Pressure (relative to manifold vacuum)"}, //	kPa 	(((A*256)+B) * 10) / 128
	{0x012C, 1, 3, "Commanded EGR"},						// % 	100*A/255
	{0x012D, 1, 3, "EGR Error"},							// % 	(A-128) * 100/128
	{0x012F, 1, 3, "Fuel Level Input"},					// % 	100*A/255
	{0x0131, 2, 2, "Distance traveled since codes cleared"}, // km 	(A*256)+B
	{0x0133, 1, 3, "Barometric pressure"},					// kPa (Absolute) 	A
	{0x013C, 2, 3, "Catalyst Temperature"},				// буC 	((A*256)+B)/10 - 40
	{0x0143, 2, 2, "Absolute load value"},					// % 	((A*256)+B)*100/255
	{0x0145, 1, 2, "Relative throttle position"},			// % 	A*100/255
	{0x0146, 1, 3, "Ambient air temperature"},				// буC 	A-40
	{0x0147, 1, 2, "Absolute throttle position B"},		// % 	A*100/255
	{0x0149, 1, 2, "Accelerator pedal position D"},		// % 	A*100/255
	{0x014C, 1, 3, "Commanded throttle actuator"},			// % 	A*100/255
	{0x014D, 2, 3, "Time run with MIL on"},				// minutes 	(A*256)+B
	{0x014E, 2, 3, "Time since trouble codes cleared"},	// minutes 	(A*256)+B
};

static int hex2int(const char *p)
{
	register char c;
	register unsigned int i=0;

	if (p[1] == 'x' || p[1] == 'X') p += 2;
	if (!p) return 0;
	while (*p && (*p==' ' || *p=='\t')) p++;
	for(c=*p;;){
		if (c>='A' && c<='F')
			c-=7;
		else if (c>='a' && c<='f')
			c-=39;
		else if (c<'0' || c>'9')
			break;
		i=(i<<4)|(c&0xF);
		c=*(++p);
	}
	return (int)i;
}

char* COBD::SendCommand(string cmd, char* lookfor, bool readall)
{
	char* rcvbuf = 0;
	size_t rcvbytes;
	int len = cmd.length();
	Wait(0);
	if(device->Writev( (char*)cmd.c_str(), len, 500 ) != len ) {
		cerr << "Incomplete data transmission" << endl;
		return 0;
	}
	bool echoed = false;
	cmd.erase(cmd.length() - 1);
	Wait(updateInterval);
	int ret = device->ReadUntilEOS(rcvbuf, &rcvbytes, "\r", 1000);
	if (ret == -1) {
		cerr << "Communication error" << endl;
		return 0;
	}
	if (ret == 0) {
		// first timeout
		if( device->Writev( (char*)"\r", 1, 500 ) != 1 ) {
			return 0;
		}
		rcvbuf = 0;
		ret = device->ReadUntilEOS(rcvbuf, &rcvbytes, "\r", 1000);
		if (ret == 0) {
			cerr << "Communication timeout" << endl;
		}
		return 0;
	}
	if (!rcvbuf) {
		return 0;
	}
	do {
		char *p;
		if (lookfor && (p = strstr(rcvbuf, lookfor))) {
			return p;
		}
		if (!lookfor && !readall) {
			if (strstr(rcvbuf, cmd.c_str())) {
				echoed = true;
				continue;
			}
			if (echoed)
				return rcvbuf;
		}
		if (!strncmp(rcvbuf, "SEARCHING", 9)) {
			rcvbuf = 0;
			if (device->ReadUntilEOS(rcvbuf, &rcvbytes, "\r", 3000) == 1) {
				continue;
			} else {
				if (strstr(rcvbuf, "NO DATA"))
					cout << rcvbuf << endl;
				break;
			}
		}
	} while ((ret = device->ReadUntilEOS(rcvbuf, &rcvbytes, "\r", 1000)) == 1);
	return rcvbuf;
}

PID_INFO* COBD::GetPidInfo(int pid)
{
	for (int i = 0; i < sizeof(pids) / sizeof(pids[0]); i++) {
		if (pids[i].pid == pid)
			return &pids[i];
	}
	return 0;
}

PID_INFO* COBD::GetPidInfo(const char* name)
{
	for (int i = 0; i < sizeof(pids) / sizeof(pids[0]); i++) {
		if (!strcmp(name, pids[i].name))
			return &pids[i];
	}
	return 0;
}

int COBD::GetSensorData(int id)
{
	int data = INVALID_PID_DATA;
	char cmd[8];
	char answer[8];
	sprintf(cmd, "%04X%\r", id);
	sprintf(answer, "41 %02X", id & 0xff);
	char* reply = SendCommand(cmd, answer);
	if (!reply) {
		// trying to recover a broken session
		SendCommand("\r", 0, true);
		reply = SendCommand(cmd, answer);
	}
	if (reply) {
		const PID_INFO* pidInfo = GetPidInfo(id);
		if (pidInfo && pidInfo->dataBytes > 1) {
			if (strlen(reply) >= 11 && !strncmp(reply, answer, 5)) {
				data = (hex2int(reply + 6) << 8) + hex2int(reply + 9);
			}
		} else {
			if (strlen(reply) >= 8 && !strncmp(reply, answer, 5)) {
				data = hex2int(reply + 6);
			}
		}
	}
	return data;
}

void COBD::Uninit()
{
	if (device) {
		device->Close();
		delete device;
	}
}

bool COBD::Init(const TCHAR* devname, int baudrate, const char* protocol)
{
	ctb::SerialPort* serialPort = new ctb::SerialPort();
 	if( serialPort->Open( devname, baudrate, 
					protocol, 
					ctb::SerialPort::NoFlowControl ) >= 0 ) {

		device = serialPort;
	} else {
		delete serialPort;
		return false;
	}

	for (int i = 0; i < sizeof(pids) / sizeof(pids[0]); i++) {
		pids[i].data.time = 0;
		pids[i].data.value = 0;
	}

	char* reply;
	const char* initstr[] = {"atz\r", "atsp0\r", "atl1\r", "atal\r", "ate0\r", "ath1\r"};
	for (int i = 0; i < sizeof(initstr) / sizeof(initstr[0]); i++) {
		Wait(0);
		reply = SendCommand(initstr[i]);
		if (reply) {
			cout << reply << endl;
		}
		Wait(100);
	}

	startTime = GetTickCount();
	connected = true;
	return connected;
}

void COBD::Wait(int interval)
{
	DWORD tick = GetTickCount();
	if (interval > 0) {
		if (lastTick) {
			int timeToWait = interval - (tick - lastTick);
			if (timeToWait > 0) {
				Sleep(timeToWait);
			}
		} else {
			Sleep(interval);
		}
	}
	lastTick = tick;
}

bool COBD::RetrieveSensor(int pid, PID_DATA& data)
{
	int value;
	Wait(updateInterval);
	if ((value = GetSensorData(pid)) != INVALID_PID_DATA) {
		data.time = GetTickCount() - startTime;
		if (data.time < ADAPT_PERIOD) {
			updateInterval = max(QUERY_INTERVAL_MIN, updateInterval - QUERY_INTERVAL_STEP);
			cout << "Decreasing interval to " << updateInterval << endl;
		}
		switch (pid) {
		case PID_LOAD:
		case PID_THROTTLE:
			data.value = value * 100 / 255;
			break;
		case PID_RPM:
			data.value = value / 4;
			break;
		case PID_COOLANT_TEMP:
		case PID_INTAKE_TEMP:
			data.value = value - 40;
			break;
		case PID_FUEL_SHORT_TERM:
		case PID_FUEL_LONG_TERM:
			data.value = (value-128) * 100/128;
			break;
		default:
			data.value = value;
		}
		return true;
	} else {
		if (GetTickCount() - startTime < ADAPT_PERIOD) {
			updateInterval = min(QUERY_INTERVAL_MAX, updateInterval + QUERY_INTERVAL_STEP);
			cout << "Increasing interval to " << updateInterval << endl;
			Sleep(updateInterval * 4);
		}
		return false;
	}
}

#define NUM_PIDS (sizeof(pids) / sizeof(pids[0]))

DWORD COBD::Update()
{
	static int count = 0;
	static int p2index = 0;
	static int p3index = 0;
	DWORD ret = 0;
	// retrieve sensors with priority 1
	for (int i = 0; i < NUM_PIDS; i++) {
		if (pids[i].active && pids[i].priority == 1) {
			RetrieveSensor(pids[i].pid, pids[i].data);
			ret++;
		}
	}
	if (p2index >= NUM_PIDS) {
		p2index = 0;
		// retrieve sensors with priority 3
		if (p3index >= NUM_PIDS) p3index = 0;
		for (int i = p3index; i < NUM_PIDS; i++) {
			if (pids[i].active && pids[i].priority == 3) {
				RetrieveSensor(pids[i].pid, pids[i].data);
				ret++;
				p3index = i + 1;
				break;
			}
		}

	}
	// retrieve sensors with priority 2
	for (int i = p2index; i < NUM_PIDS; i++) {
		if (pids[i].active && pids[i].priority == 2) {
			RetrieveSensor(pids[i].pid, pids[i].data);
			ret++;
			p2index = i + 1;
			break;
		}
	}
	count++;
	return ret;
}
