#ifndef ATA_H
#define ATA_H

#include <stdint.h>

#define SECTOR_SIZE      512
#define DISK_TOTAL_SECT  (64 * 1024 * 1024 / SECTOR_SIZE)   // 131072
#define DIR_SECTORS      16
#define DIR_START_LBA    1
#define DATA_START_LBA   (DIR_START_LBA + DIR_SECTORS)      // 17
#define FILE_SECTORS     (64 * 1024 / SECTOR_SIZE)          // 128
#define MAX_FILES        256

#define ATA_PRIMARY_IO      0x1F0
#define ATA_PRIMARY_CTRL    0x3F6

#define ATA_REG_DATA        (ATA_PRIMARY_IO + 0)
#define ATA_REG_ERROR       (ATA_PRIMARY_IO + 1)
#define ATA_REG_SECCOUNT0   (ATA_PRIMARY_IO + 2)
#define ATA_REG_LBA0        (ATA_PRIMARY_IO + 3)
#define ATA_REG_LBA1        (ATA_PRIMARY_IO + 4)
#define ATA_REG_LBA2        (ATA_PRIMARY_IO + 5)
#define ATA_REG_HDDEVSEL    (ATA_PRIMARY_IO + 6)
#define ATA_REG_COMMAND     (ATA_PRIMARY_IO + 7)
#define ATA_REG_STATUS      (ATA_PRIMARY_IO + 7)

#define ATA_CMD_READ_SECT   0x20
#define ATA_CMD_WRITE_SECT  0x30

#define ATA_STATUS_BSY      0x80
#define ATA_STATUS_DRDY     0x40
#define ATA_STATUS_DRQ      0x08


void ata_read_sector(uint32_t lba, void* buffer);

void ata_write_sector(uint32_t lba, const void* buffer);


#endif