// CPU struct for 6502 processor
#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <cstddef>
#include <string>

//Using X Macros because they are fun
#define CPU_6592_OPCODES \
FUNC_MAP(BRK)                   \
FUNC_MAP(ORA)                   \
FUNC_MAP(ASL)                   \
FUNC_MAP(PHP)                   \
FUNC_MAP(BPL)                   \
FUNC_MAP(CLC)                   \
FUNC_MAP(JSR)                   \
FUNC_MAP(AND)                   \
FUNC_MAP(BIT)                   \
FUNC_MAP(ROL)                   \
FUNC_MAP(PLP)                   \
FUNC_MAP(BMI)                   \
FUNC_MAP(SEC)                   \
FUNC_MAP(RTI)                   \
FUNC_MAP(EOR)                   \
FUNC_MAP(LSR)                   \
FUNC_MAP(PHA)                   \
FUNC_MAP(JMP)                   \
FUNC_MAP(BVC)                   \
FUNC_MAP(CLI)                   \
FUNC_MAP(RTS)                   \
FUNC_MAP(ADC)                   \
FUNC_MAP(ROR)                   \
FUNC_MAP(PLA)                   \
FUNC_MAP(ROR_ACC)               \
FUNC_MAP(BVS)                   \
FUNC_MAP(SEI)                   \
FUNC_MAP(STA)                   \
FUNC_MAP(STY)                   \
FUNC_MAP(STX)                   \
FUNC_MAP(DEY)                   \
FUNC_MAP(TXA)                   \
FUNC_MAP(BCC)                   \
FUNC_MAP(TYA)                   \
FUNC_MAP(TXS)                   \
FUNC_MAP(LDY)                   \
FUNC_MAP(LDA)                   \
FUNC_MAP(LDX)                   \
FUNC_MAP(TAY)                   \
FUNC_MAP(TAX)                   \
FUNC_MAP(BCS)                   \
FUNC_MAP(CLV)                   \
FUNC_MAP(TSX)                   \
FUNC_MAP(CPY)                   \
FUNC_MAP(CMP)                   \
FUNC_MAP(DEC)                   \
FUNC_MAP(INY)                   \
FUNC_MAP(DEX)                   \
FUNC_MAP(BNE)                   \
FUNC_MAP(CLD)                   \
FUNC_MAP(CPX)                   \
FUNC_MAP(SBC)                   \
FUNC_MAP(INC)                   \
FUNC_MAP(INX)                   \
FUNC_MAP(BEQ)                   \
FUNC_MAP(SED)                   \
FUNC_MAP(NOP)

#define CPU_6502_ADDRESSING_MODES  \
FUNC_MAP(IMP)                  \
FUNC_MAP(IMM)                  \
FUNC_MAP(ZP0)                  \
FUNC_MAP(ZPX)                  \
FUNC_MAP(ZPY)                  \
FUNC_MAP(REL)                  \
FUNC_MAP(ABS)                  \
FUNC_MAP(ABX)                  \
FUNC_MAP(ABY)                  \
FUNC_MAP(IND)                  \
FUNC_MAP(IZX)                  \
FUNC_MAP(IZY)

