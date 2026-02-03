// NES Cartridge abstraction (gamecart_s)
// See: https://www.nesdev.org/wiki/Cartridge
//
// Implements loading, freeing, and basic management of a gamecart_s.

#include "gamecart.h"
#include <stdlib.h>
#include <string.h>

bool gamecart_load(const char *path, gamecart_s *cart) {
    if (!cart) return false;
    memset(cart, 0, sizeof(*cart));
    if (!ines_load(path, &cart->rom)) return false;
    cart->mapper_type = cart->rom.mapper;
    cart->mirroring = cart->rom.mirroring_vertical ? MIRROR_VERTICAL : MIRROR_HORIZONTAL;
    // Allocate PRG RAM if needed (commonly 8KB, but can vary)
    cart->prg_ram_size = 0x2000; // 8KB default
    cart->prg_ram = malloc(cart->prg_ram_size);
    if (!cart->prg_ram) return false;
    memset(cart->prg_ram, 0, cart->prg_ram_size);
    // Mapper-specific state can be initialized here if needed
    cart->mapper = NULL;
    return true;
}

void gamecart_free(gamecart_s *cart) {
    if (!cart) return;
    ines_free(&cart->rom);
    free(cart->prg_ram);
    cart->prg_ram = NULL;
    cart->prg_ram_size = 0;
    // Free mapper-specific state if allocated
    if (cart->mapper) {
        // free(cart->mapper); // Mapper-specific cleanup
        cart->mapper = NULL;
    }
}
