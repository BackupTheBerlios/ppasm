#include "assemble.h"
#include "util.h"
#include "opcodes.h"
#include "assert.h"

/*
Many thanks to cliff biffle for reverse engineering propeller tool binary format.
http://www.cliff.biffle.org/software/propeller/binary-format.html
*/

u32 clkfreq = 0;
u8  initial_clkreg = 0x6F;


/*****************************************************************\
*   Computes checksum                                             *
\*****************************************************************/
u8 compute_checksum(u8* preamb, u8* prog, u16 num_ops)
{
    u8  sum = 0;
    u8* byteptr = (u8*)prog;

    for(unsigned i = 0; i < (num_ops * sizeof(instruction_t)); i++)
        sum += byteptr[i];

    for(unsigned i = 0; i < PREAMBLE_SIZE; i++)
        sum += preamb[i];

    return (0x14 - sum) & 0xFF;
}

/*****************************************************************\
*   Counts the number of instructions                             *
\*****************************************************************/
u16 count_instructions()
{
    /* find number of operations */
    u16 ops = MAX_INSTRUCTIONS;
    do
    {
        ops--;
        if(flags[ops].valid)
            break;
    }
    while(ops);
    return ++ops;
}

void create_preamble(u8* preamb, u8* prog, u16 num_instr)
{
    if(clkfreq)
    {
        preamb[0] = clkfreq & 0xFF;
        preamb[1] = (clkfreq >> 8) & 0xFF;
        preamb[2] = (clkfreq >> 16) & 0xFF;
        preamb[3] = (clkfreq >> 24) & 0xFF;
    }
    else /* 80 Mhz by default */
    {
        preamb[0] = 0x00;
        preamb[1] = 0xB4;
        preamb[2] = 0xC4;
        preamb[3] = 0x04;
    }

    preamb[4] = initial_clkreg;
    preamb[5] = 0; /* initial checksum */
    preamb[6] = 0x10;  /* Program base address. Must be 0x0010 (the word following the Initialization Area). */
    preamb[7] = 0x00;

    /* 0x20 is the size of preamble, need to include the whole image size */
    u16 imgsz = num_instr * 4 + PREAMBLE_SIZE;
    preamb[8] = LOW_BYTE_16(imgsz);
    preamb[9] = HIGH_BYTE_16(imgsz);

    u16 v1 = imgsz + 8;
    preamb[0x0A] = LOW_BYTE_16(v1);
    preamb[0x0B] = HIGH_BYTE_16(v1);

    preamb[0x0C] = 0x18;
    preamb[0x0D] = 0x00;

    u16 v2 = imgsz + 12;
    preamb[0x0E] = LOW_BYTE_16(v2);
    preamb[0x0F] = HIGH_BYTE_16(v2);

    u16 v3 = imgsz - 16;
    preamb[0x10] = LOW_BYTE_16(v3);
    preamb[0x11] = HIGH_BYTE_16(v3);

    preamb[0x12] = 0x02;
    preamb[0x13] = 0x00;
    preamb[0x14] = 0x08;
    preamb[0x15] = 0x00;
    preamb[0x16] = 0x00;
    preamb[0x17] = 0x00;
    preamb[0x18] = 0x35; /* push0 */
    preamb[0x19] = 0x37; /*  */
    preamb[0x1A] = 0x04; /* push2n 4	Pushes 2^5, or 32 */
    preamb[0x1B] = 0x35; /* push0 */
    preamb[0x1C] = 0x2C; /* coginit */
    preamb[0x1D] = 0x00; /* padding */
    preamb[0x1E] = 0x00; /* padding */
    preamb[0x1F] = 0x00; /* padding */

    preamb[5] = compute_checksum(preamb, prog, num_instr);
}


/*****************************************************************\
*   Assembles some file input                                    *
\*****************************************************************/
void assemble(FILE* file)
{
    if(opt_verbose > 4)
        fprintf(vfile, "assembling %u instructions\n", num_ops);

    if(!opt_raw) /* writing out the propeller tool header */
    {
        u8 preamble[PREAMBLE_SIZE];
        create_preamble(preamble, (u8*)program, num_ops);

        size_t w = fwrite(preamble, 1, PREAMBLE_SIZE, file);
        if(w != PREAMBLE_SIZE)
            sys_error("error writing out preamble");
    }

    for(u16 i = 0; i < num_ops; i++)
    {
        u32 u = u32tole(program[i].raw);
        size_t w = fwrite(&u, 4, 1, file);
        if(w != 1)
            sys_error("error writing assembled program");
    }
}

/*****************************************************************\
*   Generates the listing.                                        *
\*****************************************************************/
void generate_listing(FILE* file, size_t num_ops)
{
    if(!num_ops)
        num_ops = count_instructions();

    for(unsigned i = 0; i < num_ops; i++)
    {
        u8 j = find_if_by_value(program[i].data.cond);
        u8 o = find_opcode_by_value(program[i].data.opcode);

        assert(j != NUM_IFS);

        u16 dest =  ((u16)(program[i].data.desth)) << 8 | program[i].data.dest;
        u16 src =  ((u16)(program[i].data.srch)) << 8 | program[i].data.src;

        fprintf(file, "%04X %08X if_%s %s $%x, $%x zcri:%u%u%u%u\n", i, program[i].raw, if_pairs[j].string,
                opcodes[o].string, dest, src,
                program[i].data.z, program[i].data.c, program[i].data.r, program[i].data.imm);
    }
}
