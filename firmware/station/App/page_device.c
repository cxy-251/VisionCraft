/* 「设备」页（LVGL 版）：固件、芯片、运行状态和三处内存的用量。手写框架版在 page_classic_device.c。 */
#include "pages.h"

#include "app.h"
#include "lv_port.h"
#include "fault.h"
#include "lv_ui.h"
#include "main.h"                           /* HAL_GetTick */

extern const char app_build_stamp[];

static lv_obj_t *s_screen, *s_uptime, *s_heap, *s_tasks, *s_lvmem;

/* 一行：左边名字，右边数值 */
static lv_obj_t *row(lv_obj_t *card, const char *name, const char *value)
{
    lv_obj_t *r = ui_row(card);
    ui_text(r, name, UI_MUTED);
    return ui_text(r, value, UI_TEXT);
}

static lv_obj_t *group(const char *title)
{
    ui_text(s_screen, title, UI_MUTED);
    lv_obj_t *c = ui_card(s_screen, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(c, 10, 0);
    return c;
}

// [region reset]
/* 上次为什么复位：RCC_CSR 的复位标志 + 复位前记下的故障记录（App/fault.c）。
 * 复位键会让 NRST 脚动一下，其他复位也会顺带把 PINRSTF 置位，所以按「最具体的」先判断 */
static const char *reset_cause(uint32_t f)
{
    if (f & RCC_CSR_IWDGRSTF) return "看门狗";
    if (f & RCC_CSR_WWDGRSTF) return "窗口看门狗";
    if (f & RCC_CSR_SFTRSTF) return "软件复位";
    if (f & RCC_CSR_BORRSTF) return "上电 / 电压过低";
    if (f & RCC_CSR_PORRSTF) return "上电";
    if (f & RCC_CSR_PINRSTF) return "复位键或调试器";
    return "未知";
}

static void reset_group(void)
{
    char line[48];
    lv_obj_t *g = group("上次复位");
    row(g, "原因", reset_cause(g_reset_flags));
    const fault_record r = *(const fault_record *)&g_last_fault;
    if (r.kind == FAULT_HARD) {
        row(g, "记录", "HardFault");
        lv_snprintf(line, sizeof line, "PC %08X  LR %08X", (unsigned)r.pc, (unsigned)r.lr);
        row(g, "地址", line);
        lv_snprintf(line, sizeof line, "CFSR %08X  HFSR %08X", (unsigned)r.cfsr, (unsigned)r.hfsr);
        row(g, "故障寄存器", line);
        row(g, "任务", r.task);
    } else if (r.kind == FAULT_STACK) {
        row(g, "记录", "任务栈溢出");
        row(g, "任务", r.task);
    } else if (r.kind == FAULT_HANG) {
        row(g, "记录", "任务没有响应，停止喂狗");
        row(g, "任务", r.task);
    } else {
        row(g, "记录", "没有故障记录");
    }
}
// [endregion]

static void device_build(void)
{
    char line[40];
    s_screen = ui_screen();

    lv_obj_t *fw = group("固件");
    lv_snprintf(line, sizeof line, "%d.%d.%d", APP_FW_MAJOR, APP_FW_MINOR, APP_FW_PATCH);
    row(fw, "版本", line);
    row(fw, "编译时间", app_build_stamp);

    lv_obj_t *chip = group("芯片");
    uint32_t uid[3];
    app_chip_uid(uid);
    lv_snprintf(line, sizeof line, "%08X%08X%08X", (unsigned)uid[2], (unsigned)uid[1], (unsigned)uid[0]);
    row(chip, "型号", "STM32F407ZGT6");
    lv_obj_t *u = row(chip, "唯一 ID", line);
    lv_obj_set_style_text_font(u, &lv_font_montserrat_14, 0);   /* 24 个十六进制数字，用小一号的字才放得下 */

    lv_obj_t *run = group("运行");
    s_uptime = row(run, "运行时间", "0h 0m 0s");
    s_tasks = row(run, "任务数", "--");
    s_heap = row(run, "FreeRTOS 剩余堆", "--");
    s_lvmem = row(run, "LVGL 内存池", "--");

    reset_group();
}

static void update(uint32_t now)
{
    char line[40];
    const uint32_t s = now / 1000u;
    lv_snprintf(line, sizeof line, "%uh %um %us", (unsigned)(s / 3600u), (unsigned)(s / 60u % 60u), (unsigned)(s % 60u));
    ui_set_text(s_uptime, line);
    lv_snprintf(line, sizeof line, "%u", (unsigned)app_task_count());
    ui_set_text(s_tasks, line);
    lv_snprintf(line, sizeof line, "%u 字节", (unsigned)app_free_heap());
    ui_set_text(s_heap, line);
    lv_snprintf(line, sizeof line, "%u / %u 字节", (unsigned)g_lv_mem_used, (unsigned)LV_MEM_SIZE);
    ui_set_text(s_lvmem, line);
}

static void device_enter(void)
{
    ui_page_show(&s_screen, device_build);
    update(HAL_GetTick());
}

static void device_leave(void) { ui_page_hide(&s_screen); }

static void device_tick(uint32_t now)
{
    static uint32_t last;
    if (now - last >= 1000u) { last = now; update(now); }
    lv_port_run();
}

const gui_page g_page_device = {"DEVICE", 0, 0, device_enter, device_tick, lv_port_pointer, device_leave};
