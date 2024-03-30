#include "sys_calls.h"

int32_t halt(uint32_t status) {
    //ProcessControlBlock* current_pcb = 
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
        return -1;
    }

    if ((file_length = read_data(cur_dentry.inode_num, 0, file_buffer, 40000)) == -1) { // check if inode is valid
        return -1; 
    }

    if (file_buffer[0] != 0x7f || file_buffer[1] != 0x45 || file_buffer[2] != 0x4c || file_buffer[3] != 0x46) { // check ELF for exe
        return -1;
    }

    if(active_pcb + 1 > 2) { // Limit the number of processes running to 2
        return -1;
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
    int i;
    uint8_t* program_page_memory = (uint8_t *)PROGRAM_START;

    for(i = 0; i < file_length; i++) {
        program_page_memory[i] = file_buffer[i];
    }

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
    uint32_t esp = 0x8400000;
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

    return 0;
}


int32_t read(int32_t fd, void* buf, int32_t nbytes) {
    return 0;
}


int32_t write(int32_t fd, const void* buf, int32_t nbytes) {
    return 0;
}


int32_t open(const uint8_t* filename) {
    // Sets up file descriptor
    // Calls file_open
    return 0;
}


int32_t close(int32_t fd) {
    return 0;
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
