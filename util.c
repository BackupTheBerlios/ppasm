#include "util.h"
#include "opcodes.h"
#include "containers.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>

/**********************************************\
*                                              *
*    System error function, doesnt' return     *
*                                              *
\**********************************************/
void sys_error(const char* msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

/**********************************************\
*                                              *
*    Fatal error, doesn't return.              *
*                                              *
\**********************************************/
void fatal(const char* fmt, ...)
{
	if(fmt)
	{
        char errstr[MAX_ERROR_STRING_SIZE];

		va_list marker;
		va_start(marker, fmt);
		vsnprintf(errstr, MAX_ERROR_STRING_SIZE, fmt, marker);
		va_end(marker);
        fprintf(stderr, "%s\n", errstr);
    }
    else
        fprintf(stderr, "error: %s: fmt is NULL!\n", __FILE__);

	exit(EXIT_FAILURE);
}


/*****************************************************************\
*                                                                 *
*   Returns TRUE if @param token is a valid end of the line       *
*   comment                                                       *
*                                                                 *
\*****************************************************************/
int is_comment(const char* token)
{
    return  strlen(token) >= syntax->after_comment_len &&
            !strncmp(token, syntax->after_comment, syntax->after_comment_len);
}


/*****************************************************************\
*                                                                 *
* @return returns non zero if the label was local                 *
*                                                                 *
\*****************************************************************/
int is_local_label(const char* label)
{
    return label && *label == syntax->local_label_prefix;
}

/*****************************************************************\
*                                                                 *
* @return returns non zero if the label was local                 *
*                                                                 *
\*****************************************************************/
int is_valid_operator(const char ch)
{
    if(ch == '*' || ch == '/' || ch == '+' || ch == '-')
        return 1;

    return 0;
}

/*****************************************************************\
*                                                                 *
*   Sleeps at least @param msec milliseconds.                     *
*                                                                 *
\*****************************************************************/
void sleep_msec(ulong msec)
{
    #if defined(__unix)
    struct timespec req;
    struct timespec rem;

    req.tv_sec = msec / 1000;
    req.tv_nsec = (msec * 1000000) % 1000000000;

    while(nanosleep(&req, &rem))
    {
        req = rem;
    }
    #elif defined(WIN32)
    Sleep(milliseconds);
    #else
    #error implement your sleep function here
	#endif
}

/*****************************************************************\
*                                                                 *
*   @return current time in milliseconds.                         *
*                                                                 *
\*****************************************************************/
ulong get_time_ms()
{
    struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
