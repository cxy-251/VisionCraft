#include "diagnostics.h"
#include "eeprom_diag.h"
#include "flash_diag.h"
#include "spiflash.h"
#include "vc_protocol.h"
#include <string.h>

uint32_t g_diag_kind;
sram_diag_status g_sram_diag;
#if defined(__arm__)
static volatile uint32_t *const scratch=(volatile uint32_t *)0x680f0000u;
extern char __ram_used_end, __ccm_used_end, __ext_used_end;
uint32_t diag_ram_used(void) { return (uint32_t)(uintptr_t)&__ram_used_end-0x20000000u; }
uint32_t diag_ccm_used(void) { return (uint32_t)(uintptr_t)&__ccm_used_end-0x10000000u; }
uint32_t diag_external_used(void) { return (uint32_t)(uintptr_t)&__ext_used_end-0x68000000u; }
#else
static volatile uint32_t scratch[16384];
uint32_t diag_ram_used(void) { return 0; }
uint32_t diag_ccm_used(void) { return 0; }
uint32_t diag_external_used(void) { return 0; }
#endif
int diag_busy(void) { return g_sram_diag.result==DIAG_RUNNING || ee_diag_active() || flash_diag_active(); }
// [region ownership]
int diag_start(unsigned kind, uint32_t now)
{
    if (diag_busy() || g_ee_diag.saved_valid || g_flash_diag.saved_valid) return VC_ERR_BUSY;
    int rc=VC_OK;
    if (kind==DIAG_SRAM) {
        memset(&g_sram_diag,0,sizeof g_sram_diag);
        g_sram_diag.result=DIAG_RUNNING; g_sram_diag.phase=1; g_sram_diag.started=now;
    } else if (kind==DIAG_EEPROM) rc=ee_diag_start(now) ? VC_ERR_BUSY : VC_OK;
    else if (kind==DIAG_FLASH) rc=flash_diag_start(now) ? VC_ERR_IO : VC_OK;
    else return VC_ERR_ARGS;
    if (rc==VC_OK) g_diag_kind=kind;
    return rc;
}
void diag_cancel(void)
{
    if(g_sram_diag.result==DIAG_RUNNING) g_sram_diag.cancel=1;
    ee_diag_cancel(); flash_diag_cancel();
}
int diag_retry(uint32_t now)
{
    if(diag_busy()) return VC_ERR_BUSY;
    if(g_ee_diag.saved_valid && ee_diag_retry()==0) { g_diag_kind=DIAG_EEPROM; return VC_OK; }
    if(g_flash_diag.saved_valid && flash_diag_retry(now)==0) { g_diag_kind=DIAG_FLASH; return VC_OK; }
    return VC_ERR_ARGS;
}
// [endregion]
static uint32_t pattern(uint32_t i) { return i*0x9e3779b9u ^ 0xa55a12efu; }
void diag_poll(uint32_t now)
{
    if(g_diag_kind==DIAG_EEPROM) ee_diag_poll(now);
    else if(g_diag_kind==DIAG_FLASH) flash_diag_poll(now);
    else if(g_diag_kind==DIAG_SRAM && g_sram_diag.result==DIAG_RUNNING) {
        sram_diag_status *s=&g_sram_diag;
        s->elapsed_ms=now-s->started;
        if(s->cancel) { s->result=DIAG_CANCELLED; return; }
        const uint32_t end=s->offset+256; /* 每轮1 KB，最多128轮完成64 KB写/读 */
        for(uint32_t i=s->offset;i<end;++i) {
            if(s->phase==1) scratch[i]=pattern(i);
            else if(scratch[i]!=pattern(i)) s->bad++;
        }
        s->offset=end;
        if(s->offset==16384) {
            if(s->phase==1) { s->phase=2; s->offset=0; }
            else s->result=s->bad ? DIAG_FAIL : DIAG_PASS;
        }
    }
}
