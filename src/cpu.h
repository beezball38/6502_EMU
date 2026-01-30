// CPU struct and interface for 6502 processor
#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "cpu_defs.h"

// =============================================================================
// Instruction descriptor
// =============================================================================

typedef struct
{
    const char *name;
    byte_t opcode;
    byte_t cycles;
    byte_t length;
    instruction_func_t data_fetch;
    instruction_func_t execute;
} cpu_instruction_s;

// =============================================================================
// CPU state struct
// =============================================================================

struct c6502
{
    byte_t A;
    byte_t X;
    byte_t Y;
    byte_t SP;
    word_t PC;
    byte_t STATUS;

    size_t cycles;
    byte_t current_opcode;
    bool instruction_pending;
    bool pc_changed;
    bool does_need_additional_cycle;
    byte_t *memory;
    cpu_instruction_s table[256];
};

// =============================================================================
// CPU interface
// =============================================================================

void init_instruction_table(cpu_s *cpu);
void cpu_init(cpu_s *cpu, byte_t *memory);

cpu_instruction_s* get_instruction(cpu_s *cpu, byte_t opcode);
cpu_instruction_s* get_current_instruction(cpu_s *cpu);
char *addr_mode_to_string(cpu_addr_mode_e mode);

bool get_flag(cpu_s *cpu, cpu_status_flag_e flag);
void set_flag(cpu_s *cpu, cpu_status_flag_e flag, bool value);

byte_t peek(cpu_s *cpu);
byte_t read_from_addr(cpu_s *cpu, word_t address);
void write_to_addr(cpu_s *cpu, word_t address, byte_t value);

void push_byte(cpu_s *cpu, byte_t value);
byte_t pop_byte(cpu_s *cpu);

void adjust_pc(cpu_s *cpu, byte_t instruction_length);
void clock(cpu_s *cpu);

void irq(cpu_s *cpu);
void nmi(cpu_s *cpu);
void reset(cpu_s *cpu);

#endif
