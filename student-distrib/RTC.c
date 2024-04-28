#include "RTC.h"
#include "lib.h"
#include "i8259.h"
#include "pit.h"
#include "sys_calls.h"
#define RTC_cmd 0x70
#define RTC_data 0x71

#define RTC_register_A 0x8A
#define RTC_register_B 0x8B

#define INIT_RATE 6
// #define INIT_FREQ 8192
#define MAX_FREQ 1024

volatile int rtc_flag[NUM_TERMINALS];
volatile uint32_t rtc_counter[NUM_TERMINALS];
volatile uint32_t rtc_freq[NUM_TERMINALS]= {INIT_RATE_DEFAULT,INIT_RATE_DEFAULT,INIT_RATE_DEFAULT}; // Default to the initial rate
/*
 * RTC_init
 *   DESCRIPTION: Initializes the Real-Time Clock (RTC) by enabling RTC interrupts. This function
 *                modifies the RTC's register B to turn on bit 6, which enables periodic interrupts
 *                from the RTC. It then enables the RTC interrupt line (IRQ 8) on the Programmable
 *                Interrupt Controller (PIC).
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Modifies the state of register B in the RTC. Enables RTC interrupts on the PIC.
 */
void RTC_init() {
	cli(); // Disable interrupts
    outb(RTC_register_B, RTC_cmd); // select register B (0x8B), and disable NMI
    char prev=inb(RTC_data); // read the current value of register B
    outb(RTC_register_B, RTC_cmd); // set the index again (a read will reset the index to register D)
    outb(prev | 0x40, RTC_data); // Turns on bit 6 of register B (Enabling interrupts)
	// irq turned on with default 1024 Hz rate
    enable_irq(8); // Enable RTC on the PIC
	sti(); // Enable interrupts
}

/*
 * RTC_handler
 *   DESCRIPTION: The interrupt handler for the Real-Time Clock (RTC). It disables interrupts
 *                temporarily, checks for valid counters and then decrements the active counter,
 *                acknowledges the RTC interrupt to allow further interrupts, and finally,
 *                re-enables interrupts.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Disables and then re-enables interrupts. Acknowledges the RTC interrupt to
 *                 clear the interrupt request and allow for future RTC interrupts.
 */
void RTC_handler() {

    int curProcess = get_current_process(); // Function to get the current active thread index

    int processes_active = 1;
    int i;
    for(i = 1; i < NUM_TERMINALS; i++) {
        if((base_shell_booted_bitmask >> i) & 0x1) {
            processes_active++;
        }
    }

	if((rtc_counter[curProcess] % (MAX_FREQ / (rtc_freq[curProcess] * processes_active))) == 0) {
        // if(rtc_counter[curThread] % 256 == 0) {
			rtc_flag[curProcess] = 0;
			
		}
	rtc_counter[curProcess]++; // Increment the counter for the current thread

    outb(0x0C, RTC_cmd); // Unlock the RTC
    inb(RTC_data); // Clear interrupt flag
    send_eoi(8); // Send end of interrupt for the RTC to the PIC

}

/*
 * rtc_open
 *   DESCRIPTION: Prepares the RTC for use by setting an initial rate for RTC interrupts.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 1 indicating success
 *   SIDE EFFECTS: Modifies the rate in register A, affecting interrupt frequency.
 */
int rtc_open(const uint8_t* filename) {
    cli(); // Disable interrupts
	int curProcess = get_current_process();
    rtc_flag[curProcess] = 0; // Reset the RTC interrupt

    // Set initial rate in register A
    outb(RTC_register_A, RTC_cmd); // Select register A
    char prev = inb(RTC_data); // Read current value of register A
    outb(RTC_register_A, RTC_cmd); // Re-select register A
    outb((prev & 0xF0) | INIT_RATE, RTC_data); // Set rate to initial rate (defined by INIT_RATE)

    sti(); // Enable interrupts

    return 1; // Success
}

/*
 * rtc_close
 *   DESCRIPTION: Placeholder for closing the RTC, currently does nothing.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 indicating success
 *   SIDE EFFECTS: None.
 */
int rtc_close(int32_t fd) {
    return 0; // Success
}

/*
 * rtc_write
 *   DESCRIPTION: Adjusts the RTC's interrupt frequency by setting the rate in register A.
 *                The frequency must be a power of 2 and within valid bounds.
 *   INPUTS: buf - pointer to a 32-bit integer representing the desired frequency
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure (invalid
 *                frequency or wrong buffer size).
 *   SIDE EFFECTS: Changes the interrupt rate/frequency of the RTC.
 */
int rtc_write(int32_t fd, const void* buf, int32_t nbytes) {
	cli();

    uint32_t frequency = *((uint32_t*) buf); // Extract the desired frequency from buffer
    if(!((frequency > 0) && ((frequency & (frequency - 1)) == 0)))
		return -1; // Invalid frequency

    int curProcess = get_current_process(); // Assume function to get current thread index

	int rate;
	rate = 1;  // Initialize rate
    int value = 32768; // Max RTC frequency (2^15)

	while (value > frequency) {
        value >>= 1; // Divide value by 2 using right shift
        rate++;
    }
	if(rate < 3 || rate > 15) // RTC rate valid range
		return -1; // Invalid rate

	rtc_freq[curProcess] = frequency; // Set the frequency of the thread accordingly

    sti(); // Re-enable interrupts

    return 0; // Success
}

/*
 * rtc_read
 *   DESCRIPTION: Waits for an RTC interrupt to occur (signaled by rtc_flag), effectively
 *                synchronizing on the RTC's interrupt rate.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 indicating success
 *   SIDE EFFECTS: Blocks until an RTC interrupt occurs.
 */
int rtc_read(int32_t fd, void* buf, int32_t nbytes) {
    int curProcess = get_current_process(); // Assume function to get current thread index

    rtc_flag[curProcess] = 1; // Set the flag to wait for an interrupt

    while (rtc_flag[curProcess] == 1) {
        // Wait for the RTC_handler to clear the flag
    }

    rtc_flag[curProcess] = 1; // Reset the flag for future reads

    return 0; // Success
}
