#include "munit/munit.h"
#include "../src/cpu.h"
int main(void)
{
    CPU cpu;
    (void)cpu;
    Instruction table[0x100] = {0};
    init_instruction_table(table);
    for(int i = 0; i < 0x10; i++) {
        print_instruction(i, table);
    }
}
