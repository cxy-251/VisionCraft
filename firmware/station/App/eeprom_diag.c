#include "eeprom_diag.h"
#include "eeprom.h"
#include <string.h>

ee_diag_status g_ee_diag;
static const uint8_t patterns[] = {0x00, 0xff, 0xaa, 0x55};

int ee_diag_active(void) { return g_ee_diag.phase >= EE_BACKUP && g_ee_diag.phase <= EE_RESTORE_VERIFY; }

// [region lifecycle]
int ee_diag_start(uint32_t now)
{
    if (ee_diag_active() || g_ee_diag.saved_valid) return -1;
    uint32_t id = g_ee_diag.id + 1u;
    if (!id) id = 1;
    memset(&g_ee_diag, 0, sizeof g_ee_diag);
    g_ee_diag.id = id;
    g_ee_diag.phase = EE_BACKUP;
    g_ee_diag.result = EE_RUNNING;
    g_ee_diag.started = now;
    return 0;
}
void ee_diag_cancel(void) { if (ee_diag_active()) g_ee_diag.cancel = 1; }
int ee_diag_retry(void)
{
    if (g_ee_diag.phase != EE_RESTORE_FAILED || !g_ee_diag.saved_valid) return -1;
    g_ee_diag.phase = EE_RESTORE;
    g_ee_diag.result = EE_RUNNING;
    return 0;
}
static void finish(uint32_t now)
{
    g_ee_diag.phase = EE_DONE;
    g_ee_diag.elapsed_ms = now - g_ee_diag.started;
    g_ee_diag.result = g_ee_diag.error ? EE_FAIL : g_ee_diag.cancel ? EE_CANCELLED : EE_PASS;
}
// [endregion]

// [region steps]
void ee_diag_poll(uint32_t now)
{
    if (!ee_diag_active()) return;
    g_ee_diag.elapsed_ms = now - g_ee_diag.started;
    uint8_t data[EE_DIAG_SIZE];
    if (g_ee_diag.cancel && g_ee_diag.phase <= EE_VERIFY) {
        if (!g_ee_diag.saved_valid) { finish(now); return; }
        g_ee_diag.phase = EE_RESTORE;
    }
    switch (g_ee_diag.phase) {
    case EE_BACKUP:
        if (eeprom_read(EE_DIAG_ADDR, g_ee_diag.saved, EE_DIAG_SIZE) != 0) {
            g_ee_diag.error = EE_BACKUP_IO;
            finish(now); /* 备份失败：绝不写入测试码 */
        } else {
            g_ee_diag.saved_valid = 1;
            g_ee_diag.phase = EE_WRITE;
        }
        break;
    case EE_WRITE:
        memset(data, patterns[g_ee_diag.patterns], sizeof data);
        /* 即使写入返回错误，也可能已部分改变芯片，下一步必须尝试恢复。 */
        if (eeprom_diag_write_page(data) != 0) {
            g_ee_diag.error = EE_WRITE_IO;
            g_ee_diag.phase = EE_RESTORE;
        } else g_ee_diag.phase = EE_VERIFY;
        break;
    case EE_VERIFY:
        if (eeprom_read(EE_DIAG_ADDR, data, sizeof data) != 0) g_ee_diag.error = EE_READ_IO;
        else {
            uint8_t expected[EE_DIAG_SIZE];
            memset(expected, patterns[g_ee_diag.patterns], sizeof expected);
            if (memcmp(data, expected, sizeof data)) g_ee_diag.error = EE_MISMATCH;
        }
        if (g_ee_diag.error) g_ee_diag.phase = EE_RESTORE;
        else if (++g_ee_diag.patterns == sizeof patterns) g_ee_diag.phase = EE_RESTORE;
        else g_ee_diag.phase = EE_WRITE;
        break;
    case EE_RESTORE:
        if (eeprom_diag_write_page(g_ee_diag.saved) != 0) {
            g_ee_diag.restore_error = EE_RESTORE_IO;
            g_ee_diag.phase = EE_RESTORE_FAILED;
            g_ee_diag.result = EE_FAIL;
        } else g_ee_diag.phase = EE_RESTORE_VERIFY;
        break;
    case EE_RESTORE_VERIFY:
        if (eeprom_read(EE_DIAG_ADDR, data, sizeof data) != 0) g_ee_diag.restore_error = EE_RESTORE_IO;
        else g_ee_diag.restore_error = memcmp(data, g_ee_diag.saved, sizeof data) ? EE_RESTORE_MISMATCH : EE_OK;
        if (g_ee_diag.restore_error) {
            g_ee_diag.phase = EE_RESTORE_FAILED;
            g_ee_diag.result = EE_FAIL;
        } else {
            g_ee_diag.saved_valid = 0;
            g_ee_diag.restored = 1;
            finish(now);
        }
        break;
    default: break;
    }
}
// [endregion]
