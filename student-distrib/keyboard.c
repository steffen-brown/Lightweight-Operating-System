#include "keyboard.h"

// Directory of letters assocated with each scan code for lowercase
char scan_codes_table[SCAN_CODES] = {
    NULL, NULL, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', NULL,      
    ' ', 'q', 'w', 'e', 'r', 't', 'y', 'u',  'i', 'o', 'p', '[', ']', NULL,
    NULL, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', NULL, '\\',     
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', NULL,
    NULL, NULL, ' ',
};

// Directory of letters assocated with each scan code for shifted
char scan_codes_table_shift[SCAN_CODES] = {
    NULL, NULL, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', NULL,      
    ' ', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U',  'I', 'O', 'P', '{', '}', NULL,
    NULL, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', NULL, '|',     
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', NULL,
    NULL, NULL, ' ',
};

// Directory of letters assocated with each scan code for caps lock
char scan_codes_table_caps[SCAN_CODES] = {
    NULL, NULL, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', NULL,      
    ' ', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U',  'I', 'O', 'P', '[', ']', NULL,
    NULL, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`', NULL, '\\',   
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', NULL,
    NULL, NULL, ' ',
};

static int keyboard_index; // global variable for keyboard buffer index

/* global variables for keyboard function keys */
static int shift_flag;
static int caps_lock_flag;
static int ctrl_flag;

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
    ctrl_flag = 0;
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
    } else if (scan_code == LEFT_SHIFT_REL || scan_code == RIGHT_SHIFT_REL) { // handles flags when shift is pressed and released
        shift_flag = 0;
    } 
    
    if (scan_code == CAPS_LOCK && !caps_lock_flag) { // handles flags when caps lock is pressed
        caps_lock_flag = 1;
    } else if (scan_code == CAPS_LOCK && caps_lock_flag) {
        caps_lock_flag = 0;
    }


    if (scan_code == CTRL) { // handles flags when control is pressed and released
        ctrl_flag = 1;
    } else if (scan_code == CTRL_REL) {
        ctrl_flag = 0;
    }

    if (ctrl_flag && scan_code == L) { // clears screen (ctrl L) 
        int i;
        for (i = 0; i < BUFFER_SIZE; i++) {
            keyboard_buffer[i] = '\0';
        }
        keyboard_index = 0;
        
        clear();

        // screen_x = 0;
        // screen_y = 0;
    }

    if (scan_code == TAB) { // handles extra space when tab is pressed
        keyboard_buffer[keyboard_index] = scan_codes_table[scan_code];
        putc(keyboard_buffer[keyboard_index]);
        keyboard_index++;
        keyboard_buffer[keyboard_index] = scan_codes_table[scan_code];
        putc(keyboard_buffer[keyboard_index]);
        keyboard_index++;
    }

    // printf("Shift Flag: %d, Caps Lock Flag: %d, Scan Code: %d\n", shift_flag, caps_lock_flag, scan_code);


    // Filter out invalid or otherwise unhandled scancodes
    if (scan_code < SCAN_CODES && scan_codes_table[scan_code] && !ctrl_flag) {
        if (shift_flag && !caps_lock_flag) {
            keyboard_buffer[keyboard_index] = scan_codes_table_shift[scan_code]; // get matching character shifted for scan_code
        } else if (caps_lock_flag && !shift_flag) {
            keyboard_buffer[keyboard_index] = scan_codes_table_caps[scan_code]; // get matching character for caps locked scan_code
        } else {
            keyboard_buffer[keyboard_index] = scan_codes_table[scan_code]; // get matching character for scan_code
        }
        putc(keyboard_buffer[keyboard_index]); // print to screen
        keyboard_index++; // Advance the keyboard buffer index
    }

    send_eoi(1); // Send EOI for keyboard to the PIC
}
