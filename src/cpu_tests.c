// tests/cpu_tests.c
#include <assert.h>
#include <string.h>
#include "unity.h"
#include "cpu.h"
#include "bus.h"

#define MEM_SIZE (64 * 1024)
#define BRANCH_INSTR_LEN 0x02

// Singletons for tests, will only be accessed via get_test_cpu
static cpu_s cpu;
static bus_s bus;

// Helper to access bus RAM directly for test setup/verification
// Note: For addresses >= 0x8000, we need to use PRG ROM area
// For tests, we'll use a static PRG ROM buffer
static byte_t test_prg_rom[32 * 1024];  // 32KB PRG ROM for tests

// Unified memory access for tests - routes through bus
static inline byte_t* test_mem_ptr(word_t addr) {
    if (addr < 0x2000) {
        // Internal RAM (mirrored)
        return &bus.ram[addr & 0x07FF];
    } else if (addr >= 0x8000) {
        // PRG ROM area - use our test buffer
        return &test_prg_rom[addr - 0x8000];
    }
    // For other addresses, just use RAM (tests shouldn't need PPU/APU regions)
    return &bus.ram[addr & 0x07FF];
}

cpu_s *get_test_cpu(void);
void load_instruction(cpu_s *cpu, const byte_t *instruction, size_t len);
static void execute_instruction(cpu_s *cpu);
void load_interrupt_vector(cpu_s *cpu, byte_t irq_vector_low, byte_t irq_vector_high);

void setUp(void) {
    // Called before each test
}

void tearDown(void) {
    // Called after each test
}

cpu_s *get_test_cpu(void) {
    bus_init(&bus);
    memset(test_prg_rom, 0, sizeof(test_prg_rom));
    bus_load_prg_rom(&bus, test_prg_rom, sizeof(test_prg_rom));
    cpu_init(&cpu, &bus);
    return &cpu;
}

void load_instruction(cpu_s *cpu, const byte_t *instruction, size_t len) {
    assert(len > 0);
    const byte_t op_code = instruction[0];
    cpu->current_opcode = op_code;
    cpu->instruction_pending = true;
    cpu_instruction_s *current_instruction = get_current_instruction(cpu);
    assert(current_instruction->length == len);
    word_t pc = cpu->PC;
    for (size_t i = 0; i < len; i++) {
        *test_mem_ptr(pc++) = instruction[i];
    }
}

// Execute an instruction that was loaded with load_instruction
// This is a simplified version for testing - does not advance PC
static void execute_instruction(cpu_s *cpu) {
    cpu_instruction_s *current_instruction = get_current_instruction(cpu);
    if (current_instruction->data_fetch != NULL) {
        current_instruction->data_fetch(cpu);
    }
    if (current_instruction->execute != NULL) {
        current_instruction->execute(cpu);
    }
}

void load_interrupt_vector(cpu_s *cpu, byte_t irq_vector_low, byte_t irq_vector_high) {
    *test_mem_ptr(0xFFFE) = irq_vector_low;
    *test_mem_ptr(0xFFFF) = irq_vector_high;
}

// =============================================================================
// Tests ordered by opcode (ascending)
// =============================================================================

// 0x00 - BRK (Force Interrupt)
void test_0x00_BRK(void) {
    cpu_s *cpu = get_test_cpu();
    word_t start_pc = 0x0200;
    word_t irq_vector = 0x1234;
    word_t pushed_pc = start_pc + 0x01; // BRK pushes PC+1 (after padding byte)

    cpu->PC = start_pc;
    byte_t instr[] = {INSTRUCTION_BRK_IMP, 0x00};
    load_instruction(cpu, instr, sizeof(instr));
    load_interrupt_vector(cpu, irq_vector & 0xFF, (irq_vector >> 8) & 0xFF);
    execute_instruction(cpu);

    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_I)); // I set after push
    TEST_ASSERT_EQUAL_HEX16(irq_vector, cpu->PC);
    TEST_ASSERT_EQUAL_HEX8(pushed_pc & 0xFF, *test_mem_ptr(0x01FC));
    TEST_ASSERT_EQUAL_HEX8((pushed_pc >> 8) & 0xFF, *test_mem_ptr(0x01FD));

    byte_t pushed_status = *test_mem_ptr(0x01FB);
    TEST_ASSERT_TRUE((pushed_status & STATUS_FLAG_B) != 0); // B always set in pushed status
    // Note: I flag in pushed status reflects state BEFORE BRK, not after
}

// 0x01 - ORA IZX
void test_0x01_ORA_IZX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0F;
    cpu->X = 0;
    *test_mem_ptr(0x10) = 0x00;
    *test_mem_ptr(0x11) = 0x03;
    *test_mem_ptr(0x0300) = 0xF0;
    byte_t instr[] = {INSTRUCTION_ORA_IZX, 0x10};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xFF, cpu->A);
}

// 0x05 - ORA ZP0
void test_0x05_ORA_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0F;
    *test_mem_ptr(0x20) = 0xF0;
    byte_t instr[] = {INSTRUCTION_ORA_ZP0, 0x20};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xFF, cpu->A);
}

// 0x06 - ASL ZP0
void test_0x06_ASL_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x30) = 0x40;
    byte_t instr[] = {INSTRUCTION_ASL_ZP0, 0x30};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_C));
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_Z));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_N));
}

// 0x08 - PHP
void test_0x08_PHP(void) {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, STATUS_FLAG_N, true);
    byte_t instr[] = {INSTRUCTION_PHP_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8((cpu->STATUS | STATUS_FLAG_B | STATUS_FLAG_U), *test_mem_ptr(0x01FD));
}

// 0x09 - ORA IMM
void test_0x09_ORA_IMM(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0F;
    byte_t instr[] = {INSTRUCTION_ORA_IMM, 0xF0};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xFF, cpu->A);
}

// 0x0A - ASL ACC
void test_0x0A_ASL_ACC(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x40;
    byte_t instr[] = {INSTRUCTION_ASL_ACC};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_C));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_N));
}

// 0x0D - ORA ABS
void test_0x0D_ORA_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0F;
    *test_mem_ptr(0x0400) = 0xF0;
    byte_t instr[] = {INSTRUCTION_ORA_ABS, 0x00, 0x04};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xFF, cpu->A);
}

// 0x0E - ASL ABS
void test_0x0E_ASL_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x0400) = 0x40;
    byte_t instr[] = {INSTRUCTION_ASL_ABS, 0x00, 0x04};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_C));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_N));
}

// 0x10 - BPL
void test_0x10_BPL(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->PC = 0x0200;
    set_flag(cpu, STATUS_FLAG_N, false);
    byte_t instr[] = {INSTRUCTION_BPL_REL, 0x04};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(0x0200 + BRANCH_INSTR_LEN + 0x04, cpu->PC);
}

// 0x11 - ORA IZY
void test_0x11_ORA_IZY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0F;
    cpu->Y = 0;
    *test_mem_ptr(0x20) = 0x00;
    *test_mem_ptr(0x21) = 0x03;
    *test_mem_ptr(0x0300) = 0xF0;
    byte_t instr[] = {INSTRUCTION_ORA_IZY, 0x20};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xFF, cpu->A);
}

// 0x15 - ORA ZPX
void test_0x15_ORA_ZPX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0F;
    cpu->X = 0x02;
    *test_mem_ptr(0x22) = 0xF0;
    byte_t instr[] = {INSTRUCTION_ORA_ZPX, 0x20};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xFF, cpu->A);
}

// 0x16 - ASL ZPX
void test_0x16_ASL_ZPX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x01;
    *test_mem_ptr(0x31) = 0x40;
    byte_t instr[] = {INSTRUCTION_ASL_ZPX, 0x30};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_C));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_N));
}

// 0x18 - CLC
void test_0x18_CLC(void) {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, STATUS_FLAG_C, true);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
    byte_t instr[] = {INSTRUCTION_CLC_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_C));
}

// 0x19 - ORA ABY
void test_0x19_ORA_ABY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0F;
    cpu->Y = 0;
    *test_mem_ptr(0x0400) = 0xF0;
    byte_t instr[] = {INSTRUCTION_ORA_ABY, 0x00, 0x04};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xFF, cpu->A);
}

