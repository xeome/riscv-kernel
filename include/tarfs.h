#pragma once
#include "kernel.h"

#define FILES_MAX 2
#define DISK_MAX_SIZE align_up(sizeof(struct file) * FILES_MAX, SECTOR_SIZE)

struct tar_header {
    char name[100];      // Name of the file or directory
    char mode[8];        // File mode
    char uid[8];         // User ID
    char gid[8];         // Group ID
    char size[12];       // Size of the file
    char mtime[12];      // Modification time
    char checksum[8];    // Checksum
    char type;           // Type of file
    char linkname[100];  // Name of the linked file
    char magic[6];       // Magic value
    char version[2];     // Version
    char uname[32];      // User name
    char gname[32];      // Group name
    char devmajor[8];    // Major device number
    char devminor[8];    // Minor device number
    char prefix[155];    // Prefix
    char padding[12];    // Padding
    char data[];         // Array pointing to the data area following the header
} __attribute__((packed));

struct file {
    bool in_use;      // Is this file slot in use?
    char name[100];   // File name
    char data[1024];  // File data
    size_t size;      // File size
};

void fs_init(void);
struct file* fs_lookup(const char* filename);
void fs_flush(void);