#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define UNIMPLEMENTED() \
    fprintf(stderr, "%s:%d: %s: Unimplemented function\n", __FILE__, __LINE__, __func__); \
    abort();
#define MEM_SIZE 1024 * 1024 * 64


//instruction table
static Instruction table[256] = {0};
Word address;
Byte address_rel;
Byte value;
//function to initialize the CPU

void register_init(CPU *cpu) {
    cpu->A = 0;
    cpu->X = 0;
    cpu->Y = 0;
    cpu->SP = 0xFF;
    cpu->PC = read_from_addr(cpu, 0xFFFC) | (read_from_addr(cpu, 0xFFFD) << 8); 
    cpu->STATUS = 0x34;
    return;
}

void memory_init(CPU *cpu, Byte *memory) {
    cpu->memory = memory;
    return;
}

void init(CPU *cpu, Byte *memory) {
    memory_init(cpu, memory);
    register_init(cpu);
    return;
}

void print_cpu_state(CPU *cpu) {
    printf("A: 0x%02X\n", cpu->A);
    printf("X: 0x%02X\n", cpu->X);
    printf("Y: 0x%02X\n", cpu->Y);
    printf("SP: 0x%02X\n", cpu->SP);
    printf("PC: 0x%04X\n", cpu->PC);
    printf("STATUS: 0x%02X\n", cpu->STATUS);
    return;
}

void reset(CPU *cpu) {
    register_init(cpu);
    return;
}

void set_flag(CPU *cpu, STATUS flag, int value) {
    if (value) {
        cpu->STATUS |= flag;
    } else {
        cpu->STATUS &= ~flag;
    }
    return;
}

Byte peek(CPU *cpu) {
    assert(cpu != NULL);
    assert(cpu->memory != NULL);
    return cpu->memory[cpu->PC];
}

Byte read(CPU *cpu) {
    assert(cpu != NULL);
    assert(cpu->memory != NULL);
    return cpu->memory[cpu->PC++];
}

Byte read_from_addr(CPU *cpu, Word address) {
    assert(cpu != NULL);
    assert(cpu->memory != NULL);
    return cpu->memory[address];
}

void write_to_addr(CPU *cpu, Word address, Byte value) {
    assert(cpu != NULL);
    assert(cpu->memory != NULL);
    cpu->memory[address] = value;
    return;
}

void push(CPU *cpu, Byte byte) {
    assert(cpu != NULL);
    assert(cpu->memory != NULL);
    if(cpu->SP == 0x00){
        cpu->SP = 0xFF; //stack is full, wrap around
    }
    Word stack_addr = 0x0100 + cpu->SP;
    write_to_addr(cpu, stack_addr, byte);
    cpu->SP--;
    return;
}

Byte pop(CPU *cpu) {
    assert(cpu != NULL);
    assert(cpu->memory != NULL);
    if(cpu->SP == 0xFF){
        cpu->SP = 0x00; //stack is empty, wrap around
    }
    Word stack_addr = 0x0100 + cpu->SP;
    Byte byte = read_from_addr(cpu, stack_addr);
    cpu->SP++;
    return byte;
}

/*
    Debugging function to print the contents of an instructions
    Assumes instruction is in the table
*/

void print_instruction(Byte opcode) {
    Instruction instruction = table[opcode];
    //if any pointers are null, print "not implemented"
    if (!(instruction.name == NULL|| instruction.fetch == NULL || instruction.execute == NULL)) {
        printf("Instruction: %s\n", instruction.name);
        printf("Opcode: 0x%02X\n", instruction.opcode);
        printf("Fetch: %p\n", (void*) instruction.fetch);
        printf("Execute: %p\n", (void*) instruction.execute);
        printf("Cycles: %d\n", instruction.cycles);
        printf("Length: %d\n", instruction.length);
    } 
    return;
}

