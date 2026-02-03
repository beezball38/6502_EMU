// PPU unit tests
#include <string.h>
#include "unity.h"
#include "ppu.h"
#include "bus.h"
#include "cpu_defs.h"

// System under test
static ppu_s ppu;
static ppu_s *sut = NULL;
static bus_s test_bus;
static ppu_s test_ppu;

void setUp(void) {
    ppu_init(&ppu);
    sut = &ppu;

    // Initialize test bus and attach a PPU instance for OAM DMA tests
    bus_init(&test_bus);
    ppu_init(&test_ppu);
    test_bus.ppu = &test_ppu;
}

void tearDown(void) {
    sut = NULL;
}

// =============================================================================
// Initialization tests
// =============================================================================

void test_ppu_init_clears_all_registers(void) {
    // All registers should be zero
    TEST_ASSERT_EQUAL_HEX8(0x00, sut->ctrl_register);
    TEST_ASSERT_EQUAL_HEX8(0x00, sut->mask_register);
    TEST_ASSERT_EQUAL_HEX8(0x00, sut->status_register);
    TEST_ASSERT_EQUAL_HEX8(0x00, sut->oam_addr_register);

    // Internal state should be zero
    TEST_ASSERT_FALSE(sut->write_latch);
    TEST_ASSERT_EQUAL_HEX16(0x0000, sut->vram_addr);
    TEST_ASSERT_EQUAL_HEX16(0x0000, sut->temp_addr);
    TEST_ASSERT_EQUAL_HEX8(0x00, sut->fine_x);
    TEST_ASSERT_EQUAL_HEX8(0x00, sut->data_buffer);

    // OAM should be zeroed
    for (int i = 0; i < OAM_SIZE; i++) {
        TEST_ASSERT_EQUAL_HEX8(0x00, sut->oam[i]);
    }
}

// =============================================================================
// PPUCTRL flag tests
// =============================================================================

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
    // Set all flags
    sut->ctrl_register |= PPUCTRL_NAMETABLE_X;
    sut->ctrl_register |= PPUCTRL_NAMETABLE_Y;
    sut->ctrl_register |= PPUCTRL_INCREMENT;
    sut->ctrl_register |= PPUCTRL_SPRITE_TABLE;
    sut->ctrl_register |= PPUCTRL_BG_TABLE;
    sut->ctrl_register |= PPUCTRL_SPRITE_SIZE;
    sut->ctrl_register |= PPUCTRL_MASTER_SLAVE;
    sut->ctrl_register |= PPUCTRL_NMI_ENABLE;

    TEST_ASSERT_EQUAL_HEX8(0xFF, sut->ctrl_register);

    // Clear one flag, others should remain set
    sut->ctrl_register &= ~PPUCTRL_INCREMENT;
    TEST_ASSERT_EQUAL_HEX8(0xFB, sut->ctrl_register);
    TEST_ASSERT_TRUE(sut->ctrl_register & PPUCTRL_NAMETABLE_X);
    TEST_ASSERT_TRUE(sut->ctrl_register & PPUCTRL_NAMETABLE_Y);
    TEST_ASSERT_FALSE(sut->ctrl_register & PPUCTRL_INCREMENT);
    TEST_ASSERT_TRUE(sut->ctrl_register & PPUCTRL_SPRITE_TABLE);
}

// =============================================================================
// PPUMASK flag tests
// =============================================================================

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
    // Set all flags
    ppu_set_mask_flag(sut, PPUMASK_GRAYSCALE, true);
    ppu_set_mask_flag(sut, PPUMASK_BG_LEFT, true);
    ppu_set_mask_flag(sut, PPUMASK_SPRITE_LEFT, true);
    ppu_set_mask_flag(sut, PPUMASK_BG_ENABLE, true);
    ppu_set_mask_flag(sut, PPUMASK_SPRITE_ENABLE, true);
    ppu_set_mask_flag(sut, PPUMASK_EMPHASIZE_R, true);
    ppu_set_mask_flag(sut, PPUMASK_EMPHASIZE_G, true);
    ppu_set_mask_flag(sut, PPUMASK_EMPHASIZE_B, true);

    TEST_ASSERT_EQUAL_HEX8(0xFF, sut->mask_register);

    // Clear one flag, others should remain set
    ppu_set_mask_flag(sut, PPUMASK_BG_ENABLE, false);
    TEST_ASSERT_EQUAL_HEX8(0xF7, sut->mask_register);
    TEST_ASSERT_TRUE(ppu_get_mask_flag(sut, PPUMASK_GRAYSCALE));
    TEST_ASSERT_TRUE(ppu_get_mask_flag(sut, PPUMASK_BG_LEFT));
    TEST_ASSERT_TRUE(ppu_get_mask_flag(sut, PPUMASK_SPRITE_LEFT));
    TEST_ASSERT_FALSE(ppu_get_mask_flag(sut, PPUMASK_BG_ENABLE));
    TEST_ASSERT_TRUE(ppu_get_mask_flag(sut, PPUMASK_SPRITE_ENABLE));
}

