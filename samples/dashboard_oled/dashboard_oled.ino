/*************************************************************************
* Sample sketch based on OBD-II library for Arduino
* Using a OLED module to display realtime vehicle data
* Distributed under GPL v2.0
* Copyright (c) 2013 Stanley Huang <stanleyhuangyc@gmail.com>
* All rights reserved.
*************************************************************************/

#include <Arduino.h>
#include <OBD.h>
#include <ZtLib.h>
#include <Wire.h>

#define OLED_ADDRESS 0x27

// the following line toggles between hardware serial and software serial
//#define USE_SOFTSERIAL

#ifdef USE_SOFTSERIAL
#include <SoftwareSerial.h>
SoftwareSerial mySerial(11, 12); // RX, TX
#endif

const char PROGMEM font16x32[][32] = {
{0x00,0xE0,0xF8,0xFC,0xFE,0x1E,0x07,0x07,0x07,0x07,0x1E,0xFE,0xFC,0xF8,0xF0,0x00,0x00,0x07,0x0F,0x3F,0x3F,0x7C,0x70,0x70,0x70,0x70,0x7C,0x3F,0x1F,0x1F,0x07,0x00},/*"0",0*/
{0x00,0x00,0x00,0x06,0x07,0x07,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7F,0x7F,0x7F,0x7F,0x00,0x00,0x00,0x00,0x00,0x00},/*"1",1*/
{0x00,0x38,0x3C,0x3E,0x3E,0x0F,0x07,0x07,0x07,0xCF,0xFF,0xFE,0xFE,0x38,0x00,0x00,0x00,0x40,0x40,0x60,0x70,0x78,0x7C,0x7E,0x7F,0x77,0x73,0x71,0x70,0x70,0x00,0x00},/*"2",2*/
{0x00,0x18,0x1C,0x1E,0x1E,0x0F,0xC7,0xC7,0xE7,0xFF,0xFE,0xBE,0x9C,0x00,0x00,0x00,0x00,0x0C,0x1C,0x3C,0x3C,0x78,0x70,0x70,0x70,0x79,0x7F,0x3F,0x1F,0x0F,0x00,0x00},/*"3",3*/
{0x00,0x00,0x80,0xC0,0xE0,0x70,0x38,0x1C,0x1E,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x06,0x07,0x07,0x07,0x06,0x06,0x06,0x06,0x06,0x7F,0x7F,0x7F,0x7F,0x06,0x06,0x00},/*"4",4*/
{0x00,0x00,0x00,0x00,0xF0,0xFF,0xFF,0xFF,0xE7,0xE7,0xE7,0xE7,0xC7,0x87,0x00,0x00,0x00,0x00,0x38,0x78,0x71,0x70,0x70,0x70,0x70,0x70,0x39,0x3F,0x3F,0x1F,0x0F,0x00},/*"5",5*/
{0x00,0x80,0xE0,0xF0,0xF8,0xFC,0x7F,0x7F,0x6F,0x67,0xE1,0xE1,0xC0,0x80,0x00,0x00,0x00,0x0F,0x1F,0x3F,0x3F,0x78,0x70,0x70,0x70,0x70,0x78,0x3F,0x3F,0x1F,0x0F,0x00},/*"6",6*/
{0x00,0x07,0x07,0x07,0x07,0x07,0xC7,0xE7,0xF7,0xFF,0x7F,0x3F,0x1F,0x07,0x03,0x01,0x00,0x20,0x38,0x7C,0x7E,0x3F,0x0F,0x07,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00},/*"7",7*/
{0x00,0x00,0x00,0x1C,0xBE,0xFE,0xFF,0xE7,0xC3,0xC3,0xE7,0xFF,0xFE,0xBE,0x1C,0x00,0x00,0x00,0x0E,0x3F,0x3F,0x7F,0x71,0x60,0x60,0x60,0x71,0x7F,0x3F,0x3F,0x0F,0x00},/*"8",8*/
{0x00,0x78,0xFC,0xFE,0xFE,0x8F,0x07,0x07,0x07,0x07,0x8F,0xFE,0xFE,0xFC,0xF8,0x00,0x00,0x00,0x00,0x01,0x43,0x43,0x73,0x7B,0x7F,0x7F,0x1F,0x0F,0x07,0x03,0x00,0x00},/*"9",9*/
};

