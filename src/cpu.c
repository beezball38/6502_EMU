#include "cpu.h"
#include "bus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define UNIMPLEMENTED()                                                                   \
    fprintf(stderr, "%s:%d: %s: Unimplemented function\n", __FILE__, __LINE__, __func__); \
    abort();

#define MEM_SIZE (1024 * 1024 * 64)

/// @brief Stub for illegal/undocumented opcodes - logs warning and continues
byte_t ILLEGAL(cpu_s *cpu)
{
    fprintf(stderr, "WARNING: Illegal opcode 0x%02X at PC=0x%04X\n",
            cpu->current_opcode, cpu->PC - 1);
    return 0;
}

bool is_illegal_opcode(cpu_s *cpu, byte_t opcode)
{
    cpu_instruction_s *instr = get_instruction(cpu, opcode);
    return instr->execute == ILLEGAL;
}

word_t address;                    // used by absolute, zero page, and indirect addressing modes
offset_t address_rel;              // used only by branch instructions (signed for negative offsets)
byte_t value;                      // holds fetched value, could be immediate value or value from memory
byte_t current_instruction_length; // used by instructions to determine how many bytes to consume
bool acc_mode;                     // true when using accumulator addressing mode

/// @brief Function where all CPU instructions are defined
/// @param cpu
static void init_instruction_table(cpu_s *cpu)
{
    cpu_instruction_s *table = &cpu->table[0];

    // reference https://www.masswerk.at/6502/6502_instruction_set.html
    table[INSTRUCTION_BRK_IMP] = (cpu_instruction_s)
    {
        .name = "BRK",
        .opcode = 0x00,
        .cycles = 7,
        .length = 2, // this is actually 1, but we need to account for the dummy byte
        .data_fetch = IMP,
        .execute = BRK,
    };
    table[INSTRUCTION_ORA_IZX] = (cpu_instruction_s)
    {
        .name = "ORA",
        .opcode = 0x01,
        .cycles = 6,
        .length = 2,
        .data_fetch = IZX,
        .execute = ORA,
    };
    table[OPCODE_UNDEFINED_0x02] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x02,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x03] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x03,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x04] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x04,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_ORA_ZP0] = (cpu_instruction_s)
    {
        .name = "ORA",
        .opcode = 0x05,
        .cycles = 3,
        .length = 2,
        .data_fetch = ZP0,
        .execute = ORA,
    };
    table[INSTRUCTION_ASL_ZP0] = (cpu_instruction_s)
    {
        .name = "ASL",
        .opcode = 0x06,
        .cycles = 5,
        .length = 2,
        .data_fetch = ZP0,
        .execute = ASL,
    };
    table[OPCODE_UNDEFINED_0x07] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x07,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_PHP_IMP] = (cpu_instruction_s)
    {
        .name = "PHP",
        .opcode = 0x08,
        .cycles = 3,
        .length = 1,
        .data_fetch = IMP,
        .execute = PHP,
    };
    table[INSTRUCTION_ORA_IMM] = (cpu_instruction_s)
    {
        .name = "ORA",
        .opcode = 0x09,
        .cycles = 2,
        .length = 2,
        .data_fetch = IMM,
        .execute = ORA,
    };
    table[INSTRUCTION_ASL_ACC] = (cpu_instruction_s)
    {
        .name = "ASL",
        .opcode = 0x0A,
        .cycles = 2,
        .length = 1,
        .data_fetch = ACC,
        .execute = ASL,
    };
    table[OPCODE_UNDEFINED_0x0B] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x0B,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x0C] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x0C,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_ORA_ABS] = (cpu_instruction_s)
    {
        .name = "ORA",
        .opcode = 0x0D,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABS,
        .execute = ORA,
    };
    table[INSTRUCTION_ASL_ABS] = (cpu_instruction_s)
    {
        .name = "ASL",
        .opcode = 0x0E,
        .cycles = 6,
        .length = 3,
        .data_fetch = ABS,
        .execute = ASL,
    };
    table[OPCODE_UNDEFINED_0x0F] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x0F,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_BPL_REL] = (cpu_instruction_s)
    {
        .name = "BPL",
        .opcode = 0x10,
        .cycles = 2,
        .length = 2,
        .data_fetch = REL,
        .execute = BPL,
    };
    table[INSTRUCTION_ORA_IZY] = (cpu_instruction_s)
    {
        .name = "ORA",
        .opcode = 0x11,
        .cycles = 5,
        .length = 2,
        .data_fetch = IZY,
        .execute = ORA,
    };
    table[OPCODE_UNDEFINED_0x12] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x12,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x13] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x13,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x14] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x14,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_ORA_ZPX] = (cpu_instruction_s)
    {
        .name = "ORA",
        .opcode = 0x15,
        .cycles = 4,
        .length = 2,
        .data_fetch = ZPX,
        .execute = ORA,
    };
    table[INSTRUCTION_ASL_ZPX] = (cpu_instruction_s)
    {
        .name = "ASL",
        .opcode = 0x16,
        .cycles = 6,
        .length = 2,
        .data_fetch = ZPX,
        .execute = ASL,
    };
    table[OPCODE_UNDEFINED_0x17] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x17,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_CLC_IMP] = (cpu_instruction_s)
    {
        .name = "CLC",
        .opcode = 0x18,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = CLC,
    };
    table[INSTRUCTION_ORA_ABY] = (cpu_instruction_s)
    {
        .name = "ORA",
        .opcode = 0x19,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABY,
        .execute = ORA,
    };
    table[OPCODE_UNDEFINED_0x1A] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x1A,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x1B] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x1B,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x1C] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x1C,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_ORA_ABX] = (cpu_instruction_s)
    {
        .name = "ORA",
        .opcode = 0x1D,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABX,
        .execute = ORA,
    };
    table[INSTRUCTION_ASL_ABX] = (cpu_instruction_s)
    {
        .name = "ASL",
        .opcode = 0x1E,
        .cycles = 7,
        .length = 3,
        .data_fetch = ABX,
        .execute = ASL,
    };
    table[OPCODE_UNDEFINED_0x1F] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x1F,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_JSR_ABS] = (cpu_instruction_s)
    {
        .name = "JSR",
        .opcode = 0x20,
        .cycles = 6,
        .length = 3,
        .data_fetch = ABS,
        .execute = JSR,
    };
    table[INSTRUCTION_AND_IZX] = (cpu_instruction_s)
    {
        .name = "AND",
        .opcode = 0x21,
        .cycles = 6,
        .length = 2,
        .data_fetch = IZX,
        .execute = AND,
    };
    table[OPCODE_UNDEFINED_0x22] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x22,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x23] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x23,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_BIT_ZP0] = (cpu_instruction_s)
    {
        .name = "BIT",
        .opcode = 0x24,
        .cycles = 3,
        .length = 2,
        .data_fetch = ZP0,
        .execute = BIT,
    };
    table[INSTRUCTION_AND_ZP0] = (cpu_instruction_s)
    {
        .name = "AND",
        .opcode = 0x25,
        .cycles = 3,
        .length = 2,
        .data_fetch = ZP0,
        .execute = AND,
    };
    table[INSTRUCTION_ROL_ZP0] = (cpu_instruction_s)
    {
        .name = "ROL",
        .opcode = 0x26,
        .cycles = 5,
        .length = 2,
        .data_fetch = ZP0,
        .execute = ROL,
    };
    table[OPCODE_UNDEFINED_0x27] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x27,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_PLP_IMP] = (cpu_instruction_s)
    {
        .name = "PLP",
        .opcode = 0x28,
        .cycles = 4,
        .length = 1,
        .data_fetch = IMP,
        .execute = PLP,
    };
    table[INSTRUCTION_AND_IMM] = (cpu_instruction_s)
    {
        .name = "AND",
        .opcode = 0x29,
        .cycles = 2,
        .length = 2,
        .data_fetch = IMM,
        .execute = AND,
    };
    table[INSTRUCTION_ROL_ACC] = (cpu_instruction_s)
    {
        .name = "ROL",
        .opcode = 0x2A,
        .cycles = 2,
        .length = 1,
        .data_fetch = ACC,
        .execute = ROL,
    };
    table[OPCODE_UNDEFINED_0x2B] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x2B,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_BIT_ABS] = (cpu_instruction_s)
    {
        .name = "BIT",
        .opcode = 0x2C,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABS,
        .execute = BIT,
    };
    table[INSTRUCTION_AND_ABS] = (cpu_instruction_s)
    {
        .name = "AND",
        .opcode = 0x2D,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABS,
        .execute = AND,
    };
    table[INSTRUCTION_ROL_ABS] = (cpu_instruction_s)
    {
        .name = "ROL",
        .opcode = 0x2E,
        .cycles = 6,
        .length = 3,
        .data_fetch = ABS,
        .execute = ROL,
    };
    table[OPCODE_UNDEFINED_0x2F] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x2F,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_BMI_REL] = (cpu_instruction_s)
    {
        .name = "BMI",
        .opcode = 0x30,
        .cycles = 2,
        .length = 2,
        .data_fetch = REL,
        .execute = BMI,
    };
    table[INSTRUCTION_AND_IZY] = (cpu_instruction_s)
    {
        .name = "AND",
        .opcode = 0x31,
        .cycles = 5,
        .length = 2,
        .data_fetch = IZY,
        .execute = AND,
    };
    table[OPCODE_UNDEFINED_0x32] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x32,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x33] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x33,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_AND_ZPX] = (cpu_instruction_s)
    {
        .name = "AND",
        .opcode = 0x35,
        .cycles = 4,
        .length = 2,
        .data_fetch = ZPX,
        .execute = AND,
    };
    table[INSTRUCTION_ROL_ZPX] = (cpu_instruction_s)
    {
        .name = "ROL",
        .opcode = 0x36,
        .cycles = 6,
        .length = 2,
        .data_fetch = ZPX,
        .execute = ROL,
    };
    table[OPCODE_UNDEFINED_0x37] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x37,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_SEC_IMP] = (cpu_instruction_s)
    {
        .name = "SEC",
        .opcode = 0x38,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = SEC,
    };
    table[INSTRUCTION_AND_ABY] = (cpu_instruction_s)
    {
        .name = "AND",
        .opcode = 0x39,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABY,
        .execute = AND,
    };
    table[OPCODE_UNDEFINED_0x3A] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x3A,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x3B] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x3B,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_AND_ABX] = (cpu_instruction_s)
    {
        .name = "AND",
        .opcode = 0x3D,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABX,
        .execute = AND,
    };
    table[INSTRUCTION_ROL_ABX] = (cpu_instruction_s)
    {
        .name = "ROL",
        .opcode = 0x3E,
        .cycles = 7,
        .length = 3,
        .data_fetch = ABX,
        .execute = ROL,
    };
    table[OPCODE_UNDEFINED_0x3F] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x3F,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_RTI_IMP] = (cpu_instruction_s)
    {
        .name = "RTI",
        .opcode = 0x40,
        .cycles = 6,
        .length = 1,
        .data_fetch = IMP,
        .execute = RTI,
    };
    table[INSTRUCTION_EOR_IZX] = (cpu_instruction_s)
    {
        .name = "EOR",
        .opcode = 0x41,
        .cycles = 6,
        .length = 2,
        .data_fetch = IZX,
        .execute = EOR,
    };
    table[OPCODE_UNDEFINED_0x42] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x42,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x43] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x43,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_EOR_ZP0] = (cpu_instruction_s)
    {
        .name = "EOR",
        .opcode = 0x45,
        .cycles = 3,
        .length = 2,
        .data_fetch = ZP0,
        .execute = EOR,
    };
    table[INSTRUCTION_LSR_ZP0] = (cpu_instruction_s)
    {
        .name = "LSR",
        .opcode = 0x46,
        .cycles = 5,
        .length = 2,
        .data_fetch = ZP0,
        .execute = LSR,
    };
    table[OPCODE_UNDEFINED_0x47] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x47,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_PHA_IMP] = (cpu_instruction_s)
    {
        .name = "PHA",
        .opcode = 0x48,
        .cycles = 3,
        .length = 1,
        .data_fetch = IMP,
        .execute = PHA,
    };
    table[INSTRUCTION_EOR_IMM] = (cpu_instruction_s)
    {
        .name = "EOR",
        .opcode = 0x49,
        .cycles = 2,
        .length = 2,
        .data_fetch = IMM,
        .execute = EOR,
    };
    table[INSTRUCTION_LSR_ACC] = (cpu_instruction_s)
    {
        .name = "LSR",
        .opcode = 0x4A,
        .cycles = 2,
        .length = 1,
        .data_fetch = ACC,
        .execute = LSR,
    };
    table[OPCODE_UNDEFINED_0x4B] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x4B,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_JMP_ABS] = (cpu_instruction_s)
    {
        .name = "JMP",
        .opcode = 0x4C,
        .cycles = 3,
        .length = 3,
        .data_fetch = ABS,
        .execute = JMP,
    };
    table[INSTRUCTION_EOR_ABS] = (cpu_instruction_s)
    {
        .name = "EOR",
        .opcode = 0x4D,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABS,
        .execute = EOR,
    };
    table[INSTRUCTION_LSR_ABS] = (cpu_instruction_s)
    {
        .name = "LSR",
        .opcode = 0x4E,
        .cycles = 6,
        .length = 3,
        .data_fetch = ABS,
        .execute = LSR,
    };
    table[OPCODE_UNDEFINED_0x4F] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x4F,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_BVC_REL] = (cpu_instruction_s)
    {
        .name = "BVC",
        .opcode = 0x50,
        .cycles = 2,
        .length = 2,
        .data_fetch = REL,
        .execute = BVC,
    };
    table[INSTRUCTION_EOR_IZY] = (cpu_instruction_s)
    {
        .name = "EOR",
        .opcode = 0x51,
        .cycles = 5,
        .length = 2,
        .data_fetch = IZY,
        .execute = EOR,
    };
    table[OPCODE_UNDEFINED_0x52] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x52,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x53] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x53,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_EOR_ZPX] = (cpu_instruction_s)
    {
        .name = "EOR",
        .opcode = 0x55,
        .cycles = 4,
        .length = 2,
        .data_fetch = ZPX,
        .execute = EOR,
    };
    table[INSTRUCTION_LSR_ZPX] = (cpu_instruction_s)
    {
        .name = "LSR",
        .opcode = 0x56,
        .cycles = 6,
        .length = 2,
        .data_fetch = ZPX,
        .execute = LSR,
    };
    table[OPCODE_UNDEFINED_0x57] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x57,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_CLI_IMP] = (cpu_instruction_s)
    {
        .name = "CLI",
        .opcode = 0x58,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = CLI,
    };
    table[INSTRUCTION_EOR_ABY] = (cpu_instruction_s)
    {
        .name = "EOR",
        .opcode = 0x59,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABY,
        .execute = EOR,
    };
    table[OPCODE_UNDEFINED_0x5A] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x5A,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x5B] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x5B,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_EOR_ABX] = (cpu_instruction_s)
    {
        .name = "EOR",
        .opcode = 0x5D,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABX,
        .execute = EOR,
    };
    table[INSTRUCTION_LSR_ABX] = (cpu_instruction_s)
    {
        .name = "LSR",
        .opcode = 0x5E,
        .cycles = 7,
        .length = 3,
        .data_fetch = ABX,
        .execute = LSR,
    };
    table[OPCODE_UNDEFINED_0x5F] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x5F,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_RTS_IMP] = (cpu_instruction_s)
    {
        .name = "RTS",
        .opcode = 0x60,
        .cycles = 6,
        .length = 1,
        .data_fetch = IMP,
        .execute = RTS,
    };
    table[INSTRUCTION_ADC_IZX] = (cpu_instruction_s)
    {
        .name = "ADC",
        .opcode = 0x61,
        .cycles = 6,
        .length = 2,
        .data_fetch = IZX,
        .execute = ADC,
    };
    table[OPCODE_UNDEFINED_0x62] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x62,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x63] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x63,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x64] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x64,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_ADC_ZP0] = (cpu_instruction_s)
    {
        .name = "ADC",
        .opcode = 0x65,
        .cycles = 3,
        .length = 2,
        .data_fetch = ZP0,
        .execute = ADC,
    };
    table[INSTRUCTION_ROR_ZP0] = (cpu_instruction_s)
    {
        .name = "ROR",
        .opcode = 0x66,
        .cycles = 5,
        .length = 2,
        .data_fetch = ZP0,
        .execute = ROR,
    };
    table[OPCODE_UNDEFINED_0x67] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x67,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_PLA_IMP] = (cpu_instruction_s)
    {
        .name = "PLA",
        .opcode = 0x68,
        .cycles = 4,
        .length = 1,
        .data_fetch = IMP,
        .execute = PLA,
    };
    table[INSTRUCTION_ADC_IMM] = (cpu_instruction_s)
    {
        .name = "ADC",
        .opcode = 0x69,
        .cycles = 2,
        .length = 2,
        .data_fetch = IMM,
        .execute = ADC,
    };
    table[INSTRUCTION_ROR_ACC] = (cpu_instruction_s)
    {
        .name = "ROR",
        .opcode = 0x6A,
        .cycles = 2,
        .length = 1,
        .data_fetch = ACC,
        .execute = ROR,
    };
    table[OPCODE_UNDEFINED_0x6B] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x6B,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_JMP_IND] = (cpu_instruction_s)
    {
        .name = "JMP",
        .opcode = 0x6C,
        .cycles = 5,
        .length = 3,
        .data_fetch = IND,
        .execute = JMP,
    };
    table[INSTRUCTION_ADC_ABS] = (cpu_instruction_s)
    {
        .name = "ADC",
        .opcode = 0x6D,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABS,
        .execute = ADC,
    };
    table[INSTRUCTION_ROR_ABS] = (cpu_instruction_s)
    {
        .name = "ROR",
        .opcode = 0x6E,
        .cycles = 6,
        .length = 3,
        .data_fetch = ABS,
        .execute = ROR,
    };
    table[OPCODE_UNDEFINED_0x6F] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x6F,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_BVS_REL] = (cpu_instruction_s)
    {
        .name = "BVS",
        .opcode = 0x70,
        .cycles = 2,
        .length = 2,
        .data_fetch = REL,
        .execute = BVS,
    };
    table[INSTRUCTION_ADC_IZY] = (cpu_instruction_s)
    {
        .name = "ADC",
        .opcode = 0x71,
        .cycles = 5,
        .length = 2,
        .data_fetch = IZY,
        .execute = ADC,
    };
    table[OPCODE_UNDEFINED_0x72] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x72,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x73] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x73,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x74] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x74,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_ADC_ZPX] = (cpu_instruction_s)
    {
        .name = "ADC",
        .opcode = 0x75,
        .cycles = 4,
        .length = 2,
        .data_fetch = ZPX,
        .execute = ADC,
    };
    table[INSTRUCTION_ROR_ZPX] = (cpu_instruction_s)
    {
        .name = "ROR",
        .opcode = 0x76,
        .cycles = 6,
        .length = 2,
        .data_fetch = ZPX,
        .execute = ROR,
    };
    table[OPCODE_UNDEFINED_0x77] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x77,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_SEI_IMP] = (cpu_instruction_s)
    {
        .name = "SEI",
        .opcode = 0x78,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = SEI,
    };
    table[INSTRUCTION_ADC_ABY] = (cpu_instruction_s)
    {
        .name = "ADC",
        .opcode = 0x79,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABY,
        .execute = ADC,
    };
    table[OPCODE_UNDEFINED_0x7A] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x7A,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x7B] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x7B,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x7C] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x7C,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_ADC_ABX] = (cpu_instruction_s)
    {
        .name = "ADC",
        .opcode = 0x7D,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABX,
        .execute = ADC,
    };
    table[INSTRUCTION_ROR_ABX] = (cpu_instruction_s)
    {
        .name = "ROR",
        .opcode = 0x7E,
        .cycles = 7,
        .length = 3,
        .data_fetch = ABX,
        .execute = ROR,
    };
    table[OPCODE_UNDEFINED_0x7F] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x7F,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_STA_IZX] = (cpu_instruction_s)
    {
        .name = "STA",
        .opcode = 0x81,
        .cycles = 6,
        .length = 2,
        .data_fetch = IZX,
        .execute = STA,
    };
    table[OPCODE_UNDEFINED_0x82] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x82,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_STY_ZP0] = (cpu_instruction_s)
    {
        .name = "STY",
        .opcode = 0x84,
        .cycles = 3,
        .length = 2,
        .data_fetch = ZP0,
        .execute = STY,
    };
    table[INSTRUCTION_STA_ZP0] = (cpu_instruction_s)
    {
        .name = "STA",
        .opcode = 0x85,
        .cycles = 3,
        .length = 2,
        .data_fetch = ZP0,
        .execute = STA,
    };
    table[INSTRUCTION_STX_ZP0] = (cpu_instruction_s)
    {
        .name = "STX",
        .opcode = 0x86,
        .cycles = 3,
        .length = 2,
        .data_fetch = ZP0,
        .execute = STX,
    };
    table[OPCODE_UNDEFINED_0x87] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x87,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_DEY_IMP] = (cpu_instruction_s)
    {
        .name = "DEY",
        .opcode = 0x88,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = DEY,
    };
    table[OPCODE_UNDEFINED_0x89] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x89,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_TXA_IMP] = (cpu_instruction_s)
    {
        .name = "TXA",
        .opcode = 0x8A,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = TXA,
    };
    table[OPCODE_UNDEFINED_0x8B] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x8B,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_STY_ABS] = (cpu_instruction_s)
    {
        .name = "STY",
        .opcode = 0x8C,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABS,
        .execute = STY,
    };
    table[INSTRUCTION_STA_ABS] = (cpu_instruction_s)
    {
        .name = "STA",
        .opcode = 0x8D,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABS,
        .execute = STA,
    };
    table[INSTRUCTION_STX_ABS] = (cpu_instruction_s)
    {
        .name = "STX",
        .opcode = 0x8E,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABS,
        .execute = STX,
    };
    table[OPCODE_UNDEFINED_0x8F] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x8F,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_BCC_REL] = (cpu_instruction_s)
    {
        .name = "BCC",
        .opcode = 0x90,
        .cycles = 2,
        .length = 2,
        .data_fetch = REL,
        .execute = BCC,
    };
    table[INSTRUCTION_STA_IZY] = (cpu_instruction_s)
    {
        .name = "STA",
        .opcode = 0x91,
        .cycles = 6,
        .length = 2,
        .data_fetch = IZY,
        .execute = STA,
    };
    table[OPCODE_UNDEFINED_0x92] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x92,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x93] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x93,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_STY_ZPX] = (cpu_instruction_s)
    {
        .name = "STY",
        .opcode = 0x94,
        .cycles = 4,
        .length = 2,
        .data_fetch = ZPX,
        .execute = STY,
    };
    table[INSTRUCTION_STA_ZPX] = (cpu_instruction_s)
    {
        .name = "STA",
        .opcode = 0x95,
        .cycles = 4,
        .length = 2,
        .data_fetch = ZPX,
        .execute = STA,
    };
    table[INSTRUCTION_STX_ZPY] = (cpu_instruction_s)
    {
        .name = "STX",
        .opcode = 0x96,
        .cycles = 4,
        .length = 2,
        .data_fetch = ZPY,
        .execute = STX,
    };
    table[OPCODE_UNDEFINED_0x97] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x97,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_TYA_IMP] = (cpu_instruction_s)
    {
        .name = "TYA",
        .opcode = 0x98,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = TYA,
    };
    table[INSTRUCTION_STA_ABY] = (cpu_instruction_s)
    {
        .name = "STA",
        .opcode = 0x99,
        .cycles = 5,
        .length = 3,
        .data_fetch = ABY,
        .execute = STA,
    };
    table[INSTRUCTION_TXS_IMP] = (cpu_instruction_s)
    {
        .name = "TXS",
        .opcode = 0x9A,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = TXS,
    };
    table[OPCODE_UNDEFINED_0x9B] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x9B,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x9C] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x9C,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_STA_ABX] = (cpu_instruction_s)
    {
        .name = "STA",
        .opcode = 0x9D,
        .cycles = 5,
        .length = 3,
        .data_fetch = ABX,
        .execute = STA,
    };
    table[OPCODE_UNDEFINED_0x9E] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x9E,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0x9F] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0x9F,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_LDY_IMM] = (cpu_instruction_s)
    {
        .name = "LDY",
        .opcode = 0xA0,
        .cycles = 2,
        .length = 2,
        .data_fetch = IMM,
        .execute = LDY,
    };
    table[INSTRUCTION_LDA_IZX] = (cpu_instruction_s)
    {
        .name = "LDA",
        .opcode = 0xA1,
        .cycles = 6,
        .length = 2,
        .data_fetch = IZX,
        .execute = LDA,
    };
    table[INSTRUCTION_LDX_IMM] = (cpu_instruction_s)
    {
        .name = "LDX",
        .opcode = 0xA2,
        .cycles = 2,
        .length = 2,
        .data_fetch = IMM,
        .execute = LDX,
    };
    table[OPCODE_UNDEFINED_0xA3] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xA3,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_LDY_ZP0] = (cpu_instruction_s)
    {
        .name = "LDY",
        .opcode = 0xA4,
        .cycles = 3,
        .length = 2,
        .data_fetch = ZP0,
        .execute = LDY,
    };
    table[INSTRUCTION_LDA_ZP0] = (cpu_instruction_s)
    {
        .name = "LDA",
        .opcode = 0xA5,
        .cycles = 3,
        .length = 2,
        .data_fetch = ZP0,
        .execute = LDA,
    };
    table[INSTRUCTION_LDX_ZP0] = (cpu_instruction_s)
    {
        .name = "LDX",
        .opcode = 0xA6,
        .cycles = 3,
        .length = 2,
        .data_fetch = ZP0,
        .execute = LDX,
    };
    table[OPCODE_UNDEFINED_0xA7] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xA7,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_TAY_IMP] = (cpu_instruction_s)
    {
        .name = "TAY",
        .opcode = 0xA8,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = TAY,
    };
    table[INSTRUCTION_LDA_IMM] = (cpu_instruction_s)
    {
        .name = "LDA",
        .opcode = 0xA9,
        .cycles = 2,
        .length = 2,
        .data_fetch = IMM,
        .execute = LDA,
    };
    table[INSTRUCTION_TAX_IMP] = (cpu_instruction_s)
    {
        .name = "TAX",
        .opcode = 0xAA,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = TAX,
    };
    table[OPCODE_UNDEFINED_0xAB] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xAB,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_LDY_ABS] = (cpu_instruction_s)
    {
        .name = "LDY",
        .opcode = 0xAC,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABS,
        .execute = LDY,
    };
    table[INSTRUCTION_LDA_ABS] = (cpu_instruction_s)
    {
        .name = "LDA",
        .opcode = 0xAD,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABS,
        .execute = LDA,
    };
    table[INSTRUCTION_LDX_ABS] = (cpu_instruction_s)
    {
        .name = "LDX",
        .opcode = 0xAE,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABS,
        .execute = LDX,
    };
    table[OPCODE_UNDEFINED_0xAF] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xAF,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_BCS_REL] = (cpu_instruction_s)
    {
        .name = "BCS",
        .opcode = 0xB0,
        .cycles = 2,
        .length = 2,
        .data_fetch = REL,
        .execute = BCS,
    };
    table[INSTRUCTION_LDA_IZY] = (cpu_instruction_s)
    {
        .name = "LDA",
        .opcode = 0xB1,
        .cycles = 5,
        .length = 2,
        .data_fetch = IZY,
        .execute = LDA,
    };
    table[OPCODE_UNDEFINED_0xB2] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xB2,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0xB3] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xB3,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_LDY_ZPX] = (cpu_instruction_s)
    {
        .name = "LDY",
        .opcode = 0xB4,
        .cycles = 4,
        .length = 2,
        .data_fetch = ZPX,
        .execute = LDY,
    };
    table[INSTRUCTION_LDA_ZPX] = (cpu_instruction_s)
    {
        .name = "LDA",
        .opcode = 0xB5,
        .cycles = 4,
        .length = 2,
        .data_fetch = ZPX,
        .execute = LDA,
    };
    table[INSTRUCTION_LDX_ZPY] = (cpu_instruction_s)
    {
        .name = "LDX",
        .opcode = 0xB6,
        .cycles = 4,
        .length = 2,
        .data_fetch = ZPY,
        .execute = LDX,
    };
    table[OPCODE_UNDEFINED_0xB7] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xB7,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_CLV_IMP] = (cpu_instruction_s)
    {
        .name = "CLV",
        .opcode = 0xB8,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = CLV,
    };
    table[INSTRUCTION_LDA_ABY] = (cpu_instruction_s)
    {
        .name = "LDA",
        .opcode = 0xB9,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABY,
        .execute = LDA,
    };
    table[INSTRUCTION_TSX_IMP] = (cpu_instruction_s)
    {
        .name = "TSX",
        .opcode = 0xBA,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = TSX,
    };
    table[OPCODE_UNDEFINED_0xBB] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xBB,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_LDY_ABX] = (cpu_instruction_s)
    {
        .name = "LDY",
        .opcode = 0xBC,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABX,
        .execute = LDY,
    };
    table[INSTRUCTION_LDA_ABX] = (cpu_instruction_s)
    {
        .name = "LDA",
        .opcode = 0xBD,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABX,
        .execute = LDA,
    };
    table[INSTRUCTION_LDX_ABY] = (cpu_instruction_s)
    {
        .name = "LDX",
        .opcode = 0xBE,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABY,
        .execute = LDX,
    };
    table[OPCODE_UNDEFINED_0xBF] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xBF,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_CPY_IMM] = (cpu_instruction_s)
    {
        .name = "CPY",
        .opcode = 0xC0,
        .cycles = 2,
        .length = 2,
        .data_fetch = IMM,
        .execute = CPY,
    };
    table[INSTRUCTION_CMP_IZX] = (cpu_instruction_s)
    {
        .name = "CMP",
        .opcode = 0xC1,
        .cycles = 6,
        .length = 2,
        .data_fetch = IZX,
        .execute = CMP,
    };
    table[OPCODE_UNDEFINED_0xC2] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xC2,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0xC3] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xC3,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_CPY_ZP0] = (cpu_instruction_s)
    {
        .name = "CPY",
        .opcode = 0xC4,
        .cycles = 3,
        .length = 2,
        .data_fetch = ZP0,
        .execute = CPY,
    };
    table[INSTRUCTION_CMP_ZP0] = (cpu_instruction_s)
    {
        .name = "CMP",
        .opcode = 0xC5,
        .cycles = 3,
        .length = 2,
        .data_fetch = ZP0,
        .execute = CMP,
    };
    table[INSTRUCTION_DEC_ZP0] = (cpu_instruction_s)
    {
        .name = "DEC",
        .opcode = 0xC6,
        .cycles = 5,
        .length = 2,
        .data_fetch = ZP0,
        .execute = DEC,
    };
    table[OPCODE_UNDEFINED_0xC7] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xC7,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_INY_IMP] = (cpu_instruction_s)
    {
        .name = "INY",
        .opcode = 0xC8,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = INY,
    };
    table[INSTRUCTION_CMP_IMM] = (cpu_instruction_s)
    {
        .name = "CMP",
        .opcode = 0xC9,
        .cycles = 2,
        .length = 2,
        .data_fetch = IMM,
        .execute = CMP,
    };
    table[INSTRUCTION_DEX_IMP] = (cpu_instruction_s)
    {
        .name = "DEX",
        .opcode = 0xCA,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = DEX,
    };
    table[OPCODE_UNDEFINED_0xCB] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xCB,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_CPY_ABS] = (cpu_instruction_s)
    {
        .name = "CPY",
        .opcode = 0xCC,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABS,
        .execute = CPY,
    };
    table[INSTRUCTION_CMP_ABS] = (cpu_instruction_s)
    {
        .name = "CMP",
        .opcode = 0xCD,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABS,
        .execute = CMP,
    };
    table[INSTRUCTION_DEC_ABS] = (cpu_instruction_s)
    {
        .name = "DEC",
        .opcode = 0xCE,
        .cycles = 6,
        .length = 3,
        .data_fetch = ABS,
        .execute = DEC,
    };
    table[OPCODE_UNDEFINED_0xCF] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xCF,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_BNE_REL] = (cpu_instruction_s)
    {
        .name = "BNE",
        .opcode = 0xD0,
        .cycles = 2,
        .length = 2,
        .data_fetch = REL,
        .execute = BNE,
    };
    table[INSTRUCTION_CMP_IZY] = (cpu_instruction_s)
    {
        .name = "CMP",
        .opcode = 0xD1,
        .cycles = 5,
        .length = 2,
        .data_fetch = IZY,
        .execute = CMP,
    };
    table[OPCODE_UNDEFINED_0xD2] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xD2,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0xD3] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xD3,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_CMP_ZPX] = (cpu_instruction_s)
    {
        .name = "CMP",
        .opcode = 0xD5,
        .cycles = 4,
        .length = 2,
        .data_fetch = ZPX,
        .execute = CMP,
    };
    table[INSTRUCTION_DEC_ZPX] = (cpu_instruction_s)
    {
        .name = "DEC",
        .opcode = 0xD6,
        .cycles = 6,
        .length = 2,
        .data_fetch = ZPX,
        .execute = DEC,
    };
    table[OPCODE_UNDEFINED_0xD7] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xD7,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_CLD_IMP] = (cpu_instruction_s)
    {
        .name = "CLD",
        .opcode = 0xD8,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = CLD,
    };
    table[INSTRUCTION_CMP_ABY] = (cpu_instruction_s)
    {
        .name = "CMP",
        .opcode = 0xD9,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABY,
        .execute = CMP,
    };
    table[OPCODE_UNDEFINED_0xDA] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xDA,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0xDB] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xDB,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_CMP_ABX] = (cpu_instruction_s)
    {
        .name = "CMP",
        .opcode = 0xDD,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABX,
        .execute = CMP,
    };
    table[INSTRUCTION_DEC_ABX] = (cpu_instruction_s)
    {
        .name = "DEC",
        .opcode = 0xDE,
        .cycles = 7,
        .length = 3,
        .data_fetch = ABX,
        .execute = DEC,
    };
    table[OPCODE_UNDEFINED_0xDF] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xDF,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_CPX_IMM] = (cpu_instruction_s)
    {
        .name = "CPX",
        .opcode = 0xE0,
        .cycles = 2,
        .length = 2,
        .data_fetch = IMM,
        .execute = CPX,
    };
    table[INSTRUCTION_SBC_IZX] = (cpu_instruction_s)
    {
        .name = "SBC",
        .opcode = 0xE1,
        .cycles = 6,
        .length = 2,
        .data_fetch = IZX,
        .execute = SBC,
    };
    table[OPCODE_UNDEFINED_0xE2] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xE2,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0xE3] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xE3,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_CPX_ZP0] = (cpu_instruction_s)
    {
        .name = "CPX",
        .opcode = 0xE4,
        .cycles = 3,
        .length = 2,
        .data_fetch = ZP0,
        .execute = CPX,
    };
    table[INSTRUCTION_SBC_ZP0] = (cpu_instruction_s)
    {
        .name = "SBC",
        .opcode = 0xE5,
        .cycles = 3,
        .length = 2,
        .data_fetch = ZP0,
        .execute = SBC,
    };
    table[INSTRUCTION_INC_ZP0] = (cpu_instruction_s)
    {
        .name = "INC",
        .opcode = 0xE6,
        .cycles = 5,
        .length = 2,
        .data_fetch = ZP0,
        .execute = INC,
    };
    table[OPCODE_UNDEFINED_0xE7] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xE7,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_INX_IMP] = (cpu_instruction_s)
    {
        .name = "INX",
        .opcode = 0xE8,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = INX,
    };
    table[INSTRUCTION_SBC_IMM] = (cpu_instruction_s)
    {
        .name = "SBC",
        .opcode = 0xE9,
        .cycles = 2,
        .length = 2,
        .data_fetch = IMM,
        .execute = SBC,
    };
    table[INSTRUCTION_NOP_IMP] = (cpu_instruction_s)
    {
        .name = "NOP",
        .opcode = 0xEA,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = NOP,
    };
    table[OPCODE_UNDEFINED_0xEB] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xEB,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_CPX_ABS] = (cpu_instruction_s)
    {
        .name = "CPX",
        .opcode = 0xEC,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABS,
        .execute = CPX,
    };
    table[INSTRUCTION_SBC_ABS] = (cpu_instruction_s)
    {
        .name = "SBC",
        .opcode = 0xED,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABS,
        .execute = SBC,
    };
    table[INSTRUCTION_INC_ABS] = (cpu_instruction_s)
    {
        .name = "INC",
        .opcode = 0xEE,
        .cycles = 6,
        .length = 3,
        .data_fetch = ABS,
        .execute = INC,
    };
    table[OPCODE_UNDEFINED_0xEF] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xEF,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_BEQ_REL] = (cpu_instruction_s)
    {
        .name = "BEQ",
        .opcode = 0xF0,
        .cycles = 2,
        .length = 2,
        .data_fetch = REL,
        .execute = BEQ,
    };
    table[INSTRUCTION_SBC_IZY] = (cpu_instruction_s)
    {
        .name = "SBC",
        .opcode = 0xF1,
        .cycles = 5,
        .length = 2,
        .data_fetch = IZY,
        .execute = SBC,
    };
    table[OPCODE_UNDEFINED_0xF2] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xF2,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0xF3] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xF3,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_SBC_ZPX] = (cpu_instruction_s)
    {
        .name = "SBC",
        .opcode = 0xF5,
        .cycles = 4,
        .length = 2,
        .data_fetch = ZPX,
        .execute = SBC,
    };
    table[INSTRUCTION_INC_ZPX] = (cpu_instruction_s)
    {
        .name = "INC",
        .opcode = 0xF6,
        .cycles = 6,
        .length = 2,
        .data_fetch = ZPX,
        .execute = INC,
    };
    table[OPCODE_UNDEFINED_0xF7] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xF7,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_SED_IMP] = (cpu_instruction_s)
    {
        .name = "SED",
        .opcode = 0xF8,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = SED,
    };
    table[INSTRUCTION_SBC_ABY] = (cpu_instruction_s)
    {
        .name = "SBC",
        .opcode = 0xF9,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABY,
        .execute = SBC,
    };
    table[OPCODE_UNDEFINED_0xFA] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xFA,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[OPCODE_UNDEFINED_0xFB] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xFB,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
    table[INSTRUCTION_SBC_ABX] = (cpu_instruction_s)
    {
        .name = "SBC",
        .opcode = 0xFD,
        .cycles = 4,
        .length = 3,
        .data_fetch = ABX,
        .execute = SBC,
    };
    table[INSTRUCTION_INC_ABX] = (cpu_instruction_s)
    {
        .name = "INC",
        .opcode = 0xFE,
        .cycles = 7,
        .length = 3,
        .data_fetch = ABX,
        .execute = INC,
    };
    table[OPCODE_UNDEFINED_0xFF] = (cpu_instruction_s)
    {
        .name = "???",
        .opcode = 0xFF,
        .cycles = 2,
        .length = 1,
        .data_fetch = IMP,
        .execute = ILLEGAL,
    };
}

