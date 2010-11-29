#include "stringext.h"
#include "util.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static char*   tmpline = 0;
static size_t tmplinesz = 0;

/***********************************************************************************************\
*   Reads @param file and returns a line.                                                       *
*   @param line_len is the address where to store the length of the line                        *
*   @param comment_on is a boolean flag that is non zero if the line contains multiline comment *
*   @return line or 0 if this was an empty file or contained only \n                            *
*
*   TODO: add syntax_t support
\***********************************************************************************************/
char* read_line(FILE* file, size_t* line_len, int* comment_on)
{
    size_t numchars = 0;

    int c;
    /* skip new lines in the beginning */
    while((c = fgetc(file)) == '\n');

    while(!feof(file))
    {
        /* do we have enough space? */
        if(tmplinesz <= numchars + 1) /* +1 to compensate for the last 0, <= ist for the first case where tmplinesz = 0*/
        {
            tmplinesz += 256;
            tmpline = realloc(tmpline, tmplinesz);
        }

        if(c == '\n')
            break;

        /* are we inside a multiline comment? */
        if(*comment_on)
        {
            if(c == '}')
                *comment_on = 0;
        }
        else /* not inside a comment */
        {
            if(c == '\r') {}
            else if(c == '{')
            {
                *comment_on = 1;
            }
            else
            {
                tmpline[numchars] = c;
                numchars++;
            }
        }
        c = fgetc(file);
    }
    tmpline[numchars] = 0; /* terminate the line */
    *line_len = numchars;

    return tmpline;
}

/*****************************************************************\
*   Findes an address @param label                                *
\*****************************************************************/
void tolower_inplace(char* str, size_t strsz)
{
    for(size_t i = 0; i < strsz; i++)
    {
        if(isalpha(str[i]))
        {
            str[i] = tolower(str[i]);
        }
    }
}

/*****************************************************************\
*   Returns a lowered copy of @param token allocated with malloc. *
\*****************************************************************/
void lower_case(const char* src, char* dest, size_t strsz)
{
     for(size_t i = 0; i < strsz; i++)
     {
          dest[i] = tolower(src[i]);
     }
     dest[strsz] = 0;
}

/*////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Decodes an utf8 char from a string. Increments @param i as it progresses.
//
// UTF-8 Encoding
//
// Binary code number      | Octet 1  | Octet 2  | Octet 3  | Octet 4
// 00000000 0xxxxxxx       | 0xxxxxxx |          |          |
// 00000yyy yyxxxxxx       | 110yyyyy | 10xxxxxx |          |
// zzzzyyyy yyxxxxxx       | 1110zzzz | 10yyyyyy | 10xxxxxx |
// uuuww zzzzyyyy yyxxxxxx | 11110uuu | 10wwzzzz | 10yyyyyy | 10xxxxxx
//
// TODO: Check for illegal byte sequences or create a function that does the checking.
//
///////////////////////////////////////////////////////////////////////////////////////////////////*/
u32	decode_utf8(const char* msg, unsigned* i)
{
	if(msg[*i] & 1 << 7)
	{
		assert(msg && "trying to decode an empty string!");

		u8 mod = 1 << 7;
		u8 cleaner = 0;
		u32 octets = 0, c = 0;

		// counting octets
		while((u8)msg[*i] & mod)
		{
			octets++;
			cleaner |= mod;
			mod >>= 1;
		}

		// potential bugs:
		assert(strlen(&msg[*i]) >= octets);

		// processing octets
		for(; octets > 0; octets--)
		{
			mod = (u32)msg[*i] ^ cleaner;
			u32 tmp = mod; // left of mod bytes there should be zeros!
			c = (c << 6) | tmp;
			cleaner = 1 << 7;
			(*i)++;
		}
		return c; // in case c is 0 so it was not a multi-byte char
	}
	else return msg[(*i)++];
}


/*****************************************************************\
* Removes @param ch from the string, squeezing it in the process. *
*   @param str expects a NULL-terminated c string.                *
\*****************************************************************/
void strrm(char* dest, const char* src, char ch)
{
    int j = 0;
    for(int i = 0; src[i]; i++)
    {
        dest[j] = src[i];

        if(dest[j] != ch)
        {
            j++;
        }
    }
    dest[j] = 0;
}

const char* char_in_str(const char ch, const char* str)
{
    for(;*str; str++)
    {
        if(*str == ch)
            return str;
    }
    return 0;
}

static const char*  delim1;
static const char*  delim2;
static char*        tmptok;
static char*        token = 0;
static size_t       tokensz = 82;

/*****************************************************************\
*   Read first                        *
\*****************************************************************/
char* read_first(char* str, const char* d1, const char* d2)
{
    token = malloc(tokensz);
    if(!token)
        fatal("out of memory");

    *token = 0;
    delim1 = d1;
    delim2 = d2;
    tmptok = strtok(str, delim1); /* read first */
    return read_next();
}


/*****************************************************************\
*   Read next                            *
\*****************************************************************/
char* read_next()
{
    if(!tmptok || *tmptok == 0)
        tmptok = strtok(NULL, delim1);

    static size_t toksz;

    if(tmptok && *tmptok)
    {
        /* if first char is one of delim2 */
        if(char_in_str(*tmptok, delim2))
        {
            toksz = strspn(tmptok, delim2);
        }
        else
        {
            toksz = strcspn(tmptok, delim2);
        }

        if(toksz != 0)
        {
            if(tokensz < toksz + 1)
            {
                tokensz = toksz + 1;
                token = realloc(token, tokensz);
                if(!token)
                    fatal("out of memory");
            }

            memcpy(token, tmptok, toksz);
            token[toksz] = 0;
            tmptok += toksz;
        }
        else
        {
            *tmptok = *token = 0;
        }
    }
    else
        return 0;

    return token;
}

/*****************************************************************\
* @return error message or 0 if everything is ok.                 *
\*****************************************************************/
const char* string_to_number(const char* str, ulong* dest)
{
    if(str)
    {
        char* wrongchar;
        int base = 0;

        if(*str == '$')
        {
            base = 16;
            str++; /* skipping $ */
        }
        else if(*str == '%')
        {
            base = 2;
            str++;/* skipping $ */
        }

        /* removing _'s */
        char copy[strlen(str) + 1];
        strrm(copy, str, '_');

        *dest = strtoll(copy, &wrongchar, base);
        if(*wrongchar) /* found any wrong character? */
            return "cant' parse this number";

        return 0;
    }
    else
        return "no number found";
}

