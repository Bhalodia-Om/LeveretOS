#include "rtc.h"
#include "io.h"    // inb, outb

// The RTC is accessed through two ports, 0x70, which selects the field we want, and 0x71, which reads, or writes, that fields value. Each field has a register number.
#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

// Read one RTC register. First write which register we want to port 0x70, then read from 0x71.
static uint8_t read_register(uint8_t reg) {
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

// The RTC returns values in BCD, or binary-coded decimal. Each nibble is one decimal digit, so 0x42 means the number 42 and not 66, like in binary. This converts the BCD to a normal number.
static uint8_t bcd_to_binary(uint8_t value) {
    return (value & 0x0F) + ((value >> 4) * 10); // Get the ones and tens digit nibbles seperately.
}

RtcTime rtc_read() {
    RtcTime t;

    // Register numbers for each field, fixed by the RTC hardware.
    t.second = read_register(0x00);
    t.minute = read_register(0x02);
    t.hour   = read_register(0x04);
    t.day    = read_register(0x07);
    t.month  = read_register(0x08);
    t.year   = read_register(0x09);

    // Register 0x0B, bit 2 tells us whether the values are binary or BCD. If it's BCD which is the norm, convert each field. Bit 1 tells us 12h vs 24h, but we'll assume 24h.
    uint8_t status = read_register(0x0B);
    bool is_bcd = !(status & 0x04);   // bit 2 equals 0 means values are in BCD

    if (is_bcd) {
        t.second = bcd_to_binary(t.second);
        t.minute = bcd_to_binary(t.minute);
        t.hour   = bcd_to_binary(t.hour);
        t.day    = bcd_to_binary(t.day);
        t.month  = bcd_to_binary(t.month);
        t.year   = bcd_to_binary(t.year);
    }

    return t;
}