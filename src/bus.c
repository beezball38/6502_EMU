// System bus implementation for NES emulator
// Memory map:
//   $0000-$1FFF: Internal RAM (2KB, mirrored 4x)
//   $2000-$3FFF: PPU registers (8 bytes, mirrored every 8 bytes) - stub for now
//   $4000-$4017: APU and I/O registers (stub for now)
//   $4018-$401F: APU and I/O test registers (disabled)
//   $4020-$FFFF: Cartridge space (PRG ROM/RAM)

#include "bus.h"
#include <string.h>
#include <assert.h>

void bus_init(bus_s *bus)
{
    assert(bus != NULL);
    memset(bus->ram, 0, BUS_RAM_SIZE);
    bus->prg_rom = NULL;
    bus->prg_rom_size = 0;
}

byte_t bus_read(bus_s *bus, word_t addr)
{
    assert(bus != NULL);

    if (addr < 0x2000) {
        // Internal RAM ($0000-$1FFF)
        // 2KB mirrored 4 times: $0000-$07FF mirrors to $0800-$0FFF, $1000-$17FF, $1800-$1FFF
        return bus->ram[addr & 0x07FF];
    }
    else if (addr < 0x4000) {
        // PPU registers ($2000-$3FFF)
        // TODO: Implement PPU
        return 0;
    }
    else if (addr < 0x4018) {
        // APU and I/O registers ($4000-$4017)
        // TODO: Implement APU
        // $4014 is OAM DMA - handled specially
        // $4016-$4017 are controller ports
        return 0;
    }
    else if (addr < 0x4020) {
        // APU and I/O test registers ($4018-$401F)
        // Normally disabled
        return 0;
    }
    else {
        // Cartridge space ($4020-$FFFF)
        // Most common: PRG ROM at $8000-$FFFF
        if (addr >= 0x8000 && bus->prg_rom != NULL) {
            // Handle PRG ROM mirroring
            // For 16KB ROM: $8000-$BFFF and $C000-$FFFF both map to same 16KB
            // For 32KB ROM: $8000-$FFFF maps linearly
            size_t offset = addr - 0x8000;
            if (bus->prg_rom_size > 0) {
                offset = offset % bus->prg_rom_size;
            }
            return bus->prg_rom[offset];
        }
        // Open bus for unmapped addresses
        return 0;
    }
}

void bus_write(bus_s *bus, word_t addr, byte_t value)
{
    assert(bus != NULL);

    if (addr < 0x2000) {
        // Internal RAM ($0000-$1FFF)
        bus->ram[addr & 0x07FF] = value;
    }
    else if (addr < 0x4000) {
        // PPU registers ($2000-$3FFF)
        // TODO: Implement PPU
    }
    else if (addr < 0x4018) {
        // APU and I/O registers ($4000-$4017)
        // TODO: Implement APU
        // $4014 is OAM DMA - will need special handling
        // $4016 is controller strobe
    }
    else if (addr < 0x4020) {
        // APU and I/O test registers ($4018-$401F)
        // Normally disabled - writes ignored
    }
    else {
        // Cartridge space ($4020-$FFFF)
        // PRG ROM is read-only, but some mappers have RAM here
        // For now, ignore writes to cartridge space
    }
    (void)value; // Suppress unused parameter warning for stub cases
}

void bus_load_prg_rom(bus_s *bus, byte_t *prg_rom, size_t size)
{
    assert(bus != NULL);
    bus->prg_rom = prg_rom;
    bus->prg_rom_size = size;
}
