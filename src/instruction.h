//instruction struct for 6502 processor
#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdint.h>
#include "cpu.h"

typedef uint8_t Byte;
typedef uint16_t Word;

/*
    * Instruction struct
    * name: name of instruction
    * opcode: opcode of instruction
    * length: length of instruction in bytes
    * cycles: number of cycles instruction takes
    * execute: function pointer to execute instruction
    * fetch: function pointer to function to fetch operand
*/
typedef struct Instruction {
    char *name;
    Byte opcode;
    Byte length;
    Byte cycles;
    Byte (*fetch)(CPU *cpu);
    Byte (*execute)(CPU *cpu);
} Instruction;

void init_instruction_table();
//prototypes

//addressing modes (fetch)
Byte IMP(CPU *cpu);
Byte IMM(CPU *cpu);
Byte ZP0(CPU *cpu);
Byte ZPX(CPU *cpu);
Byte ZPY(CPU *cpu);
Byte REL(CPU *cpu);
Byte ABS(CPU *cpu);
Byte ABX(CPU *cpu);
Byte ABY(CPU *cpu);
Byte IND(CPU *cpu);
Byte IZX(CPU *cpu);
Byte IZY(CPU *cpu);

//instructions (execute)
Byte BRK(CPU *cpu);
Byte ORA(CPU *cpu);
#endif
