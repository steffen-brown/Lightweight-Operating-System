# x86_desc.S - Set up x86 segment descriptors, descriptor tables
# vim:ts=4 noexpandtab

#define ASM     1
#define error_code 4
#define no_error_code 0
#include "x86_desc.h"

.text

.globl ldt_size, tss_size
.globl gdt_desc, ldt_desc, tss_desc
.globl tss, tss_desc_ptr, ldt, ldt_desc_ptr
.globl gdt_ptr
.globl idt_desc_ptr, idt
.global pdt, pt0, pt_vidmap
.globl load_page_directory, enable_paging_bit, flush_tlb

.align 4


tss_size:
    .long tss_bottom - tss - 1

ldt_size:
    .long ldt_bottom - ldt - 1

    .word 0 # Padding
ldt_desc:
    .word KERNEL_LDT
    .long ldt

    .align 4
tss:
_tss:
    .rept 104
    .byte 0
    .endr
tss_bottom:

    .align  16
gdt:
_gdt:

    # First GDT entry cannot be used
    .quad 0

    # NULL entry
    .quad 0

    # Segmentation will not be used
    # CS and DS both are 0-4GB r/w segments
    #
    # The layout is (from Intel IA-32 reference manual):
    #  31        24 23  22  21  20  19   16 15  14 13 12  11   8 7          0
    # |----------------------------------------------------------------------|
    # |            |   | D |   | A |  Seg  |   |  D  |   |      |            |
    # | Base 31:24 | G | / | 0 | V | Limit | P |  P  | S | Type | Base 23:16 |
    # |            |   | B |   | L | 19:16 |   |  L  |   |      |            |
    # |----------------------------------------------------------------------|
    #
    # |----------------------------------------------------------------------|
    # |                                    |                                 |
    # | Base 15:0                          | Segment Limit 15:0              |
    # |                                    |                                 |
    # |----------------------------------------------------------------------|

gdt_ptr:
    # Set up an entry for kernel CS
    .quad 0x00CF9A000000FFFF

    # Set up an entry for kernel DS
    .quad 0x00CF92000000FFFF

    # Set up an entry for user CS
    .quad 0x00CFFA000000FFFF

    # Set up an entry for user DS
    .quad 0x00CFF2000000FFFF

    # Set up an entry for TSS
tss_desc_ptr:
    .quad 0

    # Set up one LDT
ldt_desc_ptr:
    .quad 0

gdt_bottom:

gdt_desc:
    .word gdt_bottom - gdt - 1
    .long gdt

    .align 16
ldt:
    .rept 4
    .quad 0
    .endr
ldt_bottom:

.align 4
    .word 0 # Padding
idt_desc_ptr:
    .word idt_bottom - idt - 1
    .long idt


    .align  16
idt:
_idt:
    .rept NUM_VEC
    .quad 0
    .endr

idt_bottom:

.extern exc_handler # Aquire local access of exc_handler function

# Make the linkage functions globally accessable
.global RTC_linkage
.global keyboard_linkage
.global PIT_linkage
.global division_error_linkage
.global debug_linkage
.global NMI_linkage
.global breakpoint_linkage
.global overflow_linkage
.global bound_range_interrupt_linkage
.global invalid_opcode_linkage
.global device_not_avalible_linkage
.global double_fault_linkage
.global invalid_tss_linkage
.global segment_not_present_linkage
.global stack_segfault_linkage
.global general_protection_fault_linkage
.global page_fault_linkage
.global x87_floating_point_linkage
.global alignment_check_linkage
.global machine_check_linkage
.global SIMD_floating_point_linkage
.global system_call_linkage
.global assert_fault_linkage
.global coprocessor_overrun_linkage

# Create interrupt linkage for keyboard and RTC
INT_LINKAGE(RTC_linkage, exc_handler, 0x28, no_error_code) # 0x28: RTC vector
INT_LINKAGE(keyboard_linkage, exc_handler, 0x21, no_error_code) # 0x21: Keyboard vector
INT_LINKAGE(PIT_linkage, exc_handler, 0x20, no_error_code) # 0x20: PIT Vector

