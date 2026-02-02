// CPU struct and interface for NES (6502-based) processor
#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "cpu_defs.h"

typedef struct bus bus_s;

typedef struct
{
    const char *name;
    byte_t opcode;
    byte_t cycles;
    byte_t length;
    instruction_func_t data_fetch;
    instruction_func_t execute;
} cpu_instruction_s;

struct cpu_s
{
    byte_t A;
    byte_t X;
    byte_t Y;
    byte_t SP;
    word_t PC;
    byte_t STATUS;

    size_t cycles;              // Total elapsed cycles (counts up from 0)
    byte_t current_opcode;
    bool instruction_pending;
    bool pc_changed;
    bus_s *bus;
    cpu_instruction_s table[256];
};

void irq(cpu_s *cpu);
void nmi(cpu_s *cpu);
void reset(cpu_s *cpu);
void adjust_pc(cpu_s *cpu, byte_t instruction_length);

bool get_flag(cpu_s *cpu, cpu_status_flag_e flag);
void set_flag(cpu_s *cpu, cpu_status_flag_e flag, bool value);

void push_byte_to_stack(cpu_s *cpu, byte_t value);
byte_t pop_byte(cpu_s *cpu);

void cpu_init(cpu_s *cpu, bus_s *bus);

cpu_instruction_s *get_instruction(cpu_s *cpu, byte_t opcode);
cpu_instruction_s *get_current_instruction(cpu_s *cpu);

byte_t read_from_addr(cpu_s *cpu, word_t address);
void write_to_addr(cpu_s *cpu, word_t address, byte_t value);

// Execute a single instruction (fetch, decode, execute, update PC)
void run_instruction(cpu_s *cpu);

// Check if an opcode is illegal/unimplemented
bool is_illegal_opcode(cpu_s *cpu, byte_t opcode);
#endif
