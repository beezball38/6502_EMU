//addressing mode implementations
#include "instruction.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>

Word address;
Byte address_rel;
Byte value;
//initialize instruction table with 256 elements
Instruction* instruction_table[256] = {0};
//macro for unimplemented function with file name, line number and function name to stderr
#define UNIMPLEMENTED() \
    fprintf(stderr, "%s:%d: %s: Unimplemented function\n", __FILE__, __LINE__, __func__); \
    abort();

void init_instruction_table(){
    instruction_table[0x00] = malloc (sizeof(Instruction));
    instruction_table[0x00]->name = "BRK";
    instruction_table[0x00]->fetch = IMP;
    instruction_table[0x00]->execute = BRK;
    instruction_table[0x00]->cycles = 7;
    instruction_table[0x00]->length = 1;

    instruction_table[0x01] = malloc (sizeof(Instruction));
    instruction_table[0x01]->name = "ORA";
    instruction_table[0x01]->fetch = IZX;
    instruction_table[0x01]->execute = ORA;
    instruction_table[0x01]->cycles = 6;
    instruction_table[0x01]->length = 2;
}

/*
    * Implied addressing mode
    * No operand needed
    * Returns 0
*/
Byte IMP (CPU *cpu) {
    (void)cpu;
    return 0;
}

/*
    * Immediate addressing mode
    * Fetches the next byte in memory and stores it in the value variable
    * Increments the program counter
*/
Byte IMM (CPU *cpu) {
    value = read(cpu);
    return 0;
}

/*
    * Zero page addressing mode
    * Fetches the next byte in memory and stores it in the zero'th page of memory
    * Increments the program counter
*/

Byte ZP0 (CPU *cpu) {
    address = read(cpu);
    address &= 0x00FF;
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Zero page X addressing mode
    * Fetches the next byte in memory and stores it in the zero'th page of memory
    * Increments the program counter
    * Adds the X register to the address
    * Stores the result in the address variable
    * Fetches the value at the address and stores it in the value variable
*/
Byte ZPX (CPU *cpu) {
    address = read(cpu);
    address &= 0x00FF;
    address += cpu->X;
    address &= 0x00FF;
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Zero page Y addressing mode
    * Fetches the next byte in memory and stores it in the zero'th page of memory
    * Increments the program counter
    * Adds the Y register to the address
    * Stores the result in the address variable
    * Fetches the value at the address and stores it in the value variable
*/
Byte ZPY (CPU *cpu) {
    address = read(cpu);
    address &= 0x00FF;
    address += cpu->Y;
    address &= 0x00FF;
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Relative addressing mode
    * Fetches the next byte in memory and stores it in the address_rel variable
    * Increments the program counter
*/
Byte REL (CPU *cpu) {
    address_rel = read(cpu);
    return 0;
}

/*
    * Absolute addressing mode
    * Fetches the next two bytes in memory and stores it in the address variable
    * Increments the program counter
    * Fetches the value at the address and stores it in the value variable
*/
Byte ABS (CPU *cpu) {
    address = read(cpu);
    address |= (read(cpu) << 8);
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Absolute X addressing mode
    * Fetches the next two bytes in memory and stores it in the address variable
    * Increments the program counter
    * Adds the X register to the address
    * Stores the result in the address variable
    * Fetches the value at the address and stores it in the value variable
*/

Byte ABX (CPU *cpu) {
    address = read(cpu);
    address |= (read(cpu) << 8);
    address += cpu->X;
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Absolute Y addressing mode
    * Fetches the next two bytes in memory and stores it in the address variable
    * Increments the program counter
    * Adds the Y register to the address
    * Stores the result in the address variable
    * Fetches the value at the address and stores it in the value variable
*/

Byte ABY (CPU *cpu) {
    address = read(cpu);
    address |= (read(cpu) << 8);
    address += cpu->Y;
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Indirect addressing mode
    * Fetches the next two bytes in memory and stores it in the address variable
    * Increments the program counter
    * Fetches the value at the address and stores it in the value variable
    * Fetches the next two bytes in memory and stores it in the address variable
    * Increments the program counter
    * Fetches the value at the address and stores it in the value variable
*/

Byte IND (CPU *cpu) {
    Word ptr = read(cpu);
    ptr |= (read(cpu) << 8);
    //simulate page boundary hardware bug
    if ((ptr & 0x00FF) == 0x00FF) {
        address = (read_from_addr(cpu, ptr & 0xFF00) << 8) | read_from_addr(cpu, ptr);
    } else {
        address = (read_from_addr(cpu, ptr + 1) << 8) | read_from_addr(cpu, ptr);
    }
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Indirect X addressing mode
    * Fetches the next byte in memory and stores it in the address variable
    * Increments the program counter
    * Adds the X register to the address
    * Stores the result in the address variable
    * Fetches the value at the address and stores it in the value variable
*/

Byte IZX (CPU *cpu) {
    Word ptr = read(cpu);
    ptr &= 0x00FF;
    address = (read_from_addr(cpu, ptr + 1) << 8) | read_from_addr(cpu, ptr);
    address += cpu->X;
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Indirect Y addressing mode
    * Fetches the next byte in memory and stores it in the address variable
    * Increments the program counter
    * Fetches the value at the address and stores it in the value variable
    * Adds the Y register to the address
    * Stores the result in the address variable
*/
//todo not so sure about this one
Byte IZY (CPU *cpu) {
    Word ptr = read(cpu);
    ptr &= 0x00FF;
    address = (read_from_addr(cpu, ptr + 1) << 8) | read_from_addr(cpu, ptr);
    address += cpu->Y;
    value = read_from_addr(cpu, address);
    return 0;
}

//instruction implementations

Byte BRK(CPU *cpu) {
    (void)cpu;
    return 0;
}

Byte ORA(CPU *cpu) {
    cpu->A |= value;
    set_flag(cpu, Z, cpu->A == 0x00);
    set_flag(cpu, N, cpu->A & 0x80);
    return 0;
}

Byte ASL(CPU *cpu) {
    set_flag(cpu, C, value & 0x80);
    value <<= 1;
    set_flag(cpu, Z, value == 0x00);
    set_flag(cpu, N, value & 0x80);
    return 0;
}
/*
    Push Processor Status on Stack
    Pushes the status register onto the stack
    Decrements the stack pointer?
*/
Byte PHP(CPU *cpu) {
    push(cpu, cpu->STATUS);
    return 0;
}