//addressing mode implementations
#include "instruction.h"
#include "cpu.h"
#include <stdio.h>

//function to implement the implied addressing mode

Byte IMP (CPU *cpu, Instruction *instruction) {
    printf("IMP\n");
    return 0;
}

Byte IMM (CPU *cpu, Instruction *instruction) {
    printf("IMM\n");
    return 0;
}

Byte ZP0 (CPU *cpu, Instruction *instruction) {
    printf("ZP0\n");
    return 0;
}

Byte ZPX (CPU *cpu, Instruction *instruction) {
    printf("ZPX\n");
    return 0;
}

Byte ZPY (CPU *cpu, Instruction *instruction) {
    printf("ZPY\n");
    return 0;
}

Byte REL (CPU *cpu, Instruction *instruction) {
    printf("REL\n");
    return 0;
}

Byte ABS (CPU *cpu, Instruction *instruction) {
    printf("ABS\n");
    return 0;
}

Byte ABX (CPU *cpu, Instruction *instruction) {
    printf("ABX\n");
    return 0;
}

Byte ABY (CPU *cpu, Instruction *instruction) {
    printf("ABY\n");
    return 0;
}

Byte IND (CPU *cpu, Instruction *instruction) {
    printf("IND\n");
    return 0;
}

Byte IZX (CPU *cpu, Instruction *instruction) {
    printf("IZX\n");
    return 0;
}

Byte IZY (CPU *cpu, Instruction *instruction) {
    printf("IZY\n");
    return 0;
}