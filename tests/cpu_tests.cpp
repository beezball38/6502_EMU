// tests/cpu_tests.cpp
#include <cassert>
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <vector>
#include "cpu.hpp"
#define MEM_SIZE 64 * 1024 * 1024
// singletons for tests, will only be accessed via get_test_cpu
static cpu_s cpu;
static byte_t memory[MEM_SIZE] = {0};

cpu_s *get_test_cpu();
void load_instruction(cpu_s *cpu, std::vector<byte_t> instruction);
void run_instruction(cpu_s *cpu);

cpu_s *get_test_cpu()
{
    memset(memory, 0, MEM_SIZE);
    cpu_init(&cpu, memory);
    init_instruction_table(&cpu);
    return &cpu;
}

void load_instruction(cpu_s *cpu, std::vector<byte_t> instruction)
{
    assert(instruction.size() > 0);
    const byte_t op_code = instruction.at(0);
    cpu->current_opcode = op_code;
    cpu->instruction_pending = true;
    cpu_instruction_s *current_instruction = get_current_instruction(cpu);
    assert(current_instruction->length == instruction.size());
    byte_t pc = cpu->PC;
    for(byte_t byte:instruction)
    {
        cpu->memory[pc++] = byte;
    }
}

void run_instruction(cpu_s *cpu)
{
        cpu_instruction_s *current_instruction = get_current_instruction(cpu);
        if(current_instruction->fetch != NULL)
        {
            current_instruction->fetch(cpu);
        }

        if(current_instruction->execute != NULL)
        {
            current_instruction->execute(cpu);
        }
}

void load_interrupt_vector(cpu_s *cpu, byte_t irq_vector_low, byte_t irq_vector_high)
{
    cpu->memory[0xFFFE] = irq_vector_low;
    cpu->memory[0xFFFF] = irq_vector_high;
}

// =============================================================================
// Tests ordered by opcode (ascending)
// =============================================================================

// 0x00 - BRK (Force Interrupt)
TEST_CASE("0x00 BRK", "[cpu][interrupt]") {
    cpu_s *cpu = get_test_cpu();
    cpu->PC = 0x0200; // Known PC so we can verify pushed return address

    load_instruction(cpu, {INSTRUCTION_BRK_IMP, 0x00}); // BRK opcode + dummy byte
    byte_t irq_vector_low = 0x34;
    byte_t irq_vector_high = 0x12;
    load_interrupt_vector(cpu, irq_vector_low, irq_vector_high); // IRQ vector -> 0x1234

    run_instruction(cpu);

    // I (interrupt disable) flag is set
    REQUIRE(get_flag(cpu, I) == true);
    // PC is loaded from IRQ vector at 0xFFFE/0xFFFF
    REQUIRE(cpu->PC == 0x1234);
    // Return address (PC+1 after opcode) pushed high then low: 0x0201
    REQUIRE(cpu->memory[0x01FC] == 0x01); // PCL
    REQUIRE(cpu->memory[0x01FD] == 0x02); // PCH
    // Status was pushed with I and B set (B cleared in register after push)
    byte_t pushed_status = cpu->memory[0x01FB];
    REQUIRE((pushed_status & I) != 0);
    REQUIRE((pushed_status & B) != 0);
}

// 0x01 - ORA (IZX)
TEST_CASE("0x01 ORA IZX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0F;
    cpu->X = 0;
    byte_t zero_page_addr = 0x10;
    byte_t pointer_low_byte = 0x00;
    byte_t pointer_high_byte = 0x03;
    cpu->memory[0x10] = pointer_low_byte;
    cpu->memory[0x11] = pointer_high_byte;
    cpu->memory[0x0300] = 0xF0;
    load_instruction(cpu, {INSTRUCTION_ORA_IZX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0xFF);
}

// 0x05 - ORA ZP0
TEST_CASE("0x05 ORA ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0F;
    byte_t zero_page_addr = 0x20;
    cpu->memory[0x20] = 0xF0;
    load_instruction(cpu, {INSTRUCTION_ORA_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0xFF);
}

// 0x06 - ASL ZP0
TEST_CASE("0x06 ASL ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t zero_page_addr = 0x30;
    cpu->memory[0x30] = 0x40;
    load_instruction(cpu, {INSTRUCTION_ASL_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, C) == false);
    REQUIRE(get_flag(cpu, Z) == false);
    REQUIRE(get_flag(cpu, N) == true);
}

// 0x08 - PHP
TEST_CASE("0x08 PHP", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, N, true);
    load_instruction(cpu, {INSTRUCTION_PHP_IMP});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x01FD] == (cpu->STATUS | B | U));
}

// 0x09 - ORA IMM
TEST_CASE("0x09 ORA IMM", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0F;
    byte_t immediate_value = 0xF0;
    load_instruction(cpu, {INSTRUCTION_ORA_IMM, immediate_value});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0xFF);
}

// 0x0A - ASL ACC
TEST_CASE("0x0A ASL ACC", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x40;
    load_instruction(cpu, {INSTRUCTION_ASL_ACC});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, C) == false);
    REQUIRE(get_flag(cpu, N) == true);
}

