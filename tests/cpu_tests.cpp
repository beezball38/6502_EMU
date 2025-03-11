// tests/cpu_tests.cpp
#include <cassert>
#include <catch2/catch_test_macros.hpp>
#include "../src/cpu.hpp"
#include <cstring>
#include <vector>
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

TEST_CASE("BRK IMP should push PC+2 and followed by the status register", "[cpu]") {
    SECTION("BRK sets break flag, pushes PC+2 and status to stack, and sets PC to IRQ vector") {
        cpu_s *cpu = get_test_cpu();
        cpu->PC = 0x1000;
        load_instruction(cpu, {0x00, 0x00});
        run_instruction(cpu);
        REQUIRE(cpu->PC == 0); //stub, put in IRQ
    }
}
