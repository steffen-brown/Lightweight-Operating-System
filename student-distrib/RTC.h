#include "types.h"

#ifndef _RTC_H
#define _RTC_H

// See c file for desciptions
extern void RTC_init();
extern void RTC_handler();

extern int rtc_open();
extern int rtc_write(const void* buf);
extern int rtc_read();
extern int rtc_close();

extern int print_one_test;

#endif
