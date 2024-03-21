#include "file_sys.h"

// Global pointers for the file system
boot_block_t *g_boot_block;
inode_t *g_inodes;
data_block_t *g_data_blocks;

// Function to initialize the file system
void fileSystem_init(uint8_t *fs_start)
{
    // Set global pointers to their respective locations in the file system memory
    g_boot_block = (boot_block_t *)fs_start;
    g_inodes = (inode_t *)(fs_start + BLOCK_SIZE);                                            // inodes start right after the boot block
    g_data_blocks = (data_block_t *)(fs_start + (1 + g_boot_block->num_inodes) * BLOCK_SIZE); // data blocks start right after bootblock + all inodes
}

// Function to read a directory entry by name
int32_t read_dentry_by_name(const uint8_t *fname, dir_entry_t *dentry)
{
    // Loop through directory entries in the boot block
    for (uint32_t i = 0; i < g_boot_block->num_dir_entries; i++)
    {
        // Check if the current directory entry matches the provided name
        if (strncmp((const char *)fname, g_boot_block->dir_entries[i].name, MAX_FILE_NAME) == 0)
        {
            // If a match is found, copy the directory entry to the provided dentry structure
            *dentry = g_boot_block->dir_entries[i];
            return FS_SUCCESS;
        }
    }
    // Return error if the name is not found
    return FS_ERROR;
}

// Function to read a directory entry by index
int32_t read_dentry_by_index(uint32_t index, dir_entry_t *dentry)
{
    // Check if the index is within range
    if (index >= g_boot_block->num_dir_entries || index < 0)
    {
        return FS_ERROR; // Return error if index is out of range
    }
    // Copy the directory entry at the provided index to the provided dentry structure
    *dentry = g_boot_block->dir_entries[index];
    return FS_SUCCESS;
}

// Function to read data from an inode
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t length)
{
    if (inode >= g_boot_block->num_inodes || inode < 0)
    {
        return FS_ERROR; // Return error if inode number is invalid
    }

    inode_t *file_inode = (inode_t *)&g_inodes[inode]; // Get the inode for the file

    uint32_t bytes_read = 0;

    // Calculate the starting block index based on the offset
    uint32_t block_index = offset / BLOCK_SIZE;
    uint32_t block_offset = offset % BLOCK_SIZE;

    while (bytes_read < length && block_index < INODE_DATA_BLOCKS)
    {
        uint32_t block_num = file_inode->blocks[block_index];
        // Ensure the block number is valid
        if (block_num >= g_boot_block->num_data_blocks)
        {
            break; // Invalid block number, stop reading
        }
        data_block_t *data_block = &g_data_blocks[block_num];
        uint32_t bytes_to_copy = min(BLOCK_SIZE - block_offset, length - bytes_read);

        // Copy data from block to buffer
        memcpy(buf + bytes_read, data_block->data + block_offset, bytes_to_copy);
        bytes_read += bytes_to_copy;

        // Move to the next block
        block_index++;
        block_offset = 0; // Reset offset for the next block
    }

    return bytes_read; // Return the number of bytes read
}

// Function to read a file by name
