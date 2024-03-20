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
static volatile int enter_flag;
static int newline_flag;

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
    enter_flag = 0;
    newline_flag = 0;
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

    if (scan_code == ENTER) { // handles flags when enter is pressed and released
        enter_flag = 1;
    } else if (scan_code == ENTER_REL) {
        enter_flag = 0;
    }

    if (keyboard_index == MAX_LINE && keyboard_index + 1 < BUFFER_SIZE) { // adds new line when end of line is reached
        keyboard_buffer[keyboard_index] = '\n';
        putc('\n');
        keyboard_index++;
        newline_flag = 1;
    }

    if (enter_flag) { // adds newline when enter is hit
        int i;

        for (i = 0; i < BUFFER_SIZE; i++) { // resets keyboard buffer
            keyboard_buffer[i] = '\0';
        }
        keyboard_index = 0;

        putc('\n');
    }

    if (ctrl_flag && scan_code == L) { // clears screen (ctrl L) 
        int i;
        for (i = 0; i < BUFFER_SIZE; i++) { // resets keyboard buffer
            keyboard_buffer[i] = '\0';
        }
        keyboard_index = 0;
        
        clear();
    }

    if (scan_code == TAB && keyboard_index + 1 < BUFFER_SIZE) { // handles extra space when tab is pressed
        if (keyboard_index + 2 < BUFFER_SIZE) {
            keyboard_buffer[keyboard_index] = scan_codes_table[scan_code];
            putc(keyboard_buffer[keyboard_index]);
            keyboard_index++;
        }
        if (keyboard_index + 2 < BUFFER_SIZE) {
            keyboard_buffer[keyboard_index] = scan_codes_table[scan_code];
            putc(keyboard_buffer[keyboard_index]);
            keyboard_index++;
        }
    }

    if (keyboard_index > 0 && keyboard_index <= BUFFER_SIZE - 1 && scan_code == BACKSPACE) { // handle backspacing
        if (newline_flag && keyboard_buffer[keyboard_index - 1] == '\n') { // when backspacing to previous line
            keyboard_buffer[keyboard_index - 1] = '\0';
            keyboard_buffer[keyboard_index - 2] = '\0';
            screen_y--;
            int cur_y = screen_y; // deal with cursor
            screen_x = MAX_LINE - 1;
            putc(keyboard_buffer[keyboard_index - 2]); // remove character
            screen_y = cur_y;
            screen_x = MAX_LINE - 1;
            keyboard_index = keyboard_index - 2; // move before /n and prev character
            newline_flag = 0;
        } else {
            keyboard_buffer[keyboard_index - 1] = '\0';
            screen_x--;
            putc(keyboard_buffer[keyboard_index - 1]); // remove character
            if (keyboard_index > 0) {
                screen_x--; // update cursor
            }
            keyboard_index--;
        }
    }

    // Filter out invalid or otherwise unhandled scancodes
    if (scan_code < SCAN_CODES && scan_codes_table[scan_code] && !ctrl_flag && keyboard_index < BUFFER_SIZE - 1) {
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

int terminal_read(void* buffer, uint32_t bytes) {
    if(bytes == 0) {
        return 0;
    }

    enter_flag = 0;

    while(!enter_flag); // Wait for enter to be pressed

    int end_flag = 0;

    uint8_t* output = (uint8_t*) buffer;

    int output_index = 0;
    while (!end_flag && output_index < bytes) {
        output[output_index] = keyboard_buffer[output_index];
        if (keyboard_buffer[output_index] == '\n') {
            end_flag = 1;
        }
        output_index++;
    }

    return output_index;
} 
