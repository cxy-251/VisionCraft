#ifndef VC_EEPROM_DIAG_H
#define VC_EEPROM_DIAG_H
#include <stdint.h>

/* AT24C02 的最后一页专供诊断；配方 0..11、设置 32..38 不在此区域。 */
#define EE_DIAG_ADDR 248u
#define EE_DIAG_SIZE 8u
enum { EE_IDLE, EE_BACKUP, EE_WRITE, EE_VERIFY, EE_RESTORE, EE_RESTORE_VERIFY, EE_DONE, EE_RESTORE_FAILED };
enum { EE_NONE, EE_RUNNING, EE_PASS, EE_FAIL, EE_CANCELLED };
enum { EE_OK, EE_BACKUP_IO, EE_WRITE_IO, EE_READ_IO, EE_MISMATCH, EE_RESTORE_IO, EE_RESTORE_MISMATCH };
typedef struct {
    uint32_t id, phase, result, error, restore_error, patterns, started, elapsed_ms;
    uint8_t saved[EE_DIAG_SIZE], saved_valid, cancel, restored;
} ee_diag_status;
extern ee_diag_status g_ee_diag; /* 仅 StationTask 修改；调试器可读取快照 */
int ee_diag_start(uint32_t now); /* 0 成功，-1 忙或有原值待恢复 */
void ee_diag_cancel(void);
int ee_diag_retry(void);        /* 恢复失败后重试，保留同一份 RAM 备份 */
int ee_diag_active(void);
void ee_diag_poll(uint32_t now); /* 每次最多一次 EEPROM 操作，不创建任务/队列 */
/* 驱动提供的固定页写入；普通 eeprom_write 禁止访问保留页。 */
int eeprom_diag_write_page(const uint8_t data[EE_DIAG_SIZE]);
#endif
