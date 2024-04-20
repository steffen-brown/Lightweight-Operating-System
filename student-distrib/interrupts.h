#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#define interrupt 0xE
#define trap 0xF

// See c file for descriptions
extern void RTC_linkage();
extern void keyboard_linkage();
extern void PIT_linkage();

extern void division_error_linkage();
extern void debug_linkage();
extern void NMI_linkage();
extern void breakpoint_linkage();
extern void overflow_linkage();
extern void bound_range_interrupt_linkage();
extern void invalid_opcode_linkage();
extern void device_not_avalible_linkage();
extern void double_fault_linkage();
extern void invalid_tss_linkage();
extern void segment_not_present_linkage();
extern void stack_segfault_linkage();
extern void general_protection_fault_linkage();
extern void page_fault_linkage();
extern void x87_floating_point_linkage();
extern void alignment_check_linkage();
extern void machine_check_linkage();
extern void SIMD_floating_point_linkage();
extern void assert_fault_linkage();
extern void coprocessor_overrun_linkage();

extern void system_call_linkage();

extern void setup_IDT();


#endif
