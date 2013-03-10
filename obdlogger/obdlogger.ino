/*************************************************************************
* Arduino OBD-II Data Logger
* Distributed under GPL v2.0
* Copyright (c) 2013 Stanley Huang <stanleyhuangyc@gmail.com>
* All rights reserved.
*************************************************************************/

#include <Arduino.h>
#include <OBD.h>
#include <SD.h>
#include <Wire.h>
#include <Multilcd.h>
#include <TinyGPS.h>

#define SD_CS_PIN 10
//#define SD_CS_PIN 4 // ethernet shield

// addition PIDs (non-OBD)
#define PID_GPS_DATETIME 0xF01
#define PID_GPS_COORDINATE 0xF02
#define PID_GPS_ALTITUDE 0xF03
#define PID_GPS_SPEED 0xF04

// GPS logging can only be enabled when there is additional serial UART
#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
#define ENABLE_GPS
#endif // defined

COBD obd;
#ifdef ENABLE_GPS
TinyGPS gps;
#endif
Sd2Card card;
SdVolume volume;
File sdfile;
LCD_OLED lcd;

uint32_t filesize = 0;
uint32_t datacount = 0;

void ProcessGPSData(char c);

bool ShowCardInfo()
{
  if (card.init(SPI_HALF_SPEED, SD_CS_PIN)) {
        const char* type;
        char buf[20];

        switch(card.type()) {
        case SD_CARD_TYPE_SD1:
            type = "SD1";
            break;
        case SD_CARD_TYPE_SD2:
            type = "SD2";
            break;
        case SD_CARD_TYPE_SDHC:
            type = "SDHC";
            break;
        default:
            type = "N/A";
        }

        sprintf(buf, "SD Type: %s", type);
        lcd.PrintString8x16(buf, 0, 0);
        if (!volume.init(card)) {
            lcd.PrintString8x16("No FAT!", 0, 2);
            return false;
        }

        uint32_t volumesize = volume.blocksPerCluster();
        volumesize >>= 1; // 512 bytes per block
        volumesize *= volume.clusterCount();
        volumesize >>= 10;

        sprintf(buf, "SD Size: %dMB", (int)volumesize);
        lcd.PrintString8x16(buf, 0, 2);
        return true;
  } else {
        lcd.PrintString8x16("No SD Card      ", 0, 2);
        return false;
  }
}

static uint32_t lastTime;
static int startDistance = 0;

static void CheckSD()
{
    uint16_t fileidx = 0;
    char filename[13];
    for (;;) {
        if (!ShowCardInfo()) {
            delay(1000);
            continue;
        }

        SD.begin(SD_CS_PIN);

        // determine file name
        for (uint16_t index = 1; index < 65535; index++) {
            sprintf(filename, "OBD%05d.CSV", index);
            if (!SD.exists(filename)) {
                fileidx = index;
                break;
            }
        }
        if (fileidx) break;
        lcd.PrintString8x16("SD error  ", 0, 4);
    }

    lcd.PrintString8x16(filename, 0, 4);

    filesize = 0;
    sdfile = SD.open(filename, FILE_WRITE);
    if (sdfile) {
        return;
    }

    lcd.PrintString8x16("File error", 0, 4);
}

void InitScreen()
{
    lcd.clear();
    lcd.PrintString8x16("kph", 92, 0);
    lcd.PrintString8x16("rpm", 92, 2);
}

void setup()
{
    // start serial communication at the adapter defined baudrate
    OBDUART.begin(OBD_SERIAL_BAUDRATE);
#ifdef ENABLE_GPS
    Serial2.begin(4800);
#endif

    lcd.begin();
    lcd.clear();
    lcd.backlight(true);
    lcd.PrintString8x16("Initializing");

    // init SD card
    pinMode(SD_CS_PIN, OUTPUT);
    CheckSD();

    // initiate OBD-II connection until success
    lcd.PrintString8x16("Waiting OBD Data", 0, 6);

    while (!obd.Init());
    obd.ReadSensor(PID_DISTANCE, startDistance);

    lcd.PrintString8x16("OBD Connected!  ", 0, 6);
    delay(1000);

    InitScreen();
    lastTime = millis();
}

