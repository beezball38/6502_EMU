#include "ines.h"
#include <stdlib.h>
#include <string.h>

static bool validate_header(const ines_header_t *header) {
    return header->magic[0] == 'N' &&
           header->magic[1] == 'E' &&
           header->magic[2] == 'S' &&
           header->magic[3] == 0x1A;
}

static void decode_header(ines_rom_t *rom) {
    byte_t flags6 = rom->header.flags6;
    byte_t flags7 = rom->header.flags7;

    rom->mapper = (flags7 & 0xF0) | (flags6 >> 4);
    rom->mirroring_vertical = (flags6 & 0x01) != 0;
    rom->has_battery = (flags6 & 0x02) != 0;
    rom->has_trainer = (flags6 & 0x04) != 0;
    rom->prg_rom_bytes = rom->header.prg_rom_size * INES_PRG_ROM_UNIT;
    rom->chr_rom_bytes = rom->header.chr_rom_size * INES_CHR_ROM_UNIT;
}

bool ines_load_file(FILE *file, ines_rom_t *rom) {
    if (!file || !rom) {
        return false;
    }

    memset(rom, 0, sizeof(ines_rom_t));

    if (fread(&rom->header, sizeof(ines_header_t), 1, file) != 1) {
        return false;
    }

    if (!validate_header(&rom->header)) {
        return false;
    }

    decode_header(rom);

    if (rom->has_trainer) {
        fseek(file, INES_TRAINER_SIZE, SEEK_CUR);
    }

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
