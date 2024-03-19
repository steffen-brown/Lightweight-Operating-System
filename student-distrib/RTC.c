#include "RTC.h"
#include "lib.h"
#include "i8259.h"

#define RTC_cmd 0x70
#define RTC_data 0x71

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

    outb(0x8B, RTC_cmd); // select register B (0x8B)
    char prev=inb(RTC_data); // read the current value of register B
    outb(0x8B, RTC_cmd); // select register B (0x8B)
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
	// test_interrupts(); // Call screen flash
	outb(0x0C, RTC_cmd); // Unlock the RTC (byte 0x0C)
	inb(RTC_data); // Allowing it to send more interrupts
	rtc_flag = 0;
	send_eoi(8); // Send end of interrupt for the RTC to the pic
	sti(); // Enable interrupts 
}

int rtc_open() {
	rtc_flag = 0;

	cli();

	outb(0x8A, RTC_cmd);
	char prev = inb(RTC_data);
	outb(0x8A, RTC_cmd);
	outb((prev & 0xF0) | INIT_RATE, RTC_data);

	sti();

	return 1;
}

int rtc_close() {
	return 0;
}

int rtc_write(const void* buf) {
	uint32_t frequency = *((uint32_t*)buf);

	if(!((frequency > 0) && ((frequency & (frequency - 1)) == 0)))
		return -1;


	int rate;
	rate = 1;
    int value = 32768;

    while (value > frequency) {
        value >>= 1; // Divide value by 2 using right shift
        rate++;
    }

	if(rate < 3 || rate > 15)
		return -1;

	int bytes_written = 0;

	cli();

	outb(0x8A, RTC_cmd);
	bytes_written++;
	char prev = inb(RTC_data);
	outb(0x8A, RTC_cmd);
	bytes_written++;
	outb((prev & 0xF0) | rate, RTC_data);

	sti();

	return bytes_written;
}

int rtc_read() {
	rtc_flag = 1;

	while(rtc_flag == 1) {};

	return 0;
}
