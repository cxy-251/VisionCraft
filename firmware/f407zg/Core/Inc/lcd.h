#ifndef __LCD_H
#define __LCD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

// LCD 映射地址与总线结构
// FSMC Bank1 Block4 (0x6C000000)
// A6 对应引脚 PF12 -> 16位模式下 HADDR[7] = 0x80
// 0x6C00007E 为寄存器 (RS=0), 0x6C000080 为数据 (RS=1)
typedef struct {
    volatile uint16_t LCD_REG;
    volatile uint16_t LCD_RAM;
} LCD_TypeDef;

#define LCD_BASE        ((uint32_t)(0x6C000000 | 0x0000007E))
#define LCD             ((LCD_TypeDef *) LCD_BASE)

// LCD 参数结构体
typedef struct {
    uint16_t width;       // LCD 宽度
    uint16_t height;      // LCD 高度
    uint16_t id;          // LCD 控制芯片 ID
    uint8_t  dir;         // 横屏/竖屏控制 (0: 竖屏, 1: 横屏)
    uint16_t wramcmd;     // 写 RAM 启动指令
    uint16_t setxcmd;     // 设置 X 坐标指令
    uint16_t setycmd;     // 设置 Y 坐标指令
} _lcd_dev;

extern _lcd_dev lcddev;
extern uint16_t POINT_COLOR; // 笔刷颜色
extern uint16_t BACK_COLOR;  // 背景颜色

// 背光控制引脚 (PB15)
#define LCD_LED_SET()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET)
#define LCD_LED_CLR()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET)

// 常用 RGB565 颜色定义
#define WHITE           0xFFFF
#define BLACK           0x0000
#define BLUE            0x001F
#define BRED            0xF81F
#define GRED            0xFFE0
#define GBLUE           0x07FF
#define RED             0xF800
#define MAGENTA         0xF81F
#define GREEN           0x07E0
#define CYAN            0x7FFF
#define YELLOW          0xFFE0
#define BROWN           0xBC40
#define BRRED           0xFC07
#define GRAY            0x8430
#define DARKBLUE        0x01CF
#define LIGHTBLUE       0x7D7C
#define GRAYBLUE        0x5458
#define LIGHTGREEN      0x841F
#define LGRAY           0xC618
#define LGRAYBLUE       0xA651
#define LBBLUE          0x2B12

void LCD_Init(void);
void LCD_DisplayOn(void);
void LCD_DisplayOff(void);
void LCD_Clear(uint16_t Color);
void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos);
void LCD_DrawPoint(uint16_t x, uint16_t y);
void LCD_Fast_DrawPoint(uint16_t x, uint16_t y, uint16_t color);
uint16_t LCD_ReadPoint(uint16_t x, uint16_t y);
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color);
void LCD_Color_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color);
void LCD_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint8_t size, uint8_t mode);
void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size);
void LCD_ShowxNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode);
void LCD_ShowString(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, uint8_t *p);
void LCD_Set_Window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);
void LCD_Lock(void);
void LCD_Unlock(void);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_H */
