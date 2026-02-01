// Picture Processing Unit for NES emulator

#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "cpu_defs.h"

// =============================================================================
// PPU register indices (addr & 0x07)
// For PPU addressing, every address 0000 - FFFF will map to 2001-2007
// These memory mapped addresses are used by the CPU and PPU to interact
// =============================================================================

typedef enum {
    PPU_REGISTER_CTRL    = 0,  // $2000 - Write
    PPU_REGISTER_MASK    = 1,  // $2001 - Write
    PPU_REGISTER_STATUS  = 2,  // $2002 - Read
    PPU_REGISTER_OAMADDR = 3,  // $2003 - Write
    PPU_REGISTER_OAMDATA = 4,  // $2004 - Read/Write
    PPU_REGISTER_SCROLL  = 5,  // $2005 - Write x2
    PPU_REGISTER_ADDR    = 6,  // $2006 - Write x2
    PPU_REGISTER_DATA    = 7,  // $2007 - Read/Write
} ppu_register_e;

/*
 * PPUCTRL ($2000) - Write only
 *
 *   7   6   5   4   3   2   1   0
 * +---+---+---+---+---+---+---+---+
 * | V | P | H | B | S | I | N | N |
 * +---+---+---+---+---+---+---+---+
 *   |   |   |   |   |   |   +---+-- Nametable select (0-3)
 *   |   |   |   |   |   +---------- VRAM increment (0: +1 across, 1: +32 down)
 *   |   |   |   |   +-------------- Sprite pattern table (0: $0000, 1: $1000)
 *   |   |   |   +------------------ BG pattern table (0: $0000, 1: $1000)
 *   |   |   +---------------------- Sprite size (0: 8x8, 1: 8x16)
 *   |   +-------------------------- Master/slave (unused on NES)
 *   +------------------------------ Generate NMI on vblank
 */
typedef enum {
    PPUCTRL_FLAG_NAMETABLE_X  = (1 << 0),
    PPUCTRL_FLAG_NAMETABLE_Y  = (1 << 1),
    PPUCTRL_FLAG_INCREMENT    = (1 << 2),
    PPUCTRL_FLAG_SPRITE_TABLE = (1 << 3),
    PPUCTRL_FLAG_BG_TABLE     = (1 << 4),
    PPUCTRL_FLAG_SPRITE_SIZE  = (1 << 5),
    PPUCTRL_FLAG_MASTER_SLAVE = (1 << 6),
    PPUCTRL_FLAG_NMI_ENABLE   = (1 << 7),
} ppu_ctrl_flag_e;

/*
 * PPUMASK ($2001) - Write only
 *
 *   7   6   5   4   3   2   1   0
 * +---+---+---+---+---+---+---+---+
 * | B | G | R | s | b | M | m | G |
 * +---+---+---+---+---+---+---+---+
 *   |   |   |   |   |   |   |   +-- Grayscale mode
 *   |   |   |   |   |   |   +------ Show BG in leftmost 8 pixels
 *   |   |   |   |   |   +---------- Show sprites in leftmost 8 pixels
 *   |   |   |   |   +-------------- Show background
 *   |   |   |   +------------------ Show sprites
 *   |   |   +---------------------- Emphasize red
 *   |   +-------------------------- Emphasize green
 *   +------------------------------ Emphasize blue
 */
typedef enum {
    PPUMASK_FLAG_GRAYSCALE     = (1 << 0),
    PPUMASK_FLAG_BG_LEFT       = (1 << 1),
    PPUMASK_FLAG_SPRITE_LEFT   = (1 << 2),
    PPUMASK_FLAG_BG_ENABLE     = (1 << 3),
    PPUMASK_FLAG_SPRITE_ENABLE = (1 << 4),
    PPUMASK_FLAG_EMPHASIZE_R   = (1 << 5),
    PPUMASK_FLAG_EMPHASIZE_G   = (1 << 6),
    PPUMASK_FLAG_EMPHASIZE_B   = (1 << 7),
} ppu_mask_flag_e;

/*
 * PPUSTATUS ($2002) - Read only
 *
 *   7   6   5   4   3   2   1   0
 * +---+---+---+---+---+---+---+---+
 * | V | S | O | . | . | . | . | . |
 * +---+---+---+---+---+---+---+---+
 *   |   |   |   +---+---+---+---+-- Unused (stale bus contents)
 *   |   |   +---------------------- Sprite overflow
 *   |   +-------------------------- Sprite 0 hit
 *   +------------------------------ Vblank started
 *
 * Note: Reading this register clears bit 7 and resets the write latch
 */
typedef enum {
    PPUSTATUS_FLAG_OVERFLOW    = (1 << 5),
    PPUSTATUS_FLAG_SPRITE0_HIT = (1 << 6),
    PPUSTATUS_FLAG_VBLANK      = (1 << 7),
} ppu_status_flag_e;

