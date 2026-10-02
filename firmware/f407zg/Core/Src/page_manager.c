#include "page_manager.h"
#include "lcd.h"
#include "bsp_backlight.h"
#include "bsp_key.h"
#include "bsp_touch.h"
#include "bsp_spi_flash.h"
#include "bsp_at24c02.h"
#include "bsp_fs_manager.h"
#include "bsp_terminal.h"
#include "bsp_lsens.h"
#include "bsp_cpu_temp.h"
#include "bsp_sram.h"
#include "bsp_usb_mouse.h"
#include "bsp_rtc.h"
#include "bsp_rng.h"
#include "bsp_dac.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

static uint8_t s_current = 0;
static uint32_t s_uptime = 0;
static uint32_t s_last_switch_tick = 0;

static void Draw_Common_Frames(void);

// ==========================================
// Page 0: 系统主看板
// ==========================================
static void Page0_Init(void) {
    POINT_COLOR = YELLOW;
    BACK_COLOR  = BLACK;
    LCD_ShowString(20, 50, lcddev.width - 40, 16, 16, (uint8_t *)"[ Hardware Overview ]");

    POINT_COLOR = WHITE;
    LCD_ShowString(20, 80, lcddev.width - 40, 16, 16, (uint8_t *)"MCU Model  : STM32F407ZGT6");
    LCD_ShowString(20, 105, lcddev.width - 40, 16, 16, (uint8_t *)"CPU Clock  : 168 MHz (HSE 8M PLL)");
    LCD_ShowString(20, 130, lcddev.width - 40, 16, 16, (uint8_t *)"Flash/SRAM : 1024 KiB / 192 KiB");

    char buf[64];
    snprintf(buf, sizeof(buf), "LCD Driver : NT35510 (ID: 0x%04X)", lcddev.id);
    LCD_ShowString(20, 155, lcddev.width - 40, 16, 16, (uint8_t *)buf);

    snprintf(buf, sizeof(buf), "Touch Chip : %s",
             g_touch.type == TOUCH_TYPE_GT9147 ? "GT9147 (Capacitive I2C)" : "Not Probed");
    POINT_COLOR = LIGHTGREEN;
    LCD_ShowString(20, 180, lcddev.width - 40, 16, 16, (uint8_t *)buf);

    snprintf(buf, sizeof(buf), "Ext Storage: W25Q128 (%s) | AT24C02 (%s)",
             Bsp_SpiFlash_GetInfo()->is_detected ? "OK" : "FAIL",
             Bsp_At24c02_GetInfo()->is_detected ? "OK" : "FAIL");
    POINT_COLOR = LIGHTBLUE;
    LCD_ShowString(20, 205, lcddev.width - 40, 16, 16, (uint8_t *)buf);

    LCD_DrawLine(20, 230, lcddev.width - 20, 230);

    POINT_COLOR = CYAN;
    LCD_ShowString(20, 245, lcddev.width - 40, 16, 16, (uint8_t *)"[ RTOS Task Statistics ]");

    POINT_COLOR = LGRAY;
    LCD_ShowString(20, 275, lcddev.width - 40, 16, 16, (uint8_t *)"Kernel OS  : FreeRTOS V10.5.1");
    LCD_ShowString(20, 300, lcddev.width - 40, 16, 16, (uint8_t *)"Scheduler  : Preemptive (Tick 1ms)");
}

static void Page0_Update(void) {
    char buf[48];
    POINT_COLOR = GREEN;
    BACK_COLOR  = BLACK;
    snprintf(buf, sizeof(buf), "Free Heap  : %u Bytes Free", (unsigned int)xPortGetFreeHeapSize());
    LCD_ShowString(20, 325, lcddev.width - 40, 16, 16, (uint8_t *)buf);

    uint32_t total = s_uptime;
    uint32_t hrs = total / 3600;
    uint32_t min = (total % 3600) / 60;
    uint32_t sec = total % 60;
    snprintf(buf, sizeof(buf), "Sys Uptime : %02lu:%02lu:%02lu (%lu s)", hrs, min, sec, total);
    POINT_COLOR = RED;
    LCD_ShowString(20, 350, lcddev.width - 40, 16, 16, (uint8_t *)buf);

    // 实时物理传感器
    float temp_c = Bsp_CpuTemp_ReadCelsius();
    uint8_t light_pct = Bsp_Lsens_ReadPercent();
    snprintf(buf, sizeof(buf), "CPU Temp   : %.1f C (ADC1_IN16)", (double)temp_c);
    POINT_COLOR = YELLOW;
    LCD_ShowString(20, 375, lcddev.width - 40, 16, 16, (uint8_t *)buf);

    snprintf(buf, sizeof(buf), "Light Sens : %u%% (PF7 ADC3)", light_pct);
    POINT_COLOR = CYAN;
    LCD_ShowString(20, 400, lcddev.width - 40, 16, 16, (uint8_t *)buf);

    char dt_buf[32];
    Bsp_RTC_GetDateString(dt_buf, sizeof(dt_buf));
    snprintf(buf, sizeof(buf), "RTC Date   : %s", dt_buf);
    POINT_COLOR = LGRAY;
    LCD_ShowString(20, 425, lcddev.width - 40, 16, 16, (uint8_t *)buf);

    snprintf(buf, sizeof(buf), "RNG Sample : 0x%08lX", (unsigned long)Bsp_RNG_Get());
    POINT_COLOR = MAGENTA;
    LCD_ShowString(20, 450, lcddev.width - 40, 16, 16, (uint8_t *)buf);

    snprintf(buf, sizeof(buf), "DAC PA4 Out: %u mV", Bsp_DAC_GetVoltage());
    POINT_COLOR = BRRED;
    LCD_ShowString(20, 475, lcddev.width - 40, 16, 16, (uint8_t *)buf);
}

