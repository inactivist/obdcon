#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>

#include "libobd.h"
#include "obdcon.h"
#include "gauge.h"

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

COBD* obd;
CGauge gauge;
CGauge gauge2;
CImageBox panel;

void Render()
{
	panel.Create(540, 300, 24, "Info");

	gauge.ib = &panel;
	gauge.Load("gauge\\%d.png");
	gauge.max = 10000;

	gauge2.ib = &panel;
	gauge2.Load("gauge\\%d.png");
	gauge2.max = 260;
/*
	for (;;) {
		gauge.Update(GetTickCount() % 10000);
		gauge2.Update((GetTickCount() / 10) % 260, 300);
		Sleep(10);
	}
*/
}

int main(int argc, char* argv[])
{
    int baudrate = 115200;
    const char* devname = "com4";
    const char* protocol = "8N1";
    int quit = 0;
    int val;


    while ( ( val=getopt( argc, argv, (char*)options ) ) != EOF ) {
	   switch ( val ) {
	   case 'b' : baudrate = strtol( optarg, NULL, 10 ); break;
	   case 'd' : devname = optarg; break;
	   case 'h' : cerr << helpMessage << endl; exit( 0 );
	   }
    }

	/*
	Render();
	getchar();
	return 0;
	*/

	obd = new COBD(devname, baudrate, protocol);
	if (!obd->connected) {
		cerr << "Error opening " << devname;
		delete obd;
		return -1;
	}

	Render();

	
	for (int n = 0; ; n++) {
		obd->Update(FLAG_PID_SPEED | FLAG_PID_RPM | FLAG_PID_THROTTLE);
		cout << "RPM: " << obd->sensors.rpm
			<< " Speed: " << obd->sensors.speed
			<< " Throttle Pos.: " << obd->sensors.throttle
			<< endl;
	}

	return 0;
}