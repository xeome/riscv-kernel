#include "tarfs.h"
#include "virtio.h"

struct file files[FILES_MAX];
uint8_t disk[DISK_MAX_SIZE];

/**
 * Converts an octal string to an integer.
 *
 * @param oct The octal string to convert.
 * @param len The length of the octal string.
 * @return The integer representation of the octal string.
 */
int oct2int(char* oct, int len) {
    int dec = 0;
    for (int i = 0; i < len; i++) {
        if (oct[i] < '0' || oct[i] > '7')
            break;

        dec = dec * 8 + (oct[i] - '0');
    }
    return dec;
}

/**
 * Initializes the file system by reading the disk and parsing the tar headers.
 *
 * Reads the disk sector by sector and parses the tar headers to populate the file system.
 * The function reads the disk sector by sector and checks the magic number of each tar header.
 * If the magic number is not "ustar", the function panics.
 * Otherwise, the function populates the file system with the file name, data, and size.
 *
 * @return void
 */
void fs_init(void) {
    // Read the disk sector by sector and populate the file system.
    for (unsigned sector = 0; sector < sizeof(disk) / SECTOR_SIZE; sector++)
        read_write_disk(&disk[sector * SECTOR_SIZE], sector, false);

    // Parse the tar headers and populate the file system.
    unsigned off = 0;
    for (int i = 0; i < FILES_MAX; i++) {
        // Check if the tar header is empty.
        struct tar_header* header = (struct tar_header*)&disk[off];
        if (header->name[0] == '\0')
            break;

        // Check if the magic number is "ustar".
        if (strcmp(header->magic, "ustar") != 0)
            PANIC("invalid tar header: magic=\"%s\"", header->magic);

        // Populate the file system with the file name, data, and size.
        const int filesz = oct2int(header->size, sizeof(header->size));
        struct file* file = &files[i];
        file->in_use = true;
        strcpy(file->name, header->name);
        memcpy(file->data, header->data, filesz);
        file->size = filesz;
        printf("file: %s, size=%d\n", file->name, file->size);

        // Move to the next tar header.
        off += align_up(sizeof(struct tar_header) + filesz, SECTOR_SIZE);
    }
}

// Write contents of each file in the files variable to the disk variable.
void fs_flush(void) {
    memset(disk, 0, sizeof(disk));
    unsigned off = 0;
    for (int file_i = 0; file_i < FILES_MAX; file_i++) {
        const struct file* file = &files[file_i];
        if (!file->in_use)
            continue;

        struct tar_header* header = (struct tar_header*)&disk[off];
        memset(header, 0, sizeof(*header));
        strcpy(header->name, file->name);
        strcpy(header->mode, "000644");
        strcpy(header->magic, "ustar");
        strcpy(header->version, "00");
        header->type = '0';

        // Convert file size to octal string
        int filesz = file->size;
        int i = 0;
        do {
            header->size[i++] = (filesz % 8) + '0';
            filesz /= 8;
        } while (filesz > 0);

        // Calculate checksum
        int checksum = ' ' * sizeof(header->checksum);
        for (unsigned i = 0; i < sizeof(struct tar_header); i++)
            checksum += (unsigned char)disk[off + i];

        for (int i = 5; i >= 0; i--) {
            header->checksum[i] = (checksum % 8) + '0';
            checksum /= 8;
        }

        // Copy file data
        memcpy(header->data, file->data, file->size);
        off += align_up(sizeof(struct tar_header) + file->size, SECTOR_SIZE);
    }

    // Write the disk sector by sector.
    for (unsigned sector = 0; sector < sizeof(disk) / SECTOR_SIZE; sector++)
        read_write_disk(&disk[sector * SECTOR_SIZE], sector, true);

    printf("wrote %d bytes to disk\n", sizeof(disk));
}

/**
 * Looks up a file in the file system by its filename.
 *
 * @param filename The name of the file to look up.
 * @return A pointer to the file struct if found, or NULL if not found.
 */
struct file* fs_lookup(const char* filename) {
    for (int i = 0; i < FILES_MAX; i++) {
        struct file* file = &files[i];
        if (!strcmp(file->name, filename))
            return file;
    }

    return NULL;
}