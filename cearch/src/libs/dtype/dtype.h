#ifndef __cearch_type_descriptor_h_
#define __cearch_type_descriptor_h_

#include <stdbool.h>

/*
    typedescritptor.h is meant to handle all things related to type description of an expression.

    It should have functions to parse literal strings into valid 'cearch_dtype_t'.

    The syntax for writing types is as follows:

        primitives:
            int
            float
            str
            bool

        nullables:
            int?
            float?
            str?
            bool?
            array<type>?

        array:
            array<int>
            array<float>
            array<str>
            array<int>

        nested arrays:
            array<array<int>>
            ...

    We do not allow nullables inside arrays.

    Since it's supposed to be a statically typed language and I don't want to handle cases like:

        [1, nil, 3].sum

        What should I do with nil? Should we break because we cannot sum nil?
        Should we consider nil as zero? If we consider nil, as zero we should have data coercing?
        Should we ignore nil? If we ignore nil, should we handle all other cases?

*/
typedef struct cearch_dtype_t cearch_dtype_t;

typedef enum {
    DTYPE_NIL,
    DTYPE_INT,
    DTYPE_FLOAT,
    DTYPE_STR,
    DTYPE_BOOL,
    DTYPE_ARRAY
} cearch_dtype_kind_t;

struct cearch_dtype_t {
    cearch_dtype_kind_t kind;

    bool nullable;

    // only used if kind is DTYPE_ARRAY
    cearch_dtype_t *inner;
};

cearch_dtype_t *cearch_dtype_parse(const char *dtype_str);

#endif // __cearch_type_descriptor_h_
