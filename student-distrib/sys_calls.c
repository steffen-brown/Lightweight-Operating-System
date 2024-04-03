#include "sys_calls.h"

static int active_pcb = 0;

/*
 * int32_t halt(uint32_t status)
 *  DESCRIPTION: Halts the current system call
 *  INPUTS: the status of the current system
 *  RETURN VALUE: 0
 *  SIDE EFFECTS: terminates a process
 */
int32_t halt(uint32_t status) {
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
    uint32_t return_value = (uint32_t)(status & 0xFF);
    if (status == 256){
        return_value = 0x100;
    }
    
    int i;
    // Close any open files
    for (i = 0; i < 8; i++) {
        if (current_pcb->files[i].flags != 0) { // '0' means the file is not in use

            if (current_pcb->files[i].operationsTable.close != NULL) {
                current_pcb->files[i].operationsTable.close(i);
            }
            current_pcb->files[i].flags = 0; // Mark as unused
        }
    }

    // Update exit status
    current_pcb->exitStatus = status;

    // check if the current process is the shell
    if (current_pcb->processID == 1) {
        // If the current process is the shell, restart the shell
        execute((uint8_t*)"shell");
    } else {
        pdt_entry_page_t new_page;
        // If the current process is not the shell, return to the parent process
        // Restore parent paging
        pdt_entry_page_setup(&new_page, 0x2, 1);
        pdt[32] = new_page.val; // Restore parent paging
        flush_tlb();

        // Restore parent stack pointer
        tss.esp0 = (uint32_t)(0x800000 - (current_pcb->parentProcessID ) * 0x2000);
        tss.ss0 = KERNEL_DS;
        // Restore parent process control block
        active_pcb--;
    }
    
    uint32_t parent_ebp = (uint32_t)current_pcb->oldEBP; // HAVE TO SAVE THIS IN EXECUTE
    halt_return(parent_ebp, parent_ebp, return_value);

    return 0;
}


FileOperationsTable dir_operations_table = {
    .read = dir_read,
    .write = dir_write,
    .open = dir_open,
    .close = dir_close
};
FileOperationsTable file_operations_table = {
    .read = file_read,
    .write = file_write,
    .open = file_open,
    .close = file_close
};
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


FileOperationsTable rtc_operations_table = {
    .read = rtc_read,
    .write = rtc_write,
    .open = rtc_open,
    .close = rtc_close
};


/*
 * int32_t execute(const uint8_t* command)
 *  DESCRIPTION: executes the called sys call based on input cmd
 *  INPUTS: input cmd
 *  RETURN VALUE: -1 if cmd invalid
 *  SIDE EFFECTS: executes a process
 */