// ==========================================
// Page 1: 屏幕背光与设置
// ==========================================
static uint8_t s_p1_last_b = 255;

static void Page1_Init(void) {
    s_p1_last_b = 255;
    POINT_COLOR = YELLOW;
    BACK_COLOR  = BLACK;
    LCD_ShowString(20, 50, lcddev.width - 40, 16, 16, (uint8_t *)"[ Backlight PWM Settings ]");

    // 进度条边框
    POINT_COLOR = WHITE;
    LCD_DrawRectangle(20, 110, lcddev.width - 20, 135);

    // 触摸按钮: [-] 与 [+]
    LCD_Fill(20, 150, 120, 190, DARKBLUE);
    LCD_Fill(lcddev.width - 120, 150, lcddev.width - 20, 190, DARKBLUE);
    POINT_COLOR = WHITE;
    BACK_COLOR  = DARKBLUE;
    LCD_ShowString(55, 162, 50, 16, 16, (uint8_t *)"- 10%");
    LCD_ShowString(lcddev.width - 85, 162, 50, 16, 16, (uint8_t *)"+ 10%");

    BACK_COLOR = BLACK;
    POINT_COLOR = LGRAY;
    LCD_ShowString(20, 215, lcddev.width - 40, 16, 16, (uint8_t *)"Touch Tips:");
    LCD_ShowString(20, 240, lcddev.width - 40, 16, 16, (uint8_t *)"- Touch inside slider bar to jump directly");
    LCD_ShowString(20, 265, lcddev.width - 40, 16, 16, (uint8_t *)"- Tap [-10%] or [+10%] buttons to adjust");
    LCD_ShowString(20, 290, lcddev.width - 40, 16, 16, (uint8_t *)"- Physical KEY0(+), KEY1(-), KEY2(Cycle)");
}

static void Page1_Update(void) {
    uint8_t cur_b = Bsp_Backlight_Get();
    if (cur_b != s_p1_last_b) {
        s_p1_last_b = cur_b;
        char buf[32];
        snprintf(buf, sizeof(buf), "Brightness : %3d %%", cur_b);
        POINT_COLOR = LIGHTGREEN;
        BACK_COLOR  = BLACK;
        LCD_ShowString(20, 80, lcddev.width - 40, 16, 16, (uint8_t *)buf);

        uint16_t total_w = lcddev.width - 42;
        uint16_t fill_w = (uint32_t)cur_b * total_w / 100;
        if (fill_w > 0) {
            LCD_Fill(21, 111, 21 + fill_w, 134, BLUE);
        }
        if (fill_w < total_w) {
            LCD_Fill(21 + fill_w + 1, 111, 21 + total_w, 134, BLACK);
        }
    }
}

static void Page1_OnKey(uint8_t key) {
    if (key == KEY0_PRES) {
        uint8_t b = Bsp_Backlight_Get();
        Bsp_Backlight_Set((b <= 90) ? b + 10 : 100);
    } else if (key == KEY1_PRES) {
        uint8_t b = Bsp_Backlight_Get();
        Bsp_Backlight_Set((b >= 15) ? b - 10 : 5);
    } else if (key == KEY2_PRES) {
        const uint8_t presets[] = {20, 50, 80, 100};
        static uint8_t idx = 2;
        idx = (idx + 1) % 4;
        Bsp_Backlight_Set(presets[idx]);
    }
}

static void Page1_OnTouch(uint16_t x, uint16_t y, uint8_t event) {
    if (event == TOUCH_EVENT_UP) return;
    // 检查触摸是否落在进度条内 (20..460, 100..145) - 支持滑动拖拽
    if (x >= 20 && x <= (lcddev.width - 20) && y >= 100 && y <= 145) {
        uint8_t b = (uint32_t)(x - 20) * 100 / (lcddev.width - 40);
        if (b < 5) b = 5;
        if (b > 100) b = 100;
        Bsp_Backlight_Set(b);
    }
    // [-10%] 按钮 (20..120, 150..190) - 仅在按下瞬态触发
    else if (event == TOUCH_EVENT_DOWN && x >= 20 && x <= 120 && y >= 150 && y <= 190) {
        uint8_t b = Bsp_Backlight_Get();
        Bsp_Backlight_Set((b >= 15) ? b - 10 : 5);
    }
    // [+10%] 按钮 (360..460, 150..190) - 仅在按下瞬态触发
    else if (event == TOUCH_EVENT_DOWN && x >= (lcddev.width - 120) && x <= (lcddev.width - 20) && y >= 150 && y <= 190) {
        uint8_t b = Bsp_Backlight_Get();
        Bsp_Backlight_Set((b <= 90) ? b + 10 : 100);
    }
}

