// System bus implementation for NES emulator

#include "bus.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>


/*
 * NES CPU Memory Map (see: https://www.nesdev.org/wiki/CPU_memory_map)
 *
 *  $0000-$07FF   2KB internal RAM
 *  $0800-$1FFF   Mirrors of $0000-$07FF
 *  $2000-$2007   PPU registers
 *  $2008-$3FFF   Mirrors of $2000-2007 (every 8 bytes)
 *  $4000-$4017   APU and I/O registers
 *  $4018-$401F   APU and I/O functionality that is normally disabled
 *  $4020-$FFFF   Cartridge space (PRG ROM, PRG RAM, mappers)
 *
 *  See also: https://www.nesdev.org/wiki/PPU_registers
 */
#define RAM_START       0x0000
#define RAM_END         0x1FFF
#define RAM_MASK        0x07FF
#define PPU_REG_START   0x2000
#define PPU_REG_END     0x3FFF
#define PPU_REG_MASK    0x0007
#define APU_IO_START    0x4000
#define APU_IO_END      0x4017
#define OAM_DMA_REG     0x4014
#define APU_TEST_START  0x4018
#define APU_TEST_END    0x401F
#define CART_START      0x4020
#define PRG_ROM_START   0x8000

void bus_init(bus_s *bus)
{
    assert(bus != NULL);
    // Randomize RAM to match real hardware behavior (RAM is undefined at power-on)
    for (size_t i = 0; i < BUS_RAM_SIZE; i++) {
        bus->ram[i] = rand() & 0xFF;
    }
    bus->prg_rom = NULL;
    bus->prg_rom_size = 0;
    bus->cpu = malloc(sizeof(cpu_s));
    bus->ppu = malloc(sizeof(ppu_s));
    cpu_init(bus->cpu);
    ppu_init(bus->ppu);
}

byte_t bus_read(bus_s *bus, word_t addr)
{
    assert(bus != NULL);

    if (addr <= RAM_END) {
        // Internal RAM (2KB mirrored 4x)
        return bus->ram[addr & RAM_MASK];
    }
    else if (addr <= PPU_REG_END) {
        // PPU registers (8 bytes mirrored)
        return ppu_read(&bus->ppu, (ppu_register_e)(addr & PPU_REG_MASK));
    }
    else if (addr <= APU_IO_END) {
        // APU and I/O registers
        // TODO: Implement APU
        return 0;
    }
    else if (addr <= APU_TEST_END) {
        // APU test registers (normally disabled)
        return 0;
    }
    else {
        // Cartridge space
        if (addr >= PRG_ROM_START && bus->prg_rom != NULL) {
            size_t offset = addr - PRG_ROM_START;
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

    if (addr <= RAM_END) {
        // Internal RAM (2KB mirrored 4x)
        bus->ram[addr & RAM_MASK] = value;
    }
    else if (addr <= PPU_REG_END) {
        // PPU registers (8 bytes mirrored)
        ppu_write(&bus->ppu, (ppu_register_e)(addr & PPU_REG_MASK), value);
    }
    else if (addr <= APU_IO_END) {
        // APU and I/O registers
        if (addr == OAM_DMA_REG) {
            // $4014 - OAM DMA: Writing triggers DMA transfer
            bus_oam_dma(bus, value);
        }
        // TODO: Implement APU
    }
    else if (addr <= APU_TEST_END) {
        // APU test registers (normally disabled)
    }
    else {
        // Cartridge space (PRG ROM read-only, mappers may have RAM)
    }
    (void)value; // Suppress unused parameter warning for stub cases
}

void bus_load_prg_rom(bus_s *bus, byte_t *prg_rom, size_t size)
{
    assert(bus != NULL);
    bus->prg_rom = prg_rom;
    bus->prg_rom_size = size;
}

void bus_load_chr_rom(bus_s *bus, byte_t *chr_rom, size_t size)
{
    assert(bus != NULL);
    ppu_load_chr_rom(&bus->ppu, chr_rom, size);
}

void bus_set_mirroring(bus_s *bus, mirroring_mode_e mode)
{
    assert(bus != NULL);
    ppu_set_mirroring(&bus->ppu, mode);
}

word_t bus_read_word(bus_s *bus, word_t addr)
{
    assert(bus != NULL);
    byte_t lo = bus_read(bus, addr);
    byte_t hi = bus_read(bus, addr + 1);
    return (word_t)(lo | (hi << 8));
}

void bus_oam_dma(bus_s *bus, byte_t page)
{
    assert(bus != NULL);

    /*
     * OAM DMA (Object Attribute Memory Direct Memory Access)
     * Copies 256 bytes from CPU memory $XX00-$XXFF to PPU OAM (sprite RAM).
     * Triggered by writing to $4014. See: https://www.nesdev.org/wiki/OAM_DMA
     *
     * Timing: Takes 513 or 514 CPU cycles depending on alignment.
     */
    word_t src_addr = (word_t)page << 8;
    for (int i = 0; i < 256; i++) {
        byte_t data = bus_read(bus, src_addr + i);
        bus->ppu->oam[i] = data;
    }
    bus->oam_dma_cycles = 513;
}

void bus_ppu_tick(bus_s *bus)
{
    assert(bus != NULL);
    ppu_tick(&bus->ppu);
}
