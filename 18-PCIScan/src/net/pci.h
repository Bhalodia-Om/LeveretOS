#ifndef PCI_H
#define PCI_H

#include <stdint.h>

// The PCI bus connects hardware to the system. Each device has a configuration space we read through two I/O ports. 0xCF8 for address and 0xCFC for data.
// This module will read the space and list all the found devices.

// Read one 32-bit word from a device config space. bus, device, function locate the device, while offset picks which field to read from.
uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

// Take a wild guess what this does.
void pci_scan();

#endif // PCI_H