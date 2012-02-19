#include <Arduino.h>
#include "OBD.h"

unsigned int COBD::hex2int(const char *p)
{
	char c = *p;
	unsigned int i = 0;
	for (char n = 0; n < 4; n++) {
		if (c >= 'A' && c <= 'F')
			c -= 7;
		else if (c < '0' || c > '9')
			break;
		i = (i << 4) | (c & 0xF);
		c = *(++p);
	}
	return i;
}
unsigned char COBD::hex2char(const char *p)
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

void COBD::Query(uint8_t pid)
{
	char cmd[8];
	sprintf(cmd, "%02X%02X 1\r", dataMode, pid);
	Serial.write(cmd);
}

bool COBD::ReadSensor(uint8_t pid, int& result)
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

uint8_t COBD::WriteData(const char* s)
{
	return Serial.write(s);
}

bool COBD::GetResponse(uint8_t pid)
{
	unsigned long currentMillis = millis();  
	byte i;
	byte errors = 0;
	bool needSpace = false;
	for (i = 0; i < sizeof(recvBuf) - 1;) {
		if (DataAvailable()) {
			char c = ReadData();
			if (needSpace) {
				if (c == ' ')
					needSpace = false;
				else
					errors++;
			}
			if (c == '>') {
				// prompt char reached
				break;
			} else if ((c >= '0' && c <= '9') || (c >='A' && c <= 'Z')) {
				recvBuf[i++] = c;
				if (i >= 3 && ((i - 3) & 1 == 0)) {
					needSpace = true;
				}

			}
		} else if ((dataMode == 1 && millis() - currentMillis > 500) || millis() - currentMillis > 3000) {
			break;
		}
	}
	recvBuf[i] = 0;
	if (recvBuf[0] == '7' && recvBuf[1] == 'E' && (recvBuf[2] == '8' || recvBuf[2] == '9') &&
		recvBuf[5] == '4' && recvBuf[6] == '1' && hex2char(recvBuf + 7) == pid) {
		return errors == 0;
	} else {
		return false;
	}
}

static const char* initcmd[] = {"atz\r", "ate0\r","atl1\r","ath1\r"};
#define CMD_COUNT (sizeof(initcmd) / sizeof(initcmd[0]))

bool COBD::Init()
{
	unsigned long currentMillis = millis();  
	char n = 0;
	dataMode = 1;
	for (char i = 0; i < CMD_COUNT; i++) {
		WriteData(initcmd[i]);
		for (;;) {
			if (DataAvailable()) {
				char c = ReadData();
				if (i == 0) {
					if (n == 0) {
						if (c == 'E') {
							recvBuf[n++] = c;
						}
					} else if (n < sizeof(recvBuf) - 1) {
						recvBuf[n++] = c;
					}
				}
				if (c == '>') {
					// finished
					delay(50);
					break;
				}
			} else if (millis() - currentMillis > 3000) {
				i = CMD_COUNT;
				break;
			}
		}
	}
	if (n > 0) {
		recvBuf[n] = 0;
		return true;
	} else {
		return false;
	}
}
