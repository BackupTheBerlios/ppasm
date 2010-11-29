#include "expression.h"

VECTOR_DECLARE(veclable, pair_t);
veclable symtable;

expression_t*   unresolved_src[MAX_INSTRUCTIONS];
expression_t*   unresolved_dest[MAX_INSTRUCTIONS];

void init_symtable()
{
    veclable_init(&symtable, 10);
}

void fini_symtable()
{
    for(size_t i = 0; i < symtable.size; i++)
        free((char*)symtable.element[i].string);

    veclable_fini(&symtable);
}

/*****************************************************************\
*   Finds a label with the name of @param label_name in symtable. *
\*****************************************************************/
size_t find_addr_label(const char* label_name)
{
    size_t i = 0;
    for(; i < symtable.size; i++)
    {
        if(!strcmp(symtable.element[i].string, label_name))
            break;
    }
    return i;
}

/*****************************************************************\
*   Findes an address @param label                                *
\*****************************************************************/
void expression_clear(expression_t* exp)
{
    if(exp->type & EXP_LABEL)
    {
        if(exp->data.label)
            free(exp->data.label);
    }
    else if(exp->type & EXP_NUMBER)
    {
        exp->data.number = 0;
    }
    exp->type = 0;
}

/*****************************************************************\
*   Findes an address @param label                                *
\*****************************************************************/
const char* expression_evaluate(expression_t** exp, ulong* result)
{
    *result = 0;

    while(*exp)
    {
        if((*exp)->type & EXP_LABEL)
        {
            size_t lpos = find_addr_label((*exp)->data.label);
            if(lpos == symtable.size)
                return "cant' evaluate the expression";
            else /* converting label to its value and putting to the expression */
            {
                free((*exp)->data.label);
                (*exp)->type &= ~EXP_LABEL;
                (*exp)->type |= EXP_NUMBER;
                (*exp)->data.number = symtable.element[lpos].value;
                assert((*exp)->type & EXP_NUMBER);
            }
        }

        switch((*exp)->type & 0b00111111)
        {
            case '+': *result += (*exp)->data.number;
                break;

            case '-': *result -= (*exp)->data.number;
                break;

            case '/': *result /= (*exp)->data.number;
                break;

            case '*': *result *= (*exp)->data.number;
                break;

            default:
                return "unknown operator in an expression";
                break;
        }

        /* freeing current expression */
        expression_t* oldexp = *exp;
        *exp = (*exp)->next;
        free(oldexp);
    }

    return 0;
}

/*****************************************************************\
*   Substitute labels with their addresses @param label           *
\*****************************************************************/
void evaluate_all_unresolved()
{
    for(unsigned i = 0; i < MAX_INSTRUCTIONS; i++)
    {
        if(flags[i].valid)
        {
            ulong result;

            if(opt_verbose > 4)
                fprintf(vfile, "%s: op: %u\n", __FUNCTION__, i);

            if(flags[i].raw_command)
            {
                const char* errmsg = expression_evaluate(&unresolved_src[i], &result);
                if(errmsg)
                    fatal("error resolving src expression: %s", errmsg);

                if(opt_verbose > 4)
                    fprintf(vfile, "\traw: %lu\n", result);

                switch(flags[i].raw_command)
                {
                    case 1:
                        program[i].byte[0] = result & 0xFF;
                        break;

                    case 2:
                        program[i].byte[1] = result & 0xFF;
                        break;

                    case 3:
                        program[i].byte[2] = result & 0xFF;
                        break;

                    case 4:
                        program[i].byte[3] = result & 0xFF;
                        break;

                    case 5: /* low word */
                        program[i].byte[1] = (result >> 8) & 0xFF;
                        program[i].byte[0] = result & 0xFF;
                        break;

                    case 6: /* high word */
                        program[i].byte[3] = (result >> 8) & 0xFF;
                        program[i].byte[2] = result & 0xFF;
                        break;

                    case 7:
                        program[i].raw = result;
                }

                continue;
            }

            if(unresolved_dest[i])
            {
                const char* errmsg = expression_evaluate(&unresolved_dest[i], &result);
                if(errmsg)
                    fatal("error resolving dest expression: %s", errmsg);

                program[i].data.dest = LOW_BYTE_16(result);
                program[i].data.desth = HIGH_BYTE_16(result);

                if(opt_verbose > 4)
                    fprintf(vfile, "\t resolved dest: %lu\n", result);
            }

            if(unresolved_src[i])
            {
                const char* errmsg = expression_evaluate(&unresolved_src[i], &result);
                if(errmsg)
                    fatal("error resolving src expression: %s", errmsg);
                program[i].data.src = LOW_BYTE_16(result);
                program[i].data.srch = HIGH_BYTE_16(result);

                if(opt_verbose > 4)
                    fprintf(vfile, "\t resolved src: %lu\n", result);
            }
        }
    }
}


