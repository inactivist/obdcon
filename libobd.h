#include <string>
#include "ctb.h"

#define QUERY_INTERVAL 200
#define QUERY_INTERVAL_MIN 50
#define QUERY_INTERVAL_MAX 500
#define QUERY_INTERVAL_STEP 25

#define PID_RPM 0x010C
#define PID_SPEED 0x010D
#define PID_THROTTLE 0x0111 
#define PID_LOAD 0x0104
#define PID_COOLANT_TEMP 0x0105
#define PID_FUEL_SHORT_TERM 0x0106
#define PID_FUEL_LONG_TERM 0x0107
#define PID_INTAKE_TEMP 0x010F

#define FLAG_PID_RPM 0x1
#define FLAG_PID_SPEED 0x2
#define FLAG_PID_THROTTLE 0x4
#define FLAG_PID_LOAD 0x8
#define FLAG_PID_COOLANT_TEMP 0x10
#define FLAG_PID_FUEL_SHORT_TERM 0x20
#define FLAG_PID_FUEL_LONG_TERM 0x40
#define FLAG_PID_INTAKE_TEMP 0x80

typedef struct {
	int speed;
	int rpm;
	int throttle;
	int intake;
	int coolant;
	int load;
	int fuelLongTerm;
	int fuelShortTerm;
} OBD_SENSOR_DATA;

class COBD;

class COBD
{
public:
	COBD():connected(false),running(true),lastTick(0),updateInterval(QUERY_INTERVAL),updateFlags(PID_RPM | PID_SPEED) {}
	~COBD() { Uninit(); }
	int GetSensorData(int id, int resultBits = 8);
	char* SendCommand(std::string cmd, char* lookfor = 0, bool readall = false);
	DWORD Update(DWORD flags);
	bool Init(const TCHAR* devname, int baudrate, const char* protocol);
	void Uninit();
	void Wait(int interval);
	bool connected;
	OBD_SENSOR_DATA sensors;
	DWORD updateFlags;
	bool running;
private:
	int RetrieveData(int pid, int resultBits = 8);
	DWORD lastTick;
	int updateInterval;
	ctb::IOBase* device;
};

#ifdef WINCE
#define MSG(x) MessageBox(0, TEXT(x), 0, MB_OK);
#else
#define MSG(x) fprintf(stderr, "%s\n", x);
#endif