#include "sys_calls.h"

// The currently active process control block index, initially 0
static int active_pcb = 0;

/*
 * Halts a process and handles the termination or switching to another process.
 * INPUTS: status - The status code for the halt operation.
 * RETURNS: The status of the program.
 * SIDE EFFECTS: Terminates the current process and may switch to another process.
 */
int32_t halt(uint32_t status) {
    ProcessControlBlock* current_pcb;
    // Assembly code to get the current PCB
    // Mask the lower 13 bits then AND with ESP to align it to the 8KB boundary
    asm volatile (
        "movl %%esp, %%eax\n"       // Move current ESP value to EAX for manipulation
        "andl $0xFFFFE000, %%eax\n" // Clear the lower 13 bits to align to 8KB boundary
        "movl %%eax, %0\n"          // Move the modified EAX value to current_pcb
        : "=r" (current_pcb)        // Output operands
        :                            // No input operands
        : "eax"                      // Clobber list, indicating EAX is modified
    );
    uint32_t return_value = (uint32_t)(status & 0xFF); // Extracts the least significant 8 bits of the status and uses it as the return value
    if (status == 256){ // If status is 256 (specific case), set return value to 0x100.
        return_value = 0x100; // program terminated by exception
    }
    
    int i;
    // Close any open files
    for (i = 0; i < 8; i++) {
        if (current_pcb->files[i].flags != 0) { // '0' means the file is not in use

            if (current_pcb->files[i].operationsTable.close != NULL) { // Check if the close operation is available
                current_pcb->files[i].operationsTable.close(i); // Close the file
            }
            current_pcb->files[i].flags = 0; // Reset the file descriptor to indicate it's available
        }
    }

    // Disable the virtual memory address for user video memory
    pt_entry_t disabled_entry;
    disabled_entry.p = 0;
    pt_vidmap[0] = disabled_entry.val;

    pdt_entry_table_t disabled_table_entry;
    disabled_table_entry.p = 0; // present
    pdt[VID_PDT_IDX] = disabled_table_entry.val;

    // Set the exit status in the PCB
    current_pcb->exitStatus = status;

    // Special handling for when the shell (process ID 1) is halted.
    if (current_pcb->processID == 1) {
        // If the current process is the shell, restart the shell
        active_pcb--;
        execute((uint8_t*)"shell");
    } else {
        pdt_entry_page_t new_page;
        // If the current process is not the shell, return to the parent process
        // Restore parent paging
        pdt_entry_page_setup(&new_page, 0x2, 1); // Create entry for 0x02 (zero indexed) 4mb page in user mode (1)
        pdt[32] = new_page.val; // Restore paging parent into the 32nd (zero indexed) 4mb virtual memory page
        flush_tlb();// Flushes the Translation Lookaside Buffer (TLB)

        // Sets the kernel stack pointer for the task state segment (TSS) to the parent's kernel stack.
        // The calculation for esp0 adjusts the stack pointer based on the process ID, ensuring each process has a unique kernel stack in memory.
        // The magic number 0x800000 represents the start of kernel memory, and 0x2000 (8KB) is the size allocated for each process's kernel stack.
        tss.esp0 = (uint32_t)(0x800000 - (current_pcb->parentProcessID ) * 0x2000); // Adjusts ESP0 for the parent process.
        tss.ss0 = KERNEL_DS; // Sets the stack segment to the kernel's data segment.
        // Restore parent process control block
        active_pcb--; // Decrement the active process count to reflect the process termination.
    }
    
    uint32_t parent_ebp = (uint32_t)current_pcb->oldEBP;// Retrieves the saved Base Pointer (EBP) of the parent process.
    halt_return(parent_ebp, parent_ebp, return_value); // Return to parent process

    return 0; // Never reached
}

// Global definition of file operation function pointers for directory files.
FileOperationsTable dir_operations_table = {
    .read = dir_read,
    .write = dir_write,
    .open = dir_open,
    .close = dir_close
};

// Global definition of file operation function pointers for regular files.
FileOperationsTable file_operations_table = {
    .read = file_read,
    .write = file_write,
    .open = file_open,
    .close = file_close
};

// Definition of stdout file descriptor.
const FileDescriptor stdout_fd = {
    .operationsTable = {
        .read = NULL,
        .write = terminal_write,
        .open = terminal_open,
        .close = terminal_close
    },
    .inode = 0, // 0 for RTC and device files
    .filePosition = 0, // Number reading from file
    .flags = 0 // Blank flags
};

