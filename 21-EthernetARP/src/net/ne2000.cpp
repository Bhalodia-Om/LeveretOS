#include "ne2000.h"
#include "pci.h"
#include "io.h"
#include "vga.h"

// --- NE2000 register offsets (from io_base) ---
#define NE_CR      0x00
#define NE_PSTART  0x01
#define NE_PSTOP   0x02
#define NE_BNRY    0x03
#define NE_RSAR0   0x08
#define NE_RSAR1   0x09
#define NE_RBCR0   0x0A
#define NE_RBCR1   0x0B
#define NE_RCR     0x0C
#define NE_TCR     0x0D
#define NE_DCR     0x0E
#define NE_ISR     0x07
#define NE_DATA    0x10
#define NE_RESET   0x1F

// --- New for lesson 20: transmit + receive registers ---
#define NE_TPSR    0x04   // page 0 write: transmit page start
#define NE_TBCR0   0x05   // page 0 write: transmit byte count, low
#define NE_TBCR1   0x06   // page 0 write: transmit byte count, high
#define NE_CURPAG  0x07   // page 1: current page (where the card writes the next rx packet)
#define NE_PHYS    0x01   // page 1: our MAC goes here (6 registers, 0x01-0x06)
#define NE_IMR     0x0F   // page 0 write: interrupt mask

// --- Command register bits ---
#define CR_STOP    0x01
#define CR_START   0x02
#define CR_RDMA_RD 0x08
#define CR_PAGE0   0x00
#define CR_PAGE1   0x40

// --- New for lesson 20 ---
#define CR_TRANS   0x04   // command: transmit the frame in the TX page now
#define CR_RDMA_WR 0x10   // remote DMA: write (command bits 3-5 = 010)

// Config-register values.
#define RCR_AB     0x04   // receive: accept broadcast packets
#define TCR_LOOP   0x02   // transmit: internal loopback (used for our test)
#define TCR_NORMAL 0x00   // transmit: normal operation
#define ISR_RDC    0x40   // ISR bit: remote DMA complete

// Card RAM layout (256-byte pages).
#define TX_PAGE    0x40   // transmit buffer starts here
#define RX_START   0x46   // receive ring first page
#define RX_STOP    0x60   // receive ring last page + 1

static uint16_t io_base = 0;
static uint8_t  mac[6] = {0};

static void ne2000_reset() {
    uint8_t v = inb(io_base + NE_RESET);
    outb(io_base + NE_RESET, v);
    while ((inb(io_base + NE_ISR) & 0x80) == 0) { }
    outb(io_base + NE_ISR, 0xFF);
}

bool ne2000_init() {
    PciDevice dev = pci_find_device(0x10EC, 0x8029);
    if (!dev.found) {
        print("NE2000: card not found\n");
        return false;
    }

    uint32_t bar0 = pci_config_read(dev.bus, dev.device, 0, 0x10);
    io_base = (uint16_t)(bar0 & 0xFFFFFFFC);

    ne2000_reset();

    outb(io_base + NE_CR, CR_STOP | CR_PAGE0);
    outb(io_base + NE_DCR, 0x48);

    outb(io_base + NE_RBCR0, 12);
    outb(io_base + NE_RBCR1, 0);
    outb(io_base + NE_RSAR0, 0);
    outb(io_base + NE_RSAR1, 0);
    outb(io_base + NE_CR, CR_RDMA_RD | CR_START);

    uint8_t prom[12];
    for (int i = 0; i < 12; i++) {
        prom[i] = inb(io_base + NE_DATA);
    }
    for (int i = 0; i < 6; i++) {
        mac[i] = prom[i * 2];
    }

    // --- New for lesson 20: set up the receive ring and start the card ---

    // Receive ring is in pages RX_START to RX_STOP. BNRY is the last page we've read (Boundary), so we start it at the first ring page, before we've read anything.
    outb(io_base + NE_PSTART, RX_START);
    outb(io_base + NE_PSTOP,  RX_STOP);
    outb(io_base + NE_BNRY,   RX_START);

    // Clear any remaining interrupt bits, and mask all interrupts.
    outb(io_base + NE_ISR, 0xFF);
    outb(io_base + NE_IMR, 0x00);

    // Put the card in monitor/loopback while we configure, so nothing happens yet.
    outb(io_base + NE_RCR, 0x20);          // monitor mode (don't receive yet)
    outb(io_base + NE_TCR, TCR_LOOP);      // loopback while we're configuring

    // Switch to page 1 to write our MAC and the current page pointer.
    outb(io_base + NE_CR, CR_STOP | CR_PAGE1);
    for (int i = 0; i < 6; i++) {
        outb(io_base + NE_PHYS + i, mac[i]);   // tell the card its MAC
    }
    outb(io_base + NE_CURPAG, RX_START + 1);   // the card writes the next packet here

    // Go back to page 0 and start the card running.
    outb(io_base + NE_CR, CR_START | CR_PAGE0);

    // Now put it in normal operating mode, aka accept broadcast and our packets, normal TX.
    outb(io_base + NE_TCR, TCR_NORMAL);
    outb(io_base + NE_RCR, RCR_AB);

    return true;
}