// ==========================================
// Page 2: 触摸屏画板与坐标校验
// ==========================================
static uint16_t s_last_touch_x = 0;
static uint16_t s_last_touch_y = 0;
static uint16_t s_prev_draw_x  = 0;
static uint16_t s_prev_draw_y  = 0;
static uint8_t  s_is_drawing   = 0;

static void Page2_Init(void) {
    s_is_drawing = 0;
    POINT_COLOR = YELLOW;
    BACK_COLOR  = BLACK;
    LCD_ShowString(20, 50, lcddev.width - 40, 16, 16, (uint8_t *)"[ Touch Canvas & Test ]");

    // 画板边框
    POINT_COLOR = LGRAY;
    LCD_DrawRectangle(10, 100, lcddev.width - 10, 700);

    // 清屏按钮 (180..300, 705..740)
    LCD_Fill(160, 705, 320, 740, DARKBLUE);
    POINT_COLOR = WHITE;
    BACK_COLOR  = DARKBLUE;
    LCD_ShowString(200, 715, 100, 16, 16, (uint8_t *)"CLEAR CANVAS");
}

static void Page2_Update(void) {
    char buf[40];
    POINT_COLOR = CYAN;
    BACK_COLOR  = BLACK;
    snprintf(buf, sizeof(buf), "Touch Pos : X=%3u, Y=%3u (%s)",
             s_last_touch_x, s_last_touch_y,
             g_touch.pressed ? "DOWN" : "UP  ");
    LCD_ShowString(20, 75, lcddev.width - 40, 16, 16, (uint8_t *)buf);
}

static void Page2_OnTouch(uint16_t x, uint16_t y, uint8_t event) {
    // 手指抬起，断开连续线段状态
    if (event == TOUCH_EVENT_UP) {
        s_is_drawing = 0;
        return;
    }

    s_last_touch_x = x;
    s_last_touch_y = y;

    // 清空画板按钮判定 (160..320, 705..740) - 仅在 DOWN 瞬态触发
    if (event == TOUCH_EVENT_DOWN && x >= 160 && x <= 320 && y >= 705 && y <= 740) {
        LCD_Fill(11, 101, lcddev.width - 11, 699, BLACK);
        s_is_drawing = 0;
        return;
    }

    // 画板区域内插值画线，避免离散点断裂
    if (x > 15 && x < (lcddev.width - 15) && y > 105 && y < 695) {
        POINT_COLOR = RED;
        if (s_is_drawing && event == TOUCH_EVENT_MOVE) {
            LCD_DrawLine(s_prev_draw_x, s_prev_draw_y, x, y);
            // 笔刷加粗 (2 像素)
            LCD_DrawLine(s_prev_draw_x + 1, s_prev_draw_y, x + 1, y);
            LCD_DrawLine(s_prev_draw_x, s_prev_draw_y + 1, x, y + 1);
        } else {
            LCD_Fill(x - 1, y - 1, x + 1, y + 1, RED);
        }
        s_prev_draw_x = x;
        s_prev_draw_y = y;
        s_is_drawing  = 1;
    } else {
        s_is_drawing = 0;
    }
}

// ==========================================
// Page 3: 硬件按键状态检测
// ==========================================
static void Page3_Init(void) {
    POINT_COLOR = YELLOW;
    BACK_COLOR  = BLACK;
    LCD_ShowString(20, 50, lcddev.width - 40, 16, 16, (uint8_t *)"[ Physical Keys Status ]");

    POINT_COLOR = LGRAY;
    LCD_ShowString(20, 80, lcddev.width - 40, 16, 16, (uint8_t *)"Press onboard keys to see real-time triggers:");

    // 绘制 4 个按键卡片边框
    LCD_DrawRectangle(30, 120, 220, 200);  // KEY0
    LCD_DrawRectangle(260, 120, 450, 200); // KEY1
    LCD_DrawRectangle(30, 230, 220, 310);  // KEY2
    LCD_DrawRectangle(260, 230, 450, 310); // WK_UP

    POINT_COLOR = WHITE;
    LCD_ShowString(50, 140, 150, 16, 16, (uint8_t *)"KEY0 (PE4)");
    LCD_ShowString(280, 140, 150, 16, 16, (uint8_t *)"KEY1 (PE3)");
    LCD_ShowString(50, 250, 150, 16, 16, (uint8_t *)"KEY2 (PE2)");
    LCD_ShowString(280, 250, 150, 16, 16, (uint8_t *)"WK_UP (PA0)");
}

