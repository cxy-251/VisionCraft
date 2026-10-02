#include "bsp_rng.h"

static RNG_HandleTypeDef s_hrng;
static uint8_t s_rng_ok = 0;

uint8_t Bsp_RNG_Init(void) {
    __HAL_RCC_RNG_CLK_ENABLE();

    s_hrng.Instance = RNG;
    if (HAL_RNG_Init(&s_hrng) != HAL_OK) {
        s_rng_ok = 0;
        return 1;
    }
    s_rng_ok = 1;
    return 0;
}

uint32_t Bsp_RNG_Get(void) {
    if (!s_rng_ok) return 0;
    uint32_t rnd = 0;
    if (HAL_RNG_GenerateRandomNumber(&s_hrng, &rnd) == HAL_OK) {
        return rnd;
    }
    return 0;
}

int32_t Bsp_RNG_GetRange(int32_t min, int32_t max) {
    if (min >= max) return min;
    uint32_t rnd = Bsp_RNG_Get();
    uint32_t range = (uint32_t)(max - min + 1);
    return min + (int32_t)(rnd % range);
}
