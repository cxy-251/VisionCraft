#include "bsp_fs_manager.h"
#include "bsp_sdio_sd.h"
#include "bsp_spi_flash.h"
#include <string.h>
#include <stdio.h>

static FATFS s_fs_sd;
static FATFS s_fs_flash;
static Fs_Explorer_State_t s_states[2] = {0};
static uint8_t s_mkfs_work[4096];

void Bsp_FsManager_Init(void) {
    s_states[0].drive_idx = 0;
    s_states[1].drive_idx = 1;

    // 1. 初始化 MicroSD (SDIO) 卷 0
    Bsp_FsManager_MountDrive(0);

    // 2. 初始化 W25Q128 Flash 卷 1
    Bsp_FsManager_MountDrive(1);

    // 3. 若 Flash 卷为空或首次使用，写入标准演示文件
    if (s_states[1].is_mounted && s_states[1].file_count == 0) {
        Bsp_FsManager_CreateDemoFile(1, "README.TXT",
            "VisionCraft OS Storage Subsystem\r\n"
            "Hardware: STM32F407ZGT6 Explorer\r\n"
            "FatFs R0.15 mounted on W25Q128 Flash\r\n"
            "Allocation: 12 MiB (Sector 4096B)\r\n");

        Bsp_FsManager_CreateDemoFile(1, "SYSINFO.LOG",
            "Kernel: FreeRTOS V10.5.1 Preemptive\r\n"
            "CPU: 168MHz HSE PLL (8M Crystal)\r\n"
            "LCD: 480x800 NT35510 FSMC 16-bit\r\n"
            "Touch: GT9147 Capacitive I2C\r\n");

        Bsp_FsManager_ScanFiles(1);
    }

    if (s_states[1].is_mounted && s_states[1].file_count > 0) {
        Bsp_FsManager_ReadPreview(1, s_states[1].files[0].name);
    }
}

FRESULT Bsp_FsManager_MountDrive(uint8_t drive_idx) {
    if (drive_idx > 1) return FR_INVALID_DRIVE;
    Fs_Explorer_State_t *st = &s_states[drive_idx];

    if (drive_idx == 0) {
        // SD 卡挂载
        if (Bsp_Sd_Init() != SD_OK) {
            st->is_mounted = 0;
            snprintf(st->status_str, sizeof(st->status_str), "NO SD CARD (Slot Empty)");
            return FR_NOT_READY;
        }
        FRESULT res = f_mount(&s_fs_sd, "0:", 1);
        if (res == FR_OK) {
            st->is_mounted = 1;
            snprintf(st->status_str, sizeof(st->status_str), "SD Card Mounted (FAT32)");
            Bsp_FsManager_ScanFiles(0);
        } else {
            st->is_mounted = 0;
            snprintf(st->status_str, sizeof(st->status_str), "Mount Failed (Code %d)", res);
        }
        return res;
    } else {
        // W25Q128 Flash 挂载
        FRESULT res = f_mount(&s_fs_flash, "1:", 1);
        if (res == FR_NO_FILESYSTEM) {
            // 未格式化，自动格式化 Flash FAT 卷
            res = Bsp_FsManager_FormatDrive(1);
            if (res == FR_OK) {
                res = f_mount(&s_fs_flash, "1:", 1);
            }
        }

        if (res == FR_OK) {
            st->is_mounted = 1;
            snprintf(st->status_str, sizeof(st->status_str), "SPI Flash FAT (12 MiB)");
            Bsp_FsManager_ScanFiles(1);
        } else {
            st->is_mounted = 0;
            snprintf(st->status_str, sizeof(st->status_str), "Flash Mount Err (%d)", res);
        }
        return res;
    }
}