// 0x1D - ORA ABX
void test_0x1D_ORA_ABX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0F;
    cpu->X = 0;
    *test_mem_ptr(0x0400) = 0xF0;
    byte_t instr[] = {INSTRUCTION_ORA_ABX, 0x00, 0x04};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xFF, cpu->A);
}

// 0x1E - ASL ABX
void test_0x1E_ASL_ABX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0;
    *test_mem_ptr(0x0400) = 0x40;
    byte_t instr[] = {INSTRUCTION_ASL_ABX, 0x00, 0x04};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_C));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_N));
}

// 0x20 - JSR
void test_0x20_JSR(void) {
    cpu_s *cpu = get_test_cpu();
    word_t start_pc = 0x0200;
    word_t target_addr = 0x1234;
    word_t pushed_addr = start_pc + 0x02; // JSR pushes return_address - 1

    cpu->PC = start_pc;
    byte_t instr[] = {INSTRUCTION_JSR_ABS, target_addr & 0xFF, (target_addr >> 8) & 0xFF};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(target_addr, cpu->PC);
    TEST_ASSERT_EQUAL_HEX8(pushed_addr & 0xFF, *test_mem_ptr(0x01FC));
    TEST_ASSERT_EQUAL_HEX8((pushed_addr >> 8) & 0xFF, *test_mem_ptr(0x01FD));
}

// 0x21 - AND IZX
void test_0x21_AND_IZX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->X = 0;
    *test_mem_ptr(0x10) = 0x00;
    *test_mem_ptr(0x11) = 0x03;
    *test_mem_ptr(0x0300) = 0x0F;
    byte_t instr[] = {INSTRUCTION_AND_IZX, 0x10};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x0F, cpu->A);
}

// 0x24 - BIT ZP0
void test_0x24_BIT_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    *test_mem_ptr(0x40) = 0x80;
    byte_t instr[] = {INSTRUCTION_BIT_ZP0, 0x40};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_Z));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_N));
}

// 0x25 - AND ZP0
void test_0x25_AND_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    *test_mem_ptr(0x50) = 0x0F;
    byte_t instr[] = {INSTRUCTION_AND_ZP0, 0x50};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x0F, cpu->A);
}

// 0x26 - ROL ZP0
void test_0x26_ROL_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x60) = 0x80;
    byte_t instr[] = {INSTRUCTION_ROL_ZP0, 0x60};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x00, *test_mem_ptr(0x60));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_Z));
}

// 0x28 - PLP
void test_0x28_PLP(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x01FC) = 0x80;
    cpu->SP = 0xFB;
    byte_t instr[] = {INSTRUCTION_PLP_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_N));
}

// 0x29 - AND IMM
void test_0x29_AND_IMM(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    byte_t instr[] = {INSTRUCTION_AND_IMM, 0x0F};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x0F, cpu->A);
}

// 0x2A - ROL ACC
void test_0x2A_ROL_ACC(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x80;
    byte_t instr[] = {INSTRUCTION_ROL_ACC};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x00, cpu->A);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_Z));
}

// 0x2C - BIT ABS
void test_0x2C_BIT_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x01;
    *test_mem_ptr(0x0500) = 0x40;
    byte_t instr[] = {INSTRUCTION_BIT_ABS, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_Z));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_V));
}

// 0x2D - AND ABS
void test_0x2D_AND_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    *test_mem_ptr(0x0500) = 0x0F;
    byte_t instr[] = {INSTRUCTION_AND_ABS, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x0F, cpu->A);
}

// 0x2E - ROL ABS
void test_0x2E_ROL_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x0500) = 0x80;
    byte_t instr[] = {INSTRUCTION_ROL_ABS, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x00, *test_mem_ptr(0x0500));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
}

// 0x30 - BMI
void test_0x30_BMI(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->PC = 0x0300;
    set_flag(cpu, STATUS_FLAG_N, true);
    byte_t instr[] = {INSTRUCTION_BMI_REL, 0x02};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(0x0300 + BRANCH_INSTR_LEN + 0x02, cpu->PC);
}

// 0x31 - AND IZY
void test_0x31_AND_IZY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->Y = 0;
    *test_mem_ptr(0x30) = 0x00;
    *test_mem_ptr(0x31) = 0x03;
    *test_mem_ptr(0x0300) = 0x0F;
    byte_t instr[] = {INSTRUCTION_AND_IZY, 0x30};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x0F, cpu->A);
}

// 0x35 - AND ZPX
void test_0x35_AND_ZPX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->X = 0x01;
    *test_mem_ptr(0x41) = 0x0F;
    byte_t instr[] = {INSTRUCTION_AND_ZPX, 0x40};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x0F, cpu->A);
}

// 0x36 - ROL ZPX
void test_0x36_ROL_ZPX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x01;
    *test_mem_ptr(0x51) = 0x80;
    byte_t instr[] = {INSTRUCTION_ROL_ZPX, 0x50};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x00, *test_mem_ptr(0x51));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
}

// 0x38 - SEC
void test_0x38_SEC(void) {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, STATUS_FLAG_C, false);
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_C));
    byte_t instr[] = {INSTRUCTION_SEC_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
}

// 0x39 - AND ABY
void test_0x39_AND_ABY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->Y = 0;
    *test_mem_ptr(0x0500) = 0x0F;
    byte_t instr[] = {INSTRUCTION_AND_ABY, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x0F, cpu->A);
}

// 0x3D - AND ABX
void test_0x3D_AND_ABX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->X = 0;
    *test_mem_ptr(0x0500) = 0x0F;
    byte_t instr[] = {INSTRUCTION_AND_ABX, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x0F, cpu->A);
}

// 0x3E - ROL ABX
void test_0x3E_ROL_ABX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0;
    *test_mem_ptr(0x0500) = 0x80;
    byte_t instr[] = {INSTRUCTION_ROL_ABX, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x00, *test_mem_ptr(0x0500));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
}

// 0x40 - RTI
void test_0x40_RTI(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x01FB) = 0x00;
    *test_mem_ptr(0x01FC) = 0x34;
    *test_mem_ptr(0x01FD) = 0x12;
    cpu->SP = 0xFA;
    byte_t instr[] = {INSTRUCTION_RTI_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(0x1234, cpu->PC);
}

// 0x41 - EOR IZX
void test_0x41_EOR_IZX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->X = 0;
    *test_mem_ptr(0x10) = 0x00;
    *test_mem_ptr(0x11) = 0x03;
    *test_mem_ptr(0x0300) = 0x0F;
    byte_t instr[] = {INSTRUCTION_EOR_IZX, 0x10};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xF0, cpu->A);
}

// 0x45 - EOR ZP0
void test_0x45_EOR_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    *test_mem_ptr(0x20) = 0x0F;
    byte_t instr[] = {INSTRUCTION_EOR_ZP0, 0x20};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xF0, cpu->A);
}

// 0x46 - LSR ZP0
void test_0x46_LSR_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x30) = 0x04;
    byte_t instr[] = {INSTRUCTION_LSR_ZP0, 0x30};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x02, *test_mem_ptr(0x30));
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_C));
}

// 0x48 - PHA
void test_0x48_PHA(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xAB;
    byte_t instr[] = {INSTRUCTION_PHA_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xAB, *test_mem_ptr(0x01FD));
}

// 0x49 - EOR IMM
void test_0x49_EOR_IMM(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    byte_t instr[] = {INSTRUCTION_EOR_IMM, 0x0F};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xF0, cpu->A);
}

// 0x4A - LSR ACC
void test_0x4A_LSR_ACC(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x04;
    byte_t instr[] = {INSTRUCTION_LSR_ACC};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x02, cpu->A);
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_C));
}

// 0x4C - JMP ABS
void test_0x4C_JMP_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    byte_t instr[] = {INSTRUCTION_JMP_ABS, 0x56, 0x34};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(0x3456, cpu->PC);
}

// 0x4D - EOR ABS
void test_0x4D_EOR_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    *test_mem_ptr(0x0500) = 0x0F;
    byte_t instr[] = {INSTRUCTION_EOR_ABS, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xF0, cpu->A);
}

// 0x4E - LSR ABS
void test_0x4E_LSR_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x0500) = 0x04;
    byte_t instr[] = {INSTRUCTION_LSR_ABS, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x02, *test_mem_ptr(0x0500));
}

