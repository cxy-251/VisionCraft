#ifndef VC_DIAGNOSTICS_H
#define VC_DIAGNOSTICS_H
#include <stdint.h>
enum { DIAG_NONE, DIAG_SRAM, DIAG_EEPROM, DIAG_FLASH };
enum { DIAG_IDLE, DIAG_RUNNING, DIAG_PASS, DIAG_FAIL, DIAG_CANCELLED };
typedef struct { uint32_t result, phase, offset, bad, started, elapsed_ms, cancel; } sram_diag_status;
extern sram_diag_status g_sram_diag;
extern uint32_t g_diag_kind;
int diag_start(unsigned kind, uint32_t now); /* vc_status；仅 StationTask */
int diag_busy(void);
void diag_cancel(void);
int diag_retry(uint32_t now);
void diag_poll(uint32_t now);
uint32_t diag_ram_used(void);
uint32_t diag_ccm_used(void);
uint32_t diag_external_used(void);
#endif
