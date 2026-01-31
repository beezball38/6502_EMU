// SDL2 Visual Debugger for 6502 emulator
// Uses a simple 8x8 bitmap font for text rendering

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

    // Status hex value
    y += LINE_HEIGHT;
    const int status_value_x = 112;

    draw_text(debugger_context, x, y, "Status:", COLOR_LABEL);
    snprintf(buf, sizeof(buf), "%02X", debugger_context->cpu->STATUS);
    draw_text(debugger_context, x + status_value_x, y, buf, COLOR_VALUE);

    // Cycles
    const int cycles_label_x = 180;
    const int cycles_value_x = 112;

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

// Draw NES screen placeholder
static void draw_screen_placeholder(debugger_s *debugger_context) {
    const int screen_border = 8;
    const int screen_content_x = SCREEN_X + screen_border;
    const int screen_content_y = SCREEN_Y + PANEL_CONTENT_Y;
    const int screen_content_w = SCREEN_W - (screen_border * 2);
    const int screen_content_h = SCREEN_H - PANEL_CONTENT_Y - screen_border;

    draw_panel(debugger_context, SCREEN_X, SCREEN_Y, SCREEN_W, SCREEN_H, "NES Screen (placeholder)");

    // Draw a dark area representing where the NES screen would go
    draw_rect(debugger_context, screen_content_x, screen_content_y, screen_content_w, screen_content_h, (SDL_Color){0, 0, 0, 255});

    // Draw centered placeholder text
    const char *placeholder_text = "PPU not implemented";
    int text_len = strlen(placeholder_text);
    int text_width = text_len * FONT_WIDTH * FONT_SCALE;
    int text_x = screen_content_x + (screen_content_w - text_width) / 2;
    int text_y = screen_content_y + (screen_content_h / 2) - (FONT_HEIGHT * FONT_SCALE / 2);
    draw_text(debugger_context, text_x, text_y, placeholder_text, COLOR_LABEL);
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

    // Controls help
    const int status_width = 140;
    x += status_width;
    draw_text(debugger_context, x, y, "[SPACE]=Step  [P]=Pause  [R]=Reset  [Z]=ZP  [T]=Stack  [Q/ESC]=Quit", COLOR_LABEL);
}

// Handle keyboard input
static void handle_input(debugger_s *debugger_context, SDL_Event *event) {
    if (event->type == SDL_KEYDOWN) {
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
                // Scroll memory up
                debugger_context->quit_requested = false;
                if (debugger_context->mem_view_addr >= 16) {
                    debugger_context->mem_view_addr -= 16;
                    debugger_context->mem_view_mode = MEM_VIEW_MODE_CUSTOM;
                }
                break;

            case SDLK_DOWN:
                // Scroll memory down
                debugger_context->quit_requested = false;
                if (debugger_context->mem_view_addr < 0xFFF0) {
                    debugger_context->mem_view_addr += 16;
                    debugger_context->mem_view_mode = MEM_VIEW_MODE_CUSTOM;
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

            default:
                debugger_context->quit_requested = false;
                break;
        }
    }
}

// Draw full-screen game view (play mode)
static void draw_game_fullscreen(debugger_s *debugger_context) {
    // Draw a full-window black area for the game
    draw_rect(debugger_context, 0, 0, DEBUGGER_WINDOW_WIDTH, DEBUGGER_WINDOW_HEIGHT, (SDL_Color){0, 0, 0, 255});

    // Centered placeholder text
    const char *placeholder_text = "PPU not implemented - Press D for debug view";
    int text_len = strlen(placeholder_text);
    int text_width = text_len * FONT_WIDTH * FONT_SCALE;
    int text_x = (DEBUGGER_WINDOW_WIDTH - text_width) / 2;
    int text_y = (DEBUGGER_WINDOW_HEIGHT / 2) - (FONT_HEIGHT * FONT_SCALE / 2);
    draw_text(debugger_context, text_x, text_y, placeholder_text, COLOR_LABEL);
}

// Render all debugger UI
static void render(debugger_s *debugger_context) {
    // Clear screen
    SDL_SetRenderDrawColor(debugger_context->renderer, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, COLOR_BG.a);
    SDL_RenderClear(debugger_context->renderer);

    if (debugger_context->play_mode) {
        // Full-screen game mode (--play flag)
        draw_game_fullscreen(debugger_context);
    } else {
        // Debug mode - draw all panels
        draw_screen_placeholder(debugger_context);
        draw_registers(debugger_context);
        draw_instruction(debugger_context);
        draw_memory(debugger_context);
        draw_controls(debugger_context);
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
    debugger_context->mem_view_mode = MEM_VIEW_MODE_ZERO_PAGE;
    debugger_context->mem_view_addr = 0x0000;
    debugger_context->run_speed = 100;  // 100 instructions per frame when running

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

    return true;
}

void debugger_run(debugger_s *debugger_context) {
    SDL_Event event;

    while (debugger_context->running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                debugger_context->running = false;
            } else {
                handle_input(debugger_context, &event);
            }
        }

        // Execute CPU instructions
        if (!debugger_context->paused) {
            // Run multiple instructions per frame
            int count = debugger_context->run_speed > 0 ? debugger_context->run_speed : 10000;
            for (int i = 0; i < count; i++) {
                run_instruction(debugger_context->cpu);
            }
        } else if (debugger_context->step_requested) {
            // Single step
            run_instruction(debugger_context->cpu);
            debugger_context->step_requested = false;
        }

        // Render
        render(debugger_context);

        // Cap frame rate when paused to reduce CPU usage
        if (debugger_context->paused) {
            SDL_Delay(16);  // ~60 FPS
        }
    }
}

void debugger_cleanup(debugger_s *debugger_context) {
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

// Main function for standalone debugger
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <rom.nes> [options]\n", argv[0]);
        fprintf(stderr, "\nOptions:\n");
        fprintf(stderr, "  --pc <addr>   Override start PC (hex, e.g. C000)\n");
        fprintf(stderr, "  --play        Start in full-screen play mode\n");
        fprintf(stderr, "  --rom-info    Print ROM header information\n");
        return 1;
    }

    const char *rom_path = argv[1];
    int start_pc = -1;
    bool play_mode = false;
    bool show_rom_info = false;

    // Parse arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--pc") == 0 && i + 1 < argc) {
            unsigned int pc;
            if (sscanf(argv[i + 1], "%x", &pc) == 1) {
                start_pc = (int)pc;
            }
            i++;
        } else if (strcmp(argv[i], "--play") == 0) {
            play_mode = true;
        } else if (strcmp(argv[i], "--rom-info") == 0) {
            show_rom_info = true;
        }
    }

    // Load ROM
    ines_rom_t rom;
    if (!ines_load(rom_path, &rom)) {
        fprintf(stderr, "Failed to load ROM: %s\n", rom_path);
        return 1;
    }

    printf("Loaded: %s\n", rom_path);
    if (show_rom_info) {
        ines_print_info(&rom);
    }

    // Initialize bus and CPU
    bus_s bus;
    bus_init(&bus);
    bus_load_prg_rom(&bus, rom.prg_rom, rom.prg_rom_bytes);

    cpu_s cpu;
    cpu_init(&cpu, &bus);

    // Set start PC if specified
    if (start_pc >= 0) {
        cpu.PC = (word_t)start_pc;
        printf("Starting at PC=$%04X (custom)\n", cpu.PC);
    } else {
        word_t reset_vector = bus_read_word(&bus, 0xFFFC);
        cpu.PC = reset_vector;
        printf("Starting at PC=$%04X (reset vector)\n", cpu.PC);
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
