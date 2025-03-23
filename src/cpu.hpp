// CPU struct for 6502 processor
#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <cstddef>
#include <string>

//Using X Macros because they are fun
#define UNIQUE_OPCODES \
TRANSFORM_FUNCTION(BRK)                   \
TRANSFORM_FUNCTION(ORA)                   \
TRANSFORM_FUNCTION(ASL)                   \
TRANSFORM_FUNCTION(PHP)                   \
TRANSFORM_FUNCTION(BPL)                   \
TRANSFORM_FUNCTION(CLC)                   \
TRANSFORM_FUNCTION(JSR)                   \
TRANSFORM_FUNCTION(AND)                   \
TRANSFORM_FUNCTION(BIT)                   \
TRANSFORM_FUNCTION(ROL)                   \
TRANSFORM_FUNCTION(PLP)                   \
TRANSFORM_FUNCTION(BMI)                   \
TRANSFORM_FUNCTION(SEC)                   \
TRANSFORM_FUNCTION(RTI)                   \
TRANSFORM_FUNCTION(EOR)                   \
TRANSFORM_FUNCTION(LSR)                   \
TRANSFORM_FUNCTION(PHA)                   \
TRANSFORM_FUNCTION(JMP)                   \
TRANSFORM_FUNCTION(BVC)                   \
TRANSFORM_FUNCTION(CLI)                   \
TRANSFORM_FUNCTION(RTS)                   \
TRANSFORM_FUNCTION(ADC)                   \
TRANSFORM_FUNCTION(ROR)                   \
TRANSFORM_FUNCTION(PLA)                   \
TRANSFORM_FUNCTION(ROR_ACC)               \
TRANSFORM_FUNCTION(BVS)                   \
TRANSFORM_FUNCTION(SEI)                   \
TRANSFORM_FUNCTION(STA)                   \
TRANSFORM_FUNCTION(STY)                   \
TRANSFORM_FUNCTION(STX)                   \
TRANSFORM_FUNCTION(DEY)                   \
TRANSFORM_FUNCTION(TXA)                   \
TRANSFORM_FUNCTION(BCC)                   \
TRANSFORM_FUNCTION(TYA)                   \
TRANSFORM_FUNCTION(TXS)                   \
TRANSFORM_FUNCTION(LDY)                   \
TRANSFORM_FUNCTION(LDA)                   \
TRANSFORM_FUNCTION(LDX)                   \
TRANSFORM_FUNCTION(TAY)                   \
TRANSFORM_FUNCTION(TAX)                   \
TRANSFORM_FUNCTION(BCS)                   \
TRANSFORM_FUNCTION(CLV)                   \
TRANSFORM_FUNCTION(TSX)                   \
TRANSFORM_FUNCTION(CPY)                   \
TRANSFORM_FUNCTION(CMP)                   \
TRANSFORM_FUNCTION(DEC)                   \
TRANSFORM_FUNCTION(INY)                   \
TRANSFORM_FUNCTION(DEX)                   \
TRANSFORM_FUNCTION(BNE)                   \
TRANSFORM_FUNCTION(CLD)                   \
TRANSFORM_FUNCTION(CPX)                   \
TRANSFORM_FUNCTION(SBC)                   \
TRANSFORM_FUNCTION(INC)                   \
TRANSFORM_FUNCTION(INX)                   \
TRANSFORM_FUNCTION(BEQ)                   \
TRANSFORM_FUNCTION(SED)                   \
TRANSFORM_FUNCTION(NOP)

#define LIST_OF_ADDR_MODES  \
TRANSFORM_FUNCTION(IMP)                  \
TRANSFORM_FUNCTION(IMM)                  \
TRANSFORM_FUNCTION(ZP0)                  \
TRANSFORM_FUNCTION(ZPX)                  \
TRANSFORM_FUNCTION(ZPY)                  \
TRANSFORM_FUNCTION(REL)                  \
TRANSFORM_FUNCTION(ABS)                  \
TRANSFORM_FUNCTION(ABX)                  \
TRANSFORM_FUNCTION(ABY)                  \
TRANSFORM_FUNCTION(IND)                  \
TRANSFORM_FUNCTION(IZX)                  \
TRANSFORM_FUNCTION(IZY)

