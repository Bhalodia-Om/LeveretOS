#include "ethernet.h"
#include "ne2000.h"   // ne2000_send, ne2000_receive, ne2000_get_mac
#include <stdint.h>

// The Ethernet header is 14 bytes, 6 dest + 6 source + 2 type.
#define ETH_HEADER_LEN 14

// Internal buffer that we will reuse for receiving frames. so eth_receive can hand the caller a pointer into it without allocation ram. Big enough for a max sized Ethernet Frame.
static uint8_t rx_frame[1600];

void eth_send(const uint8_t dest_mac[6], uint16_t ethertype, const uint8_t* payload, uint16_t length) {
    uint8_t frame[1600];

    // Bytes 0-5, destination MAC.
    for (int i = 0; i < 6; i++) frame[i] = dest_mac[i];

    // Bytes 6-11, with our own MAC as the source.
    uint8_t our_mac[6];
    ne2000_get_mac(our_mac);
    for (int i = 0; i < 6; i++) frame[6 + i] = our_mac[i];

    // bytes 12-13 are the EtherType, written with the high byte first (big-endian).
    frame[12] = (ethertype >> 8) & 0xFF;
    frame[13] = ethertype & 0xFF;

    // Bytes 14+ are the payload, the data being transmitted.
    for (uint16_t i = 0; i < length; i++) {
        frame[ETH_HEADER_LEN + i] = payload[i];
    }

    // Give the whole frame (header + payload) to the driver.
    ne2000_send(frame, ETH_HEADER_LEN + length);
}

uint16_t eth_receive(uint8_t src_mac_out[6], uint16_t* ethertype_out, uint8_t** payload_out) {
    // Ask the driver to receive a frame.
    uint16_t total = ne2000_receive(rx_frame);
    if (total < ETH_HEADER_LEN) return 0;   // nothing, or too short to be a valid frame (shorter than the header)

    // Get the source MAC out of bytes 6-11.
    for (int i = 0; i < 6; i++) src_mac_out[i] = rx_frame[6 + i];

    // Reassemble the big-endian EtherType with bytes 12-13.
    *ethertype_out = ((uint16_t)rx_frame[12] << 8) | rx_frame[13];

    // Point the caller at the payload, which is just past the 14-byte header.
    *payload_out = &rx_frame[ETH_HEADER_LEN];

    // Return count of payload bytes.
    return total - ETH_HEADER_LEN;
}