int32_t execute(const uint8_t* command_user) {
    uint8_t file_name[32];
    dir_entry_t cur_dentry;
    int file_length;
    uint8_t file_buffer[7500];
    uint8_t command[128];
    int cnt;
    for (cnt = 0; cnt <128; cnt++){
        command[cnt] = command_user[cnt];
    }
    int args_idx;
    for (args_idx = 0; command[args_idx] != ' ' && command[args_idx] != '\0'; args_idx++) { // get file name from command argument
        file_name[args_idx] = command[args_idx];
    }

    if (command[args_idx] == '\0') {
        file_name[args_idx] = '\0'; // Temp solution
    }
    
    if (read_dentry_by_name(file_name, &cur_dentry) == -1) { // check if executable file exists
        RETURN(-1);
    }

    if ((file_length = read_data(cur_dentry.inode_num, 0, file_buffer, 7500)) == -1) { // check if inode is valid
        RETURN(-1);
    }

    if (file_buffer[0] != 0x7f || file_buffer[1] != 0x45 || file_buffer[2] != 0x4c || file_buffer[3] != 0x46) { // check ELF for exe
        RETURN(-1);
    }

    if(active_pcb + 1 > 2) { // Limit the number of processes running to 2
        RETURN(0);
    }
    int new_PID = ++active_pcb; // The PID of the process that is being execuated

    // Shell: PID 1
    // Other: PID 2

    pdt_entry_page_t new_page;

    if(new_PID == 1) {
        // Set up page table entry pointing mapping 128mb (virtual) to 8mb (phyiscal)
        // populate table entry
        pdt_entry_page_setup(&new_page, 0x2, 1);
    } else if(new_PID == 2) {
        // Set up page table entry pointing mapping 128mb (virtual) to 12mb (phyiscal)
        // populate table entry
        pdt_entry_page_setup(&new_page, 0x3, 1);
    }
    // Update virtual memory address 128mb
    pdt[32] = new_page.val;
    flush_tlb();

    tss.esp0 = 0x800000 - new_PID * 0x2000; // Update the new stack pointer ESP_new = 8MB - PID * 8KB
    // Maybe update tss.ebp

    // Load the file into the correct memory location
    memcpy((void*)PROGRAM_START, (void*)file_buffer, 7500);

    // Create PCB at top of new process kernal stack
    ProcessControlBlock* new_PCB = (void*)(0x800000 - (new_PID + 1) * 0x2000); // Could be wrong
    new_PCB->exitStatus = 0;
    new_PCB->processID = new_PID;
    new_PCB->parentProcessID = new_PID - 1; // Revise to be less hard coded?
    new_PCB->pcbPointerToParent = (void*)(0x800000 - (new_PCB->parentProcessID + 1) * 0x2000);

    // Add stdin and stdout to the file descriptor array
    new_PCB->files[0] = stdin_fd;
    new_PCB->files[0].flags = 1;

    new_PCB->files[1] = stdout_fd;
    new_PCB->files[1].flags = 1;

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
    uint32_t esp = 0x8400000 - 4;
    uint32_t eflags = 0x00000202; // 202
    uint32_t cs = USER_CS;
    uint32_t eip = 0 | (uint32_t)file_buffer[27] << 24 | (uint32_t)file_buffer[26] << 16 | (uint32_t)file_buffer[25] << 8 | (uint32_t)file_buffer[24];

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

    register uint32_t saved_ebp asm("ebp");
    new_PCB->oldEBP = (void*)saved_ebp;

    asm volatile (
        "iret\n"          // Return from interrupt
    );

    RETURN(0);

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
        RETURN(-1);
    }
    if ( fd == 1){ // edge case ( can't read from stdout)
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

    if (current_pcb->files[fd].flags == 0) { // Check if file descriptor is in use
        RETURN(-1);
    }
    int bytes = current_pcb->files[fd].operationsTable.read(fd, buf, nbytes);

    RETURN(bytes);

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
    if (fd < 0 || fd > 7 || !buf || nbytes < 0) {
        RETURN(-1);
    }
    if ( fd == 0){ // edge case ( can't write to stdin)
        RETURN(-1);
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
        RETURN(-1);
    }
    int bytes = current_pcb->files[fd].operationsTable.write(fd, buf, nbytes);

    RETURN(bytes);

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
    for (i  = 2; i < 8; i++) {
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
    if ( file_type == 0) { // RTC file
        current_pcb->files[i].operationsTable = rtc_operations_table;
        current_pcb->files[i].filePosition = 0;
        current_pcb->files[i].flags = 1;
        RETURN(i);
    } else if (file_type == 1) { // Directory file
        current_pcb->files[i].operationsTable = dir_operations_table;
        current_pcb->files[i].filePosition = 0;
        current_pcb->files[i].flags = 1;
        RETURN(i);
    } else if (file_type == 2) { // Regular file
        current_pcb->files[i].operationsTable = file_operations_table;
        current_pcb->files[i].inode = dentry.inode_num;
        current_pcb->files[i].filePosition = 0;
        current_pcb->files[i].flags = 1;
        RETURN(i);
    }
    
    
    RETURN(-1);

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
    if (current_pcb->files[fd].flags == 0) {
        RETURN(-1);
    }
    current_pcb->files[fd].flags = 0;
    int ret = current_pcb->files[fd].operationsTable.close(fd);

    RETURN(ret);

    return 0;
}


int32_t getargs(uint8_t* buf, int32_t nbytes) {
    if (buf == NULL || nbytes < 0 || nbytes > argsBufferSize) {
        RETURN(-1);
        return -1;
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
    if(current_pcb->args[0] == '\0' || current_pcb->args[0] == NULL) {
        RETURN(-1);
        return -1;
    }
    int i;
    for (i = 0; i < nbytes; i++) {
        if(current_pcb->args[i] == '\0') {
            ((uint8_t*)buf)[i] = '\0';
            break;
        }
        ((uint8_t*)buf)[i] = current_pcb->args[i];
    }
    RETURN(0);
    return 0;
}


int32_t vidmap(uint8_t** screen_start) {
    return 0;
}


int32_t set_handler(int32_t signum, void* handler_address) {
    return 0;
}


int32_t sigreturn(void) {
    return 0;
}
