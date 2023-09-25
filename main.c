//scaffold
// Created by: Christopher Beser

#include <stdio.h>
#include "cpu.h"
#include "instruction.h"

int main() {
    CPU cpu;
    Instruction instruction;
    init(&cpu);
    print_cpu_state(&cpu);
    return 0;
}