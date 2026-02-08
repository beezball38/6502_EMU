#include "ppu.h"
#include <string.h>
#include <assert.h>

static ppu_s s_ppu;

ppu_s* ppu_get_instance(void)
{
    return &s_ppu;
}


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


void ppu_init(ppu_s *ppu)
{
    assert(ppu != NULL);
    memset(ppu, 0, sizeof(*ppu));
    ppu->scanline = 261;
}


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

static word_t mirror_nametable_addr(ppu_s *ppu, word_t addr)
{
    addr &= 0x0FFF;

    switch (ppu->mirroring) {
        case MIRROR_HORIZONTAL:
            if (addr < 0x800) {
                return addr & 0x3FF;
            } else {
                return 0x400 + (addr & 0x3FF);
            }

        case MIRROR_VERTICAL:
            return addr & 0x7FF;

        case MIRROR_SINGLE_LOW:
            return addr & 0x3FF;

        case MIRROR_SINGLE_HIGH:
            
            return 0x400 + (addr & 0x3FF);

        case MIRROR_FOUR_SCREEN:
            
            
            return addr & 0x7FF;

        default:
            return addr & 0x7FF;
    }
}


byte_t ppu_vram_read(ppu_s *ppu, word_t addr)
{
    assert(ppu != NULL);
    addr &= 0x3FFF;

    if (addr < 0x2000) {
        
        if (ppu->chr_rom != NULL && ppu->chr_rom_size > 0) {
            return ppu->chr_rom[addr % ppu->chr_rom_size];
        }
        return 0;
    }
    else if (addr < 0x3F00) {
        
        word_t nt_addr = addr;
        if (addr >= 0x3000) {
            nt_addr = addr - 0x1000;
        }
        return ppu->vram[mirror_nametable_addr(ppu, nt_addr)];
    }
    else {
        
        word_t pal_addr = (addr - 0x3F00) & 0x1F;

        
        if ((pal_addr & 0x13) == 0x10) {
            pal_addr &= 0x0F;
        }
        return ppu->palette[pal_addr];
    }
}

void ppu_vram_write(ppu_s *ppu, word_t addr, byte_t value)
{
    assert(ppu != NULL);
    addr &= 0x3FFF;

    if (addr < 0x2000) {
        
        return;
    }
    else if (addr < 0x3F00) {
        
        word_t nt_addr = addr;
        if (addr >= 0x3000) {
            nt_addr = addr - 0x1000;
        }
        ppu->vram[mirror_nametable_addr(ppu, nt_addr)] = value;
    }
    else {
        
        word_t pal_addr = (addr - 0x3F00) & 0x1F;

        
        if ((pal_addr & 0x13) == 0x10) {
            pal_addr &= 0x0F;
        }
        ppu->palette[pal_addr] = value;
    }
}


byte_t ppu_read(ppu_s *ppu, ppu_register_e reg)
{
    assert(ppu != NULL);

    switch (reg) {
        case PPU_REGISTER_CTRL:
            
            return 0;

        case PPU_REGISTER_MASK:
            
            return 0;

        case PPU_REGISTER_STATUS: {
            
            byte_t status = ppu->status_register;
            ppu_set_status_flag(ppu, PPUSTATUS_VBLANK, false);
            ppu->write_latch = false;
            return status;
        }

        case PPU_REGISTER_OAMADDR:
            
            return 0;

        case PPU_REGISTER_OAMDATA:
            
            return ppu->oam[ppu->oam_addr_register];

        case PPU_REGISTER_SCROLL:
            
            return 0;

        case PPU_REGISTER_ADDR:
            
            return 0;

        case PPU_REGISTER_DATA: {
            
            word_t addr = ppu->vram_addr & 0x3FFF;
            byte_t data;

            if (addr >= 0x3F00) {
                
                data = ppu_vram_read(ppu, addr);
                ppu->data_buffer = ppu_vram_read(ppu, addr - 0x1000);
            } else {
                
                data = ppu->data_buffer;
                ppu->data_buffer = ppu_vram_read(ppu, addr);
            }

            
            ppu->vram_addr += (ppu->ctrl_register & PPUCTRL_INCREMENT) ? 32 : 1;
            ppu->vram_addr &= 0x3FFF;
            return data;
        }

        default:
            return 0;
    }
}


