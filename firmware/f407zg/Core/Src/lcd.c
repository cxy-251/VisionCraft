#include "lcd.h"
#include "font.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdio.h>

_lcd_dev lcddev;
uint16_t POINT_COLOR = 0x0000;
uint16_t BACK_COLOR  = 0xFFFF;

static SRAM_HandleTypeDef hsram;
static SemaphoreHandle_t s_lcd_mutex = NULL;

void LCD_Lock(void) {
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        return;
    }
    if (s_lcd_mutex == NULL) {
        s_lcd_mutex = xSemaphoreCreateRecursiveMutex();
    }
    if (s_lcd_mutex != NULL) {
        xSemaphoreTakeRecursive(s_lcd_mutex, portMAX_DELAY);
    }
}

void LCD_Unlock(void) {
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        return;
    }
    if (s_lcd_mutex != NULL) {
        xSemaphoreGiveRecursive(s_lcd_mutex);
    }
}

static inline void LCD_WR_REG(uint16_t regval) {
    LCD->LCD_REG = regval;
}

static inline void LCD_WR_DATA(uint16_t data) {
    LCD->LCD_RAM = data;
}

static inline uint16_t LCD_RD_DATA(void) {
    return LCD->LCD_RAM;
}

static void LCD_WriteReg(uint16_t LCD_Reg, uint16_t LCD_RegValue) {
    LCD->LCD_REG = LCD_Reg;
    LCD->LCD_RAM = LCD_RegValue;
}

static uint16_t LCD_ReadReg(uint16_t LCD_Reg) {
    LCD_WR_REG(LCD_Reg);
    for (volatile int i = 0; i < 50; i++);
    return LCD_RD_DATA();
}


static void LCD_FSMC_Init(void) {
    // 1. 使能各总线时钟
    __HAL_RCC_FSMC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    // 2. 初始化背光引脚 (PB15)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    LCD_LED_SET();

    // 3. 配置 FSMC 引脚复用 (AF12)
    // GPIOD: PD0,1,4,5,8,9,10,14,15
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 |
                          GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // GPIOE: PE7..PE15
    GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
                          GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    // GPIOF: PF12 (FSMC_A6, LCD_RS)
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    // GPIOG: PG12 (FSMC_NE4, LCD_CS)
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    // 4. 配置 FSMC 存储控制器参数
    FSMC_NORSRAM_TimingTypeDef Timing = {0};
    Timing.AddressSetupTime = 15;
    Timing.AddressHoldTime = 0;
    Timing.DataSetupTime = 60;
    Timing.BusTurnAroundDuration = 0;
    Timing.CLKDivision = 0;
    Timing.DataLatency = 0;
    Timing.AccessMode = FSMC_ACCESS_MODE_A;

    hsram.Instance = FSMC_NORSRAM_DEVICE;
    hsram.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
    hsram.Init.NSBank = FSMC_NORSRAM_BANK4;
    hsram.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
    hsram.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
    hsram.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
    hsram.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
    hsram.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
    hsram.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
    hsram.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
    hsram.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
    hsram.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
    hsram.Init.ExtendedMode = FSMC_EXTENDED_MODE_DISABLE;
    hsram.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
    hsram.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;
    hsram.Init.PageSize = FSMC_PAGE_SIZE_NONE;

    HAL_SRAM_Init(&hsram, &Timing, &Timing);
}

void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos) {
    if (lcddev.id == 0x5510) {
        LCD_WR_REG(0x2A00);
        LCD_WR_DATA(Xpos >> 8);
        LCD_WR_REG(0x2A01);
        LCD_WR_DATA(Xpos & 0xFF);
        LCD_WR_REG(0x2B00);
        LCD_WR_DATA(Ypos >> 8);
        LCD_WR_REG(0x2B01);
        LCD_WR_DATA(Ypos & 0xFF);
    } else {
        LCD_WR_REG(lcddev.setxcmd);
        LCD_WR_DATA(Xpos >> 8);
        LCD_WR_DATA(Xpos & 0xFF);
        LCD_WR_REG(lcddev.setycmd);
        LCD_WR_DATA(Ypos >> 8);
        LCD_WR_DATA(Ypos & 0xFF);
    }
}