// =============================================================================
// PPUSTATUS flag tests
// =============================================================================

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
    // Set all status flags
    ppu_set_status_flag(sut, PPUSTATUS_OVERFLOW, true);
    ppu_set_status_flag(sut, PPUSTATUS_SPRITE0_HIT, true);
    ppu_set_status_flag(sut, PPUSTATUS_VBLANK, true);

    TEST_ASSERT_EQUAL_HEX8(0xE0, sut->status_register);

    // Clear one flag, others should remain set
    ppu_set_status_flag(sut, PPUSTATUS_SPRITE0_HIT, false);
    TEST_ASSERT_EQUAL_HEX8(0xA0, sut->status_register);
    TEST_ASSERT_TRUE(ppu_get_status_flag(sut, PPUSTATUS_OVERFLOW));
    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_SPRITE0_HIT));
    TEST_ASSERT_TRUE(ppu_get_status_flag(sut, PPUSTATUS_VBLANK));
}

// =============================================================================
// Register read/write tests
// =============================================================================

void test_read_write_only_registers_return_zero(void) {
    // Set ctrl and mask to non-zero to verify reads still return 0
    sut->ctrl_register = 0xFF;
    sut->mask_register = 0xFF;
    sut->oam_addr_register = 0xFF;

    // Write-only registers should return 0 when read
    TEST_ASSERT_EQUAL_HEX8(0x00, ppu_read(sut, PPU_REGISTER_CTRL));
    TEST_ASSERT_EQUAL_HEX8(0x00, ppu_read(sut, PPU_REGISTER_MASK));
    TEST_ASSERT_EQUAL_HEX8(0x00, ppu_read(sut, PPU_REGISTER_OAMADDR));
    TEST_ASSERT_EQUAL_HEX8(0x00, ppu_read(sut, PPU_REGISTER_SCROLL));
    TEST_ASSERT_EQUAL_HEX8(0x00, ppu_read(sut, PPU_REGISTER_ADDR));
}

// =============================================================================
// PPUSTATUS ($2002) read behavior
// =============================================================================

void test_status_read_returns_status_register(void) {
    // Set some status flags
    ppu_set_status_flag(sut, PPUSTATUS_FLAG_VBLANK, true);
    ppu_set_status_flag(sut, PPUSTATUS_FLAG_SPRITE0_HIT, true);

    byte_t status = ppu_read(sut, PPU_REGISTER_STATUS);
    TEST_ASSERT_EQUAL_HEX8(0xC0, status);
}

void test_status_read_clears_vblank_flag(void) {
    ppu_set_status_flag(sut, PPUSTATUS_FLAG_VBLANK, true);
    TEST_ASSERT_TRUE(ppu_get_status_flag(sut, PPUSTATUS_FLAG_VBLANK));

    ppu_read(sut, PPU_REGISTER_STATUS);

    // Vblank should be cleared after read
    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_FLAG_VBLANK));
}

void test_status_read_does_not_clear_other_flags(void) {
    ppu_set_status_flag(sut, PPUSTATUS_FLAG_VBLANK, true);
    ppu_set_status_flag(sut, PPUSTATUS_FLAG_SPRITE0_HIT, true);
    ppu_set_status_flag(sut, PPUSTATUS_FLAG_OVERFLOW, true);

    ppu_read(sut, PPU_REGISTER_STATUS);

    // Only vblank should be cleared
    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_FLAG_VBLANK));
    TEST_ASSERT_TRUE(ppu_get_status_flag(sut, PPUSTATUS_FLAG_SPRITE0_HIT));
    TEST_ASSERT_TRUE(ppu_get_status_flag(sut, PPUSTATUS_FLAG_OVERFLOW));
}

void test_status_read_resets_write_latch(void) {
    // Set write latch (as if first write to PPUSCROLL/PPUADDR happened)
    sut->write_latch = true;

    ppu_read(sut, PPU_REGISTER_STATUS);

    TEST_ASSERT_FALSE(sut->write_latch);
}

// =============================================================================
// PPUCTRL ($2000) write behavior
// =============================================================================

void test_ctrl_write_sets_register(void) {
    ppu_write(sut, PPU_REGISTER_CTRL, 0x9A);
    TEST_ASSERT_EQUAL_HEX8(0x9A, sut->ctrl_register);
}

void test_ctrl_write_updates_nametable_in_temp_addr(void) {
    // Nametable bits (0-1) should go to temp_addr bits 10-11
    ppu_write(sut, PPU_REGISTER_CTRL, 0x00);  // Nametable 0
    TEST_ASSERT_EQUAL_HEX16(0x0000, sut->temp_addr & 0x0C00);

    ppu_write(sut, PPU_REGISTER_CTRL, 0x01);  // Nametable 1
    TEST_ASSERT_EQUAL_HEX16(0x0400, sut->temp_addr & 0x0C00);

    ppu_write(sut, PPU_REGISTER_CTRL, 0x02);  // Nametable 2
    TEST_ASSERT_EQUAL_HEX16(0x0800, sut->temp_addr & 0x0C00);

    ppu_write(sut, PPU_REGISTER_CTRL, 0x03);  // Nametable 3
    TEST_ASSERT_EQUAL_HEX16(0x0C00, sut->temp_addr & 0x0C00);
}