// 0x50 - BVC
void test_0x50_BVC(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->PC = 0x0400;
    set_flag(cpu, STATUS_FLAG_V, false);
    byte_t instr[] = {INSTRUCTION_BVC_REL, 0x06};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(0x0400 + BRANCH_INSTR_LEN + 0x06, cpu->PC);
}

// 0x51 - EOR IZY
void test_0x51_EOR_IZY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->Y = 0;
    *test_mem_ptr(0x30) = 0x00;
    *test_mem_ptr(0x31) = 0x03;
    *test_mem_ptr(0x0300) = 0x0F;
    byte_t instr[] = {INSTRUCTION_EOR_IZY, 0x30};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xF0, cpu->A);
}

// 0x55 - EOR ZPX
void test_0x55_EOR_ZPX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->X = 0x01;
    *test_mem_ptr(0x41) = 0x0F;
    byte_t instr[] = {INSTRUCTION_EOR_ZPX, 0x40};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xF0, cpu->A);
}

// 0x56 - LSR ZPX
void test_0x56_LSR_ZPX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x01;
    *test_mem_ptr(0x51) = 0x04;
    byte_t instr[] = {INSTRUCTION_LSR_ZPX, 0x50};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x02, *test_mem_ptr(0x51));
}

// 0x58 - CLI
void test_0x58_CLI(void) {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, STATUS_FLAG_I, true);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_I));
    byte_t instr[] = {INSTRUCTION_CLI_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_I));
}

// 0x59 - EOR ABY
void test_0x59_EOR_ABY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->Y = 0;
    *test_mem_ptr(0x0500) = 0x0F;
    byte_t instr[] = {INSTRUCTION_EOR_ABY, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xF0, cpu->A);
}

// 0x5D - EOR ABX
void test_0x5D_EOR_ABX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xFF;
    cpu->X = 0;
    *test_mem_ptr(0x0500) = 0x0F;
    byte_t instr[] = {INSTRUCTION_EOR_ABX, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xF0, cpu->A);
}

// 0x5E - LSR ABX
void test_0x5E_LSR_ABX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0;
    *test_mem_ptr(0x0500) = 0x04;
    byte_t instr[] = {INSTRUCTION_LSR_ABX, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x02, *test_mem_ptr(0x0500));
}

// 0x60 - RTS
void test_0x60_RTS(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x01FC) = 0x02;
    *test_mem_ptr(0x01FD) = 0x03;
    cpu->SP = 0xFB;
    byte_t instr[] = {INSTRUCTION_RTS_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(0x0303, cpu->PC);
}

// 0x61 - ADC IZX
void test_0x61_ADC_IZX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x01;
    set_flag(cpu, STATUS_FLAG_C, false);
    cpu->X = 0;
    *test_mem_ptr(0x10) = 0x00;
    *test_mem_ptr(0x11) = 0x03;
    *test_mem_ptr(0x0300) = 0x02;
    byte_t instr[] = {INSTRUCTION_ADC_IZX, 0x10};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x03, cpu->A);
}

// 0x65 - ADC ZP0
void test_0x65_ADC_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x01;
    set_flag(cpu, STATUS_FLAG_C, false);
    *test_mem_ptr(0x20) = 0x02;
    byte_t instr[] = {INSTRUCTION_ADC_ZP0, 0x20};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x03, cpu->A);
}

// 0x66 - ROR ZP0
void test_0x66_ROR_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, STATUS_FLAG_C, true);
    *test_mem_ptr(0x30) = 0x00;
    byte_t instr[] = {INSTRUCTION_ROR_ZP0, 0x30};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x80, *test_mem_ptr(0x30));
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_C));
}

// 0x68 - PLA
void test_0x68_PLA(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x01FD) = 0xCD;
    cpu->SP = 0xFC;
    byte_t instr[] = {INSTRUCTION_PLA_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xCD, cpu->A);
}

// 0x69 - ADC IMM
void test_0x69_ADC_IMM(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x01;
    set_flag(cpu, STATUS_FLAG_C, false);
    byte_t instr[] = {INSTRUCTION_ADC_IMM, 0x02};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x03, cpu->A);
}

// 0x6A - ROR ACC
void test_0x6A_ROR_ACC(void) {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, STATUS_FLAG_C, true);
    cpu->A = 0x01;
    byte_t instr[] = {INSTRUCTION_ROR_ACC};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x80, cpu->A);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_N));
}

// 0x6C - JMP IND
void test_0x6C_JMP_IND(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x0010) = 0x78;
    *test_mem_ptr(0x0011) = 0x56;
    byte_t instr[] = {INSTRUCTION_JMP_IND, 0x10, 0x00};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(0x5678, cpu->PC);
}

// 0x6D - ADC ABS
void test_0x6D_ADC_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x01;
    set_flag(cpu, STATUS_FLAG_C, false);
    *test_mem_ptr(0x0500) = 0x02;
    byte_t instr[] = {INSTRUCTION_ADC_ABS, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x03, cpu->A);
}

// 0x6E - ROR ABS
void test_0x6E_ROR_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, STATUS_FLAG_C, true);
    *test_mem_ptr(0x0500) = 0x00;
    byte_t instr[] = {INSTRUCTION_ROR_ABS, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x80, *test_mem_ptr(0x0500));
}

// 0x70 - BVS
void test_0x70_BVS(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->PC = 0x0500;
    set_flag(cpu, STATUS_FLAG_V, true);
    byte_t instr[] = {INSTRUCTION_BVS_REL, 0x04};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(0x0500 + BRANCH_INSTR_LEN + 0x04, cpu->PC);
}

// 0x71 - ADC IZY
void test_0x71_ADC_IZY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x01;
    set_flag(cpu, STATUS_FLAG_C, false);
    cpu->Y = 0;
    *test_mem_ptr(0x30) = 0x00;
    *test_mem_ptr(0x31) = 0x03;
    *test_mem_ptr(0x0300) = 0x02;
    byte_t instr[] = {INSTRUCTION_ADC_IZY, 0x30};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x03, cpu->A);
}

// 0x75 - ADC ZPX
void test_0x75_ADC_ZPX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x01;
    set_flag(cpu, STATUS_FLAG_C, false);
    cpu->X = 0x01;
    *test_mem_ptr(0x41) = 0x02;
    byte_t instr[] = {INSTRUCTION_ADC_ZPX, 0x40};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x03, cpu->A);
}

// 0x76 - ROR ZPX
void test_0x76_ROR_ZPX(void) {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, STATUS_FLAG_C, true);
    cpu->X = 0x01;
    *test_mem_ptr(0x51) = 0x00;
    byte_t instr[] = {INSTRUCTION_ROR_ZPX, 0x50};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x80, *test_mem_ptr(0x51));
}

// 0x78 - SEI
void test_0x78_SEI(void) {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, STATUS_FLAG_I, false);
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_I));
    byte_t instr[] = {INSTRUCTION_SEI_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_I));
}

// 0x79 - ADC ABY
void test_0x79_ADC_ABY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x01;
    set_flag(cpu, STATUS_FLAG_C, false);
    cpu->Y = 0;
    *test_mem_ptr(0x0500) = 0x02;
    byte_t instr[] = {INSTRUCTION_ADC_ABY, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x03, cpu->A);
}

// 0x7D - ADC ABX
void test_0x7D_ADC_ABX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x01;
    set_flag(cpu, STATUS_FLAG_C, false);
    cpu->X = 0;
    *test_mem_ptr(0x0500) = 0x02;
    byte_t instr[] = {INSTRUCTION_ADC_ABX, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x03, cpu->A);
}

// 0x7E - ROR ABX
void test_0x7E_ROR_ABX(void) {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, STATUS_FLAG_C, true);
    cpu->X = 0;
    *test_mem_ptr(0x0500) = 0x00;
    byte_t instr[] = {INSTRUCTION_ROR_ABX, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x80, *test_mem_ptr(0x0500));
}

// 0x81 - STA IZX
void test_0x81_STA_IZX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xAB;
    cpu->X = 0;
    *test_mem_ptr(0x10) = 0x00;
    *test_mem_ptr(0x11) = 0x03;
    byte_t instr[] = {INSTRUCTION_STA_IZX, 0x10};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xAB, *test_mem_ptr(0x0300));
}

