#include "ne2000.h"
#include "pci.h"    // pci_find_device, pci_config_read
#include "io.h"     // inb, outb
#include "vga.h"    // print


// --- NE2000 register offsets, added to the card's base I/O port ---
// Page 0 registers, aka the default page. The command register is on every page.

#define NE_CR      0x00   // Command Register (control + which page is selected)
#define NE_PSTART  0x01   // page 0 write: receive ring start page
#define NE_PSTOP   0x02   // page 0 write: receive ring stop page
#define NE_BNRY    0x03   // page 0: boundary page (last page we've read)
#define NE_RSAR0   0x08   // remote DMA start address, low byte
#define NE_RSAR1   0x09   // remote DMA start address, high byte
#define NE_RBCR0   0x0A   // remote DMA byte count, low byte
#define NE_RBCR1   0x0B   // remote DMA byte count, high byte
#define NE_RCR     0x0C   // Receive Configuration Register
#define NE_TCR     0x0D   // Transmit Configuration Register
#define NE_DCR     0x0E   // Data Configuration Register
#define NE_ISR     0x07   // Interrupt Status Register
#define NE_DATA    0x10   // the remote DMA data port window into card RAM
#define NE_RESET   0x1F   // read then write this to reset the card

// Command Register (CR) bit meanings: 
#define CR_STOP    0x01   // stop the card
#define CR_START   0x02   // start the card
#define CR_RDMA_RD 0x08   // remote DMA: read (command bits 3-5 = 001)
#define CR_PAGE0   0x00   // select register page 0 (bits 6-7 = 00)
#define CR_PAGE1   0x40   // select register page 1 (bits 6-7 = 01)

// Where the driver will keep card information.
static uint16_t io_base = 0;      // the card's base I/O port (from PCI BAR0)
static uint8_t  mac[6] = {0};     // the card's MAC address

// Reset the card. When we read the reset register it will return the value we write. We also need to wait for it to be finished.
static void ne2000_reset() {
    uint8_t v = inb(io_base + NE_RESET);
    outb(io_base + NE_RESET, v);
    // ISR bit 7 (0x80) = "reset complete". Spin until the card sets it.
    while ((inb(io_base + NE_ISR) & 0x80) == 0) {/*Wait for interrupt status register to return no interrupts, such as from resetting.*/ }
    outb(io_base + NE_ISR, 0xFF);   // clear all interrupt-status bits
}

bool ne2000_init() {
    // First, locate the card on the PCI bus.
    PciDevice dev = pci_find_device(0x10EC, 0x8029);
    if (!dev.found) {
        print("NE2000: card not found\n");
        return false;
    }

    // Second, read BAR0, or the base port number (The first port used). For an I/O BAR the low bit is a flag so we can clear the low 2 bits, for a clean base port number.
    uint32_t bar0 = pci_config_read(dev.bus, dev.device, 0, 0x10);
    io_base = (uint16_t)(bar0 & 0xFFFFFFFC);

    // Third, reset the card into a known state.
    ne2000_reset();

    // Fourth, stop the card and set byte-wide DMA (Allows direct access to card memory, no need to read and write to our CPU) before touching its RAM.
    outb(io_base + NE_CR, CR_STOP | CR_PAGE0);
    outb(io_base + NE_DCR, 0x48);   // byte-wide DMA (bit 0 clear gives 8-bit access)

    // Fifth, Read the MAC out of the card's ROM via DMA.
    //      The MAC (Identifier of hardware) is at the very start of the card RAM, aka address 0. In the NE2000 the 6 MAC bytes are spread out. Byte i is at offset i*2.
    //      We grab 12 bytes starting at address 0, read them through the data port, and keep every other one.
    outb(io_base + NE_RBCR0, 12);   // count = 12 bytes
    outb(io_base + NE_RBCR1, 0);
    outb(io_base + NE_RSAR0, 0);    // start address = 0
    outb(io_base + NE_RSAR1, 0);
    outb(io_base + NE_CR, CR_RDMA_RD | CR_START);   // start a DMA read

    uint8_t prom[12];
    for (int i = 0; i < 12; i++) {
        prom[i] = inb(io_base + NE_DATA);   // pull one byte from the data port
    }
    for (int i = 0; i < 6; i++) {
        mac[i] = prom[i * 2];   // keep every other byte, which is the real MAC
    }

    return true;
}

void ne2000_get_mac(uint8_t mac_out[6]) {
    for (int i = 0; i < 6; i++) {
        mac_out[i] = mac[i];
    }
}
