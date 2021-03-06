#include "parse.h"
#include "opcodes.h"
#include "assemble.h"
#include "expression.h"
#include "stringext.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <strings.h>

VECTOR_DECLARE(veclable, pair_t);
extern veclable symtable;

instruction_t   program[MAX_INSTRUCTIONS];
flags_t         flags[MAX_INSTRUCTIONS];
u16 must_fit_in = MAX_INSTRUCTIONS; /* FIT directive argument */


static size_t       curr_op = 0, line_num = 0;
static const char*  last_label = 0;
static char*        token = 0;

DECLARE_FIND(op_pair_t);
DECLARE_FIND(pair_t);

/*****************************************************************\
*                                                                 *
*   Returns TRUE if @param token is a valid label.                *
*   TODO: make it return error description                        *
*                                                                 *
\*****************************************************************/
int is_valid_label(const char* str)
{
    if(str)
    {
        if(*str == syntax->local_label_prefix) /* skipping local lable modifier */
            str++;

        op_pair_t v;
        v.string = str;
        size_t i = op_pair_t_find(&v, opcodes, NUM_OPCODES, &op_pair_t_compare_string);
        if(i != NUM_OPCODES)
            return 0;

        pair_t p;
        p.string = str;
        i = pair_t_find(&p, if_pairs, NUM_IFS, &pair_t_compare_string);
        if(i != NUM_IFS)
            return 0;

        i = pair_t_find(&p, special_regs, NUM_SPECIAL_REGS, &pair_t_compare_string);
        if(i != NUM_SPECIAL_REGS)
            return 0;

        size_t strsz = strlen(str);
        if(isalpha(*str))
        {
            for(size_t i = 1; i < strsz; i++)
            {
                if(!isalnum(str[i]) && str[i] != '_' )
                {
                    return 0;
                }
            }
            return 1;
        }
    }
    return 0;
}

/*****************************************************************\
* @return error message or 0 if everything is ok.                 *
\*****************************************************************/
static const char* parse_num(expression_t** exp)
{
    if(!token)
        return "no number found";

    ulong num;
    const char* errmsg = string_to_number(token, &num);
    if(errmsg)
        return errmsg;

    *exp = malloc(sizeof(expression_t));
    (*exp)->type = EXP_NUMBER;
    (*exp)->data.number = num;

    if(opt_verbose > 4)
        fprintf(vfile, "\t\tnumber %lu\n", num);

    token = read_next();
    return 0;
}


/*****************************************************************\
*                                                                 *
*   Parses reference label.                                       *
* @return error message or 0 if everything is ok.                 *
*                                                                 *
\*****************************************************************/
static const char* parse_ref_label(expression_t** exp)
{
    if(!token || *token == 0)
        return "empty label";

    if(is_valid_label(token))
    {
        if(is_local_label(token))
        {
            if(!last_label)
                return "trying to reference a local lable without global one";

            size_t labelsz = strlen(last_label) + strlen(token);
            char tmp_name[labelsz + 1];
            strcpy(tmp_name, last_label);
            strcat(tmp_name, token);

            *exp = malloc(sizeof(expression_t));
            (*exp)->type = EXP_LABEL;
            (*exp)->data.label = strdup(tmp_name);
        }
        else
        {
            *exp = malloc(sizeof(expression_t));
            (*exp)->type = EXP_LABEL;
            (*exp)->data.label = strdup(token);
        }

        if(opt_verbose > 4)
            fprintf(vfile, "\t\tref label \"%s\"\n", (*exp)->data.label);

        token = read_next();
        return 0;
    }

    return "error processing reference label";
}

/***********************************************************************\
*                                                                       *
*   Parses a special register @param bad is the lowest register allowed *
*                                                                       *
\************************************************************************/
static const char* parse_special(expression_t** exp, unsigned bad)
{
    size_t tokensz = strlen(token);
    char lcas_token[tokensz + 1];
    lower_case(token, lcas_token, tokensz);

    pair_t p;
    p.string = lcas_token;
    size_t i = pair_t_find(&p, special_regs, NUM_SPECIAL_REGS, &pair_t_compare_string);
    if(i != NUM_SPECIAL_REGS)
    {
        if(i > bad)
        {
            if(opt_verbose > 4)
                fprintf(vfile, "\t\tspecial dest register\"%s\"\n", token);

            *exp = malloc(sizeof(expression_t));
            (*exp)->type = EXP_NUMBER;
            (*exp)->data.number = special_regs[i].value;

            token = read_next();
            return 0;
        }
        else
            fatal("error: line: %lu, this register is read only(can only be a source)", line_num);
    }
    return "not a special register";
}


