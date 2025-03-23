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
    cpu->current_instruction = &cpu->table[instruction.at(0)];
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

TEST_CASE("BRK IMP should push PC+2 and followed by the status register", "[cpu]") {
    SECTION("BRK sets Interupt flag; pushes PC+2 along with STATUS register; sets PC to IRQ vector") {
        cpu_s *cpu = get_test_cpu();
        cpu->PC = 0x1000;
        load_instruction(cpu, {0x00, 0x00});

        byte_t low_byte = 0x76;
        byte_t high_byte  = 0x89;
        word_t expected_addr = ((word_t)high_byte << 8 | (word_t) low_byte);
        load_interrupt_vector(cpu, {low_byte, high_byte});
        byte_t saved_status_reg = (cpu->STATUS | B);
        word_t saved_pc_plus_two = cpu->PC + 2;
        REQUIRE_FALSE(saved_status_reg == 0);
        run_instruction(cpu);
        REQUIRE_FALSE(saved_status_reg == 0);

        REQUIRE(cpu->PC == expected_addr);
        REQUIRE(get_flag(cpu, I));
        //the status flag after execution should have B unset, but the saved register
        //needs the B flag set
        REQUIRE(get_flag(cpu, B) == false);
        byte_t popped_byte = pop_byte(cpu);
        REQUIRE(popped_byte == saved_status_reg);
        popped_byte = pop_byte(cpu);
        REQUIRE(popped_byte == saved_pc_plus_two);
    }
}
