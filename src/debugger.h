// SDL2 Visual Debugger for NES emulator
// Displays CPU state, memory, and current instruction
// Supports pause/resume and single-step execution
#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <SDL.h>
#include <stdbool.h>
#include "cpu.h"
#include "bus.h"

// Window dimensions (16:9 aspect ratio)
#define DEBUGGER_WINDOW_WIDTH  1280
#define DEBUGGER_WINDOW_HEIGHT 720

// Font dimensions (8x8 bitmap font)
#define FONT_WIDTH  8
#define FONT_HEIGHT 8
#define FONT_SCALE  2

// Memory view modes
typedef enum {
    MEM_VIEW_MODE_ZERO_PAGE,
    MEM_VIEW_MODE_STACK,
    MEM_VIEW_MODE_CUSTOM
} mem_view_mode_e;

// Debugger state
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *font_texture;   // Bitmap font texture

    cpu_s *cpu;
    bus_s *bus;

    bool running;           // Main loop running
    bool paused;            // Execution paused
    bool step_requested;    // Step one instruction
    bool quit_requested;    // First ESC/Q pressed, waiting for confirmation
    bool play_mode;         // Full-screen game mode (no debug panels)

    mem_view_mode_e mem_view_mode;
    word_t mem_view_addr;   // Memory view start address

    int run_speed;          // Instructions per frame when running (0 = unlimited)
} debugger_s;

// Initialize the debugger (creates SDL window, loads font)
// Returns true on success, false on failure
bool debugger_init(debugger_s *dbg, cpu_s *cpu, bus_s *bus);

// Run the debugger main loop
// Returns when user quits
void debugger_run(debugger_s *dbg);

// Clean up resources
void debugger_cleanup(debugger_s *dbg);

#endif