/**************************************************************************************************\
*                                                                                                  *
*   Parses an expression or a number of them, returning the pointer to the first one in @param exp *
*                                                                                                  *
\**************************************************************************************************/
static const char* parse_expression(expression_t** exp)
{
    if(!token || *token == 0)
        return "error parsing expression";

    if(parse_num(exp))
    {
        if(parse_special(exp, 0))
        {
            if(parse_ref_label(exp))
            {
                return "error parsing expression";
            }
        }
    }

    (*exp)->type |= '+'; /* default operator */
    expression_t** e = &((*exp)->next);

    while(token && *token != 0 && is_valid_operator(*token))
    {
        u8 op = *token;
        token = read_next();

        if(parse_num(e))
        {
            if(parse_special(exp, 0))
            {
                if(parse_ref_label(e))
                {
                    /* failed to parse anything, but no matter we have one expression already */
                    return 0;
                }
            }
        }

        (*e)->type |= op;
        e = &((*e)->next);
    }

    return 0;
}


/*****************************************************************\
*   Parses a label @param label                                   *
\*****************************************************************/
static const char* parse_addr_label()
{
    if(is_valid_label(token))
    {
        size_t lpos = 0;

        if(is_local_label(token))
        {
            if(!last_label)
                return "trying to create local lable without global one";

            size_t labelsz = strlen(last_label) + strlen(token);
            char tmp_name[labelsz + 1];
            strcpy(tmp_name, last_label);
            strcat(tmp_name, token);

            pair_t p;
            p.string = tmp_name;
            lpos = pair_t_find(&p, symtable.element, symtable.size, &pair_t_compare_string);
            if(lpos != symtable.size)
                fatal("label %s was already defined!", tmp_name);

            pair_t label = {strdup(tmp_name), curr_op};
            veclable_push_back(&symtable, label);
        }
        else
        {
            pair_t p;
            p.string = token;
            lpos = pair_t_find(&p, symtable.element, symtable.size, &pair_t_compare_string);
            if(lpos != symtable.size)
                fatal("label %s was already defined!", token);

            pair_t label = {strdup(token), curr_op};
            last_label = label.string;
            veclable_push_back(&symtable, label);
        }

        if(opt_verbose > 4)
            fprintf(vfile, "adding label \"%s\" address %lu line %lu to symbol table\n", symtable.element[symtable.size - 1].string, curr_op, line_num);
   }
    else
        return "invalid label";

    token = read_next();

    if(token && (*token == '=' || !strcasecmp(token, "equ")))
    {
        token = read_next();
        if(!token)
            return "no equ/= argument!";

        ulong num;
        if(string_to_number(token, &num))
            return "error parsing after equ/=";

        symtable.element[symtable.size - 1].value = num;

        token = read_next();
    }


    return 0;
}


/*****************************************************************\
*   Parses flags, the token should be one of WZ WC WR NR          *
*   @return error message or 0 if everything is ok.               *
\*****************************************************************/
static const char* parse_flags()
{
    if(token && strlen(token) == 2 && !is_comment(token))
    {
        u8 c1 = tolower(token[0]);
        u8 c2 = tolower(token[1]);

        if(c1 == 'n' && c2 == 'r')
        {
            program[curr_op].data.z = 0;
            program[curr_op].data.c = 0;
            program[curr_op].data.r = 0;
        }
        else if(c1 == 'w')
        {
            switch(c2)
            {
                case 'z':
                    program[curr_op].data.z = 1;
                    break;

                case 'c':
                    program[curr_op].data.c = 1;
                    break;

                case 'r':
                    program[curr_op].data.r = 1;
                    break;

                default:
                    return "invalid w-flag";
            }
        }
        else
            return "invalid flags";

        if(opt_verbose > 4)
            fprintf(vfile, "\t\tread flag \"%c%c\"\n", c1, c2);
    }
    else
        return "invalid flag";

    token = read_next();
    return 0;

}

