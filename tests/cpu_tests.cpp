// tests/cpu_tests.cpp
#include <catch2/catch_test_macros.hpp>
#include "../src/cpu.hpp"  // Your CPU header

cpu_s cpu;
byte_t memory[1024*1024*64];

TEST_CASE("Sanity Check: CPU Initial State", "[cpu]") {
    cpu.A = 1;
    cpu_init(&cpu, memory);
    // Minimal check for accumulator initialization
    REQUIRE(cpu.A == 0x00);
}