bool crosses_page(word_t addr1, word_t addr2)
{
    return (addr1 & 0xFF00) != (addr2 & 0xFF00);
}

void set_zn(cpu_s *cpu, byte_t value)
{
    set_flag(cpu, STATUS_FLAG_Z, value == 0);
    set_flag(cpu, STATUS_FLAG_N, value & 0x80);
}

word_t assemble_word(byte_t high, byte_t low)
{
    return (high << 8) | low;
}

void push_address(cpu_s *cpu, word_t addr)
{
    push_byte_to_stack(cpu, ((addr) & 0xFF00) >> 8);
    push_byte_to_stack(cpu, (addr) & 0x00FF);
}


/*
    Zero-out the global variables that are
    used to store the operands of the current instruction
*/
void reset_globals()
{
    address = 0x0000;
    address_rel = 0x00;
    value = 0;
    return;
}

bool get_flag(cpu_s *cpu, cpu_status_flag_e flag)
{
    return (cpu->STATUS & flag) > 0;
}

void set_flag(cpu_s *cpu, cpu_status_flag_e flag, bool value)
{
    if (value)
        cpu->STATUS |= flag;
    else
        cpu->STATUS &= ~flag;
    return;
}
byte_t branch_pc(cpu_s *cpu)
{
    cpu->PC += address_rel;
    return 0;
}