FRESULT Bsp_FsManager_FormatDrive(uint8_t drive_idx) {
    if (drive_idx > 1) return FR_INVALID_DRIVE;
    const char *drive_str = (drive_idx == 0) ? "0:" : "1:";
    const MKFS_PARM opt = {FM_FAT, 1, 0, 0, (drive_idx == 0) ? 512 : 4096};

    FRESULT res = f_mkfs(drive_str, &opt, s_mkfs_work, sizeof(s_mkfs_work));
    if (res == FR_OK) {
        snprintf(s_states[drive_idx].status_str, sizeof(s_states[drive_idx].status_str),
                 "Formatted Successfully");
        Bsp_FsManager_ScanFiles(drive_idx);
    } else {
        snprintf(s_states[drive_idx].status_str, sizeof(s_states[drive_idx].status_str),
                 "Format Failed (%d)", res);
    }
    return res;
}

FRESULT Bsp_FsManager_ScanFiles(uint8_t drive_idx) {
    if (drive_idx > 1) return FR_INVALID_DRIVE;
    Fs_Explorer_State_t *st = &s_states[drive_idx];
    st->file_count = 0;

    const char *path = (drive_idx == 0) ? "0:/" : "1:/";
    DIR dir;
    FILINFO fno;

    FRESULT res = f_opendir(&dir, path);
    if (res != FR_OK) return res;

    while (st->file_count < FS_MAX_FILES_LIST) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) break;
        if (fno.fname[0] == '.') continue; // 跳过 "." 和 ".."

        Fs_Item_t *item = &st->files[st->file_count++];
        strncpy(item->name, fno.fname, sizeof(item->name) - 1);
        item->name[sizeof(item->name) - 1] = '\0';
        item->size = (uint32_t)fno.fsize;
        item->is_dir = (fno.fattrib & AM_DIR) ? 1 : 0;
    }
    f_closedir(&dir);

    // 计算总容量与剩余容量
    DWORD free_clusters;
    FATFS *fs_ptr;
    res = f_getfree(path, &free_clusters, &fs_ptr);
    if (res == FR_OK && fs_ptr) {
        uint32_t sec_size = (drive_idx == 0) ? 512 : 4096;
        uint32_t total_sec = (fs_ptr->n_fatent - 2) * fs_ptr->csize;
        st->total_kb = (uint32_t)(((uint64_t)total_sec * sec_size) / 1024);
        st->free_kb  = (uint32_t)(((uint64_t)free_clusters * fs_ptr->csize * sec_size) / 1024);
    }

    return FR_OK;
}

FRESULT Bsp_FsManager_CreateDemoFile(uint8_t drive_idx, const char *filename, const char *content) {
    if (drive_idx > 1 || !filename || !content) return FR_INVALID_PARAMETER;
    char path[48];
    snprintf(path, sizeof(path), "%d:/%s", drive_idx, filename);

    FIL fil;
    UINT bw = 0;
    FRESULT res = f_open(&fil, path, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) return res;

    res = f_write(&fil, content, (UINT)strlen(content), &bw);
    f_close(&fil);

    Bsp_FsManager_ScanFiles(drive_idx);
    Bsp_FsManager_ReadPreview(drive_idx, filename);
    return res;
}

FRESULT Bsp_FsManager_ReadPreview(uint8_t drive_idx, const char *filename) {
    if (drive_idx > 1 || !filename) return FR_INVALID_PARAMETER;
    Fs_Explorer_State_t *st = &s_states[drive_idx];

    char path[48];
    snprintf(path, sizeof(path), "%d:/%s", drive_idx, filename);
    strncpy(st->preview_filename, filename, sizeof(st->preview_filename) - 1);

    FIL fil;
    UINT br = 0;
    FRESULT res = f_open(&fil, path, FA_READ);
    if (res != FR_OK) {
        snprintf(st->preview_buf, sizeof(st->preview_buf), "<Cannot open file (%d)>", res);
        return res;
    }

    res = f_read(&fil, st->preview_buf, sizeof(st->preview_buf) - 1, &br);
    st->preview_buf[br] = '\0';
    f_close(&fil);
    return res;
}

const Fs_Explorer_State_t* Bsp_FsManager_GetState(uint8_t drive_idx) {
    if (drive_idx > 1) return NULL;
    return &s_states[drive_idx];
}
