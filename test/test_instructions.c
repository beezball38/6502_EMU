#include "munit/munit.h"
#include "../src/cpu.h"

//64 kb memory
#define MEM_SIZE 1024 * 64
void Test_BRK(CPU *cpu, Instruction ins);
void Test_BRK_HappyPath(CPU* cpu, Instruction ins);

int main(void)
{
    CPU cpu;
    Byte memory[MEM_SIZE] = {0};
    init(&cpu, memory);
    reset(&cpu);
    Instruction table[0x100] = {0};
    init_instruction_table(table);
    Test_BRK(&cpu, table[0]);
}

void Test_BRK(CPU *cpu, Instruction ins) {
   Test_BRK_HappyPath(cpu, ins); 
}

void Test_BRK_HappyPath(CPU* cpu, Instruction ins) {
    //set up the interupt vector
    write_to_addr(cpu, 0xFFFE, 0x00);
    write_to_addr(cpu, 0xFFFF, 0x80);
    munit_assert_string_equal(ins.name, "BRK");
    munit_assert_int(ins.opcode, ==, 0x00);
    munit_assert_int(ins.length, ==, 2);
    munit_assert_int(ins.cycles, ==, 7);
    Word old_pc = cpu->PC = 0x4000; //random PC:this can happen anywhere
    Byte old_status = cpu->STATUS;
	
    ins.fetch(cpu);

    ins.execute(cpu);

    //assert stack pointer is at 0xFC
    munit_assert_int(cpu->SP, ==, 0xFC);
    //Interupt bit should be set
    munit_assert_true(cpu->STATUS & I);
    //PC should be set to the interupt vector
    munit_assert_int(cpu->PC, ==, 0x8000);
    //test stack contents
    Byte high = read_from_addr(cpu, 0x1FF);
    Byte low = read_from_addr(cpu, 0x1FE);
    Byte status = read_from_addr(cpu, 0x1FD);
    munit_assert_int(low, ==, ((old_pc + 1) & 0xFF));
    munit_assert_int(high, ==, (old_pc >> 8));
    munit_assert_int(status, ==, (old_status | B | I));

}
