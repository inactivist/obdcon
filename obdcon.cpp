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
    "OBD Console\n"
    "obdcon [options]\n"
    "available options are:\n"
    "-b     : baudrate [any value], default is 115200\n"
    "-d     : connected device, default is COM1\n"
    "-h     : print this\n"
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

char* SendCommand(string cmd, char* lookfor = 0, bool readall = false)
{
	char* rcvbuf = 0;
	size_t rcvbytes;
	int len = cmd.length();
	if( device->Writev( (char*)cmd.c_str(), len, 500 ) != len ) {
		cerr << "Incomplete data transmission" << endl;
		return 0;
	}
	Sleep(100);
	bool echoed = false;
	cmd.erase(cmd.length() - 1);
	int ret = device->ReadUntilEOS(rcvbuf, &rcvbytes, "\r", 3000);
	if (ret == -1) {
		cerr << "Communication error" << endl;
		return 0;
	}
	if (ret == 0) {
		cerr << "Communication timeout" << endl;
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
			cout << rcvbuf << endl;
			rcvbuf = 0;
			if (device->ReadUntilEOS(rcvbuf, &rcvbytes, "\r", 5000) == 1) {
				continue;
			} else {
				cout << "Searching timeout" << endl;
				break;
			}
		} else if (strstr(rcvbuf, "NO DATA")) {
			break;
		}
	} while ((ret = device->ReadUntilEOS(rcvbuf, &rcvbytes, "\r", 1000)) == 1);
	return rcvbuf;
}

int GetSensorData(int id, int& value, int resultBits = 8)
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
	if (data != -1) {
		value = data;
	}
	return data;
}

int main(int argc, char* argv[])
{
    int baudrate = 115200;
    string devname = ctb::COM4;
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

	char* reply;
	const char* initstr[] = {"atz\r", "atsp0\r", "atl1\r", "atal\r", "ate0\r", "ath1\r"};
	for (int i = 0; i < sizeof(initstr) / sizeof(initstr[0]); i++) {
		reply = SendCommand(initstr[i]);
		if (reply) {
			cout << reply << endl;
		}
		Sleep(100);
	}

	memset(&sensors, 0, sizeof(sensors));
	for (int n = 0; ; n++) {
		GetSensorData(0x010C, sensors.rpm, 16);
		Sleep(100);
		GetSensorData(0x010D, sensors.speed);
		Sleep(100);
		GetSensorData(0x0101, sensors.throttle);
		Sleep(100);
		/*
		if (n % 4 == 0) {
			GetSensorData(0x0105, sensors.coolant);
			GetSensorData(0x010F, sensors.intake);
			GetSensorData(0x0106, sensors.fuelShortTerm);
			GetSensorData(0x0107, sensors.fuelLongTerm);
			GetSensorData(0x0104, sensors.load);
		}
		*/
		cout << "RPM: " << sensors.rpm / 4
			<< " Speed: " << sensors.speed
			<< " Throttle Pos.: " << sensors.throttle
			<< " Intake Temp.: " << sensors.intake
			<< " Coolant Temp.: " << sensors.coolant
			<< endl;
	}

    device->Close();

    delete device;
	return 0;
}