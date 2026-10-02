#include "bsp_dac.h"

static DAC_HandleTypeDef s_hdac;
static uint8_t s_dac_ok = 0;
static uint16_t s_cur_mv = 0;

uint8_t Bsp_DAC_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_DAC_CLK_ENABLE();

    // PA4: DAC_OUT1 模拟模式
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    s_hdac.Instance = DAC;
    if (HAL_DAC_Init(&s_hdac) != HAL_OK) {
        s_dac_ok = 0;
        return 1;
    }

    DAC_ChannelConfTypeDef sConfig = {0};
    sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

    if (HAL_DAC_ConfigChannel(&s_hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK) {
        s_dac_ok = 0;
        return 1;
    }

    HAL_DAC_Start(&s_hdac, DAC_CHANNEL_1);
    s_dac_ok = 1;
    Bsp_DAC_SetVoltage(0);
    return 0;
}

void Bsp_DAC_SetVoltage(uint16_t mv) {
    if (!s_dac_ok) return;
    if (mv > 3300) mv = 3300;
    s_cur_mv = mv;
    uint32_t raw = (uint32_t)mv * 4095UL / 3300UL;
    Bsp_DAC_SetRaw((uint16_t)raw);
}

void Bsp_DAC_SetRaw(uint16_t raw) {
    if (!s_dac_ok) return;
    if (raw > 4095) raw = 4095;
    HAL_DAC_SetValue(&s_hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, raw);
}

uint16_t Bsp_DAC_GetVoltage(void) {
    return s_cur_mv;
}
