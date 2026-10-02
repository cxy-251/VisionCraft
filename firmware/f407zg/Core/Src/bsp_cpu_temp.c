#include "bsp_cpu_temp.h"

static ADC_HandleTypeDef s_hadc1;
static uint8_t s_is_inited = 0;

void Bsp_CpuTemp_Init(void) {
    if (s_is_inited) return;

    __HAL_RCC_ADC1_CLK_ENABLE();

    s_hadc1.Instance = ADC1;
    s_hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    s_hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    s_hadc1.Init.ScanConvMode = DISABLE;
    s_hadc1.Init.ContinuousConvMode = DISABLE;
    s_hadc1.Init.DiscontinuousConvMode = DISABLE;
    s_hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    s_hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    s_hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    s_hadc1.Init.NbrOfConversion = 1;
    s_hadc1.Init.DMAContinuousRequests = DISABLE;
    s_hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&s_hadc1);

    // 唤醒内部温度传感器与 Vrefint
    ADC->CCR |= ADC_CCR_TSVREFE;

    s_is_inited = 1;
}

float Bsp_CpuTemp_ReadCelsius(void) {
    if (!s_is_inited) {
        Bsp_CpuTemp_Init();
    }

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    HAL_ADC_ConfigChannel(&s_hadc1, &sConfig);

    HAL_ADC_Start(&s_hadc1);
    if (HAL_ADC_PollForConversion(&s_hadc1, 20) == HAL_OK) {
        uint32_t raw = HAL_ADC_GetValue(&s_hadc1);
        // Vsense = raw * 3.3V / 4095
        // Temp = (Vsense - 0.76V) / 0.0025V/C + 25.0C
        float vsense = (float)raw * 3.3f / 4095.0f;
        float temp = (vsense - 0.76f) / 0.0025f + 25.0f;
        return temp;
    }
    return 0.0f;
}
