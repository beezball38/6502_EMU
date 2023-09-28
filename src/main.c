//scaffold
// Created by: Christopher Beser

#include <stdio.h>
#include "cpu.h"
#include "instruction.h"



int main() {
    init_instruction_table();
    CPU cpu;
    init(&cpu);
    print_cpu_state(&cpu);
    return 0;
}