void test_ctrl_write_preserves_other_temp_addr_bits(void) {
    sut->temp_addr = 0x7FFF;  // All bits set
    ppu_write(sut, PPU_REGISTER_CTRL, 0x00);  // Clear nametable bits

    // Only bits 10-11 should be cleared
    TEST_ASSERT_EQUAL_HEX16(0x73FF, sut->temp_addr);
}

// =============================================================================
// PPUMASK ($2001) write behavior
// =============================================================================

void test_mask_write_sets_register(void) {
    ppu_write(sut, PPU_REGISTER_MASK, 0x1E);
    TEST_ASSERT_EQUAL_HEX8(0x1E, sut->mask_register);
}

// =============================================================================
// OAMADDR ($2003) and OAMDATA ($2004) behavior
// =============================================================================

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
    TEST_ASSERT_EQUAL_HEX8(0x00, sut->oam_addr_register);  // Wrapped to 0
}

// =============================================================================
// PPUSCROLL ($2005) double-write behavior
// =============================================================================

void test_scroll_first_write_sets_x_scroll(void) {
    // First write: X scroll
    // Value 0xD3 = 211 = coarse X (26) + fine X (3)
    ppu_write(sut, PPU_REGISTER_SCROLL, 0xD3);

    TEST_ASSERT_EQUAL_HEX8(0x03, sut->fine_x);  // Low 3 bits
    TEST_ASSERT_EQUAL_HEX16(0x001A, sut->temp_addr & 0x001F);  // Coarse X = high 5 bits = 26
    TEST_ASSERT_TRUE(sut->write_latch);
}

