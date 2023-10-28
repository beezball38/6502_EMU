#include <stdio.h>
#include "munit/munit.h"
#include "../src/cpu.h"

// 64 kb memory
#define MEM_SIZE 1024 * 64

#define TEST_LIST \
    X(ORA, IMM) \
    X(ORA, ZP0) \
    X(ORA, ZPX) \
    X(ORA, ABS) \
    X(ORA, ABX) \
    X(ORA, ABY) \
    X(ORA, IZX) \
    X(ORA, IZY)

#define X(instruction) void Test_##instruction(CPU *cpu, Byte *memory);
LIST_OF_INSTRUCTIONS
#undef X

#define X(instruction, mode) void Test_##instruction##_##mode(CPU *cpu);
TEST_LIST
#undef X

void run(CPU *cpu, int cycles)
{
    for (int i = 0; i < cycles; i++)
    {
        clock(cpu);
    }
}

int main(void)
{
    CPU cpu;
    Byte memory[MEM_SIZE] = {0};
    init(&cpu, memory);
    Test_ORA(&cpu, memory);
    printf("All tests passed\n");
}

void Test_ORA(CPU *cpu, Byte *memory)
{   
    Test_ORA_IMM(cpu);
    init(cpu, memory);
    Test_ORA_ZP0(cpu);
    init(cpu, memory);
    Test_ORA_ZPX(cpu);
    init(cpu, memory);
    Test_ORA_ABS(cpu);
    init(cpu, memory);
    Test_ORA_ABX(cpu);
    init(cpu, memory);
    Test_ORA_ABY(cpu);
    init(cpu, memory);
    Test_ORA_IZX(cpu);
    init(cpu, memory);
    Test_ORA_IZY(cpu);
    init(cpu, memory);
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
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x80);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, N);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    // test zero case
    cpu->PC = old_pc;
    cpu->A = 0;
    cpu->memory[cpu->PC + 1] = 0x00;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, Z);
    // check positive case
    cpu->PC = old_pc;
    cpu->A = 0x7F;
    cpu->memory[cpu->PC + 1] = 0x01;
    run(cpu, ins.cycles);
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
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x80);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, N);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    // test zero case
    cpu->PC = old_pc;
    cpu->A = 0;
    cpu->memory[cpu->PC + 1] = 0x40;
    cpu->memory[0x0040] = 0x00;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, Z);
    // check positive case
    cpu->PC = old_pc;
    cpu->A = 0x7F;
    cpu->memory[cpu->PC + 1] = 0x40;
    cpu->memory[0x0040] = 0x01;
    run(cpu, ins.cycles);
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
    // test negative case
    cpu->X = 0x10;
    cpu->A = 0;
    cpu->memory[cpu->PC + 1] = 0x40;
    cpu->memory[0x0050] = 0x80;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x80);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, N);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    // test zero case
    cpu->PC = old_pc;
    cpu->X = 0x10;
    cpu->A = 0;
    cpu->memory[cpu->PC + 1] = 0x40;
    cpu->memory[0x0050] = 0x00;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, Z);
    // check positive case
    cpu->PC = old_pc;
    cpu->X = 0x10;
    cpu->A = 0x7F;
    cpu->memory[cpu->PC + 1] = 0x40;
    cpu->memory[0x0050] = 0x01;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x7F);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
}

void Test_ORA_ABS(CPU *cpu)
{
    Word old_pc = cpu->PC = 0x4000;
    cpu->memory[cpu->PC] = INSTRUCTION_ORA_ABS;
    Instruction ins = cpu->table[cpu->memory[cpu->PC]];
    munit_assert_string_equal(ins.name, "ORA");
    munit_assert_int(ins.opcode, ==, INSTRUCTION_ORA_ABS);
    munit_assert_int(ins.length, ==, 3);
    munit_assert_int(ins.cycles, ==, 4);
    // test negative case
    cpu->A = 0;
    cpu->memory[cpu->PC + 1] = 0x00;
    cpu->memory[cpu->PC + 2] = 0x80;
    cpu->memory[0x8000] = 0x80;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x80);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, N);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    // test zero case
    cpu->PC = old_pc;
    cpu->A = 0;
    cpu->memory[cpu->PC + 1] = 0x00;
    cpu->memory[cpu->PC + 2] = 0x80;
    cpu->memory[0x8000] = 0x00;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, Z);   
    // check positive case
    cpu->PC = old_pc;
    cpu->A = 0x7F;
    cpu->memory[cpu->PC + 1] = 0x00;
    cpu->memory[cpu->PC + 2] = 0x80;
    cpu->memory[0x8000] = 0x01;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x7F);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
}

