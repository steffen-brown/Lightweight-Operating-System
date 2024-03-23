#include "file_sys.h"

// Global pointers for the file system
boot_block_t *g_boot_block;
inode_t *g_inodes;
data_block_t *g_data_blocks;


/*
 * fileSystem_init
 *   DESCRIPTION: Function to initialize the file system
 *   INPUTS: TODO
 *   OUTPUTS: TODO
 *   RETURN VALUE: TODO
 *   SIDE EFFECTS: TODO
 */
void fileSystem_init(uint8_t *fs_start)
{
    // Set global pointers to their respective locations in the file system memory
    g_boot_block = (boot_block_t *)fs_start;
    g_inodes = (inode_t *)(fs_start + BLOCK_SIZE); // inodes start right after the boot block
    g_data_blocks = (data_block_t *)(fs_start + (1 + g_boot_block->num_inodes) * BLOCK_SIZE); // data blocks start right after bootblock + all inodes
}



// -----------------read dentry by name, read dentry by index and read data--------------------


/*
 * read_dentry_by_name
 *   DESCRIPTION: Function to read a directory entry by name
 *   INPUTS: TODO
 *   OUTPUTS: TODO
 *   RETURN VALUE: 0=FS_SUCCESS:success, -1=FS_ERROR:failure
 *   SIDE EFFECTS: TODO
 */
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


/*
 * read_dentry_by_index
 *   DESCRIPTION: Function to read a directory entry by index
 *          NOTE: "index" is NOT inode number. It is the index in boot block
 *   INPUTS: TODO
 *   OUTPUTS: TODO
 *   RETURN VALUE: TODO
 *   SIDE EFFECTS: TODO
 */
int32_t read_dentry_by_index(uint32_t index, dir_entry_t *dentry)
{
    // Error if index is out of bounds
    if (index >= g_boot_block->num_dir_entries || index < 0)
    {
        return FS_ERROR;
    }
    // Copy the directory entry at the provided index to the provided dentry structure
    *dentry = g_boot_block->dir_entries[index];
    return FS_SUCCESS;
}


/*
 * read_data
 *   DESCRIPTION: Function to read data from an inode
 *   INPUTS: TODO
 *   OUTPUTS: TODO
 *   RETURN VALUE: TODO
 *   SIDE EFFECTS: TODO
 */
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



// -----------------Core Driver Functions (for directory and file)--------------------


/*
 * dir_read
 *   DESCRIPTION: Read files filename by filename, including “.”
 *   INPUTS: TODO
 *   OUTPUTS: TODO
 *   RETURN VALUE: TODO
 *   SIDE EFFECTS: TODO
 */
int32_t dir_read(int32_t fd, void *buf, int32_t nbytes)
{
    // TODO
}


/*
 * dir_write
 *   DESCRIPTION: Write directory to filesystem => Does nothing since read-only file system
 *   INPUTS: TODO
 *   OUTPUTS: TODO
 *   RETURN VALUE: TODO
 *   SIDE EFFECTS: TODO
 */
int32_t dir_write(int32_t fd, const void *buf, int32_t nbytes)
{
    return FS_ERROR; // Return error (directories are read-only)
}


/*
 * dir_open
 *   DESCRIPTION: Opens a directory file (note file types) using read_dentry_by_name, return 0
 *   INPUTS: TODO
 *   OUTPUTS: TODO
 *   RETURN VALUE: TODO
 *   SIDE EFFECTS: TODO
 */
int32_t dir_open(const uint8_t *filename)
{
    dir_entry_t* dentry;
    if (read_dentry_by_name(filename, dentry) != -1){
        return FS_SUCCESS;
    }
    return FS_ERROR;
}


/*
 * dir_close
 *   DESCRIPTION: TODO
 *   INPUTS: TODO
 *   OUTPUTS: TODO
 *   RETURN VALUE: TODO
 *   SIDE EFFECTS: TODO
 */
int32_t dir_close(int32_t fd)
{
    return FS_SUCCESS; // Return success
}


/*
 * file_read
 *   DESCRIPTION: Reads nbytes bytes of data from file into buf using read_data
 *   INPUTS: TODO
 *   OUTPUTS: TODO
 *   RETURN VALUE: TODO
 *   SIDE EFFECTS: TODO
 */
int32_t file_read(int32_t fd, void *buf, int32_t nbytes)
{
    if (buf == NULL || nbytes < 0) return FS_ERROR; // Error check 1
    
    // TODO: IDK how to get this pointer and offset
    uint32_t inode_ptr = NULL;
    uint32_t offset = 0;
    int32_t transfer_size = read_data(inode_ptr, offset, buf, nbytes);
    if (transfer_size >= 0 && transfer_size <= nbytes){ // Error check 2: Incorrect/invalid number of bytes transferred
        // TODO: update file cursor line
        
        return transfer_size;
    }
    return FS_ERROR;
}


/*
 * file_write
 *   DESCRIPTION: Write file to filesystem => Does nothing since read-only file system
 *   INPUTS: TODO
 *   OUTPUTS: TODO
 *   RETURN VALUE: TODO
 *   SIDE EFFECTS: TODO
 */
int32_t file_write(int32_t fd, const void *buf, int32_t nbytes)
{
    return FS_ERROR; // Return error (files are read-only)
}


/*
 * file_open
 *   DESCRIPTION: Initialize any temporary structures, return 0
 *   INPUTS: TODO
 *   OUTPUTS: TODO
 *   RETURN VALUE: TODO
 *   SIDE EFFECTS: TODO
 */
int32_t file_open(const uint8_t *filename)
{
    if (strncmp((const int8_t *)filename, (const int8_t*)"", 1) == 0){
        return FS_ERROR;
    }
    dir_entry_t* dentry;
    return read_dentry_by_name(filename, dentry);
}


/*
 * file_close
 *   DESCRIPTION: Delete any temporary structures (undo tasks in file_open), return 0
 *   INPUTS: TODO
 *   OUTPUTS: TODO
 *   RETURN VALUE: TODO
 *   SIDE EFFECTS: TODO
 */
int32_t file_close(int32_t fd)
{
    return FS_SUCCESS;
}

// Function to read a file by name
