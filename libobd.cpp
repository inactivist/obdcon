#include <stdio.h>
#include <iostream>
#include "libobd.h"

using namespace ctb;
using namespace std;

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
	Wait(100);
	bool echoed = false;
	cmd.erase(cmd.length() - 1);
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

int COBD::GetSensorData(int id, int resultBits)
{
	int data = -1;
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
		if (resultBits == 16) {
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

	memset(&sensors, 0, sizeof(sensors));

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

	for (int n = 0; n < 3; n++) {
		cout << "Waiting for data / attempt " << n + 1 << endl;
		Wait(0);
		int value = GetSensorData(PID_RPM, 16);
		if (value > 0) {
			sensors.rpm = value / 4;
			break;
		}
		Wait(3000);
	}
	connected = sensors.rpm != 0;
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

int COBD::RetrieveData(int pid, int resultBits)
{
	int value;
	Wait(updateInterval);
	if ((value = GetSensorData(pid, resultBits)) > 0) {
		updateInterval = max(QUERY_INTERVAL_MIN, updateInterval - QUERY_INTERVAL_STEP);
		return value;
	} else {
		updateInterval = min(QUERY_INTERVAL_MAX, updateInterval + QUERY_INTERVAL_STEP);
		return -1;
	}
}

DWORD COBD::Update(DWORD flags)
{
	int value;
	DWORD ret = 0;
	if (flags & FLAG_PID_RPM) {
		if ((value = RetrieveData(PID_RPM, 16)) > 0) {
			sensors.rpm = value / 4;
			ret |= FLAG_PID_RPM;
		}
	}
	if (flags & FLAG_PID_SPEED) {
		if ((value = RetrieveData(PID_SPEED)) > 0) {
			sensors.speed = value;
			ret |= FLAG_PID_SPEED;
		}
	}
	if (flags & FLAG_PID_THROTTLE) {
		if ((value = RetrieveData(PID_THROTTLE)) > 0) {
			sensors.throttle = value;
			ret |= FLAG_PID_THROTTLE;
		}
	}
	if (flags & FLAG_PID_LOAD) {
		if ((value = RetrieveData(PID_LOAD)) > 0) {
			sensors.load = value;
			ret |= FLAG_PID_LOAD;
		}
	}
	if (flags & FLAG_PID_COOLANT_TEMP) {
		if ((value = RetrieveData(PID_COOLANT_TEMP)) > 0) {
			sensors.coolant = value;
			ret |= FLAG_PID_COOLANT_TEMP;
		}
	}
	if (flags & FLAG_PID_INTAKE_TEMP) {
		if ((value = RetrieveData(PID_INTAKE_TEMP)) > 0) {
			sensors.intake = value;
			ret |= FLAG_PID_INTAKE_TEMP;
		}
	}
	if (flags & FLAG_PID_FUEL_SHORT_TERM) {
		if ((value = RetrieveData(PID_FUEL_SHORT_TERM)) > 0) {
			sensors.fuelShortTerm = value;
			ret |= FLAG_PID_FUEL_SHORT_TERM;
		}
	}
	if (flags & FLAG_PID_FUEL_LONG_TERM) {
		if ((value = RetrieveData(PID_FUEL_LONG_TERM)) > 0) {
			sensors.fuelLongTerm = value;
			ret |= FLAG_PID_FUEL_LONG_TERM;
		}
	}
	return ret;
}
