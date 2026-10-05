/* 模拟器里的「板子其余部分」：固件页面用到的 app_* 函数和 HAL_GetTick，返回可控的假数据 */
#include "app.h"
#include "main.h"
#include <string.h>

uint32_t g_now;                                   /* 模拟时间（毫秒），由测试程序推进 */
uint32_t HAL_GetTick(void) { return g_now; }

const char app_build_stamp[] = "Oct 05 2026 12:00:00";
void app_chip_uid(uint32_t uid[3]) { uid[0] = 0x00410037; uid[1] = 0x31365105; uid[2] = 0x30303837; }
uint32_t app_free_heap(void) { return 11872; }
uint32_t app_task_count(void) { return 5; }

uint32_t app_rx_frames(void) { return 1234; }
uint32_t app_rx_errors(void) { return 0; }
uint32_t app_last_rx_tick(void) { return g_now ? g_now - 100 : 0; }   /* 假装上位机一直在线 */

app_sensors app_latest_sensors(void)
{
    /* 温度 42–44 °C 缓慢起伏，光照 3–9 % */
    const int t = (int)(g_now / 1000u);
    app_sensors s = {(int16_t)(4300 + (t % 40 < 20 ? t % 20 : 20 - t % 20) * 10 - 100), (uint16_t)(30 + (t * 7) % 60), 3291, 1};
    return s;
}

vc_result g_sim_result; int g_sim_result_fresh;
int app_take_result(vc_result *out) { if (!g_sim_result_fresh) return 0; *out = g_sim_result; g_sim_result_fresh = 0; return 1; }
/* 假缩略图：160×120，一个灰色渐变背景上的深色圆环（像上位机发来的工件图） */
static uint16_t s_img[160 * 120];
int g_sim_image_fresh, g_sim_image_done;
int app_take_image(uint16_t *w, uint16_t *h, const uint16_t **pixels)
{
    if (!g_sim_image_fresh) return 0;
    g_sim_image_fresh = 0;
    for (int y = 0; y < 120; ++y)
        for (int x = 0; x < 160; ++x) {
            const int dx = x - 80, dy = y - 60, r2 = dx * dx + dy * dy;
            int g = 90 + x / 2;                               /* 0..255 的灰度 */
            if (r2 < 50 * 50 && r2 > 20 * 20) g = 40;         /* 圆环 */
            s_img[y * 160 + x] = (uint16_t)(((g >> 3) << 11) | ((g >> 2) << 5) | (g >> 3));
        }
    *w = 160; *h = 120; *pixels = s_img;
    return 1;
}
void app_image_done(void) { g_sim_image_done++; }
