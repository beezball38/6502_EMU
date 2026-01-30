// 6502 CPU definitions - X-Macros, opcodes, and generated enums
#ifndef CPU_DEFS_H
#define CPU_DEFS_H

#include <stdint.h>

// =============================================================================
// Type definitions
// =============================================================================

typedef uint8_t byte_t;
typedef uint16_t word_t;   // used for addresses
typedef int8_t offset_t;   // used for signed branch offsets
typedef struct cpu_s cpu_s; // Forward declaration
typedef byte_t (*instruction_func_t)(cpu_s *cpu);

// =============================================================================
// X-Macro: OPCODE_MNEMONICS
// Lists all 6502 instruction mnemonics (operation names).
// =============================================================================

#define OPCODE_MNEMONICS \
X(BRK)                   \
X(ORA)                   \
X(ASL)                   \
X(PHP)                   \
X(BPL)                   \
X(CLC)                   \
X(JSR)                   \
X(AND)                   \
X(BIT)                   \
X(ROL)                   \
X(PLP)                   \
X(BMI)                   \
X(SEC)                   \
X(RTI)                   \
X(EOR)                   \
X(LSR)                   \
X(PHA)                   \
X(JMP)                   \
X(BVC)                   \
X(CLI)                   \
X(RTS)                   \
X(ADC)                   \
X(ROR)                   \
X(PLA)                   \
X(BVS)                   \
X(SEI)                   \
X(STA)                   \
X(STY)                   \
X(STX)                   \
X(DEY)                   \
X(TXA)                   \
X(BCC)                   \
X(TYA)                   \
X(TXS)                   \
X(LDY)                   \
X(LDA)                   \
X(LDX)                   \
X(TAY)                   \
X(TAX)                   \
X(BCS)                   \
X(CLV)                   \
X(TSX)                   \
X(CPY)                   \
X(CMP)                   \
X(DEC)                   \
X(INY)                   \
X(DEX)                   \
X(BNE)                   \
X(CLD)                   \
X(CPX)                   \
X(SBC)                   \
X(INC)                   \
X(INX)                   \
X(BEQ)                   \
X(SED)                   \
X(NOP)

// =============================================================================
// X-Macro: ADDRESSING_MODE_LIST
// Lists all 6502 addressing modes.
// =============================================================================

#define ADDRESSING_MODE_LIST  \
X(IMP)                  \
X(ACC)                  \
X(IMM)                  \
X(ZP0)                  \
X(ZPX)                  \
X(ZPY)                  \
X(REL)                  \
X(ABS)                  \
X(ABX)                  \
X(ABY)                  \
X(IND)                  \
X(IZX)                  \
X(IZY)

// =============================================================================
// X-Macro: INSTRUCTION_OPCODE_TABLE
// Maps each valid 6502 instruction to its mnemonic, addressing mode, and opcode.
// =============================================================================

