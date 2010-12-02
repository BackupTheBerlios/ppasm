#include "util.h"
#include "containers.h"
#include "expression.h"
#include "containers.h"
#include "stringext.h"
#include "loader.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#ifdef DO_TESTS
VECTOR_DECLARE(veclable, pair_t);
extern veclable symtable;

/*
    Tests the correct structure types
*/
static void test_types()
{
    assert(sizeof(instruction_t) == 4);
    assert(sizeof(flags_t) == 1);
    fprintf(stdout, "%s:\t\tpassed\n", __FUNCTION__);
}

/*
    Tests correct big/little-endianness
*/
static void test_endian()
{
    union testunion
    {
        u32 integer;
        u8 byte[4];
    } u;
    u.integer = 0xaabbccdd;
    u16 z = 0xFFEE;

    #ifdef PPASM_LITTLE_ENDIAN
    int c = 0;
    assert(u.byte[0] == 0xdd);
    assert(HIGH_BYTE_16(z) == 0xFF);
    assert(LOW_BYTE_16(z) == 0xEE);
    #endif

    #ifdef PPASM_BIG_ENDIAN
    int c = 0;
    assert(u.byte[0] == 0xaa);
    assert(u32tole(u.integer) == 0xcaffee00);
    assert(HIGH_BYTE_16(z) == 0xFF);
    assert(LOW_BYTE_16(z) == 0xEE);
    #endif
    c++;
     /* a test to see if some *ENDIAN was included */

    fprintf(stdout, "%s:\t\tpassed\n", __FUNCTION__);
}

/*
    Tests the read_line() function
*/
static void test_read_line()
{
    FILE* file = tmpfile();
    if(!file)
        sys_error("error opening test file!");

    const char* test1 = "\n\nli{XXX}ne1\n\rline2\nmulti-{\nXXX}line3";
    fwrite(test1, 1, strlen(test1), file);

    fseek(file, 0, SEEK_SET);

    size_t line_len;
    int comment_on = 0;

    char* line = read_line(file, &line_len, &comment_on);
    assert(!strcmp(line, "line1") && "should be \"line1\"");

    line = read_line(file, &line_len, &comment_on);
    assert(!strcmp(line, "line2") && "should be \"line2\"");

    line = read_line(file, &line_len, &comment_on);
    assert(!strcmp(line, "multi-") && "should be \"multi-\"");

    line = read_line(file, &line_len, &comment_on);
    assert(!strcmp(line, "line3") && "should be \"line3\"");

    fprintf(stdout, "%s:\t\tpassed\n", __FUNCTION__);
}

/*
    Tests the strrm() function
*/
static void test_strrm()
{
    char test1[] = {'_', '1', '0', '0', '_', '0', '0', '_', 0};
    strrm(test1, test1, '_');
    assert(!strcmp(test1, "10000"));
    fprintf(stdout, "%s:\t\tpassed\n", __FUNCTION__);
}

static void test_read_first_next()
{
    char teststr[] = "test1 test2\n\rtest3\t(test4+test5),test6;-test7";
    char* token = read_first(teststr, " \n\r\t,;", "()+-/*");

    assert(!strcmp(token, "test1"));
    token = read_next();

    assert(!strcmp(token, "test2"));
    token = read_next();

    assert(!strcmp(token, "test3"));
    token = read_next();

    assert(!strcmp(token, "("));
    token = read_next();

    assert(!strcmp(token, "test4"));
    token = read_next();

    assert(!strcmp(token, "+"));
    token = read_next();

    assert(!strcmp(token, "test5"));
    token = read_next();

    assert(!strcmp(token, ")"));
    token = read_next();

    assert(!strcmp(token, "test6"));
    token = read_next();

    assert(!strcmp(token, "-"));
    token = read_next();

    assert(!strcmp(token, "test7"));
    token = read_next();

    assert(token == 0);

    fprintf(stdout, "%s:\tpassed\n", __FUNCTION__);
}

typedef struct
{
    u8 a;
    u8 b;
}teststruct;

VECTOR_DECLARE(vecint, int);
VECTOR_DECLARE(vecstruct, teststruct);
vecstruct vs;

