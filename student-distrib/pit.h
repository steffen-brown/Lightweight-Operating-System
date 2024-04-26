
#include "types.h"

#ifndef _PIT_H
#define _PIT_H
#define PIT_FREQ 1193182

// Desciptions provided in the c file

void pit_init();
void pit_handler();

extern int cur_thread;
extern int get_current_thread();
#endif
