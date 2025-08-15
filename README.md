# Lightweight Operating System

[![UIUC ECE 391](https://img.shields.io/badge/Course-ECE%20391-orange)](https://ece.illinois.edu/)

## Description
An x86-based lightweight operating system developed from scratch in **C** and **x86 assembly**, built to run in a QEMU virtualized environment.  
The OS directly interfaces with hardware, implementing a custom scheduler, memory management, interrupt handling, and essential device drivers — all without external libraries.  

This project was completed as part of an academic course to gain low-level systems programming experience, working from bootloader to process scheduling.

---

## Features
- **Custom Bootloader** written in x86 assembly (`boot.S`)
- **Program Execution** and basic multitasking support
- **Memory Management** with paging (`paging.c`, `paging.h`)
- **Interrupt Handling** with programmable interrupt controller (PIC) support (`i8259.c`, `interrupts.c`)
- **Device Drivers**:
  - Real-time clock (`RTC.c`)
  - Keyboard input (`keyboard.c`)
  - File system driver (`file_sys.c`)
- **System Calls** for user programs (`sys_calls.c`, `sys_calls_handler.S`)
- **Custom Shell / Test Suite** (`tests.c`)
- **No external libraries** — all code implemented from scratch

---

## Directory Structure
Key files in `/src`:

- **Boot and Initialization**
  - `boot.S` — Bootloader and entry point
  - `multiboot.h` — Multiboot header definitions
- **Core OS**
  - `kernel.c` — Kernel main routines
  - `lib.c`, `lib.h` — Basic C library functions (implemented manually)
  - `types.h` — Common type definitions
- **Memory Management**
  - `paging.c`, `paging.h` — Virtual memory paging
- **Interrupts**
  - `i8259.c`, `i8259.h` — PIC programming
  - `interrupts.c`, `interrupts.h` — Interrupt setup and handling
  - `x86_desc.S`, `x86_desc.h` — Descriptor tables
- **Drivers**
  - `RTC.c`, `RTC.h` — Real-time clock
  - `keyboard.c`, `keyboard.h` — Keyboard input
  - `file_sys.c`, `file_sys.h` — File system interface
  - `pit.c`, `pit.h` — Programmable Interval Timer
- **System Calls**
  - `sys_calls.c`, `sys_calls.h` — System call implementations
  - `sys_calls_handler.S` — Assembly linkage for syscalls
- **Testing**
  - `tests.c`, `tests.h` — OS feature test functions

---

## Requirements
- **QEMU** (for running the OS)  
- **GCC** (cross-compiler for i386)  
- **Make**  

On Ubuntu/Debian:
```bash
sudo apt update
sudo apt install qemu-system-i386 build-essential
