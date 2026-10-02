#include "bsp_terminal.h"
#include "lcd.h"
#include "bsp_backlight.h"
#include "bsp_key.h"
#include "bsp_fs_manager.h"
#include "bsp_beep.h"
#include "bsp_lsens.h"
#include "bsp_cpu_temp.h"
#include "bsp_remote.h"
#include "bsp_sram.h"
#include "bsp_wm8978.h"
#include "bsp_usb_mouse.h"
#include "bsp_rtc.h"
#include "bsp_rng.h"
#include "bsp_dac.h"
#include "bsp_rs485.h"
#include "bsp_rs232.h"
#include "bsp_iwdg.h"
#include "bsp_lan8720.h"
#include "page_manager.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 终端视窗几何参数
#define WIN_X_START     10
#define WIN_Y_START     46
#define WIN_X_END       470
#define WIN_Y_END       432
#define WIN_PROMPT_Y    436

#define LINE_HEIGHT     18
#define MAX_DISP_LINES  20

static char s_lines[TERM_MAX_LINES][TERM_LINE_LEN];
static uint8_t s_line_count = 0;

static char s_cmd_buf[TERM_CMD_MAX_LEN];
static uint8_t s_cmd_len = 0;
static uint8_t s_cursor_blink = 0;

// 键盘模式: 0 = ABC, 1 = 123/SYM
static uint8_t s_kbd_mode = 0;

// 快捷药丸按钮定义
typedef struct {
    uint16_t x1, y1, x2, y2;
    const char *label;
    const char *cmd;
} ToolBtn_t;

static const ToolBtn_t s_tool_btns[] = {
    {14,  466, 84,  496, "help",  "help"},
    {90,  466, 160, 496, "ps",    "ps"},
    {166, 466, 236, 496, "free",  "free"},
    {242, 466, 312, 496, "ls",    "ls"},
    {318, 466, 388, 496, "clear", "clear"},
    {394, 466, 466, 496, "reset", "reboot"},
};
#define TOOL_BTN_COUNT (sizeof(s_tool_btns)/sizeof(s_tool_btns[0]))

// 键盘按键结构
typedef struct {
    uint16_t x1, y1, x2, y2;
    const char *label;
    char char_code; // 0 表示特殊功能键
    uint8_t action; // 1: BKSP, 2: ENTER, 3: TOGGLE_MODE, 4: SPACE, 0: 普通字符
} KeyBtn_t;

static KeyBtn_t s_keys[48];
static uint8_t s_key_count = 0;

static void Refresh_Terminal_Text(void);
static void Refresh_Prompt_Line(void);
static void Build_Keyboard_Layout(void);
static void Draw_Keyboard(void);

void Bsp_Terminal_Init(void) {
    memset(s_lines, 0, sizeof(s_lines));
    s_line_count = 0;
    memset(s_cmd_buf, 0, sizeof(s_cmd_buf));
    s_cmd_len = 0;
    s_kbd_mode = 0;

    // 默认输出启动横幅
    Bsp_Terminal_Print("VisionOS v2.4 Interactive Shell");
    Bsp_Terminal_Print("STM32F407ZGT6 Cortex-M4 @ 168MHz");
    Bsp_Terminal_Print("Type 'help' or tap pills to run.");
}

static void Push_Line(const char *line) {
    if (s_line_count < TERM_MAX_LINES) {
        snprintf(s_lines[s_line_count], TERM_LINE_LEN, "%s", line);
        s_line_count++;
    } else {
        for (uint8_t i = 1; i < TERM_MAX_LINES; i++) {
            memcpy(s_lines[i - 1], s_lines[i], TERM_LINE_LEN);
        }
        snprintf(s_lines[TERM_MAX_LINES - 1], TERM_LINE_LEN, "%s", line);
    }
}

static void Term_SendByte(uint8_t ch) {
    uint32_t timeout = 50000;
    while (!(USART1->SR & USART_SR_TXE) && timeout--) {}
    USART1->DR = ch;
}