// 0x84 - STY ZP0
void test_0x84_STY_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0xCD;
    byte_t instr[] = {INSTRUCTION_STY_ZP0, 0x20};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xCD, *test_mem_ptr(0x20));
}

// 0x85 - STA ZP0
void test_0x85_STA_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xAB;
    byte_t instr[] = {INSTRUCTION_STA_ZP0, 0x20};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xAB, *test_mem_ptr(0x20));
}

// 0x86 - STX ZP0
void test_0x86_STX_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0xEF;
    byte_t instr[] = {INSTRUCTION_STX_ZP0, 0x20};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xEF, *test_mem_ptr(0x20));
}

// 0x88 - DEY
void test_0x88_DEY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0x01;
    byte_t instr[] = {INSTRUCTION_DEY_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x00, cpu->Y);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_Z));
}

// 0x8A - TXA
void test_0x8A_TXA(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x77;
    byte_t instr[] = {INSTRUCTION_TXA_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x77, cpu->A);
}

// 0x8C - STY ABS
void test_0x8C_STY_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0xCD;
    byte_t instr[] = {INSTRUCTION_STY_ABS, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xCD, *test_mem_ptr(0x0500));
}

// 0x8D - STA ABS
void test_0x8D_STA_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xAB;
    byte_t instr[] = {INSTRUCTION_STA_ABS, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xAB, *test_mem_ptr(0x0500));
}

// 0x8E - STX ABS
void test_0x8E_STX_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0xEF;
    byte_t instr[] = {INSTRUCTION_STX_ABS, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xEF, *test_mem_ptr(0x0500));
}

// 0x90 - BCC
void test_0x90_BCC(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->PC = 0x0600;
    set_flag(cpu, STATUS_FLAG_C, false);
    byte_t instr[] = {INSTRUCTION_BCC_REL, 0x08};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(0x0600 + BRANCH_INSTR_LEN + 0x08, cpu->PC);
}

// 0x91 - STA IZY
void test_0x91_STA_IZY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xAB;
    cpu->Y = 0;
    *test_mem_ptr(0x30) = 0x00;
    *test_mem_ptr(0x31) = 0x03;
    byte_t instr[] = {INSTRUCTION_STA_IZY, 0x30};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xAB, *test_mem_ptr(0x0300));
}

// 0x94 - STY ZPX
void test_0x94_STY_ZPX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0xCD;
    cpu->X = 0x02;
    byte_t instr[] = {INSTRUCTION_STY_ZPX, 0x40};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xCD, *test_mem_ptr(0x42));
}

// 0x95 - STA ZPX
void test_0x95_STA_ZPX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xAB;
    cpu->X = 0x02;
    byte_t instr[] = {INSTRUCTION_STA_ZPX, 0x40};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xAB, *test_mem_ptr(0x42));
}

// 0x96 - STX ZPY
void test_0x96_STX_ZPY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0xEF;
    cpu->Y = 0x02;
    byte_t instr[] = {INSTRUCTION_STX_ZPY, 0x40};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xEF, *test_mem_ptr(0x42));
}

// 0x98 - TYA
void test_0x98_TYA(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0x88;
    byte_t instr[] = {INSTRUCTION_TYA_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x88, cpu->A);
}

// 0x99 - STA ABY
void test_0x99_STA_ABY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xAB;
    cpu->Y = 0;
    byte_t instr[] = {INSTRUCTION_STA_ABY, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xAB, *test_mem_ptr(0x0500));
}

// 0x9A - TXS
void test_0x9A_TXS(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x50;
    byte_t instr[] = {INSTRUCTION_TXS_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x50, cpu->SP);
}

// 0x9D - STA ABX
void test_0x9D_STA_ABX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0xAB;
    cpu->X = 0;
    byte_t instr[] = {INSTRUCTION_STA_ABX, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0xAB, *test_mem_ptr(0x0500));
}

// 0xA0 - LDY IMM
void test_0xA0_LDY_IMM(void) {
    cpu_s *cpu = get_test_cpu();
    byte_t instr[] = {INSTRUCTION_LDY_IMM, 0x42};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x42, cpu->Y);
}

// 0xA1 - LDA IZX
void test_0xA1_LDA_IZX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0;
    *test_mem_ptr(0x10) = 0x00;
    *test_mem_ptr(0x11) = 0x03;
    *test_mem_ptr(0x0300) = 0x99;
    byte_t instr[] = {INSTRUCTION_LDA_IZX, 0x10};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x99, cpu->A);
}

// 0xA2 - LDX IMM
void test_0xA2_LDX_IMM(void) {
    cpu_s *cpu = get_test_cpu();
    byte_t instr[] = {INSTRUCTION_LDX_IMM, 0x42};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x42, cpu->X);
}

// 0xA4 - LDY ZP0
void test_0xA4_LDY_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x20) = 0x42;
    byte_t instr[] = {INSTRUCTION_LDY_ZP0, 0x20};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x42, cpu->Y);
}

// 0xA5 - LDA ZP0
void test_0xA5_LDA_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x20) = 0x99;
    byte_t instr[] = {INSTRUCTION_LDA_ZP0, 0x20};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x99, cpu->A);
}

// 0xA6 - LDX ZP0
void test_0xA6_LDX_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x20) = 0x42;
    byte_t instr[] = {INSTRUCTION_LDX_ZP0, 0x20};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x42, cpu->X);
}

// 0xA8 - TAY
void test_0xA8_TAY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x66;
    byte_t instr[] = {INSTRUCTION_TAY_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x66, cpu->Y);
}

// 0xA9 - LDA IMM
void test_0xA9_LDA_IMM(void) {
    cpu_s *cpu = get_test_cpu();
    byte_t instr[] = {INSTRUCTION_LDA_IMM, 0x99};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x99, cpu->A);
}

// 0xAA - TAX
void test_0xAA_TAX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x66;
    byte_t instr[] = {INSTRUCTION_TAX_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x66, cpu->X);
}

// 0xAC - LDY ABS
void test_0xAC_LDY_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x0500) = 0x42;
    byte_t instr[] = {INSTRUCTION_LDY_ABS, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x42, cpu->Y);
}

// 0xAD - LDA ABS
void test_0xAD_LDA_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x0500) = 0x99;
    byte_t instr[] = {INSTRUCTION_LDA_ABS, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x99, cpu->A);
}

// 0xAE - LDX ABS
void test_0xAE_LDX_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x0500) = 0x42;
    byte_t instr[] = {INSTRUCTION_LDX_ABS, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x42, cpu->X);
}

// 0xB0 - BCS
void test_0xB0_BCS(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->PC = 0x0700;
    set_flag(cpu, STATUS_FLAG_C, true);
    byte_t instr[] = {INSTRUCTION_BCS_REL, 0x0A};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(0x0700 + BRANCH_INSTR_LEN + 0x0A, cpu->PC);
}

// 0xB1 - LDA IZY
void test_0xB1_LDA_IZY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0;
    *test_mem_ptr(0x30) = 0x00;
    *test_mem_ptr(0x31) = 0x03;
    *test_mem_ptr(0x0300) = 0x99;
    byte_t instr[] = {INSTRUCTION_LDA_IZY, 0x30};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x99, cpu->A);
}

// 0xB4 - LDY ZPX
void test_0xB4_LDY_ZPX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x02;
    *test_mem_ptr(0x42) = 0x42;
    byte_t instr[] = {INSTRUCTION_LDY_ZPX, 0x40};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x42, cpu->Y);
}

// 0xB5 - LDA ZPX
void test_0xB5_LDA_ZPX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x02;
    *test_mem_ptr(0x42) = 0x99;
    byte_t instr[] = {INSTRUCTION_LDA_ZPX, 0x40};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x99, cpu->A);
}

// 0xB6 - LDX ZPY
void test_0xB6_LDX_ZPY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0x02;
    *test_mem_ptr(0x42) = 0x42;
    byte_t instr[] = {INSTRUCTION_LDX_ZPY, 0x40};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x42, cpu->X);
}

// 0xB8 - CLV
void test_0xB8_CLV(void) {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, STATUS_FLAG_V, true);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_V));
    byte_t instr[] = {INSTRUCTION_CLV_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_V));
}

