#ifndef PCI_H
#define PCI_H

#include <stdint.h>

// Struct for storing a PCI device location. Also contains whether it was located.
struct PciDevice {
    bool     found;    // false = no matching device on the bus.
    uint8_t  bus;
    uint8_t  device;
};
// Search the bus for a device with a certain vendor and device ID.
// Returns location, or found = false if not found.
PciDevice pci_find_device(uint16_t vendor, uint16_t device_id);

uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

void pci_scan();

#endif // PCI_H
