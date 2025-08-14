#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "file_sys.h"
#include "paging.h"
#include "x86_desc.h"
#include "keyboard.h"
#include "RTC.h"
#define PROGRAM_START 0x08048000
#define argsBufferSize 1024
#define VID_MEM          0x8800000  // 136MB: 136*1024*1024
#define USER_STACK       0x8400000  // 128MB
#define VID_PDT_IDX      34         // Page Directory Table index for video memory paging table
#define VID_MEM_PHYSICAL 0xB8000    // Video memory start physical address

extern int32_t halt(uint32_t status); // Halts the current system call
extern int32_t execute(const uint8_t* command); // executes the called sys call
extern int32_t read(int32_t fd, void* buf, int32_t nbytes); // reads data from a file or device
extern int32_t write(int32_t fd, const void* buf, int32_t nbytes); // writes data to screen
extern int32_t open(const uint8_t* filename); // opens a file
extern int32_t close(int32_t fd); // closes a file
extern int32_t getargs(uint8_t* buf, int32_t nbytes);
extern int32_t vidmap(uint8_t** screen_start);
extern int32_t set_handler(int32_t signum, void* handler_address);
extern int32_t sigreturn(void);

typedef int (*read_func)(int32_t fd, void* buf, int32_t nbytes);
typedef int (*write_func)(int32_t fd, const void* buf, int32_t nbytes);
typedef int (*open_func)(const uint8_t* filename);
typedef int (*close_func)(int32_t fd);

// initializes file operations table struct
typedef struct FileOperationsTable {
    read_func read;
    write_func write;
    open_func open;
    close_func close;
} FileOperationsTable;

// initializes file descriptor table struct
typedef struct FileDescriptor {
    FileOperationsTable operationsTable;
    uint32_t inode;
    uint32_t filePosition;
    uint32_t flags;
} FileDescriptor;

// initializes process control block struct
typedef struct ProcessControlBlock {
    int processID;                   // Unique process identifier
    int exitStatus;                  // Exit status of the process
    FileDescriptor files[8]; // Array of file descriptors
    uint8_t args[argsBufferSize];    // Arguments for the process
    uint8_t name[32];
    void* childPCB;
    void* parentPCB;
    void* EBP;
    void* schedEBP;
} ProcessControlBlock;

extern void halt_return(uint32_t parent_ebp, uint32_t parent_esp, uint32_t ret_val);
extern void return_to_parent(void* parent_ebp);

extern uint8_t base_shell_booted_bitmask;
extern int shell_init_boot;

ProcessControlBlock* get_top_process_pcb(ProcessControlBlock* starting_pcb);
ProcessControlBlock* get_base_process_pcb(ProcessControlBlock* starting_pcb);

#define RETURN(VALUE) \
    asm volatile ( \
        "movl %%ebp, %%esp\n\t" /* Correctly set up the stack pointer */ \
        "movl %0, %%eax\n\t" /* Move VALUE into %eax, preparing for return */ \
        "jmp sys_calls_handler_end\n\t" /* Jump to the end of syscalls handler */ \
        : /* No output operands */ \
        : "r" (VALUE) /* Input operand: VALUE in any general-purpose register */ \
        : "%eax", "%esp" /* Clobber list: We're modifying %eax and potentially %esp */ \
    ); \

#endif /* SYSCALL_H */