class COBDDash : public COBD
{
public:
    void Connect()
    {
        char buf[16];

        InitScreen();
        for (int n = 1; !Init(); n++) {
            sprintf(buf, "Connecting [%d]", n);
            if (n <= 20)
                DisplayString(buf);
            else if (n == 21)
                ClearScreen();
        }
        DisplayString("Connected!      ");

        int value;
        DisplayString("Wait ECU start", 0 , 1);
        do {
          delay(500);
        } while (!ReadSensor(PID_RPM, value));
        DisplayString("ECU started   ", 0 , 1);
        delay(500);
        DisplayString("Wait ignition ", 0 , 2);
        delay(500);
        do {
          delay(500);
        } while (!ReadSensor(PID_RPM, value) || value == 0);
        DisplayString("Engine started!", 0 , 2);
        delay(1000);
    }
	void Loop()
	{
        int value;
		byte count = 0;
        ClearScreen();

        DisplayString("rpm", 11, 0);
        DisplayString("km/h", 11, 1);
        DisplayString("AIR: ", 0, 2);
        DisplayString("ENGINE: ", 0, 3);
		for (;;) {
                    char buf[16];

                        if (ReadSensor(PID_RPM, value)) {
                          sprintf(buf, "%4d", value);
                          DisplayLargeNumber(buf, 16, 0);
                        }

                        if (ReadSensor(PID_SPEED, value)) {
                          sprintf(buf, "%3d", value);
                          DisplayLargeNumber(buf, 32, 1);
                        }

                        if (count == 0) {
                          // no need to poll temperature so often
                          if (ReadSensor(PID_INTAKE_TEMP, value)) {
                            sprintf(buf, "%4dC", value);
                            DisplayString(buf, 11, 2);
                          }
                        } else if (count == 1) {
                          // no need to poll temperature so often
                          if (ReadSensor(PID_COOLANT_TEMP, value)) {
                            sprintf(buf, "%4dC", value);
                            DisplayString(buf, 11, 3);
                          }
                        }

                        if (ReadSensor(PID_ENGINE_LOAD, value)) {
                          sprintf(buf, "%d%% ", value);
                          DisplayString(buf, 8, 3);
                        }

                        if (ReadSensor(PID_INTAKE_PRESSURE, value)) {
                          sprintf(buf, "%dkPa ", value);
                          DisplayString(buf, 5, 2);
                        }

                        if (errors > 5) {
                            return;
                        }
                        count++;
		}
	}
private:
        void DisplayString(const char* s, char x = 0, char y = 0)
        {
          ZT.ScI2cMxDisplay8x16Str(OLED_ADDRESS, y << 1, x << 3, s);
        }
        void DisplayLargeNumber(const char* s, char x = 0, char y = 0)
        {
            char data[32];
            y <<= 1;
            while (*s) {
                if (*s >= '0' && *s <= '9') {
                    memcpy_P(data, font16x32[*s - '0'], 32);
                    ZT.ScI2cMxDisplayDot16x16(OLED_ADDRESS, y , x, data);
                } else {
                    ZT.ScI2cMxFillArea(OLED_ADDRESS, y, y + 1, x, x + 16, 0);
                }
                x += 16;
                s++;
            }
        }
        void ClearScreen()
        {
            ZT.ScI2cMxFillArea(OLED_ADDRESS, 0, 6, 0, 127, 0);
            delay(10);
        }
        void InitScreen()
        {
          ZT.I2cInit();
          ZT.ScI2cMxReset(OLED_ADDRESS);
          ClearScreen();
        }
        void Log(const char* s)
        {
#ifdef __AVR_ATmega32U4__
            Serial.println(s);
#endif
        }
#ifdef USE_SOFTSERIAL
        // override data communication functions
        bool DataAvailable() { return mySerial.available(); }
        char ReadData()
        {
          char c = mySerial.read();
          Serial.write(c);
          return c;
        }
        void WriteData(const char* s) { mySerial.write(s); }
        void WriteData(const char c) { mySerial.write(c); }
#endif
	char displayMode;
        int value;
};

COBDDash dash;

void loop()
{
  dash.Connect();
  dash.Loop();
}

void setup()
{
  pinMode(13, OUTPUT);

#ifdef __AVR_ATmega32U4__
  /* On Leonardo, use USB serial for debugging*/
  Serial.begin(9600);
#endif

#ifndef USE_SOFTSERIAL
  OBDUART.begin(OBD_SERIAL_BAUDRATE);
#else
  Serial.begin(9600);
  mySerial.begin(OBD_SERIAL_BAUDRATE);
#endif

}

