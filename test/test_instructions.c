#include <stdio.h>
#include <stdbool.h>
#include "munit/munit.h"
#include "../src/cpu.h"

// 64 kb memory
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

//main function for munit tests
//documentation https://nemequ.github.io/munit/

//function to get an initialized cpu with memory
CPU *cpu_create()
{
    //create cpu and memory
    CPU *cpu = malloc(sizeof(CPU));
    Byte *memory = malloc(MEM_SIZE);

    //initialize cpu and memory
    init(cpu, memory);

    return cpu;
}




int main(int argc, char *argv[])
{
    //Munit test array for first instruction only
    //test is for AND IMM
    MunitTest tests[] = {
        { "/AND/IMM", Test_AND_IMM, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
    };
    //create test suite with this one test
    MunitSuite suite = {
        "/AND/IMM", //name
        tests, //tests
        NULL, //suites
        1, //iterations
        MUNIT_SUITE_OPTION_NONE //options
    };
    //run
    return munit_suite_main(&suite, NULL, argc, argv);
}

//test for AND IMM
MunitResult Test_AND_IMM(const MunitParameter params[], void *fixture)
{
    //create cpu and memory
    CPU *cpu = cpu_create();

    //set up memory
    //why is this at the end of memory?
    //0xFFFC is the reset vector
    //0xFFFD is the reset vector + 1
    //0x0042 is the address of the AND IMM instruction
    cpu->memory[0xFFFC] = 0x42;
    cpu->memory[0xFFFD] = 0x00;
    cpu->memory[0x0042] = INSTRUCTION_AND_IMM;
    //reset cpu and check program counter
    reset(cpu);
    munit_assert_int(cpu->PC, ==, 0x0042);

    //run cpu
    run(cpu, 2);

    //check that A is 0x42
    munit_assert_int(cpu->A, ==, 0x42);

    //check that N and Z flags are set
    munit_assert_true(check_nz_flags(cpu, NEG, 0x00));

    //free cpu and memory
    free(cpu->memory);
    free(cpu);

    return MUNIT_OK;
}

