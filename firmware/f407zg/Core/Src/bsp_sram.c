#include "bsp_sram.h"
#include <string.h>

static SRAM_HandleTypeDef s_hsram;
static uint8_t s_is_ready = 0;

void Bsp_Sram_Init(void) {
    if (s_is_ready) return;

    // 1. 使能 GPIO 与 FSMC 时钟
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_FSMC_CLK_ENABLE();

    // 2. 引脚配置: 全部复用为 FSMC (AF12)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;

    // GPIOD: D0, D1, D2, D3, D13, D14, D15, NOE(PD4), NWE(PD5), A16(PD11), A17(PD12), A18(PD13), D13(PD8), D14(PD9), D15(PD10)
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 |
                          GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 |
                          GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // GPIOE: NBL0(PE0), NBL1(PE1), D4~D12 (PE7~PE15)
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_8 |
                          GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
                          GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    // GPIOF: A0~A5 (PF0~PF5), A6~A9 (PF12~PF15)
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                          GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_12 | GPIO_PIN_13 |
                          GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    // GPIOG: A10~A15 (PG0~PG5), NE3 (PG10)
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                          GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_10;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    // 3. 配置 FSMC 接口参数 (Bank 1 Subbank 3 / NE3)
    s_hsram.Instance = FSMC_NORSRAM_DEVICE;
    s_hsram.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;

    s_hsram.Init.NSBank = FSMC_NORSRAM_BANK3;
    s_hsram.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
    s_hsram.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
    s_hsram.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
    s_hsram.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
    s_hsram.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
    s_hsram.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
    s_hsram.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
    s_hsram.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
    s_hsram.Init.ExtendedMode = FSMC_EXTENDED_MODE_DISABLE;
    s_hsram.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
    s_hsram.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;
    s_hsram.Init.PageSize = FSMC_PAGE_SIZE_NONE;

    // 4. 时序配置 (IS62WV51216BLL 55ns 异步静态存储器)
    FSMC_NORSRAM_TimingTypeDef Timing = {0};
    Timing.AddressSetupTime = 1;       // 2 HCLK (约 11.9ns)
    Timing.AddressHoldTime = 0;
    Timing.DataSetupTime = 8;          // 9 HCLK (约 53.6ns)
    Timing.BusTurnAroundDuration = 0;
    Timing.CLKDivision = 0;
    Timing.DataLatency = 0;
    Timing.AccessMode = FSMC_ACCESS_MODE_A;

    HAL_SRAM_Init(&s_hsram, &Timing, &Timing);
    s_is_ready = 1;
}

uint8_t Bsp_Sram_IsReady(void) {
    return s_is_ready;
}

uint8_t Bsp_Sram_SelfTest(uint32_t test_len, uint32_t *p_err_count) {
    if (!s_is_ready) {
        Bsp_Sram_Init();
    }

    if (test_len == 0 || test_len > SRAM_SIZE_BYTES) {
        test_len = SRAM_SIZE_BYTES;
    }

    volatile uint16_t *p_sram = (volatile uint16_t *)SRAM_BANK_ADDR;
    uint32_t words = test_len / 2;
    uint32_t errors = 0;

    // 1. 写入测试特征码: (index ^ 0x5A5A)
    for (uint32_t i = 0; i < words; i++) {
        p_sram[i] = (uint16_t)(i ^ 0x5A5A);
    }

    // 2. 读回校验
    for (uint32_t i = 0; i < words; i++) {
        uint16_t expected = (uint16_t)(i ^ 0x5A5A);
        uint16_t read_val = p_sram[i];
        if (read_val != expected) {
            errors++;
            if (errors > 100) break; // 错误过多提前中断
        }
    }

    if (p_err_count) *p_err_count = errors;
    return (errors == 0) ? 1 : 0;
}
