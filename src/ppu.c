// PPU (Picture Processing Unit) implementation

#include "ppu.h"
#include <string.h>
#include <assert.h>

// =============================================================================
// NES Master Palette (64 colors, ARGB format)
// Based on the standard 2C02 palette
// =============================================================================

static const uint32_t NES_PALETTE[64] = {
    0xFF666666, 0xFF002A88, 0xFF1412A7, 0xFF3B00A4, 0xFF5C007E, 0xFF6E0040, 0xFF6C0600, 0xFF561D00,
    0xFF333500, 0xFF0B4800, 0xFF005200, 0xFF004F08, 0xFF00404D, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFADADAD, 0xFF155FD9, 0xFF4240FF, 0xFF7527FE, 0xFFA01ACC, 0xFFB71E7B, 0xFFB53120, 0xFF994E00,
    0xFF6B6D00, 0xFF388700, 0xFF0C9300, 0xFF008F32, 0xFF007C8D, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFFFFEFF, 0xFF64B0FF, 0xFF9290FF, 0xFFC676FF, 0xFFF36AFF, 0xFFFE6ECC, 0xFFFE8170, 0xFFEA9E22,
    0xFFBCBE00, 0xFF88D800, 0xFF5CE430, 0xFF45E082, 0xFF48CDDE, 0xFF4F4F4F, 0xFF000000, 0xFF000000,
    0xFFFFFEFF, 0xFFC0DFFF, 0xFFD3D2FF, 0xFFE8C8FF, 0xFFFBC2FF, 0xFFFEC4EA, 0xFFFECCC5, 0xFFF7D8A5,
    0xFFE4E594, 0xFFCFEF96, 0xFFBDF4AB, 0xFFB3F3CC, 0xFFB5EBF2, 0xFFB8B8B8, 0xFF000000, 0xFF000000,
};

// =============================================================================
// Flag access functions
// =============================================================================

bool ppu_get_ctrl_flag(ppu_s *ppu, ppu_ctrl_flag_e flag)
{
    assert(ppu != NULL);
    return (ppu->ctrl_register & flag) != 0;
}

void ppu_set_ctrl_flag(ppu_s *ppu, ppu_ctrl_flag_e flag, bool value)
{
    assert(ppu != NULL);
    if (value)
        ppu->ctrl_register |= flag;
    else
        ppu->ctrl_register &= ~flag;
}

bool ppu_get_mask_flag(ppu_s *ppu, ppu_mask_flag_e flag)
{
    assert(ppu != NULL);
    return (ppu->mask_register & flag) != 0;
}

void ppu_set_mask_flag(ppu_s *ppu, ppu_mask_flag_e flag, bool value)
{
    assert(ppu != NULL);
    if (value)
        ppu->mask_register |= flag;
    else
        ppu->mask_register &= ~flag;
}

bool ppu_get_status_flag(ppu_s *ppu, ppu_status_flag_e flag)
{
    assert(ppu != NULL);
    return (ppu->status_register & flag) != 0;
}

void ppu_set_status_flag(ppu_s *ppu, ppu_status_flag_e flag, bool value)
{
    assert(ppu != NULL);
    if (value)
        ppu->status_register |= flag;
    else
        ppu->status_register &= ~flag;
}

// =============================================================================
// PPU initialization
// =============================================================================

void ppu_init(ppu_s *ppu)
{
    assert(ppu != NULL);
    memset(ppu, 0, sizeof(*ppu));
    ppu->scanline = 261;  // Start at pre-render scanline
}

// =============================================================================
// CHR ROM and mirroring configuration
// =============================================================================

void ppu_load_chr_rom(ppu_s *ppu, byte_t *chr_rom, size_t size)
{
    assert(ppu != NULL);
    ppu->chr_rom = chr_rom;
    ppu->chr_rom_size = size;
}

void ppu_set_mirroring(ppu_s *ppu, mirroring_mode_e mode)
{
    assert(ppu != NULL);
    ppu->mirroring = mode;
}

