#include "vc_rtt.h"

#include "cmsis_os2.h"
#include "stm32f4xx.h"

#include <string.h>

#define UP_SIZE   8192u
#define DOWN_SIZE 8192u   /* 上位机可能一次发来多个最大帧（4 × 1034 字节），放不下的部分 OpenOCD 会直接丢弃 */

/* 布局必须和 SEGGER RTT 完全一致，调试器按固定偏移读取这些字段 */
typedef struct {
    const char *name;
    char *buffer;
    uint32_t size;
    volatile uint32_t wr_off;
    volatile uint32_t rd_off;
    uint32_t flags;
} rtt_buffer;

typedef struct {
    char id[16];                /* "SEGGER RTT"，调试器靠它在内存里找到控制块 */
    int32_t max_up;
    int32_t max_down;
    rtt_buffer up[1];
    rtt_buffer down[1];
} rtt_control_block;

static rtt_control_block s_cb;
static char s_up[UP_SIZE];
static char s_down[DOWN_SIZE];
static volatile uint32_t s_dropped;

void vc_rtt_init(void)
{
    s_cb.max_up = 1;
    s_cb.max_down = 1;
    s_cb.up[0] = (rtt_buffer){"vc", s_up, UP_SIZE, 0, 0, 0};
    s_cb.down[0] = (rtt_buffer){"vc", s_down, DOWN_SIZE, 0, 0, 0};

    /* 最后才写标识：调试器一旦搜到 "SEGGER RTT"，后面的字段必须已经有效。
     * 分两段拷贝，避免编译器把完整字符串常量放进 RAM 被误认成第二个控制块 */
    __DMB();
    memcpy(&s_cb.id[7], "RTT", 4);
    __DMB();
    memcpy(&s_cb.id[0], "SEGGER ", 7);
    __DMB();
}

static uint32_t up_free(void)
{
    const uint32_t wr = s_cb.up[0].wr_off;
    const uint32_t rd = s_cb.up[0].rd_off;
    /* 环形缓冲区留一个字节不用，用来区分「满」和「空」 */
    return rd > wr ? rd - wr - 1u : UP_SIZE - (wr - rd) - 1u;
}

size_t vc_rtt_write(const void *data, size_t len, uint32_t timeout_ms)
{
    if (len >= UP_SIZE)
        return 0;
    uint32_t waited = 0;
    while (up_free() < len) {
        if (waited >= timeout_ms) {
            s_dropped += (uint32_t)len;
            return 0;
        }
        osDelay(1);
        waited++;
    }

    const uint8_t *src = data;
    uint32_t wr = s_cb.up[0].wr_off;
    const uint32_t first = (UP_SIZE - wr) < len ? (UP_SIZE - wr) : (uint32_t)len;
    memcpy(&s_up[wr], src, first);
    memcpy(&s_up[0], src + first, len - first);
    wr = (wr + (uint32_t)len) % UP_SIZE;
    __DMB();                    /* 数据先写完，再移动写指针 */
    s_cb.up[0].wr_off = wr;
    return len;
}

size_t vc_rtt_read(void *dst, size_t cap)
{
    const uint32_t wr = s_cb.down[0].wr_off;
    uint32_t rd = s_cb.down[0].rd_off;
    uint8_t *out = dst;
    size_t n = 0;
    while (rd != wr && n < cap) {
        out[n++] = (uint8_t)s_down[rd];
        rd = (rd + 1u) % DOWN_SIZE;
    }
    __DMB();                    /* 数据先读完，再告诉调试器这块空间可以复用 */
    s_cb.down[0].rd_off = rd;
    return n;
}

uint32_t vc_rtt_dropped(void)
{
    return s_dropped;
}