#define INSTRUCTION_OPCODE_TABLE    \
X(BRK, IMP, 0x00)       \
X(ORA, IZX, 0x01)       \
X(ORA, ZP0, 0x05)       \
X(ASL, ZP0, 0x06)       \
X(PHP, IMP, 0x08)       \
X(ORA, IMM, 0x09)       \
X(ASL, ACC, 0x0A)       \
X(ASL, ABS, 0x0E)       \
X(ORA, ABS, 0x0D)       \
X(BPL, REL, 0x10)       \
X(ORA, IZY, 0x11)       \
X(ORA, ZPX, 0x15)       \
X(ASL, ZPX, 0x16)       \
X(CLC, IMP, 0x18)       \
X(ORA, ABY, 0x19)       \
X(ORA, ABX, 0x1D)       \
X(ASL, ABX, 0x1E)       \
X(JSR, ABS, 0x20)       \
X(AND, IZX, 0x21)       \
X(BIT, ZP0, 0x24)       \
X(AND, ZP0, 0x25)       \
X(ROL, ZP0, 0x26)       \
X(PLP, IMP, 0x28)       \
X(AND, IMM, 0x29)       \
X(ROL, ACC, 0x2A)       \
X(BIT, ABS, 0x2C)       \
X(AND, ABS, 0x2D)       \
X(ROL, ABS, 0x2E)       \
X(BMI, REL, 0x30)       \
X(AND, IZY, 0x31)       \
X(AND, ZPX, 0x35)       \
X(ROL, ZPX, 0x36)       \
X(SEC, IMP, 0x38)       \
X(AND, ABY, 0x39)       \
X(AND, ABX, 0x3D)       \
X(ROL, ABX, 0x3E)       \
X(RTI, IMP, 0x40)       \
X(EOR, IZX, 0x41)       \
X(EOR, ZP0, 0x45)       \
X(LSR, ZP0, 0x46)       \
X(PHA, IMP, 0x48)       \
X(EOR, IMM, 0x49)       \
X(LSR, ACC, 0x4A)       \
X(JMP, ABS, 0x4C)       \
X(EOR, ABS, 0x4D)       \
X(LSR, ABS, 0x4E)       \
X(BVC, REL, 0x50)       \
X(EOR, IZY, 0x51)       \
X(EOR, ZPX, 0x55)       \
X(LSR, ZPX, 0x56)       \
X(CLI, IMP, 0x58)       \
X(EOR, ABY, 0x59)       \
X(EOR, ABX, 0x5D)       \
X(LSR, ABX, 0x5E)       \
X(RTS, IMP, 0x60)       \
X(ADC, IZX, 0x61)       \
X(ADC, ZP0, 0x65)       \
X(ROR, ZP0, 0x66)       \
X(PLA, IMP, 0x68)       \
X(ADC, IMM, 0x69)       \
X(ROR, ACC, 0x6A)       \
X(JMP, IND, 0x6C)       \
X(ADC, ABS, 0x6D)       \
X(ROR, ABS, 0x6E)       \
X(BVS, REL, 0x70)       \
X(ADC, IZY, 0x71)       \
X(ADC, ZPX, 0x75)       \
X(ROR, ZPX, 0x76)       \
X(SEI, IMP, 0x78)       \
X(ADC, ABY, 0x79)       \
X(ADC, ABX, 0x7D)       \
X(ROR, ABX, 0x7E)       \
X(STA, IZX, 0x81)       \
X(STY, ZP0, 0x84)       \
X(STA, ZP0, 0x85)       \
X(STX, ZP0, 0x86)       \
X(DEY, IMP, 0x88)       \
X(TXA, IMP, 0x8A)       \
X(STY, ABS, 0x8C)       \
X(STA, ABS, 0x8D)       \
X(STX, ABS, 0x8E)       \
X(BCC, REL, 0x90)       \
X(STA, IZY, 0x91)       \
X(STY, ZPX, 0x94)       \
X(STA, ZPX, 0x95)       \
X(STX, ZPY, 0x96)       \
X(TYA, IMP, 0x98)       \
X(STA, ABY, 0x99)       \
X(TXS, IMP, 0x9A)       \
X(STA, ABX, 0x9D)       \
X(LDY, IMM, 0xA0)       \
X(LDA, IZX, 0xA1)       \
X(LDX, IMM, 0xA2)       \
X(LDY, ZP0, 0xA4)       \
X(LDA, ZP0, 0xA5)       \
X(LDX, ZP0, 0xA6)       \
X(TAY, IMP, 0xA8)       \
X(LDA, IMM, 0xA9)       \
X(TAX, IMP, 0xAA)       \
X(LDY, ABS, 0xAC)       \
X(LDA, ABS, 0xAD)       \
X(LDX, ABS, 0xAE)       \
X(BCS, REL, 0xB0)       \
X(LDA, IZY, 0xB1)       \
X(LDY, ZPX, 0xB4)       \
X(LDA, ZPX, 0xB5)       \
X(LDX, ZPY, 0xB6)       \
X(CLV, IMP, 0xB8)       \
X(LDA, ABY, 0xB9)       \
X(TSX, IMP, 0xBA)       \
X(LDY, ABX, 0xBC)       \
X(LDA, ABX, 0xBD)       \
X(LDX, ABY, 0xBE)       \
X(CPY, IMM, 0xC0)       \
X(CMP, IZX, 0xC1)       \
X(CPY, ZP0, 0xC4)       \
X(CMP, ZP0, 0xC5)       \
X(DEC, ZP0, 0xC6)       \
X(INY, IMP, 0xC8)       \
X(CMP, IMM, 0xC9)       \
X(DEX, IMP, 0xCA)       \
X(CPY, ABS, 0xCC)       \
X(CMP, ABS, 0xCD)       \
X(DEC, ABS, 0xCE)       \
X(BNE, REL, 0xD0)       \
X(CMP, IZY, 0xD1)       \
X(CMP, ZPX, 0xD5)       \
X(DEC, ZPX, 0xD6)       \
X(CLD, IMP, 0xD8)       \
X(CMP, ABY, 0xD9)       \
X(CMP, ABX, 0xDD)       \
X(DEC, ABX, 0xDE)       \
X(CPX, IMM, 0xE0)       \
X(SBC, IZX, 0xE1)       \
X(CPX, ZP0, 0xE4)       \
X(SBC, ZP0, 0xE5)       \
X(INC, ZP0, 0xE6)       \
X(INX, IMP, 0xE8)       \
X(SBC, IMM, 0xE9)       \
X(NOP, IMP, 0xEA)       \
X(CPX, ABS, 0xEC)       \
X(SBC, ABS, 0xED)       \
X(INC, ABS, 0xEE)       \
X(BEQ, REL, 0xF0)       \
X(SBC, IZY, 0xF1)       \
X(SBC, ZPX, 0xF5)       \
X(INC, ZPX, 0xF6)       \
X(SED, IMP, 0xF8)       \
X(SBC, ABY, 0xF9)       \
X(SBC, ABX, 0xFD)       \
X(INC, ABX, 0xFE)

