# x86_desc.S - Set up x86 segment descriptors, descriptor tables
# vim:ts=4 noexpandtab

#define ASM     1
#include "x86_desc.h"

.text

.globl ldt_size, tss_size
.globl gdt_desc, ldt_desc, tss_desc
.globl tss, tss_desc_ptr, ldt, ldt_desc_ptr
.globl gdt_ptr
.globl idt_desc_ptr, idt
.global pdt, pt0
.globl load_page_directory, enable_paging_bit

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

.extern exc_handler

.global RTC_linkage
.global keyboard_linkage
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

INT_LINKAGE(RTC_linkage, exc_handler, 0x28, 0)
INT_LINKAGE(keyboard_linkage, exc_handler, 0x21, 0)

INT_LINKAGE(division_error_linkage, exc_handler, 0x0, 0)
INT_LINKAGE(debug_linkage, exc_handler, 0x1, 0)
INT_LINKAGE(NMI_linkage, exc_handler, 0x2, 0)
INT_LINKAGE(breakpoint_linkage, exc_handler, 0x3, 0)
INT_LINKAGE(overflow_linkage, exc_handler, 0x4, 0)
INT_LINKAGE(bound_range_interrupt_linkage, exc_handler, 0x5, 0)
INT_LINKAGE(invalid_opcode_linkage, exc_handler, 0x6, 0)
INT_LINKAGE(device_not_avalible_linkage, exc_handler, 0x7, 0)
INT_LINKAGE(double_fault_linkage, exc_handler, 0x8, 4)
INT_LINKAGE(invalid_tss_linkage, exc_handler, 0xA, 4)
INT_LINKAGE(segment_not_present_linkage, exc_handler, 0xB, 4)
INT_LINKAGE(stack_segfault_linkage, exc_handler, 0xC, 4)

INT_LINKAGE(general_protection_fault_linkage, exc_handler, 0xD, 4)
INT_LINKAGE(page_fault_linkage, exc_handler, 0xE, 4)
INT_LINKAGE(x87_floating_point_linkage, exc_handler, 0x10, 0)
INT_LINKAGE(alignment_check_linkage, exc_handler, 0x11, 4)
INT_LINKAGE(machine_check_linkage, exc_handler, 0x12, 4)
INT_LINKAGE(SIMD_floating_point_linkage, exc_handler, 0x13, 4)

INT_LINKAGE(system_call_linkage, exc_handler, 0x80, 0)


.align 4096
pdt:
_pdt:
    .rept NUM_DIR_ETRY
    .long 0
    .endr

pdt_bottom:

.align 4096
pt0:
_pt0:
    .rept NUM_DIR_ETRY
    .long 0
    .endr
pt0_bottom:

load_page_directory:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%esp), %eax
    movl %eax, %cr3
    movl %ebp, %esp
    popl %ebp
    ret

enable_paging_bit:
    pushl %ebp
    movl %esp, %ebp

    mov %cr4, %eax
    or $0x00000010, %eax 
    mov %eax, %cr4

    movl %cr0, %eax
    or $0x80000000, %eax
    movl %eax, %cr0
    movl %ebp, %esp
    popl %ebp
    ret
