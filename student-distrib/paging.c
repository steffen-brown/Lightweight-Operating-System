#include "paging.h"
#include "x86_desc.h"
#include "lib.h"

void setup_kernel_paging() {
    pdt_entry_page_t kernal_page;

    kernal_page.p = 1;
    kernal_page.rw = 1;
    kernal_page.us = 0;
    kernal_page.pwt = 0;
    kernal_page.pcd = 0;
    kernal_page.a = 0;
    kernal_page.d = 0;
    kernal_page.ps = 1;
    kernal_page.g = 0;
    kernal_page.pat = 0;
    kernal_page.reserved = 0;
    kernal_page.address_22_31 = 1;

    pdt[1] = kernal_page.val;
    printf("%x           \n", pdt[1]);

    pdt_entry_table_t first_mb;

    first_mb.p = 1;
    first_mb.rw = 1;
    first_mb.us = 1;
    first_mb.pwt = 0;
    first_mb.pcd = 0;
    first_mb.a = 0;
    first_mb.ps = 0;
    first_mb.address = 0xB8;

    pdt[0] = first_mb.val;
    printf("%x           \n", pdt[0]);

    pt_entry_t video_memory_page;

    video_memory_page.p = 1;
    video_memory_page.rw = 1;
    video_memory_page.us = 1;
    video_memory_page.pwt = 0;
    video_memory_page.pcd = 0;
    video_memory_page.a = 0;
    video_memory_page.d = 0;
    video_memory_page.pat = 0;
    video_memory_page.g = 0;
    video_memory_page.address_31_12 = 0xB8;

    pt0[0xB8] = video_memory_page.val;
    printf("%x           \n", pt0[0xB8]);
}

void enable_paging() {
    load_page_directory(pdt);
    enable_paging();
}
