//CPU struct for 6502 processor

#ifndef CPU_H
#define CPU_H

#include <stdint.h>
typedef uint8_t Byte;
typedef uint16_t Word;

//enum for STATUS flags
typedef enum {
    C = (1 << 0), //Carry
    Z = (1 << 1), //Zero
    I = (1 << 2), //Interrupt disable
    D = (1 << 3), //Decimal mode
    B = (1 << 4), //Break
    U = (1 << 5), //Unused
    V = (1 << 6), //Overflow
    N = (1 << 7), //Negative
} STATUS;

typedef struct CPU {
    Byte A; //Accumulator
    Byte X; //X register
    Byte Y; //Y register
    Byte SP; //Stack pointer
    Word PC; //Program counter
    Byte P; //Processor status
    unsigned char* memory; //pointer to memory
} CPU;

//prototypes
void init(CPU *cpu);
void print_cpu_state(CPU *cpu);
void reset(CPU *cpu);
void setFlag(CPU *cpu, STATUS flag);

#endif
