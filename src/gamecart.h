// NES Cartridge abstraction (gamecart_s)
// See: https://www.nesdev.org/wiki/Cartridge
//
// This struct encapsulates all data and state provided by a physical NES cartridge.
//
// Includes ROM, RAM, mapper, mirroring, and any special hardware.

#ifndef GAMECART_H
#define GAMECART_H

#include "ines.h"
#include "ppu.h"
#include <stddef.h>
#include <stdint.h>

// Forward declaration for mapper-specific state
struct mapper_state_s;

typedef struct gamecart_s {
    ines_rom_t rom;                // Parsed ROM data and header
    uint8_t *prg_ram;              // Optional PRG RAM (battery-backed)
    size_t prg_ram_size;
    int mapper_type;               // Mapper number (from iNES header)
    struct mapper_state_s *mapper; // Pointer to mapper-specific state
    mirroring_mode_e mirroring;    // Mirroring mode (from ROM or mapper)
    // Add fields for battery, trainer, or special hardware as needed
} gamecart_s;

// Allocate and initialize a gamecart from a ROM file
// Returns true on success, false on failure
bool gamecart_load(const char *path, gamecart_s *cart);

// Free all resources associated with a gamecart
void gamecart_free(gamecart_s *cart);

#endif // GAMECART_H
