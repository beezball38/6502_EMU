#ifndef BUS_H
#define BUS_H

#include "cpu_defs.h"
#include "ppu.h"
#include <stddef.h>

// https://www.nesdev.org/wiki/CPU_memory_map
//
//   $0000-$07FF  2KB Internal RAM
//   $0800-$1FFF  Mirrors of $0000-$07FF
//   $2000-$2007  PPU registers
//   $2008-$3FFF  Mirrors of $2000-$2007
//   $4000-$4017  APU and I/O registers
//   $4018-$401F  APU test registers (disabled)
//   $4020-$FFFF  Cartridge space (PRG ROM, mapper registers)
//

#define BUS_RAM_SIZE 2048

typedef struct gamecart_s gamecart_s;

typedef struct bus {
    byte_t ram[BUS_RAM_SIZE];
    gamecart_s *cart;

    ppu_s *ppu;

    bool oam_dma_active;
    byte_t oam_dma_page;
    uint16_t oam_dma_cycles;
} bus_s;

bus_s* bus_get_instance(void);
void bus_init(bus_s *bus);
byte_t bus_read(bus_s *bus, word_t addr);
void bus_write(bus_s *bus, word_t addr, byte_t value);
word_t bus_read_word(bus_s *bus, word_t addr);
void bus_attach_cart(bus_s *bus, gamecart_s *cart);
void bus_set_mirroring(bus_s *bus, mirroring_mode_e mode);
void bus_oam_dma(bus_s *bus, byte_t page);

#endif
