#include "flash_diag.h"
#include "spiflash.h"
#include <string.h>

#define BASE (NOR_CAPACITY-NOR_SECTOR)
#if defined(__arm__)
#define EXTERNAL __attribute__((section(".extsram"), aligned(4)))
#else
#define EXTERNAL
#endif
static uint8_t s_saved[NOR_SECTOR] EXTERNAL;
flash_diag_status g_flash_diag;
static uint8_t pattern(uint32_t off) { return (uint8_t)((off*37u+11u) ^ (off>>8)); }
static uint32_t checksum(uint32_t crc, const uint8_t *data, size_t n)
{
    while (n--) { crc ^= *data++; for (unsigned k=0;k<8;++k) crc = (crc>>1) ^ (0xedb88320u & (0u-(crc&1))); }
    return crc;
}
int flash_diag_active(void) { return g_flash_diag.phase >= FD_BACKUP && g_flash_diag.phase <= FD_RESTORE_VERIFY; }
int flash_diag_start(uint32_t now)
{
    if (flash_diag_active() || g_flash_diag.saved_valid || g_flash.write_base != BASE || g_flash.write_size != NOR_SECTOR) return -1;
    uint32_t id = g_flash_diag.id+1u; if (!id) id=1;
    memset(&g_flash_diag, 0, sizeof g_flash_diag);
    g_flash_diag.id=id; g_flash_diag.started=now; g_flash_diag.phase=FD_BACKUP; g_flash_diag.result=FD_RUNNING;
    g_flash_diag.backup_crc=0xffffffffu; g_flash_diag.wait_started=now;
    return 0;
}
void flash_diag_cancel(void) { if (flash_diag_active()) g_flash_diag.cancel=1; }
int flash_diag_retry(uint32_t now)
{
    if (g_flash_diag.phase != FD_RESTORE_FAILED || !g_flash_diag.saved_valid) return -1;
    g_flash_diag.phase=FD_RESTORE_ERASE; g_flash_diag.result=FD_RUNNING; g_flash_diag.offset=0; g_flash_diag.wait_started=now;
    return 0;
}
static void finish(uint32_t now)
{
    g_flash_diag.phase=FD_DONE; g_flash_diag.elapsed_ms=now-g_flash_diag.started;
    g_flash_diag.result=g_flash_diag.error ? FD_FAIL : g_flash_diag.cancel ? FD_CANCELLED : FD_PASS;
}
static void failed(int error, int restore)
{
    if (restore) {
        g_flash_diag.restore_error=error; g_flash_diag.result=FD_FAIL; g_flash_diag.phase=FD_RESTORE_FAILED;
    } else {
        g_flash_diag.error=error; g_flash_diag.offset=0; g_flash_diag.phase=FD_RESTORE_ERASE;
        g_flash_diag.wait_started=g_flash_diag.started+g_flash_diag.elapsed_ms;
    }
}

