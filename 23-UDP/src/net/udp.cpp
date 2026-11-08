#include "udp.h"
#include "ipv4.h"   // ip_send, ip_receive, ip_checksum, get_ip, IP_PROTO_UDP
#include <stdint.h>

#define UDP_HEADER_LEN    8
#define PSEUDO_HEADER_LEN 12

// The max payload in one ethernet frame is 1500 bytes, minus 20-byte IPv4 header and the 8-byte UDP header.
#define UDP_MAX_PAYLOAD   1472

// The UDP checksum involves a 12-byte pseudo header, alongside the UDP header and payload.
// The pseudo-header is only for the sum, and has the following bytes:
//  0-3     source IP
//  4-7     destination IP
//  8       zero
//  9       protocol, 17 for UDP
//  10-11   UDP length, including header, big-endian
// A datagram sent to the wrong address should be ignored.
static uint16_t udp_checksum(const uint8_t src_ip[4], const uint8_t dest_ip[4], const uint8_t* udp, uint16_t udp_len) {
    static uint8_t buf[PSEUDO_HEADER_LEN + UDP_HEADER_LEN + UDP_MAX_PAYLOAD];   // buffer so we can grab data from multiple spots and access them at once.
    
    // Fill the buffer
    for (int i = 0; i < 4; i++) buf[i]     = src_ip[i];
    for (int i = 0; i < 4; i++) buf[4 + i] = dest_ip[i];
    buf[8]  = 0;
    buf[9]  = IP_PROTO_UDP;
    buf[10] = (udp_len >> 8) & 0xFF;
    buf[11] = udp_len & 0xFF;

    // also add the UDP header and payload, copying it in after the pseudo-header.
    for (uint16_t i = 0; i < udp_len; i++) buf[PSEUDO_HEADER_LEN + i] = udp[i];

    return ip_checksum(buf, PSEUDO_HEADER_LEN + udp_len);
}

bool udp_send(const uint8_t dest_ip[4], uint16_t src_port, uint16_t dest_port, const uint8_t* payload, uint16_t length) {
    if (length > UDP_MAX_PAYLOAD) return false;   // too big for one frame.

    uint8_t  datagram[UDP_HEADER_LEN + UDP_MAX_PAYLOAD]; //create a spot to create the datagram.
    uint16_t udp_len = UDP_HEADER_LEN + length;

    datagram[0] = (src_port >> 8) & 0xFF;    // Bytes 0-1, source port (ours), big-endian
    datagram[1] = src_port & 0xFF;
    datagram[2] = (dest_port >> 8) & 0xFF;   // Bytes 2-3, destination port, big-endian
    datagram[3] = dest_port & 0xFF;
    datagram[4] = (udp_len >> 8) & 0xFF;     // Bytes 4-5, length of header + payload, big-endian
    datagram[5] = udp_len & 0xFF;
    datagram[6] = 0x00;                      // Bytes 6-7, checksum. Zero before we compute it
    datagram[7] = 0x00;

    // Payload goes after the 8-byte header.
    for (uint16_t i = 0; i < length; i++) datagram[UDP_HEADER_LEN + i] = payload[i];

    // checksum the pseudo-header, header, and payload.
    uint8_t our_ip[4];
    get_ip(our_ip);
    uint16_t sum = udp_checksum(our_ip, dest_ip, datagram, udp_len);

    // In UDP a checksum of 0 means no checksum, so a result of 0 would end up getting ignored by the destination, so we instead send 0xFFFF. Both mean the same when ones-complement.
    if (sum == 0) sum = 0xFFFF;
    datagram[6] = (sum >> 8) & 0xFF;
    datagram[7] = sum & 0xFF;

    // Give the datagram to the IPv4 as UDP.
    return ip_send(dest_ip, IP_PROTO_UDP, datagram, udp_len);
}

uint16_t udp_receive(uint8_t src_ip_out[4], uint16_t* src_port_out, uint16_t our_port, uint8_t** payload_out) {
    uint8_t  src_ip[4];
    uint8_t  protocol;
    uint8_t* data;
    uint16_t len = ip_receive(src_ip, &protocol, &data); 

    if (len == 0) return 0;                       // nothing received
    if (protocol != IP_PROTO_UDP) return 0;       // not UDP
    if (len < UDP_HEADER_LEN) return 0;           // too short to have a UDP header

    // Bytes 2-3 are the destination port. Ignore datagrams meant for another port/program.
    uint16_t dest_port = ((uint16_t)data[2] << 8) | data[3];
    if (dest_port != our_port) return 0;

    // Bytes 4-5, length. Check to make sure it covers at least the header and fit inside the IP payload.
    uint16_t udp_len = ((uint16_t)data[4] << 8) | data[5];
    if (udp_len < UDP_HEADER_LEN || udp_len > len) return 0;

    // Bytes 6-7 is the checksum. 0 means the sender skipped it, so we should not check it.
    // Summing a datagram with its checksum should give 0 when ones-complement.
    // Same thing as ip_receive.
    uint16_t checksum = ((uint16_t)data[6] << 8) | data[7];
    if (checksum != 0) {
        uint8_t our_ip[4];
        get_ip(our_ip);
        if (udp_checksum(src_ip, our_ip, data, udp_len) != 0) return 0;
    }

    // return the sender IP, sender port, and payload after the header to the provided pointer.
    for (int i = 0; i < 4; i++) src_ip_out[i] = src_ip[i];
    *src_port_out = ((uint16_t)data[0] << 8) | data[1];
    *payload_out = data + UDP_HEADER_LEN;

    return udp_len - UDP_HEADER_LEN;
}