void init_instruction_table(void){
    //initialize the table
    //reference https://www.masswerk.at/6502/6502_instruction_set.html
    table[0x00] = (Instruction){
        .name = "BRK",
        .opcode = 0x00,
        .fetch = IMP,
        .execute = BRK,
        .cycles = 7,
        .length = 1
    };
    table[0x01] = (Instruction) {
        .name = "ORA",
        .opcode = 0x01,
        .fetch = IZX,
        .execute = ORA,
        .cycles = 6,
        .length = 2
    };
    table[0x02] = (Instruction) {
        .name = "???",
        .opcode = 0x02,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x03] = (Instruction) {
        .name = "???",
        .opcode = 0x03,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x04] = (Instruction) {
        .name = "???",
        .opcode = 0x04,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x05] = (Instruction) {
        .name = "ORA",
        .opcode = 0x05,
        .fetch = ZP0,
        .execute = ORA,
        .cycles = 3,
        .length = 2
    };
    table[0x06] = (Instruction) {
        .name = "ASL",
        .opcode = 0x06,
        .fetch = ZP0,
        .execute = ASL,
        .cycles = 5,
        .length = 2
    };
    table[0x07] = (Instruction) {
        .name = "???",
        .opcode = 0x07,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x08] = (Instruction) {
        .name = "PHP",
        .opcode = 0x08,
        .fetch = IMP,
        .execute = PHP,
        .cycles = 3,
        .length = 1
    };
    table[0x09] = (Instruction) {
        .name = "ORA",
        .opcode = 0x09,
        .fetch = IMM,
        .execute = ORA,
        .cycles = 2,
        .length = 2
    };
    table[0x0A] = (Instruction) {
        .name = "ASL",
        .opcode = 0x0A,
        .fetch = IMP,
        .execute = ASL,
        .cycles = 2,
        .length = 1
    };
    table[0x0B] = (Instruction) {
        .name = "???",
        .opcode = 0x0B,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x0C] = (Instruction) {
        .name = "???",
        .opcode = 0x0C,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x0D] = (Instruction) {
        .name = "ORA",
        .opcode = 0x0D,
        .fetch = ABS,
        .execute = ORA,
        .cycles = 4,
        .length = 3
    };
    table[0x0E] = (Instruction) {
        .name = "ASL",
        .opcode = 0x0E,
        .fetch = ABS,
        .execute = ASL,
        .cycles = 6,
        .length = 3
    };
    table[0x0F] = (Instruction) {
        .name = "???",
        .opcode = 0x0F,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x10] = (Instruction) {
        .name = "BPL",
        .opcode = 0x10,
        .fetch = REL,
        .execute = BPL,
        .cycles = 2,
        .length = 2
    };
    table[0x11] = (Instruction) {
        .name = "ORA",
        .opcode = 0x11,
        .fetch = IZY,
        .execute = ORA,
        .cycles = 5,
        .length = 2
    };
    table[0x12] = (Instruction) {
        .name = "???",
        .opcode = 0x12,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x13] = (Instruction) {
        .name = "???",
        .opcode = 0x13,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x14] = (Instruction) {
        .name = "???",
        .opcode = 0x14,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x15] = (Instruction) {
        .name = "ORA",
        .opcode = 0x15,
        .fetch = ZPX,
        .execute = ORA,
        .cycles = 4,
        .length = 2
    };
    table[0x16] = (Instruction) {
        .name = "ASL",
        .opcode = 0x16,
        .fetch = ZPX,
        .execute = ASL,
        .cycles = 6,
        .length = 2
    };
    table[0x17] = (Instruction) {
        .name = "???",
        .opcode = 0x17,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x18] = (Instruction) {
        .name = "CLC",
        .opcode = 0x18,
        .fetch = IMP,
        .execute = CLC,
        .cycles = 2,
        .length = 1
    };
    table[0x19] = (Instruction) {
        .name = "ORA",
        .opcode = 0x19,
        .fetch = ABY,
        .execute = ORA,
        .cycles = 4,
        .length = 3
    };
    table[0x1A] = (Instruction) {
        .name = "???",
        .opcode = 0x1A,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x1B] = (Instruction) {
        .name = "???",
        .opcode = 0x1B,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x1C] = (Instruction) {
        .name = "???",
        .opcode = 0x1C,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x1D] = (Instruction) {
        .name = "ORA",
        .opcode = 0x1D,
        .fetch = ABX,
        .execute = ORA,
        .cycles = 4,
        .length = 3
    };
    table[0x1E] = (Instruction) {
        .name = "ASL",
        .opcode = 0x1E,
        .fetch = ABX,
        .execute = ASL,
        .cycles = 7,
        .length = 3
    };
    table[0x1F] = (Instruction) {
        .name = "???",
        .opcode = 0x1F,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x20] = (Instruction) {
        .name = "JSR",
        .opcode = 0x20,
        .fetch = ABS,
        .execute = JSR,
        .cycles = 6,
        .length = 3
    };
    table[0x21] = (Instruction) {
        .name = "AND",
        .opcode = 0x21,
        .fetch = IZX,
        .execute = AND,
        .cycles = 6,
        .length = 2
    };
    table[0x22] = (Instruction) {
        .name = "???",
        .opcode = 0x22,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x23] = (Instruction) {
        .name = "???",
        .opcode = 0x23,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x24] = (Instruction) {
        .name = "BIT",
        .opcode = 0x24,
        .fetch = ZP0,
        .execute = BIT,
        .cycles = 3,
        .length = 2
    };
    table[0x25] = (Instruction) {
        .name = "AND",
        .opcode = 0x25,
        .fetch = ZP0,
        .execute = AND,
        .cycles = 3,
        .length = 2
    };
    table[0x26] = (Instruction) {
        .name = "ROL",
        .opcode = 0x26,
        .fetch = ZP0,
        .execute = ROL,
        .cycles = 5,
        .length = 2
    };
    table[0x27] = (Instruction) {
        .name = "???",
        .opcode = 0x27,
        .fetch = NULL,
        .execute = NULL,
        .cycles = 0,
        .length = 0
    };
    table[0x28] = (Instruction) {
        .name = "PLP",
        .opcode = 0x28,
        .fetch = IMP,
        .execute = PLP,
        .cycles = 4,
        .length = 1
    };
}



