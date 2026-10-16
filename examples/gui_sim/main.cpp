// 工位屏界面模拟器：按脚本「点」屏幕，每一步截图，并打印当前页面、点击次数、写了多少像素。
//   ./example_gui_sim [截图目录]     输出见 output.txt
#include <QGuiApplication>
#include <QImage>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <tuple>

extern "C" {
#include "lvgl.h"
#include "lv_port.h"
#include "gui.h"
#include "pages.h"
#include "app.h"
#include "settings.h"
#include "game.h"
#include "diagnostics.h"
#include "eeprom_diag.h"
#include "flash_diag.h"
extern uint16_t g_fb[800][480];
extern unsigned long g_pixels_written;
extern uint32_t g_now;
extern vc_result g_sim_result;
extern volatile uint32_t g_lv_flushes, g_lv_flush_px, g_lv_clicks;
extern volatile int32_t g_lv_slider;
extern uint8_t g_sim_backlight;
extern char g_term_inject[64];
extern volatile uint32_t g_paint_strokes, g_paint_points, g_paint_max_gap;

extern volatile uint32_t g_term_inject_seq;
extern volatile uint32_t g_files_loads, g_files_views, g_files_thumbs, g_files_binds;
extern app_task_snapshot g_device_tasks;
extern volatile uint32_t g_device_task_updates;
extern int g_sim_tasks_mode;
extern int g_sim_result_fresh, g_sim_image_fresh, g_sim_image_done;
}

static QString g_dir;

static unsigned visible_images(lv_obj_t *obj)
{
    unsigned n = lv_obj_check_type(obj, &lv_image_class) && lv_obj_is_visible(obj);
    for (unsigned i = 0; i < lv_obj_get_child_count(obj); ++i)
        n += visible_images(lv_obj_get_child(obj, i));
    return n;
}

static bool visible_label(lv_obj_t *obj, const char *text)
{
    if (lv_obj_check_type(obj, &lv_label_class) && lv_obj_is_visible(obj) &&
        std::strcmp(lv_label_get_text(obj), text) == 0) return true;
    for (unsigned i = 0; i < lv_obj_get_child_count(obj); ++i)
        if (visible_label(lv_obj_get_child(obj, i), text)) return true;
    return false;
}

