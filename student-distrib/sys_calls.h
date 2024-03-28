#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "file_sys.h"
#include "paging.h"
#include "x86_desc.h"

#define PROGRAM_START 0x08048000;

extern int32_t halt(uint32_t status);
extern int32_t execute(const uint8_t* command);
extern int32_t read(int32_t fd, void* buf, int32_t nbytes);
extern int32_t write(int32_t fd, const void* buf, int32_t nbytes);
extern int32_t open(const uint8_t* filename);
extern int32_t close(int32_t fd);
extern int32_t getargs(uint8_t* buf, int32_t nbytes);
extern int32_t vidmap(uint8_t** screen_start);
extern int32_t set_handler(int32_t signum, void* handler_address);
extern int32_t sigreturn(void);

#endif /* SYSCALL_H */
