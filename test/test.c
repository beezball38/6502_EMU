#include "munit/munit.h"
#include "../src/cpu.h"

extern Instruction table[256];
int main(void)
{
    CPU cpu;
    (void)cpu;
    init_instruction_table();
    for(int i = 0; i < 0x10; i++) {
        Instruction ins = table[i];
        print_instruction(ins.opcode);
    }
}
