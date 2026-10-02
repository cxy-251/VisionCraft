#include "bsp_remote.h"

static void Delay_us(uint32_t us) {
    uint32_t count = us * 28;
    while (count--) {
        __NOP();
    }
}

void Bsp_Remote_Init(void) {
    __HAL_RCC_GPIOG_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = REMOTE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(REMOTE_PORT, &GPIO_InitStruct);
}

// 测量高电平持续时间 (单位: 10us)
static uint16_t Remote_GetHighTime_10us(void) {
    uint16_t t = 0;
    while (HAL_GPIO_ReadPin(REMOTE_PORT, REMOTE_PIN) == GPIO_PIN_SET) {
        Delay_us(10);
        t++;
        if (t >= 1000) break; // 超时 10ms
    }
    return t;
}

// 测量低电平持续时间 (单位: 10us)
static uint16_t Remote_GetLowTime_10us(void) {
    uint16_t t = 0;
    while (HAL_GPIO_ReadPin(REMOTE_PORT, REMOTE_PIN) == GPIO_PIN_RESET) {
        Delay_us(10);
        t++;
        if (t >= 1200) break; // 超时 12ms
    }
    return t;
}

uint8_t Bsp_Remote_Scan(uint8_t *p_addr, uint8_t *p_cmd) {
    // 平时为空闲高电平，若出现低电平则进入检测
    if (HAL_GPIO_ReadPin(REMOTE_PORT, REMOTE_PIN) == GPIO_PIN_SET) {
        return 0;
    }

    // 1. 引导码低电平检验 (标准 9ms = 900 * 10us, 允许 700..1050)
    uint16_t low_time = Remote_GetLowTime_10us();
    if (low_time < 700 || low_time > 1050) {
        return 0;
    }

    // 2. 引导码高电平检验 (标准 4.5ms = 450 * 10us, 允许 350..550)
    uint16_t high_time = Remote_GetHighTime_10us();
    if (high_time < 350 || high_time > 550) {
        return 0;
    }

    // 3. 接收 32 位数据 (4 字节: Addr, ~Addr, Cmd, ~Cmd)
    uint8_t bytes[4] = {0};
    for (uint8_t i = 0; i < 32; i++) {
        // 先等 560us 的低电平
        uint16_t b_low = Remote_GetLowTime_10us();
        if (b_low < 30 || b_low > 90) {
            return 0; // 格式异常退出
        }

        // 测量高电平
        uint16_t b_high = Remote_GetHighTime_10us();
        uint8_t bit_val = 0;
        if (b_high >= 30 && b_high <= 80) {
            bit_val = 0; // 560us 高电平 -> 0
        } else if (b_high >= 120 && b_high <= 200) {
            bit_val = 1; // 1680us 高电平 -> 1
        } else {
            return 0; // 脉宽不在合法区间
        }

        uint8_t byte_idx = i / 8;
        bytes[byte_idx] >>= 1;
        if (bit_val) {
            bytes[byte_idx] |= 0x80;
        }
    }

    // 4. 校验反码 (允许部分兼容遥控器忽略地址校验，命令码必须反码校验)
    if ((uint8_t)(bytes[2] + bytes[3]) != 0xFF) {
        return 0;
    }

    if (p_addr) *p_addr = bytes[0];
    if (p_cmd)  *p_cmd  = bytes[2];
    return 1;
}

const char* Bsp_Remote_GetKeyName(uint8_t cmd) {
    switch (cmd) {
        case 0xA2: return "POWER";
        case 0x62: return "MENU";
        case 0xE2: return "TEST";
        case 0x22: return "+ (UP)";
        case 0x02: return "BACK";
        case 0xC2: return "|<< PREV";
        case 0xE0: return ">|| PLAY";
        case 0xA8: return ">>| NEXT";
        case 0x90: return "- (DOWN)";
        case 0x98: return "OK";
        case 0x68: return "0";
        case 0x30: return "1";
        case 0x18: return "2";
        case 0x7A: return "3";
        case 0x10: return "4";
        case 0x38: return "5";
        case 0x5A: return "6";
        case 0x42: return "7";
        case 0x4A: return "8";
        case 0x52: return "9";
        default:   return "UNKNOWN";
    }
}
