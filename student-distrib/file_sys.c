#include "file_sys.h"



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
            return i;
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
    uint32_t block_index; 
    uint32_t offset_from_block; // in bytes
    uint32_t bytes_read = 0;
    uint32_t i;
    // // Iterate through each block
    for (i = offset; i < offset + length; i++)
    {

        block_index = i / BLOCK_SIZE;
        offset_from_block = i % BLOCK_SIZE;
     
        // Ensure the block number is valid
        if (file_inode->blocks[block_index] >= g_boot_block->num_data_blocks)
        {
            break; // Invalid block number, stop reading
        }

    //     // Copy data from block to buffer ( 1 byte at a time)
        data_block_t *data_block = &g_data_blocks[file_inode->blocks[block_index]];
        memcpy(buf + bytes_read, (uint8_t*)data_block + offset_from_block, 1);

    // Update the total number of bytes read
        bytes_read += 1;
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
int32_t dir_read(int32_t fd, void *buf, int32_t nbytes)
{
    dir_entry_t dentry;
    int32_t ret = read_dentry_by_index(fd, &dentry);
    if (ret == FS_ERROR)
    {
        return FS_ERROR;
    }
    uint32_t i;
    
    // add file name, file type and size to buffer with File Name: File Type: File Size labels
    for ( i = 0; i < MAX_FILE_NAME; i++)
    {
        if (dentry.name[i] == '\0')
        {
            break;
        }
        ((uint8_t *)buf)[i] = dentry.name[i];
    }
    // ((uint32_t *)buf)[MAX_FILE_NAME] = dentry.file_type;
    // ((uint32_t *)buf)[MAX_FILE_NAME + 4] = g_inodes[dentry.inode_num].size;
    return FS_SUCCESS;
    

    // Check if the file descriptor index is valid

}
 
// function to get file type
int32_t get_file_type(int32_t fd){
    dir_entry_t dentry;
    int32_t ret = read_dentry_by_index(fd, &dentry);
    if (ret == FS_ERROR)
    {
        return FS_ERROR;
    }
    return dentry.file_type;
}
// function to get file size
int32_t get_file_size(int32_t fd){
    dir_entry_t dentry;
    int32_t ret = read_dentry_by_index(fd, &dentry);
    if (ret == FS_ERROR)
    {
        return FS_ERROR;
    }
    return g_inodes[dentry.inode_num].size;
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
    if (read_dentry_by_name(filename, dentry) != -1) {
        printf("dir_open SUCCESS");
        return FS_SUCCESS;
    }
    printf("dir_open FAILURE");
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
    
    uint32_t inode_ptr = g_boot_block->dir_entries[fd].inode_num;
    uint32_t file_size = g_inodes[inode_ptr].size; // size in bytes
    uint32_t offset = 0;
    int32_t transfer_size = read_data(inode_ptr, offset, buf, file_size);
    if (transfer_size != -1) {
        return FS_SUCCESS;
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

    dir_entry_t dentry;
    uint32_t ret = read_dentry_by_name(filename, &dentry);
    if (ret != -1){
            return ret;
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
int32_t file_close(int32_t fd)
{
    return FS_SUCCESS;
}
