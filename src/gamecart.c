#include "gamecart.h"
#include <stdlib.h>
#include <string.h>

bool gamecart_load(const char *path, gamecart_s *cart) {
    if (!cart) return false;
    memset(cart, 0, sizeof(*cart));
    if (!ines_load(path, &cart->rom)) return false;
    cart->mapper_type = cart->rom.mapper;
    cart->mirroring = cart->rom.mirroring_vertical ? MIRROR_VERTICAL : MIRROR_HORIZONTAL;
    cart->prg_ram_size = 0x2000;
    cart->prg_ram = malloc(cart->prg_ram_size);
    if (!cart->prg_ram) return false;
    memset(cart->prg_ram, 0, cart->prg_ram_size);
    cart->mapper = NULL;
    return true;
}

void gamecart_free(gamecart_s *cart) {
    if (!cart) return;
    ines_free(&cart->rom);
    free(cart->prg_ram);
    cart->prg_ram = NULL;
    cart->prg_ram_size = 0;
    if (cart->mapper) {
        cart->mapper = NULL;
    }
}