// =============================================================================
// Nametable address mirroring
// Maps PPU address $2000-$2FFF to the 2KB internal VRAM
// =============================================================================

static word_t mirror_nametable_addr(ppu_s *ppu, word_t addr)
{
    // addr is in range $2000-$2FFF (nametable space)
    // We need to map to 0-2047 (2KB VRAM)
    addr &= 0x0FFF;  // Now 0x000-0xFFF (4KB logical space)

    switch (ppu->mirroring) {
        case MIRROR_HORIZONTAL:
            // Nametables 0,1 -> VRAM 0-1023; Nametables 2,3 -> VRAM 1024-2047
            // $2000-$23FF and $2400-$27FF -> 0-1023
            // $2800-$2BFF and $2C00-$2FFF -> 1024-2047
            if (addr < 0x800) {
                return addr & 0x3FF;          // NT0 and NT1 -> first 1KB
            } else {
                return 0x400 + (addr & 0x3FF); // NT2 and NT3 -> second 1KB
            }

        case MIRROR_VERTICAL:
            // Nametables 0,2 -> VRAM 0-1023; Nametables 1,3 -> VRAM 1024-2047
            // $2000-$23FF and $2800-$2BFF -> 0-1023
            // $2400-$27FF and $2C00-$2FFF -> 1024-2047
            return addr & 0x7FF;  // Just use bit 10 to select bank

        case MIRROR_SINGLE_LOW:
            // All nametables map to first 1KB
            return addr & 0x3FF;

        case MIRROR_SINGLE_HIGH:
            // All nametables map to second 1KB
            return 0x400 + (addr & 0x3FF);

        case MIRROR_FOUR_SCREEN:
            // Full 4KB, no mirroring (requires cartridge RAM)
            // For now, just wrap within our 2KB
            return addr & 0x7FF;

        default:
            return addr & 0x7FF;
    }
}

// =============================================================================
// VRAM read/write (PPU internal bus)
// =============================================================================

byte_t ppu_vram_read(ppu_s *ppu, word_t addr)
{
    assert(ppu != NULL);
    addr &= 0x3FFF;  // PPU address space is 14-bit ($0000-$3FFF)

    if (addr < 0x2000) {
        // Pattern tables ($0000-$1FFF) - CHR ROM/RAM
        if (ppu->chr_rom != NULL && ppu->chr_rom_size > 0) {
            return ppu->chr_rom[addr % ppu->chr_rom_size];
        }
        return 0;
    }
    else if (addr < 0x3F00) {
        // Nametables ($2000-$2FFF, mirrored at $3000-$3EFF)
        word_t nt_addr = addr;
        if (addr >= 0x3000) {
            nt_addr = addr - 0x1000;  // Mirror $3000-$3EFF to $2000-$2EFF
        }
        return ppu->vram[mirror_nametable_addr(ppu, nt_addr)];
    }
    else {
        // Palette ($3F00-$3F1F, mirrored to $3FFF)
        word_t pal_addr = (addr - 0x3F00) & 0x1F;  // 32-byte palette

        // Addresses $3F10, $3F14, $3F18, $3F1C mirror $3F00, $3F04, $3F08, $3F0C
        if ((pal_addr & 0x13) == 0x10) {
            pal_addr &= 0x0F;
        }
        return ppu->palette[pal_addr];
    }
}

void ppu_vram_write(ppu_s *ppu, word_t addr, byte_t value)
{
    assert(ppu != NULL);
    addr &= 0x3FFF;  // PPU address space is 14-bit ($0000-$3FFF)

    if (addr < 0x2000) {
        // Pattern tables ($0000-$1FFF) - CHR ROM is read-only
        // CHR RAM would be writable, but we don't support that yet
        return;
    }
    else if (addr < 0x3F00) {
        // Nametables ($2000-$2FFF, mirrored at $3000-$3EFF)
        word_t nt_addr = addr;
        if (addr >= 0x3000) {
            nt_addr = addr - 0x1000;  // Mirror $3000-$3EFF to $2000-$2EFF
        }
        ppu->vram[mirror_nametable_addr(ppu, nt_addr)] = value;
    }
    else {
        // Palette ($3F00-$3F1F, mirrored to $3FFF)
        word_t pal_addr = (addr - 0x3F00) & 0x1F;  // 32-byte palette

        // Addresses $3F10, $3F14, $3F18, $3F1C mirror $3F00, $3F04, $3F08, $3F0C
        if ((pal_addr & 0x13) == 0x10) {
            pal_addr &= 0x0F;
        }
        ppu->palette[pal_addr] = value;
    }
}