// This function will send one ethernet frame. First remote-DMA (write to card ram) the bytes into the TX page, and then trigger the transmit.
void ne2000_send(const uint8_t* data, uint16_t length) {
    // Ethernet has a 60-byte minimum frame (before CRC). Add padding to short frames to fill the space.
    uint16_t len = length;
    if (len < 60) len = 60;

    // First remote-DMA WRITE the frame into the card RAM at the TX page.
    // Clear the remote-DMA-complete bit first so that we can wait on it later.
    outb(io_base + NE_ISR, ISR_RDC);

    outb(io_base + NE_RBCR0, len & 0xFF);          // byte count equals len
    outb(io_base + NE_RBCR1, (len >> 8) & 0xFF);   // High byte for above.
    outb(io_base + NE_RSAR0, 0x00);                // start address = TX_PAGE << 8
    outb(io_base + NE_RSAR1, TX_PAGE);             // High byte for above.
    outb(io_base + NE_CR, CR_RDMA_WR | CR_START);  // begin remote-DMA write.

    // Push the frame bytes through the data port one by one.
    for (uint16_t i = 0; i < length; i++) {
        outb(io_base + NE_DATA, data[i]);
    }
    // If we padded, write zero bytes for the rest up to the minimum length of 60-bytes.
    for (uint16_t i = length; i < len; i++) {
        outb(io_base + NE_DATA, 0x00);        
    }

    // Wait for the card to finish the DMA (remote-DMA-complete bit in ISR).
    while ((inb(io_base + NE_ISR) & ISR_RDC) == 0) { }
    outb(io_base + NE_ISR, ISR_RDC);   // clear it

    // Step 2 is to actually trigger the transmit.
    outb(io_base + NE_TPSR, TX_PAGE);              // frame is in the TX page
    outb(io_base + NE_TBCR0, len & 0xFF);          // send byte count
    outb(io_base + NE_TBCR1, (len >> 8) & 0xFF);
    outb(io_base + NE_CR, CR_TRANS | CR_START);    // set the transmit bit, after which the card sends it
}

// Attempt to recieve a frame. Returns the length of the packet or 0 in no packet is awaiting. The buffer must be big enough for the packet.
uint16_t ne2000_receive(uint8_t* buf) {
    // Read the card's current page, where it will write to next, from page 1.
    outb(io_base + NE_CR, CR_PAGE1 | CR_START);
    uint8_t current = inb(io_base + NE_CURPAG);
    outb(io_base + NE_CR, CR_PAGE0 | CR_START);   // Go back to page 0

    // BNRY (Boundary) is the last page we read. So the next unread page is BNRY + 1, and wrapping around the ring. If the next page is the same as current, nothing has arrived.
    uint8_t boundary = inb(io_base + NE_BNRY);
    uint8_t next = boundary + 1;
    if (next >= RX_STOP) next = RX_START;   // wrap around
    if (next == current) return 0;          // ring is empty    

    // Read the 4-byte packet header the card put in front of the packet, with remote DMA. It sits at the start of the next page.
    outb(io_base + NE_ISR, ISR_RDC);
    outb(io_base + NE_RBCR0, 4);
    outb(io_base + NE_RBCR1, 0);
    outb(io_base + NE_RSAR0, 0);
    outb(io_base + NE_RSAR1, next);
    outb(io_base + NE_CR, CR_RDMA_RD | CR_START);

    uint8_t  status    = inb(io_base + NE_DATA);   // byte 0, receive status
    uint8_t  next_page = inb(io_base + NE_DATA);   // byte 1, next packet's page
    uint8_t  len_lo    = inb(io_base + NE_DATA);   // byte 2, length low
    uint8_t  len_hi    = inb(io_base + NE_DATA);   // byte 3, length high
    (void)status;

    // The total length includes the 4-byte header, so the packet body is length - 4.
    uint16_t total_len = (uint16_t)len_lo | ((uint16_t)len_hi << 8);
    if (total_len < 4) { return 0; }               // Return if the length is just header.
    uint16_t data_len = total_len - 4;

    // Read the packet body with remote DMA, starting with the 4 bytes in the next page.
    outb(io_base + NE_ISR, ISR_RDC);
    outb(io_base + NE_RBCR0, data_len & 0xFF);
    outb(io_base + NE_RBCR1, (data_len >> 8) & 0xFF);
    outb(io_base + NE_RSAR0, 4);                    // skip the header
    outb(io_base + NE_RSAR1, next);
    outb(io_base + NE_CR, CR_RDMA_RD | CR_START);

    for (uint16_t i = 0; i < data_len; i++) {
        buf[i] = inb(io_base + NE_DATA);
    }

    // Advance BNRY to be able to reuse the previous page. The new boundary is the page before next_page, because next_page is where the next packet starts.
    uint8_t new_bnry = next_page - 1;
    if (new_bnry < RX_START) new_bnry = RX_STOP - 1;   // wrap
    outb(io_base + NE_BNRY, new_bnry);

    return data_len;
}

// Switch the card between internal loopback and normal operation. Used for the self-test in the shell.
void ne2000_set_loopback(bool on) {
    outb(io_base + NE_TCR, on ? TCR_LOOP : TCR_NORMAL);
}

void ne2000_get_mac(uint8_t mac_out[6]) {
    for (int i = 0; i < 6; i++) {
        mac_out[i] = mac[i];
    }
}
