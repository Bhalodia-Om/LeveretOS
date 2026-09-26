#ifndef NE2000_H
#define NE2000_H

#include <stdint.h>

// Driver for the NE2000 (Realtek RTL8029) network card.
// The card is controlled through I/O ports, and it has its own internal RAM. We can reach it with remove DMA.

// Find the card on the PCI bus, get its I/O base port, reset it, and read the MAC address out of it. Returns true if the card was found and set up.
bool ne2000_init();

// Copy the 6-byte MAC address into the caller's buffer. Must call ne2000_init first.
void ne2000_get_mac(uint8_t mac_out[6]);

#endif // NE2000_H