// 0x0D - ORA ABS
TEST_CASE("0x0D ORA ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0F;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x04;
    cpu->memory[0x0400] = 0xF0;
    load_instruction(cpu, {INSTRUCTION_ORA_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0xFF);
}

// 0x0E - ASL ABS
TEST_CASE("0x0E ASL ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x04;
    cpu->memory[0x0400] = 0x40;
    load_instruction(cpu, {INSTRUCTION_ASL_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, C) == false);
    REQUIRE(get_flag(cpu, N) == true);
}

// 0x10 - BPL (branch when N=0) // FAILING
TEST_CASE("0x10 BPL", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->PC = 0x0200;
    set_flag(cpu, N, false);
    byte_t branch_offset = 0x04;
    load_instruction(cpu, {INSTRUCTION_BPL_REL, branch_offset});
    run_instruction(cpu);
    REQUIRE(cpu->PC == 0x0200 + 2 + branch_offset);
}

// 0x11 - ORA IZY
TEST_CASE("0x11 ORA IZY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0F;
    cpu->Y = 0;
    byte_t zero_page_addr = 0x20;
    byte_t pointer_low_byte = 0x00;
    byte_t pointer_high_byte = 0x03;
    cpu->memory[0x20] = pointer_low_byte;
    cpu->memory[0x21] = pointer_high_byte;
    cpu->memory[0x0300] = 0xF0;
    load_instruction(cpu, {INSTRUCTION_ORA_IZY, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0xFF);
}

// 0x15 - ORA ZPX
TEST_CASE("0x15 ORA ZPX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0F;
    cpu->X = 0x02;
    byte_t zero_page_addr = 0x20;
    cpu->memory[0x22] = 0xF0;
    load_instruction(cpu, {INSTRUCTION_ORA_ZPX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0xFF);
}

// 0x16 - ASL ZPX
TEST_CASE("0x16 ASL ZPX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x01;
    byte_t zero_page_addr = 0x30;
    cpu->memory[0x31] = 0x40;
    load_instruction(cpu, {INSTRUCTION_ASL_ZPX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, C) == false);
    REQUIRE(get_flag(cpu, N) == true);
}

// 0x18 - CLC (Clear Carry Flag)
TEST_CASE("0x18 CLC", "[cpu][flags]") {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, C, true);
    REQUIRE(get_flag(cpu, C) == true);

    load_instruction(cpu, {INSTRUCTION_CLC_IMP});
    run_instruction(cpu);

    REQUIRE(get_flag(cpu, C) == false);
}

// 0x19 - ORA ABY
TEST_CASE("0x19 ORA ABY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0F;
    cpu->Y = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x04;
    cpu->memory[0x0400] = 0xF0;
    load_instruction(cpu, {INSTRUCTION_ORA_ABY, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0xFF);
}

// 0x1D - ORA ABX
TEST_CASE("0x1D ORA ABX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0F;
    cpu->X = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x04;
    cpu->memory[0x0400] = 0xF0;
    load_instruction(cpu, {INSTRUCTION_ORA_ABX, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0xFF);
}

// 0x1E - ASL ABX
TEST_CASE("0x1E ASL ABX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x04;
    cpu->memory[0x0400] = 0x40;
    load_instruction(cpu, {INSTRUCTION_ASL_ABX, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, C) == false);
    REQUIRE(get_flag(cpu, N) == true);
}

// 0x20 - JSR // FAILING
TEST_CASE("0x20 JSR", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->PC = 0x0200;
    byte_t address_low_byte = 0x34;
    byte_t address_high_byte = 0x12;
    load_instruction(cpu, {INSTRUCTION_JSR_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->PC == 0x1234);
    // Return address pushed (emulator pushes PC+1): 0x0201 low, 0x02 high
    REQUIRE(cpu->memory[0x01FC] == 0x01);
    REQUIRE(cpu->memory[0x01FD] == 0x02);
}

// 0x21 - AND IZX
TEST_CASE("0x21 AND IZX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->X = 0;
    byte_t zero_page_addr = 0x10;
    byte_t pointer_low_byte = 0x00;
    byte_t pointer_high_byte = 0x03;
    cpu->memory[0x10] = pointer_low_byte;
    cpu->memory[0x11] = pointer_high_byte;
    cpu->memory[0x0300] = 0x0F;
    load_instruction(cpu, {INSTRUCTION_AND_IZX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x0F);
}

// 0x24 - BIT ZP0 (opcode constant INSTRUCTION_BRK_ZP0, executes BIT)
TEST_CASE("0x24 BIT ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    byte_t zero_page_addr = 0x40;
    cpu->memory[0x40] = 0x80;
    load_instruction(cpu, {INSTRUCTION_BRK_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, Z) == false);
    REQUIRE(get_flag(cpu, N) == true);
}

// 0x25 - AND ZP0
TEST_CASE("0x25 AND ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    byte_t zero_page_addr = 0x50;
    cpu->memory[0x50] = 0x0F;
    load_instruction(cpu, {INSTRUCTION_AND_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x0F);
}

// 0x26 - ROL ZP0
TEST_CASE("0x26 ROL ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t zero_page_addr = 0x60;
    cpu->memory[0x60] = 0x80;
    load_instruction(cpu, {INSTRUCTION_ROL_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x60] == 0x00);
    REQUIRE(get_flag(cpu, C) == true);
    REQUIRE(get_flag(cpu, Z) == true);
}

// 0x28 - PLP
TEST_CASE("0x28 PLP", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->memory[0x01FC] = 0x80;
    cpu->SP = 0xFB;
    load_instruction(cpu, {INSTRUCTION_PLP_IMP});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, N) == true);
}

// 0x29 - AND IMM
TEST_CASE("0x29 AND IMM", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    byte_t immediate_value = 0x0F;
    load_instruction(cpu, {INSTRUCTION_AND_IMM, immediate_value});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x0F);
}

// 0x2A - ROL ACC // FAILING
TEST_CASE("0x2A ROL ACC", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x80;
    load_instruction(cpu, {INSTRUCTION_ROL_ACC});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x00);
    REQUIRE(get_flag(cpu, C) == true);
    REQUIRE(get_flag(cpu, Z) == true);
}

// 0x2C - BIT ABS
TEST_CASE("0x2C BIT ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x01;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x40;
    load_instruction(cpu, {INSTRUCTION_BIT_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, Z) == true);
    REQUIRE(get_flag(cpu, V) == true);
}

// 0x2D - AND ABS
TEST_CASE("0x2D AND ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x0F;
    load_instruction(cpu, {INSTRUCTION_AND_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x0F);
}

// 0x2E - ROL ABS
TEST_CASE("0x2E ROL ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x80;
    load_instruction(cpu, {INSTRUCTION_ROL_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x0500] == 0x00);
    REQUIRE(get_flag(cpu, C) == true);
}

// 0x30 - BMI // FAILING
TEST_CASE("0x30 BMI", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->PC = 0x0300;
    set_flag(cpu, N, true);
    byte_t branch_offset = 0x02;
    load_instruction(cpu, {INSTRUCTION_BMI_REL, branch_offset});
    run_instruction(cpu);
    REQUIRE(cpu->PC == 0x0300 + 2 + branch_offset);
}

// 0x31 - AND IZY
TEST_CASE("0x31 AND IZY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->Y = 0;
    byte_t zero_page_addr = 0x30;
    byte_t pointer_low_byte = 0x00;
    byte_t pointer_high_byte = 0x03;
    cpu->memory[0x30] = pointer_low_byte;
    cpu->memory[0x31] = pointer_high_byte;
    cpu->memory[0x0300] = 0x0F;
    load_instruction(cpu, {INSTRUCTION_AND_IZY, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x0F);
}

// 0x35 - AND ZPX
TEST_CASE("0x35 AND ZPX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->X = 0x01;
    byte_t zero_page_addr = 0x40;
    cpu->memory[0x41] = 0x0F;
    load_instruction(cpu, {INSTRUCTION_AND_ZPX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x0F);
}

// 0x36 - ROL ZPX
TEST_CASE("0x36 ROL ZPX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x01;
    byte_t zero_page_addr = 0x50;
    cpu->memory[0x51] = 0x80;
    load_instruction(cpu, {INSTRUCTION_ROL_ZPX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x51] == 0x00);
    REQUIRE(get_flag(cpu, C) == true);
}

// 0x38 - SEC (Set Carry Flag)
TEST_CASE("0x38 SEC", "[cpu][flags]") {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, C, false);
    REQUIRE(get_flag(cpu, C) == false);

    load_instruction(cpu, {INSTRUCTION_SEC_IMP});
    run_instruction(cpu);

    REQUIRE(get_flag(cpu, C) == true);
}

// 0x39 - AND ABY
TEST_CASE("0x39 AND ABY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->Y = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x0F;
    load_instruction(cpu, {INSTRUCTION_AND_ABY, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x0F);
}

// 0x3D - AND ABX
TEST_CASE("0x3D AND ABX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->X = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x0F;
    load_instruction(cpu, {INSTRUCTION_AND_ABX, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x0F);
}

// 0x3E - ROL ABX
TEST_CASE("0x3E ROL ABX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x80;
    load_instruction(cpu, {INSTRUCTION_ROL_ABX, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x0500] == 0x00);
    REQUIRE(get_flag(cpu, C) == true);
}

// 0x40 - RTI
TEST_CASE("0x40 RTI", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->memory[0x01FB] = 0x00;
    cpu->memory[0x01FC] = 0x34;
    cpu->memory[0x01FD] = 0x12;
    cpu->SP = 0xFA;
    load_instruction(cpu, {INSTRUCTION_RTI_IMP});
    run_instruction(cpu);
    REQUIRE(cpu->PC == 0x1234);
}

// 0x41 - EOR IZX
TEST_CASE("0x41 EOR IZX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->X = 0;
    byte_t zero_page_addr = 0x10;
    byte_t pointer_low_byte = 0x00;
    byte_t pointer_high_byte = 0x03;
    cpu->memory[0x10] = pointer_low_byte;
    cpu->memory[0x11] = pointer_high_byte;
    cpu->memory[0x0300] = 0x0F;
    load_instruction(cpu, {INSTRUCTION_EOR_IZX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0xF0);
}

// 0x45 - EOR ZP0
TEST_CASE("0x45 EOR ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    byte_t zero_page_addr = 0x20;
    cpu->memory[0x20] = 0x0F;
    load_instruction(cpu, {INSTRUCTION_EOR_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0xF0);
}

// 0x46 - LSR ZP0
TEST_CASE("0x46 LSR ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t zero_page_addr = 0x30;
    cpu->memory[0x30] = 0x04;
    load_instruction(cpu, {INSTRUCTION_LSR_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x30] == 0x02);
    REQUIRE(get_flag(cpu, C) == false);
}

// 0x48 - PHA
TEST_CASE("0x48 PHA", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xAB;
    load_instruction(cpu, {INSTRUCTION_PHA_IMP});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x01FD] == 0xAB);
}

// 0x49 - EOR IMM
TEST_CASE("0x49 EOR IMM", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    byte_t immediate_value = 0x0F;
    load_instruction(cpu, {INSTRUCTION_EOR_IMM, immediate_value});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0xF0);
}

// 0x4A - LSR ACC // FAILING
TEST_CASE("0x4A LSR ACC", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x04;
    load_instruction(cpu, {INSTRUCTION_LSR_ACC});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x02);
    REQUIRE(get_flag(cpu, C) == false);
}

// 0x4C - JMP ABS
TEST_CASE("0x4C JMP ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t address_low_byte = 0x56;
    byte_t address_high_byte = 0x34;
    load_instruction(cpu, {INSTRUCTION_JMP_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->PC == 0x3456);
}

// 0x4D - EOR ABS
TEST_CASE("0x4D EOR ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x0F;
    load_instruction(cpu, {INSTRUCTION_EOR_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0xF0);
}

// 0x4E - LSR ABS
TEST_CASE("0x4E LSR ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x04;
    load_instruction(cpu, {INSTRUCTION_LSR_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x0500] == 0x02);
}

// 0x50 - BVC // FAILING
TEST_CASE("0x50 BVC", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->PC = 0x0400;
    set_flag(cpu, V, false);
    byte_t branch_offset = 0x06;
    load_instruction(cpu, {INSTRUCTION_BVC_REL, branch_offset});
    run_instruction(cpu);
    REQUIRE(cpu->PC == 0x0400 + 2 + branch_offset);
}

// 0x51 - EOR IZY
TEST_CASE("0x51 EOR IZY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->Y = 0;
    byte_t zero_page_addr = 0x30;
    byte_t pointer_low_byte = 0x00;
    byte_t pointer_high_byte = 0x03;
    cpu->memory[0x30] = pointer_low_byte;
    cpu->memory[0x31] = pointer_high_byte;
    cpu->memory[0x0300] = 0x0F;
    load_instruction(cpu, {INSTRUCTION_EOR_IZY, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0xF0);
}

// 0x55 - EOR ZPX
TEST_CASE("0x55 EOR ZPX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->X = 0x01;
    byte_t zero_page_addr = 0x40;
    cpu->memory[0x41] = 0x0F;
    load_instruction(cpu, {INSTRUCTION_EOR_ZPX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0xF0);
}

// 0x56 - LSR ZPX
TEST_CASE("0x56 LSR ZPX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x01;
    byte_t zero_page_addr = 0x50;
    cpu->memory[0x51] = 0x04;
    load_instruction(cpu, {INSTRUCTION_LSR_ZPX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x51] == 0x02);
}

// 0x58 - CLI (Clear Interrupt Disable Flag)
TEST_CASE("0x58 CLI", "[cpu][flags]") {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, I, true);
    REQUIRE(get_flag(cpu, I) == true);

    load_instruction(cpu, {INSTRUCTION_CLI_IMP});
    run_instruction(cpu);

    REQUIRE(get_flag(cpu, I) == false);
}

// 0x59 - EOR ABY
TEST_CASE("0x59 EOR ABY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->Y = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x0F;
    load_instruction(cpu, {INSTRUCTION_EOR_ABY, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0xF0);
}

// 0x5D - EOR ABX
TEST_CASE("0x5D EOR ABX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->X = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x0F;
    load_instruction(cpu, {INSTRUCTION_EOR_ABX, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0xF0);
}

// 0x5E - LSR ABX
TEST_CASE("0x5E LSR ABX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x04;
    load_instruction(cpu, {INSTRUCTION_LSR_ABX, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x0500] == 0x02);
}

// 0x60 - RTS
TEST_CASE("0x60 RTS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->memory[0x01FC] = 0x02;
    cpu->memory[0x01FD] = 0x03;
    cpu->SP = 0xFB;
    load_instruction(cpu, {INSTRUCTION_RTS_IMP});
    run_instruction(cpu);
    REQUIRE(cpu->PC == 0x0303);
}

// 0x61 - ADC IZX
TEST_CASE("0x61 ADC IZX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x01;
    set_flag(cpu, C, false);
    cpu->X = 0;
    byte_t zero_page_addr = 0x10;
    byte_t pointer_low_byte = 0x00;
    byte_t pointer_high_byte = 0x03;
    cpu->memory[0x10] = pointer_low_byte;
    cpu->memory[0x11] = pointer_high_byte;
    cpu->memory[0x0300] = 0x02;
    load_instruction(cpu, {INSTRUCTION_ADC_IZX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x03);
}

// 0x65 - ADC ZP0
TEST_CASE("0x65 ADC ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x01;
    set_flag(cpu, C, false);
    byte_t zero_page_addr = 0x20;
    cpu->memory[0x20] = 0x02;
    load_instruction(cpu, {INSTRUCTION_ADC_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x03);
}

// 0x66 - ROR ZP0
TEST_CASE("0x66 ROR ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, C, true);
    byte_t zero_page_addr = 0x30;
    cpu->memory[0x30] = 0x00;
    load_instruction(cpu, {INSTRUCTION_ROR_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x30] == 0x80);
    REQUIRE(get_flag(cpu, C) == false);
}

// 0x68 - PLA
TEST_CASE("0x68 PLA", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->memory[0x01FD] = 0xCD;
    cpu->SP = 0xFC;
    load_instruction(cpu, {INSTRUCTION_PLA_IMP});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0xCD);
}

// 0x69 - ADC IMM
TEST_CASE("0x69 ADC IMM", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x01;
    set_flag(cpu, C, false);
    byte_t immediate_value = 0x02;
    load_instruction(cpu, {INSTRUCTION_ADC_IMM, immediate_value});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x03);
}

// 0x6A - ROR ACC // FAILING
TEST_CASE("0x6A ROR ACC", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, C, true);
    cpu->A = 0x01;
    load_instruction(cpu, {INSTRUCTION_ROR_ACC});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x80);
    REQUIRE(get_flag(cpu, C) == true);
    REQUIRE(get_flag(cpu, N) == true);
}

// 0x6C - JMP IND // FAILING
TEST_CASE("0x6C JMP IND", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t pointer_low_byte = 0x10;
    byte_t pointer_high_byte = 0x00;
    cpu->memory[0x0010] = 0x78;
    cpu->memory[0x0011] = 0x56;
    load_instruction(cpu, {INSTRUCTION_JMP_IND, pointer_low_byte, pointer_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->PC == 0x5678);
}

// 0x6D - ADC ABS
TEST_CASE("0x6D ADC ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x01;
    set_flag(cpu, C, false);
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x02;
    load_instruction(cpu, {INSTRUCTION_ADC_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x03);
}

// 0x6E - ROR ABS
TEST_CASE("0x6E ROR ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, C, true);
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x00;
    load_instruction(cpu, {INSTRUCTION_ROR_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x0500] == 0x80);
}

// 0x70 - BVS // FAILING
TEST_CASE("0x70 BVS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->PC = 0x0500;
    set_flag(cpu, V, true);
    byte_t branch_offset = 0x04;
    load_instruction(cpu, {INSTRUCTION_BVS_REL, branch_offset});
    run_instruction(cpu);
    REQUIRE(cpu->PC == 0x0500 + 2 + branch_offset);
}

// 0x71 - ADC IZY
TEST_CASE("0x71 ADC IZY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x01;
    set_flag(cpu, C, false);
    cpu->Y = 0;
    byte_t zero_page_addr = 0x30;
    byte_t pointer_low_byte = 0x00;
    byte_t pointer_high_byte = 0x03;
    cpu->memory[0x30] = pointer_low_byte;
    cpu->memory[0x31] = pointer_high_byte;
    cpu->memory[0x0300] = 0x02;
    load_instruction(cpu, {INSTRUCTION_ADC_IZY, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x03);
}

// 0x75 - ADC ZPX
TEST_CASE("0x75 ADC ZPX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x01;
    set_flag(cpu, C, false);
    cpu->X = 0x01;
    byte_t zero_page_addr = 0x40;
    cpu->memory[0x41] = 0x02;
    load_instruction(cpu, {INSTRUCTION_ADC_ZPX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x03);
}

// 0x76 - ROR ZPX
TEST_CASE("0x76 ROR ZPX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, C, true);
    cpu->X = 0x01;
    byte_t zero_page_addr = 0x50;
    cpu->memory[0x51] = 0x00;
    load_instruction(cpu, {INSTRUCTION_ROR_ZPX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x51] == 0x80);
}

// 0x78 - SEI (Set Interrupt Disable Flag)
TEST_CASE("0x78 SEI", "[cpu][flags]") {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, I, false);
    REQUIRE(get_flag(cpu, I) == false);

    load_instruction(cpu, {INSTRUCTION_SEI_IMP});
    run_instruction(cpu);

    REQUIRE(get_flag(cpu, I) == true);
}

// 0x79 - ADC ABY
TEST_CASE("0x79 ADC ABY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x01;
    set_flag(cpu, C, false);
    cpu->Y = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x02;
    load_instruction(cpu, {INSTRUCTION_ADC_ABY, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x03);
}

// 0x7D - ADC ABX
TEST_CASE("0x7D ADC ABX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x01;
    set_flag(cpu, C, false);
    cpu->X = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x02;
    load_instruction(cpu, {INSTRUCTION_ADC_ABX, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x03);
}

// 0x7E - ROR ABX
TEST_CASE("0x7E ROR ABX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, C, true);
    cpu->X = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x00;
    load_instruction(cpu, {INSTRUCTION_ROR_ABX, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x0500] == 0x80);
}

// 0x81 - STA IZX
TEST_CASE("0x81 STA IZX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xAB;
    cpu->X = 0;
    byte_t zero_page_addr = 0x10;
    byte_t pointer_low_byte = 0x00;
    byte_t pointer_high_byte = 0x03;
    cpu->memory[0x10] = pointer_low_byte;
    cpu->memory[0x11] = pointer_high_byte;
    load_instruction(cpu, {INSTRUCTION_STA_IZX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x0300] == 0xAB);
}

// 0x84 - STY ZP0
TEST_CASE("0x84 STY ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0xCD;
    byte_t zero_page_addr = 0x20;
    load_instruction(cpu, {INSTRUCTION_STY_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x20] == 0xCD);
}

// 0x85 - STA ZP0
TEST_CASE("0x85 STA ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xAB;
    byte_t zero_page_addr = 0x20;
    load_instruction(cpu, {INSTRUCTION_STA_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x20] == 0xAB);
}

// 0x86 - STX ZP0
TEST_CASE("0x86 STX ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0xEF;
    byte_t zero_page_addr = 0x20;
    load_instruction(cpu, {INSTRUCTION_STX_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x20] == 0xEF);
}

// 0x88 - DEY
TEST_CASE("0x88 DEY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0x01;
    load_instruction(cpu, {INSTRUCTION_DEY_IMP});
    run_instruction(cpu);
    REQUIRE(cpu->Y == 0x00);
    REQUIRE(get_flag(cpu, Z) == true);
}

// 0x8A - TXA
TEST_CASE("0x8A TXA", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x77;
    load_instruction(cpu, {INSTRUCTION_TXA_IMP});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x77);
}

// 0x8C - STY ABS
TEST_CASE("0x8C STY ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0xCD;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    load_instruction(cpu, {INSTRUCTION_STY_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x0500] == 0xCD);
}

// 0x8D - STA ABS
TEST_CASE("0x8D STA ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xAB;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    load_instruction(cpu, {INSTRUCTION_STA_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x0500] == 0xAB);
}

// 0x8E - STX ABS
TEST_CASE("0x8E STX ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0xEF;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    load_instruction(cpu, {INSTRUCTION_STX_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x0500] == 0xEF);
}

// 0x90 - BCC // FAILING
TEST_CASE("0x90 BCC", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->PC = 0x0600;
    set_flag(cpu, C, false);
    byte_t branch_offset = 0x08;
    load_instruction(cpu, {INSTRUCTION_BCC_REL, branch_offset});
    run_instruction(cpu);
    REQUIRE(cpu->PC == 0x0600 + 2 + branch_offset);
}

// 0x91 - STA IZY
TEST_CASE("0x91 STA IZY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xAB;
    cpu->Y = 0;
    byte_t zero_page_addr = 0x30;
    byte_t pointer_low_byte = 0x00;
    byte_t pointer_high_byte = 0x03;
    cpu->memory[0x30] = pointer_low_byte;
    cpu->memory[0x31] = pointer_high_byte;
    load_instruction(cpu, {INSTRUCTION_STA_IZY, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x0300] == 0xAB);
}

// 0x94 - STY ZPX
TEST_CASE("0x94 STY ZPX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0xCD;
    cpu->X = 0x02;
    byte_t zero_page_addr = 0x40;
    load_instruction(cpu, {INSTRUCTION_STY_ZPX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x42] == 0xCD);
}

// 0x95 - STA ZPX
TEST_CASE("0x95 STA ZPX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xAB;
    cpu->X = 0x02;
    byte_t zero_page_addr = 0x40;
    load_instruction(cpu, {INSTRUCTION_STA_ZPX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x42] == 0xAB);
}

// 0x96 - STX ZPY
TEST_CASE("0x96 STX ZPY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0xEF;
    cpu->Y = 0x02;
    byte_t zero_page_addr = 0x40;
    load_instruction(cpu, {INSTRUCTION_STX_ZPY, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x42] == 0xEF);
}

// 0x98 - TYA
TEST_CASE("0x98 TYA", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0x88;
    load_instruction(cpu, {INSTRUCTION_TYA_IMP});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x88);
}

// 0x99 - STA ABY
TEST_CASE("0x99 STA ABY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xAB;
    cpu->Y = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    load_instruction(cpu, {INSTRUCTION_STA_ABY, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x0500] == 0xAB);
}

// 0x9A - TXS
TEST_CASE("0x9A TXS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x50;
    load_instruction(cpu, {INSTRUCTION_TXS_IMP});
    run_instruction(cpu);
    REQUIRE(cpu->SP == 0x50);
}

// 0x9D - STA ABX
TEST_CASE("0x9D STA ABX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xAB;
    cpu->X = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    load_instruction(cpu, {INSTRUCTION_STA_ABX, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x0500] == 0xAB);
}

// 0xA0 - LDY IMM
TEST_CASE("0xA0 LDY IMM", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t immediate_value = 0x42;
    load_instruction(cpu, {INSTRUCTION_LDY_IMM, immediate_value});
    run_instruction(cpu);
    REQUIRE(cpu->Y == 0x42);
}

// 0xA1 - LDA IZX
TEST_CASE("0xA1 LDA IZX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0;
    byte_t zero_page_addr = 0x10;
    byte_t pointer_low_byte = 0x00;
    byte_t pointer_high_byte = 0x03;
    cpu->memory[0x10] = pointer_low_byte;
    cpu->memory[0x11] = pointer_high_byte;
    cpu->memory[0x0300] = 0x99;
    load_instruction(cpu, {INSTRUCTION_LDA_IZX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x99);
}

// 0xA2 - LDX IMM
TEST_CASE("0xA2 LDX IMM", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t immediate_value = 0x42;
    load_instruction(cpu, {INSTRUCTION_LDX_IMM, immediate_value});
    run_instruction(cpu);
    REQUIRE(cpu->X == 0x42);
}

// 0xA4 - LDY ZP0
TEST_CASE("0xA4 LDY ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t zero_page_addr = 0x20;
    cpu->memory[0x20] = 0x42;
    load_instruction(cpu, {INSTRUCTION_LDY_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->Y == 0x42);
}

// 0xA5 - LDA ZP0
TEST_CASE("0xA5 LDA ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t zero_page_addr = 0x20;
    cpu->memory[0x20] = 0x99;
    load_instruction(cpu, {INSTRUCTION_LDA_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x99);
}

// 0xA6 - LDX ZP0
TEST_CASE("0xA6 LDX ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t zero_page_addr = 0x20;
    cpu->memory[0x20] = 0x42;
    load_instruction(cpu, {INSTRUCTION_LDX_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->X == 0x42);
}

// 0xA8 - TAY
TEST_CASE("0xA8 TAY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x66;
    load_instruction(cpu, {INSTRUCTION_TAY_IMP});
    run_instruction(cpu);
    REQUIRE(cpu->Y == 0x66);
}

// 0xA9 - LDA IMM
TEST_CASE("0xA9 LDA IMM", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t immediate_value = 0x99;
    load_instruction(cpu, {INSTRUCTION_LDA_IMM, immediate_value});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x99);
}

// 0xAA - TAX
TEST_CASE("0xAA TAX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x66;
    load_instruction(cpu, {INSTRUCTION_TAX_IMP});
    run_instruction(cpu);
    REQUIRE(cpu->X == 0x66);
}

// 0xAC - LDY ABS
TEST_CASE("0xAC LDY ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x42;
    load_instruction(cpu, {INSTRUCTION_LDY_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->Y == 0x42);
}

// 0xAD - LDA ABS
TEST_CASE("0xAD LDA ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x99;
    load_instruction(cpu, {INSTRUCTION_LDA_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x99);
}

// 0xAE - LDX ABS
TEST_CASE("0xAE LDX ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x42;
    load_instruction(cpu, {INSTRUCTION_LDX_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->X == 0x42);
}

// 0xB0 - BCS // FAILING
TEST_CASE("0xB0 BCS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->PC = 0x0700;
    set_flag(cpu, C, true);
    byte_t branch_offset = 0x0A;
    load_instruction(cpu, {INSTRUCTION_BCS_REL, branch_offset});
    run_instruction(cpu);
    REQUIRE(cpu->PC == 0x0700 + 2 + branch_offset);
}

// 0xB1 - LDA IZY
TEST_CASE("0xB1 LDA IZY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0;
    byte_t zero_page_addr = 0x30;
    byte_t pointer_low_byte = 0x00;
    byte_t pointer_high_byte = 0x03;
    cpu->memory[0x30] = pointer_low_byte;
    cpu->memory[0x31] = pointer_high_byte;
    cpu->memory[0x0300] = 0x99;
    load_instruction(cpu, {INSTRUCTION_LDA_IZY, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x99);
}

// 0xB4 - LDY ZPX
TEST_CASE("0xB4 LDY ZPX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x02;
    byte_t zero_page_addr = 0x40;
    cpu->memory[0x42] = 0x42;
    load_instruction(cpu, {INSTRUCTION_LDY_ZPX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->Y == 0x42);
}

// 0xB5 - LDA ZPX
TEST_CASE("0xB5 LDA ZPX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x02;
    byte_t zero_page_addr = 0x40;
    cpu->memory[0x42] = 0x99;
    load_instruction(cpu, {INSTRUCTION_LDA_ZPX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x99);
}

// 0xB6 - LDX ZPY
TEST_CASE("0xB6 LDX ZPY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0x02;
    byte_t zero_page_addr = 0x40;
    cpu->memory[0x42] = 0x42;
    load_instruction(cpu, {INSTRUCTION_LDX_ZPY, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->X == 0x42);
}

// 0xB8 - CLV (Clear Overflow Flag)
TEST_CASE("0xB8 CLV", "[cpu][flags]") {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, V, true);
    REQUIRE(get_flag(cpu, V) == true);

    load_instruction(cpu, {INSTRUCTION_CLV_IMP});
    run_instruction(cpu);

    REQUIRE(get_flag(cpu, V) == false);
}

// 0xB9 - LDA ABY
TEST_CASE("0xB9 LDA ABY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x99;
    load_instruction(cpu, {INSTRUCTION_LDA_ABY, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x99);
}

// 0xBA - TSX
TEST_CASE("0xBA TSX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->SP = 0x60;
    load_instruction(cpu, {INSTRUCTION_TSX_IMP});
    run_instruction(cpu);
    REQUIRE(cpu->X == 0x60);
}

// 0xBC - LDY ABX
TEST_CASE("0xBC LDY ABX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x42;
    load_instruction(cpu, {INSTRUCTION_LDY_ABX, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->Y == 0x42);
}

// 0xBD - LDA ABX
TEST_CASE("0xBD LDA ABX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x99;
    load_instruction(cpu, {INSTRUCTION_LDA_ABX, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x99);
}

// 0xBE - LDX ABY
TEST_CASE("0xBE LDX ABY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x42;
    load_instruction(cpu, {INSTRUCTION_LDX_ABY, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->X == 0x42);
}

// 0xC0 - CPY IMM
TEST_CASE("0xC0 CPY IMM", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0x05;
    byte_t immediate_value = 0x03;
    load_instruction(cpu, {INSTRUCTION_CPY_IMM, immediate_value});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, C) == true);
    REQUIRE(get_flag(cpu, Z) == false);
}

// 0xC1 - CMP IZX
TEST_CASE("0xC1 CMP IZX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    cpu->X = 0;
    byte_t zero_page_addr = 0x10;
    byte_t pointer_low_byte = 0x00;
    byte_t pointer_high_byte = 0x03;
    cpu->memory[0x10] = pointer_low_byte;
    cpu->memory[0x11] = pointer_high_byte;
    cpu->memory[0x0300] = 0x03;
    load_instruction(cpu, {INSTRUCTION_CMP_IZX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, C) == true);
    REQUIRE(get_flag(cpu, Z) == false);
}

// 0xC4 - CPY ZP0
TEST_CASE("0xC4 CPY ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0x05;
    byte_t zero_page_addr = 0x20;
    cpu->memory[0x20] = 0x05;
    load_instruction(cpu, {INSTRUCTION_CPY_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, C) == true);
    REQUIRE(get_flag(cpu, Z) == true);
}

// 0xC5 - CMP ZP0
TEST_CASE("0xC5 CMP ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    byte_t zero_page_addr = 0x20;
    cpu->memory[0x20] = 0x05;
    load_instruction(cpu, {INSTRUCTION_CMP_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, C) == true);
    REQUIRE(get_flag(cpu, Z) == true);
}

// 0xC6 - DEC ZP0
TEST_CASE("0xC6 DEC ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t zero_page_addr = 0x30;
    cpu->memory[0x30] = 0x01;
    load_instruction(cpu, {INSTRUCTION_DEC_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x30] == 0x00);
    REQUIRE(get_flag(cpu, Z) == true);
}

// 0xC8 - INY
TEST_CASE("0xC8 INY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0x00;
    load_instruction(cpu, {INSTRUCTION_INY_IMP});
    run_instruction(cpu);
    REQUIRE(cpu->Y == 0x01);
}

// 0xC9 - CMP IMM
TEST_CASE("0xC9 CMP IMM", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0A;
    byte_t immediate_value = 0x05;
    load_instruction(cpu, {INSTRUCTION_CMP_IMM, immediate_value});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, C) == true);
    REQUIRE(get_flag(cpu, Z) == false);
}

// 0xCA - DEX
TEST_CASE("0xCA DEX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x01;
    load_instruction(cpu, {INSTRUCTION_DEX_IMP});
    run_instruction(cpu);
    REQUIRE(cpu->X == 0x00);
    REQUIRE(get_flag(cpu, Z) == true);
}

// 0xCC - CPY ABS
TEST_CASE("0xCC CPY ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0x0A;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x05;
    load_instruction(cpu, {INSTRUCTION_CPY_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, C) == true);
}

// 0xCD - CMP ABS
TEST_CASE("0xCD CMP ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0A;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x05;
    load_instruction(cpu, {INSTRUCTION_CMP_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, C) == true);
}

// 0xCE - DEC ABS
TEST_CASE("0xCE DEC ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x02;
    load_instruction(cpu, {INSTRUCTION_DEC_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x0500] == 0x01);
}

// 0xD0 - BNE // FAILING
TEST_CASE("0xD0 BNE", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->PC = 0x0800;
    set_flag(cpu, Z, false);
    byte_t branch_offset = 0x06;
    load_instruction(cpu, {INSTRUCTION_BNE_REL, branch_offset});
    run_instruction(cpu);
    REQUIRE(cpu->PC == 0x0800 + 2 + branch_offset);
}

// 0xD1 - CMP IZY
TEST_CASE("0xD1 CMP IZY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0A;
    cpu->Y = 0;
    byte_t zero_page_addr = 0x30;
    byte_t pointer_low_byte = 0x00;
    byte_t pointer_high_byte = 0x03;
    cpu->memory[0x30] = pointer_low_byte;
    cpu->memory[0x31] = pointer_high_byte;
    cpu->memory[0x0300] = 0x05;
    load_instruction(cpu, {INSTRUCTION_CMP_IZY, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, C) == true);
}

// 0xD5 - CMP ZPX
TEST_CASE("0xD5 CMP ZPX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0A;
    cpu->X = 0x01;
    byte_t zero_page_addr = 0x40;
    cpu->memory[0x41] = 0x05;
    load_instruction(cpu, {INSTRUCTION_CMP_ZPX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, C) == true);
}

// 0xD6 - DEC ZPX
TEST_CASE("0xD6 DEC ZPX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x01;
    byte_t zero_page_addr = 0x50;
    cpu->memory[0x51] = 0x02;
    load_instruction(cpu, {INSTRUCTION_DEC_ZPX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x51] == 0x01);
}

// 0xD8 - CLD (Clear Decimal Flag)
TEST_CASE("0xD8 CLD", "[cpu][flags]") {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, D, true);
    REQUIRE(get_flag(cpu, D) == true);

    load_instruction(cpu, {INSTRUCTION_CLD_IMP});
    run_instruction(cpu);

    REQUIRE(get_flag(cpu, D) == false);
}

// 0xD9 - CMP ABY
TEST_CASE("0xD9 CMP ABY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0A;
    cpu->Y = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x05;
    load_instruction(cpu, {INSTRUCTION_CMP_ABY, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, C) == true);
}

// 0xDD - CMP ABX
TEST_CASE("0xDD CMP ABX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0A;
    cpu->X = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x05;
    load_instruction(cpu, {INSTRUCTION_CMP_ABX, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, C) == true);
}

// 0xDE - DEC ABX
TEST_CASE("0xDE DEC ABX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x02;
    load_instruction(cpu, {INSTRUCTION_DEC_ABX, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x0500] == 0x01);
}

// 0xE0 - CPX IMM
TEST_CASE("0xE0 CPX IMM", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x05;
    byte_t immediate_value = 0x03;
    load_instruction(cpu, {INSTRUCTION_CPX_IMM, immediate_value});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, C) == true);
}

// 0xE1 - SBC IZX
TEST_CASE("0xE1 SBC IZX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    set_flag(cpu, C, true);
    cpu->X = 0;
    byte_t zero_page_addr = 0x10;
    byte_t pointer_low_byte = 0x00;
    byte_t pointer_high_byte = 0x03;
    cpu->memory[0x10] = pointer_low_byte;
    cpu->memory[0x11] = pointer_high_byte;
    cpu->memory[0x0300] = 0x02;
    load_instruction(cpu, {INSTRUCTION_SBC_IZX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x03);
}

// 0xE4 - CPX ZP0
TEST_CASE("0xE4 CPX ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x05;
    byte_t zero_page_addr = 0x20;
    cpu->memory[0x20] = 0x05;
    load_instruction(cpu, {INSTRUCTION_CPX_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, Z) == true);
}

// 0xE5 - SBC ZP0
TEST_CASE("0xE5 SBC ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    set_flag(cpu, C, true);
    byte_t zero_page_addr = 0x20;
    cpu->memory[0x20] = 0x02;
    load_instruction(cpu, {INSTRUCTION_SBC_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x03);
}

// 0xE6 - INC ZP0
TEST_CASE("0xE6 INC ZP0", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t zero_page_addr = 0x30;
    cpu->memory[0x30] = 0x01;
    load_instruction(cpu, {INSTRUCTION_INC_ZP0, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x30] == 0x02);
}

// 0xE8 - INX
TEST_CASE("0xE8 INX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x00;
    load_instruction(cpu, {INSTRUCTION_INX_IMP});
    run_instruction(cpu);
    REQUIRE(cpu->X == 0x01);
}

// 0xE9 - SBC IMM
TEST_CASE("0xE9 SBC IMM", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    set_flag(cpu, C, true);
    byte_t immediate_value = 0x02;
    load_instruction(cpu, {INSTRUCTION_SBC_IMM, immediate_value});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x03);
}

// 0xEA - NOP
TEST_CASE("0xEA NOP", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x42;
    load_instruction(cpu, {INSTRUCTION_NOP_IMP});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x42);
}

// 0xEC - CPX ABS
TEST_CASE("0xEC CPX ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x0A;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x05;
    load_instruction(cpu, {INSTRUCTION_CPX_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(get_flag(cpu, C) == true);
}

// 0xED - SBC ABS
TEST_CASE("0xED SBC ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    set_flag(cpu, C, true);
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x02;
    load_instruction(cpu, {INSTRUCTION_SBC_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x03);
}

// 0xEE - INC ABS
TEST_CASE("0xEE INC ABS", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x01;
    load_instruction(cpu, {INSTRUCTION_INC_ABS, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x0500] == 0x02);
}

// 0xF0 - BEQ // FAILING
TEST_CASE("0xF0 BEQ", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->PC = 0x0900;
    set_flag(cpu, Z, true);
    byte_t branch_offset = 0x04;
    load_instruction(cpu, {INSTRUCTION_BEQ_REL, branch_offset});
    run_instruction(cpu);
    REQUIRE(cpu->PC == 0x0900 + 2 + branch_offset);
}

// 0xF1 - SBC IZY
TEST_CASE("0xF1 SBC IZY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    set_flag(cpu, C, true);
    cpu->Y = 0;
    byte_t zero_page_addr = 0x30;
    byte_t pointer_low_byte = 0x00;
    byte_t pointer_high_byte = 0x03;
    cpu->memory[0x30] = pointer_low_byte;
    cpu->memory[0x31] = pointer_high_byte;
    cpu->memory[0x0300] = 0x02;
    load_instruction(cpu, {INSTRUCTION_SBC_IZY, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x03);
}

// 0xF5 - SBC ZPX
TEST_CASE("0xF5 SBC ZPX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    set_flag(cpu, C, true);
    cpu->X = 0x01;
    byte_t zero_page_addr = 0x40;
    cpu->memory[0x41] = 0x02;
    load_instruction(cpu, {INSTRUCTION_SBC_ZPX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x03);
}

// 0xF6 - INC ZPX
TEST_CASE("0xF6 INC ZPX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x01;
    byte_t zero_page_addr = 0x50;
    cpu->memory[0x51] = 0x01;
    load_instruction(cpu, {INSTRUCTION_INC_ZPX, zero_page_addr});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x51] == 0x02);
}

// 0xF8 - SED (Set Decimal Flag)
TEST_CASE("0xF8 SED", "[cpu][flags]") {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, D, false);
    REQUIRE(get_flag(cpu, D) == false);

    load_instruction(cpu, {INSTRUCTION_SED_IMP});
    run_instruction(cpu);

    REQUIRE(get_flag(cpu, D) == true);
}

// 0xF9 - SBC ABY
TEST_CASE("0xF9 SBC ABY", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    set_flag(cpu, C, true);
    cpu->Y = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x02;
    load_instruction(cpu, {INSTRUCTION_SBC_ABY, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x03);
}

// 0xFD - SBC ABX
TEST_CASE("0xFD SBC ABX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    set_flag(cpu, C, true);
    cpu->X = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x02;
    load_instruction(cpu, {INSTRUCTION_SBC_ABX, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->A == 0x03);
}

// 0xFE - INC ABX
TEST_CASE("0xFE INC ABX", "[cpu]") {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0;
    byte_t address_low_byte = 0x00;
    byte_t address_high_byte = 0x05;
    cpu->memory[0x0500] = 0x01;
    load_instruction(cpu, {INSTRUCTION_INC_ABX, address_low_byte, address_high_byte});
    run_instruction(cpu);
    REQUIRE(cpu->memory[0x0500] == 0x02);
}
