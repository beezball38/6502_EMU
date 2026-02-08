#ifndef INES_H
#define INES_H

#include "cpu_defs.h"
#include <stdbool.h>
#include <stdio.h>

#define INES_HEADER_SIZE     0x10
#define INES_PRG_ROM_UNIT    0x4000
#define INES_CHR_ROM_UNIT    0x2000
#define INES_TRAINER_SIZE    0x200

typedef struct {
    byte_t magic[4];
    byte_t prg_rom_size;
    byte_t chr_rom_size;
    byte_t flags6;
    byte_t flags7;
    byte_t flags8;
    byte_t flags9;
    byte_t flags10;
    byte_t padding[5];
} ines_header_t;

typedef struct {
    ines_header_t header;
    byte_t mapper;
    bool mirroring_vertical;
    bool has_battery;
    bool has_trainer;
    byte_t *prg_rom;
    byte_t *chr_rom;
    size_t prg_rom_bytes;
    size_t chr_rom_bytes;
} ines_rom_t;

bool ines_load(const char *path, ines_rom_t *rom);
bool ines_load_file(FILE *file, ines_rom_t *rom);
void ines_free(ines_rom_t *rom);
void ines_print_info(const ines_rom_t *rom);

#endif
