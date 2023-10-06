//scaffold
// Created by: Christopher Beser

#include <stdio.h>
#include "cpu.h"

//64 kb macro
#define MEM_SIZE 1024 * 64

CPU* get_cpu_instance(void);
CPU* get_cpu_instance(void) {
    static CPU cpu;
    return &cpu;
}

int main(void) {
    CPU* cpu = get_cpu_instance();
    Instruction table[0x100] = {0};
    init_instruction_table(table);
    for(int i = 0; i < 256; i++) {
        print_instruction(i, table);
    }
    Byte memory[MEM_SIZE] = {0};
    init(cpu, memory);
    return 0;
}
