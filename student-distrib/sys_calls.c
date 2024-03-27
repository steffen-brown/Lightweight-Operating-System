#include "sys_calls.h"

int32_t halt(uint32_t status) {
    return 0;
}


int32_t execute(const uint8_t* command) {
    uint8_t file_name[32];

    int args_idx;
    for (args_idx = 0; command[args_idx] != ' '; args_idx++) {
        file_name[args_idx] = command[args_idx];
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
