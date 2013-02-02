#include <Arduino.h>
#include <LCD4Bit_mod.h>
#include <OBD.h>

#define INIT_CMD_COUNT 8
#define MAX_CMD_LEN 6

const char initcmd[INIT_CMD_COUNT][MAX_CMD_LEN] = {"ATZ\r","ATE0\r","ATL1\r","ATI\r","0100\r","0120\r","0140\r","0145\r"};

//SoftwareSerial softSerial(2, 3); // RX, TX

unsigned int  adc_key_val[5] ={30, 150, 360, 535, 760 };
int NUM_KEYS = 5;
int adc_key_in;
char key=-1;
char oldkey=-1;

byte index = 0;
uint16_t pid = 0x0145;

//create object to control an LCD.
LCD4Bit_mod lcd = LCD4Bit_mod(2);

class COBDTester : public COBD
{
public:
    bool Init(bool passive = false)
    {
        unsigned long currentMillis;
        unsigned char n;
        char prompted;
        char buffer[OBD_RECV_BUF_SIZE];

        for (unsigned char i = 0; i < INIT_CMD_COUNT; i++) {
            lcd.clear();
            lcd.cursorTo(1, 0);
            lcd.printIn(initcmd[i]);
            lcd.cursorTo(1, 0);
            WriteData(initcmd[i]);
            n = 0;
            prompted = 0;
            currentMillis = millis();
            for (;;) {
                if (DataAvailable()) {
                    char c = ReadData();
                    if (c == '>') {
                        buffer[n] = 0;
                        prompted++;
                    } else if (n < OBD_RECV_BUF_SIZE - 1) {
                        buffer[n++] = c;

                        if (c == '\r' || c == '\n')
                            lcd.cursorTo(2, 0);
                        else
                            lcd.print(c);
                    }
                } else if (prompted) {
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
            delay(500);
        }
        errors = 0;
        return true;
    }
};

COBDTester obd;


// Convert ADC value to key number
char get_key(unsigned int input)
{
	char k;
    for (k = 0; k < NUM_KEYS; k++) {
		if (input < adc_key_val[k])
			return k;
	}
	return -1;
}

void query()
{
    char buf[17];

    switch (index) {
    case 0:
        sprintf(buf, "PID [%04X]", pid);
        break;
    case 1:
        sprintf(buf, "PID %03X[%01X]", pid >> 4, pid & 0xf);
        break;
    case 2:
        sprintf(buf, "PID %02X[%01X]%01X", pid >> 8, (pid >> 4) & 0xf, pid & 0xf);
        break;
    case 3:
        sprintf(buf, "PID %01X[%01X]%02X", pid >> 12, (pid >> 8) & 0xf, pid & 0xff);
        break;
    case 4:
        sprintf(buf, "PID [%01X]%03X", pid >> 12, pid & 0xfff);
        break;
    }

    lcd.cursorTo(1, 0);
    lcd.printIn(buf);

    obd.dataMode = (byte)(pid >> 8);
    obd.Query((byte)pid);
    //char* data = obd.GetResponse(pid);
    lcd.cursorTo(2, 0);
    //lcd.printIn(obd.recvBuf);
}

void setup()
{
    lcd.init();
    lcd.printIn("WAIT SERIAL");
    pinMode(13, OUTPUT);  //we'll use the debug LED to output a heartbeat
    digitalWrite(13, LOW);
    OBDUART.begin(38400);
    digitalWrite(13, HIGH);
    do {
        lcd.clear();
        lcd.printIn("OBD TESTER 1.1");
        lcd.cursorTo(2, 0);
        lcd.printIn("CONNECTING...");
        delay(1000);
    } while(!obd.Init());
    lcd.cursorTo(2, 0);

    char buf[16];
    lcd.cursorTo(2, 0);
    lcd.printIn("CONNECTED!   ");
    delay(1000);
    lcd.clear();
    query();
}

void loop()
{
    if (Serial.available()) {
        char c = Serial.read();
        if (c == '\r' || c == '\n') {
            lcd.cursorTo(2, 0);
        } else if (c == '>') {
            lcd.cursorTo(1, 15);
            lcd.print(c);
            lcd.cursorTo(2, 0);
            query();
        } else {
            lcd.print(c);
        }
    }

	adc_key_in = analogRead(0);    // read the value from the sensor
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
				case 2: // down key
					switch (index) {
					case 0:
					    pid--;
					    break;
					case 1:
					    pid = (pid & 0xfff0) | (((pid & 0xf) - 1) & 0xf);
					    break;
					case 2:
					    pid = (pid & 0xff0f) | (((pid & 0xf0) - 0x10) & 0xf0);
					    break;
					case 3:
					    pid = (pid & 0xf0ff) | (((pid & 0xf00) - 0x100) & 0xf00);
					    break;
					case 4:
					    pid = (pid & 0x0fff) | (((pid & 0xf000) - 0x1000) & 0xf000);
					    break;
					}
					break;
				case 1: // up key
					switch (index) {
					case 0:
					    pid++;
					    break;
					case 1:
					    pid = (pid & 0xfff0) | (((pid & 0xf) + 1) & 0xf);
					    break;
					case 2:
					    pid = (pid & 0xff0f) | (((pid & 0xf0) + 0x10) & 0xf0);
					    break;
					case 3:
					    pid = (pid & 0xf0ff) | (((pid & 0xf00) + 0x100) & 0xf00);
					    break;
					case 4:
					    pid = (pid & 0x0fff) | (((pid & 0xf000) + 0x1000) & 0xf000);
					}
					break;
				case 0: // right key
					if (index > 0) index--;
					break;
				case 3: // left key
					if (index < 4) index++;
					break;
				}
				lcd.clear();
				query();
			}
		}
	}
}
