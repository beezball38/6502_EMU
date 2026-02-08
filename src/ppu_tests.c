#include <string.h>
#include "unity.h"
#include "ppu.h"
#include "bus.h"
#include "cpu_defs.h"


static ppu_s ppu;
static ppu_s *sut = NULL;
static bus_s test_bus;
static ppu_s test_ppu;

void setUp(void) {
    ppu_init(&ppu);
    sut = &ppu;

    
    bus_init(&test_bus);
    ppu_init(&test_ppu);
    test_bus.ppu = &test_ppu;
}

void tearDown(void) {
    sut = NULL;
}


void test_ppu_init_clears_all_registers(void) {
    
    TEST_ASSERT_EQUAL_HEX8(0x00, sut->ctrl_register);
    TEST_ASSERT_EQUAL_HEX8(0x00, sut->mask_register);
    TEST_ASSERT_EQUAL_HEX8(0x00, sut->status_register);
    TEST_ASSERT_EQUAL_HEX8(0x00, sut->oam_addr_register);

    
    TEST_ASSERT_FALSE(sut->write_latch);
    TEST_ASSERT_EQUAL_HEX16(0x0000, sut->vram_addr);
    TEST_ASSERT_EQUAL_HEX16(0x0000, sut->temp_addr);
    TEST_ASSERT_EQUAL_HEX8(0x00, sut->fine_x);
    TEST_ASSERT_EQUAL_HEX8(0x00, sut->data_buffer);

    
    for (int i = 0; i < OAM_SIZE; i++) {
        TEST_ASSERT_EQUAL_HEX8(0x00, sut->oam[i]);
    }
}


void test_ctrl_flag_nametable_x(void) {
    TEST_ASSERT_FALSE(sut->ctrl_register & PPUCTRL_NAMETABLE_X);
    sut->ctrl_register |= PPUCTRL_NAMETABLE_X;
    TEST_ASSERT_TRUE(sut->ctrl_register & PPUCTRL_NAMETABLE_X);
    sut->ctrl_register &= ~PPUCTRL_NAMETABLE_X;
    TEST_ASSERT_FALSE(sut->ctrl_register & PPUCTRL_NAMETABLE_X);
}

void test_ctrl_flag_nametable_y(void) {
    TEST_ASSERT_FALSE(sut->ctrl_register & PPUCTRL_NAMETABLE_Y);
    sut->ctrl_register |= PPUCTRL_NAMETABLE_Y;
    TEST_ASSERT_TRUE(sut->ctrl_register & PPUCTRL_NAMETABLE_Y);
    sut->ctrl_register &= ~PPUCTRL_NAMETABLE_Y;
    TEST_ASSERT_FALSE(sut->ctrl_register & PPUCTRL_NAMETABLE_Y);
}

void test_ctrl_flag_increment(void) {
    TEST_ASSERT_FALSE(sut->ctrl_register & PPUCTRL_INCREMENT);
    sut->ctrl_register |= PPUCTRL_INCREMENT;
    TEST_ASSERT_TRUE(sut->ctrl_register & PPUCTRL_INCREMENT);
    sut->ctrl_register &= ~PPUCTRL_INCREMENT;
    TEST_ASSERT_FALSE(sut->ctrl_register & PPUCTRL_INCREMENT);
}

void test_ctrl_flag_sprite_table(void) {
    TEST_ASSERT_FALSE(sut->ctrl_register & PPUCTRL_SPRITE_TABLE);
    sut->ctrl_register |= PPUCTRL_SPRITE_TABLE;
    TEST_ASSERT_TRUE(sut->ctrl_register & PPUCTRL_SPRITE_TABLE);
    sut->ctrl_register &= ~PPUCTRL_SPRITE_TABLE;
    TEST_ASSERT_FALSE(sut->ctrl_register & PPUCTRL_SPRITE_TABLE);
}

void test_ctrl_flag_bg_table(void) {
    TEST_ASSERT_FALSE(sut->ctrl_register & PPUCTRL_BG_TABLE);
    sut->ctrl_register |= PPUCTRL_BG_TABLE;
    TEST_ASSERT_TRUE(sut->ctrl_register & PPUCTRL_BG_TABLE);
    sut->ctrl_register &= ~PPUCTRL_BG_TABLE;
    TEST_ASSERT_FALSE(sut->ctrl_register & PPUCTRL_BG_TABLE);
}

void test_ctrl_flag_sprite_size(void) {
    TEST_ASSERT_FALSE(sut->ctrl_register & PPUCTRL_SPRITE_SIZE);
    sut->ctrl_register |= PPUCTRL_SPRITE_SIZE;
    TEST_ASSERT_TRUE(sut->ctrl_register & PPUCTRL_SPRITE_SIZE);
    sut->ctrl_register &= ~PPUCTRL_SPRITE_SIZE;
    TEST_ASSERT_FALSE(sut->ctrl_register & PPUCTRL_SPRITE_SIZE);
}

void test_ctrl_flag_master_slave(void) {
    TEST_ASSERT_FALSE(sut->ctrl_register & PPUCTRL_MASTER_SLAVE);
    sut->ctrl_register |= PPUCTRL_MASTER_SLAVE;
    TEST_ASSERT_TRUE(sut->ctrl_register & PPUCTRL_MASTER_SLAVE);
    sut->ctrl_register &= ~PPUCTRL_MASTER_SLAVE;
    TEST_ASSERT_FALSE(sut->ctrl_register & PPUCTRL_MASTER_SLAVE);
}

void test_ctrl_flag_nmi_enable(void) {
    TEST_ASSERT_FALSE(sut->ctrl_register & PPUCTRL_NMI_ENABLE);
    sut->ctrl_register |= PPUCTRL_NMI_ENABLE;
    TEST_ASSERT_TRUE(sut->ctrl_register & PPUCTRL_NMI_ENABLE);
    sut->ctrl_register &= ~PPUCTRL_NMI_ENABLE;
    TEST_ASSERT_FALSE(sut->ctrl_register & PPUCTRL_NMI_ENABLE);
}

