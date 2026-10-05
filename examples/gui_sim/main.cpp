// 工位屏界面模拟器：按脚本「点」屏幕，每一步截图，并打印当前页面、点击次数、写了多少像素。
//   ./example_gui_sim [截图目录]     输出见 output.txt
#include <QGuiApplication>
#include <QImage>
#include <cstdio>
#include <tuple>

extern "C" {
#include "gui.h"
#include "pages.h"
#include "app.h"
extern uint16_t g_fb[800][480];
extern unsigned long g_pixels_written;
extern uint32_t g_now;
extern vc_result g_sim_result;
extern volatile uint32_t g_lv_flushes, g_lv_flush_px, g_lv_clicks;
extern volatile int32_t g_lv_slider;
extern int g_sim_result_fresh, g_sim_image_fresh, g_sim_image_done;
}

static QString g_dir;

static void save(const char *name)
{
    QImage img(480, 800, QImage::Format_RGB16);
    for (int y = 0; y < 800; ++y) memcpy(img.scanLine(y), g_fb[y], 480 * 2);
    img.save(g_dir + "/" + name + ".png");
}

// [region drive]
/* 让模拟时间前进 ms 毫秒：和固件的 StationTask 一样，每 20 ms 调一次后台任务和 gui_tick */
static void run(uint32_t ms)
{
    for (uint32_t t = 0; t < ms; t += 20) { g_now += 20; pages_background(g_now); gui_tick(g_now); }
}
static void ev(gui_event_type type, int x, int y) { gui_event e = {type, (int16_t)x, (int16_t)y}; gui_handle(&e); }
static void tap(int x, int y) { ev(GUI_DOWN, x, y); run(100); ev(GUI_UP, x, y); run(200); }
// [endregion]

static void step(const char *what)
{
    static unsigned long lastPixels;
    std::printf("  %-44s 当前页 %-12s 点击 %2lu 次，这一步写了 %6lu 个像素\n", what, gui_current()->title,
                (unsigned long)g_gui_clicks, g_pixels_written - lastPixels);
    lastPixels = g_pixels_written;
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    g_dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : ".";

    std::printf("==== 1. 首页 ====\n");
    // [region script]
    gui_init(&g_page_home); pages_init(); run(100);
    step("开机，显示首页");                                   save("gui-home");
    for (auto [x, y, what] : {std::tuple{10, 10, "标题栏"}, {88, 150, "STATION 图标的蓝色方块"}, {240, 150, "SENSORS 图标的绿色方块"}, {240, 500, "背景"}})
        std::printf("    (%3d,%3d) %-24s 0x%04x\n", x, y, what, g_fb[y][x]);
    ev(GUI_DOWN, 88, 190); run(100);
    step("按下 STATION 图标（还没抬起）");                    save("gui-home-pressed");
    ev(GUI_UP, 88, 190); run(200);
    step("在同一个图标上抬起 → 进入工位页");                  save("gui-station");
    // [endregion]

    std::printf("\n==== 2. 工位页收到一条检测结果 ====\n");
    g_sim_result = vc_result{}; g_sim_result.ok = 0; g_sim_result.defect = 1; g_sim_result.total = 42; g_sim_result.ng = 3;
    g_sim_result_fresh = 1; g_sim_image_fresh = 1; run(300);
    step("上位机发来 NG（划痕）和缩略图");                     save("gui-station-ng");
    std::printf("    缩略图的接收缓冲已归还 %d 次\n", g_sim_image_done);
    run(1000);
    step("什么都没变，又过了 1 秒");

    std::printf("\n==== 3. 返回、取消、进传感器页 ====\n");
    tap(40, 38);
    step("点左上角返回键");
    ev(GUI_DOWN, 240, 190); run(100); ev(GUI_MOVE, 240, 420); run(100); ev(GUI_UP, 240, 420); run(200);
    step("按下 SENSORS，拖出图标再抬起（取消）");
    run(130000);
    step("在首页停 130 秒（传感器历史照样在记）");
    tap(240, 190);
    step("点 SENSORS");                                        save("gui-sensors");

    std::printf("\n==== 4. 设备页 ====\n");
    tap(40, 38); tap(392, 190);
    step("返回，点 DEVICE");                                   save("gui-device");
    run(3000);
    step("停 3 秒（运行时间每秒刷新）");

    std::printf("\n==== 5. LVGL 页 ====\n");
    tap(40, 38);
    step("返回首页");
    const unsigned long f0 = g_lv_flushes, p0 = g_lv_flush_px;
    tap(88, 380);
    step("点 LVGL 图标（整块画一遍）");                            save("gui-lvgl");
    std::printf("    这一步 LVGL 写了 %lu 块、%lu 个像素\n", g_lv_flushes - f0, g_lv_flush_px - p0);
    run(2000);
    step("停 2 秒（曲线每秒加一个点）");
    tap(240, 180); tap(240, 180);
    step("点两下 TAP ME 按钮");
    std::printf("    按钮回调记到 %lu 次\n", (unsigned long)g_lv_clicks);
    ev(GUI_DOWN, 240, 286); run(100);
    for (int x = 260; x <= 420; x += 20) { ev(GUI_MOVE, x, 286); run(20); }
    ev(GUI_UP, 420, 286); run(200);
    step("按住滑块的圆点，拖到右边");                         save("gui-lvgl-used");
    std::printf("    滑块的值 %ld\n", (long)g_lv_slider);
    for (auto [x, y, what] : {std::tuple{240, 160, "按钮"}, {100, 286, "滑块左半"}, {400, 286, "滑块右半"}, {240, 700, "LVGL 背景"}})
        std::printf("    (%3d,%3d) %-12s 0x%04x\n", x, y, what, g_fb[y][x]);
    tap(40, 38);
    step("点左上角返回（框架的返回键，不经过 LVGL）");

    std::printf("\n==== 6. CLASSIC 页（手写框架版的工位界面，和 LVGL 版显示同样的数据）====\n");
    tap(240, 380);
    step("点 CLASSIC 图标（二级首页）");                         save("gui-classic-home");
    tap(88, 190);
    step("点 STATION：手写版工位页");                            save("gui-classic");
    run(1000);
    step("什么都没变，又过了 1 秒");
    tap(40, 38); tap(240, 190);
    step("返回，点 SENSORS：手写版传感器页");                    save("gui-classic-sensors");
    tap(40, 38); tap(392, 190);
    step("返回，点 DEVICE：手写版设备页");                       save("gui-classic-device");
    run(3000);
    step("停 3 秒（运行时间每秒刷新）");
    tap(40, 38); tap(40, 38);
    step("返回两次，回到首页");

    std::printf("\n共重画控件 %lu 次\n", (unsigned long)g_gui_redraws);
    return 0;
}