static void Page3_Update(void) {
    // 根据按键状态渲染背景指示灯
    uint8_t k0 = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4) == GPIO_PIN_RESET;
    uint8_t k1 = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3) == GPIO_PIN_RESET;
    uint8_t k2 = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2) == GPIO_PIN_RESET;
    uint8_t wk = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET;

    LCD_Fill(31, 165, 219, 199, k0 ? GREEN : BLACK);
    LCD_Fill(261, 165, 449, 199, k1 ? GREEN : BLACK);
    LCD_Fill(31, 275, 219, 309, k2 ? GREEN : BLACK);
    LCD_Fill(261, 275, 449, 309, wk ? GREEN : BLACK);

    POINT_COLOR = k0 ? BLACK : LGRAY;
    BACK_COLOR  = k0 ? GREEN : BLACK;
    LCD_ShowString(90, 172, 80, 16, 16, (uint8_t *)(k0 ? "ACTIVE" : "IDLE"));

    POINT_COLOR = k1 ? BLACK : LGRAY;
    BACK_COLOR  = k1 ? GREEN : BLACK;
    LCD_ShowString(320, 172, 80, 16, 16, (uint8_t *)(k1 ? "ACTIVE" : "IDLE"));

    POINT_COLOR = k2 ? BLACK : LGRAY;
    BACK_COLOR  = k2 ? GREEN : BLACK;
    LCD_ShowString(90, 282, 80, 16, 16, (uint8_t *)(k2 ? "ACTIVE" : "IDLE"));

    POINT_COLOR = wk ? BLACK : LGRAY;
    BACK_COLOR  = wk ? GREEN : BLACK;
    LCD_ShowString(320, 282, 80, 16, 16, (uint8_t *)(wk ? "ACTIVE" : "IDLE"));
}

// ==========================================
// Page 4: 存储介质状态监测 (W25Q128 & AT24C02)
// ==========================================
static uint8_t s_storage_tested = 0;
static uint8_t s_storage_flash_ok = 0;
static uint8_t s_storage_eeprom_ok = 0;
static uint8_t s_storage_sram_ok = 0;
static char s_flash_msg[64] = "Tap [RUN TEST] to verify";
static char s_eeprom_msg[64] = "Tap [RUN TEST] to verify";

