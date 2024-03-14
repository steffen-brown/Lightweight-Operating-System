/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask = 0xFF; /* IRQs 0-7  */
uint8_t slave_mask = 0xFF;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void i8259_init(void) {

    outb(0xff, 0x21);	/* mask all of 8259A-1 */
	outb(0xff, 0xA1);	/* mask all of 8259A-2 */

    // Start the intialization seqence (cascasde mode)
    outb(ICW1, MASTER_8259_PORT);
    outb(ICW1, SLAVE_8259_PORT);

    // Set the PICs vector offsets
    outb(ICW2_MASTER, MASTER_8259_DATA_PORT);
    outb(ICW2_SLAVE, SLAVE_8259_DATA_PORT);

    // Tell master PIC ther eis a slave at IRQ2
    outb(ICW3_MASTER, MASTER_8259_DATA_PORT);
    // Tell the slave its in cascade mode
    outb(ICW3_SLAVE, SLAVE_8259_DATA_PORT);

    // Enable 8086 mode on the PICs
    outb(ICW4, MASTER_8259_DATA_PORT);
    outb(ICW4, SLAVE_8259_DATA_PORT);

    // Restore masks
    outb(master_mask, MASTER_8259_DATA_PORT);
    outb(slave_mask, SLAVE_8259_DATA_PORT);

    enable_irq(2);

}

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {
    uint8_t new_mask;

    // If IRQ is connected to the primary PIC (IRQ > 8)
    if(irq_num >= 8) {
        // Create and output new secondary PIC mask w/ new enable bit
        new_mask = ~(1 << (irq_num - 8));
        slave_mask &= new_mask;
        outb(slave_mask, SLAVE_8259_DATA_PORT);
    } else { // If IRQ is connected to the secondary PIC (IRQ >= 8)
        // Create and output new primary PIC mask w/ new enable bit
        new_mask = ~(1 << irq_num);
        master_mask &= new_mask;
        outb(master_mask, MASTER_8259_DATA_PORT);
    }

}

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
    uint8_t new_mask;

    // If IRQ is connected to the primary PIC (IRQ > 8)
    if(irq_num >= 8) { 
        // Create and output new primary PIC mask w/ new disabled bit
        new_mask = 1 << (irq_num - 8);
        slave_mask |= new_mask;
        outb(slave_mask, SLAVE_8259_DATA_PORT);
    } else { // If IRQ is connected to the secondary PIC (IRQ >= 8)
        // Create and output new secondary PIC mask w/ new disabled bit
        new_mask = 1 << irq_num;
        master_mask |= new_mask;
        outb(master_mask, MASTER_8259_DATA_PORT);
    }

}

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {

    if(irq_num >= 8) {
        outb(EOI + (irq_num - 8), SLAVE_8259_PORT); 
        outb(EOI + 2, MASTER_8259_PORT);
    } else {
        outb(EOI + irq_num, MASTER_8259_PORT);
    }

}