void LCD_Set_Window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height) {
    uint16_t ex = sx + width - 1;
    uint16_t ey = sy + height - 1;

    if (lcddev.id == 0x5510) {
        LCD_WR_REG(0x2A00);
        LCD_WR_DATA(sx >> 8);
        LCD_WR_REG(0x2A01);
        LCD_WR_DATA(sx & 0xFF);
        LCD_WR_REG(0x2A02);
        LCD_WR_DATA(ex >> 8);
        LCD_WR_REG(0x2A03);
        LCD_WR_DATA(ex & 0xFF);

        LCD_WR_REG(0x2B00);
        LCD_WR_DATA(sy >> 8);
        LCD_WR_REG(0x2B01);
        LCD_WR_DATA(sy & 0xFF);
        LCD_WR_REG(0x2B02);
        LCD_WR_DATA(ey >> 8);
        LCD_WR_REG(0x2B03);
        LCD_WR_DATA(ey & 0xFF);
    } else {
        LCD_WR_REG(lcddev.setxcmd);
        LCD_WR_DATA(sx >> 8);
        LCD_WR_DATA(sx & 0xFF);
        LCD_WR_DATA(ex >> 8);
        LCD_WR_DATA(ex & 0xFF);

        LCD_WR_REG(lcddev.setycmd);
        LCD_WR_DATA(sy >> 8);
        LCD_WR_DATA(sy & 0xFF);
        LCD_WR_DATA(ey >> 8);
        LCD_WR_DATA(ey & 0xFF);
    }
}

void LCD_Fast_DrawPoint(uint16_t x, uint16_t y, uint16_t color) {
    LCD_SetCursor(x, y);
    LCD_WR_REG(lcddev.wramcmd);
    LCD_WR_DATA(color);
}

static inline void opt_delay(volatile uint8_t i) {
    while (i--) {
        __NOP();
    }
}

uint16_t LCD_ReadPoint(uint16_t x, uint16_t y) {
    uint16_t r = 0, g = 0, b = 0;
    if (x >= lcddev.width || y >= lcddev.height) return 0;
    LCD_SetCursor(x, y);
    if (lcddev.id == 0x5510) {
        LCD_WR_REG(0x2E00);
    } else {
        LCD_WR_REG(0x2E);
    }
    r = LCD_RD_DATA(); // dummy read
    if (lcddev.id == 0x1963) return r;

    opt_delay(2);
    r = LCD_RD_DATA(); // 5510 下：高8位红，低8位绿
    if (lcddev.id == 0x5510) {
        opt_delay(2);
        b = LCD_RD_DATA(); // 5510 下：高8位蓝
        g = r & 0xFF;
        g <<= 8;
    }
    return (((r >> 11) << 11) | ((g >> 10) << 5) | (b >> 11));
}

void LCD_DrawPoint(uint16_t x, uint16_t y) {
    LCD_Fast_DrawPoint(x, y, POINT_COLOR);
}

void LCD_Clear(uint16_t Color) {
    LCD_Lock();
    uint32_t total = (uint32_t)lcddev.width * lcddev.height;
    LCD_Set_Window(0, 0, lcddev.width, lcddev.height);
    LCD_WR_REG(lcddev.wramcmd);
    for (uint32_t i = 0; i < total; i++) {
        LCD_WR_DATA(Color);
    }
    LCD_Unlock();
}

