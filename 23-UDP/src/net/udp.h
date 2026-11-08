#ifndef UDP_H
#define UDP_H

#include <stdint.h>

// layer over IPv4 used for sending UDP (User Datagram Protocol) datagrams.
// UDP includes an 8-byte header with a source and destination port. The IP address is the machine, while the port is which program on that machine.
// UDP doesn't check if data arrived or in the correct order, instead just sending for simplicity and speed.

// Send one UDP datagram.
//  - dest_ip       4-byte destination IP
//  - src_port      our port to receive the reply
//  - dest_port     port of the program we are sending to
//  - payload       the bytes after the 8-byte UDP header
//  - length        number of payload bytes
// Returns true if datagram given to the IPv4 layer successfully.
bool udp_send(const uint8_t dest_ip[4], uint16_t src_port, uint16_t dest_port, const uint8_t* payload, uint16_t length);

// Try to receive a UDP datagram that is sent to our port.
//  - src_ip_out        4-byte sender IP
//  - src_port_out      filled with the sender's port
//  - our_port          only datagrams sent to this port are accepted
//  - payload_out       pointed at the data just after the UDP header, inside the received packet
//  returns the number of payload bytes, or 0 if nothing valid came through.

uint16_t udp_receive(uint8_t src_ip_out[4], uint16_t* src_port_out, uint16_t our_port,
                     uint8_t** payload_out);

#endif // UDP_H