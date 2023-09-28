#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define UNIMPLEMENTED() \
    fprintf(stderr, "%s:%d: %s: Unimplemented function\n", __FILE__, __LINE__, __func__); \
    abort();
#define MEM_SIZE 1024 * 1024 * 64

Word address;
Byte address_rel;
Byte value;
//function to initialize the CPU

void register_init(CPU *cpu) {
    cpu->A = 0;
    cpu->X = 0;
    cpu->Y = 0;
    cpu->SP = 0x01FF;
    cpu->PC = read_from_addr(cpu, 0xFFFC) | (read_from_addr(cpu, 0xFFFD) << 8); 
    cpu->STATUS = 0x34;
    return;
}

void memory_init(CPU *cpu, Byte *memory) {
    cpu->memory = memory;
    return;
}

void init(CPU *cpu, Instruction *instruction_table, Byte *memory) {
    memory_init(cpu, memory);
    register_init(cpu);
    return;
}

void print_cpu_state(CPU *cpu) {
    printf("A: 0x%02X\n", cpu->A);
    printf("X: 0x%02X\n", cpu->X);
    printf("Y: 0x%02X\n", cpu->Y);
    printf("SP: 0x%04X\n", cpu->SP);
    printf("PC: 0x%04X\n", cpu->PC);
    printf("STATUS: 0x%02X\n", cpu->STATUS);
    return;
}

void reset(CPU *cpu) {
    register_init(cpu);
    return;
}

void set_flag(CPU *cpu, STATUS flag, int value) {
    if (value) {
        cpu->STATUS |= flag;
    } else {
        cpu->STATUS &= ~flag;
    }
    return;
}

Byte peek(CPU *cpu) {
    assert(cpu != NULL);
    assert(cpu->memory != NULL);
    return cpu->memory[cpu->PC];
}

Byte read(CPU *cpu) {
    assert(cpu != NULL);
    assert(cpu->memory != NULL);
    return cpu->memory[cpu->PC++];
}

Byte read_from_addr(CPU *cpu, Word address) {
    assert(cpu != NULL);
    assert(cpu->memory != NULL);
    return cpu->memory[address];
}

void write_to_addr(CPU *cpu, Word address, Byte value) {
    assert(cpu != NULL);
    assert(cpu->memory != NULL);
    cpu->memory[address] = value;
    return;
}

void push(CPU *cpu, Byte byte) {
    assert(cpu != NULL);
    assert(cpu->memory != NULL);
    write_to_addr(cpu, cpu->SP, byte);
    cpu->SP--;
    return;
}

Byte pop(CPU *cpu) {
    assert(cpu != NULL);
    assert(cpu->memory != NULL);
    Byte byte = read_from_addr(cpu, cpu->SP);
    cpu->SP++;
    return byte;
}

void init_instruction_table(Instruction *table){
    //initialize instruction table
    table[0x00] = (Instruction){
        .name = "BRK",
        .opcode = 0x00,
        .fetch = IMP,
        .execute = BRK,
        .cycles = 7,
        .length = 1
    };
    table[0x01] = (Instruction) {
        .name = "ORA",
        .opcode = 0x01,
        .fetch = IZX,
        .execute = ORA,
        .cycles = 6,
        .length = 2
    };
}


/*
    * Implied addressing mode
    * No operand needed
    * Returns 0
*/
Byte IMP (CPU *cpu) {
    assert(cpu != NULL);
    return 0;
}

/*
    * Immediate addressing mode
    * Fetches the next byte in memory and stores it in the value variable
    * Increments the program counter
*/
Byte IMM (CPU *cpu) {
    assert(cpu != NULL);
    value = read(cpu);
    return 0;
}

/*
    * Zero page addressing mode
    * Fetches the next byte in memory and stores it in the zero'th page of memory
    * Increments the program counter
*/

Byte ZP0 (CPU *cpu) {
    assert(cpu != NULL);
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
    assert(cpu != NULL);
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
    assert(cpu != NULL);
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
    assert(cpu != NULL);
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
    assert(cpu != NULL);
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
    assert(cpu != NULL);
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
    assert(cpu != NULL);
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
    assert(cpu != NULL);
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
    assert(cpu != NULL);
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
    assert(cpu != NULL);
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

/*
 * Branch on Result Plus
 * If the negative flag is clear then add the relative displacement to the program counter to cause a branch to a new location.
*/
Byte BPL(CPU *cpu) {
    assert(cpu != NULL);
    if (!(cpu->STATUS & N)) {
        cpu->PC = address_rel;
    }
    return 0;
}

/*
 * Clear Carry Flag
 * Clears the carry flag
*/
Byte CLC(CPU *cpu) {
    assert(cpu != NULL);
    set_flag(cpu, C, 0);
    return 0;
}