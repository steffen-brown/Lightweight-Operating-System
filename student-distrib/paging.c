#include "paging.h"
#include "lib.h"

/*
 * Configures a page directory entry for a 4MB page.
 *
 * Description:
 *    This function sets up a page directory entry to point to a 4MB page frame
 *    with specified access permissions and physical memory location. It configures
 *    the entry for either user or supervisor level access, enables read/write permissions,
 *    and sets the page as present in memory.
 *
 * Inputs:
 *    - page: A pointer to the page directory entry to configure.
 *    - physical_memory_22_31: The high-order bits (22-31) of the physical address where the 4MB page frame is located.
 *    - user: A flag indicating if the page is accessible from user mode (1) or only from supervisor mode (0).
 *
 * Outputs: None.
 *
 * Side Effects:
 *    Modifies the provided page directory entry to reflect the specified configuration.
 */
void pdt_entry_page_setup(pdt_entry_page_t* page, uint32_t physical_memory_22_31, uint32_t user) {
    page->p = 1;          // Present; the page is present in memory
    page->rw = 1;         // Read/Write; the page is writable
    page->us = user;         // User/Supervisor; the page is only accessible from supervisor mode
    page->pwt = 0;        // Page Write-Through; disabled for performance
    page->pcd = 0;        // Page Cache Disable; caching enabled
    page->a = 0;          // Accessed; not accessed yet
    page->d = 0;          // Dirty; not written to yet (irrelevant for PD entries)
    page->ps = 1;         // Page Size; enables 4MB pages
    page->g = 0;          // Global; not global (irrelevant for PD entries without PGE)
    page->pat = 0;        // Page Attribute Table; not used here
    page->reserved = 0;   // Reserved bits; must be 0
    page->address_22_31 = physical_memory_22_31; // The physical address bits 22-31 of the 4MB page frame
}

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

    pdt_entry_page_setup(&kernal_page, 0x01, 0);

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


// /*
//  * video_page_mapping
//  *   DESCRIPTION: TODO
//  *   INPUTS: va: virtual address
//  *           pa: physical address
//  *   OUTPUTS: none
//  *   RETURN VALUE: none
//  *   SIDE EFFECTS: TODO
//  */
// void video_page_mapping(uint32_t va, uint32_t pa) {}