// =============================================================================
// CPU interface - register reads
// =============================================================================

byte_t ppu_read(ppu_s *ppu, ppu_register_e reg)
{
    assert(ppu != NULL);

    switch (reg) {
        case PPU_REGISTER_CTRL:
            // Write-only, return 0
            return 0;

        case PPU_REGISTER_MASK:
            // Write-only, return 0
            return 0;

        case PPU_REGISTER_STATUS: {
            // Return status with vblank flag, then clear it
            byte_t status = ppu->status_register;
            ppu_set_status_flag(ppu, PPUSTATUS_FLAG_VBLANK, false);
            ppu->write_latch = false;  // Reset write latch
            return status;
        }

        case PPU_REGISTER_OAMADDR:
            // Write-only, return 0
            return 0;

        case PPU_REGISTER_OAMDATA:
            // Return OAM data at current address
            return ppu->oam[ppu->oam_addr_register];

        case PPU_REGISTER_SCROLL:
            // Write-only, return 0
            return 0;

        case PPU_REGISTER_ADDR:
            // Write-only, return 0
            return 0;

        case PPU_REGISTER_DATA: {
            // Reads are buffered (delayed by one read), except palette reads
            word_t addr = ppu->vram_addr & 0x3FFF;
            byte_t data;

            if (addr >= 0x3F00) {
                // Palette reads are NOT buffered - return immediately
                // But the buffer gets filled with nametable data "underneath" the palette
                data = ppu_vram_read(ppu, addr);
                ppu->data_buffer = ppu_vram_read(ppu, addr - 0x1000);
            } else {
                // Normal reads are buffered (delayed by one read)
                data = ppu->data_buffer;
                ppu->data_buffer = ppu_vram_read(ppu, addr);
            }

            // Increment vram_addr by 1 or 32 based on PPUCTRL increment flag
            // VRAM address is 14-bit, wraps at $4000
            ppu->vram_addr += ppu_get_ctrl_flag(ppu, PPUCTRL_FLAG_INCREMENT) ? 32 : 1;
            ppu->vram_addr &= 0x3FFF;
            return data;
        }

        default:
            return 0;
    }
}

// =============================================================================
// CPU interface - register writes
// =============================================================================

