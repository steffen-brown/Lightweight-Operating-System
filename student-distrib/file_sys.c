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
    // g_boot_block->num_dir_entries = MAX_FILES; // Set the number of directory entries to the maximum number of files
    // g_boot_block->num_inodes = MAX_FILES;      // Set the number of inodes to the maximum number of files
    g_inodes = (inode_t *)(fs_start + BLOCK_SIZE);                                            // inodes start right after the boot block
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
    uint32_t i;
    // Loop through directory entries in the boot block
    for (i = 0; i < g_boot_block->num_dir_entries; i++)
    {
        // Check if the current directory entry matches the provided name
        if (strncmp((const char *)fname, g_boot_block->dir_entries[i].name, MAX_FILE_NAME) == 0)
        {
            // If a match is found, copy the directory entry to the provided dentry structure
            read_dentry_by_index(i, dentry);
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
    // *dentry = g_boot_block->dir_entries[index]; wrong!!
    uint8_t i;
    // Copy the directory entry at the provided index to the provided dentry structure
    dentry->file_type = g_boot_block->dir_entries[index].file_type;
    dentry->inode_num = g_boot_block->dir_entries[index].inode_num;
    for(i = 0; i < MAX_FILE_NAME; i++){
        dentry->name[i] = g_boot_block->dir_entries[index].name[i];
    }
    for ( i = 0; i < 24; i++){
        dentry->reserved[i] = g_boot_block->dir_entries[index].reserved[i];
    }
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

    // Check if inode is valid
    if (inode < 0 || inode >= g_boot_block->num_inodes)
    {
        return -1; // Return error if inode number is invalid
    }

    inode_t *file_inode = &g_inodes[inode]; // Get the inode for the file

    // Adjust length to avoid reading past the end of the file
    if (length > file_inode->size - offset)
    {
        length = file_inode->size - offset;
    }

    uint32_t bytes_read = 0;
    uint32_t block_index = offset / BLOCK_SIZE;
    uint32_t i;
    // // Iterate through each block
    for (i = block_index; i <= block_index + length; i++)
    {
        // Calculate the number of bytes to copy in this iteration
        uint32_t bytes_to_copy = BLOCK_SIZE - (offset % BLOCK_SIZE);
        if ( length < BLOCK_SIZE ) {
            bytes_to_copy = length;
        }
        
    //     // Ensure the block number is valid
    //     if (file_inode->blocks[i] >= g_boot_block->num_data_blocks)
    //     {
    //         break; // Invalid block number, stop reading
    //     }

    //     // Copy data from block to buffer
        data_block_t *data_block = &g_data_blocks[file_inode->blocks[i]];
        memcpy(buf + bytes_read, (uint8_t*)data_block + (offset % BLOCK_SIZE), bytes_to_copy);

    //     // Update the total number of bytes read
        length -= bytes_to_copy;
        bytes_read += bytes_to_copy;
        offset = 0; // Reset offset for the next block
    }

    return bytes_read; // Return the total number of bytes read
}

// -----------------Core Driver Functions (for directory and file)--------------------


/*
 * dir_read
 *   DESCRIPTION: Read each file name in the directory
 *   INPUTS: fd: file descriptor index
 *         buf: buffer to populate with file names
 *         nbytes: number of bytes to read   
 *   OUTPUTS: TODO
 *   RETURN VALUE: TODO
 *   SIDE EFFECTS: TODO
 */
// int32_t dir_read(int32_t fd, void *buf, int32_t nbytes)
// {
//     int dir_idx, name_idx;
//     for(dir_idx = 0; dir_idx < g_boot_block->num_dir_entries; dir_idx++) {
//         dir_entry_t dentry = g_boot_block->dir_entries[dir_idx];
//         if (read_dentry_by_index(dir_idx, &dentry) != -1) {
//             for (name_idx = 0; name_idx < MAX_FILE_NAME; name_idx++) {
//                 printf("%c", dentry.name[name_idx]);
//             }
//             printf("\n");

//             printf("dir_read SUCCESS");

//             return FS_SUCCESS;
//         }
//         else {
//             printf("dir_read SUCCESS");

//             return FS_ERROR;
//         }
//     }
//     printf("dir_read SUCCESS");
//     return FS_SUCCESS;
// }


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
// int32_t dir_open(const uint8_t *filename)
// {
//     dir_entry_t* dentry;
//     if (read_dentry_by_name(filename, dentry) != -1) {
//         printf("dir_open SUCCESS");
//         return FS_SUCCESS;
//     }
//     printf("dir_open FAILURE");
//     return FS_ERROR;
// }


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
// int32_t file_read(int32_t fd, void *buf, int32_t nbytes)
// {
//     if (buf == NULL || nbytes < 0) return FS_ERROR; // Error check 1
    
//     // TODO: IDK how to get this pointer and offset
//     uint32_t inode_ptr = pcb->fd_arr[fd]->inode;
//     inode_t* inode = &g_inodes[inode_ptr];
//     uint32_t offset = pcb->fd_arr[fd]->position;
//     int32_t transfer_size = read_data(inode_ptr, offset, buf, nbytes);
//     if (transfer_size >= 0 && transfer_size <= nbytes){ // Error check 2: Incorrect/invalid number of bytes transferred
//         // TODO: update file cursor line
//         pcb->fd_arr[fd]->position += transfer_size;

//         printf("file_read SUCCESS");

//         return transfer_size;
//     }

//     printf("file_read FAILURE");

//     return FS_ERROR;
// }


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
    // EDGE CASE
    // if (strncmp((const int8_t *)filename, (const int8_t*)"", 1) == 0){
    //     return FS_ERROR;
    // }

    dir_entry_t* dentry;
    int32_t fd;

    int32_t i;
    for ( i = 0; i < 8; i++){
        if (fd_arr[i].flags == FD_AVAILABLE){
            fd = i;
            break;
        }
    }
    if (read_dentry_by_name(filename, dentry) != -1){
        // Check if file is not open (slot available to open)
            fd_arr[fd].flags = FD_ACTIVE;
            fd_arr[fd].position = 0;
            fd_arr[fd].inode = dentry->inode_num;
            // printf("file_open SUCCESS");
            return fd;
    }
    else {
        return FS_ERROR;
    }
}
    


/*
 * file_close
 *   DESCRIPTION: Delete any temporary structures (undo tasks in file_open), return 0
 *   INPUTS: TODO
 *   OUTPUTS: TODO
 *   RETURN VALUE: TODO
 *   SIDE EFFECTS: TODO
 */
// int32_t file_close(int32_t fd)
// {
//     pcb->fd_arr[2]->flags = FD_AVAILABLE;
//     return FS_SUCCESS;
// }
