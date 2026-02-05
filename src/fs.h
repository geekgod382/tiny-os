#ifndef FS_H
#define FS_H

#include <stdint.h>
#include "ata.h"

struct dir_entry {
    char     name[24];
    uint32_t size;
    uint8_t  used;
    uint8_t  _pad[3];
} __attribute__((packed));

extern struct dir_entry root_dir[MAX_FILES];

int fs_read_file(const char* name, uint8_t* buffer, uint32_t buffer_size);

int fs_write_file(const char* name, const uint8_t* data, uint32_t size);

int fs_delete_file(const char* name);

void fs_init(void);

#endif