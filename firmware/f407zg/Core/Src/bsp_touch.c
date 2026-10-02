#include "bsp_touch.h"
#include <stdio.h>

Touch_Dev_t g_touch = {0};

static GPIO_TypeDef *s_sda_port = GPIOF;
static uint16_t      s_sda_pin  = GPIO_PIN_11;

static void touch_delay(uint32_t count) {
    volatile uint32_t ticks = count * 20;
    while (ticks--) {
        __NOP();
    }
}

// I2C 基层信号控制
static inline void I2C_SCL(uint8_t v) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, v ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static inline void I2C_SDA_OUT(uint8_t v) {
    HAL_GPIO_WritePin(s_sda_port, s_sda_pin, v ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static inline uint8_t I2C_SDA_IN(void) {
    return HAL_GPIO_ReadPin(s_sda_port, s_sda_pin);
}

static void I2C_SDA_Mode(uint8_t is_out) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = s_sda_pin;
    GPIO_InitStruct.Mode = is_out ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(s_sda_port, &GPIO_InitStruct);
}

static void I2C_Start(void) {
    I2C_SDA_Mode(1);
    I2C_SDA_OUT(1);
    I2C_SCL(1);
    touch_delay(5);
    I2C_SDA_OUT(0);
    touch_delay(5);
    I2C_SCL(0);
    touch_delay(5);
}

static void I2C_Stop(void) {
    I2C_SDA_Mode(1);
    I2C_SCL(0);
    I2C_SDA_OUT(0);
    touch_delay(5);
    I2C_SCL(1);
    touch_delay(5);
    I2C_SDA_OUT(1);
    touch_delay(5);
}

static uint8_t I2C_Wait_Ack(void) {
    uint8_t timeout = 0;
    I2C_SDA_Mode(0);
    I2C_SCL(1);
    touch_delay(5);
    while (I2C_SDA_IN()) {
        timeout++;
        if (timeout > 250) {
            I2C_Stop();
            return 1;
        }
    }
    I2C_SCL(0);
    touch_delay(5);
    return 0;
}

static void I2C_Ack(void) {
    I2C_SCL(0);
    I2C_SDA_Mode(1);
    I2C_SDA_OUT(0);
    touch_delay(5);
    I2C_SCL(1);
    touch_delay(5);
    I2C_SCL(0);
    touch_delay(5);
}

static void I2C_NAck(void) {
    I2C_SCL(0);
    I2C_SDA_Mode(1);
    I2C_SDA_OUT(1);
    touch_delay(5);
    I2C_SCL(1);
    touch_delay(5);
    I2C_SCL(0);
    touch_delay(5);
}

static void I2C_Send_Byte(uint8_t byte) {
    I2C_SDA_Mode(1);
    for (uint8_t i = 0; i < 8; i++) {
        I2C_SDA_OUT((byte & 0x80) >> 7);
        touch_delay(3);
        I2C_SCL(1);
        touch_delay(5);
        I2C_SCL(0);
        touch_delay(3);
        byte <<= 1;
    }
}

static uint8_t I2C_Read_Byte(uint8_t ack) {
    uint8_t byte = 0;
    I2C_SDA_Mode(0);
    for (uint8_t i = 0; i < 8; i++) {
        byte <<= 1;
        I2C_SCL(1);
        touch_delay(5);
        if (I2C_SDA_IN()) byte |= 0x01;
        I2C_SCL(0);
        touch_delay(3);
    }
    if (ack) I2C_Ack();
    else I2C_NAck();
    return byte;
}

// GT9147 读写
static uint8_t GT9147_Write_Reg(uint16_t reg, uint8_t *buf, uint8_t len) {
    I2C_Start();
    I2C_Send_Byte(0xBA);
    if (I2C_Wait_Ack()) return 1;
    I2C_Send_Byte(reg >> 8);
    if (I2C_Wait_Ack()) return 1;
    I2C_Send_Byte(reg & 0xFF);
    if (I2C_Wait_Ack()) return 1;
    for (uint8_t i = 0; i < len; i++) {
        I2C_Send_Byte(buf[i]);
        if (I2C_Wait_Ack()) return 1;
    }
    I2C_Stop();
    return 0;
}

static uint8_t GT9147_Read_Reg(uint16_t reg, uint8_t *buf, uint8_t len) {
    I2C_Start();
    I2C_Send_Byte(0xBA);
    if (I2C_Wait_Ack()) return 1;
    I2C_Send_Byte(reg >> 8);
    if (I2C_Wait_Ack()) return 1;
    I2C_Send_Byte(reg & 0xFF);
    if (I2C_Wait_Ack()) return 1;

    I2C_Start();
    I2C_Send_Byte(0xBB);
    if (I2C_Wait_Ack()) return 1;
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = I2C_Read_Byte(i < (len - 1));
    }
    I2C_Stop();
    return 0;
}

