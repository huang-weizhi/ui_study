/*=============================================================================
 * output_handle.c - Output control (fan / output enable / protection)
 *
 * fan_output()        : drives the cooling fan according to temperature/load
 * check_amt()         : validates measured current against the selected AMT range
 * set_amt_range()     : applies the requested current range setting
 * check_output_state(): main output enable/disable state machine incl. fault
 *                       handling. All results are published into spa45.
 *===========================================================================*/ \
#include "output_handle.h"
#include "at32f403a_407_wk_config.h"
#include <stdint.h>
