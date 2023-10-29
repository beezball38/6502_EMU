#include <stdio.h>
#include "munit/munit.h"
#include "../src/cpu.h"

// 64 kb memory
#define MEM_SIZE 1024 * 64

#define TEST_LIST \
    X(AND, IMM) \
    X(AND, ZP0) \
    X(AND, ZPX) \
    X(AND, ABS) \
    X(AND, ABX) \
    X(AND, ABY) \
    X(AND, IZX) \
    X(AND, IZY) \
    X(CLC, IMP) \
    X(ORA, IMM) \
    X(ORA, ZP0) \
    X(ORA, ZPX) \
    X(ORA, ABS) \
    X(ORA, ABX) \
    X(ORA, ABY) \
    X(ORA, IZX) \
    X(ORA, IZY) \
    X(PHA, IMP) \
    X(PHP, IMP) \

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
    Test_AND(&cpu, memory);
    Test_CLC(&cpu, memory);
    Test_ORA(&cpu, memory);
    Test_PHP(&cpu, memory);
    Test_PHA(&cpu, memory);
    printf("All tests passed\n");
    return EXIT_SUCCESS;
}

void Test_AND(CPU *cpu, Byte *memory)
{
    init(cpu, memory);
    Test_AND_IMM(cpu);
    init(cpu, memory);
    Test_AND_ZP0(cpu);
}

void Test_AND_IMM(CPU *cpu)
{
    Word old_pc = cpu->PC = 0x4000;
    cpu->memory[cpu->PC] = INSTRUCTION_AND_IMM;
    Instruction ins = cpu->table[cpu->memory[cpu->PC]];
    munit_assert_string_equal(ins.name, "AND");
    munit_assert_int(ins.opcode, ==, INSTRUCTION_AND_IMM);
    munit_assert_int(ins.length, ==, 2);
    munit_assert_int(ins.cycles, ==, 2);
    // test case where A is negative after AND
    cpu->A = 0x80;
    cpu->memory[cpu->PC + 1] = 0x81;
    //save status
    Byte old_status = cpu->STATUS = (rand() % 256) & ~N | Z;
    //assert N is not set and Z is set
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, Z);
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x80);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    //assert the N and Z flags have been changed, and nothing else is edited
    munit_assert_int(cpu->STATUS & N, ==, N);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
    // test case where A is zero after AND
    cpu->PC = old_pc;
    cpu->A = 0x01;
    cpu->memory[cpu->PC + 1] = 0x00;
    //save status
    old_status = cpu->STATUS = (rand() % 256) & ~Z | N;
    //assert N is set and Z is not set
    munit_assert_int(cpu->STATUS & N, ==, N);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x00);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    //assert the N and Z flags have been changed, and nothing else is edited
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, Z);
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
    // test case where A is positive after AND
    cpu->PC = old_pc;
    cpu->A = 0x01;
    cpu->memory[cpu->PC + 1] = 0x01;
    //save status
    old_status = cpu->STATUS = (rand() % 256) & ~(N | Z);
    //assert N is not set and Z is not set
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x01);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    //assert the N and Z flags have been changed, and nothing else is edited
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
}

void Test_AND_ZP0(CPU *cpu)
{
    Word old_pc = cpu->PC = 0x4000;
    cpu->memory[cpu->PC] = INSTRUCTION_AND_ZP0;
    Instruction ins = cpu->table[cpu->memory[cpu->PC]];
    munit_assert_string_equal(ins.name, "AND");
    munit_assert_int(ins.opcode, ==, INSTRUCTION_AND_ZP0);
    munit_assert_int(ins.length, ==, 2);
    munit_assert_int(ins.cycles, ==, 3);
    //todo finish this test
}

