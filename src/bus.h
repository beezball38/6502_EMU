// System bus for NES emulator
// Routes CPU memory access to: RAM, PPU registers, APU/IO, Cartridge
#ifndef BUS_H
#define BUS_H

#include "cpu_defs.h"
#include <stddef.h>

// NES internal RAM size (2KB, mirrored 4x to fill $0000-$1FFF)
#define BUS_RAM_SIZE 2048

// Bus structure - routes memory access to correct component
typedef struct bus {
    byte_t ram[BUS_RAM_SIZE];   // 2KB internal RAM ($0000-$07FF, mirrored to $1FFF)
    byte_t *prg_rom;            // Cartridge PRG ROM
    size_t prg_rom_size;        // PRG ROM size (for mirroring calculation)
    // ppu_s *ppu;              // PPU (future: registers at $2000-$3FFF)
    // apu_s *apu;              // APU (future: $4000-$4017)
} bus_s;

// Initialize the bus (zeroes RAM, NULLs components)
void bus_init(bus_s *bus);

// Read a byte from the bus
// Routes address to correct component based on memory map
byte_t bus_read(bus_s *bus, word_t addr);

// Write a byte to the bus
// Routes address to correct component based on memory map
void bus_write(bus_s *bus, word_t addr, byte_t value);

// Connect PRG ROM to the bus (for cartridge loading)
void bus_load_prg_rom(bus_s *bus, byte_t *prg_rom, size_t size);

#endif
