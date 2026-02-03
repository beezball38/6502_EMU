
// ===============================
// SDL2 Visual Debugger for NES Emulator
// ===============================
//
// Visualizes CPU, PPU, memory, and instruction state for debugging and educational purposes.
//
// For NES hardware details, see:
//   https://www.nesdev.org/wiki/NES");
//
// ===============================
// CPU Registers Panel
// ===============================
//
// 6502 CPU Registers:
//
//   +-----+-----+-----+-----+-----+-----+-----+
//   |  A  |  X  |  Y  |  SP |  PC |  P  | ... |
//   +-----+-----+-----+-----+-----+-----+-----+
//
// Status register (P): NV-BDIZC
//   N = Negative, V = Overflow, - = Unused, B = Break, D = Decimal, I = IRQ Disable, Z = Zero, C = Carry
//
// See:
//   https://www.nesdev.org/wiki/CPU_registers
//   https://www.nesdev.org/wiki/Status_flags
//
// ===============================
// PPU Pattern Table/Palette/OAM Panels
// ===============================
//
// Pattern tables: 2 x 4KB tables at $0000 and $1000 (see https://www.nesdev.org/wiki/PPU_pattern_tables)
// Palettes: 32 bytes at $3F00-$3F1F (see https://www.nesdev.org/wiki/PPU_palettes)
// OAM: 256 bytes for 64 sprites (see https://www.nesdev.org/wiki/PPU_OAM)
//
// ===============================
// Memory View Panel
// ===============================
//
// Shows memory in hex and ASCII, with selectable region (zero page, stack, custom).
//
// NES memory map:
//
//   $0000-$07FF: 2KB internal RAM
//   $0800-$1FFF: Mirrors of $0000-$07FF
//   $2000-$3FFF: PPU registers (mirrored)
//   $4000-$401F: APU and I/O registers
//   $4020-$FFFF: Cartridge space (PRG ROM, PRG RAM, mappers)
//
// See:
//   https://www.nesdev.org/wiki/CPU_memory_map
//
// ===============================
//
// For further technical details, see nesdev.org

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debugger.h"
#include "ines.h"