void Bsp_Terminal_Print(const char *str) {
    if (!str) return;

    // 1. 直接写 USART1 硬件寄存器向电脑回显，彻底绕过 HAL 库状态锁
    const char *s = str;
    while (*s) {
        Term_SendByte((uint8_t)*s++);
    }
    Term_SendByte('\r');
    Term_SendByte('\n');

    // 2. 若 USART3 (RS232) 也使能，同步回显
    if (USART3->CR1 & USART_CR1_UE) {
        const char *s3 = str;
        while (*s3) {
            uint32_t t = 50000;
            while (!(USART3->SR & USART_SR_TXE) && t--) {}
            USART3->DR = (uint8_t)*s3++;
        }
        uint32_t t = 50000;
        while (!(USART3->SR & USART_SR_TXE) && t--) {}
        USART3->DR = '\r';
        t = 50000;
        while (!(USART3->SR & USART_SR_TXE) && t--) {}
        USART3->DR = '\n';
    }

    const char *p = str;
    char line_buf[TERM_LINE_LEN];
    uint8_t col = 0;

    while (*p) {
        if (*p == '\r') {
            p++;
            continue;
        }
        if (*p == '\t') {
            uint8_t spaces = 4 - (col % 4);
            if (spaces == 0) spaces = 4;
            while (spaces-- && col < (TERM_LINE_LEN - 1)) {
                line_buf[col++] = ' ';
            }
            p++;
            continue;
        }
        if (*p == '\n') {
            line_buf[col] = '\0';
            Push_Line(line_buf);
            col = 0;
            p++;
            continue;
        }

        line_buf[col++] = *p++;
        if (col >= (TERM_LINE_LEN - 1)) {
            line_buf[col] = '\0';
            Push_Line(line_buf);
            col = 0;
        }
    }

    if (col > 0) {
        line_buf[col] = '\0';
        Push_Line(line_buf);
    }
}

static void Refresh_Terminal_Text(void) {
    // 清除文本区背景
    LCD_Fill(WIN_X_START + 1, WIN_Y_START + 1, WIN_X_END - 1, WIN_Y_END - 1, BLACK);

    POINT_COLOR = GREEN;
    BACK_COLOR  = BLACK;
    for (uint8_t i = 0; i < s_line_count; i++) {
        uint16_t cur_y = WIN_Y_START + 4 + i * LINE_HEIGHT;
        if (cur_y + 16 > WIN_Y_END) break;
        LCD_ShowString(WIN_X_START + 4, cur_y, WIN_X_END - WIN_X_START - 8, 16, 16, (uint8_t *)s_lines[i]);
    }
}

static void Refresh_Prompt_Line(void) {
    // 局域清除输入行
    LCD_Fill(WIN_X_START + 1, WIN_PROMPT_Y, WIN_X_END - 1, WIN_PROMPT_Y + 18, BLACK);

    char prompt[TERM_CMD_MAX_LEN + 8];
    snprintf(prompt, sizeof(prompt), "$ %s%s", s_cmd_buf, s_cursor_blink ? "_" : " ");

    POINT_COLOR = YELLOW;
    BACK_COLOR  = BLACK;
    LCD_ShowString(WIN_X_START + 4, WIN_PROMPT_Y + 1, WIN_X_END - WIN_X_START - 8, 16, 16, (uint8_t *)prompt);
}

