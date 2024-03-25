#include "keyboard.h"

// Directory of letters assocated with each scan code for lowercase
char scan_codes_table[SCAN_CODES] = {
    NULL, NULL, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', NULL,      
    NULL, 'q', 'w', 'e', 'r', 't', 'y', 'u',  'i', 'o', 'p', '[', ']', NULL,
    NULL, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', NULL, '\\',     
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', NULL,
    NULL, NULL, ' ',
};

// Directory of letters assocated with each scan code for shifted
char scan_codes_table_shift[SCAN_CODES] = {
    NULL, NULL, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', NULL,      
    NULL, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U',  'I', 'O', 'P', '{', '}', NULL,
    NULL, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', NULL, '|',     
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', NULL,
    NULL, NULL, ' ',
};

// Directory of letters assocated with each scan code for caps lock
char scan_codes_table_caps[SCAN_CODES] = {
    NULL, NULL, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', NULL,      
    NULL, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U',  'I', 'O', 'P', '[', ']', NULL,
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
        keyboard_buffer[keyboard_index++] = '\n';
        keyboard_buffer[keyboard_index] = '\0';
        putc('\n');
        keyboard_index = 0;
        
    }

    if (ctrl_flag && scan_code == L) { // clears screen (ctrl L) 
        int i;
        for (i = 0; i < BUFFER_SIZE; i++) { // resets keyboard buffer
            keyboard_buffer[i] = '\0';
        }
        keyboard_index = 0;
        
        clear();
    }

    if (scan_code == TAB && keyboard_index + 4 < BUFFER_SIZE) { // handles extra space when tab is pressed
            if (keyboard_index == MAX_LINE && keyboard_index + 1 < BUFFER_SIZE) { // adds new line when end of line is reached
                keyboard_buffer[keyboard_index] = '\n';
                putc('\n');
                keyboard_index++;
                newline_flag = 1;
            }
            keyboard_buffer[keyboard_index] = '\t';
            putc(' ');
            keyboard_index++;
            if (keyboard_index == MAX_LINE && keyboard_index + 1 < BUFFER_SIZE) { // adds new line when end of line is reached
                keyboard_buffer[keyboard_index] = '\n';
                putc('\n');
                keyboard_index++;
                newline_flag = 1;
            }
            keyboard_buffer[keyboard_index] = '\t';
            putc(' ');
            keyboard_index++;
            if (keyboard_index == MAX_LINE && keyboard_index + 1 < BUFFER_SIZE) { // adds new line when end of line is reached
                keyboard_buffer[keyboard_index] = '\n';
                putc('\n');
                keyboard_index++;
                newline_flag = 1;
            }
            keyboard_buffer[keyboard_index] = '\t';
            putc(' ');
            keyboard_index++;
            if (keyboard_index == MAX_LINE && keyboard_index + 1 < BUFFER_SIZE) { // adds new line when end of line is reached
                keyboard_buffer[keyboard_index] = '\n';
                putc('\n');
                keyboard_index++;
                newline_flag = 1;
            }
            keyboard_buffer[keyboard_index] = '\t';
            putc(' ');
            keyboard_index++;
            if (keyboard_index == MAX_LINE && keyboard_index + 1 < BUFFER_SIZE) { // adds new line when end of line is reached
                keyboard_buffer[keyboard_index] = '\n';
                putc('\n');
                keyboard_index++;
                newline_flag = 1;
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
        } if (keyboard_buffer[keyboard_index - 1] == '\t') {
            if (!newline_flag) {
                    keyboard_buffer[keyboard_index - 1] = '\0';
                    screen_x--;
                    putc(keyboard_buffer[keyboard_index - 1]); // remove character
                    if (keyboard_index > 0) {
                        screen_x--; // update cursor
                    }
                    keyboard_index--;
                    keyboard_buffer[keyboard_index - 1] = '\0';
                    screen_x--;
                    putc(keyboard_buffer[keyboard_index - 1]); // remove character
                    if (keyboard_index > 0) {
                        screen_x--; // update cursor
                    }
                    keyboard_index--;
                    keyboard_buffer[keyboard_index - 1] = '\0';
                    screen_x--;
                    putc(keyboard_buffer[keyboard_index - 1]); // remove character
                    if (keyboard_index > 0) {
                        screen_x--; // update cursor
                    }
                    keyboard_index--;
                    keyboard_buffer[keyboard_index - 1] = '\0';
                    screen_x--;
                    putc(keyboard_buffer[keyboard_index - 1]); // remove character
                    if (keyboard_index > 0) {
                        screen_x--; // update cursor
                    }
                    keyboard_index--;
                }
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

    send_eoi(1); // Send EOI for keyboard to the PIC (IRQ 1)
}

/*
 * terminal_read
 *   DESCRIPTION: Reads a line from the terminal into a buffer. It waits for the enter key to be
 *                pressed before copying the contents of the keyboard buffer into the provided buffer.
 *   INPUTS: buffer - pointer to the buffer where the read characters should be stored
 *           bytes - the maximum number of bytes to read into the buffer
 *   OUTPUTS: none
 *   RETURN VALUE: The number of characters read into the buffer, excluding the null terminator
 *   SIDE EFFECTS: Blocks execution until the enter key is pressed
 */
int terminal_read(void* buffer, uint32_t bytes) {
    if(bytes == 0) { // Check if the requested number of bytes to read is 0
        return 0; // If yes, return 0 immediately
    }

    enter_flag = 0; // Reset enter flag

    while(!enter_flag); // Wait for enter to be pressed

    int end_flag = 0; // Flag to indicate the end of reading

    uint8_t* output = (uint8_t*) buffer; // Cast void pointer to uint8_t pointer for byte manipulation

    int output_index = 0; // Index to track the current position in the output buffer
    while (!end_flag && output_index < bytes) {
        output[output_index] = keyboard_buffer[output_index]; // Copy from keyboard buffer to output buffer
        if (keyboard_buffer[output_index] == '\0') { // Check for null terminator
            end_flag = 1; // Set end flag if null terminator is found
        }
        output_index++; // Increment index
    }

    if(keyboard_buffer[127] == '\n') { // If trying to read a full 128 character, account for the lack of early null terminator
        output_index++;
        keyboard_buffer[127] = '\0'; // Reset null terminator for future strings
    }

    return output_index - 1; // Return number of characters read, excluding the null terminator
} 

/*
 * terminal_write
 *   DESCRIPTION: Writes a sequence of bytes from a buffer to the terminal.
 *   INPUTS: buffer - pointer to the buffer containing the bytes to write
 *           bytes - number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: The number of bytes successfully written
 *   SIDE EFFECTS: Outputs the contents of the buffer to the terminal
 */
int terminal_write(void* buffer, uint32_t bytes) {
    if (buffer == NULL || bytes == 0) { // Check for invalid buffer or request to write 0 bytes
        return -1; // Return error code -1
    }

    uint8_t* input = (uint8_t*) buffer; // Cast void pointer to uint8_t pointer for byte manipulation

    int bytes_written = 0; // Counter for the number of bytes written

    int i;
    for(i = 0; i < bytes; i++) { // Iterate over each byte in the input buffer
        putc(input[i]); // Write each byte to the terminal
        bytes_written++; // Increment the written byte count

    }

    return bytes_written; // Return the total number of bytes written

}

/*
 * terminal_open
 *   DESCRIPTION: Placeholder for opening the terminal, currently does nothing specific.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: -1 indicating that the function is not implemented
 *   SIDE EFFECTS: None
 */
int terminal_open() {
    return 0;
}

/*
 * terminal_close
 *   DESCRIPTION: Placeholder for closing the terminal, currently does nothing specific.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: -1 indicating that the function is not implemented
 *   SIDE EFFECTS: None
 */
int terminal_close() {
    return 0;
}
