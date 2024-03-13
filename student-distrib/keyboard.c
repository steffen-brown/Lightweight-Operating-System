#include "keyboard.h"

char scan_codes_table[SCAN_CODES] = {
    ' ' ,    // Error  
    ' ' ,    // Escape
    '1', 
    '2',
    '3', 
    '4',
    '5', 
    '6',
    '7', 
    '8',
    '9', 
    '0',
    '-', 
    '=',
    ' ' ,    // Backspace       
    ' ' ,    // Tab
    'q', 
    'w',
    'e', 
    'r',
    't', 
    'y',
    'u', 
    'i',
    'o', 
    'p',
    '[', 
    ']',
    ' ' ,    // Enter
    ' ' ,    // Control
    'a', 
    's',
    'd', 
    'f',
    'g', 
    'h',
    'j', 
    'k',
    'l', 
    ';',
    '\'', 
    '`',
    ' ' ,    // Shift  
    '\\',     
    'z', 
    'x',
    'c', 
    'v',
    'b', 
    'n',
    'm', 
    ',',
    '.', 
    '/',
    ' ' ,    // Shift
    ' ' ,    // Keypad
    ' ' ,    // Alt
    ' ',
};

static int keyboard_index;

void keyboard_init(void) {
    enable_irq(1); // keyboard irq
    keyboard_index = 0;
}

void keyboard_handler(void) {
    uint8_t scan_code = inb(0x60) & 0xFF; // data port
    if (scan_code < SCAN_CODES) {
        keyboard_buffer[keyboard_index] = scan_codes_table[scan_code];
        putc(keyboard_buffer[keyboard_index]);
        keyboard_index++;
    }

    send_eoi(1); // keyboard irq
    enable_irq(1);

}