static byte_t peek(cpu_s *cpu)
{
    assert(cpu != NULL && cpu->bus != NULL);
    return bus_read(cpu->bus, cpu->PC);
}

byte_t read_from_addr(cpu_s *cpu, word_t address)
{
    assert(cpu != NULL && cpu->bus != NULL);
    return bus_read(cpu->bus, address);
}

void write_to_addr(cpu_s *cpu, word_t address, byte_t value)
{
    assert(cpu != NULL && cpu->bus != NULL);
    bus_write(cpu->bus, address, value);
    return;
}

void push_byte_to_stack(cpu_s *cpu, byte_t byte)
{
    assert(cpu != NULL && cpu->bus != NULL);
    if (cpu->SP == 0x00)
    {
        cpu->SP = 0xFF; // stack is full, wrap around page 1
    }
    word_t stack_addr = 0x0100 + cpu->SP;
    write_to_addr(cpu, stack_addr, byte);
    cpu->SP--;
    return;
}

byte_t pop_byte(cpu_s *cpu)
{
    assert(cpu != NULL && cpu->bus != NULL);
    cpu->SP++;
    if (cpu->SP == 0xFF)
    {
        cpu->SP = 0x00; // stack is empty, wrap around page 1
    }
    word_t stack_addr = 0x0100 + cpu->SP;
    byte_t byte = read_from_addr(cpu, stack_addr);
    return byte;
}

