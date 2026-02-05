#include "ata.h"
#include "fs.h"
#include <stdint.h>
#include <stddef.h>

struct dir_entry root_dir[MAX_FILES];

static void fs_load_directory(void) {
    uint8_t sector[SECTOR_SIZE];
    uint8_t* p = (uint8_t*)root_dir;
    size_t to_read = sizeof(root_dir);  // 8192 bytes

    for (uint32_t i = 0; i < DIR_SECTORS; ++i) {
        ata_read_sector(DIR_START_LBA + i, sector);
        size_t chunk = (to_read > SECTOR_SIZE) ? SECTOR_SIZE : to_read;
        for (size_t j = 0; j < chunk; ++j) {
            p[i * SECTOR_SIZE + j] = sector[j];
        }
        to_read -= chunk;
        if (to_read == 0) break;
    }

}

static void fs_save_directory(void) {
    uint8_t sector[SECTOR_SIZE];
    uint8_t* p = (uint8_t*)root_dir;
    size_t remaining = sizeof(root_dir);

    for (uint32_t i = 0; i < DIR_SECTORS; ++i) {
        size_t chunk = (remaining > SECTOR_SIZE) ? SECTOR_SIZE : remaining;
        for (size_t j = 0; j < chunk; ++j) {
            sector[j] = p[i * SECTOR_SIZE + j];
        }
        for (size_t j = chunk; j < SECTOR_SIZE; ++j) {
            sector[j] = 0;
        }
        ata_write_sector(DIR_START_LBA + i, sector);
        remaining -= chunk;
        if (remaining == 0) break;
    }
}
static int fs_find_free_slot(void) {
    for (int i = 0; i < MAX_FILES; ++i) {
        if (!root_dir[i].used) {
            return i;
        }
    }
    return -1;
}

static int fs_find_by_name(const char* name) {
    for (int i = 0; i < MAX_FILES; ++i) {
        if (root_dir[i].used) {
            // simple strcmp
            const char* a = root_dir[i].name;
            const char* b = name;
            int eq = 1;
            for (int j = 0; j < 24; ++j) {
                if (a[j] != b[j]) {
                    eq = 0;
                    break;
                }
                if (a[j] == '\0') break;
            }
            if (eq) return i;
        }
    }
    return -1;
}

static uint32_t fs_slot_lba(int slot) {
    return DATA_START_LBA + (uint32_t)slot * FILE_SECTORS;
}

int fs_write_file(const char* name, const uint8_t* data, uint32_t size) {
    if (size > 64 * 1024) size = 64 * 1024;

    int slot = fs_find_by_name(name);
    if (slot < 0) {
        slot = fs_find_free_slot();
        if (slot < 0) return -1; // no space
    }

    struct dir_entry* e = &root_dir[slot];

    // store name
    for (int i = 0; i < 24; ++i) {
        if (name[i] != '\0') {
            e->name[i] = name[i];
        } else {
            e->name[i] = '\0';
            break;
        }
    }
    e->size = size;
    e->used = 1;

    // write data to disk
    uint32_t lba = fs_slot_lba(slot);
    uint8_t sector[SECTOR_SIZE];
    uint32_t remaining = size;
    const uint8_t* p = data;

    for (uint32_t s = 0; s < FILE_SECTORS; ++s) {
        for (int i = 0; i < SECTOR_SIZE; ++i) {
            if (remaining > 0) {
                sector[i] = *p++;
                remaining--;
            } else {
                sector[i] = 0;
            }
        }
        ata_write_sector(lba + s, sector);
    }

    fs_save_directory();
    return 0;
}

int fs_read_file(const char* name, uint8_t* buffer, uint32_t buffer_size) {
    int slot = fs_find_by_name(name);
    if (slot < 0) return -1; // not found

    struct dir_entry* e = &root_dir[slot];
    uint32_t size = e->size;
    if (size > buffer_size) size = buffer_size;

    uint32_t lba = fs_slot_lba(slot);
    uint8_t sector[SECTOR_SIZE];
    uint32_t remaining = size;
    uint8_t* p = buffer;

    for (uint32_t s = 0; s < FILE_SECTORS; ++s) {
        ata_read_sector(lba + s, sector);
        for (int i = 0; i < SECTOR_SIZE; ++i) {
            if (remaining > 0) {
                *p++ = sector[i];
                remaining--;
            } else {
                break;
            }
        }
        if (remaining == 0) break;
    }

    return size;
}

int fs_delete_file(const char* name) {
    int slot = fs_find_by_name(name);
    if (slot < 0) return -1; // not found

    struct dir_entry* e = &root_dir[slot];
    e->used = 0;
    e->size = 0;
    e->name[0] = '\0';

    /* Zero out the file's data sectors to avoid leftover data */
    uint8_t sector[SECTOR_SIZE];
    for (int i = 0; i < SECTOR_SIZE; ++i) sector[i] = 0;
    uint32_t lba = fs_slot_lba(slot);
    for (uint32_t s = 0; s < FILE_SECTORS; ++s) {
        ata_write_sector(lba + s, sector);
    }

    fs_save_directory();
    return 0;
}

void fs_init(void){
    fs_load_directory();
}