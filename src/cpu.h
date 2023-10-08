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
#define INSTRUCTION_JMP_IND 0x6C
#define INSTRUCTION_ADC_ABS 0x6D
#define INSTRUCTION_ROR_ABS 0x6E
#define INSTRUCTION_BVS_REL 0x70
#define INSTRUCTION_ADC_IZY 0x71
#define INSTRUCTION_ADC_ZPX 0x75
#define INSTRUCTION_ROR_ZPX 0x76
#define INSTRUCTION_SEI_IMP 0x78
#define INSTRUCTION_ADC_ABY 0x79
#define INSTRUCTION_ADC_ABX 0x7D
#define INSTRUCTION_ROR_ABX 0x7E
#define INSTRUCTION_STA_IZX 0x81
#define INSTRUCTION_STY_ZP0 0x84
#define INSTRUCTION_STA_ZP0 0x85
#define INSTRUCTION_STX_ZP0 0x86
#define INSTRUCTION_DEY_IMP 0x88
#define INSTRUCTION_TXA_IMP 0x8A
#define INSTRUCTION_STY_ABS 0x8C
#define INSTRUCTION_STA_ABS 0x8D
#define INSTRUCTION_STX_ABS 0x8E
#define INSTRUCTION_BCC_REL 0x90
#define INSTRUCTION_STA_IZY 0x91
#define INSTRUCTION_STY_ZPX 0x94
#define INSTRUCTION_STA_ZPX 0x95
#define INSTRUCTION_STX_ZPY 0x96
#define INSTRUCTION_TYA_IMP 0x98
#define INSTRUCTION_STA_ABY 0x99
#define INSTRUCTION_TXS_IMP 0x9A
#define INSTRUCTION_STA_ABX 0x9D
#define INSTRUCTION_LDY_IMM 0xA0
#define INSTRUCTION_LDA_IZX 0xA1
#define INSTRUCTION_LDX_IMM 0xA2
#define INSTRUCTION_LDY_ZP0 0xA4
#define INSTRUCTION_LDA_ZP0 0xA5
#define INSTRUCTION_LDX_ZP0 0xA6
#define INSTRUCTION_TAY_IMP 0xA8
#define INSTRUCTION_LDA_IMM 0xA9
#define INSTRUCTION_TAX_IMP 0xAA
#define INSTRUCTION_LDY_ABS 0xAC
#define INSTRUCTION_LDA_ABS 0xAD
#define INSTRUCTION_LDX_ABS 0xAE
#define INSTRUCTION_BCS_REL 0xB0
#define INSTRUCTION_LDA_IZY 0xB1
#define INSTRUCTION_LDY_ZPX 0xB4
#define INSTRUCTION_LDA_ZPX 0xB5
#define INSTRUCTION_LDX_ZPY 0xB6
#define INSTRUCTION_CLV_IMP 0xB8
#define INSTRUCTION_LDA_ABY 0xB9
#define INSTRUCTION_TSX_IMP 0xBA
#define INSTRUCTION_LDY_ABX 0xBC
#define INSTRUCTION_LDA_ABX 0xBD
#define INSTRUCTION_LDX_ABY 0xBE
#define INSTRUCTION_CPY_IMM 0xC0
#define INSTRUCTION_CMP_IZX 0xC1
#define INSTRUCTION_CPY_ZP0 0xC4
#define INSTRUCTION_CMP_ZP0 0xC5
#define INSTRUCTION_DEC_ZP0 0xC6
#define INSTRUCTION_INY_IMP 0xC8
#define INSTRUCTION_CMP_IMM 0xC9
#define INSTRUCTION_DEX_IMP 0xCA
#define INSTRUCTION_CPY_ABS 0xCC    
#define INSTRUCTION_CMP_ABS 0xCD
#define INSTRUCTION_DEC_ABS 0xCE
#define INSTRUCTION_BNE_REL 0xD0
#define INSTRUCTION_CMP_IZY 0xD1
#define INSTRUCTION_CMP_ZPX 0xD5
#define INSTRUCTION_DEC_ZPX 0xD6
#define INSTRUCTION_CLD_IMP 0xD8
#define INSTRUCTION_CMP_ABY 0xD9
#define INSTRUCTION_CMP_ABX 0xDD
#define INSTRUCTION_DEC_ABX 0xDE
#define INSTRUCTION_CPX_IMM 0xE0
#define INSTRUCTION_SBC_IZX 0xE1
#define INSTRUCTION_CPX_ZP0 0xE4
#define INSTRUCTION_SBC_ZP0 0xE5
#define INSTRUCTION_INC_ZP0 0xE6
#define INSTRUCTION_INX_IMP 0xE8
#define INSTRUCTION_SBC_IMM 0xE9
#define INSTRUCTION_NOP_IMP 0xEA
#define INSTRUCTION_CPX_ABS 0xEC
#define INSTRUCTION_SBC_ABS 0xED
#define INSTRUCTION_INC_ABS 0xEE
#define INSTRUCTION_BEQ_REL 0xF0
#define INSTRUCTION_SBC_IZY 0xF1
#define INSTRUCTION_SBC_ZPX 0xF5
#define INSTRUCTION_INC_ZPX 0xF6
#define INSTRUCTION_SED_IMP 0xF8
#define INSTRUCTION_SBC_ABY 0xF9
#define INSTRUCTION_SBC_ABX 0xFD
#define INSTRUCTION_INC_ABX 0xFE



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
} STATUS_FLAGS;

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
    * CYCLES: Number of cycles left for current instruction 
    *         or an external event to complete such as an interrupt
    *         or a DMA transfer
    * memory: pointer to memory
*/
typedef struct CPU {
    Byte A;
    Byte X;
    Byte Y;
    Byte SP;
    Word PC;
    Byte STATUS;
    Byte CYCLES;
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
void print_instruction(Byte opcode, Instruction *table);

//prototypes
void init_instruction_table(Instruction* table);
void init(CPU *cpu, Byte* memory);
void reset(CPU *cpu);
//todo add interrupt functions
void set_flag(CPU *cpu, STATUS_FLAGS flag, Byte value);
void request_additional_cycles(CPU *cpu, Byte cycles);

Byte peek(CPU *cpu);
Byte process_byte(CPU *cpu); //will consume a byte
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
Byte BVS(CPU *cpu);
Byte SEI(CPU *cpu);
Byte STA(CPU *cpu);
Byte STY(CPU *cpu);
Byte STX(CPU *cpu);
Byte DEY(CPU *cpu);
Byte TXA(CPU *cpu);
Byte BCC(CPU *cpu);
Byte TYA(CPU *cpu);
Byte TXS(CPU *cpu);
Byte LDY(CPU *cpu);
Byte LDA(CPU *cpu);
Byte LDX(CPU *cpu);
Byte TAY(CPU *cpu);
Byte TAX(CPU *cpu);
Byte BCS(CPU *cpu);
Byte CLV(CPU *cpu);
Byte TSX(CPU *cpu);
Byte CPY(CPU *cpu);
Byte CMP(CPU *cpu);
Byte DEC(CPU *cpu);
Byte INY(CPU *cpu);
Byte DEX(CPU *cpu);
Byte BNE(CPU *cpu);
Byte CLD(CPU *cpu);
Byte CPX(CPU *cpu);
Byte SBC(CPU *cpu);
Byte INC(CPU *cpu);
Byte INX(CPU *cpu);
Byte BEQ(CPU *cpu);
Byte SED(CPU *cpu);
Byte NOP(CPU *cpu);
#endif
