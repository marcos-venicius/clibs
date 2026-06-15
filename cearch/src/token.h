#ifndef __cearch_token_h_
#define __cearch_token_h_

#include <stdbool.h>

#include "./libs/utils/utils.h"

typedef enum {
    // data types
    CTK_NIL = 0,
    CTK_BOOL,
    CTK_STR,
    CTK_INT,
    CTK_FLOAT,

    // operators
    CTK_LSQUARE,
    CTK_RSQUARE,
    CTK_LPAREN,
    CTK_RPAREN,
    CTK_COMMA,
    CTK_DOT,
    CTK_LT,
    CTK_GT,
    CTK_LTE,
    CTK_GTE,
    CTK_EQ,
    CTK_NEQ,
    CTK_NOT,

    // logical operators
    CTK_OR,
    CTK_AND,

    // symbols
    CTK_SYM,

    CTK_EOF
} cearch_token_kind_enum_t;

typedef struct cearch_token_t cearch_token_t;

struct cearch_token_t {
    cearch_location_t         location;
    cearch_token_kind_enum_t  kind;
    cearch_string_t           content; // actual string representation of the token

    cearch_token_t *next;

    union {
        cearch_string_t as_str;
        bool            as_bool;
        double          as_float;
        int             as_int;
    };
};

const char *cearch_token_kind_enum_name(cearch_token_kind_enum_t kind);

#endif // __cearch_token_h_
