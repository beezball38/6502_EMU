// tests/cpu_tests.cpp
#include <catch2/catch_test_macros.hpp>
#include "../src/cpu.hpp"  // Your CPU header

cpu_s cpu;
byte_t memory[1024*1024*64];


TEST_CASE("BRK should push PC+2 and followed by the status register", "[cpu]") {
    SECTION("BRK sets break flag, pushes PC+2 and status to stack, and sets PC to IRQ vector") {
        // Set up CPU state before executing BRK
        cpu_init(&cpu, memory);
        init_instruction_table(&cpu);
        //stub write test
    }
}
