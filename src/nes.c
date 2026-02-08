#include "nes.h"
#include "gamecart.h"
#include <assert.h>

static nes_console_s s_nes;

nes_console_s* nes_get_instance(void)
{
    return &s_nes;
}

void nes_init(nes_console_s *nes)
{
    assert(nes != NULL);

    cpu_s *cpu = cpu_get_instance();
    ppu_s *ppu = ppu_get_instance();
    bus_s *bus = bus_get_instance();

    bus_init(bus);
    cpu_init(cpu);
    ppu_init(ppu);

    bus->ppu = ppu;
    cpu->bus = bus;

    nes->cpu = cpu;
    nes->ppu = ppu;
    nes->bus = bus;
}

void nes_attach_cart(nes_console_s *nes, gamecart_s *cart)
{
    assert(nes != NULL);
    bus_attach_cart(nes->bus, cart);
}

int nes_step(nes_console_s *nes)
{
    assert(nes != NULL);

    cpu_s *cpu = nes->cpu;
    ppu_s *ppu = nes->ppu;
    int result = STEP_RESULT_OK;

    size_t cycles_before = cpu->cycles;
    run_instruction(cpu);
    size_t cpu_cycles = cpu->cycles - cycles_before;
    size_t ppu_cycles = cpu_cycles * 3;

    for (size_t i = 0; i < ppu_cycles; i++) {
        ppu_tick(ppu);
    }

    if (ppu->nmi_pending) {
        ppu->nmi_pending = false;
        nmi(cpu);
        result |= STEP_RESULT_NMI_FIRED;
    }

    if (ppu_frame_complete(ppu)) {
        result |= STEP_RESULT_FRAME_COMPLETE;
    }

    return result;
}
