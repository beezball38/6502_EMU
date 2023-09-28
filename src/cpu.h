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

//prototypes
void init(CPU *cpu);
void print_cpu_state(CPU *cpu);
void reset(CPU *cpu);
void setFlag(CPU *cpu, STATUS flag, int value);
Byte peek(CPU *cpu);
Byte read(CPU *cpu);
Byte read_from_addr(CPU *cpu, Word address);
#endif