// 8x8 bitmap font for ASCII 32-127 (basic printable characters)
// Each character is 8 bytes, one byte per row, MSB is leftmost pixel
static const unsigned char font_8x8[96][8] = {
    // Space (32)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ! (33)
    {0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00},
    // " (34)
    {0x6C, 0x6C, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00},
    // # (35)
    {0x6C, 0x6C, 0xFE, 0x6C, 0xFE, 0x6C, 0x6C, 0x00},
    // $ (36)
    {0x18, 0x7E, 0xC0, 0x7C, 0x06, 0xFC, 0x18, 0x00},
    // % (37)
    {0xC6, 0xCC, 0x18, 0x30, 0x60, 0xC6, 0x86, 0x00},
    // & (38)
    {0x38, 0x6C, 0x38, 0x76, 0xDC, 0xCC, 0x76, 0x00},
    // ' (39)
    {0x18, 0x18, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ( (40)
    {0x0C, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x00},
    // ) (41)
    {0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00},
    // * (42)
    {0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00},
    // + (43)
    {0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00},
    // , (44)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30},
    // - (45)
    {0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00},
    // . (46)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00},
    // / (47)
    {0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x80, 0x00},
    // 0 (48)
    {0x7C, 0xC6, 0xCE, 0xD6, 0xE6, 0xC6, 0x7C, 0x00},
    // 1 (49)
    {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00},
    // 2 (50)
    {0x7C, 0xC6, 0x06, 0x1C, 0x70, 0xC0, 0xFE, 0x00},
    // 3 (51)
    {0x7C, 0xC6, 0x06, 0x3C, 0x06, 0xC6, 0x7C, 0x00},
    // 4 (52)
    {0x1C, 0x3C, 0x6C, 0xCC, 0xFE, 0x0C, 0x0C, 0x00},
    // 5 (53)
    {0xFE, 0xC0, 0xFC, 0x06, 0x06, 0xC6, 0x7C, 0x00},
    // 6 (54)
    {0x38, 0x60, 0xC0, 0xFC, 0xC6, 0xC6, 0x7C, 0x00},
    // 7 (55)
    {0xFE, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00},
    // 8 (56)
    {0x7C, 0xC6, 0xC6, 0x7C, 0xC6, 0xC6, 0x7C, 0x00},
    // 9 (57)
    {0x7C, 0xC6, 0xC6, 0x7E, 0x06, 0x0C, 0x78, 0x00},
    // : (58)
    {0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00},
    // ; (59)
    {0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x30},
    // < (60)
    {0x0C, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0C, 0x00},
    // = (61)
    {0x00, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00, 0x00},
    // > (62)
    {0x30, 0x18, 0x0C, 0x06, 0x0C, 0x18, 0x30, 0x00},
    // ? (63)
    {0x7C, 0xC6, 0x0C, 0x18, 0x18, 0x00, 0x18, 0x00},
    // @ (64)
    {0x7C, 0xC6, 0xDE, 0xDE, 0xDC, 0xC0, 0x7C, 0x00},
    // A (65)
    {0x38, 0x6C, 0xC6, 0xC6, 0xFE, 0xC6, 0xC6, 0x00},
    // B (66)
    {0xFC, 0xC6, 0xC6, 0xFC, 0xC6, 0xC6, 0xFC, 0x00},
    // C (67)
    {0x7C, 0xC6, 0xC0, 0xC0, 0xC0, 0xC6, 0x7C, 0x00},
    // D (68)
    {0xF8, 0xCC, 0xC6, 0xC6, 0xC6, 0xCC, 0xF8, 0x00},
    // E (69)
    {0xFE, 0xC0, 0xC0, 0xF8, 0xC0, 0xC0, 0xFE, 0x00},
    // F (70)
    {0xFE, 0xC0, 0xC0, 0xF8, 0xC0, 0xC0, 0xC0, 0x00},
    // G (71)
    {0x7C, 0xC6, 0xC0, 0xCE, 0xC6, 0xC6, 0x7E, 0x00},
    // H (72)
    {0xC6, 0xC6, 0xC6, 0xFE, 0xC6, 0xC6, 0xC6, 0x00},
    // I (73)
    {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00},
    // J (74)
    {0x1E, 0x06, 0x06, 0x06, 0xC6, 0xC6, 0x7C, 0x00},
    // K (75)
    {0xC6, 0xCC, 0xD8, 0xF0, 0xD8, 0xCC, 0xC6, 0x00},
    // L (76)
    {0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xFE, 0x00},
    // M (77)
    {0xC6, 0xEE, 0xFE, 0xD6, 0xC6, 0xC6, 0xC6, 0x00},
    // N (78)
    {0xC6, 0xE6, 0xF6, 0xDE, 0xCE, 0xC6, 0xC6, 0x00},
    // O (79)
    {0x7C, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00},
    // P (80)
    {0xFC, 0xC6, 0xC6, 0xFC, 0xC0, 0xC0, 0xC0, 0x00},
    // Q (81)
    {0x7C, 0xC6, 0xC6, 0xC6, 0xD6, 0xDE, 0x7C, 0x06},
    // R (82)
    {0xFC, 0xC6, 0xC6, 0xFC, 0xD8, 0xCC, 0xC6, 0x00},
    // S (83)
    {0x7C, 0xC6, 0xC0, 0x7C, 0x06, 0xC6, 0x7C, 0x00},
    // T (84)
    {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00},
    // U (85)
    {0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00},
    // V (86)
    {0xC6, 0xC6, 0xC6, 0xC6, 0x6C, 0x38, 0x10, 0x00},
    // W (87)
    {0xC6, 0xC6, 0xC6, 0xD6, 0xFE, 0xEE, 0xC6, 0x00},
    // X (88)
    {0xC6, 0x6C, 0x38, 0x38, 0x6C, 0xC6, 0xC6, 0x00},
    // Y (89)
    {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00},
    // Z (90)
    {0xFE, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0xFE, 0x00},
    // [ (91)
    {0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00},
    // \ (92)
    {0xC0, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x02, 0x00},
    // ] (93)
    {0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3C, 0x00},
    // ^ (94)
    {0x10, 0x38, 0x6C, 0xC6, 0x00, 0x00, 0x00, 0x00},
    // _ (95)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE},
    // ` (96)
    {0x30, 0x18, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00},
    // a (97)
    {0x00, 0x00, 0x7C, 0x06, 0x7E, 0xC6, 0x7E, 0x00},
    // b (98)
    {0xC0, 0xC0, 0xFC, 0xC6, 0xC6, 0xC6, 0xFC, 0x00},
    // c (99)
    {0x00, 0x00, 0x7C, 0xC6, 0xC0, 0xC6, 0x7C, 0x00},
    // d (100)
    {0x06, 0x06, 0x7E, 0xC6, 0xC6, 0xC6, 0x7E, 0x00},
    // e (101)
    {0x00, 0x00, 0x7C, 0xC6, 0xFE, 0xC0, 0x7C, 0x00},
    // f (102)
    {0x1C, 0x30, 0x7C, 0x30, 0x30, 0x30, 0x30, 0x00},
    // g (103)
    {0x00, 0x00, 0x7E, 0xC6, 0xC6, 0x7E, 0x06, 0x7C},
    // h (104)
    {0xC0, 0xC0, 0xFC, 0xC6, 0xC6, 0xC6, 0xC6, 0x00},
    // i (105)
    {0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3C, 0x00},
    // j (106)
    {0x0C, 0x00, 0x1C, 0x0C, 0x0C, 0x0C, 0xCC, 0x78},
    // k (107)
    {0xC0, 0xC0, 0xC6, 0xCC, 0xF8, 0xCC, 0xC6, 0x00},
    // l (108)
    {0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00},
    // m (109)
    {0x00, 0x00, 0xEC, 0xFE, 0xD6, 0xD6, 0xC6, 0x00},
    // n (110)
    {0x00, 0x00, 0xFC, 0xC6, 0xC6, 0xC6, 0xC6, 0x00},
    // o (111)
    {0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0x7C, 0x00},
    // p (112)
    {0x00, 0x00, 0xFC, 0xC6, 0xC6, 0xFC, 0xC0, 0xC0},
    // q (113)
    {0x00, 0x00, 0x7E, 0xC6, 0xC6, 0x7E, 0x06, 0x06},
    // r (114)
    {0x00, 0x00, 0xDC, 0xE6, 0xC0, 0xC0, 0xC0, 0x00},
    // s (115)
    {0x00, 0x00, 0x7E, 0xC0, 0x7C, 0x06, 0xFC, 0x00},
    // t (116)
    {0x30, 0x30, 0x7C, 0x30, 0x30, 0x30, 0x1C, 0x00},
    // u (117)
    {0x00, 0x00, 0xC6, 0xC6, 0xC6, 0xC6, 0x7E, 0x00},
    // v (118)
    {0x00, 0x00, 0xC6, 0xC6, 0x6C, 0x38, 0x10, 0x00},
    // w (119)
    {0x00, 0x00, 0xC6, 0xD6, 0xD6, 0xFE, 0x6C, 0x00},
    // x (120)
    {0x00, 0x00, 0xC6, 0x6C, 0x38, 0x6C, 0xC6, 0x00},
    // y (121)
    {0x00, 0x00, 0xC6, 0xC6, 0xC6, 0x7E, 0x06, 0x7C},
    // z (122)
    {0x00, 0x00, 0xFE, 0x0C, 0x38, 0x60, 0xFE, 0x00},
    // { (123)
    {0x0E, 0x18, 0x18, 0x70, 0x18, 0x18, 0x0E, 0x00},
    // | (124)
    {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00},
    // } (125)
    {0x70, 0x18, 0x18, 0x0E, 0x18, 0x18, 0x70, 0x00},
    // ~ (126)
    {0x76, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // DEL (127) - block character for unknown
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
};

// Color definitions
static const SDL_Color COLOR_BG       = {0x1A, 0x1A, 0x2E, 0xFF};  // Dark blue-gray
static const SDL_Color COLOR_PANEL    = {0x16, 0x21, 0x3E, 0xFF};  // Darker panel
static const SDL_Color COLOR_TEXT     = {0xE0, 0xE0, 0xE0, 0xFF};  // Light gray
static const SDL_Color COLOR_LABEL    = {0x80, 0x80, 0x80, 0xFF};  // Medium gray
static const SDL_Color COLOR_VALUE    = {0x00, 0xFF, 0x80, 0xFF};  // Bright green
static const SDL_Color COLOR_PC       = {0xFF, 0xFF, 0x00, 0xFF};  // Yellow
static const SDL_Color COLOR_FLAG_ON  = {0x00, 0xFF, 0x00, 0xFF};  // Bright green
static const SDL_Color COLOR_FLAG_OFF = {0x60, 0x60, 0x60, 0xFF};  // Dark gray
static const SDL_Color COLOR_BUTTON   = {0x30, 0x50, 0x80, 0xFF};  // Button blue
static const SDL_Color COLOR_BUTTON_H = {0x40, 0x70, 0xB0, 0xFF};  // Button hover
static const SDL_Color COLOR_PAUSED   = {0xFF, 0x80, 0x00, 0xFF};  // Orange for paused
static const SDL_Color COLOR_RUNNING  = {0x00, 0xFF, 0x00, 0xFF};  // Green for running
static const SDL_Color COLOR_ADDR     = {0x80, 0xC0, 0xFF, 0xFF};  // Light blue for addresses
static const SDL_Color COLOR_HEX      = {0xFF, 0xC0, 0x80, 0xFF};  // Light orange for hex
static const SDL_Color COLOR_ERROR    = {0xFF, 0x40, 0x40, 0xFF};  // Red for errors
static const SDL_Color COLOR_HIGHLIGHT = {0xFF, 0xFF, 0x00, 0xFF}; // Yellow for highlights

// NES Master Palette (64 colors, ARGB format) - copy for debugger use
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

// Layout constants
#define PANEL_MARGIN    20
#define PANEL_PADDING   16
#define LINE_HEIGHT     24
#define SECTION_GAP     10

// NES screen panel (left side, 4:3 aspect within 16:9 window)
#define SCREEN_X        PANEL_MARGIN
#define SCREEN_Y        PANEL_MARGIN
#define SCREEN_W        512
#define SCREEN_H        480

// Registers panel (right of screen)
#define REGS_X          (SCREEN_X + SCREEN_W + SECTION_GAP)
#define REGS_Y          PANEL_MARGIN
#define REGS_W          (DEBUGGER_WINDOW_WIDTH - REGS_X - PANEL_MARGIN)
#define REGS_H          200

// Register field positions (relative offsets)
#define REG_LABEL_X     PANEL_PADDING
#define REG_VALUE_GAP   48
#define REG_FIELD_GAP   80

// Instruction panel (below registers)
#define INSTR_X         REGS_X
#define INSTR_Y         (REGS_Y + REGS_H + SECTION_GAP)
#define INSTR_W         REGS_W
#define INSTR_H         270

// Instruction field positions
#define INSTR_LABEL_WIDTH   144

// Memory panel (bottom, full width)
#define MEM_X           PANEL_MARGIN
#define MEM_Y           (SCREEN_Y + SCREEN_H + SECTION_GAP)
#define MEM_W           (DEBUGGER_WINDOW_WIDTH - 2 * PANEL_MARGIN)
#define MEM_H           160

// Control bar (bottom)
#define CTRL_X          PANEL_MARGIN
#define CTRL_Y          (DEBUGGER_WINDOW_HEIGHT - 50)
#define CTRL_W          (DEBUGGER_WINDOW_WIDTH - 2 * PANEL_MARGIN)
#define CTRL_H          40

// Title bar height within panels
#define PANEL_TITLE_Y   4
#define PANEL_CONTENT_Y 28

// PPU View panels (right side, replacing CPU panels)
// Pattern tables panel (two 128x128 grids side by side = 256x128)
#define PATTERN_X       REGS_X
#define PATTERN_Y       PANEL_MARGIN
#define PATTERN_W       REGS_W
#define PATTERN_H       190

// Palettes panel
#define PALETTES_X      REGS_X
#define PALETTES_Y      (PATTERN_Y + PATTERN_H + SECTION_GAP)
#define PALETTES_W      REGS_W
#define PALETTES_H      120

// OAM panel
#define OAM_X           REGS_X
#define OAM_Y           (PALETTES_Y + PALETTES_H + SECTION_GAP)
#define OAM_W           REGS_W
#define OAM_H           (DEBUGGER_WINDOW_HEIGHT - OAM_Y - 60 - SECTION_GAP)

// Pattern table texture dimensions (both tables side by side)
#define PATTERN_TEX_W   256
#define PATTERN_TEX_H   128

// Create the font texture from bitmap data
static bool create_font_texture(debugger_s *debugger_context) {
    // Create surface for 96 characters (16x6 grid)
    SDL_Surface *surface = SDL_CreateRGBSurface(0, 16 * 8, 6 * 8, 32,
                                                 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (!surface) {
        fprintf(stderr, "Failed to create font surface: %s\n", SDL_GetError());
        return false;
    }

    // Fill with transparent background
    SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 0));

    // Render each character
    Uint32 white = SDL_MapRGBA(surface->format, 255, 255, 255, 255);
    for (int c = 0; c < 96; c++) {
        int cx = (c % 16) * 8;
        int cy = (c / 16) * 8;

        for (int y = 0; y < 8; y++) {
            unsigned char row = font_8x8[c][y];
            for (int x = 0; x < 8; x++) {
                if (row & (0x80 >> x)) {
                    Uint32 *pixels = (Uint32 *)surface->pixels;
                    pixels[(cy + y) * surface->w + (cx + x)] = white;
                }
            }
        }
    }

    // Create texture
    debugger_context->font_texture = SDL_CreateTextureFromSurface(debugger_context->renderer, surface);
    SDL_FreeSurface(surface);

    if (!debugger_context->font_texture) {
        fprintf(stderr, "Failed to create font texture: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

// Draw a single character at position (x, y) with given color
static void draw_char(debugger_s *debugger_context, int x, int y, char c, SDL_Color color) {
    if (c < 32 || c > 127) c = 127;  // Use block for unknown characters
    int index = c - 32;
    int sx = (index % 16) * 8;
    int sy = (index / 16) * 8;

    SDL_Rect src = {sx, sy, 8, 8};
    SDL_Rect dst = {x, y, FONT_WIDTH * FONT_SCALE, FONT_HEIGHT * FONT_SCALE};

    SDL_SetTextureColorMod(debugger_context->font_texture, color.r, color.g, color.b);
    SDL_RenderCopy(debugger_context->renderer, debugger_context->font_texture, &src, &dst);
}

// Draw a string at position (x, y) with given color
static void draw_text(debugger_s *debugger_context, int x, int y, const char *text, SDL_Color color) {
    int cx = x;
    for (const char *p = text; *p; p++) {
        draw_char(debugger_context, cx, y, *p, color);
        cx += FONT_WIDTH * FONT_SCALE;
    }
}

// Draw a filled rectangle
static void draw_rect(debugger_s *debugger_context, int x, int y, int w, int h, SDL_Color color) {
    SDL_SetRenderDrawColor(debugger_context->renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(debugger_context->renderer, &rect);
}

// Draw a rectangle outline
static void draw_rect_outline(debugger_s *debugger_context, int x, int y, int w, int h, SDL_Color color) {
    SDL_SetRenderDrawColor(debugger_context->renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderDrawRect(debugger_context->renderer, &rect);
}

// Draw a panel with border
static void draw_panel(debugger_s *debugger_context, int x, int y, int w, int h, const char *title) {
    draw_rect(debugger_context, x, y, w, h, COLOR_PANEL);
    draw_rect_outline(debugger_context, x, y, w, h, COLOR_LABEL);

    if (title) {
        draw_text(debugger_context, x + 8, y + 4, title, COLOR_LABEL);
    }
}

// Format instruction bytes for display
static void format_instruction_bytes(debugger_s *debugger_context, char *buffer, size_t size) {
    cpu_instruction_s *instr = get_instruction(debugger_context->cpu, bus_read(debugger_context->bus, debugger_context->cpu->PC));
    byte_t bytes[3] = {0};
    bytes[0] = bus_read(debugger_context->bus, debugger_context->cpu->PC);
    if (instr->length > 1) bytes[1] = bus_read(debugger_context->bus, debugger_context->cpu->PC + 1);
    if (instr->length > 2) bytes[2] = bus_read(debugger_context->bus, debugger_context->cpu->PC + 2);

    switch (instr->length) {
        case 1:
            snprintf(buffer, size, "%02X", bytes[0]);
            break;
        case 2:
            snprintf(buffer, size, "%02X %02X", bytes[0], bytes[1]);
            break;
        case 3:
            snprintf(buffer, size, "%02X %02X %02X", bytes[0], bytes[1], bytes[2]);
            break;
        default:
            snprintf(buffer, size, "??");
            break;
    }
}

// Draw CPU registers panel
static void draw_registers(debugger_s *debugger_context) {
    draw_panel(debugger_context, REGS_X, REGS_Y, REGS_W, REGS_H, "CPU Registers");

    int y = REGS_Y + PANEL_CONTENT_Y;
    int x = REGS_X + REG_LABEL_X;
    char buf[32];

    // A, X, Y registers (evenly spaced)
    const int reg_spacing = REG_FIELD_GAP;
    const int label_value_gap = 32;

    draw_text(debugger_context, x, y, "A:", COLOR_LABEL);
    snprintf(buf, sizeof(buf), "%02X", debugger_context->cpu->A);
    draw_text(debugger_context, x + label_value_gap, y, buf, COLOR_VALUE);

    draw_text(debugger_context, x + reg_spacing, y, "X:", COLOR_LABEL);
    snprintf(buf, sizeof(buf), "%02X", debugger_context->cpu->X);
    draw_text(debugger_context, x + reg_spacing + label_value_gap, y, buf, COLOR_VALUE);

    draw_text(debugger_context, x + reg_spacing * 2, y, "Y:", COLOR_LABEL);
    snprintf(buf, sizeof(buf), "%02X", debugger_context->cpu->Y);
    draw_text(debugger_context, x + reg_spacing * 2 + label_value_gap, y, buf, COLOR_VALUE);

    // SP, PC
    y += LINE_HEIGHT;
    const int sp_label_gap = 48;
    const int pc_label_x = 120;
    const int pc_value_gap = 48;

    draw_text(debugger_context, x, y, "SP:", COLOR_LABEL);
    snprintf(buf, sizeof(buf), "%02X", debugger_context->cpu->SP);
    draw_text(debugger_context, x + sp_label_gap, y, buf, COLOR_VALUE);

    draw_text(debugger_context, x + pc_label_x, y, "PC:", COLOR_LABEL);
    snprintf(buf, sizeof(buf), "%04X", debugger_context->cpu->PC);
    draw_text(debugger_context, x + pc_label_x + pc_value_gap, y, buf, COLOR_PC);

    // Status flags
    y += LINE_HEIGHT + 8;
    const int flags_label_width = 96;
    const int flag_char_spacing = 20;

    draw_text(debugger_context, x, y, "Flags:", COLOR_LABEL);

    const char *flags = "NV-BDIZC";
    int flag_x = x + flags_label_width;
    for (int i = 0; i < 8; i++) {
        byte_t mask = 0x80 >> i;
        bool set = (debugger_context->cpu->STATUS & mask) != 0;
        char flag_char[2] = {flags[i], '\0'};
        draw_text(debugger_context, flag_x, y, flag_char, set ? COLOR_FLAG_ON : COLOR_FLAG_OFF);
        flag_x += flag_char_spacing;
    }

    // Status hex value and cycles
    y += LINE_HEIGHT;
    const int status_value_x = 112;
    const int cycles_label_x = 180;
    const int cycles_value_x = 112;

    draw_text(debugger_context, x, y, "Status:", COLOR_LABEL);
    snprintf(buf, sizeof(buf), "%02X", debugger_context->cpu->STATUS);
    draw_text(debugger_context, x + status_value_x, y, buf, COLOR_VALUE);

    draw_text(debugger_context, x + cycles_label_x, y, "Cycles:", COLOR_LABEL);
    snprintf(buf, sizeof(buf), "%zu", debugger_context->cpu->cycles);
    draw_text(debugger_context, x + cycles_label_x + cycles_value_x, y, buf, COLOR_VALUE);
}

// Draw current instruction panel
static void draw_instruction(debugger_s *debugger_context) {
    draw_panel(debugger_context, INSTR_X, INSTR_Y, INSTR_W, INSTR_H, "Current Instruction");

    int y = INSTR_Y + PANEL_CONTENT_Y;
    int x = INSTR_X + PANEL_PADDING;
    char buf[64];

    // Address
    draw_text(debugger_context, x, y, "Address:", COLOR_LABEL);
    snprintf(buf, sizeof(buf), "$%04X", debugger_context->cpu->PC);
    draw_text(debugger_context, x + INSTR_LABEL_WIDTH, y, buf, COLOR_ADDR);

    // Opcode bytes
    y += LINE_HEIGHT;
    draw_text(debugger_context, x, y, "Bytes:", COLOR_LABEL);
    format_instruction_bytes(debugger_context, buf, sizeof(buf));
    draw_text(debugger_context, x + INSTR_LABEL_WIDTH, y, buf, COLOR_HEX);

    // Mnemonic
    y += LINE_HEIGHT;
    cpu_instruction_s *instr = get_instruction(debugger_context->cpu, bus_read(debugger_context->bus, debugger_context->cpu->PC));
    draw_text(debugger_context, x, y, "Mnemonic:", COLOR_LABEL);
    draw_text(debugger_context, x + INSTR_LABEL_WIDTH, y, instr->name ? instr->name : "???", COLOR_VALUE);

    // Disassembly with operand
    y += LINE_HEIGHT + 8;
    draw_text(debugger_context, x, y, "Disassembly:", COLOR_LABEL);

    // Build disassembly string
    byte_t op1 = 0, op2 = 0;
    if (instr->length > 1) op1 = bus_read(debugger_context->bus, debugger_context->cpu->PC + 1);
    if (instr->length > 2) op2 = bus_read(debugger_context->bus, debugger_context->cpu->PC + 2);

    switch (instr->length) {
        case 1:
            snprintf(buf, sizeof(buf), "%s", instr->name ? instr->name : "???");
            break;
        case 2:
            snprintf(buf, sizeof(buf), "%s $%02X", instr->name ? instr->name : "???", op1);
            break;
        case 3:
            snprintf(buf, sizeof(buf), "%s $%02X%02X", instr->name ? instr->name : "???", op2, op1);
            break;
        default:
            snprintf(buf, sizeof(buf), "???");
    }
    y += LINE_HEIGHT;
    draw_text(debugger_context, x, y, buf, COLOR_VALUE);
}

// Draw memory view panel (separate pane at the bottom)
static void draw_memory(debugger_s *debugger_context) {
    // Update memory view address based on mode
    switch (debugger_context->mem_view_mode) {
        case MEM_VIEW_MODE_ZERO_PAGE:
            debugger_context->mem_view_addr = 0x0000;
            break;
        case MEM_VIEW_MODE_STACK:
            debugger_context->mem_view_addr = 0x0100;
            break;
        case MEM_VIEW_MODE_CUSTOM:
            // Keep current address
            break;
    }

    const char *title;
    switch (debugger_context->mem_view_mode) {
        case MEM_VIEW_MODE_ZERO_PAGE: title = "Memory View - Zero Page ($0000)"; break;
        case MEM_VIEW_MODE_STACK:     title = "Memory View - Stack ($0100)"; break;
        case MEM_VIEW_MODE_CUSTOM:    title = "Memory View - Custom"; break;
        default:                      title = "Memory View"; break;
    }

    draw_panel(debugger_context, MEM_X, MEM_Y, MEM_W, MEM_H, title);

    const int mem_rows = 7;
    const int bytes_per_row = 16;
    const int addr_col_width = 80;
    const int hex_byte_width = 40;
    const int hex_group_gap = 16;
    const int ascii_start_offset = addr_col_width + (bytes_per_row * hex_byte_width) + hex_group_gap + 32;
    const int ascii_char_width = 12;
    const int mem_row_height = 18;

    int y = MEM_Y + PANEL_CONTENT_Y;
    int x = MEM_X + PANEL_PADDING;
    char buf[128];

    for (int row = 0; row < mem_rows; row++) {
        word_t addr = debugger_context->mem_view_addr + row * bytes_per_row;

        // Address column
        snprintf(buf, sizeof(buf), "%04X:", addr);
        draw_text(debugger_context, x, y, buf, COLOR_ADDR);

        // Hex bytes
        int hx = x + addr_col_width;
        for (int col = 0; col < bytes_per_row; col++) {
            byte_t val = bus_read(debugger_context->bus, addr + col);
            snprintf(buf, sizeof(buf), "%02X", val);
            draw_text(debugger_context, hx, y, buf, COLOR_HEX);
            hx += hex_byte_width;
            if (col == 7) hx += hex_group_gap;  // Gap between byte groups
        }

        // ASCII representation
        int ax = x + ascii_start_offset;
        for (int col = 0; col < bytes_per_row; col++) {
            byte_t val = bus_read(debugger_context->bus, addr + col);
            char c = (val >= 32 && val < 127) ? (char)val : '.';
            char str[2] = {c, '\0'};
            draw_text(debugger_context, ax, y, str, COLOR_TEXT);
            ax += ascii_char_width;
        }

        y += mem_row_height;
    }
}

// Update screen texture from PPU framebuffer
static void update_screen_texture(debugger_s *debugger_context) {
    void *pixels;
    int pitch;

    if (SDL_LockTexture(debugger_context->screen_texture, NULL, &pixels, &pitch) == 0) {
        uint32_t *framebuffer = ppu_get_framebuffer(&debugger_context->bus->ppu);

        // Copy framebuffer to texture (row by row in case pitch differs)
        for (int y = 0; y < PPU_SCREEN_HEIGHT; y++) {
            memcpy(
                (uint8_t *)pixels + y * pitch,
                framebuffer + y * PPU_SCREEN_WIDTH,
                PPU_SCREEN_WIDTH * sizeof(uint32_t)
            );
        }

        SDL_UnlockTexture(debugger_context->screen_texture);
    }
}

// Update pattern table texture from CHR ROM
// Renders both pattern tables (PT0 at $0000, PT1 at $1000) side by side
static void update_pattern_texture(debugger_s *debugger_context) {
    void *pixels;
    int pitch;

    if (SDL_LockTexture(debugger_context->pattern_texture, NULL, &pixels, &pitch) == 0) {
        ppu_s *ppu = &debugger_context->bus->ppu;
        int palette_base = debugger_context->ppu_palette_select * 4;

        // Get the 4 colors for the selected palette
        uint32_t colors[4];
        colors[0] = NES_PALETTE[ppu->palette[0] & 0x3F];  // Background color is always palette[0]
        for (int i = 1; i < 4; i++) {
            colors[i] = NES_PALETTE[ppu->palette[palette_base + i] & 0x3F];
        }

        // Render both pattern tables (256 tiles each, 16x16 grid of 8x8 tiles)
        for (int table = 0; table < 2; table++) {
            word_t table_base = table * 0x1000;
            int x_offset = table * 128;  // PT1 starts at x=128

            for (int tile_y = 0; tile_y < 16; tile_y++) {
                for (int tile_x = 0; tile_x < 16; tile_x++) {
                    int tile_index = tile_y * 16 + tile_x;
                    word_t tile_addr = table_base + tile_index * 16;

                    // Decode 8x8 tile (2 bitplanes)
                    for (int row = 0; row < 8; row++) {
                        byte_t plane0 = ppu_vram_read(ppu, tile_addr + row);
                        byte_t plane1 = ppu_vram_read(ppu, tile_addr + row + 8);

                        for (int col = 0; col < 8; col++) {
                            int bit = 7 - col;
                            int color_index = ((plane0 >> bit) & 1) | (((plane1 >> bit) & 1) << 1);

                            int px = x_offset + tile_x * 8 + col;
                            int py = tile_y * 8 + row;

                            uint32_t *row_pixels = (uint32_t *)((uint8_t *)pixels + py * pitch);
                            row_pixels[px] = colors[color_index];
                        }
                    }
                }
            }
        }

        SDL_UnlockTexture(debugger_context->pattern_texture);
    }
}

// Draw pattern tables panel (PPU view)
static void draw_pattern_tables(debugger_s *debugger_context) {
    draw_panel(debugger_context, PATTERN_X, PATTERN_Y, PATTERN_W, PATTERN_H, "Pattern Tables");

    int content_y = PATTERN_Y + PANEL_CONTENT_Y;
    int content_x = PATTERN_X + PANEL_PADDING;

    // Draw the pattern texture scaled to fit
    int tex_scale = 1;
    int scaled_w = PATTERN_TEX_W * tex_scale;
    int scaled_h = PATTERN_TEX_H * tex_scale;

    SDL_Rect dst = {content_x, content_y, scaled_w, scaled_h};
    SDL_RenderCopy(debugger_context->renderer, debugger_context->pattern_texture, NULL, &dst);

    // Show palette indicator
    char buf[32];
    snprintf(buf, sizeof(buf), "Palette: %d", debugger_context->ppu_palette_select);
    draw_text(debugger_context, content_x + scaled_w + 20, content_y, buf, COLOR_LABEL);

    // Label the pattern tables
    draw_text(debugger_context, content_x + 40, content_y + scaled_h + 4, "PT0", COLOR_LABEL);
    draw_text(debugger_context, content_x + 168, content_y + scaled_h + 4, "PT1", COLOR_LABEL);
}

// Draw palettes panel (PPU view)
static void draw_palettes(debugger_s *debugger_context) {
    draw_panel(debugger_context, PALETTES_X, PALETTES_Y, PALETTES_W, PALETTES_H, "Palettes");

    ppu_s *ppu = &debugger_context->bus->ppu;
    int content_y = PALETTES_Y + PANEL_CONTENT_Y;
    int content_x = PALETTES_X + PANEL_PADDING;

    const int swatch_size = 16;
    const int swatch_gap = 4;
    const int palette_gap = 20;

    // Draw 8 palettes (4 BG + 4 Sprite)
    for (int pal = 0; pal < 8; pal++) {
        int row = pal / 4;  // 0 = BG, 1 = Sprite
        int col = pal % 4;

        int base_x = content_x + col * (4 * swatch_size + 4 * swatch_gap + palette_gap);
        int base_y = content_y + row * (swatch_size + 24);

        // Label
        char label[8];
        snprintf(label, sizeof(label), "%s%d", row == 0 ? "BG" : "SP", col);
        draw_text(debugger_context, base_x, base_y - 2, label, COLOR_LABEL);

        // Draw 4 color swatches
        for (int c = 0; c < 4; c++) {
            int pal_index = pal * 4 + c;
            // Background color (index 0, 4, 8, 12...) mirrors palette[0]
            byte_t nes_color = (c == 0) ? ppu->palette[0] : ppu->palette[pal_index];
            uint32_t argb = NES_PALETTE[nes_color & 0x3F];

            int sx = base_x + c * (swatch_size + swatch_gap);
            int sy = base_y + 16;

            // Convert ARGB to SDL_Color (ARGB -> R, G, B)
            SDL_Color swatch_color = {
                (argb >> 16) & 0xFF,
                (argb >> 8) & 0xFF,
                argb & 0xFF,
                0xFF
            };
            draw_rect(debugger_context, sx, sy, swatch_size, swatch_size, swatch_color);

            // Highlight selected palette
            if (pal == debugger_context->ppu_palette_select && c == 0) {
                draw_rect_outline(debugger_context, sx - 2, sy - 2,
                                  4 * (swatch_size + swatch_gap) - swatch_gap + 4,
                                  swatch_size + 4, COLOR_HIGHLIGHT);
            }
        }
    }
}

// Draw OAM sprites panel (PPU view)
static void draw_oam(debugger_s *debugger_context) {
    draw_panel(debugger_context, OAM_X, OAM_Y, OAM_W, OAM_H, "OAM Sprites");

    ppu_s *ppu = &debugger_context->bus->ppu;
    int content_y = OAM_Y + PANEL_CONTENT_Y;
    int content_x = OAM_X + PANEL_PADDING;

    const int line_height = 18;
    const int visible_lines = (OAM_H - PANEL_CONTENT_Y - 10) / line_height;
    char buf[64];

    // Draw scroll indicator
    snprintf(buf, sizeof(buf), "[%d-%d/64]",
             debugger_context->oam_scroll_offset,
             debugger_context->oam_scroll_offset + visible_lines - 1);
    draw_text(debugger_context, OAM_X + OAM_W - 120, OAM_Y + 4, buf, COLOR_LABEL);

    for (int i = 0; i < visible_lines && (i + debugger_context->oam_scroll_offset) < 64; i++) {
        int sprite = i + debugger_context->oam_scroll_offset;
        int y_pos = content_y + i * line_height;

        byte_t sprite_y = ppu->oam[sprite * 4 + 0];
        byte_t tile_idx = ppu->oam[sprite * 4 + 1];
        byte_t attrs = ppu->oam[sprite * 4 + 2];
        byte_t sprite_x = ppu->oam[sprite * 4 + 3];

        // Format: #NN: Y=YY T=$TT A=AA X=XX
        snprintf(buf, sizeof(buf), "#%02d: Y=%3d T=$%02X A=%02X X=%3d",
                 sprite, sprite_y, tile_idx, attrs, sprite_x);

        // Dim sprites that are off-screen (Y >= 240 or Y == 0)
        SDL_Color text_color = (sprite_y == 0 || sprite_y >= 240) ? COLOR_LABEL : COLOR_TEXT;
        draw_text(debugger_context, content_x, y_pos, buf, text_color);

        // Show attribute details
        char attr_str[32];
        snprintf(attr_str, sizeof(attr_str), "P%d %s%s%s",
                 attrs & 0x03,
                 (attrs & 0x20) ? "B" : "F",  // Behind/Front of BG
                 (attrs & 0x40) ? "H" : "-",  // Horizontal flip
                 (attrs & 0x80) ? "V" : "-"); // Vertical flip
        draw_text(debugger_context, content_x + 320, y_pos, attr_str, COLOR_LABEL);
    }
}

// Draw NES screen
static void draw_screen(debugger_s *debugger_context) {
    const int screen_border = 8;
    const int screen_content_x = SCREEN_X + screen_border;
    const int screen_content_y = SCREEN_Y + PANEL_CONTENT_Y;
    const int screen_content_w = SCREEN_W - (screen_border * 2);
    const int screen_content_h = SCREEN_H - PANEL_CONTENT_Y - screen_border;

    draw_panel(debugger_context, SCREEN_X, SCREEN_Y, SCREEN_W, SCREEN_H, "NES Screen");

    // Draw the NES screen texture scaled to fit the panel
    SDL_Rect dst = {screen_content_x, screen_content_y, screen_content_w, screen_content_h};
    SDL_RenderCopy(debugger_context->renderer, debugger_context->screen_texture, NULL, &dst);
}

// Draw control bar
static void draw_controls(debugger_s *debugger_context) {
    draw_rect(debugger_context, CTRL_X, CTRL_Y, CTRL_W, CTRL_H, COLOR_PANEL);

    int x = CTRL_X + PANEL_PADDING;
    int y = CTRL_Y + 10;

    // Status indicator
    const char *status;
    SDL_Color status_color;
    if (debugger_context->quit_requested) {
        status = "QUIT?";
        status_color = COLOR_PAUSED;
    } else if (debugger_context->paused) {
        status = "PAUSED";
        status_color = COLOR_PAUSED;
    } else {
        status = "RUNNING";
        status_color = COLOR_RUNNING;
    }
    draw_text(debugger_context, x, y, status, status_color);

    // Controls help - different based on view mode
    const int status_width = 140;
    x += status_width;
    if (debugger_context->debug_view_mode == DEBUG_VIEW_CPU) {
        draw_text(debugger_context, x, y, "[SPACE]=Step [P]=Pause [R]=Reset [V]=PPU View [Q]=Quit", COLOR_LABEL);
    } else {
        draw_text(debugger_context, x, y, "[V]=CPU View [1-8]=Palette [UP/DN]=Scroll OAM [P]=Pause [Q]=Quit", COLOR_LABEL);
    }
}

// Reset CPU to initial state
static void reset_to_init_state(debugger_s *debugger_context) {
    debugger_context->cpu->PC = debugger_context->init_state.PC;
    debugger_context->cpu->SP = debugger_context->init_state.SP;
    debugger_context->cpu->STATUS = debugger_context->init_state.STATUS;
    debugger_context->cpu->A = debugger_context->init_state.A;
    debugger_context->cpu->X = debugger_context->init_state.X;
    debugger_context->cpu->Y = debugger_context->init_state.Y;
    debugger_context->cpu->cycles = 7; // Reset to initial cycle count
    debugger_context->illegal_opcode = false;
    debugger_context->paused = true;
}

// Handle keyboard input
static void handle_input(debugger_s *debugger_context, SDL_Event *event) {
    if (event->type == SDL_KEYDOWN) {
        // Handle illegal opcode state - only space resets
        if (debugger_context->illegal_opcode) {
            if (event->key.keysym.sym == SDLK_SPACE) {
                reset_to_init_state(debugger_context);
            }
            return;
        }

        switch (event->key.keysym.sym) {
            case SDLK_ESCAPE:
            case SDLK_q:
                // Quit with confirmation (press twice to quit)
                if (debugger_context->quit_requested) {
                    debugger_context->running = false;
                } else {
                    debugger_context->quit_requested = true;
                    printf("Press ESC or Q again to confirm quit\n");
                }
                break;

            case SDLK_SPACE:
            case SDLK_s:
                // Step one instruction
                debugger_context->quit_requested = false;  // Reset quit on any other key
                if (debugger_context->paused) {
                    debugger_context->step_requested = true;
                }
                break;

            case SDLK_p:
                // Toggle pause
                debugger_context->quit_requested = false;
                debugger_context->paused = !debugger_context->paused;
                break;

            case SDLK_r:
                // Reset CPU
                debugger_context->quit_requested = false;
                reset(debugger_context->cpu);
                break;

            case SDLK_z:
                // View zero page
                debugger_context->quit_requested = false;
                debugger_context->mem_view_mode = MEM_VIEW_MODE_ZERO_PAGE;
                break;

            case SDLK_t:
                // View stack (in the dedicated memory pane)
                debugger_context->quit_requested = false;
                debugger_context->mem_view_mode = MEM_VIEW_MODE_STACK;
                break;

            case SDLK_UP:
                debugger_context->quit_requested = false;
                if (debugger_context->debug_view_mode == DEBUG_VIEW_PPU) {
                    // Scroll OAM up
                    if (debugger_context->oam_scroll_offset > 0) {
                        debugger_context->oam_scroll_offset--;
                    }
                } else {
                    // Scroll memory up
                    if (debugger_context->mem_view_addr >= 16) {
                        debugger_context->mem_view_addr -= 16;
                        debugger_context->mem_view_mode = MEM_VIEW_MODE_CUSTOM;
                    }
                }
                break;

            case SDLK_DOWN:
                debugger_context->quit_requested = false;
                if (debugger_context->debug_view_mode == DEBUG_VIEW_PPU) {
                    // Scroll OAM down
                    if (debugger_context->oam_scroll_offset < 54) {  // 64 - ~10 visible
                        debugger_context->oam_scroll_offset++;
                    }
                } else {
                    // Scroll memory down
                    if (debugger_context->mem_view_addr < 0xFFF0) {
                        debugger_context->mem_view_addr += 16;
                        debugger_context->mem_view_mode = MEM_VIEW_MODE_CUSTOM;
                    }
                }
                break;

            case SDLK_PAGEUP:
                // Page up in memory
                debugger_context->quit_requested = false;
                if (debugger_context->mem_view_addr >= 0x70) {
                    debugger_context->mem_view_addr -= 0x70;
                    debugger_context->mem_view_mode = MEM_VIEW_MODE_CUSTOM;
                } else {
                    debugger_context->mem_view_addr = 0;
                    debugger_context->mem_view_mode = MEM_VIEW_MODE_CUSTOM;
                }
                break;

            case SDLK_PAGEDOWN:
                // Page down in memory
                debugger_context->quit_requested = false;
                if (debugger_context->mem_view_addr < 0xFF90) {
                    debugger_context->mem_view_addr += 0x70;
                    debugger_context->mem_view_mode = MEM_VIEW_MODE_CUSTOM;
                }
                break;

            case SDLK_PLUS:
            case SDLK_EQUALS:
                // Increase run speed
                debugger_context->quit_requested = false;
                if (debugger_context->run_speed < 1000) {
                    debugger_context->run_speed = debugger_context->run_speed == 0 ? 1 : debugger_context->run_speed * 2;
                }
                break;

            case SDLK_MINUS:
                // Decrease run speed
                debugger_context->quit_requested = false;
                if (debugger_context->run_speed > 1) {
                    debugger_context->run_speed /= 2;
                } else {
                    debugger_context->run_speed = 0;  // Unlimited
                }
                break;

            case SDLK_d:
                // Toggle debug/play mode
                debugger_context->quit_requested = false;
                debugger_context->play_mode = !debugger_context->play_mode;
                break;

            case SDLK_v:
                // Toggle CPU/PPU view mode
                debugger_context->quit_requested = false;
                if (debugger_context->debug_view_mode == DEBUG_VIEW_CPU) {
                    debugger_context->debug_view_mode = DEBUG_VIEW_PPU;
                } else {
                    debugger_context->debug_view_mode = DEBUG_VIEW_CPU;
                }
                break;

            // Number keys 1-8 for palette selection (PPU view)
            case SDLK_1:
                debugger_context->quit_requested = false;
                debugger_context->ppu_palette_select = 0;
                break;
            case SDLK_2:
                debugger_context->quit_requested = false;
                debugger_context->ppu_palette_select = 1;
                break;
            case SDLK_3:
                debugger_context->quit_requested = false;
                debugger_context->ppu_palette_select = 2;
                break;
            case SDLK_4:
                debugger_context->quit_requested = false;
                debugger_context->ppu_palette_select = 3;
                break;
            case SDLK_5:
                debugger_context->quit_requested = false;
                debugger_context->ppu_palette_select = 4;
                break;
            case SDLK_6:
                debugger_context->quit_requested = false;
                debugger_context->ppu_palette_select = 5;
                break;
            case SDLK_7:
                debugger_context->quit_requested = false;
                debugger_context->ppu_palette_select = 6;
                break;
            case SDLK_8:
                debugger_context->quit_requested = false;
                debugger_context->ppu_palette_select = 7;
                break;

            default:
                debugger_context->quit_requested = false;
                break;
        }
    }
}

// Draw full-screen game view (play mode)
static void draw_game_fullscreen(debugger_s *debugger_context) {
    // Draw black letterbox background
    draw_rect(debugger_context, 0, 0, DEBUGGER_WINDOW_WIDTH, DEBUGGER_WINDOW_HEIGHT, (SDL_Color){0, 0, 0, 255});

    // Calculate centered position maintaining 256:240 (16:15) aspect ratio
    int scale = DEBUGGER_WINDOW_HEIGHT / PPU_SCREEN_HEIGHT;  // 720/240 = 3
    int screen_w = PPU_SCREEN_WIDTH * scale;
    int screen_h = PPU_SCREEN_HEIGHT * scale;
    int screen_x = (DEBUGGER_WINDOW_WIDTH - screen_w) / 2;
    int screen_y = (DEBUGGER_WINDOW_HEIGHT - screen_h) / 2;

    // Draw the NES screen texture scaled
    SDL_Rect dst = {screen_x, screen_y, screen_w, screen_h};
    SDL_RenderCopy(debugger_context->renderer, debugger_context->screen_texture, NULL, &dst);

    // Show control hint at bottom
    const char *hint = "[D] Debug view  [P] Pause  [Q] Quit";
    int text_x = (DEBUGGER_WINDOW_WIDTH - strlen(hint) * FONT_WIDTH * FONT_SCALE) / 2;
    draw_text(debugger_context, text_x, DEBUGGER_WINDOW_HEIGHT - 30, hint, (SDL_Color){80, 80, 80, 255});
}

// Render all debugger UI
// Draw error overlay for illegal opcode
static void draw_error_overlay(debugger_s *debugger_context) {
    // Semi-transparent dark overlay
    SDL_SetRenderDrawBlendMode(debugger_context->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(debugger_context->renderer, 0, 0, 0, 200);
    SDL_Rect overlay = {0, 0, DEBUGGER_WINDOW_WIDTH, DEBUGGER_WINDOW_HEIGHT};
    SDL_RenderFillRect(debugger_context->renderer, &overlay);

    // Error message centered on screen
    const char *error_msg = "Unimplemented Opcode! Press space to reset";
    int text_len = strlen(error_msg);
    int text_width = text_len * FONT_WIDTH * FONT_SCALE;
    int text_x = (DEBUGGER_WINDOW_WIDTH - text_width) / 2;
    int text_y = DEBUGGER_WINDOW_HEIGHT / 2 - (FONT_HEIGHT * FONT_SCALE / 2);

    draw_text(debugger_context, text_x, text_y, error_msg, COLOR_ERROR);
}

static void render(debugger_s *debugger_context) {
    // Clear screen
    SDL_SetRenderDrawColor(debugger_context->renderer, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, COLOR_BG.a);
    SDL_RenderClear(debugger_context->renderer);

    if (debugger_context->play_mode) {
        // Full-screen game mode (--play flag)
        draw_game_fullscreen(debugger_context);
    } else {
        // Debug mode - draw NES screen (always visible)
        draw_screen(debugger_context);

        // Draw right-side panels based on view mode
        if (debugger_context->debug_view_mode == DEBUG_VIEW_CPU) {
            // CPU view - show registers, instruction, memory
            draw_registers(debugger_context);
            draw_instruction(debugger_context);
            draw_memory(debugger_context);
        } else {
            // PPU view - show pattern tables, palettes, OAM
            update_pattern_texture(debugger_context);
            draw_pattern_tables(debugger_context);
            draw_palettes(debugger_context);
            draw_oam(debugger_context);
        }

        draw_controls(debugger_context);
    }

    // Draw error overlay if we hit an illegal opcode
    if (debugger_context->illegal_opcode) {
        draw_error_overlay(debugger_context);
    }

    // Present
    SDL_RenderPresent(debugger_context->renderer);
}

bool debugger_init(debugger_s *debugger_context, cpu_s *cpu, bus_s *bus) {
    memset(debugger_context, 0, sizeof(*debugger_context));
    debugger_context->cpu = cpu;
    debugger_context->bus = bus;
    debugger_context->running = true;
    debugger_context->paused = true;  // Start paused
    debugger_context->step_requested = false;
    debugger_context->quit_requested = false;
    debugger_context->play_mode = false;
    debugger_context->illegal_opcode = false;
    debugger_context->mem_view_mode = MEM_VIEW_MODE_ZERO_PAGE;
    debugger_context->mem_view_addr = 0x0000;
    debugger_context->debug_view_mode = DEBUG_VIEW_CPU;
    debugger_context->ppu_palette_select = 0;
    debugger_context->oam_scroll_offset = 0;
    debugger_context->run_speed = 100;  // 100 instructions per frame when running

    // Save initial CPU state for reset
    debugger_context->init_state.PC = cpu->PC;
    debugger_context->init_state.SP = cpu->SP;
    debugger_context->init_state.STATUS = cpu->STATUS;
    debugger_context->init_state.A = cpu->A;
    debugger_context->init_state.X = cpu->X;
    debugger_context->init_state.Y = cpu->Y;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return false;
    }

    // Create window
    debugger_context->window = SDL_CreateWindow(
        "NES EMU DEBUGGER",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        DEBUGGER_WINDOW_WIDTH,
        DEBUGGER_WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!debugger_context->window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    // Create renderer
    debugger_context->renderer = SDL_CreateRenderer(debugger_context->window, -1,
                                        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!debugger_context->renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(debugger_context->window);
        SDL_Quit();
        return false;
    }

    // Enable alpha blending
    SDL_SetRenderDrawBlendMode(debugger_context->renderer, SDL_BLENDMODE_BLEND);

    // Create font texture
    if (!create_font_texture(debugger_context)) {
        SDL_DestroyRenderer(debugger_context->renderer);
        SDL_DestroyWindow(debugger_context->window);
        SDL_Quit();
        return false;
    }

    // Create screen texture (256x240, streaming for frequent updates)
    debugger_context->screen_texture = SDL_CreateTexture(
        debugger_context->renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        PPU_SCREEN_WIDTH,
        PPU_SCREEN_HEIGHT
    );
    if (!debugger_context->screen_texture) {
        fprintf(stderr, "Failed to create screen texture: %s\n", SDL_GetError());
        SDL_DestroyTexture(debugger_context->font_texture);
        SDL_DestroyRenderer(debugger_context->renderer);
        SDL_DestroyWindow(debugger_context->window);
        SDL_Quit();
        return false;
    }

    // Create pattern table texture (256x128, both tables side by side)
    debugger_context->pattern_texture = SDL_CreateTexture(
        debugger_context->renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        PATTERN_TEX_W,
        PATTERN_TEX_H
    );
    if (!debugger_context->pattern_texture) {
        fprintf(stderr, "Failed to create pattern texture: %s\n", SDL_GetError());
        SDL_DestroyTexture(debugger_context->screen_texture);
        SDL_DestroyTexture(debugger_context->font_texture);
        SDL_DestroyRenderer(debugger_context->renderer);
        SDL_DestroyWindow(debugger_context->window);
        SDL_Quit();
        return false;
    }

    return true;
}

// Execute one CPU instruction and tick PPU accordingly (3 PPU cycles per CPU cycle)
static void execute_with_ppu(debugger_s *debugger_context) {
    // Get cycles before instruction
    size_t cycles_before = debugger_context->cpu->cycles;

    // Execute one CPU instruction
    run_instruction(debugger_context->cpu);

    // Calculate how many CPU cycles the instruction took
    size_t cpu_cycles = debugger_context->cpu->cycles - cycles_before;

    // Tick PPU 3 times per CPU cycle (NTSC timing)
    for (size_t i = 0; i < cpu_cycles * 3; i++) {
        ppu_tick(&debugger_context->bus->ppu);
    }
}

void debugger_run(debugger_s *debugger_context) {
    SDL_Event event;
    bool frame_updated = false;

    while (debugger_context->running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                debugger_context->running = false;
            } else {
                handle_input(debugger_context, &event);
            }
        }

        frame_updated = false;

        // Don't execute if we hit an illegal opcode
        if (!debugger_context->illegal_opcode) {
            // Execute CPU instructions
            if (!debugger_context->paused) {
                // Run until we complete a frame (or hit instruction limit for safety)
                int max_instructions = 100000;  // Safety limit
                for (int i = 0; i < max_instructions; i++) {
                    // Check for illegal opcode before executing
                    byte_t opcode = bus_read(debugger_context->bus, debugger_context->cpu->PC);
                    if (is_illegal_opcode(debugger_context->cpu, opcode)) {
                        debugger_context->illegal_opcode = true;
                        debugger_context->paused = true;
                        break;
                    }

                    execute_with_ppu(debugger_context);

                    // Check if PPU completed a frame
                    if (ppu_frame_complete(&debugger_context->bus->ppu)) {
                        frame_updated = true;
                        break;  // One frame per render cycle
                    }
                }
            } else if (debugger_context->step_requested) {
                // Check for illegal opcode before stepping
                byte_t opcode = bus_read(debugger_context->bus, debugger_context->cpu->PC);
                if (is_illegal_opcode(debugger_context->cpu, opcode)) {
                    debugger_context->illegal_opcode = true;
                } else {
                    execute_with_ppu(debugger_context);
                    frame_updated = true;  // Update screen after step
                }
                debugger_context->step_requested = false;
            }
        }

        // Update screen texture if frame was rendered
        if (frame_updated || debugger_context->paused) {
            update_screen_texture(debugger_context);
        }

        // Render
        render(debugger_context);

        // Cap frame rate when paused or in error state to reduce CPU usage
        if (debugger_context->paused || debugger_context->illegal_opcode) {
            SDL_Delay(16);  // ~60 FPS
        }
    }
}

void debugger_cleanup(debugger_s *debugger_context) {
    if (debugger_context->pattern_texture) {
        SDL_DestroyTexture(debugger_context->pattern_texture);
        debugger_context->pattern_texture = NULL;
    }
    if (debugger_context->screen_texture) {
        SDL_DestroyTexture(debugger_context->screen_texture);
        debugger_context->screen_texture = NULL;
    }
    if (debugger_context->font_texture) {
        SDL_DestroyTexture(debugger_context->font_texture);
        debugger_context->font_texture = NULL;
    }
    if (debugger_context->renderer) {
        SDL_DestroyRenderer(debugger_context->renderer);
        debugger_context->renderer = NULL;
    }
    if (debugger_context->window) {
        SDL_DestroyWindow(debugger_context->window);
        debugger_context->window = NULL;
    }
    SDL_Quit();
}

// Test ROM constants (nestest automation mode)
#define NESTEST_ROM_PATH "roms/nestest.nes"
#define NESTEST_START_PC 0xC000
#define NESTEST_INITIAL_SP 0xFD
#define NESTEST_INITIAL_STATUS 0x24

// Main function for standalone debugger
int main(int argc, char *argv[]) {
    const char *rom_path = NULL;
    bool play_mode = false;
    bool show_rom_info = false;
    bool test_rom_mode = false;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--rom") == 0 && i + 1 < argc) {
            rom_path = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "--play") == 0) {
            play_mode = true;
        } else if (strcmp(argv[i], "--rom-info") == 0) {
            show_rom_info = true;
        } else if (strcmp(argv[i], "--test-rom") == 0) {
            test_rom_mode = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("\nOptions:\n");
            printf("  --rom <path>  Load ROM from path\n");
            printf("  --test-rom    Load nestest.nes with automation state (PC=$C000, SP=$FD, P=$24)\n");
            printf("  --play        Start in full-screen play mode\n");
            printf("  --rom-info    Print ROM header information\n");
            printf("  --help, -h    Show this help message\n");
            return 0;
        }
    }

    // Handle --test-rom mode
    if (test_rom_mode) {
        rom_path = NESTEST_ROM_PATH;
    }

    // Check that we have a ROM path
    if (rom_path == NULL) {
        fprintf(stderr, "Usage: %s [options]\n", argv[0]);
        fprintf(stderr, "\nOptions:\n");
        fprintf(stderr, "  --rom <path>  Load ROM from path\n");
        fprintf(stderr, "  --test-rom    Load nestest.nes with automation state (PC=$C000, SP=$FD, P=$24)\n");
        fprintf(stderr, "  --play        Start in full-screen play mode\n");
        fprintf(stderr, "  --rom-info    Print ROM header information\n");
        fprintf(stderr, "  --help, -h    Show this help message\n");
        return 1;
    }

    // Load cartridge
    gamecart_s cart;
    if (!gamecart_load(rom_path, &cart)) {
        fprintf(stderr, "Failed to load ROM: %s\n", rom_path);
        return 1;
    }

    printf("Loaded: %s\n", rom_path);
    if (show_rom_info) {
        ines_print_info(&cart.rom);
    }

    // Initialize bus
    bus_s bus;
    bus_init(&bus);

    // Create PPU on the stack and attach to bus
    ppu_s ppu;
    ppu_init(&ppu);
    bus.ppu = &ppu;

    // Attach cartridge and load CHR ROM into PPU
    bus_attach_cart(&bus, &cart);
    if (cart.rom.chr_rom != NULL && cart.rom.chr_rom_bytes > 0) {
        printf("CHR ROM: %zu bytes loaded\n", cart.rom.chr_rom_bytes);
    } else {
        printf("CHR ROM: None (uses CHR RAM)\n");
    }

    // Set mirroring mode
    mirroring_mode_e mirror_mode = cart.mirroring_vertical ? MIRROR_VERTICAL : MIRROR_HORIZONTAL;
    bus_set_mirroring(&bus, mirror_mode);
    printf("Mirroring: %s\n", cart.mirroring_vertical ? "Vertical" : "Horizontal");

    cpu_s cpu;
    cpu_init(&cpu, &bus);
    bus.cpu = &cpu;

    // Set CPU state based on mode
    if (test_rom_mode) {
        cpu->PC = NESTEST_START_PC;
        cpu->SP = NESTEST_INITIAL_SP;
        cpu->STATUS = NESTEST_INITIAL_STATUS;
        printf("Test ROM mode: PC=$%04X, SP=$%02X, P=$%02X\n", cpu->PC, cpu->SP, cpu->STATUS);
    } else {
        word_t reset_vector = bus_read_word(&bus, 0xFFFC);
        cpu->PC = reset_vector;
        printf("Starting at PC=$%04X (reset vector)\n", cpu->PC);
    }

    // Initialize and run debugger
    debugger_s debugger_context;
    if (!debugger_init(&debugger_context, &cpu, &bus)) {
        fprintf(stderr, "Failed to initialize debugger\n");
        ines_free(&rom);
        return 1;
    }

    // Set play mode if requested
    if (play_mode) {
        debugger_context.play_mode = true;
        debugger_context.paused = false;  // Start running in play mode
    }

    printf("\nDebugger started. Press P to run/pause, SPACE to step, D to toggle debug view, Q/ESC to quit.\n");
    debugger_run(&debugger_context);

    // Cleanup
    debugger_cleanup(&debugger_context);
    ines_free(&rom);
    return 0;
}
