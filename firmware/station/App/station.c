/* StationTask：扫描按键（上报事件）、控制蜂鸣器 */
#include "app.h"

#include "main.h"
#include "tim.h"
#include "cmsis_os2.h"
#include "vc_protocol.h"
#include "lcd.h"
#include "pages.h"
#include "touch.h"
#include "mouse.h"
#include "cursor.h"

extern osMessageQueueId_t g_beepQueue;

// [region keys]
typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    GPIO_PinState pressedLevel;   /* KEY0~2 按下为低电平，WK_UP 按下为高电平 */
    uint8_t stable;               /* 消抖后的状态：1 = 按下 */
    uint8_t count;                /* 连续读到与 stable 不同的次数 */
} button;

static button s_keys[] = {
    {KEY0_GPIO_Port, KEY0_Pin, GPIO_PIN_RESET, 0, 0},
    {KEY1_GPIO_Port, KEY1_Pin, GPIO_PIN_RESET, 0, 0},
    {KEY2_GPIO_Port, KEY2_Pin, GPIO_PIN_RESET, 0, 0},
    {KEY_WKUP_GPIO_Port, KEY_WKUP_Pin, GPIO_PIN_SET, 0, 0},
};

#define SCAN_MS        10u
#define DEBOUNCE_COUNT 2u     /* 连续 2 次（20 ms）读到新状态才算数 */

static void scan_keys(void)
{
    static uint8_t seq;
    for (uint8_t i = 0; i < sizeof(s_keys) / sizeof(s_keys[0]); ++i) {
        button *k = &s_keys[i];
        const uint8_t now = HAL_GPIO_ReadPin(k->port, k->pin) == k->pressedLevel;
        if (now == k->stable) {
            k->count = 0;
            continue;
        }
        if (++k->count < DEBOUNCE_COUNT)
            continue;
        k->stable = now;
        k->count = 0;
        const uint8_t ev[2] = {i, now ? VC_KEY_DOWN : VC_KEY_UP};
        app_send(VC_EVT_KEY, seq++, ev, sizeof(ev));
        pages_key(i, now);
        if (now)
            app_beep(15);   /* 按键音 */
    }
}
// [endregion]

// [region exti]
/* KEY_WKUP（PA0）同时接了外部中断，上升沿、下降沿都触发。中断里只做计数和记时间，
 * 用来观察按键抖动：按一次键，中断触发几次、间隔多久？按键事件本身仍由上面每 10 ms 的扫描负责。
 * 这几个 volatile 全局变量调试器可以按名字直接读（tools/exti_probe.tcl）。 */
#define EDGE_LOG 16u
volatile uint32_t g_wkupEdges;                /* 中断触发的总次数 */
volatile uint32_t g_wkupEdgeCycles[EDGE_LOG]; /* 最近 16 次触发的时刻：CPU 周期计数（168 MHz，约 25 秒绕回一次） */

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    if (pin == KEY_WKUP_Pin) {
        g_wkupEdgeCycles[g_wkupEdges % EDGE_LOG] = DWT->CYCCNT;
        g_wkupEdges++;
    }
}

/* DWT 是内核里的调试计数单元，CYCCNT 每个 CPU 周期加 1。默认关着，要先打开 */
static void cycle_counter_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}
// [endregion]

/* 给界面框架计时用：DWT 周期计数器，168 MHz */
uint32_t gui_cycles(void) { return DWT->CYCCNT; }

// [region pointer]
/* 触摸 → 指针事件：手指刚接触是「按下」，之后每次读到坐标是「移动」，离开时在最后的位置「抬起」 */
static void poll_touch(void)
{
    static int touching;
    static int16_t lx, ly;
    uint16_t x, y;
    const int n = touch_read(&x, &y);
    if (n > 0 && x < LCD_W && y < LCD_H) {               /* 坐标越界的读数丢掉（实测出现过 65535） */
        const gui_event e = {touching ? GUI_MOVE : GUI_DOWN, (int16_t)x, (int16_t)y};
        touching = 1; lx = (int16_t)x; ly = (int16_t)y;
        gui_handle(&e);
    } else if (n == 0 && touching) {
        const gui_event e = {GUI_UP, lx, ly};
        touching = 0;
        gui_handle(&e);
    }
}

