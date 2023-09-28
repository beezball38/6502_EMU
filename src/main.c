//scaffold
// Created by: Christopher Beser

#include <stdio.h>
#include "cpu.h"
#include "instruction.h"



int main() {
    //instructon table on the stack
    Instruction instruction_table[256] = {0};
    init_instruction_table(instruction_table);
    CPU cpu;
    init(&cpu, instruction_table);
    return 0;
}