#define ALL_INSTRUCTIONS    \
TRANSFORM_FUNCTION(BRK, IMP, 0x00)       \
TRANSFORM_FUNCTION(ORA, IZX, 0x01)       \
TRANSFORM_FUNCTION(ORA, ZP0, 0x05)       \
TRANSFORM_FUNCTION(ASL, ZP0, 0x06)       \
TRANSFORM_FUNCTION(PHP, IMP, 0x08)       \
TRANSFORM_FUNCTION(ORA, IMM, 0x09)       \
TRANSFORM_FUNCTION(ASL, ACC, 0x0A)       \
TRANSFORM_FUNCTION(ASL, ABS, 0x0E)       \
TRANSFORM_FUNCTION(ORA, ABS, 0x0D)       \
TRANSFORM_FUNCTION(BPL, REL, 0x10)       \
TRANSFORM_FUNCTION(ORA, IZY, 0x11)       \
TRANSFORM_FUNCTION(ORA, ZPX, 0x15)       \
TRANSFORM_FUNCTION(ASL, ZPX, 0x16)       \
TRANSFORM_FUNCTION(CLC, IMP, 0x18)       \
TRANSFORM_FUNCTION(ORA, ABY, 0x19)       \
TRANSFORM_FUNCTION(ORA, ABX, 0x1D)       \
TRANSFORM_FUNCTION(ASL, ABX, 0x1E)       \
TRANSFORM_FUNCTION(JSR, ABS, 0x20)       \
TRANSFORM_FUNCTION(AND, IZX, 0x21)       \
TRANSFORM_FUNCTION(BRK, ZP0, 0x24)       \
TRANSFORM_FUNCTION(AND, ZP0, 0x25)       \
TRANSFORM_FUNCTION(ROL, ZP0, 0x26)       \
TRANSFORM_FUNCTION(PLP, IMP, 0x28)       \
TRANSFORM_FUNCTION(AND, IMM, 0x29)       \
TRANSFORM_FUNCTION(ROL, ACC, 0x2A)       \
TRANSFORM_FUNCTION(BIT, ABS, 0x2C)       \
TRANSFORM_FUNCTION(AND, ABS, 0x2D)       \
TRANSFORM_FUNCTION(ROL, ABS, 0x2E)       \
TRANSFORM_FUNCTION(BMI, REL, 0x30)       \
TRANSFORM_FUNCTION(AND, IZY, 0x31)       \
TRANSFORM_FUNCTION(AND, ZPX, 0x35)       \
TRANSFORM_FUNCTION(ROL, ZPX, 0x36)       \
TRANSFORM_FUNCTION(SEC, IMP, 0x38)       \
TRANSFORM_FUNCTION(AND, ABY, 0x39)       \
TRANSFORM_FUNCTION(AND, ABX, 0x3D)       \
TRANSFORM_FUNCTION(ROL, ABX, 0x3E)       \
TRANSFORM_FUNCTION(RTI, IMP, 0x40)       \
TRANSFORM_FUNCTION(EOR, IZX, 0x41)       \
TRANSFORM_FUNCTION(EOR, ZP0, 0x45)       \
TRANSFORM_FUNCTION(LSR, ZP0, 0x46)       \
TRANSFORM_FUNCTION(PHA, IMP, 0x48)       \
TRANSFORM_FUNCTION(EOR, IMM, 0x49)       \
TRANSFORM_FUNCTION(LSR, ACC, 0x4A)       \
TRANSFORM_FUNCTION(JMP, ABS, 0x4C)       \
TRANSFORM_FUNCTION(EOR, ABS, 0x4D)       \
TRANSFORM_FUNCTION(LSR, ABS, 0x4E)       \
TRANSFORM_FUNCTION(BVC, REL, 0x50)       \
TRANSFORM_FUNCTION(EOR, IZY, 0x51)       \
TRANSFORM_FUNCTION(EOR, ZPX, 0x55)       \
TRANSFORM_FUNCTION(LSR, ZPX, 0x56)       \
TRANSFORM_FUNCTION(CLI, IMP, 0x58)       \
TRANSFORM_FUNCTION(EOR, ABY, 0x59)       \
TRANSFORM_FUNCTION(EOR, ABX, 0x5D)       \
TRANSFORM_FUNCTION(LSR, ABX, 0x5E)       \
TRANSFORM_FUNCTION(RTS, IMP, 0x60)       \
TRANSFORM_FUNCTION(ADC, IZX, 0x61)       \
TRANSFORM_FUNCTION(ADC, ZP0, 0x65)       \
TRANSFORM_FUNCTION(ROR, ZP0, 0x66)       \
TRANSFORM_FUNCTION(PLA, IMP, 0x68)       \
TRANSFORM_FUNCTION(ADC, IMM, 0x69)       \
TRANSFORM_FUNCTION(ROR, ACC, 0x6A)       \
TRANSFORM_FUNCTION(JMP, IND, 0x6C)       \
TRANSFORM_FUNCTION(ADC, ABS, 0x6D)       \
TRANSFORM_FUNCTION(ROR, ABS, 0x6E)       \
TRANSFORM_FUNCTION(BVS, REL, 0x70)       \
TRANSFORM_FUNCTION(ADC, IZY, 0x71)       \
TRANSFORM_FUNCTION(ADC, ZPX, 0x75)       \
TRANSFORM_FUNCTION(ROR, ZPX, 0x76)       \
TRANSFORM_FUNCTION(SEI, IMP, 0x78)       \
TRANSFORM_FUNCTION(ADC, ABY, 0x79)       \
TRANSFORM_FUNCTION(ADC, ABX, 0x7D)       \
TRANSFORM_FUNCTION(ROR, ABX, 0x7E)       \
TRANSFORM_FUNCTION(STA, IZX, 0x81)       \
TRANSFORM_FUNCTION(STY, ZP0, 0x84)       \
TRANSFORM_FUNCTION(STA, ZP0, 0x85)       \
TRANSFORM_FUNCTION(STX, ZP0, 0x86)       \
TRANSFORM_FUNCTION(DEY, IMP, 0x88)       \
TRANSFORM_FUNCTION(TXA, IMP, 0x8A)       \
TRANSFORM_FUNCTION(STY, ABS, 0x8C)       \
TRANSFORM_FUNCTION(STA, ABS, 0x8D)       \
TRANSFORM_FUNCTION(STX, ABS, 0x8E)       \
TRANSFORM_FUNCTION(BCC, REL, 0x90)       \
TRANSFORM_FUNCTION(STA, IZY, 0x91)       \
TRANSFORM_FUNCTION(STY, ZPX, 0x94)       \
TRANSFORM_FUNCTION(STA, ZPX, 0x95)       \
TRANSFORM_FUNCTION(STX, ZPY, 0x96)       \
TRANSFORM_FUNCTION(TYA, IMP, 0x98)       \
TRANSFORM_FUNCTION(STA, ABY, 0x99)       \
TRANSFORM_FUNCTION(TXS, IMP, 0x9A)       \
TRANSFORM_FUNCTION(STA, ABX, 0x9D)       \
TRANSFORM_FUNCTION(LDY, IMM, 0xA0)       \
TRANSFORM_FUNCTION(LDA, IZX, 0xA1)       \
TRANSFORM_FUNCTION(LDX, IMM, 0xA2)       \
TRANSFORM_FUNCTION(LDY, ZP0, 0xA4)       \
TRANSFORM_FUNCTION(LDA, ZP0, 0xA5)       \
TRANSFORM_FUNCTION(LDX, ZP0, 0xA6)       \
TRANSFORM_FUNCTION(TAY, IMP, 0xA8)       \
TRANSFORM_FUNCTION(LDA, IMM, 0xA9)       \
TRANSFORM_FUNCTION(TAX, IMP, 0xAA)       \
TRANSFORM_FUNCTION(LDY, ABS, 0xAC)       \
TRANSFORM_FUNCTION(LDA, ABS, 0xAD)       \
TRANSFORM_FUNCTION(LDX, ABS, 0xAE)       \
TRANSFORM_FUNCTION(BCS, REL, 0xB0)       \
TRANSFORM_FUNCTION(LDA, IZY, 0xB1)       \
TRANSFORM_FUNCTION(LDY, ZPX, 0xB4)       \
TRANSFORM_FUNCTION(LDA, ZPX, 0xB5)       \
TRANSFORM_FUNCTION(LDX, ZPY, 0xB6)       \
TRANSFORM_FUNCTION(CLV, IMP, 0xB8)       \
TRANSFORM_FUNCTION(LDA, ABY, 0xB9)       \
TRANSFORM_FUNCTION(TSX, IMP, 0xBA)       \
TRANSFORM_FUNCTION(LDY, ABX, 0xBC)       \
TRANSFORM_FUNCTION(LDA, ABX, 0xBD)       \
TRANSFORM_FUNCTION(LDX, ABY, 0xBE)       \
TRANSFORM_FUNCTION(CPY, IMM, 0xC0)       \
TRANSFORM_FUNCTION(CMP, IZX, 0xC1)       \
TRANSFORM_FUNCTION(CPY, ZP0, 0xC4)       \
TRANSFORM_FUNCTION(CMP, ZP0, 0xC5)       \
TRANSFORM_FUNCTION(DEC, ZP0, 0xC6)       \
TRANSFORM_FUNCTION(INY, IMP, 0xC8)       \
TRANSFORM_FUNCTION(CMP, IMM, 0xC9)       \
TRANSFORM_FUNCTION(DEX, IMP, 0xCA)       \
TRANSFORM_FUNCTION(CPY, ABS, 0xCC)       \
TRANSFORM_FUNCTION(CMP, ABS, 0xCD)       \
TRANSFORM_FUNCTION(DEC, ABS, 0xCE)       \
TRANSFORM_FUNCTION(BNE, REL, 0xD0)       \
TRANSFORM_FUNCTION(CMP, IZY, 0xD1)       \
TRANSFORM_FUNCTION(CMP, ZPX, 0xD5)       \
TRANSFORM_FUNCTION(DEC, ZPX, 0xD6)       \
TRANSFORM_FUNCTION(CLD, IMP, 0xD8)       \
TRANSFORM_FUNCTION(CMP, ABY, 0xD9)       \
TRANSFORM_FUNCTION(CMP, ABX, 0xDD)       \
TRANSFORM_FUNCTION(DEC, ABX, 0xDE)       \
TRANSFORM_FUNCTION(CPX, IMM, 0xE0)       \
TRANSFORM_FUNCTION(SBC, IZX, 0xE1)       \
TRANSFORM_FUNCTION(CPX, ZP0, 0xE4)       \
TRANSFORM_FUNCTION(SBC, ZP0, 0xE5)       \
TRANSFORM_FUNCTION(INC, ZP0, 0xE6)       \
TRANSFORM_FUNCTION(INX, IMP, 0xE8)       \
TRANSFORM_FUNCTION(SBC, IMM, 0xE9)       \
TRANSFORM_FUNCTION(NOP, IMP, 0xEA)       \
TRANSFORM_FUNCTION(CPX, ABS, 0xEC)       \
TRANSFORM_FUNCTION(SBC, ABS, 0xED)       \
TRANSFORM_FUNCTION(INC, ABS, 0xEE)       \
TRANSFORM_FUNCTION(BEQ, REL, 0xF0)       \
TRANSFORM_FUNCTION(SBC, IZY, 0xF1)       \
TRANSFORM_FUNCTION(SBC, ZPX, 0xF5)       \
TRANSFORM_FUNCTION(INC, ZPX, 0xF6)       \
TRANSFORM_FUNCTION(SED, IMP, 0xF8)       \
TRANSFORM_FUNCTION(SBC, ABY, 0xF9)       \
TRANSFORM_FUNCTION(SBC, ABX, 0xFD)       \
TRANSFORM_FUNCTION(INC, ABX, 0xFE)