void test_ctrl_flags_are_independent(void) {
    
    sut->ctrl_register |= PPUCTRL_NAMETABLE_X;
    sut->ctrl_register |= PPUCTRL_NAMETABLE_Y;
    sut->ctrl_register |= PPUCTRL_INCREMENT;
    sut->ctrl_register |= PPUCTRL_SPRITE_TABLE;
    sut->ctrl_register |= PPUCTRL_BG_TABLE;
    sut->ctrl_register |= PPUCTRL_SPRITE_SIZE;
    sut->ctrl_register |= PPUCTRL_MASTER_SLAVE;
    sut->ctrl_register |= PPUCTRL_NMI_ENABLE;

    TEST_ASSERT_EQUAL_HEX8(0xFF, sut->ctrl_register);

    
    sut->ctrl_register &= ~PPUCTRL_INCREMENT;
    TEST_ASSERT_EQUAL_HEX8(0xFB, sut->ctrl_register);
    TEST_ASSERT_TRUE(sut->ctrl_register & PPUCTRL_NAMETABLE_X);
    TEST_ASSERT_TRUE(sut->ctrl_register & PPUCTRL_NAMETABLE_Y);
    TEST_ASSERT_FALSE(sut->ctrl_register & PPUCTRL_INCREMENT);
    TEST_ASSERT_TRUE(sut->ctrl_register & PPUCTRL_SPRITE_TABLE);
}


void test_mask_flag_grayscale(void) {
    TEST_ASSERT_FALSE(ppu_get_mask_flag(sut, PPUMASK_GRAYSCALE));
    ppu_set_mask_flag(sut, PPUMASK_GRAYSCALE, true);
    TEST_ASSERT_TRUE(ppu_get_mask_flag(sut, PPUMASK_GRAYSCALE));
    ppu_set_mask_flag(sut, PPUMASK_GRAYSCALE, false);
    TEST_ASSERT_FALSE(ppu_get_mask_flag(sut, PPUMASK_GRAYSCALE));
}

void test_mask_flag_bg_left(void) {
    TEST_ASSERT_FALSE(ppu_get_mask_flag(sut, PPUMASK_BG_LEFT));
    ppu_set_mask_flag(sut, PPUMASK_BG_LEFT, true);
    TEST_ASSERT_TRUE(ppu_get_mask_flag(sut, PPUMASK_BG_LEFT));
    ppu_set_mask_flag(sut, PPUMASK_BG_LEFT, false);
    TEST_ASSERT_FALSE(ppu_get_mask_flag(sut, PPUMASK_BG_LEFT));
}

void test_mask_flag_sprite_left(void) {
    TEST_ASSERT_FALSE(ppu_get_mask_flag(sut, PPUMASK_SPRITE_LEFT));
    ppu_set_mask_flag(sut, PPUMASK_SPRITE_LEFT, true);
    TEST_ASSERT_TRUE(ppu_get_mask_flag(sut, PPUMASK_SPRITE_LEFT));
    ppu_set_mask_flag(sut, PPUMASK_SPRITE_LEFT, false);
    TEST_ASSERT_FALSE(ppu_get_mask_flag(sut, PPUMASK_SPRITE_LEFT));
}

void test_mask_flag_bg_enable(void) {
    TEST_ASSERT_FALSE(ppu_get_mask_flag(sut, PPUMASK_BG_ENABLE));
    ppu_set_mask_flag(sut, PPUMASK_BG_ENABLE, true);
    TEST_ASSERT_TRUE(ppu_get_mask_flag(sut, PPUMASK_BG_ENABLE));
    ppu_set_mask_flag(sut, PPUMASK_BG_ENABLE, false);
    TEST_ASSERT_FALSE(ppu_get_mask_flag(sut, PPUMASK_BG_ENABLE));
}

void test_mask_flag_sprite_enable(void) {
    TEST_ASSERT_FALSE(ppu_get_mask_flag(sut, PPUMASK_SPRITE_ENABLE));
    ppu_set_mask_flag(sut, PPUMASK_SPRITE_ENABLE, true);
    TEST_ASSERT_TRUE(ppu_get_mask_flag(sut, PPUMASK_SPRITE_ENABLE));
    ppu_set_mask_flag(sut, PPUMASK_SPRITE_ENABLE, false);
    TEST_ASSERT_FALSE(ppu_get_mask_flag(sut, PPUMASK_SPRITE_ENABLE));
}

void test_mask_flag_emphasize_r(void) {
    TEST_ASSERT_FALSE(ppu_get_mask_flag(sut, PPUMASK_EMPHASIZE_R));
    ppu_set_mask_flag(sut, PPUMASK_EMPHASIZE_R, true);
    TEST_ASSERT_TRUE(ppu_get_mask_flag(sut, PPUMASK_EMPHASIZE_R));
    ppu_set_mask_flag(sut, PPUMASK_EMPHASIZE_R, false);
    TEST_ASSERT_FALSE(ppu_get_mask_flag(sut, PPUMASK_EMPHASIZE_R));
}

void test_mask_flag_emphasize_g(void) {
    TEST_ASSERT_FALSE(ppu_get_mask_flag(sut, PPUMASK_EMPHASIZE_G));
    ppu_set_mask_flag(sut, PPUMASK_EMPHASIZE_G, true);
    TEST_ASSERT_TRUE(ppu_get_mask_flag(sut, PPUMASK_EMPHASIZE_G));
    ppu_set_mask_flag(sut, PPUMASK_EMPHASIZE_G, false);
    TEST_ASSERT_FALSE(ppu_get_mask_flag(sut, PPUMASK_EMPHASIZE_G));
}

void test_mask_flag_emphasize_b(void) {
    TEST_ASSERT_FALSE(ppu_get_mask_flag(sut, PPUMASK_EMPHASIZE_B));
    ppu_set_mask_flag(sut, PPUMASK_EMPHASIZE_B, true);
    TEST_ASSERT_TRUE(ppu_get_mask_flag(sut, PPUMASK_EMPHASIZE_B));
    ppu_set_mask_flag(sut, PPUMASK_EMPHASIZE_B, false);
    TEST_ASSERT_FALSE(ppu_get_mask_flag(sut, PPUMASK_EMPHASIZE_B));
}

