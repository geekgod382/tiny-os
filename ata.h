#ifndef ATA_H
#define ATA_H

#include "io.h"
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

static void ata_wait_busy(void) {
    while (inb(ATA_REG_STATUS) & ATA_STATUS_BSY) {
        // spin
    }
}

static void ata_wait_drq(void) {
    uint8_t st;
    do {
        st = inb(ATA_REG_STATUS);
    } while ((st & ATA_STATUS_BSY) || !(st & ATA_STATUS_DRQ));
}

void ata_read_sector(uint32_t lba, void* buffer) {
    uint16_t* buf = (uint16_t*)buffer;

    ata_wait_busy();

    outb(ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F)); // master, LBA
    outb(ATA_REG_SECCOUNT0, 1);                          // 1 sector
    outb(ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
    outb(ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_REG_COMMAND, ATA_CMD_READ_SECT);

    ata_wait_drq();

    for (int i = 0; i < SECTOR_SIZE / 2; ++i) {
        buf[i] = inw(ATA_REG_DATA);   // read 16 bits at a time, as ATA expects
    }
}

void ata_write_sector(uint32_t lba, const void* buffer) {
    const uint16_t* buf = (const uint16_t*)buffer;

    ata_wait_busy();

    outb(ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_REG_SECCOUNT0, 1);
    outb(ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
    outb(ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_REG_COMMAND, ATA_CMD_WRITE_SECT);

    ata_wait_drq();

    for (int i = 0; i < SECTOR_SIZE / 2; ++i) {
        outw(ATA_REG_DATA, buf[i]);  // write 16 bits at a time
    }

    ata_wait_busy();
}


#endif