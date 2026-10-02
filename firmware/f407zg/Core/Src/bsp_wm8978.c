#include "bsp_wm8978.h"
#include <math.h>
#include <string.h>

#define WM8978_I2C_ADDR         0x34
#define WM8978_REG_COUNT        58

static uint8_t  s_wm8978_ready = 0;
static uint16_t s_wm8978_regs[WM8978_REG_COUNT] = {0};
static I2S_HandleTypeDef s_hi2s2 = {0};

#define SINE_SAMPLES_PER_CHUNK  441 // 44100Hz 下 10ms 刚好 10 个完整 1000Hz 周期
static int16_t s_tone_buffer[SINE_SAMPLES_PER_CHUNK * 2]; // 左右声道交错

// -----------------------------------------------------------------------------
// 软件 I2C 控制引脚时序 (SCL: PB8, SDA: PB9)
// -----------------------------------------------------------------------------
static inline void I2C_Delay(void) {
    volatile uint32_t cnt = 150;
    while (cnt--) {
        __NOP();
    }
}

static inline void I2C_SCL(uint8_t v) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, v ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static inline void I2C_SDA_OUT(uint8_t v) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, v ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static inline uint8_t I2C_SDA_IN(void) {
    return (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9) == GPIO_PIN_SET) ? 1 : 0;
}

static void I2C_SDA_Mode(uint8_t is_out) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = is_out ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

static void I2C_Start(void) {
    I2C_SDA_Mode(1);
    I2C_SDA_OUT(1);
    I2C_SCL(1);
    I2C_Delay();
    I2C_SDA_OUT(0);
    I2C_Delay();
    I2C_SCL(0);
    I2C_Delay();
}

static void I2C_Stop(void) {
    I2C_SDA_Mode(1);
    I2C_SCL(0);
    I2C_SDA_OUT(0);
    I2C_Delay();
    I2C_SCL(1);
    I2C_Delay();
    I2C_SDA_OUT(1);
    I2C_Delay();
}

static uint8_t I2C_Wait_Ack(void) {
    uint16_t timeout = 500;
    I2C_SDA_Mode(0);
    I2C_SCL(1);
    I2C_Delay();
    while (I2C_SDA_IN()) {
        if (!--timeout) {
            I2C_Stop();
            return 1;
        }
    }
    I2C_SCL(0);
    I2C_Delay();
    return 0;
}

static void I2C_Send_Byte(uint8_t byte) {
    I2C_SDA_Mode(1);
    I2C_SCL(0);
    for (uint8_t i = 0; i < 8; i++) {
        I2C_SDA_OUT((byte & 0x80) ? 1 : 0);
        byte <<= 1;
        I2C_Delay();
        I2C_SCL(1);
        I2C_Delay();
        I2C_SCL(0);
    }
}

// 向 WM8978 寄存器写入数据 (7位地址 + 9位数据)
static uint8_t WM8978_Write_Reg(uint8_t reg, uint16_t val) {
    if (reg >= WM8978_REG_COUNT) return 1;

    I2C_Start();
    I2C_Send_Byte(WM8978_I2C_ADDR);
    if (I2C_Wait_Ack()) return 1;

    // 字节 1: reg[6:0] << 1 | val[8]
    I2C_Send_Byte((reg << 1) | ((val >> 8) & 0x01));
    if (I2C_Wait_Ack()) return 1;

    // 字节 2: val[7:0]
    I2C_Send_Byte(val & 0xFF);
    if (I2C_Wait_Ack()) return 1;

    I2C_Stop();
    s_wm8978_regs[reg] = val;
    return 0;
}

// -----------------------------------------------------------------------------
// I2S2 硬件初始化 (WS: PB12, CK: PB13, SD: PC3, MCK: PC6)
// -----------------------------------------------------------------------------
static void I2S2_Hardware_Init(void) {
    // 1. 配置 I2S 专用 PLL 时钟 (135.5MHz @ 44.1kHz)
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
    PeriphClkInitStruct.PLLI2S.PLLI2SN = 271;
    PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

    // 2. 使能 GPIO 与 SPI2/I2S2 时钟
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_SPI2_CLK_ENABLE();

    // 3. 配置引脚复用 (AF5)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;

    // PB12 (WS), PB13 (CK)
    GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // PC3 (SD), PC6 (MCK)
    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_6;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // 4. 配置 I2S2 协议参数 (Master TX, Philips Standard, 16bit, 44.1kHz, MCLK 使能)
    s_hi2s2.Instance = SPI2;
    s_hi2s2.Init.Mode = I2S_MODE_MASTER_TX;
    s_hi2s2.Init.Standard = I2S_STANDARD_PHILIPS;
    s_hi2s2.Init.DataFormat = I2S_DATAFORMAT_16B;
    s_hi2s2.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
    s_hi2s2.Init.AudioFreq = I2S_AUDIOFREQ_44K;
    s_hi2s2.Init.CPOL = I2S_CPOL_LOW;
    s_hi2s2.Init.ClockSource = I2S_CLOCK_PLL;
    s_hi2s2.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;

    HAL_I2S_Init(&s_hi2s2);
}

