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

// /*
//  * rtc_wait
//  *   DESCRIPTION: Waits for a specified number of RTC (Real-Time Clock) interrupt cycles.
//  *                This is typically used to delay execution for a period dependent on the current RTC frequency.
//  *   INPUTS: cycles - The number of RTC interrupt cycles to wait for.
//  *   OUTPUTS: none
//  *   RETURN VALUE: none
//  *   SIDE EFFECTS: Delays the execution by synchronizing with the RTC interrupts, effectively causing
//  *                 the calling thread to wait for the specified number of cycles. This can impact
//  *                 system responsiveness and scheduling, depending on the duration of the wait.
//  */
// void rtc_wait(int cycles) {
// 	int i;
// 	for(i = 0; i < cycles; i++) {
// 		rtc_read();
// 	}
// }

// /*
//  * rtc_frequency_test
//  *   DESCRIPTION: Tests the RTC (Real-Time Clock) by setting its interrupt frequency to various values and
//  *                displaying the current frequency on screen. It demonstrates the RTC's ability to generate
//  *                interrupts at different rates, which can be used for timing and scheduling purposes.
//  *   INPUTS: none
//  *   OUTPUTS: none
//  *   RETURN VALUE: none
//  *   SIDE EFFECTS: Changes the RTC interrupt frequency multiple times, which affects the timing and scheduling
//  *                 of RTC interrupt handling. This function may also modify the screen output to display the
//  *                 current frequency. Enabling and disabling IRQs can impact system interrupt handling.
//  */
// void rtc_frequency_test() {
// 	// enable_irq(8);
// 	// clear();
// 	// putc('2');

// 	// rtc_open();
// 	// rtc_wait(4);

// 	// int i;
// 	// for(i = 4; i <= 2056; i *= 2) {
// 	// 	clear();
// 	// 	printf("%d", i);
// 	// 	// rtc_write(&i);
// 	// 	rtc_wait(i);
// 	// }
	
// 	// rtc_close();
// 	// disable_irq(8);

// 	// clear();
// }

// /*
//  * keyboard_test
//  *   DESCRIPTION: Continuously reads input from the keyboard and echoes it back to the screen until the
//  *                sequence 'next' is entered. This function is used to test keyboard input and terminal
//  *                output functionality.
//  *   INPUTS: none
//  *   OUTPUTS: none
//  *   RETURN VALUE: none
//  *   SIDE EFFECTS: Reads keyboard input and writes to the terminal, which can modify the terminal display.
//  *                 The function loops indefinitely until a specific input sequence is received, potentially
//  *                 affecting system responsiveness or leading to a hang if the exit condition is not met.
//  */
// void keyboard_test() {
// 	terminal_open();

// 	uint8_t text_buffer[128];
// 	while(!(text_buffer[0] == 'n' && text_buffer[1] == 'e' && text_buffer[2] == 'x' && text_buffer[3] == 't')) {
// 		int bytes_read = terminal_read(text_buffer, 128);
// 		printf("Number of bytes read: %d\n", bytes_read);
// 		terminal_write(text_buffer, bytes_read);
// 	}
	
// 	clear();
// 	terminal_close();
// }

// /*
//  * file_open_test_pos
//  *   DESCRIPTION: Tests the file open operation with a positive case where the file exists ("frame0.txt"). It
//  *                attempts to open the file and checks for success.
//  *   INPUTS: none
//  *   OUTPUTS: none
//  *   RETURN VALUE: PASS if the file exists and is opened successfully; FAIL otherwise.
//  *   SIDE EFFECTS: Opens a file, which may affect file system state or file descriptor table.
//  */
// int file_open_test_pos(){
// 	TEST_HEADER;
// 	int32_t ret = file_open((uint8_t*)"frame0.txt");
// 	if(ret == -1){
// 		printf("File open failed\n");
// 		return FAIL;
// 	}
// 	printf("File open success. frame0.txt does exist so that is good.\n");
// 	return PASS;
// }

