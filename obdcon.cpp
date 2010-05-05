#include "ctb.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>

#include "obdcon.h"

using namespace ctb;
using namespace std;

static const char* options="b:d:h:";
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

static ctb::IOBase* device = NULL;

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

char* SendCommand(char* cmd)
{
	char* rcvbuf;
	size_t rcvbytes;
	int len = strlen(cmd);
	if( device->Writev( (char*)cmd, len, SERIAL_TIMEOUT ) != len ) {
		cerr << "Incomplete data transmission" << endl;
	} else if (device->ReadUntilEOS(rcvbuf, &rcvbytes, "\r", SERIAL_TIMEOUT) == 1) {
		return rcvbuf;
	}
	return 0;
}

int GetSensorData(int id, int resultBits = 8)
{
	int data = -1;
	char cmd[8];
	sprintf(cmd, "%04X%\r", id);
	char* reply = SendCommand(cmd);
	sprintf(cmd, "41 %02X", id && 0xff);
	if (reply) {
		if (resultBits == 16) {
			if (strlen(reply) >= 11 && !strncmp(reply, cmd, 5)) {
				data = (hex2int(reply + 6) << 8) + hex2int(reply + 9);
			}
		} else {
			if (strlen(reply) >= 8 && !strncmp(reply, cmd, 5)) {
				data = hex2int(reply + 6);
			}
		}
	} 
	return data;
}

int main(int argc, char* argv[])
{
    int baudrate = 115200;
    string devname = ctb::COM1;
    string protocol = "8N1";
    int quit = 0;
    int val;
	OBD_SENSOR_DATA sensors;

    while ( ( val=getopt( argc, argv, (char*)options ) ) != EOF ) {
	   switch ( val ) {
	   case 'b' : baudrate = strtol( optarg, NULL, 10 ); break;
	   case 'd' : devname = optarg; break;
	   case 'h' : cerr << helpMessage << endl; exit( 0 );
	   }
    }

	ctb::SerialPort* serialPort = new ctb::SerialPort();

 	if( serialPort->Open( devname.c_str(), baudrate, 
					protocol.c_str(), 
					ctb::SerialPort::NoFlowControl ) >= 0 ) {

		device = serialPort;
	} else {
		cerr << "Error opening " << devname;
		delete device;
		return -1;
	}

	char* reply = SendCommand("atz\r");
	if (reply) {
		cout << "Successfully connected with " << reply << endl;
	}

	memset(&sensors, 0, sizeof(sensors));
	for (;;) {
		sensors.rpm = GetSensorData(0x010C, 16);
		sensors.speed = GetSensorData(0x010D);
		sensors.throttle = GetSensorData(0x0101);
		sensors.coolant = GetSensorData(0x0105);
		sensors.intake = GetSensorData(0x010F);
		sensors.fuelShortTerm = GetSensorData(0x0106);
		sensors.fuelLongTerm = GetSensorData(0x0107);
		sensors.load = GetSensorData(0x0104);
		cout << "RPM: " << sensors.rpm
			<< " Speed: " << sensors.speed
			<< " Intake Temp.: " << sensors.intake
			<< " Coolant Temp.: " << sensors.coolant
			<< " Throttle Pos.: " << sensors.throttle
			<< " Short Term Fuel: " << sensors.fuelShortTerm
			<< " Long Term Fuel: " << sensors.fuelLongTerm
			<< " Load: " << sensors.load
			<< endl;
	}

    device->Close();

    delete device;
	return 0;
}