/*
 * VisionCraft 上下位机协议（上位机与 F407 固件共用这一份）
 *
 * 帧格式（多字节字段一律小端）：
 *
 *   +------+------+-----+------+-----+---------+------+-----------------+---------+
 *   | 0xA5 | 0x5A | ver | type | seq | len(2)  | hcrc | payload(len)    | crc(2)  |
 *   +------+------+-----+------+-----+---------+------+-----------------+---------+
 *     帧头（2）      版本  类型   序号  负载长度   头校验  负载               全帧校验
 *
 *   hcrc：ver..len 五个字节的 CRC-8。头部一旦损坏（尤其是 len），立刻丢弃，
 *         不会因为一个错误的长度吞掉后面最多 1 KB 的正常数据。
 *   crc ：ver..payload 的 CRC-16/CCITT-FALSE。
 *
 * 消息分四类，靠 type 的取值区间区分：
 *   CMD 0x01–0x3F  上位机 → 板子，请求
 *   RSP 0x81–0xBF  板子 → 上位机，应答 = 对应 CMD | 0x80，seq 与请求相同，负载第一个字节是状态码
 *   EVT 0x40–0x5F  板子 → 上位机，事件（按键等），不需要应答
 *   TEL 0x60–0x7F  板子 → 上位机，周期遥测
 */
#ifndef VC_PROTOCOL_H
#define VC_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VC_PROTO_VERSION    1u
#define VC_SOF0             0xA5u
#define VC_SOF1             0x5Au
#define VC_HEADER_SIZE      8u      /* SOF(2) ver type seq len(2) hcrc */
#define VC_TRAILER_SIZE     2u      /* crc16 */
#define VC_MAX_PAYLOAD      1024u
#define VC_MAX_FRAME        (VC_HEADER_SIZE + VC_MAX_PAYLOAD + VC_TRAILER_SIZE)

/* ---------------- 消息类型 ---------------- */
enum vc_type {
    /* CMD：请求 */
    VC_CMD_PING      = 0x01, /* 负载原样返回，用来测延迟和吞吐 */
    VC_CMD_GET_INFO  = 0x02, /* 应答见 vc_info */
    VC_CMD_SET_TIME  = 0x03, /* 负载：year(2) month day hour minute second weekday */
    VC_CMD_BEEP      = 0x04, /* 负载：duration_ms(2) */
    VC_CMD_SUB_TEL   = 0x05, /* 负载：period_ms(2)，0 = 停止遥测 */
    VC_CMD_RESULT    = 0x06, /* 负载见 vc_result：一次检测的结果，板子显示并报警 */
    VC_CMD_IMAGE_BEGIN = 0x07, /* 负载：width(2) height(2)，随后用 IMAGE_DATA 送 width×height 个 RGB565 像素 */
    VC_CMD_IMAGE_DATA  = 0x08, /* 负载：offset(4，字节) + 像素数据；全部到齐后板子显示 */

    /* EVT：事件 */
    VC_EVT_HELLO     = 0x40, /* 板子启动完成，负载同 vc_info */
    VC_EVT_KEY       = 0x41, /* 负载：key(1) action(1) */
    VC_EVT_LOG       = 0x42, /* 负载：UTF-8 文本 */

    /* TEL：遥测 */
    VC_TEL_ENV       = 0x60  /* 负载见 vc_tel_env */
};

#define VC_RSP(cmd)      ((uint8_t)((cmd) | 0x80u))
#define VC_IS_CMD(t)     ((t) >= 0x01u && (t) <= 0x3Fu)
#define VC_IS_RSP(t)     ((t) >= 0x81u && (t) <= 0xBFu)
#define VC_IS_EVT(t)     ((t) >= 0x40u && (t) <= 0x5Fu)
#define VC_IS_TEL(t)     ((t) >= 0x60u && (t) <= 0x7Fu)

/* 应答状态码：RSP 负载的第一个字节 */
enum vc_status {
    VC_OK            = 0,
    VC_ERR_ARGS      = 1,  /* 参数长度或取值不对 */
    VC_ERR_UNKNOWN   = 2,  /* 不认识这个命令 */
    VC_ERR_BUSY      = 3
};

/* 缺陷类型（检测结果） */
enum vc_defect {
    VC_DEFECT_NONE    = 0,
    VC_DEFECT_SCRATCH = 1,   /* 划痕 */
    VC_DEFECT_CHIP    = 2,   /* 缺口 */
    VC_DEFECT_SPOT    = 3,   /* 污点 */
    VC_DEFECT_OFFSET  = 4,   /* 内孔偏心 */
    VC_DEFECT_MISSING = 5,   /* 没找到工件 */
    VC_DEFECT_COUNT
};

