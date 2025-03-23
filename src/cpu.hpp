// CPU struct for 6502 processor
#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <cstddef>
#include <string>

//Using X Macros because they are fun
#define CPU_6592_OPCODES \
XMAC(BRK)                   \
XMAC(ORA)                   \
XMAC(ASL)                   \
XMAC(PHP)                   \
XMAC(BPL)                   \
XMAC(CLC)                   \
XMAC(JSR)                   \
XMAC(AND)                   \
XMAC(BIT)                   \
XMAC(ROL)                   \
XMAC(PLP)                   \
XMAC(BMI)                   \
XMAC(SEC)                   \
XMAC(RTI)                   \
XMAC(EOR)                   \
XMAC(LSR)                   \
XMAC(PHA)                   \
XMAC(JMP)                   \
XMAC(BVC)                   \
XMAC(CLI)                   \
XMAC(RTS)                   \
XMAC(ADC)                   \
XMAC(ROR)                   \
XMAC(PLA)                   \
XMAC(ROR_ACC)               \
XMAC(BVS)                   \
XMAC(SEI)                   \
XMAC(STA)                   \
XMAC(STY)                   \
XMAC(STX)                   \
XMAC(DEY)                   \
XMAC(TXA)                   \
XMAC(BCC)                   \
XMAC(TYA)                   \
XMAC(TXS)                   \
XMAC(LDY)                   \
XMAC(LDA)                   \
XMAC(LDX)                   \
XMAC(TAY)                   \
XMAC(TAX)                   \
XMAC(BCS)                   \
XMAC(CLV)                   \
XMAC(TSX)                   \
XMAC(CPY)                   \
XMAC(CMP)                   \
XMAC(DEC)                   \
XMAC(INY)                   \
XMAC(DEX)                   \
XMAC(BNE)                   \
XMAC(CLD)                   \
XMAC(CPX)                   \
XMAC(SBC)                   \
XMAC(INC)                   \
XMAC(INX)                   \
XMAC(BEQ)                   \
XMAC(SED)                   \
XMAC(NOP)

#define CPU_6502_ADDRESSING_MODES  \
XMAC(IMP)                  \
XMAC(IMM)                  \
XMAC(ZP0)                  \
XMAC(ZPX)                  \
XMAC(ZPY)                  \
XMAC(REL)                  \
XMAC(ABS)                  \
XMAC(ABX)                  \
XMAC(ABY)                  \
XMAC(IND)                  \
XMAC(IZX)                  \
XMAC(IZY)

