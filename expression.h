#ifndef EXPRESSION_H_INCLUDED
#define EXPRESSION_H_INCLUDED
#include "types.h"
#include "util.h"
#include "containers.h"
#include "string.h"

#define EXP_LABEL (1L<<6)
#define EXP_NUMBER (1L<<7)
/* ascii ops are 0x20 - 0x2F, (1L<<5) == 0x20 */

#if (__SIZEOF_POINTER__ == 8)
#pragma pack(8)
#else
#pragma pack(4)
#endif

typedef struct exp_struct expression_t;
struct exp_struct
{
    union
    {
        char*   label;
        ulong   number;
    } data;

    expression_t*       next;

    u32                 type;
};
#pragma pack()
extern expression_t*   unresolved_src[MAX_INSTRUCTIONS];
extern expression_t*   unresolved_dest[MAX_INSTRUCTIONS];

void init_symtable();
void fini_symtable();
size_t find_addr_label(const char* label_name);
void expression_clear(expression_t* exp);
const char* expression_evaluate(expression_t** exp, ulong* result);
void evaluate_all_unresolved();
#endif // EXPRESSION_H_INCLUDED
