#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"
#include "i8259.h"
#include "lib.h"

#define SCAN_CODES  58  // number of scan codes used
#define BUFFER_SIZE 128  // size of screen keyboard buffer

char keyboard_buffer[BUFFER_SIZE];
extern char scan_codes_table[SCAN_CODES];

/* initializes keyboard interrupt on the PIC */
void keyboard_init(void);

/* handles keyboard interrupts and prints typed input to screen */
void keyboard_handler(void);

#endif
