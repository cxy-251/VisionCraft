#include "bsp_at24c02.h"
#include <string.h>
#include <stdio.h>

static Eeprom_Info_t s_eeprom_info = {0};

static inline void I2C_Delay(void) {
    volatile uint32_t cnt = 250;
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
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9);
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
    I2C_SDA_Mode(0); // 切为输入模式
    I2C_SCL(1);
    I2C_Delay();
    while (I2C_SDA_IN()) {
        if (--timeout == 0) {
            I2C_SCL(0);
            return 1; // 无应答 (NACK)
        }
    }
    I2C_SCL(0);
    I2C_Delay();
    return 0; // 应答成功 (ACK)
}

static void I2C_Ack(void) {
    I2C_SDA_Mode(1);
    I2C_SDA_OUT(0);
    I2C_Delay();
    I2C_SCL(1);
    I2C_Delay();
    I2C_SCL(0);
    I2C_Delay();
}

static void I2C_Nack(void) {
    I2C_SDA_Mode(1);
    I2C_SDA_OUT(1);
    I2C_Delay();
    I2C_SCL(1);
    I2C_Delay();
    I2C_SCL(0);
    I2C_Delay();
}

static void I2C_Send_Byte(uint8_t byte) {
    I2C_SDA_Mode(1);
    for (uint8_t i = 0; i < 8; i++) {
        I2C_SDA_OUT((byte & 0x80) ? 1 : 0);
        byte <<= 1;
        I2C_Delay();
        I2C_SCL(1);
        I2C_Delay();
        I2C_SCL(0);
        I2C_Delay();
    }
}

static uint8_t I2C_Read_Byte(uint8_t ack) {
    uint8_t byte = 0;
    I2C_SDA_Mode(0); // 切为输入模式
    for (uint8_t i = 0; i < 8; i++) {
        I2C_SCL(1);
        I2C_Delay();
        byte = (byte << 1) | (I2C_SDA_IN() ? 1 : 0);
        I2C_SCL(0);
        I2C_Delay();
    }
    if (ack) {
        I2C_Ack();
    } else {
        I2C_Nack();
    }
    return byte;
}

void Bsp_At24c02_Init(void) {
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    I2C_SCL(1);
    I2C_SDA_OUT(1);

    s_eeprom_info.is_detected = Bsp_At24c02_Check();
    s_eeprom_info.capacity_bytes = AT24C02_TOTAL_SIZE;
    s_eeprom_info.page_size = AT24C02_PAGE_SIZE;
}

uint8_t Bsp_At24c02_Check(void) {
    I2C_Start();
    I2C_Send_Byte(AT24C02_ADDR); // 发送写地址
    uint8_t ack = I2C_Wait_Ack();
    I2C_Stop();
    return (ack == 0) ? 1 : 0;
}

uint8_t Bsp_At24c02_ReadByte(uint8_t addr) {
    uint8_t data = 0xFF;
    I2C_Start();
    I2C_Send_Byte(AT24C02_ADDR);
    if (I2C_Wait_Ack() != 0) {
        I2C_Stop();
        return 0xFF;
    }
    I2C_Send_Byte(addr);
    if (I2C_Wait_Ack() != 0) {
        I2C_Stop();
        return 0xFF;
    }
    I2C_Start(); // 重复起始条件
    I2C_Send_Byte(AT24C02_ADDR | 0x01); // 读操作
    if (I2C_Wait_Ack() != 0) {
        I2C_Stop();
        return 0xFF;
    }
    data = I2C_Read_Byte(0); // 读单字节，回复 NACK
    I2C_Stop();
    return data;
}

void Bsp_At24c02_WriteByte(uint8_t addr, uint8_t data) {
    I2C_Start();
    I2C_Send_Byte(AT24C02_ADDR);
    if (I2C_Wait_Ack() != 0) {
        I2C_Stop();
        return;
    }
    I2C_Send_Byte(addr);
    if (I2C_Wait_Ack() != 0) {
        I2C_Stop();
        return;
    }
    I2C_Send_Byte(data);
    I2C_Wait_Ack();
    I2C_Stop();
    HAL_Delay(6); // 保证内部写周期 (tWR <= 5ms) 完成
}

void Bsp_At24c02_Read(uint8_t addr, uint8_t *p_buf, uint16_t len) {
    if (len == 0) return;
    I2C_Start();
    I2C_Send_Byte(AT24C02_ADDR);
    if (I2C_Wait_Ack() != 0) {
        I2C_Stop();
        return;
    }
    I2C_Send_Byte(addr);
    if (I2C_Wait_Ack() != 0) {
        I2C_Stop();
        return;
    }
    I2C_Start();
    I2C_Send_Byte(AT24C02_ADDR | 0x01);
    if (I2C_Wait_Ack() != 0) {
        I2C_Stop();
        return;
    }
    for (uint16_t i = 0; i < len; i++) {
        p_buf[i] = I2C_Read_Byte(i == (len - 1) ? 0 : 1);
    }
    I2C_Stop();
}

void Bsp_At24c02_Write(uint8_t addr, const uint8_t *p_buf, uint16_t len) {
    while (len > 0) {
        // 计算当前页剩余字节数 (8 字节对齐)
        uint8_t page_offset = addr % AT24C02_PAGE_SIZE;
        uint8_t chunk_len = AT24C02_PAGE_SIZE - page_offset;
        if (chunk_len > len) {
            chunk_len = (uint8_t)len;
        }

        I2C_Start();
        I2C_Send_Byte(AT24C02_ADDR);
        if (I2C_Wait_Ack() != 0) {
            I2C_Stop();
            return;
        }
        I2C_Send_Byte(addr);
        if (I2C_Wait_Ack() != 0) {
            I2C_Stop();
            return;
        }
        for (uint8_t i = 0; i < chunk_len; i++) {
            I2C_Send_Byte(p_buf[i]);
            I2C_Wait_Ack();
        }
        I2C_Stop();
        HAL_Delay(6); // 等待页写入固化

        addr += chunk_len;
        p_buf += chunk_len;
        len -= chunk_len;
    }
}

uint8_t Bsp_At24c02_SelfTest(uint8_t test_addr, char *p_msg, uint16_t msg_len) {
    static const char test_magic[] = "VC407EE#OK";
    const uint8_t pattern_len = (uint8_t)strlen(test_magic);
    uint8_t read_back[16] = {0};

    // 1. 写入测试魔数
    Bsp_At24c02_Write(test_addr, (const uint8_t *)test_magic, pattern_len);

    // 2. 读回并校验
    Bsp_At24c02_Read(test_addr, read_back, pattern_len);
    if (memcmp(read_back, test_magic, pattern_len) != 0) {
        if (p_msg) {
            snprintf(p_msg, msg_len, "Read Mismatch at 0x%02X", test_addr);
        }
        return 0;
    }

    if (p_msg) {
        snprintf(p_msg, msg_len, "Offset 0x%02X Verified: \"%s\"", test_addr, test_magic);
    }
    return 1;
}

const Eeprom_Info_t* Bsp_At24c02_GetInfo(void) {
    return &s_eeprom_info;
}