void LCD_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color) {
    if (ex >= lcddev.width) ex = lcddev.width - 1;
    if (ey >= lcddev.height) ey = lcddev.height - 1;
    if (sx > ex || sy > ey) return;

    LCD_Lock();
    uint16_t w = ex - sx + 1;
    uint16_t h = ey - sy + 1;
    uint32_t total = (uint32_t)w * h;
    LCD_Set_Window(sx, sy, w, h);
    LCD_WR_REG(lcddev.wramcmd);
    for (uint32_t i = 0; i < total; i++) {
        LCD_WR_DATA(color);
    }
    LCD_Unlock();
}

void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    LCD_Lock();
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;

    delta_x = x2 - x1;
    delta_y = y2 - y1;
    uRow = x1;
    uCol = y1;

    if (delta_x > 0) incx = 1;
    else if (delta_x == 0) incx = 0;
    else { incx = -1; delta_x = -delta_x; }

    if (delta_y > 0) incy = 1;
    else if (delta_y == 0) incy = 0;
    else { incy = -1; delta_y = -delta_y; }

    if (delta_x > delta_y) distance = delta_x;
    else distance = delta_y;

    for (t = 0; t <= distance + 1; t++) {
        LCD_DrawPoint(uRow, uCol);
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance) {
            xerr -= distance;
            uRow += incx;
        }
        if (yerr > distance) {
            yerr -= distance;
            uCol += incy;
        }
    }
    LCD_Unlock();
}

void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    LCD_DrawLine(x1, y1, x2, y1);
    LCD_DrawLine(x1, y1, x1, y2);
    LCD_DrawLine(x1, y2, x2, y2);
    LCD_DrawLine(x2, y1, x2, y2);
}

void LCD_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint8_t size, uint8_t mode) {
    uint8_t temp;
    if (num < ' ' || num > '~') return;
    num = num - ' ';

    LCD_Lock();
    for (uint8_t r = 0; r < 16; r++) {
        temp = asc2_1608[num][r];
        for (uint8_t c = 0; c < 8; c++) {
            if (temp & 0x80) {
                LCD_Fast_DrawPoint(x + c, y + r, POINT_COLOR);
            } else if (mode == 0) {
                LCD_Fast_DrawPoint(x + c, y + r, BACK_COLOR);
            }
            temp <<= 1;
        }
    }
    LCD_Unlock();
}

void LCD_ShowString(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, uint8_t *p) {
    LCD_Lock();
    uint16_t x0 = x;
    uint16_t y0 = y;
    while (*p != '\0') {
        if (x + 8 > (x0 + width)) {
            x = x0;
            y += 16;
        }
        if ((y + 16) > (y0 + height)) break;
        LCD_ShowChar(x, y, *p, size, 0);
        x += 8;
        p++;
    }
    LCD_Unlock();
}