void Test_ORA_ABX(CPU *cpu)
{
    Word old_pc = cpu->PC = 0x4000;
    cpu->memory[cpu->PC] = INSTRUCTION_ORA_ABX;
    Instruction ins = cpu->table[cpu->memory[cpu->PC]];
    munit_assert_string_equal(ins.name, "ORA");
    munit_assert_int(ins.opcode, ==, INSTRUCTION_ORA_ABX);
    munit_assert_int(ins.length, ==, 3);
    munit_assert_int(ins.cycles, ==, 4);
    // test negative case
    cpu->A = 0;
    cpu->X = 0x10;
    cpu->memory[cpu->PC + 1] = 0x00;
    cpu->memory[cpu->PC + 2] = 0x80;
    cpu->memory[0x8010] = 0x80;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x80);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, N);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    // test zero case
    cpu->PC = old_pc;
    cpu->A = 0;
    cpu->X = 0x10;
    cpu->memory[cpu->PC + 1] = 0x00;
    cpu->memory[cpu->PC + 2] = 0x80;
    cpu->memory[0x8010] = 0x00;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, Z);   
    // check positive case
    cpu->PC = old_pc;
    cpu->A = 0x7F;
    cpu->X = 0x10;
    cpu->memory[cpu->PC + 1] = 0x00;
    cpu->memory[cpu->PC + 2] = 0x80;
    cpu->memory[0x8010] = 0x01;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x7F);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
}

void Test_ORA_ABY(CPU *cpu)
{
    Word old_pc = cpu->PC = 0x4000;
    cpu->memory[cpu->PC] = INSTRUCTION_ORA_ABY;
    Instruction ins = cpu->table[cpu->memory[cpu->PC]];
    munit_assert_string_equal(ins.name, "ORA");
    munit_assert_int(ins.opcode, ==, INSTRUCTION_ORA_ABY);
    munit_assert_int(ins.length, ==, 3);
    munit_assert_int(ins.cycles, ==, 4);
    // test negative case
    cpu->A = 0;
    cpu->Y = 0x10;
    cpu->memory[cpu->PC + 1] = 0x00;
    cpu->memory[cpu->PC + 2] = 0x80;
    cpu->memory[0x8010] = 0x80;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x80);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, N);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    // test zero case
    cpu->PC = old_pc;
    cpu->A = 0;
    cpu->Y = 0x10;
    cpu->memory[cpu->PC + 1] = 0x00;
    cpu->memory[cpu->PC + 2] = 0x80;
    cpu->memory[0x8010] = 0x00;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, Z);  
    // check positive case
    cpu->PC = old_pc;
    cpu->A = 0x7F;
    cpu->Y = 0x10;
    cpu->memory[cpu->PC + 1] = 0x00;
    cpu->memory[cpu->PC + 2] = 0x80;
    cpu->memory[0x8010] = 0x01;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x7F);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, 0); 
}

void Test_ORA_IZX(CPU *cpu)
{
    Word old_pc = cpu->PC = 0x4000;
    cpu->memory[cpu->PC] = INSTRUCTION_ORA_IZX;
    Instruction ins = cpu->table[cpu->memory[cpu->PC]];
    munit_assert_string_equal(ins.name, "ORA");
    munit_assert_int(ins.opcode, ==, INSTRUCTION_ORA_IZX);
    munit_assert_int(ins.length, ==, 2);
    munit_assert_int(ins.cycles, ==, 6);
    // test negative case
    //X-indexed, indrect 6502
    cpu->A = 0;
    cpu->X = 0x10;
    cpu->memory[cpu->PC + 1] = 0x40;
    cpu->memory[0x0050] = 0x00;
    cpu->memory[0x0051] = 0x80;
    cpu->memory[0x8000] = 0x80;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x80);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, N);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    // test zero case
    cpu->PC = old_pc;
    cpu->A = 0;
    cpu->X = 0x10;
    cpu->memory[cpu->PC + 1] = 0x40;
    cpu->memory[0x0050] = 0x00;
    cpu->memory[0x0051] = 0x80;
    cpu->memory[0x8000] = 0x00;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, Z);
    // check positive case
    cpu->PC = old_pc;
    cpu->A = 0x7F;
    cpu->X = 0x10;
    cpu->memory[cpu->PC + 1] = 0x40;
    cpu->memory[0x0050] = 0x00;
    cpu->memory[0x0051] = 0x80;
    cpu->memory[0x8000] = 0x01;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x7F);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, 0); 
}

void Test_ORA_IZY(CPU *cpu)
{
    (void)cpu;
}