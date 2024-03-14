#ifndef PAGING_H
#define PAGING_H

void setup_kernel_paging();
void enable_paging();

extern void load_page_directory(unsigned int*);
extern void enable_paging_bit();

#endif