// =============================================================================
// X-Macro: UNDEFINED_OPCODES
// Lists all undefined/illegal 6502 opcodes.
// =============================================================================

#define UNDEFINED_OPCODES \
X(0x02) X(0x03) X(0x04) X(0x07) X(0x0B) X(0x0C) X(0x0F) \
X(0x12) X(0x13) X(0x14) X(0x17) X(0x1A) X(0x1B) X(0x1C) X(0x1F) \
X(0x22) X(0x23) X(0x27) X(0x2B) X(0x2F) \
X(0x32) X(0x33) X(0x34) X(0x37) X(0x3A) X(0x3B) X(0x3C) X(0x3F) \
X(0x42) X(0x43) X(0x44) X(0x47) X(0x4B) X(0x4F) \
X(0x52) X(0x53) X(0x54) X(0x57) X(0x5A) X(0x5B) X(0x5C) X(0x5F) \
X(0x62) X(0x63) X(0x64) X(0x67) X(0x6B) X(0x6F) \
X(0x72) X(0x73) X(0x74) X(0x77) X(0x7A) X(0x7B) X(0x7C) X(0x7F) \
X(0x80) X(0x82) X(0x83) X(0x87) X(0x89) X(0x8B) X(0x8F) \
X(0x92) X(0x93) X(0x97) X(0x9B) X(0x9C) X(0x9E) X(0x9F) \
X(0xA3) X(0xA7) X(0xAB) X(0xAF) \
X(0xB2) X(0xB3) X(0xB7) X(0xBB) X(0xBF) \
X(0xC2) X(0xC3) X(0xC7) X(0xCB) X(0xCF) \
X(0xD2) X(0xD3) X(0xD4) X(0xD7) X(0xDA) X(0xDB) X(0xDC) X(0xDF) \
X(0xE2) X(0xE3) X(0xE7) X(0xEB) X(0xEF) \
X(0xF2) X(0xF3) X(0xF4) X(0xF7) X(0xFA) X(0xFB) X(0xFC) X(0xFF)

// =============================================================================
// Generated enums
// =============================================================================

// Addressing mode enum
typedef enum
{
    #define X(mode) ADDR_MODE_##mode,
    ADDRESSING_MODE_LIST
    #undef X
} cpu_addr_mode_e;

// Instruction opcode enum (each value equals its opcode for direct table indexing)
typedef enum
{
    #define X(name, mode, opcode) INSTRUCTION_##name##_##mode = opcode,
    INSTRUCTION_OPCODE_TABLE
    #undef X
} cpu_ins_e;

// Undefined opcode enum
typedef enum
{
    #define X(opcode) OPCODE_UNDEFINED_##opcode = opcode,
    UNDEFINED_OPCODES
    #undef X
} cpu_undefined_opcode_e;

// =============================================================================
// Generated function declarations
// =============================================================================

// Addressing mode handlers
#define X(name) byte_t name(cpu_s *cpu);
ADDRESSING_MODE_LIST
#undef X

// Instruction handlers
#define X(name) byte_t name(cpu_s *cpu);
OPCODE_MNEMONICS
#undef X

// =============================================================================
// Status register flags
// =============================================================================

typedef enum
{
    STATUS_FLAG_C = (1 << 0),  // Carry
    STATUS_FLAG_Z = (1 << 1),  // Zero
    STATUS_FLAG_I = (1 << 2),  // Interrupt disable
    STATUS_FLAG_D = (1 << 3),  // Decimal mode
    STATUS_FLAG_B = (1 << 4),  // Break command
    STATUS_FLAG_U = (1 << 5),  // Unused (always 1)
    STATUS_FLAG_V = (1 << 6),  // Overflow
    STATUS_FLAG_N = (1 << 7),  // Negative
} cpu_status_flag_e;

// CPU register identifiers
typedef enum
{
    REG_A,
    REG_X,
    REG_Y,
    REG_SP,
    REG_PC,
    REG_STATUS
} cpu_register_e;

#endif
