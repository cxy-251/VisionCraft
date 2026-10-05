#include "lv_port.h"

#include "lcd.h"
#include "lvgl.h"
#include "main.h"                           /* HAL_GetTick */

volatile uint32_t g_lv_flushes, g_lv_flush_px, g_lv_frame_cycles, g_lv_frame_px, g_lv_frame_flush_cycles;
static uint32_t s_flushCycles;              /* 写屏一共用了多少周期（累计） */
uint32_t gui_cycles(void);                  /* gui.c：板子上是 DWT 周期计数器，模拟器里是 0 */

#define BAR GUI_BAR_H                       /* 标题栏仍由界面框架画，LVGL 只管下面 480 × 724 */

// [region display]
/* 绘图缓冲：LVGL 先在这里画好一块（最多 16 行），再整块写进显存。整屏要 750 KB，放不下，所以分块画 */
#define BUF_LINES 16
static uint16_t s_buf[LCD_W * BUF_LINES] LV_ATTRIBUTE_LARGE_RAM_ARRAY;

static void flush(lv_display_t *disp, const lv_area_t *a, uint8_t *px)
{
    const uint16_t w = (uint16_t)lv_area_get_width(a), h = (uint16_t)lv_area_get_height(a);
    const uint32_t t0 = gui_cycles();
    lcd_draw_rgb565((uint16_t)a->x1, (uint16_t)(a->y1 + BAR), w, h, (const uint16_t *)px);
    s_flushCycles += gui_cycles() - t0;
    g_lv_flushes++;
    g_lv_flush_px += (uint32_t)w * h;
    lv_display_flush_ready(disp);           /* 告诉 LVGL 缓冲可以再用了（用 DMA 写的话，要等 DMA 完成再调） */
}
// [endregion]

// [region input]
static lv_point_t s_point;
static int s_pressed;

void lv_port_pointer(const gui_event *e)
{
    s_point.x = e->x;
    s_point.y = (int32_t)e->y - BAR;
    s_pressed = e->type != GUI_UP;
}

static void read_pointer(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->point = s_point;                  /* LVGL 来问「现在指针在哪、按没按着」，我们照实回答 */
    data->state = s_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}
// [endregion]

// [region init]
static uint32_t tick(void) { return HAL_GetTick(); }

void lv_port_init(void)
{
    lv_init();
    lv_tick_set_cb(tick);                   /* LVGL 的动画、长按、刷新节拍都靠它 */
    lv_display_t *disp = lv_display_create(LCD_W, LCD_H - BAR);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, s_buf, NULL, sizeof s_buf, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, flush);
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, read_pointer);
}
// [endregion]

void lv_port_run(void)
{
    const uint32_t before = g_lv_flushes, px = g_lv_flush_px, fc = s_flushCycles, t0 = gui_cycles();
    lv_timer_handler();
    if (g_lv_flushes != before) {          /* 这一次真的画了东西：记下用时和像素数 */
        g_lv_frame_cycles = gui_cycles() - t0;
        g_lv_frame_px = g_lv_flush_px - px;
        g_lv_frame_flush_cycles = s_flushCycles - fc;   /* 其中花在写屏上的；剩下的是 LVGL 在内存里画 */
    }
}
