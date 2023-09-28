#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNIMPLEMENTED() \
    fprintf(stderr, "%s:%d: %s: Unimplemented function\n", __FILE__, __LINE__, __func__); \
    abort();
#define MEM_SIZE 1024 * 1024 * 64
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

void memory_init(CPU *cpu) {
    cpu->memory = malloc(MEM_SIZE);
    return;
}

void memory_free(CPU *cpu) {
    free(cpu->memory);
    return;
}

void init(CPU *cpu) {
    memory_init(cpu);
    register_init(cpu);
    return;
}

//debug function to print CPU state
void print_cpu_state(CPU *cpu) {
    printf("A: 0x%02X\n", cpu->A);
    printf("X: 0x%02X\n", cpu->X);
    printf("Y: 0x%02X\n", cpu->Y);
    printf("SP: 0x%04X\n", cpu->SP);
    printf("PC: 0x%04X\n", cpu->PC);
    printf("STATUS: 0x%02X\n", cpu->STATUS);
    return;
}

//function to reset the CPU
void reset(CPU *cpu) {
    memory_free(cpu);
    memory_init(cpu);   
    register_init(cpu);
    return;
}

//function to set a flag in the status register
void setFlag(CPU *cpu, STATUS flag, int value) {
    if (value) {
        cpu->STATUS |= flag;
    } else {
        cpu->STATUS &= ~flag;
    }
    return;
}

//function to peek at the current byte in memory
Byte peek(CPU *cpu) {
    return cpu->memory[cpu->PC];
}

//function to read the current byte in memory
Byte read(CPU *cpu) {
    return cpu->memory[cpu->PC++];
}

//function to read a byte from a specific address in memory
Byte read_from_addr(CPU *cpu, Word address) {
    return cpu->memory[address];
}