static char databuf[32];
static int len = 0;
static int value = 0;

#ifdef ENABLE_GPS
void ProcessGPSData(char c)
{
    if (!gps.encode(c))
        return;

    // parsed GPS data is ready
    uint32_t curTime = millis();
    unsigned long fix_age;

    {
        unsigned long date, time;
        gps.get_datetime(&date, &time, &fix_age);
        len = sprintf(databuf, "%d,F01,%ld %ld\n", (int)(curTime - lastTime), date, time);
        sdfile.write((uint8_t*)databuf, len);
    }

    {
        long lat, lon;
        gps.get_position(&lat, &lon, &fix_age);
        len = sprintf(databuf, "%d,F02,%ld %ld\n", (int)(curTime - lastTime), lat, lon);
        sdfile.write((uint8_t*)databuf, len);
        // display LAT/LON
        sprintf(databuf, "%ld", lat);
        lcd.PrintString8x16(databuf, 0, 4);
        sprintf(databuf, "%ld", lon);
        lcd.PrintString8x16(databuf, 8 * 8, 4);
    }
    len = sprintf(databuf, "%d,F03,%ld %ld\n", (int)(curTime - lastTime), gps.speed() * 1852 / 100);
    sdfile.write((uint8_t*)databuf, len);

    len = sprintf(databuf, "%d,F04,%ld %ld\n", (int)(curTime - lastTime), gps.altitude());
    sdfile.write((uint8_t*)databuf, len);
    lastTime = curTime;
}
#endif

void RetrieveData(byte pid)
{
    // issue a query for OBD
    obd.Query(pid);

    // flush data in the buffer
    if (len > 0) {
        sdfile.write((uint8_t*)databuf, len);

        char* buf = databuf; // data in buffer saved, free for other use
        if (datacount % 100 == 99) {
            sdfile.flush();
            sprintf(buf, "%4u KB", (int)(filesize >> 10));
            lcd.PrintString8x16(buf, 72, 6);
        }

        switch (pid) {
        case PID_RPM:
            sprintf(buf, "%4d", value);
            lcd.PrintString16x16(buf, 0, 0);
            break;
        case PID_SPEED:
            sprintf(buf, "%3d", value);
            lcd.PrintString16x16(buf, 16, 2);
            break;
        case PID_DISTANCE:
            if (value >= startDistance) {
                sprintf(buf, "%d km   ", value - startDistance);
                lcd.PrintString8x16(buf, 0, 6);
            }
            break;
        }
    }

#ifdef ENABLE_GPS
    while (Serial2.available()) {
        ProcessGPSData(Serial2.read());
    }
#endif

    if (obd.GetResponse(pid, value)) {
        uint32_t curTime = millis();
        len = sprintf(databuf, "%d,%X,%d\n", (int)(curTime - lastTime), pid, value);
        filesize += len;
        datacount++;
        lastTime = curTime;
        return;
    }
    len = 0;
}

void loop()
{
    static char count = 0;

    switch (count++) {
    case 0:
    case 64:
    case 128:
    case 192:
        RetrieveData(PID_DISTANCE);
        break;
    case 4:
        RetrieveData(PID_COOLANT_TEMP);
        break;
    case 20:
        RetrieveData(PID_INTAKE_TEMP);
        break;
    }

    RetrieveData(PID_RPM);
    RetrieveData(PID_SPEED);
    //RetrieveData(PID_THROTTLE);
    //RetrieveData(PID_ABS_ENGINE_LOAD);

    if (obd.errors >= 5) {
        sdfile.close();
        lcd.clear();
        lcd.PrintString8x16("Reconnecting...");
        digitalWrite(SD_CS_PIN, LOW);
        for (int i = 0; !obd.Init(); i++) {
            if (i == 10) lcd.clear();
        }
        digitalWrite(SD_CS_PIN, HIGH);
        CheckSD();
        delay(1000);
        InitScreen();
        count = 0;
    }
}