static void Page4_DrawCards(void) {
    const Flash_Info_t *p_flash = Bsp_SpiFlash_GetInfo();
    const Eeprom_Info_t *p_eeprom = Bsp_At24c02_GetInfo();
    char buf[128];

    POINT_COLOR = YELLOW;
    BACK_COLOR  = BLACK;
    LCD_ShowString(20, 50, lcddev.width - 40, 16, 16, (uint8_t *)"[ Storage Inspector: Flash & EEPROM ]");

    // 1. W25Q128 卡片边框 (15, 75, 465, 270)
    POINT_COLOR = WHITE;
    LCD_DrawRectangle(15, 75, lcddev.width - 15, 270);
    POINT_COLOR = CYAN;
    LCD_ShowString(25, 85, 300, 16, 16, (uint8_t *)"SPI Flash: W25Q128 (16 MiB)");

    POINT_COLOR = LGRAY;
    LCD_ShowString(25, 110, 420, 16, 16, (uint8_t *)"Bus: SPI1 (PB3/PB4/PB5, CS:PB14)");

    snprintf(buf, sizeof(buf), "JEDEC ID: 0x%06lX (%s)",
             (unsigned long)p_flash->jedec_id,
             p_flash->is_detected ? "Winbond Valid" : "Unknown/Absent");
    POINT_COLOR = p_flash->is_detected ? LIGHTGREEN : RED;
    LCD_ShowString(25, 135, 420, 16, 16, (uint8_t *)buf);

    snprintf(buf, sizeof(buf), "Device ID: 0x%04X | Size: 16 MiB", p_flash->device_id);
    POINT_COLOR = WHITE;
    LCD_ShowString(25, 160, 420, 16, 16, (uint8_t *)buf);

    LCD_ShowString(25, 185, 420, 16, 16, (uint8_t *)"Test Sector: 0x00FFF000 (Last 4KB Sector)");

    snprintf(buf, sizeof(buf), "Result: %s", s_flash_msg);
    POINT_COLOR = s_storage_tested ? (s_storage_flash_ok ? GREEN : RED) : YELLOW;
    LCD_ShowString(25, 215, 420, 16, 16, (uint8_t *)buf);

    uint16_t tag_color = s_storage_tested ? (s_storage_flash_ok ? GREEN : RED) : LGRAY;
    LCD_Fill(360, 82, 450, 106, tag_color);
    POINT_COLOR = BLACK;
    BACK_COLOR  = tag_color;
    LCD_ShowString(375, 86, 70, 16, 16,
                   (uint8_t *)(s_storage_tested ? (s_storage_flash_ok ? "PASS" : "FAIL") : "READY"));

    // 2. AT24C02 卡片边框 (15, 285, 465, 475)
    BACK_COLOR = BLACK;
    POINT_COLOR = WHITE;
    LCD_DrawRectangle(15, 285, lcddev.width - 15, 475);
    POINT_COLOR = CYAN;
    LCD_ShowString(25, 295, 300, 16, 16, (uint8_t *)"I2C EEPROM: AT24C02 (256 Bytes)");

    POINT_COLOR = LGRAY;
    LCD_ShowString(25, 320, 420, 16, 16, (uint8_t *)"Bus: Soft-I2C (SCL:PB8, SDA:PB9)");

    snprintf(buf, sizeof(buf), "I2C Addr: 0xA0 (7-bit 0x50) | Status: %s",
             p_eeprom->is_detected ? "ONLINE" : "NO ACK");
    POINT_COLOR = p_eeprom->is_detected ? LIGHTGREEN : RED;
    LCD_ShowString(25, 345, 420, 16, 16, (uint8_t *)buf);

    snprintf(buf, sizeof(buf), "Structure: 32 Pages x 8 Bytes (tWR <= 5ms)");
    POINT_COLOR = WHITE;
    LCD_ShowString(25, 370, 420, 16, 16, (uint8_t *)buf);

    LCD_ShowString(25, 395, 420, 16, 16, (uint8_t *)"Test Offset: 0x00 (Page 0 Write/Read)");

    snprintf(buf, sizeof(buf), "Result: %s", s_eeprom_msg);
    POINT_COLOR = s_storage_tested ? (s_storage_eeprom_ok ? GREEN : RED) : YELLOW;
    LCD_ShowString(25, 425, 420, 16, 16, (uint8_t *)buf);

    tag_color = s_storage_tested ? (s_storage_eeprom_ok ? GREEN : RED) : LGRAY;
    LCD_Fill(360, 292, 450, 316, tag_color);
    POINT_COLOR = BLACK;
    BACK_COLOR  = tag_color;
    LCD_ShowString(375, 296, 70, 16, 16,
                   (uint8_t *)(s_storage_tested ? (s_storage_eeprom_ok ? "PASS" : "FAIL") : "READY"));

    // 3. 测试控制按钮 (30, 490, 450, 545)
    BACK_COLOR = DARKBLUE;
    LCD_Fill(30, 490, lcddev.width - 30, 545, DARKBLUE);
    POINT_COLOR = WHITE;
    LCD_ShowString(110, 508, 280, 16, 16, (uint8_t *)"[ RUN STORAGE SELF-TEST ]");

    // 4. 诊断日志底框 (15, 560, 465, 730)
    BACK_COLOR = BLACK;
    POINT_COLOR = LGRAY;
    LCD_DrawRectangle(15, 560, lcddev.width - 15, 730);
    POINT_COLOR = YELLOW;
    LCD_ShowString(25, 570, 400, 16, 16, (uint8_t *)"[ Diagnostic Console ]");
    POINT_COLOR = LGRAY;
    LCD_ShowString(25, 595, 420, 16, 16, (uint8_t *)"- Tap button or press KEY0/1/2 to re-test");
    LCD_ShowString(25, 620, 420, 16, 16, (uint8_t *)"- Tests sector erase, page write & verify");

    if (s_storage_tested) {
        snprintf(buf, sizeof(buf), "Flash  : %s", s_storage_flash_ok ? "R/W Check PASSED" : "R/W Check FAILED");
        POINT_COLOR = s_storage_flash_ok ? GREEN : RED;
        LCD_ShowString(25, 650, 420, 16, 16, (uint8_t *)buf);

        snprintf(buf, sizeof(buf), "EEPROM : %s", s_storage_eeprom_ok ? "R/W Check PASSED" : "R/W Check FAILED");
        POINT_COLOR = s_storage_eeprom_ok ? GREEN : RED;
        LCD_ShowString(25, 665, 420, 16, 16, (uint8_t *)buf);

        snprintf(buf, sizeof(buf), "ExtSRAM: %s (1024 KiB)", s_storage_sram_ok ? "R/W Check PASSED" : "R/W Check FAILED");
        POINT_COLOR = s_storage_sram_ok ? GREEN : RED;
        LCD_ShowString(25, 685, 420, 16, 16, (uint8_t *)buf);

        uint8_t all_pass = s_storage_flash_ok && s_storage_eeprom_ok && s_storage_sram_ok;
        POINT_COLOR = all_pass ? GREEN : RED;
        LCD_ShowString(25, 705, 420, 16, 16,
                       (uint8_t *)(all_pass ? ">> ALL STORAGE INTEGRITY CHECKS PASSED <<" : ">> STORAGE INTEGRITY CHECK FAILED <<"));
    } else {
        POINT_COLOR = LGRAY;
        LCD_ShowString(25, 650, 420, 16, 16, (uint8_t *)"Status : Waiting for user test execution");
    }
}

static void Page4_RunTest(void) {
    POINT_COLOR = YELLOW;
    BACK_COLOR  = DARKBLUE;
    LCD_ShowString(110, 508, 280, 16, 16, (uint8_t *)"[ TESTING IN PROGRESS... ]");

    // 1. SPI Flash 测试 (地址: 0x00FFF000，即末尾 4KB)
    s_storage_flash_ok = Bsp_SpiFlash_SelfTest(0x00FFF000, s_flash_msg, sizeof(s_flash_msg));

    // 2. EEPROM 测试 (地址: 0x00)
    s_storage_eeprom_ok = Bsp_At24c02_SelfTest(0x00, s_eeprom_msg, sizeof(s_eeprom_msg));

    // 3. 外部 1MB SRAM 测试
    uint32_t sram_err = 0;
    s_storage_sram_ok = Bsp_Sram_SelfTest(64 * 1024, &sram_err);

    s_storage_tested = 1;

    Page4_DrawCards();
}

static void Page4_Init(void) {
    Page4_DrawCards();
    if (!s_storage_tested) {
        Page4_RunTest();
    }
}

static void Page4_Update(void) {
}

static void Page4_OnKey(uint8_t key) {
    if (key == KEY0_PRES || key == KEY1_PRES || key == KEY2_PRES) {
        Page4_RunTest();
    }
}

