#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#define UNIMPLEMENTED() \
    fprintf(stderr, "%s:%d: %s: Unimplemented function\n", __FILE__, __LINE__, __func__); \
    abort();
#define MEM_SIZE 1024 * 1024 * 64
//macro for zeroing out the high byte of a word
#define zero_high_byte(word) word &= 0x00FF 
//macro to push_byte an address to the stack
#define push_addr(cpu, addr) \
    push_byte(cpu, (addr & 0xFF00) >> 8); \
    push_byte(cpu, addr & 0x00FF);
//macro to set Z and N flags
#define set_zn(cpu, value) \
    set_flag(cpu, Z, value == 0); \
    set_flag(cpu, N, value & 0x80);

Word address; //used by absolute, zero page, and indirect addressing modes
Byte address_rel; //used only by branch instructions
Byte value; //fetched value from address

void print_cpu_state(CPU *cpu) {
    printf("A: 0x%02X\n", cpu->A);
    printf("X: 0x%02X\n", cpu->X);
    printf("Y: 0x%02X\n", cpu->Y);
    printf("SP: 0x%02X\n", cpu->SP);
    printf("PC: 0x%04X\n", cpu->PC);
    printf("STATUS: 0x%02X\n", cpu->STATUS);
    return;
}

void print_instruction(Byte opcode,Instruction *table) {
    Instruction instruction = table[opcode];
    //if any pointers are null, print "not implemented"
    if (!(instruction.name == NULL || instruction.fetch == NULL || instruction.execute == NULL)) {
        printf("Instruction: %s\n", instruction.name);
        printf("Opcode: 0x%02X\n", instruction.opcode);
        printf("Fetch: %p\n", (void*) instruction.fetch);
        printf("Execute: %p\n", (void*) instruction.execute);
        printf("Cycles: %d\n", instruction.cycles);
        printf("Length: %d\n", instruction.length);
    } 
    return;
}

