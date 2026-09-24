#include "pci.h"
#include "io.h"     // inl / outl
#include "vga.h"    // print, print_int, putchar

// Our two PCI ports.
#define PCI_ADDRESS_PORT 0xCF8
#define PCI_DATA_PORT    0xCFC

// We need to make the 32-bit address, and also read the data port.
uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    // bit 31 = enable, must be 1 for reading.
    // bit 30-24 are reserved, must be 0.
    // bit 23-16 = bus
    // bit 15-11 = device
    // bit 10-8 = function
    // bit 7-2 = config offset, to get the info we actually want.
    // bit 1-0 = 0, for alignment.
    uint32_t address = (uint32_t)((1u << 31) | ((uint32_t)bus << 16) | ((uint32_t)device << 11) | ((uint32_t)function << 8) | ((uint32_t)offset & 0xFC));

    outl(PCI_ADDRESS_PORT, address);
    return inl(PCI_DATA_PORT);
}

// Print a 16-bit number as 4 hex digits (0x1234), so that we can read the IDs
static void print_hex16(uint16_t value) {
    const char* digits = "0123456789ABCDEF";
    print("0x");
    putchar(digits[(value >> 12) & 0xF]);   // top nibble
    putchar(digits[(value >> 8)  & 0xF]);
    putchar(digits[(value >> 4)  & 0xF]);
    putchar(digits[value & 0xF]);           // bottom nibble
}

// Go through the bus and print every device that is present.

void pci_scan() {
    print("Scanning PCI bus:\n");
    // bus is 0-255, device 0-31. We only check function 0 for now, but for multi-function devices, we would need to loop function 0-7.
    for (int bus = 0; bus < 256; bus++) {
        for (int device = 0; device < 32; device++) {
            // vendor ID is low 16 bits, device ID is high 16 bits.
            uint32_t id = pci_config_read(bus, device, 0, 0);
            uint16_t vendor = id & 0xFFFF;
            uint16_t devid  = (id >> 16) & 0xFFFF;

            if (vendor == 0xFFFF) continue; //0xFFFF means nothing plugged in.

            //print the info for the user.
            print("  bus ");
            print_int(bus);
            print(" dev ");
            print_int(device);
            print(": vendor ");
            print_hex16(vendor);
            print(" device ");
            print_hex16(devid);

            // Call out our network card specifically, with its vendor and device id.
            if (vendor == 0x10EC && devid == 0x8029) {
                print("  <- network card (NE2000/RTL8029)");
            }
            putchar('\n');
        }
    }
    print("PCI scan done.\n");

}