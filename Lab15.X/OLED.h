/*
 * File:   OLED.h
 * ECE 3301L - provided driver (do not modify OLED.c)
 *
 * SSD1331 0.96" RGB OLED, 96x64, 65k color, SPI via MSSP2.
 *
 * Wiring (module pin -> PIC):
 *   GND -> GND          VCC -> 3.3V (module has its own regulator)
 *   SCL/CLK  -> RD0 (SCK2)      SDA/MOSI -> RD4 (SDO2)
 *   RES/RST  -> RD3             DC       -> RD2
 *   CS       -> RD5
 *
 * Text grid: 6x8-pixel cells -> 16 columns x 8 rows.
 * Font covers ASCII 32..95 (digits, UPPERCASE, punctuation).
 *
 * Colors are RGB565. Use the macros or OLED_rgb(r,g,b) with r,b in
 * 0-31 and g in 0-63.
 */

#ifndef OLED_DRIVER_H
#define OLED_DRIVER_H

#include <stdint.h>

#define OLED_W 96
#define OLED_H 64

#define OLED_BLACK   0x0000
#define OLED_WHITE   0xFFFF
#define OLED_RED     0xF800
#define OLED_GREEN   0x07E0
#define OLED_BLUE    0x001F
#define OLED_CYAN    0x07FF
#define OLED_YELLOW  0xFFE0
#define OLED_ORANGE  0xFC60
#define OLED_MAGENTA 0xF81F

#define OLED_rgb(r,g,b) ((uint16_t)(((uint16_t)(r) << 11) | \
                         ((uint16_t)(g) << 5) | (uint16_t)(b)))

void OLED_init(void);                       /* SPI2 + panel init + clear */
void OLED_clear(void);                      /* hardware clear (fast)     */
void OLED_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                    uint16_t color);        /* hardware fill (fast)      */
void OLED_text(uint8_t row, uint8_t col, const char *s,
               uint16_t color);             /* 6x8 text, streamed        */

#endif /* OLED_DRIVER_H */
