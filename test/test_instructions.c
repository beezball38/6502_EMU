#include "munit/munit.h"
#include "../src/cpu.h"

//64 kb memory
#define MEM_SIZE 1024 * 64
void test_BRK_success(CPU *cpu, Instruction ins);

int main(void)
{
    CPU cpu;
    Byte memory[MEM_SIZE] = {0};
    init(&cpu, memory);
    reset(&cpu);
    Instruction table[0x100] = {0};
    init_instruction_table(table);
    test_BRK_success(&cpu, table[0]);
}

void test_BRK_success(CPU *cpu, Instruction ins) {
    //set up the interupt vector
    write_to_addr(cpu, 0xFFFE, 0x00);
    write_to_addr(cpu, 0xFFFF, 0x80);
    munit_assert_string_equal(ins.name, "BRK");
    munit_assert_int(ins.opcode, ==, 0x00);
    munit_assert_int(ins.length, ==, 1);
    munit_assert_int(ins.cycles, ==, 7);
    //fetch should do a no op and PC should not increment
    Word old_pc = cpu->PC = 0x4000;
    ins.fetch(cpu);
    munit_assert_int(cpu->PC, ==, old_pc);
    ins.execute(cpu);
    munit_assert_true(cpu->STATUS & I);
    //assert status was pushed to stack
    munit_assert_int(read_from_addr(cpu, 0x01FF), ==, cpu->STATUS);
    //assert PC is set to the interupt vector
    munit_assert_int(cpu->PC, ==, 0x8000);

}