// 0xB9 - LDA ABY
void test_0xB9_LDA_ABY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0;
    *test_mem_ptr(0x0500) = 0x99;
    byte_t instr[] = {INSTRUCTION_LDA_ABY, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x99, cpu->A);
}

// 0xBA - TSX
void test_0xBA_TSX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->SP = 0x60;
    byte_t instr[] = {INSTRUCTION_TSX_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x60, cpu->X);
}

// 0xBC - LDY ABX
void test_0xBC_LDY_ABX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0;
    *test_mem_ptr(0x0500) = 0x42;
    byte_t instr[] = {INSTRUCTION_LDY_ABX, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x42, cpu->Y);
}

// 0xBD - LDA ABX
void test_0xBD_LDA_ABX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0;
    *test_mem_ptr(0x0500) = 0x99;
    byte_t instr[] = {INSTRUCTION_LDA_ABX, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x99, cpu->A);
}

// 0xBE - LDX ABY
void test_0xBE_LDX_ABY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0;
    *test_mem_ptr(0x0500) = 0x42;
    byte_t instr[] = {INSTRUCTION_LDX_ABY, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x42, cpu->X);
}

// 0xC0 - CPY IMM
void test_0xC0_CPY_IMM(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0x05;
    byte_t instr[] = {INSTRUCTION_CPY_IMM, 0x03};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_Z));
}

// 0xC1 - CMP IZX
void test_0xC1_CMP_IZX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    cpu->X = 0;
    *test_mem_ptr(0x10) = 0x00;
    *test_mem_ptr(0x11) = 0x03;
    *test_mem_ptr(0x0300) = 0x03;
    byte_t instr[] = {INSTRUCTION_CMP_IZX, 0x10};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_Z));
}

// 0xC4 - CPY ZP0
void test_0xC4_CPY_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0x05;
    *test_mem_ptr(0x20) = 0x05;
    byte_t instr[] = {INSTRUCTION_CPY_ZP0, 0x20};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_Z));
}

// 0xC5 - CMP ZP0
void test_0xC5_CMP_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    *test_mem_ptr(0x20) = 0x05;
    byte_t instr[] = {INSTRUCTION_CMP_ZP0, 0x20};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_Z));
}

// 0xC6 - DEC ZP0
void test_0xC6_DEC_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x30) = 0x01;
    byte_t instr[] = {INSTRUCTION_DEC_ZP0, 0x30};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x00, *test_mem_ptr(0x30));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_Z));
}

// 0xC8 - INY
void test_0xC8_INY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0x00;
    byte_t instr[] = {INSTRUCTION_INY_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x01, cpu->Y);
}

// 0xC9 - CMP IMM
void test_0xC9_CMP_IMM(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0A;
    byte_t instr[] = {INSTRUCTION_CMP_IMM, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_Z));
}

// 0xCA - DEX
void test_0xCA_DEX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x01;
    byte_t instr[] = {INSTRUCTION_DEX_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x00, cpu->X);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_Z));
}

// 0xCC - CPY ABS
void test_0xCC_CPY_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->Y = 0x0A;
    *test_mem_ptr(0x0500) = 0x05;
    byte_t instr[] = {INSTRUCTION_CPY_ABS, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
}

// 0xCD - CMP ABS
void test_0xCD_CMP_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0A;
    *test_mem_ptr(0x0500) = 0x05;
    byte_t instr[] = {INSTRUCTION_CMP_ABS, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
}

// 0xCE - DEC ABS
void test_0xCE_DEC_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x0500) = 0x02;
    byte_t instr[] = {INSTRUCTION_DEC_ABS, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x01, *test_mem_ptr(0x0500));
}

// 0xD0 - BNE
void test_0xD0_BNE(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->PC = 0x0800;
    set_flag(cpu, STATUS_FLAG_Z, false);
    byte_t instr[] = {INSTRUCTION_BNE_REL, 0x06};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(0x0800 + BRANCH_INSTR_LEN + 0x06, cpu->PC);
}

// 0xD1 - CMP IZY
void test_0xD1_CMP_IZY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0A;
    cpu->Y = 0;
    *test_mem_ptr(0x30) = 0x00;
    *test_mem_ptr(0x31) = 0x03;
    *test_mem_ptr(0x0300) = 0x05;
    byte_t instr[] = {INSTRUCTION_CMP_IZY, 0x30};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
}

// 0xD5 - CMP ZPX
void test_0xD5_CMP_ZPX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0A;
    cpu->X = 0x01;
    *test_mem_ptr(0x41) = 0x05;
    byte_t instr[] = {INSTRUCTION_CMP_ZPX, 0x40};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
}

// 0xD6 - DEC ZPX
void test_0xD6_DEC_ZPX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x01;
    *test_mem_ptr(0x51) = 0x02;
    byte_t instr[] = {INSTRUCTION_DEC_ZPX, 0x50};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x01, *test_mem_ptr(0x51));
}

// 0xD8 - CLD
void test_0xD8_CLD(void) {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, STATUS_FLAG_D, true);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_D));
    byte_t instr[] = {INSTRUCTION_CLD_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_D));
}

// 0xD9 - CMP ABY
void test_0xD9_CMP_ABY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0A;
    cpu->Y = 0;
    *test_mem_ptr(0x0500) = 0x05;
    byte_t instr[] = {INSTRUCTION_CMP_ABY, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
}

// 0xDD - CMP ABX
void test_0xDD_CMP_ABX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x0A;
    cpu->X = 0;
    *test_mem_ptr(0x0500) = 0x05;
    byte_t instr[] = {INSTRUCTION_CMP_ABX, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
}

// 0xDE - DEC ABX
void test_0xDE_DEC_ABX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0;
    *test_mem_ptr(0x0500) = 0x02;
    byte_t instr[] = {INSTRUCTION_DEC_ABX, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x01, *test_mem_ptr(0x0500));
}

// 0xE0 - CPX IMM
void test_0xE0_CPX_IMM(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x05;
    byte_t instr[] = {INSTRUCTION_CPX_IMM, 0x03};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
}

// 0xE1 - SBC IZX
void test_0xE1_SBC_IZX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    set_flag(cpu, STATUS_FLAG_C, true);
    cpu->X = 0;
    *test_mem_ptr(0x10) = 0x00;
    *test_mem_ptr(0x11) = 0x03;
    *test_mem_ptr(0x0300) = 0x02;
    byte_t instr[] = {INSTRUCTION_SBC_IZX, 0x10};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x03, cpu->A);
}

// 0xE4 - CPX ZP0
void test_0xE4_CPX_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x05;
    *test_mem_ptr(0x20) = 0x05;
    byte_t instr[] = {INSTRUCTION_CPX_ZP0, 0x20};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_Z));
}

// 0xE5 - SBC ZP0
void test_0xE5_SBC_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    set_flag(cpu, STATUS_FLAG_C, true);
    *test_mem_ptr(0x20) = 0x02;
    byte_t instr[] = {INSTRUCTION_SBC_ZP0, 0x20};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x03, cpu->A);
}

// 0xE6 - INC ZP0
void test_0xE6_INC_ZP0(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x30) = 0x01;
    byte_t instr[] = {INSTRUCTION_INC_ZP0, 0x30};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x02, *test_mem_ptr(0x30));
}

// 0xE8 - INX
void test_0xE8_INX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x00;
    byte_t instr[] = {INSTRUCTION_INX_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x01, cpu->X);
}

// 0xE9 - SBC IMM
void test_0xE9_SBC_IMM(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    set_flag(cpu, STATUS_FLAG_C, true);
    byte_t instr[] = {INSTRUCTION_SBC_IMM, 0x02};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x03, cpu->A);
}

// 0xEA - NOP
void test_0xEA_NOP(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x42;
    byte_t instr[] = {INSTRUCTION_NOP_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x42, cpu->A);
}

// 0xEC - CPX ABS
void test_0xEC_CPX_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x0A;
    *test_mem_ptr(0x0500) = 0x05;
    byte_t instr[] = {INSTRUCTION_CPX_ABS, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
}

// 0xED - SBC ABS
void test_0xED_SBC_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    set_flag(cpu, STATUS_FLAG_C, true);
    *test_mem_ptr(0x0500) = 0x02;
    byte_t instr[] = {INSTRUCTION_SBC_ABS, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x03, cpu->A);
}