// Define stdin file descriptor globally
const FileDescriptor stdin_fd = {
    .operationsTable = {
        .read = terminal_read,
        .write = NULL,
        .open = terminal_open,
        .close = terminal_close
    },
    .inode = 0, // 0 for RTC and device files
    .filePosition = 0, // Number reading from file
    .flags = 0 // Blank flags
};

// Global definition of file operation function pointers for RTC (Real-Time Clock) files.
FileOperationsTable rtc_operations_table = {
    .read = rtc_read,
    .write = rtc_write,
    .open = rtc_open,
    .close = rtc_close
};


/*
 * Attempts to load and execute a new program, replacing the current executing program.
 *
 * Inputs: command_user - The user command including the program name and possibly arguments.
 * Returns: -1 if the command is invalid or fails to execute, otherwise returns a status code upon exiting the new program.
 * Side Effects: Loads a new program into memory, potentially replacing the currently running program.
 */
int32_t execute(const uint8_t* command_user) {
    uint8_t file_name[32]; // Buffer to store the extracted file name from the command.
    dir_entry_t cur_dentry; // Directory entry structure to hold file metadata.
    int file_length; // Variable to store the length of the file being executed.
    uint8_t file_buffer[6000]; // Buffer to temporarily hold the file contents. The size 6000 is chosen based on the expected maximum size of an executable.
    uint8_t command[128]; // Buffer to copy the user command (max size 128) to avoid modifying the original.
    int cnt;

    // Copies the command from user space to a local buffer to ensure safe manipulation.
    for (cnt = 0; cnt < 128; cnt++){
        command[cnt] = command_user[cnt];
    }

    // Extract file name from the command.
    int args_idx;
    for (args_idx = 0; command[args_idx] != ' ' && command[args_idx] != '\0'; args_idx++) { // get file name from command argument
        file_name[args_idx] = command[args_idx];
    }

    // Check if the file exists in the directory.
    file_name[args_idx] = '\0';
    
    if (read_dentry_by_name(file_name, &cur_dentry) == -1) { // check if executable file exists
        RETURN(-1); // Return command not found
    }

    // 6000: Max length of file buffer
    if ((file_length = read_data(cur_dentry.inode_num, 0, file_buffer, 6000)) == -1) { // check if inode is valid
        RETURN(-1); // Return command not found
    }

    // 0x7F: DEL, 0x45: E, 0x4C: L, 0x46: F
    if (file_buffer[0] != 0x7f || file_buffer[1] != 0x45 || file_buffer[2] != 0x4c || file_buffer[3] != 0x46) { // check ELF for exe
        RETURN(-1); // Return command not found
    }

    if(active_pcb + 1 > 2) { // Limit the number of processes running to 2
        RETURN(1);
    }
    int new_PID = ++active_pcb; // The PID of the process that is being execuated

    pdt_entry_page_t new_page;

    if(new_PID == 1) { // If the new page has PID #1
        // Set up page table entry pointing mapping 128mb (virtual) to 8mb (phyiscal)
        // populate table entry
        pdt_entry_page_setup(&new_page, 0x2, 1); // Get page directory entry for 0x2 4mb page (zero-idxed) with user permissions (1)
    } else if(new_PID == 2) { // If the new page has PID #2
        // Set up page table entry pointing mapping 128mb (virtual) to 12mb (phyiscal)
        // populate table entry
        pdt_entry_page_setup(&new_page, 0x3, 1); // Get page directory entry for 0x3 4mb page (zero-idxed) with user permissions (1)
    }
    // Update virtual memory address 128mb (32nd 4mb page - zero indexed)
    pdt[32] = new_page.val;
    flush_tlb(); // Flush the TLB

    tss.esp0 = 0x800000 - new_PID * 0x2000; // Update the new stack pointer ESP_new = 8MB - PID * 8KB (0x2000)
    // Maybe update tss.ebp

    // Load the file into the correct memory location
    memcpy((void*)PROGRAM_START, (void*)file_buffer, 6000); // 6000: The max file length

    // Create PCB at top of new process kernal stack
    ProcessControlBlock* new_PCB = (void*)(0x800000 - (new_PID + 1) * 0x2000); // Update the new PCB pointer - new_PCB = 8MB - (PID + 1) * 8KB (0x2000)
    new_PCB->exitStatus = 0;
    new_PCB->processID = new_PID;
    new_PCB->parentProcessID = new_PID - 1; // Revise to be less hard coded?
    new_PCB->pcbPointerToParent = (void*)(0x800000 - (new_PCB->parentProcessID + 1) * 0x2000); // Update the new PCB parent pointer - new_PCB = 8MB - (parent PID + 1) * 8KB (0x2000)

    // Add stdin and stdout to the file descriptor array
    new_PCB->files[0] = stdin_fd;
    new_PCB->files[0].flags = 1; // Active

    new_PCB->files[1] = stdout_fd;
    new_PCB->files[1].flags = 1; // Active

    // clear args buffer
    int i;
    for (i = 0; i < argsBufferSize; i++) {
        new_PCB->args[i] = '\0';
    }
    // remove spaces from command
    while (command[args_idx] == ' ') {
        args_idx++;
    }
    // copy args to PCB
    for (i = 0; i < argsBufferSize; i++) {
        if (command[args_idx] == '\0') {
            new_PCB->args[i] = '\0';
            break;
        }
        new_PCB->args[i] = command[args_idx];
        args_idx++;
    }
    // Set up context switch
    uint32_t ss = USER_DS;
    uint32_t esp = 0x8400000 - 4; // one int32 above the bottom of the user space
    uint32_t eflags = 0x00000202; // Allow interrupts
    uint32_t cs = USER_CS;
    uint32_t eip = 0 | (uint32_t)file_buffer[27] << 24 | (uint32_t)file_buffer[26] << 16 | (uint32_t)file_buffer[25] << 8 | (uint32_t)file_buffer[24];
    // Shifts bytes of the ESP from file approriate amount to form file EIP.

    // Context switch
    asm volatile (
        "push %0\n"       // Push SS
        "push %1\n"       // Push ESP
        "push %2\n"       // Push EFLAGS
        "push %3\n"       // Push CS
        "push %4\n"       // Push EIP

        // put esp in kernalStackTop
        // put eip in execReturnAddr

        : 
        : "r" (ss), "r" (esp), "r" (eflags), "r" (cs), "r" (eip)
        : "memory"
    );

    // Save the current EBP in the PCB for later return in halt
    register uint32_t saved_ebp asm("ebp");
    new_PCB->oldEBP = (void*)saved_ebp;

    asm volatile (
        "iret\n"          // Return from interrupt
    );

    RETURN(0); // Never reached

    return 0;
}