/*
    Internal function
    "Runs" the instruction by first fetching it's operands
    and putting them into global variables
    Then, executes the instruction using those operands
    If the instruction needs an additional cycle, returns true
*/
bool fetch_and_execute(cpu_s *cpu)
{
    assert(cpu != NULL && cpu->instruction_pending);
    cpu_instruction_s *instruction = get_current_instruction(cpu);
    bool result = instruction->data_fetch(cpu);
    result = instruction->execute(cpu) && result;
    return result;
}

//Just for constructing the CPU, not an actual interrupt
void cpu_init(cpu_s *cpu, bus_s *bus)
{
    assert(cpu != NULL && bus != NULL);
    cpu->A = 0x00;
    cpu->X = 0x00;
    cpu->Y = 0x00;
    cpu->SP = 0xFD;
    cpu->STATUS = 0x00 | STATUS_FLAG_U;
    cpu->PC = 0x0000;
    cpu->cycles = 7; // Reset sequence takes 7 cycles
    cpu->current_opcode = 0x00;
    cpu->instruction_pending = false;

    cpu->bus = bus; //attaching bus to CPU
    init_instruction_table(cpu);

    reset_globals();
    return;
}

void reset(cpu_s *cpu)
{
    cpu_init(cpu, cpu->bus);
    reset_globals();
    cpu->PC = assemble_word(read_from_addr(cpu, 0xFFFD), read_from_addr(cpu, 0xFFFC));
    cpu->STATUS = (rand() % 256) | STATUS_FLAG_U;
    return;
}