void test_mask_flags_are_independent(void) {
    
    ppu_set_mask_flag(sut, PPUMASK_GRAYSCALE, true);
    ppu_set_mask_flag(sut, PPUMASK_BG_LEFT, true);
    ppu_set_mask_flag(sut, PPUMASK_SPRITE_LEFT, true);
    ppu_set_mask_flag(sut, PPUMASK_BG_ENABLE, true);
    ppu_set_mask_flag(sut, PPUMASK_SPRITE_ENABLE, true);
    ppu_set_mask_flag(sut, PPUMASK_EMPHASIZE_R, true);
    ppu_set_mask_flag(sut, PPUMASK_EMPHASIZE_G, true);
    ppu_set_mask_flag(sut, PPUMASK_EMPHASIZE_B, true);

    TEST_ASSERT_EQUAL_HEX8(0xFF, sut->mask_register);

    
    ppu_set_mask_flag(sut, PPUMASK_BG_ENABLE, false);
    TEST_ASSERT_EQUAL_HEX8(0xF7, sut->mask_register);
    TEST_ASSERT_TRUE(ppu_get_mask_flag(sut, PPUMASK_GRAYSCALE));
    TEST_ASSERT_TRUE(ppu_get_mask_flag(sut, PPUMASK_BG_LEFT));
    TEST_ASSERT_TRUE(ppu_get_mask_flag(sut, PPUMASK_SPRITE_LEFT));
    TEST_ASSERT_FALSE(ppu_get_mask_flag(sut, PPUMASK_BG_ENABLE));
    TEST_ASSERT_TRUE(ppu_get_mask_flag(sut, PPUMASK_SPRITE_ENABLE));
}


void test_status_flag_overflow(void) {
    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_OVERFLOW));
    ppu_set_status_flag(sut, PPUSTATUS_OVERFLOW, true);
    TEST_ASSERT_TRUE(ppu_get_status_flag(sut, PPUSTATUS_OVERFLOW));
    ppu_set_status_flag(sut, PPUSTATUS_OVERFLOW, false);
    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_OVERFLOW));
}

void test_status_flag_sprite0_hit(void) {
    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_SPRITE0_HIT));
    ppu_set_status_flag(sut, PPUSTATUS_SPRITE0_HIT, true);
    TEST_ASSERT_TRUE(ppu_get_status_flag(sut, PPUSTATUS_SPRITE0_HIT));
    ppu_set_status_flag(sut, PPUSTATUS_SPRITE0_HIT, false);
    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_SPRITE0_HIT));
}

void test_status_flag_vblank(void) {
    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_VBLANK));
    ppu_set_status_flag(sut, PPUSTATUS_VBLANK, true);
    TEST_ASSERT_TRUE(ppu_get_status_flag(sut, PPUSTATUS_VBLANK));
    ppu_set_status_flag(sut, PPUSTATUS_VBLANK, false);
    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_VBLANK));
}

void test_status_flags_are_independent(void) {
    
    ppu_set_status_flag(sut, PPUSTATUS_OVERFLOW, true);
    ppu_set_status_flag(sut, PPUSTATUS_SPRITE0_HIT, true);
    ppu_set_status_flag(sut, PPUSTATUS_VBLANK, true);

    TEST_ASSERT_EQUAL_HEX8(0xE0, sut->status_register);

    
    ppu_set_status_flag(sut, PPUSTATUS_SPRITE0_HIT, false);
    TEST_ASSERT_EQUAL_HEX8(0xA0, sut->status_register);
    TEST_ASSERT_TRUE(ppu_get_status_flag(sut, PPUSTATUS_OVERFLOW));
    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_SPRITE0_HIT));
    TEST_ASSERT_TRUE(ppu_get_status_flag(sut, PPUSTATUS_VBLANK));
}


void test_read_write_only_registers_return_zero(void) {
    
    sut->ctrl_register = 0xFF;
    sut->mask_register = 0xFF;
    sut->oam_addr_register = 0xFF;

    
    TEST_ASSERT_EQUAL_HEX8(0x00, ppu_read(sut, PPU_REGISTER_CTRL));
    TEST_ASSERT_EQUAL_HEX8(0x00, ppu_read(sut, PPU_REGISTER_MASK));
    TEST_ASSERT_EQUAL_HEX8(0x00, ppu_read(sut, PPU_REGISTER_OAMADDR));
    TEST_ASSERT_EQUAL_HEX8(0x00, ppu_read(sut, PPU_REGISTER_SCROLL));
    TEST_ASSERT_EQUAL_HEX8(0x00, ppu_read(sut, PPU_REGISTER_ADDR));
}


void test_status_read_returns_status_register(void) {
    
    ppu_set_status_flag(sut, PPUSTATUS_VBLANK, true);
    ppu_set_status_flag(sut, PPUSTATUS_SPRITE0_HIT, true);

    byte_t status = ppu_read(sut, PPU_REGISTER_STATUS);
    TEST_ASSERT_EQUAL_HEX8(0xC0, status);
}

void test_status_read_clears_vblank_flag(void) {
    ppu_set_status_flag(sut, PPUSTATUS_VBLANK, true);
    TEST_ASSERT_TRUE(ppu_get_status_flag(sut, PPUSTATUS_VBLANK));

    ppu_read(sut, PPU_REGISTER_STATUS);

    
    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_VBLANK));
}

void test_status_read_does_not_clear_other_flags(void) {
    ppu_set_status_flag(sut, PPUSTATUS_VBLANK, true);
    ppu_set_status_flag(sut, PPUSTATUS_SPRITE0_HIT, true);
    ppu_set_status_flag(sut, PPUSTATUS_OVERFLOW, true);

    ppu_read(sut, PPU_REGISTER_STATUS);

    
    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_VBLANK));
    TEST_ASSERT_TRUE(ppu_get_status_flag(sut, PPUSTATUS_SPRITE0_HIT));
    TEST_ASSERT_TRUE(ppu_get_status_flag(sut, PPUSTATUS_OVERFLOW));
}

