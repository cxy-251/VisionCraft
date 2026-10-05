/*
 * 一个最小的界面框架：控件、页面、指针事件。纯 C，只依赖 lcd.h 的画图函数，
 * 所以同一份代码也能在电脑上的模拟器里跑（examples/gui_sim），用来测试和截图。
 *
 * 屏幕布局：顶部 76 像素是框架画的标题栏（不在首页时左边有返回键），下面是页面自己的区域。
 */
#ifndef VC_GUI_H
#define VC_GUI_H

#include <stdint.h>

#define GUI_BAR_H 76                       /* 标题栏高度 */

// [region event]
/* 指针事件：触摸、鼠标最后都变成这三种 */
typedef enum { GUI_DOWN, GUI_MOVE, GUI_UP } gui_event_type;
typedef struct {
    gui_event_type type;
    int16_t x, y;
} gui_event;
// [endregion]

// [region widget]
typedef struct gui_widget gui_widget;
struct gui_widget {
    int16_t x, y, w, h;                    /* 屏幕坐标 */
    const char *text;
    uint16_t color;                        /* 控件自己的主色（按钮底色、图标颜色……） */
    uint8_t pressed;                       /* 手指 / 鼠标正按在它上面 */
    uint8_t dirty;                         /* 需要重画 */
    void (*draw)(const gui_widget *w);     /* 怎么画自己 */
    void (*on_click)(gui_widget *w);       /* 被点击（按下又在它上面抬起）时调用，可以为空 */
    void *user;                            /* 给页面用的附加数据 */
};
// [endregion]

// [region page]
typedef struct gui_page gui_page;
struct gui_page {
    const char *title;
    gui_widget *widgets;
    uint8_t count;
    void (*enter)(void);                   /* 进入页面时：画背景等，可以为空 */
    void (*tick)(uint32_t now_ms);         /* 页面显示期间周期调用：刷新数据，可以为空 */
    void (*pointer)(const gui_event *e);   /* 没按在控件上的指针事件交给页面自己处理（比如交给 LVGL），可以为空 */
};
// [endregion]

void gui_init(const gui_page *home);       /* 显示首页 */
void gui_open(const gui_page *page);       /* 进入一个页面（压栈） */
void gui_back(void);                       /* 返回上一个页面 */
const gui_page *gui_current(void);
void gui_handle(const gui_event *e);       /* 交给框架一个指针事件 */
void gui_tick(uint32_t now_ms);            /* 周期调用：页面刷新 + 重画脏控件 */
void gui_invalidate(gui_widget *w);        /* 标记控件需要重画 */

/* 常用的画法 */
void gui_draw_button(const gui_widget *w);
void gui_draw_tile(const gui_widget *w);   /* 首页图标 */

void gui_line(int x0, int y0, int x1, int y1, uint16_t color);   /* 宽 2 像素的线段，画曲线用 */

/* 统计：给测试和调试器看 */
extern volatile uint32_t g_gui_clicks;     /* 一共触发了多少次点击 */
extern volatile uint32_t g_gui_redraws;    /* 一共重画了多少个控件 */
extern const char *volatile g_gui_title;  /* 当前页面的标题 */
extern volatile uint32_t g_gui_show_cycles; /* 最近一次换页整屏重画用了多少个时钟周期（板子上 168 MHz） */

#endif