static lv_obj_t *find_label(lv_obj_t *obj,const char *text)
{
    if(lv_obj_check_type(obj,&lv_label_class) && std::strcmp(lv_label_get_text(obj),text)==0) return obj;
    for(unsigned i=0;i<lv_obj_get_child_count(obj);++i)
        if(auto *found=find_label(lv_obj_get_child(obj,i),text)) return found;
    return nullptr;
}

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
    gui_init(&g_page_home); settings_load(); pages_init(); run(100);
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
    if (argc > 2 && std::strcmp(argv[2], "--diagnostics-only") == 0) {
        for(auto [label,kind] : {std::tuple{"SRAM自检",DIAG_SRAM}, {"EEPROM自检",DIAG_EEPROM}, {"Flash自检",DIAG_FLASH}}) {
            auto *text=find_label(lv_screen_active(),label);
            if(!text) return 1;
            auto *button=lv_obj_get_parent(text);
            lv_obj_scroll_to_view_recursive(button,LV_ANIM_OFF); run(200);
            lv_area_t rect;lv_obj_get_coords(button,&rect);
            tap((rect.x1+rect.x2)/2,(rect.y1+rect.y2)/2+GUI_BAR_H);
            if(g_diag_kind!=unsigned(kind)) return 1;
        }
        if(g_sram_diag.result!=DIAG_PASS || !g_ee_diag.restored || !g_flash_diag.restored) return 1;
        lv_obj_update_layout(lv_screen_active());
        auto *flashRowLabel = find_label(lv_screen_active(), "Flash 保留扇区");
        if(!flashRowLabel) return 1;
        lv_area_t card;
        lv_obj_get_coords(lv_obj_get_parent(lv_obj_get_parent(flashRowLabel)), &card);
        lv_obj_scroll_to_y(lv_screen_active(), lv_obj_get_scroll_y(lv_screen_active())+card.y1-60, LV_ANIM_OFF);
        run(200);
        save("gui-device-diagnostics");
        tap(40,38);tap(392,190);run(200);
        if(!find_label(lv_screen_active(),"Flash自检")) return 1;
        if(g_sram_diag.result!=DIAG_PASS || !g_ee_diag.restored || !g_flash_diag.restored) return 1;
        std::printf("PASS: diagnostics buttons through pointer events, simulated results retained on recreation; no storage I/O\n");
        return 0;
    }
    if (argc > 2 && std::strcmp(argv[2], "--device-only") == 0) {
        save("gui-device-tasks");
        if (g_device_tasks.count != 6 || g_device_task_updates < 4 ||
            !visible_label(lv_screen_active(), "stationTask")) return 1;
        g_sim_tasks_mode = 2; run(1200);
        if (g_device_tasks.count != 3 || visible_label(lv_screen_active(), "stationTask")) return 1;
        g_sim_tasks_mode = 1; run(1200);
        save("gui-device-tasks-error");
        if (g_device_tasks.count != 0 || !visible_label(lv_screen_active(), "任务快照不可用") ||
            visible_label(lv_screen_active(), "IDLE")) return 1;
        g_sim_tasks_mode = 0; run(1200);
        if (g_device_tasks.count != 6 || !visible_label(lv_screen_active(), "stationTask") ||
            visible_label(lv_screen_active(), "任务快照不可用")) return 1;
        for (int i = 0; i < 3; ++i) { tap(40, 38); tap(392, 190); run(1200); }
        if (!visible_label(lv_screen_active(), "stationTask")) return 1;
        std::printf("PASS: DEVICE 六任务、定时刷新、缩减隐藏、快照失败与恢复、重复进入；未进入文件页\n");
        return 0;
    }

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
    step("按住滑块，拖到右边");                         save("gui-lvgl-used");
    std::printf("    滑块的值 %ld，背光设成 %u %%\n", (long)g_lv_slider, (unsigned)g_sim_backlight);
    for (auto [x, y, what] : {std::tuple{240, 160, "按钮"}, {100, 286, "滑块左半"}, {400, 286, "滑块右半"}, {240, 700, "LVGL 背景"}})
        std::printf("    (%3d,%3d) %-12s 0x%04x\n", x, y, what, g_fb[y][x]);
    tap(40, 38);
    step("点左上角返回（框架的返回键，不经过 LVGL）");

    std::printf("\n==== 6. CLASSIC 页（手写框架版的工位界面，和 LVGL 版显示同样的数据）====\n");
    tap(240, 380);
    step("点 CLASSIC 图标（二级首页）");                         save("gui-classic-home");
    tap(88, 190);
    step("点 OLD ST：手写版工位页");                            save("gui-classic");
    run(1000);
    step("什么都没变，又过了 1 秒");
    tap(40, 38); tap(240, 190);
    step("返回，点 OLD SE：手写版传感器页");                    save("gui-classic-sensors");
    tap(40, 38); tap(392, 190);
    step("返回，点 OLD DV：手写版设备页");                       save("gui-classic-device");
    run(3000);
    step("停 3 秒（运行时间每秒刷新）");
    tap(40, 38); tap(40, 38);
    step("返回两次，回到首页");

    std::printf("\n==== 7. 终端页 ====\n");
    tap(392, 380);
    step("点 TERMINAL 图标");
    std::strcpy(g_term_inject, "help"); g_term_inject_seq++; run(200);
    step("执行一条命令（模拟器里不真正执行）");                 save("gui-term");
    tap(40, 38);
    step("返回首页");

    std::printf("\n==== 8. 设置页 ====\n");
    tap(88, 500);
    step("点 SETTINGS 图标");                                    save("gui-settings");
    tap(385, 241);
    step("点开「闲置后调暗」下拉框");                            save("gui-settings-dropdown");
    tap(385, 241);
    step("再点一下收起");
    tap(40, 38);
    step("返回首页");

    std::printf("\n==== 9. 扫雷 ====\n");
    g_game_seed = 12345;                       // 固定种子：每次运行布雷都一样，截图能复现
    tap(240, 500);
    step("点 GAMES 图标");                                       save("gui-games");
    tap(88, 190);
    step("点 MINES：新开一局");                                  save("gui-mines-new");
    auto cell = [](int r, int c) { tap(24 + c * 48 + 24, 76 + 64 + r * 48 + 24); };
    cell(4, 4);
    std::printf("    第一下点正中间：翻开 %d 格，状态 %d（0 进行中）\n", g_mines.revealed, g_mines.status);
    step("第一下：必定不是雷，连片翻开");                       save("gui-mines-first");
    for (int r = 0; r < 9; ++r)               // 按雷图把剩下不是雷的格子都点开（测试用：直接看答案）
        for (int c = 0; c < 9; ++c)
            if (!g_mines.mine[r][c] && g_mines.cell[r][c] == 0) cell(r, c);
    std::printf("    点完所有安全格：翻开 %d 格，状态 %d（1 赢），走了 %u 步\n", g_mines.revealed, g_mines.status, (unsigned)g_mines.moves);
    step("把所有不是雷的格子点开");                             save("gui-mines-win");
    tap(124, 140 + 9 * 48 + 24 + 36);
    cell(4, 4);
    for (int r = 0, done = 0; r < 9 && !done; ++r)
        for (int c = 0; c < 9 && !done; ++c)
            if (g_mines.mine[r][c]) { cell(r, c); done = 1; }
    std::printf("    新开一局，故意点一颗雷：状态 %d（2 输）\n", g_mines.status);
    step("踩雷");                                                 save("gui-mines-lose");
    tap(40, 38); tap(40, 38);
    step("返回两次，回到首页");

    std::printf("\n==== 10. 贪吃蛇 ====\n");
    tap(240, 500); tap(240, 190);
    step("点 GAMES、SNAKE");                                     save("gui-snake-new");
    auto swipe = [](int dx, int dy) { ev(GUI_DOWN, 240, 360); ev(GUI_UP, 240 + dx * 80, 360 + dy * 80); };
    swipe(0, -1);                                                    // 往上滑一下：开始
    unsigned long px0 = g_pixels_written; uint32_t s0 = g_snake.steps;
    for (int t = 0; t < 1500 && g_snake.status != 2; ++t) {          // 30 秒：每 20 ms 看一眼，朝食物拐（测试用的简单策略）
        const int hx = g_snake.x[g_snake.head], hy = g_snake.y[g_snake.head];
        const int d = g_snake.dir;
        if (g_snake.foodX > hx && d != 3) swipe(1, 0);
        else if (g_snake.foodX < hx && d != 1) swipe(-1, 0);
        else if (g_snake.foodY > hy && d != 0) swipe(0, 1);
        else if (g_snake.foodY < hy && d != 2) swipe(0, -1);
        run(20);
    }
    std::printf("    30 秒：走了 %u 步，吃了 %u 个，长度 %u，状态 %d；平均每步写 %lu 个像素\n", (unsigned)(g_snake.steps - s0),
                (unsigned)g_snake.score, (unsigned)g_snake.len, g_snake.status,
                (g_pixels_written - px0) / ((g_snake.steps - s0) ? (g_snake.steps - s0) : 1));
    step("自动玩 30 秒");                                         save("gui-snake-play");
    tap(40, 38);

    std::printf("\n==== 11. 俄罗斯方块 ====\n");
    tap(392, 190);
    step("点 TETRIS");                                            save("gui-tetris-new");
    px0 = g_pixels_written; const uint32_t c0 = g_tetris_cells_drawn, pc0 = g_tetris.pieces;
    for (int t = 0; t < 1000 && g_tetris.status == 0; ++t) {          // 20 秒：每块随机左右挪几下，然后让它自己落
        if (t % 25 == 0) { const int m = int(game_rand() % 9) - 4; for (int k = 0; k < (m < 0 ? -m : m); ++k) tap(m < 0 ? 70 : 405, 140 + 520 + 16 + 38); }
        run(20);
    }
    std::printf("    20 秒：落了 %u 块，消了 %u 行，%u 分；重画了 %u 格，共写 %lu 个像素\n", (unsigned)(g_tetris.pieces - pc0),
                (unsigned)g_tetris.lines, (unsigned)g_tetris.score, (unsigned)(g_tetris_cells_drawn - c0), g_pixels_written - px0);
    step("自动玩 20 秒");                                         save("gui-tetris-play");
    tap(380, 140 + 456);                                             // NEW：重新开一局
    for (int x = 0; x < 10; ++x) g_tetris.board[19][x] = (x >= 3 && x <= 6) ? 0 : 6;   // 摆残局：最底下一行只空中间 4 格
    g_tetris.type = 0;                                               // 当前这一块换成横着的长条（I），在第 3–6 列
    { const int8_t i_piece[4][2] = {{0, 1}, {1, 1}, {2, 1}, {3, 1}}; std::memcpy(g_tetris.cells, i_piece, sizeof i_piece); }
    g_tetris.x = 3; g_tetris.y = 0;
    for (int k = 0; k < 20; ++k) tap(290, 140 + 520 + 16 + 38);      // 一直按「v」让它落到底
    std::printf("    摆好残局再落一根长条：消了 %u 行，%u 分，最底下一行现在%s\n", (unsigned)g_tetris.lines, (unsigned)g_tetris.score,
                g_tetris.board[19][0] == 0 ? "是空的" : "还有东西");
    step("消行");                                                  save("gui-tetris-clear");
    tap(40, 38); tap(40, 38);
    step("返回两次，回到首页");

    std::printf("\n==== 12. 画板 ====\n");
    tap(392, 500);
    step("点 PAINT（第一次进来：铺一张白纸）");
    tap(8 + 60 + 28, 76 + 8 + 610 + 28);                              // 选红色
    ev(GUI_DOWN, 40, 160);                                           // 快速划一笔：每隔 20 ms 才采到一个点，相邻点隔 60 多像素
    for (int i = 1; i <= 6; ++i) { run(20); ev(GUI_MOVE, 40 + i * 60, 160 + i * 40); }
    run(20); ev(GUI_UP, 400, 400); run(100);
    tap(8 + 4 * 60 + 28, 76 + 8 + 610 + 28);                         // 选蓝色
    ev(GUI_DOWN, 60, 600);                                           // 慢慢画一段波浪：点很密
    for (int i = 1; i <= 80; ++i) { run(20); ev(GUI_MOVE, 60 + i * 4, 600 - int(60 * std::sin(i / 8.0))); }
    run(20); ev(GUI_UP, 60 + 80 * 4, 600 - int(60 * std::sin(80 / 8.0))); run(100);   // 抬起在最后一个点（真实触摸也是这样）
    std::printf("    画了 %u 笔、%u 个点，相邻两点最远隔 %u 像素\n", (unsigned)g_paint_strokes, (unsigned)g_paint_points, (unsigned)g_paint_max_gap);
    std::printf("    快的那一笔中间（两个采样点之间）(130, 220) 的像素 0x%04x（红色是 0x%04x）\n", g_fb[220][130], 0xd924);
    step("画两笔");                                                save("gui-paint");
    tap(40, 38); tap(392, 500);
    std::printf("    离开再回来：(130, 220) 的像素 0x%04x\n", g_fb[220][130]);
    step("离开再回来，画还在");
    tap(40, 38);

    std::printf("\n==== 13. 文件页 ====\n");
    const uint32_t views0 = g_files_views, binds0 = g_files_binds;
    tap(88, 661);
    step("点 FILES：根目录，列表视图（模拟器里是假目录）");    save("gui-files-root");
    run(1100);                                                       // lv_port 每秒更新一次内存池用量
    std::printf("    LVGL 内存池：用了 %u 字节，最多 %u（模拟器是 64 位，指针 8 字节，比板子多用）\n", (unsigned)g_lv_mem_used, (unsigned)g_lv_mem_peak);
    tap(240, 160);
    step("点第一项（文件夹在前）：进目录");                      save("gui-files-dir");
    for (int i = 0; i < 12; ++i) { ev(i ? GUI_MOVE : GUI_DOWN, 240, 600 - i * 30); run(20); }
    ev(GUI_UP, 240, 270); run(800);
    step("往上拖：列表滚动，控件循环使用");                       save("gui-files-scrolled");
    tap(436, 106);
    run(3000);
    step("切到图标视图：照片显示缩略图（一轮解一张）");          save("gui-files-grid");
    tap(88, 220);
    step("点一张照片");                                          save("gui-files-photo");
    tap(40, 106); tap(436, 106); tap(42, 106);
    step("回到列表、回上一级");                                  save("gui-files-root2");
    if (visible_images(lv_screen_active()) != 0) {
        std::fprintf(stderr, "FAIL: 切回列表后仍有缩略图残留\n");
        return 1;
    }
    tap(240, 409);
    step("点 SIM_NOTE.TXT：按文本看");                           save("gui-files-text");
    tap(42, 106); tap(240, 333);
    step("点 SIM_DATA.BIN：按十六进制看");                       save("gui-files-hex");
    if (g_files_views != views0 + 3 || g_files_binds <= binds0 + 2 || g_files_thumbs == 0) {
        std::fprintf(stderr, "FAIL: 文件打开、滚动绑定或缩略图加载没有完成\n");
        return 1;
    }
    std::printf("    PASS: 打开照片、文本、二进制；滚动重绑；图标切回列表无缩略图残留\n");
    tap(42, 106);
    tap(40, 38);

    std::printf("\n共重画控件 %lu 次\n", (unsigned long)g_gui_redraws);
    return 0;
}