/* 鼠标 → 指针事件：报告里只有相对移动量，光标位置要自己累加；左键按下 / 抬起就是「按下」/「抬起」，按着移动是「移动」 */
static int16_t s_cx = LCD_W / 2, s_cy = LCD_H / 2;
static void poll_mouse(void)
{
    static int left;
    mouse_report r;
    while (mouse_take(&r)) {
        int x = s_cx + r.dx, y = s_cy + r.dy;
        s_cx = (int16_t)(x < 0 ? 0 : x >= LCD_W ? LCD_W - 1 : x);   /* 光标不能出屏 */
        s_cy = (int16_t)(y < 0 ? 0 : y >= LCD_H ? LCD_H - 1 : y);
        const int down = r.buttons & 1;
        if (down != left || (down && (r.dx || r.dy))) {
            const gui_event e = {down && !left ? GUI_DOWN : down ? GUI_MOVE : GUI_UP, s_cx, s_cy};
            gui_handle(&e);
        }
        left = down;
    }
}

/* 调试用的事件注入口：一个能放 8 个事件的环形队列。调试器往 ev[head % 8] 写 type、x、y，再把 head 加 1；
 * 固件每轮循环把 tail 追到 head。只有一个槽的话，固件忙着重画时（换页要一两百毫秒）后写的事件会覆盖先写的 */
#define INJECT_N 8
volatile struct { uint32_t head, tail; struct { int16_t type, x, y, pad; } ev[INJECT_N]; } g_gui_inject;
static void poll_inject(void)
{
    while (g_gui_inject.tail != g_gui_inject.head) {
        const uint32_t i = g_gui_inject.tail % INJECT_N;
        const gui_event e = {(gui_event_type)g_gui_inject.ev[i].type, g_gui_inject.ev[i].x, g_gui_inject.ev[i].y};
        g_gui_inject.tail++;
        gui_handle(&e);
    }
}
// [endregion]

// [region loop]
void StationTask(void *argument)
{
    (void)argument;
    uint32_t beepUntil = 0;
    int beeping = 0;
    uint32_t lastTouch = 0, lastGui = 0;
    cycle_counter_init();
    gui_init(&g_page_home);             /* 先把首页画出来，触摸芯片的复位要几百毫秒 */
    pages_init();                       /* LVGL 和它的页面：控件建好放在内存里，进入页面时才画 */
    touch_init();
    cursor_init();
    mouse_init();                       /* USB 主机库开始工作：鼠标的枚举、收报告都在库自己的任务里 */

    for (;;) {
        uint16_t ms;
        /* 等蜂鸣请求，最多等一个扫描周期——这样一个循环同时完成「响铃」和「扫键」 */
        if (osMessageQueueGet(g_beepQueue, &ms, NULL, SCAN_MS) == osOK) {
            if (!beeping)
                HAL_TIM_PWM_Start(&htim13, TIM_CHANNEL_1);   /* PF8 输出约 2.7 kHz 方波 */
            beeping = 1;
            beepUntil = HAL_GetTick() + ms;
        }
        if (beeping && (int32_t)(HAL_GetTick() - beepUntil) >= 0) {
            HAL_TIM_PWM_Stop(&htim13, TIM_CHANNEL_1);
            beeping = 0;
        }
        scan_keys();
        const uint32_t now = HAL_GetTick();
        if (now - lastTouch >= 20u) {   /* 触摸每 20 ms 读一次 */
            lastTouch = now;
            poll_touch();
        }
        poll_mouse();
        poll_inject();
        pages_background(now);          /* 取走上位机的结果 / 缩略图、记录传感器历史 */
        if (now - lastGui >= 20u) {     /* 页面刷新 + 重画变了的控件 */
            lastGui = now;
            gui_tick(now);
        }
        cursor_set(mouse_connected(), s_cx, s_cy);   /* 最后画光标：这一轮如果画到了它，它已经被收起，在这里画回来 */
    }
}
// [endregion]
