#include "bsp_backlight.h"

static TIM_HandleTypeDef htim12;
static uint8_t s_brightness = 80;

void Bsp_Backlight_Init(void) {
    __HAL_RCC_TIM12_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_TIM12;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    htim12.Instance = TIM12;
    htim12.Init.Prescaler = 4 - 1;  // 84MHz / 4 = 21MHz 计数频率
    htim12.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim12.Init.Period = 1000 - 1;  // 21MHz / 1000 = 21 kHz (超出人耳听觉上限，消除电感与陶瓷电容啸叫)
    htim12.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&htim12);

    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = s_brightness * 10;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim12, &sConfigOC, TIM_CHANNEL_2);

    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_2);
}

void Bsp_Backlight_Set(uint8_t percent) {
    if (percent > 100) percent = 100;
    s_brightness = percent;
    __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, (uint32_t)percent * 10);
}

uint8_t Bsp_Backlight_Get(void) {
    return s_brightness;
}
