// © Copyright 2025 Claude Schwarz
// SPDX-License-Identifier: MIT

#ifndef RGA_COMMON_H
#define RGA_COMMON_H

#include <stdint.h>

#define RAM_DISK_SIZE 8192
#define STX_MAGIC     0x55AA
#define ETX_MAGIC     0xEEFF

#define FTCMD_READ         0x01 //reads the ram disk
#define FTCMD_WRITE        0x02 //writes the ram disk
//..//
#define FTCMD_FLASH_ERASE  0x10 //erase staging
#define FTCMD_FLASH_DATA   0x11 //write staging
#define FTCMD_FLASH_COMMIT 0x12 //commit staging to main flash and reboot
#define FTCMD_SAVE_SETTING 0x13 //write scanline and deinterlace settings into flash
//..//
#define FTCMD_GET_VERSION  0x20 //returns the version string
#define FTCMD_GET_GIT      0x21 //returns the git hash string
#define FTCMD_GET_STATUS   0x22 //returns the current video statistics
#define FTCMD_GET_SCANLINE 0x23 //returns the current scanline settings
#define FTCMD_GET_DEINT    0x24 //returns the current deinterlace settings
//..//
#define FTCMD_SET_SCANLINE 0x28 //sets the scanline settings
#define FTCMD_SET_DEINT    0x29 //sets the deinterlace settings

#define STATUS_OK        0x0000
#define STATUS_ERR_ADDR  0x0001
#define STATUS_ERR_CMD   0x0002
#define STATUS_ERR_FLASH 0x0003
#define STATUS_ERR_CRC   0xFFFF

// Staging Area beginnt bei 1 MB Offset
#define FLASH_STAGING_OFFSET 0x00100000
#define FLASH_PAGE_SIZE      256
#define FLASH_SECTOR_SIZE    4096

// Settings Area beginnt bei 2 MB Offset
#define FLASH_SETTINGS_OFFSET 0x00200000

typedef struct {
    uint8_t laced;        // War bool
    uint8_t isPAL;        // War bool
    uint8_t _padding[2];  // Auffüllen auf 4-Byte Alignment
    uint32_t last_total_lines;
    int32_t scanline_level;
    int32_t scanline_level_laced;
} RGA_VideoStatus;

#endif // RGA_COMMON_H
