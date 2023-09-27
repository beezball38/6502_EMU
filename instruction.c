//addressing mode implementations
#include "instruction.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>

Word address;
Byte address_rel;
Byte value;
//macro for unimplemented function with file name, line number and function name to stderr
#define UNIMPLEMENTED() \
    fprintf(stderr, "%s:%d: %s: Unimplemented function\n", __FILE__, __LINE__, __func__); \
    abort();


//function to implement the implied addressing mode

Byte IMP (CPU *cpu, Instruction *instruction) {
    return 0;
}

Byte IMM (CPU *cpu, Instruction *instruction) {
    //immediate mode addressing
    //the operand is the next byte after the opcode
    //the program counter is incremented by 1

    //set the operand to the next byte after the opcode
    value = cpu->memory[cpu->PC + 1];
    return 0;
}

Byte ZP0 (CPU *cpu, Instruction *instruction) {
    //zero page addressing
    //the operand is the next byte after the opcode
    //the program counter is incremented by 1

    //set the operand to the next byte after the opcode 
    address = cpu->memory[cpu->PC + 1];
    //set as zero page address
    address &= 0x00FF;
    //set value to the value at the address
    value = cpu->memory[address];
    
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