static void Page4_OnTouch(uint16_t x, uint16_t y, uint8_t event) {
    if (event == TOUCH_EVENT_DOWN && x >= 30 && x <= (lcddev.width - 30) && y >= 490 && y <= 545) {
        Page4_RunTest();
    }
}

// ==========================================
// Page 5: 文件系统管理器 (FatFs: MicroSD + SPI Flash)
// ==========================================
static uint8_t s_fs_cur_drive = 1; // 默认选中板载 W25Q128 Flash (用户未插卡也能看文件)
static uint8_t s_fs_sel_file = 0;

static void Page5_Render(void);

static void Page5_Init(void) {
    Page5_Render();
}

static void Page5_Update(void) {
}

static void Page5_Render(void) {
    const Fs_Explorer_State_t *st = Bsp_FsManager_GetState(s_fs_cur_drive);
    char buf[128];

    // 清空中部区域 (36..749)
    LCD_Fill(0, 36, lcddev.width, 749, BLACK);

    // 1. 顶部驱动器选项卡 (Tab 0: SD, Tab 1: Flash)
    uint16_t tab0_color = (s_fs_cur_drive == 0) ? BLUE : 0x2124;
    uint16_t tab1_color = (s_fs_cur_drive == 1) ? BLUE : 0x2124;

    LCD_Fill(15, 45, 230, 85, tab0_color);
    POINT_COLOR = WHITE;
    BACK_COLOR  = tab0_color;
    LCD_ShowString(45, 57, 180, 16, 16, (uint8_t *)"[ 0: MicroSD (TF) ]");

    LCD_Fill(250, 45, 465, 85, tab1_color);
    BACK_COLOR  = tab1_color;
    LCD_ShowString(275, 57, 180, 16, 16, (uint8_t *)"[ 1: SPI Flash FAT ]");

    // 2. 驱动器状态摘要栏 (15, 95, 465, 160)
    BACK_COLOR = BLACK;
    POINT_COLOR = WHITE;
    LCD_DrawRectangle(15, 95, lcddev.width - 15, 160);

    POINT_COLOR = (st && st->is_mounted) ? LIGHTGREEN : YELLOW;
    snprintf(buf, sizeof(buf), "Status : %s", st ? st->status_str : "Offline");
    LCD_ShowString(25, 105, 430, 16, 16, (uint8_t *)buf);

    POINT_COLOR = LGRAY;
    if (st && st->is_mounted) {
        snprintf(buf, sizeof(buf), "Capacity: Total %lu KiB | Free %lu KiB",
                 (unsigned long)st->total_kb, (unsigned long)st->free_kb);
    } else {
        snprintf(buf, sizeof(buf), "Media   : Unmounted / Insert Media to Read");
    }
    LCD_ShowString(25, 130, 430, 16, 16, (uint8_t *)buf);

    // 3. 文件列表框 (15, 170, 465, 470)
    POINT_COLOR = WHITE;
    LCD_DrawRectangle(15, 170, lcddev.width - 15, 470);
    POINT_COLOR = CYAN;
    snprintf(buf, sizeof(buf), "[ Directory: %d:/ ] (%u Items)", s_fs_cur_drive, st ? st->file_count : 0);
    LCD_ShowString(25, 178, 430, 16, 16, (uint8_t *)buf);

    if (!st || !st->is_mounted || st->file_count == 0) {
        POINT_COLOR = LGRAY;
        LCD_ShowString(40, 240, 400, 16, 16,
                       (uint8_t *)(st && st->is_mounted ? "< Empty Directory >" : "< Drive Offline / Not Mounted >"));
    } else {
        for (uint16_t i = 0; i < st->file_count && i < 8; i++) {
            uint16_t row_y = 205 + i * 32;
            uint8_t is_sel = (i == s_fs_sel_file);

            if (is_sel) {
                LCD_Fill(18, row_y - 2, lcddev.width - 18, row_y + 26, 0x18F3);
                BACK_COLOR = 0x18F3;
            } else {
                BACK_COLOR = BLACK;
            }

            POINT_COLOR = st->files[i].is_dir ? YELLOW : WHITE;
            snprintf(buf, sizeof(buf), "%c %-16s %7lu B",
                     st->files[i].is_dir ? 'D' : 'F',
                     st->files[i].name,
                     (unsigned long)st->files[i].size);
            LCD_ShowString(25, row_y + 4, 430, 16, 16, (uint8_t *)buf);
        }
    }

    // 4. 文件内容预览框 (15, 480, 465, 635)
    BACK_COLOR = BLACK;
    POINT_COLOR = LGRAY;
    LCD_DrawRectangle(15, 480, lcddev.width - 15, 635);
    POINT_COLOR = YELLOW;
    snprintf(buf, sizeof(buf), "Preview: %s", (st && st->preview_filename[0]) ? st->preview_filename : "(None)");
    LCD_ShowString(25, 488, 430, 16, 16, (uint8_t *)buf);

    POINT_COLOR = LIGHTGREEN;
    if (st && st->preview_buf[0]) {
        char line_buf[48];
        const char *p = st->preview_buf;
        for (int line = 0; line < 4 && *p; line++) {
            int idx = 0;
            while (*p && *p != '\r' && *p != '\n' && idx < 40) {
                line_buf[idx++] = *p++;
            }
            line_buf[idx] = '\0';
            while (*p == '\r' || *p == '\n') p++;
            LCD_ShowString(25, 515 + line * 26, 430, 16, 16, (uint8_t *)line_buf);
        }
    }

    // 5. 底部操作按钮栏 (650..705)
    LCD_Fill(15, 650, 155, 705, DARKBLUE);
    POINT_COLOR = WHITE;
    BACK_COLOR  = DARKBLUE;
    LCD_ShowString(30, 668, 120, 16, 16, (uint8_t *)"+ NEW FILE");

    LCD_Fill(170, 650, 310, 705, DARKBLUE);
    BACK_COLOR  = DARKBLUE;
    LCD_ShowString(195, 668, 110, 16, 16, (uint8_t *)"REFRESH");

    LCD_Fill(325, 650, 465, 705, 0x8000); // 暗红
    BACK_COLOR  = 0x8000;
    LCD_ShowString(355, 668, 100, 16, 16, (uint8_t *)"FORMAT");

    BACK_COLOR = BLACK;
}

