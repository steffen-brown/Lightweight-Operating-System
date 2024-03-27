#include "types.h"
#include "lib.h"

#ifndef SYSCALL_H
#define SYSCALL_H

extern uint8_t sys_calls_handler(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

int32_t halt(uint32_t status);
int32_t execute(const uint8_t* command);
int32_t read(int32_t fd, void* buf, int32_t nbytes);
int32_t write(int32_t fd, const void* buf, int32_t nbytes);
int32_t open(const uint8_t* filename);
int32_t close(int32_t fd);
int32_t getargs(uint8_t* buf, int32_t nbytes);
int32_t vidmap(uint8_t** screen_start);
int32_t set_handler(int32_t signum, void* handler_address);
int32_t sigreturn(void);

#endif /* SYSCALL_H */
