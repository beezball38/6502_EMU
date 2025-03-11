// tests/cpu_tests.cpp
#include <cassert>
#include <catch2/catch_test_macros.hpp>
#include "../src/cpu.hpp"
#include <vector>
// singletons for tests, will only be accessed via get_test_cpu
static cpu_s cpu;
static byte_t memory[1024*1024*64];

cpu_s *get_test_cpu();

cpu_s *get_test_cpu()
{
    cpu_init(&cpu, memory);
    init_instruction_table(&cpu);
    return &cpu;
}

void load_instruction(cpu_s *cpu, std::vector<byte_t> instruction)
{
    assert(instruction.size() > 0);
    cpu->current_instruction = &cpu->table[instruction.at(0)];
    printf("%d, %d\n", cpu->current_instruction->length, instruction.size());
    assert(cpu->current_instruction->length == instruction.size());
    byte_t pc = cpu->PC;
    for(byte_t byte:instruction)
    {
        cpu->memory[pc++] = byte;
    }
}

TEST_CASE("BRK IMP should push PC+2 and followed by the status register", "[cpu]") {
    SECTION("BRK sets break flag, pushes PC+2 and status to stack, and sets PC to IRQ vector") {
        cpu_s *cpu = get_test_cpu();
        cpu->PC = 0x1000;
        load_instruction(cpu, {0x00, 0x00});
        if(cpu->current_instruction->fetch != NULL)
        {
            cpu->current_instruction->fetch(cpu);
        }

        if(cpu->current_instruction->execute != NULL)
        {
            cpu->current_instruction->execute(cpu);
        }

    }
}
