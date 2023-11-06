#define MUNIT_ENABLE_ASSERT_ALIASES

#include <stdio.h>
#include <stdbool.h>
#include "munit/munit.h"
#include "../src/cpu.h"

// 64 kb memory
#define RESET_VECTOR 0xFFFC
#define MEM_SIZE 1024 * 64

#define X(instruction, mode, opcode) MunitResult Test_##instruction##_##mode(const MunitParameter params[], void *fixture);
ALL_INSTRUCTIONS
#undef X

typedef struct
{
    cpu_s *cpu;
    word_t program_counter //start of instruction
} test_fixture_s;

typedef enum
{
    NEG,
    ZERO,
    POS
} sign_t;

static void instruction_init(instruction_s ins, word_t starting_addr)
{
    
}

cpu_s *cpu_create()
{
    cpu_s *cpu = malloc(sizeof(cpu_s));
    byte_t *memory = malloc(MEM_SIZE);

    cpu_init(cpu, memory);

    return cpu;
}

static void* setup(const MunitParameter params[], void *user_data)
{
    cpu_s *cpu = cpu_create();
    test_fixture_s *fixture = malloc(sizeof(test_fixture_s));
    fixture->cpu = cpu;
    //random program counter in address space (like an NES program)
    fixture->program_counter = rand() % MEM_SIZE;
    return fixture;
}

static void tear_down(void *fixture)
{
    test_fixture_s *test_fixture = (test_fixture_s*)fixture;
    free(test_fixture->cpu->memory);
    free(test_fixture->cpu);
    free(test_fixture);
}

static bool check_nz_flags(cpu_s *cpu, sign_t sign, byte_t old_status)
{
    //ensure that only the N and Z flags have been changed
    bool valid = (cpu->STATUS & ~(N | Z)) == (old_status & ~(N | Z));
    if (!valid)
        return false;
    
    switch (sign)
    {
    case NEG:
        return (cpu->STATUS & N) && !(cpu->STATUS & Z);
    case ZERO:
        return (cpu->STATUS & Z) && !(cpu->STATUS & N);
    case POS:
        return !(cpu->STATUS & (N | Z));
    default:
        return false;
    }
}


static void run(cpu_s *cpu, size_t cycles)
{
    while(cycles--)
    {
        clock(cpu);
    }
}