void test_status_read_resets_write_latch(void) {
    
    sut->write_latch = true;

    ppu_read(sut, PPU_REGISTER_STATUS);

    TEST_ASSERT_FALSE(sut->write_latch);
}


void test_ctrl_write_sets_register(void) {
    ppu_write(sut, PPU_REGISTER_CTRL, 0x9A);
    TEST_ASSERT_EQUAL_HEX8(0x9A, sut->ctrl_register);
}

void test_ctrl_write_updates_nametable_in_temp_addr(void) {
    
    ppu_write(sut, PPU_REGISTER_CTRL, 0x00);
    TEST_ASSERT_EQUAL_HEX16(0x0000, sut->temp_addr & 0x0C00);

    ppu_write(sut, PPU_REGISTER_CTRL, 0x01);
    TEST_ASSERT_EQUAL_HEX16(0x0400, sut->temp_addr & 0x0C00);

    ppu_write(sut, PPU_REGISTER_CTRL, 0x02);
    TEST_ASSERT_EQUAL_HEX16(0x0800, sut->temp_addr & 0x0C00);

    ppu_write(sut, PPU_REGISTER_CTRL, 0x03);
    TEST_ASSERT_EQUAL_HEX16(0x0C00, sut->temp_addr & 0x0C00);
}

void test_ctrl_write_preserves_other_temp_addr_bits(void) {
    sut->temp_addr = 0x7FFF;
    ppu_write(sut, PPU_REGISTER_CTRL, 0x00);

    
    TEST_ASSERT_EQUAL_HEX16(0x73FF, sut->temp_addr);
}


void test_mask_write_sets_register(void) {
    ppu_write(sut, PPU_REGISTER_MASK, 0x1E);
    TEST_ASSERT_EQUAL_HEX8(0x1E, sut->mask_register);
}


void test_oamaddr_write_sets_address(void) {
    ppu_write(sut, PPU_REGISTER_OAMADDR, 0x42);
    TEST_ASSERT_EQUAL_HEX8(0x42, sut->oam_addr_register);
}

void test_oamdata_read_returns_oam_at_address(void) {
    sut->oam[0x10] = 0xAB;
    sut->oam_addr_register = 0x10;

    TEST_ASSERT_EQUAL_HEX8(0xAB, ppu_read(sut, PPU_REGISTER_OAMDATA));
}

void test_oamdata_write_stores_and_increments(void) {
    sut->oam_addr_register = 0x00;

    ppu_write(sut, PPU_REGISTER_OAMDATA, 0x11);
    TEST_ASSERT_EQUAL_HEX8(0x11, sut->oam[0x00]);
    TEST_ASSERT_EQUAL_HEX8(0x01, sut->oam_addr_register);

    ppu_write(sut, PPU_REGISTER_OAMDATA, 0x22);
    TEST_ASSERT_EQUAL_HEX8(0x22, sut->oam[0x01]);
    TEST_ASSERT_EQUAL_HEX8(0x02, sut->oam_addr_register);
}

void test_oamdata_write_wraps_at_256(void) {
    sut->oam_addr_register = 0xFF;

    ppu_write(sut, PPU_REGISTER_OAMDATA, 0x99);
    TEST_ASSERT_EQUAL_HEX8(0x99, sut->oam[0xFF]);
    TEST_ASSERT_EQUAL_HEX8(0x00, sut->oam_addr_register);
}


void test_scroll_first_write_sets_x_scroll(void) {
    
    
    ppu_write(sut, PPU_REGISTER_SCROLL, 0xD3);

    TEST_ASSERT_EQUAL_HEX8(0x03, sut->fine_x);
    TEST_ASSERT_EQUAL_HEX16(0x001A, sut->temp_addr & 0x001F);
    TEST_ASSERT_TRUE(sut->write_latch);
}

void test_scroll_second_write_sets_y_scroll(void) {
    
    ppu_write(sut, PPU_REGISTER_SCROLL, 0x00);
    
    
    ppu_write(sut, PPU_REGISTER_SCROLL, 0x5F);

    
    TEST_ASSERT_EQUAL_HEX16(0x7000, sut->temp_addr & 0x7000);
    
    TEST_ASSERT_EQUAL_HEX16(0x0160, sut->temp_addr & 0x03E0);
    TEST_ASSERT_FALSE(sut->write_latch);
}

void test_scroll_write_latch_toggles(void) {
    TEST_ASSERT_FALSE(sut->write_latch);

    ppu_write(sut, PPU_REGISTER_SCROLL, 0x00);
    TEST_ASSERT_TRUE(sut->write_latch);

    ppu_write(sut, PPU_REGISTER_SCROLL, 0x00);
    TEST_ASSERT_FALSE(sut->write_latch);

    ppu_write(sut, PPU_REGISTER_SCROLL, 0x00);
    TEST_ASSERT_TRUE(sut->write_latch);
}


void test_addr_first_write_sets_high_byte(void) {
    ppu_write(sut, PPU_REGISTER_ADDR, 0x21);

    
    TEST_ASSERT_EQUAL_HEX16(0x2100, sut->temp_addr & 0x3F00);
    TEST_ASSERT_TRUE(sut->write_latch);
    
    TEST_ASSERT_EQUAL_HEX16(0x0000, sut->vram_addr);
}

void test_addr_second_write_sets_low_byte_and_copies_to_vram(void) {
    ppu_write(sut, PPU_REGISTER_ADDR, 0x21);
    ppu_write(sut, PPU_REGISTER_ADDR, 0x08);

    TEST_ASSERT_EQUAL_HEX16(0x2108, sut->temp_addr);
    TEST_ASSERT_EQUAL_HEX16(0x2108, sut->vram_addr);
    TEST_ASSERT_FALSE(sut->write_latch);
}

