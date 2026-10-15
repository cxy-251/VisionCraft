/* LinkTask：从 RTT 下行缓冲区收字节 → 拆帧 → 执行命令 → 回应答 */
#include "app.h"
#include "fault.h"
#include "perf.h"
#include "clock.h"
#include "scope.h"

#include "main.h"
#include "cmsis_os2.h"
#include "vc_protocol.h"
#include "vc_rtt.h"
#include "eeprom.h"

#include <string.h>

static vc_decoder s_decoder;
extern const char app_build_stamp[];   /* build_stamp.c，每次编译生成 */

static volatile uint32_t s_lastRxTick;

uint32_t app_rx_frames(void) { return s_decoder.frames; }
uint32_t app_last_rx_tick(void) { return s_lastRxTick; }
uint32_t app_rx_errors(void) { return s_decoder.errors; }

static size_t make_info(uint8_t *dst)
{
    vc_info info;
    memset(&info, 0, sizeof(info));
    info.proto_version = VC_PROTO_VERSION;
    info.fw_major = APP_FW_MAJOR;
    info.fw_minor = APP_FW_MINOR;
    info.fw_patch = APP_FW_PATCH;
    info.uptime_ms = HAL_GetTick();
    memcpy(info.uid, (const void *)UID_BASE, sizeof(info.uid));   /* 芯片出厂烧录的 96 位唯一 ID */
    strncpy(info.build, app_build_stamp, sizeof(info.build) - 1);
    return vc_info_write(dst, &info);
}

static void reply(uint8_t cmd, uint8_t seq, uint8_t status, const uint8_t *data, uint16_t len)
{
    static uint8_t buf[1 + VC_MAX_PAYLOAD];   /* 只有 LinkTask 用，不需要锁 */
    buf[0] = status;
    if (len)
        memcpy(&buf[1], data, len);
    app_send(VC_RSP(cmd), seq, buf, (uint16_t)(1 + len));
}