/*
 * int32_t read(int32_t fd, void* buf, int32_t nbytes)
 *  DESCRIPTION: reads data from a file or device
 *  INPUTS: file descriptor, buffer containing data to read from, num of bytes to be read
 *  RETURN VALUE: -1 if invalid read
 *  SIDE EFFECTS: NONE
 */
int32_t read(int32_t fd, void* buf, int32_t nbytes) {
    if (fd < 0 || fd > 7 || !buf || nbytes < 0) {
        RETURN(-1); // Return error
    }
    if ( fd == 1){ // edge case ( can't read from stdout)
        RETURN(-1); // Return error
    }

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

    if (current_pcb->files[fd].flags == 0) { // Check if file descriptor is in use
        RETURN(-1); // Retrun error
    }
    int bytes = current_pcb->files[fd].operationsTable.read(fd, buf, nbytes); // Call read for the approriate file decriptor

    RETURN(bytes); // Return the number of bytes read

    return 0;
}

/*
 * int32_t write(int32_t fd, const void* buf, int32_t nbytes)
 *  DESCRIPTION: writes data to screen
 *  INPUTS: file descriptor, buffer containing data to write to, num of bytes to write
 *  RETURN VALUE: -1 if invalid write
 *  SIDE EFFECTS: Prints to terminal
 */
int32_t write(int32_t fd, const void* buf, int32_t nbytes) {
    if (fd < 0 || fd > 7 || !buf || nbytes < 0) { // Check that the file decriptor is in the valid range (0-7)
        RETURN(-1);
    }
    if ( fd == 0){ // edge case ( can't write to stdin)
        RETURN(-1); // Return error
    }

    ProcessControlBlock* current_pcb;
    asm volatile (
        "movl %%esp, %%eax\n"       // Move current ESP value to EAX for manipulation
        "andl $0xFFFFE000, %%eax\n" // Clear the lower 13 bits to align to 8KB boundary
        "movl %%eax, %0\n"          // Move the modified EAX value to current_pcb
        : "=r" (current_pcb)        // Output operands
        :                            // No input operands
        : "eax"                      // Clobber list, indicating EAX is modified
    );

    if (current_pcb->files[fd].flags == 0) { // Check if file descriptor is in use
        RETURN(-1); // Return error
    }
    int bytes = current_pcb->files[fd].operationsTable.write(fd, buf, nbytes);  // Call write for the approriate file decriptor

    RETURN(bytes); // Return the number of bytes read

    return 0;
}