// =============================================================================
// OAM
// =============================================================================

#define OAM_SIZE 256  // 64 sprites * 4 bytes each

// =============================================================================
// VRAM sizes
// =============================================================================

#define PPU_VRAM_SIZE     2048  // 2KB nametable RAM
#define PPU_PALETTE_SIZE  32    // 32 bytes palette RAM

// =============================================================================
// Screen dimensions
// =============================================================================

#define PPU_SCREEN_WIDTH  256
#define PPU_SCREEN_HEIGHT 240

// =============================================================================
// Nametable mirroring modes
// =============================================================================

typedef enum {
    MIRROR_HORIZONTAL,  // Vertical arrangement (CIRAM A10 = PPU A11)
    MIRROR_VERTICAL,    // Horizontal arrangement (CIRAM A10 = PPU A10)
    MIRROR_SINGLE_LOW,  // Single screen, lower bank
    MIRROR_SINGLE_HIGH, // Single screen, upper bank
    MIRROR_FOUR_SCREEN, // Four screen (requires extra VRAM on cartridge)
} mirroring_mode_e;

// =============================================================================
// PPU struct
// =============================================================================

typedef struct {
    // Registers (directly accessible via $2000-$2007)
    byte_t ctrl_register;       // $2000 - PPUCTRL
    byte_t mask_register;       // $2001 - PPUMASK
    byte_t status_register;     // $2002 - PPUSTATUS
    byte_t oam_addr_register;   // $2003 - OAMADDR

    // Internal state
    bool write_latch;   // Toggle for $2005/$2006 double writes (false = first, true = second)
    word_t vram_addr;   // Current VRAM address (15-bit, "v" register)
    word_t temp_addr;   // Temporary VRAM address (15-bit, "t" register)
    byte_t fine_x;      // Fine X scroll (3-bit)
    byte_t data_buffer; // $2007 read buffer (reads are delayed by one)

    // OAM (Object Attribute Memory) - sprite data
    byte_t oam[OAM_SIZE];

    // VRAM
    byte_t vram[PPU_VRAM_SIZE];      // 2KB nametable RAM
    byte_t palette[PPU_PALETTE_SIZE]; // Palette RAM

    // CHR ROM/RAM (from cartridge)
    byte_t *chr_rom;
    size_t chr_rom_size;

    // Nametable mirroring mode (set by cartridge)
    mirroring_mode_e mirroring;

    // Timing
    uint16_t cycle;     // Current cycle within scanline (0-340)
    int16_t scanline;   // Current scanline (-1 to 260, -1 = pre-render)
    bool nmi_pending;   // NMI needs to be sent to CPU

    // Framebuffer (256x240 pixels, 32-bit ARGB)
    uint32_t framebuffer[PPU_SCREEN_WIDTH * PPU_SCREEN_HEIGHT];
    bool frame_complete;  // Set when a frame is done rendering

} ppu_s;

// =============================================================================
// Flag access functions
// =============================================================================

bool ppu_get_ctrl_flag(ppu_s *ppu, ppu_ctrl_flag_e flag);
void ppu_set_ctrl_flag(ppu_s *ppu, ppu_ctrl_flag_e flag, bool value);

bool ppu_get_mask_flag(ppu_s *ppu, ppu_mask_flag_e flag);
void ppu_set_mask_flag(ppu_s *ppu, ppu_mask_flag_e flag, bool value);

bool ppu_get_status_flag(ppu_s *ppu, ppu_status_flag_e flag);
void ppu_set_status_flag(ppu_s *ppu, ppu_status_flag_e flag, bool value);

// =============================================================================
// PPU interface
// =============================================================================

// Initialize PPU to power-on state
void ppu_init(ppu_s *ppu);

// CPU interface - read/write PPU registers
byte_t ppu_read(ppu_s *ppu, ppu_register_e reg);
void ppu_write(ppu_s *ppu, ppu_register_e reg, byte_t value);

// Connect CHR ROM to the PPU
void ppu_load_chr_rom(ppu_s *ppu, byte_t *chr_rom, size_t size);

// Set nametable mirroring mode
void ppu_set_mirroring(ppu_s *ppu, mirroring_mode_e mode);

// =============================================================================
// Internal VRAM access (for testing and internal use)
// =============================================================================

// Read from PPU address space ($0000-$3FFF)
byte_t ppu_vram_read(ppu_s *ppu, word_t addr);

// Write to PPU address space ($0000-$3FFF)
void ppu_vram_write(ppu_s *ppu, word_t addr, byte_t value);

// =============================================================================
// PPU timing
// =============================================================================

// Advance PPU by one cycle
void ppu_tick(ppu_s *ppu);

// Get pointer to framebuffer (256x240 ARGB pixels)
uint32_t *ppu_get_framebuffer(ppu_s *ppu);

// Check and clear frame_complete flag
bool ppu_frame_complete(ppu_s *ppu);

#endif
