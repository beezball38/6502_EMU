//CPU struct for 6502 processor
#ifndef CPU_H
#define CPU_H
#include <stdint.h>
typedef uint8_t Byte;
typedef uint16_t Word;
//constants for the instructions in order of opcode (except illegal instructions)
#define INSTRUCTION_BRK_IMP 0x00
#define INSTRUCTION_ORA_IZX 0x01
#define INSTRUCTION_ORA_ZP0 0x05
#define INSTRUCTION_ASL_ZP0 0x06
#define INSTRUCTION_PHP_IMP 0x08
#define INSTRUCTION_ORA_IMM 0x09
#define INSTRUCTION_ASL_ACC 0x0A
#define INSTRUCTION_ASL_ABS 0x0E
#define INSTRUCTION_ORA_ABS 0x0D
#define INSTRUCTION_BPL_REL 0x10
#define INSTRUCTION_ORA_IZY 0x11
#define INSTRUCTION_ORA_ZPX 0x15
#define INSTRUCTION_ASL_ZPX 0x16
#define INSTRUCTION_CLC_IMP 0x18
#define INSTRUCTION_ORA_ABY 0x19
#define INSTRUCTION_ORA_ABX 0x1D
#define INSTRUCTION_ASL_ABX 0x1E
#define INSTRUCTION_JSR_ABS 0x20
#define INSTRUCTION_AND_IZX 0x21
#define INSTRUCTION_BRK_ZP0 0x24
#define INSTRUCTION_AND_ZP0 0x25
#define INSTRUCTION_ROL_ZP0 0x26
#define INSTRUCTION_PLP_IMP 0x28
#define INSTRUCTION_AND_IMM 0x29
#define INSTRUCTION_ROL_ACC 0x2A
#define INSTRUCTION_BIT_ABS 0x2C
#define INSTRUCTION_AND_ABS 0x2D
#define INSTRUCTION_ROL_ABS 0x2E
#define INSTRUCTION_BMI_REL 0x30
#define INSTRUCTION_AND_IZY 0x31
#define INSTRUCTION_AND_ZPX 0x35
#define INSTRUCTION_ROL_ZPX 0x36
#define INSTRUCTION_SEC_IMP 0x38
#define INSTRUCTION_AND_ABY 0x39
#define INSTRUCTION_AND_ABX 0x3D
#define INSTRUCTION_ROL_ABX 0x3E
#define INSTRUCTION_RTI_IMP 0x40
#define INSTRUCTION_EOR_IZX 0x41
#define INSTRUCTION_EOR_ZP0 0x45
#define INSTRUCTION_LSR_ZP0 0x46
#define INSTRUCTION_PHA_IMP 0x48
#define INSTRUCTION_EOR_IMM 0x49
#define INSTRUCTION_LSR_ACC 0x4A
#define INSTRUCTION_JMP_ABS 0x4C
#define INSTRUCTION_EOR_ABS 0x4D
#define INSTRUCTION_LSR_ABS 0x4E
#define INSTRUCTION_BVC_REL 0x50
#define INSTRUCTION_EOR_IZY 0x51
#define INSTRUCTION_EOR_ZPX 0x55
#define INSTRUCTION_LSR_ZPX 0x56
#define INSTRUCTION_CLI_IMP 0x58
#define INSTRUCTION_EOR_ABY 0x59
#define INSTRUCTION_EOR_ABX 0x5D
#define INSTRUCTION_LSR_ABX 0x5E
#define INSTRUCTION_RTS_IMP 0x60
#define INSTRUCTION_ADC_IZX 0x61
#define INSTRUCTION_ADC_ZP0 0x65
#define INSTRUCTION_ROR_ZP0 0x66
#define INSTRUCTION_PLA_IMP 0x68
#define INSTRUCTION_ADC_IMM 0x69
#define INSTRUCTION_ROR_ACC 0x6A


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
typedef enum {
    C = (1 << 0), 
    Z = (1 << 1), 
    I = (1 << 2), 
    D = (1 << 3), 
    B = (1 << 4),
    U = (1 << 5), 
    V = (1 << 6), 
    N = (1 << 7), 
} STATUS;

/*
    * Instruction struct
    * name: name of instruction
    * opcode: opcode of instruction
    * length: length of instruction in bytes
    * cycles: number of cycles instruction takes
    * execute: function pointer to execute instruction
    * fetch: function pointer to function to fetch operand
*/



/*
    * CPU struct
    * A: Accumulator
    * X: X register
    * Y: Y register
    * SP: Stack pointer (address of stack which is on the first page of memory)
    * PC: Program counter
    * STATUS: Processor status
    * memory: pointer to memory
*/
//Meant to be used as a singleton
typedef struct CPU {
    Byte A;
    Byte X;
    Byte Y;
    Byte SP;
    Word PC;
    Byte STATUS;
    unsigned char* memory;
} CPU;

typedef Byte (*Ins_Func)(CPU *cpu);
typedef struct Instruction {
    char *name;
    Byte opcode;
    Byte length;
    Byte cycles;
    Ins_Func fetch;
    Ins_Func execute;
} Instruction;


//helper functions, not for prime time
void print_cpu_state(CPU *cpu);
void print_instruction(Byte opcode);

//prototypes
void init_instruction_table(void);
void init(CPU *cpu, Byte* memory);
void reset(CPU *cpu);
//todo add interrupt functions
void set_flag(CPU *cpu, STATUS flag, Byte value);

Byte peek(CPU *cpu);
Byte read(CPU *cpu); //will consume a byte
Byte read_from_addr(CPU *cpu, Word address);
void write_to_addr(CPU *cpu, Word address, Byte value);

//For use with cpu->SP
void push_stack(CPU *cpu, Byte value);
Byte pop_stack(CPU *cpu);

//addressing modes (fetch)
Byte IMP(CPU *cpu);
Byte IMM(CPU *cpu);
Byte ZP0(CPU *cpu);
Byte ZPX(CPU *cpu);
Byte ZPY(CPU *cpu);
Byte REL(CPU *cpu);
Byte ABS(CPU *cpu);
Byte ABX(CPU *cpu);
Byte ABY(CPU *cpu);
Byte IND(CPU *cpu);
Byte IZX(CPU *cpu);
Byte IZY(CPU *cpu);

//instructions (execute) in order of opcode
Byte BRK(CPU *cpu);
Byte ORA(CPU *cpu);
Byte ASL(CPU *cpu);
Byte PHP(CPU *cpu);
Byte BPL(CPU *cpu);
Byte CLC(CPU *cpu);
Byte JSR(CPU *cpu);
Byte AND(CPU *cpu);
Byte BIT(CPU *cpu);
Byte ROL(CPU *cpu);
Byte ROL_ACC(CPU *cpu);
Byte LSR_ACC(CPU *cpu);
Byte PLP(CPU *cpu);
Byte BMI(CPU *cpu);
Byte SEC(CPU *cpu);
Byte RTI(CPU *cpu);
Byte EOR(CPU *cpu);
Byte LSR(CPU *cpu);
Byte PHA(CPU *cpu);
Byte JMP(CPU *cpu);
Byte BVC(CPU *cpu);
Byte CLI(CPU *cpu);
Byte RTS(CPU *cpu);
Byte ADC(CPU *cpu);
Byte ROR(CPU *cpu);
Byte PLA(CPU *cpu);
Byte ROR_ACC(CPU *cpu);
#endif
