#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include "config.h"

typedef unsigned long ulong;

#if defined(_MSC_VER)
typedef unsigned __int64 u64;
typedef signed   __int64 s64;
typedef unsigned __int32 u32;
typedef          __int32 s32;
typedef unsigned __int16 u16;
typedef          __int16 s16;
typedef unsigned __int8 u8;
typedef          __int8 s8;
#else
/* __GNUC__ and most others */
#include <stdint.h>
#include <limits.h>
typedef	uint8_t 	u8;
typedef	int8_t 		s8;
typedef	uint16_t	u16;
typedef int16_t		s16;
typedef uint32_t	u32;
typedef int32_t		s32;
typedef int64_t		s64;
typedef uint64_t	u64;
#endif

/*
#if (__SIZEOF_POINTER__ == 8)
#pragma pack(8)
#else
#pragma pack(4)
#endif
*/
#pragma pack(4)
typedef union
{
    #if defined(PPASM_LITTLE_ENDIAN)
    struct
    {
        u32 src: 8;
        u32 srch: 1;
        u32 dest : 8;
        u32 desth : 1;
        u32 cond: 4;
        u32 imm: 1;
        u32 r: 1;
        u32 c: 1;
        u32 z: 1;
        u32 opcode:6;
    } data;
    #elif defined(PPASM_BIG_ENDIAN)
    struct
    {
        u32 opcode:6;
        u32 z: 1;
        u32 c: 1;
        u32 r: 1;
        u32 imm: 1;
        u32 cond: 4;
        u32 desth : 1;
        u32 dest : 8;
        u32 srch: 1;
        u32 src: 8;
    } data;
    #endif
    u32 raw; /* u32 access to instruction */
    u8 byte[4]; /* byte access to instruction */
} instruction_t;

#if (__SIZEOF_POINTER__ == 8)
#pragma pack(8)
#else
#pragma pack(4)
#endif

typedef struct
{
    const char* after_comment;
    const char* multi_comment_begin;
    const char* multi_comment_end;
    unsigned    multi_comment_begin_len;
    unsigned    after_comment_len;
    unsigned    multi_comment_end_len;
    char        immediate_prefix;
    char        local_label_prefix;
} syntax_t;

typedef struct
{
    const char* string;
    ulong value;
} pair_t;

#pragma pack()

typedef struct
{
    u8  valid   : 1; /* 1 if this is a valid operation */
    u8  raw_command  : 3; /* 0: not a raw command, 1-4 raw byte, 5,6 row high/low word, 7 raw long */
    u8  reserved : 4;
} flags_t;

#define HIGH_BYTE_16(x) (u8)((x >> 8) & 0xff)
#define LOW_BYTE_16(x)   (u8)(x & 0xff)

#if defined(PPASM_LITTLE_ENDIAN)
#define u32tole(x) (x)
#elif defined(PPASM_BIG_ENDIAN)
#define u32tole(x) (x >> 24 | x << 24 | (x & 0x00ff0000) >> 8 | (x & 0x0000ff00) << 8)
#endif

#include <stdio.h>
extern u8 opt_verbose;  /* verbosity of the output for debugging */
extern u8 opt_raw;      /* dont generate/take into account propeller tool header */
extern u8 opt_listing;
extern u16 num_ops;
extern FILE* vfile;
extern instruction_t    program[MAX_INSTRUCTIONS]; /* current program */
extern flags_t          flags[MAX_INSTRUCTIONS];
extern const syntax_t*  syntax;
#endif // TYPES_H_INCLUDED