static void LCD_Init_ILI9341(void) {
    LCD_WR_REG(0xCF);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xC1);
    LCD_WR_DATA(0X30);
    LCD_WR_REG(0xED);
    LCD_WR_DATA(0x64);
    LCD_WR_DATA(0x03);
    LCD_WR_DATA(0X12);
    LCD_WR_DATA(0X81);
    LCD_WR_REG(0xE8);
    LCD_WR_DATA(0x85);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x78);
    LCD_WR_REG(0xCB);
    LCD_WR_DATA(0x39);
    LCD_WR_DATA(0x2C);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x34);
    LCD_WR_DATA(0x02);
    LCD_WR_REG(0xF7);
    LCD_WR_DATA(0x20);
    LCD_WR_REG(0xEA);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_REG(0xC0);    // Power control
    LCD_WR_DATA(0x1B);
    LCD_WR_REG(0xC1);    // Power control
    LCD_WR_DATA(0x12);
    LCD_WR_REG(0xC5);    // VCM control
    LCD_WR_DATA(0x3E);
    LCD_WR_DATA(0x28);
    LCD_WR_REG(0xC7);    // VCM control2
    LCD_WR_DATA(0x86);
    LCD_WR_REG(0x36);    // Memory Access Control
    LCD_WR_DATA(0x08);
    LCD_WR_REG(0x3A);
    LCD_WR_DATA(0x55);
    LCD_WR_REG(0xB1);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x1A);
    LCD_WR_REG(0xB6);    // Display Function Control
    LCD_WR_DATA(0x0A);
    LCD_WR_DATA(0xA2);
    LCD_WR_REG(0xF2);    // 3Gamma Function Disable
    LCD_WR_DATA(0x00);
    LCD_WR_REG(0x26);    // Gamma curve selected
    LCD_WR_DATA(0x01);
    LCD_WR_REG(0xE0);    // Set Gamma
    LCD_WR_DATA(0x0F);
    LCD_WR_DATA(0x2A);
    LCD_WR_DATA(0x28);
    LCD_WR_DATA(0x08);
    LCD_WR_DATA(0x0E);
    LCD_WR_DATA(0x08);
    LCD_WR_DATA(0x54);
    LCD_WR_DATA(0XA9);
    LCD_WR_DATA(0x43);
    LCD_WR_DATA(0x0A);
    LCD_WR_DATA(0x0F);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_REG(0XE1);    // Set Gamma
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x15);
    LCD_WR_DATA(0x17);
    LCD_WR_DATA(0x07);
    LCD_WR_DATA(0x11);
    LCD_WR_DATA(0x06);
    LCD_WR_DATA(0x2B);
    LCD_WR_DATA(0x56);
    LCD_WR_DATA(0x3C);
    LCD_WR_DATA(0x05);
    LCD_WR_DATA(0x10);
    LCD_WR_DATA(0x0F);
    LCD_WR_DATA(0x3F);
    LCD_WR_DATA(0x3F);
    LCD_WR_DATA(0x0F);
    LCD_WR_REG(0x2B);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x01);
    LCD_WR_DATA(0x3f);
    LCD_WR_REG(0x2A);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xef);
    LCD_WR_REG(0x11);    // Exit Sleep
    HAL_Delay(120);
    LCD_WR_REG(0x29);    // Display on
}

