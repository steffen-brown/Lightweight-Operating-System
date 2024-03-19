#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"
#include "i8259.h"
#include "lib.h"

#define SCAN_CODES      58      // number of scan codes used
#define BUFFER_SIZE     128     // size of screen keyboard buffer
#define KEYBOARD_PORT   0x60    // keyboard data port
#define LEFT_SHIFT      0x2A    // left shift scan code
#define RIGHT_SHIFT     0x36    // right shift scan code
#define LEFT_SHIFT_REL  0xAA    // left shift released scan code
#define RIGHT_SHIFT_REL 0xB6    // right shift released scan code 
#define CAPS_LOCK       0x3A    // caps lock scan code

char keyboard_buffer[BUFFER_SIZE];

/* initializes keyboard interrupt on the PIC */
void keyboard_init(void);

/* handles keyboard interrupts and prints typed input to screen */
void keyboard_handler(void);

#endif
