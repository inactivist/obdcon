/*************************************************************************
* OBD-II (ELM327) data accessing library for Arduino
* Distributed under GPL v2.0
* Copyright (c) 2012 Stanley Huang <stanleyhuangyc@gmail.com>
* All rights reserved.
*************************************************************************/

#define OBD_TIMEOUT_SHORT 2000 /* ms */
#define OBD_TIMEOUT_LONG 7000 /* ms */
#define OBD_TIMEOUT_INIT 3000 /* ms */
#define OBD_SERIAL_BAUDRATE 38400
#define OBD_RECV_BUF_SIZE 40

// mode 0 pids
#define PID_RPM 0x0C
#define PID_SPEED 0x0D
#define PID_THROTTLE 0x11
#define PID_ENGINE_LOAD 0x04
#define PID_COOLANT_TEMP 0x05
#define PID_INTAKE_TEMP 0x0F
#define PID_MAF_FLOW 0x10
#define PID_ABS_ENGINE_LOAD 0x43
#define PID_AMBIENT_TEMP 0x46
#define PID_FUEL_PRESSURE 0x0A
#define PID_INTAKE_PRESSURE 0x0B
#define PID_BAROMETRIC 0x33
#define PID_TIMING_ADVANCE 0x0E
#define PID_FUEL_LEVEL 0x2F
#define PID_RUNTIME 0x1F
#define PID_DISTANCE 0x31

unsigned int hex2uint16(const char *p);
unsigned char hex2uint8(const char *p);

class COBD
{
public:
    COBD()
    {
        elmRevision = 0;
        dataMode = 1;
        errors = 0;
        data = recvBuf;
    }
	bool Init(bool passive = false);
	bool ReadSensor(byte pid, int& result, bool passive = false);
	void Sleep(int seconds);
	byte dataMode;
	byte errors;
	char elmRevision;
	char* data;
	char recvBuf[OBD_RECV_BUF_SIZE];
protected:
	void Query(byte pid);
	bool GetResponse(byte pid);
	int GetPercentageValue()
	{
		return (int)hex2uint8(data) * 100 / 255;
	}
	int GetLargeValue()
	{
		return hex2uint16(data);
	}
	int GetSmallValue()
	{
		return hex2uint8(data);
	}
	int GetTemperatureValue()
	{
		return (int)hex2uint8(data) - 40;
	}
private:
	virtual bool DataAvailable();
	virtual char ReadData();
	virtual byte WriteData(const char* s);
};