void ppu_write(ppu_s *ppu, ppu_register_e reg, byte_t value)
{
    assert(ppu != NULL);

    switch (reg) {
        case PPU_REGISTER_CTRL: {
            byte_t prev_nmi = ppu->ctrl_register & PPUCTRL_FLAG_NMI_ENABLE;
            ppu->ctrl_register = value;
            // Nametable select bits go into temp_addr bits 10-11
            ppu->temp_addr = (ppu->temp_addr & 0xF3FF) | ((value & 0x03) << 10);

            // Edge case: If NMI is enabled while vblank flag is set, trigger NMI
            if (!prev_nmi && (value & PPUCTRL_FLAG_NMI_ENABLE) &&
                ppu_get_status_flag(ppu, PPUSTATUS_FLAG_VBLANK)) {
                ppu->nmi_pending = true;
            }
            break;
        }

        case PPU_REGISTER_MASK:
            ppu->mask_register = value;
            break;

        case PPU_REGISTER_STATUS:
            // Read-only, ignore writes
            break;

        case PPU_REGISTER_OAMADDR:
            ppu->oam_addr_register = value;
            break;

        case PPU_REGISTER_OAMDATA:
            ppu->oam[ppu->oam_addr_register] = value;
            ppu->oam_addr_register++;  // Wraps at 256
            break;

        case PPU_REGISTER_SCROLL:
            if (!ppu->write_latch) {
                // First write: X scroll
                ppu->fine_x = value & 0x07;  // Fine X = low 3 bits
                ppu->temp_addr = (ppu->temp_addr & 0xFFE0) | (value >> 3);  // Coarse X = high 5 bits
            } else {
                // Second write: Y scroll
                // Fine Y goes to bits 12-14 of temp_addr
                // Coarse Y goes to bits 5-9 of temp_addr
                ppu->temp_addr = (ppu->temp_addr & 0x8C1F) |
                                 ((value & 0x07) << 12) |  // Fine Y
                                 ((value >> 3) << 5);       // Coarse Y
            }
            ppu->write_latch = !ppu->write_latch;
            break;

        case PPU_REGISTER_ADDR:
            if (!ppu->write_latch) {
                // First write: high byte (bits 8-13, bit 14 cleared)
                ppu->temp_addr = (ppu->temp_addr & 0x00FF) | ((value & 0x3F) << 8);
            } else {
                // Second write: low byte, then copy to vram_addr
                ppu->temp_addr = (ppu->temp_addr & 0xFF00) | value;
                ppu->vram_addr = ppu->temp_addr;
            }
            ppu->write_latch = !ppu->write_latch;
            break;

        case PPU_REGISTER_DATA:
            // Write value to VRAM at vram_addr
            ppu_vram_write(ppu, ppu->vram_addr, value);
            // Increment vram_addr by 1 or 32 based on PPUCTRL increment flag
            // VRAM address is 14-bit, wraps at $4000
            ppu->vram_addr += ppu_get_ctrl_flag(ppu, PPUCTRL_FLAG_INCREMENT) ? 32 : 1;
            ppu->vram_addr &= 0x3FFF;
            break;

        default:
            break;
    }
}

// =============================================================================
// PPU timing constants
// =============================================================================

#define PPU_CYCLES_PER_SCANLINE  341
#define PPU_SCANLINES_PER_FRAME  262
#define PPU_VBLANK_SCANLINE      241
#define PPU_PRERENDER_SCANLINE   261

// =============================================================================
// Background rendering helpers
// =============================================================================

