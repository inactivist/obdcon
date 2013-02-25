/*
 * PCD8544 - Interface with Philips PCD8544 (or compatible) LCDs.
 *
 * Copyright (c) 2010 Carlos Rodrigues <cefrodrigues@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include "PCD8544.h"

#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#include <avr/pgmspace.h>


#define PCD8544_CMD  LOW
#define PCD8544_DATA HIGH

extern const PROGMEM unsigned char font5x8[][5];

/*
 * If this was a ".h", it would get added to sketches when using
 * the "Sketch -> Import Library..." menu on the Arduino IDE...
 */

PCD8544::PCD8544(unsigned char sclk, unsigned char sdin,
                 unsigned char dc, unsigned char reset,
                 unsigned char sce):
    pin_sclk(sclk),
    pin_sdin(sdin),
    pin_dc(dc),
    pin_reset(reset),
    pin_sce(sce)
{}


void PCD8544::begin(unsigned char width, unsigned char height, unsigned char model)
{
    width = width;
    height = height;

    column = 0;
    line = 0;

    // Sanitize the custom glyphs...
    memset(custom, 0, sizeof(custom));

    // All pins are outputs (these displays cannot be read)...
    pinMode(pin_sclk, OUTPUT);
    pinMode(pin_sdin, OUTPUT);
    pinMode(pin_dc, OUTPUT);
    pinMode(pin_reset, OUTPUT);
    pinMode(pin_sce, OUTPUT);

    // Reset the controller state...
    digitalWrite(pin_reset, HIGH);
    digitalWrite(pin_sce, HIGH);
    digitalWrite(pin_reset, LOW);
    delay(100);
    digitalWrite(pin_reset, HIGH);

    // Set the LCD parameters...
    send(PCD8544_CMD, 0x21);  // extended instruction set control (H=1)
    send(PCD8544_CMD, 0x13);  // bias system (1:48)

    if (model == CHIP_ST7576) {
        send(PCD8544_CMD, 0xe0);  // higher Vop, too faint at default
        send(PCD8544_CMD, 0x05);  // partial display mode
    } else {
        send(PCD8544_CMD, 0xc2);  // default Vop (3.06 + 66 * 0.06 = 7V)
    }

    send(PCD8544_CMD, 0x20);  // extended instruction set control (H=0)
    send(PCD8544_CMD, 0x09);  // all display segments on

    // Clear RAM contents...
    clear();

    // Activate LCD...
    send(PCD8544_CMD, 0x08);  // display blank
    send(PCD8544_CMD, 0x0c);  // normal mode (0x0d = inverse mode)
    delay(100);

    // Place the cursor at the origin...
    send(PCD8544_CMD, 0x80);
    send(PCD8544_CMD, 0x40);
}


void PCD8544::stop()
{
    clear();
    setPower(false);
}


void PCD8544::clear()
{
    setCursor(0, 0);

    for (unsigned short i = 0; i < width * (height/8); i++) {
        send(PCD8544_DATA, 0x00);
    }

    setCursor(0, 0);
}


void PCD8544::clearLine()
{
    setCursor(0, line);

    for (unsigned char i = 0; i < width; i++) {
        send(PCD8544_DATA, 0x00);
    }

    setCursor(0, line);
}


void PCD8544::setPower(bool on)
{
    send(PCD8544_CMD, on ? 0x20 : 0x24);
}


inline void PCD8544::display()
{
    setPower(true);
}


inline void PCD8544::noDisplay()
{
    setPower(false);
}


void PCD8544::setInverse(bool inverse)
{
    send(PCD8544_CMD, inverse ? 0x0d : 0x0c);
}


void PCD8544::home()
{
    setCursor(0, line);
}


void PCD8544::setCursor(unsigned char column, unsigned char line)
{
    column = (column % width);
    line = (line % (height/9 + 1));

    send(PCD8544_CMD, 0x80 | column);
    send(PCD8544_CMD, 0x40 | line);
}


void PCD8544::createChar(unsigned char chr, const unsigned char *glyph)
{
    // ASCII 0-31 only...
    if (chr >= ' ') {
        return;
    }

    custom[chr] = glyph;
}


#if ARDUINO < 100
void PCD8544::write(uint8_t chr)
#else
size_t PCD8544::write(uint8_t chr)
#endif
{
    // ASCII 7-bit only...
    if (chr >= 0x80) {
#if ARDUINO < 100
        return;
#else
        return 0;
#endif
    }

    const unsigned char *glyph;
    unsigned char pgm_buffer[5];

    if (chr >= ' ') {
        // Regular ASCII characters are kept in flash to save RAM...
        memcpy_P(pgm_buffer, &font5x8[chr - ' '], sizeof(pgm_buffer));
        glyph = pgm_buffer;
    } else {
        // Custom glyphs, on the other hand, are stored in RAM...
        if (custom[chr]) {
            glyph = custom[chr];
        } else {
            // Default to a space character if unset...
            memcpy_P(pgm_buffer, &font5x8[0], sizeof(pgm_buffer));
            glyph = pgm_buffer;
        }
    }

    // Output one column at a time...
    for (unsigned char i = 0; i < 5; i++) {
        send(PCD8544_DATA, glyph[i]);
    }

    // One column between characters...
    send(PCD8544_DATA, 0x00);

    // Update the cursor position...
    column = (column + 6) % width;

    if (column == 0) {
        line = (line + 1) % (height/9 + 1);
    }

#if ARDUINO >= 100
    return 1;
#endif
}


void PCD8544::drawBitmap(const unsigned char *data, unsigned char columns, unsigned char lines)
{
    unsigned char scolumn = column;
    unsigned char sline = line;

    // The bitmap will be clipped at the right/bottom edge of the display...
    unsigned char mx = (scolumn + columns > width) ? (width - scolumn) : columns;
    unsigned char my = (sline + lines > height/8) ? (height/8 - sline) : lines;

    for (unsigned char y = 0; y < my; y++) {
        setCursor(scolumn, sline + y);

        for (unsigned char x = 0; x < mx; x++) {
            send(PCD8544_DATA, data[y * columns + x]);
        }
    }

    // Leave the cursor in a consistent position...
    setCursor(scolumn + columns, sline);
}


void PCD8544::drawColumn(unsigned char lines, unsigned char value)
{
    unsigned char scolumn = column;
    unsigned char sline = line;

    // Keep "value" within range...
    if (value > lines*8) {
        value = lines*8;
    }

    // Find the line where "value" resides...
    unsigned char mark = (lines*8 - 1 - value)/8;

    // Clear the lines above the mark...
    for (unsigned char line = 0; line < mark; line++) {
        setCursor(scolumn, sline + line);
        send(PCD8544_DATA, 0x00);
    }

    // Compute the byte to draw at the "mark" line...
    unsigned char b = 0xff;
    for (unsigned char i = 0; i < lines*8 - mark*8 - value; i++) {
        b <<= 1;
    }

    setCursor(scolumn, sline + mark);
    send(PCD8544_DATA, b);

    // Fill the lines below the mark...
    for (unsigned char line = mark + 1; line < lines; line++) {
        setCursor(scolumn, sline + line);
        send(PCD8544_DATA, 0xff);
    }

    // Leave the cursor in a consistent position...
    setCursor(scolumn + 1, sline);
}


void PCD8544::send(unsigned char type, unsigned char data)
{
    digitalWrite(pin_dc, type);

    digitalWrite(pin_sce, LOW);
    shiftOut(pin_sdin, pin_sclk, MSBFIRST, data);
    digitalWrite(pin_sce, HIGH);
}


/* vim: set expandtab ts=4 sw=4: */
