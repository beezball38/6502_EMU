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
    Byte (*execute)(CPU *cpu, struct Instruction *instruction);
    Byte (*fetch)(CPU *cpu, struct Instruction *instruction);
} Instruction;

//prototypes

//addressing modes (fetch)
Byte IMP(CPU *cpu, Instruction *instruction);
Byte IMM(CPU *cpu, Instruction *instruction);
Byte ZP0(CPU *cpu, Instruction *instruction);
Byte ZPX(CPU *cpu, Instruction *instruction);
Byte ZPY(CPU *cpu, Instruction *instruction);
Byte REL(CPU *cpu, Instruction *instruction);
Byte ABS(CPU *cpu, Instruction *instruction);
Byte ABX(CPU *cpu, Instruction *instruction);
Byte ABY(CPU *cpu, Instruction *instruction);
Byte IND(CPU *cpu, Instruction *instruction);
Byte IZX(CPU *cpu, Instruction *instruction);
Byte IZY(CPU *cpu, Instruction *instruction);

#endif
