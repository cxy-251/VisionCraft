/* 相机照片：从 JPEG 文件开头的 EXIF 里取拍摄参数和嵌在里面的小缩略图，解码成 RGB565。只读。
 * 只在 StationTask 里调用。见手册「文件页：目录与照片缩略图」。 */
#ifndef VC_PHOTO_H
#define VC_PHOTO_H

#include <stdint.h>

typedef struct {
    char     model[32];         /* 相机型号（IFD0 0x0110） */
    char     taken[20];         /* 拍摄时间 "2025:03:14 15:09:26"（Exif 0x9003） */
    uint32_t exp_num, exp_den;  /* 快门（Exif 0x829A），秒 = num / den */
    uint16_t fnum_x10;          /* 光圈 ×10（Exif 0x829D） */
    uint16_t iso;               /* Exif 0x8827 */
    uint16_t focal_x10;         /* 焦距 ×10 mm（Exif 0x920A） */
    uint32_t full_w, full_h;    /* 原图的宽高（Exif 0xA002 / 0xA003） */
    uint32_t app1_len;          /* EXIF 段多大 */
    uint32_t thumb_len;         /* 缩略图（一张小 JPEG）多大 */
    uint16_t w, h;              /* 缩略图解码后的宽高 */
    uint32_t read_ms, decode_ms;
} photo_info;

/* 读 path 的 EXIF，把缩略图解码进 rgb565（最大 max_w × max_h）。
 * 返回 0 成功；负数：-1 打不开，-2 不是 JPEG，-3 没有 EXIF，-4 没有缩略图，-5 缩略图解码失败或太大 */
int photo_read(const char *path, photo_info *info, uint16_t *rgb565, int max_w, int max_h);
const char *photo_error_name(int rc);

#endif