/*
 * int32_t open(const uint8_t* filename)
 *  DESCRIPTION: opens a file
 *  INPUTS: name of file
 *  RETURN VALUE: -1 if invalid open
 *  SIDE EFFECTS: Sets up file descriptor for opened file
 */
int32_t open(const uint8_t* filename) {
    // Sets up file descriptor
    // Calls file_open
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
    uint32_t i = 0;
    for (i  = 2; i < 8; i++) { // Loop through all 8 file descriptors
        if (current_pcb->files[i].flags == 0) { // Check if file descriptor is available
            break;
        }
    }
    if (i == 8) { // No available file descriptors
        RETURN(-1);
    }
    dir_entry_t dentry;
    if (read_dentry_by_name(filename, &dentry) == -1) // get dentry
        RETURN(-1);
    // get the file type
    uint32_t file_type = dentry.file_type;
    // Update attributes of the file desriptors according to type
    if ( file_type == 0) { // RTC file
        current_pcb->files[i].operationsTable = rtc_operations_table;
        current_pcb->files[i].filePosition = 0;
        current_pcb->files[i].flags = 1;
        RETURN(i); // Return FD number
    } else if (file_type == 1) { // Directory file
        current_pcb->files[i].operationsTable = dir_operations_table;
        current_pcb->files[i].filePosition = 0;
        current_pcb->files[i].flags = 1;
        RETURN(i); // Return FD number
    } else if (file_type == 2) { // Regular file
        current_pcb->files[i].operationsTable = file_operations_table;
        current_pcb->files[i].inode = dentry.inode_num;
        current_pcb->files[i].filePosition = 0;
        current_pcb->files[i].flags = 1;
        RETURN(i); // Return FD number
    }
    
    
    RETURN(-1); // Return error

    return 0;
}

/*
 * int32_t close(int32_t fd)
 *  DESCRIPTION: closes a file
 *  INPUTS: file descriptor for file to close
 *  RETURN VALUE: -1 if invalid close
 *  SIDE EFFECTS: Allows for fd to be used again
 */
int32_t close(int32_t fd) {
    // check if fd less than 2 or greater than 7
    if (fd < 2 || fd > 7) {
        RETURN(-1);
    }
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
    if (current_pcb->files[fd].flags == 0) { // If file descriptor is not in use (0)
        RETURN(-1); // Return error
    }
    current_pcb->files[fd].flags = 0; // Set file descriptor flags to not in use
    int ret = current_pcb->files[fd].operationsTable.close(fd); // Return approriate close function

    RETURN(ret); // Return the status of the close

    return 0;
}


int32_t getargs(uint8_t* buf, int32_t nbytes) {
    if (buf == NULL || nbytes < 0 || nbytes > argsBufferSize) {
        RETURN(-1);
    }
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

    // Return error if there are no args
    if(current_pcb->args[0] == '\0' || current_pcb->args[0] == NULL) {
        RETURN(-1);
        return -1;
    }

    // Copy the args into the user space buffer
    int i;
    for (i = 0; i < nbytes; i++) {
        if(current_pcb->args[i] == '\0') {
            ((uint8_t*)buf)[i] = '\0';
            break;
        }
        ((uint8_t*)buf)[i] = current_pcb->args[i];
    }
    RETURN(0); // Return success
    return 0;
}


int32_t vidmap(uint8_t** screen_start) {
    // Bound checks
    uint32_t vid_addr = (uint32_t)screen_start;
    if (screen_start == NULL) { // invalid screen start
        RETURN(-1);
        return -1;
    }
    if (vid_addr > USER_STACK || vid_addr < USER_STACK - 0x400000) {
        RETURN(-1);
        return -1;
    }

    // Paging
    pdt_entry_table_t vidmem;
    vidmem.p = 1; // present
    vidmem.us = 1; // user
    vidmem.rw = 1;
    vidmem.address = ((int)pt_vidmap)/4096; // 4096 = 4kB
    pdt[VID_PDT_IDX] = vidmem.val;

    pt_entry_t vidmem_pt;
    vidmem_pt.p = 1; // present
    vidmem_pt.us = 1; // user
    vidmem_pt.rw = 1;
    vidmem_pt.address_31_12 = VID_MEM_PHYSICAL/4096; // 4096 = 4kB
    pt_vidmap[0] = vidmem_pt.val;

    flush_tlb();
    *screen_start = (uint8_t*)VID_MEM;

    RETURN(0);
    return 0;
}


int32_t set_handler(int32_t signum, void* handler_address) {
    return 0;
}


int32_t sigreturn(void) {
    return 0;
}
