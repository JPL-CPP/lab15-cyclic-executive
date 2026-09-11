/*
 * File:   OLED.c
 * ECE 3301L - provided driver for the SSD1331 RGB OLED (do not modify)
 *
 * SPI master on MSSP2 at Fosc/4 (4 Mbit/s at 16 MHz).
 * Screen clears and filled rectangles use the SSD1331's hardware
 * acceleration (a 5-11 byte command instead of streaming pixels);
 * text is streamed as RGB565 pixels, 96 bytes per character cell.
 * That difference is why the telemetry task budgets its drawing.
 */

#include <xc.h>
#include <stdint.h>
#include "OLED.h"

#define _XTAL_FREQ 16000000UL

/* Control pins */
#define OLED_CS   LATDbits.LATD5
#define OLED_DC   LATDbits.LATD2
#define OLED_RST  LATDbits.LATD3

/* ---- 5x7 font, ASCII 32..95 (space..underscore), column-major ---- */
static const uint8_t font5x7[64][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* ' ' */  {0x00,0x00,0x5F,0x00,0x00}, /* ! */
    {0x00,0x07,0x00,0x07,0x00}, /* " */    {0x14,0x7F,0x14,0x7F,0x14}, /* # */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* $ */    {0x23,0x13,0x08,0x64,0x62}, /* % */
    {0x36,0x49,0x55,0x22,0x50}, /* & */    {0x00,0x05,0x03,0x00,0x00}, /* ' */
    {0x00,0x1C,0x22,0x41,0x00}, /* ( */    {0x00,0x41,0x22,0x1C,0x00}, /* ) */
    {0x14,0x08,0x3E,0x08,0x14}, /* * */    {0x08,0x08,0x3E,0x08,0x08}, /* + */
    {0x00,0x50,0x30,0x00,0x00}, /* , */    {0x08,0x08,0x08,0x08,0x08}, /* - */
    {0x00,0x60,0x60,0x00,0x00}, /* . */    {0x20,0x10,0x08,0x04,0x02}, /* / */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 0 */    {0x00,0x42,0x7F,0x40,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 2 */    {0x21,0x41,0x45,0x4B,0x31}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 4 */    {0x27,0x45,0x45,0x45,0x39}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 6 */    {0x01,0x71,0x09,0x05,0x03}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 8 */    {0x06,0x49,0x49,0x29,0x1E}, /* 9 */
    {0x00,0x36,0x36,0x00,0x00}, /* : */    {0x00,0x56,0x36,0x00,0x00}, /* ; */
    {0x08,0x14,0x22,0x41,0x00}, /* < */    {0x14,0x14,0x14,0x14,0x14}, /* = */
    {0x00,0x41,0x22,0x14,0x08}, /* > */    {0x02,0x01,0x51,0x09,0x06}, /* ? */
    {0x32,0x49,0x79,0x41,0x3E}, /* @ */    {0x7E,0x11,0x11,0x11,0x7E}, /* A */
    {0x7F,0x49,0x49,0x49,0x36}, /* B */    {0x3E,0x41,0x41,0x41,0x22}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* D */    {0x7F,0x49,0x49,0x49,0x41}, /* E */
    {0x7F,0x09,0x09,0x09,0x01}, /* F */    {0x3E,0x41,0x49,0x49,0x7A}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* H */    {0x00,0x41,0x7F,0x41,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01}, /* J */    {0x7F,0x08,0x14,0x22,0x41}, /* K */
    {0x7F,0x40,0x40,0x40,0x40}, /* L */    {0x7F,0x02,0x0C,0x02,0x7F}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* N */    {0x3E,0x41,0x41,0x41,0x3E}, /* O */
    {0x7F,0x09,0x09,0x09,0x06}, /* P */    {0x3E,0x41,0x51,0x21,0x5E}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* R */    {0x46,0x49,0x49,0x49,0x31}, /* S */
    {0x01,0x01,0x7F,0x01,0x01}, /* T */    {0x3F,0x40,0x40,0x40,0x3F}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F}, /* V */    {0x3F,0x40,0x38,0x40,0x3F}, /* W */
    {0x63,0x14,0x08,0x14,0x63}, /* X */    {0x07,0x08,0x70,0x08,0x07}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}, /* Z */    {0x00,0x7F,0x41,0x41,0x00}, /* [ */
    {0x02,0x04,0x08,0x10,0x20}, /* \ */    {0x00,0x41,0x41,0x7F,0x00}, /* ] */
    {0x04,0x02,0x01,0x02,0x04}, /* ^ */    {0x40,0x40,0x40,0x40,0x40}  /* _ */
};

/* ---- SPI primitives ---- */
static void spi_byte(uint8_t b) {
    SSP2BUF = b;
    while (!SSP2STATbits.BF);
    (void)SSP2BUF;
}

static void cmd1(uint8_t c) {
    OLED_DC = 0; OLED_CS = 0;
    spi_byte(c);
    OLED_CS = 1;
}

static void cmd2(uint8_t c, uint8_t a) { cmd1(c); cmd1(a); }

