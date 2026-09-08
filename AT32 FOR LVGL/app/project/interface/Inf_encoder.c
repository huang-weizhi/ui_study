/**
 ******************************************************************************
 * File Name : Inf_encoder.c
 * Description: Rotary encoder driver (incremental A/B phases + middle key),
 *              EXTI-interrupt quadrature decoding
 *
 *   A phase PB7 (IN_ENCODER_A_HL_PIN, EXINT line 7)
 *   B phase PB5 (IN_ENCODER_B_HL_PIN, EXINT line 5)
 *   middle key PB6 (active low, reuses KEY_ENCODER of Inf_key)
 *
 * SIQ-02FVS3 wiring: terminals A/B = the two phases, S = push switch and
 *   C = common. Connect C to GND; A/B/S are low-active contacts that close
 *   to C. The part is rated for >= 0.5 mA per phase - if A/B look "dead"
 *   while the middle key works, add external 4.7 k pull-ups from A/B to VDD
 *   (watch the raw phases with encoder_test_read_ab() while rotating).
 *
 * Decoding is done in the EXINT9_5 ISR: both A and B edges are configured as
 *   EXINT_TRIGGER_BOTH_EDGE interrupts. The ISR samples A and B together and
 *   advances through a 16-entry quadrature table ((prev<<2)|cur), +1/-1 per
 *   legal Gray step. The accumulated edges are converted into whole "clicks"
 *   by ENC_EDGES_PER_CLICK for the upper layer / LVGL. Edges per click and
 *   the direction can be tuned with the macros below (encoders differ a lot,
 *   adjust against the real hardware).
 *
 * NOTE: this driver takes full control of EXINT lines 5/7 and enables the
 *   EXINT9_5 NVIC channel itself in encoder_init(). The WorkBench-generated
 *   wk_exint.c configures the same two lines (rising edge only, no NVIC) -
 *   encoder_init() simply re-configures them, so wk_exint.c needs no change.
 ******************************************************************************
 */

#include "Inf_encoder.h"
#include "Inf_key.h"
#include "at32f403a_407.h"
#include "at32f403a_407_wk_config.h"

/* AB transitions per mechanical detent (click): standard full cycle = 4,
 * some encoders use 2 */
#define ENC_EDGES_PER_CLICK 4
/* Set to 1 if the rotation direction is reversed on the real hardware */
#define ENCODER_DIR_INVERT 0

#if ENCODER_DIR_INVERT
#define ENC_DIR_FACTOR (-1)
#else
#define ENC_DIR_FACTOR (1)
#endif

/* EXINT wiring of the two phases (PB5 -> line 5, PB7 -> line 7, both share
 * the EXINT9_5 vector) */
#define ENC_A_EXTI_LINE EXINT_LINE_7
#define ENC_B_EXTI_LINE EXINT_LINE_5
#define ENC_EXTI_IRQ EXINT9_5_IRQn
#define ENC_EXTI_IRQ_PREEMPT_PRIO 3
#define ENC_EXTI_IRQ_SUB_PRIO 0

/* Quadrature step table indexed by (previous_ab << 2) | current_ab,
 * bit1 = A, bit0 = B. Forward = Gray sequence 00->01->11->10 (+1). */
static const int8_t step_table[16] = {
     0, /* 00 -> 00 */
    +1, /* 00 -> 01  forward */
    -1, /* 00 -> 10  reverse */
     0, /* 00 -> 11  invalid */
    -1, /* 01 -> 00  reverse */
     0, /* 01 -> 01 */
     0, /* 01 -> 10  invalid */
    +1, /* 01 -> 11  forward */
    +1, /* 10 -> 00  forward */
     0, /* 10 -> 01  invalid */
     0, /* 10 -> 10 */
    -1, /* 10 -> 11  reverse */
     0, /* 11 -> 00  invalid */
    -1, /* 11 -> 01  reverse */
    +1, /* 11 -> 10  forward */
     0  /* 11 -> 11 */
};

/* State written by the ISR, read by encoder_get_diff() */
static volatile int32_t s_edges_total; /* accumulated edges (ISR context)  */
static volatile uint8_t s_ab_prev;     /* last AB state, bit1=A, bit0=B     */
/* State used only by encoder_get_diff() (task context) */
static int32_t s_last_total;           /* edge total at last get_diff call  */
static int8_t s_rem;                   /* remainder below one click         */

/**
 * @brief  Read the current AB combination, returns bit1=A, bit0=B (0~3).
 */
static uint8_t enc_read_ab(void)
{
  uint8_t a = (gpio_input_data_bit_read(IN_ENCODER_A_HL_GPIO_PORT,
                                        IN_ENCODER_A_HL_PIN) == RESET) ? 0u : 1u;
  uint8_t b = (gpio_input_data_bit_read(IN_ENCODER_B_HL_GPIO_PORT,
                                        IN_ENCODER_B_HL_PIN) == RESET) ? 0u : 1u;
  return (uint8_t)((a << 1) | b);
}

/**
 * @brief  Advance the quadrature state machine by one edge.
 * @note   Called from the EXINT9_5 ISR. Samples both phases and adds +1/-1
 *         for a legal Gray transition, ignores bounces / illegal jumps.
 */
