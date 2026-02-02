// System bus for NES emulator
// Routes CPU memory access to: RAM, PPU registers, APU/IO, Cartridge
#ifndef BUS_H
#define BUS_H

#include "cpu_defs.h"
#include "cpu.h"
#include "ppu.h"
#include <stddef.h>

// NES internal RAM size (2KB, mirrored 4x to fill $0000-$1FFF)
#define BUS_RAM_SIZE 2048

// Bus structure - routes memory access to correct component
typedef struct bus {
    byte_t ram[BUS_RAM_SIZE];   // 2KB internal RAM ($0000-$07FF, mirrored to $1FFF)
    byte_t *prg_rom;            // Cartridge PRG ROM
    size_t prg_rom_size;        // PRG ROM size (for mirroring calculation)
    cpu_s *cpu;                  // CPU (6502 processor)
    ppu_s *ppu;                  // PPU (registers at $2000-$3FFF)
    // apu_s apu;               // APU (future: $4000-$4017)

    // OAM DMA state
    bool oam_dma_active;        // DMA transfer in progress
    byte_t oam_dma_page;        // Page to read from ($XX00-$XXFF)
    uint16_t oam_dma_cycles;    // Cycles remaining in DMA
} bus_s;

// Initialize the bus (zeroes RAM, NULLs components)
void bus_init(bus_s *bus);

// Read a byte from the bus
// Routes address to correct component based on memory map
byte_t bus_read(bus_s *bus, word_t addr);

// Write a byte to the bus
// Routes address to correct component based on memory map
void bus_write(bus_s *bus, word_t addr, byte_t value);

// Read a 16-bit word from the bus (little-endian: low byte at addr, high byte at addr+1)
word_t bus_read_word(bus_s *bus, word_t addr);

// Connect PRG ROM to the bus (for cartridge loading)
void bus_load_prg_rom(bus_s *bus, byte_t *prg_rom, size_t size);

// Connect CHR ROM to the PPU (for cartridge loading)
void bus_load_chr_rom(bus_s *bus, byte_t *chr_rom, size_t size);

// Set nametable mirroring mode
void bus_set_mirroring(bus_s *bus, mirroring_mode_e mode);

// OAM DMA - trigger DMA transfer to PPU OAM
// Copies 256 bytes from $XX00-$XXFF to PPU OAM
void bus_oam_dma(bus_s *bus, byte_t page);

// Tick the PPU (call 3 times per CPU cycle for NTSC)
void bus_ppu_tick(bus_s *bus);

#endif
