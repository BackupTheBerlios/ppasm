#ifndef STRINGEXT_H_INCLUDED
#define STRINGEXT_H_INCLUDED
#include "types.h"

void lower_case(const char* src, char* dst, size_t strsz);
void tolower_inplace(char* str, size_t strsz);
char* read_line(FILE* file, size_t* line_len, int* comment_on);
u32	decode_utf8(const char* msg, unsigned* i);
void strrm(char* dest, const char* src, char ch);
char* read_first(char* str, const char* d1, const char* d2);
char* read_next();
const char* string_to_number(const char* str, ulong* dest);
#endif // STRINGEXT_H_INCLUDED
