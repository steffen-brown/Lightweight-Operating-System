#ifndef PAGING_H
#define PAGING_H

#include "x86_desc.h"

// See c file for descriptions
void setup_kernel_paging();
void enable_paging();

extern void load_page_directory(unsigned int*);
extern void enable_paging_bit();
extern void pdt_entry_page_setup(pdt_entry_page_t* page, uint32_t physical_memory_22_31, uint32_t user);
extern void flush_tlb();

#endif