static void Build_Keyboard_Layout(void) {
    s_key_count = 0;
    uint16_t row1_y = 508;
    uint16_t row2_y = 554;
    uint16_t row3_y = 600;
    uint16_t row4_y = 646;
    uint16_t row5_y = 692;

    if (s_kbd_mode == 0) {
        // --- 字母模式 ---
        // Row 1: q w e r t y u i o p (10 键)
        const char *r1[] = {"q","w","e","r","t","y","u","i","o","p"};
        for (int i = 0; i < 10; i++) {
            s_keys[s_key_count++] = (KeyBtn_t){
                (uint16_t)(13 + i * 46), row1_y, (uint16_t)(13 + i * 46 + 40), (uint16_t)(row1_y + 40),
                r1[i], r1[i][0], 0
            };
        }
        // Row 2: a s d f g h j k l (9 键)
        const char *r2[] = {"a","s","d","f","g","h","j","k","l"};
        for (int i = 0; i < 9; i++) {
            s_keys[s_key_count++] = (KeyBtn_t){
                (uint16_t)(36 + i * 46), row2_y, (uint16_t)(36 + i * 46 + 40), (uint16_t)(row2_y + 40),
                r2[i], r2[i][0], 0
            };
        }
        // Row 3: z x c v b n m (7 键) + BKSP
        const char *r3[] = {"z","x","c","v","b","n","m"};
        for (int i = 0; i < 7; i++) {
            s_keys[s_key_count++] = (KeyBtn_t){
                (uint16_t)(13 + i * 46), row3_y, (uint16_t)(13 + i * 46 + 40), (uint16_t)(row3_y + 40),
                r3[i], r3[i][0], 0
            };
        }
        // BKSP 键
        s_keys[s_key_count++] = (KeyBtn_t){
            335, row3_y, 467, (uint16_t)(row3_y + 40),
            "BKSP", 0, 1
        };

        // Row 4: [123] + [SPACE] + . + / + [ENTER]
        s_keys[s_key_count++] = (KeyBtn_t){13, row4_y, 83, (uint16_t)(row4_y + 40), "123", 0, 3};
        s_keys[s_key_count++] = (KeyBtn_t){89, row4_y, 259, (uint16_t)(row4_y + 40), "SPACE", ' ', 4};
        s_keys[s_key_count++] = (KeyBtn_t){265, row4_y, 305, (uint16_t)(row4_y + 40), ".", '.', 0};
        s_keys[s_key_count++] = (KeyBtn_t){311, row4_y, 351, (uint16_t)(row4_y + 40), "/", '/', 0};
        s_keys[s_key_count++] = (KeyBtn_t){357, row4_y, 467, (uint16_t)(row4_y + 40), "ENTER", 0, 2};

        // Row 5: 常用符号快捷栏 (10 键: 0 1 2 3 4 5 - _ : ~)
        const char *r5[] = {"0","1","2","3","4","5","-","_","~",":"};
        for (int i = 0; i < 10; i++) {
            s_keys[s_key_count++] = (KeyBtn_t){
                (uint16_t)(13 + i * 46), row5_y, (uint16_t)(13 + i * 46 + 40), (uint16_t)(row5_y + 40),
                r5[i], r5[i][0], 0
            };
        }
    } else {
        // --- 数字/符号模式 ---
        // Row 1: 1 2 3 4 5 6 7 8 9 0
        const char *r1[] = {"1","2","3","4","5","6","7","8","9","0"};
        for (int i = 0; i < 10; i++) {
            s_keys[s_key_count++] = (KeyBtn_t){
                (uint16_t)(13 + i * 46), row1_y, (uint16_t)(13 + i * 46 + 40), (uint16_t)(row1_y + 40),
                r1[i], r1[i][0], 0
            };
        }
        // Row 2: - + = * / \ ( ) &
        const char *r2[] = {"-","+","=","*","/","\\","(",")","&"};
        for (int i = 0; i < 9; i++) {
            s_keys[s_key_count++] = (KeyBtn_t){
                (uint16_t)(36 + i * 46), row2_y, (uint16_t)(36 + i * 46 + 40), (uint16_t)(row2_y + 40),
                r2[i], r2[i][0], 0
            };
        }
        // Row 3: ! @ # $ % ^ ? + BKSP
        const char *r3[] = {"!","@","#","$","%","^","?"};
        for (int i = 0; i < 7; i++) {
            s_keys[s_key_count++] = (KeyBtn_t){
                (uint16_t)(13 + i * 46), row3_y, (uint16_t)(13 + i * 46 + 40), (uint16_t)(row3_y + 40),
                r3[i], r3[i][0], 0
            };
        }
        s_keys[s_key_count++] = (KeyBtn_t){
            335, row3_y, 467, (uint16_t)(row3_y + 40),
            "BKSP", 0, 1
        };

        // Row 4: [ABC] + [SPACE] + . + : + [ENTER]
        s_keys[s_key_count++] = (KeyBtn_t){13, row4_y, 83, (uint16_t)(row4_y + 40), "ABC", 0, 3};
        s_keys[s_key_count++] = (KeyBtn_t){89, row4_y, 259, (uint16_t)(row4_y + 40), "SPACE", ' ', 4};
        s_keys[s_key_count++] = (KeyBtn_t){265, row4_y, 305, (uint16_t)(row4_y + 40), ".", '.', 0};
        s_keys[s_key_count++] = (KeyBtn_t){311, row4_y, 351, (uint16_t)(row4_y + 40), ":", ':', 0};
        s_keys[s_key_count++] = (KeyBtn_t){357, row4_y, 467, (uint16_t)(row4_y + 40), "ENTER", 0, 2};

        // Row 5: 6 7 8 9 < > [ ] { }
        const char *r5[] = {"6","7","8","9","<",">","[","]","{","}"};
        for (int i = 0; i < 10; i++) {
            s_keys[s_key_count++] = (KeyBtn_t){
                (uint16_t)(13 + i * 46), row5_y, (uint16_t)(13 + i * 46 + 40), (uint16_t)(row5_y + 40),
                r5[i], r5[i][0], 0
            };
        }
    }
}