void test_addr_high_byte_masks_to_6_bits(void) {
    
    ppu_write(sut, PPU_REGISTER_ADDR, 0xFF);
    ppu_write(sut, PPU_REGISTER_ADDR, 0x00);

    TEST_ASSERT_EQUAL_HEX16(0x3F00, sut->vram_addr);
}


void test_data_read_increments_vram_addr_by_1(void) {
    sut->vram_addr = 0x2000;
    sut->ctrl_register = 0x00;

    ppu_read(sut, PPU_REGISTER_DATA);

    TEST_ASSERT_EQUAL_HEX16(0x2001, sut->vram_addr);
}

void test_data_read_increments_vram_addr_by_32(void) {
    sut->vram_addr = 0x2000;
    sut->ctrl_register = PPUCTRL_INCREMENT;

    ppu_read(sut, PPU_REGISTER_DATA);

    TEST_ASSERT_EQUAL_HEX16(0x2020, sut->vram_addr);
}

void test_data_write_increments_vram_addr_by_1(void) {
    sut->vram_addr = 0x2000;
    sut->ctrl_register = 0x00;

    ppu_write(sut, PPU_REGISTER_DATA, 0x42);

    TEST_ASSERT_EQUAL_HEX16(0x2001, sut->vram_addr);
}

void test_data_write_increments_vram_addr_by_32(void) {
    sut->vram_addr = 0x2000;
    sut->ctrl_register = PPUCTRL_INCREMENT;

    ppu_write(sut, PPU_REGISTER_DATA, 0x42);

    TEST_ASSERT_EQUAL_HEX16(0x2020, sut->vram_addr);
}

void test_data_read_returns_buffered_value(void) {
    sut->data_buffer = 0xAB;

    byte_t result = ppu_read(sut, PPU_REGISTER_DATA);

    TEST_ASSERT_EQUAL_HEX8(0xAB, result);
}


void test_status_read_resets_latch_for_scroll(void) {
    
    ppu_write(sut, PPU_REGISTER_SCROLL, 0x10);
    TEST_ASSERT_TRUE(sut->write_latch);

    
    ppu_read(sut, PPU_REGISTER_STATUS);
    TEST_ASSERT_FALSE(sut->write_latch);

    
    ppu_write(sut, PPU_REGISTER_SCROLL, 0x20);
    TEST_ASSERT_EQUAL_HEX8(0x00, sut->fine_x & 0x07);
}


void test_vram_write_read_nametable(void) {
    
    ppu_set_mirroring(sut, MIRROR_VERTICAL);
    ppu_vram_write(sut, 0x2000, 0x42);
    TEST_ASSERT_EQUAL_HEX8(0x42, ppu_vram_read(sut, 0x2000));

    ppu_vram_write(sut, 0x23FF, 0xAB);
    TEST_ASSERT_EQUAL_HEX8(0xAB, ppu_vram_read(sut, 0x23FF));
}

void test_vram_write_read_palette(void) {
    
    ppu_vram_write(sut, 0x3F00, 0x0D);
    TEST_ASSERT_EQUAL_HEX8(0x0D, ppu_vram_read(sut, 0x3F00));

    ppu_vram_write(sut, 0x3F01, 0x16);
    TEST_ASSERT_EQUAL_HEX8(0x16, ppu_vram_read(sut, 0x3F01));

    ppu_vram_write(sut, 0x3F11, 0x30);
    TEST_ASSERT_EQUAL_HEX8(0x30, ppu_vram_read(sut, 0x3F11));
}

void test_vram_nametable_mirroring_horizontal(void) {
    ppu_set_mirroring(sut, MIRROR_HORIZONTAL);

    
    ppu_vram_write(sut, 0x2000, 0x11);
    TEST_ASSERT_EQUAL_HEX8(0x11, ppu_vram_read(sut, 0x2400));

    ppu_vram_write(sut, 0x2456, 0x22);
    TEST_ASSERT_EQUAL_HEX8(0x22, ppu_vram_read(sut, 0x2056));

    
    ppu_vram_write(sut, 0x2800, 0x33);
    TEST_ASSERT_EQUAL_HEX8(0x33, ppu_vram_read(sut, 0x2C00));

    
    ppu_vram_write(sut, 0x2000, 0xAA);
    ppu_vram_write(sut, 0x2800, 0xBB);
    TEST_ASSERT_EQUAL_HEX8(0xAA, ppu_vram_read(sut, 0x2000));
    TEST_ASSERT_EQUAL_HEX8(0xBB, ppu_vram_read(sut, 0x2800));
}

void test_vram_nametable_mirroring_vertical(void) {
    ppu_set_mirroring(sut, MIRROR_VERTICAL);

    
    ppu_vram_write(sut, 0x2000, 0x11);
    TEST_ASSERT_EQUAL_HEX8(0x11, ppu_vram_read(sut, 0x2800));

    
    ppu_vram_write(sut, 0x2400, 0x22);
    TEST_ASSERT_EQUAL_HEX8(0x22, ppu_vram_read(sut, 0x2C00));

    
    ppu_vram_write(sut, 0x2000, 0xAA);
    ppu_vram_write(sut, 0x2400, 0xBB);
    TEST_ASSERT_EQUAL_HEX8(0xAA, ppu_vram_read(sut, 0x2000));
    TEST_ASSERT_EQUAL_HEX8(0xBB, ppu_vram_read(sut, 0x2400));
}

void test_vram_palette_mirroring(void) {
    
    ppu_vram_write(sut, 0x3F00, 0x0D);
    TEST_ASSERT_EQUAL_HEX8(0x0D, ppu_vram_read(sut, 0x3F10));

    ppu_vram_write(sut, 0x3F14, 0x2D);
    TEST_ASSERT_EQUAL_HEX8(0x2D, ppu_vram_read(sut, 0x3F04));

    
    ppu_vram_write(sut, 0x3F05, 0x15);
    TEST_ASSERT_EQUAL_HEX8(0x15, ppu_vram_read(sut, 0x3F25));
}