void ppu_write(ppu_s *ppu, ppu_register_e reg, byte_t value)
{
    assert(ppu != NULL);

    switch (reg) {
        case PPU_REGISTER_CTRL: {
            byte_t prev_nmi = ppu->ctrl_register & PPUCTRL_NMI_ENABLE;
            ppu->ctrl_register = value;
            
            ppu->temp_addr = (ppu->temp_addr & 0xF3FF) | ((value & 0x03) << 10);

            
            if (!prev_nmi && (value & PPUCTRL_NMI_ENABLE) &&
                ppu_get_status_flag(ppu, PPUSTATUS_VBLANK)) {
                ppu->nmi_pending = true;
            }
            break;
        }

        case PPU_REGISTER_MASK:
            ppu->mask_register = value;
            break;

        case PPU_REGISTER_STATUS:
            
            break;

        case PPU_REGISTER_OAMADDR:
            ppu->oam_addr_register = value;
            break;

        case PPU_REGISTER_OAMDATA:
            ppu->oam[ppu->oam_addr_register] = value;
            ppu->oam_addr_register++;
            break;

        case PPU_REGISTER_SCROLL:
            if (!ppu->write_latch) {
                
                ppu->fine_x = value & 0x07;
                ppu->temp_addr = (ppu->temp_addr & 0xFFE0) | (value >> 3);
            } else {
                
                ppu->temp_addr = (ppu->temp_addr & 0x8C1F) |
                                 ((value & 0x07) << 12) |
                                 ((value >> 3) << 5);
            }
            ppu->write_latch = !ppu->write_latch;
            break;

        case PPU_REGISTER_ADDR:
            if (!ppu->write_latch) {
                
                ppu->temp_addr = (ppu->temp_addr & 0x00FF) | ((value & 0x3F) << 8);
            } else {
                
                ppu->temp_addr = (ppu->temp_addr & 0xFF00) | value;
                ppu->vram_addr = ppu->temp_addr;
            }
            ppu->write_latch = !ppu->write_latch;
            break;

        case PPU_REGISTER_DATA:
            
            ppu_vram_write(ppu, ppu->vram_addr, value);
            
            ppu->vram_addr += (ppu->ctrl_register & PPUCTRL_INCREMENT) ? 32 : 1;
            ppu->vram_addr &= 0x3FFF;
            break;

        default:
            break;
    }
}


#define PPU_CYCLES_PER_SCANLINE  341
#define PPU_SCANLINES_PER_FRAME  262
#define PPU_VBLANK_SCANLINE      241
#define PPU_PRERENDER_SCANLINE   261


static void render_pixel(ppu_s *ppu)
{
    int x = ppu->cycle - 1;
    int y = ppu->scanline;

    if (x < 0 || x >= PPU_SCREEN_WIDTH || y < 0 || y >= PPU_SCREEN_HEIGHT) {
        return;
    }

    uint32_t pixel_color = NES_PALETTE[ppu->palette[0] & 0x3F];

    
    bool bg_enabled = ppu_get_mask_flag(ppu, PPUMASK_BG_ENABLE);
    bool bg_left_enabled = ppu_get_mask_flag(ppu, PPUMASK_BG_LEFT);

    if (bg_enabled && (x >= 8 || bg_left_enabled)) {
        
        word_t v = ppu->vram_addr;
        int coarse_x = v & 0x1F;
        int coarse_y = (v >> 5) & 0x1F;
        int fine_y = (v >> 12) & 0x07;
        int nametable = (v >> 10) & 0x03;
        int fine_x_scroll = ppu->fine_x;

        
        int tile_x = (x + fine_x_scroll) % 8;

        
        word_t nt_addr = 0x2000 | (nametable << 10) | (coarse_y << 5) | coarse_x;
        byte_t tile_index = ppu_vram_read(ppu, nt_addr);

        
        word_t pattern_base = (ppu->ctrl_register & PPUCTRL_BG_TABLE) ? 0x1000 : 0x0000;
        word_t pattern_addr = pattern_base + (tile_index * 16) + fine_y;

        
        byte_t pattern_lo = ppu_vram_read(ppu, pattern_addr);
        byte_t pattern_hi = ppu_vram_read(ppu, pattern_addr + 8);

        
        int bit = 7 - tile_x;
        byte_t pixel_lo = (pattern_lo >> bit) & 1;
        byte_t pixel_hi = (pattern_hi >> bit) & 1;
        byte_t pixel_value = (pixel_hi << 1) | pixel_lo;

        
        if (pixel_value != 0) {
            
            word_t attr_addr = 0x23C0 | (nametable << 10) | ((coarse_y / 4) << 3) | (coarse_x / 4);
            byte_t attr_byte = ppu_vram_read(ppu, attr_addr);

            
            int attr_shift = ((coarse_y & 2) << 1) | (coarse_x & 2);
            byte_t palette_num = (attr_byte >> attr_shift) & 0x03;

            
            word_t pal_addr = 0x3F00 + (palette_num << 2) + pixel_value;
            byte_t color_index = ppu_vram_read(ppu, pal_addr);

            pixel_color = NES_PALETTE[color_index & 0x3F];
        }
    }

    
    ppu->framebuffer[y * PPU_SCREEN_WIDTH + x] = pixel_color;
}


