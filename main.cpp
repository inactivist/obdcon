#include "ctb.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>

using namespace ctb;
using namespace std;

static const char* options="b:d:e:hp:t:";
static const char* helpMessage =
{
    "A simple serial port class test\n"
    "ctbtest [options]\n"
    "available options are:\n"
    "-a     : address (only GPIB)\n"
    "-b     : baudrate [any value], default is 38400\n"
    "-d     : connected device, default is COM1\n"
    "-e     : eos\n"
    "-h     : print this\n"
    "-p     : protocol like 8N1\n"
    "-t     : communication timeout in ms (default is 100ms)\n"
};

int main(int argc, char* argv[])
{
	ctb::IOBase* device = NULL;
    int baudrate = 115200;
    string devname = ctb::COM1;
    string eos = "\r";
    string protocol = "8N1";
    int timeout = 200;
    int quit = 0;
    int val;

    while ( ( val=getopt( argc, argv, (char*)options ) ) != EOF ) {
	   switch ( val ) {
	   case 'b' : baudrate = strtol( optarg, NULL, 10 ); break;
	   case 'd' : devname = optarg; break;
	   case 'h' : cerr << helpMessage << endl; exit( 0 );
	   case 'p' : protocol = optarg; break;
	   case 't' : timeout = strtol( optarg, NULL, 10 ); break;
	   }
    }

	ctb::SerialPort* serialPort = new ctb::SerialPort();

 	if( serialPort->Open( devname.c_str(), baudrate, 
					protocol.c_str(), 
					ctb::SerialPort::NoFlowControl ) >= 0 ) {

		device = serialPort;

	} else {
		fprintf(stderr, "Error opening %s port\n", devname);
		return -1;
	}

	return 0;
}