static void LCD_Init_NT35510(void) {
#define W(r, d) do { LCD_WriteReg(r, d); } while(0)
    // 1. Page 1 enable
    W(0xF000, 0x55); W(0xF001, 0xAA); W(0xF002, 0x52); W(0xF003, 0x08); W(0xF004, 0x01);

    // 电源与电压倍率配置
    W(0xB000, 0x0D); W(0xB001, 0x0D); W(0xB002, 0x0D);
    W(0xB600, 0x34); W(0xB601, 0x34); W(0xB602, 0x34);
    W(0xB100, 0x0D); W(0xB101, 0x0D); W(0xB102, 0x0D);
    W(0xB700, 0x34); W(0xB701, 0x34); W(0xB702, 0x34);
    W(0xB200, 0x00); W(0xB201, 0x00); W(0xB202, 0x00);
    W(0xB800, 0x24); W(0xB801, 0x24); W(0xB802, 0x24); W(0xBF00, 0x01);
    W(0xB300, 0x0F); W(0xB301, 0x0F); W(0xB302, 0x0F);
    W(0xB900, 0x34); W(0xB901, 0x34); W(0xB902, 0x34);
    W(0xB500, 0x08); W(0xB501, 0x08); W(0xB502, 0x08); W(0xC200, 0x03);
    W(0xBA00, 0x24); W(0xBA01, 0x24); W(0xBA02, 0x24);
    W(0xBC00, 0x00); W(0xBC01, 0x78); W(0xBC02, 0x00);
    W(0xBD00, 0x00); W(0xBD01, 0x78); W(0xBD02, 0x00);
    W(0xBE00, 0x00); W(0xBE01, 0x64);

    // 2. 伽马参数 (Gamma) 校准表 (保证液晶偏压有效，杜绝纯白透光)
    W(0xD100, 0x00); W(0xD101, 0x33); W(0xD102, 0x00); W(0xD103, 0x34); W(0xD104, 0x00); W(0xD105, 0x3A);
    W(0xD106, 0x00); W(0xD107, 0x4A); W(0xD108, 0x00); W(0xD109, 0x5C); W(0xD10A, 0x00); W(0xD10B, 0x81);
    W(0xD10C, 0x00); W(0xD10D, 0xA6); W(0xD10E, 0x00); W(0xD10F, 0xE5); W(0xD110, 0x01); W(0xD111, 0x13);
    W(0xD112, 0x01); W(0xD113, 0x54); W(0xD114, 0x01); W(0xD115, 0x82); W(0xD116, 0x01); W(0xD117, 0xCA);
    W(0xD118, 0x02); W(0xD119, 0x00); W(0xD11A, 0x02); W(0xD11B, 0x01); W(0xD11C, 0x02); W(0xD11D, 0x34);
    W(0xD11E, 0x02); W(0xD11F, 0x67); W(0xD120, 0x02); W(0xD121, 0x84); W(0xD122, 0x02); W(0xD123, 0xA4);
    W(0xD124, 0x02); W(0xD125, 0xB7); W(0xD126, 0x02); W(0xD127, 0xCF); W(0xD128, 0x02); W(0xD129, 0xDE);
    W(0xD12A, 0x02); W(0xD12B, 0xF2); W(0xD12C, 0x02); W(0xD12D, 0xFE); W(0xD12E, 0x03); W(0xD12F, 0x10);
    W(0xD130, 0x03); W(0xD131, 0x33); W(0xD132, 0x03); W(0xD133, 0x6D);

    W(0xD200, 0x00); W(0xD201, 0x33); W(0xD202, 0x00); W(0xD203, 0x34); W(0xD204, 0x00); W(0xD205, 0x3A);
    W(0xD206, 0x00); W(0xD207, 0x4A); W(0xD208, 0x00); W(0xD209, 0x5C); W(0xD20A, 0x00); W(0xD20B, 0x81);
    W(0xD20C, 0x00); W(0xD20D, 0xA6); W(0xD20E, 0x00); W(0xD20F, 0xE5); W(0xD210, 0x01); W(0xD211, 0x13);
    W(0xD212, 0x01); W(0xD213, 0x54); W(0xD214, 0x01); W(0xD215, 0x82); W(0xD216, 0x01); W(0xD217, 0xCA);
    W(0xD218, 0x02); W(0xD219, 0x00); W(0xD21A, 0x02); W(0xD21B, 0x01); W(0xD21C, 0x02); W(0xD21D, 0x34);
    W(0xD21E, 0x02); W(0xD21F, 0x67); W(0xD220, 0x02); W(0xD221, 0x84); W(0xD222, 0x02); W(0xD223, 0xA4);
    W(0xD224, 0x02); W(0xD225, 0xB7); W(0xD226, 0x02); W(0xD227, 0xCF); W(0xD228, 0x02); W(0xD229, 0xDE);
    W(0xD22A, 0x02); W(0xD22B, 0xF2); W(0xD22C, 0x02); W(0xD22D, 0xFE); W(0xD22E, 0x03); W(0xD22F, 0x10);
    W(0xD230, 0x03); W(0xD231, 0x33); W(0xD232, 0x03); W(0xD233, 0x6D);

    W(0xD300, 0x00); W(0xD301, 0x33); W(0xD302, 0x00); W(0xD303, 0x34); W(0xD304, 0x00); W(0xD305, 0x3A);
    W(0xD306, 0x00); W(0xD307, 0x4A); W(0xD308, 0x00); W(0xD309, 0x5C); W(0xD30A, 0x00); W(0xD30B, 0x81);
    W(0xD30C, 0x00); W(0xD30D, 0xA6); W(0xD30E, 0x00); W(0xD30F, 0xE5); W(0xD310, 0x01); W(0xD311, 0x13);
    W(0xD312, 0x01); W(0xD313, 0x54); W(0xD314, 0x01); W(0xD315, 0x82); W(0xD316, 0x01); W(0xD317, 0xCA);
    W(0xD318, 0x02); W(0xD319, 0x00); W(0xD31A, 0x02); W(0xD31B, 0x01); W(0xD31C, 0x02); W(0xD31D, 0x34);
    W(0xD31E, 0x02); W(0xD31F, 0x67); W(0xD320, 0x02); W(0xD321, 0x84); W(0xD322, 0x02); W(0xD323, 0xA4);
    W(0xD324, 0x02); W(0xD325, 0xB7); W(0xD326, 0x02); W(0xD327, 0xCF); W(0xD328, 0x02); W(0xD329, 0xDE);
    W(0xD32A, 0x02); W(0xD32B, 0xF2); W(0xD32C, 0x02); W(0xD32D, 0xFE); W(0xD32E, 0x03); W(0xD32F, 0x10);
    W(0xD330, 0x03); W(0xD331, 0x33); W(0xD332, 0x03); W(0xD333, 0x6D);

    W(0xD400, 0x00); W(0xD401, 0x33); W(0xD402, 0x00); W(0xD403, 0x34); W(0xD404, 0x00); W(0xD405, 0x3A);
    W(0xD406, 0x00); W(0xD407, 0x4A); W(0xD408, 0x00); W(0xD409, 0x5C); W(0xD40A, 0x00); W(0xD40B, 0x81);
    W(0xD40C, 0x00); W(0xD40D, 0xA6); W(0xD40E, 0x00); W(0xD40F, 0xE5); W(0xD410, 0x01); W(0xD411, 0x13);
    W(0xD412, 0x01); W(0xD413, 0x54); W(0xD414, 0x01); W(0xD415, 0x82); W(0xD416, 0x01); W(0xD417, 0xCA);
    W(0xD418, 0x02); W(0xD419, 0x00); W(0xD41A, 0x02); W(0xD41B, 0x01); W(0xD41C, 0x02); W(0xD41D, 0x34);
    W(0xD41E, 0x02); W(0xD41F, 0x67); W(0xD420, 0x02); W(0xD421, 0x84); W(0xD422, 0x02); W(0xD423, 0xA4);
    W(0xD424, 0x02); W(0xD425, 0xB7); W(0xD426, 0x02); W(0xD427, 0xCF); W(0xD428, 0x02); W(0xD429, 0xDE);
    W(0xD42A, 0x02); W(0xD42B, 0xF2); W(0xD42C, 0x02); W(0xD42D, 0xFE); W(0xD42E, 0x03); W(0xD42F, 0x10);
    W(0xD430, 0x03); W(0xD431, 0x33); W(0xD432, 0x03); W(0xD433, 0x6D);

    W(0xD500, 0x00); W(0xD501, 0x33); W(0xD502, 0x00); W(0xD503, 0x34); W(0xD504, 0x00); W(0xD505, 0x3A);
    W(0xD506, 0x00); W(0xD507, 0x4A); W(0xD508, 0x00); W(0xD509, 0x5C); W(0xD50A, 0x00); W(0xD50B, 0x81);
    W(0xD50C, 0x00); W(0xD50D, 0xA6); W(0xD50E, 0x00); W(0xD50F, 0xE5); W(0xD510, 0x01); W(0xD511, 0x13);
    W(0xD512, 0x01); W(0xD513, 0x54); W(0xD514, 0x01); W(0xD515, 0x82); W(0xD516, 0x01); W(0xD517, 0xCA);
    W(0xD518, 0x02); W(0xD519, 0x00); W(0xD51A, 0x02); W(0xD51B, 0x01); W(0xD51C, 0x02); W(0xD51D, 0x34);
    W(0xD51E, 0x02); W(0xD51F, 0x67); W(0xD520, 0x02); W(0xD521, 0x84); W(0xD522, 0x02); W(0xD523, 0xA4);
    W(0xD524, 0x02); W(0xD525, 0xB7); W(0xD526, 0x02); W(0xD527, 0xCF); W(0xD528, 0x02); W(0xD529, 0xDE);
    W(0xD52A, 0x02); W(0xD52B, 0xF2); W(0xD52C, 0x02); W(0xD52D, 0xFE); W(0xD52E, 0x03); W(0xD52F, 0x10);
    W(0xD530, 0x03); W(0xD531, 0x33); W(0xD532, 0x03); W(0xD533, 0x6D);

    W(0xD600, 0x00); W(0xD601, 0x33); W(0xD602, 0x00); W(0xD603, 0x34); W(0xD604, 0x00); W(0xD605, 0x3A);
    W(0xD606, 0x00); W(0xD607, 0x4A); W(0xD608, 0x00); W(0xD609, 0x5C); W(0xD60A, 0x00); W(0xD60B, 0x81);
    W(0xD60C, 0x00); W(0xD60D, 0xA6); W(0xD60E, 0x00); W(0xD60F, 0xE5); W(0xD610, 0x01); W(0xD611, 0x13);
    W(0xD612, 0x01); W(0xD613, 0x54); W(0xD614, 0x01); W(0xD615, 0x82); W(0xD616, 0x01); W(0xD617, 0xCA);
    W(0xD618, 0x02); W(0xD619, 0x00); W(0xD61A, 0x02); W(0xD61B, 0x01); W(0xD61C, 0x02); W(0xD61D, 0x34);
    W(0xD61E, 0x02); W(0xD61F, 0x67); W(0xD620, 0x02); W(0xD621, 0x84); W(0xD622, 0x02); W(0xD623, 0xA4);
    W(0xD624, 0x02); W(0xD625, 0xB7); W(0xD626, 0x02); W(0xD627, 0xCF); W(0xD628, 0x02); W(0xD629, 0xDE);
    W(0xD62A, 0x02); W(0xD62B, 0xF2); W(0xD62C, 0x02); W(0xD62D, 0xFE); W(0xD62E, 0x03); W(0xD62F, 0x10);
    W(0xD630, 0x03); W(0xD631, 0x33); W(0xD632, 0x03); W(0xD633, 0x6D);

    // 3. 切回 DCS 标准命令集 (Page 0)
    W(0xF000, 0x55); W(0xF001, 0xAA); W(0xF002, 0x52); W(0xF003, 0x08); W(0xF004, 0x00);
    W(0xB100, 0xCC); W(0xB101, 0x00);
    W(0xB600, 0x05);
    W(0xB700, 0x70); W(0xB701, 0x70);
    W(0xB800, 0x01); W(0xB801, 0x03); W(0xB802, 0x03); W(0xB803, 0x03);
    W(0xBC00, 0x02); W(0xBC01, 0x00); W(0xBC02, 0x00);
    W(0xC900, 0xD0); W(0xC901, 0x02); W(0xC902, 0x50); W(0xC903, 0x50); W(0xC904, 0x50);
    W(0x3500, 0x00);
    W(0x3A00, 0x55); // 16-bit RGB565
    W(0x3600, 0x00); // 竖屏方向

    // 退出睡眠
    LCD_WR_REG(0x1100);
    HAL_Delay(120);

    // 开启显示
    LCD_WR_REG(0x2900);
    HAL_Delay(50);
#undef W
}

