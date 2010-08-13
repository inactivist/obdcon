/*************************************************************************
* OBD Console - The OBD-II console utility
* Distributed under MPL 1.1
*
* Copyright (c) 2010 Stanley Huang <stanleyhuangyc@gmail.com>
* All rights reserved.
**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>

#include "libobd.h"
#include "obdcon.h"

using namespace std;

int main(int argc, char* argv[])
{
    int baudrate = 115200;
    const char* devname = "com4";
    const char* protocol = "8N1";
    int quit = 0;
	COBD obd;
	
#ifdef ENABLE_LOGGING
	obd.StartLogging();
#endif
	if (!obd.Init()) {
		cerr << "Error opening " << devname;
		return -1;
	}

	PID_INFO* pid[] = {
		obd.GetPidInfo(PID_RPM),
		obd.GetPidInfo(PID_SPEED),
		obd.GetPidInfo(PID_THROTTLE)
	};
	pid[0]->active = 1;
	pid[1]->active = 1;
	pid[2]->active = 1;

	for (int n = 0; ; n++) {
		obd.Update();
		cout << "RPM: " << pid[0]->data.value
			<< " Speed: " << pid[1]->data.value
			<< " Throttle Pos.: " << pid[2]->data.value
			<< endl;
	}
#ifdef ENABLE_LOGGING
	obd.StopLogging();
#endif
	return 0;
}