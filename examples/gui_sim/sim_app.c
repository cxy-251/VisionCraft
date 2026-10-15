/* 模拟器里的「板子其余部分」：固件页面用到的 app_* 函数和 HAL_GetTick，返回可控的假数据 */
#include "app.h"
#include "fault.h"
#include "main.h"
#include <string.h>

uint32_t g_now;                                   /* 模拟时间（毫秒），由测试程序推进 */
uint32_t HAL_GetTick(void) { return g_now; }

const char app_build_stamp[] = "Oct 05 2026 12:00:00";
void app_chip_uid(uint32_t uid[3]) { uid[0] = 0x00410037; uid[1] = 0x31365105; uid[2] = 0x30303837; }
uint32_t app_free_heap(void) { return 11872; }
int g_sim_tasks_mode;           /* 0 正常，1 快照失败，2 缩减列表：验证隐藏旧行 */
uint32_t app_task_count(void) { return g_sim_tasks_mode == 1 ? 11u : g_sim_tasks_mode == 2 ? 3u : 6u; }
int app_snapshot_tasks(app_task_snapshot *out)
{
    static const app_task_info tasks[] = {
        {"IDLE", 0, 428, 'r'}, {"Tmr Svc", 2, 896, 'B'}, {"USBH_Queue", 24, 740, 'B'},
        {"envTask", 16, 1308, 'B'}, {"linkTask", 32, 1736, 'B'}, {"stationTask", 24, 2668, 'R'},
    };
    out->total = app_task_count();
    out->count = g_sim_tasks_mode == 1 ? 0u : out->total;
    memcpy(out->tasks, tasks, out->count * sizeof tasks[0]);
    return g_sim_tasks_mode == 1 ? -1 : 0;
}

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

/* 设备页「上次复位」：模拟器里假装上次是上电复位、没有故障记录 */
volatile fault_record g_last_fault;
volatile uint32_t g_reset_flags = RCC_CSR_PORRSTF | RCC_CSR_PINRSTF;

/* 终端页的命令在模拟器里不执行（要用到 FreeRTOS、DWT、外部 SRAM），只回一行说明，用来看界面 */
#include "shell.h"
void shell_exec(const char *line, shell_out out, void *ctx) { (void)line; out("(simulator: commands run on the board)", ctx); }
const char *shell_complete(const char *prefix) { (void)prefix; return 0; }
/* 上位机的命令行：模拟器里没有链路 */
int app_take_shell(char cmd[64], uint8_t *seq) { (void)cmd; (void)seq; return 0; }
void app_send(uint8_t type, uint8_t seq, const void *payload, uint16_t len) { (void)type; (void)seq; (void)payload; (void)len; }
int app_tx_flush(uint32_t timeout_ms) { (void)timeout_ms; return 1; }

/* 设置页存在 EEPROM：模拟器里用一块内存假装，新芯片全是 0xFF */
#include "eeprom.h"
static uint8_t s_ee[256];
static int s_eeInit;
static void ee_init(void) { if (!s_eeInit) { memset(s_ee, 0xFF, sizeof s_ee); s_eeInit = 1; } }
void eeprom_init(void) { ee_init(); }
int eeprom_read(uint8_t addr, uint8_t *dst, size_t len) { ee_init(); if (addr + len > 256) return -1; memcpy(dst, s_ee + addr, len); return 0; }
int eeprom_write(uint8_t addr, const uint8_t *src, size_t len) { ee_init(); if (addr + len > 256) return -1; memcpy(s_ee + addr, src, len); return 0; }

/* 文件页：模拟器里没有 SD 卡。给一个假目录、一张假缩略图，只用来看界面排版；名字都标明是模拟的 */
#include "fs.h"
#include "photo.h"
#include <stdio.h>
const char *fs_result_name(int fr) { return fr ? "not ready" : "ok"; }
int fs_check(void) { return 0; }
static const uint16_t kDate = (uint16_t)((46u << 9) | (1u << 5) | 1u), kTime = (uint16_t)(12u << 11);   /* 2026-01-01 12:00 */
int fs_list(const char *path, fs_entry *out, unsigned max, unsigned *total)
{
    unsigned n = 0;
    if (strcmp(path, "/") == 0) {
        static const fs_entry root[] = {{"SIM_NOTE.TXT", 1200, 0, 0, 0}, {"SIM_DCIM", 0, 0, 0, 1}, {"SIM_DATA.BIN", 4096, 0, 0, 0}, {"SIM_LOGS", 0, 0, 0, 1}};
        for (; n < 4 && n < max; ++n) { out[n] = root[n]; out[n].date = kDate; out[n].time = kTime; }
    } else {
        for (; n < 24 && n < max; ++n) {
            snprintf(out[n].name, sizeof out[n].name, "SIM_%04u.%s", n / 2 + 1, n % 2 ? "NEF" : "JPG");
            out[n].size = n % 2 ? 25000000u + n * 13u : 9000000u + n * 7u;
            out[n].date = kDate; out[n].time = (uint16_t)(kTime + (n << 5)); out[n].dir = 0;
        }
    }
    *total = n;
    return 0;
}
int fs_read_head(const char *path, void *buf, uint32_t max, uint32_t *n)
{
    uint8_t *b = buf;
    if (strstr(path, ".TXT")) {
        static const char kText[] = "SIMULATOR: a fake text file.\nLine 2: the real board shows the file's first 8 KB.\n"
                                    "Line 3: non-ASCII bytes are shown as dots.\n";
        *n = sizeof kText - 1 < max ? sizeof kText - 1 : max;
        memcpy(b, kText, *n);
    } else {
        *n = max < 256 ? max : 256;
        for (uint32_t i = 0; i < *n; ++i) b[i] = (uint8_t)(i * 7u);   /* 假的二进制内容 */
        if (*n >= 4) memcpy(b, "II*", 4);
    }
    return 0;
}
int photo_read(const char *path, photo_info *info, uint16_t *rgb565, int max_w, int max_h)
{
    (void)path;
    memset(info, 0, sizeof *info);
    strcpy(info->model, "SIMULATOR");
    strcpy(info->taken, "2026:01:01 00:00:00");
    info->exp_num = 1; info->exp_den = 125; info->fnum_x10 = 56; info->iso = 400; info->focal_x10 = 500;
    info->w = 160; info->h = 120; info->full_w = 1600; info->full_h = 1200;
    const unsigned shift = (unsigned)(strlen(path) > 9 ? path[strlen(path) - 6] : 0) * 5u;   /* 每张换个颜色 */
    for (int y = 0; y < 120 && y < max_h; ++y)          /* 渐变色块，一看就是假的 */
        for (int x = 0; x < 160 && x < max_w; ++x)
            rgb565[y * 160 + x] = (uint16_t)((((x + shift) * 31 / 159 % 32) << 11) | ((y * 63 / 119) << 5) | ((shift / 3) % 32));
    return 0;
}
const char *photo_error_name(int rc) { return rc ? "error" : "ok"; }
volatile int32_t g_photo_last_fr;
volatile uint32_t g_sd_errors;
