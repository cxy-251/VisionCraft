#include "bsp_sdio_sd.h"
#include <string.h>

static SD_HandleTypeDef hsd;
static SdCard_Info_t s_sd_info = {0};

uint8_t Bsp_Sd_Init(void) {
    __HAL_RCC_SDIO_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    // 1. PC8, PC9, PC10, PC11, PC12 -> SDIO D0..D3, CK
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDIO;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // 2. PD2 -> SDIO CMD
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDIO;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // 3. 配置 SDIO 控制器
    hsd.Instance = SDIO;
    hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
    hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
    hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
    hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
    hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
    hsd.Init.ClockDiv = SDIO_INIT_CLK_DIV; // 初始化时钟 <= 400kHz

    // 若未插卡，HAL_SD_Init 会在此超时并返回非 HAL_OK
    if (HAL_SD_Init(&hsd) != HAL_OK) {
        s_sd_info.is_initialized = 0;
        return SD_NOT_PRESENT;
    }

    // 尝试切换为 4 位高速总线
    HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B);

    HAL_SD_CardInfoTypeDef card_info;
    if (HAL_SD_GetCardInfo(&hsd, &card_info) == HAL_OK) {
        s_sd_info.is_initialized = 1;
        s_sd_info.card_type = card_info.CardType;
        s_sd_info.block_size = card_info.BlockSize;
        s_sd_info.block_count = card_info.BlockNbr;
        s_sd_info.card_capacity_kb = (uint32_t)(((uint64_t)card_info.BlockNbr * card_info.BlockSize) / 1024);
        return SD_OK;
    }

    return SD_ERROR;
}

uint8_t Bsp_Sd_GetStatus(void) {
    if (!s_sd_info.is_initialized) return SD_NOT_PRESENT;
    return (HAL_SD_GetCardState(&hsd) == HAL_SD_CARD_TRANSFER) ? SD_OK : SD_ERROR;
}

uint8_t Bsp_Sd_ReadBlocks(uint8_t *p_buf, uint32_t block_addr, uint32_t num_blocks, uint32_t timeout) {
    if (!s_sd_info.is_initialized) return SD_NOT_PRESENT;
    if (HAL_SD_ReadBlocks(&hsd, p_buf, block_addr, num_blocks, timeout) != HAL_OK) {
        return SD_ERROR;
    }
    while (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER) {}
    return SD_OK;
}

uint8_t Bsp_Sd_WriteBlocks(const uint8_t *p_buf, uint32_t block_addr, uint32_t num_blocks, uint32_t timeout) {
    if (!s_sd_info.is_initialized) return SD_NOT_PRESENT;
    if (HAL_SD_WriteBlocks(&hsd, (uint8_t *)p_buf, block_addr, num_blocks, timeout) != HAL_OK) {
        return SD_ERROR;
    }
    while (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER) {}
    return SD_OK;
}

const SdCard_Info_t* Bsp_Sd_GetInfo(void) {
    return &s_sd_info;
}