// -----------------------------------------------------------------------------
// 对外接口实现
// -----------------------------------------------------------------------------
uint8_t Bsp_WM8978_Init(void) {
    // 1. 初始化 I2C 控制 GPIO (PB8/PB9 开漏上拉)
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    I2C_SCL(1);
    I2C_SDA_OUT(1);
    HAL_Delay(10);

    // 2. 软件复位 WM8978
    if (WM8978_Write_Reg(0, 0) != 0) {
        s_wm8978_ready = 0;
        return 1; // 无应答，WM8978 不在线
    }
    HAL_Delay(10);

    // 3. 配置 WM8978 核心寄存器链路
    WM8978_Write_Reg(1, 0x01B);  // R1: BUFIOEN=1, BIASEN=1, VMIDSEL=50k
    WM8978_Write_Reg(2, 0x1B0);  // R2: ROUT1EN=1, LOUT1EN=1, BOOSTENR=1, BOOSTENL=1
    WM8978_Write_Reg(3, 0x06F);  // R3: LOUT2EN=1, ROUT2EN=1, RMIXEN=1, LMIXEN=1, DACENR=1, DACENL=1
    WM8978_Write_Reg(4, 0x010);  // R4: 16-bit, Philips I2S 格式
    WM8978_Write_Reg(6, 0x000);  // R6: 从机模式 (BCLK 与 LRCK 由 STM32 提供)
    WM8978_Write_Reg(10, 0x008); // R10: 软静音关闭, 128x 过采样
    WM8978_Write_Reg(14, 0x108); // R14: 高通滤波
    WM8978_Write_Reg(43, 0x010); // R43: 旁路静音
    WM8978_Write_Reg(49, 0x002); // R49: 热保护使能
    WM8978_Write_Reg(50, 0x001); // R50: 左声道 DAC 接入左混音器
    WM8978_Write_Reg(51, 0x001); // R51: 右声道 DAC 接入右混音器

    // 初始音量设置 (50 / 63 适中响度)
    Bsp_WM8978_SetHPVol(50, 50);
    Bsp_WM8978_SetSPKVol(0); // 默认喇叭关闭，专供耳机

    // 4. 初始化 I2S2 硬件
    I2S2_Hardware_Init();

    // 5. 预先计算 1000Hz 44.1kHz 正弦波音频缓冲区
    const float two_pi = 6.28318530718f;
    for (int i = 0; i < SINE_SAMPLES_PER_CHUNK; i++) {
        float angle = two_pi * 1000.0f * (float)i / 44100.0f;
        int16_t sample = (int16_t)(16000.0f * sinf(angle));
        s_tone_buffer[i * 2]     = sample; // 左声道
        s_tone_buffer[i * 2 + 1] = sample; // 右声道
    }

    s_wm8978_ready = 1;
    return 0;
}

uint8_t Bsp_WM8978_IsOk(void) {
    return s_wm8978_ready;
}

void Bsp_WM8978_SetHPVol(uint8_t vol_left, uint8_t vol_right) {
    if (vol_left > 63) vol_left = 63;
    if (vol_right > 63) vol_right = 63;

    // R52/R53: bit8 是更新位 (1=立即同步更新输出音量)
    WM8978_Write_Reg(52, vol_left | 0x100);
    WM8978_Write_Reg(53, vol_right | 0x100);
}

void Bsp_WM8978_SetSPKVol(uint8_t vol) {
    if (vol > 63) vol = 63;
    // R54/R55: 喇叭音量
    WM8978_Write_Reg(54, vol | 0x100);
    WM8978_Write_Reg(55, vol | 0x100);
}

void Bsp_WM8978_PlayTone(uint16_t freq_hz, uint32_t duration_ms) {
    if (!s_wm8978_ready) return;

    // 重新计算指定频率的正弦波 (若与预设 1000Hz 不同)
    if (freq_hz != 1000 && freq_hz > 0) {
        const float two_pi = 6.28318530718f;
        for (int i = 0; i < SINE_SAMPLES_PER_CHUNK; i++) {
            float angle = two_pi * (float)freq_hz * (float)i / 44100.0f;
            int16_t sample = (int16_t)(16000.0f * sinf(angle));
            s_tone_buffer[i * 2]     = sample;
            s_tone_buffer[i * 2 + 1] = sample;
        }
    }

    uint32_t chunks = duration_ms / 10;
    if (chunks == 0) chunks = 1;

    for (uint32_t i = 0; i < chunks; i++) {
        HAL_I2S_Transmit(&s_hi2s2, (uint16_t*)s_tone_buffer, SINE_SAMPLES_PER_CHUNK * 2, 50);
    }

    // 播放结束发送 20ms 静音防止噗噗声
    int16_t silence[64] = {0};
    for (int k = 0; k < 10; k++) {
        HAL_I2S_Transmit(&s_hi2s2, (uint16_t*)silence, 64, 20);
    }
}

void Bsp_WM8978_Stop(void) {
    if (!s_wm8978_ready) return;
    int16_t silence[64] = {0};
    HAL_I2S_Transmit(&s_hi2s2, (uint16_t*)silence, 64, 20);
}

I2S_HandleTypeDef* Bsp_WM8978_GetI2SHandle(void) {
    return &s_hi2s2;
}
