#include "nor.h"
#include <string.h>

static int transaction(nor_device *d, const uint8_t *command, size_t len, uint8_t *data, size_t n, int write)
{
    uint8_t discard[NOR_PAGE];
    d->bus.select(d->bus.ctx, 1);
    int rc = d->bus.transfer(d->bus.ctx, command, discard, len);
    if (!rc && n) {
        if (!write) memset(data, 0xff, n);
        /* HAL 支持原位全双工；写操作丢弃返回字节，保留调用方数据。 */
        rc = d->bus.transfer(d->bus.ctx, data, write ? discard : data, n);
    }
    d->bus.select(d->bus.ctx, 0); /* 所有错误路径都释放 CS */
    return rc ? NOR_IO : NOR_OK;
}
static int range(nor_device *d, uint32_t addr, size_t n)
{
    return d->capacity && addr < d->capacity && n && n <= d->capacity-addr;
}
int nor_probe(nor_device *d, const nor_bus *bus)
{
    memset(d, 0, sizeof *d);
    if (!bus || !bus->select || !bus->transfer) return NOR_ARGS;
    d->bus = *bus;
    const uint8_t command = 0x9f;
    uint8_t id[3];
    const int rc = transaction(d, &command, 1, id, 3, 0);
    if (rc) return rc;
    d->id = (uint32_t)id[0]<<16 | (uint32_t)id[1]<<8 | id[2];
    if (d->id != 0xef4018u) return NOR_ID;
    d->capacity = NOR_CAPACITY;
    return NOR_OK;
}
int nor_status(nor_device *d, uint8_t *status)
{
    if (!d->capacity || !status) return NOR_ARGS;
    const uint8_t command = 0x05;
    return transaction(d, &command, 1, status, 1, 0);
}
int nor_read(nor_device *d, uint32_t addr, uint8_t *out, size_t n)
{
    if (!out || n > NOR_PAGE || !range(d, addr, n)) return NOR_ARGS;
    uint8_t status;
    int rc = nor_status(d, &status);
    if (rc) return rc;
    if (status & 1u) return NOR_BUSY;
    const uint8_t command[4] = {0x03, addr>>16, addr>>8, addr};
    return transaction(d, command, sizeof command, out, n, 0);
}
int nor_reserve(nor_device *d, uint32_t base, uint32_t size)
{
    if (!range(d, base, size) || base % NOR_SECTOR || size % NOR_SECTOR) return NOR_ARGS;
    d->write_base = base; d->write_size = size;
    return NOR_OK;
}
static uint32_t le16(const uint8_t *p) { return p[0] | (uint32_t)p[1]<<8; }
static uint32_t le32(const uint8_t *p) { return le16(p) | le16(p+2)<<16; }
int nor_reserve_from_fat(nor_device *d, const uint8_t boot[512], uint32_t *volume_bytes)
{
    d->write_base=d->write_size=0;
    if (volume_bytes) *volume_bytes=0;
    if (!boot || !volume_bytes || d->capacity!=NOR_CAPACITY || boot[510]!=0x55 || boot[511]!=0xaa ||
        (boot[0]!=0xeb && boot[0]!=0xe9) || le16(boot+11)!=512) return NOR_PROTECTED;
    const uint32_t spc=boot[13], reserved=le16(boot+14), fats=boot[16], roots=le16(boot+17), fat=le16(boot+22);
    const uint32_t total=le16(boot+19) ? le16(boot+19) : le32(boot+32);
    if (!spc || (spc&(spc-1)) || spc>128 || !reserved || (fats!=1 && fats!=2) || !roots || !fat ||
        !total || total>d->capacity/512 || reserved+fats*fat+(roots*32+511)/512>=total) return NOR_PROTECTED;
    *volume_bytes=total*512;
    if (*volume_bytes>NOR_CAPACITY-NOR_SECTOR) return NOR_PROTECTED;
    return nor_reserve(d,NOR_CAPACITY-NOR_SECTOR,NOR_SECTOR);
}
static int writable(nor_device *d, uint32_t addr, size_t n)
{
    return range(d, addr, n) && d->write_size && addr >= d->write_base &&
           addr-d->write_base < d->write_size && n <= d->write_size-(addr-d->write_base);
}
static int enable_write(nor_device *d)
{
    uint8_t status;
    int rc = nor_status(d, &status);
    if (rc) return rc;
    if (status & 1u) return NOR_BUSY;
    if (status & 0x1cu) return NOR_PROTECTED; /* 不自动解除芯片块保护 */
    const uint8_t command = 0x06;
    rc = transaction(d, &command, 1, 0, 0, 0);
    if (rc) return rc;
    rc = nor_status(d, &status);
    return rc ? rc : status & 2u ? NOR_OK : NOR_IO;
}
int nor_erase_begin(nor_device *d, uint32_t addr)
{
    if (addr % NOR_SECTOR || !writable(d, addr, NOR_SECTOR)) return NOR_ARGS;
    int rc = enable_write(d);
    if (rc) return rc;
    const uint8_t command[4] = {0x20, addr>>16, addr>>8, addr};
    return transaction(d, command, sizeof command, 0, 0, 0);
}
int nor_program_begin(nor_device *d, uint32_t addr, const uint8_t *data, size_t n)
{
    if (!data || n > NOR_PAGE || n > NOR_PAGE-addr%NOR_PAGE || !writable(d, addr, n)) return NOR_ARGS;
    int rc = enable_write(d);
    if (rc) return rc;
    const uint8_t command[4] = {0x02, addr>>16, addr>>8, addr};
    return transaction(d, command, sizeof command, (uint8_t *)data, n, 1);
}
