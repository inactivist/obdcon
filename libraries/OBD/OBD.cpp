/*************************************************************************
* OBD-II (ELM327) data accessing library for Arduino
* Distributed under GPL v2.0
* Copyright (c) 2012 Stanley Huang <stanleyhuangyc@gmail.com>
* All rights reserved.
*************************************************************************/

#include <Arduino.h>
#include <avr/pgmspace.h>
#include "OBD.h"

#define INIT_CMD_COUNT 3
#define MAX_CMD_LEN 6

const char PROGMEM s_initcmd[INIT_CMD_COUNT][MAX_CMD_LEN] = {"atz\r", "ate0\r","atl1\r"};
const char PROGMEM s_elm[] = "ELM327";
const char PROGMEM s_searching[] = "SEARCHING";
const char PROGMEM s_cmd_fmt[] = "%02X%02X 1\r";
const char PROGMEM s_cmd_sleep[MAX_CMD_LEN] = "atlp\r";
const char PROGMEM s_response_begin[] = "41 ";

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
	sprintf_P(cmd, s_cmd_fmt, dataMode, pid);
	WriteData(cmd);
}

bool COBD::ReadSensor(byte pid, int& result, bool passive)
{
    if (passive) {
        bool hasData;
        unsigned long tick = millis();
        while (!(hasData = DataAvailable()) && millis() - tick < OBD_TIMEOUT_SHORT);
        if (!hasData) {
            errors++;
            return false;
        }
    } else {
        Query(pid);
    }

    char buffer[OBD_RECV_BUF_SIZE];
    char* data = GetResponse(pid, buffer);
    if (!data)
		return false;

	switch (pid) {
	case PID_RPM:
		result = GetLargeValue(data) >> 2;
		break;
	case PID_FUEL_PRESSURE:
		result = GetSmallValue(data) * 3;
		break;
	case PID_COOLANT_TEMP:
	case PID_INTAKE_TEMP:
	case PID_AMBIENT_TEMP:
		result = GetTemperatureValue(data);
		break;
	case PID_ABS_ENGINE_LOAD:
		result = GetLargeValue(data) * 100 / 255;
		break;
	case PID_MAF_FLOW:
		result = GetLargeValue(data) / 100;
		break;
	case PID_THROTTLE:
	case PID_ENGINE_LOAD:
	case PID_FUEL_LEVEL:
		result = GetPercentageValue(data);
		break;
	case PID_SPEED:
	case PID_BAROMETRIC:
	case PID_INTAKE_PRESSURE:
		result = GetSmallValue(data);
		break;
	case PID_TIMING_ADVANCE:
		result = (GetSmallValue(data) - 128) >> 1;
		break;
	case PID_DISTANCE:
	case PID_RUNTIME:
		result = GetLargeValue(data);
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

byte COBD::WriteData(const char* s)
{
	return Serial.write(s);
}

byte COBD::WriteData(const char c)
{
	return Serial.write(c);
}

char* COBD::GetResponse(byte pid, char* buffer)
{
	unsigned long startTime = millis();
	byte i = 0;

	for (;;) {
		if (DataAvailable()) {
			char c = ReadData();
			buffer[i] = c;
			if (++i == OBD_RECV_BUF_SIZE - 1) {
                // buffer overflow
                break;
			}
			if (c == '>' && i > 6) {
				// prompt char reached
				break;
			}
		} else {
		    buffer[i] = 0;
		    unsigned int timeout;
		    if (dataMode != 1 || strstr_P(buffer, s_searching)) {
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
	buffer[i] = 0;

	char *p = buffer;
	while ((p = strstr_P(p, s_response_begin))) {
        p += 3;
        if (hex2uint8(p) == pid) {
            errors = 0;
            p += 2;
            if (*p == ' ') p++;
            return p;
        }
	}
	return 0;
}

void COBD::Sleep(int seconds)
{
    char cmd[MAX_CMD_LEN];
    strcpy_P(cmd, s_cmd_sleep);
    WriteData(cmd);
    if (seconds) {
        delay((unsigned long)seconds << 10);
        WriteData('\r');
    }
}

bool COBD::Init(bool passive)
{
	unsigned long currentMillis;
	unsigned char n;
	char prompted;
	char buffer[OBD_RECV_BUF_SIZE];

	for (unsigned char i = 0; i < INIT_CMD_COUNT; i++) {
        if (!passive) {
            char cmd[MAX_CMD_LEN];
            strcpy_P(cmd, s_initcmd[i]);
            WriteData(cmd);
        }
		n = 0;
		prompted = 0;
        currentMillis = millis();
		for (;;) {
			if (DataAvailable()) {
				char c = ReadData();
				if (i == 0) {
                    // reset command
					if (n < sizeof(s_elm) - 1) {
                        if (c == pgm_read_byte(&s_elm[n])) {
							buffer[n++] = c;
						}
					} else if (n < OBD_RECV_BUF_SIZE - 1) {
						buffer[n++] = c;
					}
				} else {
				    if ((n == 0 && c == 'O') || (n == 1 && c == 'K')) {
                        n++;
				    }
				}
                if (c == '>') {
                    prompted++;
                }
            } else if (prompted) {
                if (i == 0) {
                    // reset command
                   	buffer[n] = 0;
                    if (n > sizeof(s_elm) - 1) {
                       	char *p = strchr(buffer, '.');
                        elmRevision = p ? *(p + 1) - '0' : 0;
                        break;
                    }
                } else {
                    if (n >= 2) break;
                }
                break;
			} else {
				unsigned long elapsed = millis() - currentMillis;
				if (elapsed > OBD_TIMEOUT_INIT) {
				    // init timeout
				    //WriteData("\r");
				    return false;
				}
			}
		}
	}
    errors = 0;
	return true;
}
