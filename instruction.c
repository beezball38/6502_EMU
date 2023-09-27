//addressing mode implementations
#include "instruction.h"
#include "cpu.h"
#include <stdio.h>

//macro for unimplemented function with file name, line number and function name to stderr
#define UNIMPLEMENTED() \
    fprintf(stderr, "%s:%d: %s: Unimplemented function\n", __FILE__, __LINE__, __func__); \
    abort();


//function to implement the implied addressing mode

Byte IMP (CPU *cpu, Instruction *instruction) {
    UNIMPLEMENTED();
    return 0;
}

Byte IMM (CPU *cpu, Instruction *instruction) {
    UNIMPLEMENTED();
    return 0;
}

Byte ZP0 (CPU *cpu, Instruction *instruction) {
    UNIMPLEMENTED();
    return 0;
}

Byte ZPX (CPU *cpu, Instruction *instruction) {
    UNIMPLEMENTED();
    return 0;
}

Byte ZPY (CPU *cpu, Instruction *instruction) {
    UNIMPLEMENTED();
    return 0;
}

Byte REL (CPU *cpu, Instruction *instruction) {
    UNIMPLEMENTED();
    return 0;
}

Byte ABS (CPU *cpu, Instruction *instruction) {
    UNIMPLEMENTED();
    return 0;
}

Byte ABX (CPU *cpu, Instruction *instruction) {
    UNIMPLEMENTED();
    return 0;
}

Byte ABY (CPU *cpu, Instruction *instruction) {
    UNIMPLEMENTED();
    return 0;
}

Byte IND (CPU *cpu, Instruction *instruction) {
    UNIMPLEMENTED();
    return 0;
}

Byte IZX (CPU *cpu, Instruction *instruction) {
    UNIMPLEMENTED();
    return 0;
}

Byte IZY (CPU *cpu, Instruction *instruction) {
    UNIMPLEMENTED();
    return 0;
}