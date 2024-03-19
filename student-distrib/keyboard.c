#include "keyboard.h"

// Directory of letters assocated with each scan code
char scan_codes_table[SCAN_CODES] = {
    NULL, NULL, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', NULL,      
    ' ', 'q', 'w', 'e', 'r', 't', 'y', 'u',  'i', 'o', 'p', '[', ']', NULL,
    NULL, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', NULL, '\\',     
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', NULL,
    NULL, NULL, ' ',
};

char scan_codes_table_shift[SCAN_CODES] = {
    NULL, NULL, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', NULL,      
    ' ', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U',  'I', 'O', 'P', '{', '}', NULL,
    NULL, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', NULL, '|',     
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', NULL,
    NULL, NULL, ' ',
};

char scan_codes_table_caps[SCAN_CODES] = {
    NULL, NULL, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', NULL,      
    ' ', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U',  'I', 'O', 'P', '[', ']', NULL,
    NULL, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`', NULL, '\\',   
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', NULL,
    NULL, NULL, ' ',
};

static int keyboard_index; // global variable for keyboard buffer index
static int shift_flag;
static int caps_lock_flag;

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
    shift_flag = 0;
    caps_lock_flag = 0;
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
    uint8_t scan_code = inb(KEYBOARD_PORT) & 0xFF; // take in first 8 bits of keyboard input

    if (scan_code == LEFT_SHIFT || scan_code == RIGHT_SHIFT) {
        shift_flag = 1;
    } else if (scan_code == LEFT_SHIFT_REL || scan_code == RIGHT_SHIFT_REL) {
        shift_flag = 0;
    } 
    
    if (scan_code == CAPS_LOCK && !caps_lock_flag) {
        caps_lock_flag = 1;
    } else if (scan_code == CAPS_LOCK && caps_lock_flag) {
        caps_lock_flag = 0;
    }

    printf("Shift Flag: %d, Caps Lock Flag: %d, Scan Code: %d\n", shift_flag, caps_lock_flag, scan_code);

    // Filter out invalid or otherwise unhandled scancodes
    if (scan_code < SCAN_CODES && scan_codes_table[scan_code]) {
        if (shift_flag && caps_lock_flag) {
            keyboard_buffer[keyboard_index] = scan_codes_table[scan_code]; // get matching character for scan_code
            printf("0 ");
        } else if (shift_flag && !caps_lock_flag) {
            keyboard_buffer[keyboard_index] = scan_codes_table_shift[scan_code]; // get matching character for scan_code
            printf("1 ");
        } else if (caps_lock_flag && !shift_flag) {
            keyboard_buffer[keyboard_index] = scan_codes_table_caps[scan_code]; // get matching character for scan_code
            printf("2 ");
        } else {
            keyboard_buffer[keyboard_index] = scan_codes_table[scan_code]; // get matching character for scan_code
            printf("3 ");
        }
        putc(keyboard_buffer[keyboard_index]); // print to screen
        keyboard_index++; // Advance the keyboard buffer index
    }

    send_eoi(1); // Send EOI for keyboard to the PIC
}
