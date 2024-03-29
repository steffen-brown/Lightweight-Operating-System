#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "file_sys.h"
#include "paging.h"
#include "x86_desc.h"
#include "keyboard.h"

#define PROGRAM_START 0x08048000

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

typedef int (*read_func)(void* buf, int32_t nbytes);
typedef int (*write_func)(const void* buf, int32_t nbytes);
typedef int (*open_func)();
typedef int (*close_func)();

typedef struct FileOperationsTable {
    read_func read;
    write_func write;
    open_func open;
    close_func close;
} FileOperationsTable;

typedef struct FileDescriptor {
    FileOperationsTable operationsTable;
    uint32_t inode;
    uint32_t filePosition;
    uint32_t flags;
} FileDescriptor;

typedef struct ProcessControlBlock {
    int processID;                   // Unique process identifier
    int exitStatus;                  // Exit status of the process
    int parentProcessID;             // Parent process identifier, 0 if spawned by start up
    FileDescriptor files[8]; // Array of file descriptors
    void* pcbPointerToParent;        // Pointer to the parent process's PCB, NULL if spawned by start up
    
    // Used to resume execute for parent process after halt (probably wrong)
    void* kernelStackTop;            // Pointer to the top of the kernel stack
    void* execReturnAddr;            // Address to return to after exec's IRET
} ProcessControlBlock;




#endif /* SYSCALL_H */
