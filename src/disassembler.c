#include "disassembler.h"
#include <stdio.h>
#include <stdlib.h>
#define UNIMPLEMENTED()                                                                   \
    fprintf(stderr, "%s:%d: %s: Unimplemented function\n", __FILE__, __LINE__, __func__); \
    abort();

// initialize the disassembler
void init_disassembler(Disassembler *disassembler, Instruction *table)
{
    disassembler->table = table;
    disassembler->assembly = NULL;
}
