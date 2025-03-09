#include <stdio.h>
#include <stdlib.h>
#include "cpu.hpp"

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
    printf("%s", "LOLOLOL");
    //CPU *cpu = get_cpu_instance();
    //Byte memory[MEM_SIZE] = {0};
    //cpu_init(cpu, memory);
    return 0;
}
