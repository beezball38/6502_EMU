#ifndef NES_H
#define NES_H

#include "cpu.h"
#include "ppu.h"
#include "bus.h"

typedef struct gamecart_s gamecart_s;

typedef enum {
    STEP_RESULT_OK             = 0,
    STEP_RESULT_FRAME_COMPLETE = (1 << 0),
    STEP_RESULT_NMI_FIRED      = (1 << 1),
    STEP_RESULT_ILLEGAL_OPCODE = (1 << 2),
} step_result_e;

typedef struct {
    cpu_s *cpu;
    ppu_s *ppu;
    bus_s *bus;
} nes_console_s;

nes_console_s* nes_get_instance(void);
void nes_init(nes_console_s *nes);
void nes_attach_cart(nes_console_s *nes, gamecart_s *cart);
int nes_step(nes_console_s *nes);

#endif
