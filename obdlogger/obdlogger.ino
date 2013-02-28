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
#include <MultiLCD.h>

#define SD_CS_PIN 10
//#define SD_CS_PIN 4 // ethernet shield

COBD obd;
Sd2Card card;
SdVolume volume;
File sdfile;
CLCD_OLED LCD;

uint32_t filesize = 0;
uint32_t datacount = 0;

bool ShowCardInfo()
{
  if (card.init(SPI_HALF_SPEED, SD_CS_PIN)) {
        char* type;
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
        LCD.PrintString8x16(buf, 0, 0);
        if (!volume.init(card)) {
            LCD.PrintString8x16("No FAT!", 0, 2);
            return false;
        }

        uint32_t volumesize = volume.blocksPerCluster();
        volumesize >>= 1; // 512 bytes per block
        volumesize *= volume.clusterCount();
        volumesize >>= 10;

        sprintf(buf, "SD Size: %dMB", (int)volumesize);
        LCD.PrintString8x16(buf, 0, 2);
        return true;
  } else {
        LCD.PrintString8x16("No SD Card      ", 0, 2);
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
        LCD.PrintString8x16("SD error  ", 0, 4);
    }

    LCD.PrintString8x16(filename, 0, 4);

    filesize = 0;
    sdfile = SD.open(filename, FILE_WRITE);
    if (sdfile) {
        return;
    }

    LCD.PrintString8x16("File error", 0, 4);
}

void InitScreen()
{
    LCD.clear();
    LCD.PrintString8x16("rpm", 84, 0);
    LCD.PrintString8x16("km/h", 84, 3);
}

void setup()
{
    // start serial communication at the adapter defined baudrate
    OBDUART.begin(OBD_SERIAL_BAUDRATE);

    LCD.begin();
    LCD.clear();
    LCD.backlight(true);
    LCD.PrintString8x16("Initializing");

#if 0
    for (byte i = 0; i < 100; i++) {
        Serial.print(i);
        delay(100);
    }
    byte x = 0;
    byte y = 2;
    for (;;) {
        char buf[2];
        if (Serial.available()) {
            buf[0] = Serial.read();
            Serial.write(buf[0]);
            buf[1] = 0;
            LCD.PrintString8x16(buf, x, y % 8);
            x += 8;
            if (x >= 128) {
                x = 0;
                y += 2;
            }
        }
    }
#endif

    // init SD card
    pinMode(SD_CS_PIN, OUTPUT);
    CheckSD();

    // initiate OBD-II connection until success
    LCD.PrintString8x16("Waiting OBD Data", 0, 6);

    while (!obd.Init());
    obd.ReadSensor(PID_DISTANCE, startDistance);

    LCD.PrintString8x16("OBD Connected!  ", 0, 6);
    delay(1000);

    InitScreen();
    lastTime = millis();
}

static char buf[32];
static int len = 0;
static byte pid = 0;
static int value = 0;

void RetrieveData(byte id)
{
    obd.Query(id);

    // flush data in the buffer
    if (len > 0) {
        sdfile.write((uint8_t*)buf, len);
        if (datacount % 100 == 99) {
            sdfile.flush();
            sprintf(buf, "%4u KB", (int)(filesize >> 10));
            LCD.PrintString8x16(buf, 72, 6);
        }

        switch (pid) {
        case PID_RPM:
            sprintf(buf, "%4d", value);
            LCD.PrintString16x16(buf, 16, 0);
            break;
        case PID_SPEED:
            sprintf(buf, "%3d", value);
            LCD.PrintString16x16(buf, 32, 3);
            break;
        case PID_DISTANCE:
            if (value >= startDistance) {
                sprintf(buf, "%d km   ", value - startDistance);
                LCD.PrintString8x16(buf, 0, 6);
            }
            break;
        }
    }

    if (obd.GetResponse(id, value)) {
        uint32_t curTime = millis();
        len = sprintf(buf, "%d,%X,%d\n", (int)(curTime - lastTime), id, value);
        filesize += len;
        datacount++;
        lastTime = curTime;
        pid = id;
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

    if (obd.errors > 2) {
        sdfile.close();
        LCD.clear();
        LCD.PrintString8x16("Reconnecting...");
        digitalWrite(SD_CS_PIN, LOW);
        for (int i = 0; !obd.Init(); i++) {
            if (i == 10) LCD.clear();
        }
        digitalWrite(SD_CS_PIN, HIGH);
        CheckSD();
        delay(1000);
        InitScreen();
        count = 0;
    }
}