static void Draw_Keyboard(void) {
    Build_Keyboard_Layout();

    // 软键盘背景
    LCD_Fill(10, 504, 470, 742, 0x18C3); // 深沉背景灰

    for (uint8_t i = 0; i < s_key_count; i++) {
        KeyBtn_t *k = &s_keys[i];
        uint16_t bg = DARKBLUE;
        uint16_t fg = WHITE;

        if (k->action == 1) { // BKSP
            bg = 0x8000;      // 深红
        } else if (k->action == 2) { // ENTER
            bg = 0x03E0;      // 纯深绿
        } else if (k->action == 3) { // 123/ABC
            bg = 0x4208;      // 灰色
        }

        LCD_Fill(k->x1, k->y1, k->x2, k->y2, bg);
        POINT_COLOR = 0x7BEF; // 浅灰边框
        LCD_DrawRectangle(k->x1, k->y1, k->x2, k->y2);

        // 文字居中
        uint16_t str_len = strlen(k->label) * 8;
        uint16_t btn_w = k->x2 - k->x1;
        uint16_t off_x = (btn_w > str_len) ? (btn_w - str_len) / 2 : 2;
        uint16_t off_y = (k->y2 - k->y1 - 16) / 2;

        POINT_COLOR = fg;
        BACK_COLOR  = bg;
        LCD_ShowString(k->x1 + off_x, k->y1 + off_y, str_len + 4, 16, 16, (uint8_t *)k->label);
    }
}

void Bsp_Terminal_DrawView(void) {
    // 1. 终端视窗外框与标题
    POINT_COLOR = 0x39E7; // 终端边框暗青
    LCD_DrawRectangle(WIN_X_START, WIN_Y_START, WIN_X_END, WIN_PROMPT_Y + 22);
    LCD_DrawLine(WIN_X_START, WIN_Y_END, WIN_X_END, WIN_Y_END);

    // 2. 刷新文本内容与提示符
    Refresh_Terminal_Text();
    Refresh_Prompt_Line();

    // 3. 绘制快捷药丸按钮栏
    for (uint8_t i = 0; i < TOOL_BTN_COUNT; i++) {
        const ToolBtn_t *b = &s_tool_btns[i];
        LCD_Fill(b->x1, b->y1, b->x2, b->y2, 0x2124); // 深蓝灰药丸底色
        POINT_COLOR = CYAN;
        LCD_DrawRectangle(b->x1, b->y1, b->x2, b->y2);

        uint16_t str_len = strlen(b->label) * 8;
        uint16_t off_x = (b->x2 - b->x1 - str_len) / 2;
        uint16_t off_y = (b->y2 - b->y1 - 16) / 2;
        POINT_COLOR = YELLOW;
        BACK_COLOR  = 0x2124;
        LCD_ShowString(b->x1 + off_x, b->y1 + off_y, str_len + 2, 16, 16, (uint8_t *)b->label);
    }

    // 4. 绘制虚拟键盘
    Draw_Keyboard();
}

void Bsp_Terminal_Update(void) {
    static uint32_t last_blink = 0;
    uint32_t now = xTaskGetTickCount();
    if (now - last_blink >= pdMS_TO_TICKS(500)) {
        last_blink = now;
        s_cursor_blink = !s_cursor_blink;
        Refresh_Prompt_Line();
    }
}

