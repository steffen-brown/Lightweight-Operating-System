#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"
#include "i8259.h"
#include "lib.h"

#define SCAN_CODES      58      // number of scan codes used
#define BUFFER_SIZE     127     // size of screen keyboard buffer
#define MAX_LINE        73      // maximum number of characters per line
#define TAB_SPACE       4       // space for tabs
#define KEYBOARD_PORT   0x60    // keyboard data port
#define NUM_TERMINALS   3       // number of terminals
#define FOUR_KB         4096    // 4 KB in bytes
#define VIDEO_MEM       ((uint8_t *)0xB8000) // Base address of video memory
#define F_OFFSET        0x3A    

#define LEFT_SHIFT      0x2A    // left shift scan code
#define RIGHT_SHIFT     0x36    // right shift scan code
#define LEFT_SHIFT_REL  0xAA    // left shift released scan code
#define RIGHT_SHIFT_REL 0xB6    // right shift released scan code 
#define CAPS_LOCK       0x3A    // caps lock scan code
#define TAB             0x0F    // tab scan code
#define CTRL            0x1D    // control scan code
#define CTRL_REL        0x9D    // control released scan code
#define L               0x26    // L scan code
#define ENTER           0x1C    // enter scan code
#define ENTER_REL       0X9C    // enter released scan code
#define BACKSPACE       0x0E    // backspace scan code
#define ALT             0x38    // alt scan code
#define ALT_REL         0xB8    // alt released scan code
#define F1              0x3B    // F1 scan code
#define F2              0x3C    // F2 scan code
#define F3              0x3D    // F3 scan code

char keyboard_buffer[NUM_TERMINALS][BUFFER_SIZE]; // Keyboard buffers for the 3 termnals
extern int cur_terminal; // Global variable for the current displayed terminal

/* initializes keyboard interrupt on the PIC */
void keyboard_init(void);

/* handles keyboard interrupts and prints typed input to screen */
void keyboard_handler(void);

extern int terminal_read(int32_t fd, void* buffer, int32_t bytes);
extern int terminal_write(int32_t fd, const void* buffer, int32_t bytes);

extern int terminal_open(const uint8_t* filename);
extern int terminal_close(int32_t fd);

#define CONTEXT_SAVE_CALL(func, ...) ({        \
                            \
    func(__VA_ARGS__);                     /* Call the function with variable arguments */ \
                                 \
})

#endif