static void handle(const vc_frame *f)
{
    switch (f->type) {
    case VC_CMD_PING:
        reply(f->type, f->seq, VC_OK, f->payload, f->len);
        break;

    case VC_CMD_GET_INFO: {
        uint8_t info[VC_INFO_SIZE];
        reply(f->type, f->seq, VC_OK, info, (uint16_t)make_info(info));
        break;
    }

    case VC_CMD_BEEP:
        if (f->len != 2) {
            reply(f->type, f->seq, VC_ERR_ARGS, NULL, 0);
            break;
        }
        app_beep(vc_get_u16(f->payload));
        reply(f->type, f->seq, VC_OK, NULL, 0);
        break;

    case VC_CMD_SUB_TEL:
        if (f->len != 2) {
            reply(f->type, f->seq, VC_ERR_ARGS, NULL, 0);
            break;
        }
        app_set_telemetry_period(vc_get_u16(f->payload));
        reply(f->type, f->seq, VC_OK, NULL, 0);
        break;

    case VC_CMD_RESULT: {
        vc_result r;
        if (vc_result_read(&r, f->payload, f->len) != 0 || r.defect >= VC_DEFECT_COUNT) {
            reply(f->type, f->seq, VC_ERR_ARGS, NULL, 0);
            break;
        }
        app_post_result(&r);
        reply(f->type, f->seq, VC_OK, NULL, 0);
        break;
    }

    case VC_CMD_IMAGE_BEGIN:
        reply(f->type, f->seq, f->len == 4 ? app_image_begin(vc_get_u16(f->payload), vc_get_u16(f->payload + 2))
                                           : (uint8_t)VC_ERR_ARGS, NULL, 0);
        break;

    case VC_CMD_IMAGE_DATA:
        reply(f->type, f->seq, f->len > 4 ? app_image_data(vc_get_u32(f->payload), f->payload + 4, (uint16_t)(f->len - 4))
                                          : (uint8_t)VC_ERR_ARGS, NULL, 0);
        break;

    case VC_CMD_RECIPE_GET: {
        vc_recipe r;
        const int rc = recipe_load(&r);
        if (rc != 0) {
            reply(f->type, f->seq, rc == -1 ? VC_ERR_EMPTY : VC_ERR_IO, NULL, 0);
            break;
        }
        uint8_t buf[VC_RECIPE_SIZE];
        reply(f->type, f->seq, VC_OK, buf, (uint16_t)vc_recipe_write(buf, &r));
        break;
    }

    case VC_CMD_RECIPE_SET: {
        vc_recipe r;
        if (vc_recipe_read(&r, f->payload, f->len) != 0) {
            reply(f->type, f->seq, VC_ERR_ARGS, NULL, 0);
            break;
        }
        reply(f->type, f->seq, recipe_save(&r) == 0 ? VC_OK : VC_ERR_IO, NULL, 0);
        break;
    }

    // [region shell]
    case VC_CMD_SHELL: {
        /* 接下了就先不回：StationTask 执行完带着输出回应答（同一个 seq）。没接下才在这里回错误 */
        const uint8_t st = app_post_shell(f->seq, f->payload, f->len);
        if (st != VC_OK)
            reply(f->type, f->seq, st, NULL, 0);
        break;
    }
    // [endregion]

    // [region clock]
    case VC_CMD_SET_TIME: {
        vc_datetime t;
        reply(f->type, f->seq, vc_datetime_read(&t, f->payload, f->len) == 0 ?
              app_clock_set(&t) : VC_ERR_ARGS, NULL, 0);
        break;
    }
    case VC_CMD_GET_TIME: {
        app_clock clock;
        uint8_t data[VC_CLOCK_SIZE] = {0};
        const uint8_t rc = f->len ? VC_ERR_ARGS : app_clock_read(&clock);
        if (rc == VC_OK) {
            data[0] = clock.valid; data[1] = clock.source;
            vc_datetime_write(data + 2, &clock.time);
        }
        reply(f->type, f->seq, rc, data, rc == VC_OK ? sizeof data : 0);
        break;
    }
    // [endregion]

    // [region scope]
    case VC_CMD_SCOPE_START: {
        vc_scope_meta meta;
        uint8_t data[VC_SCOPE_META_SIZE];
        const uint8_t rc = f->len == 6 ? app_scope_start(vc_get_u32(f->payload), vc_get_u16(f->payload + 4), &meta) : VC_ERR_ARGS;
        if (rc == VC_OK) vc_scope_meta_write(data, &meta);
        reply(f->type, f->seq, rc, data, rc == VC_OK ? sizeof data : 0);
        break;
    }
    case VC_CMD_SCOPE_READ: {
        static uint8_t data[VC_SCOPE_BLOCK_HEADER + 2*VC_SCOPE_CHUNK_SAMPLES];
        uint16_t len = 0;
        const uint8_t rc = f->len == 8 ? app_scope_read(vc_get_u32(f->payload), vc_get_u16(f->payload+4),
            vc_get_u16(f->payload+6), data, &len) : VC_ERR_ARGS;
        reply(f->type, f->seq, rc, data, len);
        break;
    }
    case VC_CMD_SCOPE_STOP:
        if (!f->len) app_scope_stop();
        reply(f->type, f->seq, f->len ? VC_ERR_ARGS : VC_OK, NULL, 0);
        break;
    // [endregion]

    default:
        if (VC_IS_CMD(f->type))
            reply(f->type, f->seq, VC_ERR_UNKNOWN, NULL, 0);
        break;
    }
}

void LinkTask(void *argument)
{
    (void)argument;
    vc_decoder_init(&s_decoder);
    app_scope_init();

    uint8_t info[VC_INFO_SIZE];
    app_send(VC_EVT_HELLO, 0, info, (uint16_t)make_info(info));

    uint8_t chunk[64];
    for (;;) {
        app_scope_poll();
        if (g_perf_input_request == 1) {
            g_perf_input_cycles = gui_cycles();
            __DMB();
            g_perf_input_request = 2;
        }
        health_checkin(HEALTH_LINK);   /* 看门狗监工：我还活着 */
        const size_t n = vc_rtt_read(chunk, sizeof(chunk));
        vc_frame f;
        for (size_t i = 0; i < n; ++i) {
            if (vc_decoder_feed(&s_decoder, chunk[i], &f)) {
                s_lastRxTick = HAL_GetTick();
                handle(&f);
            }
        }
        if (n == 0)
            osDelay(1);   /* 没数据就让出 CPU；调试器每毫秒左右轮询一次，再快也没意义 */
    }
}
