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
    Word old_pc = cpu->PC = 0x4000;
    //assert stack pointer is at 0xFF
    munit_assert_int(cpu->SP, ==, 0xFF);
    ins.fetch(cpu);
    munit_assert_int(cpu->PC, ==, old_pc);
    ins.execute(cpu);
    //assert stack pointer is at 0xFC
    munit_assert_int(cpu->SP, ==, 0xFC);
    munit_assert_true(cpu->STATUS & I);
    munit_assert_int(cpu->PC, ==, 0x8000);
}