// 0xEE - INC ABS
void test_0xEE_INC_ABS(void) {
    cpu_s *cpu = get_test_cpu();
    *test_mem_ptr(0x0500) = 0x01;
    byte_t instr[] = {INSTRUCTION_INC_ABS, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x02, *test_mem_ptr(0x0500));
}

// 0xF0 - BEQ
void test_0xF0_BEQ(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->PC = 0x0900;
    set_flag(cpu, STATUS_FLAG_Z, true);
    byte_t instr[] = {INSTRUCTION_BEQ_REL, 0x04};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(0x0900 + BRANCH_INSTR_LEN + 0x04, cpu->PC);
}

// 0xF1 - SBC IZY
void test_0xF1_SBC_IZY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    set_flag(cpu, STATUS_FLAG_C, true);
    cpu->Y = 0;
    *test_mem_ptr(0x30) = 0x00;
    *test_mem_ptr(0x31) = 0x03;
    *test_mem_ptr(0x0300) = 0x02;
    byte_t instr[] = {INSTRUCTION_SBC_IZY, 0x30};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x03, cpu->A);
}

// 0xF5 - SBC ZPX
void test_0xF5_SBC_ZPX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    set_flag(cpu, STATUS_FLAG_C, true);
    cpu->X = 0x01;
    *test_mem_ptr(0x41) = 0x02;
    byte_t instr[] = {INSTRUCTION_SBC_ZPX, 0x40};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x03, cpu->A);
}

// 0xF6 - INC ZPX
void test_0xF6_INC_ZPX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0x01;
    *test_mem_ptr(0x51) = 0x01;
    byte_t instr[] = {INSTRUCTION_INC_ZPX, 0x50};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x02, *test_mem_ptr(0x51));
}

// 0xF8 - SED
void test_0xF8_SED(void) {
    cpu_s *cpu = get_test_cpu();
    set_flag(cpu, STATUS_FLAG_D, false);
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_D));
    byte_t instr[] = {INSTRUCTION_SED_IMP};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_D));
}

// 0xF9 - SBC ABY
void test_0xF9_SBC_ABY(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    set_flag(cpu, STATUS_FLAG_C, true);
    cpu->Y = 0;
    *test_mem_ptr(0x0500) = 0x02;
    byte_t instr[] = {INSTRUCTION_SBC_ABY, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x03, cpu->A);
}

// 0xFD - SBC ABX
void test_0xFD_SBC_ABX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->A = 0x05;
    set_flag(cpu, STATUS_FLAG_C, true);
    cpu->X = 0;
    *test_mem_ptr(0x0500) = 0x02;
    byte_t instr[] = {INSTRUCTION_SBC_ABX, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x03, cpu->A);
}

// 0xFE - INC ABX
void test_0xFE_INC_ABX(void) {
    cpu_s *cpu = get_test_cpu();
    cpu->X = 0;
    *test_mem_ptr(0x0500) = 0x01;
    byte_t instr[] = {INSTRUCTION_INC_ABX, 0x00, 0x05};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(0x02, *test_mem_ptr(0x0500));
}

// =============================================================================
// Edge Case Tests - High Priority
// =============================================================================

// --- Negative Branch Offset Tests ---
// Branch instructions use signed offsets (0x80-0xFF = negative)

void test_BPL_negative_offset(void) {
    cpu_s *cpu = get_test_cpu();
    word_t start_pc = 0x0210;
    byte_t offset_byte = 0xFC;
    word_t expected_pc = start_pc + BRANCH_INSTR_LEN + (int8_t)offset_byte;

    cpu->PC = start_pc;
    set_flag(cpu, STATUS_FLAG_N, false);
    byte_t instr[] = {INSTRUCTION_BPL_REL, offset_byte};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(expected_pc, cpu->PC);
}

void test_BMI_negative_offset(void) {
    cpu_s *cpu = get_test_cpu();
    word_t start_pc = 0x0310;
    byte_t offset_byte = 0xF0;
    word_t expected_pc = start_pc + BRANCH_INSTR_LEN + (int8_t)offset_byte;

    cpu->PC = start_pc;
    set_flag(cpu, STATUS_FLAG_N, true);
    byte_t instr[] = {INSTRUCTION_BMI_REL, offset_byte};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(expected_pc, cpu->PC);
}

void test_BVC_negative_offset(void) {
    cpu_s *cpu = get_test_cpu();
    word_t start_pc = 0x0420;
    byte_t offset_byte = 0xFE;
    word_t expected_pc = start_pc + BRANCH_INSTR_LEN + (int8_t)offset_byte;

    cpu->PC = start_pc;
    set_flag(cpu, STATUS_FLAG_V, false);
    byte_t instr[] = {INSTRUCTION_BVC_REL, offset_byte};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(expected_pc, cpu->PC);
}

void test_BVS_negative_offset(void) {
    cpu_s *cpu = get_test_cpu();
    word_t start_pc = 0x0530;
    byte_t offset_byte = 0x80;
    word_t expected_pc = start_pc + BRANCH_INSTR_LEN + (int8_t)offset_byte;

    cpu->PC = start_pc;
    set_flag(cpu, STATUS_FLAG_V, true);
    byte_t instr[] = {INSTRUCTION_BVS_REL, offset_byte};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(expected_pc, cpu->PC);
}

void test_BCC_negative_offset(void) {
    cpu_s *cpu = get_test_cpu();
    word_t start_pc = 0x0640;
    byte_t offset_byte = 0xF8;
    word_t expected_pc = start_pc + BRANCH_INSTR_LEN + (int8_t)offset_byte;

    cpu->PC = start_pc;
    set_flag(cpu, STATUS_FLAG_C, false);
    byte_t instr[] = {INSTRUCTION_BCC_REL, offset_byte};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(expected_pc, cpu->PC);
}

void test_BCS_negative_offset(void) {
    cpu_s *cpu = get_test_cpu();
    word_t start_pc = 0x0750;
    byte_t offset_byte = 0xEE;
    word_t expected_pc = start_pc + BRANCH_INSTR_LEN + (int8_t)offset_byte;

    cpu->PC = start_pc;
    set_flag(cpu, STATUS_FLAG_C, true);
    byte_t instr[] = {INSTRUCTION_BCS_REL, offset_byte};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(expected_pc, cpu->PC);
}

void test_BNE_negative_offset(void) {
    cpu_s *cpu = get_test_cpu();
    word_t start_pc = 0x0860;
    byte_t offset_byte = 0xFA;
    word_t expected_pc = start_pc + BRANCH_INSTR_LEN + (int8_t)offset_byte;

    cpu->PC = start_pc;
    set_flag(cpu, STATUS_FLAG_Z, false);
    byte_t instr[] = {INSTRUCTION_BNE_REL, offset_byte};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(expected_pc, cpu->PC);
}

void test_BEQ_negative_offset(void) {
    cpu_s *cpu = get_test_cpu();
    word_t start_pc = 0x0970;
    byte_t offset_byte = 0xE0;
    word_t expected_pc = start_pc + BRANCH_INSTR_LEN + (int8_t)offset_byte;

    cpu->PC = start_pc;
    set_flag(cpu, STATUS_FLAG_Z, true);
    byte_t instr[] = {INSTRUCTION_BEQ_REL, offset_byte};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(expected_pc, cpu->PC);
}

// --- JMP Indirect Page Boundary Bug Test ---
// The 6502 has a bug: JMP ($xxFF) reads high byte from $xx00, not $xx00+$0100

void test_JMP_IND_page_boundary_bug(void) {
    cpu_s *cpu = get_test_cpu();
    // Use addresses that don't conflict with PC (which is at 0x0000 and uses 0x0000-0x0002)
    // Use page 2 ($02xx) for the indirect pointer - with 2KB RAM mirroring this maps to $0200+
    word_t ptr_addr = 0x02FF; // pointer at page boundary
    word_t target_addr = 0x1234;
    word_t buggy_high_addr = ptr_addr & 0xFF00; // wraps to $0200, not $0300
    word_t wrong_high_addr = (ptr_addr & 0xFF00) + 0x0100; // $0300

    // Load instruction first before setting up the memory
    byte_t instr[] = {INSTRUCTION_JMP_IND, ptr_addr & 0xFF, (ptr_addr >> 8) & 0xFF};
    load_instruction(cpu, instr, sizeof(instr));

    // Now set up the indirect address data
    *test_mem_ptr(ptr_addr) = target_addr & 0xFF;
    *test_mem_ptr(buggy_high_addr) = (target_addr >> 8) & 0xFF;
    *test_mem_ptr(wrong_high_addr) = 0xFF; // should NOT be used

    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX16(target_addr, cpu->PC);
}

