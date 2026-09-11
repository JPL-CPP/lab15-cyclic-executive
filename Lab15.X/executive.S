; =====================================================================
; File:   executive.S
; Author: <your names here>
;
; Lab 15 - Station Keeping: The Cyclic Executive
; ECE 3301L - Introduction to Microcontrollers Laboratory
;
; YOU WRITE the scheduling core. The C side (lab15.c, provided, do
; not modify) owns the Timer0 ISR shell and calls frame_tick() every
; 10 ms; your code makes every scheduling DECISION:
;
;   - toggle the RC0 frame marker (scope heartbeat)
;   - overrun detection: ex_busy still set when the tick lands means
;     the last frame's work overran -> count it in ex_overruns and
;     set ex_skip_slow (shed telemetry, protect the control loop)
;   - rate groups: set ex_frame_flag every tick; ex_run_10hz every
;     10th tick; ex_run_1hz every 100th tick
;
; C-side globals (all volatile uint8_t):
;   ex_busy, ex_frame_flag, ex_run_10hz, ex_run_1hz,
;   ex_skip_slow, ex_overruns
;
; Reminder: C symbol `x` is `_x` here; C globals are banked, so use
; banksel + BANKMASK(_x),b to touch them. Your own counters can live
; in the access bank (udata_acs) - no banking needed.
; =====================================================================

#include <xc.inc>

GLOBAL  _frame_tick, _exec_init
GLOBAL  _ex_busy, _ex_frame_flag, _ex_run_10hz, _ex_run_1hz
GLOBAL  _ex_skip_slow, _ex_overruns

PSECT udata_acs
c10:    DS 1                    ; frames until the next 10 Hz slot
c100:   DS 1                    ; frames until the next 1 Hz slot

PSECT extext,class=CODE,reloc=2

; ---------------------------------------------------------------------
; exec_init - called once from C before the frame timer starts.
; TODO: load c10 with 10 and c100 with 100.
; (Why not zero? Trace what decfsz does to a counter that starts
; at 0 - that bug costs real teams real launches.)
; ---------------------------------------------------------------------
_exec_init:
    ; TODO
    return

; ---------------------------------------------------------------------
; frame_tick - called from the Timer0 ISR, exactly every 10 ms.
; Keep it SHORT: every cycle here is stolen from the frame.
;
; TODO:
;   1. Toggle the RC0 frame marker (btg LATC,0,a)
;   2. Overrun check: if _ex_busy != 0 (tstfsz), increment
;      _ex_overruns (saturate at 255) and set _ex_skip_slow (setf)
;   3. Set _ex_frame_flag (the 100 Hz group is always due)
;   4. decfsz c10 -> reached 0? reload 10, set _ex_run_10hz
;   5. decfsz c100 -> reached 0? reload 100, set _ex_run_1hz
; ---------------------------------------------------------------------
_frame_tick:
    ; TODO - implement the executive tick (see steps above)
    return

    END
