// tests/cpu_tests.cpp
#include <catch2/catch_test_macros.hpp>
#include "cpu.hpp"  // Your CPU header

TEST_CASE("Sanity Check: CPU Initial State", "[cpu]") {
    cpu_s cpu;  // Create CPU instance
    cpu.A = 42;
    // Minimal check for accumulator initialization
    REQUIRE(cpu.A == 0x00);
}
