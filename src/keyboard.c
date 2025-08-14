#include "keyboard.h"
#include "sys_calls.h"
#include "pit.h"
#include "lib.h"

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

static int keyboard_index[NUM_TERMINALS]; // global variable for keyboard buffer index

/* global variables for keyboard function keys */
static int shift_flag;
static int caps_lock_flag;
static int ctrl_flag;
static volatile int enter_flag[NUM_TERMINALS];
static int newline_flag;
static int alt_flag;
int cur_terminal;

uint8_t* videomem_buffer[NUM_TERMINALS] = {
    VIDEO_MEM + 1 * FOUR_KB, // Terminal 1 video memory buffer
    VIDEO_MEM + 2 * FOUR_KB, // Terminal 2 video memory buffer
    VIDEO_MEM + 3 * FOUR_KB  // Terminal 3 video memory buffer
};

/*
 * keyboard_init
 *   DESCRIPTION: Initializes keyboard interrupt on the PIC
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Now accepts keyboard interrupts to occur
 */
void keyboard_init(void) {
    // keyboard interrupt = IRQ1
    enable_irq(1);

    // initialize keyboard for 3 terminals
    keyboard_index[0] = 0;
    keyboard_index[1] = 0;
    keyboard_index[2] = 0;
    cur_terminal = 1;

    // keyboard flags
    shift_flag = 0;
    caps_lock_flag = 0;
    ctrl_flag = 0;
    enter_flag[0] = 0;
    enter_flag[1] = 0;
    enter_flag[2] = 0;
    newline_flag = 0;
    alt_flag = 0;
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
    cli();
    ProcessControlBlock* current_PCB;
    // Assembly code to get the current PCB
    // Mask the lower 13 bits then AND with ESP to align it to the 8KB boundary
    asm volatile (
        "movl %%esp, %%eax\n"       // Move current ESP value to EAX for manipulation
        "andl $0xFFFFE000, %%eax\n" // Clear the lower 13 bits to align to 8KB boundary
        "movl %%eax, %0\n"          // Move the modified EAX value to current_pcb
        : "=r" (current_PCB)        // Output operands
        :                            // No input operands
        : "eax"                      // Clobber list, indicating EAX is modified
    );

    // Get index for screen_x/screen_y arrays by getting base thread/terminal number
    int cursor_idx;
    cursor_idx = cur_terminal - 1;

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
    
    if (scan_code == ALT) { // handles flags when alt is pressed and released
        alt_flag = 1;
    } else if (scan_code == ALT_REL) {
        alt_flag = 0;
    }

    if (scan_code == ENTER) { // handles flags when enter is pressed and released
        enter_flag[cur_terminal - 1] = 1;
        keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1]++] = '\n';
        keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1]] = '\0';
        putc_keyboard('\n');
        keyboard_index[cur_terminal - 1] = 0;
    }

    if (keyboard_index[cur_terminal - 1] == MAX_LINE && keyboard_index[cur_terminal - 1] + 1 < BUFFER_SIZE) { // adds new line when end of line is reached
        keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1]] = '\n';
        putc_keyboard('\n');
        keyboard_index[cur_terminal - 1]++;
        newline_flag = 1;
    }

    if (ctrl_flag && scan_code == L) { // clears screen (ctrl L) 
        int i;
        for (i = 0; i < BUFFER_SIZE; i++) { // resets keyboard buffer
            keyboard_buffer[cur_terminal - 1][i] = '\0';
        }
        keyboard_index[cur_terminal - 1] = 0;
        
        clear();
    }

    if (scan_code == TAB && keyboard_index[cur_terminal - 1] + TAB_SPACE < BUFFER_SIZE && screen_x[cursor_idx] + TAB_SPACE < MAX_LINE) { // handles extra space when tab is pressed
        if (keyboard_index[cur_terminal - 1] + 2 < BUFFER_SIZE) {
            keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1]] = '\t';
            putc_keyboard(' '); // tab space
            keyboard_index[cur_terminal - 1]++;
        }
        if (keyboard_index[cur_terminal - 1] + 2 < BUFFER_SIZE) {
            keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1]] = '\t';
            putc_keyboard(' '); // tab space
            keyboard_index[cur_terminal - 1]++;
        }
        if (keyboard_index[cur_terminal - 1] + 2 < BUFFER_SIZE) {
            keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1]] = '\t';
            putc_keyboard(' '); // tab space
            keyboard_index[cur_terminal - 1]++;
        }
        if (keyboard_index[cur_terminal - 1] + 2 < BUFFER_SIZE) {
            keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1]] = '\t';
            putc_keyboard(' '); // tab space
            keyboard_index[cur_terminal - 1]++;
        }
    }

    if (keyboard_index[cur_terminal - 1] > 0 && keyboard_index[cur_terminal - 1] <= BUFFER_SIZE - 1 && scan_code == BACKSPACE) { // handle backspacing
        if (newline_flag && keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1] - 1] == '\n') { // when backspacing to previous line
            keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1] - 1] = '\0';
            keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1] - 2] = '\0';
            screen_y[cursor_idx]--;
            int cur_y = screen_y[cursor_idx]; // deal with cursor
            screen_x[cursor_idx] = MAX_LINE - 1;
            putc_keyboard(keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1] - 2]); // remove character
            screen_y[cursor_idx] = cur_y;
            screen_x[cursor_idx] = MAX_LINE - 1;
            keyboard_index[cur_terminal - 1] = keyboard_index[cur_terminal - 1] - 2; // move before \n and prev character
            newline_flag = 0;
        } else if (keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1] - 1] == '\t') { // deleting extra spaces when tab exists
            keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1] - 1] = '\0';
            screen_x[cursor_idx]--;
            putc_keyboard(keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1] - 1]); // remove character
            if (keyboard_index[cur_terminal - 1] > 0) {
                screen_x[cursor_idx]--; // update cursor
            }
            keyboard_index[cur_terminal - 1]--;

            keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1] - 1] = '\0';
            screen_x[cursor_idx]--;
            putc_keyboard(keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1] - 1]); // remove character
            if (keyboard_index[cur_terminal - 1] > 0) {
                screen_x[cursor_idx]--; // update cursor
            }
            keyboard_index[cur_terminal - 1]--;

            keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1] - 1] = '\0';
            screen_x[cursor_idx]--;
            putc_keyboard(keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1] - 1]); // remove character
            if (keyboard_index[cur_terminal - 1] > 0) {
                screen_x[cursor_idx]--; // update cursor
            }
            keyboard_index[cur_terminal - 1]--;

            keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1] - 1] = '\0';
            screen_x[cursor_idx]--;
            putc_keyboard(keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1] - 1]); // remove character
            if (keyboard_index[cur_terminal - 1] > 0) {
                screen_x[cursor_idx]--; // update cursor
            }
            keyboard_index[cur_terminal - 1]--;
        } else {
            keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1] - 1] = '\0';
            screen_x[cursor_idx]--;
            putc_keyboard(keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1] - 1]); // remove character
            if (keyboard_index[cur_terminal - 1] > 0) {
                screen_x[cursor_idx]--; // update cursor
            }
            keyboard_index[cur_terminal - 1]--;
        }
    }
    update_cursor(screen_x[cursor_idx], screen_y[cursor_idx]);

    // Filter out invalid or otherwise unhandled scancodes
    if (scan_code < SCAN_CODES && scan_codes_table[scan_code] && !ctrl_flag && !alt_flag && keyboard_index[cur_terminal - 1] < BUFFER_SIZE - 1) {
        if (shift_flag && !caps_lock_flag) {
            keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1]] = scan_codes_table_shift[scan_code]; // get matching character shifted for scan_code
        } else if (caps_lock_flag && !shift_flag) {
            keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1]] = scan_codes_table_caps[scan_code]; // get matching character for caps locked scan_code
        } else {
            keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1]] = scan_codes_table[scan_code]; // get matching character for scan_code
        }
        putc_keyboard(keyboard_buffer[cur_terminal - 1][keyboard_index[cur_terminal - 1]]); // print to screen
        keyboard_index[cur_terminal - 1]++; // Advance the keyboard buffer index
    }

    if (alt_flag && (scan_code == F1 || scan_code == F2 || scan_code == F3)) { 
        int selected_terminal = scan_code - F_OFFSET;


        // Switch vidmem to new terminal
        memcpy(videomem_buffer[cur_terminal - 1], VIDEO_MEM, FOUR_KB);
        memcpy(VIDEO_MEM, videomem_buffer[selected_terminal - 1], FOUR_KB);

        cur_terminal = selected_terminal; // Set the current terminal to match the terminal just selected
        update_cursor(screen_x[cur_terminal-1], screen_y[cur_terminal-1]);
        
        // If no terminal exists, boot it up!
        if(!(base_shell_booted_bitmask & (1 << (selected_terminal - 1)))) { 
            register uint32_t saved_ebp asm("ebp");
            current_PCB->schedEBP = (void*)saved_ebp; // save ebp for scheduling

            // set up and switch vid memory
            shell_init_boot = selected_terminal;
            cur_process = selected_terminal;
            send_eoi(1);
            CONTEXT_SAVE_CALL(execute, (uint8_t*)"shell"); // Save context of sys call and execuate a new shell in a new thread
        }
    }

    asm volatile (
        "movl $0, %%eax\n"          // Clear eax to use it for setting debug registers to 0
        "movl %%eax, %%dr0\n"       // Clear DR0
        "movl %%eax, %%dr1\n"       // Clear DR1
        "movl %%eax, %%dr2\n"       // Clear DR2
        "movl %%eax, %%dr3\n"       // Clear DR3
        "movl %%eax, %%dr7\n"       // Clear DR7 - disables all breakpoints
        :
        :
        : "eax"                     // Clobber list, eax is modified
    );

    outb(0x61, 0x20); // Marco to send end EOI for keyboard | 0x61 encoding for EOI | 0x20 PIC i/o port
    sti();
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
int terminal_read(int32_t fd, void* buffer, int32_t bytes) {
    if(bytes == 0) { // Check if the requested number of bytes to read is 0
        return 0; // If yes, return 0 immediately
    }

    enter_flag[cur_process - 1] = 0; // Reset enter flag

    while(!enter_flag[cur_process - 1]); // Wait for enter to be pressed

    int end_flag = 0; // Flag to indicate the end of reading

    uint8_t* output = (uint8_t*) buffer; // Cast void pointer to uint8_t pointer for byte manipulation

    int output_index = 0; // Index to track the current position in the output buffer
    while (!end_flag && output_index < bytes) {
        output[output_index] = keyboard_buffer[cur_terminal - 1][output_index]; // Copy from keyboard buffer to output buffer
        if (keyboard_buffer[cur_terminal - 1][output_index] == '\0') { // Check for null terminator
            end_flag = 1; // Set end flag if null terminator is found
        }
        output_index++; // Increment index
    }

    if(keyboard_buffer[cur_terminal - 1][127] == '\n') { // If trying to read a full 128 character, account for the lack of early null terminator
        output_index++;
        keyboard_buffer[cur_terminal - 1][127] = '\0'; // Reset null terminator for future strings
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
int terminal_write(int32_t fd, const void* buffer, int32_t bytes) {
    cli();
    if (buffer == NULL || bytes == 0) { // Check for invalid buffer or request to write 0 bytes
        return -1; // Return error code -1
    }

    uint8_t* input = (uint8_t*) buffer; // Cast void pointer to uint8_t pointer for byte manipulation

    int bytes_written = 0; // Counter for the number of bytes written

    int i;
    for(i = 0; i < bytes; i++) { // Iterate over each byte in the input buffer
        if(input[i] == '\t') {
            putc(' ', 0);
        } else {
            putc(input[i], 0); // Write each byte to the terminal
        }
        bytes_written++; // Increment the written byte count

    }

    sti();
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
int terminal_open(const uint8_t* filename) {
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
int terminal_close(int32_t fd) {
    return 0;
}
