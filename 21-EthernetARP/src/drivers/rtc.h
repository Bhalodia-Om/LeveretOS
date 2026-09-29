#ifndef RTC_H
#define RTC_H

#include <stdint.h>
// RTC stands for Real-Time Clock!

// A simple struct to hold a moment in time read from the RTC.
struct RtcTime {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t year;    // only the last two digits, like 25 for 2025
};

// Read the current time and date from the RTC (Real-Time Clock) chip.
RtcTime rtc_read();

#endif // RTC_H
