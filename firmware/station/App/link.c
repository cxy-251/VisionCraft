/* LinkTask：从 RTT 下行缓冲区收字节 → 拆帧 → 执行命令 → 回应答 */
#include "app.h"

#include "main.h"
#include "cmsis_os2.h"
#include "vc_protocol.h"
#include "vc_rtt.h"

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

    default:   /* 包括 SET_TIME：RTC 还没配置 */
        if (VC_IS_CMD(f->type))
            reply(f->type, f->seq, VC_ERR_UNKNOWN, NULL, 0);
        break;
    }
}

void LinkTask(void *argument)
{
    (void)argument;
    vc_decoder_init(&s_decoder);

    uint8_t info[VC_INFO_SIZE];
    app_send(VC_EVT_HELLO, 0, info, (uint16_t)make_info(info));

    uint8_t chunk[64];
    for (;;) {
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
