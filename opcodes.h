#ifndef OPCODES_H_INCLUDED
#define OPCODES_H_INCLUDED
#include "types.h"

/* propeller conditional opcode prefixes */
#define IF_ALWAYS 0b1111
#define IF_NEVER 0b0000
#define IF_E 0b1010
#define IF_NE 0b0101
#define IF_A 0b0001
#define IF_B 0b1100
#define IF_AE 0b0011
#define IF_BE 0b1110
#define IF_C 0b1100
#define IF_NC 0b0011
#define IF_Z 0b1010
#define IF_NZ 0b0101
#define IF_C_EQ_Z 0b1001
#define IF_C_NE_Z 0b0110
#define IF_C_AND_Z 0b1000
#define IF_C_AND_NZ 0b0100
#define IF_NC_AND_Z 0b0010
#define IF_NC_AND_NZ 0b0001
#define IF_C_OR_Z 0b1110
#define IF_C_OR_NZ 0b1101
#define IF_NC_OR_Z 0b1011
#define IF_NC_OR_NZ 0b0111
#define IF_Z_EQ_C 0b1001
#define IF_Z_NE_C 0b0110
#define IF_Z_AND_C 0b1000
#define IF_Z_AND_NC 0b0010
#define IF_NZ_AND_C 0b0100
#define IF_NZ_AND_NC 0b0001
#define IF_Z_OR_C 0b1110
#define IF_Z_OR_NC 0b1011
#define IF_NZ_OR_C 0b1101
#define IF_NZ_OR_NC 0b0111
#define NUM_IFS 32

/* propeller opcodes */
#define OP_ABS 0b101010
#define OP_ABSNEG 0b101011
#define OP_ADD 0b100000
#define OP_ADDABS 0b100010
#define OP_ADDS 0b110100
#define OP_ADDSX 0b110110
#define OP_ADDX 0b110010
#define OP_AND 0b011000
#define OP_ANDN 0b011001
#define OP_CALL 0b010111
#define OP_CMP 0b100001
#define OP_CMPS 0b110000
#define OP_CMPSUB 0b111000
#define OP_CMPSX 0b110001
#define OP_CMPX 0b110011
#define OP_DJNZ 0b111001
#define OP_HUBOP 0b000011
#define OP_COGID OP_HUBOP
#define OP_COGINIT OP_HUBOP
#define OP_COGSTOP OP_HUBOP
#define OP_LOCK OP_HUBOP
#define OP_CLKSET OP_HUBOP
#define OP_JMP 0b010111
#define OP_JMPRET 0b010111
#define OP_MAX  0b010011
#define OP_MAXS  0b010001
#define OP_MIN  0b010010
#define OP_MINS  0b010000
#define OP_MOV 0b101000
#define OP_MOVD 0b010101
#define OP_MOVS 0b010100
#define OP_MOVI 0b010110
#define OP_MUXC  0b011100
#define OP_MUXNC  0b011101
#define OP_MUXNZ  0b011111
#define OP_MUXZ  0b011110
#define OP_NEG  0b101001
#define OP_NEGC  0b101100
#define OP_NEGNC  0b101101
#define OP_NEGNZ  0b101111
#define OP_NEGZ  0b101110
#define OP_OR  0b011010
#define OP_RCL  0b001101
#define OP_RCR  0b001100
#define OP_RDBYTE  0b000000
#define OP_RDLONG  0b000010
#define OP_RDWORD 0b000001
#define OP_RET  0b010111
#define OP_REV  0b001111
#define OP_ROL  0b001001
#define OP_ROR  0b001000
#define OP_SAR  0b001110
#define OP_SHL  0b001011
#define OP_SHR  0b001010
#define OP_SUB  0b100001
#define OP_SUBABS  0b100011
#define OP_SUBS  0b110101
#define OP_SUBSX  0b110111
#define OP_SUBX  0b110011
#define OP_SUMC  0b100100
#define OP_SUMNC 0b100101
#define OP_SUMNZ 0b100111
#define OP_SUMZ 0b100110
#define OP_TEST  0b011000
#define OP_TESTN  0b011001
#define OP_TJNZ  0b111010
#define OP_TJZ  0b111011
#define OP_WAITCNT  0b111110
#define OP_WAITPEQ  0b111100
#define OP_WAITPNE  0b111101
#define OP_WAITVID  0b111111
#define OP_WRBYTE  0b000000
#define OP_WRLONG  0b000010
#define OP_WRWORD  0b000001
#define OP_XOR  0b011011
#define NUM_OPCODES 78
#define NUM_SPECIAL_REGS 16
#define NUM_SPECIAL_SRCONLY 4
#if (__SIZEOF_POINTER__ == 8)
#pragma pack(8)
#else
#pragma pack(4)
#endif

/* this is an entry in a list of opcodes and their default/predefined operands/flags */
typedef struct
{
    const char* string;
    u8          opcode;
    u8          src;

    struct
    {
      u16 need_dest: 1; /* this flag makes parse_opcode try to read destination address */
      u16 need_src: 1; /* this flag makes parse_opcode try to read source address */
      u16 predefined_src: 1; /* this flag makes parse_opcode write predefined src into source */
      u16 z : 1; /* does z flag need to be set? */
      u16 c: 1; /* does c flag need to be set? */
      u16 r: 1; /* does r flag need to be set? */
      u16 imm: 1; /* does immediate flag need to be set? */
    } flags;
} op_pair_t;

u8 find_if_by_prefix(u8 prefix);
#pragma pack()

extern op_pair_t opcodes[]; /* table of opcodes */
extern pair_t if_pairs[];  /* table of if prefixes */
extern pair_t special_regs[];

#endif // OPCODES_H_INCLUDED