void Test_ORA(CPU *cpu, Byte *memory)
{   
    init(cpu, memory);
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
    Byte old_status = cpu->STATUS = (rand() % 256) | U;
    cpu->A = 0;
    cpu->memory[cpu->PC + 1] = 0x80;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x80);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, N);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
    // test zero case
    cpu->PC = old_pc;
    old_status = cpu->STATUS = (rand() % 256) | U;
    cpu->A = 0;
    cpu->memory[cpu->PC + 1] = 0x00;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, Z);
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
    // check positive case
    cpu->PC = old_pc;
    old_status = cpu->STATUS = (rand() % 256) | U;
    cpu->A = 0x7F;
    cpu->memory[cpu->PC + 1] = 0x01;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x7F);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    // check flags
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
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
    Byte old_status = cpu->STATUS = (rand() % 256) | U;
    cpu->A = 0;
    cpu->memory[cpu->PC + 1] = 0x40;
    cpu->memory[0x0040] = 0x80;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x80);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, N);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
    // test zero case
    old_status = cpu->STATUS = (rand() % 256) | U;
    cpu->PC = old_pc;
    cpu->A = 0;
    cpu->memory[cpu->PC + 1] = 0x40;
    cpu->memory[0x0040] = 0x00;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, Z);
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
    // check positive case
    old_status = cpu->STATUS = (rand() % 256) | U;
    cpu->PC = old_pc;
    cpu->A = 0x7F;
    cpu->memory[cpu->PC + 1] = 0x40;
    cpu->memory[0x0040] = 0x01;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x7F);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
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
    Byte old_status = cpu->STATUS = (rand() % 256) | U;
    cpu->X = 0x10;
    cpu->A = 0;
    cpu->memory[cpu->PC + 1] = 0x40;
    cpu->memory[0x0050] = 0x80;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x80);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, N);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
    // test zero case
    old_status = cpu->STATUS = (rand() % 256) | U;
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
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
    // check positive case
    old_status = cpu->STATUS = (rand() % 256) | U;
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
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
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
    Byte old_status = cpu->STATUS = (rand() % 256) | U;
    cpu->A = 0;
    cpu->memory[cpu->PC + 1] = 0x00;
    cpu->memory[cpu->PC + 2] = 0x80;
    cpu->memory[0x8000] = 0x80;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x80);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, N);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
    // test zero case
    old_status = cpu->STATUS = (rand() % 256) | U;
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
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z)); 
    // check positive case
    old_status = cpu->STATUS = (rand() % 256) | U;
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
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
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
    Byte old_status = cpu->STATUS = (rand() % 256) | U;
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
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
    // test zero case
    old_status = cpu->STATUS = (rand() % 256) | U;
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
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
    // check positive case
    old_status = cpu->STATUS = (rand() % 256) | U;
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
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
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
    Byte old_status = cpu->STATUS = (rand() % 256) | U;
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
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
    // test zero case
    old_status = cpu->STATUS = (rand() % 256) | U;
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
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z)); 
    // check positive case
    old_status = cpu->STATUS = (rand() % 256) | U;
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
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
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
    Byte old_status = cpu->STATUS = (rand() % 256) | U;
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
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
    // test zero case
    old_status = cpu->STATUS = (rand() % 256) | U;
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
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));    
    // check positive case
    old_status = cpu->STATUS = (rand() % 256) | U;
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
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
}

void Test_ORA_IZY(CPU *cpu)
{
    Word old_pc = cpu->PC = 0x4000;
    cpu->memory[cpu->PC] = INSTRUCTION_ORA_IZY;
    Instruction ins = cpu->table[cpu->memory[cpu->PC]];
    munit_assert_string_equal(ins.name, "ORA");
    munit_assert_int(ins.opcode, ==, INSTRUCTION_ORA_IZY);
    munit_assert_int(ins.length, ==, 2);
    munit_assert_int(ins.cycles, ==, 5);
    // test negative case
    //Indirect, Y-indexed 6502
    Byte old_status = cpu->STATUS = (rand() % 256) | U;
    cpu->A = 0;
    cpu->Y = 0x10;
    cpu->memory[cpu->PC + 1] = 0x40;
    cpu->memory[0x0040] = 0x00;
    cpu->memory[0x0041] = 0x80;
    cpu->memory[0x8010] = 0x80;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x80);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, N);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
    // test zero case
    old_status = cpu->STATUS = (rand() % 256) | U;
    cpu->PC = old_pc;
    cpu->A = 0;
    cpu->Y = 0x10;
    cpu->memory[cpu->PC + 1] = 0x40;
    cpu->memory[0x0040] = 0x00;
    cpu->memory[0x0041] = 0x80;
    cpu->memory[0x8010] = 0x00;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, Z);
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));

    // check positive case
    old_status = cpu->STATUS = (rand() % 256) | U;
    cpu->PC = old_pc;
    cpu->A = 0x7F;
    cpu->Y = 0x10;
    cpu->memory[cpu->PC + 1] = 0;
    cpu->memory[0x0040] = 0x00;
    cpu->memory[0x0041] = 0x80;
    cpu->memory[0x8010] = 0x01;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->A, ==, 0x7F);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS & N, ==, 0);
    munit_assert_int(cpu->STATUS & Z, ==, 0);
    munit_assert_int(cpu->STATUS & ~(N | Z), ==, old_status & ~(N | Z));
}