/*****************************************************************\
*                                                                 *
*   Parses if_* prefixes.                                         *
*   @return error message or 0 if everything is ok.               *
*                                                                 *
\*****************************************************************/
static const char* parse_ifs()
{
    size_t tokensz = strlen(token);
    char lcas_token[tokensz + 1];
    lower_case(token, lcas_token, tokensz);

    if(lcas_token[0] == 'i' && lcas_token[1] == 'f' && lcas_token[2] == '_')
    {
        pair_t p;
        p.string = &lcas_token[3] /* skip "if_" */;
        size_t i = pair_t_find(&p, if_pairs, NUM_IFS, &pair_t_compare_string);
        if(i != NUM_IFS)
        {
            program[curr_op].data.cond = if_pairs[i].value;

            if(opt_verbose > 4)
                fprintf(vfile, "\tprefix \"%s\"\n", token);

            token = read_next();
            return 0;
        }
    }

    /* default condition, only nop has 0b0000 by default
    which is overwritten by a special case anyway */
    program[curr_op].data.cond = 0b1111;
    return "unknown IF_ predicate";
}


/*****************************************************************\
*   Parses destination  .                                         *
*   @return error message or 0 if everything is ok.               *
\*****************************************************************/
static const char* parse_dest()
{
    if(!token || *token == 0)
        return "error processing instruction source";

    return parse_expression(&unresolved_dest[curr_op]);
}


/*****************************************************************\
*   Parses source operand.                                         *
*   @return error message or 0 if everything is ok.               *
\*****************************************************************/
static const char* parse_src()
{
    if(!token || *token == 0)
        return "error processing instruction source";

    if(*token == syntax->immediate_prefix) /* immediate? */
    {
        program[curr_op].data.imm = 1;
        token++; /* skip syntax->immediate_prefix */

        if(*token == 0)
        {
            token = read_next();
            if(!token || *token == 0)
                return "nothing after immediate symbol";
        }

        if(opt_verbose > 4)
            fprintf(vfile, "\t\timmediate\n");
    }

    return parse_expression(&unresolved_src[curr_op]);
}

u8 alignment;

/*****************************************************************\
*   Parses opcodes                                                *
*   @return error message or 0 if everything is ok.               *
\*****************************************************************/
static const char* parse_opcode()
{
    if(!token || is_comment(token))
        return 0;

    if(curr_op > must_fit_in)
        fatal("program doesn't fit in %u longs", must_fit_in);

    parse_ifs();

    if(!token)
        return "opcode is missing";

    size_t tokensz = strlen(token);
    char lcas_token[tokensz + 1];
    lower_case(token, lcas_token, tokensz);

    /* special case NOP handling */
    if(!strcmp(lcas_token, "nop"))
    {
        program[curr_op].raw = 0;
        token = read_next();
    }
    /* special case LONG handling */
    else if(!strcmp(lcas_token, "long"))
    {
        token = read_next();
        parse_expression(&unresolved_src[curr_op]);
        flags[curr_op].raw_command = 7;
    }
    else
    {
        op_pair_t v;
        v.string = lcas_token;
        size_t i = op_pair_t_find(&v, opcodes, NUM_OPCODES, &op_pair_t_compare_string);
        if(i == NUM_OPCODES)
            return "unknown opcode";

        if(opt_verbose > 4)
            fprintf(vfile, "\topcode \"%s\"\n", token);

        program[curr_op].data.opcode = opcodes[i].value;

        /* assigning default flags */
        program[curr_op].data.z = opcodes[i].flags.z;
        program[curr_op].data.c = opcodes[i].flags.c;
        program[curr_op].data.r = opcodes[i].flags.r;
        program[curr_op].data.imm = opcodes[i].flags.imm;

        /* check if we need special value in src register */
        if(opcodes[i].flags.predefined_src)
        {
            program[curr_op].data.src = opcodes[i].src;
            program[curr_op].data.srch = 0;
        }

        token = read_next();

        /* processing destination */
        if(opcodes[i].flags.need_dest)
        {
            const char* errmsg = parse_dest();
            if(errmsg)
                return errmsg;
        }

        /* processing source */
        if(opcodes[i].flags.need_src)
        {
            const char* errmsg = parse_src();
            if(errmsg)
                return errmsg;
        }

        while(!parse_flags()); /* parse all valid w* flags */
    }

    flags[curr_op].valid = 1; /* marking current instruction as valid */
    curr_op++;
    return 0;
}