// [region steps]
/* 各轮只传一页或查一次状态；芯片擦除/编程的 BUSY 时间交给下一轮，不忙等。 */
void flash_diag_poll(uint32_t now)
{
    if (!flash_diag_active()) return;
    flash_diag_status *s=&g_flash_diag;
    s->elapsed_ms=now-s->started;
    if (s->cancel && s->phase < FD_RESTORE_ERASE) {
        if (!s->dirty) { s->saved_valid=0; finish(now); return; }
        s->phase=FD_RESTORE_ERASE; s->offset=0; s->wait_started=now;
    }
    uint8_t data[NOR_PAGE], status;
    int rc;
    switch (s->phase) {
    case FD_BACKUP:
        rc=nor_read(&g_flash, BASE+s->offset, s_saved+s->offset, NOR_PAGE);
        if (rc) { s->error=FD_BACKUP_IO; finish(now); return; }
        s->backup_crc=checksum(s->backup_crc,s_saved+s->offset,NOR_PAGE);
        if ((s->offset+=NOR_PAGE)==NOR_SECTOR) {
            s->saved_valid=1; s->backup_crc^=0xffffffffu; s->offset=0; s->phase=FD_ERASE;
        }
        break;
    case FD_ERASE: case FD_RESTORE_ERASE: {
        const int restore=s->phase==FD_RESTORE_ERASE;
        rc=nor_status(&g_flash,&status);
        if (rc) { failed(restore ? FD_RESTORE_IO : FD_ERASE_IO,restore); break; }
        if (status&1u) { if(now-s->wait_started>3000u) failed(FD_TIMEOUT,restore); break; }
        /* 先标 dirty：传输失败也可能已让芯片开始擦除。 */
        s->dirty=1;
        rc=nor_erase_begin(&g_flash,BASE);
        if (rc) { failed(restore ? FD_RESTORE_IO : FD_ERASE_IO,restore); break; }
        s->wait_started=now; s->phase=restore ? FD_RESTORE_ERASE_WAIT : FD_ERASE_WAIT;
        break;
    }
    case FD_ERASE_WAIT: case FD_PROGRAM_WAIT: case FD_RESTORE_ERASE_WAIT: case FD_RESTORE_PROGRAM_WAIT: {
        const int restore=s->phase>=FD_RESTORE_ERASE;
        rc=nor_status(&g_flash,&status);
        if (rc || ((status&1u) && now-s->wait_started>3000u)) { failed(rc ? (restore?FD_RESTORE_IO:FD_READ_IO) : FD_TIMEOUT,restore); break; }
        if (status&1u) break;
        if (s->phase==FD_ERASE_WAIT) { s->offset=0; s->phase=FD_BLANK; }
        else if (s->phase==FD_RESTORE_ERASE_WAIT) { s->offset=0; s->phase=FD_RESTORE_PROGRAM; }
        else if ((s->offset+=NOR_PAGE)==NOR_SECTOR) {
            s->offset=0;
            s->phase=restore ? FD_RESTORE_VERIFY : FD_VERIFY;
            if (restore) s->restored_crc=0xffffffffu;
        } else s->phase=restore ? FD_RESTORE_PROGRAM : FD_PROGRAM;
        break;
    }
    case FD_BLANK:
        rc=nor_read(&g_flash,BASE+s->offset,data,NOR_PAGE);
        if (rc) { failed(FD_READ_IO,0); break; }
        for (unsigned i=0;i<NOR_PAGE;++i) if(data[i]!=0xff) { failed(FD_MISMATCH,0); return; }
        if ((s->offset+=NOR_PAGE)==NOR_SECTOR) { s->offset=0; s->phase=FD_PROGRAM; }
        break;
    case FD_PROGRAM: case FD_RESTORE_PROGRAM: {
        const int restore=s->phase==FD_RESTORE_PROGRAM;
        for(unsigned i=0;i<NOR_PAGE;++i) data[i]=pattern(s->offset+i);
        rc=nor_program_begin(&g_flash,BASE+s->offset,restore ? s_saved+s->offset : data,NOR_PAGE);
        if(rc) { failed(restore ? FD_RESTORE_IO : FD_PROGRAM_IO,restore); break; }
        s->wait_started=now; s->phase=restore ? FD_RESTORE_PROGRAM_WAIT : FD_PROGRAM_WAIT;
        break;
    }
    case FD_VERIFY: case FD_RESTORE_VERIFY: {
        const int restore=s->phase==FD_RESTORE_VERIFY;
        rc=nor_read(&g_flash,BASE+s->offset,data,NOR_PAGE);
        if(rc) { failed(restore?FD_RESTORE_IO:FD_READ_IO,restore); break; }
        for(unsigned i=0;i<NOR_PAGE;++i) {
            const uint8_t expected=restore ? s_saved[s->offset+i] : pattern(s->offset+i);
            if(data[i]!=expected) { failed(restore?FD_RESTORE_MISMATCH:FD_MISMATCH,restore); return; }
        }
        if(restore) s->restored_crc=checksum(s->restored_crc,data,NOR_PAGE);
        if((s->offset+=NOR_PAGE)==NOR_SECTOR) {
            s->offset=0;
            if(restore) {
                s->restored_crc^=0xffffffffu; s->saved_valid=0; s->restored=1; finish(now);
            } else s->phase=FD_RESTORE_ERASE;
        }
        break;
    }
    default: break;
    }
}
// [endregion]