#define ALL_INSTRUCTIONS    \
XMAC(BRK, IMP, 0x00)       \
XMAC(ORA, IZX, 0x01)       \
XMAC(ORA, ZP0, 0x05)       \
XMAC(ASL, ZP0, 0x06)       \
XMAC(PHP, IMP, 0x08)       \
XMAC(ORA, IMM, 0x09)       \
XMAC(ASL, ACC, 0x0A)       \
XMAC(ASL, ABS, 0x0E)       \
XMAC(ORA, ABS, 0x0D)       \
XMAC(BPL, REL, 0x10)       \
XMAC(ORA, IZY, 0x11)       \
XMAC(ORA, ZPX, 0x15)       \
XMAC(ASL, ZPX, 0x16)       \
XMAC(CLC, IMP, 0x18)       \
XMAC(ORA, ABY, 0x19)       \
XMAC(ORA, ABX, 0x1D)       \
XMAC(ASL, ABX, 0x1E)       \
XMAC(JSR, ABS, 0x20)       \
XMAC(AND, IZX, 0x21)       \
XMAC(BRK, ZP0, 0x24)       \
XMAC(AND, ZP0, 0x25)       \
XMAC(ROL, ZP0, 0x26)       \
XMAC(PLP, IMP, 0x28)       \
XMAC(AND, IMM, 0x29)       \
XMAC(ROL, ACC, 0x2A)       \
XMAC(BIT, ABS, 0x2C)       \
XMAC(AND, ABS, 0x2D)       \
XMAC(ROL, ABS, 0x2E)       \
XMAC(BMI, REL, 0x30)       \
XMAC(AND, IZY, 0x31)       \
XMAC(AND, ZPX, 0x35)       \
XMAC(ROL, ZPX, 0x36)       \
XMAC(SEC, IMP, 0x38)       \
XMAC(AND, ABY, 0x39)       \
XMAC(AND, ABX, 0x3D)       \
XMAC(ROL, ABX, 0x3E)       \
XMAC(RTI, IMP, 0x40)       \
XMAC(EOR, IZX, 0x41)       \
XMAC(EOR, ZP0, 0x45)       \
XMAC(LSR, ZP0, 0x46)       \
XMAC(PHA, IMP, 0x48)       \
XMAC(EOR, IMM, 0x49)       \
XMAC(LSR, ACC, 0x4A)       \
XMAC(JMP, ABS, 0x4C)       \
XMAC(EOR, ABS, 0x4D)       \
XMAC(LSR, ABS, 0x4E)       \
XMAC(BVC, REL, 0x50)       \
XMAC(EOR, IZY, 0x51)       \
XMAC(EOR, ZPX, 0x55)       \
XMAC(LSR, ZPX, 0x56)       \
XMAC(CLI, IMP, 0x58)       \
XMAC(EOR, ABY, 0x59)       \
XMAC(EOR, ABX, 0x5D)       \
XMAC(LSR, ABX, 0x5E)       \
XMAC(RTS, IMP, 0x60)       \
XMAC(ADC, IZX, 0x61)       \
XMAC(ADC, ZP0, 0x65)       \
XMAC(ROR, ZP0, 0x66)       \
XMAC(PLA, IMP, 0x68)       \
XMAC(ADC, IMM, 0x69)       \
XMAC(ROR, ACC, 0x6A)       \
XMAC(JMP, IND, 0x6C)       \
XMAC(ADC, ABS, 0x6D)       \
XMAC(ROR, ABS, 0x6E)       \
XMAC(BVS, REL, 0x70)       \
XMAC(ADC, IZY, 0x71)       \
XMAC(ADC, ZPX, 0x75)       \
XMAC(ROR, ZPX, 0x76)       \
XMAC(SEI, IMP, 0x78)       \
XMAC(ADC, ABY, 0x79)       \
XMAC(ADC, ABX, 0x7D)       \
XMAC(ROR, ABX, 0x7E)       \
XMAC(STA, IZX, 0x81)       \
XMAC(STY, ZP0, 0x84)       \
XMAC(STA, ZP0, 0x85)       \
XMAC(STX, ZP0, 0x86)       \
XMAC(DEY, IMP, 0x88)       \
XMAC(TXA, IMP, 0x8A)       \
XMAC(STY, ABS, 0x8C)       \
XMAC(STA, ABS, 0x8D)       \
XMAC(STX, ABS, 0x8E)       \
XMAC(BCC, REL, 0x90)       \
XMAC(STA, IZY, 0x91)       \
XMAC(STY, ZPX, 0x94)       \
XMAC(STA, ZPX, 0x95)       \
XMAC(STX, ZPY, 0x96)       \
XMAC(TYA, IMP, 0x98)       \
XMAC(STA, ABY, 0x99)       \
XMAC(TXS, IMP, 0x9A)       \
XMAC(STA, ABX, 0x9D)       \
XMAC(LDY, IMM, 0xA0)       \
XMAC(LDA, IZX, 0xA1)       \
XMAC(LDX, IMM, 0xA2)       \
XMAC(LDY, ZP0, 0xA4)       \
XMAC(LDA, ZP0, 0xA5)       \
XMAC(LDX, ZP0, 0xA6)       \
XMAC(TAY, IMP, 0xA8)       \
XMAC(LDA, IMM, 0xA9)       \
XMAC(TAX, IMP, 0xAA)       \
XMAC(LDY, ABS, 0xAC)       \
XMAC(LDA, ABS, 0xAD)       \
XMAC(LDX, ABS, 0xAE)       \
XMAC(BCS, REL, 0xB0)       \
XMAC(LDA, IZY, 0xB1)       \
XMAC(LDY, ZPX, 0xB4)       \
XMAC(LDA, ZPX, 0xB5)       \
XMAC(LDX, ZPY, 0xB6)       \
XMAC(CLV, IMP, 0xB8)       \
XMAC(LDA, ABY, 0xB9)       \
XMAC(TSX, IMP, 0xBA)       \
XMAC(LDY, ABX, 0xBC)       \
XMAC(LDA, ABX, 0xBD)       \
XMAC(LDX, ABY, 0xBE)       \
XMAC(CPY, IMM, 0xC0)       \
XMAC(CMP, IZX, 0xC1)       \
XMAC(CPY, ZP0, 0xC4)       \
XMAC(CMP, ZP0, 0xC5)       \
XMAC(DEC, ZP0, 0xC6)       \
XMAC(INY, IMP, 0xC8)       \
XMAC(CMP, IMM, 0xC9)       \
XMAC(DEX, IMP, 0xCA)       \
XMAC(CPY, ABS, 0xCC)       \
XMAC(CMP, ABS, 0xCD)       \
XMAC(DEC, ABS, 0xCE)       \
XMAC(BNE, REL, 0xD0)       \
XMAC(CMP, IZY, 0xD1)       \
XMAC(CMP, ZPX, 0xD5)       \
XMAC(DEC, ZPX, 0xD6)       \
XMAC(CLD, IMP, 0xD8)       \
XMAC(CMP, ABY, 0xD9)       \
XMAC(CMP, ABX, 0xDD)       \
XMAC(DEC, ABX, 0xDE)       \
XMAC(CPX, IMM, 0xE0)       \
XMAC(SBC, IZX, 0xE1)       \
XMAC(CPX, ZP0, 0xE4)       \
XMAC(SBC, ZP0, 0xE5)       \
XMAC(INC, ZP0, 0xE6)       \
XMAC(INX, IMP, 0xE8)       \
XMAC(SBC, IMM, 0xE9)       \
XMAC(NOP, IMP, 0xEA)       \
XMAC(CPX, ABS, 0xEC)       \
XMAC(SBC, ABS, 0xED)       \
XMAC(INC, ABS, 0xEE)       \
XMAC(BEQ, REL, 0xF0)       \
XMAC(SBC, IZY, 0xF1)       \
XMAC(SBC, ZPX, 0xF5)       \
XMAC(INC, ZPX, 0xF6)       \
XMAC(SED, IMP, 0xF8)       \
XMAC(SBC, ABY, 0xF9)       \
XMAC(SBC, ABX, 0xFD)       \
XMAC(INC, ABX, 0xFE)

