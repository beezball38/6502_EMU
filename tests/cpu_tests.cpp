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
    cpu->current_instruction = &cpu->table[op_code];
    assert(cpu->current_instruction->length == instruction.size());
    byte_t pc = cpu->PC;
    for(byte_t byte:instruction)
    {
        cpu->memory[pc++] = byte;
    }
}

void run_instruction(cpu_s *cpu)
{
        if(cpu->current_instruction->fetch != NULL)
        {
            cpu->current_instruction->fetch(cpu);
        }

        if(cpu->current_instruction->execute != NULL)
        {
            cpu->current_instruction->execute(cpu);
        }
}

void load_interrupt_vector(cpu_s *cpu, std::vector<byte_t> irq_bytes)
{
    const int IRQ_SIZE = 2;
    assert(irq_bytes.size() == IRQ_SIZE);

    byte_t low_byte = irq_bytes.at(0);
    byte_t high_byte = irq_bytes.at(1);
    cpu->memory[0xFFFE] = low_byte;
    cpu->memory[0xFFFF] = high_byte;
}

// =============================================================================
// Tests ordered by opcode (ascending)
// =============================================================================

// 0x18 - CLC (Clear Carry Flag)
TEST_CASE("0x18 CLC", "[cpu][flags]") {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, C, true);
    REQUIRE(get_flag(cpu, C) == true);

    load_instruction(cpu, {INSTRUCTION_CLC_IMP});
    run_instruction(cpu);

    REQUIRE(get_flag(cpu, C) == false);
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

// 0x58 - CLI (Clear Interrupt Disable Flag)
TEST_CASE("0x58 CLI", "[cpu][flags]") {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, I, true);
    REQUIRE(get_flag(cpu, I) == true);

    load_instruction(cpu, {INSTRUCTION_CLI_IMP});
    run_instruction(cpu);

    REQUIRE(get_flag(cpu, I) == false);
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

// 0xB8 - CLV (Clear Overflow Flag)
TEST_CASE("0xB8 CLV", "[cpu][flags]") {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, V, true);
    REQUIRE(get_flag(cpu, V) == true);

    load_instruction(cpu, {INSTRUCTION_CLV_IMP});
    run_instruction(cpu);

    REQUIRE(get_flag(cpu, V) == false);
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

// 0xF8 - SED (Set Decimal Flag)
TEST_CASE("0xF8 SED", "[cpu][flags]") {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, D, false);
    REQUIRE(get_flag(cpu, D) == false);

    load_instruction(cpu, {INSTRUCTION_SED_IMP});
    run_instruction(cpu);

    REQUIRE(get_flag(cpu, D) == true);
}
