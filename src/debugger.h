#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <SDL.h>
#include <stdbool.h>
#include "cpu.h"
#include "bus.h"

#define DEBUGGER_WINDOW_WIDTH  1280
#define DEBUGGER_WINDOW_HEIGHT 720

#define FONT_WIDTH  8
#define FONT_HEIGHT 8
#define FONT_SCALE  2

typedef enum {
    MEM_VIEW_MODE_ZERO_PAGE,
    MEM_VIEW_MODE_STACK,
    MEM_VIEW_MODE_CUSTOM
} mem_view_mode_e;

typedef enum {
    DEBUG_VIEW_CPU,
    DEBUG_VIEW_PPU
} debug_view_mode_e;

typedef struct {
    word_t PC;
    byte_t SP;
    byte_t STATUS;
    byte_t A;
    byte_t X;
    byte_t Y;
} cpu_init_state_s;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *font_texture;
    SDL_Texture *screen_texture;
    SDL_Texture *pattern_texture;

    cpu_s *cpu;
    bus_s *bus;

    bool running;
    bool paused;
    bool step_requested;
    bool quit_requested;
    bool play_mode;
    bool illegal_opcode;

    cpu_init_state_s init_state;

    mem_view_mode_e mem_view_mode;
    word_t mem_view_addr;

    debug_view_mode_e debug_view_mode;
    int ppu_palette_select;
    int oam_scroll_offset;

    int run_speed;
} debugger_s;

bool debugger_init(debugger_s *dbg, cpu_s *cpu, bus_s *bus);
void debugger_run(debugger_s *dbg);
void debugger_cleanup(debugger_s *dbg);

#endif
