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

class COBD
{
public:
	bool Init();
	bool ReadSensor(uint8_t pid, int& result);
	char recvBuf[64];
	uint8_t dataMode;
private:
	virtual bool DataAvailable();
	virtual char ReadData();
	virtual uint8_t WriteData(const char* s);
	unsigned int hex2int(const char *p);
	unsigned char hex2char(const char *p);
	void Query(uint8_t pid);
	bool GetResponse(uint8_t pid);
	int GetPercentageValue()
	{
		return (int)hex2char(recvBuf + 9) * 100 / 255;
	}
	int GetLargeValue()
	{
		return hex2int(recvBuf + 9);
	}
	int GetSmallValue()
	{
		return hex2char(recvBuf + 9);
	}
	int GetTemperatureValue()
	{
		return (int)hex2char(recvBuf + 9) - 40;
	}
};
