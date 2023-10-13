#include <stdio.h>
#include "munit/munit.h"
#include "../src/cpu.h"

// 64 kb memory
#define MEM_SIZE 1024 * 64

#define TEST_LIST \
    X(ORA, IMM) \
    X(ORA, ZP0) \
    X(ORA, ZPX) \


#define X(instruction) void Test_##instruction(CPU *cpu);
LIST_OF_INSTRUCTIONS
#undef X

#define X(instruction, mode) void Test_##instruction##_##mode(CPU *cpu);
TEST_LIST
#undef X

int main(void)
{
    CPU cpu;
    Byte memory[MEM_SIZE] = {0};
    init(&cpu, memory);
    reset(&cpu);
    Test_ORA(&cpu);
    printf("All tests passed\n");
}
void Test_ORA(CPU *cpu)
{
    Test_ORA_IMM(cpu);
    Test_ORA_ZP0(cpu);
    Test_ORA_ZPX(cpu);
}

void Test_ORA_IMM(CPU *cpu)
{
    Word old_pc = cpu->PC = 0x4000;
    cpu->memory[cpu->PC] = 0x09;
    cpu->memory[cpu->PC + 1] = 0x80;
    Instruction ins = cpu->table[cpu->memory[cpu->PC]];
    munit_assert_string_equal(ins.name, "ORA");
    munit_assert_int(ins.opcode, ==, 0x09);
    munit_assert_int(ins.length, ==, 2);
    munit_assert_int(ins.cycles, ==, 2);
    // test negative case
    cpu->A = 0;
    cpu->memory[cpu->PC + 1] = 0x80;
    clock(cpu);
    munit_assert_int(cpu->A, ==, 0x80);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, N);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    // test zero case
    cpu->PC = old_pc;
    cpu->A = 0;
    cpu->memory[cpu->PC + 1] = 0x00;
    clock(cpu);
    munit_assert_int(cpu->A, ==, 0);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, Z);
    // check positive case
    cpu->PC = old_pc;
    cpu->A = 0x7F;
    cpu->memory[cpu->PC + 1] = 0x01;
    clock(cpu);
    munit_assert_int(cpu->A, ==, 0x7F);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    // check flags
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
}

void Test_ORA_ZP0(CPU *cpu)
{
    Word old_pc = cpu->PC = 0x4000;
    cpu->memory[cpu->PC] = INSTRUCTION_ORA_ZP0;
    Instruction ins = cpu->table[cpu->memory[cpu->PC]];
    munit_assert_string_equal(ins.name, "ORA");
    munit_assert_int(ins.opcode, ==, INSTRUCTION_ORA_ZP0);
    munit_assert_int(ins.length, ==, 2);
    munit_assert_int(ins.cycles, ==, 3);
    // test negative case
    // put 0x80 in memory location 0x0040
    cpu->A = 0;
    cpu->memory[cpu->PC + 1] = 0x40;
    cpu->memory[0x0040] = 0x80;
    clock(cpu);
    munit_assert_int(cpu->A, ==, 0x80);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, N);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    // test zero case
    cpu->PC = old_pc;
    cpu->A = 0;
    cpu->memory[cpu->PC + 1] = 0x40;
    cpu->memory[0x0040] = 0x00;
    clock(cpu);
    munit_assert_int(cpu->A, ==, 0);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, Z);
    // check positive case
    cpu->PC = old_pc;
    cpu->A = 0x7F;
    cpu->memory[cpu->PC + 1] = 0x40;
    cpu->memory[0x0040] = 0x01;
    clock(cpu);
    munit_assert_int(cpu->A, ==, 0x7F);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
}

void Test_ORA_ZPX(CPU *cpu)
{
    Word old_pc = cpu->PC = 0x4000;
    cpu->memory[cpu->PC] = INSTRUCTION_ORA_ZPX;
    Instruction ins = cpu->table[cpu->memory[cpu->PC]];
    munit_assert_string_equal(ins.name, "ORA");
    munit_assert_int(ins.opcode, ==, INSTRUCTION_ORA_ZPX);
    munit_assert_int(ins.length, ==, 2);
    munit_assert_int(ins.cycles, ==, 4);
}