/*****************************************************************\
*   Processes directives                                          *
*                                                                 *
\*****************************************************************/
static const char* parse_directives()
{
    size_t tokensz = strlen(token);
    char lcas_token[tokensz + 1];
    lower_case(token, lcas_token, tokensz);

    if(!strcmp(lcas_token, "fit"))
    {
        if(opt_verbose > 4)
            fprintf(vfile, "\tdirective FIT\n");

        token = read_next();

        ulong num;
        if(string_to_number(token, &num))
            fatal("line %lu: error parsing FIT argument", line_num);

        token = read_next();
        must_fit_in = num;
    }
    else if(!strcmp(lcas_token, "org"))
    {
        if(opt_verbose > 4)
            fprintf(vfile, "\tdirective ORG\n");

        token = read_next();

        ulong num;
        if(string_to_number(token, &num))
            fatal("error parsing ORG argument");

        curr_op = num;
        token = read_next();
    }
    else if(!strcmp(lcas_token, "res"))
    {
        if(opt_verbose > 4)
            fprintf(vfile, "\tdirective RES\n");

        token = read_next();

        ulong num;
        if(string_to_number(token, &num))
            fatal("error parsing RES argument");

        curr_op += num;
        token = read_next();
    }
    else if(!strcmp(token, "_CLKFREQ"))
    {
        if(opt_verbose > 4)
            fprintf(vfile, "\tdirective _CLKFREQ\n");

        token = read_next();

        ulong num;
        if(string_to_number(token, &num))
            fatal("error parsing _CLKFREQ argument");

        clkfreq = num;
        token = read_next();
    }
    else if(!token || is_comment(lcas_token))
    {

    }
    else
        return "unknown directive";

    return 0;
}

/*****************************************************************\
*   Parses the file @param filename                               *
\*****************************************************************/
void parse(FILE* file)
{
    memset(program, 0, sizeof(instruction_t) * MAX_INSTRUCTIONS); /* clear program space */
    memset(unresolved_src, 0, sizeof(expression_t*) * MAX_INSTRUCTIONS);
    memset(unresolved_dest, 0, sizeof(expression_t*) * MAX_INSTRUCTIONS);
    memset(flags, 0, sizeof(flags_t) * MAX_INSTRUCTIONS);

    init_symtable();

    curr_op = 0; /* reset current op */
    line_num = 0; /* reset line counter */

    const char* errmsg; /* error messages, returned by parse_* functions */
    int     comment_on = 0; /* 1 if there is a multiline comment, 0 othrewise */
    size_t  linesz; /* size of read line */

    while(!feof(file))
    {
        line_num++;

        char* line = read_line(file, &linesz, &comment_on);


        if(opt_verbose > 4)
            fprintf(vfile, "parsing line %lu: \"%s\" curr_op:%lu\n", line_num, line, curr_op);

        token = read_first(line, " ,\t\n\r", "+-/*=");

        while(token && !is_comment(token))
        {
            if(parse_directives()) /* is this a directory */
            {
                if(parse_opcode()) /* if this is not an opcode, it must be a label */
                {
                    if(errmsg = parse_addr_label())
                        fatal("line %u: failed to parse label \"%s\": %s", line_num, token, errmsg);
                    else
                    {
                        if(errmsg = parse_opcode())
                            fatal("line %u: failed to parse \"%s\": %s", line_num, token, errmsg);
                    }
                }
            }
        }
    }

    if(opt_verbose > 4)
        fprintf(vfile, "last instruction %lu\n", curr_op);

    evaluate_all_unresolved();

    /* TODO this is a temporary hack till I add Directive/Value pairs */
    pair_t p;
    p.string = "_CLKREG";
    size_t lpos = pair_t_find(&p, symtable.element, symtable.size, &pair_t_compare_string);
    if(lpos != symtable.size)
    {
        clkreg = symtable.element[lpos].value;
    }


    fini_symtable();
}
