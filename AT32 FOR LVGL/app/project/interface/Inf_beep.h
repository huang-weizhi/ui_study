/*=============================================================================
 * Inf_beep.h - Active buzzer driver interface
 *
 *   Hardware: active buzzer, high level = on; pin in at32f403a_407_wk_config.h
 *         OUT_BEEP_H_PIN / OUT_BEEP_H_GPIO_PORT (PC3)
 *   Note: an active buzzer can only sound/silence (fixed tone), so all tones
 *         are expressed as on/off rhythms instead of PWM frequencies.
 *
 *   BeepSetStatus(s) : request a tone (can be called directly from LVGL event
 *                      callbacks, e.g. BeepSetStatus(BEEP_SHORT) on a button
 *                      press for click feedback)
 *   BeepProc(beep)   : advance the tone state machine; call periodically
 *                      (every BEEP_TASK_TIME ms)
 *===========================================================================*/
#ifndef _INF_BEEP_H
#define _INF_BEEP_H

#include <stdint.h>
#include "at32f403a_407.h"

/* Current output state */
typedef enum
{
    BEEP_CLOSE = 0, /* silent (output low) */
    BEEP_OPEN,      /* sounding (output high) */
} BEEP_WORK;

/* Tone types */
typedef enum
{
    BEEP_OFF = 0,   /* silent */
    BEEP_SHORT,     /* short beep ~80ms (confirm/notice, common LVGL click feedback) */
    BEEP_LONG,      /* long beep 1s */
    BEEP_FAST_BEEP, /* fast consecutive beeps x8 */
    BEEP_SLOW_BEEP, /* slow consecutive beeps x4 */
    BEEP_SIREN,     /* alarm: long beep x3 segments */
    BEEP_AMBULANCE, /* rapid "beep-beep" x6 segments */
    BEEP_CONFIRM,   /* double-beep confirm */
    BEEP_ERROR,     /* error warning x3 beeps */
    BEEP_ALWAYS,    /* sound continuously until BeepSetStatus(BEEP_OFF) */
    BEEP_VOLTAGE1,  /* range/voltage notice 1 */
    BEEP_VOLTAGE2,  /* range/voltage notice 2 */
    BEEP_VOLTAGE3,  /* range/voltage notice 3 */
    BEEP_VOLTAGE4,  /* range/voltage notice 4 */
    BEEP_VOLTAGE5,  /* range/voltage notice 5 */
} BEEP_STATUS;

typedef struct
{
    BEEP_WORK beep_work;  /* current output state */
    BEEP_STATUS status;   /* currently requested tone */
    uint16_t counter;     /* remaining tick count of the current on/off segment */
    uint16_t cycle_count; /* number of beeps already sounded */
    uint8_t phase;        /* 0 = sounding segment, 1 = pause segment */
} beep_t;

extern beep_t sbeep;

void BeepProc(beep_t *beep);   /* advance the state machine (call every 10ms) */
void BeepSetStatus(BEEP_STATUS status); /* request a tone */
#endif