int main(int argc, char *argv[])
{
    MunitTest tests[] = {
        {
            "/AND IMM",
            Test_AND_IMM,
            setup,
            tear_down, 
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/AND ZP0", 
            Test_AND_ZP0, 
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/AND ZPX", 
            Test_AND_ZPX, 
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/AND ABS",
            Test_AND_ABS,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/AND ABX",
            Test_AND_ABX,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/AND ABY",
            Test_AND_ABY,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        // {
        //     "/AND IZX",
        //     Test_AND_IZX,
        //     setup,
        //     tear_down,
        //     MUNIT_TEST_OPTION_NONE,
        // },
        // {
        //     "/AND IZY",
        //     Test_AND_IZY,
        //     setup,
        //     tear_down,
        //     MUNIT_TEST_OPTION_NONE,
        // },
        { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
    };
    MunitSuite suite = {
        "Instruction Tests", //name
        tests, //tests
        NULL, //suites
        1, //iterations
        MUNIT_SUITE_OPTION_NONE, //options
    };
    return munit_suite_main(&suite, NULL, argc, argv);
}

MunitResult Test_AND_IMM(const MunitParameter params[], void *fixture)
{
    test_fixture_s *test_fixture = (test_fixture_s*)fixture;
    cpu_s *cpu = test_fixture->cpu;
    word_t program_counter = test_fixture->program_counter;
    byte_t opcode = INSTRUCTION_AND_IMM;
    cpu->memory[program_counter] = opcode;
    instruction_s instruction = cpu->table[opcode];

    //ensure instruction is correct

    assert_int(instruction.name, ==, "AND");
    assert_int(instruction.cycles, ==, 2);
    assert_int(instruction.length, ==, 2);
    assert_int(instruction.fetch, ==, IMM);
    assert_int(instruction.execute, ==, AND);

    cpu->memory[RESET_VECTOR] = (byte_t) (program_counter & 0x00FF);
    cpu->memory[RESET_VECTOR + 1] = (byte_t)((program_counter & 0xFF00) >> 8);
    
    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = 0x80;
    cpu->A = 0x81;

    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x80);
    assert_true(check_nz_flags(cpu, NEG, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = 0x00;
    cpu->A = 0x00;

    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x00);
    assert_true(check_nz_flags(cpu, ZERO, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = 0x01;
    cpu->A = 0x03;

    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x01);
    assert_true(check_nz_flags(cpu, POS, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);
    
    return MUNIT_OK;
}

MunitResult Test_AND_ZP0(const MunitParameter params[], void *fixture)
{
    test_fixture_s *test_fixture = (test_fixture_s*)fixture;
    cpu_s *cpu = test_fixture->cpu;
    word_t program_counter = test_fixture->program_counter;
    byte_t opcode = INSTRUCTION_AND_ZP0;
    cpu->memory[program_counter] = opcode;
    instruction_s instruction = cpu->table[opcode];
    //ensure instruction is correct
    assert_int(instruction.name, ==, "AND");
    assert_int(instruction.cycles, ==, 3);
    assert_int(instruction.length, ==, 2);
    assert_int(instruction.fetch, ==, ZP0);
    assert_int(instruction.execute, ==, AND);

    byte_t zero_page_address = 0x80;

    cpu->memory[RESET_VECTOR] = (byte_t) (program_counter & 0x00FF);
    cpu->memory[RESET_VECTOR + 1] = (byte_t)((program_counter & 0xFF00) >> 8);
    
    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = zero_page_address;
    cpu->A = 0x81;
    cpu->memory[zero_page_address] = 0x81;

    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x81);
    assert_true(check_nz_flags(cpu, NEG, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = zero_page_address;
    cpu->A = 0x00;
    cpu->memory[zero_page_address] = 0x00;

    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x00);
    assert_true(check_nz_flags(cpu, ZERO, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = zero_page_address;
    cpu->A = 0x03;
    cpu->memory[zero_page_address] = 0x01;

    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x01);
    assert_true(check_nz_flags(cpu, POS, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    return MUNIT_OK;
}

MunitResult Test_AND_ZPX(const MunitParameter params[], void *fixture)
{
    test_fixture_s *test_fixture = (test_fixture_s*)fixture;
    cpu_s *cpu = test_fixture->cpu;
    word_t program_counter = test_fixture->program_counter;
    byte_t opcode = INSTRUCTION_AND_ZPX;
    cpu->memory[program_counter] = opcode;
    instruction_s instruction = cpu->table[opcode];
    //ensure instruction is correct
    assert_int(instruction.name, ==, "AND");
    assert_int(instruction.cycles, ==, 4);
    assert_int(instruction.length, ==, 2);
    assert_int(instruction.fetch, ==, ZPX);
    assert_int(instruction.execute, ==, AND);

    byte_t zero_page_address = 0x80;
    byte_t x = 0x01;

    cpu->memory[RESET_VECTOR] = (byte_t) (program_counter & 0x00FF);
    cpu->memory[RESET_VECTOR + 1] = (byte_t)((program_counter & 0xFF00) >> 8);

    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = zero_page_address;
    cpu->X = x;
    cpu->A = 0x81;
    cpu->memory[zero_page_address + x] = 0x81;

    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x81);
    assert_true(check_nz_flags(cpu, NEG, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = zero_page_address;
    cpu->X = x;
    cpu->A = 0x00;
    cpu->memory[zero_page_address + x] = 0x00;

    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x00);
    assert_true(check_nz_flags(cpu, ZERO, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = zero_page_address;
    cpu->X = x;
    cpu->A = 0x03;
    cpu->memory[zero_page_address + x] = 0x01;

    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x01);
    assert_true(check_nz_flags(cpu, POS, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    //test wrap around
    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = 0xFF;
    cpu->X = 0x01;
    cpu->A = 0x03;
    cpu->memory[0x0000] = 0x01;

    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x01);
    assert_true(check_nz_flags(cpu, POS, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);
    
    return MUNIT_OK;
}

MunitResult Test_AND_ABS(const MunitParameter params[], void *fixture)
{
    test_fixture_s *test_fixture = (test_fixture_s*)fixture;
    cpu_s *cpu = test_fixture->cpu;
    word_t program_counter = test_fixture->program_counter;
    byte_t opcode = INSTRUCTION_AND_ABS;
    cpu->memory[program_counter] = opcode;
    instruction_s instruction = cpu->table[opcode];
    //ensure instruction is correct
    assert_int(instruction.name, ==, "AND");
    assert_int(instruction.cycles, ==, 4);
    assert_int(instruction.length, ==, 3);
    assert_int(instruction.fetch, ==, ABS);
    assert_int(instruction.execute, ==, AND);

    word_t address = 0x8000;

    cpu->memory[RESET_VECTOR] = (byte_t) (program_counter & 0x00FF);
    cpu->memory[RESET_VECTOR + 1] = (byte_t)((program_counter & 0xFF00) >> 8);

    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = (byte_t)(address & 0x00FF);
    cpu->memory[program_counter + 2] = (byte_t)((address & 0xFF00) >> 8);
    cpu->A = 0x81;
    cpu->memory[address] = 0x80;
    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x80);
    assert_true(check_nz_flags(cpu, NEG, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = (byte_t)(address & 0x00FF);
    cpu->memory[program_counter + 2] = (byte_t)((address & 0xFF00) >> 8);
    cpu->A = 0x00;
    cpu->memory[address] = 0x00;
    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x00);
    assert_true(check_nz_flags(cpu, ZERO, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = (byte_t)(address & 0x00FF);
    cpu->memory[program_counter + 2] = (byte_t)((address & 0xFF00) >> 8);
    cpu->A = 0x03;
    cpu->memory[address] = 0x01;
    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x01);
    assert_true(check_nz_flags(cpu, POS, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);
    return MUNIT_OK;
}

MunitResult Test_AND_ABX(const MunitParameter params[], void *fixture)
{
    test_fixture_s *test_fixture = (test_fixture_s*)fixture;
    cpu_s *cpu = test_fixture->cpu;
    word_t program_counter = test_fixture->program_counter;
    byte_t opcode = INSTRUCTION_AND_ABX;
    cpu->memory[program_counter] = opcode;
    instruction_s instruction = cpu->table[opcode];
    //ensure instruction is correct
    assert_int(instruction.name, ==, "AND");
    assert_int(instruction.cycles, ==, 4);
    assert_int(instruction.length, ==, 3);
    assert_int(instruction.fetch, ==, ABX);
    assert_int(instruction.execute, ==, AND);

    word_t address = 0x8000;
    byte_t x = 0x01;

    cpu->memory[RESET_VECTOR] = (byte_t) (program_counter & 0x00FF);
    cpu->memory[RESET_VECTOR + 1] = (byte_t)((program_counter & 0xFF00) >> 8);

    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = (byte_t)(address & 0x00FF);
    cpu->memory[program_counter + 2] = (byte_t)((address & 0xFF00) >> 8);
    cpu->X = x;
    cpu->A = 0x81;
    cpu->memory[address + x] = 0x80;
    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x80);
    assert_true(check_nz_flags(cpu, NEG, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = (byte_t)(address & 0x00FF);
    cpu->memory[program_counter + 2] = (byte_t)((address & 0xFF00) >> 8);
    cpu->X = x;
    cpu->A = 0x00;
    cpu->memory[address + x] = 0x00;
    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x00);
    assert_true(check_nz_flags(cpu, ZERO, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);
    
    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = (byte_t)(address & 0x00FF);
    cpu->memory[program_counter + 2] = (byte_t)((address & 0xFF00) >> 8);
    cpu->X = x;
    cpu->A = 0x03;
    cpu->memory[address + x] = 0x01;
    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x01);
    assert_true(check_nz_flags(cpu, POS, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    //candidate for page boundery cross
    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = 0xFF;
    cpu->memory[program_counter + 2] = 0x00;
    cpu->X = 0x01;
    cpu->A = 0x03;
    cpu->memory[0x0100] = 0x01;
    run(cpu, instruction.cycles);
    //PC should not have changed
    assert_true(cpu->does_need_additional_cycle);
    clock(cpu);
    //PC should have changed
    assert_int(cpu->PC, ==, program_counter + instruction.length);
    assert_int(cpu->A, ==, 0x01);
    assert_true(check_nz_flags(cpu, POS, cpu->STATUS));
    return MUNIT_OK;
}

MunitResult Test_AND_ABY(const MunitParameter params[], void *fixture)
{
    test_fixture_s *test_fixture = (test_fixture_s*)fixture;
    cpu_s *cpu = test_fixture->cpu;
    word_t program_counter = test_fixture->program_counter;
    byte_t opcode = INSTRUCTION_AND_ABY;
    cpu->memory[program_counter] = opcode;
    instruction_s instruction = cpu->table[opcode];
    //ensure instruction is correct
    assert_int(instruction.name, ==, "AND");
    assert_int(instruction.cycles, ==, 4);
    assert_int(instruction.length, ==, 3);
    assert_int(instruction.fetch, ==, ABY);
    assert_int(instruction.execute, ==, AND);

    word_t address = 0x8000;
    byte_t y = 0x01;

    cpu->memory[RESET_VECTOR] = (byte_t) (program_counter & 0x00FF);
    cpu->memory[RESET_VECTOR + 1] = (byte_t)((program_counter & 0xFF00) >> 8);

    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = (byte_t)(address & 0x00FF);
    cpu->memory[program_counter + 2] = (byte_t)((address & 0xFF00) >> 8);
    cpu->Y = y;
    cpu->A = 0x81;
    cpu->memory[address + y] = 0x80;
    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x80);
    assert_true(check_nz_flags(cpu, NEG, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = (byte_t)(address & 0x00FF);
    cpu->memory[program_counter + 2] = (byte_t)((address & 0xFF00) >> 8);
    cpu->Y = y;
    cpu->A = 0x00;
    cpu->memory[address + y] = 0x00;
    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x00);
    assert_true(check_nz_flags(cpu, ZERO, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);
    
    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = (byte_t)(address & 0x00FF);
    cpu->memory[program_counter + 2] = (byte_t)((address & 0xFF00) >> 8);
    cpu->Y = y;
    cpu->A = 0x03;
    cpu->memory[address + y] = 0x01;
    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x01);
    assert_true(check_nz_flags(cpu, POS, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    //candidate for page boundery cross
    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = 0xFF;
    cpu->memory[program_counter + 2] = 0x00;
    cpu->Y = 0x01;
    cpu->A = 0x03;
    cpu->memory[0x0100] = 0x01;
    run(cpu, instruction.cycles);
    //PC should not have changed
    assert_true(cpu->does_need_additional_cycle);
    clock(cpu);
    //PC should have changed
    assert_int(cpu->PC, ==, program_counter + instruction.length);
    assert_int(cpu->A, ==, 0x01);
    assert_true(check_nz_flags(cpu, POS, cpu->STATUS));
    return MUNIT_OK;
}

MunitResult Test_AND_IZX(const MunitParameter params[], void *fixture)
{
    //unimplemented
    assert_true(false);
    return MUNIT_OK;
}

MunitResult Test_AND_IZY(const MunitParameter params[], void *fixture)
{
    //unimplemented
    assert_true(false);
    return MUNIT_OK;
}