/*
    * Implied addressing mode
    * No operand needed
    * Returns 0
*/
Byte IMP (CPU *cpu) {
    assert(cpu != NULL);
    return 0;
}

/*
    * Immediate addressing mode
    * Fetches the next byte in memory and stores it in the value variable
    * Increments the program counter
*/
Byte IMM (CPU *cpu) {
    assert(cpu != NULL);
    value = read(cpu);
    return 0;
}

/*
    * Zero page addressing mode
    * Fetches the next byte in memory and stores it in the zero'th page of memory
    * Increments the program counter
*/

Byte ZP0 (CPU *cpu) {
    assert(cpu != NULL);
    address = read(cpu);
    address &= 0x00FF;
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Zero page X addressing mode
    * Fetches the next byte in memory and stores it in the zero'th page of memory
    * Increments the program counter
    * Adds the X register to the address
    * Stores the result in the address variable
    * Fetches the value at the address and stores it in the value variable
*/
Byte ZPX (CPU *cpu) {
    assert(cpu != NULL);
    address = read(cpu);
    address &= 0x00FF;
    address += cpu->X;
    address &= 0x00FF;
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Zero page Y addressing mode
    * Fetches the next byte in memory and stores it in the zero'th page of memory
    * Increments the program counter
    * Adds the Y register to the address
    * Stores the result in the address variable
    * Fetches the value at the address and stores it in the value variable
*/
Byte ZPY (CPU *cpu) {
    assert(cpu != NULL);
    address = read(cpu);
    address &= 0x00FF;
    address += cpu->Y;
    address &= 0x00FF;
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Relative addressing mode
    * Fetches the next byte in memory and stores it in the address_rel variable
    * Increments the program counter
*/
Byte REL (CPU *cpu) {
    assert(cpu != NULL);
    address_rel = read(cpu);
    return 0;
}

/*
    * Absolute addressing mode
    * Fetches the next two bytes in memory and stores it in the address variable
    * Increments the program counter
    * Fetches the value at the address and stores it in the value variable
*/
Byte ABS (CPU *cpu) {
    assert(cpu != NULL);
    address = read(cpu);
    address |= (read(cpu) << 8);
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Absolute X addressing mode
    * Fetches the next two bytes in memory and stores it in the address variable
    * Increments the program counter
    * Adds the X register to the address
    * Stores the result in the address variable
    * Fetches the value at the address and stores it in the value variable
*/

Byte ABX (CPU *cpu) {
    assert(cpu != NULL);
    address = read(cpu);
    address |= (read(cpu) << 8);
    address += cpu->X;
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Absolute Y addressing mode
    * Fetches the next two bytes in memory and stores it in the address variable
    * Increments the program counter
    * Adds the Y register to the address
    * Stores the result in the address variable
    * Fetches the value at the address and stores it in the value variable
*/

Byte ABY (CPU *cpu) {
    assert(cpu != NULL);
    address = read(cpu);
    address |= (read(cpu) << 8);
    address += cpu->Y;
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Indirect addressing mode
    * Fetches the next two bytes in memory and stores it in the address variable
    * Increments the program counter
    * Fetches the value at the address and stores it in the value variable
    * Fetches the next two bytes in memory and stores it in the address variable
    * Increments the program counter
    * Fetches the value at the address and stores it in the value variable
*/

Byte IND (CPU *cpu) {
    assert(cpu != NULL);
    Word ptr = read(cpu);
    ptr |= (read(cpu) << 8);
    //simulate page boundary hardware bug
    if ((ptr & 0x00FF) == 0x00FF) {
        address = (read_from_addr(cpu, ptr & 0xFF00) << 8) | read_from_addr(cpu, ptr);
    } else {
        address = (read_from_addr(cpu, ptr + 1) << 8) | read_from_addr(cpu, ptr);
    }
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Indirect X addressing mode
    * Fetches the next byte in memory and stores it in the address variable
    * Increments the program counter
    * Adds the X register to the address
    * Stores the result in the address variable
    * Fetches the value at the address and stores it in the value variable
*/

Byte IZX (CPU *cpu) {
    assert(cpu != NULL);
    Word ptr = read(cpu);
    ptr &= 0x00FF;
    address = (read_from_addr(cpu, ptr + 1) << 8) | read_from_addr(cpu, ptr);
    address += cpu->X;
    value = read_from_addr(cpu, address);
    return 0;
}

/*
    * Indirect Y addressing mode
    * Fetches the next byte in memory and stores it in the address variable
    * Increments the program counter
    * Fetches the value at the address and stores it in the value variable
    * Adds the Y register to the address
    * Stores the result in the address variable
*/
//todo not so sure about this one
Byte IZY (CPU *cpu) {
    assert(cpu != NULL);
    Word ptr = read(cpu);
    ptr &= 0x00FF;
    address = (read_from_addr(cpu, ptr + 1) << 8) | read_from_addr(cpu, ptr);
    address += cpu->Y;
    value = read_from_addr(cpu, address);
    return 0;
}

//instruction implementations

Byte BRK(CPU *cpu) {
    (void)cpu;
    return 0;
}

Byte ORA(CPU *cpu) {
    cpu->A |= value;
    set_flag(cpu, Z, cpu->A == 0x00);
    set_flag(cpu, N, cpu->A & 0x80);
    return 0;
}

Byte ASL(CPU *cpu) {
    set_flag(cpu, C, value & 0x80);
    value <<= 1;
    set_flag(cpu, Z, value == 0x00);
    set_flag(cpu, N, value & 0x80);
    return 0;
}
/*
    Push Processor Status on Stack
    Pushes the status register onto the stack
    Decrements the stack pointer?
*/
Byte PHP(CPU *cpu) {
    push(cpu, cpu->STATUS);
    return 0;
}

//branch helper function that takes a flag as a parameter
//returns 1 if page boundary is crossed and branch taken, 0 otherwise
Byte branch_on_flag(CPU *cpu, STATUS flag) {
    assert(cpu != NULL);
    int cycles = 0;
    if (cpu->STATUS & flag) {
        if ((cpu->PC & 0xFF00) != ((cpu->PC + address_rel) & 0xFF00)) {
            cycles++;
        }
        cpu->PC += address_rel;
    }
    return cycles;
}

/*
    Branch on Result Plus (N = 0)
    If the negative flag is not set, add the relative address to the program counter
    If the program counter crosses a page boundary, return 1 to indicate an extra cycle is required
*/
Byte BPL(CPU *cpu) {
    assert(cpu != NULL);
    int cycles = branch_on_flag(cpu, N);
    return cycles;
}

/*
 * Clear Carry Flag
 * Clears the carry flag
*/
Byte CLC(CPU *cpu) {
    assert(cpu != NULL);
    set_flag(cpu, C, 0);
    return 0;
}

/*
    Jump to Subroutine
    Pushes the program counter onto the stack
    Sets the program counter to the address
*/
Byte JSR(CPU *cpu) {
    assert(cpu != NULL);
    push(cpu, (cpu->PC - 1) >> 8);
    push(cpu, (cpu->PC - 1) & 0x00FF);
    cpu->PC = address;
    return 0;
}

/*
    AND Memory with Accumulator
    ANDs the value with the accumulator
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
Byte AND(CPU *cpu) {
    assert(cpu != NULL);
    cpu->A &= value;
    set_flag(cpu, Z, cpu->A == 0x00);
    set_flag(cpu, N, cpu->A & 0x80);
    return 0;
}

/*
    BIT Test Bits in Memory with Accumulator
    ANDs the value with the accumulator
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
    Sets the overflow flag to the sixth bit of the value
*/
Byte BIT(CPU *cpu) {
    assert(cpu != NULL);
    Byte result = cpu->A & value;
    //this flag is set based on the result of the AND
    set_flag(cpu, Z, result == 0x00);
    //these 2 flags are set to the 6th and 7th bits of the value
    set_flag(cpu, N, value & 0x80);
    set_flag(cpu, V, value & 0x40);
    return 0;
}

/*
    Rotate Left (Memory)
    Gets value from address and rotates it left by 1 bit
    Sets the carry flag to the 7th bit of the value
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
    Stores the result in memory
*/
Byte ROL(CPU *cpu) {
    //assume address is set
    assert(cpu != NULL);
    set_flag(cpu, C, value & 0x80);
    value <<= 1;
    set_flag(cpu, Z, value == 0x00);
    set_flag(cpu, N, value & 0x80);
    write_to_addr(cpu, address, value);
    return 0;
}

/*
    Rotate Left (Accumulator)
    Rotates the accumulator left by 1 bit
    Sets the carry flag to the 7th bit of the value
    Sets the zero flag if the result is zero
    Sets the negative flag if the result is negative
*/
Byte ROL_ACC(CPU *cpu) {
    assert(cpu != NULL);
    set_flag(cpu, C, cpu->A & 0x80);
    cpu->A <<= 1;
    set_flag(cpu, Z, cpu->A == 0x00);
    set_flag(cpu, N, cpu->A & 0x80);
    return 0;
}

/*
    Pull Processor Status from Stack
    The status register will be pulled with the break
    flag and bit 5 ignored.
*/
Byte PLP(CPU *cpu) {
    assert(cpu != NULL);
    cpu->STATUS = pop(cpu);
    set_flag(cpu, U, 1);
    set_flag(cpu, B, 0);
    return 0;
}