#define ALL_INSTRUCTIONS    \
FUNC_MAP(BRK, IMP, 0x00)       \
FUNC_MAP(ORA, IZX, 0x01)       \
FUNC_MAP(ORA, ZP0, 0x05)       \
FUNC_MAP(ASL, ZP0, 0x06)       \
FUNC_MAP(PHP, IMP, 0x08)       \
FUNC_MAP(ORA, IMM, 0x09)       \
FUNC_MAP(ASL, ACC, 0x0A)       \
FUNC_MAP(ASL, ABS, 0x0E)       \
FUNC_MAP(ORA, ABS, 0x0D)       \
FUNC_MAP(BPL, REL, 0x10)       \
FUNC_MAP(ORA, IZY, 0x11)       \
FUNC_MAP(ORA, ZPX, 0x15)       \
FUNC_MAP(ASL, ZPX, 0x16)       \
FUNC_MAP(CLC, IMP, 0x18)       \
FUNC_MAP(ORA, ABY, 0x19)       \
FUNC_MAP(ORA, ABX, 0x1D)       \
FUNC_MAP(ASL, ABX, 0x1E)       \
FUNC_MAP(JSR, ABS, 0x20)       \
FUNC_MAP(AND, IZX, 0x21)       \
FUNC_MAP(BRK, ZP0, 0x24)       \
FUNC_MAP(AND, ZP0, 0x25)       \
FUNC_MAP(ROL, ZP0, 0x26)       \
FUNC_MAP(PLP, IMP, 0x28)       \
FUNC_MAP(AND, IMM, 0x29)       \
FUNC_MAP(ROL, ACC, 0x2A)       \
FUNC_MAP(BIT, ABS, 0x2C)       \
FUNC_MAP(AND, ABS, 0x2D)       \
FUNC_MAP(ROL, ABS, 0x2E)       \
FUNC_MAP(BMI, REL, 0x30)       \
FUNC_MAP(AND, IZY, 0x31)       \
FUNC_MAP(AND, ZPX, 0x35)       \
FUNC_MAP(ROL, ZPX, 0x36)       \
FUNC_MAP(SEC, IMP, 0x38)       \
FUNC_MAP(AND, ABY, 0x39)       \
FUNC_MAP(AND, ABX, 0x3D)       \
FUNC_MAP(ROL, ABX, 0x3E)       \
FUNC_MAP(RTI, IMP, 0x40)       \
FUNC_MAP(EOR, IZX, 0x41)       \
FUNC_MAP(EOR, ZP0, 0x45)       \
FUNC_MAP(LSR, ZP0, 0x46)       \
FUNC_MAP(PHA, IMP, 0x48)       \
FUNC_MAP(EOR, IMM, 0x49)       \
FUNC_MAP(LSR, ACC, 0x4A)       \
FUNC_MAP(JMP, ABS, 0x4C)       \
FUNC_MAP(EOR, ABS, 0x4D)       \
FUNC_MAP(LSR, ABS, 0x4E)       \
FUNC_MAP(BVC, REL, 0x50)       \
FUNC_MAP(EOR, IZY, 0x51)       \
FUNC_MAP(EOR, ZPX, 0x55)       \
FUNC_MAP(LSR, ZPX, 0x56)       \
FUNC_MAP(CLI, IMP, 0x58)       \
FUNC_MAP(EOR, ABY, 0x59)       \
FUNC_MAP(EOR, ABX, 0x5D)       \
FUNC_MAP(LSR, ABX, 0x5E)       \
FUNC_MAP(RTS, IMP, 0x60)       \
FUNC_MAP(ADC, IZX, 0x61)       \
FUNC_MAP(ADC, ZP0, 0x65)       \
FUNC_MAP(ROR, ZP0, 0x66)       \
FUNC_MAP(PLA, IMP, 0x68)       \
FUNC_MAP(ADC, IMM, 0x69)       \
FUNC_MAP(ROR, ACC, 0x6A)       \
FUNC_MAP(JMP, IND, 0x6C)       \
FUNC_MAP(ADC, ABS, 0x6D)       \
FUNC_MAP(ROR, ABS, 0x6E)       \
FUNC_MAP(BVS, REL, 0x70)       \
FUNC_MAP(ADC, IZY, 0x71)       \
FUNC_MAP(ADC, ZPX, 0x75)       \
FUNC_MAP(ROR, ZPX, 0x76)       \
FUNC_MAP(SEI, IMP, 0x78)       \
FUNC_MAP(ADC, ABY, 0x79)       \
FUNC_MAP(ADC, ABX, 0x7D)       \
FUNC_MAP(ROR, ABX, 0x7E)       \
FUNC_MAP(STA, IZX, 0x81)       \
FUNC_MAP(STY, ZP0, 0x84)       \
FUNC_MAP(STA, ZP0, 0x85)       \
FUNC_MAP(STX, ZP0, 0x86)       \
FUNC_MAP(DEY, IMP, 0x88)       \
FUNC_MAP(TXA, IMP, 0x8A)       \
FUNC_MAP(STY, ABS, 0x8C)       \
FUNC_MAP(STA, ABS, 0x8D)       \
FUNC_MAP(STX, ABS, 0x8E)       \
FUNC_MAP(BCC, REL, 0x90)       \
FUNC_MAP(STA, IZY, 0x91)       \
FUNC_MAP(STY, ZPX, 0x94)       \
FUNC_MAP(STA, ZPX, 0x95)       \
FUNC_MAP(STX, ZPY, 0x96)       \
FUNC_MAP(TYA, IMP, 0x98)       \
FUNC_MAP(STA, ABY, 0x99)       \
FUNC_MAP(TXS, IMP, 0x9A)       \
FUNC_MAP(STA, ABX, 0x9D)       \
FUNC_MAP(LDY, IMM, 0xA0)       \
FUNC_MAP(LDA, IZX, 0xA1)       \
FUNC_MAP(LDX, IMM, 0xA2)       \
FUNC_MAP(LDY, ZP0, 0xA4)       \
FUNC_MAP(LDA, ZP0, 0xA5)       \
FUNC_MAP(LDX, ZP0, 0xA6)       \
FUNC_MAP(TAY, IMP, 0xA8)       \
FUNC_MAP(LDA, IMM, 0xA9)       \
FUNC_MAP(TAX, IMP, 0xAA)       \
FUNC_MAP(LDY, ABS, 0xAC)       \
FUNC_MAP(LDA, ABS, 0xAD)       \
FUNC_MAP(LDX, ABS, 0xAE)       \
FUNC_MAP(BCS, REL, 0xB0)       \
FUNC_MAP(LDA, IZY, 0xB1)       \
FUNC_MAP(LDY, ZPX, 0xB4)       \
FUNC_MAP(LDA, ZPX, 0xB5)       \
FUNC_MAP(LDX, ZPY, 0xB6)       \
FUNC_MAP(CLV, IMP, 0xB8)       \
FUNC_MAP(LDA, ABY, 0xB9)       \
FUNC_MAP(TSX, IMP, 0xBA)       \
FUNC_MAP(LDY, ABX, 0xBC)       \
FUNC_MAP(LDA, ABX, 0xBD)       \
FUNC_MAP(LDX, ABY, 0xBE)       \
FUNC_MAP(CPY, IMM, 0xC0)       \
FUNC_MAP(CMP, IZX, 0xC1)       \
FUNC_MAP(CPY, ZP0, 0xC4)       \
FUNC_MAP(CMP, ZP0, 0xC5)       \
FUNC_MAP(DEC, ZP0, 0xC6)       \
FUNC_MAP(INY, IMP, 0xC8)       \
FUNC_MAP(CMP, IMM, 0xC9)       \
FUNC_MAP(DEX, IMP, 0xCA)       \
FUNC_MAP(CPY, ABS, 0xCC)       \
FUNC_MAP(CMP, ABS, 0xCD)       \
FUNC_MAP(DEC, ABS, 0xCE)       \
FUNC_MAP(BNE, REL, 0xD0)       \
FUNC_MAP(CMP, IZY, 0xD1)       \
FUNC_MAP(CMP, ZPX, 0xD5)       \
FUNC_MAP(DEC, ZPX, 0xD6)       \
FUNC_MAP(CLD, IMP, 0xD8)       \
FUNC_MAP(CMP, ABY, 0xD9)       \
FUNC_MAP(CMP, ABX, 0xDD)       \
FUNC_MAP(DEC, ABX, 0xDE)       \
FUNC_MAP(CPX, IMM, 0xE0)       \
FUNC_MAP(SBC, IZX, 0xE1)       \
FUNC_MAP(CPX, ZP0, 0xE4)       \
FUNC_MAP(SBC, ZP0, 0xE5)       \
FUNC_MAP(INC, ZP0, 0xE6)       \
FUNC_MAP(INX, IMP, 0xE8)       \
FUNC_MAP(SBC, IMM, 0xE9)       \
FUNC_MAP(NOP, IMP, 0xEA)       \
FUNC_MAP(CPX, ABS, 0xEC)       \
FUNC_MAP(SBC, ABS, 0xED)       \
FUNC_MAP(INC, ABS, 0xEE)       \
FUNC_MAP(BEQ, REL, 0xF0)       \
FUNC_MAP(SBC, IZY, 0xF1)       \
FUNC_MAP(SBC, ZPX, 0xF5)       \
FUNC_MAP(INC, ZPX, 0xF6)       \
FUNC_MAP(SED, IMP, 0xF8)       \
FUNC_MAP(SBC, ABY, 0xF9)       \
FUNC_MAP(SBC, ABX, 0xFD)       \
FUNC_MAP(INC, ABX, 0xFE)

typedef uint8_t byte_t;
typedef uint16_t word_t; //used for addresses
typedef struct c6502 cpu_s; // Forward declaration
typedef byte_t (*instruction_func_t)(cpu_s *cpu);

typedef enum 
{
    #define FUNC_MAP(mode) ADDR_MODE_##mode,
    CPU_6502_ADDRESSING_MODES
    #undef FUNC_MAP
} cpu_addr_mode_e;

typedef enum
{
    #define FUNC_MAP(name, mode, opcode) INSTRUCTION_##name##_##mode = opcode,
    ALL_INSTRUCTIONS
    #undef FUNC_MAP
} cpu_ins_e;

#define FUNC_MAP(name) byte_t name(cpu_s *cpu);
CPU_6502_ADDRESSING_MODES
CPU_6592_OPCODES
#undef FUNC_MAP


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
} cpu_status_flag_e;

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
bool get_flag(cpu_s *cpu, cpu_status_flag_e flag);

/*
    6502 set flag
    Sets flag in status register
*/
void set_flag(cpu_s *cpu, cpu_status_flag_e flag, bool value);

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