static void increment_scroll_x(ppu_s *ppu)
{
    if (!ppu_get_mask_flag(ppu, PPUMASK_BG_ENABLE) &&
        !ppu_get_mask_flag(ppu, PPUMASK_SPRITE_ENABLE)) {
        return;
    }

    if ((ppu->vram_addr & 0x001F) == 31) {
        
        ppu->vram_addr &= ~0x001F;
        ppu->vram_addr ^= 0x0400;
    } else {
        ppu->vram_addr++;
    }
}


static void increment_scroll_y(ppu_s *ppu)
{
    if (!ppu_get_mask_flag(ppu, PPUMASK_BG_ENABLE) &&
        !ppu_get_mask_flag(ppu, PPUMASK_SPRITE_ENABLE)) {
        return;
    }

    if ((ppu->vram_addr & 0x7000) != 0x7000) {
        
        ppu->vram_addr += 0x1000;
    } else {
        
        ppu->vram_addr &= ~0x7000;

        int coarse_y = (ppu->vram_addr >> 5) & 0x1F;
        if (coarse_y == 29) {
            
            coarse_y = 0;
            ppu->vram_addr ^= 0x0800;
        } else if (coarse_y == 31) {
            
            coarse_y = 0;
        } else {
            coarse_y++;
        }
        ppu->vram_addr = (ppu->vram_addr & ~0x03E0) | (coarse_y << 5);
    }
}


static void copy_horizontal_bits(ppu_s *ppu)
{
    if (!ppu_get_mask_flag(ppu, PPUMASK_BG_ENABLE) &&
        !ppu_get_mask_flag(ppu, PPUMASK_SPRITE_ENABLE)) {
        return;
    }
    
    ppu->vram_addr = (ppu->vram_addr & ~0x041F) | (ppu->temp_addr & 0x041F);
}


static void copy_vertical_bits(ppu_s *ppu)
{
    if (!ppu_get_mask_flag(ppu, PPUMASK_BG_ENABLE) &&
        !ppu_get_mask_flag(ppu, PPUMASK_SPRITE_ENABLE)) {
        return;
    }
    
    ppu->vram_addr = (ppu->vram_addr & ~0x7BE0) | (ppu->temp_addr & 0x7BE0);
}


void ppu_tick(ppu_s *ppu)
{
    assert(ppu != NULL);

    
    ppu->cycle++;
    if (ppu->cycle >= PPU_CYCLES_PER_SCANLINE) {
        ppu->cycle = 0;
        ppu->scanline++;
        if (ppu->scanline >= PPU_SCANLINES_PER_FRAME) {
            ppu->scanline = 0;
        }
    }

    
    if (ppu->scanline >= 0 && ppu->scanline < 240) {
        
        if (ppu->cycle >= 1 && ppu->cycle <= 256) {
            render_pixel(ppu);

            
            if (ppu->cycle % 8 == 0) {
                increment_scroll_x(ppu);
            }
        }

        
        if (ppu->cycle == 256) {
            increment_scroll_y(ppu);
        }

        
        if (ppu->cycle == 257) {
            copy_horizontal_bits(ppu);
        }
    }

    
    if (ppu->scanline == PPU_PRERENDER_SCANLINE) {
        
        if (ppu->cycle == 1) {
            ppu_set_status_flag(ppu, PPUSTATUS_VBLANK, false);
            ppu_set_status_flag(ppu, PPUSTATUS_SPRITE0_HIT, false);
            ppu_set_status_flag(ppu, PPUSTATUS_OVERFLOW, false);
        }

        
        if (ppu->cycle >= 280 && ppu->cycle <= 304) {
            copy_vertical_bits(ppu);
        }

        
        if (ppu->cycle == 257) {
            copy_horizontal_bits(ppu);
        }
    }

    
    if (ppu->scanline == PPU_VBLANK_SCANLINE && ppu->cycle == 1) {
        ppu_set_status_flag(ppu, PPUSTATUS_VBLANK, true);
        ppu->frame_complete = true;
        if (ppu->ctrl_register & PPUCTRL_NMI_ENABLE) {
            ppu->nmi_pending = true;
        }
    }
}


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