void init_instruction_table(Instruction* table){
    //reference https://www.masswerk.at/6502/6502_instruction_set.html
    table[INSTRUCTION_BRK_IMP] = (Instruction){
        .name = "BRK",
        .opcode = 0x00,
        .fetch = IMP,
        .execute = BRK,
        .cycles = 7,
        .length = 2 //this is actually 1, but we need to account for the dummy byte
    };
    table[INSTRUCTION_ORA_IZX] = (Instruction) {
        .name = "ORA",
        .opcode = 0x01,
        .fetch = IZX,
        .execute = ORA,
        .cycles = 6,
        .length = 2
    };
    table[0x02] = (Instruction) {
        .name = "???",
        .opcode = 0x02,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x03] = (Instruction) {
        .name = "???",
        .opcode = 0x03,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x04] = (Instruction) {
        .name = "???",
        .opcode = 0x04,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_ORA_ZP0] = (Instruction) {
        .name = "ORA",
        .opcode = 0x05,
        .fetch = ZP0,
        .execute = ORA,
        .cycles = 3,
        .length = 2
    };
    table[INSTRUCTION_ASL_ZP0] = (Instruction) {
        .name = "ASL",
        .opcode = 0x06,
        .fetch = ZP0,
        .execute = ASL,
        .cycles = 5,
        .length = 2
    };
    table[0x07] = (Instruction) {
        .name = "???",
        .opcode = 0x07,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_PHP_IMP] = (Instruction) {
        .name = "PHP",
        .opcode = 0x08,
        .fetch = IMP,
        .execute = PHP,
        .cycles = 3,
        .length = 1
    };
    table[INSTRUCTION_ORA_IMM] = (Instruction) {
        .name = "ORA",
        .opcode = 0x09,
        .fetch = IMM,
        .execute = ORA,
        .cycles = 2,
        .length = 2
    };
    table[INSTRUCTION_ASL_ACC] = (Instruction) {
        .name = "ASL",
        .opcode = 0x0A,
        .fetch = IMP,
        .execute = ASL,
        .cycles = 2,
        .length = 1
    };
    table[0x0B] = (Instruction) {
        .name = "???",
        .opcode = 0x0B,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x0C] = (Instruction) {
        .name = "???",
        .opcode = 0x0C,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_ORA_ABS] = (Instruction) {
        .name = "ORA",
        .opcode = 0x0D,
        .fetch = ABS,
        .execute = ORA,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_ASL_ABS] = (Instruction) {
        .name = "ASL",
        .opcode = 0x0E,
        .fetch = ABS,
        .execute = ASL,
        .cycles = 6,
        .length = 3
    };
    table[0x0F] = (Instruction) {
        .name = "???",
        .opcode = 0x0F,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_BPL_REL] = (Instruction) {
        .name = "BPL",
        .opcode = 0x10,
        .fetch = REL,
        .execute = BPL,
        .cycles = 2,
        .length = 2
    };
    table[INSTRUCTION_ORA_IZY] = (Instruction) {
        .name = "ORA",
        .opcode = 0x11,
        .fetch = IZY,
        .execute = ORA,
        .cycles = 5,
        .length = 2
    };
    table[0x12] = (Instruction) {
        .name = "???",
        .opcode = 0x12,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x13] = (Instruction) {
        .name = "???",
        .opcode = 0x13,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x14] = (Instruction) {
        .name = "???",
        .opcode = 0x14,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_ORA_ZPX] = (Instruction) {
        .name = "ORA",
        .opcode = 0x15,
        .fetch = ZPX,
        .execute = ORA,
        .cycles = 4,
        .length = 2
    };
    table[INSTRUCTION_ASL_ZPX] = (Instruction) {
        .name = "ASL",
        .opcode = 0x16,
        .fetch = ZPX,
        .execute = ASL,
        .cycles = 6,
        .length = 2
    };
    table[0x17] = (Instruction) {
        .name = "???",
        .opcode = 0x17,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_CLC_IMP] = (Instruction) {
        .name = "CLC",
        .opcode = 0x18,
        .fetch = IMP,
        .execute = CLC,
        .cycles = 2,
        .length = 1
    };
    table[INSTRUCTION_ORA_ABY] = (Instruction) {
        .name = "ORA",
        .opcode = 0x19,
        .fetch = ABY,
        .execute = ORA,
        .cycles = 4,
        .length = 3
    };
    table[0x1A] = (Instruction) {
        .name = "???",
        .opcode = 0x1A,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x1B] = (Instruction) {
        .name = "???",
        .opcode = 0x1B,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x1C] = (Instruction) {
        .name = "???",
        .opcode = 0x1C,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_ORA_ABX] = (Instruction) {
        .name = "ORA",
        .opcode = 0x1D,
        .fetch = ABX,
        .execute = ORA,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_ASL_ABX] = (Instruction) {
        .name = "ASL",
        .opcode = 0x1E,
        .fetch = ABX,
        .execute = ASL,
        .cycles = 7,
        .length = 3
    };
    table[0x1F] = (Instruction) {
        .name = "???",
        .opcode = 0x1F,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_JSR_ABS] = (Instruction) {
        .name = "JSR",
        .opcode = 0x20,
        .fetch = ABS,
        .execute = JSR,
        .cycles = 6,
        .length = 3
    };
    table[INSTRUCTION_AND_IZX] = (Instruction) {
        .name = "AND",
        .opcode = 0x21,
        .fetch = IZX,
        .execute = AND,
        .cycles = 6,
        .length = 2
    };
    table[0x22] = (Instruction) {
        .name = "???",
        .opcode = 0x22,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x23] = (Instruction) {
        .name = "???",
        .opcode = 0x23,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_BRK_ZP0] = (Instruction) {
        .name = "BIT",
        .opcode = 0x24,
        .fetch = ZP0,
        .execute = BIT,
        .cycles = 3,
        .length = 2
    };
    table[INSTRUCTION_AND_ZP0] = (Instruction) {
        .name = "AND",
        .opcode = 0x25,
        .fetch = ZP0,
        .execute = AND,
        .cycles = 3,
        .length = 2
    };
    table[INSTRUCTION_ROL_ZP0] = (Instruction) {
        .name = "ROL",
        .opcode = 0x26,
        .fetch = ZP0,
        .execute = ROL,
        .cycles = 5,
        .length = 2
    };
    table[0x27] = (Instruction) {
        .name = "???",
        .opcode = 0x27,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_PLP_IMP] = (Instruction) {
        .name = "PLP",
        .opcode = 0x28,
        .fetch = IMP,
        .execute = PLP,
        .cycles = 4,
        .length = 1
    };
    table[INSTRUCTION_AND_IMM] = (Instruction) {
        .name = "AND",
        .opcode = 0x29,
        .fetch = IMM,
        .execute = AND,
        .cycles = 2,
        .length = 2
    };
    table[INSTRUCTION_ROL_ACC] = (Instruction) {
        .name = "ROL",
        .opcode = 0x2A,
        .fetch = IMP,
        .execute = ROL_ACC,
        .cycles = 2,
        .length = 1
    };
    table[0x2B] = (Instruction) {
        .name = "???",
        .opcode = 0x2B,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_BIT_ABS] = (Instruction) {
        .name = "BIT",
        .opcode = 0x2C,
        .fetch = ABS,
        .execute = BIT,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_AND_ABS] = (Instruction) {
        .name = "AND",
        .opcode = 0x2D,
        .fetch = ABS,
        .execute = AND,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_ROL_ABS] = (Instruction) {
        .name = "ROL",
        .opcode = 0x2E,
        .fetch = ABS,
        .execute = ROL,
        .cycles = 6,
        .length = 3
    };
    table[0x2F] = (Instruction) {
        .name = "???",
        .opcode = 0x2F,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_BMI_REL] = (Instruction) {
        .name = "BMI",
        .opcode = 0x30,
        .fetch = REL,
        .execute = BMI,
        .cycles = 2,
        .length = 2
    };
    table[INSTRUCTION_AND_IZY] = (Instruction) {
        .name = "AND",
        .opcode = 0x31,
        .fetch = IZY,
        .execute = AND,
        .cycles = 5,
        .length = 2
    };
    table[0x32] = (Instruction) {
        .name = "???",
        .opcode = 0x32,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x33] = (Instruction) {
        .name = "???",
        .opcode = 0x33,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_AND_ZPX] = (Instruction) {
        .name = "AND",
        .opcode = 0x35,
        .fetch = ZPX,
        .execute = AND,
        .cycles = 4,
        .length = 2
    };
    table[INSTRUCTION_ROL_ZPX] = (Instruction) {
        .name = "ROL",
        .opcode = 0x36,
        .fetch = ZPX,
        .execute = ROL,
        .cycles = 6,
        .length = 2
    };
    table[0x37] = (Instruction) {
        .name = "???",
        .opcode = 0x37,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_SEC_IMP] = (Instruction) {
        .name = "SEC",
        .opcode = 0x38,
        .fetch = IMP,
        .execute = SEC,
        .cycles = 2,
        .length = 1
    };
    table[INSTRUCTION_AND_ABY] = (Instruction) {
        .name = "AND",
        .opcode = 0x39,
        .fetch = ABY,
        .execute = AND,
        .cycles = 4,
        .length = 3
    };
    table[0x3A] = (Instruction) {
        .name = "???",
        .opcode = 0x3A,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x3B] = (Instruction) {
        .name = "???",
        .opcode = 0x3B,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_AND_ABX] = (Instruction) {
        .name = "AND",
        .opcode = 0x3D,
        .fetch = ABX,
        .execute = AND,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_ROL_ABX] = (Instruction) {
        .name = "ROL",
        .opcode = 0x3E,
        .fetch = ABX,
        .execute = ROL,
        .cycles = 7,
        .length = 3
    };
    table[0x3F] = (Instruction) {
        .name = "???",
        .opcode = 0x3F,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_RTI_IMP] = (Instruction) {
        .name = "RTI",
        .opcode = 0x40,
        .fetch = IMP,
        .execute = RTI,
        .cycles = 6,
        .length = 1
    };
    table[INSTRUCTION_EOR_IZX] = (Instruction) {
        .name = "EOR",
        .opcode = 0x41,
        .fetch = IZX,
        .execute = EOR,
        .cycles = 6,
        .length = 2
    };
    table[0x42] = (Instruction) {
        .name = "???",
        .opcode = 0x42,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x43] = (Instruction) {
        .name = "???",
        .opcode = 0x43,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_EOR_ZP0] = (Instruction) {
        .name = "EOR",
        .opcode = 0x45,
        .fetch = ZP0,
        .execute = EOR,
        .cycles = 3,
        .length = 2
    };
    table[INSTRUCTION_LSR_ZP0] = (Instruction) {
        .name = "LSR",
        .opcode = 0x46,
        .fetch = ZP0,
        .execute = LSR,
        .cycles = 5,
        .length = 2
    };
    table[0x47] = (Instruction) {
        .name = "???",
        .opcode = 0x47,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_PHA_IMP] = (Instruction) {
        .name = "PHA",
        .opcode = 0x48,
        .fetch = IMP,
        .execute = PHA,
        .cycles = 3,
        .length = 1
    };
    table[INSTRUCTION_EOR_IMM] = (Instruction) {
        .name = "EOR",
        .opcode = 0x49,
        .fetch = IMM,
        .execute = EOR,
        .cycles = 2,
        .length = 2
    };
    table[INSTRUCTION_LSR_ACC] = (Instruction) {
        .name = "LSR",
        .opcode = 0x4A,
        .fetch = IMP,
        .execute = LSR_ACC,
        .cycles = 2,
        .length = 1
    };
    table[0x4B] = (Instruction) {
        .name = "???",
        .opcode = 0x4B,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_JMP_ABS] = (Instruction) {
        .name = "JMP",
        .opcode = 0x4C,
        .fetch = ABS,
        .execute = JMP,
        .cycles = 3,
        .length = 3
    };
    table[INSTRUCTION_EOR_ABS] = (Instruction) {
        .name = "EOR",
        .opcode = 0x4D,
        .fetch = ABS,
        .execute = EOR,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_LSR_ABS] = (Instruction) {
        .name = "LSR",
        .opcode = 0x4E,
        .fetch = ABS,
        .execute = LSR,
        .cycles = 6,
        .length = 3
    };
    table[0x4F] = (Instruction) {
        .name = "???",
        .opcode = 0x4F,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_BVC_REL] = (Instruction) {
        .name = "BVC",
        .opcode = 0x50,
        .fetch = REL,
        .execute = BVC,
        .cycles = 2,
        .length = 2
    };
    table[INSTRUCTION_EOR_IZY] = (Instruction) {
        .name = "EOR",
        .opcode = 0x51,
        .fetch = IZY,
        .execute = EOR,
        .cycles = 5,
        .length = 2
    };
    table[0x52] = (Instruction) {
        .name = "???",
        .opcode = 0x52,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x53] = (Instruction) {
        .name = "???",
        .opcode = 0x53,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_EOR_ZPX] = (Instruction) {
        .name = "EOR",
        .opcode = 0x55,
        .fetch = ZPX,
        .execute = EOR,
        .cycles = 4,
        .length = 2
    };
    table[INSTRUCTION_LSR_ZPX] = (Instruction) {
        .name = "LSR",
        .opcode = 0x56,
        .fetch = ZPX,
        .execute = LSR,
        .cycles = 6,
        .length = 2
    };
    table[0x57] = (Instruction) {
        .name = "???",
        .opcode = 0x57,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_CLI_IMP] = (Instruction) {
        .name = "CLI",
        .opcode = 0x58,
        .fetch = IMP,
        .execute = CLI,
        .cycles = 2,
        .length = 1
    };
    table[INSTRUCTION_EOR_ABY] = (Instruction) {
        .name = "EOR",
        .opcode = 0x59,
        .fetch = ABY,
        .execute = EOR,
        .cycles = 4,
        .length = 3
    };
    table[0x5A] = (Instruction) {
        .name = "???",
        .opcode = 0x5A,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x5B] = (Instruction) {
        .name = "???",
        .opcode = 0x5B,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_EOR_ABX] = (Instruction) {
        .name = "EOR",
        .opcode = 0x5D,
        .fetch = ABX,
        .execute = EOR,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_LSR_ABX] = (Instruction) {
        .name = "LSR",
        .opcode = 0x5E,
        .fetch = ABX,
        .execute = LSR,
        .cycles = 7,
        .length = 3
    };
    table[0x5F] = (Instruction) {
        .name = "???",
        .opcode = 0x5F,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_RTS_IMP] = (Instruction) {
        .name = "RTS",
        .opcode = 0x60,
        .fetch = IMP,
        .execute = RTS,
        .cycles = 6,
        .length = 1
    };
    table[INSTRUCTION_ADC_IZX] = (Instruction) {
        .name = "ADC",
        .opcode = 0x61,
        .fetch = IZX,
        .execute = ADC,
        .cycles = 6,
        .length = 2
    };
    table[0x62] = (Instruction) {
        .name = "???",
        .opcode = 0x62,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x63] = (Instruction) {
        .name = "???",
        .opcode = 0x63,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x64] = (Instruction) {
        .name = "???",
        .opcode = 0x64,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_ADC_ZP0] = (Instruction) {
        .name = "ADC",
        .opcode = 0x65,
        .fetch = ZP0,
        .execute = ADC,
        .cycles = 3,
        .length = 2
    };
    table[INSTRUCTION_ROR_ZP0] = (Instruction) {
        .name = "ROR",
        .opcode = 0x66,
        .fetch = ZP0,
        .execute = ROR,
        .cycles = 5,
        .length = 2
    };
    table[0x67] = (Instruction) {
        .name = "???",
        .opcode = 0x67,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_PLA_IMP] = (Instruction) {
        .name = "PLA",
        .opcode = 0x68,
        .fetch = IMP,
        .execute = PLA,
        .cycles = 4,
        .length = 1
    };
    table[INSTRUCTION_ADC_IMM] = (Instruction) {
        .name = "ADC",
        .opcode = 0x69,
        .fetch = IMM,
        .execute = ADC,
        .cycles = 2,
        .length = 2
    };
    table[INSTRUCTION_ROR_ACC] = (Instruction) {
        .name = "ROR",
        .opcode = 0x6A,
        .fetch = IMP,
        .execute = ROR_ACC,
        .cycles = 2,
        .length = 1
    };
    table[0x6B] = (Instruction) {
        .name = "???",
        .opcode = 0x6B,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_JMP_IND] = (Instruction) {
        .name = "JMP",
        .opcode = 0x6C,
        .fetch = IND,
        .execute = JMP,
        .cycles = 5,
        .length = 3
    };
    table[INSTRUCTION_ADC_ABS] = (Instruction) {
        .name = "ADC",
        .opcode = 0x6D,
        .fetch = ABS,
        .execute = ADC,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_ROR_ABS] = (Instruction) {
        .name = "ROR",
        .opcode = 0x6E,
        .fetch = ABS,
        .execute = ROR,
        .cycles = 6,
        .length = 3
    };
    table[0x6F] = (Instruction) {
        .name = "???",
        .opcode = 0x6F,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_BVS_REL] = (Instruction) {
        .name = "BVS",
        .opcode = 0x70,
        .fetch = REL,
        .execute = BVS,
        .cycles = 2,
        .length = 2
    };
    table[INSTRUCTION_ADC_IZY] = (Instruction) {
        .name = "ADC",
        .opcode = 0x71,
        .fetch = IZY,
        .execute = ADC,
        .cycles = 5,
        .length = 2
    };
    table[0x72] = (Instruction) {
        .name = "???",
        .opcode = 0x72,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x73] = (Instruction) {
        .name = "???",
        .opcode = 0x73,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x74] = (Instruction) {
        .name = "???",
        .opcode = 0x74,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_ADC_ZPX] = (Instruction) {
        .name = "ADC",
        .opcode = 0x75,
        .fetch = ZPX,
        .execute = ADC,
        .cycles = 4,
        .length = 2
    };
    table[INSTRUCTION_ROR_ZPX] = (Instruction) {
        .name = "ROR",
        .opcode = 0x76,
        .fetch = ZPX,
        .execute = ROR,
        .cycles = 6,
        .length = 2
    };
    table[0x77] = (Instruction) {
        .name = "???",
        .opcode = 0x77,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_SEI_IMP] = (Instruction) {
        .name = "SEI",
        .opcode = 0x78,
        .fetch = IMP,
        .execute = SEI,
        .cycles = 2,
        .length = 1
    };
    table[INSTRUCTION_ADC_ABY] = (Instruction) {
        .name = "ADC",
        .opcode = 0x79,
        .fetch = ABY,
        .execute = ADC,
        .cycles = 4,
        .length = 3
    };
    table[0x7A] = (Instruction) {
        .name = "???",
        .opcode = 0x7A,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x7B] = (Instruction) {
        .name = "???",
        .opcode = 0x7B,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x7C] = (Instruction) {
        .name = "???",
        .opcode = 0x7C,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_ADC_ABX] = (Instruction) {
        .name = "ADC",
        .opcode = 0x7D,
        .fetch = ABX,
        .execute = ADC,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_ROR_ABX] = (Instruction) {
        .name = "ROR",
        .opcode = 0x7E,
        .fetch = ABX,
        .execute = ROR,
        .cycles = 7,
        .length = 3
    };
    table[0x7F] = (Instruction) {
        .name = "???",
        .opcode = 0x7F,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_STA_IZX] = (Instruction) {
        .name = "STA",
        .opcode = 0x81,
        .fetch = IZX,
        .execute = STA,
        .cycles = 6,
        .length = 2
    };
    table[0x82] = (Instruction) {
        .name = "???",
        .opcode = 0x82,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_STY_ZP0] = (Instruction) {
        .name = "STY",
        .opcode = 0x84,
        .fetch = ZP0,
        .execute = STY,
        .cycles = 3,
        .length = 2
    };
    table[INSTRUCTION_STA_ZP0] = (Instruction) {
        .name = "STA",
        .opcode = 0x85,
        .fetch = ZP0,
        .execute = STA,
        .cycles = 3,
        .length = 2
    };
    table[INSTRUCTION_STX_ZP0] = (Instruction) {
        .name = "STX",
        .opcode = 0x86,
        .fetch = ZP0,
        .execute = STX,
        .cycles = 3,
        .length = 2
    };
    table[0x87] = (Instruction) {
        .name = "???",
        .opcode = 0x87,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_DEY_IMP] = (Instruction) {
        .name = "DEY",
        .opcode = 0x88,
        .fetch = IMP,
        .execute = DEY,
        .cycles = 2,
        .length = 1
    };
    table[0x89] = (Instruction) {
        .name = "???",
        .opcode = 0x89,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_TXA_IMP] = (Instruction) {
        .name = "TXA",
        .opcode = 0x8A,
        .fetch = IMP,
        .execute = TXA,
        .cycles = 2,
        .length = 1
    };
    table[0x8B] = (Instruction) {
        .name = "???",
        .opcode = 0x8B,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_STY_ABS] = (Instruction) {
        .name = "STY",
        .opcode = 0x8C,
        .fetch = ABS,
        .execute = STY,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_STA_ABS] = (Instruction) {
        .name = "STA",
        .opcode = 0x8D,
        .fetch = ABS,
        .execute = STA,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_STX_ABS] = (Instruction) {
        .name = "STX",
        .opcode = 0x8E,
        .fetch = ABS,
        .execute = STX,
        .cycles = 4,
        .length = 3
    };
    table[0x8F] = (Instruction) {
        .name = "???",
        .opcode = 0x8F,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_BCC_REL] = (Instruction) {
        .name = "BCC",
        .opcode = 0x90,
        .fetch = REL,
        .execute = BCC,
        .cycles = 2,
        .length = 2
    };
    table[INSTRUCTION_STA_IZY] = (Instruction) {
        .name = "STA",
        .opcode = 0x91,
        .fetch = IZY,
        .execute = STA,
        .cycles = 6,
        .length = 2
    };
    table[0x92] = (Instruction) {
        .name = "???",
        .opcode = 0x92,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x93] = (Instruction) {
        .name = "???",
        .opcode = 0x93,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_STY_ZPX] = (Instruction) {
        .name = "STY",
        .opcode = 0x94,
        .fetch = ZPX,
        .execute = STY,
        .cycles = 4,
        .length = 2
    };
    table[INSTRUCTION_STA_ZPX] = (Instruction) {
        .name = "STA",
        .opcode = 0x95,
        .fetch = ZPX,
        .execute = STA,
        .cycles = 4,
        .length = 2
    };
    table[INSTRUCTION_STX_ZPY] = (Instruction) {
        .name = "STX",
        .opcode = 0x96,
        .fetch = ZPY,
        .execute = STX,
        .cycles = 4,
        .length = 2
    };
    table[0x97] = (Instruction) {
        .name = "???",
        .opcode = 0x97,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_TYA_IMP] = (Instruction) {
        .name = "TYA",
        .opcode = 0x98,
        .fetch = IMP,
        .execute = TYA,
        .cycles = 2,
        .length = 1
    };
    table[INSTRUCTION_STA_ABY] = (Instruction) {
        .name = "STA",
        .opcode = 0x99,
        .fetch = ABY,
        .execute = STA,
        .cycles = 5,
        .length = 3
    };
    table[INSTRUCTION_TXS_IMP] = (Instruction) {
        .name = "TXS",
        .opcode = 0x9A,
        .fetch = IMP,
        .execute = TXS,
        .cycles = 2,
        .length = 1
    };
    table[0x9B] = (Instruction) {
        .name = "???",
        .opcode = 0x9B,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x9C] = (Instruction) {
        .name = "???",
        .opcode = 0x9C,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_STA_ABX] = (Instruction) {
        .name = "STA",
        .opcode = 0x9D,
        .fetch = ABX,
        .execute = STA,
        .cycles = 5,
        .length = 3
    };
    table[0x9E] = (Instruction) {
        .name = "???",
        .opcode = 0x9E,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x9F] = (Instruction) {
        .name = "???",
        .opcode = 0x9F,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_LDY_IMM] = (Instruction) {
        .name = "LDY",
        .opcode = 0xA0,
        .fetch = IMM,
        .execute = LDY,
        .cycles = 2,
        .length = 2
    };
    table[INSTRUCTION_LDA_IZX] = (Instruction) {
        .name = "LDA",
        .opcode = 0xA1,
        .fetch = IZX,
        .execute = LDA,
        .cycles = 6,
        .length = 2
    };
    table[INSTRUCTION_LDX_IMM] = (Instruction) {
        .name = "LDX",
        .opcode = 0xA2,
        .fetch = IMM,
        .execute = LDX,
        .cycles = 2,
        .length = 2
    };
    table[0xA3] = (Instruction) {
        .name = "???",
        .opcode = 0xA3,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_LDY_ZP0] = (Instruction) {
        .name = "LDY",
        .opcode = 0xA4,
        .fetch = ZP0,
        .execute = LDY,
        .cycles = 3,
        .length = 2
    };
    table[INSTRUCTION_LDA_ZP0] = (Instruction) {
        .name = "LDA",
        .opcode = 0xA5,
        .fetch = ZP0,
        .execute = LDA,
        .cycles = 3,
        .length = 2
    };
    table[INSTRUCTION_LDX_ZP0] = (Instruction) {
        .name = "LDX",
        .opcode = 0xA6,
        .fetch = ZP0,
        .execute = LDX,
        .cycles = 3,
        .length = 2
    };
    table[0xA7] = (Instruction) {
        .name = "???",
        .opcode = 0xA7,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_TAY_IMP] = (Instruction) {
        .name = "TAY",
        .opcode = 0xA8,
        .fetch = IMP,
        .execute = TAY,
        .cycles = 2,
        .length = 1
    };
    table[INSTRUCTION_LDA_IMM] = (Instruction) {
        .name = "LDA",
        .opcode = 0xA9,
        .fetch = IMM,
        .execute = LDA,
        .cycles = 2,
        .length = 2
    };
    table[INSTRUCTION_TAX_IMP] = (Instruction) {
        .name = "TAX",
        .opcode = 0xAA,
        .fetch = IMP,
        .execute = TAX,
        .cycles = 2,
        .length = 1
    };
    table[0xAB] = (Instruction) {
        .name = "???",
        .opcode = 0xAB,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_LDY_ABS] = (Instruction) {
        .name = "LDY",
        .opcode = 0xAC,
        .fetch = ABS,
        .execute = LDY,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_LDA_ABS] = (Instruction) {
        .name = "LDA",
        .opcode = 0xAD,
        .fetch = ABS,
        .execute = LDA,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_LDX_ABS] = (Instruction) {
        .name = "LDX",
        .opcode = 0xAE,
        .fetch = ABS,
        .execute = LDX,
        .cycles = 4,
        .length = 3
    };
    table[0xAF] = (Instruction) {
        .name = "???",
        .opcode = 0xAF,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_BCS_REL] = (Instruction) {
        .name = "BCS",
        .opcode = 0xB0,
        .fetch = REL,
        .execute = BCS,
        .cycles = 2,
        .length = 2
    };
    table[INSTRUCTION_LDA_IZY] = (Instruction) {
        .name = "LDA",
        .opcode = 0xB1,
        .fetch = IZY,
        .execute = LDA,
        .cycles = 5,
        .length = 2
    };
    table[0xB2] = (Instruction) {
        .name = "???",
        .opcode = 0xB2,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0xB3] = (Instruction) {
        .name = "???",
        .opcode = 0xB3,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_LDY_ZPX] = (Instruction) {
        .name = "LDY",
        .opcode = 0xB4,
        .fetch = ZPX,
        .execute = LDY,
        .cycles = 4,
        .length = 2
    };
    table[INSTRUCTION_LDA_ZPX] = (Instruction) {
        .name = "LDA",
        .opcode = 0xB5,
        .fetch = ZPX,
        .execute = LDA,
        .cycles = 4,
        .length = 2
    };
    table[INSTRUCTION_LDX_ZPY] = (Instruction) {
        .name = "LDX",
        .opcode = 0xB6,
        .fetch = ZPY,
        .execute = LDX,
        .cycles = 4,
        .length = 2
    };
    table[0xB7] = (Instruction) {
        .name = "???",
        .opcode = 0xB7,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_CLV_IMP] = (Instruction) {
        .name = "CLV",
        .opcode = 0xB8,
        .fetch = IMP,
        .execute = CLV,
        .cycles = 2,
        .length = 1
    };
    table[INSTRUCTION_LDA_ABY] = (Instruction) {
        .name = "LDA",
        .opcode = 0xB9,
        .fetch = ABY,
        .execute = LDA,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_TSX_IMP] = (Instruction) {
        .name = "TSX",
        .opcode = 0xBA,
        .fetch = IMP,
        .execute = TSX,
        .cycles = 2,
        .length = 1
    };
    table[0xBB] = (Instruction) {
        .name = "???",
        .opcode = 0xBB,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_LDY_ABX] = (Instruction) {
        .name = "LDY",
        .opcode = 0xBC,
        .fetch = ABX,
        .execute = LDY,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_LDA_ABX] = (Instruction) {
        .name = "LDA",
        .opcode = 0xBD,
        .fetch = ABX,
        .execute = LDA,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_LDX_ABY] = (Instruction) {
        .name = "LDX",
        .opcode = 0xBE,
        .fetch = ABY,
        .execute = LDX,
        .cycles = 4,
        .length = 3
    };
    table[0xBF] = (Instruction) {
        .name = "???",
        .opcode = 0xBF,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_CPY_IMM] = (Instruction) {
        .name = "CPY",
        .opcode = 0xC0,
        .fetch = IMM,
        .execute = CPY,
        .cycles = 2,
        .length = 2
    };
    table[INSTRUCTION_CMP_IZX] = (Instruction) {
        .name = "CMP",
        .opcode = 0xC1,
        .fetch = IZX,
        .execute = CMP,
        .cycles = 6,
        .length = 2
    };
    table[0xC2] = (Instruction) {
        .name = "???",
        .opcode = 0xC2,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0xC3] = (Instruction) {
        .name = "???",
        .opcode = 0xC3,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_CPY_ZP0] = (Instruction) {
        .name = "CPY",
        .opcode = 0xC4,
        .fetch = ZP0,
        .execute = CPY,
        .cycles = 3,
        .length = 2
    };
    table[INSTRUCTION_CMP_ZP0] = (Instruction) {
        .name = "CMP",
        .opcode = 0xC5,
        .fetch = ZP0,
        .execute = CMP,
        .cycles = 3,
        .length = 2
    };
    table[INSTRUCTION_DEC_ZP0] = (Instruction) {
        .name = "DEC",
        .opcode = 0xC6,
        .fetch = ZP0,
        .execute = DEC,
        .cycles = 5,
        .length = 2
    };
    table[0xC7] = (Instruction) {
        .name = "???",
        .opcode = 0xC7,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_INY_IMP] = (Instruction) {
        .name = "INY",
        .opcode = 0xC8,
        .fetch = IMP,
        .execute = INY,
        .cycles = 2,
        .length = 1
    };
    table[INSTRUCTION_CMP_IMM] = (Instruction) {
        .name = "CMP",
        .opcode = 0xC9,
        .fetch = IMM,
        .execute = CMP,
        .cycles = 2,
        .length = 2
    };
    table[INSTRUCTION_DEX_IMP] = (Instruction) {
        .name = "DEX",
        .opcode = 0xCA,
        .fetch = IMP,
        .execute = DEX,
        .cycles = 2,
        .length = 1
    };
    table[0xCB] = (Instruction) {
        .name = "???",
        .opcode = 0xCB,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_CPY_ABS] = (Instruction) {
        .name = "CPY",
        .opcode = 0xCC,
        .fetch = ABS,
        .execute = CPY,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_CMP_ABS] = (Instruction) {
        .name = "CMP",
        .opcode = 0xCD,
        .fetch = ABS,
        .execute = CMP,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_DEC_ABS] = (Instruction) {
        .name = "DEC",
        .opcode = 0xCE,
        .fetch = ABS,
        .execute = DEC,
        .cycles = 6,
        .length = 3
    };
    table[0xCF] = (Instruction) {
        .name = "???",
        .opcode = 0xCF,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_BNE_REL] = (Instruction) {
        .name = "BNE",
        .opcode = 0xD0,
        .fetch = REL,
        .execute = BNE,
        .cycles = 2,
        .length = 2
    };
    table[INSTRUCTION_CMP_IZY] = (Instruction) {
        .name = "CMP",
        .opcode = 0xD1,
        .fetch = IZY,
        .execute = CMP,
        .cycles = 5,
        .length = 2
    };
    table[0xD2] = (Instruction) {
        .name = "???",
        .opcode = 0xD2,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0xD3] = (Instruction) {
        .name = "???",
        .opcode = 0xD3,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_CMP_ZPX] = (Instruction) {
        .name = "CMP",
        .opcode = 0xD5,
        .fetch = ZPX,
        .execute = CMP,
        .cycles = 4,
        .length = 2
    };
    table[INSTRUCTION_DEC_ZPX] = (Instruction) {
        .name = "DEC",
        .opcode = 0xD6,
        .fetch = ZPX,
        .execute = DEC,
        .cycles = 6,
        .length = 2
    };
    table[0xD7] = (Instruction) {
        .name = "???",
        .opcode = 0xD7,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_CLD_IMP] = (Instruction) {
        .name = "CLD",
        .opcode = 0xD8,
        .fetch = IMP,
        .execute = CLD,
        .cycles = 2,
        .length = 1
    };
    table[INSTRUCTION_CMP_ABY] = (Instruction) {
        .name = "CMP",
        .opcode = 0xD9,
        .fetch = ABY,
        .execute = CMP,
        .cycles = 4,
        .length = 3
    };
    table[0xDA] = (Instruction) {
        .name = "???",
        .opcode = 0xDA,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0xDB] = (Instruction) {
        .name = "???",
        .opcode = 0xDB,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_CMP_ABX] = (Instruction) {
        .name = "CMP",
        .opcode = 0xDD,
        .fetch = ABX,
        .execute = CMP,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_DEC_ABX] = (Instruction) {
        .name = "DEC",
        .opcode = 0xDE,
        .fetch = ABX,
        .execute = DEC,
        .cycles = 7,
        .length = 3
    };
    table[0xDF] = (Instruction) {
        .name = "???",
        .opcode = 0xDF,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_CPX_IMM] = (Instruction) {
        .name = "CPX",
        .opcode = 0xE0,
        .fetch = IMM,
        .execute = CPX,
        .cycles = 2,
        .length = 2
    };
    table[INSTRUCTION_SBC_IZX] = (Instruction) {
        .name = "SBC",
        .opcode = 0xE1,
        .fetch = IZX,
        .execute = SBC,
        .cycles = 6,
        .length = 2
    };
    table[0xE2] = (Instruction) {
        .name = "???",
        .opcode = 0xE2,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0xE3] = (Instruction) {
        .name = "???",
        .opcode = 0xE3,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_CPX_ZP0] = (Instruction) {
        .name = "CPX",
        .opcode = 0xE4,
        .fetch = ZP0,
        .execute = CPX,
        .cycles = 3,
        .length = 2
    };
    table[INSTRUCTION_SBC_ZP0] = (Instruction) {
        .name = "SBC",
        .opcode = 0xE5,
        .fetch = ZP0,
        .execute = SBC,
        .cycles = 3,
        .length = 2
    };
    table[INSTRUCTION_INC_ZP0] = (Instruction) {
        .name = "INC",
        .opcode = 0xE6,
        .fetch = ZP0,
        .execute = INC,
        .cycles = 5,
        .length = 2
    };
    table[0xE7] = (Instruction) {
        .name = "???",
        .opcode = 0xE7,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_INX_IMP] = (Instruction) {
        .name = "INX",
        .opcode = 0xE8,
        .fetch = IMP,
        .execute = INX,
        .cycles = 2,
        .length = 1
    };
    table[INSTRUCTION_SBC_IMM] = (Instruction) {
        .name = "SBC",
        .opcode = 0xE9,
        .fetch = IMM,
        .execute = SBC,
        .cycles = 2,
        .length = 2
    };
    table[INSTRUCTION_NOP_IMP] = (Instruction) {
        .name = "NOP",
        .opcode = 0xEA,
        .fetch = IMP,
        .execute = NOP,
        .cycles = 2,
        .length = 1
    };
    table[0xEB] = (Instruction) {
        .name = "???",
        .opcode = 0xEB,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_CPX_ABS] = (Instruction) {
        .name = "CPX",
        .opcode = 0xEC,
        .fetch = ABS,
        .execute = CPX,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_SBC_ABS] = (Instruction) {
        .name = "SBC",
        .opcode = 0xED,
        .fetch = ABS,
        .execute = SBC,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_INC_ABS] = (Instruction) {
        .name = "INC",
        .opcode = 0xEE,
        .fetch = ABS,
        .execute = INC,
        .cycles = 6,
        .length = 3
    };
    table[0xEF] = (Instruction) {
        .name = "???",
        .opcode = 0xEF,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_BEQ_REL] = (Instruction) {
        .name = "BEQ",
        .opcode = 0xF0,
        .fetch = REL,
        .execute = BEQ,
        .cycles = 2,
        .length = 2
    };
    table[INSTRUCTION_SBC_IZY] = (Instruction) {
        .name = "SBC",
        .opcode = 0xF1,
        .fetch = IZY,
        .execute = SBC,
        .cycles = 5,
        .length = 2
    };
    table[0xF2] = (Instruction) {
        .name = "???",
        .opcode = 0xF2,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0xF3] = (Instruction) {
        .name = "???",
        .opcode = 0xF3,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_SBC_ZPX] = (Instruction) {
        .name = "SBC",
        .opcode = 0xF5,
        .fetch = ZPX,
        .execute = SBC,
        .cycles = 4,
        .length = 2
    };
    table[INSTRUCTION_INC_ZPX] = (Instruction) {
        .name = "INC",
        .opcode = 0xF6,
        .fetch = ZPX,
        .execute = INC,
        .cycles = 6,
        .length = 2
    };
    table[0xF7] = (Instruction) {
        .name = "???",
        .opcode = 0xF7,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_SED_IMP] = (Instruction) {
        .name = "SED",
        .opcode = 0xF8,
        .fetch = IMP,
        .execute = SED,
        .cycles = 2,
        .length = 1
    };
    table[INSTRUCTION_SBC_ABY] = (Instruction) {
        .name = "SBC",
        .opcode = 0xF9,
        .fetch = ABY,
        .execute = SBC,
        .cycles = 4,
        .length = 3
    };
    table[0xFA] = (Instruction) {
        .name = "???",
        .opcode = 0xFA,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0xFB] = (Instruction) {
        .name = "???",
        .opcode = 0xFB,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[INSTRUCTION_SBC_ABX] = (Instruction) {
        .name = "SBC",
        .opcode = 0xFD,
        .fetch = ABX,
        .execute = SBC,
        .cycles = 4,
        .length = 3
    };
    table[INSTRUCTION_INC_ABX] = (Instruction) {
        .name = "INC",
        .opcode = 0xFE,
        .fetch = ABX,
        .execute = INC,
        .cycles = 7,
        .length = 3
    };
    table[0xFF] = (Instruction) {
        .name = "???",
        .opcode = 0xFF,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
}

void reset_globals() {
    address = 0x0000;
    address_rel = 0x00;
    value = 0;
    return;
}

void register_init(CPU *cpu) {
    cpu->A = 0;
    cpu->X = 0;
    cpu->Y = 0;
    cpu->SP = 0xFF; //onelonecoder sets this to 0xFD, but I think it should be 0xFF?
    cpu->PC = (read_from_addr(cpu, 0xFFFD) << 8) | read_from_addr(cpu, 0xFFFC);
    cpu->STATUS = 0x00 | U; //set unused bit
    return;
}

void set_flag(CPU *cpu, STATUS_FLAGS flag, Byte value) {
    if (value) {
        cpu->STATUS |= flag;
    } else {
        cpu->STATUS &= ~flag;
    }
    return;
}


void request_additional_cycles(CPU *cpu, Byte cycles) {
    cpu->CYCLES += cycles;
    return;
}

bool crosses_page_boundary(Word old_pc, Word new_pc) {
    return (old_pc & 0xFF00) != (new_pc & 0xFF00);
}

Byte branch_pc(CPU *cpu){
    Byte cycles = 0;
    Word old_pc = cpu->PC;
    cpu->PC += address_rel;
    if(crosses_page_boundary(old_pc, cpu->PC)){
        cycles++;
    }
    return cycles;
}

/*
    gets the value at current PC without incrementing
*/
Byte peek(CPU *cpu) {
    assert(cpu != NULL);
    assert(cpu->memory != NULL);
    return cpu->memory[cpu->PC];
}

Byte read_from_addr(CPU *cpu, Word address) {
    assert(cpu != NULL);
    assert(cpu->memory != NULL);
    return cpu->memory[address];
}

void write_to_addr(CPU *cpu, Word address, Byte value) {
    assert(cpu != NULL);
    assert(cpu->memory != NULL);
    cpu->memory[address] = value;
    return;
}

/*
    Stack functions
    Only memory on the first page (0x0100 - 0x01FF) is used for the stack
*/
void push_byte(CPU *cpu, Byte byte) {
    assert(cpu != NULL);
    assert(cpu->memory != NULL);
    if(cpu->SP == 0x00){
        cpu->SP = 0xFF; //stack is full, wrap around
    }
    Word stack_addr = 0x0100 + cpu->SP;
    write_to_addr(cpu, stack_addr, byte);
    //assert byte was actually written
    cpu->SP--;
    return;
}

Byte pop_byte(CPU *cpu) {
    assert(cpu != NULL);
    assert(cpu->memory != NULL);
    cpu->SP++;
    if(cpu->SP == 0xFF){
        cpu->SP = 0x00; //stack is empty, wrap around
    }
    Word stack_addr = 0x0100 + cpu->SP;
    Byte byte = read_from_addr(cpu, stack_addr);
    return byte;
}

//CPU interface functions
void init(CPU *cpu, Byte *memory) {
    cpu->memory = memory;
    register_init(cpu);
    reset_globals();
    return;
}

void reset(CPU *cpu) {
    register_init(cpu);
    reset_globals();
    return;
}

void irq(CPU *cpu) {
    if (cpu->STATUS & I) {
        return;
    }
    push_addr(cpu, cpu->PC);
    push_byte(cpu, cpu->STATUS);
    set_flag(cpu, I, 1);
    cpu->PC = (read_from_addr(cpu, 0xFFFF) << 8) | read_from_addr(cpu, 0xFFFE);
    cpu->CYCLES += 7;
    return;
}

void nmi(CPU *cpu) {
    push_addr(cpu, cpu->PC);
    push_byte(cpu, cpu->STATUS);
    set_flag(cpu, I, 1);
    cpu->PC = (read_from_addr(cpu, 0xFFFB) << 8) | read_from_addr(cpu, 0xFFFA);
    cpu->CYCLES += 8;
    return;
}

/*
    Addressing mode functions
    These functions fetch the address and value of the operand
    and store them in the address and value variables
    If an additional cycle is needed, the function returns 1
*/


/*
    * Implied addressing mode
    * No operand needed
    * Returns 0
*/
Byte IMP (CPU *cpu) {
    assert(cpu != NULL);
    return 0;
}

/*
    * Immediate addressing mode
    * Fetches the next byte in memory and stores it in the value variable
    * Increments the program counter
*/
Byte IMM (CPU *cpu) {
    assert(cpu != NULL);
    value = read_from_addr(cpu, cpu->PC + 1);
    return 0;
}

/*
    * Zero page addressing mode
    * Fetches the next byte in memory and stores it in the zero'th page of memory
    * Increments the program counter
*/

Byte ZP0 (CPU *cpu) {
    assert(cpu != NULL);
    value = read_from_addr(cpu, cpu->PC + 1);
    zero_high_byte(address);
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
Byte ZPX (CPU *cpu) {
    assert(cpu != NULL);
    address = read_from_addr(cpu, cpu->PC + 1);
    zero_high_byte(address);
    address += cpu->X;
    zero_high_byte(address); //for page wrap around
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
Byte ZPY (CPU *cpu) {
    assert(cpu != NULL);
    address = read_from_addr(cpu, cpu->PC + 1);
    zero_high_byte(address);
    address += cpu->Y;
    zero_high_byte(address);
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Relative addressing mode
    * Fetches the next byte in memory and stores it in the address_rel variable
    * Increments the program counter
*/
Byte REL (CPU *cpu) {
    assert(cpu != NULL);
    address_rel = read_from_addr(cpu, cpu->PC + 1);
    return 0;
}

/*
    * Absolute addressing mode
    * Fetches the next two bytes in memory and stores it in the address variable
    * Increments the program counter by two
    * Fetches the value at the address and stores it in the value variable
*/
Byte ABS (CPU *cpu) {
    assert(cpu != NULL);
    Byte low_byte = read_from_addr(cpu, cpu->PC + 1);
    Byte high_byte = read_from_addr(cpu, cpu->PC + 2);
    address = (high_byte << 8) | low_byte;
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

Byte ABX (CPU *cpu) {
    assert(cpu != NULL);
    Byte low_byte = read_from_addr(cpu, cpu->PC + 1);
    Byte high_byte = read_from_addr(cpu, cpu->PC + 2);
    address += cpu->X;
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Absolute Y addressing mode
    * Fetches the next two bytes in memory and stores it in the address variable
    * Increments the program counter
    * Adds the Y register to the address
    * Stores the result in the address variable
    * Fetches the value at the address and stores it in the value variable
*/

Byte ABY (CPU *cpu) {
    assert(cpu != NULL);
    Byte low_byte = read_from_addr(cpu, cpu->PC + 1);
    Byte high_byte = read_from_addr(cpu, cpu->PC + 2);
    address += cpu->Y;
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Indirect addressing mode
    * Fetches the next two bytes in memory and stores it in the address variable
    * Increments the program counter
    * Fetches the value at the address and stores it in the value variable
    * Fetches the next two bytes in memory and stores it in the address variable
    * Increments the program counter
    * Fetches the value at the address and stores it in the value variable
*/

Byte IND (CPU *cpu) {
    assert(cpu != NULL);
    Word ptr = read_from_addr(cpu, cpu->PC + 1);
    ptr |= ((read_from_addr(cpu, cpu->PC + 2)) << 8);
    //simulate page boundary hardware bug
    if ((ptr & 0x00FF) == 0x00FF) {
        address = (read_from_addr(cpu, ptr & 0xFF00) << 8) | read_from_addr(cpu, ptr);
    } else {
        address = (read_from_addr(cpu, ptr + 1) << 8) | read_from_addr(cpu, ptr);
    }
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Indirect X addressing mode
    * Fetches the next byte in memory and stores it in the address variable
    * Increments the program counter
    * Adds the X register to the address
    * Stores the result in the address variable
    * Fetches the value at the address and stores it in the value variable
*/

Byte IZX (CPU *cpu) {
    assert(cpu != NULL);
    Word ptr = read_from_addr(cpu, cpu->PC + 1);
    zero_high_byte(ptr);
    address = (read_from_addr(cpu, ptr + 1) << 8) | read_from_addr(cpu, ptr);
    address += cpu->X;
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Indirect Y addressing mode
    * Fetches the next byte in memory and stores it in the address variable
    * Increments the program counter
    * Fetches the value at the address and stores it in the value variable
    * Adds the Y register to the address
    * Stores the result in the address variable
*/
//todo not so sure about this one
Byte IZY (CPU *cpu) {
    assert(cpu != NULL);
    Word ptr = read_from_addr(cpu, cpu->PC + 1);
    zero_high_byte(ptr);
    address = (read_from_addr(cpu, ptr + 1) << 8) | read_from_addr(cpu, ptr);
    address += cpu->Y;
    value = read_from_addr(cpu, address);
    return 0;
}
//6502 instructions LITTLE ENDIAN
/*
    BRK - Force Interrupt
    Pushes the program counter onto the stack
    Pushes the status register onto the stack
    Sets the interrupt flag
    Sets the program counter to the interrupt vector  
*/

Byte BRK(CPU *cpu) {
    assert(cpu != NULL);
    set_flag(cpu, I, 1);
    cpu->PC++;
    //pushes the next instruction address onto the stack
    push_addr(cpu, cpu->PC);
    set_flag(cpu, B, 1); //sets the break flag because this is a software interrupt
    push_byte(cpu, cpu->STATUS);
    set_flag(cpu, B, 0); //clears the break flag
    cpu->PC = (read_from_addr(cpu, 0xFFFF) << 8) | read_from_addr(cpu, 0xFFFE);
    return 0;
}

Byte ORA(CPU *cpu) {
    cpu->A |= value;
    set_zn(cpu, cpu->A);
    return 0;
}

Byte ASL(CPU *cpu) {
    set_flag(cpu, C, value & 0x80);
    value <<= 1;
    set_zn(cpu, value);
    return 0;
}
/*
    Push Processor Status on Stack
    Pushes the status register onto the stack
    Decrements the stack pointer?
*/
Byte PHP(CPU *cpu) {
    push_byte(cpu, cpu->STATUS);
    return 0;
}

//branch helper function that takes a flag as a parameter
//if flag value is 1, we check if the flag is set
//otherwise we check if the flag is not set
//returns 1 if page boundary is crossed and branch taken, 0 otherwise
Byte branch_on_flag(CPU *cpu, STATUS_FLAGS flag, Byte flag_value) {
    assert(cpu != NULL);
    //checks if the flag is set to the flag value
    if ((cpu->STATUS & flag) == (flag_value & flag)) {
        Word old_PC = cpu->PC;
        cpu->PC += address_rel;
        //checks if the page boundary is crossed
        if(crosses_page_boundary(old_PC, cpu->PC)) {
            return 1;
        }
    }
    return 0;
}

/*
    Branch on Result Plus (N = 0)
    If the negative flag is not set, add the relative address to the program counter
    If the program counter crosses a page boundary, return 1 to indicate an extra cycle is required
*/
Byte BPL(CPU *cpu) {
    assert(cpu != NULL);
    Byte cycles = branch_on_flag(cpu, N, 0);
    return cycles;
}

/*
 * Clear Carry Flag
 * Clears the carry flag
*/
Byte CLC(CPU *cpu) {
    assert(cpu != NULL);
    set_flag(cpu, C, 0);
    return 0;
}

/*
    Jump to Subroutine
    Pushes the program counter onto the stack
    Sets the program counter to the address
*/
Byte JSR(CPU *cpu) {
    assert(cpu != NULL);
    Word val = cpu->PC + 1;
    push_addr(cpu, val); //todo check if this is correct
    cpu->PC = address;
    return 0;
}

/*
    AND Memory with Accumulator
    ANDs the value with the accumulator
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
Byte AND(CPU *cpu) {
    assert(cpu != NULL);
    cpu->A &= value;
    set_zn(cpu, cpu->A);
    return 0;
}

/*
    BIT Test Bits in Memory with Accumulator
    ANDs the value with the accumulator
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
    Sets the overflow flag to the sixth bit of the value
*/
Byte BIT(CPU *cpu) {
    assert(cpu != NULL);
    Byte result = cpu->A & value;
    //this flag is set based on the result of the AND
    set_flag(cpu, Z, result == 0x00);
    //these 2 flags are set to the 6th and 7th bits of the value
    set_flag(cpu, N, value & 0x80);
    set_flag(cpu, V, value & 0x40);
    return 0;
}

/*
    Rotate Left (Memory)
    Gets value from address and rotates it left by 1 bit
    Sets the carry flag to the 7th bit of the value
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
    Stores the result in memory
*/
Byte ROL(CPU *cpu) {
    //assume address and value are set
    assert(cpu != NULL);
    set_flag(cpu, C, value & 0x80);
    value <<= 1;
    set_zn(cpu, value);
    write_to_addr(cpu, address, value);
    return 0;
}

/*
    Rotate Left (Accumulator)
    Rotates the accumulator left by 1 bit
    Sets the carry flag to the 7th bit of the value
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
Byte ROL_ACC(CPU *cpu) {
    assert(cpu != NULL);
    set_flag(cpu, C, cpu->A & 0x80);
    cpu->A <<= 1;
    set_zn(cpu, cpu->A);
    return 0;
}

/*
    Pull Processor Status from Stack
    The status register will be pulled with the break
    flag and bit 5 ignored.
*/
Byte PLP(CPU *cpu) {
    assert(cpu != NULL);
    cpu->STATUS = pop_byte(cpu);
    set_flag(cpu, U, 1);
    set_flag(cpu, B, 0);
    return 0;
}

/*
    Branch on Result Minus (N = 1)
    If the negative flag is set, add the relative address to the program counter
    If the program counter crosses a page boundary, return 1 to indicate an extra cycle is required
*/
Byte BMI(CPU *cpu) {
    assert(cpu != NULL);
    Byte cycles = branch_on_flag(cpu, N, 1);
    return cycles;
}

/*
    SEC Set Carry Flag
    Sets the carry flag to 1
*/
Byte SEC(CPU *cpu) {
    assert(cpu != NULL);
    set_flag(cpu, C, 1);
    return 0;
}

/*
    Return from Interrupt
    Pulls the program counter from the stack
    Pulls the status register from the stack
    Bit 5 and the break flag are ignoreds
*/
Byte RTI(CPU *cpu) {
    assert(cpu != NULL);
    cpu->STATUS = pop_byte(cpu);
    cpu->PC = pop_byte(cpu);
    cpu->PC |= (pop_byte(cpu) << 8);
    set_flag(cpu, U, 1); //this is always logical 1
    set_flag(cpu, B, 0); //B flag is only 1 when BRK is executed
    return 0;
}

/*
    EOR Exclusive-OR Memory with Accumulator
    XORs the value with the accumulator
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/

Byte EOR(CPU *cpu) {
    assert(cpu != NULL);
    cpu->A ^= value;
    set_zn(cpu, cpu->A);
    return 0;
}

/*
    LSR Logical Shift Right (Memory)
    Shifts the value right by 1 bit
    Sets the carry flag to the 0th bit of the value
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
    Stores the result in memory
*/
Byte LSR(CPU *cpu) {
    assert(cpu != NULL);
    set_flag(cpu, C, value & 0x01);
    value >>= 1;
    set_flag(cpu, Z, value == 0x00);
    set_flag(cpu, N, value & 0x80);
    write_to_addr(cpu, address, value);
    return 0;
}

/*
    LSR Logical Shift Right (Accumulator)
    Shifts the accumulator right by 1 bit
    Sets the carry flag to the 0th bit of the value
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
Byte LSR_ACC(CPU *cpu) {
    assert(cpu != NULL);
    set_flag(cpu, C, cpu->A & 0x01);
    cpu->A >>= 1;
    set_flag(cpu, Z, cpu->A == 0x00);
    set_flag(cpu, N, cpu->A & 0x80);
    return 0;
}

/*
    PHA Push Accumulator on Stack
    Pushes the accumulator onto the stack
*/
Byte PHA(CPU *cpu) {
    assert(cpu != NULL);
    push_byte(cpu, cpu->A);
    return 0;
}

/*
    Jump to New Location
    Sets the program counter to the address
    (PC+1) -> PCL
    (PC+2) -> PCH
*/
Byte JMP(CPU *cpu) {
    assert(cpu != NULL);
    Byte low = read_from_addr(cpu, cpu->PC + 1);
    Byte high = read_from_addr(cpu, cpu->PC + 2);
    cpu->PC = (high << 8) | low;
    return 0;
}

/*
    BVC Branch on Overflow Clear
    If the overflow flag is not set, add the relative address to the program counter
    If the program counter crosses a page boundary, return 1 to indicate an extra cycle is required
*/
Byte BVC(CPU *cpu) {
    assert(cpu != NULL);
    Byte cycles = branch_on_flag(cpu, V, 0);
    return cycles;
}

/*
    CLI Clear Interrupt Disable Bit
    Sets the interrupt disable flag to 0
*/
Byte CLI(CPU *cpu) {
    assert(cpu != NULL);
    set_flag(cpu, I, 0);
    return 0;
}

/*
    RTS Return from Subroutine
    Pulls the program counter from the stack
    Increments the program counter
*/
Byte RTS(CPU *cpu) {
    assert(cpu != NULL);
    cpu->PC = pop_byte(cpu);
    cpu->PC |= (pop_byte(cpu) << 8);
    cpu->PC++; //increment so we don't return to the same instruction
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

Byte ADC(CPU *cpu) {
    assert(cpu != NULL);
    Word result = cpu->A + value + cpu->STATUS & C;
    set_flag(cpu, C, result > 0xFF);
    set_flag(cpu, Z, (result & 0x00FF) == 0x0000);
    set_flag(cpu, N, result & 0x0080);
    //overflow flag is set if the sign of the result is different from the sign of the accumulator
    Byte accumulator_msb = cpu->A & 0x80;
    Byte value_msb = value & 0x80;
    Byte result_msb = result & 0x80;
    Byte overflow = ((accumulator_msb ^ value_msb) == 0) && ((accumulator_msb ^ result_msb) != 0);
    set_flag(cpu, V, overflow);
    cpu->A = (Byte)(result & 0x00FF);
    return 0;
}

/*
    ROR Rotate Right (Memory)
    Gets value from address and rotates it right by 1 bit
    Sets the carry flag to the 0th bit of the value
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
    Stores the result in memory
*/
Byte ROR(CPU *cpu) {
    assert(cpu != NULL);
    Byte carry = cpu->STATUS & C;
    //set the carry flag to the 0th bit of the value
    set_flag(cpu, C, value & 0x01);
    //shift the value right by 1 bit
    value >>= 1;
    //set the 7th bit to the carry flag
    value |= (carry << 7);
    set_zn(cpu, value);
    write_to_addr(cpu, address, value);
    return 0;
}

/*
    PLA Pull Accumulator from Stack
    Pulls the accumulator from the stack
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
Byte PLA(CPU *cpu) {
    assert(cpu != NULL);
    cpu->A = pop_byte(cpu);
    set_zn(cpu, cpu->A);
    return 0;
}

/*
    ROR Rotate Right (Accumulator)
    Rotates the accumulator right by 1 bit
    Sets the carry flag to the 0th bit of the value
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
Byte ROR_ACC(CPU *cpu) {
    assert(cpu != NULL);
    Byte carry = cpu->STATUS & C;
    set_flag(cpu, C, cpu->A & 0x01);
    cpu->A >>= 1;
    cpu->A |= (carry << 7);
    set_zn(cpu, cpu->A);
    return 0;
}

/*
    Branch on Overflow Set
    If the overflow flag is set, add the relative address to the program counter
    If the program counter crosses a page boundary, return 1 to indicate an extra cycle is required
*/
Byte BVS(CPU *cpu) {
    assert(cpu != NULL);
    Byte cycles = branch_on_flag(cpu, V, 1);
    return cycles;
}

/*
    SEI Set Interrupt Disable Status
    Sets the interrupt disable flag to 1
*/
Byte SEI(CPU *cpu) {
    assert(cpu != NULL);
    set_flag(cpu, I, 1);
    return 0;
}

/*
    STA Store Accumulator in Memory
    Stores the accumulator in memory
*/
Byte STA(CPU *cpu) {
    assert(cpu != NULL);
    write_to_addr(cpu, address, cpu->A);
    return 0;
}

/*
    STY Store Index Y in Memory
    Stores the Y register in memory
*/
Byte STY(CPU *cpu) {
    assert(cpu != NULL);
    write_to_addr(cpu, address, cpu->Y);
    return 0;
}

/*
    STX Store Index X in Memory
    Stores the X register in memory
*/
Byte STX(CPU *cpu) {
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
Byte DEY(CPU *cpu) {
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
Byte TXA(CPU *cpu) {
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

Byte BCC(CPU *cpu) {
    assert(cpu != NULL);
    Byte cycles = branch_on_flag(cpu, C, 0);
    return cycles;
}

/*
    TYA Transfer Index Y to Accumulator
    Transfers the Y register to the accumulator
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
Byte TYA(CPU *cpu) {
    assert(cpu != NULL);
    cpu->A = cpu->Y;
    set_zn(cpu, cpu->A);
    return 0;
}

/*
    TXS Transfer Index X to Stack Register
    Transfers the X register to the stack pointer
*/
Byte TXS(CPU *cpu) {
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
Byte LDY(CPU *cpu) {
    assert(cpu != NULL);
    cpu->Y = value;
    set_zn(cpu, cpu->Y);
    return 0;
}

/*
    LDA Load Accumulator
    Loads the accumulator with the value
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
Byte LDA(CPU *cpu) {
    assert(cpu != NULL);
    cpu->A = value;
    set_zn(cpu, cpu->A);
    return 0;
}

/*
    LDX Load Index X
    Loads the X register with the value
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
Byte LDX(CPU *cpu) {
    assert(cpu != NULL);
    cpu->X = value;
    set_zn(cpu, cpu->X);
    return 0;
}

/*
    TAY Transfer Accumulator to Index Y
    Transfers the accumulator to the Y register
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
Byte TAY(CPU *cpu) {
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
Byte TAX(CPU *cpu) {
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
Byte BCS(CPU *cpu) {
    assert(cpu != NULL);
    Byte cycles = branch_on_flag(cpu, C, 1);
    return cycles;
}

/*
    CLV Clear Overflow Flag
    Sets the overflow flag to 0
*/
Byte CLV(CPU *cpu) {
    assert(cpu != NULL);
    set_flag(cpu, V, 0);
    return 0;
}

/*
    TSX Transfer Stack Pointer to Index X
    Transfers the stack pointer to the X register
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
Byte TSX(CPU *cpu) {
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
Byte CPY(CPU *cpu) {
    assert(cpu != NULL);
    Byte result = cpu->Y - value;
    set_flag(cpu, C, cpu->Y >= value);
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
Byte CMP(CPU *cpu) {
    assert(cpu != NULL);
    Byte result = cpu->A - value;
    set_flag(cpu, C, cpu->A >= value);
    set_zn(cpu, result);
    return 0;
}

/*
    DEC Decrement Memory by One
    Decrements the value by 1
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
    Stores the result in memory
*/
Byte DEC(CPU *cpu) {
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
Byte DEX(CPU *cpu) {
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
Byte BNE(CPU *cpu) {
    assert(cpu != NULL);
    Byte cycles = branch_on_flag(cpu, Z, 0);
    return cycles;
}

/*
    CLD Clear Decimal Mode
    Sets the decimal flag to 0
*/
Byte CLD(CPU *cpu) {
    assert(cpu != NULL);
    set_flag(cpu, D, 0);
    return 0;
}

/*
    CPX Compare value and Index X
    Compares the value with the X register
    Sets the carry flag if the X register is greater than or equal to the value
    Sets the zero flag if the X register is equal to the value
    Sets the negative flag if the X register is less than the value
*/
Byte CPX(CPU *cpu) {
    assert(cpu != NULL);
    Byte result = cpu->X - value;
    set_flag(cpu, C, cpu->X >= value);
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
Byte SBC(CPU *cpu) {
    assert(cpu != NULL);
    Word result = cpu->A - value - (1 - cpu->STATUS & C);
    set_flag(cpu, C, result < 0x100);
    set_flag(cpu, Z, (result & 0x00FF) == 0x0000);
    set_flag(cpu, N, result & 0x0080);
    //overflow flag is set if the sign of the result is different from the sign of the accumulator
    Byte accumulator_msb = cpu->A & 0x80;
    Byte value_msb = value & 0x80;
    Byte result_msb = result & 0x80;
    Byte overflow = ((accumulator_msb ^ value_msb) != 0) && ((accumulator_msb ^ result_msb) != 0);
    set_flag(cpu, V, overflow);
    cpu->A = (Byte)(result & 0x00FF);
    return 0;
}

/*
    INC Increment Memory by One
    Increments the value by 1
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
    Stores the result in memory
*/
Byte INC(CPU *cpu) {
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
Byte INX(CPU *cpu) {
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
Byte INY(CPU *cpu) {
    assert(cpu != NULL);
    cpu->Y++;
    set_flag(cpu, Z, cpu->Y == 0x00);
    set_flag(cpu, N, cpu->Y & 0x80);
    return 0;
}

/*
    BEQ Branch on Result Zero
    If the zero flag is set, add the relative address to the program counter
    If the program counter crosses a page boundary, return 1 to indicate an extra cycle is required
*/
Byte BEQ(CPU *cpu) {
    assert(cpu != NULL);
    Byte cycles = branch_on_flag(cpu, Z, 1);
    return cycles;
}

/*
    SED Set Decimal Flag
    Sets the decimal flag to 1
*/
Byte SED(CPU *cpu) {
    assert(cpu != NULL);
    set_flag(cpu, D, 1);
    return 0;
}

/*
    NOP No Operation
    Does nothing
*/
Byte NOP(CPU *cpu) {
    (void)cpu;
    return 0;
}
