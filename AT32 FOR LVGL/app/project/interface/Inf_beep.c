/*=============================================================================
 * Inf_beep.c - Active buzzer driver (GPIO, high level = on)
 *
 *   Pin: OUT_BEEP_H_* (PC3), defined in at32f403a_407_wk_config.h
 *   Principle: an active buzzer can only sound/silence, so the tones are
 *         expressed as an on/off rhythm state machine. BeepSetStatus()
 *         requests a tone, BeepProc() advances one 10ms step, and a single
 *         tone returns to BEEP_OFF automatically when finished.
 *   LVGL: LVGL does not drive the buzzer directly; call
 *         BeepSetStatus(BEEP_SHORT) inside LVGL event callbacks to get a
 *         button/click feedback tone.
 *===========================================================================*/
#include "Inf_beep.h"
#include "at32f403a_407_wk_config.h"

/* One tick = BeepProc() call period (10ms) */
#define BEEP_TICK_MS 10

/* Tone template: on = sounding length (ticks), off = pause length (ticks),
 * reps = number of beeps.
 * Note: when reps > 1, off should be > 0, otherwise adjacent beeps merge
 * into a single continuous tone */
typedef struct
{
    uint16_t on;   /* sounding tick count */
    uint16_t off;  /* pause tick count between beeps */
    uint16_t reps; /* number of beeps */
} beep_pattern_t;

static const beep_pattern_t s_pattern[] = {
    /* [BEEP_OFF]        */ {0,   0,  0},
    /* [BEEP_SHORT] 80ms */ {8,   0,  1},
    /* [BEEP_LONG]  1s   */ {100, 0,  1},
    /* [BEEP_FAST_BEEP]  */ {3,   1,  8},
    /* [BEEP_SLOW_BEEP]  */ {15,  25, 4},
    /* [BEEP_SIREN]      */ {120, 20, 3},
    /* [BEEP_AMBULANCE]  */ {10,  10, 6},
    /* [BEEP_CONFIRM]    */ {6,   6,  2},
    /* [BEEP_ERROR]      */ {30,  30, 3},
    /* [BEEP_VOLTAGE1]   */ {10,  0,  1},
    /* [BEEP_VOLTAGE2]   */ {10,  10, 2},
    /* [BEEP_VOLTAGE3]   */ {10,  10, 3},
    /* [BEEP_VOLTAGE4]   */ {15,  10, 2},
    /* [BEEP_VOLTAGE5]   */ {15,  10, 3},
};

beep_t sbeep = {
    .beep_work = BEEP_CLOSE,
    .status = BEEP_OFF,
    .counter = 0,
    .cycle_count = 0,
    .phase = 0,
};

/* Active buzzer: high level sounds */
static void beep_out(uint8_t on)
{
    if (on)
        gpio_bits_set(OUT_BEEP_H_GPIO_PORT, OUT_BEEP_H_PIN);
    else
        gpio_bits_reset(OUT_BEEP_H_GPIO_PORT, OUT_BEEP_H_PIN);
}

void BeepProc(beep_t *beep)
{
    const beep_pattern_t *pat;

    /* BEEP_OFF: silence and reset */
    if (beep->status == BEEP_OFF)
    {
        beep_out(0);
        beep->beep_work = BEEP_CLOSE;
        beep->counter = 0;
        beep->phase = 0;
        beep->cycle_count = 0;
        return;
    }

    /* BEEP_ALWAYS: sound continuously until the upper layer switches to
     * BEEP_OFF */
    if (beep->status == BEEP_ALWAYS)
    {
        beep_out(1);
        beep->beep_work = BEEP_OPEN;
        return;
    }

    pat = &s_pattern[beep->status];

    if (beep->phase == 0)
    {
        /* ---- sounding segment ---- */
        if (beep->counter == 0)
        {
            if (beep->cycle_count >= pat->reps) /* defensive, should not happen */
            {
                beep->status = BEEP_OFF;
                return;
            }
            beep_out(1);
            beep->beep_work = BEEP_OPEN;
            beep->counter = pat->on;
        }
        beep->counter--;
        if (beep->counter == 0)
        {
            beep->cycle_count++;
            if (beep->cycle_count >= pat->reps || pat->off == 0)
            {
                /* tone finished */
                beep_out(0);
                beep->beep_work = BEEP_CLOSE;
                beep->status = BEEP_OFF;
                beep->counter = 0;
                beep->phase = 0;
                beep->cycle_count = 0;
            }
            else
            {
                /* enter pause segment, prepare the next beep */
                beep_out(0);
                beep->beep_work = BEEP_CLOSE;
                beep->phase = 1;
                beep->counter = pat->off;
            }
        }
    }
    else
    {
        /* ---- pause segment ---- */
        beep->counter--;
        if (beep->counter == 0)
        {
            beep->phase = 0;
            beep->counter = 0;
        }
    }
}

void BeepSetStatus(BEEP_STATUS status)
{
    sbeep.status = status;
    sbeep.counter = 0;
    sbeep.cycle_count = 0;
    sbeep.phase = 0;
    /* silence immediately to avoid residue from a previous tone */
    beep_out(0);
    sbeep.beep_work = BEEP_CLOSE;
}
