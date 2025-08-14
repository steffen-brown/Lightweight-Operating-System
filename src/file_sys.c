#include "file_sys.h"



/*
 * fileSystem_init
 *   DESCRIPTION: Function to initialize the file system
 *   INPUTS: uint8_t* fs_start: file name to start
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
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
 *   INPUTS: fname: filename
 *           dentry: directory entry to populate
 *   RETURN VALUE: dentry idx:success, -1=FS_ERROR:failure. Also dentry used as both input and output
 *   SIDE EFFECTS: none
 */
int32_t read_dentry_by_name(const uint8_t *fname, dir_entry_t *dentry)
{
    if (fname == NULL || strlen((int8_t*)fname) < 1 || strlen((int8_t*)fname) > MAX_FILE_NAME) 
    {return -1;}
    uint32_t i;
    
    // Loop through directory entries in the boot block
    for (i = 0; i < g_boot_block->num_dir_entries; i++)
    {
        // Check if the current directory entry matches the provided name
        if (strncmp((const int8_t *)fname, (const int8_t *)g_boot_block->dir_entries[i].name, MAX_FILE_NAME) == 0)
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
 *   INPUTS: index: dentry index
 *           dentry: directory entry struct to populate with details if dir entry found
 *   RETURN VALUE: 0 for success, -1 for fail. Also dentry used as both input and output
 *   SIDE EFFECTS: none
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
 *   INPUTS: inode: inode number
 *           offset: offset within data block
 *           buf: buffer to fill with data read
 *           length: bytes to read (should be <= buf size)
 *   RETURN VALUE: number of bytes read. Also buf used as both input and output
 *   SIDE EFFECTS: none
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

    return (int32_t)bytes_read; // Return the total number of bytes read
}

// -----------------Core Driver Functions (for directory and file)--------------------


/*
 * dir_read
 *   DESCRIPTION: Read each file name in the directory
 *   INPUTS: fd: file descriptor index
 *         buf: buffer to populate with file names
 *         nbytes: number of bytes to read   
 *   RETURN VALUE: 0 for success, -1 for fail
 *   SIDE EFFECTS: none
 */
int32_t dir_read(int32_t fd, void *buf, int32_t nbytes)
{
     ProcessControlBlock* current_pcb;
    // Assembly code to get the current PCB
    // Clear the lower 13 bits then AND with ESP to align it to the 8KB boundary
    asm volatile (
        "movl %%esp, %%eax\n"       // Move current ESP value to EAX for manipulation
        "andl $0xFFFFE000, %%eax\n" // Clear the lower 13 bits to align to 8KB boundary
        "movl %%eax, %0\n"          // Move the modified EAX value to current_pcb
        : "=r" (current_pcb)        // Output operands
        :                            // No input operands
        : "eax"                      // Clobber list, indicating EAX is modified
    );
    uint32_t start = current_pcb->files[fd].filePosition;
    if( current_pcb->files[fd].filePosition >= g_boot_block->num_dir_entries){
        current_pcb->files[fd].filePosition = 0;
        return FS_SUCCESS;
    }
    current_pcb->files[fd].filePosition++;
    dir_entry_t dentry;
    int32_t ret = read_dentry_by_index(start, &dentry);
    if (ret == FS_ERROR)
    {
        return FS_ERROR;
    }
    uint32_t i;
    
    // add file name, file type and size to buffer with File Name: File Type: File Size labels
    for ( i = 0; i < nbytes; i++)
    {
        if (dentry.name[i] == '\0')
        {
            break;
        }
        ((uint8_t *)buf)[i] = dentry.name[i];
    }
    // ((uint32_t *)buf)[MAX_FILE_NAME] = dentry.file_type;
    // ((uint32_t *)buf)[MAX_FILE_NAME + 4] = g_inodes[dentry.inode_num].size;
    return i;
    
    // Check if the file descriptor index is valid
}
 
// // helper function to get file type
// int32_t get_file_type(int32_t fd){
//     dir_entry_t dentry;
//     int32_t ret = read_dentry_by_index(fd, &dentry);
//     if (ret == FS_ERROR)
//     {
//         return FS_ERROR;
//     }
//     return dentry.file_type;
// }
// // helper function to get file size
// int32_t get_file_size(int32_t fd){
//     dir_entry_t dentry;
//     int32_t ret = read_dentry_by_index(fd, &dentry);
//     if (ret == FS_ERROR)
//     {
//         return FS_ERROR;
//     }
//     return g_inodes[dentry.inode_num].size;
// }


/*
 * dir_write
 *   DESCRIPTION: Write directory to filesystem => Does nothing since read-only file system
 *   INPUTS: fd: filedir num in file system struct
 *           buf: buffer with data to write
 *           nbytes: number of bytes of data to write
 *   RETURN VALUE: -1 because read onyl file system
 *   SIDE EFFECTS: none
 */
int32_t dir_write(int32_t fd, const void *buf, int32_t nbytes)
{
    return FS_ERROR; // Return error (directories are read-only)
}


/*
 * dir_open
 *   DESCRIPTION: Opens a directory file (note file types) using read_dentry_by_name, return 0
 *   INPUTS: filename: name of dir to open
 *   RETURN VALUE: 0 for success, -1 for failure
 *   SIDE EFFECTS: none
 */
int32_t dir_open(const uint8_t *filename)
{
    dir_entry_t* dentry;
    if (read_dentry_by_name(filename, dentry) != -1) {
        return FS_SUCCESS;
    }
    return FS_ERROR;
}


/*
 * dir_close
 *   DESCRIPTION: Close directory, Undo dir_open
 *   INPUTS: fd: file to close
 *   RETURN VALUE: 0:file closed
 *   SIDE EFFECTS: none
 */
int32_t dir_close(int32_t fd)
{
    return FS_SUCCESS; // Return success
}


/*
 * file_read
 *   DESCRIPTION: Reads nbytes bytes of data from file into buf using read_data
 *   INPUTS: fd: file to read from
 *           buf: buffer to fill with read data
 *           nbytes: number of data bytes to read
 *   RETURN VALUE: number of bytes read if success, -1 if fail
 *   SIDE EFFECTS: none
 */
int32_t file_read(int32_t fd, void *buf, int32_t nbytes)
{
    ProcessControlBlock* current_pcb;
    // Assembly code to get the current PCB
    // Clear the lower 13 bits then AND with ESP to align it to the 8KB boundary
    asm volatile (
        "movl %%esp, %%eax\n"       // Move current ESP value to EAX for manipulation
        "andl $0xFFFFE000, %%eax\n" // Clear the lower 13 bits to align to 8KB boundary
        "movl %%eax, %0\n"          // Move the modified EAX value to current_pcb
        : "=r" (current_pcb)        // Output operands
        :                            // No input operands
        : "eax"                      // Clobber list, indicating EAX is modified
    );
    uint32_t inode_ptr = current_pcb->files[fd].inode;
    // uint32_t inode_ptr = g_boot_block->dir_entries[fd].inode_num;
    uint32_t file_size = g_inodes[inode_ptr].size; // size in bytes
    if(current_pcb->files[fd].filePosition >= file_size){ // if file position is at or past the end of the file
        current_pcb->files[fd].filePosition = 0; // reset file position
        return 0;
    }
    uint32_t offset = current_pcb->files[fd].filePosition;
    int32_t transfer_size = read_data(inode_ptr, offset, buf, nbytes);
    if (transfer_size == -1) { // if read_data returns -1, return error
         return FS_ERROR;
    }
    else {
        current_pcb->files[fd].filePosition += transfer_size; // update file position based on number of bytes read
        return transfer_size;
    }
   
}


/*
 * file_write
 *   DESCRIPTION: Write file to filesystem => Does nothing since read-only file system
 *   INPUTS: fd: file to write to
 *           buf: buffer with data to write
 *           nbytes: number of data bytes to write
 *   RETURN VALUE: -1 error because read only file system
 *   SIDE EFFECTS: none
 */
int32_t file_write(int32_t fd, const void *buf, int32_t nbytes)
{
    return FS_ERROR; // Return error (files are read-only)
}

/*
 * file_open
 *   DESCRIPTION: Initialize any temporary structures, return 0
 *   INPUTS: filename: file to open
 *   RETURN VALUE: dir entry index if success, -1 if not
 *   SIDE EFFECTS: none
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
 *   INPUTS: fd: file to close
 *   RETURN VALUE: 0:file closed successfully
 *   SIDE EFFECTS: none
 */
int32_t file_close(int32_t fd)
{
    return FS_SUCCESS;
}
