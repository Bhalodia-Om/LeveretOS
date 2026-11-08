#include "ipv4.h"
#include "ethernet.h"   // eth_send, eth_receive, ETHERTYPE_IPV4
#include "arp.h"        // arp_resolve
#include <stdint.h>

#define IP_HEADER_LEN 20

// Set our own IP and the gateway IP, hardcoded by QEMU.
static const uint8_t OUR_IP[4]     = {10, 0, 2, 15};
static const uint8_t GATEWAY_IP[4] = {10, 0, 2, 2};

// A counter for the identification field in the header, which must differ between packets. So we'll just add one for every header we build.
static uint16_t ip_id_counter = 0;

// Internal buffer for packets received, so ip_receive can hand a pointer instead of allocating ram every time.
static uint8_t rx_packet[1600];

// Checksum for IPv4. We treat the header as a row of 16-bit big-endian words and add them into a
// 32-bit sum. The sum can carry above 16 bits, so we fold the top half back into the bottom half
// until nothing is left to carry. We then flip every bit (ones-complement) to get the 16-bit result.
static uint16_t ip_checksum(const uint8_t* header, int len) {
    uint32_t sum = 0;

    // Add each 16-bit word. Byte i is the high half, byte i+1 is the low half (big-endian).
    for (int i = 0; i < len; i += 2) {
        uint16_t word = ((uint16_t)header[i] << 8) | header[i + 1];
        sum += word;
    }

    // Fold whatever ran over 16 bits.
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // Ones-complement, or flip every bit of the folded 16-bit sum.
    return (uint16_t)(~sum);
}

// Build the 20-byte header in place for a packet.
static void build_ip_header(uint8_t* h, const uint8_t dest_ip[4], uint8_t protocol, uint16_t payload_len) {
    uint16_t total_len = IP_HEADER_LEN + payload_len;
    uint16_t id = ip_id_counter++;
   
    h[0] = 0x45;                    // Byte 0, version in the high nibble, header length in the low. Length counts 32-bit words, so 5 means 20 bytes.
    h[1] = 0x00;                    // Byte 1, DSCP (Differentiated Services Code Point) / ECN (Explicit Congestion Notification). We aren't using these, so zero.
    h[2] = (total_len >> 8) & 0xFF; // Bytes 2-3, total length (header + payload), big-endian.
    h[3] = total_len & 0xFF;        
    h[4] = (id >> 8) & 0xFF;        // Bytes 4-5, identification, also big-endian. Different value per packet sent from our computer.
    h[5] = id & 0xFF;
    h[6] = 0x00;                    // Bytes 6-7, flags and fragment offset. We never fragment, so we leave this as zero.
    h[7] = 0x00;
    h[8] = 64;                      // Byte 8, time to live. Represents the number of router hops our packet can do.
    h[9] = protocol;                // Byte 9, protocol, saying what is in the payload.
    h[10] = 0x00;                   // Bytes 10-11, header checksum. Must be zero while we compute it, then filled afterwards.
    h[11] = 0x00;

    for (int i = 0; i < 4; i++) h[12 + i] = OUR_IP[i];      // Bytes 12-15, source IP (us).
    for (int i = 0; i < 4; i++) h[16 + i] = dest_ip[i];     // Bytes 16-19, destination IP.

    // Every other field is set and the checksum field is zero, so compute and store the checksum now.
    uint16_t sum = ip_checksum(h, IP_HEADER_LEN);
    h[10] = (sum >> 8) & 0xFF;
    h[11] = sum & 0xFF;
}

bool ip_send(const uint8_t dest_ip[4], uint8_t protocol, const uint8_t* payload, uint16_t length) {
    // In QEMU user-net, all our packets leave through the gateway's MAC. With ARP we can get the MAC from IP. Ideally we should check if dest_ip is on our sub network first.
    // As we are using an emulator where we are the only device on the network, this is unneeded.
    uint8_t gateway_mac[6];
    if (!arp_resolve(GATEWAY_IP, gateway_mac)) {
        return false;   // could not find the gateway, so we cannot send our packet.
    }

    // Build the packet, with the 20 byte header first.
    uint8_t packet[1600];
    build_ip_header(packet, dest_ip, protocol, length);
    for (uint16_t i = 0; i < length; i++) {
        packet[IP_HEADER_LEN + i] = payload[i];
    }

    // Give the IPv4 packet to the Ethernet layer.
    eth_send(gateway_mac, ETHERTYPE_IPV4, packet, IP_HEADER_LEN + length);
    return true;
}

uint16_t ip_receive(uint8_t src_ip_out[4], uint8_t* protocol_out, uint8_t** payload_out) {
    uint8_t   src_mac[6];
    uint16_t  ethertype;
    uint8_t*  frame_payload;
    uint16_t  len = eth_receive(src_mac, &ethertype, &frame_payload);

    if (len == 0) return 0;                       // nothing received.
    if (ethertype != ETHERTYPE_IPV4) return 0;    // not an IPv4 frame.
    if (len < IP_HEADER_LEN) return 0;            // too short to have a header, let alone a payload.

    // Copy the IPv4 packet into our own buffer so the pointer we return stays accessible.
    for (uint16_t i = 0; i < len; i++) rx_packet[i] = frame_payload[i];

    // Verify the checksum to check for any corruption. Same as before, but includes the checksum field. Should fold to 0xFFFF, ones-complement is 0. Any other result means the header is corrupt, so don't receive packet.
    if (ip_checksum(rx_packet, IP_HEADER_LEN) != 0) return 0;

    // The low nibble of byte 0 is the length of the header in 32-bit sections. We use this instead of assuming 20 in case there is more data sent.
    uint8_t header_len = (rx_packet[0] & 0x0F) * 4;
    if (header_len < IP_HEADER_LEN || header_len > len) return 0;

    // The destination IP is bytes 16-19. Ignore the packet if it wasn't meant for our system.
    for (int i = 0; i < 4; i++) {
        if (rx_packet[16 + i] != OUR_IP[i]) return 0;
    }

    // Hand the caller the sender IP (12-15), protocol (9), and the payload, which is past the header.
    for (int i = 0; i < 4; i++) src_ip_out[i] = rx_packet[12 + i];
    *protocol_out = rx_packet[9];
    *payload_out = &rx_packet[header_len];

    // The payload length is the total length field minus the length of our header.
    uint16_t total_len = ((uint16_t)rx_packet[2] << 8) | rx_packet[3];
    if (total_len > len || total_len < header_len) return 0;
    return total_len - header_len;
}