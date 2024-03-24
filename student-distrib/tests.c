#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "RTC.h"
#include "keyboard.h"
#include "file_sys.h"
#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 0x13; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	} 

	return result;
}

/*
 * exceptions_test
 *   DESCRIPTION: Tests CPU exception handling by deliberately triggering specific exceptions.
 *                Supports triggering a Division Exception (Divide-by-zero) and an Overflow Exception.
 *   INPUTS: vector - The exception vector to test. 0x0 for Division Exception, 0x4 for Overflow Exception.
 *   OUTPUTS: May print the result of a division or trigger an exception.
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Triggers CPU exceptions based on the input vector. This can halt the system
 *                 or require handling by an exception handler.
 */
void exceptions_test(int vector){
	TEST_HEADER;

	if(vector == 0x0) {
		// Division Exception
		int a = 1;
		int b = 0;
		int result = a / b;
		printf("%d", result);
	} else if(vector == 0x4) {
		// Overflow Exception
		asm volatile (
            "mov $0x7fffffff, %eax\n\t"
            "add %eax, %eax\n\t"     
            "into\n\t"                 
        );
	}

}

/*
 * rtc_interrupt_test
 *   DESCRIPTION: Enables the IRQ line for the RTC, allowing the system to receive RTC interrupts.
 *                This function is typically used to test the RTC interrupt handling.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Enables the RTC interrupts on the PIC, which can affect system behavior and timekeeping.
 */
void rtc_interrupt_test() {
	enable_irq(8);
}

/*
 * paging_setup_test
 *   DESCRIPTION: Attempts to dereference a given memory address, which can be used to test
 *                page fault handling. Typically used to access protected or non-present pages.
 *   INPUTS: memory_address - The memory address to dereference and read.
 *   OUTPUTS: Prints the attempt and the value read from the memory address, if successful.
 *   RETURN VALUE: none
 *   SIDE EFFECTS: May cause a page fault if the memory address is protected or not present,
 *                 which must be handled by the page fault handler.
 */
void paging_setup_test(int memory_address) {
	TEST_HEADER;

	printf("Attempting to dereference memory at %x\n", memory_address);
	char *ptr = (char *)memory_address; // Typically, address zero is protected.
	char out = *ptr; // This line attempts to write to the protected address.
	printf("Successfully Dereferenced value: %c\n", out);
}

/*
 * syscall_test
 *   DESCRIPTION: Triggers a software interrupt for the system call interrupt vector (0x80),
 *                which is typically used to enter the kernel from user space in a controlled manner.
 *                This can be used to test system call handling.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Transitions control to the system call interrupt handler, affecting system state
 *                 and potentially modifying kernel or user space resources based on the system call implementation.
 */
void syscall_test() {
	asm volatile ("int $0x80");
}



/* Test suite entry point */
void launch_tests(){
	// disable_irq(8);
	clear();

	//TEST_OUTPUT("idt_test", idt_test());
	
	/* Multiple Exception Testing */

	//exceptions_test(0x0); // Divide by 0
	// exceptions_test(0x4); // Overflow

	/* RTC Testing */

	//rtc_interrupt_test();

	/* Paging Testing (all edges) */

	// paging_setup_test(0xB8000); // Start of video page
	// paging_setup_test(0xB7FFF); // below video page

	// paging_setup_test(0xB8FFF); // End of kernal page
	// paging_setup_test(0xB9000); // Above end of kernal page

	// paging_setup_test(0x400000); // Start of kernal page
	// paging_setup_test(0x3FFFFF); // below kernal page

	// paging_setup_test(0x7FFFFF); // end of kernal page
	// paging_setup_test(0x800000); // above end of kernal page

	/* Syscall and Keyboard Echo Testing*/

	// syscall_test(); // Test int x80 (Also can do keyboard echoing)

	// uint8_t text[120];
	// int bytes = terminal_read(text, 120);
	// text[119] = '\0';


	// terminal_write(text, bytes);
	// file_open_test_pos();
	// file_open_test_neg();
	file_read_test();
	// dir_read_test();
}

/* Checkpoint 2 tests */
// File open test
int file_open_test_pos(){
	TEST_HEADER;
	int32_t ret = file_open((uint8_t*)"frame0.txt");
	if(ret == -1){
		printf("File open failed\n");
		return FAIL;
	}
	printf("File open success\n");
	return PASS;
}

int file_open_test_neg(){
	TEST_HEADER;
	int32_t ret = file_open((uint8_t*)"sad.txt");
	if(ret == -1){
		printf("File open failed\n");
		return FAIL;
	}
	printf("File open success\n");
	return PASS;
}

int file_read_test(){
	TEST_HEADER;
	clear();
	uint8_t  buf[8192];
	int r;
	int32_t nb=8192;
	const uint8_t* file_name = (uint8_t *)"frame0.txt";
	int f;
	f = file_open(file_name);
	r = file_read(f, buf, nb);
	printf("%s", buf);
	return PASS;

}

int dir_read_test(){
	TEST_HEADER;
	clear();
	uint8_t  buf[34];
	int32_t file_type, file_size,ret;
	uint32_t i;
	uint32_t dir_num = g_boot_block->num_dir_entries;
	for (i = 0; i < dir_num; i++) {
		memset(buf, 0, 34); // Fill the buffer with zeroes
		ret = dir_read(i, buf, 1024);
		if (ret == -1) {
			printf("dir_read failed\n");
			return FAIL;
		}
		// get the file type and size
		file_type = g_boot_block->dir_entries[i].file_type;
		file_size = g_inodes[g_boot_block->dir_entries[i].inode_num].size;
		
		printf("File name: %s, ", buf);
		printf("File type: %d,  ", file_type);
		printf("File size: %d, ", file_size);
		printf("\n");


	}

}

int read_data_test(){
	TEST_HEADER;
	clear();
	uint8_t  buf[8192];
	int r;
	int32_t nb=8192;
	uint32_t inode_ptr = g_boot_block->dir_entries[7].inode_num;
    uint32_t file_size = g_inodes[inode_ptr].size; // size in bytes

	int inode = 0;
	int offset = 0;
	r = read_data(inode, offset, buf, nb);
	printf("%s", buf);
	return PASS;

}

// File close test
// file read test
// file write test
// dir read test
// dir write test
// dir open test
// dir close test

