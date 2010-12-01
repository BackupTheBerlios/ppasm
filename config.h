#if defined(__x86_64) || defined(__i386) || defined(_M_IX86) || defined(_M_X64)
#define PPASM_LITTLE_ENDIAN
#elif defined(__ppc__) || defined(_M_PPC) || defined(__sparc__) || defined(__sparc) || defined(__hppa__) || defined(__hppa__)
#define PPASM_BIG_ENDIAN
#else
#error unknown arch: please define your endian
#endif

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500L
#define _XPG6

#define MAX_LABEL_SIZE 256
#define MAX_ERROR_STRING_SIZE 1024
#define MAX_INSTRUCTIONS 512

#define VERSION_MINOR 4
#define VERSION_MAJOR 0
/*
#define DO_TESTS
*/
