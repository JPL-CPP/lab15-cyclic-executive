/*
 * File:   lab15.c
 * Author: Jacky Li
 *
 * Lab 15 - Station Keeping: The Cyclic Executive
 * ECE 3301L - Introduction to Microcontrollers Laboratory
 *
 * A miniature flight computer: three "station subsystems" run as
 * rate groups under an assembly cyclic executive (executive.S).
 *
 *   100 Hz  task_sensor     sample the pot (ADC AN0)
 *    10 Hz  task_thermal    PWM fan speed follows the pot (Lab 10)
 *     1 Hz  task_telemetry  LCD status page (Lab 5/7 driver)
 *
 * Timer0 fires every 10 ms (the minor frame). The C ISR is only
 * vector plumbing - it reloads the timer and calls the asm
 * frame_tick(), which owns all scheduling decisions including
 * overrun triage: if a frame overruns, telemetry is shed but the
 * sensor/control loop keeps its deadline.
 *
 * Fault injection: hold S1 (RB4) to make the thermal task busy-wait
 * far past its budget. Watch overruns count up on the LCD while the
 * RC0 frame marker on the scope stays a rock-solid 50 Hz square wave.
 *
 * Pin Assignments:
 *   RA0 (AN0)  - potentiometer
 *   RC2/CCP1   - PWM out -> 1K -> TIP122 base (fan, Lab 10 wiring)
 *   RC0        - frame marker (scope)
 *   RD0        - LCD RS,  RD2 - LCD EN,  RA4-RA7 - LCD D4-D7
 *   RB4 (S1)   - overrun injection button
 */

#include <xc.h>
#include <stdint.h>
#include "PIC18F46K22-Config.h"
#include "LCD.h"

#define _XTAL_FREQ 16000000UL

/* ---- Executive state shared with executive.S ---- */
volatile uint8_t ex_busy, ex_frame_flag, ex_run_10hz, ex_run_1hz;
volatile uint8_t ex_skip_slow, ex_overruns;
extern void exec_init(void);
extern void frame_tick(void);

/* Timer0 reload for a 10 ms frame: 4 MHz instr clock, 16-bit, 1:1.
 * 65536 - 40000 = 25536 = 0x63C0. */
#define FRAME_RELOAD_H  0x63
#define FRAME_RELOAD_L  0xC0

static volatile uint16_t frame_count = 0;   /* for telemetry */
static uint8_t sensor_val = 0;              /* latest sample  */

/* ---- The ISR: vector plumbing only. Decisions live in asm. ---- */
void __interrupt() isr(void) {
    if (INTCONbits.TMR0IF) {
        TMR0H = FRAME_RELOAD_H;             /* H must be written first */
        TMR0L = FRAME_RELOAD_L;
        INTCONbits.TMR0IF = 0;
        frame_count++;
        frame_tick();                       /* asm executive takes over */
    }
}

static void init(void) {
    OSCCONbits.IRCF = 0b111;    /* 16 MHz HFINTOSC */
    OSCCONbits.SCS  = 0b10;

    ANSELA = 0x00; ANSELB = 0x00; ANSELC = 0x00; ANSELD = 0x00;
    ANSELAbits.ANSA0 = 1;  TRISAbits.TRISA0 = 1;    /* pot */
    TRISCbits.TRISC0 = 0;  LATCbits.LATC0 = 0;      /* frame marker */
    TRISBbits.TRISB4 = 1;                            /* S1 */

    /* ADC (Lab 6 settings) */
    ADCON0bits.CHS = 0;  ADCON0bits.ADON = 1;
    ADCON1 = 0x00;
    ADCON2bits.ADFM = 1; ADCON2bits.ACQT = 0b100; ADCON2bits.ADCS = 0b010;

    /* PWM on CCP1 (Lab 10 settings, ~2 kHz) */
    TRISCbits.TRISC2 = 0;  LATCbits.LATC2 = 0;
    PR2 = 124;
    CCPR1L = 0;  CCP1CONbits.DC1B = 0;
    CCP1CONbits.CCP1M = 0b1100;
    T2CONbits.T2CKPS = 0b11;  T2CONbits.TMR2ON = 1;

    /* Timer0: 16-bit, Fosc/4, no prescaler, 10 ms frame */
    T0CONbits.T08BIT = 0;
    T0CONbits.T0CS   = 0;
    T0CONbits.PSA    = 1;
    TMR0H = FRAME_RELOAD_H;  TMR0L = FRAME_RELOAD_L;
    INTCONbits.TMR0IF = 0;
    INTCONbits.TMR0IE = 1;
    T0CONbits.TMR0ON = 1;
}

/* ---------------- Rate-group tasks ---------------- */

static void task_sensor(void) {             /* 100 Hz */
    ADCON0bits.GO = 1;
    while (ADCON0bits.GO);
    sensor_val = (uint8_t)((((uint16_t)ADRESH << 8) | ADRESL) >> 2);
}

static void task_thermal(void) {            /* 10 Hz */
    /* pot 0-255 -> duty 0-500 (approx x2): fan follows the knob */
    uint16_t duty = (uint16_t)sensor_val * 2;
    CCPR1L = (uint8_t)(duty >> 2);
    CCP1CONbits.DC1B = duty & 0x03;

    /* Fault injection: S1 held -> blow the frame budget (~25 ms) */
    if (PORTBbits.RB4 == 0) {
        __delay_ms(25);
    }
}

static void task_telemetry(void) {          /* 1 Hz */
    LCD_cursor_set(1, 1);
    LCD_write_string("F:");
    LCD_write_variable((int32_t)frame_count, 5);
    LCD_write_string(" OV:");
    LCD_write_variable((int32_t)ex_overruns, 3);

    LCD_cursor_set(2, 1);
    LCD_write_string("ADC:");
    LCD_write_variable((int32_t)sensor_val, 3);
    LCD_write_string(" PWM:");
    LCD_write_variable((int32_t)((uint16_t)sensor_val * 100 / 255), 3);
    LCD_write_string("%");
}

/* ---------------- Main: consume what the executive schedules ------ */

void main(void) {
    init();
    LCD_init();
    LCD_clear();

    exec_init();                /* asm: arm the rate counters */
    INTCONbits.GIE = 1;         /* frames start landing NOW */

    while (1) {
        if (ex_frame_flag) {
            ex_frame_flag = 0;
            ex_busy = 1;                    /* frame work in progress */

            task_sensor();                  /* 100 Hz: always */

            if (ex_run_10hz) {
                ex_run_10hz = 0;
                if (!ex_skip_slow) task_thermal();
            }
            if (ex_run_1hz) {
                ex_run_1hz = 0;
                if (!ex_skip_slow) task_telemetry();
                ex_skip_slow = 0;           /* recovered: resume slow work */
            }

            ex_busy = 0;                    /* made the deadline */
        }
    }
}