// /*
//  * file_open_test_neg
//  *   DESCRIPTION: Tests the file open operation with a negative case where the file does not exist ("sad.txt"). It
//  *                attempts to open the file and expects failure.
//  *   INPUTS: none
//  *   OUTPUTS: none
//  *   RETURN VALUE: PASS if the file does not exist and the open operation fails; FAIL otherwise.
//  *   SIDE EFFECTS: Attempts to open a non-existing file, which should not affect the file system but may
//  *                 modify internal state related to error handling.
//  */
// int file_open_test_neg(){
// 	TEST_HEADER;
// 	int32_t ret = file_open((uint8_t*)"sad.txt");
// 	if(ret == -1){
// 		printf("File open failed. sad.txt does not exist so that is good.\n");
// 		return FAIL;
// 	}
// 	printf("File open success\n");
// 	return PASS;
// }

// /*
//  * file_read_test
//  *   DESCRIPTION: Tests reading from a file. It tries to read the contents of a specified file into a buffer and
//  *                displays the content based on the file type (text or binary).
//  *   INPUTS: char* file - the name of the file to be read.
//  *   OUTPUTS: none
//  *   RETURN VALUE: PASS if the file is successfully read; the specific return value may vary based on the test's
//  *                 implementation details.
//  *   SIDE EFFECTS: Reads a file which may affect the file descriptor's read pointer or the terminal display
//  *                 if the content is printed.
//  */
// int file_read_test(char * file){
// 	TEST_HEADER;
// 	clear();
// 	uint8_t buf[40000];
// 	int32_t r; int32_t f;
// 	char type[4];
// 	int32_t i;
// 	int32_t nb=40000;

// 	const uint8_t* file_name = (uint8_t *)file;

// 	// To check if txt file or not
// 	strncpy(type, file + strlen(file) - 4, 4);
// 	memset(buf, 0, 40000);

// 	f = file_open(file_name);
// 	if (f == -1){
// 		printf("File open failed. Too long a file name perhaps?");
// 		return PASS;
// 	}

// 	r = file_read(f, buf, nb);

// 	if (strncmp((const int8_t *)type, (const int8_t *)".txt", 4) == 0){
// 		printf("%s", buf);
// 	}
// 	else {
// 		for (i = 0; i < r; i++){
// 			if(i == 80) {
// 				putc('\n');
// 			}
// 			if(buf[i] != '\0') 
// 				putc(buf[i]);

// 		}

// 		printf("%x\n", buf[23]);
// 		printf("%x\n", buf[24]);
// 		printf("%x\n", buf[25]);
// 		printf("%x\n", buf[26]);
// 		printf("%x\n", buf[27]);
// 	}

// 	printf("\n");
// 	file_close(f);
// 	printf("\nFile Name: %s\n", file);
// 	printf("File closed successfully\n");

// 	return PASS;

// }

// /*
//  * dir_read_test
//  *   DESCRIPTION: Tests reading directory entries from a filesystem. It sequentially reads each directory entry,
//  *                displaying the name, type, and size of the files contained.
//  *   INPUTS: none
//  *   OUTPUTS: none
//  *   RETURN VALUE: PASS if all directory entries are read successfully; FAIL otherwise.
//  *   SIDE EFFECTS: Accesses filesystem metadata which can affect system performance or caching but does not
//  *                 modify any filesystem state.
//  */
// int dir_read_test(){
// 	TEST_HEADER;
// 	clear();
// 	uint8_t  buf[34];
// 	int32_t file_type, file_size,ret;
// 	uint32_t i;
// 	uint32_t dir_num = g_boot_block->num_dir_entries;
// 	for (i = 0; i < dir_num; i++) {
// 		memset(buf, 0, 34); // Fill the buffer with zeroes
// 		ret = dir_read(i, buf, 1024);
// 		if (ret == -1) {
// 			printf("dir_read failed\n");
// 			return FAIL;
// 		}
// 		// get the file type and size
// 		file_type = g_boot_block->dir_entries[i].file_type;
// 		file_size = g_inodes[g_boot_block->dir_entries[i].inode_num].size;
		
// 		printf("File name: %s, ", buf);
// 		printf("File type: %d,  ", file_type);
// 		printf("File size: %d, ", file_size);
// 		printf("\n");
// 	}
// 	return PASS;
// }


