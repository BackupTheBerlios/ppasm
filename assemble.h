#ifndef ASSEMBLE_H_INCLUDED
#define ASSEMBLE_H_INCLUDED
#include "types.h"
#include <stdlib.h>
#include <stdio.h>

#define PREAMBLE_SIZE 0x20
u16 count_instructions();
void create_preamble(u8* preamb, u8* prog, u16 num_instr);
void assemble(FILE* file);
void generate_listing(FILE* file, size_t num_ops);
extern u32 clkfreq;
extern u8 clkreg;
#endif // ASSEMBLE_H_INCLUDED