typedef uint8_t byte_t;
typedef uint16_t word_t;
typedef struct c6502 cpu_s; // Forward declaration
typedef byte_t (*instruction_func_t)(cpu_s *cpu);

typedef enum 
{
    #define TRANSFORM_FUNCTION(mode) ADDR_MODE_##mode,
    LIST_OF_ADDR_MODES
    #undef TRANSFORM_FUNCTION
} cpu_addr_mode_t;

typedef enum
{
    #define TRANSFORM_FUNCTION(name, mode, opcode) INSTRUCTION_##name##_##mode = opcode,
    ALL_INSTRUCTIONS
    #undef TRANSFORM_FUNCTION
} cpu_ins_t;

#define TRANSFORM_FUNCTION(name) byte_t name(cpu_s *cpu);
LIST_OF_ADDR_MODES
UNIQUE_OPCODES
#undef TRANSFORM_FUNCTION


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
} cpu_status_t;

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
    byte_t length;
    byte_t cycles;
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
} cpu_register_t;

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
char *addr_mode_to_string(cpu_addr_mode_t mode);


/*
    6502 get flag
    Reads flag from status register
*/
bool get_flag(cpu_s *cpu, cpu_status_t flag);

/*
    6502 set flag
    Sets flag in status register
*/
void set_flag(cpu_s *cpu, cpu_status_t flag, bool value);

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