void irq(cpu_s *cpu)
{
    if (get_flag(cpu, STATUS_FLAG_I)) //interupt is overridden by I flag
    {
        return;
    }
    push_address(cpu, cpu->PC);
    push_byte_to_stack(cpu, cpu->STATUS);
    set_flag(cpu, STATUS_FLAG_I, true);
    cpu->PC = (read_from_addr(cpu, 0xFFFF) << 8) | read_from_addr(cpu, 0xFFFE);
    cpu->cycles += 7; // IRQ takes 7 cycles
    return;
}

void nmi(cpu_s *cpu)
{
    push_address(cpu, cpu->PC);
    push_byte_to_stack(cpu, cpu->STATUS);
    set_flag(cpu, STATUS_FLAG_I, 1);
    cpu->PC = (read_from_addr(cpu, 0xFFFB) << 8) | read_from_addr(cpu, 0xFFFA);
    cpu->cycles += 7; // NMI takes 7 cycles
    return;
}

void adjust_pc(cpu_s *cpu, byte_t length)
{
    cpu->PC += length;
    return;
}

cpu_instruction_s *get_instruction(cpu_s *cpu, byte_t opcode)
{
    return &cpu->table[opcode];
}

cpu_instruction_s *get_current_instruction(cpu_s *cpu)
{
    return get_instruction(cpu, cpu->current_opcode);
}


/*
 * Implied addressing mode
 * No operand needed
 * Returns 0
 */
byte_t IMP(cpu_s *cpu)
{
    assert(cpu != NULL);
    acc_mode = false;
    return 0;
}

/*
 * Accumulator addressing mode
 * Operand is the accumulator
 * Sets value to accumulator and acc_mode flag for write-back
 * Returns 0
 */
byte_t ACC(cpu_s *cpu)
{
    assert(cpu != NULL);
    value = cpu->A;
    acc_mode = true;
    return 0;
}

/*
 * Immediate addressing mode
 * Fetches the next byte in memory and stores it in the value variable
 * Increments the program counter
 */
byte_t IMM(cpu_s *cpu)
{
    assert(cpu != NULL);
    acc_mode = false;
    value = read_from_addr(cpu, cpu->PC + 1);
    return 0;
}

/*
 * Zero page addressing mode
 * Fetches the next byte in memory and stores it in the zero'th page of memory
 * Increments the program counter
 */

byte_t ZP0(cpu_s *cpu)
{
    assert(cpu != NULL);
    acc_mode = false;
    address = read_from_addr(cpu, cpu->PC + 1);
    address = address & 0x00FF;
    value = read_from_addr(cpu, address);
    return 0;
}

/*
 * Zero page X addressing mode
 * Fetches the next byte in memory and stores it in the zero'th page of memory
 * Increments the program counter
 * Adds the X register to the address
 * Stores the result in the address variable
 * Fetches the value at the address and stores it in the value variable
 */
byte_t ZPX(cpu_s *cpu)
{
    assert(cpu != NULL);
    acc_mode = false;
    address = read_from_addr(cpu, cpu->PC + 1) & 0x00FF;
    address += cpu->X;
    address = address & 0x00FF; //wrap around zero page
    value = read_from_addr(cpu, address);
    return 0;
}

