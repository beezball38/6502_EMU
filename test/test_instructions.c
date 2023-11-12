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



typedef struct test_fixture
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

typedef struct instruction_init
{
    cpu_s *cpu;
    cpu_register_t active_register;
    cpu_addr_mode_t addr_mode;
    word_t instruction_addr;
    byte_t opcode;
    word_t address; 
    word_t pointer_address; //for indirect addressing modes
    sign_t sign; //for testing positive, negative, and zero cases
    byte_t reg_offset;
    byte_t cycles;
} instruction_init_s;

static void* setup(const MunitParameter params[], void *user_data);
static void tear_down(void *fixture);
static void run(cpu_s *cpu, size_t cycles);
static void instruction_init(instruction_init_s *args);
static void and_test(instruction_init_s *args, byte_t expected);
static void clear_test(instruction_init_s *args, cpu_status_t flag);
static void set_test(instruction_init_s *args, cpu_status_t flag);
static void lda_test(instruction_init_s *args, byte_t expected);
static byte_t get_test_val(sign_t sign);
static void load_reset_vector(cpu_s *cpu, word_t program_counter);
static bool check_nz_flags(cpu_s *cpu, sign_t sign, byte_t old_status);
static byte_t get_test_val(sign_t sign)
{
    switch(sign)
    {
        case NEG:
            return 0x80;
        case ZERO:
            return 0x00;
        case POS:
            return 0x01;
        default:
            return 0x00;
    }
}

static void load_reset_vector(cpu_s *cpu, word_t program_counter)
{
    cpu->memory[RESET_VECTOR] = (byte_t) (program_counter & 0x00FF);
    cpu->memory[RESET_VECTOR + 1] = (byte_t)((program_counter & 0xFF00) >> 8);
}

static void and_test(instruction_init_s *args, byte_t expected)
{
    reset(args->cpu);
    instruction_init(args);
    args->cpu->A = get_test_val(args->sign);  
    run(args->cpu, args->cycles);
    assert_int(args->cpu->A, ==, expected);
    assert_true(check_nz_flags(args->cpu, args->sign, args->cpu->STATUS));
}

static void clear_test(instruction_init_s *args, cpu_status_t flag)
{
    reset(args->cpu);
    instruction_init(args);
    run(args->cpu, args->cycles);
    assert_false(args->cpu->STATUS & flag);
}

static void set_test(instruction_init_s *args, cpu_status_t flag)
{
    reset(args->cpu);
    instruction_init(args);
    run(args->cpu, args->cycles);
    assert_true(args->cpu->STATUS & flag);
}

static void lda_test(instruction_init_s *args, byte_t expected)
{
    reset(args->cpu);
    instruction_init(args);
    run(args->cpu, args->cycles);
    assert_int(args->cpu->A, ==, expected);
    assert_true(check_nz_flags(args->cpu, args->sign, args->cpu->STATUS));
}