void test_scroll_second_write_sets_y_scroll(void) {
    // First write (X)
    ppu_write(sut, PPU_REGISTER_SCROLL, 0x00);
    // Second write: Y scroll
    // Value 0x5F = 95 = coarse Y (11) + fine Y (7)
    ppu_write(sut, PPU_REGISTER_SCROLL, 0x5F);

    // Fine Y (7) goes to bits 12-14
    TEST_ASSERT_EQUAL_HEX16(0x7000, sut->temp_addr & 0x7000);
    // Coarse Y (11) goes to bits 5-9
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

// =============================================================================
// PPUADDR ($2006) double-write behavior
// =============================================================================

void test_addr_first_write_sets_high_byte(void) {
    ppu_write(sut, PPU_REGISTER_ADDR, 0x21);

    // High byte goes to bits 8-13 (bit 14 cleared)
    TEST_ASSERT_EQUAL_HEX16(0x2100, sut->temp_addr & 0x3F00);
    TEST_ASSERT_TRUE(sut->write_latch);
    // vram_addr not updated yet
    TEST_ASSERT_EQUAL_HEX16(0x0000, sut->vram_addr);
}

void test_addr_second_write_sets_low_byte_and_copies_to_vram(void) {
    ppu_write(sut, PPU_REGISTER_ADDR, 0x21);  // High byte
    ppu_write(sut, PPU_REGISTER_ADDR, 0x08);  // Low byte

    TEST_ASSERT_EQUAL_HEX16(0x2108, sut->temp_addr);
    TEST_ASSERT_EQUAL_HEX16(0x2108, sut->vram_addr);  // Copied on second write
    TEST_ASSERT_FALSE(sut->write_latch);
}

void test_addr_high_byte_masks_to_6_bits(void) {
    // Writing 0xFF should only keep lower 6 bits (0x3F)
    ppu_write(sut, PPU_REGISTER_ADDR, 0xFF);
    ppu_write(sut, PPU_REGISTER_ADDR, 0x00);

    TEST_ASSERT_EQUAL_HEX16(0x3F00, sut->vram_addr);
}

// =============================================================================
// PPUDATA ($2007) address increment behavior
// =============================================================================

void test_data_read_increments_vram_addr_by_1(void) {
    sut->vram_addr = 0x2000;
    sut->ctrl_register = 0x00;  // Increment by 1

    ppu_read(sut, PPU_REGISTER_DATA);

    TEST_ASSERT_EQUAL_HEX16(0x2001, sut->vram_addr);
}

void test_data_read_increments_vram_addr_by_32(void) {
    sut->vram_addr = 0x2000;
    sut->ctrl_register = PPUCTRL_FLAG_INCREMENT;  // Increment by 32

    ppu_read(sut, PPU_REGISTER_DATA);

    TEST_ASSERT_EQUAL_HEX16(0x2020, sut->vram_addr);
}

void test_data_write_increments_vram_addr_by_1(void) {
    sut->vram_addr = 0x2000;
    sut->ctrl_register = 0x00;  // Increment by 1

    ppu_write(sut, PPU_REGISTER_DATA, 0x42);

    TEST_ASSERT_EQUAL_HEX16(0x2001, sut->vram_addr);
}

void test_data_write_increments_vram_addr_by_32(void) {
    sut->vram_addr = 0x2000;
    sut->ctrl_register = PPUCTRL_FLAG_INCREMENT;  // Increment by 32

    ppu_write(sut, PPU_REGISTER_DATA, 0x42);

    TEST_ASSERT_EQUAL_HEX16(0x2020, sut->vram_addr);
}

void test_data_read_returns_buffered_value(void) {
    sut->data_buffer = 0xAB;

    byte_t result = ppu_read(sut, PPU_REGISTER_DATA);

    TEST_ASSERT_EQUAL_HEX8(0xAB, result);
}

// =============================================================================
// Write latch shared between SCROLL and ADDR
// =============================================================================

void test_status_read_resets_latch_for_scroll(void) {
    // First scroll write
    ppu_write(sut, PPU_REGISTER_SCROLL, 0x10);
    TEST_ASSERT_TRUE(sut->write_latch);

    // Read status resets latch
    ppu_read(sut, PPU_REGISTER_STATUS);
    TEST_ASSERT_FALSE(sut->write_latch);

    // Next scroll write is first write again
    ppu_write(sut, PPU_REGISTER_SCROLL, 0x20);
    TEST_ASSERT_EQUAL_HEX8(0x00, sut->fine_x & 0x07);  // Should be X scroll
}

// =============================================================================
// VRAM internal read/write tests
// =============================================================================

void test_vram_write_read_nametable(void) {
    // Write directly to nametable via internal VRAM function
    ppu_set_mirroring(sut, MIRROR_VERTICAL);
    ppu_vram_write(sut, 0x2000, 0x42);
    TEST_ASSERT_EQUAL_HEX8(0x42, ppu_vram_read(sut, 0x2000));

    ppu_vram_write(sut, 0x23FF, 0xAB);
    TEST_ASSERT_EQUAL_HEX8(0xAB, ppu_vram_read(sut, 0x23FF));
}

void test_vram_write_read_palette(void) {
    // Write to palette memory
    ppu_vram_write(sut, 0x3F00, 0x0D);  // Background color
    TEST_ASSERT_EQUAL_HEX8(0x0D, ppu_vram_read(sut, 0x3F00));

    ppu_vram_write(sut, 0x3F01, 0x16);  // BG palette 0, color 1
    TEST_ASSERT_EQUAL_HEX8(0x16, ppu_vram_read(sut, 0x3F01));

    ppu_vram_write(sut, 0x3F11, 0x30);  // Sprite palette 0, color 1
    TEST_ASSERT_EQUAL_HEX8(0x30, ppu_vram_read(sut, 0x3F11));
}

void test_vram_nametable_mirroring_horizontal(void) {
    ppu_set_mirroring(sut, MIRROR_HORIZONTAL);

    // NT0 and NT1 should mirror each other (both map to first 1KB)
    ppu_vram_write(sut, 0x2000, 0x11);
    TEST_ASSERT_EQUAL_HEX8(0x11, ppu_vram_read(sut, 0x2400));  // NT1 mirrors NT0

    ppu_vram_write(sut, 0x2456, 0x22);
    TEST_ASSERT_EQUAL_HEX8(0x22, ppu_vram_read(sut, 0x2056));  // NT0 mirrors NT1

    // NT2 and NT3 should mirror each other (both map to second 1KB)
    ppu_vram_write(sut, 0x2800, 0x33);
    TEST_ASSERT_EQUAL_HEX8(0x33, ppu_vram_read(sut, 0x2C00));  // NT3 mirrors NT2

    // NT0 and NT2 should be independent
    ppu_vram_write(sut, 0x2000, 0xAA);
    ppu_vram_write(sut, 0x2800, 0xBB);
    TEST_ASSERT_EQUAL_HEX8(0xAA, ppu_vram_read(sut, 0x2000));
    TEST_ASSERT_EQUAL_HEX8(0xBB, ppu_vram_read(sut, 0x2800));
}

void test_vram_nametable_mirroring_vertical(void) {
    ppu_set_mirroring(sut, MIRROR_VERTICAL);

    // NT0 and NT2 should mirror each other
    ppu_vram_write(sut, 0x2000, 0x11);
    TEST_ASSERT_EQUAL_HEX8(0x11, ppu_vram_read(sut, 0x2800));  // NT2 mirrors NT0

    // NT1 and NT3 should mirror each other
    ppu_vram_write(sut, 0x2400, 0x22);
    TEST_ASSERT_EQUAL_HEX8(0x22, ppu_vram_read(sut, 0x2C00));  // NT3 mirrors NT1

    // NT0 and NT1 should be independent
    ppu_vram_write(sut, 0x2000, 0xAA);
    ppu_vram_write(sut, 0x2400, 0xBB);
    TEST_ASSERT_EQUAL_HEX8(0xAA, ppu_vram_read(sut, 0x2000));
    TEST_ASSERT_EQUAL_HEX8(0xBB, ppu_vram_read(sut, 0x2400));
}

void test_vram_palette_mirroring(void) {
    // $3F10, $3F14, $3F18, $3F1C mirror $3F00, $3F04, $3F08, $3F0C
    ppu_vram_write(sut, 0x3F00, 0x0D);
    TEST_ASSERT_EQUAL_HEX8(0x0D, ppu_vram_read(sut, 0x3F10));  // $3F10 mirrors $3F00

    ppu_vram_write(sut, 0x3F14, 0x2D);
    TEST_ASSERT_EQUAL_HEX8(0x2D, ppu_vram_read(sut, 0x3F04));  // $3F04 mirrors $3F14

    // Palette is mirrored every 32 bytes
    ppu_vram_write(sut, 0x3F05, 0x15);
    TEST_ASSERT_EQUAL_HEX8(0x15, ppu_vram_read(sut, 0x3F25));  // $3F25 mirrors $3F05
}

void test_vram_nametable_mirror_at_3000(void) {
    // $3000-$3EFF mirrors $2000-$2EFF
    ppu_set_mirroring(sut, MIRROR_VERTICAL);

    ppu_vram_write(sut, 0x2000, 0x42);
    TEST_ASSERT_EQUAL_HEX8(0x42, ppu_vram_read(sut, 0x3000));

    ppu_vram_write(sut, 0x30AB, 0x99);
    TEST_ASSERT_EQUAL_HEX8(0x99, ppu_vram_read(sut, 0x20AB));
}

// =============================================================================
// PPUDATA register behavior with VRAM
// =============================================================================

void test_ppudata_write_to_nametable(void) {
    ppu_set_mirroring(sut, MIRROR_VERTICAL);

    // Set VRAM address to nametable 0
    ppu_write(sut, PPU_REGISTER_ADDR, 0x20);
    ppu_write(sut, PPU_REGISTER_ADDR, 0x00);

    // Write via PPUDATA
    ppu_write(sut, PPU_REGISTER_DATA, 0x42);

    // Verify it was written
    TEST_ASSERT_EQUAL_HEX8(0x42, ppu_vram_read(sut, 0x2000));
}

void test_ppudata_read_from_nametable_is_buffered(void) {
    ppu_set_mirroring(sut, MIRROR_VERTICAL);

    // Pre-populate nametable
    ppu_vram_write(sut, 0x2000, 0xAA);
    ppu_vram_write(sut, 0x2001, 0xBB);

    // Set VRAM address
    ppu_write(sut, PPU_REGISTER_ADDR, 0x20);
    ppu_write(sut, PPU_REGISTER_ADDR, 0x00);

    // First read returns garbage (buffer was empty)
    byte_t first = ppu_read(sut, PPU_REGISTER_DATA);
    TEST_ASSERT_EQUAL_HEX8(0x00, first);  // Buffer was empty

    // Second read returns the first value (buffered)
    byte_t second = ppu_read(sut, PPU_REGISTER_DATA);
    TEST_ASSERT_EQUAL_HEX8(0xAA, second);
}

void test_ppudata_read_from_palette_is_not_buffered(void) {
    // Pre-populate palette
    ppu_vram_write(sut, 0x3F00, 0x0D);
    ppu_vram_write(sut, 0x3F01, 0x16);

    // Set VRAM address to palette
    ppu_write(sut, PPU_REGISTER_ADDR, 0x3F);
    ppu_write(sut, PPU_REGISTER_ADDR, 0x00);

    // Palette reads are immediate, not buffered
    byte_t first = ppu_read(sut, PPU_REGISTER_DATA);
    TEST_ASSERT_EQUAL_HEX8(0x0D, first);

    byte_t second = ppu_read(sut, PPU_REGISTER_DATA);
    TEST_ASSERT_EQUAL_HEX8(0x16, second);
}

void test_ppudata_increments_vram_addr(void) {
    // Set VRAM address
    ppu_write(sut, PPU_REGISTER_ADDR, 0x20);
    ppu_write(sut, PPU_REGISTER_ADDR, 0x00);

    // Write increments by 1 (default)
    ppu_write(sut, PPU_REGISTER_DATA, 0x11);
    TEST_ASSERT_EQUAL_HEX16(0x2001, sut->vram_addr);

    ppu_write(sut, PPU_REGISTER_DATA, 0x22);
    TEST_ASSERT_EQUAL_HEX16(0x2002, sut->vram_addr);

    // Set increment to 32
    ppu_write(sut, PPU_REGISTER_CTRL, PPUCTRL_FLAG_INCREMENT);

    ppu_write(sut, PPU_REGISTER_DATA, 0x33);
    TEST_ASSERT_EQUAL_HEX16(0x2022, sut->vram_addr);
}

// =============================================================================
// CHR ROM tests
// =============================================================================

static byte_t test_chr_rom[8192];  // 8KB CHR ROM for testing

void test_chr_rom_read(void) {
    // Set up test CHR ROM
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

// =============================================================================
// PPU timing tests
// =============================================================================

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

    // Run 340 cycles (0-339), should still be on scanline 0
    for (int i = 0; i < 340; i++) {
        ppu_tick(sut);
    }
    TEST_ASSERT_EQUAL_INT(0, sut->scanline);
    TEST_ASSERT_EQUAL_INT(340, sut->cycle);

    // 341st cycle should advance to scanline 1
    ppu_tick(sut);
    TEST_ASSERT_EQUAL_INT(1, sut->scanline);
    TEST_ASSERT_EQUAL_INT(0, sut->cycle);
}

void test_ppu_frame_completes_after_262_scanlines(void) {
    sut->scanline = 261;  // Last scanline
    sut->cycle = 340;     // Last cycle

    ppu_tick(sut);

    // Should wrap to scanline 0, cycle 0
    TEST_ASSERT_EQUAL_INT(0, sut->scanline);
    TEST_ASSERT_EQUAL_INT(0, sut->cycle);
}

void test_vblank_flag_set_at_scanline_241(void) {
    sut->scanline = 241;
    sut->cycle = 0;
    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_FLAG_VBLANK));

    ppu_tick(sut);  // cycle 1

    TEST_ASSERT_TRUE(ppu_get_status_flag(sut, PPUSTATUS_FLAG_VBLANK));
}

