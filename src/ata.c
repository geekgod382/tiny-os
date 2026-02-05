#include "io.h"
#include "ata.h"
#include <stdint.h>

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