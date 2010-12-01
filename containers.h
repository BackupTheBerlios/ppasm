#ifndef CONTAINERS_H_INCLUDED
#define CONTAINERS_H_INCLUDED
#include <assert.h>
#include <stdlib.h>

#define VECTOR_DECLARE(NAME, TYPE) \
typedef struct\
{\
    TYPE*   element;\
    size_t  size;\
    size_t  capacity;\
} NAME;\
\
static inline void NAME##_init(NAME* vec, size_t cap)\
{\
    vec->size = 0;\
    vec->capacity = cap;\
    if(cap)\
        vec->element = malloc(cap * sizeof(NAME));\
}\
static inline void NAME##_fini(NAME* vec)\
{\
    free(vec->element);\
}\
\
static inline void NAME##_push_back(NAME* vec, TYPE el)\
{\
    if(vec->capacity == vec->size)\
    {\
        assert(vec->capacity);\
        vec->capacity <<= 1;\
        vec->element = realloc(vec->element, vec->capacity * sizeof(NAME));\
        assert(vec->element);\
    }\
    vec->element[vec->size++] = el;\
}\
\
static inline TYPE NAME##_pop(NAME* vec)\
{\
    if(!vec->size)\
        assert(0 && "popping back from an empty array");\
    return vec->element[--vec->size];\
}\
\
static inline int NAME##_is_empty(NAME* vec)\
{\
    return !vec->size;\
}\
\
static inline void NAME##_clear(NAME* vec)\
{\
    vec->size = 0;\
}\
static inline void NAME##_remove(NAME* vec, size_t idx)\
{\
    memmove(&vec->element[idx], &vec->element[idx + 1], sizeof(NAME) * (vec->size - idx - 1));\
    vec->size--;\
}\

#define DECLARE_FIND(NAME)\
inline static size_t NAME##_find(NAME* value, NAME* array, size_t arraysz, int(*comparator)(NAME*, NAME*))\
{\
    size_t i = 0;\
    for(; i < arraysz; i++)\
    {\
        if(!comparator(&array[i], value))\
        {\
            break;\
        }\
    }\
    return i;\
}\

//*/
#endif // CONTAINERS_H_INCLUDED
