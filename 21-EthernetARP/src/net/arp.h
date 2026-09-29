#ifndef ARP_H
#define ARP_H

#include <stdint.h>

// ARP gets a MAC address from an IP address over the local network. This will send a request for an IP, and wait for a reply, and then return the mac address.

// We will resolve the IPv4 address given to a MAC address.
//      ip      : 4-byte target IP for the QEMU gateway, e.g. 10.0.2.2
//      mac_out : will be filled with the 6-byte MAC if we get a reply.
// We will return true if a reply arrived, and false upon timing out.
bool arp_resolve(const uint8_t ip[4], uint8_t mac_out[6]);

#endif // ARP_H