#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"
#include "i8259.h"
#include "lib.h"

#define SCAN_CODES  58
#define BUFFER_SIZE 128

char keyboard_buffer[BUFFER_SIZE];

extern char scan_codes_table[SCAN_CODES];

void keyboard_init(void);
void keyboard_handler(void);

#endif