// --- ADC Overflow Flag Edge Cases ---
// Overflow occurs when sign of result differs from sign of both operands

void test_ADC_overflow_positive_plus_positive(void) {
    cpu_s *cpu = get_test_cpu();
    byte_t operand_a = 0x50;
    byte_t operand_b = 0x50;
    byte_t expected_result = (operand_a + operand_b) & 0xFF;
    word_t mem_addr = 0x0500;

    cpu->A = operand_a;
    set_flag(cpu, STATUS_FLAG_C, false);
    set_flag(cpu, STATUS_FLAG_V, false);
    *test_mem_ptr(mem_addr) = operand_b;
    byte_t instr[] = {INSTRUCTION_ADC_ABS, mem_addr & 0xFF, (mem_addr >> 8) & 0xFF};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(expected_result, cpu->A);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_V));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_N));
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_C));
}

void test_ADC_overflow_negative_plus_negative(void) {
    cpu_s *cpu = get_test_cpu();
    byte_t operand_a = 0x90;
    byte_t operand_b = 0x90;
    byte_t expected_result = (operand_a + operand_b) & 0xFF;
    word_t mem_addr = 0x0500;

    cpu->A = operand_a;
    set_flag(cpu, STATUS_FLAG_C, false);
    set_flag(cpu, STATUS_FLAG_V, false);
    *test_mem_ptr(mem_addr) = operand_b;
    byte_t instr[] = {INSTRUCTION_ADC_ABS, mem_addr & 0xFF, (mem_addr >> 8) & 0xFF};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(expected_result, cpu->A);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_V));
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_N));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
}

void test_ADC_no_overflow_positive_plus_negative(void) {
    cpu_s *cpu = get_test_cpu();
    byte_t operand_a = 0x50;
    byte_t operand_b = 0xD0;
    byte_t expected_result = (operand_a + operand_b) & 0xFF;
    word_t mem_addr = 0x0500;

    cpu->A = operand_a;
    set_flag(cpu, STATUS_FLAG_C, false);
    set_flag(cpu, STATUS_FLAG_V, false);
    *test_mem_ptr(mem_addr) = operand_b;
    byte_t instr[] = {INSTRUCTION_ADC_ABS, mem_addr & 0xFF, (mem_addr >> 8) & 0xFF};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(expected_result, cpu->A);
    TEST_ASSERT_FALSE(get_flag(cpu, STATUS_FLAG_V));
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_C));
}

void test_SBC_overflow_positive_minus_negative(void) {
    cpu_s *cpu = get_test_cpu();
    byte_t operand_a = 0x50;
    byte_t operand_b = 0xB0;
    byte_t expected_result = (operand_a - operand_b) & 0xFF;
    word_t mem_addr = 0x0500;

    cpu->A = operand_a;
    set_flag(cpu, STATUS_FLAG_C, true);
    set_flag(cpu, STATUS_FLAG_V, false);
    *test_mem_ptr(mem_addr) = operand_b;
    byte_t instr[] = {INSTRUCTION_SBC_ABS, mem_addr & 0xFF, (mem_addr >> 8) & 0xFF};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(expected_result, cpu->A);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_V));
}

void test_SBC_overflow_negative_minus_positive(void) {
    cpu_s *cpu = get_test_cpu();
    byte_t operand_a = 0x80;
    byte_t operand_b = 0x01;
    byte_t expected_result = (operand_a - operand_b) & 0xFF;
    word_t mem_addr = 0x0500;

    cpu->A = operand_a;
    set_flag(cpu, STATUS_FLAG_C, true);
    set_flag(cpu, STATUS_FLAG_V, false);
    *test_mem_ptr(mem_addr) = operand_b;
    byte_t instr[] = {INSTRUCTION_SBC_ABS, mem_addr & 0xFF, (mem_addr >> 8) & 0xFF};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(expected_result, cpu->A);
    TEST_ASSERT_TRUE(get_flag(cpu, STATUS_FLAG_V));
}

// --- Page Boundary Crossing Tests ---
// These addressing modes add an extra cycle when crossing page boundaries

void test_LDA_ABX_page_cross(void) {
    cpu_s *cpu = get_test_cpu();
    word_t base_addr = 0x10F0;
    byte_t index = 0x10;
    word_t effective_addr = base_addr + index;
    byte_t expected_value = 0x42;

    cpu->X = index;
    *test_mem_ptr(effective_addr) = expected_value;
    byte_t instr[] = {INSTRUCTION_LDA_ABX, base_addr & 0xFF, (base_addr >> 8) & 0xFF};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(expected_value, cpu->A);
}

void test_LDA_ABY_page_cross(void) {
    cpu_s *cpu = get_test_cpu();
    word_t base_addr = 0x11E0;
    byte_t index = 0x20;
    word_t effective_addr = base_addr + index;
    byte_t expected_value = 0x55;

    cpu->Y = index;
    *test_mem_ptr(effective_addr) = expected_value;
    byte_t instr[] = {INSTRUCTION_LDA_ABY, base_addr & 0xFF, (base_addr >> 8) & 0xFF};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(expected_value, cpu->A);
}

void test_LDA_IZY_page_cross(void) {
    cpu_s *cpu = get_test_cpu();
    byte_t zp_addr = 0x20;
    word_t base_addr = 0x13D0;
    byte_t index = 0x30;
    word_t effective_addr = base_addr + index;
    byte_t expected_value = 0x77;

    cpu->Y = index;
    *test_mem_ptr(zp_addr) = base_addr & 0xFF;
    *test_mem_ptr(zp_addr + 1) = (base_addr >> 8) & 0xFF;
    *test_mem_ptr(effective_addr) = expected_value;
    byte_t instr[] = {INSTRUCTION_LDA_IZY, zp_addr};
    load_instruction(cpu, instr, sizeof(instr));
    execute_instruction(cpu);
    TEST_ASSERT_EQUAL_HEX8(expected_value, cpu->A);
}

