#include "keyboard.h"

char scan_codes_table[SCAN_CODES] = {
    NULL, NULL, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', NULL,      
    NULL, 'q', 'w', 'e', 'r', 't', 'y', 'u',  'i', 'o', 'p', '[', ']', NULL,
    NULL, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', NULL, '\\',     
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', NULL,
    NULL, NULL, ' ',
};

static int keyboard_index; // global variable for keyboard buffer index

/*
 * keyboard_init
 *   DESCRIPTION: Initializes keyboard interrupt on the PIC
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Now accepts keyboard interrupts to occur
 */
void keyboard_init(void) {
    enable_irq(1); // keyboard interrupt = IRQ1
    keyboard_index = 0;
}

/*
 * keyboard_handler
 *   DESCRIPTION: Handles keyboard interrupts and prints typed input on keyboard to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Prints input to screen
 */
void keyboard_handler(void) {
    uint8_t scan_code = inb(0x60) & 0xFF; // take in first 8 bits of keyboard input

    if (scan_code < SCAN_CODES) {
        keyboard_buffer[keyboard_index] = scan_codes_table[scan_code]; // get matching character for scan_code
        if(keyboard_buffer[keyboard_index]) {
            putc(keyboard_buffer[keyboard_index]); // print to screen
        }
        keyboard_index++;
    }

    send_eoi(1); // keyboard irq
}
