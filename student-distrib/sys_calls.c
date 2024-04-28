#include "sys_calls.h"
#include "pit.h"

// The currently active process control block index, initially 0
int aux_processes = 0; // Number of non base shell active processes
uint8_t base_shell_live_bitmask = 0x00; // Representing shells currently open, Shell 3 | Shell 2 | Shell 1 (LSB)
uint8_t base_shell_booted_bitmask = 0x00; // Representing shells currently booted, Shell 3 | Shell 2 | Shell 1 (LSB) 
int shell_init_boot = 1; // Global variable used to boot the correct shell
uint8_t active_processes[6]; // Array to store the active processes

/*
 * Halts a process and handles the termination or switching to another process.
 * INPUTS: status - The status code for the halt operation.
 * RETURNS: The status of the program.
 * SIDE EFFECTS: Terminates the current process and may switch to another process.
 */
int32_t halt(uint32_t status) {
    cli();
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

    // Set the exit status in the PCB
    current_pcb->exitStatus = status;

    // Special handling for when the shell (process ID 1) is halted.
    if (current_pcb->processID >= 1 && current_pcb->processID <= 3) {
        // If the current process is the shell, restart the shell
        base_shell_live_bitmask &= ~(1 << (current_pcb->processID - 1));
        execute((uint8_t*)"shell");
    } else {
        pdt_entry_page_t new_page;
        // If the current process ics not the shell, return to the parent process
        // Restore parent paging
        
        pdt_entry_page_setup(&new_page, ((ProcessControlBlock*)current_pcb->parentPCB)->processID + 1, 1); // Create entry for 0x02 (zero indexed) 4mb page in user mode (1)
        pdt[32] = new_page.val; // Restore paging parent into the 32nd (zero indexed) 4mb virtual memory page
        flush_tlb();// Flushes the Translation Lookaside Buffer (TLB)

        // Sets the kernel stack pointer for the task state segment (TSS) to the parent's kernel stack.
        // The calculation for esp0 adjusts the stack pointer based on the process ID, ensuring each process has a unique kernel stack in memory.
        // The magic number 0x800000 represents the start of kernel memory, and 0x2000 (8KB) is the size allocated for each process's kernel stack.
        tss.esp0 = (uint32_t)(BASE_MEM - (((ProcessControlBlock*)current_pcb->parentPCB)->processID) * PCB_MEM); // Adjusts ESP0 for the parent process.
        tss.ss0 = KERNEL_DS; // Sets the stack segment to the kernel's data segment.
        // Restore parent process control block
        active_processes[(int)((int)((ProcessControlBlock*)current_pcb->processID) - 1)] = 0; // Set the process as inactive
        aux_processes--; // Decrement the active process count to reflect the process termination.
        ((ProcessControlBlock*)current_pcb->parentPCB)->childPCB = 0;
    }
    
    uint32_t parent_ebp = (uint32_t)((ProcessControlBlock*)current_pcb->parentPCB)->EBP;// Retrieves the saved Base Pointer (EBP) of the parent process.
    halt_return(parent_ebp, parent_ebp, return_value); // Return to the parent process

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
    cli();
    uint8_t file_name[32]; // Buffer to store the extracted file name from the command.
    dir_entry_t cur_dentry; // Directory entry structure to hold file metadata.
    uint8_t file_metadata[28]; // Buffer to temporarily hold the file contents. The size 6000 is chosen based on the expected maximum size of an executable.
    uint8_t command[128]; // Buffer to copy the user command (max size 128) to avoid modifying the original.
    int cnt;

    ProcessControlBlock* current_PCB;
    // Assembly code to get the current PCB
    // Mask the lower 13 bits then AND with ESP to align it to the 8KB boundary
    asm volatile (
        "movl %%esp, %%eax\n"       // Move current ESP value to EAX for manipulation
        "andl $0xFFFFE000, %%eax\n" // Clear the lower 13 bits to align to 8KB boundary
        "movl %%eax, %0\n"          // Move the modified EAX value to current_pcb
        : "=r" (current_PCB)        // Output operands
        :                            // No input operands
        : "eax"                      // Clobber list, indicating EAX is modified
    );

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
    if (read_data(cur_dentry.inode_num, 0, file_metadata, 28) == -1) { // check if inode is valid
        RETURN(-1); // Return command not found
    }

    // 0x7F: DEL, 0x45: E, 0x4C: L, 0x46: F
    if (file_metadata[0] != 0x7f || file_metadata[1] != 0x45 || file_metadata[2] != 0x4c || file_metadata[3] != 0x46) { // check ELF for exe
        RETURN(-1); // Return command not found
    }

    int next_pid;
    uint8_t shell_to_reboot;
    int base_boot = 0; // If this is the first process in a thread

    // If process getting booted is a shell...
    if(strncmp((int8_t*)file_name, "shell", 5) == 0) {
        if(shell_init_boot != 0) {
            // Initially booting up a shell
            next_pid = shell_init_boot;
            base_boot = 1;
            
            active_processes[shell_init_boot - 1] = 1; // Set the shell as an active process
            base_shell_booted_bitmask |= 1 << (shell_init_boot - 1); // Update the new shell on the booted shell mask
            base_shell_live_bitmask = base_shell_booted_bitmask;

            shell_init_boot = 0; // Set the initial boot flag to none
        } else if((shell_to_reboot = (base_shell_booted_bitmask ^ base_shell_live_bitmask)) != 0) {
            // If a base shell is open but not alive (rebooting)
            if(shell_to_reboot == 1) {
                next_pid = 1;
            } else if(shell_to_reboot == 2) {
                next_pid = 2;
            } else if(shell_to_reboot == 4) {
                next_pid = 3;
            }
            base_shell_live_bitmask = base_shell_booted_bitmask; // Updated the live shells to accont for rebooted shell
            base_boot = 1;
        } else {
            // Secondary shell process is started
            int i;
            for(i = 3; i < 6; i++) { // Loop through the active ( non base shell) processes to find an available PID
                if(active_processes[i] == 0) {
                    next_pid = i; // Set the next PID to the available PID
                    active_processes[i] = 1; // Set the shell process as active
                    break;
                }
            }

            next_pid++;
            aux_processes++;
        }
    } else {
        // Secondary non-shell process has started
        // Secondary shell process is started
        int i;
        for(i = 3; i < 6; i++) { // Loop through the active ( non base shell) processes to find an available PID
            if(active_processes[i] == 0) { // Check if the process is active
                next_pid = i; // Set the next PID to the available PID
                active_processes[i] = 1; // Set the non shell process as active
                break;
            }
        }

        next_pid++; // Increment the PID
        aux_processes++; // Increment the number of active ( non base shell) processes
    }
    
    if(aux_processes > 3) { // Limit the number of processes running to 6 ( including 3 base shell)
        aux_processes--; // Decrement the number of active (non base shell) processes
        terminal_write(1, "Max number of processes reached!\n", 33); // Write to terminal
        RETURN(1);
    }

    pdt_entry_page_t new_page;

    pdt_entry_page_setup(&new_page, next_pid + 1, 1); // Get page directory entry for 0x2 4mb page (zero-idxed) with user permissions (1)
    // Update virtual memory address 128mb (32nd 4mb page - zero indexed)
    pdt[32] = new_page.val;
    flush_tlb(); // Flush the TLB

    tss.esp0 = 0x800000 - next_pid * 0x2000; // Update the new stack pointer ESP_new = 8MB - PID * 8KB (0x2000)
    // Maybe update tss.ebp

    // Load the file into the correct memory location
    uint32_t file_length = ((inode_t*)(&g_inodes[cur_dentry.inode_num]))->size;
    // Load the file into the correct memory location
    read_data(cur_dentry.inode_num, 0, (uint8_t*)PROGRAM_START, file_length);

    // Create PCB at top of new process kernal stack
    ProcessControlBlock* new_PCB = (void*)(BASE_MEM - (next_pid + 1) * PCB_MEM); // Update the new PCB pointer - new_PCB = 8MB - (PID + 1) * 8KB (0x2000)
    new_PCB->processID = next_pid;
    new_PCB->exitStatus = 0;
    strcpy((int8_t*)new_PCB->name, (int8_t*)file_name);
    new_PCB->parentPCB = base_boot ? 0 : (ProcessControlBlock*)(BASE_MEM - (current_PCB->processID + 1) * PCB_MEM); // Update the new PCB parent pointer - new_PCB = 8MB - (parent PID + 1) * 8KB (0x2000)
    new_PCB->childPCB = (ProcessControlBlock*)0;
    // If this is not the first process, update teh parent PCB to point to the child PCB
    if(!base_boot) {
        current_PCB->childPCB = (ProcessControlBlock*)new_PCB;
    }

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
    uint32_t eip = 0 | (uint32_t)file_metadata[27] << 24 | (uint32_t)file_metadata[26] << 16 | (uint32_t)file_metadata[25] << 8 | (uint32_t)file_metadata[24];
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
    current_PCB->EBP = (void*)saved_ebp;
    

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
        current_pcb->files[i].operationsTable.open((uint8_t*)"rtc");
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


/*
 * int32_t vidmap(uint8_t** screen_start)
 *  DESCRIPTION: maps the text-mode video memory into user space at a pre-set virtual address
 *  INPUTS: screen_start - double pointer to user video memory (start)
 *  RETURN VALUE: -1 if invalid screen_start, 0 if successfully mapped
 *  SIDE EFFECTS: changes PDT and Page table for vid map (more of an effect than a side effect)
 */
int32_t vidmap(uint8_t** screen_start) {
    ProcessControlBlock* current_pcb;
    asm volatile (
        "movl %%esp, %%eax\n"       // Move current ESP value to EAX for manipulation
        "andl $0xFFFFE000, %%eax\n" // Clear the lower 13 bits to align to 8KB boundary
        "movl %%eax, %0\n"          // Move the modified EAX value to current_pcb
        : "=r" (current_pcb)        // Output operands
        :                            // No input operands
        : "eax"                      // Clobber list, indicating EAX is modified
    );

    int cur_process_local = get_base_process_pcb(current_pcb)->processID;

    // Step 1: Bound checks
    uint32_t vid_addr = (uint32_t)screen_start;
    if (screen_start == NULL) { // invalid screen start
        RETURN(-1);
        return -1;
    }
    if (vid_addr > USER_STACK || vid_addr < USER_STACK - 0x400000) { // 0x400000 = 4MB
        RETURN(-1);
        return -1;
    }

    // Step 2: Paging setup

    // Update PDT
    pdt_entry_table_t vidmem;
    vidmem.p = 1; // present
    vidmem.us = 1; // user
    vidmem.rw = 1;
    vidmem.address = ((int)pt_vidmap)/4096; // 4096 = 4kB; pt_vidmap is paging table for user space video memory
    pdt[VID_PDT_IDX] = vidmem.val;

    // Update Paging table for user space video memory
    pt_entry_t vidmem_pt;
    vidmem_pt.p = 1; // present
    vidmem_pt.us = 1; // user
    vidmem_pt.rw = 1;
    if(cur_terminal == cur_process_local) {
        vidmem_pt.address_31_12 = VID_MEM_PHYSICAL/4096; // 4096 = 4kB
    } else {
        vidmem_pt.address_31_12 = VID_MEM_PHYSICAL/4096 + cur_process_local;
    }
    pt_vidmap[0] = vidmem_pt.val;

    // Step 3: Flush TLB, update screen start and return
    flush_tlb(); // Updated tables so flush Translation Lookaside Buffer
    *screen_start = (uint8_t*)VID_MEM; // Update screen start to start of (user-space) video memory

    RETURN(0);
    return 0;
}


int32_t set_handler(int32_t signum, void* handler_address) {
    return 0;
}


int32_t sigreturn(void) {
    return 0;
}



// Syscall helpers

ProcessControlBlock* get_top_process_pcb(ProcessControlBlock* starting_pcb) {
    while(starting_pcb->childPCB != 0) {
        starting_pcb = starting_pcb->childPCB;
    }

    return starting_pcb;
}

ProcessControlBlock* get_base_process_pcb(ProcessControlBlock* starting_pcb) {
    while(starting_pcb->parentPCB != 0) {
        starting_pcb = starting_pcb->parentPCB;
    }

    return starting_pcb;
}