/* 按键事件 */
enum vc_key    { VC_KEY0 = 0, VC_KEY1 = 1, VC_KEY2 = 2, VC_KEY_WKUP = 3 };
enum vc_action { VC_KEY_DOWN = 0, VC_KEY_UP = 1 };

/* ---------------- 负载结构 ----------------
 * 结构体只用来在代码里集中描述字段；收发时用下面的 vc_put/vc_get 逐字段读写，
 * 不直接 memcpy 整个结构体——两端的编译器对齐和字节序不一定一样。 */

typedef struct {
    uint8_t  proto_version;
    uint16_t fw_major, fw_minor, fw_patch;
    uint32_t uptime_ms;
    uint8_t  uid[12];          /* 芯片唯一 ID */
    char     build[24];        /* 编译时间 "Oct  3 2026 01:23:45"，以 0 结尾 */
} vc_info;
#define VC_INFO_SIZE (1u + 2u * 3u + 4u + 12u + 24u)

typedef struct {
    uint8_t  ok;               /* 1 = 合格 */
    uint8_t  defect;           /* enum vc_defect */
    uint16_t inspect_ms;       /* 上位机检测耗时 */
    uint32_t total;            /* 本次运行累计检测件数 */
    uint32_t ng;               /* 其中不合格件数 */
} vc_result;
#define VC_RESULT_SIZE (1u + 1u + 2u + 4u + 4u)

/* 下发到板子屏幕的缩略图：固定最大尺寸，板子上预留这么大的缓冲区 */
#define VC_THUMB_MAX_W 160u
#define VC_THUMB_MAX_H 120u
#define VC_IMAGE_CHUNK 1000u     /* 每个 IMAGE_DATA 帧最多带的像素字节数 */

typedef struct {
    int16_t  cpu_temp_c100;    /* 片内温度 ×100，例如 3512 = 35.12 °C */
    uint16_t light_permille;   /* 光照 0–1000 */
    uint16_t vref_mv;          /* 由内部参考电压反推的 VDDA，毫伏 */
    uint32_t uptime_ms;
    uint32_t rx_frames;        /* 板子收到的有效帧数 */
    uint32_t rx_errors;        /* 板子丢弃的坏帧数 */
} vc_tel_env;
#define VC_TEL_ENV_SIZE (2u + 2u + 2u + 4u + 4u + 4u)

/* ---------------- 小端读写 ---------------- */
static inline void vc_put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static inline void vc_put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static inline uint16_t vc_get_u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static inline uint32_t vc_get_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

size_t vc_info_write(uint8_t *dst, const vc_info *info);           /* 返回写入字节数 */
int    vc_info_read(vc_info *info, const uint8_t *src, size_t len); /* 成功返回 0 */
size_t vc_result_write(uint8_t *dst, const vc_result *r);
int    vc_result_read(vc_result *r, const uint8_t *src, size_t len);
size_t vc_tel_env_write(uint8_t *dst, const vc_tel_env *t);
int    vc_tel_env_read(vc_tel_env *t, const uint8_t *src, size_t len);

/* ---------------- 校验 ---------------- */
uint8_t  vc_crc8(const uint8_t *data, size_t len);                   /* poly 0x07, init 0 */
uint16_t vc_crc16(const uint8_t *data, size_t len, uint16_t crc);   /* CCITT-FALSE，首次传 0xFFFF */

/* ---------------- 组帧 ---------------- */
/* 把一帧写进 dst，返回帧长；cap 不够或 len 超限返回 0 */
size_t vc_encode(uint8_t *dst, size_t cap, uint8_t type, uint8_t seq, const uint8_t *payload, uint16_t len);

/* ---------------- 拆帧 ----------------
 * 字节流状态机：一个字节一个字节地喂，拼出完整帧时返回 1。
 * 任何一步出错就丢弃已收的字节、重新找帧头，所以数据流中途接入、丢字节、
 * 混进杂散字节都能自动恢复同步。 */
typedef struct {
    uint8_t  type;
    uint8_t  seq;
    uint16_t len;
    const uint8_t *payload;   /* 指向解码器内部缓冲区，下次 feed 之前有效 */
} vc_frame;

typedef struct {
    uint8_t  state;
    uint16_t pos;
    uint16_t len;
    uint8_t  buf[VC_MAX_FRAME];
    uint32_t frames;          /* 收到的有效帧 */
    uint32_t errors;          /* 丢弃的坏帧（头校验、长度、全帧校验、版本不对） */
} vc_decoder;

void vc_decoder_init(vc_decoder *d);
int  vc_decoder_feed(vc_decoder *d, uint8_t byte, vc_frame *out);

#ifdef __cplusplus
}
#endif

#endif /* VC_PROTOCOL_H */
