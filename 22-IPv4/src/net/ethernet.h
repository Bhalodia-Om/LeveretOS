#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>

// A layer over the driver, which will be used to build the ethernet frame, by providing a destination, type, and a payload.

// Common EtherType values / protocols. Comes first as big-endian
#define ETHERTYPE_ARP  0x0806
#define ETHERTYPE_IPV4 0x0800

// Build an ethernet frame and send it.
//  dest_mac    : 6-byte destination MAC. For broadcast us 0xFF.
//  ethertype   : which protocol is being used.
//  payload     : the bytes after the header, pointer.
//  length      : how many bytes of payload.
void eth_send(const uint8_t dest_mac[6], uint16_t ethertype, const uint8_t* payload, uint16_t length);

// Function to try to receive one frame. If one has arrived, we fill in the source MAC and ethertype for whatever called the function.
// Also points the payload_out at the beginning of the payload, which is inside an internal buffer, and returns the length of the payload. 0 is returned if no frame.
uint16_t eth_receive(uint8_t src_mac_out[6], uint16_t* ethertype_out, uint8_t** payload_out);

#endif // ETHERNET_H
