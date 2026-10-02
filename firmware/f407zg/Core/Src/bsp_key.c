#include "bsp_key.h"

void Bsp_Key_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // KEY0(PE4), KEY1(PE3), KEY2(PE2) 配置为上拉输入
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    // WK_UP(PA0) 配置为下拉输入
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

// 按键扫描，mode: 0-不支持连续按，1-支持连续按
uint8_t Bsp_Key_Scan(uint8_t mode) {
    static uint8_t key_up = 1;
    if (mode) key_up = 1;

    uint8_t k0 = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4);
    uint8_t k1 = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3);
    uint8_t k2 = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2);
    uint8_t wk = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

    if (key_up && (k0 == 0 || k1 == 0 || k2 == 0 || wk == 1)) {
        key_up = 0;
        if (k0 == 0) return KEY0_PRES;
        if (k1 == 0) return KEY1_PRES;
        if (k2 == 0) return KEY2_PRES;
        if (wk == 1) return WKUP_PRES;
    } else if (k0 == 1 && k1 == 1 && k2 == 1 && wk == 0) {
        key_up = 1;
    }
    return 0;
}
