//scaffold
// Created by: Christopher Beser

#include <stdio.h>
#include "cpu.h"

//64 kb macro
#define MEM_SIZE 1024 * 64
CPU* get_cpu_instance() {
    static CPU cpu;
    return &cpu;
}

int main() {
    CPU* cpu = get_cpu_instance();
    init_instruction_table();
    Byte opcode = 0x01;
    print_instruction(opcode);
    Byte memory[MEM_SIZE] = {0};
    init(cpu, memory);
    return 0;
}