/*
 * Zero page Y addressing mode
 * Fetches the next byte in memory and stores it in the zero'th page of memory
 * Increments the program counter
 * Adds the Y register to the address
 * Stores the result in the address variable
 * Fetches the value at the address and stores it in the value variable
 */
byte_t ZPY(cpu_s *cpu)
{
    assert(cpu != NULL);
    acc_mode = false;
    address = read_from_addr(cpu, cpu->PC + 1) & 0x00FF;
    address += cpu->Y;
    address = address & 0x00FF; //wrap around zero page
    value = read_from_addr(cpu, address);
    return 0;
}

/*
 * Relative addressing mode
 * Fetches the next byte in memory and stores it in the address_rel variable
 * Increments the program counter
 */
byte_t REL(cpu_s *cpu)
{
    assert(cpu != NULL);
    acc_mode = false;
    address_rel = read_from_addr(cpu, cpu->PC + 1);
    return 0;
}

/*
 * Absolute addressing mode
 * Fetches the next two bytes in memory and stores it in the address variable
 * Increments the program counter by two
 * Fetches the value at the address and stores it in the value variable
 */
byte_t ABS(cpu_s *cpu)
{
    assert(cpu != NULL);
    acc_mode = false;
    byte_t low_byte = read_from_addr(cpu, cpu->PC + 1);
    byte_t high_byte = read_from_addr(cpu, cpu->PC + 2);
    address = assemble_word(high_byte, low_byte);
    value = read_from_addr(cpu, address);
    return 0;
}

/*
 * Absolute X addressing mode
 * Fetches the next two bytes in memory and stores it in the address variable
 * Increments the program counter by two
 * Adds the X register to the address
 * Stores the result in the address variable
 * Fetches the value at the address and stores it in the value variable
 */

byte_t ABX(cpu_s *cpu)
{
    assert(cpu != NULL);
    acc_mode = false;
    byte_t low_byte = read_from_addr(cpu, cpu->PC + 1);
    byte_t high_byte = read_from_addr(cpu, cpu->PC + 2);
    address = assemble_word(high_byte, low_byte);
    address += cpu->X;
    //indicate that we need an additional cycle if the address crosses a page boundary
    value = read_from_addr(cpu, address);
    return crosses_page(address - cpu->X, address) ? 1 : 0;
}

/*
 * Absolute Y addressing mode
 * Fetches the next two bytes in memory and stores it in the address variable
 * Increments the program counter
 * Adds the Y register to the address
 * Stores the result in the address variable
 * Fetches the value at the address and stores it in the value variable
 */

byte_t ABY(cpu_s *cpu)
{
    assert(cpu != NULL);
    acc_mode = false;
    byte_t low_byte = read_from_addr(cpu, cpu->PC + 1);
    byte_t high_byte = read_from_addr(cpu, cpu->PC + 2);
    address = assemble_word(high_byte, low_byte);
    address += cpu->Y;
    value = read_from_addr(cpu, address);
    return crosses_page(address - cpu->Y, address) ? 1 : 0;
}

/*
 * Indirect addressing mode
 * Fetches the next two bytes in memory and stores it in the ptr variable
 * Checks if the low byte of the ptr is 0xFF
 * Simulate the page boundary hardware bug: if the low byte is 0xFF, the high byte is fetched from the same page
 * Otherwise, the high byte is fetched from ptr + 1
 */

