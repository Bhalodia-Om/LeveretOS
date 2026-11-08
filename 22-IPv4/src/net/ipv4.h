#ifndef IPV4_H
#define IPV4_H

#include <stdint.h>

// A layer that will go over Ethernet to parse our data into IPv4 packets.
// IPv4 is the protocol that we will use to carry packets from one IP address to another. We build the 20 byte header, and add a checksum to detect corruption.
// We then use the Ethernet layer to send the packet. Multi-byte fields in the header are big-endian.

// Protocol numbers that go in the header's protocol field, which say what is in the payload.
#define IP_PROTO_ICMP 1  // Internet Control Message Protocol, for error messages and other operational information.
#define IP_PROTO_TCP  6  // Transmission Control Protocol, used to ensure that the data is delivered reliably, accurately, and in the right order. More Accurate.
#define IP_PROTO_UDP  17 // User Datagram Protocol, used for data that isn't checked for whether it has arrived or if it is in the right order. Faster.

// Build an IPv4 packet and send it:
//   dest_ip  : 4-byte destination IP, e.g. { 10, 0, 2, 2 }.
//   protocol : what IP_PROTO to use, telling what data is in the payload.
//   payload  : the bytes after the 20-byte header.
//   length   : length of payload, in bytes.

// We find the gateway's MAC with ARP, put the header before the payload, then send it. Returns true if the frame was handed off.
bool ip_send(const uint8_t dest_ip[4], uint8_t protocol, const uint8_t* payload, uint16_t length);

// Attempt to receive an IPv4 packet that is addressed to us.
//   src_ip_out    : the 4-byte sender IP.
//   protocol_out  : the protocol number from the header.
//   payload_out   : pointed at the payload inside an internal buffer.
// Returns the number of payload bytes, or 0 if nothing valid arrived.
uint16_t ip_receive(uint8_t src_ip_out[4], uint8_t* protocol_out, uint8_t** payload_out);

#endif // IPV4_H