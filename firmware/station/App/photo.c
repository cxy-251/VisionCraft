/* 相机照片的 EXIF 和缩略图。文件内容是外来的数据，每个偏移、长度都先查边界再用。
 * 见手册「文件页：目录与照片缩略图」。 */
#include "photo.h"

#include "ff.h"
#include "main.h"
#include "src/libs/tjpgd/tjpgd.h"     /* LVGL 自带的 TJpgDec（ChaN 写的小 JPEG 解码器，lv_conf.h 里 LV_USE_TJPGD 打开） */

#include <string.h>

#if defined(__arm__)
#define EXTSRAM __attribute__((section(".extsram"), aligned(4)))
#else
#define EXTSRAM
#endif

/* 文件开头 68 KB 一次读进外部 SRAM，EXIF 段（APP1，最长 64 KB）就在里面，在内存里找。
 * 从文件第 0 字节读到对齐的缓冲：FatFs 整扇区直接交给 DMA，一条命令读很多块（见 sdcard.c 的对齐要求） */
#define HEAD_SIZE (68u * 1024u)
static uint8_t s_head[HEAD_SIZE] EXTSRAM;
static uint32_t s_pool[3500 / 4 + 1];  /* TJpgDec 的工作区：哈夫曼表、量化表、一个 MCU 的缓冲 */

volatile uint32_t g_photo_reads;       /* 读过几张（给测试读） */
volatile int32_t g_photo_last_rc, g_photo_last_fr;   /* 上一次的结果和 FatFs 的返回值 */
volatile uint32_t g_photo_fails;

// [region tiff]
/* EXIF 里是一个 TIFF 结构：8 字节文件头（字节序 II/MM、42、第一个 IFD 的偏移），
 * 然后是一串 IFD（目录）：2 字节项数 + 每项 12 字节（标签、类型、个数、值或值的偏移）+ 下一个 IFD 的偏移 */
typedef struct {
    const uint8_t *p;           /* TIFF 文件头的位置 */
    uint32_t len;
    int le;                     /* 1 = II（小端），0 = MM（大端） */
} tiff;

static int u16(const tiff *t, uint32_t off, uint32_t *v)
{
    if (off > t->len || t->len - off < 2) return 0;
    const uint8_t *b = t->p + off;
    *v = t->le ? (uint32_t)(b[0] | b[1] << 8) : (uint32_t)(b[0] << 8 | b[1]);
    return 1;
}

