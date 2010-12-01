#include "opcodes.h"

pair_t if_pairs[] = {
{ "always", IF_ALWAYS },
{ "never", IF_NEVER },
{ "e", IF_E },
{ "ne", IF_NE },
{ "a", IF_A },
{ "b", IF_B },
{ "ae", IF_AE },
{ "be", IF_BE },
{ "c", IF_C },
{ "nc", IF_NC },
{ "z", IF_Z },
{ "nz", IF_NZ },
{ "c_eq_z", IF_C_EQ_Z },
{ "c_ne_z", IF_C_NE_Z },
{ "c_and_z", IF_C_AND_Z },
{ "c_and_nz", IF_C_AND_NZ },
{ "nc_and_z", IF_NC_AND_Z },
{ "nc_and_nz", IF_NC_AND_NZ },
{ "c_or_z", IF_C_OR_Z  },
{ "c_or_nz", IF_C_OR_NZ },
{ "nc_or_z", IF_NC_OR_Z },
{ "nc_or_nz", IF_NC_OR_NZ },
{ "z_eq_c", IF_Z_EQ_C },
{ "z_ne_c", IF_Z_NE_C },
{ "z_and_c", IF_Z_AND_C },
{ "z_and_nc", IF_Z_AND_NC },
{ "nz_and_c", IF_NZ_AND_C },
{ "nz_and_nc", IF_NZ_AND_NC },
{ "z_or_c", IF_Z_OR_C },
{ "z_or_nc", IF_Z_OR_NC },
{ "nz_or_c", IF_NZ_OR_C },
{ "nz_or_nc", IF_NZ_OR_NC}
};

