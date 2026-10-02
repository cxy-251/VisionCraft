#ifndef __BSP_FS_MANAGER_H
#define __BSP_FS_MANAGER_H

#include "main.h"
#include "ff.h"

#define FS_MAX_FILES_LIST 10

typedef struct {
    char     name[32];
    uint32_t size;
    uint8_t  is_dir;
} Fs_Item_t;

typedef struct {
    uint8_t    drive_idx;         // 0: MicroSD, 1: W25Q128 Flash
    uint8_t    is_mounted;
    char       status_str[64];
    uint32_t   total_kb;
    uint32_t   free_kb;
    uint16_t   file_count;
    Fs_Item_t  files[FS_MAX_FILES_LIST];
    char       preview_buf[256];
    char       preview_filename[32];
} Fs_Explorer_State_t;

void Bsp_FsManager_Init(void);
FRESULT Bsp_FsManager_MountDrive(uint8_t drive_idx);
FRESULT Bsp_FsManager_FormatDrive(uint8_t drive_idx);
FRESULT Bsp_FsManager_ScanFiles(uint8_t drive_idx);
FRESULT Bsp_FsManager_CreateDemoFile(uint8_t drive_idx, const char *filename, const char *content);
FRESULT Bsp_FsManager_ReadPreview(uint8_t drive_idx, const char *filename);
const Fs_Explorer_State_t* Bsp_FsManager_GetState(uint8_t drive_idx);

#endif /* __BSP_FS_MANAGER_H */
