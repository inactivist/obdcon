/*************************************************************************
* libobd - The OBD-II access library
* Distributed under MPL 1.1
*
* Copyright (c) 2010 Stanley Huang <stanleyhuangyc@gmail.com>
* All rights reserved.
**************************************************************************/

#include <stdio.h>
#include <iostream>
#include <time.h>
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

// DO NOT TRANSLATE ANY STRINGS IN THIS FUNCTION!
int COBD::ProcessResponse(char *msg_received)
{
   int i = 0;
   char *msg = msg_received;
   int is_hex_num = TRUE;

   if (!msg)
	   return RUBBISH;

   while(*msg && (*msg <= ' '))
      msg++;

   if (*msg == '>')
	   msg ++;
   else if (strncmp(msg, "SEARCHING...", 12) == 0)
      msg += 13;
   else if (strncmp(msg, "BUS INIT: OK", 12) == 0)
      msg += 13;
   else if (strncmp(msg, "BUS INIT: ...OK", 15) == 0)
      msg += 16;

	if (!*msg)
		return RUBBISH;

   for(i = 0; *msg; msg++) //loop to copy data
   {
      if (*msg > ' ')  // if the character is not a special character or space
      {
         if (*msg == '<') // Detect <DATA_ERROR
         {
            if (strncmp(msg, "<DATA ERROR", 10) == 0)
               return DATA_ERROR2;
            else
               return RUBBISH;
         }
         msg_received[i] = *msg; // rewrite response
         if (!isxdigit(*msg) && *msg != ':' && *msg != '>')
            is_hex_num = FALSE;
         i++;
      }
      else if (((*msg == '\n') || (*msg == '\r')) && (msg_received[i-1] != SPECIAL_DELIMITER)) // if the character is a CR or LF
         msg_received[i++] = SPECIAL_DELIMITER; // replace CR with SPECIAL_DELIMITER
   }
   
   if (i > 0)
      if (msg_received[i-1] == SPECIAL_DELIMITER)
         i--;
   msg_received[i] = '\0'; // terminate the string

   if (is_hex_num)
      return HEX_DATA;

   if (strstr(msg_received, "NODATA"))
      return ERR_NO_DATA;
   if (strstr(msg_received, "UNABLETOCONNECT"))
      return UNABLE_TO_CONNECT;
   if (strcmp(msg_received + strlen(msg_received) - 7, "BUSBUSY") == 0)
      return BUS_BUSY;
   if (strcmp(msg_received + strlen(msg_received) - 9, "DATAERROR") == 0)
      return DATA_ERROR;
   if (strcmp(msg_received + strlen(msg_received) - 8, "BUSERROR") == 0 ||
       strcmp(msg_received + strlen(msg_received) - 7, "FBERROR") == 0)
      return BUS_ERROR;
   if (strcmp(msg_received + strlen(msg_received) - 8, "CANERROR") == 0)
      return CAN_ERROR;
   if (strcmp(msg_received + strlen(msg_received) - 10, "BUFFERFULL") == 0)
      return BUFFER_FULL;
   if (strncmp(msg_received, "BUSINIT:", 8) == 0)
   {
      if (strcmp(msg_received + strlen(msg_received) - 5, "ERROR") == 0)
         return BUS_INIT_ERROR;
      else
         return SERIAL_ERROR;
   }
   if (strcmp(msg_received, "?") == 0)
      return UNKNOWN_CMD;
   if (strncmp(msg_received, "ELM320", 6) == 0)
      return INTERFACE_ELM320;
   if (strncmp(msg_received, "ELM322", 6) == 0)
      return INTERFACE_ELM322;
   if (strncmp(msg_received, "ELM323", 6) == 0)
      return INTERFACE_ELM323;
   if (strncmp(msg_received, "ELM327", 6) == 0)
      return INTERFACE_ELM327;
   if (strncmp(msg_received, "OBDLink", 7) == 0 ||
       strncmp(msg_received, "STN1000", 7) == 0 ||
       strncmp(msg_received, "STN11", 5) == 0)
      return INTERFACE_OBDLINK;
   if (strcmp(msg_received, "OBDIItoRS232Interpreter") == 0)
      return ELM_MFR_STRING;
   
   return RUBBISH;
}