static void instruction_init(instruction_init_s *args)
{
    cpu_s *cpu = args->cpu;
    cpu_addr_mode_t addr_mode = args->addr_mode;
    word_t instruction_addr = args->instruction_addr;

    switch(addr_mode)
    {
        case ADDR_MODE_IMP:
            cpu->memory[instruction_addr] = args->opcode;
            break;
        case ADDR_MODE_IMM:
            cpu->memory[instruction_addr] = args->opcode;
            cpu->memory[instruction_addr + 1] = get_test_val(args->sign);
            break; 
        case ADDR_MODE_ZP0:
            cpu->memory[instruction_addr] = args->opcode;
            byte_t zero_page_address = (byte_t)(args->address & 0x00FF);
            cpu->memory[instruction_addr + 1] = zero_page_address;
            cpu->memory[zero_page_address] = get_test_val(args->sign); 
            break;
        case ADDR_MODE_ZPX:
            cpu->memory[instruction_addr] = args->opcode;
            byte_t zero_page_address_x = (byte_t)(args->address & 0x00FF);
            cpu->memory[instruction_addr + 1] = zero_page_address_x;
            cpu->X = args->reg_offset;
            cpu->memory[zero_page_address_x + args->reg_offset] = get_test_val(args->sign);
            break;
        case ADDR_MODE_ABS:
            cpu->memory[instruction_addr] = args->opcode;
            cpu->memory[instruction_addr + 1] = (byte_t)(args->address & 0x00FF);
            cpu->memory[instruction_addr + 2] = (byte_t)((args->address & 0xFF00) >> 8);
            cpu->memory[args->address] = get_test_val(args->sign);
            break;
        case ADDR_MODE_ABX:
            cpu->memory[instruction_addr] = args->opcode;
            cpu->memory[instruction_addr + 1] = (byte_t)(args->address & 0x00FF);
            cpu->memory[instruction_addr + 2] = (byte_t)((args->address & 0xFF00) >> 8);
            cpu->X = args->reg_offset;
            cpu->memory[args->address + args->reg_offset] = get_test_val(args->sign);
            break;
        case ADDR_MODE_ABY:
            cpu->memory[instruction_addr] = args->opcode;
            cpu->memory[instruction_addr + 1] = (byte_t)(args->address & 0x00FF);
            cpu->memory[instruction_addr + 2] = (byte_t)((args->address & 0xFF00) >> 8);
            cpu->Y = args->reg_offset;
            cpu->memory[args->address + args->reg_offset] = get_test_val(args->sign);
            break;
        case ADDR_MODE_IND:
            //account for page boundery bug
            cpu->memory[instruction_addr] = args->opcode;
            cpu->memory[instruction_addr + 1] = (byte_t)(args->pointer_address & 0x00FF);
            cpu->memory[instruction_addr + 2] = (byte_t)((args->pointer_address & 0xFF00) >> 8);
            cpu->memory[args->pointer_address] = (byte_t)(args->address & 0x00FF);
            cpu->memory[args->pointer_address + 1] = (byte_t)((args->address & 0xFF00) >> 8);
            //was a page boundery crossed?
            if ((args->pointer_address & 0xFF00) != ((args->pointer_address + 1) & 0xFF00))
            {
                cpu->memory[args->pointer_address] = (byte_t)(args->address & 0x00FF);
                cpu->memory[args->pointer_address - 0x0100 + 1] = (byte_t)((args->address & 0xFF00) >> 8);
            }
            break;
        case ADDR_MODE_IZX:
            cpu->memory[instruction_addr] = args->opcode;
            byte_t zero_page_address_izx = (byte_t)(args->pointer_address & 0x00FF);
            cpu->memory[instruction_addr + 1] = zero_page_address_izx;
            cpu->X = args->reg_offset;
            cpu->memory[zero_page_address_izx + args->reg_offset] = (byte_t)(args->address & 0x00FF);
            cpu->memory[zero_page_address_izx + args->reg_offset + 1] = (byte_t)((args->address & 0xFF00) >> 8);
            cpu->memory[args->address] = get_test_val(args->sign);
            break;
    }      
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
        {
            "/AND IZX",
            Test_AND_IZX,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/AND IZY",
            Test_AND_IZY,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/CLC IMP",
            Test_CLC_IMP,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/CLI IMP",
            Test_CLI_IMP,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/CLV IMP",
            Test_CLV_IMP,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/CLD IMP",
            Test_CLD_IMP,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/SEC IMP",
            Test_SEC_IMP,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/SEI IMP",
            Test_SEI_IMP,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/SED IMP",
            Test_SED_IMP,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/LDA IMM",
            Test_LDA_IMM,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/LDA ZP0",
            Test_LDA_ZP0,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/LDA ZPX",
            Test_LDA_ZPX,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/LDA ABS",
            Test_LDA_ABS,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/LDA ABX",
            Test_LDA_ABX,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/LDA ABY",
            Test_LDA_ABY,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/LDA IZX",
            Test_LDA_IZX,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
        },
        {
            "/LDA IZY",
            Test_LDA_IZY,
            setup,
            tear_down,
            MUNIT_TEST_OPTION_NONE,
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
    test_fixture_s *test_fixture = (test_fixture_s*)fixture;
    cpu_s *cpu = test_fixture->cpu;
    word_t program_counter = test_fixture->program_counter;
    byte_t opcode = INSTRUCTION_AND_IMM;
    cpu->memory[program_counter] = opcode;
    cpu_instruction_s instruction = cpu->table[opcode];

    assert_int(instruction.name, ==, "AND");
    assert_int(instruction.cycles, ==, 2);
    assert_int(instruction.length, ==, 2);
    assert_int(instruction.fetch, ==, IMM);
    assert_int(instruction.execute, ==, AND);

    load_reset_vector(cpu, program_counter);
    instruction_init_s args = 
    {
        .cpu = cpu,
        .addr_mode = ADDR_MODE_IMM,
        .instruction_addr = program_counter,
        .opcode = opcode,
        .cycles = instruction.cycles,
        .sign = NEG
    };

    reset(cpu);
    and_test(&args, 0x80);
    assert_true(check_nz_flags(cpu, NEG, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    reset(cpu);
    args.sign = ZERO;
    and_test(&args, 0x00);
    assert_true(check_nz_flags(cpu, ZERO, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    reset(cpu);
    args.sign = POS;
    and_test(&args, 0x01);
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
    
    cpu_instruction_s instruction = cpu->table[opcode];
    byte_t zero_page_address = 0x80;
    instruction_init_s args = 
    {
        .cpu = cpu,
        .addr_mode = ADDR_MODE_ZP0,
        .instruction_addr = program_counter,
        .opcode = opcode,
        .address = zero_page_address,
        .cycles = instruction.cycles,
        .sign = NEG
    };
    
    assert_int(instruction.name, ==, "AND");
    assert_int(instruction.cycles, ==, 3);
    assert_int(instruction.length, ==, 2);
    assert_int(instruction.fetch, ==, ZP0);
    assert_int(instruction.execute, ==, AND);

    load_reset_vector(cpu, program_counter); 
    
    reset(cpu);
    and_test(&args, 0x80);
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    reset(cpu);
    args.sign = ZERO;
    and_test(&args, 0x00);
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    reset(cpu);
    args.sign = POS;
    and_test(&args, 0x01);
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
    cpu_instruction_s instruction = cpu->table[opcode];
    byte_t zero_page_address = 0x80;
    byte_t x = 0x01;
    //ensure instruction is correct
    assert_int(instruction.name, ==, "AND");
    assert_int(instruction.cycles, ==, 4);
    assert_int(instruction.length, ==, 2);
    assert_int(instruction.fetch, ==, ZPX);
    assert_int(instruction.execute, ==, AND);


    load_reset_vector(cpu, program_counter);
    instruction_init_s args = 
    {
        .cpu = cpu,
        .addr_mode = ADDR_MODE_ZPX,
        .instruction_addr = program_counter,
        .opcode = opcode,
        .address = zero_page_address,
        .reg_offset = x,
        .cycles = instruction.cycles,
        .sign = NEG
    };
    reset(cpu);
    and_test(&args, 0x80);
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    reset(cpu);
    args.sign = ZERO;
    and_test(&args, 0x00);
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    reset(cpu);
    args.sign = POS;
    and_test(&args, 0x01);
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    //test wrap around edge case
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
    cpu_instruction_s instruction = cpu->table[opcode];
    word_t address = 0x8000;
    instruction_init_s args = 
    {
        .cpu = cpu,
        .addr_mode = ADDR_MODE_ABS,
        .instruction_addr = program_counter,
        .opcode = opcode,
        .address = address,
        .cycles = instruction.cycles,   
        .sign = NEG
    };
    //ensure instruction is correct
    assert_int(instruction.name, ==, "AND");
    assert_int(instruction.cycles, ==, 4);
    assert_int(instruction.length, ==, 3);
    assert_int(instruction.fetch, ==, ABS);
    assert_int(instruction.execute, ==, AND);

    load_reset_vector(cpu, program_counter);

    reset(cpu);
    and_test(&args, 0x80);
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    reset(cpu);
    args.sign = ZERO;
    and_test(&args, 0x00);
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    reset(cpu);
    args.sign = POS;
    and_test(&args, 0x01);
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
    cpu_instruction_s instruction = cpu->table[opcode];
    word_t address = 0x8000;
    byte_t x = 0x01;
    instruction_init_s args = 
    {
        .cpu = cpu,
        .addr_mode = ADDR_MODE_ABX,
        .instruction_addr = program_counter,
        .opcode = opcode,
        .address = address,
        .reg_offset = x,
        .cycles = instruction.cycles,
        .sign = NEG
    };
    //ensure instruction is correct
    assert_int(instruction.name, ==, "AND");
    assert_int(instruction.cycles, ==, 4);
    assert_int(instruction.length, ==, 3);
    assert_int(instruction.fetch, ==, ABX);
    assert_int(instruction.execute, ==, AND);

    load_reset_vector(cpu, program_counter);
    and_test(&args, 0x80);
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    reset(cpu);
    args.sign = ZERO;   
    and_test(&args, 0x00);
    assert_int(cpu->PC, ==, program_counter + instruction.length);
    
    reset(cpu);
    args.sign = POS;
    and_test(&args, 0x01);
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
    cpu_instruction_s instruction = cpu->table[opcode];
    word_t address = 0x8000;
    byte_t y= 0x01;
    instruction_init_s args = 
    {
        .cpu = cpu,
        .addr_mode = ADDR_MODE_ABY,
        .instruction_addr = program_counter,
        .opcode = opcode,
        .address = address,
        .reg_offset = y,
        .cycles = instruction.cycles,
        .sign = NEG
    };
    //ensure instruction is correct
    assert_int(instruction.name, ==, "AND");
    assert_int(instruction.cycles, ==, 4);
    assert_int(instruction.length, ==, 3);
    assert_int(instruction.fetch, ==, ABY);
    assert_int(instruction.execute, ==, AND);


    load_reset_vector(cpu, program_counter);
    and_test(&args, 0x80);
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    args.sign = ZERO;   
    and_test(&args, 0x00);
    assert_int(cpu->PC, ==, program_counter + instruction.length);
    
    args.sign = POS;
    and_test(&args, 0x01);
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
    return MUNIT_SKIP;
}

MunitResult Test_AND_IZY(const MunitParameter params[], void *fixture)
{
    return MUNIT_SKIP;
}

MunitResult Test_CLC_IMP(const MunitParameter params[], void *fixture)
{
    test_fixture_s *test_fixture = (test_fixture_s*)fixture;
    cpu_s *cpu = test_fixture->cpu;
    word_t program_counter = test_fixture->program_counter;
    byte_t opcode = INSTRUCTION_CLC_IMP;
    cpu_instruction_s instruction = cpu->table[opcode];
    //ensure instruction is correct
    assert_int(instruction.name, ==, "CLC");
    assert_int(instruction.cycles, ==, 2);
    assert_int(instruction.length, ==, 1);
    assert_int(instruction.fetch, ==, IMP);
    assert_int(instruction.execute, ==, CLC);

    load_reset_vector(cpu, program_counter);    
    instruction_init_s args = 
    {
        .cpu = cpu,
        .addr_mode = ADDR_MODE_IMP,
        .instruction_addr = program_counter,
        .opcode = INSTRUCTION_CLC_IMP,
    };
    reset(cpu);
    instruction_init(&args);
    cpu->STATUS = 0x00 | U;
    set_flag(cpu, C, true);
    assert_true(cpu->STATUS & C);
    run(cpu, instruction.cycles);
    assert_false(cpu->STATUS & C);
    assert_int(cpu->PC, ==, program_counter + instruction.length);
    return MUNIT_OK;
}

MunitResult Test_CLI_IMP(const MunitParameter params[], void *fixture)
{
    test_fixture_s *test_fixture = (test_fixture_s*)fixture;
    cpu_s *cpu = test_fixture->cpu;
    word_t program_counter = test_fixture->program_counter;
    byte_t opcode = INSTRUCTION_CLI_IMP;
    cpu_instruction_s instruction = cpu->table[opcode];
    //ensure instruction is correct
    assert_int(instruction.name, ==, "CLI");
    assert_int(instruction.cycles, ==, 2);
    assert_int(instruction.length, ==, 1);
    assert_int(instruction.fetch, ==, IMP);
    assert_int(instruction.execute, ==, CLI);

    load_reset_vector(cpu, program_counter);    
    instruction_init_s args = 
    {
        .cpu = cpu,
        .addr_mode = ADDR_MODE_IMP,
        .instruction_addr = program_counter,
        .opcode = INSTRUCTION_CLI_IMP,
    };
    reset(cpu);
    instruction_init(&args);
    cpu->STATUS = 0x00 | U;
    set_flag(cpu, I, true);
    assert_true(cpu->STATUS & I);
    run(cpu, instruction.cycles);
    assert_false(cpu->STATUS & I);
    assert_int(cpu->PC, ==, program_counter + instruction.length);
    return MUNIT_OK;
}

MunitResult Test_CLV_IMP(const MunitParameter params[], void *fixture)
{
    test_fixture_s *test_fixture = (test_fixture_s*)fixture;
    cpu_s *cpu = test_fixture->cpu;
    word_t program_counter = test_fixture->program_counter;
    byte_t opcode = INSTRUCTION_CLV_IMP;
    cpu_instruction_s instruction = cpu->table[opcode];
    instruction_init_s args = 
    {
        .cpu = cpu,
        .addr_mode = ADDR_MODE_IMP,
        .instruction_addr = program_counter,
        .cycles = instruction.cycles,
        .opcode = INSTRUCTION_CLV_IMP,
    };
    //ensure instruction is correct
    assert_int(instruction.name, ==, "CLV");
    assert_int(instruction.cycles, ==, 2);
    assert_int(instruction.length, ==, 1);
    assert_int(instruction.fetch, ==, IMP);
    assert_int(instruction.execute, ==, CLV);

    load_reset_vector(cpu, program_counter);    
    clear_test(&args, V);
    assert_int(cpu->PC, ==, program_counter + instruction.length);
    return MUNIT_OK;
}

MunitResult Test_CLD_IMP(const MunitParameter params[], void *fixture)
{
    test_fixture_s *test_fixture = (test_fixture_s*)fixture;
    cpu_s *cpu = test_fixture->cpu;
    word_t program_counter = test_fixture->program_counter;
    byte_t opcode = INSTRUCTION_CLD_IMP;
    cpu_instruction_s instruction = cpu->table[opcode];
    //ensure instruction is correct
    assert_int(instruction.name, ==, "CLD");
    assert_int(instruction.cycles, ==, 2);
    assert_int(instruction.length, ==, 1);
    assert_int(instruction.fetch, ==, IMP);
    assert_int(instruction.execute, ==, CLD);

    load_reset_vector(cpu, program_counter);    
    instruction_init_s args = 
    {
        .cpu = cpu,
        .addr_mode = ADDR_MODE_IMP,
        .instruction_addr = program_counter,
        .cycles = instruction.cycles,
        .opcode = INSTRUCTION_CLD_IMP,
    };
    clear_test(&args, D);
    assert_int(cpu->PC, ==, program_counter + instruction.length);
    return MUNIT_OK;
}

MunitResult Test_SEC_IMP(const MunitParameter params[], void *fixture)
{
    test_fixture_s *test_fixture = (test_fixture_s*)fixture;
    cpu_s *cpu = test_fixture->cpu;
    word_t program_counter = test_fixture->program_counter;
    byte_t opcode = INSTRUCTION_SEC_IMP;

    cpu_instruction_s instruction = cpu->table[opcode];
    assert_int(instruction.name, ==, "SEC"); 
    assert_int(instruction.cycles, ==, 2);
    assert_int(instruction.length, ==, 1);
    assert_int(instruction.fetch, ==, IMP);
    assert_int(instruction.execute, ==, SEC);

    load_reset_vector(cpu, program_counter);
    instruction_init_s args = 
    {
        .cpu = cpu,
        .addr_mode = ADDR_MODE_IMP,
        .instruction_addr = program_counter,
        .cycles = instruction.cycles,
        .opcode = opcode,
    };
    set_test(&args, C);
    assert_int(cpu->PC, ==, program_counter + instruction.length);
    return MUNIT_OK;
}

MunitResult Test_SEI_IMP(const MunitParameter params[], void *fixture)
{
    test_fixture_s *test_fixture = (test_fixture_s*)fixture;
    cpu_s *cpu = test_fixture->cpu;
    word_t program_counter = test_fixture->program_counter;
    byte_t opcode = INSTRUCTION_SEI_IMP;

    cpu_instruction_s instruction = cpu->table[opcode];
    assert_int(instruction.name, ==, "SEI"); 
    assert_int(instruction.cycles, ==, 2);
    assert_int(instruction.length, ==, 1);
    assert_int(instruction.fetch, ==, IMP);
    assert_int(instruction.execute, ==, SEI);

    load_reset_vector(cpu, program_counter);
    instruction_init_s args = 
    {
        .cpu = cpu,
        .addr_mode = ADDR_MODE_IMP,
        .instruction_addr = program_counter,
        .cycles = instruction.cycles,
        .opcode = opcode,
    };
    set_test(&args, I);
    assert_int(cpu->PC, ==, program_counter + instruction.length);
    return MUNIT_OK;
}

MunitResult Test_SED_IMP(const MunitParameter params[], void *fixture)
{
    test_fixture_s *test_fixture = (test_fixture_s*)fixture;
    cpu_s *cpu = test_fixture->cpu;
    word_t program_counter = test_fixture->program_counter;
    byte_t opcode = INSTRUCTION_SED_IMP;

    cpu_instruction_s instruction = cpu->table[opcode];
    assert_int(instruction.name, ==, "SED"); 
    assert_int(instruction.cycles, ==, 2);
    assert_int(instruction.length, ==, 1);
    assert_int(instruction.fetch, ==, IMP);
    assert_int(instruction.execute, ==, SED);

    load_reset_vector(cpu, program_counter);
    instruction_init_s args = 
    {
        .cpu = cpu,
        .addr_mode = ADDR_MODE_IMP,
        .instruction_addr = program_counter,
        .cycles = instruction.cycles,
        .opcode = opcode,
    };
    set_test(&args, D);
    assert_int(cpu->PC, ==, program_counter + instruction.length);
    return MUNIT_OK;
}

MunitResult Test_LDA_IMM(const MunitParameter params[], void *fixture)
{
    test_fixture_s *test_fixture = (test_fixture_s*)fixture;
    cpu_s *cpu = test_fixture->cpu;
    word_t program_counter = test_fixture->program_counter;

    byte_t opcode = INSTRUCTION_LDA_IMM;
    cpu_instruction_s instruction = cpu->table[opcode];
    assert_int(instruction.name, ==, "LDA");
    assert_int(instruction.cycles, ==, 2);
    assert_int(instruction.length, ==, 2);
    assert_int(instruction.fetch, ==, IMM);
    assert_int(instruction.execute, ==, LDA);

    load_reset_vector(cpu, program_counter);
    instruction_init_s args = 
    {
        .cpu = cpu,
        .addr_mode = ADDR_MODE_IMM,
        .instruction_addr = program_counter,
        .opcode = opcode,
        .cycles = instruction.cycles,
        .sign = NEG
    };
    reset(cpu);
    assert_int(cpu->A, ==, 0x00);
    instruction_init(&args);
    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x80);
    assert_true(check_nz_flags(cpu, NEG, cpu->STATUS));

    args.sign = ZERO;
    reset(cpu);
    assert_int(cpu->A, ==, 0x00);
    instruction_init(&args);
    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x00);
    assert_true(check_nz_flags(cpu, ZERO, cpu->STATUS));

    args.sign = POS;
    reset(cpu);
    assert_int(cpu->A, ==, 0x00);
    instruction_init(&args);
    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x01);
    assert_true(check_nz_flags(cpu, POS, cpu->STATUS));
    return MUNIT_OK;
}

MunitResult Test_LDA_ZP0(const MunitParameter params[], void *fixture)
{
    test_fixture_s *test_fixture = (test_fixture_s*)fixture;
    cpu_s *cpu = test_fixture->cpu;
    word_t program_counter = test_fixture->program_counter;

    byte_t opcode = INSTRUCTION_LDA_ZP0;
    cpu_instruction_s instruction = cpu->table[opcode];
    byte_t zero_page_address = 0x80;
    instruction_init_s args = 
    {
        .cpu = cpu,
        .addr_mode = ADDR_MODE_ZP0,
        .instruction_addr = program_counter,
        .opcode = opcode,
        .address = zero_page_address,
        .cycles = instruction.cycles,
        .sign = NEG
    };
    assert_int(instruction.name, ==, "LDA");
    assert_int(instruction.cycles, ==, 3);
    assert_int(instruction.length, ==, 2);
    assert_int(instruction.fetch, ==, ZP0);
    assert_int(instruction.execute, ==, LDA);
    
    load_reset_vector(cpu, program_counter);
    lda_test(&args, 0x80);
    assert_true(check_nz_flags(cpu, NEG, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    args.sign = ZERO;
    lda_test(&args, 0x00);
    assert_true(check_nz_flags(cpu, ZERO, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    args.sign = POS;
    lda_test(&args, 0x01);
    assert_true(check_nz_flags(cpu, POS, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    return MUNIT_OK;
}

MunitResult Test_LDA_ZPX(const MunitParameter params[], void *fixture)
{
    test_fixture_s *test_fixture = (test_fixture_s*)fixture;
    cpu_s *cpu = test_fixture->cpu;
    word_t program_counter= test_fixture->program_counter;

    byte_t opcode = INSTRUCTION_LDA_ZPX;
    cpu_instruction_s instruction = cpu->table[opcode];
    byte_t zero_page_address = 0x80;
    byte_t x = 0x01;
    instruction_init_s args = 
    {
        .cpu = cpu,
        .addr_mode = ADDR_MODE_ZPX,
        .instruction_addr = program_counter,
        .opcode = opcode,
        .address = zero_page_address,
        .reg_offset = x,
        .cycles = instruction.cycles,
        .sign = NEG
    };
    assert_int(instruction.name, ==, "LDA");
    assert_int(instruction.cycles, ==, 4);
    assert_int(instruction.length, ==, 2);
    assert_int(instruction.fetch, ==, ZPX);
    assert_int(instruction.execute, ==, LDA);

    load_reset_vector(cpu, program_counter);
    lda_test(&args, 0x80);
    assert_true(check_nz_flags(cpu, NEG, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    args.sign = ZERO;
    lda_test(&args, 0x00);
    assert_true(check_nz_flags(cpu, ZERO, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    args.sign = POS;
    lda_test(&args, 0x01);
    assert_true(check_nz_flags(cpu, POS, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    //test wrap around edge case    
    reset(cpu);
    cpu->memory[program_counter] = opcode;
    cpu->memory[program_counter + 1] = 0xFF;
    cpu->X = 0x01;
    cpu->A = 0x03;
    cpu->memory[0x00e00] = 0x01;

    run(cpu, instruction.cycles);
    assert_int(cpu->A, ==, 0x01);
    assert_true(check_nz_flags(cpu, POS, cpu->STATUS));
    assert_int(cpu->PC, ==, program_counter + instruction.length);

    return MUNIT_OK;
}

MunitResult Test_LDA_ABS(const MunitParameter params[], void *fixture)
{
    test_fixture_s *test_fixture = (test_fixture_s*)fixture;
    cpu_s *cpu= test_fixture->cpu;
    word_t program_counter = test_fixture->program_counter;

    byte_t opcode = INSTRUCTION_LDA_ABS;
    cpu_instruction_s instruction = cpu->table[opcode];
    word_t address = 0x8000;
    instruction_init_s args = 
    {
        .cpu = cpu,
        .addr_mode = ADDR_MODE_ABS,
        .instruction_addr = program_counter,
        .opcode = opcode,
        .address = address,
        .cycles = instruction.cycles,
        .sign = NEG
    };

    assert_int(instruction.name, ==, "LDA");
    assert_int(instruction.cycles, ==, 4);
    assert_int(instruction.length, ==, 3);
    assert_int(instruction.fetch, ==, ABS);
    assert_int(instruction.execute, ==, LDA);

    load_reset_vector(cpu, program_counter);
    lda_test(&args, 0x80);
    assert_true(check_nz_flags(cpu, NEG, cpu->STATUS));

    args.sign = ZERO;
    lda_test(&args, 0x00);
    assert_true(check_nz_flags(cpu, ZERO, cpu->STATUS));

    args.sign = POS;
    lda_test(&args, 0x01);
    assert_true(check_nz_flags(cpu, POS, cpu->STATUS));

    return MUNIT_SKIP;
}

MunitResult Test_LDA_ABX(const MunitParameter params[], void *fixture)
{
    return MUNIT_SKIP;
}

MunitResult Test_LDA_ABY(const MunitParameter params[], void *fixture)
{
    return MUNIT_SKIP;
}

MunitResult Test_LDA_IZX(const MunitParameter params[], void *fixture)
{
    return MUNIT_SKIP;
}

MunitResult Test_LDA_IZY(const MunitParameter params[], void *fixture)
{
    return MUNIT_SKIP;
}
