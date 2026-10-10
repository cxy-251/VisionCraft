#include "shell.h"

#include "FreeRTOS.h"
#include "task.h"
#include "app.h"
#include "eeprom.h"
#include "fault.h"
#include "fs.h"
#include "sdcard.h"
#include "settings.h"
#include "lcd.h"
#include "lv_port.h"
#include "lvgl.h"                      /* lv_snprintf：newlib-nano 的 printf 不带浮点，这个也不带，但小 */
#include "main.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ARGS 4

typedef struct {
    shell_out out;
    void *ctx;
} shell_io;

static void say(shell_io *io, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static void say(shell_io *io, const char *fmt, ...)
{
    char line[96];
    va_list ap;
    va_start(ap, fmt);
    lv_vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    io->out(line, io->ctx);
}

// [region commands]
static void cmd_help(shell_io *io, int argc, char **argv);

static void cmd_uptime(shell_io *io, int argc, char **argv)
{
    (void)argc; (void)argv;
    const uint32_t s = HAL_GetTick() / 1000u;
    say(io, "up %luh %lum %lus", (unsigned long)(s / 3600u), (unsigned long)(s / 60u % 60u), (unsigned long)(s % 60u));
}

/* 每个任务：状态、优先级、栈从来没用到过的字节数（FreeRTOS 在栈里填 0xA5，数还剩多少没被改过） */
static void cmd_ps(shell_io *io, int argc, char **argv)
{
    (void)argc; (void)argv;
    TaskStatus_t t[10];
    const UBaseType_t n = uxTaskGetSystemState(t, 10, NULL);
    static const char state[] = "RrBSD";   /* 运行、就绪、阻塞、挂起、已删除 */
    say(io, "%-12s %s %4s %9s", "TASK", "S", "PRIO", "STACK-FREE");
    for (UBaseType_t i = 0; i < n; ++i)
        say(io, "%-12s %c %4lu %9lu", t[i].pcTaskName, state[t[i].eCurrentState < 5 ? t[i].eCurrentState : 4],
            (unsigned long)t[i].uxCurrentPriority, (unsigned long)t[i].usStackHighWaterMark * 4u);
}

static void cmd_free(shell_io *io, int argc, char **argv)
{
    (void)argc; (void)argv;
    say(io, "freertos heap  free %lu  min %lu", (unsigned long)xPortGetFreeHeapSize(),
        (unsigned long)xPortGetMinimumEverFreeHeapSize());
    say(io, "lvgl pool      used %lu  peak %lu  of %lu", (unsigned long)g_lv_mem_used,
        (unsigned long)g_lv_mem_peak, (unsigned long)LV_MEM_SIZE);
}

static void cmd_sensors(shell_io *io, int argc, char **argv)
{
    (void)argc; (void)argv;
    const app_sensors s = app_latest_sensors();
    if (!s.valid) { say(io, "no data yet"); return; }
    say(io, "temp %d.%02d C  light %d.%d %%  vdda %d mV", s.cpu_temp_c100 / 100, abs(s.cpu_temp_c100 % 100),
        s.light_permille / 10, s.light_permille % 10, s.vdda_mv);
}

static void cmd_beep(shell_io *io, int argc, char **argv)
{
    const int ms = argc > 1 ? atoi(argv[1]) : 100;
    if (ms <= 0 || ms > 2000) { say(io, "usage: beep <1-2000 ms>"); return; }
    app_beep((uint16_t)ms);
    say(io, "beep %d ms", ms);
}

static void cmd_bl(shell_io *io, int argc, char **argv)
{
    const int p = argc > 1 ? atoi(argv[1]) : -1;
    if (p < 10 || p > 100) { say(io, "usage: bl <10-100>"); return; }   /* 最低 10 %：0 就看不见屏幕了 */
    settings_set_brightness((uint8_t)p);   /* 改的是设置里的亮度，但不存；设置页或演示页离开时才存 */
    say(io, "backlight %d %% (not saved)", p);
}

/* 外部 SRAM 自检：测最后 64 KB（前面放着缩略图和 LVGL 内存池，见链接脚本的 .extsram）。写一串伪随机数、读回比较，顺便量速度 */
static void cmd_sram(shell_io *io, int argc, char **argv)
{
    (void)argc; (void)argv;
    volatile uint32_t *p = (volatile uint32_t *)0x680F0000u;
    const uint32_t words = 16384u;
    uint32_t x = 0x1234567u, bad = 0;
    const uint32_t t0 = DWT->CYCCNT;
    for (uint32_t i = 0; i < words; ++i) { x = x * 1103515245u + 12345u; p[i] = x; }
    const uint32_t t1 = DWT->CYCCNT;
    x = 0x1234567u;
    for (uint32_t i = 0; i < words; ++i) { x = x * 1103515245u + 12345u; if (p[i] != x) bad++; }
    const uint32_t t2 = DWT->CYCCNT;
    /* 速度：64 KB ÷ 用时；168 MHz，周期数换算成 KB/s */
    say(io, "sram 0x680F0000 +64KB: %lu bad words", (unsigned long)bad);
    say(io, "write %lu KB/s  read %lu KB/s", (unsigned long)(64ull * 168000000u / (t1 - t0)),
        (unsigned long)(64ull * 168000000u / (t2 - t1)));
}

// [region sd]
/* SD 卡：初始化、挂载、打印卡的身份（CID）和文件系统。全程只读 */
static void cmd_sd(shell_io *io, int argc, char **argv)
{
    (void)argc; (void)argv;
    const int fr = fs_mount();
    if (!sd_ready()) { say(io, "no card (init %lu ms)", (unsigned long)sd_init_ms()); return; }
    const sd_info *c = sd_get_info();
    say(io, "card %s  %lu MB  mid 0x%02x oem %.2s name %.5s rev %u.%u", c->type ? "SDHC/XC" : "SDSC",
        (unsigned long)(c->blocks / 2048u), c->mid, c->oem, c->name, c->rev >> 4, c->rev & 15u);
    say(io, "made %u-%02u  sn %08lx  clk %lu kHz  init %lu ms", c->year, c->month, (unsigned long)c->serial,
        (unsigned long)(c->clock_hz / 1000u), (unsigned long)sd_init_ms());
    if (fr != FR_OK) { say(io, "mount: %s", fs_result_name(fr)); return; }
    FATFS *v = fs_volume();
    static const char *const kType[] = {"?", "FAT12", "FAT16", "FAT32", "exFAT"};
    char label[24] = "";
    DWORD vsn = 0;
    f_getlabel("", label, &vsn);
    say(io, "%s  label '%s'  cluster %u KB  %lu clusters", kType[v->fs_type < 5 ? v->fs_type : 0], label,
        v->csize / 2u, (unsigned long)(v->n_fatent - 2u));
    say(io, "partition at sector %lu  fat %lu  data %lu  mount %lu ms", (unsigned long)v->volbase,
        (unsigned long)v->fatbase, (unsigned long)v->database, (unsigned long)fs_mount_ms());
}

/* ls [路径]：列目录。最多列 20 项（远程终端一帧 1 KB 装得下），后面只报个数 */
static void cmd_ls(shell_io *io, int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "/";
    const int m = fs_mount();
    if (m != FR_OK) { say(io, "mount: %s", fs_result_name(m)); return; }
    static DIR d;
    static FILINFO fi;                 /* 带 256 字节的长文件名，放静态区不放任务栈 */
    const FRESULT fr = f_opendir(&d, path);
    if (fr != FR_OK) { say(io, "%s: %s", path, fs_result_name(fr)); return; }
    unsigned n = 0, dirs = 0;
    unsigned long long bytes = 0;
    const uint32_t t0 = HAL_GetTick();
    while (f_readdir(&d, &fi) == FR_OK && fi.fname[0]) {
        const int isDir = (fi.fattrib & AM_DIR) != 0;
        if (n < 20) {
            if (isDir) say(io, "  %-24s  <dir>", fi.fname);
            else say(io, "  %-24s %9lu", fi.fname, (unsigned long)fi.fsize);
        }
        n++;
        dirs += (unsigned)isDir;
        bytes += fi.fsize;
    }
    f_closedir(&d);
    say(io, "%u entries (%u dirs), %lu KB, %lu ms", n, dirs, (unsigned long)(bytes / 1024u),
        (unsigned long)(HAL_GetTick() - t0));
}

/* sdbench：从数据区开头读一段，比较「一次读 64 块」和「一次读 1 块」的速度。只读，不改卡 */
static void cmd_sdbench(shell_io *io, int argc, char **argv)
{
    (void)argc; (void)argv;
    const int m = fs_mount();
    if (m != FR_OK) { say(io, "mount: %s", fs_result_name(m)); return; }
    uint8_t *buf = (uint8_t *)0x680F0000u;   /* 外部 SRAM 末尾 64 KB（sram 自检也用这里），DMA 能直接写 */
    const uint32_t start = fs_volume()->database;
    uint32_t t0 = HAL_GetTick();
    for (uint32_t i = 0; i < 64u; ++i)          /* 64 次 × 64 块 × 512 字节 = 2 MB */
        if (sd_read(buf, start + i * 64u, 64)) { say(io, "read error at %lu", (unsigned long)(start + i * 64u)); return; }
    const uint32_t multi = HAL_GetTick() - t0;
    t0 = HAL_GetTick();
    for (uint32_t i = 0; i < 512u; ++i)         /* 512 次 × 1 块 = 256 KB */
        if (sd_read(buf, start + i, 1)) { say(io, "read error at %lu", (unsigned long)(start + i)); return; }
    const uint32_t single = HAL_GetTick() - t0;
    say(io, "64 blocks/read: 2048 KB in %lu ms = %lu KB/s", (unsigned long)multi, (unsigned long)(2048u * 1000u / multi));
    say(io, "1 block/read:    256 KB in %lu ms = %lu KB/s", (unsigned long)single, (unsigned long)(256u * 1000u / single));
}

/* sdclk [div]：改 SDIO 时钟（48 MHz / (div + 2)）。sdsum：读数据区开头 2 MB 算 FNV-1a 校验和。
 * 两个一起用：不同时钟读同一段数据，校验和一样，说明快了也没读错 */
static void cmd_sdclk(shell_io *io, int argc, char **argv)
{
    if (fs_mount() != FR_OK) { say(io, "no card"); return; }
    if (argc > 1) sd_set_clock_div((uint8_t)atoi(argv[1]));
    say(io, "sdio clock %lu kHz", (unsigned long)(sd_get_info()->clock_hz / 1000u));
}

static void cmd_sdsum(shell_io *io, int argc, char **argv)
{
    (void)argc; (void)argv;
    if (fs_mount() != FR_OK) { say(io, "no card"); return; }
    const uint32_t *buf = (const uint32_t *)0x680F0000u;
    const uint32_t start = fs_volume()->database;
    uint32_t h = 2166136261u;
    for (uint32_t i = 0; i < 64u; ++i) {
        if (sd_read((void *)buf, start + i * 64u, 64)) { say(io, "read error"); return; }
        for (uint32_t w = 0; w < 64u * 512u / 4u; ++w) h = (h ^ buf[w]) * 16777619u;
    }
    say(io, "2048 KB from sector %lu: fnv1a %08lx", (unsigned long)start, (unsigned long)h);
}
// [endregion]

static void cmd_recipe(shell_io *io, int argc, char **argv)
{
    (void)argc; (void)argv;
    vc_recipe r;
    if (recipe_load(&r) != 0) { say(io, "no valid recipe in eeprom"); return; }
    say(io, "surface %u  area %u  offset %u.%u", r.surface_threshold, r.min_defect_area,
        r.max_center_offset_x10 / 10, r.max_center_offset_x10 % 10);
    say(io, "chip %u.%u  scratch %u.%u", r.min_chip_depth_x10 / 10, r.min_chip_depth_x10 % 10,
        r.scratch_elongation_x10 / 10, r.scratch_elongation_x10 % 10);
}

static void cmd_last(shell_io *io, int argc, char **argv)
{
    (void)argc; (void)argv;
    const fault_record r = *(const fault_record *)&g_last_fault;
    say(io, "RCC_CSR 0x%08lx", (unsigned long)g_reset_flags);
    if (r.kind == FAULT_HARD)
        say(io, "hardfault pc %08lx lr %08lx cfsr %08lx task %s", (unsigned long)r.pc, (unsigned long)r.lr,
            (unsigned long)r.cfsr, r.task);
    else if (r.kind == FAULT_STACK) say(io, "stack overflow in %s", r.task);
    else if (r.kind == FAULT_HANG) say(io, "task %s stopped checking in", r.task);
    else say(io, "no fault record");
}

static void cmd_settings(shell_io *io, int argc, char **argv)
{
    (void)argc; (void)argv;
    say(io, "brightness %u  key_beep %u  dim %u min", g_settings.brightness, g_settings.key_beep, g_settings.dim_minutes);
    say(io, "loaded rc %ld  saved %lu times  dimmed %u", (long)g_settings_load_rc,
        (unsigned long)g_settings_saves, g_screen_dimmed);
}

/* 读 EEPROM 原始字节：ee <地址> [长度]，十六进制显示 */
static void cmd_ee(shell_io *io, int argc, char **argv)
{
    const int addr = argc > 1 ? (int)strtol(argv[1], NULL, 0) : 0;
    int len = argc > 2 ? (int)strtol(argv[2], NULL, 0) : 16;
    if (addr < 0 || addr > 255 || len < 1 || len > 32 || addr + len > 256) { say(io, "usage: ee <addr 0-255> [len 1-32]"); return; }
    uint8_t b[32];
    if (eeprom_read((uint8_t)addr, b, (size_t)len) != 0) { say(io, "eeprom read failed"); return; }
    for (int i = 0; i < len; i += 8) {
        char line[64], *p = line;
        p += lv_snprintf(p, 8, "%02x:", addr + i);
        for (int j = i; j < len && j < i + 8; ++j) p += lv_snprintf(p, 5, " %02x", b[j]);
        say(io, "%s", line);
    }
}

static void cmd_reboot(shell_io *io, int argc, char **argv)
{
    (void)argc; (void)argv;
    say(io, "rebooting");
    NVIC_SystemReset();
}

static const struct {
    const char *name;
    void (*fn)(shell_io *io, int argc, char **argv);
    const char *help;
} kCommands[] = {
    {"help", cmd_help, "list commands"},
    {"uptime", cmd_uptime, "time since boot"},
    {"ps", cmd_ps, "tasks: state, priority, free stack"},
    {"free", cmd_free, "freertos heap, lvgl pool"},
    {"sensors", cmd_sensors, "temperature, light, vdda"},
    {"beep", cmd_beep, "beep <ms>"},
    {"bl", cmd_bl, "backlight <10-100>"},
    {"sram", cmd_sram, "external sram self-test"},
    {"sd", cmd_sd, "sd card info and mount (read-only)"},
    {"ls", cmd_ls, "ls [path]: list a directory"},
    {"sdbench", cmd_sdbench, "sd card read speed"},
    {"sdclk", cmd_sdclk, "sdclk [div]: sdio clock 48MHz/(div+2)"},
    {"sdsum", cmd_sdsum, "checksum of 2 MB read from the card"},
    {"recipe", cmd_recipe, "recipe stored in eeprom"},
    {"last", cmd_last, "last reset cause and fault record"},
    {"settings", cmd_settings, "current settings"},
    {"ee", cmd_ee, "ee <addr> [len]: dump eeprom bytes"},
    {"reboot", cmd_reboot, "software reset"},
};
#define N_COMMANDS (sizeof kCommands / sizeof kCommands[0])

static void cmd_help(shell_io *io, int argc, char **argv)
{
    (void)argc; (void)argv;
    for (unsigned i = 0; i < N_COMMANDS; ++i)
        say(io, "%-8s %s", kCommands[i].name, kCommands[i].help);
}
// [endregion]

// [region exec]
/* 按空格切成参数，第一个是命令名。只认 ASCII；整行最长 63 个字符 */
void shell_exec(const char *line, shell_out out, void *ctx)
{
    shell_io io = {out, ctx};
    char buf[64];
    strncpy(buf, line, sizeof buf - 1);
    buf[sizeof buf - 1] = 0;
    char *argv[MAX_ARGS];
    int argc = 0;
    for (char *p = strtok(buf, " "); p && argc < MAX_ARGS; p = strtok(NULL, " "))
        argv[argc++] = p;
    if (argc == 0)
        return;
    for (unsigned i = 0; i < N_COMMANDS; ++i) {
        if (strcmp(argv[0], kCommands[i].name) == 0) {
            kCommands[i].fn(&io, argc, argv);
            return;
        }
    }
    say(&io, "unknown command: %s (try help)", argv[0]);
}
// [endregion]

const char *shell_complete(const char *prefix)
{
    const size_t n = strlen(prefix);
    for (unsigned i = 0; n && i < N_COMMANDS; ++i)
        if (strncmp(prefix, kCommands[i].name, n) == 0)
            return kCommands[i].name;
    return 0;
}
