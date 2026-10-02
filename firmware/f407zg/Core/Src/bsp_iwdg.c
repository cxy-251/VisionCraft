#include "bsp_iwdg.h"

static IWDG_HandleTypeDef s_hiwdg;
static uint8_t s_iwdg_enabled = 0;

uint8_t Bsp_IWDG_Init(uint16_t timeout_ms) {
    if (timeout_ms < 100) timeout_ms = 100;
    if (timeout_ms > 8000) timeout_ms = 8000;

    // LSI 约 32kHz，Prescaler = 64，频率 = 500Hz，每计数值为 2ms
    uint32_t reload = timeout_ms / 2;
    if (reload > 4095) reload = 4095;

    s_hiwdg.Instance = IWDG;
    s_hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
    s_hiwdg.Init.Reload = reload;

    if (HAL_IWDG_Init(&s_hiwdg) != HAL_OK) {
        s_iwdg_enabled = 0;
        return 1;
    }

    s_iwdg_enabled = 1;
    return 0;
}

void Bsp_IWDG_Feed(void) {
    if (s_iwdg_enabled) {
        HAL_IWDG_Refresh(&s_hiwdg);
    }
}

uint8_t Bsp_IWDG_IsEnabled(void) {
    return s_iwdg_enabled;
}
