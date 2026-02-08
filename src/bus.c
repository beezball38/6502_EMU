#include "bus.h"
#include "gamecart.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static bus_s s_bus;

bus_s* bus_get_instance(void)
{
    return &s_bus;
}

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
#define PRG_RAM_START   0x6000
#define PRG_RAM_END     0x7FFF
#define PRG_ROM_START   0x8000

void bus_init(bus_s *bus)
{
    assert(bus != NULL);
    for (size_t i = 0; i < BUS_RAM_SIZE; i++) {
        bus->ram[i] = rand() & 0xFF;
    }
    bus->cart = NULL;
    bus->ppu = NULL;
    bus->oam_dma_active = false;
    bus->oam_dma_cycles = 0;
}

static byte_t read_prg_ram(bus_s *bus, word_t addr)
{
    if (bus->cart && bus->cart->prg_ram) {
        return bus->cart->prg_ram[addr - PRG_RAM_START];
    }
    return 0;
}

static byte_t read_prg_rom(bus_s *bus, word_t addr)
{
    if (bus->cart && bus->cart->rom.prg_rom != NULL) {
        size_t prg_rom_size = bus->cart->rom.prg_rom_bytes;
        size_t offset = addr - PRG_ROM_START;
        if (prg_rom_size > 0) {
            offset = offset % prg_rom_size;
        }
        return bus->cart->rom.prg_rom[offset];
    }
    return 0;
}

static void write_prg_ram(bus_s *bus, word_t addr, byte_t value)
{
    if (bus->cart && bus->cart->prg_ram) {
        bus->cart->prg_ram[addr - PRG_RAM_START] = value;
    }
}

byte_t bus_read(bus_s *bus, word_t addr)
{
    assert(bus != NULL);

    if (addr <= RAM_END) {
        return bus->ram[addr & RAM_MASK];
    }
    else if (addr <= PPU_REG_END) {
        return ppu_read(bus->ppu, (ppu_register_e)(addr & PPU_REG_MASK));
    }
    else if (addr <= APU_IO_END) {
        return 0;
    }
    else if (addr <= APU_TEST_END) {
        return 0;
    }
    else {
        if (addr >= PRG_RAM_START && addr <= PRG_RAM_END) {
            return read_prg_ram(bus, addr);
        }
        if (addr >= PRG_ROM_START) {
            return read_prg_rom(bus, addr);
        }
        return 0;
    }
}

void bus_write(bus_s *bus, word_t addr, byte_t value)
{
    assert(bus != NULL);

    if (addr <= RAM_END) {
        bus->ram[addr & RAM_MASK] = value;
    }
    else if (addr <= PPU_REG_END) {
        ppu_write(bus->ppu, (ppu_register_e)(addr & PPU_REG_MASK), value);
    }
    else if (addr == OAM_DMA_REG) {
        bus_oam_dma(bus, value);
    }
    else if (addr >= PRG_RAM_START && addr <= PRG_RAM_END) {
        write_prg_ram(bus, addr, value);
    }
    (void)value;
}

void bus_attach_cart(bus_s *bus, gamecart_s *cart)
{
    assert(bus != NULL);
    bus->cart = cart;
    if (cart && bus->ppu) {
        ppu_load_chr_rom(bus->ppu, cart->rom.chr_rom, cart->rom.chr_rom_bytes);
        ppu_set_mirroring(bus->ppu, cart->mirroring);
    }
}

void bus_set_mirroring(bus_s *bus, mirroring_mode_e mode)
{
    assert(bus != NULL);
    ppu_set_mirroring(bus->ppu, mode);
}

word_t bus_read_word(bus_s *bus, word_t addr)
{
    assert(bus != NULL);
    byte_t lo = bus_read(bus, addr);
    byte_t hi = bus_read(bus, addr + 1);
    return (word_t)(lo | (hi << 8));
}

// https://www.nesdev.org/wiki/OAM_DMA
void bus_oam_dma(bus_s *bus, byte_t page)
{
    assert(bus != NULL);
    word_t src_addr = (word_t)page << 8;
    ppu_s *ppu = bus->ppu;
    for (int i = 0; i < 256; i++) {
        ppu->oam[i] = bus_read(bus, src_addr + i);
    }
    bus->oam_dma_cycles = 513;
}
