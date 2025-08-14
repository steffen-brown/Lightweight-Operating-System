#include "pit.h"
#include "lib.h"
#include "i8259.h"
#include "sys_calls.h"

int cur_process = 1; // Global variable for the current thread being computed

void pit_init() {
    int divisor = PIT_FREQ / 100; // Calculate the divisor for the PIT
    outb(0x34, 0x43); // Set the PIT to mode 2, rate generator
    outb(divisor & 0xFF, 0x40); // Set the PIT to 50ms
    outb((divisor >> 8), 0x40); // Set the PIT to 50ms
    enable_irq(0); // Enable the PIT on the PIC

}
 
void pit_handler() {
    ProcessControlBlock* current_PCB;
    // Assembly code to get the current PCB
    // Mask the lower 13 bits then AND with ESP to align it to the 8KB boundary
    asm volatile (
        "movl %%esp, %%eax\n"       // Move current ESP value to EAX for manipulation
        "andl $0xFFFFE000, %%eax\n" // Clear the lower 13 bits to align to 8KB boundary
        "movl %%eax, %0\n"          // Move the modified EAX value to current_pcb
        : "=r" (current_PCB)        // Output operands
        :                            // No input operands
        : "eax"                      // Clobber list, indicating EAX is modified
    );

    int saved_process = cur_process; // Save current thread nimber before advancing

    // Advance the thread in round robin fashion
    cur_process++;
    if(cur_process == MAX_THREADS) {
        cur_process = 1;
    }
    if(cur_process == 2 && !(base_shell_booted_bitmask & 0x2)) { // bitmask logic
        cur_process++;
    }
    if(cur_process == NUM_TERMINALS && !(base_shell_booted_bitmask & 0x4)) { // bitmask logic
        cur_process = 1;
    }

    if(saved_process != cur_process) {
        // Get the PCB of the active process on the active thread
        ProcessControlBlock* top_PCB = get_top_process_pcb((ProcessControlBlock*)(BASE_MEM - (cur_process + 1) * PCB_MEM));

        // Save current EBP
        register uint32_t saved_ebp asm("ebp");
        current_PCB->schedEBP = (void*)saved_ebp; // Save the current EBP for the current scheduling process

        pdt_entry_page_t new_page;

        // Restore parent paging
        pdt_entry_page_setup(&new_page, top_PCB->processID + 1, 1); // Create entry for 0x02 (zero indexed) 4mb page in user mode (1)
        pdt[32] = new_page.val; // Restore paging parent into the 32nd (zero indexed) 4mb virtual memory page
        flush_tlb();// Flushes the Translation Lookaside Buffer (TLB)

        // Sets the kernel stack pointer for the task state segment (TSS) to the parent's kernel stack.
        tss.esp0 = (uint32_t)(BASE_MEM - top_PCB->processID * PCB_MEM); // Adjusts ESP0 for the parent process.
        tss.ss0 = KERNEL_DS; // Sets the stack segment to the kernel's data segment.

        pt_entry_t vidmem_pt;
        vidmem_pt.p = 1; // present on
        vidmem_pt.us = 1; // user access enabled
        vidmem_pt.rw = 1; // read write privlages enabled

        // Update userspace video memory to display to the correct terminal
        if(cur_terminal == cur_process) {
            vidmem_pt.address_31_12 = VID_MEM_PHYSICAL/4096; // 4096 = 4kB
        } else {
            vidmem_pt.address_31_12 = VID_MEM_PHYSICAL + cur_process;
        }
        pt_vidmap[0] = vidmem_pt.val;
        flush_tlb();

        send_eoi(0);
        // Context switch to prexisiting thread
        return_to_parent(top_PCB->schedEBP); // Return to the parent process with the saved EBP (scheduling)
    }
    
    
    send_eoi(0); // Send end of interrupt for the PIT to the pic
}

int get_current_process() {
    return cur_process-1;
}