void Test_PHP(CPU *cpu, Byte *memory)
{
    init(cpu, memory);
    Test_PHP_IMP(cpu);
}

void Test_PHP_IMP(CPU *cpu)
{
    Word old_pc = cpu->PC = 0x4000;
    cpu->memory[cpu->PC] = INSTRUCTION_PHP_IMP;
    Instruction ins = cpu->table[cpu->memory[cpu->PC]];
    munit_assert_string_equal(ins.name, "PHP");
    munit_assert_int(ins.opcode, ==, INSTRUCTION_PHP_IMP);
    munit_assert_int(ins.length, ==, 1);
    munit_assert_int(ins.cycles, ==, 3);

    Byte old_status = cpu->STATUS = (rand() % 256) | U;
    cpu->SP = 0xFF;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->memory[0x01FF], ==, old_status | B); 
    munit_assert_int(cpu->SP, ==, 0xFE);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
}

void Test_PHA(CPU *cpu, Byte *memory)
{
    init(cpu, memory);
    Test_PHA_IMP(cpu);
}

void Test_PHA_IMP(CPU *cpu)
{
    Word old_pc = cpu->PC = 0x4000;
    cpu->memory[cpu->PC] = INSTRUCTION_PHA_IMP;
    Instruction ins = cpu->table[cpu->memory[cpu->PC]];
    munit_assert_string_equal(ins.name, "PHA");
    munit_assert_int(ins.opcode, ==, INSTRUCTION_PHA_IMP);
    munit_assert_int(ins.length, ==, 1);
    munit_assert_int(ins.cycles, ==, 3);

    //set A to random value between 0 and 255
    Byte old_A = cpu->A = rand() % 256;
    //ensure stack is empty
    cpu->SP = 0xFF;
    run(cpu, ins.cycles);
    //check that A was pushed to stack
    munit_assert_int(cpu->memory[0x01FF], ==, old_A);
    //check that stack pointer was decremented
    munit_assert_int(cpu->SP, ==, 0xFE);
    //check that program counter was incremented
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
}

void Test_CLC(CPU *cpu, Byte *memory)
{
    init(cpu, memory);
    Test_CLC_IMP(cpu);
}

void Test_CLC_IMP(CPU *cpu)
{
    Word old_pc = cpu->PC = 0x4000;
    cpu->memory[cpu->PC] = INSTRUCTION_CLC_IMP;
    Instruction ins = cpu->table[cpu->memory[cpu->PC]];
    munit_assert_string_equal(ins.name, "CLC");
    munit_assert_int(ins.opcode, ==, INSTRUCTION_CLC_IMP);
    munit_assert_int(ins.length, ==, 1);
    munit_assert_int(ins.cycles, ==, 2);


    Byte old_status = cpu->STATUS = (rand() % 256) | C | U;
    run(cpu, ins.cycles);
    munit_assert_int(cpu->STATUS & C, ==, 0);
    munit_assert_int(cpu->PC, ==, old_pc + ins.length);
    munit_assert_int(cpu->STATUS, ==, old_status & ~C);
}
