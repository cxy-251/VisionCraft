#include "bsp_lan8720.h"

static ETH_HandleTypeDef s_heth;
static uint8_t s_phy_detected = 0;

uint8_t Bsp_LAN8720_HardwareReset(void) {
    __HAL_RCC_GPIOD_CLK_ENABLE();

    // PD3: ETH_RESET 推挽输出
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // 拉低复位引脚
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET);
    HAL_Delay(20);
    // 拉高使芯片脱离复位状态
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_Delay(50);

    return 0;
}

uint8_t Bsp_LAN8720_Probe(uint16_t *p_id1, uint16_t *p_id2) {
    Bsp_LAN8720_HardwareReset();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_ETH_CLK_ENABLE();

    // PC1: ETH_MDC, PA2: ETH_MDIO (AF11)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    s_heth.Instance = ETH;
    s_heth.Init.MediaInterface = ETH_MEDIA_INTERFACE_RMII;

    uint32_t val1 = 0, val2 = 0;
    if (HAL_ETH_ReadPHYRegister(&s_heth, LAN8720_PHY_ADDR, 2, &val1) == HAL_OK &&
        HAL_ETH_ReadPHYRegister(&s_heth, LAN8720_PHY_ADDR, 3, &val2) == HAL_OK) {
        if (p_id1) *p_id1 = (uint16_t)val1;
        if (p_id2) *p_id2 = (uint16_t)val2;

        if ((uint16_t)val1 == LAN8720_PHY_ID1 && ((uint16_t)val2 & LAN8720_PHY_ID2_MASK) == LAN8720_PHY_ID2) {
            s_phy_detected = 1;
            return 0; // 成功探测到 LAN8720
        }
    }

    s_phy_detected = 0;
    return 1;
}

uint8_t Bsp_LAN8720_GetLinkStatus(void) {
    if (!s_phy_detected) return 0;
    uint32_t bsr = 0;
    // 读两次 Basic Status Register (Register 1)，清除 Latch 状态
    HAL_ETH_ReadPHYRegister(&s_heth, LAN8720_PHY_ADDR, 1, &bsr);
    HAL_ETH_ReadPHYRegister(&s_heth, LAN8720_PHY_ADDR, 1, &bsr);
    return (bsr & (1U << 2)) ? 1 : 0; // Bit 2: Link Status
}
