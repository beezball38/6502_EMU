// CPU struct for 6502 processor
#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define UNIQUE_OPCODES \
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
    X(ROL_ACC)               \
    X(LSR_ACC)               \
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
    X(ROR_ACC)               \
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

#define LIST_OF_ADDR_MODES  \
    X(IMP)                  \
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

#define ALL_INSTRUCTIONS    \
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
    X(BRK, ZP0, 0x24)       \
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

typedef uint8_t byte_t;
typedef uint16_t word_t;
typedef struct CPU CPU; // Forward declaration

typedef enum 
{
    #define X(mode) ADDR_MODE_##mode,
    LIST_OF_ADDR_MODES
    #undef X
} addr_mode_t;

#define X(name) byte_t name(CPU *cpu);
LIST_OF_ADDR_MODES
#undef X

typedef enum
{
    #define X(name, mode, opcode) INSTRUCTION_##name##_##mode = opcode,
    ALL_INSTRUCTIONS
    #undef X
} instruction_info_t;


#define X(name) byte_t name(CPU *cpu);
UNIQUE_OPCODES
#undef X

typedef byte_t (*Ins_Func)(CPU *cpu);
/*
 * Status register flags
 * C: Carry
 * Z: Zero
 * I: Interrupt disable
 * D: Decimal mode
 * B: Break
 * U: Unused
 * V: Overflow
 * N: Negative
 */
typedef enum
{
    C = (1 << 0),
    Z = (1 << 1),
    I = (1 << 2),
    D = (1 << 3),
    B = (1 << 4),
    U = (1 << 5),
    V = (1 << 6),
    N = (1 << 7),
} status_flag_t;


/*
 * Instruction struct
 * name: name of instruction
 * opcode: opcode of instruction
 * length: length of instruction in bytes
 * cycles: number of cycles instruction takes
 * execute: function pointer to execute instruction
 * fetch: function pointer to function to fetch operand
 */
typedef struct Instruction
{
    char *name;
    byte_t opcode;
    byte_t length;
    byte_t cycles;
    Ins_Func fetch;
    Ins_Func execute;
} Instruction;


/*
    * CPU struct
    * A: accumulator
    * X: index register X
    * Y: index register Y
    * SP: stack pointer
    * PC: program counter
    * STATUS: status register
    * additional_cycles: additional cycles to add to instruction
    * pc_changed: flag to indicate if PC changed
    * table: instruction table
    * memory: memory
*/
struct CPU
{
    byte_t A;
    byte_t X;
    byte_t Y;
    byte_t SP;
    word_t PC;
    byte_t STATUS;

    size_t cycles;
    Instruction *current_instruction;
    bool pc_changed;
    bool does_need_additional_cycle;
    byte_t *memory;
    Instruction table[256];
};

void init_instruction_table(CPU *cpu);
void cpu_init(CPU *cpu, byte_t *memory);
/*
    6502 get flag
    Reads flag from status register
*/
bool get_flag(CPU *cpu, status_flag_t flag);
/*
    6502 set flag
    Sets flag in status register
*/
void set_flag(CPU *cpu, status_flag_t flag, bool value);

/*
    6502 peek
    Peeks at the next byte in memory
    Does not increment PC
*/
byte_t peek(CPU *cpu);
/*
    6502 read from address
    Reads a byte from memory at address
*/

byte_t read_from_addr(CPU *cpu, word_t address);
/*
    6502 write to address
    Writes a byte to memory at address
*/

void write_to_addr(CPU *cpu, word_t address, byte_t value);
/*
    6502 push to stack
    Pushes a byte to the stack
*/

void push_byte(CPU *cpu, byte_t value);
/*
    6502 pop from stack
    Pops a byte from the stack
*/

byte_t pop_byte(CPU *cpu);
/*
    Adjust PC by instruction length
*/
void adjust_pc(CPU *cpu, byte_t instruction_length);

/*
    6502 clock
    Executes one tick of the CPU
    Executes the instructions on the last cycle (not clock cycle accurate to the 6502)
*/
void clock(CPU *cpu);

/*
    6502 irq interrupt
    Interrupt request signal
    Will not be executed if I flag is set
    Takes 7 cycles
*/
void irq(CPU *cpu);
/*
    6502 nmi interrupt
    Non-maskable interrupt
    Does not check I flag before executing
    Takes 8 cycles
*/
void nmi(CPU *cpu);
/*
    6502 reset
    Resets CPU to initial state
    sets PC to address stored at 0xFFFC
    Takes 8 cycles
*/
void reset(CPU *cpu);

#endif