static void Page5_OnTouch(uint16_t x, uint16_t y, uint8_t event) {
    if (event != TOUCH_EVENT_DOWN) return;

    // 1. 切换驱动器选项卡 (45..85)
    if (y >= 45 && y <= 85) {
        if (x >= 15 && x <= 230) {
            s_fs_cur_drive = 0;
            s_fs_sel_file = 0;
            Bsp_FsManager_MountDrive(0);
            Page5_Render();
            return;
        } else if (x >= 250 && x <= 465) {
            s_fs_cur_drive = 1;
            s_fs_sel_file = 0;
            Bsp_FsManager_MountDrive(1);
            Page5_Render();
            return;
        }
    }

    // 2. 点击文件列表项选择预览 (205..461)
    if (y >= 205 && y <= 461 && x >= 15 && x <= 465) {
        uint8_t row = (y - 205) / 32;
        const Fs_Explorer_State_t *st = Bsp_FsManager_GetState(s_fs_cur_drive);
        if (st && row < st->file_count) {
            s_fs_sel_file = row;
            Bsp_FsManager_ReadPreview(s_fs_cur_drive, st->files[row].name);
            Page5_Render();
            return;
        }
    }

    // 3. 点击底部按钮 (650..705)
    if (y >= 650 && y <= 705) {
        if (x >= 15 && x <= 155) {
            // [ + NEW FILE ]
            static uint32_t s_note_idx = 1;
            char fname[32];
            char content[96];
            snprintf(fname, sizeof(fname), "LOG_%lu.TXT", (unsigned long)s_note_idx++);
            snprintf(content, sizeof(content), "Log Entry created at Uptime: %lu s\r\nDrive: %d:/\r\nStatus: OK\r\n",
                     (unsigned long)s_uptime, s_fs_cur_drive);
            Bsp_FsManager_CreateDemoFile(s_fs_cur_drive, fname, content);
            Page5_Render();
            return;
        } else if (x >= 170 && x <= 310) {
            // [ REFRESH ]
            Bsp_FsManager_MountDrive(s_fs_cur_drive);
            Page5_Render();
            return;
        } else if (x >= 325 && x <= 465) {
            // [ FORMAT ]
            Bsp_FsManager_FormatDrive(s_fs_cur_drive);
            if (s_fs_cur_drive == 1) {
                Bsp_FsManager_CreateDemoFile(1, "README.TXT", "VisionCraft FatFs Re-Formatted!\r\n");
            }
            Page5_Render();
            return;
        }
    }
}

// ==========================================
// Page 6: 终端控制台与交互软键盘
// ==========================================
static void Page6_Init(void) {
    Bsp_Terminal_DrawView();
}

static void Page6_Update(void) {
    Bsp_Terminal_Update();
}

static void Page6_OnKey(uint8_t key) {
    Bsp_Terminal_OnKey(key);
}

static void Page6_OnTouch(uint16_t x, uint16_t y, uint8_t event) {
    Bsp_Terminal_OnTouch(x, y, event);
}

// ==========================================
// 页面注册表与总控
// ==========================================
static const Page_t s_pages[PAGE_COUNT] = {
    {"System Overview",   Page0_Init, Page0_Update, NULL,        NULL},
    {"Backlight PWM",     Page1_Init, Page1_Update, Page1_OnKey, Page1_OnTouch},
    {"Touch Screen Pad",  Page2_Init, Page2_Update, NULL,        Page2_OnTouch},
    {"Hardware Keys",     Page3_Init, Page3_Update, NULL,        NULL},
    {"Storage Inspector", Page4_Init, Page4_Update, Page4_OnKey, Page4_OnTouch},
    {"File Explorer",     Page5_Init, Page5_Update, NULL,        Page5_OnTouch},
    {"Terminal Console",  Page6_Init, Page6_Update, Page6_OnKey, Page6_OnTouch}
};

