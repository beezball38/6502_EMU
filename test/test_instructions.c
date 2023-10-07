#include "munit/munit.h"
#include "../src/cpu.h"

//64 kb memory
#define MEM_SIZE 1024 * 64
void test_BRK(CPU *cpu, Instruction ins);

int main(void)
{
    CPU cpu;
    Byte memory[MEM_SIZE] = {0};
    init(&cpu, memory);
    reset(&cpu);
    Instruction table[0x100] = {0};
    init_instruction_table(table);
    test_BRK(&cpu, table[0]);
}

void test_BRK(CPU *cpu, Instruction ins) {
    munit_assert_string_equal(ins.name, "BRK");
    munit_assert_int(ins.opcode, ==, 0x00);
    munit_assert_int(ins.length, ==, 1);
    munit_assert_int(ins.cycles, ==, 7);
    //fetch should do a no op and PC should not increment
    Byte old_pc = cpu->PC;
    ins.fetch(cpu);