void encoder_exti_isr(void)
{
  uint8_t ab = enc_read_ab();
  int8_t step = step_table[((uint8_t)(s_ab_prev << 2)) | ab];

  if (step != 0)
  {
    s_ab_prev = ab;
    s_edges_total += (int32_t)step * ENC_DIR_FACTOR;
  }
}

/* -------------------- API -------------------- */

void encoder_init(void)
{
  gpio_init_type gpio_init_struct;
  exint_init_type exint_init_struct;

  /* A/B phases as pull-up inputs (the phases are low-active to C=GND) */
  crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);
  gpio_default_para_init(&gpio_init_struct);
  gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
  gpio_init_struct.gpio_pull = GPIO_PULL_UP;
  gpio_init_struct.gpio_pins = IN_ENCODER_A_HL_PIN | IN_ENCODER_B_HL_PIN;
  gpio_init(IN_ENCODER_A_HL_GPIO_PORT, &gpio_init_struct);

  /* Route GPIOB lines 5 and 7 to EXINT lines 5 and 7 */
  gpio_exint_line_config(GPIO_PORT_SOURCE_GPIOB, GPIO_PINS_SOURCE5);
  gpio_exint_line_config(GPIO_PORT_SOURCE_GPIOB, GPIO_PINS_SOURCE7);

  /* Both phases, both edges: any A/B transition wakes the ISR */
  exint_default_para_init(&exint_init_struct);
  exint_init_struct.line_select = EXINT_LINE_5;
  exint_init_struct.line_mode = EXINT_LINE_INTERRUPT;
  exint_init_struct.line_polarity = EXINT_TRIGGER_BOTH_EDGE;
  exint_init_struct.line_enable = TRUE;
  exint_init(&exint_init_struct);

  exint_init_struct.line_select = EXINT_LINE_7;
  exint_init(&exint_init_struct);

  /* Clear any pending flag left from the startup config, then take the
   * power-on level as the starting state BEFORE enabling the interrupt so
   * the very first edge is decoded from a valid previous state. */
  exint_flag_clear(EXINT_LINE_5 | EXINT_LINE_7);
  s_edges_total = 0;
  s_last_total = 0;
  s_rem = 0;
  s_ab_prev = enc_read_ab();
  nvic_irq_enable(ENC_EXTI_IRQ, ENC_EXTI_IRQ_PREEMPT_PRIO,
                  ENC_EXTI_IRQ_SUB_PRIO);
}

int16_t encoder_get_diff(void)
{
  int32_t delta, acc, clicks;

  /* Snapshot the ISR-maintained counter atomically */
  __disable_irq();
  delta = s_edges_total - s_last_total;
  s_last_total = s_edges_total;
  __enable_irq();

  acc = delta + s_rem;                /* convert together with carried-over remainder */
  clicks = acc / ENC_EDGES_PER_CLICK; /* C integer division truncates toward 0 */
  s_rem = (int8_t)(acc % ENC_EDGES_PER_CLICK);

  if (clicks > 32767)
    clicks = 32767;
  else if (clicks < -32768)
    clicks = -32768;
  return (int16_t)clicks;
}

/* Middle key reuses KEY_ENCODER of Inf_key (key_scan() must be called
 * periodically as well) */
uint8_t encoder_key_is_down(void)
{
  return key_is_down(KEY_ENCODER);
}

uint8_t encoder_key_pressed(void)
{
  return key_pressed(KEY_ENCODER);
}

uint8_t encoder_key_released(void)
{
  return key_released(KEY_ENCODER);
}

/**
 * @brief  Hardware test helper: read the raw A/B phase levels.
 * @note   Slow-rotate the knob and poll this value; a working encoder cycles
 *         00->01->11->10 (Gray). If it never changes, the phases are NOT
 *         reaching the MCU: check the wiring (A/B to PB7/PB5, common C to
 *         GND) and that A/B lines are not stuck high/low.
 * @retval bit1 = A level, bit0 = B level (value 0..3)
 */
uint8_t encoder_test_read_ab(void)
{
  return enc_read_ab();
}

/**
 * @brief  Hardware test helper: net detents + direction since the last call.
 * @note   Decoding runs in the EXINT ISR, so the caller only reads the click
 *         accumulator (no internal scan needed):
 *           int16_t enc = encoder_test_read();  // one detent per click
 *           -> +1/-1 per click (CW/CCW), 0 = idle.
 *         If the reported direction is reversed on the real hardware, set
 *         ENCODER_DIR_INVERT to 1 in this file. If the value stays 0 while
 *         rotating, watch the raw phases with encoder_test_read_ab() first
 *         (see its note for wiring checks).
 * @retval > 0 : CW, < 0 : CCW, 0 : not rotated since the last call
 */
int16_t encoder_test_read(void)
{
  return encoder_get_diff();
}

/* EXINT9_5 ISR (lines 5..9 share this vector; lines 5 and 7 are used here).
 * Implemented as a strong symbol that overrides the weak startup default. */
void EXINT9_5_IRQHandler(void)
{
  if (exint_interrupt_flag_get(EXINT_LINE_5) != RESET)
  {
    exint_flag_clear(EXINT_LINE_5);
    encoder_exti_isr();
  }
  if (exint_interrupt_flag_get(EXINT_LINE_7) != RESET)
  {
    exint_flag_clear(EXINT_LINE_7);
    encoder_exti_isr();
  }
}
