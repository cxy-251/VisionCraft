#include "random.h"
#include "rng.h"
volatile uint32_t g_rng_good, g_rng_bad, g_rng_last;
int random_word(uint32_t *value)
{
    if (!value) return 0;
    *value = 0;
    uint32_t sample;
    if (HAL_RNG_GenerateRandomNumber(&hrng, &sample) != HAL_OK) { g_rng_bad++; return 0; }
    *value = g_rng_last = sample;
    g_rng_good++;
    return 1;
}
