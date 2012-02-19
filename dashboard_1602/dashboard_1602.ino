/*************************************************************************
* Sample application based on OBD-II data accessing library for Arduino
* Distributed under GPL v2.0
* Copyright (c) 2012 Stanley Huang <stanleyhuangyc@gmail.com>
* All rights reserved.
*************************************************************************/

#include <LCD4Bit_mod.h>
#include <OBD.h>

//create object to control an LCD.  
//number of lines in display=1
LCD4Bit_mod lcd = LCD4Bit_mod(2); 
COBD obd;

//Key message
char msgs[5][15] = {"Right Key OK ", 
                    "Up Key OK    ", 
                    "Down Key OK  ", 
                    "Left Key OK  ", 
                    "Select Key OK" };
int  adc_key_val[5] ={30, 150, 360, 535, 760 };
int NUM_KEYS = 5;
int adc_key_in;
int key=-1;
int oldkey=-1;
uint8_t errors;
unsigned long lastTick = millis();
char modes[2] = {0, 5};

char modePids[] = {PID_RPM, PID_SPEED, PID_THROTTLE, PID_ENGINE_LOAD,
	PID_COOLANT_TEMP, PID_INTAKE_TEMP, PID_AMBIENT_TEMP, PID_MAF_FLOW,
	PID_ABS_ENGINE_LOAD, PID_FUEL_PRESSURE, PID_INTAKE_PRESSURE, PID_BAROMETRIC,
	PID_TIMING_ADVANCE, PID_FUEL_LEVEL, PID_RUNTIME, PID_DISTANCE};

char* modeLabels[] = {
	"Engine       rpm", "Speed       km/h", "Throttle       %", "Engine Load    %",
	"Coolant        C", "Intake Air     C", "Env. Temp      C", "MAF Flow     kpa",
	"Abs. Load      %", "Fuel         kpa", "Intake       kpa", "Barometer    kpa",
	"Timing Adv.", "Fuel Level     %", "Run Time", "Distance      km"};

char modePos[] = {8, 8, 11, 12,
	11, 11, 11, 9,
	11, 9, 9, 10,
	12, 11, 8, 10};

char* modeFmts[] = {"%4u", "%3u", "%3u", "%u",
	"%3d", "%3d", "%3d", "%3u",
	"%3u", "%3u", "%3u", "%u",
	"%3d", "%3u", "%04u:%02u", "%04u"};

#define TOTAL_PIDS (sizeof(modePids) / sizeof(modePids[0]))

void updateMode()
{
	lcd.clear();
	lcd.printIn(modeLabels[modes[0]]);
	lcd.cursorTo(2, 0);
	lcd.printIn(modeLabels[modes[1]]);
}

bool showData(int index)
{
	int value;
	char mode = modes[index];
	uint8_t pid = modePids[mode];
	if (!obd.ReadSensor(pid, value))
		return false;

	char buf[16];
	if (pid == PID_RUNTIME) {
		sprintf(buf, modeFmts[mode], (unsigned int)value / 60, (unsigned int)value % 60);
	} else {
		sprintf(buf, modeFmts[mode], value);
	}
	lcd.cursorTo(index + 1, modePos[mode]);
	lcd.printIn(buf);
	return true;					
}

bool setupConnection()
{
	char buf[16];
  lcd.clear();
  lcd.printIn("Connecting...");
  errors = 0;
  while (!obd.Init()) {
	  lcd.cursorTo(2, 0);
	  sprintf(buf, "Attempts #%d", ++errors);
	  lcd.printIn(buf);
  }
  lcd.clear();
  lcd.printIn("Connected!");
  lcd.cursorTo(2, 0);
  lcd.printIn(obd.recvBuf);
  delay(1000);
  updateMode();
}

void setup() { 
  pinMode(13, OUTPUT);  //we'll use the debug LED to output a heartbeat
  lcd.init();
  Serial.begin(9600);
  setupConnection();
  //optionally, now set up our application-specific display settings, overriding whatever the lcd did in lcd.init()
  //lcd.commandWrite(0x0F);//cursor on, display on, blink on.  (nasty!)
}

void loop()
{
	adc_key_in = analogRead(0);    // read the value from the sensor  
	digitalWrite(13, HIGH);
	key = get_key(adc_key_in);		        // convert into key press
	
	if (key != oldkey) {
		delay(50);		// wait for debounce time
		adc_key_in = analogRead(0);    // read the value from the sensor  
		key = get_key(adc_key_in);		        // convert into key press
		if (key != oldkey)				
		{			
			oldkey = key;
			if (key >=0){
				switch (key) {
				case 3: // left key
					do {
						modes[0] = modes[0] > 0 ? modes[0] - 1 : TOTAL_PIDS - 1;
					} while (modes[0] == modes[1]);
					break;
				case 0: // right key
					do {
						modes[0] = modes[0] < TOTAL_PIDS - 1 ? modes[0] + 1 : 0;
					} while (modes[0] == modes[1]);
					break;
				case 1: // up key
					do {
						modes[1] = modes[1] > 0 ? modes[1] - 1 : TOTAL_PIDS - 1;
					} while (modes[0] == modes[1]);
					break;
				case 2: // down key
					do {
						modes[1] = modes[1] < TOTAL_PIDS - 1 ? modes[1] + 1 : 0;
					} while (modes[0] == modes[1]);
					break;
				}
				updateMode();
			}
		}
	}
	digitalWrite(13, LOW);

	unsigned long curTick = millis();
	if (curTick - lastTick > 1000) {
		if (!showData(0)) {
			errors++;
		} else {
			errors = 0;
		}
		if (!showData(1)) {
			errors++;
		} else {
			errors = 0;
		}
		if (errors > 5) {
			if (setupConnection()) {
				errors = 0;
			}
		}
		lastTick = curTick;
	}

	/*
	sprintf(buf, "%3d", obd.QueryIntakePressure());
    lcd.cursorTo(1, 10);
	lcd.printIn(buf);

	sprintf(buf, "%3d", obd.QueryFuelPressure());
    lcd.cursorTo(2, 10);
	lcd.printIn(buf);
	*/

	//delay(1000);
}

// Convert ADC value to key number
int get_key(unsigned int input)
{
	int k;
    
	for (k = 0; k < NUM_KEYS; k++)
	{
		if (input < adc_key_val[k])
		{
           
    return k;
        }
	}
    
    if (k >= NUM_KEYS)
        k = -1;     // No valid key pressed
    
    return k;
}
