#include "bsp_beep.h"
#include "bsp_wm8978.h"
#include "FreeRTOS.h"
#include "task.h"

static void Beep_Delay_us(uint32_t us) {
    uint32_t count = us * 28; // 168MHz 下约 28 次循环对应 1us
    while (count--) {
        __NOP();
    }
}

void Bsp_Beep_Init(void) {
    __HAL_RCC_GPIOF_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = BEEP_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BEEP_PORT, &GPIO_InitStruct);

    // 默认关闭 (低电平三极管截止)
    HAL_GPIO_WritePin(BEEP_PORT, BEEP_PIN, GPIO_PIN_RESET);
}

void Bsp_Beep_Tone(uint16_t freq_hz, uint16_t duration_ms) {
    if (freq_hz == 0 || duration_ms == 0) return;

    uint32_t half_period_us = 1000000UL / (freq_hz * 2);
    if (half_period_us == 0) half_period_us = 1;

    uint32_t cycles = ((uint32_t)duration_ms * 1000UL) / (half_period_us * 2);

    for (uint32_t i = 0; i < cycles; i++) {
        HAL_GPIO_WritePin(BEEP_PORT, BEEP_PIN, GPIO_PIN_SET);
        Beep_Delay_us(half_period_us);
        HAL_GPIO_WritePin(BEEP_PORT, BEEP_PIN, GPIO_PIN_RESET);
        Beep_Delay_us(half_period_us);
    }

    HAL_GPIO_WritePin(BEEP_PORT, BEEP_PIN, GPIO_PIN_RESET);
}

void Bsp_Beep_Click(void) {
    // 1. 板载蜂鸣器发声 15ms
    Bsp_Beep_Tone(2500, 15);

    // 2. 若 WM8978 已就绪，同步向耳机输出 10ms 1kHz 测试提示音
    if (Bsp_WM8978_IsOk()) {
        Bsp_WM8978_PlayTone(1000, 10);
    }
}