// /*
//  * dir_write_test
//  *   DESCRIPTION: Tests write operation on a directory, which should fail as the filesystem is read-only. This
//  *                test attempts to write to a directory and checks for the expected failure.
//  *   INPUTS: char* write - data attempted to be written to the directory.
//  *   OUTPUTS: none
//  *   RETURN VALUE: PASS if the write operation correctly fails; FAIL if it unexpectedly succeeds.
//  *   SIDE EFFECTS: Attempts to write to a read-only filesystem, which should not change the filesystem state
//  *                 but may modify internal error handling or logging mechanisms.
//  */
// int dir_write_test(char* write){
// 	TEST_HEADER;
// 	clear();
// 	int32_t ret = dir_write(0, write, 5);
// 	if (ret == -1){
// 		printf("DIR_WRITE: Brother, I am a read-only file-system");
// 		return PASS;
// 	}
// 	else {
// 		printf("Did I allow write?");
// 		return FAIL;
// 	}
// }

// /*
//  * file_write_test
//  *   DESCRIPTION: Tests write operation on a file, which should fail due to the filesystem's read-only nature.
//  *                This test attempts to write to a file ("frame0.txt") and expects failure.
//  *   INPUTS: char* write - the data attempted to be written to the file.
//  *   OUTPUTS: none
//  *   RETURN VALUE: PASS if the file write operation fails as expected; FAIL if it unexpectedly succeeds.
//  *   SIDE EFFECTS: Tries to modify a file in a read-only filesystem, which should not affect the filesystem
//  *                 but might alter internal state or error handling mechanisms.
//  */
// int file_write_test(char* write){
// 	TEST_HEADER;
// 	clear();
// 	const uint8_t* file_name = (uint8_t *)"frame0.txt";
// 	int32_t f = file_open(file_name);
// 	if (f != -1) {
// 		int32_t ret = file_write(f, write, 5);
// 		if (ret == -1){
// 			printf("FILE_WRITE: Brother, I am a read-only file-system");
// 			return PASS;
// 		}
// 		else {
// 			printf("Did I allow write?");
// 			return FAIL;
// 		}
// 	}
// 	return FAIL;
// }

// /*
//  * clear_wait
//  *   DESCRIPTION: Clears the terminal and waits for a specified number of seconds by leveraging the RTC to
//  *                introduce a delay without busy waiting.
//  *   INPUTS: int seconds - the number of seconds to wait after clearing the screen.
//  *   OUTPUTS: none
//  *   RETURN VALUE: none
//  *   SIDE EFFECTS: Clears the terminal screen which modifies the terminal display and uses the RTC, potentially
//  *                 affecting the system's timing or interrupt handling.
//  */
// void clear_wait(int seconds) {
// 	// enable_irq(8);
// 	// int rate = 4;
// 	// rtc_write(&rate);

// 	// int i;
// 	// for(i = 0; i < seconds * rate; i++) {
// 	// 	rtc_read();
// 	// }

// 	// clear();
// 	// disable_irq(8);
// }




/* Test suite entry point */
void launch_tests(){
	disable_irq(8);
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


	/* --------------Checkpoint 2 Tests-------------- */
	

	// // RTC Test
	// rtc_frequency_test();

	// // Keyboard Test
	// keyboard_test();

	// // Test opening a VALID file
	// file_open_test_pos();

	// clear_wait(10);

	// // Test opening and INVALID file
	// file_open_test_neg();

	// clear_wait(10);

	// // Read and print the directory files
	// dir_read_test();

	// clear_wait(10);
	
	// // Read and print GREP executable
	// char file1[] = "shell";
	// file_read_test(file1);

	// clear_wait(10);

	// // Read and print fish frame text file
	// char file2[] = "frame0.txt";
	// file_read_test(file2);

	// clear_wait(10);

	// // Try to read and print a very long text file with an invalid suffix
	// char file3[] = "verylargetextwithverylongname.txt";
	// file_read_test(file3);

	// clear_wait(10);

	// // Read and print a very long text file with an valid suffix
	// char file4[] = "verylargetextwithverylongname.tx";
	// file_read_test(file4);

	// clear_wait(10);

	// // Attempt and fail to write to a file
	// char write[] = "holaAmigos";
	// file_write_test(write);

	// clear_wait(10);
	
	// // Attempt and fail to write to a directory
	// dir_write_test(write);
	
}
