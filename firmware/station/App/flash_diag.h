#ifndef VC_FLASH_DIAG_H
#define VC_FLASH_DIAG_H
#include <stdint.h>
enum { FD_IDLE, FD_BACKUP, FD_ERASE, FD_ERASE_WAIT, FD_BLANK, FD_PROGRAM, FD_PROGRAM_WAIT,
       FD_VERIFY, FD_RESTORE_ERASE, FD_RESTORE_ERASE_WAIT, FD_RESTORE_PROGRAM,
       FD_RESTORE_PROGRAM_WAIT, FD_RESTORE_VERIFY, FD_DONE, FD_RESTORE_FAILED };
enum { FD_NONE, FD_RUNNING, FD_PASS, FD_FAIL, FD_CANCELLED };
enum { FD_OK, FD_BACKUP_IO, FD_ERASE_IO, FD_PROGRAM_IO, FD_READ_IO, FD_MISMATCH, FD_TIMEOUT, FD_RESTORE_IO, FD_RESTORE_MISMATCH };
typedef struct {
    uint32_t id, phase, result, error, restore_error, offset, started, wait_started, elapsed_ms;
    uint32_t backup_crc, restored_crc;
    uint8_t saved_valid, dirty, cancel, restored;
} flash_diag_status;
extern flash_diag_status g_flash_diag;
int flash_diag_start(uint32_t now);
void flash_diag_cancel(void);
int flash_diag_retry(uint32_t now);
int flash_diag_active(void);
void flash_diag_poll(uint32_t now);
#endif
