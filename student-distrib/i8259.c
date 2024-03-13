/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void i8259_init(void) {
    uint8_t cache1, cache2;
    uint32_t flags;

    // Disable interupts and save flags
    cli_and_save(flags);

    // Save current mask
    cache1 = inb(MASTER_8259_DATA_PORT);
    cache2 = inb(SLAVE_8259_DATA_PORT);

    // Mask both of the PICs
    outb(0xFF, MASTER_8259_DATA_PORT);
    outb(0xFF, SLAVE_8259_DATA_PORT);

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
    outb(cache1, MASTER_8259_DATA_PORT);
    outb(cache2, SLAVE_8259_DATA_PORT);

    // Restore interupt flags
    restore_flags(flags);
}

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {
    uint8_t new_mask;
    uint32_t flags;

    // Disable interupts and save flags
    cli_and_save(flags);

    // If IRQ is connected to the primary PIC (IRQ < 8)
    if(irq_num < 8) {
        // Create and output new primary PIC mask w/ new enable bit
        new_mask = inb(MASTER_8259_DATA_PORT) & ~(1 << irq_num);
        outb(new_mask, MASTER_8259_DATA_PORT);
    } else if(irq_num < 16) { // If IRQ is connected to the secondary PIC (IRQ >= 8)
        // Create and output new secondary PIC mask w/ new enable bit
        new_mask = inb(SLAVE_8259_DATA_PORT) & ~(1 << (irq_num - 8));
        outb(new_mask, SLAVE_8259_DATA_PORT);
    }

    // Restore interupt flags
    restore_flags(flags);
}

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
    uint8_t new_mask;
    uint32_t flags;

    // Disable interupts and save flags
    cli_and_save(flags);

    // If IRQ is connected to the primary PIC (IRQ < 8)
    if(irq_num < 8) {
        // Create and output new primary PIC mask w/ new disabled bit
        new_mask = inb(MASTER_8259_DATA_PORT) | (1 << irq_num);
        outb(new_mask, MASTER_8259_DATA_PORT);
    } else if(irq_num < 16) { // If IRQ is connected to the secondary PIC (IRQ >= 8)
        // Create and output new secondary PIC mask w/ new disabled bit
        new_mask = inb(SLAVE_8259_DATA_PORT) | (1 << (irq_num - 8));
        outb(new_mask, SLAVE_8259_DATA_PORT);
    }

    // Restore interupt flags
    restore_flags(flags);
}

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {
    if(irq_num >= 8) {
        // Send EOI to secondary PIC
        outb(SLAVE_8259_PORT, EOI + (irq_num - 8));
        // Send general EOI to primary PIC
        outb(MASTER_8259_PORT, EOI + 2);
    } else {
        // Send EOI to primary PIC
        outb(MASTER_8259_PORT, EOI + irq_num);
    }
}
