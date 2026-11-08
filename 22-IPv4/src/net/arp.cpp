#include "arp.h"
#include "ethernet.h"   // eth_send, eth_receive, ETHERTYPE_ARP
#include "ne2000.h"     // ne2000_get_mac
#include <stdint.h>

// Our own IP, hardcoded by QEMU.
static const uint8_t OUR_IP[4] = {10, 0, 2, 15};

// ARP packet field values (all the multi-byte ones are big-endian).
#define ARP_HTYPE_ETHERNET 0x0001   // hardware type: Ethernet
#define ARP_PTYPE_IPV4     0x0800   // protocol type: IPv4
#define ARP_HLEN           6        // MAC length
#define ARP_PLEN           4        // IPv4 length
#define ARP_OP_REQUEST     0x0001   // operation request
#define ARP_OP_REPLY       0x0002   // operation reply

// Build the 28-byte ARP request into the buffer.
static void build_arp_request(uint8_t* buf, const uint8_t target_ip[4]) {
    uint8_t our_mac[6];
    ne2000_get_mac(our_mac);

    // Bytes 0-1: hardware type (Ethernet), big-endian.
    buf[0] = (ARP_HTYPE_ETHERNET >> 8) & 0xFF;
    buf[1] = ARP_HTYPE_ETHERNET & 0xFF;
    // Bytes 2-3: protocol type (IPv4), big-endian.
    buf[2] = (ARP_PTYPE_IPV4 >> 8) & 0xFF;
    buf[3] = ARP_PTYPE_IPV4 & 0xFF;
    // Byte 4: hardware address length (6). Byte 5: protocol address length (4).
    buf[4] = ARP_HLEN;
    buf[5] = ARP_PLEN;
    // Bytes 6-7: operation (request), big-endian.
    buf[6] = (ARP_OP_REQUEST >> 8) & 0xFF;
    buf[7] = ARP_OP_REQUEST & 0xFF;
    // Bytes 8-13: sender MAC (us).
    for (int i = 0; i < 6; i++) buf[8 + i] = our_mac[i];
    // Bytes 14-17: sender IP (us).
    for (int i = 0; i < 4; i++) buf[14 + i] = OUR_IP[i];
    // Bytes 18-23: target MAC. Unknown, so all zero, as that is what we're requesting.
    for (int i = 0; i < 6; i++) buf[18 + i] = 0x00;
    // Bytes 24-27: target IP (the one we're resolving to MAC).
    for (int i = 0; i < 4; i++) buf[24 + i] = target_ip[i];
}

bool arp_resolve(const uint8_t ip[4], uint8_t mac_out[6]) {
    // First, Build the request and broadcast it.
    uint8_t request[28];
    build_arp_request(request, ip);

    uint8_t broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    eth_send(broadcast, ETHERTYPE_ARP, request, 28);

    // Second, Poll for a reply. Try many times, since it doesn't come back instantly.
    for (int tries = 0; tries < 200000; tries++) {
        uint8_t   src_mac[6];
        uint16_t  ethertype;
        uint8_t*  payload;
        uint16_t  len = eth_receive(src_mac, &ethertype, &payload); // & means a pointer, and not a copy.

        if (len == 0) continue;                    // Nothing received yet.
        if (ethertype != ETHERTYPE_ARP) continue;   // Frame received is not an ARP frame.
        if (len < 28) continue;                     // Frame too short to be an ARP packet.

        // Check if the operation is a reply (bytes 6-7).
        uint16_t op = ((uint16_t)payload[6] << 8) | payload[7];
        if (op != ARP_OP_REPLY) continue;

        // The reply's sender IP (bytes 14-17), should be the IP we asked about.
        bool ip_matches = true;
        for (int i = 0; i < 4; i++) {
            if (payload[14 + i] != ip[i]) { ip_matches = false; break; }
        }
        if (!ip_matches) continue;

        // The answer, which is the sender MAC (bytes 8-13), which is the MAC for the IP we gave.
        for (int i = 0; i < 6; i++) mac_out[i] = payload[8 + i];
        return true;
    }

    return false;   // timed out, no reply, possibly no such IP existed.
}
