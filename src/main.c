//scaffold
// Created by: Christopher Beser

#include <stdio.h>
#include "cpu.h"

//64 kb macro
#define MEM_SIZE 1024 * 64

int main() {
    //instructon table on the stack
    Instruction instruction_table[256] = {0};
    init_instruction_table(instruction_table);
    CPU cpu;
    Byte memory[MEM_SIZE] = {0};
    init(&cpu, instruction_table, memory);
    return 0;
}