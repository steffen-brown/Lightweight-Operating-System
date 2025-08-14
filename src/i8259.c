/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask = 0xFF; /* IRQs 0-7  */
uint8_t slave_mask = 0xFF;  /* IRQs 8-15 */

/*
 * i8259_init
 *   DESCRIPTION: Initializes the 8259 Programmable Interrupt Controller (PIC) for IRQ handling. This involves
 *                masking all IRQs initially, starting the initialization sequence in cascade mode, setting
 *                vector offsets for both master and slave PICs, configuring them for 8086 mode, and restoring
 *                the IRQ masks to their previous state. Finally, it enables the IRQ line for the cascade
 *                between the master and slave PICs.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Modifies the operational mode of the 8259 PICs, affecting how IRQs are handled.
 */
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

    enable_irq(2); // Enable IRQ for the secondary pic on primary

}

/*
 * enable_irq
 *   DESCRIPTION: Enables (unmasks) a specific IRQ line on the 8259 PIC, allowing the system to
 *                receive hardware interrupts on that line. It adjusts the IRQ masks for either
 *                the master or slave PIC depending on the IRQ number.
 *   INPUTS: irq_num - The IRQ line number to be enabled.
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Modifies the interrupt mask on the 8259 PIC, potentially allowing new interrupts to occur.
 */
void enable_irq(uint32_t irq_num) {
    uint8_t new_mask;

    // If IRQ is connected to the primary PIC (IRQ >= 8)
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

/*
 * disable_irq
 *   DESCRIPTION: Disables (masks) a specific IRQ line on the 8259 PIC, preventing the system from
 *                receiving hardware interrupts on that line. It adjusts the IRQ masks for either
 *                the master or slave PIC depending on the IRQ number.
 *   INPUTS: irq_num - The IRQ line number to be disabled.
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Modifies the interrupt mask on the 8259 PIC, blocking interrupts from the specified line.
 */
void disable_irq(uint32_t irq_num) {
    uint8_t new_mask;

    // If IRQ is connected to the primary PIC (IRQ >= 8)
    if(irq_num >= 8) { 
        // If IRQ is connected to the secondary PIC (IRQ >= 8)
        // Create and output new primary PIC mask w/ new disabled bit
        new_mask = 1 << (irq_num - 8); // Offset of 8
        slave_mask |= new_mask;
        outb(slave_mask, SLAVE_8259_DATA_PORT);
    } else { 
        // Create and output new secondary PIC mask w/ new disabled bit
        new_mask = 1 << irq_num;
        master_mask |= new_mask;
        outb(master_mask, MASTER_8259_DATA_PORT);
    }

}

/*
 * send_eoi
 *   DESCRIPTION: Sends an End-Of-Interrupt (EOI) signal to the 8259 PIC for a specific IRQ line.
 *                This signals the PIC that the interrupt has been handled and the PIC can resume
 *                sending interrupt requests. For interrupts from the slave PIC, an EOI is also sent
 *                to the master PIC for the cascade IRQ (IRQ 2).
 *   INPUTS: irq_num - The IRQ line number that has been handled.
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Signals the 8259 PIC to resume sending interrupts for the specified IRQ line.
 */
void send_eoi(uint32_t irq_num) {
    if(irq_num >= 8) { // If IRQ is connected to secondary PIC (IRQ >= 8)
        outb(EOI + (irq_num - 8), SLAVE_8259_PORT); // Send EOI for approriate IRQ port on secondary pic (offset 8)
        outb(EOI + 2, MASTER_8259_PORT); // Send EOI for IRQ 2 on the primary pic
    } else {
        outb(EOI + irq_num, MASTER_8259_PORT); // Send EOI for approriate IRQ port on primary pic
    }
}

