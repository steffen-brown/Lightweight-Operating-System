#include "types.h"

#ifndef _RTC_H
#define _RTC_H

// See c file for desciptions
extern void RTC_init();
extern void RTC_handler();

extern int rtc_open(const uint8_t* filename);
extern int rtc_write(int32_t fd, const void* buf, int32_t nbytes);
extern int rtc_read(int32_t fd, void* buf, int32_t nbytes);
extern int rtc_close(int32_t fd);

#endif
