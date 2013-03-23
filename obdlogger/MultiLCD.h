class LCD_Null {
public:
    virtual void PrintString8x16(const char* s, char x = 0, char y = 0) {}
    virtual void PrintString16x16(const char* s, char x = 0, char y = 0) {}
    virtual void clear() {}
    virtual void begin() {}
};

extern const PROGMEM unsigned char font16x32[][32];
extern const PROGMEM unsigned char font5x8[][5];

#include "PCD8544.h"

class LCD_PCD8544 : public PCD8544 {
public:
    void PrintString8x16(const char* s, char x = 0, char y = 0)
    {
        setCursor(x, y * 4);
        while (*s++) write(*s);
    }
    void PrintString16x16(const char* s, char x = 0, char y = 0) {}
    void backlight(bool on)
    {
        pinMode(2, OUTPUT);
        digitalWrite(2, on ? LOW : HIGH);
    }
};

#include "ZtLib.h"

#define OLED_ADDRESS 0x27

class LCD_OLED : public ZtLib {
public:
    void PrintString8x16(const char* s, char x = 0, char y = 0);
    void PrintString16x16(const char* s, char x = 0, char y = 0);
    void clear();
    void begin();
    void backlight(bool on) {}
};