void test_vram_nametable_mirror_at_3000(void) {
    
    ppu_set_mirroring(sut, MIRROR_VERTICAL);

    ppu_vram_write(sut, 0x2000, 0x42);
    TEST_ASSERT_EQUAL_HEX8(0x42, ppu_vram_read(sut, 0x3000));

    ppu_vram_write(sut, 0x30AB, 0x99);
    TEST_ASSERT_EQUAL_HEX8(0x99, ppu_vram_read(sut, 0x20AB));
}


void test_ppudata_write_to_nametable(void) {
    ppu_set_mirroring(sut, MIRROR_VERTICAL);

    
    ppu_write(sut, PPU_REGISTER_ADDR, 0x20);
    ppu_write(sut, PPU_REGISTER_ADDR, 0x00);

    
    ppu_write(sut, PPU_REGISTER_DATA, 0x42);

    
    TEST_ASSERT_EQUAL_HEX8(0x42, ppu_vram_read(sut, 0x2000));
}

void test_ppudata_read_from_nametable_is_buffered(void) {
    ppu_set_mirroring(sut, MIRROR_VERTICAL);

    
    ppu_vram_write(sut, 0x2000, 0xAA);
    ppu_vram_write(sut, 0x2001, 0xBB);

    
    ppu_write(sut, PPU_REGISTER_ADDR, 0x20);
    ppu_write(sut, PPU_REGISTER_ADDR, 0x00);

    
    byte_t first = ppu_read(sut, PPU_REGISTER_DATA);
    TEST_ASSERT_EQUAL_HEX8(0x00, first);

    
    byte_t second = ppu_read(sut, PPU_REGISTER_DATA);
    TEST_ASSERT_EQUAL_HEX8(0xAA, second);
}

void test_ppudata_read_from_palette_is_not_buffered(void) {
    
    ppu_vram_write(sut, 0x3F00, 0x0D);
    ppu_vram_write(sut, 0x3F01, 0x16);

    
    ppu_write(sut, PPU_REGISTER_ADDR, 0x3F);
    ppu_write(sut, PPU_REGISTER_ADDR, 0x00);

    
    byte_t first = ppu_read(sut, PPU_REGISTER_DATA);
    TEST_ASSERT_EQUAL_HEX8(0x0D, first);

    byte_t second = ppu_read(sut, PPU_REGISTER_DATA);
    TEST_ASSERT_EQUAL_HEX8(0x16, second);
}

void test_ppudata_increments_vram_addr(void) {
    
    ppu_write(sut, PPU_REGISTER_ADDR, 0x20);
    ppu_write(sut, PPU_REGISTER_ADDR, 0x00);

    
    ppu_write(sut, PPU_REGISTER_DATA, 0x11);
    TEST_ASSERT_EQUAL_HEX16(0x2001, sut->vram_addr);

    ppu_write(sut, PPU_REGISTER_DATA, 0x22);
    TEST_ASSERT_EQUAL_HEX16(0x2002, sut->vram_addr);

    
    ppu_write(sut, PPU_REGISTER_CTRL, PPUCTRL_INCREMENT);

    ppu_write(sut, PPU_REGISTER_DATA, 0x33);
    TEST_ASSERT_EQUAL_HEX16(0x2022, sut->vram_addr);
}


static byte_t test_chr_rom[8192];

void test_chr_rom_read(void) {
    
    test_chr_rom[0x0000] = 0x11;
    test_chr_rom[0x0100] = 0x22;
    test_chr_rom[0x1000] = 0x33;
    test_chr_rom[0x1FFF] = 0x44;

    ppu_load_chr_rom(sut, test_chr_rom, sizeof(test_chr_rom));

    TEST_ASSERT_EQUAL_HEX8(0x11, ppu_vram_read(sut, 0x0000));
    TEST_ASSERT_EQUAL_HEX8(0x22, ppu_vram_read(sut, 0x0100));
    TEST_ASSERT_EQUAL_HEX8(0x33, ppu_vram_read(sut, 0x1000));
    TEST_ASSERT_EQUAL_HEX8(0x44, ppu_vram_read(sut, 0x1FFF));
}


void test_ppu_cycle_increments(void) {
    TEST_ASSERT_EQUAL_INT(0, sut->cycle);
    ppu_tick(sut);
    TEST_ASSERT_EQUAL_INT(1, sut->cycle);
    ppu_tick(sut);
    TEST_ASSERT_EQUAL_INT(2, sut->cycle);
}

void test_ppu_scanline_increments_after_341_cycles(void) {
    sut->scanline = 0;
    sut->cycle = 0;

    
    for (int i = 0; i < 340; i++) {
        ppu_tick(sut);
    }
    TEST_ASSERT_EQUAL_INT(0, sut->scanline);
    TEST_ASSERT_EQUAL_INT(340, sut->cycle);

    
    ppu_tick(sut);
    TEST_ASSERT_EQUAL_INT(1, sut->scanline);
    TEST_ASSERT_EQUAL_INT(0, sut->cycle);
}

void test_ppu_frame_completes_after_262_scanlines(void) {
    sut->scanline = 261;
    sut->cycle = 340;

    ppu_tick(sut);

    
    TEST_ASSERT_EQUAL_INT(0, sut->scanline);
    TEST_ASSERT_EQUAL_INT(0, sut->cycle);
}

void test_vblank_flag_set_at_scanline_241(void) {
    sut->scanline = 241;
    sut->cycle = 0;
    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_VBLANK));

    ppu_tick(sut);

    TEST_ASSERT_TRUE(ppu_get_status_flag(sut, PPUSTATUS_VBLANK));
}

void test_vblank_flag_cleared_at_prerender(void) {
    
    ppu_set_status_flag(sut, PPUSTATUS_VBLANK, true);
    ppu_set_status_flag(sut, PPUSTATUS_SPRITE0_HIT, true);
    ppu_set_status_flag(sut, PPUSTATUS_OVERFLOW, true);

    sut->scanline = 261;
    sut->cycle = 0;

    ppu_tick(sut);

    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_VBLANK));
    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_SPRITE0_HIT));
    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_OVERFLOW));
}