int COBD::RetrieveValue(int pid_l, char* data)
{
	int value = INVALID_PID_DATA;
	for (int i = 0; i < sizeof(pids) / sizeof(pids[0]); i++) {
		if ((pids[i].pid & 0xff) == pid_l) {
			if (pids[i].dataBytes == 1)
				value = hex2int(data);
			else
				value = (hex2int(data) << 8)+ hex2int(data + 3);
			pids[i].data.time = GetTickCount();
			switch (pids[i].pid) {
			case PID_ENGINE_LOAD:
			case PID_THROTTLE:
				pids[i].data.value = value * 100 / 255;
				break;
			case PID_RPM:
				pids[i].data.value = value / 4;
				break;
			case PID_COOLANT_TEMP:
			case PID_INTAKE_TEMP:
				pids[i].data.value = value - 40;
				break;
			case PID_FUEL_SHORT_TERM_1:
			case PID_FUEL_LONG_TERM_1:
			case PID_FUEL_SHORT_TERM_2:
			case PID_FUEL_LONG_TERM_2:
				pids[i].data.value = (value-128) * 100/128;
				break;
			default:
				pids[i].data.value = value;
			}
		}
	}
	return value;
}

char* COBD::SendCommand(const char* cmd)
{
	int len = strlen(cmd);
	if(device->Write((char*)cmd, len) != len ) {
		cerr << "Unable to send command" << endl;
		return 0;
	}
	int retry = 0;
	char* v = 0;
	memset(rcvbuf, 0, sizeof(rcvbuf));
	if (fplog) {
		fprintf(fplog, "===%d===\r\n", GetTickCount() - startTime);
	}
	int offset = device->Read(rcvbuf, sizeof(rcvbuf));
	if (offset <= 0) return 0;
	char* parse = rcvbuf;
	while (offset < sizeof(rcvbuf)) {
		char *p = strstr(parse, "7E8");
		int pid_l;
		if (p && p[7] == '4' && p[8] == '1' && (pid_l = atoi(p + 10))) {
			parse = p + 13;
			RetrieveValue(pid_l, parse);
			continue;
		} else if (strchr(parse, '>')) {
			break;
		}
		// read till prompt character appears
		int bytes = device->Read(rcvbuf + offset, 1);
		if (bytes < 0) return 0;
		if (bytes == 0) {
			if (++retry < 100) {
				Sleep(10);
				continue;
			} else if (offset == 0) {
				return 0;
			} else {
				break;
			}
		}
		retry = 0;
		offset++;
	}
	if (fplog) {
		fprintf(fplog, "%s\r\n===%d===\r\n", rcvbuf, GetTickCount() - startTime);
	}
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

int COBD::QuerySensor(int id)
{
	int value = INVALID_PID_DATA;
	char cmd[16];
	sprintf(cmd, "%04X 1\r", id);
	char* reply = SendCommand(cmd);
	switch (ProcessResponse(reply)) {
	case ERR_NO_DATA:
		cout << "No data" << endl;
		Sleep(3000);
		break;
	case UNABLE_TO_CONNECT:
		cout << "Unable to connect to vehcile" << endl;
		Sleep(3000);
		break;
	case BUS_STOPPED:
		cout << "Stopped" << endl;
		Sleep(3000);
		break;
	}
	return value;
}

void COBD::Uninit()
{
	if (device) {
		device->Close();
		delete device;
	}
}

bool COBD::Init()
{
	ctb::SerialPort* serialPort = new ctb::SerialPort();
#ifdef WINCE
	const TCHAR* portfmt = TEXT("COM%d:");
#else
    const char* portfmt = "com%d";
#endif
	TCHAR portname[8];
	wsprintf(portname, portfmt, comport);
 	if( serialPort->Open(portname, baudrate, 
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

	const char* initstr[] = {"atz\r", "ate0\r", "atsp0\r", "atl1\r", "atal\r", "ath1\r"};
	for (int i = 0; i < sizeof(initstr) / sizeof(initstr[0]); i++) {
		Wait(0);
		char* reply = SendCommand(initstr[i]);
		if (reply) {
			switch (ProcessResponse(reply)) {
			case INTERFACE_ELM327:
				cout << "ELM327 adapter detected" << endl;
				break;
			}
		} else {
			cout << "Error sending command " << initstr[i] << endl;
		}
		Wait(100);
	}
	char* reply = SendCommand("0100 1");
	if (reply) cout << "Valid PIDS: " << reply << endl;

	for (int i = 0; i < 10; i++) {
		Wait(5000);
		cout << "Wait for data attempt " << i + 1 << endl;
		if (RetrieveSensor(PID_RPM))
			break;
	}
	startTime = GetTickCount();
	return true;
}

void COBD::Wait(int interval, int minimum)
{
	if (interval > 0) {
		if (lastTick) {
			int timeToWait = interval - (GetTickCount() - lastTick);
			if (timeToWait > 0) {
				Sleep(timeToWait);
			} else {
				Sleep(minimum);
			}
		} else {
			Sleep(interval);
		}
	}
	lastTick = GetTickCount();
}

bool COBD::RetrieveSensor(int pid)
{
	int value;
	Wait(queryInterval);
	value = QuerySensor(pid);
#if 0
	if (GetTickCount() - startTime < 30000) {
		if (value != INVALID_PID_DATA) {
			failures = 0;
			queryInterval = max(minInterval, queryInterval - QUERY_INTERVAL_STEP);
			return true;
		} else {
			failures++;
			if (queryInterval <= minInterval && failures >= 2) {
				minInterval += 10;
				queryInterval = minInterval;
			} else { 
				queryInterval = min(QUERY_INTERVAL_MAX, queryInterval + QUERY_INTERVAL_STEP);
			}
			cout << "Increasing interval to " << queryInterval << endl;
			return false;
		}
	} else {
	}
#endif
	return value != INVALID_PID_DATA;
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
			RetrieveSensor(pids[i].pid);
			ret++;
		}
	}
	if (p2index >= NUM_PIDS) {
		p2index = 0;
		// retrieve sensors with priority 3
		if (p3index >= NUM_PIDS) p3index = 0;
		for (int i = p3index; i < NUM_PIDS; i++) {
			if (pids[i].active && pids[i].priority == 3) {
				RetrieveSensor(pids[i].pid);
				ret++;
				p3index = i + 1;
				break;
			}
		}

	}
	// retrieve sensors with priority 2
	for (int i = p2index; i < NUM_PIDS; i++) {
		if (pids[i].active && pids[i].priority == 2) {
			RetrieveSensor(pids[i].pid);
			ret++;
			p2index = i + 1;
			break;
		}
	}
	count++;
	return ret;
}

int IsFileExist(const char* filename)
{
	FILE *fp = fopen(filename, "rb");
	if (fp) {
		fclose(fp);
		return 1;
	}
	return 0;
}

bool COBD::StartLogging()
{
	char path[MAX_PATH];
	time_t tm=time(NULL);
	struct tm *btm;
	btm=gmtime(&tm);

	int n = GetModuleFileName(GetModuleHandle(NULL), path, MAX_PATH);
	for (; n > 0; n--) {
		if (path[n] == '\\') {
			path[n + 1] = 0;
			break;
		}
	}
	strcat(path, "obddata-");
	
	char *p = path + strlen(path);
	sprintf(p, "%02d%02d%02d%02d", btm->tm_mon + 1, btm->tm_mday, btm->tm_hour, btm->tm_min);
	strcat(path, ".log");
	fplog = fopen(path, "wb");
	return fplog != 0;
}

void COBD::StopLogging()
{
	fclose(fplog);
	fplog = 0;
}