void Bsp_Terminal_ExecuteCommand(const char *cmd_line) {
    if (!cmd_line || strlen(cmd_line) == 0) return;

    // 先回显执行的命令
    char echo[TERM_CMD_MAX_LEN + 4];
    snprintf(echo, sizeof(echo), "$ %s", cmd_line);
    Bsp_Terminal_Print(echo);

    // 解析命令
    char buf[TERM_CMD_MAX_LEN];
    strncpy(buf, cmd_line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    // 去除前导空格
    char *p = buf;
    while (*p == ' ') p++;
    if (*p == '\0') {
        Refresh_Terminal_Text();
        return;
    }

    char *cmd = p;
    char *arg = NULL;
    char *space = strchr(p, ' ');
    if (space) {
        *space = '\0';
        arg = space + 1;
        while (*arg == ' ') arg++;
    }

    if (strcmp(cmd, "help") == 0) {
        Bsp_Terminal_Print("Supported Commands:");
        Bsp_Terminal_Print(" ps       - List FreeRTOS tasks & stack");
        Bsp_Terminal_Print(" free     - Query dynamic heap memory");
        Bsp_Terminal_Print(" ls       - List Flash/SD files");
        Bsp_Terminal_Print(" cat <f>  - Read and display file");
        Bsp_Terminal_Print(" uptime   - Print system uptime");
        Bsp_Terminal_Print(" sys      - Print system & core clock");
        Bsp_Terminal_Print(" beep [ms]- Beep buzzer on PF8");
        Bsp_Terminal_Print(" light    - Read PF7 light sensor");
        Bsp_Terminal_Print(" temp     - Read CPU junction temp");
        Bsp_Terminal_Print(" ir       - Poll PG11 IR remote");
        Bsp_Terminal_Print(" sram     - Test 1024KB external SRAM");
        Bsp_Terminal_Print(" audio [f]- Play tone to headset (WM8978)");
        Bsp_Terminal_Print(" mouse    - USB Host mouse coordinates");
        Bsp_Terminal_Print(" bl <val> - Set backlight (5..100)");
        Bsp_Terminal_Print(" time [h m s] - Query / set RTC time");
        Bsp_Terminal_Print(" date [y m d] - Query / set RTC date");
        Bsp_Terminal_Print(" rand [max]   - Hardware RNG true random");
        Bsp_Terminal_Print(" dac <mv>     - Set PA4 DAC voltage (0-3300)");
        Bsp_Terminal_Print(" rs485 [msg]  - Send/read SP3485 (PA2/PA3 PG8)");
        Bsp_Terminal_Print(" rs232 [msg]  - Send/read SP3232 (PB10/PB11)");
        Bsp_Terminal_Print(" wdt [on/halt]- IWDG independent watchdog");
        Bsp_Terminal_Print(" eth          - Probe LAN8720A PHY & link");
        Bsp_Terminal_Print(" whoami       - MCU & system identity");
        Bsp_Terminal_Print(" clear        - Clean terminal buffer");
        Bsp_Terminal_Print(" reboot       - Software NVIC reset");
    } else if (strcmp(cmd, "ps") == 0) {
        static char task_list_buf[384];
        Bsp_Terminal_Print("Name          State Prio Stack Num");
        vTaskList(task_list_buf);
        Bsp_Terminal_Print(task_list_buf);
    } else if (strcmp(cmd, "free") == 0) {
        char free_msg[64];
        snprintf(free_msg, sizeof(free_msg), "Heap Free : %u B (Min %u B)",
                 (unsigned int)xPortGetFreeHeapSize(),
                 (unsigned int)xPortGetMinimumEverFreeHeapSize());
        Bsp_Terminal_Print(free_msg);
    } else if (strcmp(cmd, "ls") == 0) {
        const Fs_Explorer_State_t *st = Bsp_FsManager_GetState(1); // 默认 Flash 卷
        if (st && st->is_mounted) {
            char ls_hdr[64];
            snprintf(ls_hdr, sizeof(ls_hdr), "[Drive 1: Flash FAT (%u files)]", st->file_count);
            Bsp_Terminal_Print(ls_hdr);
            for (uint8_t i = 0; i < st->file_count && i < 10; i++) {
                char item[64];
                snprintf(item, sizeof(item), "  %-12s %lu B", st->files[i].name, st->files[i].size);
                Bsp_Terminal_Print(item);
            }
        } else {
            Bsp_Terminal_Print("Flash Drive 1: not mounted");
        }
    } else if (strcmp(cmd, "cat") == 0) {
        if (!arg || strlen(arg) == 0) {
            Bsp_Terminal_Print("Usage: cat <filename>");
        } else {
            char path[48];
            snprintf(path, sizeof(path), "1:%s", arg);
            FIL f;
            if (f_open(&f, path, FA_READ) == FR_OK) {
                char fbuf[96];
                UINT br = 0;
                f_read(&f, fbuf, sizeof(fbuf) - 1, &br);
                fbuf[br] = '\0';
                f_close(&f);
                Bsp_Terminal_Print(fbuf);
            } else {
                Bsp_Terminal_Print("Error: file not found");
            }
        }
    } else if (strcmp(cmd, "uptime") == 0) {
        uint32_t t = xTaskGetTickCount() / 1000;
        char ut_buf[64];
        snprintf(ut_buf, sizeof(ut_buf), "Uptime: %02lu:%02lu:%02lu (%lu s)",
                 t / 3600, (t % 3600) / 60, t % 60, t);
        Bsp_Terminal_Print(ut_buf);
    } else if (strcmp(cmd, "beep") == 0) {
        int ms = arg ? atoi(arg) : 50;
        if (ms < 10) ms = 10;
        if (ms > 2000) ms = 2000;
        Bsp_Beep_Tone(2500, (uint16_t)ms);
        char bmsg[48];
        snprintf(bmsg, sizeof(bmsg), "Buzzer sounded (%d ms @ 2.5kHz)", ms);
        Bsp_Terminal_Print(bmsg);
    } else if (strcmp(cmd, "light") == 0) {
        uint16_t raw = Bsp_Lsens_ReadRaw();
        uint8_t pct = Bsp_Lsens_ReadPercent();
        char lbuf[64];
        snprintf(lbuf, sizeof(lbuf), "Light: %u%% (PF7 raw ADC: %u)", pct, raw);
        Bsp_Terminal_Print(lbuf);
    } else if (strcmp(cmd, "temp") == 0) {
        float c = Bsp_CpuTemp_ReadCelsius();
        char tbuf[64];
        snprintf(tbuf, sizeof(tbuf), "CPU Temp: %.1f C (ADC1_IN16)", (double)c);
        Bsp_Terminal_Print(tbuf);
    } else if (strcmp(cmd, "ir") == 0) {
        uint8_t addr = 0, key_cmd = 0;
        if (Bsp_Remote_Scan(&addr, &key_cmd)) {
            char ir_buf[64];
            snprintf(ir_buf, sizeof(ir_buf), "IR Key: 0x%02X (%s) Addr:0x%02X",
                     key_cmd, Bsp_Remote_GetKeyName(key_cmd), addr);
            Bsp_Terminal_Print(ir_buf);
        } else {
            Bsp_Terminal_Print("PG11: Idle (press remote key to test)");
        }
    } else if (strcmp(cmd, "sram") == 0) {
        Bsp_Terminal_Print("Testing 1024 KiB onboard SRAM...");
        uint32_t t_start = xTaskGetTickCount();
        uint32_t errs = 0;
        uint8_t ok = Bsp_Sram_SelfTest(1024 * 1024, &errs);
        uint32_t t_cost = xTaskGetTickCount() - t_start;
        char smsg[64];
        if (ok) {
            snprintf(smsg, sizeof(smsg), "SRAM: 1024 KiB OK (0x68000000, %lu ms)", t_cost);
        } else {
            snprintf(smsg, sizeof(smsg), "SRAM: FAIL (%lu errors at 0x68000000)", errs);
        }
        Bsp_Terminal_Print(smsg);
    } else if (strcmp(cmd, "play") == 0 || strcmp(cmd, "audio") == 0) {
        if (!Bsp_WM8978_IsOk()) {
            Bsp_Terminal_Print("WM8978: Not ready / I2C failed");
        } else {
            uint16_t freq = 1000;
            uint32_t dur = 500;
            if (arg && strlen(arg) > 0) {
                int f = atoi(arg);
                if (f >= 100 && f <= 10000) freq = (uint16_t)f;
            }
            char amsg[64];
            snprintf(amsg, sizeof(amsg), "Audio: %u Hz (%lu ms) -> Headset", freq, dur);
            Bsp_Terminal_Print(amsg);
            Bsp_WM8978_PlayTone(freq, dur);
        }
    } else if (strcmp(cmd, "mouse") == 0) {
        const Bsp_UsbMouse_State_t *m = Bsp_UsbMouse_GetState();
        char mmsg[80];
        snprintf(mmsg, sizeof(mmsg), "Mouse: %s (%d,%d) L:%d R:%d Pkts:%lu",
                 m->is_connected ? "Online" : "Offline",
                 m->x, m->y, m->btn_left, m->btn_right, m->packet_count);
        Bsp_Terminal_Print(mmsg);
    } else if (strcmp(cmd, "bl") == 0) {
        if (arg) {
            int val = atoi(arg);
            if (val >= 5 && val <= 100) {
                Bsp_Backlight_Set((uint8_t)val);
                char msg[48];
                snprintf(msg, sizeof(msg), "Backlight adjusted to %d%%", val);
                Bsp_Terminal_Print(msg);
            } else {
                Bsp_Terminal_Print("Range: 5..100");
            }
        } else {
            Bsp_Terminal_Print("Usage: bl <5..100>");
        }
    } else if (strcmp(cmd, "time") == 0) {
        if (arg && strlen(arg) > 0) {
            int h = 0, m = 0, s = 0;
            if (sscanf(arg, "%d %d %d", &h, &m, &s) >= 2) {
                if (Bsp_RTC_SetTime((uint8_t)h, (uint8_t)m, (uint8_t)s) == 0) {
                    Bsp_Terminal_Print("RTC Time updated");
                } else {
                    Bsp_Terminal_Print("Failed: invalid values");
                }
            } else {
                Bsp_Terminal_Print("Usage: time <h> <m> [s]");
            }
        } else {
            char tmsg[48];
            char tbuf[16];
            Bsp_RTC_GetTimeString(tbuf, sizeof(tbuf));
            snprintf(tmsg, sizeof(tmsg), "RTC Time: %s", tbuf);
            Bsp_Terminal_Print(tmsg);
        }
    } else if (strcmp(cmd, "date") == 0) {
        if (arg && strlen(arg) > 0) {
            int y = 0, m = 0, d = 0, w = 1;
            if (sscanf(arg, "%d %d %d %d", &y, &m, &d, &w) >= 3) {
                if (Bsp_RTC_SetDate((uint8_t)y, (uint8_t)m, (uint8_t)d, (uint8_t)w) == 0) {
                    Bsp_Terminal_Print("RTC Date updated");
                } else {
                    Bsp_Terminal_Print("Failed: invalid values");
                }
            } else {
                Bsp_Terminal_Print("Usage: date <yy> <mm> <dd> [week:1..7]");
            }
        } else {
            char dmsg[48];
            char dbuf[24];
            Bsp_RTC_GetDateString(dbuf, sizeof(dbuf));
            snprintf(dmsg, sizeof(dmsg), "RTC Date: %s", dbuf);
            Bsp_Terminal_Print(dmsg);
        }
    } else if (strcmp(cmd, "rand") == 0) {
        if (arg && strlen(arg) > 0) {
            int max_val = atoi(arg);
            if (max_val > 0) {
                int32_t val = Bsp_RNG_GetRange(0, max_val);
                char rmsg[48];
                snprintf(rmsg, sizeof(rmsg), "RNG [0..%d] : %ld", max_val, (long)val);
                Bsp_Terminal_Print(rmsg);
            } else {
                Bsp_Terminal_Print("Usage: rand [max]");
            }
        } else {
            uint32_t raw = Bsp_RNG_Get();
            char rmsg[64];
            snprintf(rmsg, sizeof(rmsg), "RNG: 0x%08lX (%lu)", (unsigned long)raw, (unsigned long)raw);
            Bsp_Terminal_Print(rmsg);
        }
    } else if (strcmp(cmd, "dac") == 0) {
        if (arg && strlen(arg) > 0) {
            int mv = atoi(arg);
            if (mv >= 0 && mv <= 3300) {
                Bsp_DAC_SetVoltage((uint16_t)mv);
                char dmsg[48];
                snprintf(dmsg, sizeof(dmsg), "DAC PA4 set to %d mV", mv);
                Bsp_Terminal_Print(dmsg);
            } else {
                Bsp_Terminal_Print("Range: 0..3300 mV");
            }
        } else {
            char dmsg[64];
            snprintf(dmsg, sizeof(dmsg), "DAC PA4 Out: %u mV (Usage: dac <0..3300>)", Bsp_DAC_GetVoltage());
            Bsp_Terminal_Print(dmsg);
        }
    } else if (strcmp(cmd, "rs485") == 0) {
        if (arg && strlen(arg) > 0) {
            Bsp_RS485_SendString(arg);
            Bsp_RS485_SendString("\r\n");
            char msg[64];
            snprintf(msg, sizeof(msg), "RS485 TX: '%s'", arg);
            Bsp_Terminal_Print(msg);
        } else {
            if (Bsp_RS485_Available()) {
                char rx_data[64] = {0};
                uint16_t n = Bsp_RS485_Receive((uint8_t *)rx_data, sizeof(rx_data) - 1);
                char msg[80];
                snprintf(msg, sizeof(msg), "RS485 RX (%u B): %s", n, rx_data);
                Bsp_Terminal_Print(msg);
            } else {
                Bsp_Terminal_Print("RS485: Ready (Usage: rs485 <msg>)");
            }
        }
    } else if (strcmp(cmd, "rs232") == 0) {
        if (arg && strlen(arg) > 0) {
            Bsp_RS232_SendString(arg);
            Bsp_RS232_SendString("\r\n");
            char msg[64];
            snprintf(msg, sizeof(msg), "RS232 TX: '%s'", arg);
            Bsp_Terminal_Print(msg);
        } else {
            if (Bsp_RS232_Available()) {
                char rx_data[64] = {0};
                uint16_t n = Bsp_RS232_Receive((uint8_t *)rx_data, sizeof(rx_data) - 1);
                char msg[80];
                snprintf(msg, sizeof(msg), "RS232 RX (%u B): %s", n, rx_data);
                Bsp_Terminal_Print(msg);
            } else {
                Bsp_Terminal_Print("RS232: Ready (Usage: rs232 <msg>)");
            }
        }
    } else if (strcmp(cmd, "wdt") == 0) {
        if (arg && strcmp(arg, "on") == 0) {
            if (!Bsp_IWDG_IsEnabled()) {
                Bsp_IWDG_Init(2000);
                Bsp_Terminal_Print("IWDG: Started (Timeout 2000ms)");
            } else {
                Bsp_Terminal_Print("IWDG: Already Running");
            }
        } else if (arg && strcmp(arg, "halt") == 0) {
            Bsp_Terminal_Print("IWDG Halt: loop forever to trigger reset...");
            Refresh_Terminal_Text();
            taskDISABLE_INTERRUPTS();
            while (1);
        } else {
            char wmsg[64];
            snprintf(wmsg, sizeof(wmsg), "IWDG Status: %s (Usage: wdt on/halt)",
                     Bsp_IWDG_IsEnabled() ? "ACTIVE (Feeding)" : "OFF");
            Bsp_Terminal_Print(wmsg);
        }
    } else if (strcmp(cmd, "eth") == 0) {
        uint16_t id1 = 0, id2 = 0;
        uint8_t res = Bsp_LAN8720_Probe(&id1, &id2);
        char emsg[64];
        if (res == 0) {
            uint8_t link = Bsp_LAN8720_GetLinkStatus();
            snprintf(emsg, sizeof(emsg), "LAN8720: Detected (ID:0x%04X, Link:%s)", id1, link ? "UP" : "DOWN");
        } else {
            snprintf(emsg, sizeof(emsg), "LAN8720: Probe FAIL (ID: 0x%04X 0x%04X)", id1, id2);
        }
        Bsp_Terminal_Print(emsg);
    } else if (strcmp(cmd, "whoami") == 0) {
        Bsp_Terminal_Print("Operator: VisionCraft Console");
        Bsp_Terminal_Print("Chip: STM32F407ZGT6 @ 168MHz");
    } else if (strcmp(cmd, "sys") == 0) {
        char sbuf[64];
        snprintf(sbuf, sizeof(sbuf), "CPU: STM32F407ZGT6 @ %lu MHz", SystemCoreClock / 1000000);
        Bsp_Terminal_Print(sbuf);
        snprintf(sbuf, sizeof(sbuf), "Tick: %lu | Kernel: FreeRTOS 10.5.1", xTaskGetTickCount());
        Bsp_Terminal_Print(sbuf);
    } else if (strcmp(cmd, "clear") == 0) {
        memset(s_lines, 0, sizeof(s_lines));
        s_line_count = 0;
    } else if (strcmp(cmd, "reboot") == 0) {
        Bsp_Terminal_Print("Rebooting system...");
        Refresh_Terminal_Text();
        vTaskDelay(pdMS_TO_TICKS(300));
        NVIC_SystemReset();
    } else {
        char err[48];
        snprintf(err, sizeof(err), "Unknown cmd: '%.25s'", cmd);
        Bsp_Terminal_Print(err);
    }

    Refresh_Terminal_Text();
}

void Bsp_Terminal_OnTouch(uint16_t x, uint16_t y, uint8_t event) {
    if (event != TOUCH_EVENT_DOWN) return;

    // 1. 检查快捷药丸按钮栏 (y: 466 ~ 496)
    if (y >= 466 && y <= 496) {
        for (uint8_t i = 0; i < TOOL_BTN_COUNT; i++) {
            const ToolBtn_t *b = &s_tool_btns[i];
            if (x >= b->x1 && x <= b->x2) {
                Bsp_Beep_Click();
                // 点击快捷按钮，直接执行该命令
                Bsp_Terminal_ExecuteCommand(b->cmd);
                return;
            }
        }
    }

    // 2. 检查虚拟键盘按键 (y: 504 ~ 742)
    if (y >= 504 && y <= 742) {
        for (uint8_t i = 0; i < s_key_count; i++) {
            KeyBtn_t *k = &s_keys[i];
            if (x >= k->x1 && x <= k->x2 && y >= k->y1 && y <= k->y2) {
                Bsp_Beep_Click();
                // 命中按键
                if (k->action == 1) { // BKSP
                    if (s_cmd_len > 0) {
                        s_cmd_len--;
                        s_cmd_buf[s_cmd_len] = '\0';
                        Refresh_Prompt_Line();
                    }
                } else if (k->action == 2) { // ENTER
                    if (s_cmd_len > 0) {
                        char tmp[TERM_CMD_MAX_LEN];
                        strncpy(tmp, s_cmd_buf, sizeof(tmp) - 1);
                        tmp[sizeof(tmp) - 1] = '\0';
                        s_cmd_len = 0;
                        s_cmd_buf[0] = '\0';
                        Refresh_Prompt_Line();
                        Bsp_Terminal_ExecuteCommand(tmp);
                    }
                } else if (k->action == 3) { // 切换模式
                    s_kbd_mode = !s_kbd_mode;
                    Draw_Keyboard();
                } else if (k->action == 4) { // SPACE
                    if (s_cmd_len < (TERM_CMD_MAX_LEN - 1)) {
                        s_cmd_buf[s_cmd_len++] = ' ';
                        s_cmd_buf[s_cmd_len] = '\0';
                        Refresh_Prompt_Line();
                    }
                } else { // 普通字符
                    if (s_cmd_len < (TERM_CMD_MAX_LEN - 1)) {
                        s_cmd_buf[s_cmd_len++] = k->char_code;
                        s_cmd_buf[s_cmd_len] = '\0';
                        Refresh_Prompt_Line();
                    }
                }
                return;
            }
        }
    }
}

void Bsp_Terminal_OnKey(uint8_t key) {
    // 实体按键映射快捷执行
    if (key == KEY0_PRES) {
        Bsp_Terminal_ExecuteCommand("help");
    } else if (key == KEY1_PRES) {
        Bsp_Terminal_ExecuteCommand("ps");
    } else if (key == KEY2_PRES) {
        Bsp_Terminal_ExecuteCommand("free");
    }
}
