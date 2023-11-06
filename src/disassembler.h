#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H
#include "cpu.h"
typedef struct Disassembler
{
    Instruction* table;
    // disassemble the machine code into assembly code
    void (*disassemble)(struct Disassembler *self, byte_t *memory, word_t pc);
    // print the assembly code
    void (*print)(struct Disassembler *self);
    // the assembly code
    char *assembly;
} Disassembler;

// initialize the disassembler
void init_disassembler(Disassembler *disassembler, Instruction *table);
#endif