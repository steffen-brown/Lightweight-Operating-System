#ifndef FILE_SYS_H
#define FILE_SYS_H

#include "lib.h"
#include "sys_calls.h"

// Constants
#define BLOCK_SIZE 4096                                         // Block size in bytes (4kB)
#define MAX_FILES 63                                            // Maximum number of files (excluding directory entry)
#define MAX_FILE_NAME 32                                        // Maximum file name length
#define MAX_OPEN_FILES 8                                        // Maximum number of open files per task
#define DIR_ENTRY_SIZE 64                                       // Directory entry size in bytes
#define BOOT_BLOCK_RESERVED 52                                  // Reserved bytes in the boot block
#define INODE_DATA_BLOCKS ((BLOCK_SIZE / sizeof(uint32_t)) - 1) // Adjust for inode structure size


// File types
#define FILE_TYPE_RTC 0
#define FILE_TYPE_DIR 1
#define FILE_TYPE_REG 2

// File system return codes
#define FS_SUCCESS 0
#define FS_ERROR -1

// File descriptor flags
#define FD_ERROR     -1
#define FD_AVAILABLE  0
#define FD_ACTIVE     1

// Directory entry structure
typedef struct
{
    uint8_t name[MAX_FILE_NAME]; // File name
    uint32_t file_type;        // File type
    uint32_t inode_num;       // Inode number
    uint8_t reserved[24];     // Reserved space to fill the structure to 64 bytes
} dir_entry_t;

// Boot block structure
typedef struct
{
    uint32_t num_dir_entries;              // Number of directory entries
    uint32_t num_inodes;                   // Number of inodes
    uint32_t num_data_blocks;              // Number of data blocks
    uint8_t reserved[BOOT_BLOCK_RESERVED]; // Reserved bytes
    dir_entry_t dir_entries[MAX_FILES];    // Directory entries
} boot_block_t;



// Inode structure (for regular files)
typedef struct
{
    uint32_t size;                      // File size in bytes
    uint32_t blocks[INODE_DATA_BLOCKS]; // Data blocks of the file
} inode_t;

// File descriptor structure
typedef struct
{
    void *file_ops;    // Pointer to file operations table
    uint32_t inode;    // Inode number (valid for data files)
    uint32_t position; // Current position in the file
    uint32_t flags;    // File status flags
} file_desc_t;

// Data block structure
typedef struct
{
    uint8_t data[BLOCK_SIZE]; // Data block
} data_block_t;

// Process control block (PCB) structure: will use for CP3
// typedef struct
// {
//     file_desc_t* fd_arr[8]; // (Max 8) active files
//     int32_t active[8];
// } pcb_t;
// pcb_t* pcb; // initialize struct

// file_desc_t fd_arr[8]; // (Max 8) active files

// int i;
// for(i = 0; i < 8; i++){
//     pcb->fd_arr[i].active = -1;
// }

// Function prototypes for file system operations
void fileSystem_init(uint8_t *fs_start); // Function to initialize the file system
int read_dentry_by_name(const uint8_t *fname, dir_entry_t *dentry); // Function to read a directory entry by name
int read_dentry_by_index(uint32_t index, dir_entry_t *dentry); // Function to read a directory entry by index
int read_data(uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t length); // Function to read data from an inode

// Prototypes for file system abstractions
int32_t dir_read(int32_t fd, void *buf, int32_t nbytes); // Read each file name in the directory
int32_t dir_write(int32_t fd, const void *buf, int32_t nbytes); // Write directory to filesystem => Does nothing since read-only file system
int32_t dir_open(const uint8_t *filename); // Opens a directory file (note file types) using read_dentry_by_name, return 0
int32_t dir_close(int32_t fd); // Close directory, Undo dir_open
int32_t file_read(int32_t fd, void *buf, int32_t nbytes); // Reads nbytes bytes of data from file into buf using read_data
int32_t file_write(int32_t fd, const void *buf, int32_t nbytes); // Write file to filesystem => Does nothing since read-only file system
int32_t file_open(const uint8_t *filename); // Initialize any temporary structures, return 0
int32_t file_close(int32_t fd); // Delete any temporary structures (undo tasks in file_open), return 0

// Global pointers for the file system
boot_block_t *g_boot_block;
inode_t *g_inodes;
data_block_t *g_data_blocks;

#endif // FILE_SYS_H
