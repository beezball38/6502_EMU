#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"

// 64 kb macro
#define MEM_SIZE 1024 * 64

CPU *get_cpu_instance(void);
CPU *get_cpu_instance(void)
{
    static CPU cpu;
    return &cpu;
}

int main(void)
{
    CPU *cpu = get_cpu_instance();
    init_instruction_table(cpu);
    Byte memory[MEM_SIZE] = {0};
    init(cpu, memory);
    return 0;
}
