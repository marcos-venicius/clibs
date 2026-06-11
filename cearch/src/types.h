#ifndef _CEARCH_TYPES_H_
#define _CEARCH_TYPES_H_

#include "./arena.h"

#include <stdbool.h>

// the order of the fields matters
typedef enum {
    CDTK_NIL,
    CDTK_INT,
    CDTK_FLOAT,
    CDTK_STR,
    CDTK_BOOL,
    CDTK_ARRAY,
} Cearch_Data_Type_Kind;

typedef struct Cearch_Data_Type Cearch_Data_Type;

struct Cearch_Data_Type {
    Cearch_Data_Type_Kind kind;
    bool nullable;

    // only used if kind is CDTK_ARRAY
    Cearch_Data_Type *inner;
};

void cearch_printf_type(Cearch_Data_Type *type);
// This function is not thread safe because it store global state
Cearch_Data_Type *cearch_parse_data_type(Clibs_Arena *allocator, const char *function_name, const char *type_strig);
bool cearch_types_are_identical(Cearch_Data_Type *left, Cearch_Data_Type *right);

#endif // _CEARCH_TYPES_H_
