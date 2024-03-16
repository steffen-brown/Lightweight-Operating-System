#include "paging.h"
#include "x86_desc.h"
#include "lib.h"

/*
 * setup_kernel_paging
 *   DESCRIPTION: Configures the initial paging setup for the kernel. This involves setting up
 *                a Page Directory Table (PDT) entry for the kernel space with large pages (4MB),
 *                a PDT entry for the first 4MB of physical memory to use standard 4KB pages, and
 *                setting up a Page Table (PT) entry specifically for video memory.
 *   INPUTS: none
 *   OUTPUTS: Modifies the global Page Directory Table (pdt) and the first Page Table (pt0)
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Changes the paging structure which affects memory access patterns. Specifically,
 *                 it sets up large page support for the kernel and maps the first 4MB of physical
 *                 memory and video memory into the virtual address space.
 */
void setup_kernel_paging() {
    pdt_entry_page_t kernal_page;

    // Setup a Page Directory entry for kernel space
    kernal_page.p = 1;          // Present; the page is present in memory
    kernal_page.rw = 1;         // Read/Write; the page is writable
    kernal_page.us = 0;         // User/Supervisor; the page is only accessible from supervisor mode
    kernal_page.pwt = 0;        // Page Write-Through; disabled for performance
    kernal_page.pcd = 0;        // Page Cache Disable; caching enabled
    kernal_page.a = 0;          // Accessed; not accessed yet
    kernal_page.d = 0;          // Dirty; not written to yet (irrelevant for PD entries)
    kernal_page.ps = 1;         // Page Size; enables 4MB pages
    kernal_page.g = 0;          // Global; not global (irrelevant for PD entries without PGE)
    kernal_page.pat = 0;        // Page Attribute Table; not used here
    kernal_page.reserved = 0;   // Reserved bits; must be 0
    kernal_page.address_22_31 = 1; // The physical address bits 22-31 of the 4MB page frame

    pdt[1] = kernal_page.val;   // Maps the second 4MB of virtual memory to a 4MB page in PDT

    pdt_entry_table_t first_mb;

    // Setup the first Page Directory entry for the first 4MB of physical memory
    first_mb.p = 1;             // Present; the page is present in memory
    first_mb.rw = 1;            // Read/Write; the page is writable
    first_mb.us = 0;            // User/Supervisor; the page is only accessible from supervisor mode
    first_mb.pwt = 0;           // Page Write-Through; disabled for performance
    first_mb.pcd = 0;           // Page Cache Disable; caching enabled
    first_mb.a = 0;             // Accessed; not accessed yet
    first_mb.ps = 0;            // Page Size; 0 to use 4KB pages
    first_mb.address = (uint32_t)pt0 >> 12; // The address of the first Page Table, shifted to fit PD entry

    pdt[0] = first_mb.val; // Maps the first 4MB of virtual memory to the first PT

    pt_entry_t video_memory_page;

    // Setup a Page Table entry for video memory
    video_memory_page.p = 1;    // Present; the page is present in memory
    video_memory_page.rw = 1;   // Read/Write; the page is writable
    video_memory_page.us = 0;   // User/Supervisor; the page is only accessible from supervisor mode
    video_memory_page.pwt = 0;  // Page Write-Through; disabled for performance
    video_memory_page.pcd = 0;  // Page Cache Disable; caching enabled
    video_memory_page.a = 0;    // Accessed; not accessed yet
    video_memory_page.d = 0;    // Dirty; not written to yet
    video_memory_page.pat = 0;  // Page Attribute Table; not used here
    video_memory_page.g = 0;    // Global; not global
    video_memory_page.address_31_12 = 0xB8; // Physical address of video memory (0xB8000 >> 12)

    pt0[0xB8] = video_memory_page.val; // Maps virtual address corresponding to 0xB8000 to video memory
}

/*
 * enable_paging
 *   DESCRIPTION: Enables hardware paging by loading the page directory table (PDT) into the CR3
 *                register and setting the appropriate bit in the CR0 register to enable paging mode.
 *                This function wraps the operations of setting up the processor to translate virtual
 *                addresses to physical addresses using the paging mechanism.
 *   INPUTS: none (uses the global 'pdt' implicitly)
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Changes the CPU's memory addressing mode to use paging. This affects all memory
 *                 accesses following the invocation of this function.
 */
void enable_paging() {
    load_page_directory(pdt); // Load the address of the global page directory table into CR3.
    enable_paging_bit(); // Set the paging enable bit in CR0 to activate paging.
}
