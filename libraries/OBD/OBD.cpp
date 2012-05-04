/*************************************************************************
* OBD-II (ELM327) data accessing library for Arduino
* Distributed under GPL v2.0
* Copyright (c) 2012 Stanley Huang <stanleyhuangyc@gmail.com>
* All rights reserved.
*************************************************************************/

#include <Arduino.h>
#include "OBD.h"

unsigned int hex2uint16(const char *p)
{
	char c = *p;
	unsigned int i = 0;
	for (char n = 0; c && n < 4; c = *(++p)) {
		if (c >= 'A' && c <= 'F') {
			c -= 7;
        } else if (c == ' ') {
            continue;
        } else if (c < '0' || c > '9') {
			break;
        }
		i = (i << 4) | (c & 0xF);
		n++;
	}
	return i;
}

unsigned char hex2uint8(const char *p)
{
	unsigned char c1 = *p;
	unsigned char c2 = *(p + 1);
	if (c1 >= 'A' && c1 <= 'F')
		c1 -= 7;
	else if (c1 < '0' || c1 > '9')
		return 0;

	if (c2 >= 'A' && c2 <= 'F')
		c2 -= 7;
	else if (c2 < '0' || c2 > '9')
		return 0;

	return c1 << 4 | (c2 & 0xf);
}

void COBD::Query(unsigned char pid)
{
	char cmd[8];
	if (elmRevision >= 4)
        sprintf(cmd, "%02X%02X 1\r", dataMode, pid);
    else
        sprintf(cmd, "%02X%02X\r", dataMode, pid);
	WriteData(cmd);
}

bool COBD::ReadSensor(unsigned char pid, int& result)
{
	Query(pid);
	if (!GetResponse(pid))
		return false;

	switch (pid) {
	case PID_RPM:
		result = GetLargeValue() >> 2;
		break;
	case PID_FUEL_PRESSURE:
		result = GetSmallValue() * 3;
		break;
	case PID_COOLANT_TEMP:
	case PID_INTAKE_TEMP:
	case PID_AMBIENT_TEMP:
		result = GetTemperatureValue();
		break;
	case PID_ABS_ENGINE_LOAD:
		result = GetLargeValue() * 100 / 255;
		break;
	case PID_MAF_FLOW:
		result = GetLargeValue() / 100;
		break;
	case PID_THROTTLE:
	case PID_ENGINE_LOAD:
	case PID_FUEL_LEVEL:
		result = GetPercentageValue();
		break;
	case PID_SPEED:
	case PID_BAROMETRIC:
	case PID_INTAKE_PRESSURE:
		result = GetSmallValue();
		break;
	case PID_TIMING_ADVANCE:
		result = (GetSmallValue() - 128) >> 1;
		break;
	case PID_DISTANCE:
	case PID_RUNTIME:
		result = GetLargeValue();
		break;
	default:
		return false;
	}
	return true;
}

bool COBD::DataAvailable()
{
	return Serial.available();
}

char COBD::ReadData()
{
	return Serial.read();
}

unsigned char COBD::WriteData(const char* s)
{
	return Serial.write(s);
}

bool COBD::GetResponse(unsigned char pid)
{
	unsigned long startTime = millis();
	byte i = 0;
	data = 0;

	for (;;) {
		if (DataAvailable()) {
			char c = ReadData();
			recvBuf[i] = c;
			if (++i == sizeof(recvBuf) - 1) {
                // buffer overflow
                break;
			}
			if (c == '>' && i > 6) {
				// prompt char reached
				break;
			}
		} else {
		    recvBuf[i] = 0;
		    unsigned int timeout;
		    if (dataMode != 1 || strstr(recvBuf, "SEARCHING")) {
                timeout = OBD_TIMEOUT_LONG;
		    } else {
		        timeout = OBD_TIMEOUT_SHORT;
		    }
		    if (millis() - startTime > timeout) {
                // timeout
                errors++;
                break;
		    }
		}
	}
	recvBuf[i] = 0;

	char *p = recvBuf;
	while ((p = strstr(p, "41 "))) {
        p += 3;
        if (hex2uint8(p) == pid) {
            errors = 0;
            p += 2;
            if (*p == ' ') p++;
            data = p;
            return true;
        }
	}
	data = recvBuf;
	return false;
}

static const char* initcmd[] = {"atz\r", "ate0\r","atl1\r"};
#define CMD_COUNT (sizeof(initcmd) / sizeof(initcmd[0]))
static const char* s_elm = "ELM327";
static const char* s_ok = "OK";

void COBD::Sleep(int seconds)
{
    WriteData("atlp\r");
    if (seconds) {
        delay((unsigned long)seconds * 1000);
        WriteData("\r");
    }
}

bool COBD::Init()
{
	unsigned long currentMillis;
	unsigned char n;
	dataMode = 1;
	char prompted;

    data = recvBuf;
    elmRevision = 0;
	for (unsigned char i = 0; i < CMD_COUNT; i++) {
		WriteData(initcmd[i]);
		n = 0;
		prompted = 0;
        currentMillis = millis();
		for (;;) {
			if (DataAvailable()) {
				char c = ReadData();
				if (i == 0) {
                    // reset command
					if (n < sizeof(s_elm)) {
                        if (c == s_elm[n]) {
							recvBuf[n++] = c;
						}
					} else if (n < sizeof(recvBuf) - 1) {
						recvBuf[n++] = c;
					}
				} else {
				    if (n < sizeof(s_ok) && c == s_ok[n]) {
                        n++;
				    }
				}
                if (c == '>') {
                    prompted++;
                }
            } else if (prompted) {
                if (i == 0) {
                    // reset command
                   	recvBuf[n] = 0;
                    if (n >= sizeof(s_elm)) {
                       	char *p = strstr(recvBuf, "1.");
                        elmRevision = p ? *(p + 2) - '0' : 0;
                        break;
                    }
                } else {
                    if (n >= sizeof(s_ok)) break;
                }
                break;
			} else {
				unsigned long elapsed = millis() - currentMillis;
				if (elapsed > OBD_TIMEOUT_LONG) {
				    // timeout
				    //WriteData("\r");
				    return false;
				}
			}
		}
	}
    errors = 0;
	return true;
}
