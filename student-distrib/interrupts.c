#include "x86_desc.h"
#include "interrupts.h"
#include "lib.h"
#include "i8259.h"
#include "RTC.h"

void exc_handler(int vector) {
    if(vector == 0x28) {
        printf("RTC");
        RTC_handler();
        send_eoi(8);
    }
    else if(vector == 0x21) {
        printf("Keyboard");
        send_eoi(1);
    }

   
}

void set_IDT_int_entry_metadata(idt_desc_t* entry) {
    entry->seg_selector = KERNEL_CS;
    entry->reserved3 = 0;
    entry->reserved2 = 1;
    entry->reserved1 = 1;
    entry->size = 1;
    entry->reserved0 = 0;
    entry->dpl = 0;
    entry->present = 1;
}

void setup_IDT() {
    idt_desc_t entry;

    SET_IDT_ENTRY(entry, RTC_linkage);
    set_IDT_int_entry_metadata(&entry);
    idt[0x28] = entry;
    
    SET_IDT_ENTRY(entry, keyboard_linkage);
    set_IDT_int_entry_metadata(&entry);
    idt[0x21] = entry;
}