static void subtest1()
{
    vecstruct_init(&vs, 2);
    teststruct c1 = {1, 2}, c2 = {3, 4};
    vecstruct_push_back(&vs, c1);
    vecstruct_push_back(&vs, c2);
     //fprintf(stdout, "c1: %u %u, c2: %u %u", c1.a, c1.b, c2.a, c2.b);

}

static void test_conatiners()
{
    vecint v;
    vecint_init(&v, 2); // capacity 2, should at least reallocate once
    vecint_push_back(&v, 0);
    vecint_push_back(&v, 1);
    vecint_push_back(&v, 2);
    vecint_push_back(&v, 3);

    assert(vecint_pop(&v) == 3);
    assert(vecint_pop(&v) == 2);
    assert(vecint_pop(&v) == 1);
    assert(vecint_pop(&v) == 0);
    vecint_fini(&v);

    subtest1();

    teststruct c1, c2;
    c2 = vecstruct_pop(&vs);
    c1 = vecstruct_pop(&vs);
    assert(vecstruct_is_empty(&vs));
    //fprintf(stdout, "c1: a%u b%u, c2: a%u b%u\n", c1.a, c1.b, c2.a, c2.b);

    assert(c1.a == 1);
    assert(c1.b == 2);
    assert(c2.a == 3);
    assert(c2.b == 4);
    vecstruct_fini(&vs);
    fprintf(stdout, "%s:\tpassed\n", __FUNCTION__);
}

/*
    Tests expressions and their evaluation
*/
void test_expressions()
{
    expression_t* ex1 = malloc(sizeof(expression_t));
    expression_t* ex2 = malloc(sizeof(expression_t));
    expression_t* ex3 = malloc(sizeof(expression_t));
    expression_t* ex4 = malloc(sizeof(expression_t));

    ex1->next = ex2;
    ex2->next = ex3;
    ex3->next = ex4;


    /* trying successful evaluation first */

    ex1->type = EXP_NUMBER | '+';
    ex1->data.number = 14;

    ex2->type = EXP_NUMBER | '-';
    ex2->data.number = 4;

    ex3->type = EXP_NUMBER | '/';
    ex3->data.number = 2;

    ex4->type = EXP_NUMBER | '*';
    ex4->data.number = 3;

    ulong result;

    assert(!expression_evaluate(&ex1, &result));
    assert(result == 15);
    assert(ex1 == 0);

    /* trying unsuccessful evaluation */
    ex1 = malloc(sizeof(expression_t));
    ex2 = malloc(sizeof(expression_t));
    ex3 = malloc(sizeof(expression_t));
    ex4 = malloc(sizeof(expression_t));

    ex1->next = ex2;
    ex2->next = ex3;
    ex3->next = ex4;

    ex1->type = EXP_NUMBER | '+';
    ex1->data.number = 14;

    ex2->type = EXP_NUMBER | '-';
    ex2->data.number = 4;

    ex3->type = EXP_NUMBER | '/';
    ex3->data.number = 2;

    ex4->type = EXP_LABEL;
    ex4->data.label = "impossible to find label";

    assert(expression_evaluate(&ex1, &result));
    assert(result == 5);
    assert(ex1 == ex4);

    fprintf(stdout, "%s:\tpassed\n", __FUNCTION__);
}

void test_time()
{
    ulong t2, t1 = get_time_ms();

    sleep_msec(1234); /* don't remove 1234, cause it tests overflow of tv.tv_nsec */
    t2 = get_time_ms() - t1;

    assert(t2 >= 1234);
    fprintf(stdout, "%s:\tpassed\n", __FUNCTION__);
}

void test_loader()
{
    u8 b[11];
    u32 data = 0x5c7c0000;
    encode(b, data);
    assert(b[0] == 0x92 && b[1] == 0x92 && b[2] == 0x92 && b[3] == 0x92 &&
           b[4] == 0x92 && b[5] == 0x92 && b[6] == 0xdb && b[7] == 0x9b &&
           b[8] == 0xd2 && b[9] == 0x9b && b[10] == 0xf3);

    fprintf(stdout, "%s:\tpassed\n", __FUNCTION__);

}

/***************************************************\
*                                                   *
*   Main tester entry.                              *
*                                                   *
\***************************************************/
int main(int argc, char* argv[])
{
    test_types();
    test_endian();
    test_read_line();
    test_read_first_next();
    test_strrm();
    test_conatiners();
    test_expressions();
    test_time();
    test_loader();
    return 0;
}
#endif