void test_vblank_flag_cleared_at_prerender(void) {
    // Set vblank flag
    ppu_set_status_flag(sut, PPUSTATUS_FLAG_VBLANK, true);
    ppu_set_status_flag(sut, PPUSTATUS_FLAG_SPRITE0_HIT, true);
    ppu_set_status_flag(sut, PPUSTATUS_FLAG_OVERFLOW, true);

    sut->scanline = 261;  // Pre-render scanline
    sut->cycle = 0;

    ppu_tick(sut);  // cycle 1

    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_FLAG_VBLANK));
    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_FLAG_SPRITE0_HIT));
    TEST_ASSERT_FALSE(ppu_get_status_flag(sut, PPUSTATUS_FLAG_OVERFLOW));
}

void test_nmi_triggered_when_vblank_and_nmi_enabled(void) {
    // Enable NMI
    ppu_set_ctrl_flag(sut, PPUCTRL_FLAG_NMI_ENABLE, true);

    sut->scanline = 241;
    sut->cycle = 0;
    sut->nmi_pending = false;

    ppu_tick(sut);

    TEST_ASSERT_TRUE(sut->nmi_pending);
}

void test_nmi_not_triggered_when_nmi_disabled(void) {
    // NMI disabled (default)
    TEST_ASSERT_FALSE(ppu_get_ctrl_flag(sut, PPUCTRL_FLAG_NMI_ENABLE));

    sut->scanline = 241;
    sut->cycle = 0;
    sut->nmi_pending = false;

    ppu_tick(sut);

    TEST_ASSERT_FALSE(sut->nmi_pending);
    TEST_ASSERT_TRUE(ppu_get_status_flag(sut, PPUSTATUS_FLAG_VBLANK));  // Flag still set
}