static void Draw_Common_Frames(void) {
    // 1. 顶部标题栏 (0..35)
    LCD_Fill(0, 0, lcddev.width, 35, DARKBLUE);
    POINT_COLOR = WHITE;
    BACK_COLOR  = DARKBLUE;

    char title_buf[64];
    snprintf(title_buf, sizeof(title_buf), "[%d/%d] %s", s_current + 1, PAGE_COUNT, s_pages[s_current].title);
    LCD_ShowString(12, 10, 360, 16, 16, (uint8_t *)title_buf);

    char time_buf[16];
    Bsp_RTC_GetTimeString(time_buf, sizeof(time_buf));
    POINT_COLOR = YELLOW;
    BACK_COLOR  = DARKBLUE;
    LCD_ShowString(390, 10, 80, 16, 16, (uint8_t *)time_buf);

    // 2. 清空中部视窗 (36..749)
    LCD_Fill(0, 36, lcddev.width, 749, BLACK);

    // 3. 底部导航栏 (750..800)
    LCD_Fill(0, 750, lcddev.width, 800, 0x18E3); // 深灰蓝底

    // [< PREV] 按钮
    LCD_Fill(10, 755, 120, 795, BLUE);
    POINT_COLOR = WHITE;
    BACK_COLOR  = BLUE;
    LCD_ShowString(25, 767, 80, 16, 16, (uint8_t *)"< PREV");

    // 中间提示
    POINT_COLOR = YELLOW;
    BACK_COLOR  = 0x18E3;
    LCD_ShowString(155, 767, 180, 16, 16, (uint8_t *)"[WK_UP: Switch]");

    // [NEXT >] 按钮
    LCD_Fill(lcddev.width - 120, 755, lcddev.width - 10, 795, BLUE);
    POINT_COLOR = WHITE;
    BACK_COLOR  = BLUE;
    LCD_ShowString(lcddev.width - 105, 767, 80, 16, 16, (uint8_t *)"NEXT >");

    BACK_COLOR = BLACK;
}

void Page_Set(uint8_t idx) {
    if (idx >= PAGE_COUNT) idx = 0;
    s_current = idx;
    Bsp_UsbMouse_ResetBg();
    Draw_Common_Frames();
    if (s_pages[s_current].init) {
        s_pages[s_current].init();
    }
}

void Page_Next(void) {
    Page_Set((s_current + 1) % PAGE_COUNT);
}

void Page_Prev(void) {
    Page_Set((s_current + PAGE_COUNT - 1) % PAGE_COUNT);
}

void Page_Manager_Init(void) {
    s_storage_flash_ok = Bsp_SpiFlash_SelfTest(0x00FFF000, s_flash_msg, sizeof(s_flash_msg));
    s_storage_eeprom_ok = Bsp_At24c02_SelfTest(0x00, s_eeprom_msg, sizeof(s_eeprom_msg));
    uint32_t sram_err = 0;
    s_storage_sram_ok = Bsp_Sram_SelfTest(64 * 1024, &sram_err);
    s_storage_tested = 1;

    Bsp_Terminal_Init();

    s_current = 6;
    Page_Set(6);
}

void Page_Manager_Update(void) {
    s_uptime = xTaskGetTickCount() / 1000;

    static char s_last_time_str[16] = {0};
    char cur_time_str[16];
    Bsp_RTC_GetTimeString(cur_time_str, sizeof(cur_time_str));
    if (strcmp(cur_time_str, s_last_time_str) != 0) {
        strncpy(s_last_time_str, cur_time_str, sizeof(s_last_time_str));
        LCD_Lock();
        uint16_t old_pt = POINT_COLOR;
        uint16_t old_bg = BACK_COLOR;
        POINT_COLOR = YELLOW;
        BACK_COLOR  = DARKBLUE;
        LCD_ShowString(390, 10, 80, 16, 16, (uint8_t *)cur_time_str);
        POINT_COLOR = old_pt;
        BACK_COLOR  = old_bg;
        LCD_Unlock();
    }

    if (s_pages[s_current].update) {
        s_pages[s_current].update();
    }
}

void Page_Manager_OnKey(uint8_t key) {
    // WK_UP 作为全局硬件切页按键 (带 300ms 防抖)
    if (key == WKUP_PRES) {
        uint32_t now = xTaskGetTickCount();
        if ((now - s_last_switch_tick) >= pdMS_TO_TICKS(300)) {
            s_last_switch_tick = now;
            Page_Next();
        }
        return;
    }
    if (s_pages[s_current].on_key) {
        s_pages[s_current].on_key(key);
    }
}

void Page_Manager_OnTouch(uint16_t x, uint16_t y, uint8_t event) {
    // 底部导航栏触摸判定: 仅在 DOWN 瞬态且防抖冷却时间大于 300ms 时触发切页
    if (event == TOUCH_EVENT_DOWN && y >= 750) {
        uint32_t now = xTaskGetTickCount();
        if ((now - s_last_switch_tick) >= pdMS_TO_TICKS(300)) {
            s_last_switch_tick = now;
            if (x >= 10 && x <= 120) {
                Page_Prev();
                return;
            } else if (x >= (lcddev.width - 120) && x <= (lcddev.width - 10)) {
                Page_Next();
                return;
            }
        }
        return; // 导航栏区域不向子页面派发事件
    }
    // 中部视窗事件派发给当前页面
    if (s_pages[s_current].on_touch) {
        s_pages[s_current].on_touch(x, y, event);
    }
}
