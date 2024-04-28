
#include "types.h"

#ifndef _PIT_H
#define _PIT_H
#define PIT_FREQ 1193182
#define MAX_THREADS 4
#define BASE_MEM 0x800000
#define PCB_MEM 0x2000

// Desciptions provided in the c file

void pit_init();
void pit_handler();

extern int cur_process;
extern int get_current_process();
#endif
