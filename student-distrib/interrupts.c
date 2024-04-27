#include "x86_desc.h"
#include "interrupts.h"
#include "lib.h"
#include "i8259.h"
#include "RTC.h"
#include "keyboard.h"
#include "sys_calls.h"
#include "pit.h"

/*
 * read_cr2
 *   DESCRIPTION: Reads the current value of the CR2 register. CR2 contains the linear address
 *                that caused a page fault, providing context for debugging and handling the fault.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: The current value of the CR2 register.
 *   SIDE EFFECTS: none
 */
uint32_t read_cr2() {
    uint32_t cr2;
    asm volatile ("mov %%cr2, %0" : "=r" (cr2)); // Inline assembly to read CR2 into cr2 variable
    return cr2;
}

/*
 * exc_handler
 *   DESCRIPTION: Handles CPU exceptions by displaying an exception message and the type of exception.
 *                Special handling for Page Fault (0x0E) to display the faulting address. Handles
 *                specific hardware interrupts like the RTC (0x28) and keyboard (0x21) interrupts,
 *                and a system call interrupt (0x80).
 *   INPUTS: vector - The exception or interrupt vector number.
 *   OUTPUTS: Prints exception details to the screen.
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Can halt the system for critical exceptions. Invokes specific handlers for
 *                 RTC, keyboard, and system calls.
 */
void exc_handler(int vector) {
    uint32_t eax, ebx, ecx, edx;

    asm("mov %%eax, %0" : "=r"(eax)); // Read EAX into eax
    asm("mov %%ebx, %0" : "=r"(ebx)); // Read EBX into ebx
    asm("mov %%ecx, %0" : "=r"(ecx)); // Read ECX into ecx
    asm("mov %%edx, %0" : "=r"(edx)); // Read EDX into edx

    // Range check for defined CPU exceptions
    if(vector >= 0 && vector <= 0x13) { // 0x00 to 0x13: CPU exception vectors
        printf("Exception %d\n", vector);
        
        // Array of exception messages corresponding to each CPU exception vector
        char exceptions[20][30] = {
            "Division Error",
            "Debug",
            "Non-maskable Interrupt",
            "Breakpoint",
            "Overflow",
            "Bound Range Exceeded",
            "Invalid Opcode",
            "Device Not Avalable",
            "Double Fault",
            "Coprocessor Segment Overrun",
            "Invalid TSS",
            "Segment Not Present",
            "Stack-Segment Fault",
            "General Protection Fault",
            "Page Fault",
            "Test Assertion",
            "x87 Floating-Point Exception",
            "Alignment Check",
            "Machine Check",
            "SIMD Floating-Point Exception"
        };

        printf("Type: %s\n", exceptions[vector]);

        // Special handling for Page Faults to print the faulting address
        if(vector == 0x0E) { // 0x0E: Page Fault vector number
            printf("Address: %d\n", read_cr2());
        }

        cli();
        while(1){}

        halt(256); // Halts the system for a critical exception
    }
    // Hardware interrupt handling
    if(vector == 0x28) { // 0x28: RTC interrupt vector number
        RTC_handler();
    }
    else if(vector == 0x21) { // 0x21: Keyboard interrupt vector number
        keyboard_handler();
    } else if(vector == 0x20) {
        pit_handler();
    }

}

/*
 * set_IDT_entry_metadata
 *   DESCRIPTION: Configures the metadata for an IDT entry based on the type of interrupt or trap.
 *                Sets the segment selector to the kernel code segment, and configures attributes
 *                including descriptor privilege level (DPL) and presence bit.
 *   INPUTS: entry - Pointer to the IDT entry to configure.
 *           type - The type of the interrupt gate (interrupt or trap).
 *   OUTPUTS: Modifies the specified IDT entry's metadata.
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Modifies the input IDT entry.
 */
void set_IDT_entry_metadata(idt_desc_t* entry, int type) {
    entry->seg_selector = KERNEL_CS; // KERNEL_CS: Kernel code segment selector
    // Reserved bits and size bit extracted from the type argument
    entry->reserved3 = type & 1; // Least significant bit: interrupt gate type
    entry->reserved2 = (type >> 1) & 1; // 2nd bit: 32-bit gate size
    entry->reserved1 = (type >> 2) & 1; // 3rd bit: unused, set according to type
    entry->size = (type >> 3) & 1; // 4th bit: gate size (1 for 32-bit, 0 for 16-bit)
    entry->reserved0 = 0; // Always 0
    entry->dpl = 0; // Descriptor Privilege Level: kernel level
    entry->present = 1; // Present bit: entry is present
}