void test_nmi_triggered_when_vblank_and_nmi_enabled(void) {
    
    ppu_set_ctrl_flag(sut, PPUCTRL_NMI_ENABLE, true);

    sut->scanline = 241;
    sut->cycle = 0;
    sut->nmi_pending = false;

    ppu_tick(sut);

    TEST_ASSERT_TRUE(sut->nmi_pending);
}

void test_nmi_not_triggered_when_nmi_disabled(void) {
    
    TEST_ASSERT_FALSE(ppu_get_ctrl_flag(sut, PPUCTRL_NMI_ENABLE));

    sut->scanline = 241;
    sut->cycle = 0;
    sut->nmi_pending = false;

    ppu_tick(sut);

    TEST_ASSERT_FALSE(sut->nmi_pending);
    TEST_ASSERT_TRUE(ppu_get_status_flag(sut, PPUSTATUS_VBLANK));
}


static bus_s test_bus;

void test_oam_dma_copies_256_bytes(void) {
    bus_init(&test_bus);
    test_bus.ppu = &test_ppu;

    for (int i = 0; i < 256; i++) {
        test_bus.ram[0x0200 + i] = (byte_t)i;
    }

    
    bus_oam_dma(&test_bus, 0x02);

    
    for (int i = 0; i < 256; i++) {
        TEST_ASSERT_EQUAL_HEX8((byte_t)i, test_bus.ppu->oam[i]);
    }
}