static int u32(const tiff *t, uint32_t off, uint32_t *v)
{
    if (off > t->len || t->len - off < 4) return 0;
    const uint8_t *b = t->p + off;
    *v = t->le ? (uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24
               : (uint32_t)b[0] << 24 | (uint32_t)b[1] << 16 | (uint32_t)b[2] << 8 | (uint32_t)b[3];
    return 1;
}

/* 一项的值：4 字节以内直接放在项里，更长的放在别处、项里存偏移 */
static uint32_t value_offset(const tiff *t, uint32_t entry, uint32_t type, uint32_t count)
{
    static const uint8_t kSize[] = {0, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8};   /* 各类型一个值几个字节 */
    const uint32_t bytes = (type < sizeof kSize ? kSize[type] : 1) * count;
    uint32_t off = entry + 8;
    if (bytes > 4 && !u32(t, entry + 8, &off)) return 0xFFFFFFFFu;
    return off;
}

static void read_ascii(const tiff *t, uint32_t off, uint32_t count, char *out, size_t n)
{
    size_t i = 0;
    for (; i + 1 < n && i < count && off + i < t->len && t->p[off + i]; ++i)
        out[i] = (char)t->p[off + i];
    out[i] = 0;
}

static int read_rational(const tiff *t, uint32_t off, uint32_t *num, uint32_t *den)
{
    return u32(t, off, num) && u32(t, off + 4, den) && *den != 0;
}

typedef struct {
    uint32_t exif_ifd, thumb_off, thumb_len, next;
} ifd_result;

/* 走一个 IFD，认识的标签填进 info / r */
static int walk_ifd(const tiff *t, uint32_t ifd, photo_info *info, ifd_result *r)
{
    uint32_t n;
    if (!u16(t, ifd, &n) || n > 200) return 0;
    for (uint32_t i = 0; i < n; ++i) {
        const uint32_t e = ifd + 2 + i * 12;
        uint32_t tag, type, count, v;
        if (!u16(t, e, &tag) || !u16(t, e + 2, &type) || !u32(t, e + 4, &count)) return 0;
        const uint32_t off = value_offset(t, e, type, count);
        switch (tag) {
        case 0x0110: read_ascii(t, off, count, info->model, sizeof info->model); break;          /* 型号 */
        case 0x9003: read_ascii(t, off, count, info->taken, sizeof info->taken); break;          /* 拍摄时间 */
        case 0x8769: if (u32(t, e + 8, &v)) r->exif_ifd = v; break;                              /* Exif 子目录 */
        case 0x0201: if (u32(t, e + 8, &v)) r->thumb_off = v; break;                             /* 缩略图位置 */
        case 0x0202: if (u32(t, e + 8, &v)) r->thumb_len = v; break;                             /* 缩略图长度 */
        case 0x829A: read_rational(t, off, &info->exp_num, &info->exp_den); break;               /* 快门 */
        case 0x8827: if (u16(t, e + 8, &v)) info->iso = (uint16_t)v; break;
        case 0xA002: case 0xA003:                                                                /* 原图宽、高：SHORT 或 LONG */
            if (type == 3 ? u16(t, e + 8, &v) : u32(t, e + 8, &v)) { if (tag == 0xA002) info->full_w = v; else info->full_h = v; }
            break;
        case 0x829D: case 0x920A: {                                                              /* 光圈、焦距 */
            uint32_t a, b;
            if (read_rational(t, off, &a, &b)) {
                const uint16_t x10 = (uint16_t)((a * 10u + b / 2u) / b);
                if (tag == 0x829D) info->fnum_x10 = x10; else info->focal_x10 = x10;
            }
            break;
        }
        }
    }
    if (!u32(t, ifd + 2 + n * 12, &r->next)) r->next = 0;
    return 1;
}
// [endregion]

// [region decode]
/* TJpgDec 要两个回调：要数据时调 infunc（这里从内存里拷），解出一块像素调 outfunc（这里转成 RGB565 存起来） */
typedef struct {
    const uint8_t *src;
    size_t left;
    uint16_t *out;
    int max_w, max_h;
} decode_ctx;

static size_t in_mem(JDEC *jd, uint8_t *buf, size_t n)
{
    decode_ctx *c = jd->device;
    if (n > c->left) n = c->left;
    if (buf) memcpy(buf, c->src, n);   /* buf 为空表示跳过 n 字节 */
    c->src += n;
    c->left -= n;
    return n;
}

static int out_rgb565(JDEC *jd, void *bitmap, JRECT *r)
{
    decode_ctx *c = jd->device;
    const uint8_t *rgb = bitmap;       /* JD_FORMAT 0：每个像素 R、G、B 三个字节 */
    for (int y = r->top; y <= r->bottom; ++y)
        for (int x = r->left; x <= r->right; ++x, rgb += 3)
            if (x < c->max_w && y < c->max_h)
                c->out[y * c->max_w + x] = (uint16_t)((rgb[0] >> 3) << 11 | (rgb[1] >> 2) << 5 | rgb[2] >> 3);
    return 1;                          /* 返回 0 会让解码中止 */
}
// [endregion]

// [region read]
static int photo_read_impl(const char *path, photo_info *info, uint16_t *rgb565, int max_w, int max_h);

int photo_read(const char *path, photo_info *info, uint16_t *rgb565, int max_w, int max_h)
{
    const int rc = photo_read_impl(path, info, rgb565, max_w, max_h);
    g_photo_last_rc = rc;
    if (rc) g_photo_fails++;
    return rc;
}

static int photo_read_impl(const char *path, photo_info *info, uint16_t *rgb565, int max_w, int max_h)
{
    memset(info, 0, sizeof *info);
    static FIL f;
    const uint32_t t0 = HAL_GetTick();
    const FRESULT fo = f_open(&f, path, FA_READ);
    g_photo_last_fr = fo;
    if (fo != FR_OK) return -1;
    UINT n = 0;
    const FRESULT fr = f_read(&f, s_head, HEAD_SIZE, &n);
    g_photo_last_fr = fr;
    f_close(&f);
    info->read_ms = HAL_GetTick() - t0;
    g_photo_reads++;
    if (fr != FR_OK || n < 4 || s_head[0] != 0xFF || s_head[1] != 0xD8) return -2;   /* SOI */
    /* 一段一段往后找：FF xx 长度(2，大端，含这两个字节) 内容…… 找 APP1 且以 "Exif\0\0" 开头的那段 */
    uint32_t pos = 2, len = 0;
    int found = 0;
    for (int seg = 0; seg < 16 && !found && pos + 4 <= n; ++seg) {
        const uint8_t *m = s_head + pos;
        if (m[0] != 0xFF || m[1] == 0xDA) break;           /* 到了图像数据（SOS），后面不会再有 EXIF */
        len = (uint32_t)(m[2] << 8 | m[3]);
        if (len < 2 || pos + 2 + len > n) break;           /* 这一段不完整地在读进来的范围里 */
        found = m[1] == 0xE1 && len >= 16 && memcmp(m + 4, "Exif\0\0", 6) == 0;   /* APP1 也可能是 XMP */
        if (!found) pos += 2 + len;
    }
    if (!found) return -3;
    info->app1_len = len;
    const uint8_t *app1 = s_head + pos + 4;

    tiff t = {app1 + 6, len - 2 - 6, 0};
    if (t.p[0] == 'I' && t.p[1] == 'I') t.le = 1;
    else if (!(t.p[0] == 'M' && t.p[1] == 'M')) return -3;
    uint32_t ifd0;
    if (!u32(&t, 4, &ifd0)) return -3;
    ifd_result r0 = {0}, r1 = {0}, rx = {0};
    if (!walk_ifd(&t, ifd0, info, &r0)) return -3;      /* IFD0：主图的信息 */
    if (r0.exif_ifd) walk_ifd(&t, r0.exif_ifd, info, &rx);   /* Exif 子目录：拍摄参数 */
    if (!r0.next || !walk_ifd(&t, r0.next, info, &r1)) return -4;   /* IFD1：缩略图 */
    if (!r1.thumb_len || r1.thumb_off > t.len || t.len - r1.thumb_off < r1.thumb_len) return -4;
    info->thumb_len = r1.thumb_len;

    const uint32_t t1 = HAL_GetTick();
    JDEC jd;
    decode_ctx c = {t.p + r1.thumb_off, r1.thumb_len, rgb565, max_w, max_h};
    if (jd_prepare(&jd, in_mem, s_pool, sizeof s_pool, &c) != JDR_OK) return -5;
    if (jd.width > max_w || jd.height > max_h) return -5;
    info->w = jd.width;
    info->h = jd.height;
    c.max_w = jd.width;                                    /* 按实际宽度存，一行接一行 */
    if (jd_decomp(&jd, out_rgb565, 0) != JDR_OK) return -5;
    info->decode_ms = HAL_GetTick() - t1;
    return 0;
}
// [endregion]

const char *photo_error_name(int rc)
{
    switch (rc) {
    case 0: return "ok";
    case -1: return "cannot open";
    case -2: return "not a jpeg";
    case -3: return "no exif";
    case -4: return "no thumbnail";
    case -5: return "thumbnail decode failed";
    }
    return "?";
}
