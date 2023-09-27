//implementations
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEM_SIZE 1024 * 1024 * 64

//function definitions

//function to initialize the CPU

void init(CPU *cpu) {
    //initialize registers
    cpu->A = 0;
    cpu->X = 0;
    cpu->Y = 0;
    cpu->SP = 0xFD;
    cpu->PC = 0;
    cpu->STATUS = 0;
    //initialize memory
    cpu->memory = (unsigned char*)malloc(MEM_SIZE);
    memset(cpu->memory, 0, MEM_SIZE);
    return;
}

//debug function to print CPU state
void print_cpu_state(CPU *cpu) {
    printf("A: %02X\n", cpu->A);
    printf("X: %02X\n", cpu->X);
    printf("Y: %02X\n", cpu->Y);
    printf("SP: %02X\n", cpu->SP);
    printf("PC: %04X\n", cpu->PC);
    printf("STATUS: %02X\n", cpu->STATUS);
    return;
}