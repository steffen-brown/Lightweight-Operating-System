#include "x86_desc.h"
#include "interrupts.h"
#include "lib.h"
#include "i8259.h"
#include "RTC.h"
#include "keyboard.h"

void exc_handler(int vector) {
    if(vector >= 0 && vector <= 0x13) {
        printf("Error %d      \n", vector);
        while(1) {};
    }
    if(vector == 0x28) {
        printf("RTC        \n");
        RTC_handler();
    }
    else if(vector == 0x21) {
        keyboard_handler();
    }

}

void set_IDT_entry_metadata(idt_desc_t* entry, int type) {
    entry->seg_selector = KERNEL_CS;
    entry->reserved3 = type & 1;
    entry->reserved2 = (type >> 1) & 1;
    entry->reserved1 = (type >> 2) & 1;
    entry->size = (type >> 3) & 1;
    entry->reserved0 = 0;
    entry->dpl = 0;
    entry->present = 1;
}



void setup_IDT() {
    idt_desc_t entry;

    SET_IDT_ENTRY(entry, division_error_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x0] = entry;

    SET_IDT_ENTRY(entry, debug_linkage);
    set_IDT_entry_metadata(&entry, trap);
    idt[0x1] = entry;
    
    SET_IDT_ENTRY(entry, NMI_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x2] = entry;
    
    SET_IDT_ENTRY(entry, breakpoint_linkage);
    set_IDT_entry_metadata(&entry, trap);
    idt[0x3] = entry;
    
    SET_IDT_ENTRY(entry, overflow_linkage);
    set_IDT_entry_metadata(&entry, trap);
    idt[0x4] = entry;
    
    SET_IDT_ENTRY(entry, bound_range_interrupt_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x5] = entry;
    
    SET_IDT_ENTRY(entry, invalid_opcode_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x6] = entry;
    
    SET_IDT_ENTRY(entry, device_not_avalible_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x7] = entry;
    
    SET_IDT_ENTRY(entry, double_fault_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x8] = entry;
    
    SET_IDT_ENTRY(entry, invalid_tss_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0xA] = entry;
    
    SET_IDT_ENTRY(entry, segment_not_present_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0xB] = entry;
    
    SET_IDT_ENTRY(entry, stack_segfault_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0xC] = entry;
    
    SET_IDT_ENTRY(entry, general_protection_fault_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0xD] = entry;
    
    SET_IDT_ENTRY(entry, page_fault_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0xE] = entry;
    
    SET_IDT_ENTRY(entry, x87_floating_point_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x10] = entry;
    
    
    SET_IDT_ENTRY(entry, alignment_check_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x11] = entry;
    
    
    SET_IDT_ENTRY(entry, machine_check_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x12] = entry;
    
    
    SET_IDT_ENTRY(entry, SIMD_floating_point_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x13] = entry;
    

    SET_IDT_ENTRY(entry, keyboard_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x21] = entry;

    SET_IDT_ENTRY(entry, RTC_linkage);
    set_IDT_entry_metadata(&entry, interrupt);
    idt[0x28] = entry;
    
    
}


