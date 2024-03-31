#include "sys_calls.h"

int32_t halt(uint32_t status) {

    return 0;
}



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

static int active_pcb = 0;

int32_t execute(const uint8_t* command) {
    uint8_t file_name[32];
    dir_entry_t cur_dentry;

    int file_length;
    uint8_t file_buffer[40000];

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

    if ((file_length = read_data(cur_dentry.inode_num, 0, file_buffer, 40000)) == -1) { // check if inode is valid
        RETURN(-1);
    }

    if (file_buffer[0] != 0x7f || file_buffer[1] != 0x45 || file_buffer[2] != 0x4c || file_buffer[3] != 0x46) { // check ELF for exe
        RETURN(-1);
    }

    if(active_pcb + 1 > 2) { // Limit the number of processes running to 2
        RETURN(0);
    }
    int new_PID = active_pcb; // The PID of the process that is being execuated
    new_PID++;

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
    memcpy((void*)PROGRAM_START, (void*)file_buffer, 5606);

    // Create PCB at top of new process kernal stack
    ProcessControlBlock* new_PCB = (void*)(0x800000 - (new_PID + 1) * 0x2000); // Could be wrong
    new_PCB->exitStatus = 0;
    new_PCB->processID = new_PID;
    new_PCB->parentProcessID = new_PID - 1; // Revise to be less hard coded?
    new_PCB->pcbPointerToParent = (void*)(0x800000 - (new_PCB->parentProcessID + 1) * 0x2000);

    // Add stdin and stdout to the file descriptor array
    new_PCB->files[0] = stdin_fd;
    stdin_fd.operationsTable.open();

    new_PCB->files[1] = stdout_fd;
    stdout_fd.operationsTable.open();

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

    asm volatile (
        "iret\n"          // Return from interrupt
    );

    asm volatile (
        "jmp sys_calls_handler_end"
    );

    return 0;
}


int32_t read(int32_t fd, void* buf, int32_t nbytes) {
    if (fd < 0 || fd > 7 || !buf || nbytes < 0) {
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

    int bytes = current_pcb->files[fd].operationsTable.read(buf, nbytes);

    RETURN(bytes);

    return 0;
}


int32_t write(int32_t fd, const void* buf, int32_t nbytes) {
    if (fd < 0 || fd > 7 || !buf || nbytes < 0) {
        RETURN(-1)
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

    int bytes = current_pcb->files[fd].operationsTable.write(buf, nbytes);

    RETURN(bytes);

    return 0;
}

// add element to filearray at the first available index
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
}


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
    current_pcb->files[fd].operationsTable.close(fd);
}


int32_t getargs(uint8_t* buf, int32_t nbytes) {
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