uint16_t g_lcd_probe_raw[16] = {0};

void LCD_Init(void) {
    LCD_FSMC_Init();
    HAL_Delay(50);

    // 默认指令集与尺寸参数
    lcddev.wramcmd = 0x2C;
    lcddev.setxcmd = 0x2A;
    lcddev.setycmd = 0x2B;
    lcddev.width   = 240;
    lcddev.height  = 320;
    lcddev.id      = 0;

    // 1. 尝试 0xD3 (ILI9341 / 9342)
    LCD_WR_REG(0xD3);
    g_lcd_probe_raw[0] = LCD_RD_DATA(); // dummy
    g_lcd_probe_raw[1] = LCD_RD_DATA(); // 0x00
    g_lcd_probe_raw[2] = LCD_RD_DATA(); // 0x93
    g_lcd_probe_raw[3] = LCD_RD_DATA(); // 0x41
    if ((g_lcd_probe_raw[2] == 0x93) && (g_lcd_probe_raw[3] == 0x41)) {
        lcddev.id = 0x9341;
    }

    // 2. 尝试 0x04 (ST7789)
    if (lcddev.id == 0) {
        LCD_WR_REG(0x04);
        g_lcd_probe_raw[4] = LCD_RD_DATA(); // dummy
        g_lcd_probe_raw[5] = LCD_RD_DATA(); // 0x00
        g_lcd_probe_raw[6] = LCD_RD_DATA(); // 0x85
        g_lcd_probe_raw[7] = LCD_RD_DATA(); // 0x52
        if ((g_lcd_probe_raw[6] == 0x85) && (g_lcd_probe_raw[7] == 0x52)) {
            lcddev.id = 0x7789;
        }
    }

    // 3. 尝试 0xD4 (NT35310)
    if (lcddev.id == 0) {
        LCD_WR_REG(0xD4);
        g_lcd_probe_raw[8] = LCD_RD_DATA();
        g_lcd_probe_raw[9] = LCD_RD_DATA();
        g_lcd_probe_raw[10] = LCD_RD_DATA();
        g_lcd_probe_raw[11] = LCD_RD_DATA();
        if ((g_lcd_probe_raw[10] == 0x53) && (g_lcd_probe_raw[11] == 0x10)) {
            lcddev.id = 0x5310;
        }
    }

    // 4. 尝试 0xDA00 / 0xDB00 (NT35510, 16位指令)
    if (lcddev.id == 0) {
        LCD_WR_REG(0xDA00);
        g_lcd_probe_raw[12] = LCD_RD_DATA();
        LCD_WR_REG(0xDB00);
        g_lcd_probe_raw[13] = LCD_RD_DATA();
        LCD_WR_REG(0xDC00);
        g_lcd_probe_raw[14] = LCD_RD_DATA();
        if ((g_lcd_probe_raw[13] & 0xFF) == 0x80 || (g_lcd_probe_raw[14] & 0xFF) == 0x00) {
            if (g_lcd_probe_raw[12] || g_lcd_probe_raw[13]) {
                lcddev.id = 0x5510;
            }
        }
    }

    // 5. 尝试 0x00 (ILI9325 / 9320)
    if (lcddev.id == 0) {
        g_lcd_probe_raw[15] = LCD_ReadReg(0x00);
        if (g_lcd_probe_raw[15] == 0x9325 || g_lcd_probe_raw[15] == 0x9328) {
            lcddev.id = g_lcd_probe_raw[15];
        }
    }

    // 6. 尝试 0xA1 (SSD1963)
    if (lcddev.id == 0) {
        LCD_WR_REG(0xA1);
        uint16_t s1 = LCD_RD_DATA();
        uint16_t s2 = LCD_RD_DATA();
        if (s1 == 0x57 && s2 == 0x61) {
            lcddev.id = 0x1963;
        }
    }

    // 针对探测到的型号设置分辨率与指令
    if (lcddev.id == 0x5310) {
        lcddev.width = 320;
        lcddev.height = 480;
    } else if (lcddev.id == 0x5510 || lcddev.id == 0x1963) {
        lcddev.width = 480;
        lcddev.height = 800;
        lcddev.wramcmd = 0x2C00;
        lcddev.setxcmd = 0x2A00;
        lcddev.setycmd = 0x2B00;
    }

    // 根据识别出的型号执行对应初始化
    if (lcddev.id == 0x5510) {
        LCD_Init_NT35510();
    } else {
        LCD_Init_ILI9341();
    }

    // 开启背光并清屏
    LCD_LED_SET();
    LCD_Clear(BLACK);
}
