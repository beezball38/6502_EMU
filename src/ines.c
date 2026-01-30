// iNES ROM loader implementation
#include "ines.h"
#include <stdlib.h>
#include <string.h>

#define PRG_ROM_START 0x8000
#define PRG_ROM_MIRROR 0xC000

static bool validate_header(const ines_header_t *header) {
    // Check magic number: "NES" + 0x1A
    return header->magic[0] == 'N' &&
           header->magic[1] == 'E' &&
           header->magic[2] == 'S' &&
           header->magic[3] == 0x1A;
}

static void parse_header(ines_rom_t *rom) {
    byte_t flags6 = rom->header.flags6;
    byte_t flags7 = rom->header.flags7;

    // Mapper number: high nibble of flags7 | low nibble of flags6
    rom->mapper = (flags7 & 0xF0) | (flags6 >> 4);

    // Bit 0 of flags6: mirroring (0 = horizontal, 1 = vertical)
    rom->mirroring_vertical = (flags6 & 0x01) != 0;

    // Bit 1 of flags6: battery-backed PRG RAM
    rom->has_battery = (flags6 & 0x02) != 0;

    // Bit 2 of flags6: trainer present
    rom->has_trainer = (flags6 & 0x04) != 0;

    // Calculate ROM sizes in bytes
    rom->prg_rom_bytes = rom->header.prg_rom_size * INES_PRG_ROM_UNIT;
    rom->chr_rom_bytes = rom->header.chr_rom_size * INES_CHR_ROM_UNIT;
}

bool ines_load_file(FILE *file, ines_rom_t *rom) {
    if (!file || !rom) {
        return false;
    }

    memset(rom, 0, sizeof(ines_rom_t));

    // Read header
    if (fread(&rom->header, sizeof(ines_header_t), 1, file) != 1) {
        return false;
    }

    // Validate magic number
    if (!validate_header(&rom->header)) {
        return false;
    }

    // Parse header flags
    parse_header(rom);

    // Skip trainer if present
    if (rom->has_trainer) {
        fseek(file, INES_TRAINER_SIZE, SEEK_CUR);
    }

    // Allocate and read PRG ROM
    if (rom->prg_rom_bytes > 0) {
        rom->prg_rom = malloc(rom->prg_rom_bytes);
        if (!rom->prg_rom) {
            return false;
        }
        if (fread(rom->prg_rom, 1, rom->prg_rom_bytes, file) != rom->prg_rom_bytes) {
            free(rom->prg_rom);
            rom->prg_rom = NULL;
            return false;
        }
    }

    // Allocate and read CHR ROM
    if (rom->chr_rom_bytes > 0) {
        rom->chr_rom = malloc(rom->chr_rom_bytes);
        if (!rom->chr_rom) {
            free(rom->prg_rom);
            rom->prg_rom = NULL;
            return false;
        }
        if (fread(rom->chr_rom, 1, rom->chr_rom_bytes, file) != rom->chr_rom_bytes) {
            free(rom->prg_rom);
            free(rom->chr_rom);
            rom->prg_rom = NULL;
            rom->chr_rom = NULL;
            return false;
        }
    }

    return true;
}

bool ines_load(const char *path, ines_rom_t *rom) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        return false;
    }

    bool result = ines_load_file(file, rom);
    fclose(file);
    return result;
}

void ines_free(ines_rom_t *rom) {
    if (rom) {
        free(rom->prg_rom);
        free(rom->chr_rom);
        rom->prg_rom = NULL;
        rom->chr_rom = NULL;
    }
}

void ines_load_prg_into_memory(const ines_rom_t *rom, byte_t *memory) {
    if (!rom || !rom->prg_rom || !memory) {
        return;
    }

    // Load PRG ROM at $8000
    memcpy(&memory[PRG_ROM_START], rom->prg_rom, rom->prg_rom_bytes);

    // If only 16KB (1 bank), mirror it at $C000
    if (rom->prg_rom_bytes == INES_PRG_ROM_UNIT) {
        memcpy(&memory[PRG_ROM_MIRROR], rom->prg_rom, rom->prg_rom_bytes);
    }
}

void ines_print_info(const ines_rom_t *rom) {
    if (!rom) {
        return;
    }

    printf("=== iNES ROM Info ===\n");
    printf("PRG ROM: %zu KB (%d x 16KB banks)\n",
           rom->prg_rom_bytes / 1024, rom->header.prg_rom_size);
    printf("CHR ROM: %zu KB (%d x 8KB banks)\n",
           rom->chr_rom_bytes / 1024, rom->header.chr_rom_size);
    printf("Mapper: %d\n", rom->mapper);
    printf("Mirroring: %s\n", rom->mirroring_vertical ? "Vertical" : "Horizontal");
    printf("Battery: %s\n", rom->has_battery ? "Yes" : "No");
    printf("Trainer: %s\n", rom->has_trainer ? "Yes" : "No");
    printf("=====================\n");
}
