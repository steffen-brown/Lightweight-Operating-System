#include "sys_calls.h"

sys_calls_handler:
    pushl   %ebp 
    movl    %esp, %ebp 

    pushl   %ebx 
    pushl   %esi 
    pushl   %edi /* Save callee saved registers */

    movl    8(%ebp), %eax /* Load call number */

    cpml    $1, %eax
    jl      return_error /* If call number < 1, error */
    cpml    $10, %eax
    jg      return_error /* If call number > 10, error */

    pushl   20(%ebp)
    pushl   16(%ebp)
    pushl   12(%ebp)

    jmp     *jump_table(, %eax, 4) /* Jump to the appropriate syscall */
    movl    $0, %eax
    jmp     sys_calls_handler_end

return_error:
    movl    $-1, %eax  /* return with error value */
    jmp     sys_calls_handler_end

sys_calls_handler_end:
    addl    $12, %esp /* pop arguments */

    popl    %ebx 
    popl    %esi 
    popl    %edi /* Restore callee saved registers */

    leave
    ret

jump_table:
        .long 0x1, halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn


