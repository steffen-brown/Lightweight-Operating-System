#include "sys_calls.h"

int32_t halt(uint32_t status) {
    return 0;
}


int32_t execute(const uint8_t* command) {
    uint8_t file_name[32];
    dir_entry_t cur_dentry;

    int file_length;
    uint8_t file_buffer[sizeof(int32_t)];

    int args_idx;
    for (args_idx = 0; command[args_idx] != ' '; args_idx++) { // get file name from command argument
        file_name[args_idx] = command[args_idx];
    }

    if (read_dentry_by_name(file_name, &cur_dentry) == -1) { // check if executable file exists
        return -1;
    }

    if ((file_length = read_data(cur_dentry.inode_num, 0, file_buffer, sizeof(int32_t))) == -1) { // check if inode is valid
        return -1; 
    }

    if (file_buffer[0] != 0x7f || file_buffer[1] != 0x45 || file_buffer[2] != 0x4c || file_buffer[3] != 0x46) { // check ELF for exe
        return -1;
    }
    

    pdt_entry_page_t new_page;

    if(strncmp((const int8_t *)file_name, (const int8_t *)"shell", 5)) {
        // Set up page table entry pointing mapping 128mb (virtual) to 8mb (phyiscal)
        // populate table entry
        pdt_entry_page_setup(&new_page, 0x2);
    } else {
        // Set up page table entry pointing mapping 128mb (virtual) to 12mb (phyiscal)
        // populate table entry
        pdt_entry_page_setup(&new_page, 0x3);
    }

    // Update virtual memory address 128mb
    pdt[31] = new_page.val;
    flush_tlb();


    // Load the file into the correct memory location
    int i;
    uint8_t* program_page_memory = (uint8_t *)PROGRAM_START;

    for(i = 0; i < file_length; i++) {
        program_page_memory[i] = file_buffer[i];
    }




    return 0;
}


int32_t read(int32_t fd, void* buf, int32_t nbytes) {
    return 0;
}


int32_t write(int32_t fd, const void* buf, int32_t nbytes) {
    return 0;
}


int32_t open(const uint8_t* filename) {
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
