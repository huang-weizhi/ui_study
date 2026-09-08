// PA45.c
#include "PA45.h"

/* Global instance definition (zero-initialized, then configured by init function) */
PA45_Global_t spa45 = {0};

/**
 * @brief Initialize the global power data structure
 * @note  Should be called once at system startup (before or after peripheral/ADC initialization)
 */
void Power_Global_Init(void)
{
    // Safest approach: zero everything first, then set non-zero defaults
    memset(&spa45, 0, sizeof(PA45_Global_t));
    spa45.function.speaker = 1;
     
}