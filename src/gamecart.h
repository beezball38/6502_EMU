#ifndef GAMECART_H
#define GAMECART_H

#include "ines.h"
#include "ppu.h"
#include <stddef.h>
#include <stdint.h>

typedef struct mapper_state_s mapper_state_s;

typedef struct gamecart_s {
    ines_rom_t rom;
    uint8_t *prg_ram;
    size_t prg_ram_size;
    int mapper_type;
    mapper_state_s *mapper;
    mirroring_mode_e mirroring;
} gamecart_s;

bool gamecart_load(const char *path, gamecart_s *cart);
void gamecart_free(gamecart_s *cart);

#endif
