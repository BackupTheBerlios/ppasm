#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED
#include "types.h"
#define MIN(a, b) a < b ? a : b;
#define MAX(a, b) a > b ? a : b;

void sys_error(const char* msg);
void fatal(const char* fmt, ...);
int is_comment(const char* str);
int is_valid_istruction(const instruction_t* instruction);
int is_valid_label(const char* label);
int is_local_label(const char* label);
int is_valid_operator(const char op);
void sleep_msec(ulong msec);
ulong get_time_ms();
#endif // UTIL_H_INCLUDED
