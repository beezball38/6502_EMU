// tests/cpu_tests.cpp
#include <catch2/catch_test_macros.hpp>
#include "../src/cpu.hpp"  // Your CPU header
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


TEST_CASE("BRK IMP should push PC+2 and followed by the status register", "[cpu]") {
    SECTION("BRK sets break flag, pushes PC+2 and status to stack, and sets PC to IRQ vector") {
        cpu_s *cpu = get_test_cpu();
        cpu->PC = 0x1000;
        cpu->memory[cpu->PC] = 0x00; //OpCode
    }
}
