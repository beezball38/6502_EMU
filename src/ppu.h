#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "cpu_defs.h"

// https://www.nesdev.org/wiki/PPU_memory_map
//
//   $0000-$0FFF  Pattern table 0 (CHR ROM)
//   $1000-$1FFF  Pattern table 1 (CHR ROM)
//   $2000-$23FF  Nametable 0
//   $2400-$27FF  Nametable 1
//   $2800-$2BFF  Nametable 2
//   $2C00-$2FFF  Nametable 3
//   $3000-$3EFF  Mirrors of $2000-$2EFF
//   $3F00-$3F1F  Palette RAM
//   $3F20-$3FFF  Mirrors of $3F00-$3F1F
//

typedef enum {
    PPU_REGISTER_CTRL    = 0,
    PPU_REGISTER_MASK    = 1,
    PPU_REGISTER_STATUS  = 2,
    PPU_REGISTER_OAMADDR = 3,
    PPU_REGISTER_OAMDATA = 4,
    PPU_REGISTER_SCROLL  = 5,
    PPU_REGISTER_ADDR    = 6,
    PPU_REGISTER_DATA    = 7,
} ppu_register_e;

// https://www.nesdev.org/wiki/PPU_registers#PPUCTRL
//
//   7 6 5 4 3 2 1 0
//   V P H B S I N N
//   | | | | | | +-+-- Nametable select
//   | | | | | +------ VRAM increment (0: +1, 1: +32)
//   | | | | +-------- Sprite pattern table
//   | | | +---------- BG pattern table
//   | | +------------ Sprite size (0: 8x8, 1: 8x16)
//   | +-------------- PPU master/slave (unused)
//   +---------------- Generate NMI on vblank
//
typedef enum {
    PPUCTRL_NAMETABLE_X  = (1 << 0),
    PPUCTRL_NAMETABLE_Y  = (1 << 1),
    PPUCTRL_INCREMENT    = (1 << 2),
    PPUCTRL_SPRITE_TABLE = (1 << 3),
    PPUCTRL_BG_TABLE     = (1 << 4),
    PPUCTRL_SPRITE_SIZE  = (1 << 5),
    PPUCTRL_MASTER_SLAVE = (1 << 6),
    PPUCTRL_NMI_ENABLE   = (1 << 7),
} ppu_ctrl_flag_e;

// https://www.nesdev.org/wiki/PPU_registers#PPUMASK
//
//   7 6 5 4 3 2 1 0
//   B G R s b M m G
//   | | | | | | | +-- Grayscale
//   | | | | | | +---- Show BG in left 8 pixels
//   | | | | | +------ Show sprites in left 8 pixels
//   | | | | +-------- Show background
//   | | | +---------- Show sprites
//   | | +------------ Emphasize red
//   | +-------------- Emphasize green
//   +---------------- Emphasize blue
//
typedef enum {
    PPUMASK_GRAYSCALE     = (1 << 0),
    PPUMASK_BG_LEFT       = (1 << 1),
    PPUMASK_SPRITE_LEFT   = (1 << 2),
    PPUMASK_BG_ENABLE     = (1 << 3),
    PPUMASK_SPRITE_ENABLE = (1 << 4),
    PPUMASK_EMPHASIZE_R   = (1 << 5),
    PPUMASK_EMPHASIZE_G   = (1 << 6),
    PPUMASK_EMPHASIZE_B   = (1 << 7),
} ppu_mask_flag_e;

// https://www.nesdev.org/wiki/PPU_registers#PPUSTATUS
//
//   7 6 5 4 3 2 1 0
//   V S O . . . . .
//   | | |
//   | | +------------ Sprite overflow
//   | +-------------- Sprite 0 hit
//   +---------------- Vblank (cleared on read)
//
typedef enum {
    PPUSTATUS_OVERFLOW    = (1 << 5),
    PPUSTATUS_SPRITE0_HIT = (1 << 6),
    PPUSTATUS_VBLANK      = (1 << 7),
} ppu_status_flag_e;

// https://www.nesdev.org/wiki/PPU_OAM
// 64 sprites, 4 bytes each:
//   Byte 0: Y position
//   Byte 1: Tile index
//   Byte 2: Attributes
//   Byte 3: X position
//
#define OAM_SIZE 256
#define PPU_VRAM_SIZE     2048
#define PPU_PALETTE_SIZE  32
#define PPU_SCREEN_WIDTH  256
#define PPU_SCREEN_HEIGHT 240

// https://www.nesdev.org/wiki/Mirroring
typedef enum {
    MIRROR_HORIZONTAL,
    MIRROR_VERTICAL,
    MIRROR_SINGLE_LOW,
    MIRROR_SINGLE_HIGH,
    MIRROR_FOUR_SCREEN,
} mirroring_mode_e;

typedef struct {
    byte_t ctrl_register;
    byte_t mask_register;
    byte_t status_register;
    byte_t oam_addr_register;

    bool write_latch;
    word_t vram_addr;
    word_t temp_addr;
    byte_t fine_x;
    byte_t data_buffer;

    byte_t oam[OAM_SIZE];
    byte_t vram[PPU_VRAM_SIZE];
    byte_t palette[PPU_PALETTE_SIZE];

    byte_t *chr_rom;
    size_t chr_rom_size;
    mirroring_mode_e mirroring;

    uint16_t cycle;
    int16_t scanline;
    bool nmi_pending;

    uint32_t framebuffer[PPU_SCREEN_WIDTH * PPU_SCREEN_HEIGHT];
    bool frame_complete;
} ppu_s;

ppu_s* ppu_get_instance(void);

bool ppu_get_ctrl_flag(ppu_s *ppu, ppu_ctrl_flag_e flag);
void ppu_set_ctrl_flag(ppu_s *ppu, ppu_ctrl_flag_e flag, bool value);
bool ppu_get_mask_flag(ppu_s *ppu, ppu_mask_flag_e flag);
void ppu_set_mask_flag(ppu_s *ppu, ppu_mask_flag_e flag, bool value);
bool ppu_get_status_flag(ppu_s *ppu, ppu_status_flag_e flag);
void ppu_set_status_flag(ppu_s *ppu, ppu_status_flag_e flag, bool value);

void ppu_init(ppu_s *ppu);
void ppu_reset(ppu_s *ppu);
byte_t ppu_read(ppu_s *ppu, ppu_register_e reg);
void ppu_write(ppu_s *ppu, ppu_register_e reg, byte_t value);
void ppu_load_chr_rom(ppu_s *ppu, byte_t *chr_rom, size_t size);
void ppu_set_mirroring(ppu_s *ppu, mirroring_mode_e mode);
byte_t ppu_vram_read(ppu_s *ppu, word_t addr);
void ppu_vram_write(ppu_s *ppu, word_t addr, byte_t value);
void ppu_tick(ppu_s *ppu);
uint32_t *ppu_get_framebuffer(ppu_s *ppu);
bool ppu_frame_complete(ppu_s *ppu);

#endif
