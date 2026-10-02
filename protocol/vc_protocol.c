/* VisionCraft 上下位机协议：校验、组帧、拆帧。上位机与固件编译的是同一个文件。 */
#include "vc_protocol.h"

#include <string.h>

/* ---------------- 校验 ---------------- */

uint8_t vc_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0;
    while (len--) {
        crc ^= *data++;
        for (int i = 0; i < 8; ++i)
            crc = (uint8_t)((crc & 0x80u) ? (crc << 1) ^ 0x07u : crc << 1);
    }
    return crc;
}

uint16_t vc_crc16(const uint8_t *data, size_t len, uint16_t crc)
{
    while (len--) {
        crc ^= (uint16_t)(*data++) << 8;
        for (int i = 0; i < 8; ++i)
            crc = (uint16_t)((crc & 0x8000u) ? (crc << 1) ^ 0x1021u : crc << 1);
    }
    return crc;
}

/* ---------------- 组帧 ---------------- */

size_t vc_encode(uint8_t *dst, size_t cap, uint8_t type, uint8_t seq, const uint8_t *payload, uint16_t len)
{
    const size_t total = VC_HEADER_SIZE + len + VC_TRAILER_SIZE;
    if (len > VC_MAX_PAYLOAD || cap < total)
        return 0;

    dst[0] = VC_SOF0;
    dst[1] = VC_SOF1;
    dst[2] = VC_PROTO_VERSION;
    dst[3] = type;
    dst[4] = seq;
    vc_put_u16(&dst[5], len);
    dst[7] = vc_crc8(&dst[2], 5);
    if (len)
        memcpy(&dst[VC_HEADER_SIZE], payload, len);
    vc_put_u16(&dst[VC_HEADER_SIZE + len], vc_crc16(&dst[2], 6u + len, 0xFFFFu));
    return total;
}

/* ---------------- 拆帧 ---------------- */

enum { S_SOF0, S_SOF1, S_HEADER, S_BODY };

void vc_decoder_init(vc_decoder *d)
{
    memset(d, 0, sizeof(*d));
    d->state = S_SOF0;
}

static void reset(vc_decoder *d)
{
    d->state = S_SOF0;
    d->pos = 0;
}

int vc_decoder_feed(vc_decoder *d, uint8_t byte, vc_frame *out)
{
    switch (d->state) {
    case S_SOF0:
        if (byte == VC_SOF0) {
            d->buf[0] = byte;
            d->pos = 1;
            d->state = S_SOF1;
        }
        return 0;

    case S_SOF1:
        if (byte == VC_SOF1) {
            d->buf[1] = byte;
            d->pos = 2;
            d->state = S_HEADER;
        } else if (byte != VC_SOF0) {   /* A5 A5 5A 也要能对上：第二个 A5 当作新的帧头 */
            reset(d);
        }
        return 0;

    case S_HEADER:
        d->buf[d->pos++] = byte;
        if (d->pos < VC_HEADER_SIZE)
            return 0;
        d->len = vc_get_u16(&d->buf[5]);
        if (vc_crc8(&d->buf[2], 5) != d->buf[7] || d->buf[2] != VC_PROTO_VERSION || d->len > VC_MAX_PAYLOAD) {
            d->errors++;
            reset(d);
            return 0;
        }
        d->state = S_BODY;
        return 0;

    case S_BODY:
        d->buf[d->pos++] = byte;
        if (d->pos < VC_HEADER_SIZE + d->len + VC_TRAILER_SIZE)
            return 0;
        {
            const uint16_t expect = vc_get_u16(&d->buf[VC_HEADER_SIZE + d->len]);
            const uint16_t actual = vc_crc16(&d->buf[2], 6u + d->len, 0xFFFFu);
            reset(d);
            if (expect != actual) {
                d->errors++;
                return 0;
            }
        }
        d->frames++;
        out->type = d->buf[3];
        out->seq = d->buf[4];
        out->len = d->len;
        out->payload = &d->buf[VC_HEADER_SIZE];
        return 1;
    }
    reset(d);
    return 0;
}

/* ---------------- 负载 ---------------- */

size_t vc_info_write(uint8_t *dst, const vc_info *info)
{
    uint8_t *p = dst;
    *p++ = info->proto_version;
    vc_put_u16(p, info->fw_major); p += 2;
    vc_put_u16(p, info->fw_minor); p += 2;
    vc_put_u16(p, info->fw_patch); p += 2;
    vc_put_u32(p, info->uptime_ms); p += 4;
    memcpy(p, info->uid, sizeof(info->uid)); p += sizeof(info->uid);
    memcpy(p, info->build, sizeof(info->build)); p += sizeof(info->build);
    return (size_t)(p - dst);
}

int vc_info_read(vc_info *info, const uint8_t *src, size_t len)
{
    if (len < VC_INFO_SIZE)
        return -1;
    const uint8_t *p = src;
    info->proto_version = *p++;
    info->fw_major = vc_get_u16(p); p += 2;
    info->fw_minor = vc_get_u16(p); p += 2;
    info->fw_patch = vc_get_u16(p); p += 2;
    info->uptime_ms = vc_get_u32(p); p += 4;
    memcpy(info->uid, p, sizeof(info->uid)); p += sizeof(info->uid);
    memcpy(info->build, p, sizeof(info->build));
    info->build[sizeof(info->build) - 1] = '\0';
    return 0;
}

size_t vc_tel_env_write(uint8_t *dst, const vc_tel_env *t)
{
    uint8_t *p = dst;
    vc_put_u16(p, (uint16_t)t->cpu_temp_c100); p += 2;
    vc_put_u16(p, t->light_permille); p += 2;
    vc_put_u16(p, t->vref_mv); p += 2;
    vc_put_u32(p, t->uptime_ms); p += 4;
    vc_put_u32(p, t->rx_frames); p += 4;
    vc_put_u32(p, t->rx_errors); p += 4;
    return (size_t)(p - dst);
}

int vc_tel_env_read(vc_tel_env *t, const uint8_t *src, size_t len)
{
    if (len < VC_TEL_ENV_SIZE)
        return -1;
    const uint8_t *p = src;
    t->cpu_temp_c100 = (int16_t)vc_get_u16(p); p += 2;
    t->light_permille = vc_get_u16(p); p += 2;
    t->vref_mv = vc_get_u16(p); p += 2;
    t->uptime_ms = vc_get_u32(p); p += 4;
    t->rx_frames = vc_get_u32(p); p += 4;
    t->rx_errors = vc_get_u32(p);
    return 0;
}