/*
 * setup_IDT
 *   DESCRIPTION: Initializes the Interrupt Descriptor Table (IDT) with entries for various CPU exceptions,
 *                hardware interrupts, and a system call. Sets up each entry with the appropriate linkage
 *                address, configures the metadata for the type of interrupt, and specifies the kernel code
 *                segment selector.
 *   INPUTS: none
 *   OUTPUTS: Modifies the global IDT array.
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Configures the IDT for handling CPU exceptions, hardware interrupts, and system calls.
 */
void setup_IDT() {
    idt_desc_t entry;

    // Setup entries for CPU exceptions by assigning handlers and configuring as interrupts
    SET_IDT_ENTRY(entry, division_error_linkage); // Fill entry with handler pointer
    set_IDT_entry_metadata(&entry, interrupt); // Fill all other parts of the entry
    idt[0x0] = entry; // Insert entry into interrupt descriptor table (same for the following)  0x0: Division exception

    SET_IDT_ENTRY(entry, debug_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x1] = entry;// 0x1: Debug exception
    
    SET_IDT_ENTRY(entry, NMI_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x2] = entry; // 0x2: Non-Maskable Interrupt
    
    SET_IDT_ENTRY(entry, breakpoint_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x3] = entry; // 0x3: Breakpoint exception
    
    SET_IDT_ENTRY(entry, overflow_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x4] = entry;// 0x4: Overflow exception
    
    SET_IDT_ENTRY(entry, bound_range_interrupt_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x5] = entry; // 0x5: Bound Range Exceeded exception
    
    SET_IDT_ENTRY(entry, invalid_opcode_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x6] = entry; // 0x6: Invalid Opcode exception
    
    SET_IDT_ENTRY(entry, device_not_avalible_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x7] = entry; // 0x7: Device Not Available exception
    
    SET_IDT_ENTRY(entry, double_fault_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x8] = entry; // 0x8: Double Fault exception

    SET_IDT_ENTRY(entry, coprocessor_overrun_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x9] = entry; // 0x9: Coprocessor Segment Overrun
    
    SET_IDT_ENTRY(entry, invalid_tss_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0xA] = entry; // 0xA: Invalid Task State Segment exception
    
    SET_IDT_ENTRY(entry, segment_not_present_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0xB] = entry; // 0xB: Segment Not Present exception
    
    SET_IDT_ENTRY(entry, stack_segfault_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0xC] = entry; // 0xC: Stack-Segment Fault
    
    SET_IDT_ENTRY(entry, general_protection_fault_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0xD] = entry; // 0xD: General Protection Fault
    
    SET_IDT_ENTRY(entry, page_fault_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0xE] = entry; // 0xE: Page Fault

    SET_IDT_ENTRY(entry, assert_fault_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0xF] = entry; // # 0xF: Assertion fault
    
    SET_IDT_ENTRY(entry, x87_floating_point_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x10] = entry; // 0x10: x87 Floating-Point Exception
    
    SET_IDT_ENTRY(entry, alignment_check_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x11] = entry; // 0x11: Alignment Check exception
    
    SET_IDT_ENTRY(entry, machine_check_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x12] = entry; // 0x12: Machine Check exception
    
    SET_IDT_ENTRY(entry, SIMD_floating_point_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x13] = entry; // 0x13: SIMD Floating-Point Exception
    
    // Additional entries setup for keyboard, RTC
    SET_IDT_ENTRY(entry, keyboard_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x21] = entry; // 0x21: Keyboard interrupt vector

    SET_IDT_ENTRY(entry, RTC_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x28] = entry; // 0x28 RTC interrupt vector

    SET_IDT_ENTRY(entry, PIT_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x20] = entry; // 0x20 PIT interrupt vector

    // Additional entry setup for system call
    SET_IDT_ENTRY(entry, system_call_linkage);
    set_IDT_entry_metadata(&entry, trap);
    entry.dpl = 3;
    idt[0x80] = entry; // Syscall interrupt vector
}