# Create interrupt linkage for exceptions
INT_LINKAGE(division_error_linkage, exc_handler, 0x0, no_error_code) # 0x0: Division exception
INT_LINKAGE(debug_linkage, exc_handler, 0x1, no_error_code) # 0x1: Debug exception
INT_LINKAGE(NMI_linkage, exc_handler, 0x2, no_error_code) # 0x2: Non-Maskable Interrupt
INT_LINKAGE(breakpoint_linkage, exc_handler, 0x3, no_error_code) # 0x3: Breakpoint exception
INT_LINKAGE(overflow_linkage, exc_handler, 0x4, no_error_code) # 0x4: Overflow exception
INT_LINKAGE(bound_range_interrupt_linkage, exc_handler, 0x5, no_error_code) # 0x5: Bound Range Exceeded exception
INT_LINKAGE(invalid_opcode_linkage, exc_handler, 0x6, no_error_code) # 0x6: Invalid Opcode exception
INT_LINKAGE(device_not_avalible_linkage, exc_handler, 0x7, no_error_code) # 0x7: Device Not Available exception
INT_LINKAGE(double_fault_linkage, exc_handler, 0x8, error_code) # 0x8: Double Fault exception
INT_LINKAGE(coprocessor_overrun_linkage, exc_handler, 0x9, no_error_code) # 0x9: Coprocessor Segment Overrun
INT_LINKAGE(invalid_tss_linkage, exc_handler, 0xA, error_code) # 0xA: Invalid Task State Segment exception
INT_LINKAGE(segment_not_present_linkage, exc_handler, 0xB, error_code) # 0xB: Segment Not Present exception
INT_LINKAGE(stack_segfault_linkage, exc_handler, 0xC, error_code) # 0xC: Stack-Segment Fault
INT_LINKAGE(general_protection_fault_linkage, exc_handler, 0xD, error_code) # 0xD: General Protection Fault
INT_LINKAGE(page_fault_linkage, exc_handler, 0xE, error_code) # 0xE: Page Fault
INT_LINKAGE(assert_fault_linkage, exc_handler, 0xF, no_error_code) # 0xF: Assertion fault
INT_LINKAGE(x87_floating_point_linkage, exc_handler, 0x10, no_error_code) # 0x10: x87 Floating-Point Exception
INT_LINKAGE(alignment_check_linkage, exc_handler, 0x11, error_code) # 0x11: Alignment Check exception
INT_LINKAGE(machine_check_linkage, exc_handler, 0x12, error_code) # 0x12: Machine Check exception
INT_LINKAGE(SIMD_floating_point_linkage, exc_handler, 0x13, error_code) # 0x13: SIMD Floating-Point Exception

# Create interrupt linkage for system calls
system_call_linkage:
    pushfl
    push %ebx
    push %ecx
    push %edx
    push %esi
    push %edi
    push %ebp
    call sys_calls_handler
    pop %ebp
    pop %edi
    pop %esi
    pop %edx
    pop %ecx
    pop %ebx
    popfl
    iret


# Allocate space for the paging directory table (init w/ 0s)
.align 4096
pdt:
_pdt:
    .rept NUM_DIR_ETRY
    .long 0
    .endr

pdt_bottom:

# Allocate space for a paging table (init w/ 0s)
.align 4096
pt0:
_pt0:
    .rept NUM_DIR_ETRY
    .long 0
    .endr
pt0_bottom:

# Allocate space for a paging table for video memory (init w/ 0s)
.align 4096
pt_vidmap:
_pt_vidmap:
    .rept NUM_DIR_ETRY
    .long 0
    .endr
pt_vidmap_bottom:



/*
 * load_page_directory
 *   DESCRIPTION: Sets the page directory base register (CR3) to the specified page directory address
 *                for paging translation.
 *   INPUTS: Address of the page directory (via stack argument)
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Modifies the CR3 register, affecting the paging translation table used by the system.
 */
load_page_directory:
    pushl %ebp          # Save the base pointer.
    movl %esp, %ebp     # Set the base pointer to the current stack pointer.
    movl 8(%esp), %eax  # Move the first argument (address of the page directory) into EAX.
    movl %eax, %cr3     # Load the page directory base address into CR3.
    movl %ebp, %esp     # Restore the stack pointer.
    popl %ebp           # Restore the base pointer.
    ret                 # Return from the function.

/*
 * enable_paging_bit
 *   DESCRIPTION: Enables paging by setting the paging enable bit in the CR0 register and the Page Size
 *                Extension (PSE) bit in the CR4 register, allowing the system to use memory paging.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Modifies CR0 to enable paging and CR4 to enable PSE, affecting the system's memory management mode.
 */
enable_paging_bit:
    pushl %ebp          # Save the base pointer.
    movl %esp, %ebp     # Set the base pointer to the current stack pointer.

    mov %cr4, %eax      # Move the current value of CR4 into EAX.
    or $0x00000010, %eax # OR EAX with 0x10 to set the PSE (Page Size Extension) bit.
    mov %eax, %cr4      # Write the new value back to CR4.

    movl %cr0, %eax     # Move the current value of CR0 into EAX.
    or $0x80000000, %eax # OR EAX with 0x80000000 to set the paging enable bit.
    movl %eax, %cr0     # Write the new value back to CR0.

    movl %ebp, %esp     # Restore the stack pointer.
    popl %ebp           # Restore the base pointer.
    ret                 # Return from the function.

/*
 * Flushes the Translation Lookaside Buffer (TLB).
 *
 * Description:
 *    This function flushes the TLB by reloading the value of the CR3 register.
 *    Flushing the TLB is necessary after changes to page tables to ensure that
 *    old mappings are not used by the processor. This operation is critical for
 *    maintaining the consistency of virtual memory.
 *
 * Inputs: None.
 *
 * Outputs: None.
 *
 * Side Effects:
 *    Invalidates the entries in the TLB, forcing page table entries to be reloaded
 *    on subsequent memory accesses.
 */
flush_tlb:
    pushl %ebp          # Save the base pointer.
    movl %esp, %ebp     # Set the base pointer to the current stack pointer.

    movl	%cr3,%eax
	movl	%eax,%cr3   # Flush TLB

    movl %ebp, %esp     # Restore the stack pointer.
    popl %ebp           # Restore the base pointer.
    ret                 # Return from the function.
