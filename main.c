#include "types.h"
#include "opcodes.h"
#include "parse.h"
#include "assemble.h"
#include "expression.h"
#include "loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

u8 opt_verbose = 0;
u8 opt_raw = 0;
u8 opt_listing = 0;
u8 opt_propcmd = 0xFF;
u16 num_ops = 0;
FILE* vfile;
const char serial_device[] = "/dev/ttyUSB0";

#define HELPMSG1 "This is a Propeller P8A32 assembler by Konstantin Schlese (c) 2010 nulleight@gmail.com\n\
version "
#define HELPMSG2 " compiled at "
#define HELPMSG3 "\nusage: ppasm [<options>...] <asmfile>\n\
options:\n\
        -r: raw output, no propeller tool bootloader\n\
        -l: generate listing file\n\
        -d: disassemble to stdout\n\
        -o <outfile>: specify the output file\n\
        -h: this help message\n\
        -v[0-9]: verbose level\n\
        -u[0-4]: download a program to propeller\n\
                 0 - get version and shutdown\n\
                 1 - download to ram and run\n\
                 2 - program eeprom and shutdown\n\
                 3 - program eeprom and run\n\
        -s <device>: serial port, where propeller is located"

#define QUOTE_X(t) #t
#define QUOTE(t)QUOTE_X(t)
#define DOT "."
#define SPACE " "
static const char helpmsg[] =
    HELPMSG1""QUOTE(VERSION_MAJOR)""DOT""QUOTE(VERSION_MINOR)""HELPMSG2""__DATE__""SPACE""__TIME__""HELPMSG3;

const syntax_t parallax_syntax =
{
    "'",
    "{",
    "}",
    1,
    1,
    1,
    '#',
    ':'
};

const syntax_t c_syntax =
{
    "//",
    "/*",
    "*/",
    2,
    2,
    2,
    '$',
    '.'
};

const syntax_t* syntax = &parallax_syntax; /* current syntax */

static const char* infile = NULL;
static const char* outfile = NULL;
static void (*action)();

/*****************************************************************\
*                                                                 *
*   This is the "assemble" action.                                *
*                                                                 *
\*****************************************************************/
void act_assemble()
{
    if(infile == NULL)
        fatal("error: input filename was not specified!");

    FILE* file = fopen(infile, "rb");
    if(!file)
        sys_error("error opening input file!");

    parse(file);
    num_ops = count_instructions();

    fclose(file);

    if(outfile == NULL)
        outfile = "out.binary";

    if(opt_listing)
    {
        size_t sz = strlen(outfile);
        char listfile[sz + 5];
        memcpy(listfile, outfile, sz + 1);
        strcat(listfile, ".lst");

        if(!(file = fopen(listfile, "wb")))
            sys_error("error opening listing file!");

        generate_listing(file, 0);
        fclose(file);
    }

    if(opt_propcmd != 0xFF)
    {
        prop_action(serial_device, opt_propcmd);
    }
    else
    {
        if(!(file = fopen(outfile, "wb")))
            sys_error("error opening output file!");

        assemble(file);

        fclose(file);
    }
}

/*****************************************************************\
*                                                                 *
*   This is a "disassemble" action                                *
*                                                                 *
\*****************************************************************/

void act_disassemble()
{
    if(infile == NULL)
        fatal("error: input filename was not specified!");

    FILE* file;
    if(!(file = fopen(infile, "rb")))
        sys_error("error opening input file!");

    if(!opt_raw)
        fseek(file, PREAMBLE_SIZE, SEEK_SET); /* 0x20 is the size of propeller tool preamble */

    size_t i = fread(program, 4, MAX_INSTRUCTIONS, file);
    generate_listing(stdout, i);
}

/*************************************************************************\
*                                                                         *
*   Main entry                                                            *
*                                                                         *
\*************************************************************************/
#ifndef DO_TESTS
int main(int argc, char* argv[])
{
    if(argc < 2)
        fatal(helpmsg);

    vfile = stdout;
    action = act_assemble;
    const char* serial_device = 0;

    for(int parmNum = 1; parmNum < argc; parmNum++)
    {
        if(argv[parmNum][0] != '-')
        {
            infile = argv[parmNum];
        }
        else
        {
            switch(argv[parmNum][1])
            {
                case 'd':
                    action = act_disassemble;
                    break;

                case 'r':
                    opt_raw = 1;
                    break;

                case 'l':
                    opt_listing = 1;
                    break;

                case 'u':
                    if(isdigit(argv[parmNum][2]) && argv[parmNum][2] - 0x30 < 4)
                        opt_propcmd = argv[parmNum][2] - 0x30;
                    else
                        fatal("unknown downloading command");
                    break;

                case 'h':
                    fatal(helpmsg);

                case 'v':
                    if(isdigit(argv[parmNum][2]))
                        opt_verbose = argv[parmNum][2] - 0x30;
                    else
                        fatal("-v option needs a number");
                    break;

                case 'o':
                    parmNum++;
                    if(parmNum < argc)
                        outfile = argv[parmNum];
                    else
                        fatal("error: no output specified with -o");
                    break;

                case 's':
                    parmNum++;
                    if(parmNum < argc)
                        serial_device = argv[parmNum];
                    else
                        fatal("error: no device specified with -s");
                    break;

                default:
                    fatal("error: unknown option %c", argv[parmNum][1]);
            }
        }
    }

    action();

    return 0;
}
#endif