/* 6-bit per channel color triplet for the accelerated draw commands */
static void cmd_color(uint16_t rgb565) {
    cmd1((uint8_t)((rgb565 >> 10) & 0x3E));     /* R5 -> 6-bit */
    cmd1((uint8_t)((rgb565 >> 5)  & 0x3F));     /* G6           */
    cmd1((uint8_t)((rgb565 << 1)  & 0x3E));     /* B5 -> 6-bit */
}

static void set_window(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    cmd1(0x15); cmd1(x1); cmd1(x2);             /* column range */
    cmd1(0x75); cmd1(y1); cmd1(y2);             /* row range    */
}

void OLED_init(void) {
    /* Pins: RD0=SCK2, RD4=SDO2 outputs; RD2/RD3/RD5 GPIO outputs */
    ANSELD = 0x00;
    TRISDbits.TRISD0 = 0;
    TRISDbits.TRISD4 = 0;
    TRISDbits.TRISD2 = 0;
    TRISDbits.TRISD3 = 0;
    TRISDbits.TRISD5 = 0;
    OLED_CS = 1; OLED_DC = 0;

    /* MSSP2: SPI master, Fosc/4, mode 0, sample middle */
    SSP2CON1 = 0x00;
    SSP2STATbits.CKE = 1;
    SSP2CON1bits.SSPM = 0b0000;     /* SPI master, clock = Fosc/4 */
    SSP2CON1bits.CKP  = 0;
    SSP2CON1bits.SSPEN = 1;

    /* Hardware reset pulse */
    OLED_RST = 0; __delay_ms(5);
    OLED_RST = 1; __delay_ms(5);

    /* SSD1331 init (datasheet recommended sequence) */
    cmd1(0xAE);                     /* display off              */
    cmd2(0xA0, 0x72);               /* remap: RGB, 65k color    */
    cmd2(0xA1, 0x00);               /* start line 0             */
    cmd2(0xA2, 0x00);               /* display offset 0         */
    cmd1(0xA4);                     /* normal display           */
    cmd2(0xA8, 0x3F);               /* multiplex 1/64           */
    cmd2(0xAD, 0x8E);               /* master configure         */
    cmd2(0xB0, 0x0B);               /* power save off           */
    cmd2(0xB1, 0x31);               /* phase 1/2 period         */
    cmd2(0xB3, 0xF0);               /* clock divide / osc freq  */
    cmd2(0x8A, 0x64);               /* precharge A              */
    cmd2(0x8B, 0x78);               /* precharge B              */
    cmd2(0x8C, 0x64);               /* precharge C              */
    cmd2(0xBB, 0x3A);               /* precharge level          */
    cmd2(0xBE, 0x3E);               /* VCOMH                    */
    cmd2(0x87, 0x06);               /* master current           */
    cmd2(0x81, 0x91);               /* contrast A               */
    cmd2(0x82, 0x50);               /* contrast B               */
    cmd2(0x83, 0x7D);               /* contrast C               */
    OLED_clear();
    cmd1(0xAF);                     /* display on               */
    __delay_ms(100);
}

void OLED_clear(void) {
    /* Hardware window clear: 5 command bytes wipe 12 KB of pixels. */
    cmd1(0x25);
    cmd1(0); cmd1(0);
    cmd1(OLED_W - 1); cmd1(OLED_H - 1);
    __delay_ms(1);                  /* GAC needs a moment */
}

void OLED_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                    uint16_t color) {
    if (w == 0 || h == 0) return;
    cmd2(0x26, 0x01);               /* enable rectangle fill    */
    cmd1(0x22);                     /* draw rectangle           */
    cmd1(x); cmd1(y);
    cmd1((uint8_t)(x + w - 1)); cmd1((uint8_t)(y + h - 1));
    cmd_color(color);               /* outline                  */
    cmd_color(color);               /* fill                     */
    __delay_us(500);                /* GAC draw time            */
}

void OLED_text(uint8_t row, uint8_t col, const char *s, uint16_t color) {
    while (*s && col < 16) {
        uint8_t ch = (uint8_t)*s++;
        if (ch >= 'a' && ch <= 'z') ch -= 32;   /* fold lowercase */
        if (ch < 32 || ch > 95) ch = 32;
        const uint8_t *glyph = font5x7[ch - 32];

        set_window((uint8_t)(col * 6), (uint8_t)(row * 8),
                   (uint8_t)(col * 6 + 5), (uint8_t)(row * 8 + 7));
        OLED_DC = 1; OLED_CS = 0;
        for (uint8_t y = 0; y < 8; y++) {       /* window fills row-wise */
            for (uint8_t x = 0; x < 6; x++) {
                uint8_t on = (x < 5) && ((glyph[x] >> y) & 1);
                uint16_t px = on ? color : OLED_BLACK;
                spi_byte((uint8_t)(px >> 8));
                spi_byte((uint8_t)(px & 0xFF));
            }
        }
        OLED_CS = 1;
        col++;
    }
}
