.global sys_calls_handler
.global sys_calls_handler_end
.global halt_return

sys_calls_handler:
    pushl   %ebp 
    movl    %esp, %ebp 

    pushl   %ebx 
    pushl   %esi 
    pushl   %edi /* Save callee saved registers */

    cmpl    $1, %eax
    jl      return_error /* If call number < 1, error */
    cmpl    $10, %eax
    jg      return_error /* If call number > 10, error */

    pushl   %edx
    pushl   %ecx
    pushl   %ebx

    pushl $0 /* Maintain c calling convention stack spacing for function args */

    jmp     *jump_table(, %eax, 4) /* Jump to the appropriate syscall */

return_error:
    movl    $-1, %eax  /* return with error value */
    jmp     sys_calls_handler_end

sys_calls_handler_end:
    addl    $20, %esp /* pop arguments */

    popl    %ebx 
    popl    %esi 
    popl    %edi /* Restore callee saved registers */

    popl %ebp
    ret

jump_table:
        .long 0x1, halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn

/* define halt_return(parent_esp, parent_ebp, ret_val) */
halt_return:
    movl 12(%esp), %eax
    movl 8(%esp), %ebp

    movl %ebp, %esp
    jmp sys_calls_handler_end
    