static void Touch_GPIO_Init(void) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // SCL: PB0
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);

    // INT: PB1 (开漏或浮空输入)
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // RST: PC13
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // SDA 初步配置
    I2C_SDA_Mode(1);
    I2C_SDA_OUT(1);
}

void Bsp_Touch_Init(void) {
    Touch_GPIO_Init();

    // 尝试 SDA = PF11 与 SDA = PB2 两种可能走线
    GPIO_TypeDef *candidate_ports[] = {GPIOF, GPIOB};
    uint16_t      candidate_pins[]  = {GPIO_PIN_11, GPIO_PIN_2};

    for (int k = 0; k < 2; k++) {
        s_sda_port = candidate_ports[k];
        s_sda_pin  = candidate_pins[k];
        I2C_SDA_Mode(1);

        // 1. 复位 GT9147 并配置为 0xBA 地址模式
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);  // INT = 0
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // RST = 0
        HAL_Delay(20);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);   // RST = 1
        HAL_Delay(20);

        // 释放 INT 引脚为浮空输入
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        HAL_Delay(50);

        // 2. 读取 Product ID 寄存器 0x8140 (4 bytes)
        uint8_t id[5] = {0};
        if (GT9147_Read_Reg(0x8140, id, 4) == 0) {
            if ((id[0] == '9' && id[1] == '1') || id[0] == '1' || id[0] == '9') {
                g_touch.type = TOUCH_TYPE_GT9147;
                return;
            }
        }
    }

    // 若未读取到特定 ID，仍默认启用 GT9147 协议在 PF11 进行扫描
    s_sda_port = GPIOF;
    s_sda_pin  = GPIO_PIN_11;
    g_touch.type = TOUCH_TYPE_GT9147;
}

uint8_t Bsp_Touch_Scan(void) {
    if (g_touch.type != TOUCH_TYPE_GT9147) return 0;

    uint8_t status = 0;
    if (GT9147_Read_Reg(0x814E, &status, 1) != 0) return 0;

    // 最高位为 1 表示有新的触控数据
    if (status & 0x80) {
        uint8_t points = status & 0x0F;
        if (points > 0) {
            uint8_t buf[4] = {0};
            if (GT9147_Read_Reg(0x8150, buf, 4) == 0) {
                uint16_t raw_x = buf[0] | (buf[1] << 8);
                uint16_t raw_y = buf[2] | (buf[3] << 8);

                // 限制在屏幕尺寸内
                if (raw_x < 480 && raw_y < 800) {
                    g_touch.x = raw_x;
                    g_touch.y = raw_y;
                    g_touch.pressed = 1;
                }
            }
        } else {
            g_touch.pressed = 0;
        }

        // 必须向 0x814E 清零缓冲区标记
        uint8_t zero = 0;
        GT9147_Write_Reg(0x814E, &zero, 1);
        return g_touch.pressed;
    } else {
        g_touch.pressed = 0;
    }
    return 0;
}
