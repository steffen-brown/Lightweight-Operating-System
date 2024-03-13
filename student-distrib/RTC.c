#include "RTC.h"
#include "lib.h"
#include "i8259.h"

void RTC_init() {
    /*
    RTC Status register B:

	    |7|6|5|4|3|2|1|0|  RTC Status Register B
	     | | | | | | | `---- 1=enable daylight savings, 0=disable (default)
	     | | | | | | `----- 1=24 hour mode, 0=12 hour mode (24 default)
	     | | | | | `------ 1=time/date in binary, 0=BCD (BCD default)
	     | | | | `------- 1=enable square wave frequency, 0=disable
	     | | | `-------- 1=enable update ended interrupt, 0=disable
	     | | `--------- 1=enable alarm interrupt, 0=disable
	     | `---------- 1=enable periodic interrupt, 0=disable
	     `----------- 1=disable clock update, 0=update count normally
    */

    outb(0x8B, 0x70); // select register B
    char prev=inb(0x71); // read the current value of register B
    outb(0x8B, 0x70); // select register B
    outb(prev | 0x40, 0x71); // Turns on bit 6 of register B

	enable_irq(8);
}

void RTC_handler() {
    //test_interrupts();
	printf("RTC        \n");
	send_eoi(1);
}
