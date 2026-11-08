#include "pci.h"
#include "io.h"
#include "vga.h"

#define PCI_ADDRESS_PORT 0xCF8
#define PCI_DATA_PORT    0xCFC

uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (uint32_t)((1u << 31) | ((uint32_t)bus << 16) | ((uint32_t)device << 11) | ((uint32_t)function << 8) | ((uint32_t)offset & 0xFC));

    outl(PCI_ADDRESS_PORT, address);
    return inl(PCI_DATA_PORT);
}

static void print_hex16(uint16_t value) {
    const char* digits = "0123456789ABCDEF";
    print("0x");
    putchar(digits[(value >> 12) & 0xF]);
    putchar(digits[(value >> 8)  & 0xF]);
    putchar(digits[(value >> 4)  & 0xF]);
    putchar(digits[value & 0xF]);
}

void pci_scan() {
    print("Scanning PCI bus:\n");
    for (int bus = 0; bus < 256; bus++) {
        for (int device = 0; device < 32; device++) {
            uint32_t id = pci_config_read(bus, device, 0, 0);
            uint16_t vendor = id & 0xFFFF;
            uint16_t devid  = (id >> 16) & 0xFFFF;

            if (vendor == 0xFFFF) continue;

            print("  bus ");
            print_int(bus);
            print(" dev ");
            print_int(device);
            print(": vendor ");
            print_hex16(vendor);
            print(" device ");
            print_hex16(devid);

            if (vendor == 0x10EC && devid == 0x8029) {
                print("  <- network card (NE2000/RTL8029)");
            }
            putchar('\n');
        }
    }
    print("PCI scan done.\n");
}

// Look for a specific card using vendor + device ID. Same as pci_scan, but stop and return as soon as we find a match. Uses same logic as pci_scan, which has comments in lesson 18.
PciDevice pci_find_device(uint16_t vendor_id, uint16_t device_id) {
    for (int bus = 0; bus < 256; bus++) {
        for (int device = 0; device < 32; device++) {
            uint32_t id = pci_config_read(bus, device, 0, 0);
            uint16_t vendor = id & 0xFFFF;
            uint16_t dev_id = (id >> 16) & 0xFFFF;
            if (vendor == vendor_id && dev_id == device_id) {
                PciDevice result = { true, (uint8_t)bus, (uint8_t)device };
                return result;
            }
        }
    }
    PciDevice notfound = { false, 0, 0 };   // nothing matched
    return notfound;
}
