#include "assemble.h"
#include "util.h"
#include "opcodes.h"
#include "assert.h"

/*
Many thanks to cliff biffle for reverse engineering propeller tool binary format.
http://www.cliff.biffle.org/software/propeller/binary-format.html
*/
u8 preamble[] = {
/*0x00*/ 0x00, 0xB4, 0xC4, 04, /* Initial clock frequency, in Hz, hier ist 80Mhz little endian*/
/*0x04*/ 0x6F, /* Initial CLK register contents (see manual p.28) */
/*0x05*/ 0x00,  /* crc */
/*0x06*/ 0x10, 0x00, /* 	Program base address. Must be 0x0010 (the word following the Initialization Area). */
/*0x08*/ 0x00, 0x00, /* length of image in bytes (1) */
/*0x0A*/ 0x00, 0x00, /* (1) + 8 */
/*0x0C*/ 0x18, 0x00,
/*0x0E*/ 0x00, 0x00, /* (1) + 12 */
/*0x10*/ 0x00, 0x00, /* (1) - 16 */
/*0x12*/ 0x02, /* This seems to be an offset, in long words from 0x0010, to the start of the outermost SPIN object's data/code. */
/*0x13*/ 0x00, /* Number of sub-objects. For our purposes, 0x00. */
/*0x14*/ 0x08, 0x00,
/*0x16*/ 0x00, 0x00,
/*0x18*/ 0x35, /* push0 */
/*0x19*/ 0x37, 0x04, /* push2n 4	Pushes 2^5, or 32 */
/*0x1B*/ 0x35, /* push0 */
/*0x1C*/ 0x2C, /* coginit */
/*0x1D*/ 0x00, 0x00, 0x00 /* padding for word alignment */
};

u32 clkfreq = 0;


/*****************************************************************\
*   Computes checksum                                             *
\*****************************************************************/
u8 compute_checksum(u16 num_ops)
{
    u8  sum = 0;
    u8* byteptr = (u8*)program;

    for(unsigned i = 0; i < (num_ops * sizeof(instruction_t)); i++)
        sum += byteptr[i];

    for(unsigned i = 0; i < PREAMBLE_SIZE; i++)
        sum += preamble[i];

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

/*****************************************************************\
*   Assembles some file input                                    *
\*****************************************************************/
void assemble(FILE* file)
{
    if(opt_verbose > 4)
        fprintf(vfile, "assembling %u instructions\n", num_ops);

    /* writing out the propeller tool header */
    if(!opt_raw)
    {
        if(clkfreq)
        {
            preamble[0] = clkfreq & 0xFF;
            preamble[1] = (clkfreq >> 8) & 0xFF;
            preamble[2] = (clkfreq >> 16) & 0xFF;
            preamble[3] = (clkfreq >> 24) & 0xFF;
        }

        u16 imgsz = num_ops * 4 + PREAMBLE_SIZE;/* 0x20 is the size of preamble, need to include the whole image size */
        preamble[8] = LOW_BYTE_16(imgsz);
        preamble[9] = HIGH_BYTE_16(imgsz);

        u16 v1 = imgsz + 8;
        preamble[0x0A] = LOW_BYTE_16(v1);
        preamble[0x0B] = HIGH_BYTE_16(v1);

        u16 v2 = imgsz + 12;
        preamble[0x0E] = LOW_BYTE_16(v2);
        preamble[0x0F] = HIGH_BYTE_16(v2);

        u16 v3 = imgsz - 16;
        preamble[0x10] = LOW_BYTE_16(v3);
        preamble[0x11] = HIGH_BYTE_16(v3);

        preamble[5] = compute_checksum(num_ops);
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
