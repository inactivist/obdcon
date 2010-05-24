#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>

#include "libobd.h"
#include "obdcon.h"

using namespace std;

static const char* options="b:d:h:";
static const char* helpMessage =
{
    "OBD Console\n"
    "obdcon [options]\n"
    "available options are:\n"
    "-b     : baudrate [any value], default is 115200\n"
    "-d     : connected device, default is COM1\n"
    "-h     : print this\n"
};

int main(int argc, char* argv[])
{
    int baudrate = 115200;
    const char* devname = "com4";
    const char* protocol = "8N1";
    int quit = 0;
    int val;
	COBD obd;

    while ( ( val=getopt( argc, argv, (char*)options ) ) != EOF ) {
	   switch ( val ) {
	   case 'b' : baudrate = strtol( optarg, NULL, 10 ); break;
	   case 'd' : devname = optarg; break;
	   case 'h' : cerr << helpMessage << endl; exit( 0 );
	   }
    }

	if (!obd.Init(devname, baudrate, protocol)) {
		cerr << "Error opening " << devname;
		return -1;
	}

	for (int n = 0; ; n++) {
		obd.Update(FLAG_PID_SPEED | FLAG_PID_RPM | FLAG_PID_THROTTLE);
		cout << "RPM: " << obd.sensors.rpm
			<< " Speed: " << obd.sensors.speed
			<< " Throttle Pos.: " << obd.sensors.throttle
			<< endl;
	}

	return 0;
}