byte_t IND(cpu_s *cpu)
{
    assert(cpu != NULL);
    acc_mode = false;
    byte_t low_byte = read_from_addr(cpu, cpu->PC + 1);
    byte_t high_byte = read_from_addr(cpu, cpu->PC + 2);
    word_t ptr = assemble_word(high_byte, low_byte);

    // simulate page boundary hardware bug
    low_byte = read_from_addr(cpu, ptr);
    high_byte = (ptr & 0x00FF) == 0x00FF ? read_from_addr(cpu, ptr & 0xFF00) : read_from_addr(cpu, ptr + 1);

    address = assemble_word(high_byte, low_byte);
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Indirect X addressing mode
    * Fetches the next byte in memory interprets it as a zero page address
    * Assembles the address by adding the X register to the zero page address
    * Fetches the value at the address and stores it in the value variable
*/

byte_t IZX(cpu_s *cpu)
{
    assert(cpu != NULL);
    acc_mode = false;
    byte_t zp_addr = read_from_addr(cpu, cpu->PC + 1);
    byte_t low = read_from_addr(cpu, (zp_addr + cpu->X) & 0x00FF);
    byte_t high = read_from_addr(cpu, (zp_addr + cpu->X + 1) & 0x00FF);
    address = assemble_word(high, low);
    value = read_from_addr(cpu, address);
    return 0; //this will never cross a page boundary
}

/*
 * Indirect Y addressing mode
 * Fetches the next byte in memory and stores it in the address variable
 * Increments the program counter
 * Fetches the value at the address and stores it in the value variable
 * Adds the Y register to the address
 * Stores the result in the address variable
 */
byte_t IZY(cpu_s *cpu)
{
    assert(cpu != NULL);
    acc_mode = false;
    word_t ptr = (word_t)read_from_addr(cpu, cpu->PC + 1);
    ptr = ptr & 0x00FF;
    address = assemble_word(read_from_addr(cpu, (ptr + 1) & 0x00FF), read_from_addr(cpu, ptr));
    address += cpu->Y;
    value = read_from_addr(cpu, address);
    return crosses_page(address - cpu->Y, address) ? 1 : 0; //indicate if we need an additional cycle
}

/*
    BRK - Force Interrupt
    Pushes the program counter onto the stack
    Pushes the status register onto the stack
    Sets the interrupt flag
    Sets the program counter to the interrupt vector
*/

byte_t BRK(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->PC++; // BRK has a padding byte
    push_address(cpu, cpu->PC);
    set_flag(cpu, STATUS_FLAG_B, true); // sets the break flag because this is a software interrupt
    push_byte_to_stack(cpu, cpu->STATUS);
    set_flag(cpu, STATUS_FLAG_B, false); // clears the break flag
    set_flag(cpu, STATUS_FLAG_I, true);
    cpu->PC = assemble_word(read_from_addr(cpu, 0xFFFF), read_from_addr(cpu, 0xFFFE));
    cpu->pc_changed = true;
    return 0;
}

/*
    ORA - OR Memory with Accumulator
    ORs the value with the accumulator
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
byte_t ORA(cpu_s *cpu)
{
    cpu->A |= value;
    set_zn(cpu, cpu->A);
    return 1; // this instruction can take an extra cycle
}


/*
    ASL Arithmetic Shift Left
    Shifts the value left by 1 bit
    Sets the carry flag to the 7th bit of the value
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
    Stores the result in accumulator or memory based on addressing mode
*/
byte_t ASL(cpu_s *cpu)
{
    assert(cpu != NULL);
    set_flag(cpu, STATUS_FLAG_C, value & 0x80);
    value <<= 1;
    set_zn(cpu, value);
    if (acc_mode)
    {
        cpu->A = value;
    }
    else
    {
        write_to_addr(cpu, address, value);
    }
    return 0;
}
/*
    Push Processor Status on Stack
    Pushes the status register onto the stack
*/
byte_t PHP(cpu_s *cpu)
{
    push_byte_to_stack(cpu, cpu->STATUS | STATUS_FLAG_B | STATUS_FLAG_U); // pushes the status register with the break and unused bits set
    return 0;
}

// branch helper function that takes a flag as a parameter
// if flag value is 1, we check if the flag is set
// otherwise we check if the flag is not set
// Adds extra cycles directly to cpu->cycles: +1 if taken, +2 if taken and page cross
byte_t branch_on_flag(cpu_s *cpu, cpu_status_flag_e flag, byte_t flag_value)
{
    assert(cpu != NULL);
    // checks if the flag is set to the flag value
    if (get_flag(cpu, flag) == flag_value)
    {
        cpu->PC += 2; // Move past the 2-byte branch instruction
        word_t old_PC = cpu->PC;
        cpu->PC += address_rel;
        cpu->pc_changed = true;
        // +1 for branch taken, +1 more if page boundary crossed
        if (crosses_page(old_PC, cpu->PC))
        {
            cpu->cycles += 2;
        }
        else
        {
            cpu->cycles += 1;
        }
    }
    return 0;
}

/*
    Branch on Result Plus (N = 0)
    If the negative flag is not set, add the relative address to the program counter
    If the program counter crosses a page boundary, return 1 to indicate an extra cycle is required
*/
byte_t BPL(cpu_s *cpu)
{
    assert(cpu != NULL);
    byte_t cycles = branch_on_flag(cpu, STATUS_FLAG_N, 0);
    return cycles;
}

/*
 * Clear Carry Flag
 * Clears the carry flag
 */
byte_t CLC(cpu_s *cpu)
{
    assert(cpu != NULL);
    set_flag(cpu, STATUS_FLAG_C, false);
    return 0;
}

/*
    Jump to Subroutine
    Pushes the program counter onto the stack
    Sets the program counter to the address
*/
byte_t JSR(cpu_s *cpu)
{
    assert(cpu != NULL);
    word_t val = cpu->PC + 2; // push address of last byte of JSR instruction
    push_address(cpu, val);
    cpu->PC = address;
    cpu->pc_changed = true;
    return 0;
}

/*
    AND Memory with Accumulator
    ANDs the value with the accumulator
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
    This instruction can potentially take an extra cycle
*/
byte_t AND(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->A &= value;
    set_zn(cpu, cpu->A);
    return 1;
}

/*
    BIT Test Bits in Memory with Accumulator
    ANDs the value with the accumulator
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
    Sets the overflow flag to the sixth bit of the value
*/
byte_t BIT(cpu_s *cpu)
{
    assert(cpu != NULL);
    byte_t result = cpu->A & value;
    // this flag is set based on the result of the AND
    set_flag(cpu, STATUS_FLAG_Z, result == 0x00);
    // these 2 flags are set to the 6th and 7th bits of the value
    set_flag(cpu, STATUS_FLAG_N, value & 0x80);
    set_flag(cpu, STATUS_FLAG_V, value & 0x40);
    return 0;
}

/*
    ROL Rotate Left
    Rotates the value left by 1 bit through carry
    Old carry goes into bit 0, old bit 7 goes into carry
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
    Stores the result in accumulator or memory based on addressing mode
*/
byte_t ROL(cpu_s *cpu)
{
    assert(cpu != NULL);
    byte_t carry = (byte_t) get_flag(cpu, STATUS_FLAG_C);
    set_flag(cpu, STATUS_FLAG_C, value & 0x80);
    value <<= 1;
    value |= carry;
    set_zn(cpu, value);
    if (acc_mode)
    {
        cpu->A = value;
    }
    else
    {
        write_to_addr(cpu, address, value);
    }
    return 0;
}

/*
    Pull Processor Status from Stack
    The status register will be pulled with the break
    flag and bit 5 ignored.
*/
byte_t PLP(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->STATUS = pop_byte(cpu);
    set_flag(cpu, STATUS_FLAG_U, true);
    set_flag(cpu, STATUS_FLAG_B, false);
    return 0;
}

/*
    Branch on Result Minus (N = 1)
    If the negative flag is set, add the relative address to the program counter
    If the program counter crosses a page boundary, return 1 to indicate an extra cycle is required
*/
byte_t BMI(cpu_s *cpu)
{
    assert(cpu != NULL);
    byte_t cycles = branch_on_flag(cpu, STATUS_FLAG_N, 1);
    return cycles;
}

/*
    SEC Set Carry Flag
    Sets the carry flag to 1
*/
byte_t SEC(cpu_s *cpu)
{
    assert(cpu != NULL);
    set_flag(cpu, STATUS_FLAG_C, true);
    return 0;
}

/*
    Return from Interrupt
    Pulls the program counter from the stack
    Pulls the status register from the stack
    Bit 5 and the break flag are ignoreds
*/
byte_t RTI(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->STATUS = pop_byte(cpu);
    byte_t low = pop_byte(cpu);
    byte_t high = pop_byte(cpu);
    cpu->PC = assemble_word(high, low);
    set_flag(cpu, STATUS_FLAG_U, true); // this is always logical 1
    set_flag(cpu, STATUS_FLAG_B, false); // B flag is only 1 when BRK is executed
    cpu->pc_changed = true;
    return 0;
}

/*
    EOR Exclusive-OR Memory with Accumulator
    XORs the value with the accumulator
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/

byte_t EOR(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->A ^= value;
    set_zn(cpu, cpu->A);
    return 1; // Can take extra cycle on page cross
}

/*
    LSR Logical Shift Right
    Shifts the value right by 1 bit
    Sets the carry flag to the 0th bit of the value
    Sets the zero flag if the result is zero
    Sets the negative flag to 0 (bit 7 is always 0 after shift)
    Stores the result in accumulator or memory based on addressing mode
*/
byte_t LSR(cpu_s *cpu)
{
    assert(cpu != NULL);
    set_flag(cpu, STATUS_FLAG_C, value & 0x01);
    value >>= 1;
    set_zn(cpu, value);
    if (acc_mode)
    {
        cpu->A = value;
    }
    else
    {
        write_to_addr(cpu, address, value);
    }
    return 0;
}

/*
    PHA Push Accumulator on Stack
    Pushes the accumulator onto the stack
*/
byte_t PHA(cpu_s *cpu)
{
    assert(cpu != NULL);
    push_byte_to_stack(cpu, cpu->A);
    return 0;
}

/*
    Jump to New Location
    Sets the program counter to the address
    (PC+1) -> PCL
    (PC+2) -> PCH
*/
byte_t JMP(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->PC = address;
    cpu->pc_changed = true;
    return 0;
}

/*
    BVC Branch on Overflow Clear
    If the overflow flag is not set, add the relative address to the program counter
    If the program counter crosses a page boundary, return 1 to indicate an extra cycle is required
*/
byte_t BVC(cpu_s *cpu)
{
    assert(cpu != NULL);
    byte_t cycles = branch_on_flag(cpu, STATUS_FLAG_V, 0);
    return cycles;
}

/*
    CLI Clear Interrupt Disable Bit
    Sets the interrupt disable flag to 0
*/
byte_t CLI(cpu_s *cpu)
{
    assert(cpu != NULL);
    set_flag(cpu, STATUS_FLAG_I, false);
    return 0;
}

/*
    RTS Return from Subroutine
    Pulls the program counter from the stack
    Increments the program counter
*/
byte_t RTS(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->PC = pop_byte(cpu);
    cpu->PC |= (pop_byte(cpu) << 8);
    cpu->PC++; // increment to instruction after JSR
    cpu->pc_changed = true;
    return 0;
}

/*
    ADC Add Memory to Accumulator with Carry
    Adds the value to the accumulator
    Adds the carry flag to the result
    Sets the carry flag if the result is greater than 255
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
    Sets the overflow flag if the result is greater than 127 or less than -128
*/

byte_t ADC(cpu_s *cpu)
{
    assert(cpu != NULL);
    word_t result = cpu->A + value + get_flag(cpu, STATUS_FLAG_C);
    set_flag(cpu, STATUS_FLAG_C, result > 0xFF);
    set_flag(cpu, STATUS_FLAG_Z, (result & 0x00FF) == 0x0000);
    set_flag(cpu, STATUS_FLAG_N, result & 0x0080);
    // overflow flag is set if the sign of the result is different from the sign of the accumulator
    byte_t accumulator_msb = cpu->A & 0x80;
    byte_t value_msb = value & 0x80;
    byte_t result_msb = result & 0x80;
    byte_t overflow = ((accumulator_msb ^ value_msb) == 0) && ((accumulator_msb ^ result_msb) != 0);
    set_flag(cpu, STATUS_FLAG_V, overflow);
    cpu->A = (byte_t)(result & 0x00FF);
    return 1; // Can take extra cycle on page cross
}

/*
    ROR Rotate Right
    Rotates the value right by 1 bit through carry
    Old carry goes into bit 7, old bit 0 goes into carry
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
    Stores the result in accumulator or memory based on addressing mode
*/
byte_t ROR(cpu_s *cpu)
{
    assert(cpu != NULL);
    byte_t carry = (byte_t) get_flag(cpu, STATUS_FLAG_C);
    set_flag(cpu, STATUS_FLAG_C, value & 0x01);
    value >>= 1;
    value |= (carry << 7);
    set_zn(cpu, value);
    if (acc_mode)
    {
        cpu->A = value;
    }
    else
    {
        write_to_addr(cpu, address, value);
    }
    return 0;
}

/*
    PLA Pull Accumulator from Stack
    Pulls the accumulator from the stack
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
byte_t PLA(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->A = pop_byte(cpu);
    set_zn(cpu, cpu->A);
    return 0;
}

/*
    Branch on Overflow Set
    If the overflow flag is set, add the relative address to the program counter
    If the program counter crosses a page boundary, return 1 to indicate an extra cycle is required
*/
byte_t BVS(cpu_s *cpu)
{
    assert(cpu != NULL);
    byte_t cycles = branch_on_flag(cpu, STATUS_FLAG_V, 1);
    return cycles;
}

/*
    SEI Set Interrupt Disable Status
    Sets the interrupt disable flag to 1
*/
byte_t SEI(cpu_s *cpu)
{
    assert(cpu != NULL);
    set_flag(cpu, STATUS_FLAG_I, true);
    return 0;
}

/*
    STA Store Accumulator in Memory
    Stores the accumulator in memory
*/
byte_t STA(cpu_s *cpu)
{
    assert(cpu != NULL);
    write_to_addr(cpu, address, cpu->A);
    return 0;
}

/*
    STY Store Index Y in Memory
    Stores the Y register in memory
*/
byte_t STY(cpu_s *cpu)
{
    assert(cpu != NULL);
    write_to_addr(cpu, address, cpu->Y);
    return 0;
}

/*
    STX Store Index X in Memory
    Stores the X register in memory
*/
byte_t STX(cpu_s *cpu)
{
    assert(cpu != NULL);
    write_to_addr(cpu, address, cpu->X);
    return 0;
}

/*
    DEY Decrement Index Y by One
    Decrements the Y register by 1
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
byte_t DEY(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->Y--;
    set_zn(cpu, cpu->Y);
    return 0;
}

/*
    TXA Transfer Index X to Accumulator
    Transfers the X register to the accumulator
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
byte_t TXA(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->A = cpu->X;
    set_zn(cpu, cpu->A);
    return 0;
}

/*
    BCC Branch on Carry Clear
    If the carry flag is not set, add the relative address to the program counter
    If the program counter crosses a page boundary, return 1 to indicate an extra cycle is required
*/

byte_t BCC(cpu_s *cpu)
{
    assert(cpu != NULL);
    byte_t cycles = branch_on_flag(cpu, STATUS_FLAG_C, 0);
    return cycles;
}

/*
    TYA Transfer Index Y to Accumulator
    Transfers the Y register to the accumulator
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
byte_t TYA(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->A = cpu->Y;
    set_zn(cpu, cpu->A);
    return 0;
}

/*
    TXS Transfer Index X to Stack Register
    Transfers the X register to the stack pointer
*/
byte_t TXS(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->SP = cpu->X;
    return 0;
}

/*
    LDY Load Index Y
    Loads the Y register with the value
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
byte_t LDY(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->Y = value;
    set_zn(cpu, cpu->Y);
    return 1; // Can take extra cycle on page cross
}

/*
    LDA Load Accumulator
    Loads the accumulator with the value
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
byte_t LDA(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->A = value;
    set_zn(cpu, cpu->A);
    return 1;
}

/*
    LDX Load Index X
    Loads the X register with the value
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
byte_t LDX(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->X = value;
    set_zn(cpu, cpu->X);
    return 1; // Can take extra cycle on page cross
}

/*
    TAY Transfer Accumulator to Index Y
    Transfers the accumulator to the Y register
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
byte_t TAY(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->Y = cpu->A;
    set_zn(cpu, cpu->Y);
    return 0;
}

/*
    TAX Transfer Accumulator to Index X
    Transfers the accumulator to the X register
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
byte_t TAX(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->X = cpu->A;
    set_zn(cpu, cpu->X);
    return 0;
}

/*
    BCS Branch on Carry Set
    If the carry flag is set, add the relative address to the program counter
    If the program counter crosses a page boundary, return 1 to indicate an extra cycle is required
*/
byte_t BCS(cpu_s *cpu)
{
    assert(cpu != NULL);
    byte_t cycles = branch_on_flag(cpu, STATUS_FLAG_C, 1);
    return cycles;
}

/*
    CLV Clear Overflow Flag
    Sets the overflow flag to 0
*/
byte_t CLV(cpu_s *cpu)
{
    assert(cpu != NULL);
    set_flag(cpu, STATUS_FLAG_V, false);
    return 0;
}

/*
    TSX Transfer Stack Pointer to Index X
    Transfers the stack pointer to the X register
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
byte_t TSX(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->X = cpu->SP;
    set_zn(cpu, cpu->X);
    return 0;
}

/*
    CPY Compare value and Index Y
    Compares the value with the Y register
    Sets the carry flag if the Y register is greater than or equal to the value
    Sets the zero flag if the Y register is equal to the value
    Sets the negative flag if the Y register is less than the value
*/
byte_t CPY(cpu_s *cpu)
{
    assert(cpu != NULL);
    byte_t result = cpu->Y - value;
    set_flag(cpu, STATUS_FLAG_C, cpu->Y >= value);
    set_zn(cpu, result);
    return 0;
}

/*
    CMP Compare value and Accumulator
    Compares the value with the accumulator
    Sets the carry flag if the accumulator is greater than or equal to the value
    Sets the zero flag if the accumulator is equal to the value
    Sets the negative flag if the accumulator is less than the value
*/
byte_t CMP(cpu_s *cpu)
{
    assert(cpu != NULL);
    byte_t result = cpu->A - value;
    set_flag(cpu, STATUS_FLAG_C, cpu->A >= value);
    set_zn(cpu, result);
    return 1; // Can take extra cycle on page cross
}

/*
    DEC Decrement Memory by One
    Decrements the value by 1
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
    Stores the result in memory
*/
byte_t DEC(cpu_s *cpu)
{
    assert(cpu != NULL);
    value--;
    set_zn(cpu, value);
    write_to_addr(cpu, address, value);
    return 0;
}

/*
    DEX Decrement Index X by One
    Decrements the X register by 1
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
byte_t DEX(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->X--;
    set_zn(cpu, cpu->X);
    return 0;
}

/*
    BNE Branch on Result not Zero
    If the zero flag is not set, add the relative address to the program counter
    If the program counter crosses a page boundary, return 1 to indicate an extra cycle is required
*/
byte_t BNE(cpu_s *cpu)
{
    assert(cpu != NULL);
    byte_t cycles = branch_on_flag(cpu, STATUS_FLAG_Z, 0);
    return cycles;
}

/*
    CLD Clear Decimal Mode
    Sets the decimal flag to 0
*/
byte_t CLD(cpu_s *cpu)
{
    assert(cpu != NULL);
    set_flag(cpu, STATUS_FLAG_D, false);
    return 0;
}

/*
    CPX Compare value and Index X
    Compares the value with the X register
    Sets the carry flag if the X register is greater than or equal to the value
    Sets the zero flag if the X register is equal to the value
    Sets the negative flag if the X register is less than the value
*/
byte_t CPX(cpu_s *cpu)
{
    assert(cpu != NULL);
    byte_t result = cpu->X - value;
    set_flag(cpu, STATUS_FLAG_C, cpu->X >= value);
    set_zn(cpu, result);
    return 0;
}

/*
    SBC Subtract Memory from Accumulator with Borrow
    Subtracts the value from the accumulator
    Subtracts the carry flag from the result
    Sets the carry flag if the result is less than 0
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
    Sets the overflow flag if the result is greater than 127 or less than -128
*/
byte_t SBC(cpu_s *cpu)
{
    assert(cpu != NULL);
    word_t result = cpu->A - value - (1 - ((byte_t) get_flag(cpu, STATUS_FLAG_C)));
    set_flag(cpu, STATUS_FLAG_C, result < 0x100);
    set_flag(cpu, STATUS_FLAG_Z, (result & 0x00FF) == 0x0000);
    set_flag(cpu, STATUS_FLAG_N, result & 0x0080);
    // overflow flag is set if the sign of the result is different from the sign of the accumulator
    byte_t accumulator_msb = cpu->A & 0x80;
    byte_t value_msb = value & 0x80;
    byte_t result_msb = result & 0x80;
    byte_t overflow = ((accumulator_msb ^ value_msb) != 0) && ((accumulator_msb ^ result_msb) != 0);
    set_flag(cpu, STATUS_FLAG_V, overflow);
    cpu->A = (byte_t)(result & 0x00FF);
    return 1; // Can take extra cycle on page cross
}

/*
    INC Increment Memory by One
    Increments the value by 1
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
    Stores the result in memory
*/
byte_t INC(cpu_s *cpu)
{
    assert(cpu != NULL);
    value++;
    set_zn(cpu, value);
    write_to_addr(cpu, address, value);
    return 0;
}

/*
    INX Increment Index X by One
    Increments the X register by 1
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
byte_t INX(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->X++;
    set_zn(cpu, cpu->X);
    return 0;
}

/*
    INY Increment Index Y by One
    Increments the Y register by 1
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
byte_t INY(cpu_s *cpu)
{
    assert(cpu != NULL);
    cpu->Y++;
    set_flag(cpu, STATUS_FLAG_Z, cpu->Y == 0x00);
    set_flag(cpu, STATUS_FLAG_N, cpu->Y & 0x80);
    return 0;
}

/*
    BEQ Branch on Result Zero
    If the zero flag is set, add the relative address to the program counter
    If the program counter crosses a page boundary, return 1 to indicate an extra cycle is required
*/
byte_t BEQ(cpu_s *cpu)
{
    assert(cpu != NULL);
    const byte_t cycles = branch_on_flag(cpu, STATUS_FLAG_Z, 1);
    return cycles;
}

/*
    SED Set Decimal Flag
    Sets the decimal flag to 1
*/
byte_t SED(cpu_s *cpu)
{
    assert(cpu != NULL);
    set_flag(cpu, STATUS_FLAG_D, true);
    return 0;
}

/*
    NOP No Operation
    Does nothing
*/
byte_t NOP(cpu_s *cpu)
{
    assert(cpu != NULL);
    return 0;
}

/*
    Run a single instruction (fetch, decode, execute, update PC)
    Tracks cycle count for timing accuracy.

    Cycle counting:
    - Base cycles come from instruction->cycles
    - Extra cycle for page boundary crossing (indexed addressing modes)
      only if instruction allows it (execute returns 1)
    - Branch instructions add their own extra cycles directly
*/
void run_instruction(cpu_s *cpu)
{
    assert(cpu != NULL);

    cpu->current_opcode = bus_read(cpu->bus, cpu->PC);
    cpu->instruction_pending = true;
    cpu->pc_changed = false;

    cpu_instruction_s *instr = get_current_instruction(cpu);

    // Fetch operand - returns 1 if page boundary crossed (for indexed modes)
    byte_t page_crossed = 0;
    if (instr->data_fetch) {
        page_crossed = instr->data_fetch(cpu);
    }

    // Execute - returns 1 if instruction can take page-crossing penalty
    // (branches handle their own extra cycles internally)
    byte_t can_take_penalty = 0;
    if (instr->execute) {
        can_take_penalty = instr->execute(cpu);
    }

    // Add base cycles
    cpu->cycles += instr->cycles;

    // Add page-crossing penalty if applicable
    if (page_crossed && can_take_penalty) {
        cpu->cycles += 1;
    }

    // Advance PC if instruction didn't modify it
    if (!cpu->pc_changed) {
        cpu->PC += instr->length;
    }

    cpu->instruction_pending = false;
}
