#define MUNIT_ENABLE_ASSERT_ALIASES

#include <stdio.h>
#include <stdbool.h>
#include "munit/munit.h"
#include "../src/cpu.h"

// 64 kb memory
#define RESET_VECTOR 0xFFFC
#define MEM_SIZE 1024 * 64

//instruction test suite prototypes
#define X(instruction) void Test_##instruction(CPU *cpu, Byte *memory);
LIST_OF_INSTRUCTIONS
#undef X

typedef enum
{
    NEG,
    ZERO,
    POS
} Result_Sign;

//test prototypes
#define TEST_LIST \
    X(AND, IMM) \
    X(AND, ZP0) \
    X(AND, ZPX) \
    X(AND, ABS) \
    X(AND, ABX) \
    X(AND, ABY) \
    X(AND, IZX) \
    X(AND, IZY) \
    X(CLC, IMP) \
    X(LDA, IMM) \
    X(LDA, ZP0) \
    X(LDA, ZPX) \
    X(LDA, ABS) \
    X(LDA, ABX) \
    X(LDA, ABY) \
    X(LDA, IZX) \
    X(LDA, IZY) \
    X(ORA, IMM) \
    X(ORA, ZP0) \
    X(ORA, ZPX) \
    X(ORA, ABS) \
    X(ORA, ABX) \
    X(ORA, ABY) \
    X(ORA, IZX) \
    X(ORA, IZY) \
    X(PHA, IMP) \
    X(PHP, IMP) \



//xmacro for munit tests
#define X(instruction, mode) MunitResult Test_##instruction##_##mode(const MunitParameter params[], void *fixture);
TEST_LIST
#undef X

typedef struct
{
    CPU *cpu;
    Word program_counter //start of instruction
} Test_Fixture;

CPU *cpu_create()
{
    CPU *cpu = malloc(sizeof(CPU));
    Byte *memory = malloc(MEM_SIZE);

    cpu_init(cpu, memory);

    return cpu;
}

static void* setup(const MunitParameter params[], void *user_data)
{
    CPU *cpu = cpu_create();
    Test_Fixture *fixture = malloc(sizeof(Test_Fixture));
    fixture->cpu = cpu;
    //random program counter in address space (like an NES program)
    fixture->program_counter = rand() % MEM_SIZE;
    return fixture;
}

static void tear_down(void *fixture)
{
    Test_Fixture *test_fixture = (Test_Fixture*)fixture;
    free(test_fixture->cpu->memory);
    free(test_fixture->cpu);
    free(test_fixture);
}

static bool check_nz_flags(CPU *cpu, Result_Sign sign, Byte old_status)
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

static void run(CPU *cpu, size_t cycles)
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
            "Instructions", //name
            Test_AND_IMM, //test
            setup, //setup
            tear_down, //tear down
            MUNIT_TEST_OPTION_NONE, //options
        },
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
    Test_Fixture *test_fixture = (Test_Fixture*)fixture;
    CPU *cpu = test_fixture->cpu;
    Word program_counter = test_fixture->program_counter;
    Byte opcode = INSTRUCTION_AND_IMM;

    //set up reset vector
    cpu->memory[RESET_VECTOR] = (Byte)program_counter;
    cpu->memory[RESET_VECTOR + 1] = (Byte)(program_counter >> 8);

    //set up instruction for negative case
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = 0x80;
    cpu->A = 0x81;
    cpu->STATUS = rand() % 256 & ~(N | Z) | U;

    //run
    run(cpu, 2);
    assert_int(cpu->A, ==, 0x80);
    assert_true(check_nz_flags(cpu, NEG, cpu->STATUS));
    //program counter should be incremented by length of instruction
    assert_int(cpu->PC, ==, program_counter + 2);
    return MUNIT_OK;
}