// =============================================================================
// OAM DMA tests ($4014)
// =============================================================================

static bus_s test_bus;

void test_oam_dma_copies_256_bytes(void) {
    bus_init(&test_bus);

    // Fill RAM page $02 with test data
    for (int i = 0; i < 256; i++) {
        test_bus.ram[0x0200 + i] = (byte_t)i;
    }

    // Trigger OAM DMA from page $02
    bus_oam_dma(&test_bus, 0x02);

    // Verify all 256 bytes were copied to OAM
    for (int i = 0; i < 256; i++) {
        TEST_ASSERT_EQUAL_HEX8((byte_t)i, test_bus.ppu->oam[i]);
    }
}

void test_oam_dma_reads_from_correct_page(void) {
    bus_init(&test_bus);

    // Fill different pages with different patterns
    for (int i = 0; i < 256; i++) {
        test_bus.ram[0x0000 + i] = 0xAA;  // Page $00
        test_bus.ram[0x0100 + i] = 0xBB;  // Page $01
        test_bus.ram[0x0200 + i] = 0xCC;  // Page $02
    }

    // DMA from page $01
    bus_oam_dma(&test_bus, 0x01);

    // Should have $BB pattern
    TEST_ASSERT_EQUAL_HEX8(0xBB, test_bus.ppu->oam[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, test_bus.ppu->oam[127]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, test_bus.ppu->oam[255]);
}

void test_oam_dma_via_bus_write(void) {
    bus_init(&test_bus);

    // Fill page $03 with test data
    for (int i = 0; i < 256; i++) {
        test_bus.ram[0x0300 + i] = (byte_t)(255 - i);
    }

    // Write to $4014 triggers DMA
    bus_write(&test_bus, 0x4014, 0x03);

    // Verify OAM was filled
    TEST_ASSERT_EQUAL_HEX8(0xFF, test_bus.ppu->oam[0]);
    TEST_ASSERT_EQUAL_HEX8(0xFE, test_bus.ppu->oam[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00, test_bus.ppu->oam[255]);
}

void test_oam_dma_sets_cycle_count(void) {
    bus_init(&test_bus);

    bus_oam_dma(&test_bus, 0x00);

    // Should take 513 cycles
    TEST_ASSERT_EQUAL_UINT16(513, test_bus.oam_dma_cycles);
}

// =============================================================================
// Edge case tests
// =============================================================================

void test_vram_addr_wraps_at_4000(void) {
    // Set VRAM address near the end
    ppu_write(sut, PPU_REGISTER_ADDR, 0x3F);
    ppu_write(sut, PPU_REGISTER_ADDR, 0xFF);
    TEST_ASSERT_EQUAL_HEX16(0x3FFF, sut->vram_addr);

    // Write increments and should wrap
    ppu_write(sut, PPU_REGISTER_DATA, 0x42);
    TEST_ASSERT_EQUAL_HEX16(0x0000, sut->vram_addr);  // Wrapped from $4000 to $0000
}

void test_vram_addr_wraps_on_read(void) {
    sut->vram_addr = 0x3FFF;
    sut->ctrl_register = 0;  // Increment by 1

    ppu_read(sut, PPU_REGISTER_DATA);

    TEST_ASSERT_EQUAL_HEX16(0x0000, sut->vram_addr);  // Wrapped
}

void test_vram_addr_wraps_with_32_increment(void) {
    // Set VRAM address so +32 would overflow
    sut->vram_addr = 0x3FF0;
    sut->ctrl_register = PPUCTRL_FLAG_INCREMENT;  // Increment by 32

    ppu_write(sut, PPU_REGISTER_DATA, 0x42);

    // 0x3FF0 + 32 = 0x4010, masked to 0x0010
    TEST_ASSERT_EQUAL_HEX16(0x0010, sut->vram_addr);
}

void test_nmi_triggered_when_enabling_during_vblank(void) {
    // Set vblank flag (simulating we're in vblank)
    ppu_set_status_flag(sut, PPUSTATUS_FLAG_VBLANK, true);

    // NMI is disabled
    TEST_ASSERT_FALSE(ppu_get_ctrl_flag(sut, PPUCTRL_FLAG_NMI_ENABLE));
    sut->nmi_pending = false;

    // Enable NMI - should trigger immediately since vblank is set
    ppu_write(sut, PPU_REGISTER_CTRL, PPUCTRL_FLAG_NMI_ENABLE);

    TEST_ASSERT_TRUE(sut->nmi_pending);
}

void test_nmi_not_triggered_when_already_enabled(void) {
    // Set vblank flag and NMI already enabled
    ppu_set_status_flag(sut, PPUSTATUS_FLAG_VBLANK, true);
    sut->ctrl_register = PPUCTRL_FLAG_NMI_ENABLE;
    sut->nmi_pending = false;

    // Write same value (NMI still enabled) - should NOT trigger again
    ppu_write(sut, PPU_REGISTER_CTRL, PPUCTRL_FLAG_NMI_ENABLE);

    TEST_ASSERT_FALSE(sut->nmi_pending);
}

void test_palette_mirroring_beyond_3f1f(void) {
    // Palette at $3F20-$3FFF mirrors $3F00-$3F1F
    ppu_vram_write(sut, 0x3F05, 0xAB);

    // $3F25 should mirror $3F05
    TEST_ASSERT_EQUAL_HEX8(0xAB, ppu_vram_read(sut, 0x3F25));
    // $3F45 should also mirror $3F05
    TEST_ASSERT_EQUAL_HEX8(0xAB, ppu_vram_read(sut, 0x3F45));
    // $3FE5 should also mirror $3F05
    TEST_ASSERT_EQUAL_HEX8(0xAB, ppu_vram_read(sut, 0x3FE5));
}

void test_ppu_init_starts_at_prerender_scanline(void) {
    ppu_init(&test_ppu);

    TEST_ASSERT_EQUAL_INT(261, test_ppu.scanline);
    TEST_ASSERT_EQUAL_INT(0, test_ppu.cycle);
}

void test_sprite_palette_mirrors_to_bg_palette(void) {
    // $3F10 mirrors $3F00 (universal background)
    ppu_vram_write(sut, 0x3F00, 0x0F);
    TEST_ASSERT_EQUAL_HEX8(0x0F, ppu_vram_read(sut, 0x3F10));

    // Writing to $3F10 affects $3F00
    ppu_vram_write(sut, 0x3F10, 0x1D);
    TEST_ASSERT_EQUAL_HEX8(0x1D, ppu_vram_read(sut, 0x3F00));

    // Same for $3F14/$3F04, $3F18/$3F08, $3F1C/$3F0C
    ppu_vram_write(sut, 0x3F04, 0x21);
    TEST_ASSERT_EQUAL_HEX8(0x21, ppu_vram_read(sut, 0x3F14));

    ppu_vram_write(sut, 0x3F18, 0x31);
    TEST_ASSERT_EQUAL_HEX8(0x31, ppu_vram_read(sut, 0x3F08));
}

// =============================================================================
// Main
// =============================================================================

int main(void) {
    UNITY_BEGIN();

    // Initialization
    RUN_TEST(test_ppu_init_clears_all_registers);

    // PPUCTRL flags
    RUN_TEST(test_ctrl_flag_nametable_x);
    RUN_TEST(test_ctrl_flag_nametable_y);
    RUN_TEST(test_ctrl_flag_increment);
    RUN_TEST(test_ctrl_flag_sprite_table);
    RUN_TEST(test_ctrl_flag_bg_table);
    RUN_TEST(test_ctrl_flag_sprite_size);
    RUN_TEST(test_ctrl_flag_master_slave);
    RUN_TEST(test_ctrl_flag_nmi_enable);
    RUN_TEST(test_ctrl_flags_are_independent);

    // PPUMASK flags
    RUN_TEST(test_mask_flag_grayscale);
    RUN_TEST(test_mask_flag_bg_left);
    RUN_TEST(test_mask_flag_sprite_left);
    RUN_TEST(test_mask_flag_bg_enable);
    RUN_TEST(test_mask_flag_sprite_enable);
    RUN_TEST(test_mask_flag_emphasize_r);
    RUN_TEST(test_mask_flag_emphasize_g);
    RUN_TEST(test_mask_flag_emphasize_b);
    RUN_TEST(test_mask_flags_are_independent);

    // PPUSTATUS flags
    RUN_TEST(test_status_flag_overflow);
    RUN_TEST(test_status_flag_sprite0_hit);
    RUN_TEST(test_status_flag_vblank);
    RUN_TEST(test_status_flags_are_independent);

    // Register reads (write-only)
    RUN_TEST(test_read_write_only_registers_return_zero);

    // PPUSTATUS read behavior
    RUN_TEST(test_status_read_returns_status_register);
    RUN_TEST(test_status_read_clears_vblank_flag);
    RUN_TEST(test_status_read_does_not_clear_other_flags);
    RUN_TEST(test_status_read_resets_write_latch);

    // PPUCTRL write behavior
    RUN_TEST(test_ctrl_write_sets_register);
    RUN_TEST(test_ctrl_write_updates_nametable_in_temp_addr);
    RUN_TEST(test_ctrl_write_preserves_other_temp_addr_bits);

    // PPUMASK write behavior
    RUN_TEST(test_mask_write_sets_register);

    // OAM access
    RUN_TEST(test_oamaddr_write_sets_address);
    RUN_TEST(test_oamdata_read_returns_oam_at_address);
    RUN_TEST(test_oamdata_write_stores_and_increments);
    RUN_TEST(test_oamdata_write_wraps_at_256);

    // PPUSCROLL double-write
    RUN_TEST(test_scroll_first_write_sets_x_scroll);
    RUN_TEST(test_scroll_second_write_sets_y_scroll);
    RUN_TEST(test_scroll_write_latch_toggles);

    // PPUADDR double-write
    RUN_TEST(test_addr_first_write_sets_high_byte);
    RUN_TEST(test_addr_second_write_sets_low_byte_and_copies_to_vram);
    RUN_TEST(test_addr_high_byte_masks_to_6_bits);

    // PPUDATA address increment
    RUN_TEST(test_data_read_increments_vram_addr_by_1);
    RUN_TEST(test_data_read_increments_vram_addr_by_32);
    RUN_TEST(test_data_write_increments_vram_addr_by_1);
    RUN_TEST(test_data_write_increments_vram_addr_by_32);
    RUN_TEST(test_data_read_returns_buffered_value);

    // Write latch behavior
    RUN_TEST(test_status_read_resets_latch_for_scroll);

    // VRAM internal read/write
    RUN_TEST(test_vram_write_read_nametable);
    RUN_TEST(test_vram_write_read_palette);
    RUN_TEST(test_vram_nametable_mirroring_horizontal);
    RUN_TEST(test_vram_nametable_mirroring_vertical);
    RUN_TEST(test_vram_palette_mirroring);
    RUN_TEST(test_vram_nametable_mirror_at_3000);

    // PPUDATA register behavior with VRAM
    RUN_TEST(test_ppudata_write_to_nametable);
    RUN_TEST(test_ppudata_read_from_nametable_is_buffered);
    RUN_TEST(test_ppudata_read_from_palette_is_not_buffered);
    RUN_TEST(test_ppudata_increments_vram_addr);

    // CHR ROM
    RUN_TEST(test_chr_rom_read);

    // PPU timing
    RUN_TEST(test_ppu_cycle_increments);
    RUN_TEST(test_ppu_scanline_increments_after_341_cycles);
    RUN_TEST(test_ppu_frame_completes_after_262_scanlines);
    RUN_TEST(test_vblank_flag_set_at_scanline_241);
    RUN_TEST(test_vblank_flag_cleared_at_prerender);
    RUN_TEST(test_nmi_triggered_when_vblank_and_nmi_enabled);
    RUN_TEST(test_nmi_not_triggered_when_nmi_disabled);

    // OAM DMA
    RUN_TEST(test_oam_dma_copies_256_bytes);
    RUN_TEST(test_oam_dma_reads_from_correct_page);
    RUN_TEST(test_oam_dma_via_bus_write);
    RUN_TEST(test_oam_dma_sets_cycle_count);

    // Edge cases
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