void test_oam_dma_reads_from_correct_page(void) {
    bus_init(&test_bus);
    test_bus.ppu = &test_ppu;

    for (int i = 0; i < 256; i++) {
        test_bus.ram[0x0000 + i] = 0xAA;
        test_bus.ram[0x0100 + i] = 0xBB;
        test_bus.ram[0x0200 + i] = 0xCC;
    }

    
    bus_oam_dma(&test_bus, 0x01);

    
    TEST_ASSERT_EQUAL_HEX8(0xBB, test_bus.ppu->oam[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, test_bus.ppu->oam[127]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, test_bus.ppu->oam[255]);
}

void test_oam_dma_via_bus_write(void) {
    bus_init(&test_bus);
    test_bus.ppu = &test_ppu;

    for (int i = 0; i < 256; i++) {
        test_bus.ram[0x0300 + i] = (byte_t)(255 - i);
    }

    
    bus_write(&test_bus, 0x4014, 0x03);

    
    TEST_ASSERT_EQUAL_HEX8(0xFF, test_bus.ppu->oam[0]);
    TEST_ASSERT_EQUAL_HEX8(0xFE, test_bus.ppu->oam[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00, test_bus.ppu->oam[255]);
}

void test_oam_dma_sets_cycle_count(void) {
    bus_init(&test_bus);
    test_bus.ppu = &test_ppu;

    bus_oam_dma(&test_bus, 0x00);

    
    TEST_ASSERT_EQUAL_UINT16(513, test_bus.oam_dma_cycles);
}


void test_vram_addr_wraps_at_4000(void) {
    
    ppu_write(sut, PPU_REGISTER_ADDR, 0x3F);
    ppu_write(sut, PPU_REGISTER_ADDR, 0xFF);
    TEST_ASSERT_EQUAL_HEX16(0x3FFF, sut->vram_addr);

    
    ppu_write(sut, PPU_REGISTER_DATA, 0x42);
    TEST_ASSERT_EQUAL_HEX16(0x0000, sut->vram_addr);
}

void test_vram_addr_wraps_on_read(void) {
    sut->vram_addr = 0x3FFF;
    sut->ctrl_register = 0;

    ppu_read(sut, PPU_REGISTER_DATA);

    TEST_ASSERT_EQUAL_HEX16(0x0000, sut->vram_addr);
}

void test_vram_addr_wraps_with_32_increment(void) {
    
    sut->vram_addr = 0x3FF0;
    sut->ctrl_register = PPUCTRL_INCREMENT;

    ppu_write(sut, PPU_REGISTER_DATA, 0x42);

    
    TEST_ASSERT_EQUAL_HEX16(0x0010, sut->vram_addr);
}

void test_nmi_triggered_when_enabling_during_vblank(void) {
    
    ppu_set_status_flag(sut, PPUSTATUS_VBLANK, true);

    
    TEST_ASSERT_FALSE(ppu_get_ctrl_flag(sut, PPUCTRL_NMI_ENABLE));
    sut->nmi_pending = false;

    
    ppu_write(sut, PPU_REGISTER_CTRL, PPUCTRL_NMI_ENABLE);

    TEST_ASSERT_TRUE(sut->nmi_pending);
}

void test_nmi_not_triggered_when_already_enabled(void) {
    
    ppu_set_status_flag(sut, PPUSTATUS_VBLANK, true);
    sut->ctrl_register = PPUCTRL_NMI_ENABLE;
    sut->nmi_pending = false;

    
    ppu_write(sut, PPU_REGISTER_CTRL, PPUCTRL_NMI_ENABLE);

    TEST_ASSERT_FALSE(sut->nmi_pending);
}

void test_palette_mirroring_beyond_3f1f(void) {
    
    ppu_vram_write(sut, 0x3F05, 0xAB);

    
    TEST_ASSERT_EQUAL_HEX8(0xAB, ppu_vram_read(sut, 0x3F25));
    
    TEST_ASSERT_EQUAL_HEX8(0xAB, ppu_vram_read(sut, 0x3F45));
    
    TEST_ASSERT_EQUAL_HEX8(0xAB, ppu_vram_read(sut, 0x3FE5));
}

void test_ppu_init_starts_at_prerender_scanline(void) {
    ppu_init(&test_ppu);

    TEST_ASSERT_EQUAL_INT(261, test_ppu.scanline);
    TEST_ASSERT_EQUAL_INT(0, test_ppu.cycle);
}

void test_sprite_palette_mirrors_to_bg_palette(void) {
    
    ppu_vram_write(sut, 0x3F00, 0x0F);
    TEST_ASSERT_EQUAL_HEX8(0x0F, ppu_vram_read(sut, 0x3F10));

    
    ppu_vram_write(sut, 0x3F10, 0x1D);
    TEST_ASSERT_EQUAL_HEX8(0x1D, ppu_vram_read(sut, 0x3F00));

    
    ppu_vram_write(sut, 0x3F04, 0x21);
    TEST_ASSERT_EQUAL_HEX8(0x21, ppu_vram_read(sut, 0x3F14));

    ppu_vram_write(sut, 0x3F18, 0x31);
    TEST_ASSERT_EQUAL_HEX8(0x31, ppu_vram_read(sut, 0x3F08));
}


int main(void) {
    UNITY_BEGIN();

    
    RUN_TEST(test_ppu_init_clears_all_registers);

    
    RUN_TEST(test_ctrl_flag_nametable_x);
    RUN_TEST(test_ctrl_flag_nametable_y);
    RUN_TEST(test_ctrl_flag_increment);
    RUN_TEST(test_ctrl_flag_sprite_table);
    RUN_TEST(test_ctrl_flag_bg_table);
    RUN_TEST(test_ctrl_flag_sprite_size);
    RUN_TEST(test_ctrl_flag_master_slave);
    RUN_TEST(test_ctrl_flag_nmi_enable);
    RUN_TEST(test_ctrl_flags_are_independent);

    
    RUN_TEST(test_mask_flag_grayscale);
    RUN_TEST(test_mask_flag_bg_left);
    RUN_TEST(test_mask_flag_sprite_left);
    RUN_TEST(test_mask_flag_bg_enable);
    RUN_TEST(test_mask_flag_sprite_enable);
    RUN_TEST(test_mask_flag_emphasize_r);
    RUN_TEST(test_mask_flag_emphasize_g);
    RUN_TEST(test_mask_flag_emphasize_b);
    RUN_TEST(test_mask_flags_are_independent);

    
    RUN_TEST(test_status_flag_overflow);
    RUN_TEST(test_status_flag_sprite0_hit);
    RUN_TEST(test_status_flag_vblank);
    RUN_TEST(test_status_flags_are_independent);

    
    RUN_TEST(test_read_write_only_registers_return_zero);

    
    RUN_TEST(test_status_read_returns_status_register);
    RUN_TEST(test_status_read_clears_vblank_flag);
    RUN_TEST(test_status_read_does_not_clear_other_flags);
    RUN_TEST(test_status_read_resets_write_latch);

    
    RUN_TEST(test_ctrl_write_sets_register);
    RUN_TEST(test_ctrl_write_updates_nametable_in_temp_addr);
    RUN_TEST(test_ctrl_write_preserves_other_temp_addr_bits);

    
    RUN_TEST(test_mask_write_sets_register);

    
    RUN_TEST(test_oamaddr_write_sets_address);
    RUN_TEST(test_oamdata_read_returns_oam_at_address);
    RUN_TEST(test_oamdata_write_stores_and_increments);
    RUN_TEST(test_oamdata_write_wraps_at_256);

    
    RUN_TEST(test_scroll_first_write_sets_x_scroll);
    RUN_TEST(test_scroll_second_write_sets_y_scroll);
    RUN_TEST(test_scroll_write_latch_toggles);

    
    RUN_TEST(test_addr_first_write_sets_high_byte);
    RUN_TEST(test_addr_second_write_sets_low_byte_and_copies_to_vram);
    RUN_TEST(test_addr_high_byte_masks_to_6_bits);

    
    RUN_TEST(test_data_read_increments_vram_addr_by_1);
    RUN_TEST(test_data_read_increments_vram_addr_by_32);
    RUN_TEST(test_data_write_increments_vram_addr_by_1);
    RUN_TEST(test_data_write_increments_vram_addr_by_32);
    RUN_TEST(test_data_read_returns_buffered_value);

    
    RUN_TEST(test_status_read_resets_latch_for_scroll);

    
    RUN_TEST(test_vram_write_read_nametable);
    RUN_TEST(test_vram_write_read_palette);
    RUN_TEST(test_vram_nametable_mirroring_horizontal);
    RUN_TEST(test_vram_nametable_mirroring_vertical);
    RUN_TEST(test_vram_palette_mirroring);
    RUN_TEST(test_vram_nametable_mirror_at_3000);

    
    RUN_TEST(test_ppudata_write_to_nametable);
    RUN_TEST(test_ppudata_read_from_nametable_is_buffered);
    RUN_TEST(test_ppudata_read_from_palette_is_not_buffered);
    RUN_TEST(test_ppudata_increments_vram_addr);

    
    RUN_TEST(test_chr_rom_read);

    
    RUN_TEST(test_ppu_cycle_increments);
    RUN_TEST(test_ppu_scanline_increments_after_341_cycles);
    RUN_TEST(test_ppu_frame_completes_after_262_scanlines);
    RUN_TEST(test_vblank_flag_set_at_scanline_241);
    RUN_TEST(test_vblank_flag_cleared_at_prerender);
    RUN_TEST(test_nmi_triggered_when_vblank_and_nmi_enabled);
    RUN_TEST(test_nmi_not_triggered_when_nmi_disabled);

    
    RUN_TEST(test_oam_dma_copies_256_bytes);
    RUN_TEST(test_oam_dma_reads_from_correct_page);
    RUN_TEST(test_oam_dma_via_bus_write);
    RUN_TEST(test_oam_dma_sets_cycle_count);

    
    RUN_TEST(test_vram_addr_wraps_at_4000);
    RUN_TEST(test_vram_addr_wraps_on_read);
    RUN_TEST(test_vram_addr_wraps_with_32_increment);
    RUN_TEST(test_nmi_triggered_when_enabling_during_vblank);
    RUN_TEST(test_nmi_not_triggered_when_already_enabled);
    RUN_TEST(test_palette_mirroring_beyond_3f1f);
    RUN_TEST(test_ppu_init_starts_at_prerender_scanline);
    RUN_TEST(test_sprite_palette_mirrors_to_bg_palette);

    return UNITY_END();
}