// Render a single background pixel at the current position
static void render_pixel(ppu_s *ppu)
{
    int x = ppu->cycle - 1;  // Cycles 1-256 produce pixels 0-255
    int y = ppu->scanline;

    if (x < 0 || x >= PPU_SCREEN_WIDTH || y < 0 || y >= PPU_SCREEN_HEIGHT) {
        return;
    }

    uint32_t pixel_color = NES_PALETTE[ppu->palette[0] & 0x3F];  // Default: background color

    // Check if background rendering is enabled
    bool bg_enabled = ppu_get_mask_flag(ppu, PPUMASK_FLAG_BG_ENABLE);
    bool bg_left_enabled = ppu_get_mask_flag(ppu, PPUMASK_FLAG_BG_LEFT);

    if (bg_enabled && (x >= 8 || bg_left_enabled)) {
        // Get coarse X, coarse Y, fine X, fine Y from vram_addr
        word_t v = ppu->vram_addr;
        int coarse_x = v & 0x1F;
        int coarse_y = (v >> 5) & 0x1F;
        int fine_y = (v >> 12) & 0x07;
        int nametable = (v >> 10) & 0x03;
        int fine_x_scroll = ppu->fine_x;

        // Calculate which pixel within the current tile we're rendering
        int tile_x = (x + fine_x_scroll) % 8;

        // Nametable address: $2000 + nametable offset + coarse position
        word_t nt_addr = 0x2000 | (nametable << 10) | (coarse_y << 5) | coarse_x;
        byte_t tile_index = ppu_vram_read(ppu, nt_addr);

        // Pattern table address
        word_t pattern_base = ppu_get_ctrl_flag(ppu, PPUCTRL_FLAG_BG_TABLE) ? 0x1000 : 0x0000;
        word_t pattern_addr = pattern_base + (tile_index * 16) + fine_y;

        // Fetch pattern table bytes (low and high planes)
        byte_t pattern_lo = ppu_vram_read(ppu, pattern_addr);
        byte_t pattern_hi = ppu_vram_read(ppu, pattern_addr + 8);

        // Get the 2-bit pixel value (bit 7 is leftmost pixel)
        int bit = 7 - tile_x;
        byte_t pixel_lo = (pattern_lo >> bit) & 1;
        byte_t pixel_hi = (pattern_hi >> bit) & 1;
        byte_t pixel_value = (pixel_hi << 1) | pixel_lo;

        // If pixel is not transparent (0), get color from palette
        if (pixel_value != 0) {
            // Attribute table address
            word_t attr_addr = 0x23C0 | (nametable << 10) | ((coarse_y / 4) << 3) | (coarse_x / 4);
            byte_t attr_byte = ppu_vram_read(ppu, attr_addr);

            // Which quadrant of the 32x32 pixel attribute area?
            int attr_shift = ((coarse_y & 2) << 1) | (coarse_x & 2);
            byte_t palette_num = (attr_byte >> attr_shift) & 0x03;

            // Palette address: $3F00 + palette*4 + pixel_value
            word_t pal_addr = 0x3F00 + (palette_num << 2) + pixel_value;
            byte_t color_index = ppu_vram_read(ppu, pal_addr);

            pixel_color = NES_PALETTE[color_index & 0x3F];
        }
    }

    // Write pixel to framebuffer
    ppu->framebuffer[y * PPU_SCREEN_WIDTH + x] = pixel_color;
}

// Increment the horizontal scroll (coarse X and nametable X)
static void increment_scroll_x(ppu_s *ppu)
{
    if (!ppu_get_mask_flag(ppu, PPUMASK_FLAG_BG_ENABLE) &&
        !ppu_get_mask_flag(ppu, PPUMASK_FLAG_SPRITE_ENABLE)) {
        return;  // Rendering disabled
    }

    if ((ppu->vram_addr & 0x001F) == 31) {
        // Coarse X wraps and toggles nametable X
        ppu->vram_addr &= ~0x001F;  // Clear coarse X
        ppu->vram_addr ^= 0x0400;   // Toggle nametable X bit
    } else {
        ppu->vram_addr++;  // Increment coarse X
    }
}

// Increment the vertical scroll (fine Y, coarse Y, nametable Y)
static void increment_scroll_y(ppu_s *ppu)
{
    if (!ppu_get_mask_flag(ppu, PPUMASK_FLAG_BG_ENABLE) &&
        !ppu_get_mask_flag(ppu, PPUMASK_FLAG_SPRITE_ENABLE)) {
        return;  // Rendering disabled
    }

    if ((ppu->vram_addr & 0x7000) != 0x7000) {
        // Fine Y < 7, just increment
        ppu->vram_addr += 0x1000;
    } else {
        // Fine Y wraps to 0
        ppu->vram_addr &= ~0x7000;

        int coarse_y = (ppu->vram_addr >> 5) & 0x1F;
        if (coarse_y == 29) {
            // Row 29 wraps to 0 and toggles nametable Y
            coarse_y = 0;
            ppu->vram_addr ^= 0x0800;  // Toggle nametable Y bit
        } else if (coarse_y == 31) {
            // Row 31 wraps to 0 without toggling
            coarse_y = 0;
        } else {
            coarse_y++;
        }
        ppu->vram_addr = (ppu->vram_addr & ~0x03E0) | (coarse_y << 5);
    }
}

