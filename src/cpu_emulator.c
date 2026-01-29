#include <stdio.h>
#include "cpu.h"
// 64 kb macro
#define MEM_SIZE 1024 * 64

cpu_s *get_cpu_instance(void);
cpu_s *get_cpu_instance(void)
{
    static cpu_s cpu;
    return &cpu;
}

int main(void)
{
    cpu_s *cpu = get_cpu_instance();
    byte_t memory[MEM_SIZE] = {0};
    cpu_init(cpu, memory);
    return 0;
}
