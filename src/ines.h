// iNES ROM loader
#ifndef INES_H
#define INES_H

#include "cpu_defs.h"
#include <stdbool.h>
#include <stdio.h>

#define INES_HEADER_SIZE     0x10
#define INES_PRG_ROM_UNIT    0x4000  // 16KB
#define INES_CHR_ROM_UNIT    0x2000  // 8KB
#define INES_TRAINER_SIZE    0x200   // 512 bytes

// iNES header structure
typedef struct {
    byte_t magic[4];        // "NES" + 0x1A
    byte_t prg_rom_size;    // PRG ROM size in 16KB units
    byte_t chr_rom_size;    // CHR ROM size in 8KB units
    byte_t flags6;          // Mapper low nibble, mirroring, battery, trainer
    byte_t flags7;          // Mapper high nibble, NES 2.0 identifier
    byte_t flags8;          // PRG RAM size (rarely used)
    byte_t flags9;          // TV system (rarely used)
    byte_t flags10;         // TV system, PRG RAM (unofficial)
    byte_t padding[5];      // Unused padding
} ines_header_t;

// Parsed ROM data
typedef struct {
    ines_header_t header;
    byte_t mapper;
    bool mirroring_vertical;  // false = horizontal, true = vertical
    bool has_battery;
    bool has_trainer;
    byte_t *prg_rom;          // PRG ROM data (CPU code)
    byte_t *chr_rom;          // CHR ROM data (PPU graphics)
    size_t prg_rom_bytes;
    size_t chr_rom_bytes;
} ines_rom_t;

// Load ROM from file path
// Returns true on success, false on failure
// Caller must call ines_free() when done
bool ines_load(const char *path, ines_rom_t *rom);

// Load ROM from file handle
bool ines_load_file(FILE *file, ines_rom_t *rom);

// Free ROM data
void ines_free(ines_rom_t *rom);

// Load PRG ROM into CPU memory (for simple mapper 0)
// Handles mirroring for 16KB ROMs
void ines_load_prg_into_memory(const ines_rom_t *rom, byte_t *memory);

// Print ROM info for debugging
void ines_print_info(const ines_rom_t *rom);

#endif