// =============================================================================
// Main - Unity test runner
// =============================================================================

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_0x00_BRK);
    RUN_TEST(test_0x01_ORA_IZX);
    RUN_TEST(test_0x05_ORA_ZP0);
    RUN_TEST(test_0x06_ASL_ZP0);
    RUN_TEST(test_0x08_PHP);
    RUN_TEST(test_0x09_ORA_IMM);
    RUN_TEST(test_0x0A_ASL_ACC);
    RUN_TEST(test_0x0D_ORA_ABS);
    RUN_TEST(test_0x0E_ASL_ABS);
    RUN_TEST(test_0x10_BPL);
    RUN_TEST(test_0x11_ORA_IZY);
    RUN_TEST(test_0x15_ORA_ZPX);
    RUN_TEST(test_0x16_ASL_ZPX);
    RUN_TEST(test_0x18_CLC);
    RUN_TEST(test_0x19_ORA_ABY);
    RUN_TEST(test_0x1D_ORA_ABX);
    RUN_TEST(test_0x1E_ASL_ABX);
    RUN_TEST(test_0x20_JSR);
    RUN_TEST(test_0x21_AND_IZX);
    RUN_TEST(test_0x24_BIT_ZP0);
    RUN_TEST(test_0x25_AND_ZP0);
    RUN_TEST(test_0x26_ROL_ZP0);
    RUN_TEST(test_0x28_PLP);
    RUN_TEST(test_0x29_AND_IMM);
    RUN_TEST(test_0x2A_ROL_ACC);
    RUN_TEST(test_0x2C_BIT_ABS);
    RUN_TEST(test_0x2D_AND_ABS);
    RUN_TEST(test_0x2E_ROL_ABS);
    RUN_TEST(test_0x30_BMI);
    RUN_TEST(test_0x31_AND_IZY);
    RUN_TEST(test_0x35_AND_ZPX);
    RUN_TEST(test_0x36_ROL_ZPX);
    RUN_TEST(test_0x38_SEC);
    RUN_TEST(test_0x39_AND_ABY);
    RUN_TEST(test_0x3D_AND_ABX);
    RUN_TEST(test_0x3E_ROL_ABX);
    RUN_TEST(test_0x40_RTI);
    RUN_TEST(test_0x41_EOR_IZX);
    RUN_TEST(test_0x45_EOR_ZP0);
    RUN_TEST(test_0x46_LSR_ZP0);
    RUN_TEST(test_0x48_PHA);
    RUN_TEST(test_0x49_EOR_IMM);
    RUN_TEST(test_0x4A_LSR_ACC);
    RUN_TEST(test_0x4C_JMP_ABS);
    RUN_TEST(test_0x4D_EOR_ABS);
    RUN_TEST(test_0x4E_LSR_ABS);
    RUN_TEST(test_0x50_BVC);
    RUN_TEST(test_0x51_EOR_IZY);
    RUN_TEST(test_0x55_EOR_ZPX);
    RUN_TEST(test_0x56_LSR_ZPX);
    RUN_TEST(test_0x58_CLI);
    RUN_TEST(test_0x59_EOR_ABY);
    RUN_TEST(test_0x5D_EOR_ABX);
    RUN_TEST(test_0x5E_LSR_ABX);
    RUN_TEST(test_0x60_RTS);
    RUN_TEST(test_0x61_ADC_IZX);
    RUN_TEST(test_0x65_ADC_ZP0);
    RUN_TEST(test_0x66_ROR_ZP0);
    RUN_TEST(test_0x68_PLA);
    RUN_TEST(test_0x69_ADC_IMM);
    RUN_TEST(test_0x6A_ROR_ACC);
    RUN_TEST(test_0x6C_JMP_IND);
    RUN_TEST(test_0x6D_ADC_ABS);
    RUN_TEST(test_0x6E_ROR_ABS);
    RUN_TEST(test_0x70_BVS);
    RUN_TEST(test_0x71_ADC_IZY);
    RUN_TEST(test_0x75_ADC_ZPX);
    RUN_TEST(test_0x76_ROR_ZPX);
    RUN_TEST(test_0x78_SEI);
    RUN_TEST(test_0x79_ADC_ABY);
    RUN_TEST(test_0x7D_ADC_ABX);
    RUN_TEST(test_0x7E_ROR_ABX);
    RUN_TEST(test_0x81_STA_IZX);
    RUN_TEST(test_0x84_STY_ZP0);
    RUN_TEST(test_0x85_STA_ZP0);
    RUN_TEST(test_0x86_STX_ZP0);
    RUN_TEST(test_0x88_DEY);
    RUN_TEST(test_0x8A_TXA);
    RUN_TEST(test_0x8C_STY_ABS);
    RUN_TEST(test_0x8D_STA_ABS);
    RUN_TEST(test_0x8E_STX_ABS);
    RUN_TEST(test_0x90_BCC);
    RUN_TEST(test_0x91_STA_IZY);
    RUN_TEST(test_0x94_STY_ZPX);
    RUN_TEST(test_0x95_STA_ZPX);
    RUN_TEST(test_0x96_STX_ZPY);
    RUN_TEST(test_0x98_TYA);
    RUN_TEST(test_0x99_STA_ABY);
    RUN_TEST(test_0x9A_TXS);
    RUN_TEST(test_0x9D_STA_ABX);
    RUN_TEST(test_0xA0_LDY_IMM);
    RUN_TEST(test_0xA1_LDA_IZX);
    RUN_TEST(test_0xA2_LDX_IMM);
    RUN_TEST(test_0xA4_LDY_ZP0);
    RUN_TEST(test_0xA5_LDA_ZP0);
    RUN_TEST(test_0xA6_LDX_ZP0);
    RUN_TEST(test_0xA8_TAY);
    RUN_TEST(test_0xA9_LDA_IMM);
    RUN_TEST(test_0xAA_TAX);
    RUN_TEST(test_0xAC_LDY_ABS);
    RUN_TEST(test_0xAD_LDA_ABS);
    RUN_TEST(test_0xAE_LDX_ABS);
    RUN_TEST(test_0xB0_BCS);
    RUN_TEST(test_0xB1_LDA_IZY);
    RUN_TEST(test_0xB4_LDY_ZPX);
    RUN_TEST(test_0xB5_LDA_ZPX);
    RUN_TEST(test_0xB6_LDX_ZPY);
    RUN_TEST(test_0xB8_CLV);
    RUN_TEST(test_0xB9_LDA_ABY);
    RUN_TEST(test_0xBA_TSX);
    RUN_TEST(test_0xBC_LDY_ABX);
    RUN_TEST(test_0xBD_LDA_ABX);
    RUN_TEST(test_0xBE_LDX_ABY);
    RUN_TEST(test_0xC0_CPY_IMM);
    RUN_TEST(test_0xC1_CMP_IZX);
    RUN_TEST(test_0xC4_CPY_ZP0);
    RUN_TEST(test_0xC5_CMP_ZP0);
    RUN_TEST(test_0xC6_DEC_ZP0);
    RUN_TEST(test_0xC8_INY);
    RUN_TEST(test_0xC9_CMP_IMM);
    RUN_TEST(test_0xCA_DEX);
    RUN_TEST(test_0xCC_CPY_ABS);
    RUN_TEST(test_0xCD_CMP_ABS);
    RUN_TEST(test_0xCE_DEC_ABS);
    RUN_TEST(test_0xD0_BNE);
    RUN_TEST(test_0xD1_CMP_IZY);
    RUN_TEST(test_0xD5_CMP_ZPX);
    RUN_TEST(test_0xD6_DEC_ZPX);
    RUN_TEST(test_0xD8_CLD);
    RUN_TEST(test_0xD9_CMP_ABY);
    RUN_TEST(test_0xDD_CMP_ABX);
    RUN_TEST(test_0xDE_DEC_ABX);
    RUN_TEST(test_0xE0_CPX_IMM);
    RUN_TEST(test_0xE1_SBC_IZX);
    RUN_TEST(test_0xE4_CPX_ZP0);
    RUN_TEST(test_0xE5_SBC_ZP0);
    RUN_TEST(test_0xE6_INC_ZP0);
    RUN_TEST(test_0xE8_INX);
    RUN_TEST(test_0xE9_SBC_IMM);
    RUN_TEST(test_0xEA_NOP);
    RUN_TEST(test_0xEC_CPX_ABS);
    RUN_TEST(test_0xED_SBC_ABS);
    RUN_TEST(test_0xEE_INC_ABS);
    RUN_TEST(test_0xF0_BEQ);
    RUN_TEST(test_0xF1_SBC_IZY);
    RUN_TEST(test_0xF5_SBC_ZPX);
    RUN_TEST(test_0xF6_INC_ZPX);
    RUN_TEST(test_0xF8_SED);
    RUN_TEST(test_0xF9_SBC_ABY);
    RUN_TEST(test_0xFD_SBC_ABX);
    RUN_TEST(test_0xFE_INC_ABX);

    // Edge case tests - negative branch offsets
    RUN_TEST(test_BPL_negative_offset);
    RUN_TEST(test_BMI_negative_offset);
    RUN_TEST(test_BVC_negative_offset);
    RUN_TEST(test_BVS_negative_offset);
    RUN_TEST(test_BCC_negative_offset);
    RUN_TEST(test_BCS_negative_offset);
    RUN_TEST(test_BNE_negative_offset);
    RUN_TEST(test_BEQ_negative_offset);

    // Edge case tests - JMP indirect page boundary bug
    RUN_TEST(test_JMP_IND_page_boundary_bug);

    // Edge case tests - ADC/SBC overflow
    RUN_TEST(test_ADC_overflow_positive_plus_positive);
    RUN_TEST(test_ADC_overflow_negative_plus_negative);
    RUN_TEST(test_ADC_no_overflow_positive_plus_negative);
    RUN_TEST(test_SBC_overflow_positive_minus_negative);
    RUN_TEST(test_SBC_overflow_negative_minus_positive);

    // Edge case tests - page boundary crossing
    RUN_TEST(test_LDA_ABX_page_cross);
    RUN_TEST(test_LDA_ABY_page_cross);
    RUN_TEST(test_LDA_IZY_page_cross);

    return UNITY_END();
}
