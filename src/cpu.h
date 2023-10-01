//CPU struct for 6502 processor
#ifndef CPU_H
#define CPU_H
#include <stdint.h>
typedef uint8_t Byte;
typedef uint16_t Word;

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
    * SP: Stack pointer
    * PC: Program counter
    * STATUS: Processor status
    * memory: pointer to memory
*/
//Meant to be used as a singleton
typedef struct CPU {
    Byte A;
    Byte X;
    Byte Y;
    Word SP;
    Word PC;
    Byte STATUS;
    unsigned char* memory;
} CPU;

typedef struct Instruction {
    char *name;
    Byte opcode;
    Byte length;
    Byte cycles;
    Byte (*fetch)(CPU *cpu);
    Byte (*execute)(CPU *cpu);
} Instruction;

//helper functions, not for prime time
void print_cpu_state(CPU *cpu);
void print_instruction(Byte opcode);

//prototypes
void init_instruction_table();
void init(CPU *cpu, Byte* memory);
void reset(CPU *cpu);
//todo add interrupt functions
void set_flag(CPU *cpu, STATUS flag, int value);

Byte peek(CPU *cpu);
Byte read(CPU *cpu); //will consume a byte
Byte read_from_addr(CPU *cpu, Word address);
void write_to_addr(CPU *cpu, Word address, Byte value);

//For use with cpu->SP
void push(CPU *cpu, Byte value);
Byte pop(CPU *cpu);

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


//cpu singleton
extern CPU cpu;
#endif