op_pair_t opcodes[] = {
/* opcode,  OP_, pdef_src,{nd?ns?ss? z  c  r  i} */
{ "abs",    OP_ABS,     0, {1, 1, 0, 0, 0, 1, 0} },
{ "absneg", OP_ABSNEG,  0, {1, 1, 0, 0, 0, 1, 0} },
{ "add",    OP_ADD,     0, {1, 1, 0, 0, 0, 1, 0} },
{ "addabs", OP_ADDABS,  0, {1, 1, 0, 0, 0, 1, 0} },
{ "adds",   OP_ADDS,    0, {1, 1, 0, 0, 0, 1, 0} },
{ "addsx",  OP_ADDSX,   0, {1, 1, 0, 0, 0, 1, 0} },
{ "addx",   OP_ADDX,    0, {1, 1, 0, 0, 0, 1, 0} },
{ "and",    OP_AND,     0, {1, 1, 0, 0, 0, 1, 0} },
{ "andn",   OP_ANDN,    0, {1, 1, 0, 0, 0, 1, 0} },
{ "call",   OP_CALL,    0, {0, 1, 0, 0, 0, 1, 1} },
{ "clkset", OP_CLKSET,  0, {1, 0, 1, 0, 0, 0, 1} },
{ "cmp",    OP_CMP,     0, {1, 1, 0, 0, 0, 0, 0} },
{ "cmps",   OP_CMPS,    0, {1, 1, 0, 0, 0, 0, 0} },
{ "cmpsub", OP_CMPSUB,  0, {1, 1, 0, 0, 0, 1, 0} },
{ "cmpsx",  OP_CMPSX,   0, {1, 1, 0, 0, 0, 0, 0} },
{ "cmpx",   OP_CMPX,    0, {1, 1, 0, 0, 0, 0, 0} },
{ "cogid",  OP_COGID,   1, {1, 0, 1, 0, 0, 1, 1} },
{ "coginit",OP_COGINIT, 2, {1, 0, 1, 0, 0, 0, 1} },
{ "cogstop",OP_COGSTOP, 3, {1, 0, 1, 0, 0, 0, 1} },
{ "djnz",   OP_DJNZ,    0, {1, 1, 0, 0, 0, 1, 0} },
{ "hubop",  OP_HUBOP,   0, {1, 1, 0, 0, 0, 0, 0} },
{ "jmp",    OP_JMP,     0, {0, 1, 0, 0, 0, 0, 0} },
{ "jmpret", OP_JMPRET,  0, {1, 1, 0, 0, 0, 1, 0} },
{ "lockclr",OP_LOCK,    7, {1, 0, 1, 0, 0, 0, 1} },
{ "locknew",OP_LOCK,    4, {1, 0, 1, 0, 0, 1, 1} },
{ "lockret",OP_LOCK,    5, {1, 0, 1, 0, 0, 0, 1} },
{ "lockset",OP_LOCK,    6, {1, 0, 1, 0, 0, 0, 1} },
{ "max",    OP_MAX,     0, {1, 1, 0, 0, 0, 1, 0} },
{ "maxs",   OP_MAXS,    0, {1, 1, 0, 0, 0, 1, 0} },
{ "min",    OP_MIN,     0, {1, 1, 0, 0, 0, 1, 0} },
{ "mins",   OP_MINS,    0, {1, 1, 0, 0, 0, 1, 0} },
{ "mov",    OP_MOV,     0, {1, 1, 0, 0, 0, 1, 0} },
{ "movd",   OP_MOVD,    0, {1, 1, 0, 0, 0, 1, 0} },
{ "movi",   OP_MOVI,    0, {1, 1, 0, 0, 0, 1, 0} },
{ "movs",   OP_MOVS,    0, {1, 1, 0, 0, 0, 1, 0} },
{ "muxc",   OP_MUXC,    0, {1, 1, 0, 0, 0, 1, 0} },
{ "muxnc",  OP_MUXNC,   0, {1, 1, 0, 0, 0, 1, 0} },
{ "muxnz",  OP_MUXNZ,   0, {1, 1, 0, 0, 0, 1, 0} },
{ "muxz",   OP_MUXZ,    0, {1, 1, 0, 0, 0, 1, 0} },
{ "neg",    OP_NEG,     0, {1, 1, 0, 0, 0, 1, 0} },
{ "negc",   OP_NEGC,    0, {1, 1, 0, 0, 0, 1, 0} },
{ "negnc",  OP_NEGNC,   0, {1, 1, 0, 0, 0, 1, 0} },
{ "negnz",  OP_NEGNZ,   0, {1, 1, 0, 0, 0, 1, 0} },
{ "negz",   OP_NEGZ,    0, {1, 1, 0, 0, 0, 1, 0} },
{ "or",     OP_OR,      0, {1, 1, 0, 0, 0, 1, 0} },
{ "rcl",    OP_RCL,     0, {1, 1, 0, 0, 0, 1, 0} },
{ "rcr",    OP_RCR,     0, {1, 1, 0, 0, 0, 1, 0} },
{ "rdbyte", OP_RDBYTE,  0, {1, 1, 0, 0, 0, 1, 0} },
{ "rdlong", OP_RDLONG,  0, {1, 1, 0, 0, 0, 1, 0} },
{ "rdword", OP_RDWORD,  0, {1, 1, 0, 0, 0, 1, 0} },
{ "ret",    OP_RET,     0, {0, 0, 0, 0, 0, 0, 1} },
{ "rev",    OP_REV,     0, {1, 1, 0, 0, 0, 1, 0} },
{ "rol",    OP_ROL,     0, {1, 1, 0, 0, 0, 1, 0} },
{ "ror",    OP_ROR,     0, {1, 1, 0, 0, 0, 1, 0} },
{ "sar",    OP_SAR,     0, {1, 1, 0, 0, 0, 1, 0} },
{ "shl",    OP_SHL,     0, {1, 1, 0, 0, 0, 1, 0} },
{ "shr",    OP_SHR,     0, {1, 1, 0, 0, 0, 1, 0} },
{ "sub",    OP_SUB,     0, {1, 1, 0, 0, 0, 1, 0} },
{ "subabs", OP_SUBABS,  0, {1, 1, 0, 0, 0, 1, 0} },
{ "subs",   OP_SUBS,    0, {1, 1, 0, 0, 0, 1, 0} },
{ "subsx",  OP_SUBSX,   0, {1, 1, 0, 0, 0, 1, 0} },
{ "subx",   OP_SUBX,    0, {1, 1, 0, 0, 0, 1, 0} },
{ "sumc",   OP_SUMC,    0, {1, 1, 0, 0, 0, 1, 0} },
{ "sumnc",  OP_SUMNC,   0, {1, 1, 0, 0, 0, 1, 0} },
{ "sumnz",  OP_SUMNZ,   0, {1, 1, 0, 0, 0, 1, 0} },
{ "sumz",   OP_SUMZ,    0, {1, 1, 0, 0, 0, 1, 0} },
{ "test",   OP_TEST,    0, {1, 1, 0, 0, 0, 0, 0} },
{ "testn",  OP_TESTN,   0, {1, 1, 0, 0, 0, 0, 0} },
{ "tjnz",   OP_TJNZ,    0, {1, 1, 0, 0, 0, 0, 0} },
{ "tjz",    OP_TJZ,     0, {1, 1, 0, 0, 0, 0, 0} },
{ "waitcnt",OP_WAITCNT, 0, {1, 1, 0, 0, 0, 1, 0} },
{ "waitpeq",OP_WAITPEQ, 0, {1, 1, 0, 0, 0, 0, 0} },
{ "waitpne",OP_WAITPNE, 0, {1, 1, 0, 0, 0, 0, 0} },
{ "waitvid",OP_WAITVID, 0, {1, 1, 0, 0, 0, 0, 0} },
{ "wrbyte", OP_WRBYTE,  0, {1, 1, 0, 0, 0, 0, 0} },
{ "wrlong", OP_WRLONG,  0, {1, 1, 0, 0, 0, 0, 0} },
{ "wrword", OP_WRWORD,  0, {1, 1, 0, 0, 0, 0, 0} },
{ "xor",    OP_XOR,     0, {1, 1, 0, 0, 0, 1, 0} },
{ 0,    0,     0, {0, 0, 0, 0, 0, 0, 0} }

};

pair_t special_regs[] = {
{ "par", 0x1F0 },
{ "cnt", 0x1F1 },
{ "ina", 0x1F2 },
{ "inb", 0x1F3 },
{ "outa", 0x1F4 },
{ "outb", 0x1F5 },
{ "dira", 0x1F6 },
{ "dirb", 0x1F7 },
{ "ctra", 0x1F8 },
{ "ctrb", 0x1F9 },
{ "frqa", 0x1FA },
{ "frqb", 0x1FB },
{ "phsa", 0x1FC },
{ "phsb", 0x1FD },
{ "vcfg", 0x1FE },
{ "vcsl", 0x1FF }
};

/*****************************************************************\
*                                                                 *
*   Finds index of an if pair by it's @param value.               *
*                                                                 *
\*****************************************************************/
u8 find_if_by_value(u8 value)
{
    u8 j = 0;
    for(; j < NUM_IFS; j++)
    {
        if(if_pairs[j].value == value)
        {
            break;
        }
    }
    return j;
}

/*****************************************************************\
*                                                                 *
*   Finds index of an opcode by it's @param value.                *
*                                                                 *
\*****************************************************************/
u8 find_opcode_by_value(u8 value)
{
    u8 j = 0;
    for(; j < NUM_OPCODES; j++)
    {
        if(opcodes[j].opcode == value)
        {
            break;
        }
    }
    return j;
}