// Copy horizontal bits from temp_addr to vram_addr
static void copy_horizontal_bits(ppu_s *ppu)
{
    if (!ppu_get_mask_flag(ppu, PPUMASK_FLAG_BG_ENABLE) &&
        !ppu_get_mask_flag(ppu, PPUMASK_FLAG_SPRITE_ENABLE)) {
        return;
    }
    // Copy bits: ....A.. ...BCDEF from t to v
    ppu->vram_addr = (ppu->vram_addr & ~0x041F) | (ppu->temp_addr & 0x041F);
}

// Copy vertical bits from temp_addr to vram_addr
static void copy_vertical_bits(ppu_s *ppu)
{
    if (!ppu_get_mask_flag(ppu, PPUMASK_FLAG_BG_ENABLE) &&
        !ppu_get_mask_flag(ppu, PPUMASK_FLAG_SPRITE_ENABLE)) {
        return;
    }
    // Copy bits: GHI A.BC DEF..... from t to v
    ppu->vram_addr = (ppu->vram_addr & ~0x7BE0) | (ppu->temp_addr & 0x7BE0);
}

// =============================================================================
// PPU tick - advance one PPU cycle
// =============================================================================

void ppu_tick(ppu_s *ppu)
{
    assert(ppu != NULL);

    // Advance cycle first (so cycle 1 is the first "active" cycle)
    ppu->cycle++;
    if (ppu->cycle >= PPU_CYCLES_PER_SCANLINE) {
        ppu->cycle = 0;
        ppu->scanline++;
        if (ppu->scanline >= PPU_SCANLINES_PER_FRAME) {
            ppu->scanline = 0;  // Frame complete, start new frame
        }
    }

    // Visible scanlines (0-239): render pixels
    if (ppu->scanline >= 0 && ppu->scanline < 240) {
        // Cycles 1-256: render visible pixels
        if (ppu->cycle >= 1 && ppu->cycle <= 256) {
            render_pixel(ppu);

            // Increment horizontal scroll every 8 cycles (end of each tile)
            if (ppu->cycle % 8 == 0) {
                increment_scroll_x(ppu);
            }
        }

        // Cycle 256: increment vertical scroll
        if (ppu->cycle == 256) {
            increment_scroll_y(ppu);
        }

        // Cycle 257: copy horizontal bits from t to v
        if (ppu->cycle == 257) {
            copy_horizontal_bits(ppu);
        }
    }

    // Pre-render scanline (261): similar to visible but no pixel output
    if (ppu->scanline == PPU_PRERENDER_SCANLINE) {
        // Cycle 1: clear flags
        if (ppu->cycle == 1) {
            ppu_set_status_flag(ppu, PPUSTATUS_FLAG_VBLANK, false);
            ppu_set_status_flag(ppu, PPUSTATUS_FLAG_SPRITE0_HIT, false);
            ppu_set_status_flag(ppu, PPUSTATUS_FLAG_OVERFLOW, false);
        }

        // Cycles 280-304: repeatedly copy vertical bits
        if (ppu->cycle >= 280 && ppu->cycle <= 304) {
            copy_vertical_bits(ppu);
        }

        // Cycle 257: copy horizontal bits
        if (ppu->cycle == 257) {
            copy_horizontal_bits(ppu);
        }
    }

    // VBlank start: scanline 241, cycle 1
    if (ppu->scanline == PPU_VBLANK_SCANLINE && ppu->cycle == 1) {
        ppu_set_status_flag(ppu, PPUSTATUS_FLAG_VBLANK, true);
        ppu->frame_complete = true;  // Signal frame is ready
        if (ppu_get_ctrl_flag(ppu, PPUCTRL_FLAG_NMI_ENABLE)) {
            ppu->nmi_pending = true;
        }
    }
}

// =============================================================================
// Framebuffer access
// =============================================================================

uint32_t *ppu_get_framebuffer(ppu_s *ppu)
{
    assert(ppu != NULL);
    return ppu->framebuffer;
}

bool ppu_frame_complete(ppu_s *ppu)
{
    assert(ppu != NULL);
    if (ppu->frame_complete) {
        ppu->frame_complete = false;
        return true;
    }
    return false;
}
