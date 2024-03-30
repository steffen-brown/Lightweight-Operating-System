#include "RTC.h"
#include "lib.h"
#include "i8259.h"

#define RTC_cmd 0x70
#define RTC_data 0x71

#define RTC_register_A 0x8A
#define RTC_register_B 0x8B

#define INIT_RATE 15

volatile int rtc_flag;

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

    outb(RTC_register_B, RTC_cmd); // select register B (0x8B)
    char prev=inb(RTC_data); // read the current value of register B
    outb(RTC_register_B, RTC_cmd); // select register B (0x8B)
    outb(prev | 0x40, RTC_data); // Turns on bit 6 of register B (Enabling interrupts)

	enable_irq(8); // Enable RTC on the PIC
}

/*
 * RTC_handler
 *   DESCRIPTION: The interrupt handler for the Real-Time Clock (RTC). It disables interrupts
 *                temporarily, triggers a screen flash (or any defined test_interrupts behavior),
 *                and then acknowledges the RTC interrupt to allow further interrupts. Finally,
 *                it re-enables interrupts.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Disables and then re-enables interrupts. Acknowledges the RTC interrupt to
 *                 clear the interrupt request and allow for future RTC interrupts. May cause
 *                 a screen flash or other behaviors defined in test_interrupts().
 */
void RTC_handler() {
	cli(); // Disable interrupts
	//test_interrupts(); // Call screen flash
	if(screen_x == 79) {
		putc('\n');
	}
	outb(0x0C, RTC_cmd); // Unlock the RTC (byte 0x0C)
	inb(RTC_data); // Allowing it to send more interrupts
	rtc_flag = 0;
	send_eoi(8); // Send end of interrupt for the RTC to the pic
	sti(); // Enable interrupts 
}

/*
 * rtc_open
 *   DESCRIPTION: Prepares the RTC for use by setting an initial rate for RTC interrupts.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 1 indicating success
 *   SIDE EFFECTS: Modifies the rate in register A, affecting interrupt frequency.
 */
int rtc_open() {
	rtc_flag = 0; // Reset the RTC interrupt flag

	cli(); // Disable interrupts

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
int rtc_close() {
	return 0; // Success
}

/*
 * rtc_write
 *   DESCRIPTION: Adjusts the RTC's interrupt frequency by setting the rate in register A.
 *                The frequency must be a power of 2 and within valid bounds.
 *   INPUTS: buf - pointer to a 32-bit integer representing the desired frequency
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure (invalid frequency)
 *   SIDE EFFECTS: Changes the interrupt rate/frequency of the RTC.
 */
int rtc_write(const void* buf) {
	if(buf == 0) {
		return -1;
	}

	uint32_t frequency = *((uint32_t*)buf); // Desired frequency

	// Validate frequency is a power of 2 and within valid bounds
	if(!((frequency > 0) && ((frequency & (frequency - 1)) == 0)))
		return -1; // Invalid frequency


	int rate;
	rate = 1;  // Initialize rate
    int value = 32768; // Max RTC frequency (2^15)

    while (value > frequency) {
        value >>= 1; // Divide value by 2 using right shift
        rate++;
    }

	// Check if rate is within RTC's valid range
	if(rate < 3 || rate > 15) // RTC rate valid range
		return -1; // Invalid rate

	// Set the rate in register A
	cli(); // Disable interrupts

	outb(RTC_register_A, RTC_cmd); // Select register A
	char prev = inb(RTC_data); // Read current value of register A
	outb(RTC_register_A, RTC_cmd); // Re-select register A
	outb((prev & 0xF0) | rate, RTC_data); // Set new rate

	sti(); // Enable interrupts

	return 0; // Success
}

/*
 * rtc_read
 *   DESCRIPTION: Waits for an interrupt to occur (signaled by rtc_flag), effectively
 *                synchronizing on the RTC's interrupt rate.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 indicating success
 *   SIDE EFFECTS: Blocks until an RTC interrupt occurs.
 */
int rtc_read() {
	rtc_flag = 1; // Set the flag to wait for an interrupt

	// Wait for the RTC interrupt handler to clear the flag
	while(rtc_flag == 1) {};

	return 0; // Success
}
