#include "bsp_lsens.h"

static ADC_HandleTypeDef s_hadc3;
static uint8_t s_is_inited = 0;

void Bsp_Lsens_Init(void) {
    if (s_is_inited) return;

    // 1. 使能 GPIOF 与 ADC3 时钟
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_ADC3_CLK_ENABLE();

    // 2. 配置 PF7 为模拟输入 (ADC3_IN5)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    // 3. 配置 ADC3
    s_hadc3.Instance = ADC3;
    s_hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    s_hadc3.Init.Resolution = ADC_RESOLUTION_12B;
    s_hadc3.Init.ScanConvMode = DISABLE;
    s_hadc3.Init.ContinuousConvMode = DISABLE;
    s_hadc3.Init.DiscontinuousConvMode = DISABLE;
    s_hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    s_hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    s_hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    s_hadc3.Init.NbrOfConversion = 1;
    s_hadc3.Init.DMAContinuousRequests = DISABLE;
    s_hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&s_hadc3);

    s_is_inited = 1;
}

uint16_t Bsp_Lsens_ReadRaw(void) {
    if (!s_is_inited) {
        Bsp_Lsens_Init();
    }

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_5;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    HAL_ADC_ConfigChannel(&s_hadc3, &sConfig);

    HAL_ADC_Start(&s_hadc3);
    if (HAL_ADC_PollForConversion(&s_hadc3, 20) == HAL_OK) {
        return (uint16_t)HAL_ADC_GetValue(&s_hadc3);
    }
    return 0;
}

uint8_t Bsp_Lsens_ReadPercent(void) {
    uint16_t raw = Bsp_Lsens_ReadRaw();
    // 正点原子探索者光敏电阻: 光线越亮阻值越小，PF7 分压越低
    // 换算为 0~100% (亮=100%, 暗=0%)
    if (raw > 4000) raw = 4000;
    uint32_t inv = 4000 - raw;
    return (uint8_t)(inv * 100 / 4000);
}
