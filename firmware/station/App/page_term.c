/* 「终端」页：上面是输出，中间一排常用命令和输入行，下面是 LVGL 自带的键盘。命令由 shell.c 执行。
 * 见手册「终端页：命令解析与 LVGL 键盘」。 */
#include "pages.h"

#include "lv_port.h"
#include "lv_ui.h"
#include "shell.h"

#include <string.h>

LV_FONT_DECLARE(vc_font_mono_16)       /* tools/make_cjk_font.sh 从 Noto Sans Mono 生成：等宽 ASCII */

#define LOG_SIZE 2048u                 /* 输出最多留这么多字符，满了丢掉最早的行 */

char g_term_log[LOG_SIZE];             /* 输出的全部文字（给测试读） */
volatile uint32_t g_term_runs;         /* 执行过几条命令 */
char g_term_inject[64];                /* 调试器写一条命令进来 …… */
volatile uint32_t g_term_inject_seq;   /* …… 再把这个加 1，下一轮就当作敲了回车执行 */

static lv_obj_t *s_screen, *s_out, *s_outBox, *s_input, *s_kb;
static size_t s_len;

// [region log]
static void append(const char *line, void *ctx)
{
    (void)ctx;
    const size_t n = strlen(line) + 1;           /* 加一个换行 */
    if (n >= LOG_SIZE) return;
    while (s_len + n >= LOG_SIZE) {             /* 放不下：从头丢掉整行，直到放得下 */
        char *nl = memchr(g_term_log, '\n', s_len);
        const size_t cut = nl ? (size_t)(nl - g_term_log) + 1 : s_len;
        memmove(g_term_log, g_term_log + cut, s_len - cut);
        s_len -= cut;
    }
    memcpy(g_term_log + s_len, line, n - 1);
    s_len += n;
    g_term_log[s_len - 1] = '\n';
    g_term_log[s_len] = 0;
}

static void show_log(void)
{
    if (!s_out) return;
    lv_label_set_text_static(s_out, g_term_log);   /* 文字就在 g_term_log 里，LVGL 不另外拷一份 */
    lv_obj_update_layout(s_outBox);
    lv_obj_scroll_to_y(s_outBox, LV_COORD_MAX, LV_ANIM_OFF);   /* 滚到最后一行 */
}

static void run(const char *cmd)
{
    char echo[72];
    lv_snprintf(echo, sizeof echo, "> %s", cmd);
    append(echo, 0);
    shell_exec(cmd, append, 0);
    g_term_runs++;
    show_log();
}
// [endregion]

// [region input]
static void on_ready(lv_event_t *e)            /* 键盘上的 ✓ / 回车 */
{
    (void)e;
    char cmd[64];
    strncpy(cmd, lv_textarea_get_text(s_input), sizeof cmd - 1);
    cmd[sizeof cmd - 1] = 0;
    lv_textarea_set_text(s_input, "");
    if (cmd[0]) run(cmd);
}

static void on_quick(lv_event_t *e)            /* 常用命令按钮：按钮上的字就是命令 */
{
    lv_obj_t *btn = lv_event_get_target_obj(e);
    run(lv_label_get_text(lv_obj_get_child(btn, 0)));
}
// [endregion]

static void term_build(void)
{
    s_screen = ui_screen();
    lv_obj_set_style_pad_row(s_screen, 8, 0);
    lv_obj_set_scrollable(s_screen, false);

    /* 输出：等宽字体，表格才对得齐 */
    s_outBox = ui_card(s_screen, LV_PCT(100), 0);
    lv_obj_set_flex_grow(s_outBox, 1);
    lv_obj_set_scrollable(s_outBox, true);
    s_out = lv_label_create(s_outBox);
    lv_obj_set_width(s_out, LV_PCT(100));
    lv_obj_set_style_text_font(s_out, &vc_font_mono_16, 0);
    lv_obj_set_style_text_color(s_out, lv_color_hex(UI_TEXT), 0);
    if (s_len == 0)
        append("type a command, or tap one below", 0);

    lv_obj_t *quick = ui_row(s_screen);
    static const char *const kQuick[] = {"help", "ps", "free", "sensors", "last"};
    for (unsigned i = 0; i < sizeof kQuick / sizeof kQuick[0]; ++i) {
        lv_obj_t *b = lv_button_create(quick);
        lv_obj_set_height(b, 44);
        lv_obj_add_event_cb(b, on_quick, LV_EVENT_CLICKED, NULL);
        lv_label_set_text(lv_label_create(b), kQuick[i]);
    }

    s_input = lv_textarea_create(s_screen);
    lv_obj_set_width(s_input, LV_PCT(100));
    lv_textarea_set_one_line(s_input, true);
    lv_textarea_set_placeholder_text(s_input, "command");
    lv_textarea_set_max_length(s_input, 63);
    lv_obj_add_event_cb(s_input, on_ready, LV_EVENT_READY, NULL);

    s_kb = lv_keyboard_create(s_screen);
    lv_obj_set_size(s_kb, LV_PCT(100), 250);
    lv_keyboard_set_mode(s_kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_textarea(s_kb, s_input);   /* 键盘敲的字进这个输入框，✓ 键让输入框发 READY */
}

static void term_enter(void)
{
    ui_page_show(&s_screen, term_build);
    show_log();                                /* 输出在 g_term_log 里，离开再回来也还在 */
}

static void term_leave(void)
{
    ui_page_hide(&s_screen);
    s_out = s_outBox = s_input = s_kb = NULL;  /* 这些控件随页面一起删掉了 */
}

static void term_tick(uint32_t now)
{
    (void)now;
    static uint32_t seen;
    if (g_term_inject_seq != seen) {           /* 调试器塞进来的命令 */
        seen = g_term_inject_seq;
        g_term_inject[sizeof g_term_inject - 1] = 0;
        run(g_term_inject);
    }
    lv_port_run();
}

const gui_page g_page_term = {"TERMINAL", 0, 0, term_enter, term_tick, lv_port_pointer, term_leave};
