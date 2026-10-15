#ifndef VC_NOR_H
#define VC_NOR_H
#include <stddef.h>
#include <stdint.h>
#define NOR_CAPACITY (16u * 1024u * 1024u)
#define NOR_SECTOR 4096u
#define NOR_PAGE 256u
enum { NOR_OK, NOR_ARGS, NOR_IO, NOR_BUSY, NOR_ID, NOR_PROTECTED };
typedef struct {
    void *ctx;
    void (*select)(void *, int selected);
    int (*transfer)(void *, const uint8_t *tx, uint8_t *rx, size_t n);
} nor_bus;
typedef struct {
    nor_bus bus;
    uint32_t id, capacity, write_base, write_size;
} nor_device;
int nor_probe(nor_device *d, const nor_bus *bus);
int nor_status(nor_device *d, uint8_t *status);
int nor_read(nor_device *d, uint32_t addr, uint8_t *out, size_t n);
/* 唯一可写区必须显式设置，未设置时完全只读；没有整片擦除/任意写入口。 */
int nor_reserve(nor_device *d, uint32_t base, uint32_t size);
/* 只识别有效的 FAT12/16 超级软盘 BPB；卷末之后才允许保留最后一扇区。未知布局保持只读。 */
int nor_reserve_from_fat(nor_device *d, const uint8_t boot[512], uint32_t *volume_bytes);
int nor_erase_begin(nor_device *d, uint32_t addr);
int nor_program_begin(nor_device *d, uint32_t addr, const uint8_t *data, size_t n);
#endif