typedef uint8_t byte_t;
typedef uint16_t word_t;
typedef struct c6502 cpu_s; // Forward declaration
typedef byte_t (*instruction_func_t)(cpu_s *cpu);

typedef enum 
{
    #define XMAC(mode) ADDR_MODE_##mode,
    CPU_6502_ADDRESSING_MODES
    #undef XMAC
} cpu_addr_mode_e;

typedef enum
{
    #define XMAC(name, mode, opcode) INSTRUCTION_##name##_##mode = opcode,
    ALL_INSTRUCTIONS
    #undef XMAC
} cpu_ins_e;

#define XMAC(name) byte_t name(cpu_s *cpu);
CPU_6502_ADDRESSING_MODES
CPU_6592_OPCODES
#undef XMAC


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
} cpu_status_e;

/*
 * Instruction struct
 * name: name of instruction
 * opcode: opcode of instruction
 * length: length of instruction in bytes
 * cycles: number of cycles instruction takes
 * execute: function pointer to execute instruction
 * fetch: function pointer to function to fetch operand
 */
typedef struct
{
    std::string name;
    byte_t opcode;
    byte_t cycles;
    byte_t length;
    instruction_func_t fetch;
    instruction_func_t execute;
} cpu_instruction_s;

typedef enum
{
    A,
    X,
    Y,
    SP,
    PC,
    STATUS
} cpu_register_e;

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
struct c6502
{
    byte_t A;
    byte_t X;
    byte_t Y;
    byte_t SP;
    word_t PC;
    byte_t STATUS;

    size_t cycles;
    cpu_instruction_s *current_instruction;
    bool pc_changed;
    bool does_need_additional_cycle;
    byte_t *memory;
    cpu_instruction_s table[256];
};

void init_instruction_table(cpu_s *cpu);
void cpu_init(cpu_s *cpu, byte_t *memory);
//to_string function for addressing modes
char *addr_mode_to_string(cpu_addr_mode_e mode);


/*
    6502 get flag
    Reads flag from status register
*/
bool get_flag(cpu_s *cpu, cpu_status_e flag);

/*
    6502 set flag
    Sets flag in status register
*/
void set_flag(cpu_s *cpu, cpu_status_e flag, bool value);

/*
    6502 peek
    Peeks at the next byte in memory
    Does not increment PC
*/
byte_t peek(cpu_s *cpu);

/*
    6502 read from address
    Reads a byte from memory at address
*/
byte_t read_from_addr(cpu_s *cpu, word_t address);

/*
    6502 write to address
    Writes a byte to memory at address
*/
void write_to_addr(cpu_s *cpu, word_t address, byte_t value);

/*
    6502 push to stack
    Pushes a byte to the stack
*/
void push_byte(cpu_s *cpu, byte_t value);

/*
    6502 pop from stack
    Pops a byte from the stack
*/
byte_t pop_byte(cpu_s *cpu);

/*
    Adjust PC by instruction length
*/
void adjust_pc(cpu_s *cpu, byte_t instruction_length);

/*
    6502 clock
    Executes one tick of the CPU
    Executes the instructions on the last cycle (not clock cycle accurate to the 6502)
*/
void clock(cpu_s *cpu);

/*
    6502 irq interrupt
    Interrupt request signal
    Will not be executed if I flag is set
    Takes 7 cycles
*/
void irq(cpu_s *cpu);

/*
    6502 nmi interrupt
    Non-maskable interrupt
    Does not check I flag before executing
    Takes 8 cycles
*/
void nmi(cpu_s *cpu);

/*
    6502 reset
    Resets CPU to initial state
    sets PC to address stored at 0xFFFC
    Takes 8 cycles
*/
void reset(cpu_s *cpu);

#endif

